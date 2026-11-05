#include <gtest/gtest.h>
#include <cmath>
#include <Eigen/Geometry>
#include "my_traversability/cloud_filter.hpp"

using namespace my_traversability;

// Test 1: Verifies that NaN and Infinity values are caught
TEST(CloudFilterTest, ValidPointsDetection)
{
  // 1. Normal real point should be TRUE
  EXPECT_TRUE(CloudFilter::isValid(1.0f, 2.0f, 0.5f));
  // 2. Point with NaN should be FALSE
  EXPECT_FALSE(CloudFilter::isValid(NAN, 2.0f, 0.5f));
  // 3. Point with Infinity should be FALSE
  EXPECT_FALSE(CloudFilter::isValid(1.0f, INFINITY, 0.5f));
}
// Test 2: Verifies our 3D Bounding Box logic
TEST(CloudFilterTest, BoundingBoxCheck)
{
  // Box bounds: X: [0 to 4], Y: [-2 to 2], Z: [-0.5 to 1.5]
  
  // Point inside the box -> TRUE
  EXPECT_TRUE(CloudFilter::isInBoundingBox(2.0f, 0.0f, 0.5f, 0.0f, 4.0f, -2.0f, 2.0f, -0.5f, 1.5f));
  // Point outside in X (5.0 > 4.0) -> FALSE
  EXPECT_FALSE(CloudFilter::isInBoundingBox(5.0f, 0.0f, 0.5f, 0.0f, 4.0f, -2.0f, 2.0f, -0.5f, 1.5f));
}
// Test 3: Verifies coordinate transformation
TEST(CloudFilterTest, TransformPointCheck)
{
  // Create a transform that translates +1 meter in X and +2 meters in Y
  Eigen::Isometry3f transform = Eigen::Isometry3f::Identity();
  transform.translation() = Eigen::Vector3f(1.0f, 2.0f, 0.0f);
  float in_x = 0.5f, in_y = 1.0f, in_z = 2.0f;
  float out_x = 0.0f, out_y = 0.0f, out_z = 0.0f;
  CloudFilter::transformPoint(transform, in_x, in_y, in_z, out_x, out_y, out_z);
  // 0.5 + 1.0 = 1.5
  EXPECT_FLOAT_EQ(out_x, 1.5f);
  // 1.0 + 2.0 = 3.0
  EXPECT_FLOAT_EQ(out_y, 3.0f);
  // 2.0 + 0.0 = 2.0
  EXPECT_FLOAT_EQ(out_z, 2.0f);
}
