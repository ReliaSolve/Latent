# Building *Latent* measurement devices

This document describes how to build the hardware devices
for the *Latent* test programs for virtual-reality (VR) systems.

## vrpn_device_latency_test/arduino_inputs_latency_test

The *vrpn_device_latency_test* program estimates the latency between
a potentiometer attached to an Arduino and a *vrpn_Analog* or *vrpn_Tracker*
device.  These devices can be either run with a local server (described
by a configuration file) or connected to an externally-run server (using
a full VRPN device name).

The *arduino_inputs_latency_test* does the same, but using two inputs
from an Arduino (the potentiometer on analog input 0 and the other on
analog input 1).  It is designed to work with a phototransistor input
to measure light and has a special filtered version of the
*vrpn_streaming_arduino* software that runs on the Arduino to deal with
the large ouput impedance from having a 500 kOhm resisitor in series
with the phototransistor.

Both of these require a rotary potentiometer to be hooked up to analog
input 0 of the Arduino and the second requires a phototransistor (or other)
circuit to be hooked up to analog input 1.

### Handling high-impedence circuits

When I used two different input channels (0 and 1), I saw
voltages from one affecting the other, even for slowly-
changing signals.  A comment was made on one of the forums
that you should read a channel twice when switching between
channels and throw away the first reading, and that's what
we're doing here.  Doing just one read in between was not
enough to remove this affect on my board, so I did a bunch
of them.  It takes about 100 microseconds for
each read (according to the onling documentation for
analogRead()).

Observation with an oscilloscope revealed that there was an
actual voltage change on the line when the analog reader was
switched to a phototransistor input from a potentiometer.  When it was
switched from the phototransistor back to the potentiometer,
there was no blip on the potentiometer.

The documentation reported a 10kOhm resistor on the photo-
transistor, but measurement found it more like 500 kOhms.
Arduino forums recommend a resistance of more like 10 kOhms
on the inputs.  Switching to an impedance of 25 kOhms dropped
the signal to 0 on the phototransistor.

The decay time for the signal is around 1ms when it is
switched to the phototransistor channel.

**Alternatives:** Stopping reading the potentiometer channel removed the
cross-talk.  Placing a grounded analog input on the channel
that is read immediately ahead of the phototransistor caused
the voltage to come up from 0 over about 1ms; this at least had
a constant scaling factor on the values being read (as opposed
to following a channel with varying voltage).

