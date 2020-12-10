#include "ConfigManager.h"
#include "UART32.h"
#include "usb lib/usb_ch9.h"
#include "MidiController.h"
#include <stdint.h>
#include <string.h>
#define _SUPPRESS_PLIB_WARNING
#include <peripheral/nvm.h>

#define FORCE_SETTINGS_OVERRIDE 0

MidiProgramm defaultProgramm = {"default programm        ", 0, 0, 100, 0, 0, 0, 0, 0, 0, 0, 2};    //default programm has no effects enabled and uses the standard bend range of +-2
CoilConfig defaultCoil = {"default coil          ", 200, 0, 10, 25, 800};                              //default coil must not output anything, so the max on time is set to zero

/*
 * This here is a bit of voodoo. In the linker I have ordered the compiler not to touch NVM above 0x9D020000 when generating code, so this will always be free.
 * It is exactly at the half way point in NVM, so we have got 50% of our flash after this available for the updated code.
 * 
 * At atartup the bootloader checks the value in the fwStatus variable, which tells it if it should start copying the data from the upper half of NVM to the lower->(the one we are executing from) 
 * 0x10 is the code for normal running code. It jumps to execution
 * 0x20 is the code for a pending software update, while keeping current settings. 
 * 0x21 is the code for a pending software update, but with the settings coied over as well.
 * 
 * The bootloader protects the settings by ignoring NVM between [resMemStart] and [resMemEnd] while copying and erasing. it will start to ignore the first page after teh value in resMemStart, and continue erasing/writing in the first page after resMemEnd
 */

//initialize the default configuration
const volatile CONF ConfigData __attribute__((aligned(BYTE_PAGE_SIZE),space(prog), address(0x9D01F000 - BYTE_PAGE_SIZE * 4))) = {.cfg.name = "Midi Stick", .cfg.ledMode1 = 1, .cfg.ledMode2 = 3, .cfg.ledMode3 = 2, .cfg.auxMode = 0, .cfg.fwVersion = "V0.95"
    , .cfg.fwStatus = 0x10, .cfg.resMemStart = ((uint32_t) &ConfigData), .cfg.resMemEnd = ((uint32_t) &ConfigData.cfg.stereoSlope), .cfg.compileDate = __DATE__, .cfg.compileTime = __TIME__, .devName = {sizeof(USBDevNameHeader),USB_DESCRIPTOR_STRING, {'M','i','d','i','S','t','i','c','k',' ',' ',' ',' ',' '}}, .cfg.USBPID = 0x0002, 
    .cfg.stereoPosition = 64, .cfg.stereoWidth = 16, .cfg.stereoSlope = 255};

const volatile uint8_t FWUpdate[] __attribute__((address(0x9d020000), space(fwUpgradeReserved))) = {FORCE_SETTINGS_OVERRIDE};    //dummy data

//erase all memory possibly containing previous firmware updates
void NVM_eraseFWUpdate(){ 
    NVM_memclr4096(&FWUpdate, 0x1f000);
}

//write a data packet into the NVM, at the given offset
void NVM_writeFWUpdate(void* src, uint32_t pageOffset){
    uint32_t offset = (pageOffset * PAGE_SIZE);
    NVM_memclr4096((void*) ((uint32_t) &FWUpdate + offset), PAGE_SIZE);
    NVM_memcpy128((void*) ((uint32_t) &FWUpdate + offset), src, PAGE_SIZE);
}

//write the fwStatus to 0x20 or 0x21, depending on what is required
void NVM_commitFWUpdate(unsigned settingsOverwrite){
    void* pageStart = (void*) &ConfigData.cfg; //the page containing the config data starts here and contains the coil configs as well
    
    //copy page data to ram
    void* buffer = malloc(PAGE_SIZE);
    memcpy(buffer, pageStart, PAGE_SIZE);
    
    //change the data
    ((CFGData *)buffer)->fwStatus = 0x20 | settingsOverwrite;
    
    //erase the page and write data from ram
    NVM_erasePage(pageStart);
    NVM_memcpy128(pageStart, buffer, PAGE_SIZE);
    free(buffer);
}

