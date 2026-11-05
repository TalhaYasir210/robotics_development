#include "my_traversability/elevation_grid_map.hpp"

#include <cmath>
#include <limits>
#include <algorithm>
#include <stdexcept>

namespace my_traversability
{

// 1. Default Constructor: Relies on class in-class member initializers, then initializes memory buffers
ElevationGridMap::ElevationGridMap()
{
  initialize();
}

// 2. Parameterized Constructor: Uses initializer list to configure metric extents & thresholds
ElevationGridMap::ElevationGridMap(
  float grid_width,
  float grid_length,
  float resolution,
  float safe_step_threshold,
  float obstacle_threshold,
  float min_z_cutoff,
  float max_z_cutoff)
: grid_width_(grid_width),
  grid_length_(grid_length),
  resolution_(resolution),
  half_width_(grid_width * 0.5f),
  safe_step_threshold_(safe_step_threshold),
  obstacle_threshold_(obstacle_threshold),
  min_z_cutoff_(min_z_cutoff),
  max_z_cutoff_(max_z_cutoff)
{
  initialize();
}

// 3. initialize(): One-time heap allocation. Pre-sizes all Structure-of-Arrays (SoA) buffers.
void ElevationGridMap::initialize()
{
  if (grid_width_ <= 0.0f || grid_length_ <= 0.0f || resolution_ <= 0.0f) {
    throw std::invalid_argument("Map length, width, and resolution must be positive non-zero values.");
  }

  half_width_ = grid_width_ * 0.5f;

  // Calculate grid cell counts along X (rows) and Y (cols)
  rows_ = static_cast<int>(std::ceil(grid_length_ / resolution_));
  cols_ = static_cast<int>(std::ceil(grid_width_ / resolution_));
  total_cells_ = static_cast<size_t>(rows_ * cols_);

  // One-time contiguous buffer allocation:
  // - min_z_ starts at +infinity so any real sensor point pulls it down
  // - max_z_ starts at -infinity so any real sensor point pushes it up
  // - cell_counts_ starts at 0 hits
  // - cost_map_ starts at -1 (Nav2 unknown cell standard)
  min_z_.assign(total_cells_, std::numeric_limits<float>::infinity());
  max_z_.assign(total_cells_, -std::numeric_limits<float>::infinity());
  cell_counts_.assign(total_cells_, 0U);
  cost_map_.assign(total_cells_, static_cast<int8_t>(-1));

  initialized_ = true;
}

// 4. reset(): Zero-allocation in-place buffer wipe called at 30 Hz before streaming the next point cloud
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

// 5. addPoint(): Converts metric point into grid index and updates elevation bounds in-place
bool ElevationGridMap::addPoint(float x, float y, float z) noexcept
{
  // Filter out noisy points below ground or above robot clearance
  if (z < min_z_cutoff_ || z > max_z_cutoff_) {
    return false;
  }

  int row = 0;
  int col = 0;
  if (!toGridCoordinates(x, y, row, col)) {
    return false;
  }

  const size_t index = toIndex(row, col);

  // Update min/max elevations without conditional branching
  min_z_[index] = std::min(min_z_[index], z);
  max_z_[index] = std::max(max_z_[index], z);

  // Guard counter against 16-bit unsigned overflow
  if (cell_counts_[index] < std::numeric_limits<uint16_t>::max()) {
    ++cell_counts_[index];
  }

  return true;
}

// 6. computeTraversability(): Evaluates Delta-Z and maps to standard Nav2 occupancy costs (0, 1-99, 100)
void ElevationGridMap::computeTraversability() noexcept
{
  if (!initialized_) {
    return;
  }

  const float range = obstacle_threshold_ - safe_step_threshold_;
  const float inv_range = (range > 1e-5f) ? (1.0f / range) : 0.0f;

  for (size_t i = 0; i < total_cells_; ++i) {
    // If no points hit this cell, leave it as -1 (unknown)
    if (cell_counts_[i] == 0U) {
      cost_map_[i] = static_cast<int8_t>(-1);
      continue;
    }

    const float delta_z = max_z_[i] - min_z_[i];

    // Flat floor: safe to traverse smoothly
    if (delta_z <= safe_step_threshold_) {
      cost_map_[i] = static_cast<int8_t>(0);
    }
    // High obstacle or cliff: impassable
    else if (delta_z >= obstacle_threshold_) {
      cost_map_[i] = static_cast<int8_t>(100);
    }
    // Moderate roughness / ramp: linear interpolation between 1 and 99
    else {
      const float normalized = (delta_z - safe_step_threshold_) * inv_range;
      const int cost = 1 + static_cast<int>(std::round(normalized * 98.0f));
      cost_map_[i] = static_cast<int8_t>(std::clamp(cost, 1, 99));
    }
  }
}

}  // namespace my_traversability