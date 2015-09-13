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
#include <string>
#include <iostream>
#include <vector>
#include <DeviceThreadVRPNAnalog.h>
#include <vrpn_Streaming_Arduino.h>

// Global state.

std::string g_arduinoPortName;
int g_arduinoChannel = 0;

void Usage(std::string name)
{
  std::cerr << "Usage: " << name << " Arduino_serial_port Arduino_channel Analog_config_file Analod_channel [-count N]" << std::endl;
  std::cerr << "       -count: Repeat the test N times (default 100)" << std::endl;
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
  // Parse the command line.
  size_t realParams = 0;
  std::string analogConfigFileName;
  int analogChannel = 0;
  int count = 100;
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

  // Measure and report the average update rates of each device.
  // @todo

  // We're done.  Shut down the threads and exit.
  return 0;
}

