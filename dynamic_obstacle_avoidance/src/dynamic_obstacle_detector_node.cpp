#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include <vector>
#include <cmath>

using std::placeholders::_1;

struct Point2D {
    double x;
    double y;
};

struct Cluster {
    int id;
    double centroid_x;
    double centroid_y;
    double abs_vel_x;
    double abs_vel_y;
    int age;
};

class DynamicObstacleDetectorNode : public rclcpp::Node {
public:
    DynamicObstacleDetectorNode() : Node("dynamic_obstacle_detector_node"), brakes_active_(false), last_scan_time_(0) {
        // Parameters
        this->declare_parameter<double>("danger_zone_x_min", 0.1);
        this->declare_parameter<double>("danger_zone_x_max", 2.0);
        this->declare_parameter<double>("danger_zone_y_width", 1.2); // +/- 0.6m
        this->declare_parameter<double>("dynamic_velocity_threshold", 0.15); // m/s
        this->declare_parameter<double>("cluster_tolerance", 0.3); // m
        
        // Subscribers
        scan_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
            "/scan", rclcpp::SensorDataQoS(), std::bind(&DynamicObstacleDetectorNode::scanCallback, this, _1));
            
        odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
            "/odom", rclcpp::SensorDataQoS(), std::bind(&DynamicObstacleDetectorNode::odomCallback, this, _1));
            
        cmd_vel_nav_sub_ = this->create_subscription<geometry_msgs::msg::TwistStamped>(
            "/cmd_vel_nav", 10, std::bind(&DynamicObstacleDetectorNode::cmdVelCallback, this, _1));
            
        // Publisher
        cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("/cmd_vel", 10);
        
        next_cluster_id_ = 0;
        
        RCLCPP_INFO(this->get_logger(), "Dynamic Obstacle Detector Node Started!");
    }

