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

uint32_t lastVMSBlock = 0;
enum {LISTSTATE_NORMAL, LISTSTATE_UNLINKED} HID_blocklistState;

MAPTABLE_HEADER * HID_currMapHeader = 0;

VMS_BLOCK * VMS_find(uint32_t uid);
MAPTABLE_HEADER * MAPPER_findFreeSpace(uint32_t size);
void VMS_relinkMaptable(MAPTABLE_HEADER * listStart);

void HID_parseCMD(uint8_t * input, uint8_t * output, USB_HANDLE handle, uint8_t dataSize) {
    if(input[0] == USB_CMD_SUPPORT_EXTINSTR){
        output[0] = 1;
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
    
    }else if(input[0] == USB_CMD_VMS_CLEARBLOCKS){
        memset(VMSBLOCKS, 0, VMS_PREVIEW_MEMSIZE);
        lastVMSBlock = 0;
        
    }else if(input[0] == USB_CMD_VMS_WRITEBLOCK){
        VMS_BLOCK * blocks = (VMS_BLOCK *) VMSBLOCKS;
        for(; lastVMSBlock < 0xffff; lastVMSBlock++){
            if(blocks[lastVMSBlock].uid == 0) break;
        }
        if(lastVMSBlock != 0xffff){
            memcpy(&blocks[lastVMSBlock], input + 1, sizeof(VMS_BLOCK));
        }
        
        UART_print("got new Block! (placed at 0x%08x)\r\n", &blocks[lastVMSBlock]);
        UART_print("uid = %d\r\n", blocks[lastVMSBlock].uid);
        UART_print("next[0] = %d\r\n", blocks[lastVMSBlock].nextBlocks[0]);
        UART_print("offBlock = %d\r\n", blocks[lastVMSBlock].offBlock);
        UART_print("target = %d\r\n", blocks[lastVMSBlock].target);
        UART_print("period = %d\r\n", blocks[lastVMSBlock].period);
        UART_print("param1 = %d\r\n", blocks[lastVMSBlock].param1);
        UART_print("mode = %d\r\n", blocks[lastVMSBlock].behavior);
        
        lastVMSBlock++;
        
        output[0] = 1;
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
        
    }else if(input[0] == USB_VMS_GETMAXBLOCKCOUNT){
        uint32_t count = VMS_PREVIEW_MEMSIZE / sizeof(VMS_BLOCK);
        output[0] = count >> 8;
        output[1] = count & 0xff;
        output[2] = sizeof(VMS_BLOCK);
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
        
    }else if(input[0] == USB_VMS_UNLINK){
        HID_blocklistState = LISTSTATE_UNLINKED;
        VMS_BLOCK * blocks = (VMS_BLOCK *) VMSBLOCKS;
        uint32_t lb = 0;
        for(; lb < 0xffff; lb++){
            if(blocks[lb].uid == 0) break;
            uint8_t i = 0;
            for(i = 0; i < VMS_MAX_BRANCHES; i ++){
                if(blocks[lb].nextBlocks[i] != 0){
                    VMS_BLOCK * nb = blocks[lb].nextBlocks[i];
                    blocks[lb].nextBlocks[i] = nb->uid;
                }
            }
            //TODO offblocks
        }
        
    }else if(input[0] == USB_VMS_RELINK){
        VMS_BLOCK * blocks = (VMS_BLOCK *) VMSBLOCKS;
        uint32_t lb = 0;
        UART_print("start relink:\r\n");
        
        for(; lb < 0xffff; lb++){
            if(blocks[lb].uid == 0) break;
            uint8_t i = 0;
            uint32_t lastUID = blocks[lb].offBlock;
            blocks[lb].offBlock = VMS_find(blocks[lb].offBlock);
            UART_print("      relinked offblock of 0x%08x to %d=0x%08x\r\n", &blocks[lb], lastUID, blocks[lb].offBlock);
            
            for(i = 0; i < VMS_MAX_BRANCHES; i ++){
                if(blocks[lb].nextBlocks[i] != 0){
                    if(blocks[lb].nextBlocks[i] == VMS_DIE){ 
                        UART_print("VMS is kill. NO\r\n");
                        break;
                    }
                    lastUID = blocks[lb].nextBlocks[i];
                    blocks[lb].nextBlocks[i] = VMS_find(blocks[lb].nextBlocks[i]);
                    UART_print("      relinked next[%d] of 0x%08x to %d=0x%08x\r\n", i, &blocks[lb], lastUID, blocks[lb].nextBlocks[i]);
                }
            }
        }
        UART_print("---- done!\r\n");
        
        HID_blocklistState = LISTSTATE_NORMAL;
        
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
        
    }else if(input[0] == USB_MAP_CLEAR){
        if(HID_currMapHeader != 0){
            free(HID_currMapHeader);
            HID_currMapHeader = 0;
        }
        memset(MAPTABLE, 0, MAPPER_FLASHSIZE);
        
    }else if(input[0] == USB_MAP_STARTWRITE){
        if(HID_currMapHeader != 0){
            free(HID_currMapHeader);
            HID_currMapHeader = 0;
        }
        
        MAPTABLE_HEADER * receivedHeader = &input[4];
        
        HID_currMapHeader = malloc(sizeof(MAPTABLE_HEADER) + sizeof(MAPTABLE_ENTRY) * receivedHeader->listEntries);
        
        if(HID_currMapHeader == 0) UART_print("FUUUUUCK no ram :(\r\n");
        
        memcpy(HID_currMapHeader, receivedHeader, sizeof(MAPTABLE_HEADER));
        UART_print("start map table write for program %d with %d items\r\n", HID_currMapHeader->programNumber, HID_currMapHeader->listEntries);
        
        output[0] = 1;
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
        
    }else if(input[0] == USB_MAP_WRITEENTRY){
        output[0] = 0;
        if(HID_currMapHeader != 0){
            MAPTABLE_ENTRY * receivedEntry = &input[4];

            if(HID_currMapHeader->listEntries > input[1]){
                MAPTABLE_ENTRY * currTarget = (MAPTABLE_ENTRY *) ((uint32_t) HID_currMapHeader + sizeof(MAPTABLE_HEADER) + sizeof(MAPTABLE_ENTRY) * input[1]);
                memcpy(currTarget, receivedEntry, sizeof(MAPTABLE_ENTRY));
                UART_print("   written entry %d (start = %d, end = %d, sb = %d, freq = %d, flags = 0x%02x, ot = %d)\r\n", input[1], currTarget->startNote, currTarget->endNote, currTarget->data.VMS_Startblock, currTarget->data.noteFreq, currTarget->data.flags, currTarget->data.targetOT);
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
            UART_print("searching for %d free bytes in the maplist\r\n", requiredSize);
            MAPTABLE_HEADER * targetDest = MAPPER_findFreeSpace(requiredSize);
            
            if(targetDest != 0){
               UART_print("copying maptable\r\n");
               memcpy(targetDest, HID_currMapHeader, requiredSize);
            }else{
               UART_print("what even happened. there should be plenty of space left in the map table:(\r\n"); 
            }
            
            free(HID_currMapHeader);
            HID_currMapHeader = 0;            
            output[0] = 1;
        }
        
        handle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, output, dataSize);
    }
}

