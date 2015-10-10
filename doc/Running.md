# Running *Latent* programs

This document describes how to run the programs to operate
the *Latent* test device for virtual-reality (VR) systems.

## arduino_inputs_latency_test

The *arduino_inputs_latency_test* program estimates the end-to-end latency
within a VR system: the latency between a potentiometer attached to an Arduino
and a light sensor attached to the same Ardiuno.  Running the program with no
arguments provides a usage message:

    Usage: C:\tmp\vs2015_64\vr_latency_tester\INSTALL\bin\arduino_inputs_latency_test.exe Arduino_serial_port Potentiometer_channel Test_channel [-count N] [-arrivalTime]
           -count: Repeat the test N times (default 200)
           -arrivalTime: Use arrival time of messages (default is reported sampling time)
           Arduino_serial_port: Name of the serial device to use to talk to the Arduino.  The Arduino must be running the vrpn_streaming_arduino program.
                        (On windows, something like COM5)
                        (On mac, something like /dev/tty.usbmodem1411)
           Potentiometer_channel: The channel that has the potentiometer on it
           Test_channel: The channel that has the test input on it

When using the device constructed according to the [build instructions](./Building.md),


## vrpn_device_latency_test

The *vrpn_device_latency_test* program estimates the device-related
latency within a VR system:
the latency between a potentiometer attached to an Arduino and the reports of 
a *vrpn_Analog* or *vrpn_Tracker*
device.  These devices can be either run with a local server (described
by a configuration file) or connected to an externally-run server (using
a full VRPN device name).  Running the program with no arguments provides
a usage message:

	Usage: C:\tmp\vs2015_64\vr_latency_tester\INSTALL\bin\vrpn_device_latency_test.exe Arduino_serial_port Arduino_channel DEVICE_TYPE [Device_config_file|Device_device_name] Device_channel [-count N] [-arrivalTime]
	       -count: Repeat the test N times (default 200)
	       -arrivalTime: Use arrival time of messages (default reported sampling time)
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

Once again, the program will report the Arduino values where the rotation turned
around, and it will then estimate and report the latency between the potentiometer
readings and the VRPN device readings in milliseconds.

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

### Set-up

**Scene:** The scene being displayed should have a brightness gradient, either
one or more sudden transition from darker to lighter or one or more gradual
transitions in brightness as the device is rotated.  These transitions in the
light signal is what will be aligned with the orientation during the training
session and used to determine the optimal time shift.

**Interference:** Wrapping my hand around the three wires connected to the
phototransistor causes a large oscillation in the signal that is not presento
when it it simply leaning against the screen and being held in place by putty
holding the wires together against a block on the desk.

**Strobing displays:** OLED displays in particular, and others with LED
back-lights, can have brightness strobing over the course of a single frame.

## head_shake_latency_test

The *head_shake_latency_test* program estimates the end-to-end latency of very high-
latency systems by observing the tracker reports while the viewer shakes
the head-mounted display such that the image moves 180 degrees out of phase
with the head rotation.  Running the program with no arguments provides
a usage message:

    Usage: C:\tmp\vs2015_64\vr_latency_tester\INSTALL\bin\head_shake_latency_test.exe[-verbosity N] TrackerName Sensor
           -verbosity: How much info to print (default 2)
           TrackerName: The Name of the tracker to use (e.g., com_osvr_Multiserver/OSVRHackerDevKit0@localhost)
           Sensor: The sensor to read from (e.g., 0)

For this to be effective, it must be possible to rotate the head-mounted
display fast enough that the image gets out of phase; for latencies of
around 100ms or less, it won't be possible to rotate the HMD fast enough
or be certain enough of the phase difference between the motion and the
display.

The HMD can be rotated around any axis.  When there is periodic motion,
the system estimates the period of motion of the largest-moving axis and
reports the period of half a cycle in milliseconds.

This program is run alongside the standard rendering program and is
pointed to the VRPN tracker (and sensor ID) that is controlling the
head.

