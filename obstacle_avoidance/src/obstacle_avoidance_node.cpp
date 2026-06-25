#include <memory>
#include <chrono>
#include <thread>
#include <signal.h> 
#include <sstream>
#include <iomanip>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp/qos.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "geometry_msgs/msg/point_stamped.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "std_msgs/msg/string.hpp"

#include "sensor_processor.hpp"
#include "autonomy_fsm.hpp"
#include "motion_controller.hpp"

using std::placeholders::_1;

rclcpp::Node::SharedPtr g_node = nullptr;
rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr g_publisher = nullptr;
bool g_is_shutting_down = false; 

void sigint_handler(int sig) {
    (void)sig; 
    
    if (g_is_shutting_down) return;
    g_is_shutting_down = true;

    if (g_node && g_publisher) {
        RCLCPP_WARN(g_node->get_logger(), "🛑 EMERGENCY BRAKE ENGAGED: Forcing wheels to [0.0, 0.0] before network death...");
        
        auto stop_msg = geometry_msgs::msg::TwistStamped();
        stop_msg.header.stamp = g_node->get_clock()->now(); 
        stop_msg.header.frame_id = "base_link";
        stop_msg.twist.linear.x = 0.0;
        stop_msg.twist.angular.z = 0.0;

        for (int i = 0; i < 5; i++) {
            g_publisher->publish(stop_msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        RCLCPP_INFO(g_node->get_logger(), "Brakes successfully transmitted. Goodbye.");
    }
    
    g_publisher.reset();
    g_node.reset();
    rclcpp::shutdown();
}

class ObstacleAvoidanceNode : public rclcpp::Node {
public:
    ObstacleAvoidanceNode() : Node("obstacle_avoidance_node") {
        
        this->declare_parameter("goal_x", 2.0);
        this->declare_parameter("goal_y", 0.5);
        goal_x_ = this->get_parameter("goal_x").as_double();
        goal_y_ = this->get_parameter("goal_y").as_double();

        double x_min = -0.16; 
        double x_max = 3.90;
        double y_min = -1.48; 
        double y_max = 2.46;

        if (goal_x_ < x_min || goal_x_ > x_max || goal_y_ < y_min || goal_y_ > y_max) {
            RCLCPP_ERROR(this->get_logger(), "MISSION ABORTED: Target (%.2f, %.2f) is outside the Safe Zone!", goal_x_, goal_y_);
            RCLCPP_INFO(this->get_logger(), "SAFE LIMITS: X [%.2f to %.2f] | Y [%.2f to %.2f]", x_min, x_max, y_min, y_max);
            is_geofence_blown_ = true; 
            current_status_ = "State: BLOCKED | Error: Target coordinate is completely outside the Geo-Fence!";
        }

        auto qos = rclcpp::QoS(rclcpp::KeepLast(10));
        auto sensor_qos = rclcpp::SensorDataQoS(); 

        publisher_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("/cmd_vel", qos);
        input_pub_ = this->create_publisher<geometry_msgs::msg::PointStamped>("/input_goal", qos);
        status_pub_ = this->create_publisher<std_msgs::msg::String>("/robot_status", qos);
        telemetry_pub_ = this->create_publisher<std_msgs::msg::String>("/robot_telemetry", qos);

        scan_front_pub_ = this->create_publisher<sensor_msgs::msg::LaserScan>("/scan_front", sensor_qos);
        scan_left_pub_ = this->create_publisher<sensor_msgs::msg::LaserScan>("/scan_left", sensor_qos);
        scan_right_pub_ = this->create_publisher<sensor_msgs::msg::LaserScan>("/scan_right", sensor_qos);
        
        odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
            "/odom", qos, std::bind(&ObstacleAvoidanceNode::odom_callback, this, _1));

        scan_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
            "/scan", sensor_qos, std::bind(&ObstacleAvoidanceNode::scan_callback, this, _1));
            
        goal_sub_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(
            "/goal_pose", qos, std::bind(&ObstacleAvoidanceNode::goal_callback, this, _1));
        
        // 🚨 INITIALIZE DUMMY DATA FOR THE HEARTBEAT
        current_cmd_msg_.header.frame_id = "base_link";
        current_cmd_msg_.twist.linear.x = 0.0;
        current_cmd_msg_.twist.angular.z = 0.0;

        // Give the dummy scans a frame and empty data so RViz doesn't throw errors before Gazebo starts
        auto init_dummy_scan = [](sensor_msgs::msg::LaserScan& scan) {
            scan.header.frame_id = "base_scan";
            scan.angle_min = 0.0; scan.angle_max = 2.0 * M_PI; scan.angle_increment = M_PI / 180.0;
            scan.ranges.assign(360, std::numeric_limits<float>::infinity());
        };
        init_dummy_scan(current_front_scan_);
        init_dummy_scan(current_left_scan_);
        init_dummy_scan(current_right_scan_);

        heartbeat_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100), 
            std::bind(&ObstacleAvoidanceNode::heartbeat_callback, this));

        RCLCPP_INFO(this->get_logger(), "Pure Heartbeat Architecture Online. Target: X=%.2f, Y=%.2f", goal_x_, goal_y_);
    }

    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr get_publisher() {
        return publisher_;
    }

