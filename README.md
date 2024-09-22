# Dubsiren RP2040

DUBSIREN_rp2040 is a simple Project, which was initiated to emulate a analog Tonegenerator, which is used to make some noises for Dub and Reggae.
Such Dubsirens are simple Synthesizers without VCA (Voltage Controlled Amplifier) and VCF (Voltage Controlled Filter), 
but mostly with an VCO (Voltage Controlled Oscillator) which is modulated by an LFO (Low Frequency Generator).

Of course, it is possible to build up the whole project with analog circuits, but the Challange was to develop the same with a digital kernal. 
Advantage is the possibility to store all parameters, simplifing the electronic and to make it hackable.

The Project is based on a cheap Raspberry Pi Pico with the fast CPU RP2040. It is fast enough to emulate VCO and LFO and has enough possibilities to store the data internally.

The Dubsiren creates an Rectangle Tone Signal with adjustable dutycycle.
The Tone can be modulated by an calculated LFO and an simple envelope-generator

On the RPI are following adjustable values, which can be adjusted with Potentiometer or Switches

* Basic Frequency
* Frequency LFO
* Amount LFO (neg.and pos)
* Amount Envelope
* Envelope Time
* Duty Cycle of Waveform

>[!NOTE]
   >
   >This is an non-profit project without the goal to make the big money.
   > its Just for Fun and we exclude any form of liability!


