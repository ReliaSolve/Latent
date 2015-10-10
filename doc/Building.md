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

### The basic board

A prototype unit was built using an Arduino Uno board.  The particular
board used was the SainSmart UNO ATmega328P, which was purchased
through [Amazon.com](http://smile.amazon.com/SainSmart-ATmega328P-CABLE-Included-Arduino/dp/B006GX8IAY/ref=sr_1_5?s=electronics&ie=UTF8&qid=1422119284&sr=1-5&keywords=arduino+uno) for $17.69.

![Arduino Uno](/arduino_uno.jpg)


### Connecting the devices

Viewing the screen brightness can be done using an IR phototransistor
purchased at Radio Shack, which also operated in the visible-light
range (part number 276-0145, $1.99).  It can be hooked up according
to the SHARP Optoelectronics Photodiode/Phototransistor Application
Circuit (see right). To produce voltages that changed over a range,
a 500kΩ resistor (RL) can be placed in series with the phototransistor.
To make the values be larger for brighter light, a common-collector
configuration with the resistor placed between the transistor output
and ground can be used.  ![Photogransistor circuit](/phototransistor_circuit.png)

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

