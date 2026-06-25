#include "autonomy_fsm.hpp"
#include <sstream>  // Added for string formatting
#include <iomanip>  // Added for decimal precision

AutonomyFSM::AutonomyFSM() : current_mode_(Mode::TRACKING), is_turning_left_(false), recovery_ticks_(0) {}

void AutonomyFSM::reset() {
    current_mode_ = Mode::TRACKING;
    is_turning_left_ = false;
    recovery_ticks_ = 0;
}

Mode AutonomyFSM::get_current_mode() const { return current_mode_; }

FSMDecision AutonomyFSM::update_state(const ProcessedSensorData& data) {
    FSMDecision decision;
    
    if (!data.is_valid) {
        decision.reasoning = "Waiting for odometry/sensor data";
        decision.current_mode = current_mode_;
        decision.is_turning_left = is_turning_left_;
        return decision;
    }

    if (current_mode_ == Mode::ARRIVED || current_mode_ == Mode::BLOCKED) {
        decision.reasoning = (current_mode_ == Mode::ARRIVED) ? "Mission Accomplished." : "Mission Aborted: Blocked.";
    }
    else if (data.distance_to_goal < 0.2) {
        current_mode_ = Mode::ARRIVED;
        decision.reasoning = "Arrived at Target.";
    }
    else if (data.distance_to_goal < 0.55 && data.min_front < danger_distance_) {
        current_mode_ = Mode::BLOCKED;
        decision.reasoning = "Target coordinate is inside an obstacle!";
    }
    else if (current_mode_ == Mode::TRACKING) {
        if (data.min_front < danger_distance_) {
            current_mode_ = Mode::DODGING;
            is_turning_left_ = (data.avg_left > data.avg_right);
            
            // 🚨 NEW: Build a detailed string with the exact numerical comparison
            std::stringstream ss;
            ss << "Dodging " << (is_turning_left_ ? "LEFT" : "RIGHT") 
               << "! (Left Gap: " << std::fixed << std::setprecision(2) << data.avg_left 
               << "m vs Right Gap: " << data.avg_right << "m)";
            decision.reasoning = ss.str();
            
        } else {
            decision.reasoning = "Path clear. Tracking goal.";
        }
    } 
    else if (current_mode_ == Mode::DODGING) {
        if (data.min_front > clear_distance_) {
            current_mode_ = Mode::RECOVERING;
            recovery_ticks_ = 25; 
            decision.reasoning = "Gap found. Pushing through...";
        } else {
            decision.reasoning = "Actively dodging...";
        }
    } 
    else if (current_mode_ == Mode::RECOVERING) {
        recovery_ticks_--;
        if (data.min_front < danger_distance_) {
            current_mode_ = Mode::DODGING;
            is_turning_left_ = (data.avg_left > data.avg_right);
            decision.reasoning = "Obstacle reappeared. Resuming dodge.";
        } else if (recovery_ticks_ <= 0) {
            current_mode_ = Mode::TRACKING;
            decision.reasoning = "Obstacle cleared. Re-acquiring target.";
        } else {
            decision.reasoning = "Pushing through gap...";
        }
    }

    decision.current_mode = current_mode_;
    decision.is_turning_left = is_turning_left_;
    return decision;
}