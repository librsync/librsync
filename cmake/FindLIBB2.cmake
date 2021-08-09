# - Check for the presence of libb2
#
# The following vars can be set to change behaviour;
#  LIBB2_INCLUDE_DIR - cached override for LIBB2_INCLUDE_DIRS.
#  LIBB2_LIBRARY_RELEASE - cached override for LIBB2_LIBRARIES.
#
# The following variables are set when libb2 is found:
#  LIBB2_FOUND = Set to true, if all components of libb2 have been found.
#  LIBB2_INCLUDE_DIRS  = Include path for the header files of libb2.
#  LIBB2_LIBRARIES = Link these to use libb2.

find_path (LIBB2_INCLUDE_DIR blake2.h)
find_library (LIBB2_LIBRARY_RELEASE b2)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (LIBB2 DEFAULT_MSG LIBB2_LIBRARY_RELEASE LIBB2_INCLUDE_DIR)

# Set output vars from auto-detected/cached vars.
if (LIBB2_FOUND)
  set(LIBB2_INCLUDE_DIRS "${LIBB2_INCLUDE_DIR}")
  set(LIBB2_LIBRARIES "${LIBB2_LIBRARY_RELEASE}")
endif (LIBB2_FOUND)

mark_as_advanced (LIBB2_INCLUDE_DIR LIBB2_LIBRARY_RELEASE)
