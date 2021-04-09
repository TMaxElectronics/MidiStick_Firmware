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
#include <xc.h>
#define _SUPPRESS_PLIB_WARNING
#include <peripheral/nvm.h>

#define FW_STATE_NORMAL 0xff
#define FW_STATE_UPDATEREQUEST 0x01     //copy the firmware as normal
#define FW_STATE_VERIFICATION 0x02      //copy the FW and overwrite settings

#ifndef ConfigManagerInc
#define ConfigManagerInc

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
    char name[22];
    uint16_t maxOnTime;
    uint16_t minOnTime;
    
    uint16_t maxDuty;
    uint16_t holdoffTime;
    uint16_t ontimeLimit;
} CoilConfig;

typedef struct{
    char name[24];
    uint8_t ledMode1;
    uint8_t ledMode2;
    uint8_t ledMode3;
    uint8_t auxMode;
    
    uint8_t fwStatus;
    char fwVersion[24];
    uint32_t resMemStart;
    uint32_t resMemEnd;
    char compileDate[20];
    char compileTime[20];
    uint16_t USBPID;
} CFGData;

#define CONFIG_KEEP_CC 0b1
    
typedef struct{
    uint8_t stereoPosition;
    uint8_t stereoWidth;
    uint8_t stereoSlope;
    
    uint8_t flags;
    uint8_t selectedCC;
} EXPCFGData;

typedef struct {
    uint8_t bLength;
    uint8_t bDscType;
    uint16_t string[14];
} USBDevNameHeader;

//this is where the configuration info is saved
typedef struct {
    CoilConfig coils[32];
    CFGData cfg;
    EXPCFGData expCfg;
    USBDevNameHeader devName;
} CONF;

extern const volatile CONF ConfigData; 
extern const volatile uint8_t NVM_mapMem[];
extern const volatile uint8_t NVM_blockMem[];

//read and write for programm data
unsigned NVM_readProgrammConfig(MidiProgramm * dest, uint8_t index);    //reads config from NVM (Non Volatile Memory) to ram
unsigned NVM_writeProgrammConfig(MidiProgramm * src, uint8_t index);    //writes config to NVM
void NVM_copyProgrammData(MidiProgramm * dst, MidiProgramm * src);      //copies data from one pointer to another. could be done with a memcpy, but I did it like this...
unsigned NVM_isValidProgramm(uint8_t index);                            //checks if the data is valid (first byte of name is not 0)
MidiProgramm * NVM_getProgrammConfig(uint8_t id);


//read and write for coil data
unsigned NVM_readCoilConfig(CoilConfig * dest, uint8_t index);
unsigned NVM_writeCoilConfig(CoilConfig * src, uint8_t index);
void NVM_copyCoilData(CoilConfig * dst, CoilConfig * src);
unsigned NVM_isCoilConfigValid(uint8_t index);

void NVM_eraseFWUpdate();
void NVM_writeFWUpdate(void* src, uint32_t pageOffset);

unsigned NVM_writeCFG(CFGData * src);
unsigned NVM_writeExpCFG(EXPCFGData * src);
unsigned NVM_updateDevicePID(uint16_t newPID);

void NVM_memclr4096(void* start, uint32_t length);
void NVM_memcpy4(void * dst, void * src, uint32_t length);
void NVM_memcpy128(void * dst, void * src, uint32_t length);    //Copy over data aligned to 128 byte boundaries (uses the ROW_PROGRAMM operation)

//I got this code from one of my older projects, which got it from another older project :/ Though it looks like it might be from the plib, which I ant to use as little as possible, because of its deprecation
//So unfortunately i am not able to give credit to who it is from, I belive i got it from a post on the microchip forums, but I really don't remember
//So this toast is to you, unknown savior
void NVM_writeRow(void* address, void * data);
void NVM_writeWord(void* address, unsigned int data);
void NVM_erasePage(void* address);
unsigned int __attribute__((nomips16)) NVM_operation(unsigned int nvmop);

uint32_t NVM_getUpdateCRC();
void NVM_crc(uint8_t data);
CFGData * NVM_getConfig();
EXPCFGData * NVM_getExpConfig();
void NVM_commitFWUpdate(unsigned settingsOverwrite);
char * NVM_getFWVersionString();
char * NVM_getBootloaderVersionString() ;
uint32_t NVM_getBootloaderVersion();
void NVM_finishFWUpdate();
void NVM_clearAllCoils();
void NVM_clearAllProgramms();
uint32_t NVM_getSerialNumber();
char * NVM_getFWCompileDate();
char * NVM_getFWCompileTime();

#endif