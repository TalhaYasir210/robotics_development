#include "cv_bridge/cv_bridge.hpp"
#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "sensor_msgs/msg/camera_info.hpp"
#include "opencv2/opencv.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "swarm_interfaces/srv/get_qr.hpp"
#include "swarm_interfaces/srv/save_qr.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <chrono>
#include <unordered_map>
#include <zbar.h>

class QrTfProjector : public rclcpp::Node {
public:
  QrTfProjector() : Node("qr_tf_projector") {
    // Declare and get the role parameter ('mapper' or 'explorer')
    this->declare_parameter<std::string>("robot_role", "mapper");
    this->declare_parameter<double>("qr_size", 0.2);
    
    robot_role_ = this->get_parameter("robot_role").as_string();
    qr_real_width_ = this->get_parameter("qr_size").as_double();

    // Setup TF2 Listener for both roles
    tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_, this, false); // Inherit remappings!

    // Subscribe to camera topics
    cam_info_sub_ = this->create_subscription<sensor_msgs::msg::CameraInfo>(
        "camera/camera_info", rclcpp::SensorDataQoS(),
        std::bind(&QrTfProjector::cam_info_callback, this, std::placeholders::_1));

    image_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
        "camera/image_raw", rclcpp::SensorDataQoS(),
        std::bind(&QrTfProjector::image_callback, this, std::placeholders::_1));

    if (robot_role_ == "mapper") {
      // Mapper Needs: SaveQR Client
      save_qr_client_ =
          this->create_client<swarm_interfaces::srv::SaveQR>("/save_qr");
      RCLCPP_INFO(this->get_logger(),
                  "Started in MAPPER mode. Will save QRs to database.");
    } else if (robot_role_ == "explorer") {
      // Explorer Needs: GetQR Client and InitialPose Publisher
      get_qr_client_ =
          this->create_client<swarm_interfaces::srv::GetQR>("/get_qr");
      initial_pose_pub_ =
          this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
              "initialpose", 10);
      RCLCPP_INFO(this->get_logger(),
                  "Started in EXPLORER mode. Will query database to localize using inverse kinematics.");
    } else {
      RCLCPP_ERROR(this->get_logger(),
                   "Invalid robot_role. Must be 'mapper' or 'explorer'.");
    }
  }

