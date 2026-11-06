#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include "my_traversability/traversability_node.hpp"

namespace my_traversability
{

class TraversabilityNodeTest : public ::testing::Test
{
protected:
  static void SetUpTestSuite()
  {
    rclcpp::init(0, nullptr);
  }

  static void TearDownTestSuite()
  {
    rclcpp::shutdown();
  }
};

// 1. Verify default parameters are declared and set properly
TEST_F(TraversabilityNodeTest, DeclaresDefaultParameters)
{
  auto node = std::make_shared<TraversabilityNode>();

  EXPECT_TRUE(node->has_parameter("grid_width"));
  EXPECT_TRUE(node->has_parameter("grid_length"));
  EXPECT_TRUE(node->has_parameter("resolution"));
  EXPECT_TRUE(node->has_parameter("target_frame"));
  EXPECT_TRUE(node->has_parameter("pointcloud_topic"));

  EXPECT_NEAR(node->get_parameter("grid_width").as_double(), 4.0, 1e-4);
  EXPECT_NEAR(node->get_parameter("grid_length").as_double(), 4.0, 1e-4);
  EXPECT_NEAR(node->get_parameter("resolution").as_double(), 0.05, 1e-4);
  EXPECT_EQ(node->get_parameter("target_frame").as_string(), "base_footprint");
  EXPECT_EQ(node->get_parameter("pointcloud_topic").as_string(), "/camera/depth/points");
}

// 2. Verify dynamic parameter reconfiguration works at runtime
TEST_F(TraversabilityNodeTest, HandlesRuntimeParameterChanges)
{
  auto node = std::make_shared<TraversabilityNode>();

  // Set new obstacle threshold at runtime
  auto result = node->set_parameter(rclcpp::Parameter("obstacle_threshold", 0.18));
  EXPECT_TRUE(result.successful);
  EXPECT_NEAR(node->get_parameter("obstacle_threshold").as_double(), 0.18, 1e-4);

  // Rejection of invalid resolution (<= 0.0)
  auto invalid_result = node->set_parameter(rclcpp::Parameter("resolution", -0.05));
  EXPECT_FALSE(invalid_result.successful);
}

}  // namespace my_traversability

