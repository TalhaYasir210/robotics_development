/**
 * @file test_cloud_filter.cpp
 * @brief Unit tests for the CloudFilter utility class.
 *
 * Verifies fast NaN/Inf coordinate validation, 3D axis-aligned bounding box checks,
 * Eigen affine isometry rigid body projections, and in-place vector compaction.
 */

#include <gtest/gtest.h>
#include <limits>
#include "traversability_mapping/cloud_filter.hpp"

using namespace traversability_mapping;

TEST(CloudFilterTest, NaNAndInfRejection)
{
  EXPECT_TRUE(CloudFilter::isValid(1.0f, 2.0f, 3.0f));
  EXPECT_FALSE(CloudFilter::isValid(std::numeric_limits<float>::quiet_NaN(), 2.0f, 3.0f));
  EXPECT_FALSE(CloudFilter::isValid(1.0f, std::numeric_limits<float>::infinity(), 3.0f));
  EXPECT_FALSE(CloudFilter::isValid(1.0f, 2.0f, -std::numeric_limits<float>::infinity()));
}

TEST(CloudFilterTest, BoundingBoxCheck)
{
  EXPECT_TRUE(CloudFilter::isInBoundingBox(1.0f, 0.5f, 0.2f, 0.0f, 2.0f, -1.0f, 1.0f, -0.5f, 0.5f));
  EXPECT_FALSE(CloudFilter::isInBoundingBox(2.5f, 0.5f, 0.2f, 0.0f, 2.0f, -1.0f, 1.0f, -0.5f,
    0.5f));
  EXPECT_FALSE(CloudFilter::isInBoundingBox(1.0f, -1.5f, 0.2f, 0.0f, 2.0f, -1.0f, 1.0f, -0.5f,
    0.5f));
}

TEST(CloudFilterTest, RigidTransformProjection)
{
  // Create an Isometry3f with 90-degree yaw rotation around Z and translation [1, 2, 0.5]
  Eigen::Isometry3f transform = Eigen::Isometry3f::Identity();
  transform.translate(Eigen::Vector3f(1.0f, 2.0f, 0.5f));
  transform.rotate(Eigen::AngleAxisf(M_PI_2, Eigen::Vector3f::UnitZ()));

  // Point (1, 0, 0): rotated 90 deg yaw -> (0, 1, 0), then translated -> (1, 3, 0.5)
  float ox = 0.0f, oy = 0.0f, oz = 0.0f;
  CloudFilter::transformPoint(transform, 1.0f, 0.0f, 0.0f, ox, oy, oz);

  EXPECT_NEAR(ox, 1.0f, 1e-4f);
  EXPECT_NEAR(oy, 3.0f, 1e-4f);
  EXPECT_NEAR(oz, 0.5f, 1e-4f);
}

TEST(CloudFilterTest, FilterInPlaceCompaction)
{
  std::vector<Point3D> points = {
    {0.5f, 0.0f, 0.0f},   // valid
    {5.0f, 0.0f, 0.0f},   // outside X
    {0.5f, -3.0f, 0.0f},  // outside Y
    {1.0f, 0.5f, 0.1f},   // valid
    {std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f}  // NaN
  };

  size_t remaining = CloudFilter::filterInPlace(
    points,
    0.0f, 2.0f,
    -1.0f, 1.0f,
    -0.5f, 0.5f);

  EXPECT_EQ(remaining, 2u);
  EXPECT_EQ(points.size(), 2u);
  EXPECT_NEAR(points[0].x, 0.5f, 1e-4f);
  EXPECT_NEAR(points[1].x, 1.0f, 1e-4f);
}
