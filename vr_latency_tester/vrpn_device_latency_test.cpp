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

#include <stdlib.h>
#include <cmath>
#include <string>
#include <iostream>
#include <vector>
#include <DeviceThreadVRPNAnalog.h>
#include <DeviceThreadVRPNTracker.h>
#include <ArduinoComparer.h>
#include <vrpn_Streaming_Arduino.h>

// Global state.

unsigned g_verbosity = 2;       //< Larger numbers are more verbose
std::string g_arduinoPortName;
int g_arduinoChannel = 0;

void Usage(std::string name)
{
  std::cerr << "Usage: " << name << " Arduino_serial_port Arduino_channel DEVICE_TYPE [Device_config_file|Device_device_name] Device_channel [-count N] [-arrivalTime] [-verbosity N]" << std::endl;
  std::cerr << "       -count: Repeat the test N times (default 10)" << std::endl;
  std::cerr << "       -arrivalTime: Use arrival time of messages (default is reported sampling time)" << std::endl;
  std::cerr << "       -verbosity: How much info to print (default "
    << g_verbosity << ")" << std::endl;
  std::cerr << "       Arduino_serial_port: Name of the serial device to use "
            << "to talk to the Arduino.  The Arduino must be running "
            << "the vrpn_streaming_arduino program." << std::endl;
  std::cerr << "                    (On windows, something like COM5)" << std::endl;
  std::cerr << "                    (On mac, something like /dev/tty.usbmodem1411)" << std::endl;
  std::cerr << "       Arduino_channel: The channel that has the potentiometer on it" << std::endl;
  std::cerr << "       DEVICE_TYPE: [analog|tracker]" << std::endl;
  std::cerr << "       Device_config_file: Name of the config file that will construct exactly one vrpn_Device-derived device named Analog0 (for Analog) or Tracker0 (for Tracker)" << std::endl;
  std::cerr << "       Device_device_name: Name of the VRPN device to connect to, including server description (example: Analog0@localhost)" << std::endl;
  std::cerr << "       Device_channel: The channel that has the value to test" << std::endl;
  exit(-1);
}

// Helper function that creates a vrpn_Streaming_Arduino given a name
// and connection to use.  It uses the global state telling which channel
// the potentiometer is on to determine how many ports to request.

static vrpn_Analog *CreateStreamingServer(
  const char *deviceName, vrpn_Connection *c)
{
  return new vrpn_Streaming_Arduino(deviceName, c,
                g_arduinoPortName, g_arduinoChannel+1);
}

