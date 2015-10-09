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
#include <ArduinoComparer.h>
#include <vrpn_Streaming_Arduino.h>

// Global state.

unsigned g_verbosity = 2;       //< Larger numbers are more verbose
std::string g_arduinoPortName;
int g_arduinoChannel = 0;
int g_arduinoTestChannel = 1;

void Usage(std::string name)
{
  std::cerr << "Usage: " << name << " Arduino_serial_port Potentiometer_channel Test_channel [-count N] [-arrivalTime]" << std::endl;
  std::cerr << "       -count: Repeat the test N times (default 200)" << std::endl;
  std::cerr << "       -arrivalTime: Use arrival time of messages (default is reported sampling time)" << std::endl;
  std::cerr << "       Arduino_serial_port: Name of the serial device to use "
            << "to talk to the Arduino.  The Arduino must be running "
            << "the vrpn_streaming_arduino program." << std::endl;
  std::cerr << "                    (On windows, something like COM5)" << std::endl;
  std::cerr << "                    (On mac, something like /dev/tty.usbmodem1411)" << std::endl;
  std::cerr << "       Potentiometer_channel: The channel that has the potentiometer on it" << std::endl;
  std::cerr << "       Test_channel: The channel that has the test input on it" << std::endl;
  exit(-1);
}

// Helper function that creates a vrpn_Streaming_Arduino given a name
// and connection to use.  It uses the global state telling which channel
// the potentiometer is on to determine how many ports to request.

static vrpn_Analog *CreateStreamingServer(
  const char *deviceName, vrpn_Connection *c)
{
  int num_channels = g_arduinoChannel + 1;
  if (g_arduinoTestChannel > g_arduinoChannel) {
    num_channels = g_arduinoTestChannel + 1;
  }
  return new vrpn_Streaming_Arduino(deviceName, c,
                g_arduinoPortName, num_channels);
}

int main(int argc, const char *argv[])
{
  // Constants that may some day become options.
  size_t REQUIRED_PASSES = 3;
  int TURN_AROUND_THRESHOLD = 7;

  // Parse the command line.
  size_t realParams = 0;
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
        g_arduinoTestChannel = atoi(argv[i]);
        break;
      default:
        Usage(argv[0]);
    }
  }
  if (realParams != 3) {
    Usage(argv[0]);
  }

  // Construct the thread to handle the ground-truth potentiometer
  // reading from the Ardiuno, and also the test channel.
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
    std::cout << "Waiting for reports from Arduino (you may need to move them):" << std::endl;
  }
  double lastArduinoValue, lastDeviceValue;
  do {
    r = arduino.GetReports();
    if (r.size() > 0) {
      if (r[0].values.size() <= g_arduinoChannel) {
        std::cerr << "Report size from Arduino: " << r[0].values.size()
          << " is too small for requested channel: " << g_arduinoChannel << std::endl;
        return -3;
      }
      lastArduinoValue = r.back().values[g_arduinoChannel];

      if (r[0].values.size() <= g_arduinoTestChannel) {
        std::cerr << "Report size from Arduino: " << r[0].values.size()
          << " is too small for requested channel: " << g_arduinoTestChannel << std::endl;
        return -4;
      }
      lastDeviceValue = r.back().values[g_arduinoTestChannel];
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
      lastDeviceValue = r.back().values[g_arduinoTestChannel];
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
    aComp.addDeviceReports(r);
    if (r.size() > 0) {
      thisArduinoValue = r.back().values[g_arduinoChannel];
    }

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
  if (!aComp.computeLatency(g_arduinoChannel, g_arduinoTestChannel, latency, arrivalTime)) {
    std::cerr << "Could not compute latency" << std::endl;
    return -8;
  }
  std::cout << "Error-minimizing latency, device behind Arduino (milliseconds): "
    << latency * 1e3 << std::endl;

  // We're done.  Shut down the threads and exit.
  return 0;
}

