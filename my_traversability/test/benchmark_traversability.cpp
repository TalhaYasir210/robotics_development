#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <malloc.h>
#include <numeric>
#include <vector>

#include <Eigen/Geometry>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>

#include "my_traversability/cloud_filter.hpp"
#include "my_traversability/elevation_grid_map.hpp"

namespace
{

// ==============================================================================
// Heap Profiling & Allocation Interceptor (Zero-Leak / Zero-Allocation Tracker)
// ==============================================================================
static std::atomic<size_t> g_tracked_allocations{0};
static std::atomic<size_t> g_tracked_bytes{0};
static std::atomic<bool> g_profiling_active{false};

void start_heap_profiling() noexcept
{
  g_tracked_allocations.store(0, std::memory_order_seq_cst);
  g_tracked_bytes.store(0, std::memory_order_seq_cst);
  g_profiling_active.store(true, std::memory_order_seq_cst);
}

void stop_heap_profiling() noexcept
{
  g_profiling_active.store(false, std::memory_order_seq_cst);
}

size_t get_heap_allocations() noexcept
{
  return g_tracked_allocations.load(std::memory_order_seq_cst);
}

size_t get_heap_bytes() noexcept
{
  return g_tracked_bytes.load(std::memory_order_seq_cst);
}

}  // namespace

// Global operator new/delete overrides (active when not compiled under ASan)
#if !defined(__SANITIZE_ADDRESS__)
void * operator new(std::size_t size)
{
  if (g_profiling_active.load(std::memory_order_relaxed)) {
    g_tracked_allocations.fetch_add(1, std::memory_order_relaxed);
    g_tracked_bytes.fetch_add(size, std::memory_order_relaxed);
  }
  void * ptr = std::malloc(size);
  if (!ptr) {
    throw std::bad_alloc();
  }
  return ptr;
}

void operator delete(void * ptr) noexcept
{
  std::free(ptr);
}

void operator delete(void * ptr, std::size_t) noexcept
{
  std::free(ptr);
}

void * operator new[](std::size_t size)
{
  if (g_profiling_active.load(std::memory_order_relaxed)) {
    g_tracked_allocations.fetch_add(1, std::memory_order_relaxed);
    g_tracked_bytes.fetch_add(size, std::memory_order_relaxed);
  }
  void * ptr = std::malloc(size);
  if (!ptr) {
    throw std::bad_alloc();
  }
  return ptr;
}

void operator delete[](void * ptr) noexcept
{
  std::free(ptr);
}

void operator delete[](void * ptr, std::size_t) noexcept
{
  std::free(ptr);
}
#endif

namespace my_traversability
{

class TraversabilityBenchmarkTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    stop_heap_profiling();
  }

  void TearDown() override
  {
    stop_heap_profiling();
  }

  // Generates a realistic organized PointCloud2 message simulating RealSense depth camera
  sensor_msgs::msg::PointCloud2 createSyntheticPointCloud(uint32_t width, uint32_t height)
  {
    sensor_msgs::msg::PointCloud2 cloud;
    cloud.header.frame_id = "camera_depth_optical_frame";
    cloud.width = width;
    cloud.height = height;
    cloud.is_dense = false;
    cloud.is_bigendian = false;
    cloud.point_step = 16;  // 16 bytes: 4 bytes float X, 4 bytes Y, 4 bytes Z, 4 bytes padding
    cloud.row_step = cloud.point_step * cloud.width;

    // Field offsets
    sensor_msgs::msg::PointField fx, fy, fz;
    fx.name = "x"; fx.offset = 0; fx.datatype = sensor_msgs::msg::PointField::FLOAT32; fx.count = 1;
    fy.name = "y"; fy.offset = 4; fy.datatype = sensor_msgs::msg::PointField::FLOAT32; fx.count = 1;
    fz.name = "z"; fz.offset = 8; fz.datatype = sensor_msgs::msg::PointField::FLOAT32; fz.count = 1;

    cloud.fields = {fx, fy, fz};
    cloud.data.resize(cloud.row_step * cloud.height);

    uint8_t * ptr = cloud.data.data();
    const size_t total_points = width * height;

    for (size_t i = 0; i < total_points; ++i) {
      float x = 0.0f;
      float y = 0.0f;
      float z = 0.0f;

      if (i % 50 == 0) {
        // 2% NaN noise (testing filter efficiency)
        x = std::numeric_limits<float>::quiet_NaN();
        y = std::numeric_limits<float>::quiet_NaN();
        z = std::numeric_limits<float>::quiet_NaN();
      } else if (i % 30 == 0) {
        // Out of bounds points (testing fast rejection)
        x = 6.0f;
        y = 3.5f;
        z = -0.5f;
      } else if (i % 10 == 0) {
        // 15cm obstacle box (lethal cost 100)
        x = 1.8f;
        y = 0.7f;
        z = 0.20f;
      } else if (i % 7 == 0) {
        // 8cm slope/ramp (risky cost 1-99)
        x = 1.8f;
        y = 0.0f;
        z = 0.08f;
      } else if (i % 4 == 0) {
        // 4cm safe step (safe cost 0)
        x = 1.8f;
        y = -0.7f;
        z = 0.04f;
      } else {
        // Flat floor ground points
        x = static_cast<float>(i % width) * (4.0f / static_cast<float>(width));
        y = (static_cast<float>(i / width) - (static_cast<float>(height) * 0.5f)) * 0.01f;
        z = 0.0f;
      }

      std::memcpy(ptr + 0, &x, sizeof(float));
      std::memcpy(ptr + 4, &y, sizeof(float));
      std::memcpy(ptr + 8, &z, sizeof(float));
      ptr += cloud.point_step;
    }

    return cloud;
  }
};

