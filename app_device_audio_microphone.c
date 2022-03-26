/********************************************************************
 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the "Company") for its PIC(R) Microcontroller is intended and
 supplied to you, the Company's customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *******************************************************************/

/** INCLUDES *******************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <sys/attribs.h>

#include "usb.h"
#include "usb_device_audio.h"
#include "UART32.h"

volatile int16_t * audioData;
static int16_t volatile txData[48];
static uint8_t volatile writePointer = 0;
static uint8_t volatile readPointer = 0;
static uint8_t volatile dataCount = 0;

static int16_t RxData[8];

    
static USB_HANDLE AudioTXHandle;    //USB handle.  Must be initialized to 0 at startup.
static USB_HANDLE AudioRXHandle;    //USB handle.  Must be initialized to 0 at startup.

void USBAudio_init(){
    audioData = malloc(514);
    
    AudioTXHandle = NULL;
    AudioRXHandle = NULL;
    
    //set up sampling interrupt via ADC as a timer... Fs = 48kHz
    AD1CON1 = 0b1000000011100000;
    AD1CON2 = 0;
    AD1CON2bits.SMPI = 0;
    AD1CON3 = 0;
    AD1CON3bits.SAMC = 7;
    AD1CON3bits.ADCS = 24;
    
    IEC0bits.AD1IE = 1;
    IPC5bits.AD1IP = 7;

    //enable the Audio Streaming(Isochronous) endpoint
    //USBEnableEndpoint(USB_DEVICE_AUDIO_INPUT_ENDPOINT,USB_OUT_ENABLED | USB_IN_ENABLED | USB_DISALLOW_SETUP);
    USBEnableEndpoint(USB_DEVICE_AUDIO_OUTPUT_ENDPOINT , USB_IN_ENABLED | USB_DISALLOW_SETUP);
    
    //USBRxHandle = USBRxOnePacket(USB_DEVICE_AUDIO_INPUT_ENDPOINT, (uint8_t*)RxData, sizeof(RxData));
    
    AD1CON1SET = _AD1CON1_ASAM_MASK;
}

int32_t trash;

void __ISR(_ADC_VECTOR) USBAudio_sampleInt(){
    trash = ADC1BUF0;
    IFS0CLR = _IFS0_AD1IF_MASK;
    writePointer ++;
    audioData[writePointer] = (LATB & _LATB_LATB15_MASK) ? 8192 : 0;//(trash > 250000) ? 8192 : -8192;//
}

int16_t ma = 0;
void APP_DeviceAudioMicrophoneTasks()
{
    unsigned char frameCounter;

    /* If the device is not configured yet, or the device is suspended, then
     * we don't need to run the demo since we can't send any data.
     */
    if( (USBGetDeviceState() < CONFIGURED_STATE) ||
        (USBIsDeviceSuspended() == true))
    {
        return;
    }
    
    /* If the button is held down, play the audio file or continue playing the
     * audio file.  Stop playing the file when the button is released.
     */
    /* if we aren't already busy sending data. */
    if(!USBHandleBusy(AudioTXHandle)){
        uint8_t currPos = 0;
        for(currPos = 0; currPos < 48; currPos ++){
            LATBSET = _LATB_LATB5_MASK;
            LATBCLR = _LATB_LATB5_MASK;
            
            dataCount = writePointer - readPointer;
            if(dataCount > 1){ 
                ++readPointer;
                ma = (audioData[readPointer] + ma) >> 1;
            }
            
            UART_sendChar(currPos);
            UART_sendChar(writePointer);
            UART_sendChar(readPointer);
            UART_sendChar(dataCount);
            while((U2STA & _U2STA_TRMT_MASK) == 0);
            txData[currPos] = ma;
        }
        AudioTXHandle = USBTxOnePacket(USB_DEVICE_AUDIO_OUTPUT_ENDPOINT , (uint8_t*) txData, sizeof(txData));
    }
    
    /*if(!USBHandleBusy(USBRxHandle))
    {
        
        USBRxHandle = USBRxOnePacket(USB_DEVICE_AUDIO_INPUT_ENDPOINT, (uint8_t*)RxData, sizeof(RxData));
    }*/
}