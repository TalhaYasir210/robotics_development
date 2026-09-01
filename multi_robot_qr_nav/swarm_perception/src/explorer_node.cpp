#include "cv_bridge/cv_bridge.hpp"
#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "nav2_msgs/srv/manage_lifecycle_nodes.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "opencv2/opencv.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/camera_info.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "swarm_interfaces/srv/get_map.hpp"
#include "swarm_interfaces/srv/get_qr.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include <chrono>
#include <cmath>
#include <set>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <zbar.h>

using namespace std::chrono_literals;

enum class State {
  SEARCHING,
  CHECKING_OBSTACLE,
  YIELDING,
  SERVOING,
  RECOVERY_SWEEP,
  PERIODIC_SWEEP,
  BLIND_APPROACH,
  LOCALIZING,
  TURN_AWAY,
  DONE
};

class ExplorerNode : public rclcpp::Node {
public:
  ExplorerNode() : Node("bot2_explorer") {
    this->declare_parameter<std::string>("robot_name", "tb3_2");
    this->declare_parameter<std::string>("global_frame", "map");
    this->declare_parameter<std::string>("camera_frame", "");
    this->declare_parameter<double>("target_distance", 0.50);
    this->declare_parameter<double>("qr_size", 0.28);

    robot_name_ = this->get_parameter("robot_name").as_string();
    global_frame_ = this->get_parameter("global_frame").as_string();
    camera_frame_ = this->get_parameter("camera_frame").as_string();
    if (camera_frame_.empty()) {
      camera_frame_ = robot_name_ + "/camera_rgb_optical_frame";
    }
    target_distance_ = this->get_parameter("target_distance").as_double();
    qr_real_width_ = this->get_parameter("qr_size").as_double();

    tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
    tf_buffer_->setUsingDedicatedThread(true);
    tf_listener_ =
        std::make_shared<tf2_ros::TransformListener>(*tf_buffer_, this, false);

    std::string cam_info_topic = "/" + robot_name_ + "/camera/camera_info";
    std::string image_topic = "/" + robot_name_ + "/camera/image_raw";
    std::string scan_topic = "/" + robot_name_ + "/scan";
    std::string cmd_vel_topic = "/" + robot_name_ + "/cmd_vel";
    std::string initial_pose_topic = "/" + robot_name_ + "/initialpose";
    std::string map_topic = "/" + robot_name_ + "/map";

    cam_info_sub_ = this->create_subscription<sensor_msgs::msg::CameraInfo>(
        cam_info_topic, rclcpp::SensorDataQoS(),
        std::bind(&ExplorerNode::camInfoCallback, this, std::placeholders::_1));

    image_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
        image_topic, rclcpp::SensorDataQoS(),
        std::bind(&ExplorerNode::imageCallback, this, std::placeholders::_1));

    scan_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
        scan_topic, rclcpp::SensorDataQoS(),
        std::bind(&ExplorerNode::scanCallback, this, std::placeholders::_1));

    cmd_vel_pub_ =
        this->create_publisher<geometry_msgs::msg::Twist>(cmd_vel_topic, 10);
    initial_pose_pub_ =
        this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
            initial_pose_topic, 10);
    map_pub_ = this->create_publisher<nav_msgs::msg::OccupancyGrid>(
        map_topic, rclcpp::QoS(10).transient_local());

    get_qr_client_ =
        this->create_client<swarm_interfaces::srv::GetQR>("/get_qr");
    get_map_client_ =
        this->create_client<swarm_interfaces::srv::GetMap>("/get_map");
    manage_loc_client_ =
        this->create_client<nav2_msgs::srv::ManageLifecycleNodes>(
            "/tb3_2/lifecycle_manager_localization/manage_nodes");
    manage_nav_client_ =
        this->create_client<nav2_msgs::srv::ManageLifecycleNodes>(
            "/tb3_2/lifecycle_manager_navigation/manage_nodes");

    control_timer_ = this->create_wall_timer(
        100ms, std::bind(&ExplorerNode::controlLoop, this));

    last_sweep_end_time_ = this->now();

    RCLCPP_INFO(this->get_logger(),
                "Bot 2 Explorer initialized. Blind Wandering started!");
  }

