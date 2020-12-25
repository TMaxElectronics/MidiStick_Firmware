/*
 * File:   HIDManager.c
 * Author: Thorb
 *
 * Created on 7. Dezember 2020, 17:27
 */


#include <xc.h>
#include <string.h>

#include "HIDController.h"
#include "usb lib/usb.h"
#include "ADSREngine.h"
#include "mapper.h"
#include "UART32.h"

#define VMS_RAMSIZE 4096
#define PROTOCOL_VERSION 1

uint32_t lastVMSBlock = 0;
enum {LISTSTATE_NORMAL, LISTSTATE_UNLINKED} HID_blocklistState;

MAPTABLE_HEADER * HID_currMapHeader = 0;
MAPTABLE_HEADER * HID_currMapBuffer = 0;
VMS_BLOCK * VMS_currWriteBuffer = 0;
void * VMS_currWriteOffset = 0;

VMS_BLOCK * VMS_find(uint32_t uid);
MAPTABLE_HEADER * MAPPER_findFreeSpace(uint32_t size);
void VMS_relinkMaptable(MAPTABLE_HEADER * listStart);
VMS_BLOCK * VMS_findFreeSpace();
void VMS_unlink(VMS_BLOCK * block);
void VMS_unlinkMapEntry(MAPTABLE_ENTRY * entry);
void VMS_relinkAll();
void VMS_relinkRamList();

