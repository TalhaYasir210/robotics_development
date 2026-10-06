/**
 * @file cloud_filter.hpp
 * @brief High-performance point cloud filtering and SIMD frame projection utilities.
 *
 * Defines the CloudFilter class and Point3D struct. Provides vectorized coordinate
 * transformations via Eigen::Isometry3f, strict NaN/Inf filtering, and bounding box checks.
 */

#ifndef TRAVERSABILITY_MAPPING_CLOUD_FILTER_HPP_
#define TRAVERSABILITY_MAPPING_CLOUD_FILTER_HPP_

#include <cmath>
#include <vector>
#include <Eigen/Core>
#include <Eigen/Geometry>

namespace traversability_mapping
{

/**
 * @struct Point3D
 * @brief Plain Old Data (POD) 3D coordinate struct aligned for cache efficiency.
 */
struct Point3D
{
  float x;
  float y;
  float z;
};

/**
 * @class CloudFilter
 * @brief High-performance point cloud filtering and SIMD frame projection utility.
 *
 * Provides vectorized coordinate transformations via Eigen::Isometry3f,
 * strict NaN/Inf filtering, and bounding box checks with zero heap allocation.
 */
class CloudFilter
{
public:
  CloudFilter() = default;
  ~CloudFilter() = default;

  /**
   * @brief Fast NaN and infinity check on coordinates.
   * @return true if all coordinates are finite numbers.
   */
  static inline bool isValid(float x, float y, float z) noexcept
  {
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
  }

  /**
   * @brief Fast 3D axis-aligned bounding box test.
   */
  static inline bool isInBoundingBox(
    float x, float y, float z,
    float min_x, float max_x,
    float min_y, float max_y,
    float min_z, float max_z) noexcept
  {
    return  x >= min_x && x <= max_x &&
           y >= min_y && y <= max_y &&
           z >= min_z && z <= max_z;
  }

  /**
   * @brief Projects a single point using an affine isometry transform.
   * Leverages Eigen's vectorized Matrix-Vector multiply (SIMD).
   * @param transform Rigid body transform (rotation + translation).
   * @param in_x Source X coordinate.
   * @param in_y Source Y coordinate.
   * @param in_z Source Z coordinate.
   * @param[out] out_x Transformed X coordinate.
   * @param[out] out_y Transformed Y coordinate.
   * @param[out] out_z Transformed Z coordinate.
   */
  static inline void transformPoint(
    const Eigen::Isometry3f & transform,
    float in_x, float in_y, float in_z,
    float & out_x, float & out_y, float & out_z) noexcept
  {
    const Eigen::Vector3f pt(in_x, in_y, in_z);
    const Eigen::Vector3f transformed = transform * pt;
    out_x = transformed.x();
    out_y = transformed.y();
    out_z = transformed.z();
  }

  /**
   * @brief Combined filter, transform, and bounding box check in a single inline pass.
   * @return true if point is finite, transforms successfully, and lies within cutoff range.
   */
  static inline bool filterAndTransform(
    const Eigen::Isometry3f & transform,
    float in_x, float in_y, float in_z,
    float & out_x, float & out_y, float & out_z,
    float min_z_cutoff, float max_z_cutoff) noexcept
  {
    if (!isValid(in_x, in_y, in_z)) {
      return false;
    }

    transformPoint(transform, in_x, in_y, in_z, out_x, out_y, out_z);

    return  out_z >= min_z_cutoff && out_z <= max_z_cutoff;
  }

  /**
   * @brief Vectorized batch point projection from source array to destination array.
   * @param transform Rigid body transform.
   * @param input Input list of points.
   * @param[out] output Output list of transformed points.
   */
  static void transformBatch(
    const Eigen::Isometry3f & transform,
    const std::vector<Point3D> & input,
    std::vector<Point3D> & output);

  /**
   * @brief In-place bounding box filter on contiguous vector of points.
   * Compacts valid points to the front of the vector without reallocation.
   * @param[in,out] points Vector of 3D points.
   * @return Number of valid points remaining.
   */
  static size_t filterInPlace(
    std::vector<Point3D> & points,
    float min_x, float max_x,
    float min_y, float max_y,
    float min_z, float max_z);
};

}  // namespace traversability_mapping

#endif  // TRAVERSABILITY_MAPPING_CLOUD_FILTER_HPP_
