#ifndef MY_TRAVERSABILITY_ELEVATION_GRID_MAP_HPP_
#define MY_TRAVERSABILITY_ELEVATION_GRID_MAP_HPP_

#include <vector>
#include <cmath>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <cstddef>

namespace my_traversability
{

class ElevationGridMap
{
private:
    // Metric geometry (ROS REP-103: X = Forward, Y = Left, Z = Up)
    float grid_width_{4.0f};       // Total lateral span along Y (-half_width_ to +half_width_)
    float grid_length_{4.0f};      // Total longitudinal span along X (0.0 to grid_length_)
    float resolution_{0.05f};      // Cell size in meters (5 cm)
    float half_width_{2.0f};

    // Discrete grid geometry aligned with ROS OccupancyGrid standard:
    // width_x_  = cells along X-axis (forward)
    // height_y_ = cells along Y-axis (lateral)
    int width_x_{0};
    int height_y_{0};
    size_t total_cells_{0};

    // Traversability metric thresholds (meters)
    float safe_step_threshold_{0.06f};   // <= 6 cm: Cost 0 (Flat/Traversable)
    float obstacle_threshold_{0.12f};    // >= 12 cm: Cost 100 (Lethal)
    float min_z_cutoff_{-0.15f};         // Below wheel contact plane
    float max_z_cutoff_{1.05f};          // Overhead clearance limit

    bool initialized_{false};

    // Structure-of-Arrays (SoA) contiguous memory buffers
    std::vector<float> min_z_;
    std::vector<float> max_z_;
    std::vector<uint16_t> cell_counts_;
    std::vector<int8_t> cost_map_;

public:
    // Constructors & Destructor
    ElevationGridMap();
    ElevationGridMap(
        float grid_width,
        float grid_length,
        float resolution,
        float safe_step_threshold,
        float obstacle_threshold,
        float min_z_cutoff,
        float max_z_cutoff);
    ~ElevationGridMap() = default;

    // Rule of 5: Ban expensive buffer clones, allow cheap moves
    ElevationGridMap(const ElevationGridMap &) = delete;
    ElevationGridMap & operator=(const ElevationGridMap &) = delete;
    ElevationGridMap(ElevationGridMap &&) noexcept = default;
    ElevationGridMap & operator=(ElevationGridMap &&) noexcept = default;

    // Lifecycle & Processing Pipeline
    void initialize();
    void reset() noexcept;
    bool addPoint(float x, float y, float z) noexcept;
    void computeTraversability() noexcept;

    // Index & Coordinate Helpers (Standard ROS OccupancyGrid Row-Major: index = iy * width_x + ix)
    inline size_t toIndex(int ix, int iy) const noexcept
    {
        return static_cast<size_t>(iy * width_x_ + ix);
    }

    inline bool toGridCoordinates(float x, float y, int & ix, int & iy) const noexcept
    {
        // X maps to forward cells [0, grid_length_]
        if (x < 0.0f || x >= grid_length_) {
            return false;
        }

        // Y maps to lateral cells [-half_width_, +half_width_]
        if (y < -half_width_ || y >= half_width_) {
            return false;
        }

        ix = static_cast<int>(x / resolution_);
        iy = static_cast<int>((y + half_width_) / resolution_);

        return (ix >= 0 && ix < width_x_ && iy >= 0 && iy < height_y_);
    }

    inline void getCellCenter(size_t index, float & x, float & y) const noexcept
    {
        const int ix = static_cast<int>(index % static_cast<size_t>(width_x_));
        const int iy = static_cast<int>(index / static_cast<size_t>(width_x_));
        x = (static_cast<float>(ix) + 0.5f) * resolution_;
        y = -half_width_ + (static_cast<float>(iy) + 0.5f) * resolution_;
    }

    // Getters for ROS Node Publishing and Testing
    inline bool isInitialized() const noexcept { return initialized_; }
    inline int getWidthX() const noexcept { return width_x_; }
    inline int getHeightY() const noexcept { return height_y_; }
    inline int getRows() const noexcept { return height_y_; }  // Compatibility
    inline int getCols() const noexcept { return width_x_; }   // Compatibility
    inline size_t getTotalCells() const noexcept { return total_cells_; }
    inline float getResolution() const noexcept { return resolution_; }
    inline float getGridWidth() const noexcept { return grid_width_; }
    inline float getGridLength() const noexcept { return grid_length_; }
    inline float getHalfWidth() const noexcept { return half_width_; }

    inline const std::vector<int8_t> & getCostMap() const noexcept { return cost_map_; }
    inline const std::vector<float> & getMinZ() const noexcept { return min_z_; }
    inline const std::vector<float> & getMaxZ() const noexcept { return max_z_; }
    inline const std::vector<uint16_t> & getCellCounts() const noexcept { return cell_counts_; }
};

}  // namespace my_traversability

#endif  // MY_TRAVERSABILITY_ELEVATION_GRID_MAP_HPP_
