#include "rclcpp/rclcpp.hpp"
#include "swarm_interfaces/srv/save_qr.hpp"
#include "swarm_interfaces/srv/get_qr.hpp"
#include "swarm_interfaces/srv/save_map.hpp"
#include "swarm_interfaces/srv/get_map.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include <unordered_map>
#include <string>

class DatabaseNode : public rclcpp::Node
{
public:
    DatabaseNode() : Node("database_node"),
                     qr_logger_(this->get_logger().get_child("QR_DB")),
                     map_logger_(this->get_logger().get_child("MAP_DB"))
    {
        // Service for Bot 1 (Mapper) to save QR coordinates
        save_qr_server_ = this->create_service<swarm_interfaces::srv::SaveQR>(
            "/save_qr",
            std::bind(&DatabaseNode::handle_save_qr, this, std::placeholders::_1, std::placeholders::_2));

        // Service for Bot 2 (Explorer) to get QR coordinates
        get_qr_server_ = this->create_service<swarm_interfaces::srv::GetQR>(
            "/get_qr",
            std::bind(&DatabaseNode::handle_get_qr, this, std::placeholders::_1, std::placeholders::_2));

        // Service for Bot 1 (Mapper) to save Map
        save_map_server_ = this->create_service<swarm_interfaces::srv::SaveMap>(
            "/save_map",
            std::bind(&DatabaseNode::handle_save_map, this, std::placeholders::_1, std::placeholders::_2));

        // Service for Bot 2 (Explorer) to get Map
        get_map_server_ = this->create_service<swarm_interfaces::srv::GetMap>(
            "/get_map",
            std::bind(&DatabaseNode::handle_get_map, this, std::placeholders::_1, std::placeholders::_2));

        RCLCPP_INFO(this->get_logger(), "Backend Database Node is online. Waiting for requests...");
    }

private:
    rclcpp::Logger qr_logger_;
    rclcpp::Logger map_logger_;

    // The in-memory database storing QR IDs and their global poses
    std::unordered_map<std::string, geometry_msgs::msg::Pose> qr_database_;
    
    // Map storage
    nav_msgs::msg::OccupancyGrid latest_map_;
    bool is_mapping_complete_ = false;
    bool has_map_ = false;

    rclcpp::Service<swarm_interfaces::srv::SaveQR>::SharedPtr save_qr_server_;
    rclcpp::Service<swarm_interfaces::srv::GetQR>::SharedPtr get_qr_server_;
    rclcpp::Service<swarm_interfaces::srv::SaveMap>::SharedPtr save_map_server_;
    rclcpp::Service<swarm_interfaces::srv::GetMap>::SharedPtr get_map_server_;

    // Callback to save a QR code
    void handle_save_qr(const std::shared_ptr<swarm_interfaces::srv::SaveQR::Request> request,
                        std::shared_ptr<swarm_interfaces::srv::SaveQR::Response> response)
    {
        RCLCPP_INFO(qr_logger_, "Received request to save QR: '%s'", request->qr_id.c_str());
        
        // Save to the hash map
        qr_database_[request->qr_id] = request->pose;
        
        response->success = true;
        RCLCPP_INFO(qr_logger_, "Successfully saved '%s' at [X: %.2f, Y: %.2f]", 
                    request->qr_id.c_str(), request->pose.position.x, request->pose.position.y);
    }

    // Callback to retrieve a QR code
    void handle_get_qr(const std::shared_ptr<swarm_interfaces::srv::GetQR::Request> request,
                       std::shared_ptr<swarm_interfaces::srv::GetQR::Response> response)
    {
        RCLCPP_INFO(qr_logger_, "Received request to fetch QR: '%s'", request->qr_id.c_str());

        // Check if the QR ID exists in the hash map
        auto it = qr_database_.find(request->qr_id);
        if (it != qr_database_.end())
        {
            response->pose = it->second;
            response->success = true;
            RCLCPP_INFO(qr_logger_, "QR '%s' found. Returning coordinates.", request->qr_id.c_str());
        }
        else
        {
            response->success = false;
            RCLCPP_WARN(qr_logger_, "QR '%s' not found in database!", request->qr_id.c_str());
        }
    }

    // Callback to save map
    void handle_save_map(const std::shared_ptr<swarm_interfaces::srv::SaveMap::Request> request,
                        std::shared_ptr<swarm_interfaces::srv::SaveMap::Response> response)
    {
        RCLCPP_INFO(map_logger_, "Received request to save Map. Complete flag: %s", request->is_mapping_complete ? "TRUE" : "FALSE");
        
        latest_map_ = request->map;
        is_mapping_complete_ = request->is_mapping_complete;
        has_map_ = true;
        
        response->success = true;
    }

    // Callback to get map
    void handle_get_map(const std::shared_ptr<swarm_interfaces::srv::GetMap::Request> request,
                        std::shared_ptr<swarm_interfaces::srv::GetMap::Response> response)
    {
        // Suppress print to avoid spamming every 15s
        RCLCPP_DEBUG(map_logger_, "Received request to fetch Map.");

        if (has_map_)
        {
            response->map = latest_map_;
            response->is_mapping_complete = is_mapping_complete_;
            response->success = true;
            RCLCPP_DEBUG(map_logger_, "Map served successfully.");
        }
        else
        {
            response->success = false;
            // Only warn periodically to avoid spam
            RCLCPP_WARN_THROTTLE(map_logger_, *this->get_clock(), 5000, "Map not found in database yet!");
        }
    }
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<DatabaseNode>());
    rclcpp::shutdown();
    return 0;
}