void MAPPER_finishWrite(MAPTABLE_HEADER * listStart){
    //TODO shit this into the flash
}

void VMS_relinkMaptable(MAPTABLE_HEADER * listStart){
    uint8_t currEntry = 0;
    MAPTABLE_ENTRY * entries = (MAPTABLE_ENTRY *) ((uint32_t) listStart + sizeof(MAPTABLE_HEADER));
    for(; currEntry < listStart->listEntries; currEntry++){
        if(entries[currEntry].data.VMS_Startblock != 0){
            uint32_t lastUID = entries[currEntry].data.VMS_Startblock;
            entries[currEntry].data.VMS_Startblock = VMS_find(entries[currEntry].data.VMS_Startblock);
            UART_print("      relinked startblock of %d to %d=0x%08x\r\n", currEntry, lastUID, entries[currEntry].data.VMS_Startblock);
        }
    }
}

VMS_BLOCK * VMS_find(uint32_t uid){
    VMS_BLOCK * blocks = (VMS_BLOCK *) VMSBLOCKS;
    uint32_t lb = 0;
    for(; lb < 0xffff; lb++){
        if(blocks[lb].uid == 0) break;
        if(blocks[lb].uid == uid) return &blocks[lb];
    }
    return 0;
}

MAPTABLE_HEADER * MAPPER_findFreeSpace(uint32_t size){
    MAPTABLE_HEADER * currHeader = (MAPTABLE_HEADER *) &MAPTABLE;
    while(1){
        //if the current header is empty (aka all 0) we return it
        UART_print("checking 0x%08x: count = %d, num = %d\r\n", currHeader, currHeader->listEntries, currHeader->programNumber);
        if(currHeader->listEntries == 0 && currHeader->programNumber == 0) return currHeader;
        currHeader += sizeof(MAPTABLE_HEADER) + sizeof(MAPTABLE_ENTRY) * currHeader->listEntries;
        if(((uint32_t) currHeader - (uint32_t) &MAPTABLE) >= (16384 - size)) break;
    }
    return 0;
}