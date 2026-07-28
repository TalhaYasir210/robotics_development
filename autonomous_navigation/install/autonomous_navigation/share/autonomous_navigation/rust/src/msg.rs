#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};



// Corresponds to autonomous_navigation__msg__NavigationCommand
/// True if this is a follow waypoints command, False if it's a single pose goal

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct NavigationCommand {

    // This member is not documented.
    #[allow(missing_docs)]
    pub is_waypoint_nav: bool,

    /// The target poses.
    /// If is_waypoint_nav is False, only the first pose in the array is used.
    /// If is_waypoint_nav is True, the robot will navigate through all poses sequentially.
    pub waypoints: Vec<geometry_msgs::msg::PoseStamped>,

}



impl Default for NavigationCommand {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::NavigationCommand::default())
  }
}

impl rosidl_runtime_rs::Message for NavigationCommand {
  type RmwMsg = super::msg::rmw::NavigationCommand;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        is_waypoint_nav: msg.is_waypoint_nav,
        waypoints: msg.waypoints
          .into_iter()
          .map(|elem| geometry_msgs::msg::PoseStamped::into_rmw_message(std::borrow::Cow::Owned(elem)).into_owned())
          .collect(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      is_waypoint_nav: msg.is_waypoint_nav,
        waypoints: msg.waypoints
          .iter()
          .map(|elem| geometry_msgs::msg::PoseStamped::into_rmw_message(std::borrow::Cow::Borrowed(elem)).into_owned())
          .collect(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      is_waypoint_nav: msg.is_waypoint_nav,
      waypoints: msg.waypoints
          .into_iter()
          .map(geometry_msgs::msg::PoseStamped::from_rmw_message)
          .collect(),
    }
  }
}


