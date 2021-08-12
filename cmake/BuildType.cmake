# Manage CMAKE_BUILD_TYPE.
#
# This sets the default build type to "Release or "Debug" for a git clone, and
# sets up the possible values for the cmake-gui. It understands multi-config
# generators and respects values set on the cmdline. It comes from;
#
# https://blog.kitware.com/cmake-and-the-default-build-type/

# Set the default build type for when none was specified.
set(DEFAULT_BUILD_TYPE "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set(DEFAULT_BUILD_TYPE "Debug")
endif()

# If CMAKE_BUILD_TYPE is not set and not using a multi-config generator, set
# it to the default and setup the possible values for the cmake-gui.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE
    STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE
    PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