void HID_parseCMD(uint8_t * input, uint8_t * output, USB_HANDLE handle, uint8_t dataSize) {
    if(input[0] == PROTOCOL_VERSION){
        output[0] = 1;
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
    
    }else if(input[0] == USB_CMD_VMS_CLEARBLOCKS){
        Midi_setEnabled(0);
        if(VMS_currWriteBuffer != 0){
            free(VMS_currWriteBuffer);
            VMS_currWriteBuffer = 0;
        }
        
        HID_erasePending = 1;
        HID_currErasePage = NVM_blockMem;
        
        output[0] = 1;
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
        
    }else if(input[0] == USB_CMD_VMS_ERASECHECK){
        output[0] = USB_CMD_VMS_ERASECHECK;
        output[1] = 0;
        output[2] = 0;
        output[3] = 0;
        output[4] = HID_erasePending;
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
        
    }else if(input[0] == USB_CMD_VMS_WRITEBLOCK){
        if(Midi_enabled) Midi_setEnabled(0);
        if(VMS_currWriteBuffer == 0){
            VMS_currWriteBuffer = malloc(VMS_RAMSIZE);
            //UART_print("allocated blockBuffer 0x%08x\r\n", VMS_currWriteBuffer);
            memset(VMS_currWriteBuffer, 0, VMS_RAMSIZE);
            lastVMSBlock = 0;
            VMS_currWriteOffset = (void*) VMS_findFreeSpace();
            //UART_print("new Offset = 0x%08x\r\n", VMS_currWriteOffset);
        }
        
        if(!(lastVMSBlock < VMS_RAMSIZE / sizeof(VMS_BLOCK))){
            //UART_print("Blocklist overflow! attempting relink, which will probably fail\r\n");
            VMS_relinkRamList();
            VMS_writeRamList();
            lastVMSBlock = 0;
            VMS_currWriteOffset = (void*) VMS_findFreeSpace();
        }
        
        memcpy(&VMS_currWriteBuffer[lastVMSBlock], input + 1, sizeof(VMS_BLOCK));
        
        //UART_print("got Block %d! (placed at 0x%08x)\r\n", lastVMSBlock, &VMS_currWriteBuffer[lastVMSBlock]);
        
        lastVMSBlock++;
        
        output[0] = 1;
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
        
    }else if(input[0] == USB_VMS_GETMAXBLOCKCOUNT){
        uint32_t count = BLOCKMEM_SIZE / sizeof(VMS_BLOCK);
        //UART_print("Maximum blockcount = %d\r\n", count);
        output[0] = count >> 8;
        output[1] = count & 0xff;
        output[2] = sizeof(VMS_BLOCK);
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
        
    }else if(input[0] == USB_VMS_RELINK){
        //UART_print("got relink command!\r\n");
        if(VMS_currWriteBuffer != 0){
            //UART_print("Relinking and writing ramList\r\n");
            VMS_relinkRamList();
            VMS_writeRamList();
            free(VMS_currWriteBuffer);
            VMS_currWriteBuffer = 0;
        }
        
        if(HID_blocklistState == LISTSTATE_UNLINKED){
            //UART_print("Attempting full relink\r\n");
            VMS_relinkAll();
        }

        output[0] = (HID_blocklistState == LISTSTATE_NORMAL);
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
        Midi_setEnabled(1);
        
    }else if(input[0] == USB_MAP_CLEAR){
        Midi_setEnabled(0);
        if(HID_currMapHeader != 0){
            free(HID_currMapHeader);
            HID_currMapHeader = 0;
        }
        
        if(HID_currMapBuffer == 0){
            HID_currMapBuffer = malloc(MAPMEM_SIZE);
        }
        memset(HID_currMapBuffer, 0, MAPMEM_SIZE);
        Midi_setEnabled(1);
        
    }else if(input[0] == USB_MAP_STARTWRITE){
        Midi_setEnabled(0);
        if(HID_currMapHeader != 0){
            free(HID_currMapHeader);
            HID_currMapHeader = 0;
        }
        
        MAPTABLE_HEADER * receivedHeader = &input[4];
        
        HID_currMapHeader = malloc(sizeof(MAPTABLE_HEADER) + sizeof(MAPTABLE_ENTRY) * receivedHeader->listEntries);
        memset(HID_currMapHeader, 0, sizeof(MAPTABLE_HEADER) + sizeof(MAPTABLE_ENTRY) * receivedHeader->listEntries);
        
        if(HID_currMapHeader == 0) UART_print("FUUUUUCK no ram :(\r\n");
        
        memcpy(HID_currMapHeader, receivedHeader, sizeof(MAPTABLE_HEADER));
        UART_print("start map \"%.18s\"table write for program %d with %d items\r\n", HID_currMapHeader->name, HID_currMapHeader->programNumber, HID_currMapHeader->listEntries);
        
        output[0] = 1;
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
        
    }else if(input[0] == USB_MAP_WRITEENTRY){
        output[0] = 0;
        if(HID_currMapHeader != 0){
            MAPTABLE_ENTRY * receivedEntry = &input[4];

            if(HID_currMapHeader->listEntries > input[1]){
                MAPTABLE_ENTRY * currTarget = (MAPTABLE_ENTRY *) ((uint32_t) HID_currMapHeader + sizeof(MAPTABLE_HEADER) + sizeof(MAPTABLE_ENTRY) * input[1]);
                memcpy(currTarget, receivedEntry, sizeof(MAPTABLE_ENTRY));
                //UART_print("   written entry %d (start = %d, end = %d, sb = %d, freq = %d, flags = 0x%02x, ot = %d)\r\n", input[1], currTarget->startNote, currTarget->endNote, currTarget->data.VMS_Startblock, currTarget->data.noteFreq, currTarget->data.flags, currTarget->data.targetOT);
            }else{
                UART_print("   NO. %d > %d\r\n", input[1], HID_currMapHeader->listEntries);
            }
            output[0] = 1;
        }
        
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
        
    }else if(input[0] == USB_MAP_ENDENTRY){        
        output[0] = 0;
        if(HID_currMapHeader != 0){
            //TODO move this to MAPPER_finishWrite(MAPTABLE_HEADER * listStart)
            VMS_relinkMaptable(HID_currMapHeader);
            
            uint32_t requiredSize = sizeof(MAPTABLE_HEADER) + sizeof(MAPTABLE_ENTRY) * HID_currMapHeader->listEntries;
            //UART_print("searching for %d free bytes in the maplist\r\n", requiredSize);
            MAPTABLE_HEADER * targetDest = MAPPER_findFreeSpace(requiredSize);
            
            if(targetDest != 0 && HID_currMapBuffer != 0){
               //UART_print("copying maptable\r\n");
               memcpy(targetDest, HID_currMapHeader, requiredSize);
            }else{
               UART_print("what even happened. This sucks\r\n"); 
            }
            
            free(HID_currMapHeader);
            HID_currMapHeader = 0;            
            output[0] = 1;
        }
        
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
        
    }else if(input[0] == USB_MAP_ENDALL){        
        output[0] = 0;
        if(HID_currMapBuffer != 0){
            //UART_print("writing maptable to flash\r\n"); 
            NVM_memclr4096(NVM_mapMem, MAPMEM_SIZE);
            NVM_memcpy128(NVM_mapMem, HID_currMapBuffer, MAPMEM_SIZE);      
            free(HID_currMapBuffer);
            HID_currMapBuffer = 0;  
            output[0] = 1;
            MAPPER_handleMapWrite();
        }
        Midi_setEnabled(1);
        
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
    }else if(input[0] == USB_VMS_READ_BLOCK){  
        output[0] = USB_VMS_READ_BLOCK;
        if(!HID_erasePending){
            uint16_t currBlock = input[1] | (input[2] << 8) | (input[3] << 16) | (input[4] << 24);

            if(currBlock < BLOCKMEM_SIZE / sizeof(VMS_BLOCK)){
                VMS_BLOCK * buff = malloc(sizeof(VMS_BLOCK));
                VMS_BLOCK * cb = &(((VMS_BLOCK*) NVM_blockMem)[currBlock]);//(VMS_BLOCK *) (NVM_blockMem + currBlock * sizeof(VMS_BLOCK));
                //UART_print("Read valid block[%d]. UID is 0x%08x\r\n", currBlock, cb->uid);
                memcpy(buff, cb, sizeof(VMS_BLOCK));
                VMS_unlink(buff);
                memcpy(&output[1], buff, 63);
                free(buff);
            }else{
                UART_sendString("Read invalid block\r\n", 1);
                memset(&output[1], 0xff, 63);
            }
        }else{
            UART_sendString("still erasing!", 1);
            memset(&output[1], 0xff, 63);
        }
        
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
        
    }else if(input[0] == USB_MAP_READ_HEADER){ 
        output[0] = USB_MAP_READ_HEADER; 
        if(!HID_erasePending){
            uint16_t currHeader = input[1];

            if(currHeader < 127){
                MAPTABLE_HEADER * head = MAPPER_getHeader(currHeader);
                if(head == 0){
                    //UART_sendString("Read invalid map header", 1);
                    memset(&output[1], 0xff, 63);
                }else{
                    //UART_sendString("Read valid map header", 1);
                    memcpy(&output[1], head, sizeof(MAPTABLE_HEADER));
                    //UART_sendString("done", 1);
                }
            }else{
                //UART_sendString("Read invalid map header", 1);
                memset(&output[1], 0xff, 63);
            }
        }else{
            //UART_sendString("Read invalid map header", 1);
            memset(&output[1], 0xff, 63);
        }
        
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
    }else if(input[0] == USB_MAP_READ_ENTRY){  
        uint16_t currEntry = input[1];
        uint16_t currHeader = input[2];
        output[0] = USB_MAP_READ_ENTRY;
        
        if(currHeader < 127 && currEntry < 127){
            //UART_print("Read valid map entry %d from header %d\r\n", currEntry, currHeader); 
            MAPTABLE_ENTRY * entry = MAPPER_getEntry(currHeader, currEntry);
            if(entry != 0){
                //UART_print("Read valid map entry: 0x%08x, startblock = 0x%08x\r\n", entry, entry->data.VMS_Startblock);
                MAPTABLE_ENTRY * buff = malloc(sizeof(MAPTABLE_ENTRY));
                memcpy(buff, entry, sizeof(MAPTABLE_ENTRY));
                //UART_sendString("start unlink", 1);
                VMS_unlinkMapEntry(buff);
                //UART_sendString("end unlink", 1);
                memcpy(&output[1], buff, sizeof(MAPTABLE_ENTRY));
                free(buff);
            }else{
                //UART_sendString("Read invalid map entry", 1);
                memset(&output[1], 0xff, 63);
            }
        }else{
            UART_sendString("Read invalid map entry", 1);
            memset(&output[1], 0xff, 63);
        }
        
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
    }
}

