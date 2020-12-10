#include <xc.h>
#include <stdint.h>

#include "SignalGenerator.h"
#include "mapper.h"
#include "VMSRoutines.h"

#define MAX_PORTAMENTOTIME 10000

uint8_t MAPTABLE[MAPPER_FLASHSIZE];// __attribute__((address(0x9d020000), space(fwUpgradeReserved)));    //dummy data
/*const struct{
    MAPTABLE_HEADER h0;
    MAPTABLE_ENTRY h0e0;
    MAPTABLE_ENTRY h0e1;
    MAPTABLE_ENTRY h0e2;
    MAPTABLE_HEADER h1;
    MAPTABLE_ENTRY h1e0;
    MAPTABLE_ENTRY h1e1;
    MAPTABLE_ENTRY h1e2;
    MAPTABLE_ENTRY h1e3;
    MAPTABLE_ENTRY h1e4;
    MAPTABLE_ENTRY h1e5;
    MAPTABLE_HEADER h2;
    MAPTABLE_ENTRY h2e0;
} MAPTABLE = {.h0 = {.programNumber = 0, .listEntries = 3}, .h1 = {.programNumber = 1, .listEntries = 6}, .h2 = {.programNumber = 2, .listEntries = 1}
            , .h0e0 = {.startNote = 0, .endNote = 127, .data.VMS_Startblock = &IDLE, .data.flags =     MAP_ENA_DAMPER | MAP_ENA_STEREO | MAP_ENA_VOLUME | MAP_ENA_PITCHBEND | MAP_ENA_PORTAMENTO | MAP_FREQ_MODE, .data.noteFreq = 0, .data.targetOT = 0xff}
            , .h0e1 = {.startNote = 51, .endNote = 61, .data.VMS_Startblock = &BD_IDLE, .data.flags = MAP_ENA_DAMPER | MAP_ENA_STEREO | MAP_ENA_VOLUME | MAP_ENA_PITCHBEND, .data.noteFreq = 200, .data.targetOT = 0xff}
            , .h0e2 = {.startNote = 61, .endNote = 127, .data.VMS_Startblock = &IDLE, .data.flags =   MAP_ENA_DAMPER | MAP_ENA_STEREO | MAP_ENA_VOLUME | MAP_ENA_PITCHBEND | MAP_ENA_PORTAMENTO | MAP_FREQ_MODE, .data.noteFreq = 0, .data.targetOT = 0xff}};
*/

const MAPTABLE_ENTRY MAPPER_DEFMAP = {.data.VMS_Startblock = &DEF_ON, .data.flags = MAP_ENA_DAMPER | MAP_ENA_STEREO | MAP_ENA_VOLUME | MAP_ENA_PITCHBEND | MAP_FREQ_MODE, .data.noteFreq = 0, .data.targetOT = 0xff};

void MAPPER_map(uint8_t voice, uint8_t note, uint8_t velocity, uint8_t channel){
    MAPTABLE_DATA * map = MAPPER_getMap(note, channelData[channel].currentMap);
    
    uint32_t currNoteFreq = 0;
    if(map->flags & MAP_FREQ_MODE){
        int16_t currNote = (int16_t) note + map->noteFreq;
        if(currNote > 127 || currNote < 1) return;
        currNoteFreq = Midi_NoteFreq[currNote];//(( * channelData[channel].bendFactor) / 20000);    //get the required note frequency, including any bender offset
    }else{
        currNoteFreq = map->noteFreq;
    }
    if(map->flags & MAP_ENA_PITCHBEND) currNoteFreq = ((currNoteFreq * channelData[channel].bendFactor) / 20000); else currNoteFreq *= 10;
    
    
    int32_t targetOT = (Midi_currCoil->maxOnTime - Midi_currCoil->minOnTime) * map->targetOT;
    targetOT /= 0xff;
    
    if(map->flags & MAP_ENA_DAMPER){
        targetOT *= channelData[channel].damperPedal ? 6 : 10;
        targetOT /= 10;
    }
    
    if(map->flags & MAP_ENA_STEREO){
        targetOT *= channelData[channel].currStereoVolume;
        targetOT /= 0xff;
    }
    
    if(map->flags & MAP_ENA_VOLUME){
        targetOT *= channelData[channel].currVolume;
        targetOT /= 127;
    }
    
    UART_print("OT=%d;freq=%d\r\n", targetOT, currNoteFreq);
    if(currNoteFreq != 0 && targetOT > 0){
        Midi_voice[voice].currNoteOrigin = channel;
        
        //TODO fix this...
        Midi_voice[voice].noteAge = 0;

        Midi_voice[voice].currNote = note;
        Midi_voice[voice].on = 1;
        Midi_voice[voice].otTarget = targetOT;
        Midi_voice[voice].otCurrent = 0;

        if(map->flags & MAP_ENA_PORTAMENTO){
            int32_t differenceFactor = (Midi_voice[voice].freqTarget * 1000) / currNoteFreq;
            if(differenceFactor < 0) differenceFactor = 0;
            if(differenceFactor > 2000000) differenceFactor = 2000000;
            differenceFactor *= 1000;
            Midi_voice[voice].freqFactor = differenceFactor;
            
            
            int32_t pTimeTarget = (MAX_PORTAMENTOTIME * channelData[channel].portamentoTime) >> 14;
            
            UART_print("differenceFactor = %d (%d), pTimeTarget = %d, ", differenceFactor, Midi_voice[voice].freqFactor, pTimeTarget);
            
            if(pTimeTarget == 0){
                Midi_voice[voice].portamentoParam = (1000000 - differenceFactor);
            }else{
                Midi_voice[voice].portamentoParam = (1000000 - differenceFactor) / pTimeTarget;
            }
            UART_print("portamentoParam = %d\r\n", Midi_voice[voice].portamentoParam);
        }else{
            Midi_voice[voice].freqFactor = 1000000;
        }
        Midi_voice[voice].freqTarget = currNoteFreq;

        Midi_voice[voice].noiseCurrent = 0;
        Midi_voice[voice].noiseTarget = 400;
        Midi_voice[voice].noiseFactor = 0;

        SigGen_setNoteTPR(voice, currNoteFreq);
        
        Midi_voice[voice].map = map;

        VMS_resetVoice(&Midi_voice[voice], map->VMS_Startblock);
    }
}