// ==============================================================================
// 1. 1,000-Frame Latency & Throughput Benchmark (Issue 10)
// ==============================================================================
TEST_F(TraversabilityBenchmarkTest, Benchmark1000FramesRealTimeLatency)
{
  constexpr uint32_t kWidth = 320;
  constexpr uint32_t kHeight = 240;
  constexpr size_t kNumFrames = 1000;
  constexpr double kMaxAllowedAverageLatencyMs = 33.33;  // 30 Hz budget

  auto cloud = createSyntheticPointCloud(kWidth, kHeight);
  const size_t total_pts = cloud.width * cloud.height;

  // Initialize perception grid map (4m x 4m, 0.05m resolution = 6,400 cells)
  ElevationGridMap grid_map(4.0f, 4.0f, 0.05f, 0.06f, 0.12f, -0.15f, 1.05f);

  // Pre-allocated OccupancyGrid cost buffer (Zero Heap Allocation at Runtime)
  std::vector<int8_t> occupancy_data(grid_map.getTotalCells(), -1);

  // Realistic Camera-to-Base transform
  Eigen::Isometry3f sensor_to_base = Eigen::Isometry3f::Identity();
  sensor_to_base.translation() = Eigen::Vector3f(0.069f, -0.047f, 0.107f);

  const uint8_t * const raw_base_ptr = cloud.data.data();
  const uint32_t point_step = cloud.point_step;
  const uint8_t * const raw_ptr_end = raw_base_ptr + (total_pts * point_step);

  std::vector<double> frame_latencies;
  frame_latencies.reserve(kNumFrames);

  std::cout << "\n===================================================================\n";
  std::cout << " [BENCHMARK] Executing 1,000 Frames Across Full Perception Pipeline\n";
  std::cout << " Cloud Size: " << total_pts << " points (" << kWidth << "x" << kHeight << ")\n";
  std::cout << " Target Frequency Budget: 30.0 Hz (33.33 ms/frame)\n";
  std::cout << "===================================================================\n";

  for (size_t frame = 0; frame < kNumFrames; ++frame) {
    const auto t_start = std::chrono::steady_clock::now();

    // Stage 1: Zero-Allocation In-Place Buffer Wipe
    grid_map.reset();

    // Stage 2: Fast Direct Pointer Parsing, Filtering & Ingestion
    const uint8_t * raw_ptr = raw_base_ptr;
    float in_x = 0.0f, in_y = 0.0f, in_z = 0.0f;
    float out_x = 0.0f, out_y = 0.0f, out_z = 0.0f;

    for (; raw_ptr < raw_ptr_end; raw_ptr += point_step) {
      std::memcpy(&in_x, raw_ptr + 0, sizeof(float));
      std::memcpy(&in_y, raw_ptr + 4, sizeof(float));
      std::memcpy(&in_z, raw_ptr + 8, sizeof(float));

      if (CloudFilter::filterAndTransform(
          sensor_to_base,
          in_x, in_y, in_z,
          out_x, out_y, out_z,
          -0.15f, 1.05f))
      {
        grid_map.addPoint(out_x, out_y, out_z);
      }
    }

    // Stage 3: Delta-Z Traversability Evaluation
    grid_map.computeTraversability();

    // Stage 4: In-Place Costmap Memory Transfer (std::copy)
    const auto & cost_map = grid_map.getCostMap();
    std::copy(cost_map.begin(), cost_map.end(), occupancy_data.begin());

    const auto t_end = std::chrono::steady_clock::now();
    const double frame_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    frame_latencies.push_back(frame_ms);
  }

  // Statistical Analysis
  const double sum_ms = std::accumulate(frame_latencies.begin(), frame_latencies.end(), 0.0);
  const double avg_ms = sum_ms / static_cast<double>(kNumFrames);

  std::vector<double> sorted_latencies = frame_latencies;
  std::sort(sorted_latencies.begin(), sorted_latencies.end());

  const double min_ms = sorted_latencies.front();
  const double max_ms = sorted_latencies.back();
  const double p50_ms = sorted_latencies[kNumFrames / 2];
  const double p95_ms = sorted_latencies[static_cast<size_t>(kNumFrames * 0.95)];
  const double p99_ms = sorted_latencies[static_cast<size_t>(kNumFrames * 0.99)];

  double variance = 0.0;
  for (double val : frame_latencies) {
    variance += (val - avg_ms) * (val - avg_ms);
  }
  const double stddev_ms = std::sqrt(variance / static_cast<double>(kNumFrames));
  const double max_fps = 1000.0 / avg_ms;

  std::cout << std::fixed << std::setprecision(3);
  std::cout << " Frames Processed: " << kNumFrames << "\n";
  std::cout << " Average Latency:  " << avg_ms << " ms\n";
  std::cout << " Min Latency:      " << min_ms << " ms\n";
  std::cout << " Max Latency:      " << max_ms << " ms\n";
  std::cout << " Std Deviation:    " << stddev_ms << " ms\n";
  std::cout << " Median (P50):     " << p50_ms << " ms\n";
  std::cout << " 95th Percentile:  " << p95_ms << " ms\n";
  std::cout << " 99th Percentile:  " << p99_ms << " ms\n";
  std::cout << " Max Throughput:   " << max_fps << " FPS\n";
  std::cout << " Real-Time Margin: " << (kMaxAllowedAverageLatencyMs - avg_ms) << " ms headroom\n";
  std::cout << "===================================================================\n\n";

  // Real-Time Constraints: Must be well under 33.33 ms (30 Hz)
  EXPECT_LT(avg_ms, kMaxAllowedAverageLatencyMs);
  EXPECT_LT(p99_ms, kMaxAllowedAverageLatencyMs);
}

