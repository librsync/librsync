# Copyright (C) 2015 Adam Schubert <adam.schubert@sg1-game.net>
# Copyright 2016 Martin Pool
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


project(librsync C)
cmake_minimum_required(VERSION 3.6)

INCLUDE(CMakeDependentOption)
include(GNUInstallDirs)

set(LIBRSYNC_MAJOR_VERSION 2)
set(LIBRSYNC_MINOR_VERSION 3)
set(LIBRSYNC_PATCH_VERSION 3)

set(LIBRSYNC_VERSION
  ${LIBRSYNC_MAJOR_VERSION}.${LIBRSYNC_MINOR_VERSION}.${LIBRSYNC_PATCH_VERSION})

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Turn on generating compile_commands.json for clang-tidy and iwyu.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_SYSTEM_PROCESSOR)
	message(FATAL_ERROR "No target CPU architecture set")
endif()

if (NOT CMAKE_SYSTEM_NAME)
	message(FATAL_ERROR "No target OS set")
endif()

# Set CMAKE_BUILD_TYPE if unset.
include(BuildType)
message (STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

option(BUILD_SHARED_LIBS "Build librsync as a shared library." ON)

# Option ENABLE_TRACE defaults to ON for Debug builds.
if (CMAKE_BUILD_TYPE MATCHES Debug)
  option(ENABLE_TRACE "Compile in detailed trace messages" ON)
else ()
  option(ENABLE_TRACE "Compile in detailed trace messages" OFF)
endif()
set(DO_RS_TRACE 0)
if (ENABLE_TRACE)
    set(DO_RS_TRACE 1)
endif (ENABLE_TRACE)
message(STATUS "DO_RS_TRACE=${DO_RS_TRACE}")

# Add an option to include compression support
option(ENABLE_COMPRESSION "Whether or not to build with compression support" OFF)
# TODO: Remove this warning when compression is implemented.
#       Consider turning compression ON by default.
if (ENABLE_COMPRESSION)
    message(WARNING "Compression support is not functional. See issue #8.")
endif (ENABLE_COMPRESSION)

include ( CheckIncludeFiles )
check_include_files ( sys/file.h HAVE_SYS_FILE_H )
check_include_files ( sys/stat.h HAVE_SYS_STAT_H )
check_include_files ( sys/types.h HAVE_SYS_TYPES_H )
check_include_files ( unistd.h HAVE_UNISTD_H )
check_include_files ( io.h HAVE_IO_H )
check_include_files ( fcntl.h HAVE_FCNTL_H )
check_include_files ( mcheck.h HAVE_MCHECK_H )
check_include_files ( zlib.h HAVE_ZLIB_H )
check_include_files ( bzlib.h HAVE_BZLIB_H )

# Remove compression support if not needed
if (NOT ENABLE_COMPRESSION)
  SET(HAVE_BZLIB_H 0)
  SET(HAVE_ZLIB_H 0)
endif (NOT ENABLE_COMPRESSION)

include ( CheckSymbolExists )
check_symbol_exists ( __func__ "" HAVE___FUNC__ )
check_symbol_exists ( __FUNCTION__ "" HAVE___FUNCTION__ )

include ( CheckFunctionExists )
check_function_exists ( fseeko HAVE_FSEEKO )
check_function_exists ( fseeko64 HAVE_FSEEKO64 )
check_function_exists ( _fseeki64 HAVE__FSEEKI64 )
check_function_exists ( fstat64 HAVE_FSTAT64 )
check_function_exists ( _fstati64 HAVE__FSTATI64 )
check_function_exists ( fileno HAVE_FILENO )
check_function_exists ( _fileno HAVE__FILENO )

include(CheckTypeSize)
check_type_size ( "long" SIZEOF_LONG )
check_type_size ( "long long" SIZEOF_LONG_LONG )
check_type_size ( "off_t" SIZEOF_OFF_T )
check_type_size ( "off64_t" SIZEOF_OFF64_T )
check_type_size ( "size_t" SIZEOF_SIZE_T )
check_type_size ( "unsigned int" SIZEOF_UNSIGNED_INT )
check_type_size ( "unsigned long" SIZEOF_UNSIGNED_LONG )
check_type_size ( "unsigned short" SIZEOF_UNSIGNED_SHORT )

# Check for printf "%zu" size_t formatting support.
if(WIN32)
  # CheckCSourceRuns checking for "%zu" succeeds but still gives warnings on win32.
  set(HAVE_PRINTF_Z OFF)
  # Not using unsupported %zu generates warnings about using %I64 with MinGW.
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format")
  message (STATUS "Compiling to Win32 - printf \"%zu\" size_t formatting support disabled")
elseif(CMAKE_CROSSCOMPILING)
  # CheckCSourceRuns doesn't work when cross-compiling; assume C99 compliant support.
  set(HAVE_PRINTF_Z ON)
  message (STATUS "Cross compiling - assuming printf \"%zu\" size_t formatting support")
else()
  include(CheckCSourceRuns)
  check_c_source_runs("#include <stdio.h>\nint main(){char o[8];sprintf(o, \"%zu\", (size_t)7);return o[0] != '7';}" HAVE_PRINTF_Z)
endif()

include (TestBigEndian)
TEST_BIG_ENDIAN(WORDS_BIGENDIAN)
if (WORDS_BIGENDIAN)
  message(STATUS "System is big-endian.")
else (WORDS_BIGENDIAN)
  message(STATUS "System is little-endian.")
endif (WORDS_BIGENDIAN)

# OS X
if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
  set(CMAKE_SKIP_BUILD_RPATH FALSE)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" isSystemDir)
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
  endif()
endif()

if (CMAKE_C_COMPILER_ID MATCHES "(Clang|Gnu|GNU)")
  # TODO: Set -Werror when the build is clean.
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -pedantic")
  if (CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wconversion")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-sign-conversion")
  endif()
elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_CRT_SECURE_NO_WARNINGS")
endif()

site_name(BUILD_HOSTNAME)

message (STATUS "PROJECT_NAME  = ${PROJECT_NAME}")
message (STATUS "BUILD_HOSTNAME = ${BUILD_HOSTNAME}")
message (STATUS "CMAKE_SYSTEM = ${CMAKE_SYSTEM}")

# Find POPT
find_package(POPT)
if (POPT_FOUND)
  message (STATUS "POPT_INCLUDE_DIRS  = ${POPT_INCLUDE_DIRS}")
  message (STATUS "POPT_LIBRARIES = ${POPT_LIBRARIES}")
  include_directories(${POPT_INCLUDE_DIRS})
endif (POPT_FOUND)

# Add an option to exclude rdiff executable from build
# This is useful, because it allows to remove POPT dependency if a user is interested only in the
# rsync library itself and not in the rdiff executable
cmake_dependent_option(BUILD_RDIFF "Whether or not to build rdiff executable" ON "POPT_FOUND" OFF)

# Find BZIP
find_package (BZip2)
if (BZIP2_FOUND)
  message (STATUS "BZIP2_INCLUDE_DIRS  = ${BZIP2_INCLUDE_DIRS}")
  message (STATUS "BZIP2_LIBRARIES = ${BZIP2_LIBRARIES}")
  include_directories(${BZIP2_INCLUDE_DIRS})
endif (BZIP2_FOUND)

# Find ZLIB
find_package (ZLIB)
if (ZLIB_FOUND)
  message (STATUS "ZLIB_INCLUDE_DIRS  = ${ZLIB_INCLUDE_DIRS}")
  message (STATUS "ZLIB_LIBRARIES = ${ZLIB_LIBRARIES}")
  include_directories(${ZLIB_INCLUDE_DIRS})
endif (ZLIB_FOUND)

# Find libb2
find_package(LIBB2)
if (LIBB2_FOUND)
  message (STATUS "LIBB2_INCLUDE_DIRS  = ${LIBB2_INCLUDE_DIRS}")
  message (STATUS "LIBB2_LIBRARIES = ${LIBB2_LIBRARIES}")
endif (LIBB2_FOUND)

# Add an option to use LIBB2 if found. It defaults to off because the
# reference implementation is currently faster.
cmake_dependent_option(USE_LIBB2 "Use the libb2 blake2 implementation." OFF "LIBB2_FOUND" OFF)

if (USE_LIBB2)
  message (STATUS "Using libb2 blake2 implementation.")
  include_directories(${LIBB2_INCLUDE_DIRS})
  set(blake2_LIBS ${LIBB2_LIBRARIES})
else (USE_LIBB2)
  message (STATUS "Using included blake2 implementation.")
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/blake2)
  set(blake2_SRCS src/blake2/blake2b-ref.c)