int main(int argc, const char *argv[])
{
  // Constants that may some day become options.
  size_t REQUIRED_PASSES = 3;
  int TURN_AROUND_THRESHOLD = 7;

  // Parse the command line.
  size_t realParams = 0;
  std::string deviceType;
  std::string deviceConfigFileName;
  int deviceChannel = 0;
  int count = 10;
  bool arrivalTime = false;
  for (size_t i = 1; i < argc; i++) {
    if (argv[i] == std::string("-count")) {
      if (++i > argc) {
        std::cerr << "Error: -count parameter requires value" << std::endl;
        Usage(argv[0]);
      }
      count = atoi(argv[i]);
      if (count < 10) {
        std::cerr << "Error: -count parameter must be >= 10, found "
          << argv[i] << std::endl;
        Usage(argv[0]);
      }
    } else if (argv[i] == std::string("-verbosity")) {
      if (++i > argc) {
        std::cerr << "Error: -verbosity parameter requires value" << std::endl;
        Usage(argv[0]);
      }
      g_verbosity = atoi(argv[i]);
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
        deviceType = argv[i];
        break;
      case 4:
        deviceConfigFileName = argv[i];
        break;
      case 5:
        deviceChannel = atoi(argv[i]);
        break;
      default:
        Usage(argv[0]);
    }
  }
  if (realParams != 5) {
    Usage(argv[0]);
  }

  // Construct the thread to handle the ground-truth potentiometer
  // reading from the Ardiuno.
  DeviceThreadVRPNAnalog  arduino(CreateStreamingServer);

  // Construct the thread to handle the to-be-measured
  // reading from the Device.  If the "config file" name
  // has an '@' sign in it, treat it as a device name and construct
  // a remote device using it as the name.
  DeviceThread *device = NULL;
  size_t at = deviceConfigFileName.find('@');

  if (deviceType == "analog") {
    if (at != std::string::npos) {
      device = new DeviceThreadVRPNAnalog(deviceConfigFileName);
    } else {
      device = new DeviceThreadVRPNAnalog(deviceConfigFileName, "Analog0");
    }
  } else if (deviceType == "tracker") {
    if (at != std::string::npos) {
      device = new DeviceThreadVRPNTracker(deviceConfigFileName);
    } else {
      device = new DeviceThreadVRPNTracker(deviceConfigFileName, "Tracker0");
    }
  } else {
    std::cerr << "Unrecognized device type: " << deviceType << std::endl;
    return -2;
  }

  //-----------------------------------------------------------------
  // Wait until we get at least one report from each device
  // or timeout.  Make sure the report sizes are large enough
  // to support the channels we're reading.
  struct timeval start, now;
  vrpn_gettimeofday(&start, NULL);
  size_t arduinoCount = 0;
  size_t deviceCount = 0;
  std::vector<DeviceThreadReport> r;
  if (g_verbosity > 0) {
    std::cout << "Waiting for reports from all devices (you may need to move them):" << std::endl;
  }
  double lastArduinoValue, lastDeviceValue;
  do {
    r = arduino.GetReports();
    if (r.size() > 0) {
      if (r[0].values.size() <= g_arduinoChannel) {
        std::cerr << "Report size from Arduino: " << r[0].values.size()
          << " is too small for requested channel: " << g_arduinoChannel << std::endl;
        delete device;
        return -3;
      }
      lastArduinoValue = r.back().values[g_arduinoChannel];
    }
    arduinoCount += r.size();

    r = device->GetReports();
    if (r.size() > 0) {
      if (r[0].values.size() <= deviceChannel) {
        std::cerr << "Report size from Device: " << r[0].values.size()
          << " is too small for requested channel: " << deviceChannel << std::endl;
        delete device;
        return -4;
      }
      lastDeviceValue = r.back().values[deviceChannel];
    }
    deviceCount += r.size();

    vrpn_gettimeofday(&now, NULL);
  } while ( ((arduinoCount == 0) || (deviceCount == 0))
            && (vrpn_TimevalDurationSeconds(now, start) < 20) );
  if (arduinoCount == 0) {
    std::cerr << "No reports from Arduino" << std::endl;
    delete device;
    return -5;
  }
  if (deviceCount == 0) {
    std::cerr << "No reports from Device" << std::endl;
    delete device;
    return -6;
  }

  //-----------------------------------------------------------------
  // Produce the slow-motion mapping between the two devices.
  // Keep track of the latest reported value from the device at
  // all times and assign this value to the vector of entries
  // for each possible value for the Arduino report (0-1023).
  // Every time we get a new Ardiuno report, we add the stored
  // Device value to its vector of associated values.  We
  // continue until we have rotated left and right at least
  // the required number of times.
  if (g_verbosity > 0) {
    std::cout << "Producing mapping between devices:" << std::endl;
    std::cout << "  (Rotate slowly left and right " << REQUIRED_PASSES
      << " times)" << std::endl;
  }

  // Clear out all available reports so we start fresh
  arduino.GetReports();
  device->GetReports();

  // Keep shoveling values into the vectors until they have turned
  // around at least 8 times (four up, four down)
  ArduinoComparer aComp;
  size_t requiredTurns = 2 * REQUIRED_PASSES;
  int lastDirection = 1;  //< 1 for going up, -1 for going down  
  double lastExtremum = lastArduinoValue;
  size_t numTurns = 0;
  do {
    // Fill in a default value in case we get no reports.
    double thisArduinoValue = lastArduinoValue;

    // Find the new value for the Arduino and the Device, if any.
    r = arduino.GetReports();
    if (r.size() > 0) {
      thisArduinoValue = r.back().values[g_arduinoChannel];
    }
    r = device->GetReports();
    if (r.size() > 0) {
      lastDeviceValue = r.back().values[deviceChannel];
    }

    // If we have a new Arduino value, add a new entry into the
    // appropriate vector and check to see if we've turned around.
    if (thisArduinoValue != lastArduinoValue) {

      // Add the new value into the appropriate bin.
      aComp.addMapping(thisArduinoValue, lastDeviceValue);

      // See if we have turned around.  If we're going in the same direction
      // we were, we adjust the extremum.  If the opposite, we see if we've
      // gone past the threshold and turn around if so.
      double offset = thisArduinoValue - lastExtremum;
      if (offset * lastDirection > 0) {

        // Going in the same direction, keep moving the extremum value to
        // keep up with how far we have gone.
        lastExtremum = thisArduinoValue;
      } else if (fabs(thisArduinoValue - lastExtremum) > TURN_AROUND_THRESHOLD) {

        // We went in the opposite direction from our overall motion and
        // moved past the threshold.
        lastDirection *= -1;
        lastExtremum = thisArduinoValue;
        if (g_verbosity > 1) {
          std::cout << "  Turned around at value " << lastExtremum << std::endl;
        }
        numTurns++;
      }

      // Store the new value for future tests.
      lastArduinoValue = thisArduinoValue;
    }    
  } while (numTurns < requiredTurns);

  // Compute the range over which we have values and the average value
  // of the readings in each bin to use for our lookup table mapping from
  // Arduino reading to Device reading.
  size_t numInterpolatedValue;
  if (!aComp.constructMapping(numInterpolatedValue)) {
    std::cerr << "Could not construct Arduino mapping." << std::endl;
    delete device;
    return -7;
  }
  if (g_verbosity > 0) {
    std::cout << "Min Arduino value " << aComp.minArduinoValue()
      << " (device value " << aComp.getDeviceValueFor(aComp.minArduinoValue()) << ")" << std::endl;
    std::cout << "Max Arduino value " << aComp.maxArduinoValue()
      << " (device value " << aComp.getDeviceValueFor(aComp.maxArduinoValue()) << ")" << std::endl;
    std::cout << "  (Filled in " << numInterpolatedValue << " skipped values)"
      << std::endl;
  }

  // Have them cycle the rotation the specified number of times
  // moving rapidly and keep track of all of the reports from both the
  // Arduino and the Device.
  //   Keep shoveling values into the vectors until they have turned
  // around at least twice the specified number of times (up and down
  // down again for each)
  if (g_verbosity > 0) {
    std::cout << "Measuring latency between devices:" << std::endl;
    std::cout << "  (Rotate rapidly left and right " << count
      << " times)" << std::endl;
  }

  requiredTurns = 2 * count;
  lastDirection = 1;  //< 1 for going up, -1 for going down  
  lastExtremum = lastArduinoValue;
  numTurns = 0;
  std::vector<DeviceThreadReport> arduinoReports, deviceReports;
  do {
    // Fill in a default value in case we get no reports.
    double thisArduinoValue = lastArduinoValue;

    // Find the new value for the Arduino and the Device, if any.
    r = arduino.GetReports();
    aComp.addArduinoReports(r);
    if (r.size() > 0) {
      thisArduinoValue = r.back().values[g_arduinoChannel];
    }
    r = device->GetReports();
    aComp.addDeviceReports(r);

    // If we have a new Arduino value, check to see if we've turned around.
    if (thisArduinoValue != lastArduinoValue) {

      // See if we have turned around.  If we're going in the same direction
      // we were, we adjust the extremum.  If the opposite, we see if we've
      // gone past the threshold and turn around if so.
      double offset = thisArduinoValue - lastExtremum;
      if (offset * lastDirection > 0) {

        // Going in the same direction, keep moving the extremum value to
        // keep up with how far we have gone.
        lastExtremum = thisArduinoValue;
      } else if (fabs(thisArduinoValue - lastExtremum) > TURN_AROUND_THRESHOLD) {

        // We went in the opposite direction from our overall motion and
        // moved past the threshold.
        lastDirection *= -1;
        lastExtremum = thisArduinoValue;
        if (g_verbosity > 1) {
          std::cout << "  Turned around at value " << lastExtremum << std::endl;
        }
        numTurns++;
      }

      // Store the new value for future tests.
      lastArduinoValue = thisArduinoValue;
    }    
  } while (numTurns < requiredTurns);

  // Compute the latency between the Arduino and the device
  double latency;
  if (!aComp.computeLatency(g_arduinoChannel, deviceChannel, latency, arrivalTime)) {
    std::cerr << "Could not compute latency" << std::endl;
    delete device;
    return -8;
  }
  std::cout << "Error-minimizing latency, device behind Arduino (milliseconds): "
    << latency * 1e3 << std::endl;

  // We're done.  Shut down the threads and exit.
  delete device;
  return 0;
}

