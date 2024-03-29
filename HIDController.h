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
#include "usb lib/usb.h"

//USB Config commands
#define USB_CMD_PING                    0x00
#define USB_CMD_GET_PROTOCOL_VERSION    0x01
#define USB_CMD_DIE                     0x02
#define USB_CMD_LIVE_PREV               0x10
#define USB_CMD_SET_ADSR_PREV           0x11

#define USB_CMD_GET_PROGRAMM            0x20
#define USB_CMD_SET_PROGRAMM            0x21
#define USB_CMD_GET_COIL_CONFIG         0x22
#define USB_CMD_SET_COIL_CONFIG         0x23
#define USB_CMD_SET_COIL_CONFIG         0x23
#define USB_CMD_GET_COILCONFIG_INDEX    0x24
#define USB_CMD_SET_COILCONFIG_INDEX    0x25
#define USB_CMD_CLEAR_ALL_PROGRAMMS     0x26
#define USB_CMD_CLEAR_ALL_COILS         0x27
#define USB_CMD_COILCONFIG_NOT_CHANGED  0x28
#define USB_CMD_SET_STERO_CONFIG        0x29
#define USB_CMD_GET_STERO_CONFIG        0x2a
#define USB_CMD_SET_SYNTH_MODE          0x2b
#define USB_CMD_SET_TR_PARAM            0x2c
#define USB_CMD_TR_WD_RESET             0x2d
#define USB_CMD_RAW_WRITE_NOTES         0x2e
#define USB_CMD_SET_MASTERVOL           0x2f

#define USB_CMD_GET_DEVCFG              0x30
#define USB_CMD_SET_DEVCFG              0x31
#define USB_CMD_BLINK                   0x32
#define USB_CMD_SET_USBPID              0x33
#define USB_CMD_IS_SAFE                 0x34
#define USB_CMD_SET_EXP_DEVCFG          0x35
#define USB_CMD_GET_EXP_DEVCFG          0x36
#define USB_GET_CURR_NOTES              0xf0

#define USB_CMD_ENTER_PROGMODE          0x40
#define USB_CMD_EXIT_PROGMODE           0x41
#define USB_CMD_FWUPDATE_ERASE          0x42                 //erases the currently stored FW data at 0x9D020000, if there is any
#define USB_CMD_FWUPDATE_START_BULK_WRITE 0x43      //set up for bulk data
#define USB_CMD_FWUPDATE_BULK_WRITE_FINISHED 0x88      //set up for bulk data
#define USB_CMD_FWUPDATE_GET_CRC        0x44                
#define USB_CMD_FWUPDATE_COMMIT         0x45                 
#define USB_CMD_FWUPDATE_REBOOT         0x46 

#define USB_CMD_GET_SERIAL_NR           0x50
#define USB_CMD_GET_FWVERSION           0x51
#define USB_CMD_GET_BLVERSION           0x52

#define USB_CMD_GET_PROTOCOL_VERS       0x80
#define USB_CMD_VMS_WRITEBLOCK          0x81
#define USB_CMD_VMS_CLEARBLOCKS         0x82
#define USB_VMS_GETMAXBLOCKCOUNT        0x83
#define USB_VMS_UNLINK                  0x84
#define USB_VMS_RELINK                  0x85
#define USB_CMD_VMS_ERASECHECK          0x8a
#define USB_VMS_READ_BLOCK              0x8c

#define USB_MAP_CLEAR                   0x86
#define USB_MAP_STARTWRITE              0x87
#define USB_MAP_WRITEENTRY              0x88
#define USB_MAP_ENDENTRY                0x89
#define USB_MAP_ENDALL                  0x8b
#define USB_MAP_READ_HEADER             0x8d
#define USB_MAP_READ_ENTRY              0x8e
  
#define MAPMEM_SIZE   16384
#define BLOCKMEM_SIZE 98304 //24*BYTE_PAGE_SIZE

extern uint32_t HID_currErasePage;
extern unsigned HID_erasePending;

void HID_parseCMD(uint8_t * input, uint8_t * output, USB_HANDLE * handle, uint8_t dataSize);
