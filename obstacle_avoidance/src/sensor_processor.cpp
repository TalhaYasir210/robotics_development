#include "sensor_processor.hpp" 
#include <limits>
#include <algorithm>

SensorProcessor::SensorProcessor() {
    current_x_ = 0.0;
    current_y_ = 0.0;
    current_yaw_ = 0.0;
    odom_received_ = false;
}

void SensorProcessor::update_odometry(double x, double y, double yaw) {
    current_x_ = x;
    current_y_ = y;
    current_yaw_ = yaw;
    odom_received_ = true;
}

ProcessedSensorData SensorProcessor::process_scan(const sensor_msgs::msg::LaserScan::SharedPtr msg, double goal_x, double goal_y) {
    ProcessedSensorData data;
    data.is_valid = odom_received_;

    if (!odom_received_) {
        return data;
    }

    // 🚨 NEW: Copy the header/metadata to our three new visualization scans
    data.front_scan = *msg;
    data.left_scan = *msg;
    data.right_scan = *msg;

    // Erase all data initially by filling them with infinity (RViz ignores infinity)
    std::fill(data.front_scan.ranges.begin(), data.front_scan.ranges.end(), std::numeric_limits<float>::infinity());
    std::fill(data.left_scan.ranges.begin(), data.left_scan.ranges.end(), std::numeric_limits<float>::infinity());
    std::fill(data.right_scan.ranges.begin(), data.right_scan.ranges.end(), std::numeric_limits<float>::infinity());

    float min_front = 10.0;
    float sum_left = 0.0;
    int count_left = 0;
    float sum_right = 0.0;
    int count_right = 0;

    for (size_t i = 0; i < msg->ranges.size(); i++) {
        float d = msg->ranges[i];
        bool is_valid_point = !(std::isinf(d) || std::isnan(d) || d == 0.0);
        float math_d = is_valid_point ? d : 10.0; 

        // Split the data into the three separate regions
        if (i <= 30 || i >= 330) {
            if (math_d < min_front) min_front = math_d;
            if (is_valid_point) data.front_scan.ranges[i] = d; // Feed visualizer
        } 
        else if (i > 30 && i <= 90) {
            sum_left += math_d; count_left++;
            if (is_valid_point) data.left_scan.ranges[i] = d; // Feed visualizer
        } 
        else if (i >= 270 && i < 330) {
            sum_right += math_d; count_right++;
            if (is_valid_point) data.right_scan.ranges[i] = d; // Feed visualizer
        }
    }

    data.min_front = min_front;
    data.avg_left = (count_left > 0) ? (sum_left / count_left) : 0.0;
    data.avg_right = (count_right > 0) ? (sum_right / count_right) : 0.0;

    data.distance_to_goal = std::hypot(goal_x - current_x_, goal_y - current_y_);
    
    double angle_to_goal = std::atan2(goal_y - current_y_, goal_x - current_x_);
    data.angle_error = angle_to_goal - current_yaw_;
    
    while (data.angle_error > M_PI) data.angle_error -= 2.0 * M_PI;
    while (data.angle_error < -M_PI) data.angle_error += 2.0 * M_PI;

    return data;
}