private:
    double goal_x_ = 0.0; 
    double goal_y_ = 0.0;
    double current_x_ = 0.0; 
    double current_y_ = 0.0; 
    bool is_geofence_blown_ = false;

    std::string current_status_ = "State: BOOTING | Waiting for sensor data...";
    std::string current_telem_ = "Pose: [0.00, 0.00] | Target Dist: 0.00m | Waiting for Odometry...";

    // Storage for the Heartbeat Loop
    geometry_msgs::msg::TwistStamped current_cmd_msg_;
    sensor_msgs::msg::LaserScan current_front_scan_;
    sensor_msgs::msg::LaserScan current_left_scan_;
    sensor_msgs::msg::LaserScan current_right_scan_;

    SensorProcessor processor_;
    AutonomyFSM fsm_;
    MotionController controller_;

    rclcpp::TimerBase::SharedPtr heartbeat_timer_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
    rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr scan_sub_;
    rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr goal_sub_;
    
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr publisher_;
    rclcpp::Publisher<geometry_msgs::msg::PointStamped>::SharedPtr input_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr telemetry_pub_; 
    
    rclcpp::Publisher<sensor_msgs::msg::LaserScan>::SharedPtr scan_front_pub_;
    rclcpp::Publisher<sensor_msgs::msg::LaserScan>::SharedPtr scan_left_pub_;
    rclcpp::Publisher<sensor_msgs::msg::LaserScan>::SharedPtr scan_right_pub_;

    const std::string state_names_[5] = {"TRACKING", "DODGING", "RECOVERING", "ARRIVED", "BLOCKED"};

    // 🚨 THE UNIVERSAL HEARTBEAT: Publishes everything at a strict 10Hz regardless of state
    void heartbeat_callback() {
        auto now = this->get_clock()->now();

        // 1. Goal
        geometry_msgs::msg::PointStamped goal_msg;
        goal_msg.header.stamp = now;
        goal_msg.header.frame_id = "odom"; 
        goal_msg.point.x = goal_x_;
        goal_msg.point.y = goal_y_;
        input_pub_->publish(goal_msg);

        // 2. Status & Telemetry
        std_msgs::msg::String status_msg, telem_msg;
        status_msg.data = current_status_;
        telem_msg.data = current_telem_;
        status_pub_->publish(status_msg);
        telemetry_pub_->publish(telem_msg);

        // 3. Movement Commands
        current_cmd_msg_.header.stamp = now;
        publisher_->publish(current_cmd_msg_);

        // 4. LiDAR Regions
        current_front_scan_.header.stamp = now;
        current_left_scan_.header.stamp = now;
        current_right_scan_.header.stamp = now;
        scan_front_pub_->publish(current_front_scan_);
        scan_left_pub_->publish(current_left_scan_);
        scan_right_pub_->publish(current_right_scan_);
    }

    void goal_callback(const geometry_msgs::msg::PoseStamped::SharedPtr msg) {
        double new_x = msg->pose.position.x;
        double new_y = msg->pose.position.y;
        
        // Match the same geofence used at startup
        double x_min = -0.16; 
        double x_max = 3.90;
        double y_min = -1.48; 
        double y_max = 2.46;

        if (new_x < x_min || new_x > x_max || new_y < y_min || new_y > y_max) {
            RCLCPP_WARN(this->get_logger(), "RViz Goal Rejected: (%.2f, %.2f) is outside the Safe Zone!", new_x, new_y);
            return;
        }

        goal_x_ = new_x;
        goal_y_ = new_y;
        is_geofence_blown_ = false; // Reset error state
        fsm_.reset(); // Force FSM to start tracking again
        RCLCPP_INFO(this->get_logger(), "New Goal Received from RViz: X=%.2f, Y=%.2f", goal_x_, goal_y_);
    }

    void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg) {
        current_x_ = msg->pose.pose.position.x;
        current_y_ = msg->pose.pose.position.y;
        
        double qx = msg->pose.pose.orientation.x;
        double qy = msg->pose.pose.orientation.y;
        double qz = msg->pose.pose.orientation.z;
        double qw = msg->pose.pose.orientation.w;
        double yaw = std::atan2(2.0 * (qw * qz + qx * qy), 1.0 - 2.0 * (qy * qy + qz * qz));

        processor_.update_odometry(current_x_, current_y_, yaw);
    }

    void scan_callback(const sensor_msgs::msg::LaserScan::SharedPtr msg) {
        
        if (is_geofence_blown_) return; 

        ProcessedSensorData data = processor_.process_scan(msg, goal_x_, goal_y_);

        if (!data.is_valid) {
            RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000, "Waiting for /odom data...");
            return; 
        }

        // 🚨 OVERWRITE DUMMIES WITH REAL SENSOR DATA
        current_front_scan_ = data.front_scan;
        current_left_scan_ = data.left_scan;
        current_right_scan_ = data.right_scan;

        FSMDecision decision = fsm_.update_state(data);

        current_status_ = "State: " + state_names_[(int)decision.current_mode] + " | " + decision.reasoning;

        std::stringstream ss;
        ss << "Pose: [" << std::fixed << std::setprecision(2) << current_x_ << ", " << current_y_ 
           << "] | Target Dist: " << data.distance_to_goal << "m | AngErr: " << data.angle_error << "rad"
           << " || LiDAR -> Front(Min): " << data.min_front << "m (Danger: <0.30m, Safe: >0.45m) | "
           << "Left(Avg): " << data.avg_left << "m | Right(Avg): " << data.avg_right << "m";
        current_telem_ = ss.str();

        static int tick = 0;
        bool should_log_debug = (tick++ % 5 == 0);
        if (should_log_debug) { 
            RCLCPP_DEBUG(this->get_logger(), "[Processor] Pose: [%.2f, %.2f] | Dist: %.2fm | AngErr: %.2frad | MinFront: %.2fm", 
                        current_x_, current_y_, data.distance_to_goal, data.angle_error, data.min_front);
        }

        static Mode last_mode = Mode::TRACKING;
        if (decision.current_mode != last_mode) {
            if (decision.current_mode == Mode::BLOCKED) {
                RCLCPP_ERROR(this->get_logger(), "[Worker 2: FSM] BLOCKED: Target is inside an obstacle!");
            } else {
                RCLCPP_INFO(this->get_logger(), "[Worker 2: FSM] State Changed to %s! Reason: %s", 
                           state_names_[(int)decision.current_mode].c_str(), decision.reasoning.c_str());
            }
            last_mode = decision.current_mode;
        }

        VelocityCommand cmd = controller_.calculate_velocity(decision, data.angle_error);

        // 🚨 OVERWRITE DUMMY COMMANDS WITH REAL DRIVING LOGIC
        current_cmd_msg_.twist.linear.x = cmd.linear_x;
        current_cmd_msg_.twist.angular.z = cmd.angular_z;

        // Auto-shutdown removed so topics stay alive forever when arrived!
        if (decision.current_mode == Mode::ARRIVED) {
            RCLCPP_INFO_ONCE(this->get_logger(), "Target Reached! Holding position and keeping topics alive...");
        }
    }
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    
    auto node = std::make_shared<ObstacleAvoidanceNode>();
    
    g_node = node;
    g_publisher = node->get_publisher();
    signal(SIGINT, sigint_handler);
    
    rclcpp::spin(node);
    
    g_publisher.reset();
    g_node.reset();
    node.reset();
    
    return 0;
}