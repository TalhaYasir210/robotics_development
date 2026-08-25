#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <std_msgs/msg/bool.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/point_stamped.hpp>
#include <geometry_msgs/msg/quaternion.hpp>
#include <nav2_msgs/action/navigate_to_pose.hpp>
#include <cv_bridge/cv_bridge.hpp>
#include <opencv2/opencv.hpp>
#include <zbar.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <cmath>
#include <string>
#include <unordered_set>
#include "swarm_interfaces/srv/save_qr.hpp"

using namespace std::placeholders;

class QRInterceptorNode : public rclcpp::Node
{
public:
    using NavigateToPose = nav2_msgs::action::NavigateToPose;
    using GoalHandleNav = rclcpp_action::ClientGoalHandle<NavigateToPose>;

    QRInterceptorNode() : Node("qr_interceptor_node"), state_("EXPLORING")
    {
        // Declare and get parameters
        this->declare_parameter("robot_name", "tb3_1");
        this->declare_parameter("global_frame", "map");
        this->declare_parameter("camera_frame", "");
        this->declare_parameter("target_distance", 0.5);
        this->declare_parameter("qr_size", 0.2);

        robot_name_ = this->get_parameter("robot_name").as_string();
        global_frame_ = this->get_parameter("global_frame").as_string();
        camera_frame_ = this->get_parameter("camera_frame").as_string();
        target_distance_ = this->get_parameter("target_distance").as_double();
        qr_real_width_ = this->get_parameter("qr_size").as_double();

        if (camera_frame_.empty()) {
            camera_frame_ = robot_name_ + "/camera_rgb_optical_frame";
        }
        
        // TF2 Setup
        tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
        tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_, this, false);
        
        rmw_qos_profile_t qos_profile = rmw_qos_profile_sensor_data;
        auto qos = rclcpp::QoS(rclcpp::QoSInitialization(qos_profile.history, 1), qos_profile);
        
        // Dynamic Topic Binding
        std::string cam_info_topic = "/" + robot_name_ + "/camera/camera_info";
        std::string image_topic = "/" + robot_name_ + "/camera/image_raw";
        std::string resume_topic = "/" + robot_name_ + "/explore/resume";
        std::string nav_action = "/" + robot_name_ + "/navigate_to_pose";

        cam_info_sub_ = this->create_subscription<sensor_msgs::msg::CameraInfo>(
            cam_info_topic, qos,
            std::bind(&QRInterceptorNode::camInfoCallback, this, _1));
            
        image_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            image_topic, qos,
            std::bind(&QRInterceptorNode::imageCallback, this, _1));
            
        explore_resume_pub_ = this->create_publisher<std_msgs::msg::Bool>(
            resume_topic, 10);
            
        nav_client_ = rclcpp_action::create_client<NavigateToPose>(
            this, nav_action);
            
        db_client_ = this->create_client<swarm_interfaces::srv::SaveQR>("/save_qr");
            
        RCLCPP_INFO(this->get_logger(), "QR Interceptor (Multi-Robot Ready) initialized for [%s].", robot_name_.c_str());
    }

