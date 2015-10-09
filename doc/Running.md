# Running *Latent* programs

This document describes how to run the programs to operate
the *Latent* test device for virtual-reality (VR) systems.

## vrpn_device_latency_test

The *vrpn_device_latency_test* program estimates the latency between
a potentiometer attached to an Arduino and a *vrpn_Analog* or *vrpn_Tracker*
device.  These devices can be either run with a local server (described
by a configuration file) or connected to an externally-run server (using
a full VRPN device name).  Running the program with no arguments provides
a usage message:

	Usage: C:\tmp\vs2015_64\vr_latency_tester\INSTALL\bin\vrpn_device_latency_test.exe Arduino_serial_port Arduino_channel DEVICE_TYPE [Device_config_file|Device_device_name] Device_channel [-count N]
	       -count: Repeat the test N times (default 200)
	       Arduino_serial_port: Name of the serial device to use to talk to the Arduino.  The Arduino must be running the vrpn_streaming_arduino program.
	                    (On windows, something like COM5)
	                    (On mac, something like /dev/tty.usbmodem1411)
	       Arduino_channel: The channel that has the potentiometer on it
	       DEVICE_TYPE: [analog|tracker]
	       Device_config_file: Name of the config file that will construct exactly one vrpn_Device-derived device named Analog0 (for Analog) or Tracker0 (for Tracker)
	       Device_device_name: Name of the VRPN device to connect to, including server description (example: Analog0@localhost)
	       Device_channel: The channel that has the value to test

When the program is run, it will ask you to rotate the device slowly to the left and right
within the bounds you will be using to test.  This is used to establish a mapping between
the potentiometer value at a particular orientation and the VRPN sensor value at that
orientation.  The speed of rotation should be slow enough to remove any latency from the
system, presumably doing a left-and-right rotation over a period of about five seconds
will be sufficient.  During the rotations, it reports the potentiometer values at which
the rotation was reversed:

    Waiting for reports from all devices (you may need to move them):
    Producing mapping between devices:
      (Rotate slowly left and right 3 times)
      Turned around at value 955
      Turned around at value 798
      Turned around at value 953
      Turned around at value 798
      Turned around at value 953
      Turned around at value 798

After the three rotations, the program reports the minimum and maximum potentiometer
reading along with the VRPN device reading associated with each endpoint.  It should
be the case that there is a significant difference between the readings at the two
ends.  (What is significant depends on the range of the particular sensor being used.)
For example, the orientation of a tracker sensor on an OSVR HDK might look like this:

    Min Arduino value 790 (device value -0.438398)
    Max Arduino value 963 (device value 0.310125)
      (Filled in 0 skipped values)
    Measuring latency between devices:
      (Rotate rapidly left and right 10 times)

The program also reports how many values between the minimum and maximum were
skipped -- received no readings -- and had to be filled in by linear interpolation.
There should be a small number of these relative to the difference between the
minumum and maximum Arduino value.

As shown above, the program then asks you to rapidly rotate left and right between
the bounds found above.  This rotation does not need to be at a constant speed or
with a constant period.  The program will check time-domain shifts of all of the
readings to find the most-consistent latency offset regardless of speed.  The speed
should be fast enough to expose the latency, so should include rotations that are
as fast as possible.

### Examples

**Windows OSVR HDK**: To connect to an Arduino on serial port COM5 (you can find
out which port using the *Tools/Serial Port* menu in the Arduino programming tool)
with the potentiometer plugged into analog input 0,
and to a running OSVR server connected to an OSVR HDK with the default device names
(*com_osvr_Multiserver/OSVRHackerDevKit0* for the non-predicted inertial tracker
unit and *com_osvr_Multiserver/OSVRHackerDevKitPrediction0* for the predicted one),
checking the rotation around the Z axis (first three channels are translation around
X, Y, Z; next three are rotation), you would use the following command:

	vrpn_device_latency_test.exe COM5 0 tracker com_osvr_Multiserver/OSVRHackerDevKit0@localhost 5

(When the OSVR server is switched over to the IANA-specified OSVR port, the device name will change to com_osvr_Multiserver/OSVRHackerDevKit0@localhost:7728.)


