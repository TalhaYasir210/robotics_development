#include "cv_bridge/cv_bridge.hpp"
#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "opencv2/opencv.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "swarm_interfaces/srv/get_qr.hpp"
#include "swarm_interfaces/srv/save_qr.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include <chrono>
#include <unordered_map>

class QrTfProjector : public rclcpp::Node {
public:
  QrTfProjector() : Node("qr_tf_projector") {
    // Declare and get the role parameter ('mapper' or 'explorer')
    this->declare_parameter<std::string>("robot_role", "mapper");
    robot_role_ = this->get_parameter("robot_role").as_string();

    // 1. Subscribe to the raw camera feed
    image_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
        "camera/image_raw", 10,
        std::bind(&QrTfProjector::image_callback, this, std::placeholders::_1));

    if (robot_role_ == "mapper") {
      // Mapper Needs: TF2 Listener and SaveQR Client
      tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
      tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);
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
                  "Started in EXPLORER mode. Will query database to localize.");
    } else {
      RCLCPP_ERROR(this->get_logger(),
                   "Invalid robot_role. Must be 'mapper' or 'explorer'.");
    }
  }

private:
  std::string robot_role_;
  std::unordered_map<std::string, rclcpp::Time>
      qr_cooldown_map_; // Prevent spamming the database

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;

  // Mapper specific
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
  rclcpp::Client<swarm_interfaces::srv::SaveQR>::SharedPtr save_qr_client_;

  // Explorer specific
  rclcpp::Client<swarm_interfaces::srv::GetQR>::SharedPtr get_qr_client_;
  rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr
      initial_pose_pub_;

  void image_callback(const sensor_msgs::msg::Image::SharedPtr msg) {
    try {
      cv_bridge::CvImagePtr cv_ptr =
          cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
      cv::Mat frame = cv_ptr->image;

      cv::QRCodeDetector qr_detector;
      std::string qr_data = qr_detector.detectAndDecode(frame);

      if (!qr_data.empty()) {
        auto now = this->now();

        // Check if we queried this QR within the last 5 seconds
        if (qr_cooldown_map_.find(qr_data) != qr_cooldown_map_.end()) {
          double elapsed = (now - qr_cooldown_map_[qr_data]).seconds();
          if (elapsed < 5.0) {
            return; // Cooldown active, skip processing
          }
        }

        // Update timestamp
        qr_cooldown_map_[qr_data] = now;
        RCLCPP_INFO(this->get_logger(), "Scanned QR: '%s'", qr_data.c_str());

        if (robot_role_ == "mapper") {
          handle_mapper_logic(qr_data, msg->header.frame_id);
        } else if (robot_role_ == "explorer") {
          handle_explorer_logic(qr_data);
        }
      }
    } catch (cv_bridge::Exception &e) {
      RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
    }
  }

  void handle_mapper_logic(const std::string &qr_id,
                           const std::string &camera_frame) {
    try {
      geometry_msgs::msg::TransformStamped transform;
      transform =
          tf_buffer_->lookupTransform("map", camera_frame, tf2::TimePointZero);

      // Build the async request
      auto request = std::make_shared<swarm_interfaces::srv::SaveQR::Request>();
      request->qr_id = qr_id;
      request->pose.position.x = transform.transform.translation.x;
      request->pose.position.y = transform.transform.translation.y;
      request->pose.position.z = transform.transform.translation.z;

      // Wait for the service to be available
      if (!save_qr_client_->wait_for_service(std::chrono::seconds(1))) {
        RCLCPP_WARN(this->get_logger(),
                    "Database service not available, skipping save.");
        return;
      }

      RCLCPP_INFO(this->get_logger(), "Sending '%s' to database...",
                  qr_id.c_str());



      // Send request asynchronously so it doesn't block the camera feed
      save_qr_client_->async_send_request(
          request,
          [this,
           qr_id](rclcpp::Client<swarm_interfaces::srv::SaveQR>::SharedFuture
                      future) {
            if (future.get()->success) {
              RCLCPP_INFO(this->get_logger(),
                          "Database confirmed save for '%s'.", qr_id.c_str());
            }
          });
    } catch (tf2::TransformException &ex) {
      RCLCPP_WARN(this->get_logger(), "TF lookup failed, cannot save QR: %s",
                  ex.what());
    }
  }

  void handle_explorer_logic(const std::string &qr_id) {
    auto request = std::make_shared<swarm_interfaces::srv::GetQR::Request>();
    request->qr_id = qr_id;

    if (!get_qr_client_->wait_for_service(std::chrono::seconds(1))) {
      RCLCPP_WARN(this->get_logger(),
                  "Database service not available, cannot localize.");
      return;
    }

    RCLCPP_INFO(this->get_logger(),
                "Asking database for coordinates of '%s'...", qr_id.c_str());



    get_qr_client_->async_send_request(
        request,
        [this, qr_id](
            rclcpp::Client<swarm_interfaces::srv::GetQR>::SharedFuture future) {
          auto response = future.get();
          if (response->success) {
            RCLCPP_INFO(this->get_logger(),
                        "Received coordinates! X: %.2f, Y: %.2f",
                        response->pose.position.x, response->pose.position.y);

            // Publish to AMCL to solve the kidnapped robot problem
            geometry_msgs::msg::PoseWithCovarianceStamped init_pose;
            init_pose.header.frame_id = "map";
            init_pose.header.stamp = this->now();
            init_pose.pose.pose = response->pose;

            initial_pose_pub_->publish(init_pose);
            RCLCPP_INFO(
                this->get_logger(),
                "Published to /initialpose. Kidnapped robot localized!");
          } else {
            RCLCPP_WARN(this->get_logger(), "Database does not know '%s' yet.",
                        qr_id.c_str());
            // DO NOT ERASE ANYTHING HERE. 
            // Let the 5-second cooldown in image_callback do its job.
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
