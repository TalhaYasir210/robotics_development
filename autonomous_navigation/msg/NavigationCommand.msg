# True if this is a follow waypoints command, False if it's a single pose goal
bool is_waypoint_nav

# The target poses. 
# If is_waypoint_nav is False, only the first pose in the array is used.
# If is_waypoint_nav is True, the robot will navigate through all poses sequentially.
geometry_msgs/PoseStamped[] waypoints
