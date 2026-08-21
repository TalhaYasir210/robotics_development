#include "rclcpp/rclcpp.hpp"
#include "swarm_interfaces/srv/save_qr.hpp"
#include "swarm_interfaces/srv/get_qr.hpp"
#include <unordered_map>
#include <string>

class DatabaseNode : public rclcpp::Node
{
public:
    DatabaseNode() : Node("database_node")
    {
        // Service for Bot 1 (Mapper) to save QR coordinates
        save_qr_server_ = this->create_service<swarm_interfaces::srv::SaveQR>(
            "/save_qr",
            std::bind(&DatabaseNode::handle_save_qr, this, std::placeholders::_1, std::placeholders::_2));

        // Service for Bot 2 (Explorer) to get QR coordinates
        get_qr_server_ = this->create_service<swarm_interfaces::srv::GetQR>(
            "/get_qr",
            std::bind(&DatabaseNode::handle_get_qr, this, std::placeholders::_1, std::placeholders::_2));

        RCLCPP_INFO(this->get_logger(), "Backend Database Node is online. Waiting for requests...");
    }

private:
    // The in-memory database storing QR IDs and their global poses
    std::unordered_map<std::string, geometry_msgs::msg::Pose> qr_database_;

    rclcpp::Service<swarm_interfaces::srv::SaveQR>::SharedPtr save_qr_server_;
    rclcpp::Service<swarm_interfaces::srv::GetQR>::SharedPtr get_qr_server_;

    // Callback to save a QR code
    void handle_save_qr(const std::shared_ptr<swarm_interfaces::srv::SaveQR::Request> request,
                        std::shared_ptr<swarm_interfaces::srv::SaveQR::Response> response)
    {
        RCLCPP_INFO(this->get_logger(), "Received request to save QR: '%s'", request->qr_id.c_str());
        
        // Save to the hash map
        qr_database_[request->qr_id] = request->pose;
        
        response->success = true;
        RCLCPP_INFO(this->get_logger(), "Successfully saved '%s' at [X: %.2f, Y: %.2f]", 
                    request->qr_id.c_str(), request->pose.position.x, request->pose.position.y);
    }

    // Callback to retrieve a QR code
    void handle_get_qr(const std::shared_ptr<swarm_interfaces::srv::GetQR::Request> request,
                       std::shared_ptr<swarm_interfaces::srv::GetQR::Response> response)
    {
        RCLCPP_INFO(this->get_logger(), "Received request to fetch QR: '%s'", request->qr_id.c_str());

        // Check if the QR ID exists in the hash map
        auto it = qr_database_.find(request->qr_id);
        if (it != qr_database_.end())
        {
            response->pose = it->second;
            response->success = true;
            RCLCPP_INFO(this->get_logger(), "QR '%s' found. Returning coordinates.", request->qr_id.c_str());
        }
        else
        {
            response->success = false;
            RCLCPP_WARN(this->get_logger(), "QR '%s' not found in database!", request->qr_id.c_str());
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
