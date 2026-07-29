#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "autonomous_navigation::autonomous_navigation__rosidl_generator_py" for configuration ""
set_property(TARGET autonomous_navigation::autonomous_navigation__rosidl_generator_py APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(autonomous_navigation::autonomous_navigation__rosidl_generator_py PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_NOCONFIG "autonomous_navigation::autonomous_navigation__rosidl_generator_c;Python3::Python;autonomous_navigation::autonomous_navigation__rosidl_typesupport_c;geometry_msgs::geometry_msgs__rosidl_generator_c;geometry_msgs::geometry_msgs__rosidl_typesupport_fastrtps_c;geometry_msgs::geometry_msgs__rosidl_typesupport_introspection_c;geometry_msgs::geometry_msgs__rosidl_typesupport_c;geometry_msgs::geometry_msgs__rosidl_typesupport_fastrtps_cpp;geometry_msgs::geometry_msgs__rosidl_typesupport_introspection_cpp;geometry_msgs::geometry_msgs__rosidl_typesupport_cpp;geometry_msgs::geometry_msgs__rosidl_generator_py;std_msgs::std_msgs__rosidl_generator_c;std_msgs::std_msgs__rosidl_typesupport_fastrtps_c;std_msgs::std_msgs__rosidl_typesupport_fastrtps_cpp;std_msgs::std_msgs__rosidl_typesupport_introspection_c;std_msgs::std_msgs__rosidl_typesupport_c;std_msgs::std_msgs__rosidl_typesupport_introspection_cpp;std_msgs::std_msgs__rosidl_typesupport_cpp;std_msgs::std_msgs__rosidl_generator_py"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libautonomous_navigation__rosidl_generator_py.so"
  IMPORTED_SONAME_NOCONFIG "libautonomous_navigation__rosidl_generator_py.so"
  )

list(APPEND _cmake_import_check_targets autonomous_navigation::autonomous_navigation__rosidl_generator_py )
list(APPEND _cmake_import_check_files_for_autonomous_navigation::autonomous_navigation__rosidl_generator_py "${_IMPORT_PREFIX}/lib/libautonomous_navigation__rosidl_generator_py.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
