#include "cv_bridge/cv_bridge.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "sensor_msgs/msg/camera_info.hpp"
#include "opencv2/opencv.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "std_msgs/msg/bool.hpp"
#include "swarm_interfaces/srv/save_qr.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <chrono>
#include <unordered_set>
#include <zbar.h>
#include <cmath>

using namespace std::chrono_literals;

enum class State {
    SEARCHING,
    SERVOING,
    BACKING_UP,
    RECOVERY_SWEEP,
    BLIND_APPROACH
};

class QRInterceptorNode : public rclcpp::Node {
public:
    QRInterceptorNode() : Node("bot1_interceptor") {
        this->declare_parameter<std::string>("robot_name", "tb3_1");
        this->declare_parameter<std::string>("global_frame", "map");
        this->declare_parameter<std::string>("camera_frame", "");
        this->declare_parameter<double>("target_distance", 0.2);
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
        tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_, this, false);

        std::string cam_info_topic = "/" + robot_name_ + "/camera/camera_info";
        std::string image_topic = "/" + robot_name_ + "/camera/image_raw";
        std::string scan_topic = "/" + robot_name_ + "/scan";
        std::string resume_topic = "/" + robot_name_ + "/explore/resume";
        std::string cmd_vel_topic = "/" + robot_name_ + "/cmd_vel";

        cam_info_sub_ = this->create_subscription<sensor_msgs::msg::CameraInfo>(
            cam_info_topic, rclcpp::SensorDataQoS(),
            std::bind(&QRInterceptorNode::camInfoCallback, this, std::placeholders::_1));

        image_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            image_topic, rclcpp::SensorDataQoS(),
            std::bind(&QRInterceptorNode::imageCallback, this, std::placeholders::_1));

        scan_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
            scan_topic, rclcpp::SensorDataQoS(),
            std::bind(&QRInterceptorNode::scanCallback, this, std::placeholders::_1));

        explore_resume_pub_ = this->create_publisher<std_msgs::msg::Bool>(resume_topic, 10);
        cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>(cmd_vel_topic, 10);

        db_client_ = this->create_client<swarm_interfaces::srv::SaveQR>("/save_qr");

        // Control loop timer for Servoing fallbacks and backing up
        control_timer_ = this->create_wall_timer(
            100ms, std::bind(&QRInterceptorNode::controlLoop, this));

        RCLCPP_INFO(this->get_logger(), "Bot 1 Visual Servoing Interceptor initialized. Waiting for QR codes...");
    }