void VMS_relinkMaptable(MAPTABLE_HEADER * listStart){
    uint8_t currEntry = 0;
    MAPTABLE_ENTRY * entries = (MAPTABLE_ENTRY *) ((uint32_t) listStart + sizeof(MAPTABLE_HEADER));
    for(; currEntry < listStart->listEntries; currEntry++){
        if(entries[currEntry].data.VMS_Startblock != 0){
            uint32_t lastUID = entries[currEntry].data.VMS_Startblock;
            entries[currEntry].data.VMS_Startblock = VMS_find(entries[currEntry].data.VMS_Startblock);
            //UART_print("      relinked startblock of %d to %d=0x%08x\r\n", currEntry, lastUID, entries[currEntry].data.VMS_Startblock);
        }
    }
}

void VMS_writeRamList(){
    if(VMS_currWriteBuffer == 0) return;
    if(VMS_currWriteOffset == 0){
        UART_sendString("Error: no write offset given!", 1);
        VMS_currWriteOffset = (void*) VMS_findFreeSpace();
    }
    
    NVM_memcpy4(VMS_currWriteOffset, VMS_currWriteBuffer, lastVMSBlock * sizeof(VMS_BLOCK));
    
    /*uint32_t currPageStart = ((uint32_t) VMS_currWriteOffset / PAGE_SIZE) * PAGE_SIZE;
    if(currPageStart != VMS_currWriteOffset){
        
        UART_sendString("We need to append!", 1);
        uint32_t offset = ((uint32_t) VMS_currWriteOffset - currPageStart);
        uint8_t * buff = malloc(VMS_RAMSIZE + PAGE_SIZE);
        memset(buff + VMS_RAMSIZE, 0xff, PAGE_SIZE);
        memcpy(buff, currPageStart, offset);
        memcpy((void *) ((uint32_t) buff + (uint32_t) offset), VMS_currWriteBuffer, VMS_RAMSIZE);
        
        NVM_memclr4096((void*) currPageStart, VMS_RAMSIZE + PAGE_SIZE);
        NVM_memcpy128((void*) currPageStart, buff, VMS_RAMSIZE + PAGE_SIZE);
        
        free(buff);
    }else{
        NVM_memclr4096((void*) VMS_currWriteOffset, VMS_RAMSIZE);
    }
    */
}

