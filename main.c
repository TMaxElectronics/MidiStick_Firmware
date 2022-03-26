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

// USERID = No Setting
#pragma config PMDL1WAY = OFF           // Peripheral Module Disable Configuration (Allow only one reconfiguration)
#pragma config IOL1WAY = OFF            // Peripheral Pin Select Configuration (Allow only one reconfiguration)
#pragma config FUSBIDIO = OFF           // USB USID Selection (Controlled by Port Function)
#pragma config FVBUSONIO = OFF          // USB VBUS ON Selection (Controlled by Port Function)

// DEVCFG2
#pragma config FPLLIDIV = DIV_4         // PLL Input Divider (4x Divider)
#pragma config FPLLMUL = MUL_24         // PLL Multiplier (24x Multiplier)
#pragma config UPLLIDIV = DIV_4         // USB PLL Input Divider (4x Divider)
#pragma config UPLLEN = ON              // USB PLL Enable (Enabled)
#pragma config FPLLODIV = DIV_2         // System PLL Output Clock Divider (PLL Divide by 2)
#pragma config FNOSC = PRIPLL           // Oscillator Selection Bits (Primary Osc w/PLL (XT+,HS+,EC+PLL))

// DEVCFG1
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disabled)
#pragma config IESO = OFF               // Internal/External Switch Over (Disabled)
#pragma config POSCMOD = HS             // Primary Oscillator Configuration (HS osc mode)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FPBDIV = DIV_1           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/1)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Clock Switch Disable, FSCM Disabled)
#pragma config WDTPS = PS1048576        // Watchdog Timer Postscaler (1:1048576)
#pragma config WINDIS = OFF             // Watchdog Timer Window Enable (Watchdog Timer is in Non-Window Mode)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (WDT Disabled (SWDTEN Bit Controls))
#pragma config FWDTWINSZ = WINSZ_25     // Watchdog Timer Window Size (Window Size is 25%)

// DEVCFG0
#pragma config JTAGEN = OFF             // JTAG Enable (JTAG Disabled)
#pragma config ICESEL = ICS_PGx1        // ICE/ICD Comm Channel Select (Communicate on PGEC1/PGED1)
#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF                 // Code Protect (Protection Disabled)


#include <xc.h>
#include "usb lib/usb.h"
#include "usb lib/usb_device.h"
#include "usb lib/usb_functions.h"
#include <sys/attribs.h>
#define _SUPPRESS_PLIB_WARNING
#include <plib.h>
#include "MidiController.h"
#include "UART32.h"
#include "ConfigManager.h"
#include "DLL.h"
#include "ADSREngine.h"
#include "HIDController.h"

void initIO();
void initUSB();

/*
 * Note:
 * 
 * The USB library comes curtesy of microchip, BUT i had to modify it a little bit to support the pic I have.
 * The software is roughly based on the device_audio_midi example from microchip, though all of the midi interpreting code was written by myself, aswell as the configuration code.
 * 
 * You are free to use this code in you projects, modified or not, but I would be happy if you could give credit to me (TMax-Electronics.de) if you do.
 * This code comes with no warranty, it is the implementers responsibility to verify correct function and he is liable for any damage caused to equipment.
 * 
 */

