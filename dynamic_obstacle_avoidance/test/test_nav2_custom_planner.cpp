#include <gtest/gtest.h>
#include "dynamic_obstacle_avoidance/nav2_custom_planner.hpp"
#include "rclcpp/rclcpp.hpp"

TEST(Nav2CustomPlannerTest, initialization) {
    // Basic initialization test to ensure the plugin class can be instantiated
    EXPECT_NO_THROW({
        dynamic_obstacle_avoidance::Nav2CustomPlanner planner;
    });
}

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    rclcpp::shutdown();
    return result;
}
