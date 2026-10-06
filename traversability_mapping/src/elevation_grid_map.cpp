/**
 * @file elevation_grid_map.cpp
 * @brief Pure C++ implementation of the 2.5D elevation and traversability grid map engine.
 *
 * This file implements the ElevationGridMap class. It maintains cache-aligned Structure-of-Arrays (SoA)
 * buffers for min/max elevation and point counts per cell. It performs point insertion, coordinate
 * quantization, and traversability cost computation based on height difference (delta-z) thresholds
 * with zero dynamic memory allocation during execution.
 */

#include "traversability_mapping/elevation_grid_map.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace traversability_mapping
{

ElevationGridMap::ElevationGridMap()
{
}

ElevationGridMap::ElevationGridMap(
  float grid_width,
  float grid_length,
  float resolution,
  float safe_step_threshold,
  float obstacle_threshold,
  float min_z_cutoff,
  float max_z_cutoff)
{
  initialize(
    grid_width,
    grid_length,
    resolution,
    safe_step_threshold,
    obstacle_threshold,
    min_z_cutoff,
    max_z_cutoff);
}

void ElevationGridMap::initialize(
  float grid_width,
  float grid_length,
  float resolution,
  float safe_step_threshold,
  float obstacle_threshold,
  float min_z_cutoff,
  float max_z_cutoff)
{
  grid_width_ = grid_width;
  grid_length_ = grid_length;
  resolution_ = resolution;
  half_width_ = grid_width_ / 2.0f;

  safe_step_threshold_ = safe_step_threshold;
  obstacle_threshold_ = obstacle_threshold;
  min_z_cutoff_ = min_z_cutoff;
  max_z_cutoff_ = max_z_cutoff;

  cols_ = static_cast<int>(std::round(grid_width_ / resolution_));
  rows_ = static_cast<int>(std::round(grid_length_ / resolution_));

  if (cols_ < 1) {
    cols_ = 1;
  }
  if (rows_ < 1) {
    rows_ = 1;
  }

  total_cells_ = static_cast<size_t>(rows_ * cols_);

  // Allocate all SoA buffers ONCE
  min_z_.assign(total_cells_, std::numeric_limits<float>::infinity());
  max_z_.assign(total_cells_, -std::numeric_limits<float>::infinity());
  cell_counts_.assign(total_cells_, 0);
  cost_map_.assign(total_cells_, -1);

  initialized_ = true;
}

void ElevationGridMap::reset() noexcept
{
  if (!initialized_) {
    return;
  }

  std::fill(min_z_.begin(), min_z_.end(), std::numeric_limits<float>::infinity());
  std::fill(max_z_.begin(), max_z_.end(), -std::numeric_limits<float>::infinity());
  std::fill(cell_counts_.begin(), cell_counts_.end(), static_cast<uint16_t>(0));
  std::fill(cost_map_.begin(), cost_map_.end(), static_cast<int8_t>(-1));
}

bool ElevationGridMap::addPoint(float x, float y, float z) noexcept
{
  if (!initialized_) {
    return false;
  }

  // Z-height noise and overhang filters
  if (z < min_z_cutoff_ || z > max_z_cutoff_) {
    return false;
  }

  // Forward (X) and Lateral (Y) bounds check
  if (x < 0.0f || x >= grid_length_ || y < -half_width_ || y >= half_width_) {
    return false;
  }

  // Map continuous coordinates to discrete grid cell indices
  const int row = static_cast<int>(x / resolution_);
  const int col = static_cast<int>((y + half_width_) / resolution_);

  // Boundary safety guard
  if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
    return false;
  }

  // Flattened 1D index: row * cols_ + col (O(1) direct access)
  const size_t index = static_cast<size_t>(row * cols_ + col);

  // Update Structure-of-Arrays buffers in cache-friendly fashion
  if (z < min_z_[index]) {
    min_z_[index] = z;
  }
  if (z > max_z_[index]) {
    max_z_[index] = z;
  }
  if (cell_counts_[index] < std::numeric_limits<uint16_t>::max()) {
    ++cell_counts_[index];
  }

  return true;
}

void ElevationGridMap::computeTraversability() noexcept
{
  if (!initialized_) {
    return;
  }

  const float span = obstacle_threshold_ - safe_step_threshold_;
  const float inv_span = (span > 1e-6f) ? (99.0f / span) : 0.0f;

  // Single sequential pass over contiguous cost_map_ and elevation buffers
  for (size_t i = 0; i < total_cells_; ++i) {
    if (cell_counts_[i] == 0) {
      cost_map_[i] = -1;  // Unknown / Occluded
      continue;
    }

    const float delta_z = max_z_[i] - min_z_[i];

    if (delta_z <= safe_step_threshold_) {
      cost_map_[i] = 0;  // Safe flat ground
    } else if (delta_z >= obstacle_threshold_) {
      cost_map_[i] = 100;  // Non-traversable obstacle / steep step / curb
    } else {
      // Linear normalization between safe_step and obstacle_step mapped to [1, 99]
      const float normalized = (delta_z - safe_step_threshold_) * inv_span;
      const int cost_val = static_cast<int>(std::round(normalized));
      cost_map_[i] = static_cast<int8_t>(std::clamp(cost_val, 1, 99));
    }
  }
}

bool ElevationGridMap::toGrid(float x, float y, int & row, int & col) const noexcept
{
  if (x < 0.0f || x >= grid_length_ || y < -half_width_ || y >= half_width_) {
    return false;
  }
  row = static_cast<int>(x / resolution_);
  col = static_cast<int>((y + half_width_) / resolution_);
  return  row >= 0 && row < rows_ && col >= 0 && col < cols_;
}

bool ElevationGridMap::toWorld(int row, int col, float & x, float & y) const noexcept
{
  if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
    return false;
  }
  x = (static_cast<float>(row) + 0.5f) * resolution_;
  y = -half_width_ + (static_cast<float>(col) + 0.5f) * resolution_;
  return true;
}

int8_t ElevationGridMap::getCostAt(int row, int col) const noexcept
{
  if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
    return -1;
  }
  return cost_map_[toIndex(row, col)];
}

uint16_t ElevationGridMap::getCountAt(int row, int col) const noexcept
{
  if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
    return 0;
  }
  return cell_counts_[toIndex(row, col)];
}

float ElevationGridMap::getMinZAt(int row, int col) const noexcept
{
  if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
    return std::numeric_limits<float>::infinity();
  }
  return min_z_[toIndex(row, col)];
}

float ElevationGridMap::getMaxZAt(int row, int col) const noexcept
{
  if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
    return -std::numeric_limits<float>::infinity();
  }
  return max_z_[toIndex(row, col)];
}

}  // namespace traversability_mapping
