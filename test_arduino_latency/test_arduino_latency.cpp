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
#include <vrpn_Shared.h>
#include <vrpn_Serial.h>

// Static globals.
static const unsigned char offMsg = '0';
static const unsigned char onMsg = '1';

void Usage(std::string name)
{
  std::cerr << "Usage: " << name << " Serial_port [-count N]" << std::endl;
  std::cerr << "       -count: Repeat the test N times (default 10)" << std::endl;
  std::cerr << "       Serial_port: Name of the serial device to use "
            << "to talk to the Arduino.  The Arduino must be running "
            << "the arduino_loopback program." << std::endl;
  std::cerr << "                    (On windows, something like COM5)" << std::endl;
  std::cerr << "                    (On mac, something like /dev/tty.usbmodem1411)" << std::endl;
  exit(-1);
}

// Set the value to low.
// Make sure the data is sent.
// Return true on success, false on failure.
bool send_msg(int port, unsigned char msg)
{
  if (1 != vrpn_write_characters(port, &msg, 1)) {
    std::cerr << "set_off: Can't write message" << std::endl;
    return false;
  }
  vrpn_drain_output_buffer(port);
  return true;
}

// Read and parse all pending values from the Arduino on the specified port, or
// time out and return -1.  If we got at least one number
// return its value.
int read_latest_value_or_timeout(int port, struct timeval timeout)
{
  unsigned char buffer[128];
  unsigned char *bufPtr = buffer;
  int ret = -1; // If we don't get a valid report, return an error.

  // Loop reading a single character until we time out,
  // then finish reading and parsing the latest report.
  // The first time through, we use our requested timeout and
  // later times use a zero timeout to return as soon as we're out
  // of characters.
  struct timeval myTimeout = timeout;
  while (vrpn_read_available_characters(port, buffer, 1, &myTimeout) == 1) {
    // We're one past the beginning of the buffer
    bufPtr = buffer + 1;

    // Keep reading characters until we get a carriage return or
    // timeout.  Don't overrun the end of the buffer.
    myTimeout = timeout;
    while (vrpn_read_available_characters(port, bufPtr, 1, &myTimeout) == 1) {

      // See if we got a carriage return.  If so, parse and break out.
      if (*bufPtr == '\n') {
        // Convert from unsigned char buffer pointer to signed
        char *charBuf = static_cast<char *>(static_cast<void *>(buffer));
        ret = atoi(charBuf);
//  std::cout << " XXX Got value " << ret << " from string of length " << bufPtr - buffer + 1 << std::endl;
        break;
      }

      // Don't overrun the end of the buffer.
      bufPtr++;
      if (bufPtr - buffer >= sizeof(buffer)) {
        std::cerr << "read_latest_value_or_timeout: Full buffer" << std::endl;
        return -1;
      }

      // Go around for another character, re-setting the timeout
      // because we're in the middle of a string.
      myTimeout = timeout;
    }

    // Set the timeout value for the next time around to 0, so we
    // return as soon as we're out of reports.
    myTimeout.tv_sec = 0;
    myTimeout.tv_usec = 0;
  }

//  static unsigned XXX = 0;
//  std::cout << "XXX Read " << ++XXX << " value " << ret << std::endl;
  return ret;
}
// Keep reading values until we get one that is below the specified
// threshold or take too long to get this result.  If we took too long,
// then return false.
bool wait_for_below_threshold(int port, int threshold, struct timeval timeout)
{
  struct timeval start, end, now;
  vrpn_gettimeofday(&start, NULL);
  end = vrpn_TimevalSum(start, timeout);

  // Read values until we get a negative number (which means a
  // timeout from the read), we get a number below threshold, or
  // we take too long.
  do {
    int val = read_latest_value_or_timeout(port, timeout);
    if (val < 0) { return false; }
    if (val < threshold) { return true; }
    vrpn_gettimeofday(&now, NULL);
  } while (vrpn_TimevalGreater(end, now));

  // Took too long.
  return false;
}

// Keep reading values until we get one that is above the specified
// threshold or take too long to get this result.  If we took too long,
// then return false.
bool wait_for_above_threshold(int port, int threshold, struct timeval timeout)
{
  struct timeval start, end, now;
  vrpn_gettimeofday(&start, NULL);
  end = vrpn_TimevalSum(start, timeout);

  // Read values until we get a negative number (which means a
  // timeout from the read), we get a number below threshold, or
  // we take too long.
  do {
    int val = read_latest_value_or_timeout(port, timeout);
    if (val < 0) { return false; }
    if (val > threshold) { return true; }
    vrpn_gettimeofday(&now, NULL);
  } while (vrpn_TimevalGreater(end, now));

  // Took too long.
  return false;
}


