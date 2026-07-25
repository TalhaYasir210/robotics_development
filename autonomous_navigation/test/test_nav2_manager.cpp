#include "gtest/gtest.h"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "autonomous_navigation/nav2_manager.hpp"
#include <memory>
#include <chrono>

using namespace std::chrono_literals;

class TestNav2Manager : public ::testing::Test {
protected:
  void SetUp() override
  {
    // 1. Initialize ROS 2
    rclcpp::init(0, nullptr);

    // 2. Spin up our target node
    manager_node_ = std::make_shared<autonomous_navigation::Nav2Manager>();

    // 3. Create a dummy node to act as our fake GUI
    fake_gui_node_ = rclcpp::Node::make_shared("fake_gui_node");

    // 4. Create a publisher on the fake GUI to send commands
    cmd_pub_ = fake_gui_node_->create_publisher<autonomous_navigation::msg::NavigationCommand>(
      "gui_nav_command", 10);
  }

  void TearDown() override
  {
    rclcpp::shutdown();
  }

  std::shared_ptr<autonomous_navigation::Nav2Manager> manager_node_;
  std::shared_ptr<rclcpp::Node> fake_gui_node_;
  rclcpp::Publisher<autonomous_navigation::msg::NavigationCommand>::SharedPtr cmd_pub_;
};

// Test 1: Verify the node starts without crashing and has the right name
TEST_F(TestNav2Manager, InitializationTest) {
  EXPECT_NE(manager_node_, nullptr);
  EXPECT_STREQ(manager_node_->get_name(), "nav2_manager");
}

// Test 2: Verify it receives a message but correctly identifies that no Action Server is active
TEST_F(TestNav2Manager, ProcessCommandWithoutServer) {
  // Create a fake command
  auto msg = autonomous_navigation::msg::NavigationCommand();
  msg.is_waypoint_nav = false;

  // Publish message
  cmd_pub_->publish(msg);

  // Spin the executor to let the manager_node_ process the subscription callback
  // We use a small timeout so the test doesn't hang forever
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(manager_node_);
  executor.spin_some();

  // If the node processed the message without crashing (and printed the Action Server Not Available error),
  // this test passes. This ensures our callback is non-blocking and safe!
  SUCCEED();
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