MAPTABLE_DATA * MAPPER_getMap(uint8_t note, MAPTABLE_HEADER * table){
    uint8_t currScan = 0;
    MAPTABLE_ENTRY * entries = (MAPTABLE_ENTRY *) ((uint32_t) table + sizeof(MAPTABLE_HEADER));
    for(currScan = 0; currScan < table->listEntries; currScan++){
        if(entries[currScan].endNote >= note && entries[currScan].startNote <= note){
            return &entries[currScan].data;
        }
    }
    return &MAPPER_DEFMAP.data;
}

void MAPPER_programChangeHandler(uint8_t channel, uint8_t program){
    channelData[channel].currentMap = MAPPER_findHeader(program);
}

MAPTABLE_HEADER * MAPPER_findHeader(uint8_t prog){
    MAPTABLE_HEADER * currHeader = (MAPTABLE_HEADER *) &MAPTABLE;
    while(currHeader->listEntries != 0){
        if(currHeader->programNumber == prog) return currHeader;
        currHeader += sizeof(MAPTABLE_HEADER) + sizeof(MAPTABLE_ENTRY) * currHeader->listEntries;
        if(((uint32_t) currHeader - (uint32_t) &MAPTABLE) >= 16384) break;
    }
    //return default maptable
    return (MAPTABLE_HEADER *) &MAPTABLE;
}

void MAPPER_volumeHandler(uint8_t channel){
    uint8_t currChannel = 0;
    for(;currChannel < 4; currChannel ++){
        if(Midi_voice[currChannel].currNoteOrigin == channel && Midi_voice[currChannel].on){
            MAPTABLE_DATA * map = Midi_voice[currChannel].map;
            
        
            int32_t targetOT = (Midi_currCoil->maxOnTime - Midi_currCoil->minOnTime) * map->targetOT;
            targetOT /= 0xff;

            if(map->flags & MAP_ENA_DAMPER){
                targetOT *= channelData[channel].damperPedal ? 6 : 10;
                targetOT /= 10;
            }

            if(map->flags & MAP_ENA_STEREO){
                targetOT *= channelData[channel].currStereoVolume;
                targetOT /= 0xff;
            }

            if(map->flags & MAP_ENA_VOLUME){
                targetOT *= channelData[channel].currVolume;
                targetOT /= 127;
            }
            
            Midi_voice[currChannel].otTarget = targetOT;
            
            if(Midi_voice[currChannel].otFactor > 0){
                Midi_voice[currChannel].otCurrent = (Midi_voice[currChannel].otTarget * Midi_voice[currChannel].otFactor) / 1000000 + Midi_currCoil->minOnTime;
            }
        }
    }
}

void MAPPER_bendHandler(uint8_t channel){
    uint8_t currChannel = 0;
    for(;currChannel < 4; currChannel ++){
        if(Midi_voice[currChannel].currNoteOrigin == channel && Midi_voice[currChannel].on){
            MAPTABLE_DATA * map = Midi_voice[currChannel].map;
            if(map->flags & MAP_ENA_PITCHBEND == 0) continue;
            
            uint32_t currNoteFreq = 0;
            if(map->flags & MAP_FREQ_MODE){
                int16_t currNote = (int16_t) Midi_voice[currChannel].currNote + map->noteFreq;
                if(currNote > 127 || currNote < 1) return;
                currNoteFreq = Midi_NoteFreq[currNote];
            }else{
                currNoteFreq = map->noteFreq;
            }
            if(map->flags & MAP_ENA_PITCHBEND) currNoteFreq = ((currNoteFreq * channelData[channel].bendFactor) / 20000); else currNoteFreq *= 10;
            
            Midi_voice[currChannel].freqTarget = currNoteFreq;
            Midi_voice[currChannel].freqCurrent = ((Midi_voice[currChannel].freqTarget >> 4) * Midi_voice[currChannel].freqFactor) / 62500;  //62500 is 1000000 >> 4
            
            SigGen_setNoteTPR(currChannel, Midi_voice[currChannel].freqCurrent);
        }
    }
}

