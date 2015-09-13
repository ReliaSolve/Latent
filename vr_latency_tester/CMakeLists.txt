cmake_minimum_required(VERSION 2.8.12)
project(vr_latency_test)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(USE_SUPERBUILD "Use Superbuild to download and install dependencies." ON)

#########################################################################
# If we're doing a Superbuild...
if(USE_SUPERBUILD)
    include(ExternalProject)
    include(Superbuild.cmake)
    return()
endif()

#########################################################################
# If we're not doing a Superbuild, or if we're recursing and building
# ourselves inside the Superbuild.

# Local CMake Modules
list(APPEND CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

# Common libraries that we need
find_package(VRPN REQUIRED)
include_directories({$VRPN_INCLUDE_DIRS})

# The common classes we want for multiple test programs are put
# into a library.
add_library(DeviceThread
    DeviceThread.cpp
    DeviceThread.h
)
target_link_libraries(DeviceThread ${VRPN_LIBRARY})

add_executable(vrpn_tracker_latency_test vrpn_tracker_latency_test.cpp)
target_link_libraries(vrpn_tracker_latency_test DeviceThread)
install(TARGETS vrpn_tracker_latency_test DESTINATION bin)

