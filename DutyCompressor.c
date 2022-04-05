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

#include <stdint.h>
#include "MidiController.h"
#include "SignalGenerator.h"
#include "UART32.h"

#define COMPSTATE_ATTAC 0
#define COMPSTATE_SUSTAIN 1
#define COMPSTATE_RELEASE 2

#define COMP_UNITYGAIN 0x400

static int32_t currGain = 1024;
static uint32_t compressorState = 0; 
static uint32_t compressorSustainCount = 0; 
static uint32_t compressorWaitCount = 0; 

void COMP_compress(){
    if(++compressorWaitCount == 10){
        compressorWaitCount = 0;
        
        if(NVM_getExpConfig()->compressorAttac == 0){
            currGain = COMP_UNITYGAIN;
            return;
        }
    
        uint32_t totalDuty = 0;

        //calculate current dutycycle
        uint8_t c = 0;
        for(; c < MIDI_VOICECOUNT; c++){
            uint32_t ourDuty = (Midi_voice[c].otCurrent * Midi_voice[c].freqCurrent) / 10; //TODO preemtively increase the frequency used for calculation if noise is on
            if(Midi_voice[c].hyperVoiceCount == 2 && Midi_voice[c].noiseCurrent == 0) ourDuty *= 2;
            totalDuty += ourDuty;
        }

        totalDuty = (totalDuty * SigGen_masterVol) / 0xff;
        totalDuty = (totalDuty * currGain) >> 10;

        // are we at > 50% of maxDuty 
        
        //TODO add adjustable parameters
        if(totalDuty > 10000 * Midi_currCoil->maxDuty){
            compressorState = COMPSTATE_ATTAC;
            if((currGain -= NVM_getExpConfig()->compressorAttac) < 0) currGain = 0;

        }else if(totalDuty < 10000 * Midi_currCoil->maxDuty){
            switch(compressorState){
                case COMPSTATE_ATTAC:
                    compressorSustainCount = 0;
                    //continue to next case
                case COMPSTATE_SUSTAIN:
                    if(compressorSustainCount == NVM_getExpConfig()->compressorSustain){
                        compressorState = COMPSTATE_RELEASE;
                        compressorSustainCount = 0;
                    }else{
                        //UART_print("Sustaining: %d\r\n", currGain);
                        compressorState = COMPSTATE_SUSTAIN;
                        compressorSustainCount ++;
                    }
                    break;

                case COMPSTATE_RELEASE:
                    //UART_print("Releasing: %d\r\n", currGain);
                    if(currGain != COMP_UNITYGAIN){ 
                        if((currGain += NVM_getExpConfig()->compressorRelease) >= COMP_UNITYGAIN) currGain = COMP_UNITYGAIN;
                    }
                    break;
            }
        }
    }
}

inline int32_t COMP_getGain(){
    return currGain;
}