endif (USE_LIBB2)

# Doxygen doc generator.
find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile @ONLY)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif(DOXYGEN_FOUND)

# Code tidy target to reformat code with indent.
file(GLOB tidy_SRCS src/*.[ch] tests/*.[ch])
set(TYPE_RE "(\\w+_t)")
set(CAST_RE "(\\(${TYPE_RE}( \\*+)?\\))")
add_custom_target(tidy
    COMMENT "Reformatting all code to preferred coding style."
    # Note indent requires all userdefined types to be specified with '-T <type>' args to
    # format them correctly. Rather than do that, we just postprocess with sed.
    #
    # Hide the enclosing 'extern "C" {...}' block for indenting in librsync.h
    COMMAND sed -r -i "s:^(extern \"C\") \\{:\\1;:; s:^\\}(\\s+/\\* extern \"C\" \\*/):;\\1:" src/librsync.h
    # Linux format with no tabs, indent 4, preproc indent 2, 80 columns, swallow blank lines.
    COMMAND indent -linux -nut -i4 -ppi2 -l80 -lc80 -fc1 -sob -T FILE -T Rollsum -T rs_result ${tidy_SRCS}
    # Remove space between * or & and identifier after userdefined types,
    # remove space after type cast for userdefined types like indent -ncs,
    # and remove trailing whitespace.
    COMMAND sed -r -i "s:((${TYPE_RE}|${CAST_RE}) (&|\\*+)) :\\1:g; s:(${CAST_RE}) :\\1:g; s:\\s+$::" ${tidy_SRCS}
    # Restore the enclosing 'extern "C" {...}' block in librsync.h
    COMMAND sed -r -i "s:^(extern \"C\");:\\1 {:; s:^;(\\s+/\\* extern \"C\" \\*/):}\\1:" src/librsync.h
    VERBATIM
)
# Code tidyc target to reformat all code and comments with https://github.com/dbaarda/tidyc.
add_custom_target(tidyc
    COMMENT "Reformatting all code and comments to preferred coding style."
    # Recomended format, reformat linebreaks, reformat comments, 80 columns.
    COMMAND tidyc -R -C -l80 -T FILE -T Rollsum -T rs_result ${tidy_SRCS}
    VERBATIM
)

