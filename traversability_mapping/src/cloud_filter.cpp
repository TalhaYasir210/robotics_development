/**
 * @file cloud_filter.cpp
 * @brief Implementation of SIMD-accelerated point cloud transformations and in-place filters.
 *
 * This file implements batch coordinate transformations via Eigen affine isometries and
 * in-place bounding box compaction routines for 3D point cloud structures without heap reallocation.
 */

#include "traversability_mapping/cloud_filter.hpp"

#include <algorithm>

namespace traversability_mapping
{

void CloudFilter::transformBatch(
  const Eigen::Isometry3f & transform,
  const std::vector<Point3D> & input,
  std::vector<Point3D> & output)
{
  output.resize(input.size());

  // Direct matrix and translation access for unrolled vectorization
  const auto & linear = transform.linear();
  const auto & translation = transform.translation();

  const size_t n = input.size();
  for (size_t i = 0; i < n; ++i) {
    const auto & pt = input[i];
    if (!isValid(pt.x, pt.y, pt.z)) {
      output[i] = {pt.x, pt.y, pt.z};
      continue;
    }

    const float ox = linear(0, 0) * pt.x + linear(0, 1) * pt.y + linear(0,
        2) * pt.z + translation.x();
    const float oy = linear(1, 0) * pt.x + linear(1, 1) * pt.y + linear(1,
        2) * pt.z + translation.y();
    const float oz = linear(2, 0) * pt.x + linear(2, 1) * pt.y + linear(2,
        2) * pt.z + translation.z();

    output[i] = {ox, oy, oz};
  }
}

size_t CloudFilter::filterInPlace(
  std::vector<Point3D> & points,
  float min_x, float max_x,
  float min_y, float max_y,
  float min_z, float max_z)
{
  size_t valid_count = 0;
  const size_t total = points.size();

  for (size_t i = 0; i < total; ++i) {
    const auto & p = points[i];
    if (isValid(p.x, p.y, p.z) &&
      isInBoundingBox(p.x, p.y, p.z, min_x, max_x, min_y, max_y, min_z, max_z))
    {
      if (valid_count != i) {
        points[valid_count] = p;
      }
      ++valid_count;
    }
  }

  // Truncate size without deallocating underlying buffer
  points.resize(valid_count);
  return valid_count;
}

}  // namespace traversability_mapping
