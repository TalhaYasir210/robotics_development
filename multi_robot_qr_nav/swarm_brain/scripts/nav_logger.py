#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import PoseStamped, PoseWithCovarianceStamped
from nav2_msgs.action import NavigateToPose
import math

class NavLogger(Node):
    def __init__(self):
        super().__init__('nav_logger')
        
        # Subscribe to RViz goal pose
        self.goal_sub = self.create_subscription(
            PoseStamped,
            'goal_pose',
            self.goal_callback,
            10
        )
        
        # Subscribe to Nav2 action feedback
        self.feedback_sub = self.create_subscription(
            NavigateToPose.Impl.FeedbackMessage,
            'navigate_to_pose/_action/feedback',
            self.feedback_callback,
            10
        )
        
        # Subscribe to amcl_pose to check if localized
        self.amcl_sub = self.create_subscription(
            PoseWithCovarianceStamped,
            'amcl_pose',
            self.amcl_callback,
            10
        )
        
        self.is_localized = False
        self.get_logger().info('Nav Logger started! Waiting for Bot 2 to localize (amcl_pose)...')

    def amcl_callback(self, msg: PoseWithCovarianceStamped):
        if not self.is_localized:
            self.is_localized = True
            self.get_logger().info('Bot 2 is now localized! Navigation logging is ACTIVATED.')

    def goal_callback(self, msg: PoseStamped):
        if not self.is_localized:
            return
            
        x = msg.pose.position.x
        y = msg.pose.position.y
        # Compute yaw from quaternion
        q = msg.pose.orientation
        siny_cosp = 2 * (q.w * q.z + q.x * q.y)
        cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z)
        yaw = math.atan2(siny_cosp, cosy_cosp)
        yaw_deg = math.degrees(yaw)
        
        self.get_logger().info(f'\n=========================================\n'
                               f'>>> NEW 2D GOAL RECEIVED\n'
                               f'>>> X: {x:.2f}, Y: {y:.2f}\n'
                               f'>>> Yaw: {yaw_deg:.2f}°\n'
                               f'=========================================')

    def feedback_callback(self, msg):
        if not self.is_localized:
            return
            
        feedback = msg.feedback
        dist = feedback.distance_remaining
        
        if not hasattr(self, 'log_counter'):
            self.log_counter = 0
            
        self.log_counter += 1
        # The feedback topic typically publishes at 10Hz, so logging every 10 messages = 1Hz
        if self.log_counter % 10 == 0:
            
            # We can also compute angle error if we want, but distance is the most useful
            # Let's just log distance remaining cleanly
            self.get_logger().info(f'[Nav2 Feedback] Distance remaining to goal: {dist:.2f} meters')

def main(args=None):
    rclpy.init(args=args)
    node = NavLogger()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