private:
    void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg) {
        robot_vx_ = msg->twist.twist.linear.x;
        robot_vy_ = msg->twist.twist.linear.y;
        robot_omega_ = msg->twist.twist.angular.z;
    }

    void scanCallback(const sensor_msgs::msg::LaserScan::SharedPtr msg) {
        double current_time = msg->header.stamp.sec + msg->header.stamp.nanosec * 1e-9;
        if (last_scan_time_ == 0) {
            last_scan_time_ = current_time;
            return;
        }
        
        double dt = current_time - last_scan_time_;
        if (dt <= 0.0) return;
        last_scan_time_ = current_time;

        std::vector<Point2D> points;
        
        // 1. Convert to Cartesian and filter
        for (size_t i = 0; i < msg->ranges.size(); ++i) {
            double r = msg->ranges[i];
            if (r < msg->range_min || r > msg->range_max || !std::isfinite(r)) continue;
            
            double angle = msg->angle_min + i * msg->angle_increment;
            
            // Only consider points in front FOV (-90 to 90 degrees approx)
            if (angle > -1.57 && angle < 1.57) {
                Point2D p;
                p.x = r * std::cos(angle);
                p.y = r * std::sin(angle);
                
                // Pre-filter: Only keep points within a reasonable tracking range (e.g. 0 to 5.0m)
                if (p.x > 0.0 && p.x < 5.0 && std::abs(p.y) < 3.0) {
                    points.push_back(p);
                }
            }
        }

        // 2. Simple Euclidean Clustering
        double cluster_tolerance = this->get_parameter("cluster_tolerance").as_double();
        std::vector<std::vector<Point2D>> clusters_points;
        if (!points.empty()) {
            std::vector<Point2D> current_cluster_pts;
            current_cluster_pts.push_back(points[0]);
            
            for (size_t i = 1; i < points.size(); ++i) {
                double dx = points[i].x - points[i-1].x;
                double dy = points[i].y - points[i-1].y;
                double dist = std::hypot(dx, dy);
                
                if (dist < cluster_tolerance) {
                    current_cluster_pts.push_back(points[i]);
                } else {
                    clusters_points.push_back(current_cluster_pts);
                    current_cluster_pts.clear();
                    current_cluster_pts.push_back(points[i]);
                }
            }
            if (!current_cluster_pts.empty()) {
                clusters_points.push_back(current_cluster_pts);
            }
        }

        // 3. Compute Centroids
        std::vector<Cluster> new_clusters;
        for (const auto& c_pts : clusters_points) {
            if (c_pts.size() < 3) continue; // Ignore very small clusters (noise)
            
            double sum_x = 0, sum_y = 0;
            for (const auto& p : c_pts) {
                sum_x += p.x;
                sum_y += p.y;
            }
            Cluster c;
            c.centroid_x = sum_x / c_pts.size();
            c.centroid_y = sum_y / c_pts.size();
            c.id = -1;
            c.age = 0;
            c.abs_vel_x = 0.0;
            c.abs_vel_y = 0.0;
            new_clusters.push_back(c);
        }

        // 4. Tracking and Velocity Calculation
        bool dynamic_obstacle_in_danger_zone = false;
        double danger_x_min = this->get_parameter("danger_zone_x_min").as_double();
        double danger_x_max = this->get_parameter("danger_zone_x_max").as_double();
        double danger_y_width = this->get_parameter("danger_zone_y_width").as_double();
        double dyn_thresh = this->get_parameter("dynamic_velocity_threshold").as_double();

        for (auto& new_c : new_clusters) {
            double best_dist = 0.5; // Max match distance reduced to 0.5m to prevent false tracking of suddenly placed objects
            int best_match_idx = -1;
            
            for (size_t j = 0; j < prev_clusters_.size(); ++j) {
                double dist = std::hypot(new_c.centroid_x - prev_clusters_[j].centroid_x, 
                                         new_c.centroid_y - prev_clusters_[j].centroid_y);
                if (dist < best_dist) {
                    best_dist = dist;
                    best_match_idx = j;
                }
            }
            
            if (best_match_idx != -1) {
                new_c.id = prev_clusters_[best_match_idx].id;
                new_c.age = prev_clusters_[best_match_idx].age + 1;
                
                // Relative velocity
                double vx_rel = (new_c.centroid_x - prev_clusters_[best_match_idx].centroid_x) / dt;
                double vy_rel = (new_c.centroid_y - prev_clusters_[best_match_idx].centroid_y) / dt;
                
                // Absolute velocity estimation
                // v_abs = v_rel + v_robot + w_robot x r
                new_c.abs_vel_x = vx_rel + robot_vx_ - robot_omega_ * new_c.centroid_y;
                new_c.abs_vel_y = vy_rel + robot_vy_ + robot_omega_ * new_c.centroid_x;
                
                // Smooth velocity slightly
                new_c.abs_vel_x = 0.7 * prev_clusters_[best_match_idx].abs_vel_x + 0.3 * new_c.abs_vel_x;
                new_c.abs_vel_y = 0.7 * prev_clusters_[best_match_idx].abs_vel_y + 0.3 * new_c.abs_vel_y;
            } else {
                new_c.id = next_cluster_id_++;
            }
            
            // 5. Check Danger Zone
            double abs_speed = std::hypot(new_c.abs_vel_x, new_c.abs_vel_y);
            bool in_danger_zone = (new_c.centroid_x > danger_x_min && new_c.centroid_x < danger_x_max && 
                                   std::abs(new_c.centroid_y) < (danger_y_width / 2.0));
                                   
            // Require consistent tracking (age >= 4) and ignore crazy teleportations (speed < 3.0)
            if (in_danger_zone && new_c.age >= 4 && abs_speed > dyn_thresh && abs_speed < 3.0) {
                dynamic_obstacle_in_danger_zone = true;
                RCLCPP_WARN(this->get_logger(), "Dynamic Obstacle Detected! Speed: %.2f m/s, Dist: %.2f m. BRAKING!", 
                            abs_speed, new_c.centroid_x);
            }
        }
        
        brakes_active_ = dynamic_obstacle_in_danger_zone;
        prev_clusters_ = new_clusters;
    }

    void cmdVelCallback(const geometry_msgs::msg::TwistStamped::SharedPtr msg) {
        if (brakes_active_) {
            geometry_msgs::msg::TwistStamped stop_msg = *msg;
            stop_msg.twist.linear.x = 0.0;
            stop_msg.twist.linear.y = 0.0;
            stop_msg.twist.angular.z = 0.0;
            cmd_vel_pub_->publish(stop_msg);
        } else {
            cmd_vel_pub_->publish(*msg);
        }
    }

    rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr scan_sub_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
    rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_vel_nav_sub_;
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_vel_pub_;

    bool brakes_active_;
    double last_scan_time_;
    
    double robot_vx_ = 0.0;
    double robot_vy_ = 0.0;
    double robot_omega_ = 0.0;
    
    int next_cluster_id_;
    std::vector<Cluster> prev_clusters_;
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<DynamicObstacleDetectorNode>());
    rclcpp::shutdown();
    return 0;
}