# clang-tidy target to check code for errors.
add_custom_target(clang-tidy
    COMMENT "Check code for errors using clang-tidy."
    COMMAND run-clang-tidy -p ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM
)

# iwyu target to check includes for correctness.
add_custom_target(iwyu
    COMMENT "Check #includes for correctness using iwyu_tool."
    COMMAND iwyu_tool -p ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM
)

# iwyu-fix target to fix includes for correctness.
# Note fileutil.c gets mangled by this and is excluded.
add_custom_target(iwyu-fix
    COMMENT "Fix #includes for correctness using iwyu_tool and fix_include."
    COMMAND iwyu_tool -p ${CMAKE_CURRENT_BINARY_DIR} | fix_include --noblank_lines --ignore_re=fileutil.c
    VERBATIM
)

# Testing

add_executable(isprefix_test
    tests/isprefix_test.c src/isprefix.c)
add_test(NAME isprefix_test COMMAND isprefix_test)

add_executable(netint_test
    tests/netint_test.c src/netint.c src/util.c src/trace.c src/tube.c
    src/scoop.c)
target_compile_options(netint_test PRIVATE -DLIBRSYNC_STATIC_DEFINE)
add_test(NAME netint_test COMMAND netint_test)

add_executable(rollsum_test
    tests/rollsum_test.c src/rollsum.c)
add_test(NAME rollsum_test COMMAND rollsum_test)

add_executable(rabinkarp_test
    tests/rabinkarp_test.c src/rabinkarp.c)
add_test(NAME rabinkarp_test COMMAND rabinkarp_test)
add_executable(rabinkarp_perf
    tests/rabinkarp_perf.c src/rabinkarp.c)

add_executable(hashtable_test
    tests/hashtable_test.c src/hashtable.c)
add_test(NAME hashtable_test COMMAND hashtable_test)

add_executable(checksum_test
    tests/checksum_test.c src/checksum.c src/rollsum.c src/rabinkarp.c src/mdfour.c ${blake2_SRCS})
target_compile_options(checksum_test PRIVATE -DLIBRSYNC_STATIC_DEFINE)
target_link_libraries(checksum_test ${blake2_LIBS})
add_test(NAME checksum_test COMMAND checksum_test)

add_executable(sumset_test
    tests/sumset_test.c src/sumset.c src/util.c src/trace.c src/hex.c
    src/checksum.c src/rollsum.c src/rabinkarp.c src/mdfour.c src/hashtable.c ${blake2_SRCS})
target_compile_options(sumset_test PRIVATE -DLIBRSYNC_STATIC_DEFINE)
target_link_libraries(sumset_test ${blake2_LIBS})
add_test(NAME sumset_test COMMAND sumset_test)

# On Windows we need to explicitly execute bash for scripts.
if (WIN32)
    set(WIN_BASH bash -e)
endif (WIN32)

