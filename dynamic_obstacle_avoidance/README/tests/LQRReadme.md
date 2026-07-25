# LQR Controller Unit Tests

This document describes the unit test cases created for the `LQRController` class. The tests are written using the Google Test (`gtest`) framework.

## 1. TestZeroError
Validates the base condition when the robot is exactly at its target.
*   **Action**: Sets distance error and heading error to `0.0`.
*   **Expectation**: Both linear velocity (`v`) and angular velocity (`omega`) commands must compute to `0.0` (within a small tolerance).

## 2. TestPositiveDistanceError
Validates the response to a target that is directly ahead of the robot.
*   **Action**: Sets a positive distance error (`1.0`) with zero heading error.
*   **Expectation**: The linear velocity (`v`) command must be positive to drive the robot forward, and the angular velocity (`omega`) must remain `0.0`.

## 3. TestPositiveHeadingError
Validates the response when the robot needs to turn towards a target.
*   **Action**: Sets a positive heading error (`1.0`) with zero distance error.
*   **Expectation**: The angular velocity (`omega`) command must be positive to correct the heading, and the linear velocity (`v`) must remain `0.0`.

## 4. TestNegativeErrors
Validates the control logic for negative positional and rotational errors.
*   **Action**: Sets both distance and heading errors to negative values.
*   **Expectation**: Both the linear velocity (`v`) and angular velocity (`omega`) commands must be negative to correct the states in the opposite direction.
