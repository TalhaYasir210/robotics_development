#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};


#[link(name = "autonomous_navigation__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__autonomous_navigation__msg__NavigationCommand() -> *const std::ffi::c_void;
}

#[link(name = "autonomous_navigation__rosidl_generator_c")]
extern "C" {
    fn autonomous_navigation__msg__NavigationCommand__init(msg: *mut NavigationCommand) -> bool;
    fn autonomous_navigation__msg__NavigationCommand__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<NavigationCommand>, size: usize) -> bool;
    fn autonomous_navigation__msg__NavigationCommand__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<NavigationCommand>);
    fn autonomous_navigation__msg__NavigationCommand__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<NavigationCommand>, out_seq: *mut rosidl_runtime_rs::Sequence<NavigationCommand>) -> bool;
}

// Corresponds to autonomous_navigation__msg__NavigationCommand
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// True if this is a follow waypoints command, False if it's a single pose goal

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct NavigationCommand {

    // This member is not documented.
    #[allow(missing_docs)]
    pub is_waypoint_nav: bool,

    /// The target poses.
    /// If is_waypoint_nav is False, only the first pose in the array is used.
    /// If is_waypoint_nav is True, the robot will navigate through all poses sequentially.
    pub waypoints: rosidl_runtime_rs::Sequence<geometry_msgs::msg::rmw::PoseStamped>,

}



impl Default for NavigationCommand {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !autonomous_navigation__msg__NavigationCommand__init(&mut msg as *mut _) {
        panic!("Call to autonomous_navigation__msg__NavigationCommand__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for NavigationCommand {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { autonomous_navigation__msg__NavigationCommand__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { autonomous_navigation__msg__NavigationCommand__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { autonomous_navigation__msg__NavigationCommand__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for NavigationCommand {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for NavigationCommand where Self: Sized {
  const TYPE_NAME: &'static str = "autonomous_navigation/msg/NavigationCommand";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__autonomous_navigation__msg__NavigationCommand() }
  }
}