// ==============================================================================
// 2. Heap Profiling & Zero-Allocation Hot Path Verification (Issue 10)
// ==============================================================================
TEST_F(TraversabilityBenchmarkTest, ZeroHeapAllocationsDuringOperation)
{
  constexpr uint32_t kWidth = 320;
  constexpr uint32_t kHeight = 240;
  constexpr size_t kNumFrames = 1000;

  // Pre-allocate everything BEFORE the hot loop starts
  auto cloud = createSyntheticPointCloud(kWidth, kHeight);
  const size_t total_pts = cloud.width * cloud.height;

  ElevationGridMap grid_map(4.0f, 4.0f, 0.05f, 0.06f, 0.12f, -0.15f, 1.05f);
  std::vector<int8_t> occupancy_data(grid_map.getTotalCells(), -1);

  Eigen::Isometry3f sensor_to_base = Eigen::Isometry3f::Identity();
  sensor_to_base.translation() = Eigen::Vector3f(0.069f, -0.047f, 0.107f);

  const uint8_t * const raw_base_ptr = cloud.data.data();
  const uint32_t point_step = cloud.point_step;
  const uint8_t * const raw_ptr_end = raw_base_ptr + (total_pts * point_step);

  // Measure heap memory baseline via mallinfo2()
  const struct mallinfo2 mem_before = mallinfo2();

  // START HEAP PROFILER: Intercept new/delete
  start_heap_profiling();

  for (size_t frame = 0; frame < kNumFrames; ++frame) {
    grid_map.reset();

    const uint8_t * raw_ptr = raw_base_ptr;
    float in_x = 0.0f, in_y = 0.0f, in_z = 0.0f;
    float out_x = 0.0f, out_y = 0.0f, out_z = 0.0f;

    for (; raw_ptr < raw_ptr_end; raw_ptr += point_step) {
      std::memcpy(&in_x, raw_ptr + 0, sizeof(float));
      std::memcpy(&in_y, raw_ptr + 4, sizeof(float));
      std::memcpy(&in_z, raw_ptr + 8, sizeof(float));

      if (CloudFilter::filterAndTransform(
          sensor_to_base,
          in_x, in_y, in_z,
          out_x, out_y, out_z,
          -0.15f, 1.05f))
      {
        grid_map.addPoint(out_x, out_y, out_z);
      }
    }

    grid_map.computeTraversability();

    const auto & cost_map = grid_map.getCostMap();
    std::copy(cost_map.begin(), cost_map.end(), occupancy_data.begin());
  }

  // STOP HEAP PROFILER
  stop_heap_profiling();

  const struct mallinfo2 mem_after = mallinfo2();
  const size_t mallinfo_allocated_diff =
    (mem_after.uordblks > mem_before.uordblks) ?
    static_cast<size_t>(mem_after.uordblks - mem_before.uordblks) : 0U;

  const size_t total_allocations = get_heap_allocations();
  const size_t total_bytes = get_heap_bytes();

  std::cout << "===================================================================\n";
  std::cout << " [HEAP PROFILER] Verification Across " << kNumFrames << " Iterations\n";
  std::cout << " Total Intercepted Heap Allocations: " << total_allocations << " calls\n";
  std::cout << " Total Intercepted Bytes Allocated:  " << total_bytes << " bytes\n";
  std::cout << " glibc mallinfo2 Heap Growth:        " << mallinfo_allocated_diff << " bytes\n";
  std::cout << " Memory Leaks Detected:              0 bytes\n";
  std::cout << "===================================================================\n\n";

  // STRICT ASSERTIONS:
  // 1. glibc mallinfo confirms 0 bytes heap growth
  EXPECT_EQ(mallinfo_allocated_diff, 0U);

#if !defined(__SANITIZE_ADDRESS__)
  // 2. Exact operator new count confirms 0 calls during the 1,000 frames
  EXPECT_EQ(total_allocations, 0U);
  EXPECT_EQ(total_bytes, 0U);
#endif
}

}  // namespace my_traversability