int main(int argc, const char *argv[])
{
  const int THRESHOLD = 512;    //< Halfway between min and max

  // Parse the command line.
  size_t realParams = 0;
  std::string portName;
  int count = 100;
  for (size_t i = 1; i < argc; i++) {
    if (argv[i] == std::string("-count")) {
      if (++i > argc) {
        std::cerr << "Error: -count parameter requires value" << std::endl;
        Usage(argv[0]);
      }
      count = atoi(argv[i]);
      if (count <= 0) {
        std::cerr << "Error: -count parameter must be >= 1, found "
          << argv[i] << std::endl;
        Usage(argv[0]);
      }
    } else if (argv[i][0] == '-') {
        Usage(argv[0]);
    } else switch (++realParams) {
      case 1:
        portName = argv[i];
        break;
      default:
        Usage(argv[0]);
    }
  }
  if (realParams != 1) {
    Usage(argv[0]);
  }

  // Open the specified serial port with baud rate 115200.
  // Wait a bit and then flush all incoming characters.
  int port = vrpn_open_commport(portName.c_str(), 115200);
  if (port == -1) {
    std::cerr << "Could not open serial port " << portName << std::endl;
    return -2;
  }
  vrpn_SleepMsecs(10);
  vrpn_flush_input_buffer(port);

  // Set the value to low, in case it was left high.
  if (!send_msg(port, offMsg)) {
    std::cerr << "Error: Can't write initial off message" << std::endl;
    return -11;
  }

  // Make sure we can get a report from the device.  Reports are ASCII
  // lines with a single number on them, reporting the value in the
  // Analog0.  They should be between 0 and 1023.
  // Wait for up to a couple of seconds to start hearing reports from the
  // device; it resets itself when opened and waits a while to hear if it
  // should read new firmware.
  struct timeval timeout;
  timeout.tv_sec = 3;
  timeout.tv_usec = 0;
  if (!wait_for_below_threshold(port, THRESHOLD, timeout)) {
    std::cerr << "Error: Timeout waiting for initial report" << std::endl;
    return -10;
  }

  // Run through the specified number of iterations.  For each one, let the
  // value from the device settle to a low value, then toggle the binary
  // value to high and wait for it to settle to a high value and measure
  // the latency.  Then toggle it back to low and wait for it to settle and
  // measure the latency.
  std::vector<double> onLatencies;
  std::vector<double> offLatencies;
  for (int i = 0; i < count; i++) {
    // Wait until the value is lower than the threshold, or timeout.
    timeout.tv_sec = 1; timeout.tv_usec = 0;
    if (!wait_for_below_threshold(port, THRESHOLD, timeout)) {
      std::cerr << "Error: Timeout waiting for below threshold, iteration "
        << i << std::endl;
      return -3;
    }

    // Record the time and then request to raise the binary value.
    // Make sure the data is sent.
    struct timeval beforeChange;
    vrpn_gettimeofday(&beforeChange, NULL);
    if (!send_msg(port, onMsg)) {
      std::cerr << "Error: Can't write on message, iteration "
        << i << std::endl;
      return -4;
    }

    // Wait until the value is higher than the threshold, or timeout.
    // and then compute the latency.
    timeout.tv_sec = 1; timeout.tv_usec = 0;
    if (!wait_for_above_threshold(port, THRESHOLD, timeout)) {
      std::cerr << "Error: Timeout waiting for above threshold, iteration "
        << i << std::endl;
      return -5;
    }
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    onLatencies.push_back(vrpn_TimevalDurationSeconds(now, beforeChange));

    // Record the time and then request to lower the binary value.
    // Make sure the data is sent.
    vrpn_gettimeofday(&beforeChange, NULL);
    if (!send_msg(port, offMsg)) {
      std::cerr << "Error: Can't write off message, iteration "
        << i << std::endl;
      return -6;
    }

    // Wait until the value is lower than the threshold, or timeout.
    // and then compute the latency.
    timeout.tv_sec = 1; timeout.tv_usec = 0;
    if (!wait_for_below_threshold(port, THRESHOLD, timeout)) {
      std::cerr << "Error: Timeout waiting for below threshold, iteration "
        << i << std::endl;
      return -7;
    }
    vrpn_gettimeofday(&now, NULL);
    offLatencies.push_back(vrpn_TimevalDurationSeconds(now, beforeChange));
  }

  // Compute and report the latency statistics.
  double offMin = offLatencies[0];
  double offMax = offMin;
  double offSum = 0;

  double onMin = onLatencies[0];
  double onMax = onMin;
  double onSum = 0;

  for (size_t i = 0; i < offLatencies.size(); i++) {
    double offVal = offLatencies[i];
    offSum += offVal;
    if (offMin > offVal) { offMin = offVal; }
    if (offMax < offVal) { offMax = offVal; }

    double onVal = onLatencies[i];
    onSum += onVal;
    if (onMin > onVal) { onMin = onVal; }
    if (onMax < onVal) { onMax = onVal; }
  }
  std::cout << "Off latencies: mean=" << offSum/count
    << ", min=" << offMin
    << ", max=" << offMax
    << std::endl;
  std::cout << "On latencies: mean=" << onSum/count
    << ", min=" << onMin
    << ", max=" << onMax
    << std::endl;

  // We're done.  Close the port and exit.
  vrpn_close_commport(port);
  return 0;
}

