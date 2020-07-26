#include "ConfigManager.h"
#include "UART32.h"
#include <stdint.h>
#include <string.h>
#define _SUPPRESS_PLIB_WARNING
#include <peripheral/nvm.h>

MidiProgramm defaultProgramm = {"default programm        ", 0, 0, 100, 0, 0, 0, 0, 0, 0, 0, 2};    //default programm has no effects enabled and uses the standard bend range of +-2
CoilConfig defaultCoil = {"default coil            ", 200, 0, 10, 25, 600};                              //default coil must not output anything, so the max on time is set to zero

/*
 * This here is a bit of voodoo. In the linker I have ordered the compiler not to touch NVM above 0x9D020000 when generating code, so this will always be free.
 * It is exactly at the half way point in NVM, so we have got 50% of our flash after this available for new code.
 * 
 * At atartup the bootloader checks the value at 0x9D020000, which indicates the state of the program. 
 * 0 or 0xff means it has been cleared/not programed, and it will jump to normal execution.
 * 0x10 is the code for normal running code. It jumps to execution
 * 0x20 is the code for a pending software update. The bootloader will calculate the CRC and if it matches the one written by the updater, it proceeds to erase the code from 0x9D00000 - 0x9D01ffff, and write to that memory region the new code from 0x9D020000 - 0x9D03fffff
 */

const volatile uint8_t FWUpdate[] __attribute__((address(0x9d020000), space(fwUpgradeReserved))) = {0xaa, 0xee};

//this is where the configuration info is saved
const volatile struct {
    MidiProgramm programm[128]; 
    CoilConfig coils[32];
    CFGData cfg;
} ConfigData __attribute__((aligned(BYTE_PAGE_SIZE),space(prog), address(0x9D01F000 - BYTE_PAGE_SIZE * 4))) = {.cfg.name = "Midi Stick V1.0", .cfg.ledMode1 = 1, .cfg.ledMode2 = 3, .cfg.ledMode3 = 2, .cfg.auxMode = 0, .cfg.fwVersion = "V0.8", .cfg.fwStatus = 0x10, .cfg.resMemStart = ((uint32_t) &ConfigData), .cfg.resMemEnd = ((uint32_t) &ConfigData.cfg.resMemEnd), .cfg.compileDate = __DATE__, .cfg.compileTime = __TIME__};

void NVM_eraseFWUpdate(){
    NVM_memclr4096(&FWUpdate, 0x1f000);
}

void NVM_writeFWUpdate(void* src, uint32_t pageOffset){
    uint32_t offset = (pageOffset * PAGE_SIZE);
    NVM_memclr4096((void*) ((uint32_t) &FWUpdate + offset), PAGE_SIZE);
    NVM_memcpy128((void*) ((uint32_t) &FWUpdate + offset), src, PAGE_SIZE);
    //UART_sendString("written to ", 0); UART_sendHex(&FWUpdate + offset, 1);
}

void NVM_commitFWUpdate(unsigned settingsOverwrite){
    void* pageStart = (void*) &ConfigData.cfg; //the page containing the config data starts here and contains the coil configs as well
    
    //copy page data to ram
    void* buffer = malloc(PAGE_SIZE);
    memcpy(buffer, pageStart, PAGE_SIZE);
    
    ((CFGData *)buffer)->fwStatus = 0x20 | settingsOverwrite;
    
    //erase the page and write data from ram
    NVM_erasePage(pageStart);
    NVM_memcpy128(pageStart, buffer, PAGE_SIZE);
    free(buffer);
}

void NVM_finishFWUpdate(){      //if an update was just performed, we need to reset the command and load the new firmwares info into the flash
    //UART_sendString("addr ", 0); UART_sendHex(ConfigData.cfg.fwVersion, 1);
    if(ConfigData.cfg.fwStatus == 0x10) return;
    void* pageStart = (void*) &ConfigData.cfg; //the page containing the config data starts here and contains the coil configs as well
    CFGData * updatedCFG = (CFGData *) ((uint32_t) &ConfigData.cfg + 0x20000);
    UART_sendString("Info> Firmware was upgraded to ", 0); UART_sendString(updatedCFG->fwVersion, 1);
    
    //copy page data to ram
    CFGData * buffer = malloc(PAGE_SIZE);
    memcpy(buffer, pageStart, PAGE_SIZE);
    
    buffer->fwStatus = 0x10;
    memcpy(buffer->fwVersion, updatedCFG->fwVersion, 24);
    buffer->resMemEnd = updatedCFG->resMemEnd;
    buffer->resMemStart = updatedCFG->resMemStart;
    
    //erase the page and write data from ram
    NVM_erasePage(pageStart);
    NVM_memcpy128(pageStart, buffer, PAGE_SIZE);
    free(buffer);
}

