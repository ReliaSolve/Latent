/*
  Copyright 2015 ReliaSolve.com

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

// This code is based partially on the Sensics
// RenderManagerOpenGLExample.cpp program,
// which was also released under the Apache 2.0
// license.

#include <stdlib.h>
#include <cmath>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <DeviceThreadVRPNAnalog.h>
#include <vrpn_Streaming_Arduino.h>

#include <RenderManager.h>
// Needed for render buffer calls.  OSVR will have called glewInit() for us
// when we open the display.
#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#endif
#include <GL/GL.h>

//This must come after we include <GL/GL.h> so its pointer types are defined.
#include <GraphicsLibraryOpenGL.h>

// Global state.
unsigned g_verbosity = 2;       //< Larger numbers are more verbose
std::string g_arduinoPortName;
int g_arduinoChannel = 1;
GLfloat g_red = 0;    //< Clear color
GLfloat g_green = 0;  //< Clear color
GLfloat g_blue = 0;   //< Clear color

void Usage(std::string name)
{
  std::cerr << "Usage: " << name << " [-count N] [-arrivalTime] Arduino_serial_port Photosensor_channel DISPLAY_CONFIG RENDERMANAGER_CONFIG" << std::endl;
  std::cerr << "       -count: Repeat the test N times (default 10)" << std::endl;
  std::cerr << "       -arrivalTime: Use arrival time of messages (default is reported sampling time)" << std::endl;
  std::cerr << "       Arduino_serial_port: Name of the serial device to use "
            << "to talk to the Arduino.  The Arduino must be running "
            << "the vrpn_streaming_arduino program." << std::endl;
  std::cerr << "                    (On windows, something like COM5)" << std::endl;
  std::cerr << "                    (On mac, something like /dev/tty.usbmodem1411)" << std::endl;
  std::cerr << "       Photosensor_channel: The channel that has the photosensor on it" << std::endl;
  std::cerr << "       DISPLAY_CONFIG: The name of the RenderManager display configuration .json file" << std::endl;
  std::cerr << "       RENDERMANAGER_CONFIG: The name of the RenderManager configuration .json file" << std::endl;
  exit(-1);
}

// Callback to set up a given display, which may have one or more eyes in it
void SetupDisplay(
  void *userData              //< Passed into SetDisplayCallback
  , osvr::renderkit::GraphicsLibrary  library //< Graphics library context to use
  , osvr::renderkit::RenderBuffer     buffers //< Buffers to use
  )
{
  // Make sure our pointers are filled in correctly.  The config file selects
  // the graphics library to use, and may not match our needs.
  if (library.OpenGL == nullptr) {
    std::cerr << "SetupDisplay: No OpenGL GraphicsLibrary, check config file" << std::endl;
    return;
  }

  // Clear the screen to the specified color and clear depth
  glClearColor(g_red, g_green, g_blue, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// Helper function that creates a vrpn_Streaming_Arduino given a name
// and connection to use.  It uses the global state telling which channel
// the potentiometer is on to determine how many ports to request.

static vrpn_Analog *CreateStreamingServer(
  const char *deviceName, vrpn_Connection *c)
{
  int num_channels = g_arduinoChannel + 1;
  return new vrpn_Streaming_Arduino(deviceName, c,
                g_arduinoPortName, num_channels);
}

// Helper function to compute and print the min, mean, and
// max for a vector and print the results.
void print_stats(std::string name, std::vector<double> vals)
{
  if (vals.size() > 0) {
    double minVal = vals[0], maxVal = vals[0] , meanVal = vals[0];
    for (size_t i = 1; i < vals.size(); i++) {
      meanVal += vals[i];
      if (vals[i] > maxVal) { maxVal = vals[i]; }
      if (vals[i] < minVal) { minVal = vals[i]; }
    }
    meanVal /= vals.size();
    std::cout << name << " min: " << minVal << ", mean " << meanVal
      << ", max " << maxVal
      << ", range " << maxVal - minVal << std::endl;
  }
}


int main(int argc, const char *argv[])
{
  // Parse the command line.
  size_t realParams = 0;
  int count = 10;
  bool arrivalTime = false;
  std::string displayConfigJsonFileName;
  std::string pipelineConfigJsonFileName;

  for (size_t i = 1; i < argc; i++) {
    if (argv[i] == std::string("-count")) {
      if (++i > argc) {
        std::cerr << "Error: -count parameter requires value" << std::endl;
        Usage(argv[0]);
      }
      count = atoi(argv[i]);
      if (count < 1) {
        std::cerr << "Error: -count parameter must be >= 1, found "
          << argv[i] << std::endl;
        Usage(argv[0]);
      }
    } else if (argv[i] == std::string("-arrivalTime")) {
      arrivalTime = true;
    } else if (argv[i][0] == '-') {
        Usage(argv[0]);
    } else switch (++realParams) {
      case 1:
        g_arduinoPortName = argv[i];
        break;
      case 2:
        g_arduinoChannel = atoi(argv[i]);
        break;
      case 3:
        displayConfigJsonFileName = argv[i];
        break;
      case 4:
        pipelineConfigJsonFileName = argv[i];
        break;
      default:
        Usage(argv[0]);
    }
  }
  if (realParams != 4) {
    Usage(argv[0]);
  }

  // Construct the thread to handle the photosensor
  // reading from the Ardiuno.
  DeviceThreadVRPNAnalog  arduino(CreateStreamingServer);

  //-----------------------------------------------------------------
  // Wait until we get at least one report from the device
  // or timeout.  Make sure the report sizes are large enough
  // to support the channels we're reading.
  struct timeval start, now;
  vrpn_gettimeofday(&start, NULL);
  size_t arduinoCount = 0;
  std::vector<DeviceThreadReport> r;
  if (g_verbosity > 0) {
    std::cout << "Waiting for reports from Arduino:" << std::endl;
  }
  double lastArduinoValue;
  do {
    r = arduino.GetReports();
    if (r.size() > 0) {
      if (r[0].values.size() <= g_arduinoChannel) {
        std::cerr << "Report size from Arduino: " << r[0].values.size()
          << " is too small for requested channel: " << g_arduinoChannel << std::endl;
        return -3;
      }
      lastArduinoValue = r.back().values[g_arduinoChannel];
    }
    arduinoCount += r.size();

    vrpn_gettimeofday(&now, NULL);
  } while ( (arduinoCount == 0)
            && (vrpn_TimevalDurationSeconds(now, start) < 20) );
  if (arduinoCount == 0) {
    std::cerr << "No reports from Arduino" << std::endl;
    return -5;
  }

  //-----------------------------------------------------------------
  // Construct a RenderManager to use to send images to the screen,
  // and register the display update we'll use to make
  // the images (black or white) to test the rendering latency.
  if (g_verbosity > 0) {
    std::cout << "Run an OSVR server for us to connect to and place"
      << " the photosensor in front of the screen at the location"
      << " whose latency you want to render." << std::endl;
  }
  osvr::clientkit::ClientContext context(
    "com.reliasolve.RenderManagerLatencyTest");
  osvr::renderkit::RenderManager *render =
    osvr::renderkit::createRenderManager(context.get(),
      displayConfigJsonFileName, pipelineConfigJsonFileName);
  if ((render == nullptr) ||
    (!render->doingOkay())) {
    std::cerr << "Could not create RenderManager" << std::endl;
    return 1;
  }

  // Set callback to handle setting up rendering in a display
  render->SetDisplayCallback(SetupDisplay);

  // Open the display and make sure this worked.
  osvr::renderkit::RenderManager::OpenResults ret = render->OpenDisplay();
  if (ret.status == osvr::renderkit::RenderManager::OpenStatus::FAILURE) {
    std::cerr << "Could not open display" << std::endl;
    delete render;
    return 2;
  }

  //-----------------------------------------------------------------
  // Wait a bit for any DirectMode switching/power on to happen.
  vrpn_SleepMsecs(1000);

  //-----------------------------------------------------------------
  // Render a dark scene for half a second and then read the Arduino value
  // to get a baseline for dark.  Half a second is an arbitrary number that
  // should be larger than the latency present in the system.
  g_red = g_green = g_blue = 0;
  render->Render();
  vrpn_SleepMsecs(500);
  r = arduino.GetReports();
  if (r.size() == 0) {
    std::cerr << "Could not read Arduino value after dark rendering" << std::endl;
    delete render;
    return 3;
  }
  double dark = r.back().values[g_arduinoChannel];
  if (g_verbosity > 1) {
    std::cout << "Dark-screen photosensor value: " << dark << std::endl;
  }

  //-----------------------------------------------------------------
  // Render a bright scene for half a second and then read the Arduino value
  // to get a baseline for bright.  Half a second is an arbitrary number that
  // should be larger than the latency present in the system.
  g_red = g_green = g_blue = 1;
  render->Render();
  vrpn_SleepMsecs(500);
  r = arduino.GetReports();
  if (r.size() == 0) {
    std::cerr << "Could not read Arduino value after bright rendering" << std::endl;
    delete render;
    return 4;
  }
  double bright = r.back().values[g_arduinoChannel];
  if (g_verbosity > 1) {
    std::cout << "Bright-screen photosensor value: " << bright << std::endl;
  }
  double threshold = (dark + bright) / 2;
  if (threshold - dark < 10) {
    std::cerr << "Bright/dark difference insufficient: " << threshold - dark
      << std::endl;
    return 5;
  }
  if (g_verbosity > 1) {
    std::cout << "Threshold photosensor value: " << threshold << std::endl;
  }

  //-----------------------------------------------------------------
  // Do as many iterations as we're asked for, reporting the latency
  // between when we asked for rendering and when we saw the screen
  // brightness go from below halfway between dark and bright to
  // above halfway between.
  std::vector<double> pre_delays_ms, post_delays_ms;
  for (size_t i = 0; i < count; i++) {
    // Render dark and wait long enough for it to settle.
    g_red = g_green = g_blue = 0;
    render->Render();
    vrpn_SleepMsecs(500);
    r = arduino.GetReports();

    // Store the time, render bright, store the after-render time,
    // and then wait for the bright to have finished.
    g_red = g_green = g_blue = 1;
    struct timeval pre_render;
    vrpn_gettimeofday(&pre_render, NULL);
    render->Render();
    struct timeval post_render;
    vrpn_gettimeofday(&post_render, NULL);
    vrpn_SleepMsecs(500);
    r = arduino.GetReports();

    // Find where we cross the threshold from dark to bright and
    // report latency to pre-render and post-render times.
    for (size_t t = 1; t < r.size(); t++) {
      if ((r[t - 1].values[g_arduinoChannel] < threshold) &&
          (r[t].values[g_arduinoChannel] >= threshold)) {
        if (g_verbosity > 1) {
          pre_delays_ms.push_back(vrpn_TimevalDurationSeconds(r[t].sampleTime, pre_render) * 1e3);
          post_delays_ms.push_back(vrpn_TimevalDurationSeconds(r[t].sampleTime, post_render) * 1e3);
          std::cout << "Latency from pre-render: "
            << pre_delays_ms[i]
            << "ms, from post-render: "
            << post_delays_ms[i]
            << std::endl;
        }
        break;
      }
    }
  }

  //-----------------------------------------------------------------
  // Compute and report statistics on the measurements.
  print_stats("Pre-delay (ms)", pre_delays_ms);
  print_stats("Post-delay (ms)", post_delays_ms);

  //-----------------------------------------------------------------
  // We're done.  Shut down the threads and exit.
  return 0;
}
