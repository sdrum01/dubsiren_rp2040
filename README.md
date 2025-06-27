# DUB-IY Dubsiren with RP2040 

DUBSIREN_rp2040 is a simple Project, which was initiated to emulate a analog Frequencygenerator, which is used to make some noises for Dub and Reggae.
As result we was creating the Dubsiren "DUB-IY". 

Dubsirens are simple Synthesizers without VCA (Voltage Controlled Amplifier) and VCF (Voltage Controlled Filter), 
but mostly with an VCO (Voltage Controlled Oscillator) which is modulated by an LFO (Low Frequency Generator).


Normally such Sirens were build up with analog circuits, but the Challenge was to develop the same result as digital solution to store the parameters, simplifing the electronic and to make it hackable.

DUB-IY is based on a Raspberry Pi Pico with the CPU RP2040. It is fast enough to emulate VCO and LFO and has enough possibilities to store the data internally.

The Dubsiren creates an Rectangle Tone Signal with adjustable dutycycle.
The Tone can be modulated by 2 LFO and an envelope-generator to modulate the timing parameter. 

Finally, the signal is perfected with an integrated echo effect. 

On the  are following values can be adjusted with Potentiometer and Switches:

* Waveform LFO1 
(Rectangle, Tri-Rectangle, Triangle, Sawtooth, Pulse) 
* Waveform LFO2 (Rectangle, Triangle, Sawtooth) 
* Oscillator Basic Frequency
* Frequency LFO1 and LFO2
* Amount LFO1 and LFO2 (neg.and pos)
* Amount Timing Envelope
* Duty Cycle of Osscillator Waveform
* Retriggering of LFO-Start
* Delay Time (Echo) 
* Feedback (Echo) 
* Master Volume

Only the combination of LFO 1+2 gets many Experimental Siren sounds, but  the Timing Envelope makes the Sound much more dynamic. 



>[!NOTE]
   >
   >This is an non-profit project without the goal to make the big money.
   > its Just for Fun and we exclude any form of liability!
   
waiting list:
to reserve one device or PCB pls write me an email: 
dub-iy@sdrum.elementfx.com





Have a look into the short introduction on youtube:

[DUB-IY INTRODUCTION]( https://youtu.be/Yc06HZqR8gg?si=ktLNwQYJj4XRiWYL) 

**short manual in german**

[Kurzanleitung Deutsch](https://sdrum01.github.io/dubsiren_rp2040/doku/manual.md)



