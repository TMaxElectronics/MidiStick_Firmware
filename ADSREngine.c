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
#include <string.h>

#include "MidiController.h"
#include "SignalGenerator.h"
#include "UART32.h"
#include "ADSREngine.h"

#include "DLL.h"
#include "VMSRoutines.h"
#include "HIDController.h"

DLLObject * VMS_listHead = 0;
uint8_t * VMSBLOCKS = 0;

//256 value sin lookup table x := [0, pi)
const int32_t SINTABLE[] = {0, 6135, 12270, 18403, 24533, 30660, 36782, 42898, 49008, 55111, 61205, 67290, 73365, 79429, 85480, 91519, 97545, 103555, 109550, 115529, 121490, 127432, 133356, 139259, 145142, 151002, 156840, 162655, 168444, 174209, 179947, 185658, 191341, 196996, 202620, 208214, 213777, 219308, 224805, 230269, 235698, 241091, 246449, 251769, 257051, 262294, 267498, 272662, 277785, 282865, 287904, 292898, 297849, 302755, 307615, 312429, 317196, 321915, 326586, 331207, 335779, 340300, 344770, 349188, 353553, 357865, 362123, 366327, 370475, 374568, 378604, 382583, 386505, 390368, 394173, 397918, 401603, 405228, 408792, 412294, 415734, 419112, 422426, 425677, 428864, 431986, 435043, 438035, 440960, 443819, 446612, 449337, 451994, 454583, 457104, 459556, 461939, 464253, 466496, 468669, 470772, 472803, 474764, 476653, 478470, 480215, 481888, 483488, 485015, 486469, 487851, 489158, 490392, 491552, 492638, 493650, 494588, 495451, 496239, 496953, 497592, 498156, 498645, 499059, 499397, 499661, 499849, 499962, 500000, 499962, 499849, 499661, 499397, 499059, 498645, 498156, 497592, 496953, 496239, 495451, 494588, 493650, 492638, 491552, 490392, 489158, 487851, 486469, 485015, 483488, 481888, 480215, 478470, 476653, 474764, 472803, 470772, 468669, 466496, 464253, 461939, 459556, 457104, 454583, 451994, 449337, 446612, 443819, 440960, 438035, 435043, 431986, 428864, 425677, 422426, 419112, 415734, 412294, 408792, 405228, 401603, 397918, 394173, 390368, 386505, 382583, 378604, 374568, 370475, 366327, 362123, 357865, 353553, 349188, 344770, 340300, 335779, 331207, 326586, 321915, 317196, 312429, 307615, 302755, 297849, 292898, 287904, 282865, 277785, 272662, 267498, 262294, 257051, 251769, 246449, 241091, 235698, 230269, 224805, 219308, 213777, 208214, 202620, 196996, 191341, 185658, 179947, 174209, 168444, 162655, 156840, 151002, 145142, 139259, 133356, 127432, 121490, 115529, 109550, 103555, 97545, 91519, 85480, 79429, 73365, 67290, 61205, 55111, 49008, 42898, 36782, 30660, 24533, 18403, 12270, 6135};

void * lastData = 0;

void VMS_init(){
    
    //if an erase has failed previously we'd have a half empty list which would mess stuff up. If we find one we erase everything (might be very slow...)
    if(!HID_erasePending){
        UART_sendString("start check... ", 0);
        VMS_BLOCK * cb = &(((VMS_BLOCK*) NVM_blockMem)[0]);
        if(cb->uid == 0xffffffff || cb->uid == 0){
            uint32_t currIndex = 1;
            for(;currIndex < BLOCKMEM_SIZE / sizeof(VMS_BLOCK); currIndex++){
                cb = &(((VMS_BLOCK*) NVM_blockMem)[currIndex]);
                if(cb->uid != 0xffffffff && cb->uid != 0){
                    UART_sendString("error found! starting erase", 1);
                    Midi_setEnabled(0);
                    HID_erasePending = 1;
                    HID_currErasePage = NVM_blockMem;
                    break;
                }
            }
        }
        UART_sendString("done!", 1);
    }
    
    VMS_listHead = createNewDll();
}

