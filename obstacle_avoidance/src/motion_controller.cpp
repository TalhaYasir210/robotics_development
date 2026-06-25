#include "motion_controller.hpp"
#include <cmath>
#include <algorithm>

MotionController::MotionController() {}

VelocityCommand MotionController::calculate_velocity(const FSMDecision& decision, double angle_error) {
    VelocityCommand cmd;
    cmd.linear_x = 0.0;
    cmd.angular_z = 0.0;

    if (decision.current_mode == Mode::ARRIVED || decision.current_mode == Mode::BLOCKED) {
        cmd.linear_x = 0.0;
        cmd.angular_z = 0.0;
    } 
    else if (decision.current_mode == Mode::DODGING) {
        cmd.linear_x = 0.0; 
        cmd.angular_z = decision.is_turning_left ? 0.6 : -0.6; 
    } 
    else if (decision.current_mode == Mode::RECOVERING) {
        cmd.linear_x = 0.25; 
        cmd.angular_z = 0.0; 
    }
    else if (decision.current_mode == Mode::TRACKING) {
        // STRICT PIVOT: Rotate first to minimize angle error before moving forward
        if (std::abs(angle_error) > 0.1) {
            cmd.linear_x = 0.0; // Stop completely to rotate
            cmd.angular_z = std::clamp(0.8 * angle_error, -0.5, 0.5); 
        } else {
            cmd.linear_x = 0.20; // Move forward
            cmd.angular_z = std::clamp(0.6 * angle_error, -0.5, 0.5); // Minor corrections while driving
        }
    }

    return cmd;
}