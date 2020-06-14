#include "ConfigManager.h"
#include <stdint.h>
#include <string.h>
#define _SUPPRESS_PLIB_WARNING
#include <peripheral/nvm.h>

MidiProgramm defaultProgramm = {"default programm        ", 0, 0, 100, 0, 0, 0, 0, 0, 0, 2};    //default programm has no effects enabled and uses the standard bend range of +-2
CoilConfig defaultCoil = {"default coil            ", 0, 0, 0, 0};                              //default coil must not output anything, so the max on time is set to zero

unsigned NVM_readProgrammConfig(MidiProgramm * dest, uint8_t index){
    if(index > 127){
        NVM_copyProgrammData(dest, &defaultProgramm);
        return 0;
    }
    MidiProgramm curr = ConfigData.programm[index];
    if(curr.name[0] == 0){     //programm data is not present -> load default values
        NVM_copyProgrammData(dest, &defaultProgramm);
        return 0;
    }
    NVM_copyProgrammData(dest, &curr);
    return 1;
}

unsigned NVM_writeProgrammConfig(MidiProgramm * src, uint8_t index){
    //get start of the current page
    void* pageStart = (void*) &ConfigData + ((index * sizeof(MidiProgramm)) / BYTE_PAGE_SIZE) * BYTE_PAGE_SIZE;
    
    //copy page data to ram
    void* settingsBuffer = malloc(BYTE_PAGE_SIZE);
    memcpy(settingsBuffer, pageStart, BYTE_PAGE_SIZE);
    
    //overwrite old data with the new
    uint32_t dataOffset = (void*) &ConfigData + (index * sizeof(MidiProgramm)) - pageStart; //subtract actual data position from offset to get relative offset in copied data
    NVM_copyProgrammData(settingsBuffer + dataOffset, src);
    
    //erase the page and write data from ram
    NVM_erasePage(pageStart);
    NVM_memcpy128(pageStart, settingsBuffer, BYTE_PAGE_SIZE);
    free(settingsBuffer);
}

unsigned NVM_isValidProgramm(uint8_t index){
    return ConfigData.programm[index].name[0] != 0;
}

void NVM_copyProgrammData(MidiProgramm * dst, MidiProgramm * src){
    dst->AttacTime = src->AttacTime;
    dst->DecayTime = src->DecayTime;
    dst->releaseTime = src->releaseTime;
    dst->sustainPower = src->sustainPower;
    dst->sustainEnabled = src->sustainEnabled;
    dst->fmFreq = src->fmFreq;
    dst->fmAmpl = src->fmAmpl;
    dst->amFreq = src->amFreq;
    dst->amFreq = src->amFreq;
    dst->noteOffset = src->noteOffset;
    dst->bendRange = src->bendRange;
    memcpy(dst->name, src->name, 24);
}

unsigned NVM_readCoilConfig(CoilConfig * dest, uint8_t index){
    if(index > 32){
        NVM_copyCoilData(dest, &defaultCoil);
        return 0;
    }
    CoilConfig curr = ConfigData.coils[index];
    if(curr.name[0] == 0){     //programm data is not present -> load default values
        NVM_copyCoilData(dest, &defaultCoil);
        return 0;
    }
    NVM_copyCoilData(dest, &curr);
    return 1;
}

unsigned NVM_writeCoilConfig(CoilConfig * src, uint8_t index){
    void* pageStart = (void*) &ConfigData.coils[0];
    void* settingsBuffer = malloc(BYTE_PAGE_SIZE);
    memcpy(settingsBuffer, pageStart, BYTE_PAGE_SIZE);
    uint32_t dataOffset = (void*) &ConfigData.coils[index] - pageStart; //subtract actual data position from offset to get relative offset in copied data
    NVM_copyCoilData(settingsBuffer + dataOffset, src);
    //at this point we have our new data in memory and can erase and then write it to NVM
    NVM_erasePage(pageStart);
    NVM_memcpy128(pageStart, settingsBuffer, BYTE_PAGE_SIZE);
    free(settingsBuffer);
}

unsigned NVM_isCoilConfigValid(uint8_t index){
    if(index > 31) return 0;
    return ConfigData.coils[index].name[0] != 0;
}

void NVM_copyCoilData(CoilConfig * dst, CoilConfig * src){
    dst->limiterMode = src->limiterMode;
    dst->maxDuty = src->maxDuty;
    dst->maxOnTime = src->maxOnTime;
    dst->minOnTime = src->minOnTime;
    memcpy(dst->name, src->name, 24);
}

const char* NVM_getDeviceName(){
    return ConfigData.name;
}

void NVM_memcpy128(void * dst, void * src, uint32_t length){
    uint32_t currOffset = 0;
    for(;currOffset < length; currOffset += 128) NVM_writeRow(dst + currOffset, src + currOffset);
}

void NVM_writeRow(void* address, void * data){
    NVMADDR = KVA_TO_PA((unsigned int) address);
    NVMSRCADDR = KVA_TO_PA((unsigned int) data);
    NVM_operation(0x4003); //NVM Row Program
}

void NVM_writeWord(void* address, unsigned int data){
    NVMADDR = KVA_TO_PA((unsigned int)address);
    NVMDATA = data;
    NVM_operation(0x4001);
}

void NVM_erasePage(void* address){
    NVMADDR = KVA_TO_PA((unsigned int)address);
    NVM_operation(0x4004);
}
 
unsigned int __attribute__((nomips16)) NVM_operation(unsigned int nvmop){
    int int_status;
    int susp;
    
    int_status = INTDisableInterrupts();
    
    NVMCON = NVMCON_WREN | nvmop;
    {
    unsigned long t0 = _CP0_GET_COUNT();
    while (_CP0_GET_COUNT() - t0 < (80/2)*6);
    }

    NVMKEY = 0xAA996655;
    NVMKEY = 0x556699AA;
    NVMCONSET = NVMCON_WR;
    
    while(NVMCON & NVMCON_WR);
    
    NVMCONCLR = NVMCON_WREN;
    INTRestoreInterrupts(int_status);
    
    return(NVMIsError());
}