void NVM_memclr4096(void* start, uint32_t length){
    uint32_t currOffset = 0;
    for(;currOffset < length; currOffset += PAGE_SIZE) NVM_erasePage(start + currOffset);
}

unsigned NVM_readProgrammConfig(MidiProgramm * dest, uint8_t index){
    if(!NVM_isValidProgramm(index)){
        NVM_copyProgrammData(dest, &defaultProgramm);
        //UART_sendString("invalid: ", 0); UART_sendHex(ConfigData.programm[index].name[0], 1);
        return 0;
    }
    MidiProgramm curr = ConfigData.programm[index];
    //UART_sendString("valid: ", 0); UART_sendHex(curr.name[0], 1);
    NVM_copyProgrammData(dest, &curr);
    return 1;
}

void NVM_clearAllProgramms(){
    void* pageStart = (void*) &ConfigData;
    NVM_memclr4096(pageStart, sizeof(MidiProgramm) * 128);  //clear the data. the 128 align with flash pages, so we don't need to worry about erasing data from the coil configs
    //UART_sendString("deleted all programms", 1);
}

void NVM_clearAllCoils(){
    void* pageStart = (void*) &ConfigData.coils[0];
    NVM_memclr4096(pageStart, sizeof(CoilConfig) * 32);  //clear the configs, this also aligns with the flash page, so again we don't need to worry about deleting configuration stuff or programms
    //UART_sendString("deleted all coils", 1);
}

unsigned NVM_writeProgrammConfig(MidiProgramm * src, uint8_t index){
    if((uint8_t) ConfigData.programm[index].name[0] == 0xff){     //has the memory been cleared aleady?
        NVM_memcpy4(&ConfigData.programm[index], src, sizeof(MidiProgramm)); //if it is, we only need to write the data
        //UART_sendString("norase", 1);
    }else{
        //UART_sendString("erase, because", 0); UART_sendHex(ConfigData.programm[index].name[0], 1);
        //get start of the current page
        void* pageStart = (void*) &ConfigData + ((index * sizeof(MidiProgramm)) / PAGE_SIZE) * PAGE_SIZE;

        //copy page data to ram
        void* settingsBuffer = malloc(PAGE_SIZE);
        memcpy(settingsBuffer, pageStart, PAGE_SIZE);

        //overwrite old data with the new
        uint32_t dataOffset = (void*) &ConfigData + (index * sizeof(MidiProgramm)) - pageStart; //subtract actual data position from offset to get relative offset in copied data
        NVM_copyProgrammData(settingsBuffer + dataOffset, src);

        //erase the page and write data from ram
        NVM_erasePage(pageStart);
        NVM_memcpy128(pageStart, settingsBuffer, PAGE_SIZE);
        free(settingsBuffer);
    }
}

unsigned NVM_writeCFG(CFGData * src){
    void* pageStart = (void*) &ConfigData.cfg; //the page containing the config data starts here and contains the coil configs as well
    
    //copy page data to ram
    void* settingsBuffer = malloc(PAGE_SIZE);
    memcpy(settingsBuffer, pageStart, PAGE_SIZE);
    
    //overwrite old data with the new
    uint32_t dataOffset = 0; //subtract actual data position from offset to get relative offset in copied data
    //UART_sendString("\r\n\noffset: ", 0); UART_sendString(settingsBuffer + dataOffset, 1); 
    memcpy(settingsBuffer + dataOffset, src, 28);
    
    //UART_sendString("new offset: ", 0); UART_sendString(settingsBuffer + dataOffset, 1); 
    //UART_sendString("new: ", 0); UART_sendString(src->name, 1); 
    
    //erase the page and write data from ram
    NVM_erasePage(pageStart);
    NVM_memcpy128(pageStart, settingsBuffer, PAGE_SIZE);
    //UART_sendString("write:  ", 0); UART_sendString(ConfigData.cfg.name, 1); 
    free(settingsBuffer);
}

unsigned NVM_isValidProgramm(uint8_t index){
    return ConfigData.programm[index].name[0] != 0 && (uint8_t) ConfigData.programm[index].name[0] != 0xff;
}

