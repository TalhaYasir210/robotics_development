#ifndef SENSOR_PROCESSOR_HPP_
#define SENSOR_PROCESSOR_HPP_

#include <cmath>
#include "sensor_msgs/msg/laser_scan.hpp"

// 📦 THE DASHBOARD: A clean package of data we hand back to the Manager
struct ProcessedSensorData {
    float min_front;
    float avg_left;
    float avg_right;
    double distance_to_goal;
    double angle_error;
    bool is_valid; 

    // 🚨 NEW: Separate LaserScans for RViz Color Coding
    sensor_msgs::msg::LaserScan front_scan;
    sensor_msgs::msg::LaserScan left_scan;
    sensor_msgs::msg::LaserScan right_scan;
};

class SensorProcessor {
public:
    SensorProcessor();
    void update_odometry(double x, double y, double yaw);
    ProcessedSensorData process_scan(const sensor_msgs::msg::LaserScan::SharedPtr msg, double goal_x, double goal_y);

private:
    double current_x_;
    double current_y_;
    double current_yaw_;
    bool odom_received_;
};

#endif // SENSOR_PROCESSOR_HPP_