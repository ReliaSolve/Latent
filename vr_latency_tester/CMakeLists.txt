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
include_directories(${VRPN_INCLUDE_DIRS})
set(VRPN_GENERIC_SERVER_LIBS gpsnmea vrpn_atmel)

# Will know to look for the other VRPN libraries in the
# INSTALL directory if we're using Superbuild.  Otherwise,
# they will be found in the same place the server library
# is found.
link_directories(${CMAKE_INSTALL_PREFIX}/lib)

# The common classes we want for multiple test programs are put
# into a library.
add_library(DeviceThread
    DeviceThread.cpp
    DeviceThread.h
    DeviceThreadVRPNAnalog.cpp
    DeviceThreadVRPNAnalog.h
)
target_link_libraries(DeviceThread
  ${VRPN_SERVER_LIBRARIES}
  ${VRPN_GENERIC_SERVER_LIBS}
)

add_executable(vrpn_analog_latency_test vrpn_analog_latency_test.cpp)
target_link_libraries(vrpn_analog_latency_test
  DeviceThread
)
install(TARGETS vrpn_analog_latency_test DESTINATION bin)