private:
    void camInfoCallback(const sensor_msgs::msg::CameraInfo::SharedPtr msg)
    {
        if (!camera_info_) {
            camera_info_ = msg;
        }
    }

    void imageCallback(const sensor_msgs::msg::Image::SharedPtr msg)
    {
        if (state_ != "EXPLORING" || !camera_info_) {
            return;
        }
        
        cv_bridge::CvImagePtr cv_ptr;
        try {
            cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
        } catch (cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
            return;
        }
        
        cv::Mat gray;
        cv::cvtColor(cv_ptr->image, gray, cv::COLOR_BGR2GRAY);
        
        zbar::ImageScanner scanner;
        scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);
        
        zbar::Image zbar_image(gray.cols, gray.rows, "Y800", gray.ptr(), gray.cols * gray.rows);
        int n = scanner.scan(zbar_image);
        
        if (n > 0) {
            for (zbar::Image::SymbolIterator symbol = zbar_image.symbol_begin();
                 symbol != zbar_image.symbol_end(); ++symbol)
            {
                std::string payload = symbol->get_data();
                
                // Ignore already processed QR codes
                if (processed_qrs_.count(payload) > 0) {
                    continue;
                }
                
                RCLCPP_INFO(this->get_logger(), "QR detected: %s", payload.c_str());
                
                int n_pts = symbol->get_location_size();
                if (n_pts >= 4) {
                    // Extract corners in ZBar order (top-left, bottom-left, bottom-right, top-right)
                    std::vector<cv::Point2f> image_points;
                    for (int i = 0; i < 4; ++i) {
                        image_points.push_back(cv::Point2f(symbol->get_location_x(i), symbol->get_location_y(i)));
                    }
                    
                    // Robust Geometric Line-of-Sight Approach (No solvePnP to avoid rotation bugs)
                    // 1. Calculate the pixel size of the QR code
                    double side1 = cv::norm(image_points[0] - image_points[1]);
                    double side2 = cv::norm(image_points[1] - image_points[2]);
                    double side3 = cv::norm(image_points[2] - image_points[3]);
                    double side4 = cv::norm(image_points[3] - image_points[0]);
                    double pixel_width = (side1 + side2 + side3 + side4) / 4.0;
                    
                    // 2. Use Pinhole camera model to find depth (Z) and XY
                    double fx = camera_info_->k[0];
                    double fy = camera_info_->k[4];
                    double cx = camera_info_->k[2];
                    double cy = camera_info_->k[5];
                    
                    double Z = (fx * qr_real_width_) / pixel_width;
                    double center_x = (image_points[0].x + image_points[1].x + image_points[2].x + image_points[3].x) / 4.0;
                    double center_y = (image_points[0].y + image_points[1].y + image_points[2].y + image_points[3].y) / 4.0;
                    
                    double X = (center_x - cx) * Z / fx;
                    double Y = (center_y - cy) * Z / fy;
                    
                    geometry_msgs::msg::PoseStamped pose_cam;
                    pose_cam.header.stamp = msg->header.stamp;
                    pose_cam.header.frame_id = camera_frame_;
                    pose_cam.pose.position.x = X;
                    pose_cam.pose.position.y = Y;
                    pose_cam.pose.position.z = Z;
                    // Keep orientation flat for now, we will calculate Nav2 yaw manually
                    pose_cam.pose.orientation.w = 1.0;
                    
                    try {
                        auto transform = tf_buffer_->lookupTransform(global_frame_, camera_frame_, rclcpp::Time(0), rclcpp::Duration::from_nanoseconds(0));
                        geometry_msgs::msg::PoseStamped qr_map;
                        tf2::doTransform(pose_cam, qr_map, transform);
                        
                        // 3. Line of sight from camera to QR code in global frame
                        double cam_x = transform.transform.translation.x;
                        double cam_y = transform.transform.translation.y;
                        double qr_x = qr_map.pose.position.x;
                        double qr_y = qr_map.pose.position.y;
                        
                        double dx = cam_x - qr_x;
                        double dy = cam_y - qr_y;
                        double dist = std::sqrt(dx*dx + dy*dy);
                        
                        double nx = dx / dist; // Vector FROM qr TO camera
                        double ny = dy / dist;
                        
                        // 4. Place goal target_distance_ away from QR code along the line of sight!
                        // This guarantees the robot drives straight at it and it remains perfectly centered in the camera.
                        double goal_x = qr_x + target_distance_ * nx;
                        double goal_y = qr_y + target_distance_ * ny;
                        
                        // The robot must face the QR code, so yaw is exactly opposite to the nx,ny vector
                        double goal_yaw = std::atan2(-ny, -nx);
                        
                        processed_qrs_.insert(payload); // Add to cooldown set
                        
                        executeInterception(goal_x, goal_y, goal_yaw, qr_map.pose, payload);
                        zbar_image.set_data(NULL, 0);
                        return; // Handle one
                    } catch (const tf2::TransformException &ex) {
                        // Suppressed to avoid spamming. Will retry on next frame.
                    }
                }
            }
        }
        zbar_image.set_data(NULL, 0);
    }

    void executeInterception(double x, double y, double yaw, geometry_msgs::msg::Pose exact_pose, const std::string& payload)
    {
        state_ = "PREEMPTING";
        RCLCPP_INFO(this->get_logger(), "Preempting explore-lite...");
        
        std_msgs::msg::Bool pause_msg;
        pause_msg.data = false;
        explore_resume_pub_->publish(pause_msg);
        
        state_ = "NAVIGATING";
        RCLCPP_INFO(this->get_logger(), "Targeting: (%.2f, %.2f) in %s", x, y, global_frame_.c_str());
        
        if (!nav_client_->wait_for_action_server(std::chrono::seconds(5))) {
            RCLCPP_ERROR(this->get_logger(), "Nav2 Action Server not available! Resuming exploration.");
            processed_qrs_.erase(payload); // Remove from set so we try again later
            resumeExploration();
            return;
        }
        
        auto goal_msg = NavigateToPose::Goal();
        goal_msg.pose.header.frame_id = global_frame_;
        goal_msg.pose.header.stamp = this->get_clock()->now();
        goal_msg.pose.pose.position.x = x;
        goal_msg.pose.pose.position.y = y;
        
        double qz = std::sin(yaw / 2.0);
        double qw = std::cos(yaw / 2.0);
        goal_msg.pose.pose.orientation.z = qz;
        goal_msg.pose.pose.orientation.w = qw;
        
        auto send_goal_options = rclcpp_action::Client<NavigateToPose>::SendGoalOptions();
        send_goal_options.goal_response_callback =
            std::bind(&QRInterceptorNode::goalResponseCallback, this, _1, exact_pose, payload);
        send_goal_options.result_callback =
            std::bind(&QRInterceptorNode::resultCallback, this, _1, exact_pose, payload);
            
        nav_client_->async_send_goal(goal_msg, send_goal_options);
    }

    void goalResponseCallback(const GoalHandleNav::SharedPtr & goal_handle, geometry_msgs::msg::Pose exact_pose, const std::string& payload)
    {
        (void)exact_pose;
        if (!goal_handle) {
            RCLCPP_WARN(this->get_logger(), "Nav2 Goal rejected");
            processed_qrs_.erase(payload); // Remove from set so we try again later
            resumeExploration();
        } else {
            RCLCPP_INFO(this->get_logger(), "Nav2 Goal accepted, waiting for result");
        }
    }

    void resultCallback(const GoalHandleNav::WrappedResult & result, geometry_msgs::msg::Pose exact_pose, const std::string& payload)
    {
        state_ = "LOGGING";
        
        RCLCPP_INFO(this->get_logger(), "=========================================");
        if (result.code == rclcpp_action::ResultCode::SUCCEEDED) {
            RCLCPP_INFO(this->get_logger(), "QR CODE REACHED SUCCESSFULLY");
        } else {
            RCLCPP_INFO(this->get_logger(), "QR CODE REACH FAILED / ABORTED. Attempting to save coordinates anyway.");
        }
        
        // Save to database ALWAYS (since the solvePnP coordinates are accurate from afar)
        if (db_client_->wait_for_service(std::chrono::seconds(1))) {
            auto request = std::make_shared<swarm_interfaces::srv::SaveQR::Request>();
            request->qr_id = payload;
            request->pose = exact_pose;
            db_client_->async_send_request(request);
            RCLCPP_INFO(this->get_logger(), ">>> SUCCESS: Sent QR exact coordinates to Database! <<<");
        } else {
            RCLCPP_WARN(this->get_logger(), ">>> FAILED: Database service not available! QR NOT SAVED. <<<");
            processed_qrs_.erase(payload); // Remove from set to retry later
        }

        RCLCPP_INFO(this->get_logger(), "Decoded Payload: %s", payload.c_str());
        RCLCPP_INFO(this->get_logger(), "Exact 3D Position (%s frame):", global_frame_.c_str());
        RCLCPP_INFO(this->get_logger(), "X: %.3f", exact_pose.position.x);
        RCLCPP_INFO(this->get_logger(), "Y: %.3f", exact_pose.position.y);
        RCLCPP_INFO(this->get_logger(), "Z: %.3f", exact_pose.position.z);
        RCLCPP_INFO(this->get_logger(), "=========================================");
        
        resumeExploration();
    }

    void resumeExploration()
    {
        state_ = "RESUMING";
        RCLCPP_INFO(this->get_logger(), "Resuming explore-lite...");
        std_msgs::msg::Bool resume_msg;
        resume_msg.data = true;
        explore_resume_pub_->publish(resume_msg);
        state_ = "EXPLORING";
    }

    std::string robot_name_;
    std::string global_frame_;
    std::string camera_frame_;
    double target_distance_;
    double qr_real_width_;
    std::string state_;

    sensor_msgs::msg::CameraInfo::SharedPtr camera_info_;
    std::unordered_set<std::string> processed_qrs_;
    
    std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
    std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
    
    rclcpp::Subscription<sensor_msgs::msg::CameraInfo>::SharedPtr cam_info_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
    rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr explore_resume_pub_;
    rclcpp_action::Client<NavigateToPose>::SharedPtr nav_client_;
    rclcpp::Client<swarm_interfaces::srv::SaveQR>::SharedPtr db_client_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<QRInterceptorNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