# Disable rdiff specific tests
if (BUILD_RDIFF)
    add_test(NAME rdiff_bad_option
        COMMAND ${WIN_BASH} rdiff_bad_option.test $<TARGET_FILE:rdiff>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_test(NAME Help
        COMMAND ${WIN_BASH} help.test $<TARGET_FILE:rdiff>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_test(NAME Mutate
        COMMAND ${WIN_BASH} mutate.test $<TARGET_FILE:rdiff>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_test(NAME Signature
        COMMAND ${WIN_BASH} signature.test $<TARGET_FILE:rdiff>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_test(NAME Sources
        COMMAND ${WIN_BASH} sources.test $<TARGET_FILE:rdiff>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_test(NAME Triple
        COMMAND ${WIN_BASH} triple.test $<TARGET_FILE:rdiff>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_test(NAME Delta
        COMMAND ${WIN_BASH} delta.test $<TARGET_FILE:rdiff>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_test(NAME Changes
        COMMAND ${WIN_BASH} changes.test $<TARGET_FILE:rdiff>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
endif (BUILD_RDIFF)


# `make check` that will build everything and then run the tests.
# See https://cmake.org/Wiki/CMakeEmulateMakeCheck and
# https://github.com/librsync/librsync/issues/49
if (BUILD_RDIFF)
  set(LAST_TARGET rdiff)
else (BUILD_RDIFF)
  set(LAST_TARGET rsync)
endif (BUILD_RDIFF)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -C Debug)
add_dependencies(check ${LAST_TARGET}
    isprefix_test
    netint_test
    rollsum_test
    rabinkarp_test
    hashtable_test
    checksum_test
    sumset_test)

enable_testing()

# Create conf files
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)

# We need to be able to #include "file" from a few places,
# * The original source dir
# * The generated source dir
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)


########### next target ###############

# Only list the .c files that need to be compiled
# (Don't list .h files)

set(rsync_LIB_SRCS
    src/prototab.c
    src/base64.c
    src/buf.c
    src/checksum.c
    src/command.c
    src/delta.c
    src/deltagen.c
    src/emit.c
    src/fileutil.c
    src/hashtable.c
    src/hex.c
    src/job.c
    src/mdfour.c
    src/mksum.c
    src/msg.c
    src/netint.c
    src/patch.c
    src/readsums.c
    src/rollsum.c
    src/rabinkarp.c
    src/scoop.c
    src/stats.c
    src/sumset.c
    src/trace.c
    src/tube.c
    src/util.c
    src/version.c
    src/whole.c
    ${blake2_SRCS})

add_library(rsync ${rsync_LIB_SRCS})
# TODO: Enable this when GenerateExportHeader works more widely.
# include(GenerateExportHeader)
# generate_export_header(rsync BASE_NAME librsync
#     EXPORT_FILE_NAME ${CMAKE_SOURCE_DIR}/src/librsync_export.h)
target_link_libraries(rsync ${blake2_LIBS})

# Optionally link zlib and bzip2 if
# - compression is enabled
# - and libraries are found
if (ENABLE_COMPRESSION)
  if (ZLIB_FOUND AND BZIP2_FOUND)
    target_link_libraries(rsync ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES})
  else (ZLIB_FOUND AND BZIP2_FOUND)
    message (WARNING "zlib and bzip2 librares are required to enable compression")
  endif (ZLIB_FOUND AND BZIP2_FOUND)
endif (ENABLE_COMPRESSION)

# Set properties/options for shared vs static library.
if (BUILD_SHARED_LIBS)
  set_target_properties(rsync PROPERTIES C_VISIBILITY_PRESET hidden)
else (BUILD_SHARED_LIBS)
  target_compile_options(rsync PUBLIC -DLIBRSYNC_STATIC_DEFINE)
endif (BUILD_SHARED_LIBS)

set_target_properties(rsync PROPERTIES
    VERSION ${LIBRSYNC_VERSION}
    SOVERSION ${LIBRSYNC_MAJOR_VERSION})
install(TARGETS rsync ${INSTALL_TARGETS_DEFAULT_ARGS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)


########### next target ###############

if (BUILD_RDIFF)
  set(rdiff_SRCS
      src/rdiff.c
      src/isprefix.c)

  add_executable(rdiff ${rdiff_SRCS})
  if (POPT_FOUND)
    target_link_libraries(rdiff rsync ${POPT_LIBRARIES})
  else (POPT_FOUND)
    message (WARNING "Popt library is required for rdiff target")
  endif (POPT_FOUND)

  install(TARGETS rdiff ${INSTALL_TARGETS_DEFAULT_ARGS}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endif (BUILD_RDIFF)


########### install files ###############

install(FILES
    src/librsync.h
    src/librsync_export.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

message (STATUS "CMAKE_C_FLAGS  = ${CMAKE_C_FLAGS}")

install(FILES
    doc/librsync.3
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man3)
install(FILES
    doc/rdiff.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

# vim: shiftwidth=4 expandtab
