# DUB-IY Dub Siren with RP2040 CPU

DUBSIREN_rp2040 is a simple project designed to emulate a analog frequency generator for dub and reggae music. The result is the "DUB-IY" dub siren. 

Dub sirens are simple synthesizers without VCA (Voltage Controlled Amplifier) or VCF (Voltage Controlled Filter), but usually with a VCO (Voltage Controlled Oscillator) modulated by an LFO (Low Frequency Generator).

Such sirens are typically built with analog circuits. The challenge, however, was to optimize the parameter storage of digital solutions, simplify the electronics and to make it hackable.

DUB-IY is based on a Raspberry Pi Pico with an RP2040 CPU. It is fast enough to emulate VCOs and LFOs and offers sufficient options for internal data storage.

The Dub siren creates an square wave signal with an adjustable duty cycle. The sound can be modulated by two LFOs and an envelope generator to modulate the timing parameter. 

Finally, the signal is perfected with an integrated echo effect. 

The following values can be adjusted using potentiometers and switches:

* LFO1 waveform
(square, triple square, triangle, sawtooth, pulse)
* LFO2 waveform (square, triangle, sawtooth)
* Oscillator base frequency
* LFO1 and LFO2 frequency
* LFO1 and LFO2 contribution (negative and positive)
* Timing envelope contribution
* Oscillator waveform duty cycle
* LFO start retriggering
* Delay time (echo)
* Feedback (echo)
* Overall volume

The combination of LFOs 1 and 2 alone produces many experimental siren sounds,
but the timing envelope adds significantly more dynamics to the sound.

>[!NOTE]
   >
   >This is a non-profit project with no goal of making big money.
   >It's purely for fun, and we assume no liability!
   
Waiting list:
to reserve one device or PCB pls write me an email: 
dub-iy@sdrum.elementfx.com





Have a look into the short introduction on youtube:

[DUB-IY INTRODUCTION]( https://youtu.be/Yc06HZqR8gg?si=ktLNwQYJj4XRiWYL) 

**short manual in german**

[Kurzanleitung Deutsch](https://sdrum01.github.io/dubsiren_rp2040)



