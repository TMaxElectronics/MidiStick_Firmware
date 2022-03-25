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

/* This audio sample is a 200Hz sin wave, sampled at 8000Hz, with a 10-bit
 * resolution.  The audio input is specified in this example to be a 16-bit 
 * microphone, but during testing on various OSes showed that some systems have
 * trouble receiving more than a 10-bit input without causing distortion.
 *
 * The data below is generated via the following formula:
 *   audioSamples[x] = 1024 * sine(2 * pi * (x+1)/8000Hz/200Hz) for x=0:39
 * x=0:39 because there are 40 samples required for one full cycle of the sine
 * wave (8000Hz/200Hz = exactly 40 samples/cycle).
 */
const int16_t audioSamples[] =
{
    160,
    316,
    465,
    602,
    724,
    828,
    912,
    974,
    1011,
    1024,
    1011,
    974,
    912,
    828,
    724,
    602,
    465,
    316,
    160,
    0,
    -160,
    -316,
    -465,
    -602,
    -724,
    -828,
    -912,
    -974,
    -1011,
    -1024,
    -1011,
    -974,
    -912,
    -828,
    -724,
    -602,
    -465,
    -316,
    -160,
    0,
};

static int16_t TxData[48];
static int16_t RxData[8];

    
static USB_HANDLE AudioTXHandle;    //USB handle.  Must be initialized to 0 at startup.
static USB_HANDLE AudioRXHandle;    //USB handle.  Must be initialized to 0 at startup.
static unsigned long currentSample;
static int16_t FrameData[8];

/*********************************************************************
* Function: void APP_DeviceAudioMicrophoneInitialize(void);
*
* Overview: Initializes the demo code
*
* PreCondition: None
*
* Input: None
*
* Output: None
*
********************************************************************/
void USBAudio_init(){
    AudioTXHandle = NULL;
    AudioRXHandle = NULL;

    currentSample = 0;
    
    //set up sampling interrupt via ADC as a timer... Fs = 48kHz
    AD1CON1 = 0b1000000011100000;
    AD1CON2 = 0;
    AD1CON3 = 0;
    AD1CON3bits.SAMC = 7;
    AD1CON3bits.ADCS = 24;
    
    IEC0bits.AD1IE = 1;
    IPC5bits.AD1IP = 5;

    //enable the Audio Streaming(Isochronous) endpoint
    //USBEnableEndpoint(USB_DEVICE_AUDIO_INPUT_ENDPOINT,USB_OUT_ENABLED | USB_IN_ENABLED | USB_DISALLOW_SETUP);
    USBEnableEndpoint(USB_DEVICE_AUDIO_OUTPUT_ENDPOINT , USB_IN_ENABLED | USB_DISALLOW_SETUP);
    
    //USBRxHandle = USBRxOnePacket(USB_DEVICE_AUDIO_INPUT_ENDPOINT, (uint8_t*)RxData, sizeof(RxData));
    
    AD1CON1SET = _AD1CON1_ASAM_MASK;
}

void __ISR(_ADC_VECTOR) USBAudio_sampleInt(){
    uint32_t trash = ADC1BUF0;
    IFS0CLR = _IFS0_AD1IF_MASK;
}

/*********************************************************************
* Function: void APP_DeviceAudioMicrophoneInitialize(void);
*
* Overview: Initializes the demo code
*
* PreCondition: None
*
* Input: None
*
* Output: None
*
********************************************************************/
void APP_DeviceAudioMicrophoneSOFHandler()
{
    
}

/*********************************************************************
* Function: void APP_DeviceAudioMicrophoneTasks(void);
*
* Overview: Keeps the Custom HID demo running.
*
* PreCondition: The demo should have been initialized and started via
*   the APP_DeviceAudioMicrophoneInitialize() and APP_DeviceAudioMicrophoneStart() demos
*   respectively.
*
* Input: None
*
* Output: None
*
********************************************************************/
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
        /* then for each of the entries in the FrameData buffer... */
        for(frameCounter = 0; frameCounter < sizeof(TxData)/sizeof(int16_t); frameCounter++)
        {
            /* copy the next sample from the audioSamples array. */
            TxData[frameCounter] = audioSamples[currentSample++];

            /* If we have reached the end of the audioSamples array, jump
             * back to the start in this case so that we keep playing the
             * same file/tone continuously as long as the button is held.
             */
            if( currentSample >= (sizeof(audioSamples)/sizeof(int16_t)) )
            {
                currentSample = 0;
            }
        }

        /* actually send the data now. */
        AudioTXHandle = USBTxOnePacket(USB_DEVICE_AUDIO_OUTPUT_ENDPOINT , (uint8_t*)TxData, sizeof(TxData));
    
        //LATBINV = _LATB_LATB5_MASK;
    }
    
    /*if(!USBHandleBusy(USBRxHandle))
    {
        
        USBRxHandle = USBRxOnePacket(USB_DEVICE_AUDIO_INPUT_ENDPOINT, (uint8_t*)RxData, sizeof(RxData));
    }*/
}