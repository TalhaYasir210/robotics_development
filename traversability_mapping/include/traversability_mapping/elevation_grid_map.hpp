/**
 * @file elevation_grid_map.hpp
 * @brief Pure C++ class definition for the 2.5D elevation and traversability grid map engine.
 *
 * Defines the ElevationGridMap class using a Structure-of-Arrays (SoA) layout with
 * zero heap allocations in the real-time update loop. Provides coordinate projection,
 * point accumulation, and delta-Z traversability cost calculation.
 */

#ifndef TRAVERSABILITY_MAPPING_ELEVATION_GRID_MAP_HPP_
#define TRAVERSABILITY_MAPPING_ELEVATION_GRID_MAP_HPP_

#include <cstddef>
#include <cstdint>
#include <vector>
#include <limits>
#include <algorithm>
#include <cmath>

namespace traversability_mapping
{

/**
 * @class ElevationGridMap
 * @brief Pure C++ 2.5D Elevation and Traversability Engine.
 *
 * Implements a zero-dynamic-allocation, Structure-of-Arrays (SoA) elevation grid map.
 * Operates completely independently of ROS middleware to facilitate deterministic
 * unit testing, high cache locality, and SIMD/compiler auto-vectorization.
 */
class ElevationGridMap
{
public:
  /**
   * @brief Default constructor. Buffers remain unallocated until initialize() is called.
   */
  ElevationGridMap();

  /**
   * @brief Parameterized constructor initializing map geometry and thresholds.
   * @param grid_width Lateral dimension in meters (Y axis, from -width/2 to +width/2).
   * @param grid_length Forward dimension in meters (X axis, from 0 to +length).
   * @param resolution Cell grid size in meters per cell.
   * @param safe_step_threshold Elevation delta below which terrain is classified safe (cost 0).
   * @param obstacle_threshold Elevation delta above which terrain is classified an obstacle (cost 100).
   * @param min_z_cutoff Minimum vertical cutoff in base_link frame (ground noise filter).
   * @param max_z_cutoff Maximum vertical cutoff in base_link frame (ceiling/overhang filter).
   */
  ElevationGridMap(
    float grid_width,
    float grid_length,
    float resolution,
    float safe_step_threshold,
    float obstacle_threshold,
    float min_z_cutoff,
    float max_z_cutoff);

  ~ElevationGridMap() = default;

  // Move-constructible and move-assignable, disable copying to prevent expensive heap clones
  ElevationGridMap(ElevationGridMap &&) noexcept = default;
  ElevationGridMap & operator=(ElevationGridMap &&) noexcept = default;
  ElevationGridMap(const ElevationGridMap &) = delete;
  ElevationGridMap & operator=(const ElevationGridMap &) = delete;

  /**
   * @brief Allocates SoA memory buffers once according to map dimensions.
   * Guarantees zero heap allocation in subsequent reset/insert/compute calls.
   */
  void initialize(
    float grid_width,
    float grid_length,
    float resolution,
    float safe_step_threshold,
    float obstacle_threshold,
    float min_z_cutoff,
    float max_z_cutoff);

  /**
   * @brief Fast in-place buffer reset via std::fill.
   * Resets min_z to +infinity, max_z to -infinity, cell_counts to 0, cost_map to -1.
   */
  void reset() noexcept;

  /**
   * @brief Inserts a 3D point in the robot base frame into the elevation grid.
   * Performs bounds and cutoff checks, maps coordinates to (row, col),
   * and updates the corresponding SoA buffers in O(1) constant time.
   * @param x Forward coordinate in meters.
   * @param y Lateral coordinate in meters.
   * @param z Vertical coordinate in meters.
   * @return true if point falls within bounds and vertical cutoff; false otherwise.
   */
  bool addPoint(float x, float y, float z) noexcept;

  /**
   * @brief Computes traversability cost for all grid cells in a contiguous pass.
   *
   * Logic:
   * - count == 0: Cost -1 (Unknown / Occluded)
   * - delta_z <= safe_step: Cost 0 (Safe floor)
   * - delta_z >= obstacle_step: Cost 100 (Obstacle / Step / Curb)
   * - Otherwise: Cost 1 to 99 via linear normalization:
   *     cost = static_cast<int8_t>(((delta_z - safe_step) / (obstacle_step - safe_step)) * 99.0f)
   */
  void computeTraversability() noexcept;

