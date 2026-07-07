#ifndef LQR_CONTROLLER_HPP
#define LQR_CONTROLLER_HPP

#include <Eigen/Dense>

class LQRController {
public:
    LQRController();
    
    // Initialize LQR parameters Q and R
    void init_lqr_parameters(double q_distance, double q_heading, double r_v, double r_omega, double dt);
    
    // Update state error
    void update_state_error(double distance_error, double heading_error);
    
    // Compute optimal control command
    void compute_control_command(double& v_cmd, double& omega_cmd);

private:
    // Solve Discrete Algebraic Riccati Equation
    void solve_dare();
    
    // Calculate optimal gain matrix K
    void calculate_optimal_gain();

    Eigen::Matrix2d A_;
    Eigen::Matrix2d B_;
    Eigen::Matrix2d Q_;
    Eigen::Matrix2d R_;
    Eigen::Matrix2d P_;
    Eigen::Matrix2d K_;
    
    Eigen::Vector2d state_error_;
};

#endif // LQR_CONTROLLER_HPP
