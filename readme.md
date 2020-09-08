# A MidiStick. What's that?
Its a USB stick sized MIDI over usb compatible interrupter for tesla coils.
It generates the pulses that control the arcs.

This Repo contains the Firmware it uses

# What are the features?
It is a four-voice polyphonic synthesizer that has an ADSR (attack decay sustain release) engine with adjustable parameters, has programmable presets for these effects that can be selected with the MIDI program change command and can impose limits on the output signal, to prevent damage to the attached coil. 

# How far is it?
The hardware is done, with V1.0 supporting HFBR series transmitter and V1.1 supporting both HFBR and IF-E96E ones. There is also an option for an aux output to send audio to anything else OR use an external E-STOP button. It also supports stereo, with each stick getting a unique stereo position.

The software is working as well, but there are still some bugs to fix until V1.0.
The stick currently has four voices, functioning ADSR, can load and save presets for coil and sound parameters, decode MIDI messages coming in via USB and generate the interrupter waveform for a Tesla Coil. And multiple sticks can be used simultainiously to allow for multi-coil setups.

# Releases
Moved this to the [releases page](https://github.com/TMaxElectronics/MidiStick_Firmware/releases)