unsigned NVM_isCoilConfigValid(uint8_t index){
    if(index > 31) return 0;
    return ConfigData.coils[index].name[0] != 0 && (uint8_t) ConfigData.coils[index].name[0] != 0xff;
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
    if(!NVM_isCoilConfigValid(index)){
        NVM_copyCoilData(dest, &defaultCoil);
        return 0;
    }
    CoilConfig curr = ConfigData.coils[index];
    NVM_copyCoilData(dest, &curr);
    return 1;
}

unsigned NVM_writeCoilConfig(CoilConfig * src, uint8_t index){
    if((uint8_t) ConfigData.coils[index].name[0] == 0xff){     //has the memory been cleared aleady?
        NVM_memcpy4(&ConfigData.coils[index], src, sizeof(CoilConfig)); //if it is, we only need to write the data
    }else{
        void* pageStart = (void*) &ConfigData.coils[0];
        void* settingsBuffer = malloc(PAGE_SIZE);
        memcpy(settingsBuffer, pageStart, PAGE_SIZE);
        uint32_t dataOffset = (void*) &ConfigData.coils[index] - pageStart; //subtract actual data position from offset to get relative offset in copied data
        NVM_copyCoilData(settingsBuffer + dataOffset, src);
        //at this point we have our new data in memory and can erase and then write it to NVM
        NVM_erasePage(pageStart);
        NVM_memcpy128(pageStart, settingsBuffer, PAGE_SIZE);
        free(settingsBuffer);
    }
}

void NVM_copyCoilData(CoilConfig * dst, CoilConfig * src){
    dst->holdoffTime = src->holdoffTime;
    dst->ontimeLimit = src->ontimeLimit;
    dst->maxDuty = src->maxDuty;
    dst->maxOnTime = src->maxOnTime;
    dst->minOnTime = src->minOnTime;
    memcpy(dst->name, src->name, 24);
}

void NVM_memcpy128(void * dst, void * src, uint32_t length){
    uint32_t currOffset = 0;
    for(;currOffset < length; currOffset += 128){
        NVM_writeRow(dst + currOffset, src + currOffset);
        //UART_sendString("written to ", 0); UART_sendHex(dst + currOffset, 1);
    }
}

void NVM_memcpy4(void * dst, void * src, uint32_t length){
    uint32_t currOffset = 0;
    for(;currOffset < length; currOffset += 4){
        uint32_t * data = src + currOffset;
        NVM_writeWord(dst + currOffset, *(data));
        //UART_sendString("written to ", 0); UART_sendHex(dst + currOffset, 1);
    }
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
    
    if(NVMCONbits.LVDERR) UART_sendString("smol folt", 1);
    if(NVMCONbits.WRERR) UART_sendString("REEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE", 1);
    
    return(NVMIsError());
}

CFGData * NVM_getConfig(){
    return &ConfigData.cfg;
}

int currCRC = 0xffffffff;
int poly = 0xEDB88320;

void NVM_crc(uint8_t data) {
    int i;
    currCRC ^= data;
    for (i = 0; i < 8; ++i) {
        if (currCRC & 1)
            currCRC = (currCRC >> 1) ^ 0xA001;
        else
            currCRC = (currCRC >> 1);
    }
}

uint32_t NVM_getUpdateCRC(){
    currCRC = 0xffffffff;
    //UART_sendString("calc crc", 1);
    uint32_t currByte = 0;
    uint8_t * currPos = FWUpdate;
    while((uint32_t) currPos < 0x9d040000){
        if(currPos == FWUpdate || currPos > 0x9d03fff0) UART_sendHex(*(currPos), 1);
        NVM_crc(*(currPos++));
    }
    return currCRC;
}

char * NVM_getFWVersionString(){
    return ConfigData.cfg.fwVersion;
}

uint32_t NVM_getBootloaderVersion() {
    return *((uint32_t*) 0x9fc00bec);       //pointer to the bl version
}

uint32_t NVM_getSerialNumber() {
    return *((uint32_t*) 0x9fc00be8);       //pointer to the bl version
}

char * NVM_getBootloaderVersionString() {
    uint32_t version = NVM_getBootloaderVersion();
    uint16_t majorNum = version >> 16;
    uint16_t minorNum = version & 0xffff;
    
    char * buff = malloc(128);
    memset(buff, 0, 128);
    sprintf(buff, "%d.%d", majorNum, minorNum);
    return buff;
}

char * NVM_getFWCompileTime() {
    return ConfigData.cfg.compileTime;
}

char * NVM_getFWCompileDate() {
    return ConfigData.cfg.compileDate;
}
