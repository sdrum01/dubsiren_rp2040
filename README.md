# Dubsiren RP2040

DUBSIREN_rp2040 is a simple Project, which was initiated to emulate a analog Frequencygenerator, which is used to make some noises for Dub and Reggae.
As result we was creating the Dubsiren "DUB-IY". 

Dubsirens are simple Synthesizers without VCA (Voltage Controlled Amplifier) and VCF (Voltage Controlled Filter), 
but mostly with an VCO (Voltage Controlled Oscillator) which is modulated by an LFO (Low Frequency Generator).



Normally such Sirens were build up with analog circuits, but the Challange was to develop the same result as pure digital solution. The
Advantage is the possibility to store all parameters, simplifing the electronic and to make it hackable.

DUB-IY is based on a Raspberry Pi Pico with the CPU RP2040. It is fast enough to emulate VCO and LFO and has enough possibilities to store the data internally.

The Dubsiren creates an Rectangle Tone Signal with adjustable dutycycle.
The Tone can be modulated by 2 LFO and an envelope-generator to modulate the timing parameter. 

On the  are following values can be adjusted with Potentiometer and Switches:

* Waveform LFO1 
** Rectangle
* Waveform LFO2
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