private:
  std::string robot_name_;
  std::string global_frame_;
  std::string camera_frame_;
  double target_distance_;
  double qr_real_width_;

  State state_ = State::SEARCHING;
  bool obstacle_detected_ = false;

  double dist_at_check_ = 0.0;
  rclcpp::Time check_start_time_;
  double closest_obstacle_dist_ = 999.0;
  double closest_obstacle_angle_ = 0.0;
  double left_clearance_ = 999.0;
  double right_clearance_ = 999.0;
  double front_60_dist_ = 999.0;
  double ignore_checking_until_dist_ = -1.0;

  std::string target_qr_payload_;
  tf2::Transform latest_tf_cam_to_qr_;
  double last_Z_ = 999.0;
  rclcpp::Time last_seen_time_;
  rclcpp::Time recovery_start_time_;
  rclcpp::Time periodic_sweep_start_time_;
  rclcpp::Time last_sweep_end_time_;
  rclcpp::Time blind_approach_start_time_;
  rclcpp::Time turn_away_start_time_;
  double blind_approach_duration_ = 0.0;
  std::set<std::string> ignored_qrs_;

  rclcpp::Subscription<sensor_msgs::msg::CameraInfo>::SharedPtr cam_info_sub_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr scan_sub_;
  sensor_msgs::msg::CameraInfo::SharedPtr camera_info_;

  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
  rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr
      initial_pose_pub_;
  rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr map_pub_;

  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

  rclcpp::Client<swarm_interfaces::srv::GetQR>::SharedPtr get_qr_client_;
  rclcpp::Client<swarm_interfaces::srv::GetMap>::SharedPtr get_map_client_;
  rclcpp::Client<nav2_msgs::srv::ManageLifecycleNodes>::SharedPtr
      manage_loc_client_;
  rclcpp::Client<nav2_msgs::srv::ManageLifecycleNodes>::SharedPtr
      manage_nav_client_;
  rclcpp::TimerBase::SharedPtr control_timer_;
  rclcpp::TimerBase::SharedPtr map_update_timer_;

  void camInfoCallback(const sensor_msgs::msg::CameraInfo::SharedPtr msg) {
    if (!camera_info_) {
      camera_info_ = msg;
    }
  }

  void scanCallback(const sensor_msgs::msg::LaserScan::SharedPtr msg) {
    if (state_ == State::LOCALIZING || state_ == State::DONE)
      return;

    bool local_obstacle = false;
    int num_ranges = msg->ranges.size();
    if (num_ranges == 0)
      return;

    double angle_increment = msg->angle_increment;
    double angle_min = msg->angle_min;

    double min_dist = 999.0;
    double min_angle = 0.0;
    double min_left = 999.0;
    double min_right = 999.0;
    double min_front_60 = 999.0;

    for (int i = 0; i < num_ranges; ++i) {
      double range = msg->ranges[i];
      if (std::isnan(range) || std::isinf(range))
        continue;

      double angle = angle_min + i * angle_increment;
      while (angle < 0)
        angle += 2 * M_PI;
      while (angle >= 2 * M_PI)
        angle -= 2 * M_PI;

      double angle_deg = angle * 180.0 / M_PI;

      if (range < min_dist) {
        min_dist = range;
        min_angle = angle_deg;
      }

      // Left vs Right clearance for smart wandering
      if (angle_deg > 0.0 && angle_deg <= 90.0) {
        if (range < min_left)
          min_left = range;
      } else if (angle_deg >= 270.0 && angle_deg < 360.0) {
        if (range < min_right)
          min_right = range;
      }

      // Front 60 (±30) for Bot 1 dynamic detection
      if (angle_deg <= 30.0 || angle_deg >= 330.0) {
        if (range < min_front_60)
          min_front_60 = range;
      }

      // Front cone for wandering (increased detection range and angle)
      if (state_ == State::SEARCHING || state_ == State::CHECKING_OBSTACLE ||
          state_ == State::YIELDING) {
        if ((angle_deg <= 90.0 || angle_deg >= 270.0) && range < 1.20) {
          local_obstacle = true;
        }
      }
      // Narrower cone for servoing (0.3m)
      else if (state_ == State::SERVOING || state_ == State::RECOVERY_SWEEP ||
               state_ == State::PERIODIC_SWEEP) {
        if ((angle_deg <= 30.0 || angle_deg >= 330.0) && range < 0.20) {
          local_obstacle = true;
        }
      }
    }
    closest_obstacle_dist_ = min_dist;
    closest_obstacle_angle_ = min_angle;
    left_clearance_ = min_left;
    right_clearance_ = min_right;
    front_60_dist_ = min_front_60;
    obstacle_detected_ = local_obstacle;
  }

  void controlLoop() {
    if (state_ == State::SEARCHING) {
      if ((this->now() - last_sweep_end_time_).seconds() > 30.0) {
        RCLCPP_INFO(this->get_logger(),
                    "[Wanderer] 30 seconds passed without a QR code. "
                    "Initiating 360-degree Periodic Sweep!");
        state_ = State::PERIODIC_SWEEP;
        periodic_sweep_start_time_ = this->now();
        geometry_msgs::msg::Twist twist;
        cmd_vel_pub_->publish(twist); // Stop
        return;
      }

      if (front_60_dist_ < 1.5 && front_60_dist_ > 0.2) {
        if (ignore_checking_until_dist_ < 0 ||
            front_60_dist_ > ignore_checking_until_dist_ + 0.3) {
          state_ = State::CHECKING_OBSTACLE;
          check_start_time_ = this->now();
          dist_at_check_ = front_60_dist_;
          geometry_msgs::msg::Twist twist;
          cmd_vel_pub_->publish(twist); // Stop
          RCLCPP_INFO(this->get_logger(),
                      "[Smart Yield] Obstacle in front 60deg at %.2fm. "
                      "Checking if dynamic...",
                      front_60_dist_);
          return;
        }
      } else if (front_60_dist_ > 1.5) {
        ignore_checking_until_dist_ = -1.0;
      }

      geometry_msgs::msg::Twist twist;
      if (obstacle_detected_) {
        if (left_clearance_ > right_clearance_) {
          twist.angular.z = 0.5; // More space on left
        } else {
          twist.angular.z = -0.5; // More space on right
        }
      } else {
        twist.linear.x = 0.2; // Drive forward
        twist.angular.z =
            0.2 *
            std::sin(
                this->now().seconds()); // Weave slightly to break out of rooms
      }
      cmd_vel_pub_->publish(twist);
    } else if (state_ == State::CHECKING_OBSTACLE) {
      geometry_msgs::msg::Twist twist;
      cmd_vel_pub_->publish(twist); // Stay stopped

      double elapsed = (this->now() - check_start_time_).seconds();
      if (elapsed > 0.5) {
        if (front_60_dist_ < dist_at_check_ - 0.05) {
          RCLCPP_WARN(this->get_logger(),
                      "[Smart Yield] Obstacle is MOVING towards us (%.2fm -> "
                      "%.2fm). Yielding to Bot 1!",
                      dist_at_check_, front_60_dist_);
          state_ = State::YIELDING;
        } else {
          RCLCPP_INFO(this->get_logger(),
                      "[Smart Yield] Obstacle is static. Resuming search.");
          state_ = State::SEARCHING;
          ignore_checking_until_dist_ = front_60_dist_;
        }
      }
    } else if (state_ == State::YIELDING) {
      geometry_msgs::msg::Twist twist;

      if (front_60_dist_ < 1.0) {
        twist.linear.x = -0.15; // Reverse
        RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000,
                             "[Smart Yield] Bot 1 is close in front (%.2fm)! "
                             "Reversing to make space...",
                             front_60_dist_);
      } else {
        twist.linear.x = 0.0; // Stay stopped
      }
      cmd_vel_pub_->publish(twist);

      if (front_60_dist_ > 1.5 && !obstacle_detected_) {
        RCLCPP_INFO(this->get_logger(),
                    "[Smart Yield] Path is clear. Resuming search!");
        state_ = State::SEARCHING;
        ignore_checking_until_dist_ = -1.0;
      }
    } else if (state_ == State::SERVOING) {
      if (obstacle_detected_) {
        RCLCPP_WARN(
            this->get_logger(),
            "[Servoing] Obstacle too close! Aborting to prevent crash.");
        geometry_msgs::msg::Twist twist;
        cmd_vel_pub_->publish(twist);
        state_ = State::SEARCHING;
        return;
      }

      double time_since_last_seen = (this->now() - last_seen_time_).seconds();
      if (time_since_last_seen > 0.3 && time_since_last_seen <= 2.5) {
        geometry_msgs::msg::Twist twist;
        cmd_vel_pub_->publish(twist); // Brake
      } else if (time_since_last_seen > 2.5) {
        if (last_Z_ < 0.70) {
          RCLCPP_INFO(this->get_logger(),
                      "[Servoing] Close to QR code at %.2fm. SUCCESS!",
                      last_Z_);
          startLocalization();
        } else if (last_Z_ <= 1.0) {
          RCLCPP_WARN(this->get_logger(),
                      "[Servoing] Lost sight at %.2fm. Blindly driving forward "
                      "to reach 0.5m...",
                      last_Z_);
          state_ = State::BLIND_APPROACH;
          blind_approach_start_time_ = this->now();
          double distance_to_cover = last_Z_ - 0.50;
          if (distance_to_cover < 0.0)
            distance_to_cover = 0.0;
          blind_approach_duration_ = distance_to_cover / 0.15;
        } else {
          RCLCPP_WARN(this->get_logger(),
                      "[Servoing] Lost sight at far range (%.2fm). Initiating "
                      "Recovery Sweep...",
                      last_Z_);
          state_ = State::RECOVERY_SWEEP;
          recovery_start_time_ = this->now();
        }
      }
    } else if (state_ == State::RECOVERY_SWEEP) {
      double elapsed = (this->now() - recovery_start_time_).seconds();
      geometry_msgs::msg::Twist twist;
      twist.linear.x = 0.0;
      if (elapsed < 42.0) {
        twist.angular.z = 0.15; // Spin slowly to avoid motion blur
      } else {
        twist.angular.z = 0.0;
        RCLCPP_WARN(
            this->get_logger(),
            "[Recovery] 360 Sweep finished. QR code not found. Aborting.");
        state_ = State::SEARCHING;
        last_sweep_end_time_ = this->now();
      }
      cmd_vel_pub_->publish(twist);
    } else if (state_ == State::PERIODIC_SWEEP) {
      double elapsed = (this->now() - periodic_sweep_start_time_).seconds();
      geometry_msgs::msg::Twist twist;
      twist.linear.x = 0.0;
      if (elapsed < 42.0) {
        twist.angular.z = 0.15; // Spin slowly to avoid motion blur
      } else {
        twist.angular.z = 0.0;
        RCLCPP_INFO(this->get_logger(),
                    "[Wanderer] Periodic Sweep finished. Resuming wandering.");
        state_ = State::SEARCHING;
        last_sweep_end_time_ = this->now();
      }
      cmd_vel_pub_->publish(twist);
    } else if (state_ == State::BLIND_APPROACH) {
      double elapsed = (this->now() - blind_approach_start_time_).seconds();
      if (elapsed < blind_approach_duration_) {
        geometry_msgs::msg::Twist twist;
        twist.linear.x = 0.15; // Drive forward
        twist.angular.z = 0.0;
        cmd_vel_pub_->publish(twist);
      } else {
        geometry_msgs::msg::Twist stop_twist;
        cmd_vel_pub_->publish(stop_twist);
        last_Z_ = 0.50; // Update estimated distance
        RCLCPP_INFO(
            this->get_logger(),
            "[Blind Approach] Reached estimated 0.5m distance. SUCCESS!");
        startLocalization();
      }
    } else if (state_ == State::TURN_AWAY) {
      double elapsed = (this->now() - turn_away_start_time_).seconds();
      geometry_msgs::msg::Twist twist;
      if (elapsed < 6.0) {
        twist.angular.z = 0.4; // Slower turn away
        twist.linear.x = 0.0;
      } else {
        twist.angular.z = 0.0;
        state_ = State::SEARCHING;
      }
      cmd_vel_pub_->publish(twist);
    }
  }

  void imageCallback(const sensor_msgs::msg::Image::SharedPtr msg) {
    if (!camera_info_ || state_ == State::LOCALIZING || state_ == State::DONE)
      return;

    try {
      cv_bridge::CvImagePtr cv_ptr =
          cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
      cv::Mat gray;
      cv::cvtColor(cv_ptr->image, gray, cv::COLOR_BGR2GRAY);

      zbar::ImageScanner scanner;
      scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);
      zbar::Image zbar_image(gray.cols, gray.rows, "Y800", gray.ptr(),
                             gray.cols * gray.rows);
      int n = scanner.scan(zbar_image);

      if (n > 0) {
        last_sweep_end_time_ = this->now();
        for (zbar::Image::SymbolIterator symbol = zbar_image.symbol_begin();
             symbol != zbar_image.symbol_end(); ++symbol) {
          std::string payload = symbol->get_data();

          if (ignored_qrs_.find(payload) != ignored_qrs_.end()) {
            continue;
          }

          double center_x_check = 0;
          for (int i = 0; i < 4; ++i) {
            center_x_check += symbol->get_location_x(i);
          }
          center_x_check /= 4.0;
          if (std::abs(camera_info_->k[2] - center_x_check) > 400.0) {
            continue;
          }

          if (state_ == State::SEARCHING ||
              state_ == State::CHECKING_OBSTACLE || state_ == State::YIELDING ||
              state_ == State::PERIODIC_SWEEP) {
            RCLCPP_INFO(this->get_logger(),
                        "\n=========================================");
            RCLCPP_INFO(this->get_logger(), ">>> QR DETECTED: %s",
                        payload.c_str());
            RCLCPP_INFO(
                this->get_logger(),
                ">>> Preempting Wanderer and beginning Visual Servoing!");
            target_qr_payload_ = payload;
            state_ = State::SERVOING;
          }

          if ((state_ == State::SERVOING || state_ == State::RECOVERY_SWEEP) &&
              payload == target_qr_payload_) {
            if (state_ == State::RECOVERY_SWEEP) {
              RCLCPP_INFO(this->get_logger(),
                          "[Recovery] QR code reacquired! Resuming tracking.");
              state_ = State::SERVOING;
            }
            last_seen_time_ = this->now();

            std::vector<cv::Point2f> image_points;
            for (int i = 0; i < 4; ++i) {
              image_points.push_back(cv::Point2f(symbol->get_location_x(i),
                                                 symbol->get_location_y(i)));
            }

            double side1 = cv::norm(image_points[0] - image_points[1]);
            double side2 = cv::norm(image_points[1] - image_points[2]);
            double side3 = cv::norm(image_points[2] - image_points[3]);
            double side4 = cv::norm(image_points[3] - image_points[0]);
            double pixel_width = (side1 + side2 + side3 + side4) / 4.0;

            double fx = camera_info_->k[0];
            double fy = camera_info_->k[4];
            double cx = camera_info_->k[2];
            double cy = camera_info_->k[5];

            double Z = (fx * qr_real_width_) / pixel_width;
            last_Z_ = Z;
            double center_x = (image_points[0].x + image_points[1].x +
                               image_points[2].x + image_points[3].x) /
                              4.0;

            // solvePnP for pristine Orientation
            double hw = qr_real_width_ / 2.0;
            std::vector<cv::Point3f> object_points = {
                cv::Point3f(-hw, hw, 0), cv::Point3f(-hw, -hw, 0),
                cv::Point3f(hw, -hw, 0), cv::Point3f(hw, hw, 0)};
            cv::Mat camera_matrix =
                (cv::Mat_<double>(3, 3) << fx, 0, cx, 0, fy, cy, 0, 0, 1);
            cv::Mat dist_coeffs = cv::Mat::zeros(4, 1, CV_64F);
            cv::Mat rvec, tvec;
            cv::solvePnP(object_points, image_points, camera_matrix,
                         dist_coeffs, rvec, tvec);

            cv::Mat R;
            cv::Rodrigues(rvec, R);
            tf2::Matrix3x3 tf2_R(
                R.at<double>(0, 0), R.at<double>(0, 1), R.at<double>(0, 2),
                R.at<double>(1, 0), R.at<double>(1, 1), R.at<double>(1, 2),
                R.at<double>(2, 0), R.at<double>(2, 1), R.at<double>(2, 2));
            tf2::Quaternion tf2_q;
            tf2_R.getRotation(tf2_q);

            // Save the transform for Inverse Kinematics later
            latest_tf_cam_to_qr_.setOrigin(tf2::Vector3(
                tvec.at<double>(0), tvec.at<double>(1), tvec.at<double>(2)));
            latest_tf_cam_to_qr_.setRotation(tf2_q);

            double error_x = cx - center_x;
            geometry_msgs::msg::Twist twist;

            if (Z > target_distance_) {
              double ang_z = 0.0015 * error_x;
              if (ang_z > 0.3)
                ang_z = 0.3;
              if (ang_z < -0.3)
                ang_z = -0.3;
              double dynamic_threshold = cx * 0.3;

              if (std::abs(error_x) > dynamic_threshold) {
                twist.linear.x = 0.05;
                twist.angular.z = ang_z;
                RCLCPP_INFO(
                    this->get_logger(),
                    "[Servoing] Aligning angle first (Error: %.1f pixels)...",
                    error_x);
              } else {
                twist.linear.x = 0.15;
                twist.angular.z = ang_z;
                RCLCPP_INFO(
                    this->get_logger(),
                    "[Servoing] Aligned! Distance: %.2fm | Driving forward...",
                    Z);
              }
              cmd_vel_pub_->publish(twist);
            } else {
              RCLCPP_INFO(this->get_logger(),
                          "[Servoing] Reached wall threshold (%.2fm)!",
                          target_distance_);
              startLocalization();
            }
          }
        }
      }
      zbar_image.set_data(NULL, 0);
    } catch (cv_bridge::Exception &e) {
      RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
    }
  }

  void startLocalization() {
    state_ = State::LOCALIZING;
    geometry_msgs::msg::Twist stop_twist;
    cmd_vel_pub_->publish(stop_twist);

    RCLCPP_INFO(this->get_logger(), ">>> INITIATING LOCALIZATION SEQUENCE <<<");

    if (!get_qr_client_->wait_for_service(std::chrono::seconds(1))) {
      RCLCPP_ERROR(
          this->get_logger(),
          "Database get_qr service not available! Aborting localization.");
      return;
    }

    auto request = std::make_shared<swarm_interfaces::srv::GetQR::Request>();
    request->qr_id = target_qr_payload_;

    RCLCPP_INFO(this->get_logger(),
                "Asking database for coordinates of '%s'...",
                target_qr_payload_.c_str());

    get_qr_client_->async_send_request(
        request,
        [this](
            rclcpp::Client<swarm_interfaces::srv::GetQR>::SharedFuture future) {
          auto response = future.get();
          if (response->success) {
            RCLCPP_INFO(this->get_logger(),
                        "Database found QR! Map position: X: %.2f, Y: %.2f",
                        response->pose.position.x, response->pose.position.y);

            try {
              // 1. Get Base -> Camera transform
              std::string base_frame = robot_name_ + "/base_footprint";
              geometry_msgs::msg::TransformStamped tf_base_to_cam_msg =
                  tf_buffer_->lookupTransform(base_frame, camera_frame_,
                                              tf2::TimePointZero);

              tf2::Transform tf_base_to_cam;
              tf2::fromMsg(tf_base_to_cam_msg.transform, tf_base_to_cam);

              // 2. Calculate Base -> QR
              tf2::Transform tf_base_to_qr =
                  tf_base_to_cam * latest_tf_cam_to_qr_;

              // 3. Get Map -> QR from database response
              tf2::Transform tf_map_to_qr;
              tf2::fromMsg(response->pose, tf_map_to_qr);

              // 4. Inverse Kinematics: Map -> Base = (Map -> QR) * (Base ->
              // QR)^-1
              tf2::Transform tf_map_to_base =
                  tf_map_to_qr * tf_base_to_qr.inverse();

              geometry_msgs::msg::Pose msg_map_to_base;
              tf2::toMsg(tf_map_to_base, msg_map_to_base);

              // 5. Store for publishing after map loads
              final_initialpose_.header.frame_id = "map";
              final_initialpose_.header.stamp = this->now();
              final_initialpose_.pose.pose = msg_map_to_base;
              final_initialpose_.pose.covariance[0] = 0.25;
              final_initialpose_.pose.covariance[7] = 0.25;
              final_initialpose_.pose.covariance[35] = 0.5;

              RCLCPP_INFO(this->get_logger(),
                          "Base localized perfectly via Inverse Kinematics.");

              // Proceed to fetch the map
              fetchMap();

            } catch (tf2::TransformException &ex) {
              RCLCPP_WARN(this->get_logger(),
                          "TF lookup failed during inverse kinematics: %s",
                          ex.what());
            }
          } else {
            RCLCPP_WARN(this->get_logger(),
                        "Database does not know '%s' yet. Ignoring it and "
                        "resuming exploration.",
                        target_qr_payload_.c_str());
            ignored_qrs_.insert(target_qr_payload_);
            state_ = State::TURN_AWAY;
            turn_away_start_time_ = this->now();
          }
        });
  }

  geometry_msgs::msg::PoseWithCovarianceStamped final_initialpose_;

  void fetchMap() {
    RCLCPP_INFO(this->get_logger(), "Requesting Map from Database...");

    if (!get_map_client_->wait_for_service(std::chrono::seconds(1))) {
      RCLCPP_ERROR(this->get_logger(),
                   "Database get_map service not available!");
      return;
    }

    auto request = std::make_shared<swarm_interfaces::srv::GetMap::Request>();
    get_map_client_->async_send_request(
        request,
        [this](rclcpp::Client<swarm_interfaces::srv::GetMap>::SharedFuture
                   future) {
          auto response = future.get();
          if (response->success) {
            RCLCPP_INFO(this->get_logger(),
                        "Map retrieved from database successfully!");

            // Modify map header to tb3_2 map frame
            nav_msgs::msg::OccupancyGrid map_msg = response->map;
            map_msg.header.frame_id = "map";
            map_msg.header.stamp = this->now();

            // Publish Map
            map_pub_->publish(map_msg);
            RCLCPP_INFO(this->get_logger(), "Published map to /tb3_2/map");

            // 1. Trigger AMCL and Nav2 Lifecycle Nodes FIRST
            RCLCPP_INFO(this->get_logger(),
                        ">>> ACTIVATING AMCL AND NAV2 LIFECYCLE NODES <<<");
            auto req = std::make_shared<
                nav2_msgs::srv::ManageLifecycleNodes::Request>();
            req->command =
                nav2_msgs::srv::ManageLifecycleNodes::Request::STARTUP;

            manage_loc_client_->async_send_request(req);
            manage_nav_client_->async_send_request(req);

            // 2. Wait for AMCL to transition to Active so it can receive the
            // initialpose topic
            RCLCPP_INFO(this->get_logger(), "Waiting for AMCL to wake up...");
            rclcpp::sleep_for(3s);

            // 3. Publish initialpose
            final_initialpose_.header.stamp = this->now();
            initial_pose_pub_->publish(final_initialpose_);

            RCLCPP_INFO(
                this->get_logger(),
                ">>> AMCL TRIGGERED! Robot Map Position: X: %.2f, Y: %.2f <<<",
                final_initialpose_.pose.pose.position.x,
                final_initialpose_.pose.pose.position.y);

            RCLCPP_INFO(
                this->get_logger(),
                "Explorer Node going DORMANT. Handoff to Nav2 complete!");
            state_ = State::DONE;

            // Start continuous map update timer
            map_update_timer_ = this->create_wall_timer(
                15s, std::bind(&ExplorerNode::updateMap, this));
          } else {
            RCLCPP_WARN(this->get_logger(),
                        "Map not found in database! Retrying in 2 seconds...");
            // Simple retry timer could go here
          }
        });
  }

  void updateMap() {
    if (!get_map_client_->wait_for_service(std::chrono::seconds(1))) {
      return;
    }

    auto request = std::make_shared<swarm_interfaces::srv::GetMap::Request>();
    get_map_client_->async_send_request(
        request,
        [this](rclcpp::Client<swarm_interfaces::srv::GetMap>::SharedFuture
                   future) {
          auto response = future.get();
          if (response->success) {
            nav_msgs::msg::OccupancyGrid map_msg = response->map;
            map_msg.header.frame_id = "map";
            map_msg.header.stamp = this->now();
            map_pub_->publish(map_msg);
            RCLCPP_INFO(this->get_logger(),
                        "Updated map published to /tb3_2/map");

            if (response->is_mapping_complete) {
              RCLCPP_INFO(this->get_logger(),
                          "Database indicates mapping is complete. Stopping "
                          "map updates.");
              map_update_timer_->cancel();
            }
          }
        });
  }
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<ExplorerNode>());
  rclcpp::shutdown();
  return 0;
}