void VMS_run(){
    if(VMS_listHead == 0) return;
    if(VMS_listHead->next == VMS_listHead) return;
    
    DLLObject * currObject = VMS_listHead->next;
    
    while(currObject != VMS_listHead){
        VMS_listDataObject * currBlock = currObject->data;
        lastData = currObject;
        currObject = currObject->next;
    
        if(SYS_getTime() > currBlock->nextRunTime){
            if(VMS_calculateValue(currBlock)){
                currBlock->nextRunTime = SYS_getTime() + currBlock->period * 24;
            }
        }
    
    }
    
}

inline uint32_t SYS_timeSince(uint32_t start){
    return (_CP0_GET_COUNT() - start) / 24;
}

inline uint32_t SYS_getTime(){
    return _CP0_GET_COUNT();
}

void VMS_resetVoice(SynthVoice * voice, VMS_BLOCK * startBlock){
    if(!(startBlock > 0x9d000000 && startBlock < 0xa0010000)) return;
    if(startBlock->uid == 0xffffffff || startBlock->uid == 0) return;
    //if(voice->id > 0) __builtin_software_breakpoint();
    DLLObject * currObject = VMS_listHead->next;
    VMS_listDataObject * data;
    //UART_sendString("reset voice", 1);
    
    while(currObject != VMS_listHead){
        data = currObject->data;
        if(data->targetVoice == voice){
            //UART_print("removed block 0x%08x at 0x%08x\r\n", data->block, data);
            
            switch(data->block->type){
                case VMS_SIN:
                    free(data->data);
                    break;
            }

            free(data);
            currObject = currObject->next;
            DLL_remove(currObject->prev);
        }else{
            currObject = currObject->next;
        }
    }
    
    VMS_addBlockToList(startBlock, voice);
}

int32_t VMS_getKnownValue(KNOWN_VALUE ID, SynthVoice * voice){
    //UART_print("get %d\r\n", ID);
    switch(ID){
        case maxOnTime:
            return Midi_currCoil->maxOnTime;
        case minOnTime:
            return Midi_currCoil->minOnTime;
        case otCurrent:
            return voice->otCurrent;
        case otTarget:
            return ((voice->otTarget * channelData[voice->currNoteOrigin].currVolume * channelData[voice->currNoteOrigin].currStereoVolume) / 32385);
        case otFactor:
            return voice->otFactor;
        case freqCurrent:
            return voice->freqCurrent;
        case freqTarget:
            return voice->freqTarget;
        case freqFactor:
            return voice->freqFactor;
        case pTime:
            return voice->portamentoParam;
        case circ1:
            return voice->circ1;
        case circ2:
            return voice->circ2;
        case circ3:
            return voice->circ3;
        case circ4:
            return voice->circ4;
            
        case CC_102 ... CC_119:
            return channelData[voice->currNoteOrigin].parameters[ID - CC_102];
    }
    return 0;
}

void VMS_setKnownValue(KNOWN_VALUE ID, int32_t value, SynthVoice * voice){
    switch(ID){
        //all other values can't be written by VMS
        default:
            return;
            
        case otCurrent:
            voice->otCurrent = value;
            break;
        case onTime:
            if(value == 0){
                voice->otFactor = 0;
                voice->otCurrent = 0;
            }else{
                voice->otFactor = value;
                voice->otCurrent = (voice->otTarget * value) / 1000000 + Midi_currCoil->minOnTime;
                //UART_print("OT is now %d", Midi_voice[0].otCurrent);
            }
            break;
        case freqCurrent:
            voice->freqCurrent = value;
            voice->periodCurrent = 1000000 / value;
            break;
        case frequency:
            voice->freqFactor = value;
            //UART_print("factor target = %d;  ", value);
            value >>= 4;                                                //we need to pre scale the factor to prevent 32bit integer overflow
            //UART_print("factor target >>= %d;  ", value);
            voice->freqCurrent = (voice->freqTarget * value) / 62500;  //62500 is 1000000 >> 4
            //UART_print("value = %d;  \r\n", voice->freqCurrent);       //we need to pre scale the factor to prevent 32bit integer overflow
            
            SigGen_setNoteTPR(voice->id, voice->freqCurrent);
            break;
        case noise:
            voice->noiseFactor = value;
            voice->noiseCurrent = (voice->noiseTarget * value) / 1000000;
            break;
        case circ1:
            voice->circ1 = value;
            //UART_print("circ=%d (%d)\r\n", voice->circ1, value);
            break;
    }
    SigGen_limit();
}

