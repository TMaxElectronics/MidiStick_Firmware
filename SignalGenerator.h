/*
    Copyright (C) 2020,2021 TMax-Electronics.de
   
    This file is part of the MidiStick Firmware.

    the MidiStick Firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    the MidiStick Firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the MidiStick Firmware.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef SigGen_Inc
#define SigGen_Inc

#include <xc.h>
#include <stdint.h>

#include "MidiController.h"

#define SIGGEN_DEFAULTVOLUME 0xff

typedef enum {SIGGEN_music4V, SIGGEN_musicSID, SIGGEN_TR, SIGGEN_AUDIO_ZCD} GENMODE;
GENMODE SigGen_genMode;
int32_t SigGen_currAudioPeak[5];

extern uint32_t SigGen_masterVol;

void SigGen_init();

//set the timer period register to the appropriate value for the required frequency. To allow for smooth pitch bend commands the frequency is in 1/10th Hz
void SigGen_setNoteTPR(uint8_t voice, uint32_t freqTenths);

inline void SigGen_timerHandler(uint8_t voice);

void SigGen_setOTLimit(uint16_t limit);

void SigGen_setMaxOT(uint16_t max);

void SigGen_setMinOT(uint16_t min);

void SigGen_setHoldOff(uint16_t ho);

void SigGen_limit();

void SigGen_setTR(uint32_t freq, uint32_t ot, uint32_t burstLength, uint32_t burstFreq);

void SigGen_setMode(GENMODE newMode);

void SigGen_kill();

void SigGen_resetWatchDog();

void SigGen_writeRaw(RAW_WRITE_NOTES_Cmd * raw);

void SigGen_setMasterVol(uint32_t newVol);

void SigGen_resetMasterVol();

void SigGen_handleAudioSample(int16_t sample);

void SigGen_setZCD(int16_t threshold, int16_t thresholdWidth, uint32_t lowpass, uint32_t holdoff, uint32_t sampleEn);

#endif