# PulseTrain

This is an Arduino MEGA-based Pulse Generation library, developed to primarily run 
valves, stepper motors, LEDs or anything else that relies upon pulse waveform 
output for a finite number of periods, or even time, however you want to think about it.
There's nothing domain specific in this library.


## Features

- DC control by setting period to 0
- Pulse resolution default set at 0.5 us with periods configurable up to 33 ms.
- The number of periods output is a 16 bit value
- attach a single TIMER to as many pins as you'd like so long as they do the same thing
- detach and reattach other PulseTrains to a timer to reconfigure the system on the fly 

## Installation Notes

With Arduino 1.0, add the PulseTrain folder to the "libraries" folder of your sketchbook directory.

You can put the examples in your own sketchbook directory, or in hardware/libraries/PulseTrain/examples, as you prefer.

If you get an error message when building the examples similar to "pulsetrain.h not found", 
it's a problem with where you put the PulseTrain folder. 
The server won't work if the header is directly in the libraries folder.