#include "dynamic_obstacle_avoidance/lqr_controller.hpp"
#include <iostream>
#include <cmath>

LQRController::LQRController() {
    A_ = Eigen::Matrix2d::Identity();
    B_ = Eigen::Matrix2d::Zero();
    Q_ = Eigen::Matrix2d::Identity();
    R_ = Eigen::Matrix2d::Identity();
    P_ = Eigen::Matrix2d::Zero();
    K_ = Eigen::Matrix2d::Zero();
    state_error_ = Eigen::Vector2d::Zero();
}

void LQRController::init_lqr_parameters(double q_distance, double q_heading, double r_v, double r_omega, double dt) {
    Q_ << q_distance, 0.0,
          0.0, q_heading;
          
    R_ << r_v, 0.0,
          0.0, r_omega;
          
    // Discrete time A and B matrices for decoupled unicycle kinematics (simplified)
    // x = [distance_error, heading_error]^T
    // u = [v, omega]^T
    // x_{k+1} = A x_k + B u_k
    A_ = Eigen::Matrix2d::Identity();
    B_ << -dt, 0.0,
          0.0, -dt;
          
    solve_dare();
    calculate_optimal_gain();
}

void LQRController::solve_dare() {
    P_ = Q_; // initial guess
    double tolerance = 1e-5;
    int max_iterations = 1000;
    
    for (int i = 0; i < max_iterations; ++i) {
        Eigen::Matrix2d P_next = A_.transpose() * P_ * A_ - 
                                 A_.transpose() * P_ * B_ * 
                                 (R_ + B_.transpose() * P_ * B_).inverse() * 
                                 B_.transpose() * P_ * A_ + Q_;
                                 
        if ((P_next - P_).norm() < tolerance) {
            P_ = P_next;
            break;
        }
        P_ = P_next;
    }
}

void LQRController::calculate_optimal_gain() {
    K_ = (R_ + B_.transpose() * P_ * B_).inverse() * B_.transpose() * P_ * A_;
}

void LQRController::update_state_error(double distance_error, double heading_error) {
    state_error_(0) = distance_error;
    state_error_(1) = heading_error;
}

void LQRController::compute_control_command(double& v_cmd, double& omega_cmd) {
    // Control law: u = -Kx
    Eigen::Vector2d u = -K_ * state_error_;
    v_cmd = u(0);
    omega_cmd = u(1);
}
