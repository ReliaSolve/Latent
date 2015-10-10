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
#include <DeviceThreadVRPNTracker.h>

// Global state.

unsigned g_verbosity = 2;       //< Larger numbers are more verbose

void Usage(std::string name)
{
  std::cerr << "Usage: " << name << " TrackerName Sensor" << std::endl;
  std::cerr << "       TrackerName: The Name of the tracker to use (e.g., com_osvr_Multiserver/OSVRHackerDevKit0@localhost)" << std::endl;
  std::cerr << "       Sensor: The sensor to read from (e.g., 0)" << std::endl;
  exit(-1);
}

int main(int argc, const char *argv[])
{
  // Parse the command line.
  size_t realParams = 0;
  std::string trackerName;
  int trackerSensor;
  for (size_t i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
        Usage(argv[0]);
    } else switch (++realParams) {
      case 1:
        trackerName = argv[i];
        break;
      case 2:
        trackerSensor = atoi(argv[i]);
        break;
      default:
        Usage(argv[0]);
    }
  }
  if (realParams != 2) {
    Usage(argv[0]);
  }

  //-----------------------------------------------------------------
  // Construct the thread to handle the to-be-measured
  // reading from the Device.
  DeviceThreadVRPNTracker device(trackerName);

  //-----------------------------------------------------------------
  // Wait until we get at least one report from the device
  // or timeout.  Make sure the report sizes are large enough
  // to support the channels we're reading.
  struct timeval start, now;
  vrpn_gettimeofday(&start, NULL);
  std::vector<DeviceThreadReport> r;
  if (g_verbosity > 0) {
    std::cout << "Waiting for reports from tracker (you may need to move it):" << std::endl;
  }
  do {
    r = device.GetReports();
    vrpn_gettimeofday(&now, NULL);
  } while ( (r.size() == 0)
            && (vrpn_TimevalDurationSeconds(now, start) < 5) );
  if (r.size() == 0) {
    std::cerr << "No reports from tracker" << std::endl;
    return -5;
  }

  //-----------------------------------------------------------------
  // Start accumulating reports, analyzing as we go.
  if (g_verbosity > 0) {
    std::cout << "Oscillate the orientation of the HMD at the slowest rate "
      << "that causes image features that would normally be moving opposite "
      << "the rotation (left on the screen when rotating the head to the right) "
      << "are rotating in the same direction as the rotation (left on the screen "
      << "when rotating the head to the right)."
      << std::endl;
    std::cout << "Kill the program using ^C to exit." << std::endl;
  }

  while (true) {

  }

  // We're done.  Shut down the threads and exit.
  return 0;
}

