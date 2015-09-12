This repository holds programs and instructions for building and programming
a latency test device for virtual-reality (VR) systems.

*arduino_loopback*:
Program to run on an Arduino that will loop back the state of digital
pin 3 and report it as analog 0, presuming that a jumper has been
installed between the two.  It is meant to operate in combination with
the *test_arduino_latency* program.

*test_arduino_latency*:
Program to send commands to the Arduino to toggle the state of the
digital output and measure how long it takes to see a change in the
analog value.  It talks with the *arduino_loopback* program running
on an Arduino to do this.

# Licence
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

