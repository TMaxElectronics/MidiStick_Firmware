# A MidiStick. What's that?
Its a USB stick sized MIDI over usb compatible interrupter for tesla coils.
It generates the pulses that control the arcs.

This Repo contains the Firmware it uses

# What are the features?
It is a four-voice polyphonic synthesizer that has an ADSR (attack decay sustain release) engine with adjustable parameters, has programmable presets for these effects that can be selected with the MIDI program change command and can impose limits on the output signal, to prevent damage to the attached coil. 
It will also support modulation and an adjustable stereo position for each stick in the future.

# How far is it?
The hardware is done. The software is now in a not quite V1.0 test state.
It currently has four voices, functioning ADSR, can load and save presets for coil parameters and sound parameters and can decode MIDI messages coming in via USB.

Each stick can be given a unique 21 character nickname inthe config software, and has a unique serial number that the software uses to identify it.
A bootloader is included too (but it has no fallback if the firmware update fails, requiring a pickit or other programmer to recover. A UART fallback option will be a V1.0 feature)

# Releases
- Beta 1.0: First stable release, missing modulation, and just about every feature required...

- V0.9: four voices, a proper duty cycle limiter, a bootloader, a usable configuration utility as well as settings for each synth.
        Beware when updating using a pic-programmer, as it will overwrite the serial number stored in the bootflash at 
