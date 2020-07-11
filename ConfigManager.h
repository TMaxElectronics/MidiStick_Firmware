#include <stdint.h>
#include <xc.h>
#define _SUPPRESS_PLIB_WARNING
#include <peripheral/nvm.h>

typedef struct{
    char name[24];
    uint32_t AttacTime;
    uint32_t DecayTime;
    uint16_t sustainPower;
    uint16_t sustainEnabled;
    uint32_t releaseTime;
    
    uint32_t fmFreq;
    uint32_t fmAmpl;
    uint32_t amFreq;
    uint32_t amAmpl;
    
    int32_t noteOffset;
    float bendRange;
} MidiProgramm;

typedef struct{
    char name[24];
    uint32_t maxOnTime;
    uint32_t minOnTime;
    
    uint16_t maxDuty;
    uint16_t holdoffTime;
    uint16_t ontimeLimit;
} CoilConfig;

typedef struct{
    uint32_t fwStatus;
    uint32_t fwVersion;
    uint32_t fwCrc;
    uint8_t data[];
} FWUpdate;

//this is where the configuration info is saved
const struct {
    MidiProgramm programm[128]; 
    CoilConfig coils[32];
    char name[24];
} ConfigData __attribute__((aligned(BYTE_PAGE_SIZE),space(prog))) = {.name = "Midi Stick V1.0         "};

//read and write for programm data
unsigned NVM_readProgrammConfig(MidiProgramm * dest, uint8_t index);    //reads config from NVM (Non Volatile Memory) to ram
unsigned NVM_writeProgrammConfig(MidiProgramm * src, uint8_t index);    //writes config to NVM
void NVM_copyProgrammData(MidiProgramm * dst, MidiProgramm * src);      //copies data from one pointer to another. could be done with a memcpy, but I did it like this...
unsigned NVM_isValidProgramm(uint8_t index);                            //checks if the data is valid (first byte of name is not 0)

//read and write for coil data
unsigned NVM_readCoilConfig(CoilConfig * dest, uint8_t index);
unsigned NVM_writeCoilConfig(CoilConfig * src, uint8_t index);
void NVM_copyCoilData(CoilConfig * dst, CoilConfig * src);
unsigned NVM_isCoilConfigValid(uint8_t index);

const char* NVM_getDeviceName();        //returns the pointer to the device name

void NVM_eraseFWUpdate();
void NVM_writeFWUpdate(void* src, uint32_t pageOffset);

void NVM_memclr4096(void* start, uint32_t length);
void NVM_memcpy128(void * dst, void * src, uint32_t length);    //Copy over data aligned to 128 byte boundaries (uses the ROW_PROGRAMM operation)

//I got this code from one of my older projects, which got it from another older project :/ Though it looks like it might be from the plib, which I ant to use as little as possible, because of its deprecation
//So unfortunately i am not able to give credit to who it is from, I belive i got it from a post on the microchip forums, but I really don't remember
//So this toast is to you, unknown savior
void NVM_writeRow(void* address, void * data);
void NVM_writeWord(void* address, unsigned int data);
void NVM_erasePage(void* address);
unsigned int __attribute__((nomips16)) NVM_operation(unsigned int nvmop);
