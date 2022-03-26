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

#include <stdio.h>
#include <xc.h>
#include <stdarg.h>

#include "UART32.h"

//send data to the debug min ID
void UART_print(char * format, ...){
    va_list arg;
    va_start (arg, format);
    
    uint8_t * buff = (uint8_t*) malloc(256);
    uint32_t length = vsprintf(buff, format, arg);
    
    UART_sendString(buff, 0);
    
    free(buff);
    
    va_end (arg);
}

void UART_init(int baudRate1, int baudRate2){
    U2MODE = 0b1000000000000000;                //UART Module ON, U1RX and U1TX only, Autobaud off, 8bit Data no parity, High Speed mode off
    U2STA = 0b0101010000000000;                 //Tx interrupt when all is transmitted, Rx & Tx enabled, Rx interrupt when buffer is full
    U2MODEbits.BRGH = 1;
    U2BRG = ( 48000000 / ( 4 * baudRate1 ) ) - 1;
    //U1RXR = 0b0100;                             //Set U1Rx to be on pin 6
    RPB0R = 0b0010;                             //Set U1Tx to be on pin 2
}

void UART_sendString(char *data, unsigned newLine){
    while((*data) != 0) UART_sendChar(*data++);
    if(newLine){
        UART_sendChar('\n');
        UART_sendChar('\r');
    }
}

void UART_sendInt(unsigned long data, unsigned newLine){
    
    char buffer[10] = {"          "};;
    
    sprintf(buffer, "%d\0" ,data);
    
    UART_sendString(buffer, newLine);
}

void UART_sendHex(uint32_t data, unsigned newLine){
    
    char * buffer = malloc(128);
    if(buffer == 0) UART_sendString("fuck off i don't have enough heap anymore...", 1);
    
    sprintf(buffer, "%x\0" ,data);
    
    UART_sendString("0x", 0); UART_sendString(buffer, newLine);
    
    free(buffer);
    
}

void UART_sendBin(unsigned long long data, char length, unsigned newLine){
    
    char out[length];
    char currBit = length-1;
    for(;currBit >= 0; currBit--){
        if(((data >> currBit) & 1)){
            out[length-1-currBit] = '1';
        }else{
            out[length-1-currBit] = '0';
        }
    }
    
    UART_sendChar('0');
    UART_sendChar('b');
    int currPos = 0;
    for(;currPos < length; currPos ++){
        UART_sendChar(out[currPos]);     
    }
    if(newLine) UART_sendChar('\n');
    if(newLine) UART_sendChar('\r');
    
}

void UART_sendChar(char data){
    while(U2STAbits.UTXBF);
    U2TXREG = data;
}

void UART_clearOERR(){
    U2STAbits.OERR = 0;
}