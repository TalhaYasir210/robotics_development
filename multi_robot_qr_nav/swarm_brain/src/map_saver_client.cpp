#include "rclcpp/rclcpp.hpp"
#include "swarm_interfaces/srv/save_map.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "rcl_interfaces/msg/log.hpp"
#include <chrono>

using namespace std::chrono_literals;

class MapSaverClient : public rclcpp::Node
{
public:
    MapSaverClient() : Node("map_saver_client")
    {
        // Service client
        save_map_client_ = this->create_client<swarm_interfaces::srv::SaveMap>("/save_map");

        // Subscribers
        map_sub_ = this->create_subscription<nav_msgs::msg::OccupancyGrid>(
            "/map", 10, std::bind(&MapSaverClient::map_callback, this, std::placeholders::_1));

        rosout_sub_ = this->create_subscription<rcl_interfaces::msg::Log>(
            "/rosout", 10, std::bind(&MapSaverClient::rosout_callback, this, std::placeholders::_1));

        // Timer (15 seconds)
        timer_ = this->create_wall_timer(
            15s, std::bind(&MapSaverClient::timer_callback, this));

        RCLCPP_INFO(this->get_logger(), "Map Saver Client started. Syncing /map to database every 15s.");
    }

private:
    rclcpp::Client<swarm_interfaces::srv::SaveMap>::SharedPtr save_map_client_;
    rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr map_sub_;
    rclcpp::Subscription<rcl_interfaces::msg::Log>::SharedPtr rosout_sub_;
    rclcpp::TimerBase::SharedPtr timer_;

    nav_msgs::msg::OccupancyGrid latest_map_;
    bool has_map_ = false;
    bool is_mapping_complete_ = false;
    bool stopped_ = false;

    void map_callback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg)
    {
        latest_map_ = *msg;
        has_map_ = true;
    }

    void rosout_callback(const rcl_interfaces::msg::Log::SharedPtr msg)
    {
        if (stopped_) return;

        // Check if message is from explore_node and contains "no frontier" (or similar)
        // Usually explore_lite logs something like "No frontiers found. Exploring finished."
        if (msg->name == "explore_node" || msg->name == "tb3_1.explore_node")
        {
            std::string log_msg = msg->msg;
            // lower case conversion for robust check
            std::transform(log_msg.begin(), log_msg.end(), log_msg.begin(),
                           [](unsigned char c){ return std::tolower(c); });

            if (log_msg.find("no frontier") != std::string::npos || log_msg.find("exploring finished") != std::string::npos)
            {
                if (!is_mapping_complete_)
                {
                    RCLCPP_INFO(this->get_logger(), "Detected exploration completion from /rosout!");
                    is_mapping_complete_ = true;
                    // Trigger one immediate final save
                    timer_callback();
                }
            }
        }
    }

    void timer_callback()
    {
        if (stopped_ || !has_map_)
        {
            return;
        }

        if (!save_map_client_->wait_for_service(1s))
        {
            RCLCPP_WARN(this->get_logger(), "Database /save_map service not available, skipping sync...");
            return;
        }

        auto request = std::make_shared<swarm_interfaces::srv::SaveMap::Request>();
        request->map = latest_map_;
        request->is_mapping_complete = is_mapping_complete_;

        // Use async send request
        save_map_client_->async_send_request(request, 
            [this](rclcpp::Client<swarm_interfaces::srv::SaveMap>::SharedFuture future) {
                if (future.get()->success) {
                    RCLCPP_INFO(this->get_logger(), "Successfully synced map to database. (Complete: %d)", this->is_mapping_complete_);
                } else {
                    RCLCPP_ERROR(this->get_logger(), "Failed to sync map to database.");
                }
            });

        if (is_mapping_complete_)
        {
            RCLCPP_INFO(this->get_logger(), "Final map sent. Stopping timer.");
            timer_->cancel();
            stopped_ = true;
        }
    }
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MapSaverClient>());
    rclcpp::shutdown();
    return 0;
}
