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

void Usage(std::string name)
{
  std::cerr << "Usage: " << name << " Arduino_serial_port Arduino_channel Analog_config_file Analog_channel [-count N]" << std::endl;
  std::cerr << "       -count: Repeat the test N times (default 200)" << std::endl;
  std::cerr << "       Arduino_serial_port: Name of the serial device to use "
            << "to talk to the Arduino.  The Arduino must be running "
            << "the vrpn_streaming_arduino program." << std::endl;
  std::cerr << "                    (On windows, something like COM5)" << std::endl;
  std::cerr << "                    (On mac, something like /dev/tty.usbmodem1411)" << std::endl;
  std::cerr << "       Arduino_channel: The channel that has the potentiometer on it" << std::endl;
  std::cerr << "       Analog_config_file: Name of the config file that will construct exactly one vrpn_Analog-derived device named Analog0" << std::endl;
  std::cerr << "       Analog_channel: The channel that has the value to test" << std::endl;
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
  std::string analogConfigFileName;
  int analogChannel = 0;
  int count = 10;
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
        analogConfigFileName = argv[i];
        break;
      case 4:
        analogChannel = atoi(argv[i]);
        break;
      default:
        Usage(argv[0]);
    }
  }
  if (realParams != 4) {
    Usage(argv[0]);
  }

  // Construct the thread to handle the ground-truth potentiometer
  // reading from the Ardiuno.
  DeviceThreadVRPNAnalog  arduino(CreateStreamingServer);

  // Construct the thread to handle the to-be-measured
  // reading from the Analog.
  DeviceThreadVRPNAnalog  analog(analogConfigFileName);

  //-----------------------------------------------------------------
  // Wait until we get at least one report from each device
  // or timeout.  Make sure the report sizes are large enough
  // to support the channels we're reading.
  struct timeval start, now;
  vrpn_gettimeofday(&start, nullptr);
  size_t arduinoCount = 0;
  size_t analogCount = 0;
  std::vector<DeviceThreadReport> r;
  if (g_verbosity > 0) {
    std::cout << "Waiting for reports from all devices (you may need to move them):" << std::endl;
  }
  double lastArduinoValue, lastAnalogValue;
  do {
    r = arduino.GetReports();
    if (r.size() > 0) {
      if (r[0].values.size() <= g_arduinoChannel) {
        std::cerr << "Report size from Arduino: " << r[0].values.size()
          << " is too small for requested channel: " << g_arduinoChannel << std::endl;
        return -2;
      }
      lastArduinoValue = r.back().values[g_arduinoChannel];
    }
    arduinoCount += r.size();

    r = analog.GetReports();
    if (r.size() > 0) {
      if (r[0].values.size() <= analogChannel) {
        std::cerr << "Report size from Analog: " << r[0].values.size()
          << " is too small for requested channel: " << analogChannel << std::endl;
        return -2;
      }
      lastAnalogValue = r.back().values[analogChannel];
    }
    analogCount += r.size();

    vrpn_gettimeofday(&now, nullptr);
  } while ( ((arduinoCount == 0) || (analogCount == 0))
            && (vrpn_TimevalDurationSeconds(now, start) < 20) );
  if (arduinoCount == 0) {
    std::cerr << "No reports from Arduino" << std::endl;
    return -3;
  }
  if (analogCount == 0) {
    std::cerr << "No reports from Analog" << std::endl;
    return -4;
  }

  //-----------------------------------------------------------------
  // Produce the slow-motion mapping between the two devices.
  // Keep track of the latest reported value from the analog at
  // all times and assign this value to the vector of entries
  // for each possible value for the Arduino report (0-1023).
  // Every time we get a new Ardiuno report, we add the stored
  // Analog value to its vector of associated values.  We
  // continue until we have rotated left and right at least
  // the required number of times.
  if (g_verbosity > 0) {
    std::cout << "Producing mapping between devices:" << std::endl;
    std::cout << "  (Rotate slowly left and right " << REQUIRED_PASSES
      << " times)" << std::endl;
  }

  // Clear out all available reports so we start fresh
  arduino.GetReports();
  analog.GetReports();

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

    // Find the new value for the Arduino and the Analog, if any.
    r = arduino.GetReports();
    if (r.size() > 0) {
      thisArduinoValue = r.back().values[g_arduinoChannel];
    }
    r = analog.GetReports();
    if (r.size() > 0) {
      lastAnalogValue = r.back().values[analogChannel];
    }

    // If we have a new Arduino value, add a new entry into the
    // appropriate vector and check to see if we've turned around.
    if (thisArduinoValue != lastArduinoValue) {

      // Add the new value into the appropriate bin.
      aComp.addMapping(thisArduinoValue, lastAnalogValue);

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
  // Arduino reading to Analog reading.
  size_t numInterpolatedValue, numNonMonotonicValues;
  if (!aComp.constructMapping(numInterpolatedValue, numNonMonotonicValues)) {
    std::cerr << "Could not construct Arduino mapping." << std::endl;
    return -5;
  }
  if (g_verbosity > 0) {
    std::cout << "Min Arduino value " << aComp.minArduinoValue()
      << " (analog value " << aComp.getDeviceValueFor(aComp.minArduinoValue()) << ")" << std::endl;
    std::cout << "Max Arduino value " << aComp.maxArduinoValue()
      << " (analog value " << aComp.getDeviceValueFor(aComp.maxArduinoValue()) << ")" << std::endl;
    std::cout << "  (Filled in " << numInterpolatedValue << " skipped values)"
      << std::endl;
    std::cout << "  (Replaced " << numNonMonotonicValues << " non-monotonic values"
      << " out of " << aComp.maxArduinoValue()-aComp.minArduinoValue()+1 << " total)"
      << std::endl;
  }

  // Have them cycle the rotation the specified number of times
  // moving rapidly and keep track of all of the reports from both the
  // Arduino and the Analog.
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
  std::vector<DeviceThreadReport> arduinoReports, analogReports;
  do {
    // Fill in a default value in case we get no reports.
    double thisArduinoValue = lastArduinoValue;

    // Find the new value for the Arduino and the Analog, if any.
    r = arduino.GetReports();
    aComp.addArduinoReports(r);
    if (r.size() > 0) {
      thisArduinoValue = r.back().values[g_arduinoChannel];
    }
    r = analog.GetReports();
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

  // Compute the error associated with the two measurements with no time
  // shift between them.


  // @todo

  // We're done.  Shut down the threads and exit.
  return 0;
}

