/*
 * File:   qTouchSPI.c
 * Author: Thorb
 *
 * Created on February 18, 2019, 3:42 PM
 */


#include <xc.h>
#include <stdint.h>
#include "qTouchSPI.h"

void initSPI(uint32_t freq){
    SDI2R = 0b0011; //MISO on RB13
    RPB8R = 0b0100;
    SPI2CONbits.ENHBUF = 0;
    SPI2CONbits.MODE16 = 0;
    SPI2CONbits.MODE32 = 0;
    SPI2CONbits.SMP = 0;
    SPI2CONbits.CKE = 0;
    SPI2CONbits.CKP = 1;
    SPI2CONbits.MSTEN = 1;
    SPI2CONbits.ON = 1;
    SPI2BRG = ((48000000/(2*freq)) - 1);
    LATBSET = 1<<14;
    
    sendData(0b00011000);
    uint8_t trash = SPI2BUF;
    sendData(0b00110011);
    trash = SPI2BUF;
    sendData(0b11100000);
    trash = SPI2BUF;
    
    calQTouch();
}

void calQTouch(){
    LATBCLR = 1<<14;
    
    LATBCLR = 1<<14;
    sendData(0b00001010);
    char trash = SPI2BUF;
    //if(SPI2BUF != 0) LATBbits.LATB3 = 1;
    
    sendData(0b00110001);
    trash = SPI2BUF;
    //if(UButton.integer == 0xFF) LATBINV = 0b1000;
    
    sendData(0b11001000);
    trash = SPI2BUF;
    //if(SPI2BUF != 0) LATBbits.LATB3 = 1;
    LATBSET = 1<<14;
}

void updateQTouch(){
    while(!PORTBbits.RB9);
    LATBCLR = 1<<14;
    
    wait(100);
    
    sendData(0b00001010);
    URE0.integer = SPI2BUF;
    //if(SPI2BUF != 0) LATBbits.LATB3 = 1;
    
    sendData(0b00110001);
    UButton.integer = SPI2BUF;
    //if(UButton.integer == 0xFF) LATBINV = 0b1000;
    
    sendData(0b11000000);
    sliderPos = SPI2BUF;
    //if(SPI2BUF != 0) LATBbits.LATB3 = 1;
    LATBSET = 1<<14;
}

void debugQTouch(){
    while(!PORTBbits.RB9);
    LATBCLR = 1<<14;
    
    LATBCLR = 1<<14;
    sendData(0b00100001);
    char trash = SPI2BUF;
    UButton.buttons.CTRL = (trash == 0x08) ? 1 : 0;
    //if(SPI2BUF != 0) LATBbits.LATB3 = 1;
    
    sendData(2);
    trash = SPI2BUF;
    //if(UButton.integer == 0xFF) LATBINV = 0b1000;
    
    sendData(0);
    trash = SPI2BUF;
    //if(SPI2BUF != 0) LATBbits.LATB3 = 1;
    LATBSET = 1<<14;
}

Buttons getButtons(){
    return UButton.buttons;
}

uint8_t getSliderPos(){
    return sliderPos;
}

void sendData(uint8_t data){
    SPI2BUF = data;
    while(SPI2STATbits.SPIRBE);
}