void VMS_relinkRamList(){
    if(VMS_currWriteBuffer == 0) return;
    unsigned success = 1;
    
    uint32_t lb = 0;

    for(; lb < 0xffff; lb++){
        if(VMS_currWriteBuffer[lb].uid == 0) break;
        //UART_print("Relinking %d\r\n", lb);
        VMS_currWriteBuffer[lb].offBlock = VMS_find(VMS_currWriteBuffer[lb].offBlock);

        uint8_t i = 0;
        for(i = 0; i < VMS_MAX_BRANCHES; i ++){
            if(VMS_currWriteBuffer[lb].nextBlocks[i] != 0 && VMS_currWriteBuffer[lb].nextBlocks[i] < 0xffff){
                if(VMS_currWriteBuffer[lb].nextBlocks[i] == VMS_DIE){ 
                    break;
                }
                VMS_BLOCK * newBlock = VMS_find(VMS_currWriteBuffer[lb].nextBlocks[i]);
                VMS_currWriteBuffer[lb].nextBlocks[i] = newBlock;
                success &= (newBlock != 0);
            }
        }
    }
    
    //if there was a block in the list, which pointed to a block that we didn't find, we'll have to set the list state to unlinked
    if(!success){
        HID_blocklistState = LISTSTATE_UNLINKED;
    }
}

void VMS_relinkAll(){
    unsigned success = 1;
    VMS_BLOCK * buff = malloc(PAGE_SIZE);
    memset(buff, 0, PAGE_SIZE);
    
    void * currPageOffset = 0;
    while(currPageOffset < BLOCKMEM_SIZE){
        memcpy(buff, (void*) ((uint32_t) NVM_blockMem + (uint32_t) currPageOffset), PAGE_SIZE);
        uint32_t lb = 0;
        if(buff[0].uid == 0xffffffff) break;
        
        for(; lb < PAGE_SIZE / sizeof(VMS_BLOCK); lb++){
            if(buff[lb].uid == 0xffffffff) break;
            buff[lb].offBlock = VMS_find(buff[lb].offBlock);
            
            uint8_t i = 0;
            for(i = 0; i < VMS_MAX_BRANCHES; i ++){
                if(buff[lb].nextBlocks[i] != 0 && buff[lb].nextBlocks[i] < 0xffff){
                    if(buff[lb].nextBlocks[i] == VMS_DIE){ 
                        break;
                    }
                    VMS_BLOCK * newBlock = VMS_find(buff[lb].nextBlocks[i]);
                    buff[lb].nextBlocks[i] = newBlock;
                    success &= (newBlock != 0);
                }
            }
        }
        
        NVM_erasePage((void*) ((uint32_t) NVM_blockMem + (uint32_t) currPageOffset));
        NVM_memcpy128((void*) ((uint32_t) NVM_blockMem + (uint32_t) currPageOffset), buff, PAGE_SIZE);
        
        currPageOffset += PAGE_SIZE;
    }
    
    //if there was a block in the list, which pointed to a block that we didn't find, we'll have to set the list state to unlinked
    if(success){
        HID_blocklistState = LISTSTATE_NORMAL;
    }else{
        UART_print("FUUUUUCK relinking failed!\r\n");
    }
}

