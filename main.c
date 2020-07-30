
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
    
    //initialize debug UART - this is not needed for normal operation, but does not affect preformance much
    UART_init(115200, 0);
    
    //print device information (fw version, build date&time, bootloader version and serial number)
    UART_sendString("\r\n\n\nMidiStick ", 0); UART_sendString(NVM_getFWVersionString(), 0); UART_sendString(" - from ", 0); UART_sendString(NVM_getFWCompileDate(), 0); UART_sendString(" ", 0); UART_sendString(NVM_getFWCompileTime(), 1);  
    char * blVer = NVM_getBootloaderVersionString();
    UART_sendString("\r\nBootloader V", 0); UART_sendString(blVer, 1);
    UART_sendString("Serial number: ", 0); UART_sendInt(NVM_getSerialNumber(), 1);
    free(blVer);
    
    //enable interrupts. the asm stuff is required to set the CP0 register to enable them
    INTCONbits.MVEC = 1;
    unsigned int val;
    asm volatile("mfc0   %0,$12" : "=r"(val));
    val |= 0b1;
    asm volatile("mtc0   %0,$12" : "+r"(val));
    
    //Check for remenants of a finished update and remove them
    NVM_finishFWUpdate();
    
    while(1){   //run the required tasks
        USBDeviceTasks();
        Midi_run();
    }
}

void initIO(){
    TRISA = 0;
    LATA = 0;
    TRISBCLR = _LATB_LATB5_MASK | _LATB_LATB7_MASK | _LATB_LATB8_MASK | _LATB_LATB9_MASK | _LATB_LATB15_MASK;
    
    //The LEDs are common anode and connected to the +5V rail, so enable the open drain inputs
    ODCBSET = _LATB_LATB7_MASK | _LATB_LATB8_MASK | _LATB_LATB9_MASK;
    
    LATBSET = _LATB_LATB7_MASK | _LATB_LATB9_MASK;
    LATBCLR = _LATB_LATB5_MASK | _LATB_LATB8_MASK | _LATB_LATB15_MASK;
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


void _general_exception_handler ( void ){            //FROM MICROCHIP HARMONY!{
    while(1);
}