void main(void) {
    //set flash timings to the optimum
    SYSTEMConfigPerformance(48000000);
    
    //init the devise
    initIO();
    initUSB();
    
    //initialize debug UART - this is not needed for normal operation, but does not affect preformance
    UART_init(12000000, 0);
    
    //print device information (fw version, build date&time, bootloader version and serial number)
    UART_sendString("\r\n\n\nMidiStick ", 0); UART_sendString(NVM_getFWVersionString(), 0); UART_sendString(" - from ", 0); UART_sendString(NVM_getFWCompileDate(), 0); UART_sendString(" ", 0); UART_sendString(NVM_getFWCompileTime(), 1);  
    char * blVer = NVM_getBootloaderVersionString();
    UART_sendString("\r\nBootloader V", 0); UART_sendString(blVer, 1);
    UART_sendString("Serial number: ", 0); UART_sendInt(NVM_getSerialNumber(), 1);
    free(blVer);
    
    /* This is a workaround for a glitch in the V1.1 bootloader that caused the opto-transmitter to turn on for one second during initialisation.
     * How does this dirty fix work? 
     * Usually flash needs to be erased prior to changing anything in it. This is because of how it is written, as a write operation can only pull bits to 0, not set them to 1.
     * But in our case all we need is to clear the bit that turns on the transmitter at boot, so we don't technically have to erase the page before writing.
     * 
     * To make sure that current value is exactly what what we expect it to be, we check if *instr == 0x34088380, which is the opcode and data for 
     * ori		$t0, $zero, 0b1000001110000000
     *                        ^  This is the bit in question
     * If we find this in the code, we write 0x34080380 which is
     * ori		$t0, $zero, 0b0000001110000000
     * So even though this is a dirty fix, it is one that should be ok to do.
     */
     
    volatile uint32_t *instr = 0x9fc004b0;
    
    if(*instr != 0x34080380){ UART_sendString("Instruction = ", 0); UART_sendHex(*instr, 0); UART_sendString(" Should be = ", 0); UART_sendHex(0x34080380, 1); }
    if(*instr == 0x34088380){
        UART_sendString("Found V1.1 bootloader with existing output glitch! Patching...", 1);
        
        NVM_writeWord(instr, 0x34080380);

        UART_sendString("Instruction is now = ", 0); UART_sendHex(*instr, 0);
        if(*instr != 0x34080380){
            UART_sendString("Write error! Pray to god that the write did not kill the BL", 1);
        }
    }
    
    
    
    //enable interrupts. the asm stuff is required to set the CP0 register to enable them
    INTCONbits.MVEC = 1;
    unsigned int val;
    asm volatile("mfc0   %0,$12" : "=r"(val));
    val |= 0b1;
    asm volatile("mtc0   %0,$12" : "+r"(val));
    
    //Check for remenants of a finished update and remove them
    NVM_finishFWUpdate();
    
    UART_sendString("Stick is ready!", 1);
    
    Midi_LED(LED_POWER, LED_ON);
    
    uint32_t lastRun = 0;
    while(1){   //run the required tasks
        APP_DeviceAudioMicrophoneTasks();
        USBDeviceTasks();
        Midi_run();
        VMS_run();
        
        if(HID_erasePending){
            NVM_erasePage(HID_currErasePage);
            HID_currErasePage += PAGE_SIZE;
            if(HID_currErasePage > NVM_blockMem + BLOCKMEM_SIZE){
                HID_erasePending = 0;
                Midi_setEnabled(1);
            }
        }
        
        /*if(_CP0_GET_COUNT() - lastRun > 240000){
            lastRun = _CP0_GET_COUNT();
            UART_print("T1CON: ON = %d, PR1 = %d TMR1 = %d\r\n", T1CONbits.ON, PR1, TMR1);
        }*/
    }
}

void initIO(){
    TRISA = 0;
    LATA = 0;
    TRISBCLR = _LATB_LATB5_MASK | _LATB_LATB7_MASK | _LATB_LATB8_MASK | _LATB_LATB9_MASK | _LATB_LATB15_MASK;
    //The LEDs are common anode and connected to the +5V rail, so enable the open drain inputs
    ODCBSET = _LATB_LATB7_MASK | _LATB_LATB8_MASK | _LATB_LATB9_MASK;
    
    LATBSET = _LATB_LATB7_MASK | _LATB_LATB8_MASK | _LATB_LATB9_MASK;
    LATBCLR = _LATB_LATB5_MASK  | _LATB_LATB15_MASK;
}

void initUSB() {
    SYSKEY = 0;// force lock
    SYSKEY = 0xAA996655;  // unlock sequence
    SYSKEY = 0x556699AA;  // lock sequence
    
    OSCTUN = 0x00009000;    //active clock tuning enabled, source = USB
    
    SYSKEY = 0;             //Re-lock oscillator registers  

    USBDeviceInit();
    USBDeviceAttach();
}


void _general_exception_handler ( void ){
    INTCONbits.MVEC = 0;
    LATBCLR = _LATB_LATB15_MASK | _LATB_LATB5_MASK;  
    
    while(1){
        LATBINV = _LATB_LATB9_MASK; //blink red led
        uint32_t a = 0;
        for(a = 0; a < 10000000; a ++);
    }
}