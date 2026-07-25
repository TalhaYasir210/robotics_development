#include <gtest/gtest.h>
#include "dynamic_obstacle_avoidance/lqr_controller.hpp"
#include <cmath>

class LQRControllerTest : public ::testing::Test {
protected:
    LQRController lqr;
    
    void SetUp() override {
        // Initialize with typical parameters:
        // q_distance = 1.0, q_heading = 1.0, r_v = 1.0, r_omega = 1.0, dt = 0.1
        lqr.init_lqr_parameters(1.0, 1.0, 1.0, 1.0, 0.1);
    }
};

TEST_F(LQRControllerTest, TestZeroError) {
    lqr.update_state_error(0.0, 0.0);
    double v = 0.0, omega = 0.0;
    lqr.compute_control_command(v, omega);
    
    EXPECT_NEAR(v, 0.0, 1e-5);
    EXPECT_NEAR(omega, 0.0, 1e-5);
}

TEST_F(LQRControllerTest, TestPositiveDistanceError) {
    lqr.update_state_error(1.0, 0.0);
    double v = 0.0, omega = 0.0;
    lqr.compute_control_command(v, omega);
    
    // With B_ = [[-dt, 0], [0, -dt]] and positive error, u = -Kx should be positive
    EXPECT_GT(v, 0.0);
    EXPECT_NEAR(omega, 0.0, 1e-5);
}

TEST_F(LQRControllerTest, TestPositiveHeadingError) {
    lqr.update_state_error(0.0, 1.0);
    double v = 0.0, omega = 0.0;
    lqr.compute_control_command(v, omega);
    
    EXPECT_NEAR(v, 0.0, 1e-5);
    EXPECT_GT(omega, 0.0);
}

TEST_F(LQRControllerTest, TestNegativeErrors) {
    lqr.update_state_error(-1.0, -0.5);
    double v = 0.0, omega = 0.0;
    lqr.compute_control_command(v, omega);
    
    EXPECT_LT(v, 0.0);
    EXPECT_LT(omega, 0.0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