private:
  std::string robot_role_;
  double qr_real_width_;
  std::unordered_map<std::string, rclcpp::Time> qr_cooldown_map_;

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Subscription<sensor_msgs::msg::CameraInfo>::SharedPtr cam_info_sub_;
  sensor_msgs::msg::CameraInfo::SharedPtr camera_info_;

  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
  
  rclcpp::Client<swarm_interfaces::srv::SaveQR>::SharedPtr save_qr_client_;
  rclcpp::Client<swarm_interfaces::srv::GetQR>::SharedPtr get_qr_client_;
  rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr initial_pose_pub_;

  void cam_info_callback(const sensor_msgs::msg::CameraInfo::SharedPtr msg) {
    if (!camera_info_) camera_info_ = msg;
  }

  void image_callback(const sensor_msgs::msg::Image::SharedPtr msg) {
    if (!camera_info_) return;

    try {
      cv_bridge::CvImagePtr cv_ptr =
          cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
          
      cv::Mat gray;
      cv::cvtColor(cv_ptr->image, gray, cv::COLOR_BGR2GRAY);

      zbar::ImageScanner scanner;
      scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);
      zbar::Image zbar_image(gray.cols, gray.rows, "Y800", gray.ptr(), gray.cols * gray.rows);
      int n = scanner.scan(zbar_image);

      if (n > 0) {
        for (zbar::Image::SymbolIterator symbol = zbar_image.symbol_begin();
             symbol != zbar_image.symbol_end(); ++symbol) {
             
          std::string qr_data = symbol->get_data();
          auto now = this->now();

          // Check if we queried this QR within the last 5 seconds
          if (qr_cooldown_map_.find(qr_data) != qr_cooldown_map_.end()) {
            double elapsed = (now - qr_cooldown_map_[qr_data]).seconds();
            if (elapsed < 5.0) {
              continue; // Cooldown active, skip processing
            }
          }

          // Update timestamp
          qr_cooldown_map_[qr_data] = now;
          RCLCPP_INFO(this->get_logger(), "Scanned QR: '%s'", qr_data.c_str());

          if (robot_role_ == "mapper") {
            handle_mapper_logic(qr_data, msg->header.frame_id);
          } else if (robot_role_ == "explorer") {
            // EXPLORER: Use SolvePnP to get rough Transform(Camera -> QR)
            if (symbol->get_location_size() >= 4) {
              std::vector<cv::Point2f> image_points;
              for (int i = 0; i < 4; ++i) {
                image_points.push_back(cv::Point2f(symbol->get_location_x(i), symbol->get_location_y(i)));
              }
              
              double hw = qr_real_width_ / 2.0;
              std::vector<cv::Point3f> object_points = {
                  cv::Point3f(-hw, hw, 0),   
                  cv::Point3f(-hw, -hw, 0),  
                  cv::Point3f(hw, -hw, 0),   
                  cv::Point3f(hw, hw, 0)     
              };
              
              cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) << 
                  camera_info_->k[0], camera_info_->k[1], camera_info_->k[2],
                  camera_info_->k[3], camera_info_->k[4], camera_info_->k[5],
                  camera_info_->k[6], camera_info_->k[7], camera_info_->k[8]);
              cv::Mat dist_coeffs = cv::Mat::zeros(4, 1, CV_64F); 
              
              cv::Mat rvec, tvec;
              cv::solvePnP(object_points, image_points, camera_matrix, dist_coeffs, rvec, tvec);
              
              cv::Mat R;
              cv::Rodrigues(rvec, R);
              tf2::Matrix3x3 tf2_R(
                  R.at<double>(0,0), R.at<double>(0,1), R.at<double>(0,2),
                  R.at<double>(1,0), R.at<double>(1,1), R.at<double>(1,2),
                  R.at<double>(2,0), R.at<double>(2,1), R.at<double>(2,2)
              );
              tf2::Quaternion tf2_q;
              tf2_R.getRotation(tf2_q);
              
              tf2::Transform tf_cam_to_qr;
              tf_cam_to_qr.setOrigin(tf2::Vector3(tvec.at<double>(0), tvec.at<double>(1), tvec.at<double>(2)));
              tf_cam_to_qr.setRotation(tf2_q);
              
              // Proceed with Inverse Kinematics
              handle_explorer_logic(qr_data, tf_cam_to_qr, msg->header.frame_id);
            }
          }
        }
      }
    } catch (cv_bridge::Exception &e) {
      RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
    }
  }

  void handle_mapper_logic(const std::string &qr_id, const std::string &camera_frame) {
    try {
      geometry_msgs::msg::TransformStamped transform;
      transform = tf_buffer_->lookupTransform("map", camera_frame, tf2::TimePointZero);

      auto request = std::make_shared<swarm_interfaces::srv::SaveQR::Request>();
      request->qr_id = qr_id;
      request->pose.position.x = transform.transform.translation.x;
      request->pose.position.y = transform.transform.translation.y;
      request->pose.position.z = transform.transform.translation.z;

      if (!save_qr_client_->wait_for_service(std::chrono::seconds(1))) {
        RCLCPP_WARN(this->get_logger(), "Database service not available, skipping save.");
        return;
      }

      RCLCPP_INFO(this->get_logger(), "Sending '%s' to database...", qr_id.c_str());

      save_qr_client_->async_send_request(
          request,
          [this, qr_id](rclcpp::Client<swarm_interfaces::srv::SaveQR>::SharedFuture future) {
            if (future.get()->success) {
              RCLCPP_INFO(this->get_logger(), "Database confirmed save for '%s'.", qr_id.c_str());
            }
          });
    } catch (tf2::TransformException &ex) {
      RCLCPP_WARN(this->get_logger(), "TF lookup failed, cannot save QR: %s", ex.what());
    }
  }

  void handle_explorer_logic(const std::string &qr_id, const tf2::Transform& tf_cam_to_qr, const std::string& camera_frame) {
    auto request = std::make_shared<swarm_interfaces::srv::GetQR::Request>();
    request->qr_id = qr_id;

    if (!get_qr_client_->wait_for_service(std::chrono::seconds(1))) {
      RCLCPP_WARN(this->get_logger(), "Database service not available, cannot localize.");
      return;
    }

    RCLCPP_INFO(this->get_logger(), "Asking database for coordinates of '%s'...", qr_id.c_str());

    get_qr_client_->async_send_request(
        request,
        [this, qr_id, tf_cam_to_qr, camera_frame](rclcpp::Client<swarm_interfaces::srv::GetQR>::SharedFuture future) {
          auto response = future.get();
          if (response->success) {
            RCLCPP_INFO(this->get_logger(), "Database found QR! Map position: X: %.2f, Y: %.2f", response->pose.position.x, response->pose.position.y);

            try {
              // 1. Get Base -> Camera transform
              std::string base_frame = "tb3_2/base_footprint"; 
              geometry_msgs::msg::TransformStamped tf_base_to_cam_msg = tf_buffer_->lookupTransform(base_frame, camera_frame, tf2::TimePointZero);
              
              tf2::Transform tf_base_to_cam;
              tf2::fromMsg(tf_base_to_cam_msg.transform, tf_base_to_cam);
              
              // 2. Calculate Base -> QR
              tf2::Transform tf_base_to_qr = tf_base_to_cam * tf_cam_to_qr;
              
              // 3. Get Map -> QR from database response
              tf2::Transform tf_map_to_qr;
              tf2::fromMsg(response->pose, tf_map_to_qr);
              
              // 4. Inverse Kinematics: Map -> Base = (Map -> QR) * (Base -> QR)^-1
              tf2::Transform tf_map_to_base = tf_map_to_qr * tf_base_to_qr.inverse();
              
              geometry_msgs::msg::Pose msg_map_to_base;
              tf2::toMsg(tf_map_to_base, msg_map_to_base);

              // 5. Publish to AMCL
              geometry_msgs::msg::PoseWithCovarianceStamped init_pose;
              init_pose.header.frame_id = "map";
              init_pose.header.stamp = this->now();
              init_pose.pose.pose = msg_map_to_base;
              // Provide a generous covariance matrix to let AMCL's particle filter figure out the rest
              init_pose.pose.covariance[0] = 0.25;
              init_pose.pose.covariance[7] = 0.25;
              init_pose.pose.covariance[35] = 0.5;

              initial_pose_pub_->publish(init_pose);
              RCLCPP_INFO(this->get_logger(), ">>> SUCCESSFULLY LOCALIZED ROBOT BASE <<<");
              RCLCPP_INFO(this->get_logger(), "Robot Map Position: X: %.2f, Y: %.2f", msg_map_to_base.position.x, msg_map_to_base.position.y);
            } catch (tf2::TransformException &ex) {
              RCLCPP_WARN(this->get_logger(), "TF lookup failed during inverse kinematics: %s", ex.what());
            }
          } else {
            RCLCPP_WARN(this->get_logger(), "Database does not know '%s' yet.", qr_id.c_str());
          }
        });
  }
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<QrTfProjector>());
  rclcpp::shutdown();
  return 0;
}
