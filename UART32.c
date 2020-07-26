/*
        UART Driver for pic32
        Copyright Sep. 2018 TMAX-Electronics.de
 */

#include <stdio.h>
#include <xc.h>

void UART_init(int baudRate1, int baudRate2){
    
    U1MODE = 0b1000000000000000;                //UART Module ON, U1RX and U1TX only, Autobaud off, 8bit Data no parity, High Speed mode off
    U1MODEbits.BRGH = 0;
    //U1MODEbits.RXINV = 0;
    U1STA = 0b0101010000000000;                 //Tx interrupt when all is transmitted, Rx & Tx enabled, Rx interrupt when buffer is full
    U1BRG = ( 48000000 / ( 16 * baudRate1 ) ) - 1;//11;//
    //U1RXR = 0b0100;                             //Set U1Rx to be on pin 6
    RPB7R = 0b0001;                             //Set U1Tx to be on pin 2
    //INTCONbits.MVEC = 1;
    
    //IEC1bits.U1RXIE = 1;
    //IPC8bits.U1IP = 7;
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
    
    sprintf(buffer, "%u" ,data);
    
    int currPos = 0;
    int length = 10;
    
    for(;currPos < length; currPos ++){
        UART_sendChar(buffer[currPos]);
    }
    
    if(newLine) UART_sendChar('\n');
    if(newLine) UART_sendChar('\r');
}

void UART_sendHex(unsigned char data, unsigned newLine){
    
    char buffer[5] = {"     "};;
    
    sprintf(buffer, "%x" ,data);
    
    int currPos = 0;
    int length = 5;
    
    for(;currPos < length; currPos ++){
        UART_sendChar(buffer[currPos]);
    }
    
    if(newLine) UART_sendChar('\n');
    if(newLine) UART_sendChar('\r');
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
    U1TXREG = data;
    while(U1STAbits.UTXBF);
}

void clearU2Oerr(){
    char trash2;
    if(!U1STAbits.OERR){
        U1STAbits.URXEN = 0;
        while(U1STAbits.URXDA) trash2 = U2RXREG;
        U1STAbits.URXEN = 1;
    }
}