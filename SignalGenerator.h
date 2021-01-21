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

#include <xc.h>
#include <stdint.h>

#include "MidiController.h"

#define SIGGEN_MAX_OTLIMIT 1000 
#define SIGGEN_MAX_MAXOT 400 
#define SIGGEN_MIN_MINOT 5          //we need this to prvent the note off timer from firing while we are still in the note on interrupt
#define SIGGEN_MIN_HOLDOFF 0 

void SigGen_init();

//set the timer period register to the appropriate value for the required frequency. To allow for smooth pitch bend commands the frequency is in 1/10th Hz
void SigGen_setNoteTPR(uint8_t voice, uint32_t freqTenths);

inline void SigGen_timerHandler(uint8_t voice);

void SigGen_setOTLimit(uint16_t limit);

void SigGen_setMaxOT(uint16_t max);

void SigGen_setMinOT(uint16_t min);

void SigGen_setHoldOff(uint16_t ho);

void SigGen_limit();