#ifndef MOTION_CONTROLLER_HPP_
#define MOTION_CONTROLLER_HPP_

#include "autonomy_fsm.hpp" // Needs to know what a "Mode" is
#include <algorithm>        // Required for std::clamp

// 📦 THE ACTION: The final speeds we hand back to the Manager
struct VelocityCommand {
    double linear_x;
    double angular_z;
};

class MotionController {
public:
    MotionController();

    // The Manager feeds us the Brain's decision and the current angle error
    VelocityCommand calculate_velocity(const FSMDecision& decision, double angle_error);
};

#endif // MOTION_CONTROLLER_HPP_