  // Const Accessors (Zero-Copy)
  [[nodiscard]] const std::vector<float> & getMinZ() const noexcept {return min_z_;}
  [[nodiscard]] const std::vector<float> & getMaxZ() const noexcept {return max_z_;}
  [[nodiscard]] const std::vector<uint16_t> & getCellCounts() const noexcept {return cell_counts_;}
  [[nodiscard]] const std::vector<int8_t> & getCostMap() const noexcept {return cost_map_;}

  [[nodiscard]] int getRows() const noexcept {return rows_;}
  [[nodiscard]] int getCols() const noexcept {return cols_;}
  [[nodiscard]] size_t getTotalCells() const noexcept {return total_cells_;}
  [[nodiscard]] float getResolution() const noexcept {return resolution_;}
  [[nodiscard]] float getGridWidth() const noexcept {return grid_width_;}
  [[nodiscard]] float getGridLength() const noexcept {return grid_length_;}
  [[nodiscard]] float getHalfWidth() const noexcept {return half_width_;}
  [[nodiscard]] float getSafeStepThreshold() const noexcept {return safe_step_threshold_;}
  [[nodiscard]] float getObstacleThreshold() const noexcept {return obstacle_threshold_;}
  [[nodiscard]] float getMinZCutoff() const noexcept {return min_z_cutoff_;}
  [[nodiscard]] float getMaxZCutoff() const noexcept {return max_z_cutoff_;}
  [[nodiscard]] bool isInitialized() const noexcept {return initialized_;}

  /**
   * @brief Converts continuous (x, y) coordinates to discrete (row, col) cell coordinates.
   * @param x Forward coordinate (meters).
   * @param y Lateral coordinate (meters).
   * @param[out] row Output row index [0, rows_-1].
   * @param[out] col Output column index [0, cols_-1].
   * @return true if (x, y) is inside grid boundaries.
   */
  [[nodiscard]] bool toGrid(float x, float y, int & row, int & col) const noexcept;

  /**
   * @brief Converts discrete (row, col) cell coordinates to cell center world coordinates (x, y).
   * @param row Row index [0, rows_-1].
   * @param col Column index [0, cols_-1].
   * @param[out] x Output forward coordinate (meters).
   * @param[out] y Output lateral coordinate (meters).
   * @return true if (row, col) is within grid dimensions.
   */
  [[nodiscard]] bool toWorld(int row, int col, float & x, float & y) const noexcept;

  /**
   * @brief Computes 1D flattened index from 2D coordinates: row * cols_ + col.
   */
  [[nodiscard]] inline size_t toIndex(int row, int col) const noexcept
  {
    return static_cast<size_t>(row * cols_ + col);
  }

  /**
   * @brief Queries traversability cost at specific (row, col).
   */
  [[nodiscard]] int8_t getCostAt(int row, int col) const noexcept;

  /**
   * @brief Queries cell count at specific (row, col).
   */
  [[nodiscard]] uint16_t getCountAt(int row, int col) const noexcept;

  /**
   * @brief Queries min elevation at specific (row, col).
   */
  [[nodiscard]] float getMinZAt(int row, int col) const noexcept;

  /**
   * @brief Queries max elevation at specific (row, col).
   */
  [[nodiscard]] float getMaxZAt(int row, int col) const noexcept;

private:
  // Map Geometry Configuration
  float grid_width_{4.0f};
  float grid_length_{4.0f};
  float resolution_{0.05f};
  float half_width_{2.0f};
  int rows_{0};
  int cols_{0};
  size_t total_cells_{0};

  // Traversability Thresholds
  float safe_step_threshold_{0.04f};
  float obstacle_threshold_{0.12f};
  float min_z_cutoff_{-0.15f};
  float max_z_cutoff_{1.50f};

  // Pre-allocated flat Structure of Arrays (SoA) memory buffers
  std::vector<float> min_z_;
  std::vector<float> max_z_;
  std::vector<uint16_t> cell_counts_;
  std::vector<int8_t> cost_map_;

  bool initialized_{false};
};

}  // namespace traversability_mapping

#endif  // TRAVERSABILITY_MAPPING_ELEVATION_GRID_MAP_HPP_
