# ps3-charger
ATMEGAx8 code to emulate enough of a USB host to get a PS3 controller to charge.

## Background

This project is based on work by Jim Paris. Original source and explanation can be found here:

http://ps3.jim.sh/sixaxis/ps3-charger/
https://web.archive.org/web/20120328002711/http://forums.ps2dev.org/viewtopic.php?t=12778

## Modifications

The original code only works for one controller. The easiest way to support multiple controllers was to drive multiple "USB ports" in parallel. This way there is no need to change the ASM code.

Another change was how controller connection/disconnection is detected. Instead of checking whether the only controller is connected or not, it's now checked against the previous state (which controllers were connected previously).

Last change was an update from ATtiny to ATmega simply because I had them in stock. 12MHz crystal was also removed to simplify the PCB. This however requires manually calibrating the RC oscillator to 12MHz.

## Building

A ready made PCB is available on EasyEDA:

link

However programming it requires a custom pogo-connector and calibration of the internal RC oscillator to 12MHz. This can be done by enabling CLKO functionality and connecting a frequency counter to the CLKO pin (PB0).