int32_t VMS_getParam(VMS_BLOCK * block, SynthVoice * voice, uint8_t param){
    switch(param){
        case 1:
            return (block->param1 > 0 && block->param1 < KNOWNVAL_MAX) ? VMS_getKnownValue(block->param1, voice) : block->param1;
        case 2:
            return (block->param2 > 0 && block->param2 < KNOWNVAL_MAX) ? VMS_getKnownValue(block->param2, voice) : block->param2;
    }
    return 0;
}

int32_t VMS_getCurrentFactor(KNOWN_VALUE ID, SynthVoice * voice){
    switch(ID){
        default:
            return 0;
        case onTime:
            return voice->otFactor;
        case frequency:
            return voice->freqFactor;
        case noise:
            return voice->noiseFactor;
        case circ1:
            return voice->circ1;
    }
    return 0;
}

unsigned VMS_hasReachedThreshold(VMS_BLOCK * block, int32_t factor, DIRECTION customDirection){
    switch(customDirection){
        case RISING:
            return factor >= block->targetFactor;
        case FALLING:
            return factor <= block->targetFactor;
        case NONE:
            return 0;
    }
}

void VMS_removeBlockFromList(VMS_listDataObject * target){
    DLLObject * currObject = VMS_listHead->next;
    if(currObject == VMS_listHead) return;
    
    VMS_listDataObject * data = currObject->data;
    
    while(data != target){
        currObject = currObject->next;
        if(currObject == VMS_listHead) return;
        data = currObject->data;
    }
    
    switch(target->block->type){
        case VMS_SIN:
            //UART_print("DS (0x%08x) ", target->data);
            free(target->data);
            break;
    }
    
    //UART_print("removed block 0x%08x at 0x%08x for voice %d\r\n", target->block, data, data->targetVoice->id);
    //if(data->data > 0xa0000000 && data->data < 0xa0010000) free(data->data);
    
    free(data);
    DLL_remove(currObject);
}

void VMS_addBlockToList(VMS_BLOCK * block, SynthVoice * voice){
    if(block == 0) return;
    
    VMS_listDataObject * data = malloc(sizeof(VMS_listDataObject));
    memset(data, 0, sizeof(VMS_listDataObject));
    
    data->block = block;
    data->targetVoice = voice;
    data->period = block->period;
    data->nextRunTime = SYS_getTime();
    
    int32_t param1 = VMS_getParam(block, voice, 1);
    
    switch(block->type){
        case VMS_EXP:
            if(param1 > 1000){
                data->thresholdDirection = RISING;
            }else{
                data->thresholdDirection = FALLING;
            }
            //UART_print("exp: param = %d => %s\r\n", param1, (data->thresholdDirection == RISING) ? "rising" : "falling");
            break;
        case VMS_EXP_INV:
            if(param1 > 1000){
                data->thresholdDirection = FALLING;
            }else{
                data->thresholdDirection = RISING;
            }
            break;
        case VMS_LIN:
            if(param1 > 0){
                data->thresholdDirection = RISING;
            }else{
                data->thresholdDirection = FALLING;
            }
            //UART_print("LIN: param = %d => %s\r\n", param1, (data->thresholdDirection == RISING) ? "rising" : "falling");
            break;
        case VMS_SIN:
            data->thresholdDirection = NONE;
            break;
        default:
            data->thresholdDirection = (block->targetFactor < VMS_getCurrentFactor(block->target, voice)) ? FALLING : RISING;
            //UART_print("tfrom %d to %d (%s)\r\n", VMS_getCurrentFactor(block->target, voice), block->targetFactor, (data->thresholdDirection == FALLING) ? "falling" : "rising");
            break;
    }
    
    
    switch(block->type){
        case VMS_SIN:
            data->data = malloc(sizeof(SINE_DATA));
            memset(data->data, 0, sizeof(SINE_DATA));
            break;
    }
    
    DLL_add(data, VMS_listHead);
}

