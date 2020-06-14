# A MidiStick. What's that?
Its a USB stick sized MIDI over usb compatible interrupter for tesla coils.
It generates the pulses that control the arcs.

This Repo contains the Firmware it uses

# What are the features?
The end goal is to have a polyphonic synthesizer that has an ADSR (attack decay sustain release) engine with adjustable parameters, supports modulation of frequency and "amplitude" (which here is pulse width modulation), has programmable presets for these effects that can be selected with the MIDI program change command and can impose limits on the output signal, to prevent damage to the attached coil.

# How far is it?
The hardware is done. The software not quite.
It currently has two voices, functioning ADSR, can load and save presets for coil parameters and sound parameters and can decode MIDI messages coming in via USB.

So no modulation yet.

# Releases
- Beta 1.0: First stable release, missing modulation