void VMS_unlink(VMS_BLOCK * block){
    if(block < 0xa0000000 || block > 0xa0010000) return; //can't write into anything but ram
    if(block->offBlock == 0xffffffff) return;
    if(block->offBlock > 0x1000 && block->offBlock != VMS_DIE) block->offBlock = block->offBlock->uid;
    uint8_t i;
    for(i = 0; i < VMS_MAX_BRANCHES; i ++){
        //UART_print("unlink 0x%08x\r\n", block->nextBlocks[i]);
        if(block->nextBlocks[i] > 0x1000 && block->nextBlocks[i] != VMS_DIE){
            block->nextBlocks[i] = block->nextBlocks[i]->uid;
        }
    }
    //UART_print("done!\r\n");
}

void VMS_unlinkMapEntry(MAPTABLE_ENTRY * entry){
    if(entry < 0xa0000000 || entry > 0xa0010000) return; //can't write into anything but ram
    //UART_print("start = 0x%08x -> ", entry->data.VMS_Startblock);
    if(entry->data.VMS_Startblock > 0x1000 && entry->data.VMS_Startblock != VMS_DIE) entry->data.VMS_Startblock = entry->data.VMS_Startblock->uid;
    //UART_print("0x%08x\r\n", entry->data.VMS_Startblock);
}

VMS_BLOCK * VMS_find(uint32_t uid){
    if(uid == 0) return 0;
    uint32_t lb = 0;
    
    if(VMS_currWriteBuffer != 0){
        //UART_print("checking ram\r\n");
        for(; lb < VMS_RAMSIZE / sizeof(VMS_BLOCK); lb++){
            if(VMS_currWriteBuffer[lb].uid == 0 || VMS_currWriteBuffer[lb].uid == 0xffffffff) break;
            //since the block list is still in ram at this point we have to calculate the pointer it will have once written into flash
            if(VMS_currWriteBuffer[lb].uid == uid){
                VMS_BLOCK * ret = (VMS_BLOCK *) ((uint32_t) VMS_currWriteOffset + sizeof(VMS_BLOCK) * lb);
                //UART_print("found %d at 0x%08x\r\n", uid, ret);
                return ret;
            }
            
        }
    }
    
    //UART_print("block not found in ram, checking flash\r\n");
    
    lb = 0;
    VMS_BLOCK * flashBlocks = (VMS_BLOCK *) NVM_blockMem;
    for(; lb < BLOCKMEM_SIZE / sizeof(VMS_BLOCK); lb++){
        if(flashBlocks[lb].uid == 0xffffffff) break;
        if(flashBlocks[lb].uid == uid){
            //UART_print("found %d at 0x%08x\r\n", uid, &flashBlocks[lb]);
            return &flashBlocks[lb];
        }
    }
    
    return 0;
}

MAPTABLE_HEADER * MAPPER_findFreeSpace(uint32_t size){
    if(HID_currMapBuffer == 0) return 0;
    MAPTABLE_HEADER * currHeader = (MAPTABLE_HEADER *) HID_currMapBuffer;
    while(1){
        //if the current header is empty (aka all 0) we return it
        //UART_print("checking 0x%08x: count = %d, num = %d\r\n", currHeader, currHeader->listEntries, currHeader->programNumber);
        if(currHeader->listEntries == 0 && currHeader->programNumber == 0) return currHeader;
        currHeader += sizeof(MAPTABLE_HEADER) + sizeof(MAPTABLE_ENTRY) * currHeader->listEntries;
        if(((uint32_t) currHeader - (uint32_t) HID_currMapBuffer) >= (MAPMEM_SIZE - size)) break;
    }
    return 0;
}

VMS_BLOCK * VMS_findFreeSpace(){
    VMS_BLOCK * currBlock = (VMS_BLOCK *) &NVM_blockMem;
    uint32_t c = 0;
    while(1){
        if(currBlock[c].uid == 0xffffffff) return &currBlock[c];
        c++; 
        if(((uint32_t) &currBlock[c] - (uint32_t) &NVM_blockMem) >= (BLOCKMEM_SIZE)) break;
    }
    return 0;
}