void NVM_finishFWUpdate(){      //if an update was just performed, we need to reset the command and load the new firmware's info into the flash
    if(ConfigData.cfg.fwStatus == 0x10) return;
    CFGData * pageStart = (void*) &ConfigData.cfg;
    CFGData * updatedCFG = (CFGData *) ((uint32_t) &ConfigData.cfg + 0x20000);  //this loads the FW information from the region of the updated firmware
    
    CFGData * buffer = malloc(PAGE_SIZE);
    memcpy(buffer, pageStart, PAGE_SIZE);
    
    //make sure all parameters are within their valid range, if one is outside it, write the default value
    //This is needed if a setting was added, as the value at that location is not predictable, and could potenitally cause problems
    //If for example a device with V0.9 was updated to V0.91 (where the custom Pids were added) it would have a pid of 0x3e1, which would effectively soft brick the device
    if(buffer->name[0] == 0){
        strcpy(buffer->name, "Midi Stick");
    }
    
    if(buffer->ledMode1 > LED_TYPE_COUNT){
        buffer->ledMode1 = LED_DATA;
    }
    
    if(buffer->ledMode2 > LED_TYPE_COUNT){
        buffer->ledMode2 = LED_DUTY_LIMITER;
    }
    
    if(buffer->ledMode3 > LED_TYPE_COUNT){
        buffer->ledMode3 = LED_OUT_ON;
    }
    
    if(buffer->auxMode > AUX_MODE_COUNT){
        buffer->auxMode = AUX_AUDIO;
    }
    
    if(buffer->resMemStart != ((uint32_t) &ConfigData)){
        buffer->resMemStart == ((uint32_t) &ConfigData);
    }
    
    if(buffer->resMemEnd != ((uint32_t) &ConfigData.cfg.stereoSlope)){
        buffer->resMemEnd == buffer->resMemStart + sizeof(CFGData);
    }
    
    if(buffer->USBPID != 0x0002 && (buffer->USBPID >= 0x1010 || buffer->USBPID < 0x1000)){
        buffer->USBPID = 0x0002;
    }
    
    if(buffer->stereoPosition > 127){
        buffer->stereoPosition = 64;
    }
    
    if(buffer->stereoWidth > 64){
        buffer->stereoWidth = 64;
    }
    
    if(buffer->stereoSlope == 0){
        buffer->stereoSlope = 0xff;
    }
    
    //overwrite the old data for fwStatus (otherwise the bootloader would just copy stuff again) and the firmware version string
    buffer->fwStatus = 0x10;
    memcpy(buffer->fwVersion, updatedCFG->fwVersion, 24);
    buffer->resMemEnd = updatedCFG->resMemEnd;
    buffer->resMemStart = updatedCFG->resMemStart;
    UART_sendString("Info> Firmware was upgraded to ", 0); UART_sendString(updatedCFG->fwVersion, 1);
    
    //erase the old and write the new data
    NVM_erasePage(pageStart);
    NVM_memcpy128(pageStart, buffer, PAGE_SIZE);
    free(buffer);
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
}                       //checks if the data is valid (first byte of name is not 0)

MidiProgramm * NVM_getProgrammConfig(uint8_t id){
    if(!NVM_isValidProgramm(id)) return &defaultProgramm;
    return &ConfigData.programm[id];
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
    memcpy(settingsBuffer + dataOffset, src, 28);
    
    //calculate the offset to the usb name string
    uint32_t usbNameOffset = (uint32_t) ConfigData.devName.string - (uint32_t) pageStart;
    uint16_t * currTarget = (uint16_t * ) (usbNameOffset + (uint32_t) settingsBuffer);
    
    //convert the 8bit string to 16bit unicode and overwrite the old one
    uint8_t c = 0;
    for(c = 0; c < 21; c++){
        *(currTarget++) = src->name[c];  
    }
    
    //erase the page and write data from ram
    NVM_erasePage(pageStart);
    NVM_memcpy128(pageStart, settingsBuffer, PAGE_SIZE);
    //UART_sendString("write:  ", 0); UART_sendString(ConfigData.cfg.name, 1); 
    free(settingsBuffer);
    return 1;
}

unsigned NVM_updateDevicePID(uint16_t newPID){
    if(newPID != 0x0002 && (newPID < 0x1000 || newPID > 0x1010)) return 0;
    void* pageStart = (void*) &ConfigData.cfg; //the page containing the config data starts here and contains the coil configs as well
    
    //copy page data to ram
    void* settingsBuffer = malloc(PAGE_SIZE);
    memcpy(settingsBuffer, pageStart, PAGE_SIZE);
    
    CFGData * currCFG = settingsBuffer;
    
    currCFG->USBPID = newPID;
    
    //erase the page and write data from ram
    NVM_erasePage(pageStart);
    NVM_memcpy128(pageStart, settingsBuffer, PAGE_SIZE);
    UART_sendString("write:  ", 0); UART_sendInt(ConfigData.cfg.USBPID, 1); 
    free(settingsBuffer);
    return 1;
}


unsigned NVM_SriteStereoParameters(uint8_t c, uint8_t w, uint8_t s){
    void* pageStart = (void*) &ConfigData.cfg; //the page containing the config data starts here and contains the coil configs as well
    
    //copy page data to ram
    void* settingsBuffer = malloc(PAGE_SIZE);
    memcpy(settingsBuffer, pageStart, PAGE_SIZE);
    
    CFGData * currCFG = settingsBuffer;
    
    currCFG->stereoPosition = c;
    currCFG->stereoWidth = w;
    currCFG->stereoSlope = s;
    
    //erase the page and write data from ram
    NVM_erasePage(pageStart);
    NVM_memcpy128(pageStart, settingsBuffer, PAGE_SIZE);
    free(settingsBuffer);
    return 1;
}

unsigned NVM_isValidProgramm(uint8_t index){
    return (ConfigData.programm[index].name[0] != 0 && (uint8_t) ConfigData.programm[index].name[0] != 0xff) && index < 128;
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

//NVM read and write functions:


void NVM_memclr4096(void* start, uint32_t length){
    uint32_t currOffset = 0;
    for(;currOffset < length; currOffset += PAGE_SIZE) NVM_erasePage(start + currOffset);
}

void NVM_memcpy128(void * dst, void * src, uint32_t length){
    uint32_t currOffset = 0;
    for(;currOffset < length; currOffset += 128) NVM_writeRow(dst + currOffset, src + currOffset);
}

void NVM_memcpy4(void * dst, void * src, uint32_t length){
    uint32_t currOffset = 0;
    for(;currOffset < length; currOffset += 4){
        uint32_t * data = src + currOffset;
        NVM_writeWord(dst + currOffset, *data);
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
    LATBCLR = _LATB_LATB15_MASK | _LATB_LATB5_MASK;  //make sure to turn off the output just in case
    
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



//get the pointer to the cfg data
CFGData * NVM_getConfig(){
    return &ConfigData.cfg;
}

//temporary storage of the crc result
int currCRC = 0xffffffff;

//add one byte to the crc calculation
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

//calculate the crc of the update in flash
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


//get software version data:

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
