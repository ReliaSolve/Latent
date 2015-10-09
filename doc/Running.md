This document describes how to run the programs to operate
the *Latent* test device for virtual-reality (VR) systems.

vrpn_device_latency_test
========================

The *vrpn_device_latency_test* program estimates the latency between
a potentiometer attached to an Arduino and a *vrpn_Analog* or *vrpn_Tracker*
device.  These devices can be either run with a local server (described
by a configuration file) or connected to an externally-run server (using
a full VRPN device name).  Running the program with no arguments provides
a usage message:

> Usage: C:\tmp\vs2015_64\vr_latency_tester\INSTALL\bin\vrpn_device_latency_test.exe Arduino_serial_port Arduino_channel DEVICE_TYPE [Device_config_file|Device_device_name] Device_channel [-count N]
>       -count: Repeat the test N times (default 200)
>       Arduino_serial_port: Name of the serial device to use to talk to the Arduino.  The Arduino must be running the vrpn_streaming_arduino program.
>                    (On windows, something like COM5)
>                    (On mac, something like /dev/tty.usbmodem1411)
>       Arduino_channel: The channel that has the potentiometer on it
>       DEVICE_TYPE: [analog|tracker]
>       Device_config_file: Name of the config file that will construct exactly one vrpn_Device-derived device named Analog0 (for Analog) or Tracker0 (for Tracker)
>       Device_device_name: Name of the VRPN device to connect to, including server description (example: Analog0@localhost)
>       Device_channel: The channel that has the value to test



