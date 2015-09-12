cmake_minimum_required(VERSION 2.8.12)
project(test_arduino_latency)

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

add_executable(test_arduino_latency test_arduino_latency.cpp)
target_link_libraries(test_arduino_latency ${VRPN_LIBRARY})
install(TARGETS test_arduino_latency DESTINATION bin)

