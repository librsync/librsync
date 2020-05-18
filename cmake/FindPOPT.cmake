#--------------------------------------------------------------------------------
# Copyright (C) 2012-2013, Lars Baehren <lbaehren@gmail.com>
# Copyright (C) 2015 Adam Schubert <adam.schubert@sg1-game.net>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
#  * Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#--------------------------------------------------------------------------------

# - Check for the presence of POPT
#
# The following vars can be set to change behaviour;
#  POPT_ROOT_DIR - path hint for finding popt.
#  POPT_INCLUDE_DIR - cached override for POPT_INCLUDE_DIRS.
#  POPT_LIBRARY_RELEASE - cached override for POPT_LIBRARIES.
#
# The following variables are set when POPT is found:
#  POPT_FOUND      = Set to true, if all components of POPT have been found.
#  POPT_INCLUDE_DIRS   = Include path for the header files of POPT.
#  POPT_LIBRARIES  = Link these to use POPT.

# Check with PkgConfig (to retrieve static dependencies such as iconv)
find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_search_module (POPT QUIET IMPORTED_TARGET popt)
  if (POPT_FOUND)
    # PkgConfig found it, set cached vars to use the imported target it created.
    set(POPT_INCLUDE_DIR "" CACHE PATH "Path to headers for popt.")
    set(POPT_LIBRARY_RELEASE PkgConfig::POPT CACHE FILEPATH "Path to library for popt.")
  endif (POPT_FOUND)
endif (PKG_CONFIG_FOUND)

# Fallback to searching for path and library if PkgConfig didn't work.
if (NOT POPT_FOUND)
  find_path (POPT_INCLUDE_DIR popt.h
    HINTS ${POPT_ROOT_DIR} ${CMAKE_INSTALL_PREFIX} $ENV{programfiles}\\GnuWin32 $ENV{programfiles32}\\GnuWin32
    PATH_SUFFIXES include)
  find_library (POPT_LIBRARY_RELEASE popt
    HINTS ${POPT_ROOT_DIR} ${CMAKE_INSTALL_PREFIX} $ENV{programfiles}\\GnuWin32 $ENV{programfiles32}\\GnuWin32
    PATH_SUFFIXES lib)
endif (NOT POPT_FOUND)

# Check library and paths and set POPT_FOUND appropriately.
INCLUDE(FindPackageHandleStandardArgs)
if (TARGET "${POPT_LIBRARY_RELEASE}")
  # The library is a taget created by PkgConfig.
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(POPT
    REQUIRED_VARS POPT_LIBRARY_RELEASE
    VERSION_VAR POPT_VERSION)
else ()
  # The library is a library file and header include path.
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(POPT DEFAULT_MSG POPT_LIBRARY_RELEASE POPT_INCLUDE_DIR)
endif ()

# Set output vars from auto-detected/cached vars.
if (POPT_FOUND)
  set(POPT_INCLUDE_DIRS "${POPT_INCLUDE_DIR}")
  set(POPT_LIBRARIES "${POPT_LIBRARY_RELEASE}")
endif (POPT_FOUND)

# Mark cache vars as advanced.
mark_as_advanced (POPT_INCLUDE_DIR POPT_LIBRARY_RELEASE)