private:
    std::string robot_name_;
    std::string global_frame_;
    std::string camera_frame_;
    double target_distance_;
    double qr_real_width_;

    State state_ = State::SEARCHING;
    bool obstacle_detected_ = false;
    std::string target_qr_payload_;
    geometry_msgs::msg::Pose latest_qr_pose_map_;
    double last_Z_ = 999.0;
    rclcpp::Time last_seen_time_;
    rclcpp::Time backup_start_time_;
    rclcpp::Time recovery_start_time_;
    rclcpp::Time blind_approach_start_time_;
    double blind_approach_duration_ = 0.0;

    std::unordered_set<std::string> processed_qrs_;

    rclcpp::Subscription<sensor_msgs::msg::CameraInfo>::SharedPtr cam_info_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
    rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr scan_sub_;
    sensor_msgs::msg::CameraInfo::SharedPtr camera_info_;

    rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr explore_resume_pub_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
    
    std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
    std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
    
    rclcpp::Client<swarm_interfaces::srv::SaveQR>::SharedPtr db_client_;
    rclcpp::TimerBase::SharedPtr control_timer_;

    void camInfoCallback(const sensor_msgs::msg::CameraInfo::SharedPtr msg) {
        if (!camera_info_) {
            camera_info_ = msg;
        }
    }

    void scanCallback(const sensor_msgs::msg::LaserScan::SharedPtr msg) {
        if (state_ != State::SERVOING && state_ != State::RECOVERY_SWEEP) {
            obstacle_detected_ = false;
            return;
        }
        
        bool local_obstacle = false;
        int num_ranges = msg->ranges.size();
        if (num_ranges == 0) return;

        double angle_increment = msg->angle_increment;
        double angle_min = msg->angle_min;

        for (int i = 0; i < num_ranges; ++i) {
            double range = msg->ranges[i];
            if (std::isnan(range) || std::isinf(range)) continue;
            
            double angle = angle_min + i * angle_increment;
            while (angle < 0) angle += 2 * M_PI;
            while (angle >= 2 * M_PI) angle -= 2 * M_PI;
            
            double angle_deg = angle * 180.0 / M_PI;

            // Front cone (-30 to 30 deg): Check if closer than 0.15m (allows reaching 0.2m target)
            if (angle_deg <= 30.0 || angle_deg >= 330.0) {
                if (range < 0.15) {
                    local_obstacle = true;
                    break;
                }
            }
            // Left side (30 to 120 deg) and Right side (240 to 330 deg)
            else if ((angle_deg > 30.0 && angle_deg < 120.0) || (angle_deg > 240.0 && angle_deg < 330.0)) {
                if (range < 0.20) { // Side clearance threshold
                    local_obstacle = true;
                    break;
                }
            }
        }
        obstacle_detected_ = local_obstacle;
    }

    void imageCallback(const sensor_msgs::msg::Image::SharedPtr msg) {
        if (!camera_info_ || state_ == State::BACKING_UP) return;

        try {
            cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
            cv::Mat gray;
            cv::cvtColor(cv_ptr->image, gray, cv::COLOR_BGR2GRAY);

            zbar::ImageScanner scanner;
            scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);
            zbar::Image zbar_image(gray.cols, gray.rows, "Y800", gray.ptr(), gray.cols * gray.rows);
            int n = scanner.scan(zbar_image);

            if (n > 0) {
                for (zbar::Image::SymbolIterator symbol = zbar_image.symbol_begin(); symbol != zbar_image.symbol_end(); ++symbol) {
                    std::string payload = symbol->get_data();

                    if (processed_qrs_.find(payload) != processed_qrs_.end()) {
                        continue; 
                    }

                    // Calculate center_x to check if the QR code is too far off-center
                    double center_x_check = 0;
                    for (int i = 0; i < 4; ++i) {
                        center_x_check += symbol->get_location_x(i);
                    }
                    center_x_check /= 4.0;
                    if (std::abs(camera_info_->k[2] - center_x_check) > 400.0) {
                        continue; // Ignore target if pixel error is more than 400
                    }

                    if (state_ == State::SEARCHING) {
                        RCLCPP_INFO(this->get_logger(), "\n=========================================");
                        RCLCPP_INFO(this->get_logger(), ">>> QR DETECTED: %s", payload.c_str());
                        RCLCPP_INFO(this->get_logger(), ">>> Preempting explore-lite and beginning Visual Servoing approach!");
                        
                        // Pause exploration
                        std_msgs::msg::Bool pause_msg;
                        pause_msg.data = false;
                        explore_resume_pub_->publish(pause_msg);
                        
                        target_qr_payload_ = payload;
                        state_ = State::SERVOING;
                    }

                    if ((state_ == State::SERVOING || state_ == State::RECOVERY_SWEEP) && payload == target_qr_payload_) {
                        if (state_ == State::RECOVERY_SWEEP) {
                            RCLCPP_INFO(this->get_logger(), "[Recovery] QR code reacquired! Resuming tracking.");
                            state_ = State::SERVOING;
                        }
                        
                        last_seen_time_ = this->now();

                        // 1. Pixel Extraction
                        std::vector<cv::Point2f> image_points;
                        for (int i = 0; i < 4; ++i) {
                            image_points.push_back(cv::Point2f(symbol->get_location_x(i), symbol->get_location_y(i)));
                        }
                        
                        double side1 = cv::norm(image_points[0] - image_points[1]);
                        double side2 = cv::norm(image_points[1] - image_points[2]);
                        double side3 = cv::norm(image_points[2] - image_points[3]);
                        double side4 = cv::norm(image_points[3] - image_points[0]);
                        double pixel_width = (side1 + side2 + side3 + side4) / 4.0;
                        
                        // 2. Pinhole Math for pristine Position
                        double fx = camera_info_->k[0];
                        double fy = camera_info_->k[4];
                        double cx = camera_info_->k[2];
                        double cy = camera_info_->k[5];
                        
                        double Z = (fx * qr_real_width_) / pixel_width;
                        last_Z_ = Z;
                        double center_x = (image_points[0].x + image_points[1].x + image_points[2].x + image_points[3].x) / 4.0;
                        double center_y = (image_points[0].y + image_points[1].y + image_points[2].y + image_points[3].y) / 4.0;
                        
                        double X = (center_x - cx) * Z / fx;
                        double Y = (center_y - cy) * Z / fy;
                        
                        // 3. solvePnP for pristine Orientation
                        double hw = qr_real_width_ / 2.0;
                        std::vector<cv::Point3f> object_points = {
                            cv::Point3f(-hw, hw, 0), cv::Point3f(-hw, -hw, 0), cv::Point3f(hw, -hw, 0), cv::Point3f(hw, hw, 0)
                        };
                        cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) << fx, 0, cx, 0, fy, cy, 0, 0, 1);
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

                        // 4. Construct Pose and Transform to Map
                        geometry_msgs::msg::PoseStamped pose_cam;
                        pose_cam.header.stamp = msg->header.stamp;
                        pose_cam.header.frame_id = camera_frame_;
                        pose_cam.pose.position.x = X; // Use pristine Pinhole position
                        pose_cam.pose.position.y = Y;
                        pose_cam.pose.position.z = Z;
                        pose_cam.pose.orientation = tf2::toMsg(tf2_q); // Use solvePnP orientation
                        
                        try {
                            auto transform = tf_buffer_->lookupTransform(global_frame_, camera_frame_, tf2::TimePointZero);
                            geometry_msgs::msg::PoseStamped qr_map;
                            tf2::doTransform(pose_cam, qr_map, transform);
                            
                            // Save this highly accurate calculation as our best guess
                            latest_qr_pose_map_ = qr_map.pose;
                            
                            // 5. Visual Servoing Control!
                            // Ignore Nav2 inflation layers by driving manually.
                            double error_x = cx - center_x;
                            geometry_msgs::msg::Twist twist;
                            
                            if (Z > target_distance_) { 
                                // "Brain" Logic: Euclidean/Angle Error Control
                                
                                // Calculate angular velocity and clamp it to max 0.3 rad/s to prevent camera motion blur and overshooting!
                                double ang_z = 0.0015 * error_x; 
                                if (ang_z > 0.3) ang_z = 0.3;
                                if (ang_z < -0.3) ang_z = -0.3;

                                // Dynamically scale the alignment threshold based on camera resolution
                                // cx is half the image width. We use 30% of cx as the threshold.
                                double dynamic_threshold = cx * 0.3;

                                if (std::abs(error_x) > dynamic_threshold) {
                                    // If angle error is large, turn to face it and drive forward slowly
                                    twist.linear.x = 0.05; // Gentle forward movement so it doesn't get stuck far away
                                    twist.angular.z = ang_z; 
                                    RCLCPP_INFO(this->get_logger(), "[Servoing] Aligning angle first (Error: %.1f pixels)...", error_x);
                                } else {
                                    // Once aligned, drive forward while making minor steering adjustments
                                    twist.linear.x = 0.15; // Safe approach speed
                                    twist.angular.z = ang_z; 
                                    RCLCPP_INFO(this->get_logger(), "[Servoing] Aligned! Distance: %.2fm | Driving forward...", Z);
                                }
                                cmd_vel_pub_->publish(twist);
                            } else {
                                // We are extremely close to the wall. Stop and save!
                                RCLCPP_INFO(this->get_logger(), "[Servoing] Reached wall threshold (%.2fm)!", target_distance_);
                                saveAndBackup();
                            }
                        } catch (const tf2::TransformException &ex) {
                            RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "TF Error: %s", ex.what());
                        }
                    }
                }
            }
            zbar_image.set_data(NULL, 0);
        } catch (cv_bridge::Exception &e) {
            RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
        }
    }

    void controlLoop() {
        auto now = this->now();

        if (state_ == State::SERVOING) {
            // [LiDAR Obstacle Avoidance Disabled per User Request]
            // if (obstacle_detected_) {
            //     RCLCPP_WARN(this->get_logger(), "[Servoing] Obstacle too close (Sides or Front)! Aborting to prevent crash.");
            //     geometry_msgs::msg::Twist twist;
            //     cmd_vel_pub_->publish(twist);
            //     state_ = State::SEARCHING;
            //     resumeExploration();
            //     return;
            // }

            double time_since_last_seen = (now - last_seen_time_).seconds();
            
            // Anti-Overshoot Brake: If we haven't seen it in 0.3 seconds, STOP moving!
            // This prevents the robot from blindly spinning and throwing the QR code out of the camera view.
            if (time_since_last_seen > 0.3 && time_since_last_seen <= 2.5) {
                geometry_msgs::msg::Twist twist;
                cmd_vel_pub_->publish(twist);
            }

            // If we lost sight of the QR code for more than 2.5 seconds
            if (time_since_last_seen > 2.5) {
                if (last_Z_ < 0.60) {
                    RCLCPP_INFO(this->get_logger(), "[Servoing] close to QR code at %.2fm. SUCCESS!", last_Z_);
                    saveAndBackup();
                } else if (last_Z_ <= 1.0) { // Since QR size is 0.28, true loss distance is ~0.82m. 1.0m gives a safe buffer.
                    RCLCPP_WARN(this->get_logger(), "[Servoing] Lost sight at %.2fm. Blindly driving forward to reach 0.5m...", last_Z_);
                    state_ = State::BLIND_APPROACH;
                    blind_approach_start_time_ = now;
                    double distance_to_cover = last_Z_ - 0.50;
                    if (distance_to_cover < 0.0) distance_to_cover = 0.0;
                    blind_approach_duration_ = distance_to_cover / 0.15; // Target velocity is 0.15 m/s
                } else {
                    // We lost it from far away. Initiate Recovery Sweep.
                    RCLCPP_WARN(this->get_logger(), "[Servoing] Lost sight at far range (%.2fm). Initiating Recovery Sweep...", last_Z_);
                    state_ = State::RECOVERY_SWEEP;
                    recovery_start_time_ = now;
                }
            }
        } else if (state_ == State::RECOVERY_SWEEP) {
            double elapsed = (now - recovery_start_time_).seconds();
            geometry_msgs::msg::Twist twist;
            twist.linear.x = 0.0;
            
            // Perform a full 360-degree sweep to locate the QR code.
            // 2 * pi radians ~= 6.28 radians.
            // At 0.5 rad/s, it takes ~12.6 seconds to complete a 360.
            if (elapsed < 13.0) {
                twist.angular.z = 0.5; // Spin Left (CCW)
            } else {
                // Sweep finished without finding it.
                twist.angular.z = 0.0;
                RCLCPP_WARN(this->get_logger(), "[Recovery] 360 Sweep finished. QR code not found. Aborting.");
                state_ = State::SEARCHING;
                resumeExploration();
            }
            cmd_vel_pub_->publish(twist);
        } else if (state_ == State::BLIND_APPROACH) {
            double elapsed = (now - blind_approach_start_time_).seconds();
            if (elapsed < blind_approach_duration_) {
                geometry_msgs::msg::Twist twist;
                twist.linear.x = 0.15; // Drive forward at 0.15 m/s
                twist.angular.z = 0.0;
                cmd_vel_pub_->publish(twist);
            } else {
                geometry_msgs::msg::Twist stop_twist;
                cmd_vel_pub_->publish(stop_twist);
                last_Z_ = 0.50; // Update estimated distance
                RCLCPP_INFO(this->get_logger(), "[Blind Approach] Reached estimated 0.5m distance. SUCCESS!");
                saveAndBackup();
            }
        } else if (state_ == State::BACKING_UP) {
            // Reverse out of the inflation zone for 4 seconds
            if ((now - backup_start_time_).seconds() < 4.0) {
                geometry_msgs::msg::Twist twist;
                twist.linear.x = -0.15; // Reverse
                twist.angular.z = 0.0;
                cmd_vel_pub_->publish(twist);
            } else {
                // Stop reversing and resume exploration
                geometry_msgs::msg::Twist twist;
                twist.linear.x = 0.0;
                cmd_vel_pub_->publish(twist);
                
                RCLCPP_INFO(this->get_logger(), "Backed safely out of inflation zone. Resuming Exploration.");
                state_ = State::SEARCHING;
                resumeExploration();
            }
        }
    }

    void saveAndBackup() {
        // Stop the robot
        geometry_msgs::msg::Twist stop_twist;
        cmd_vel_pub_->publish(stop_twist);

        // 1. Log all the PhD math for the user
        RCLCPP_INFO(this->get_logger(), "=========================================");
        RCLCPP_INFO(this->get_logger(), ">>> FINALIZING QR CALCULATION <<<");
        RCLCPP_INFO(this->get_logger(), "Payload: %s", target_qr_payload_.c_str());
        RCLCPP_INFO(this->get_logger(), "Final Calculated Distance (Z): %.3f meters", last_Z_);
        RCLCPP_INFO(this->get_logger(), "Map X: %.3f | Map Y: %.3f | Map Z: %.3f", 
            latest_qr_pose_map_.position.x, latest_qr_pose_map_.position.y, latest_qr_pose_map_.position.z);
        RCLCPP_INFO(this->get_logger(), "Orientation logged via solvePnP.");
        
        // 2. Save to Database
        if (db_client_->wait_for_service(std::chrono::seconds(1))) {
            auto request = std::make_shared<swarm_interfaces::srv::SaveQR::Request>();
            request->qr_id = target_qr_payload_;
            request->pose = latest_qr_pose_map_;
            db_client_->async_send_request(request);
            RCLCPP_INFO(this->get_logger(), ">>> SUCCESS: Exact coordinates and orientation sent to Database! <<<");
        } else {
            RCLCPP_WARN(this->get_logger(), ">>> FAILED: Database service not available! <<<");
        }
        RCLCPP_INFO(this->get_logger(), "=========================================\n");

        processed_qrs_.insert(target_qr_payload_);

        // 3. Begin backing up out of the inflation zone
        state_ = State::BACKING_UP;
        backup_start_time_ = this->now();
        RCLCPP_INFO(this->get_logger(), "Reversing to escape Nav2 Inflation Radius...");
    }

    void resumeExploration() {
        std_msgs::msg::Bool msg;
        msg.data = true;
        explore_resume_pub_->publish(msg);
    }
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<QRInterceptorNode>());
    rclcpp::shutdown();
    return 0;
}
