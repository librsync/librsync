# Copyright (C) 2024 Lars Erik Wik <lars.erik.wik@northern.tech>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


cmake_minimum_required(VERSION 3.6)
project(librsync_example_pull_api C)

# Find the librsync library and include directory
find_library(RSYNC_LIBRARY NAMES rsync librsync.so)
find_path(RSYNC_INCLUDE_DIR NAMES librsync.h)

# Check if librsync was found
if (NOT RSYNC_LIBRARY OR NOT RSYNC_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find librsync")
endif()

# Add the include directory to your project
include_directories(${RSYNC_INCLUDE_DIR})

# Create client executable
add_executable(client client.c)

# Link client with librsync
target_link_libraries(client ${RSYNC_LIBRARY})

# Create server executable
add_executable(server server.c)

# Link server with librsync
target_link_libraries(server ${RSYNC_LIBRARY})