void VMS_nextBlock(VMS_listDataObject * data, unsigned blockSet){
    SynthVoice * voice = data->targetVoice;
    VMS_BLOCK * block = data->block;
    
    if(blockSet){
        uint8_t c = 0;
        for(; c < VMS_MAX_BRANCHES; c++){
            if(block->nextBlocks[c] == VMS_DIE){
                return;
            }
            if(block->nextBlocks[c] > 0x100000 && block->nextBlocks[c] < 0x9d04ffff) VMS_addBlockToList(block->nextBlocks[c], voice);
        }
    }else{
        if(block->offBlock != 0){
            VMS_addBlockToList(block->offBlock, voice);
        }
    }
    VMS_removeBlockFromList(data);
}

void VMS_clear(){
    if(VMS_listHead == 0) return;
    DLLObject * currObject = VMS_listHead->next;
    VMS_listDataObject * data;
    
    while(currObject != VMS_listHead){
        data = currObject->data;
        if(data > 0xa0000000 && data < 0xa0010000){
            switch(data->block->type){
                case VMS_SIN:
                    free(data->data);
                    break;
            }

            free(data);
        }
        currObject = currObject->next;
        DLL_remove(currObject->prev);
    }
}

unsigned VMS_calculateValue(VMS_listDataObject * data){
    lastData = data;
    SynthVoice * voice = data->targetVoice;
    VMS_BLOCK * block = data->block;
    
    if(block->behavior == NORMAL){
        if(!channelData[voice->currNoteOrigin].sustainPedal && !voice->on){
            VMS_nextBlock(data, 0);
            return 0;
        }
    }else{
        if(voice->on){
            VMS_nextBlock(data, 0);
            return 0;
        }
    }
    
    int32_t param1 = (block->flags & VMS_FLAG_ISVARIABLE_PARAM1) ? VMS_getKnownValue(block->param1, voice) : block->param1;
    int32_t param2 = (block->flags & VMS_FLAG_ISVARIABLE_PARAM2) ? VMS_getKnownValue(block->param2, voice) : block->param2;
    int32_t param3 = (block->flags & VMS_FLAG_ISVARIABLE_PARAM3) ? VMS_getKnownValue(block->param3, voice) : block->param3;
    
    //if(block->flags & VMS_FLAG_ISVARIABLE_PARAM1) UART_print("param1 = %d", param1);
    
    int32_t currFactor = VMS_getCurrentFactor(block->target, voice);
    
    SINE_DATA * d = (SINE_DATA *) data->data;
    int32_t currFactorDiff;
    
    switch(block->type){
        case VMS_EXP:
            if(param1 > 1000 && currFactor < 1000){
                currFactor = 1000;
            }
            currFactor = (currFactor * param1) / 1000;
            
            if(param1 < 1000 && currFactor < 1000){
                currFactor = 0;
            }
            
            break;
            
        case VMS_EXP_INV:
            currFactorDiff = abs(block->targetFactor - currFactor);
            currFactorDiff = (currFactorDiff * param1) / 1000;
            if(currFactorDiff < 10000){
                currFactorDiff = 0;
            }
            currFactor = block->targetFactor - currFactorDiff;
            break;
            
        case VMS_LIN:
            currFactor += param1;
            //if(block->target == frequency) UART_print("frequency lin: factor = %d, param1 = %d\r\n", currFactor, param1);
            break;
        case VMS_SIN:
            d->currCount += param3;
            if(d->currCount > 0xff) d->currCount = -0xff;
            currFactor = ((param1 * qSin(d->currCount)) / 1000);
            currFactor += param2;
            if(currFactor < 0) currFactor = 0;
            break;
        case VMS_JUMP:
            currFactor = block->targetFactor;
            break;
    }
    
    if(VMS_hasReachedThreshold(block, currFactor, data->thresholdDirection)){
        currFactor = block->targetFactor;
        VMS_nextBlock(data, 1);
        VMS_setKnownValue(block->target, currFactor, voice);
        return 0;
    }
    
    VMS_setKnownValue(block->target, currFactor, voice);
    return 1;
}

int32_t qSin(int32_t x){
    int32_t xt = abs(x) % 0xff;
    
    if(xt == 0) return 0;
    return (x > 0) ? SINTABLE[xt] : -SINTABLE[xt];
}