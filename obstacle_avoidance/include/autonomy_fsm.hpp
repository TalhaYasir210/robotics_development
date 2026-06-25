#ifndef AUTONOMY_FSM_HPP_
#define AUTONOMY_FSM_HPP_

#include "sensor_processor.hpp"
#include <string>

// Reverted to original 5 states
enum class Mode { TRACKING = 0, DODGING = 1, RECOVERING = 2, ARRIVED = 3, BLOCKED = 4 };

struct FSMDecision {
    Mode current_mode;
    std::string reasoning;
    bool is_turning_left;
};

class AutonomyFSM {
public:
    AutonomyFSM();
    void reset();
    Mode get_current_mode() const; 
    FSMDecision update_state(const ProcessedSensorData& data);

private:
    Mode current_mode_;
    bool is_turning_left_;
    int recovery_ticks_; 

    // Reverted to original thresholds
    const float danger_distance_ = 0.30f;   // Distance to start dodging
    const float clear_distance_ = 0.45f;    // Distance to trigger recovery
};

#endif // AUTONOMY_FSM_HPP_