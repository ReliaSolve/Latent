cmake_minimum_required(VERSION 2.8.11)
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
include_directories(BEFORE ${VRPN_INCLUDE_DIRS})
set(VRPN_GENERIC_SERVER_LIBS gpsnmea)
if (UNIX)
    set(VRPN_GENERIC_SERVER_LIBS gpsnmea vrpn_atmel)
endif()

# Common libraries we can make use if
find_package(OSVRRenderManager)
if (OSVRRENDERMANAGER_FOUND)
    include_directories(BEFORE ${OSVRRenderManager_INCLUDE_DIRS})
    find_package(osvr REQUIRED)
    find_package(OpenGL REQUIRED)
endif()

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
    DeviceThreadVRPNTracker.cpp
    DeviceThreadVRPNTracker.h
    ArduinoComparer.cpp
    ArduinoComparer.h
    OscillationEstimator.cpp
    OscillationEstimator.h
)
target_link_libraries(DeviceThread
  ${VRPN_SERVER_LIBRARIES}
  ${VRPN_GENERIC_SERVER_LIBS}
)

add_executable(vrpn_device_latency_test vrpn_device_latency_test.cpp)
target_link_libraries(vrpn_device_latency_test
  DeviceThread
)
install(TARGETS vrpn_device_latency_test DESTINATION bin)

add_executable(arduino_inputs_latency_test arduino_inputs_latency_test.cpp)
target_link_libraries(arduino_inputs_latency_test
  DeviceThread
)
install(TARGETS arduino_inputs_latency_test DESTINATION bin)

add_executable(head_shake_latency_test head_shake_latency_test.cpp)
target_link_libraries(head_shake_latency_test
  DeviceThread
)
install(TARGETS head_shake_latency_test DESTINATION bin)

if (OSVRRENDERMANAGER_FOUND)
    add_executable(RenderManager_latency_test RenderManager_latency_test.cpp)
    target_link_libraries(RenderManager_latency_test
      DeviceThread
      ${OSVRRenderManager_LIBRARIES}
      ${OPENGL_LIBRARY}
      osvr::osvrClientKitCpp
    )
    install(TARGETS RenderManager_latency_test DESTINATION bin)

    add_custom_command(TARGET RenderManager_latency_test POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OSVRRenderManager_INCLUDE_DIR}/../bin/osvrRenderManager.dll"
        "${CMAKE_BINARY_DIR}"
      ) 
endif()

