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
#include <vrpn_Serial.h>

void Usage(std::string name)
{
  std::cerr << "Usage: " << name << " Serial_port" << std::endl;
  std::cerr << "       Serial_port: Name of the serial device to use "
            << "to talk to the Arduino.  The Arduino must be running "
            << "the arduino_loopback program." << std::endl;
  std::cerr << "                    (On windows, something like COM5)" << std::endl;
  std::cerr << "                    (On mac, something like /dev/tty.usbmodem1411)" << std::endl;
  exit(-1);
}

int main(int argc, const char *argv[])
{
  // Parse the command line.
  if (argc != 2) {
    Usage(argv[0]);
  }
  std::string portName = argv[1];

  // Open the specified serial port with baud rate 115200.
  int port = vrpn_open_commport(portName.c_str(), 115200);
  if (port == -1) {
    std::cerr << "Could not open serial port " << portName << std::endl;
    return -2;
  }

  // Make sure we can get a report from the device.  Reports are ASCII
  // lines with a single number on them, reporting the value in the
  // Analog0.  They should be between 0 and 1023.
  // Wait for up to a couple of seconds to start hearing reports from the
  // device; it resets itself when opened and waits a while to hear if it
  // should read new firmware.

  // We're done.  Close the port and exit.
  vrpn_close_commport(port);
  return 0;
}

