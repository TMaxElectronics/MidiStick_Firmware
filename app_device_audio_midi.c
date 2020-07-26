/*******************************************************************************
Copyright 2016 Microchip Technology Inc. (www.microchip.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

To request to license the code under the MLA license (www.microchip.com/mla_license), 
please contact mla_licensing@microchip.com
*******************************************************************************/

/** INCLUDES *******************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>

#include "system.h"

#include "usb.h"
#include "usb_device_midi.h"


/** VARIABLES ******************************************************/
/* Some processors have a limited range of RAM addresses where the USB module
 * is able to access.  The following section is for those devices.  This section
 * assigns the buffers that need to be used by the USB module into those
 * specific areas.
 */
#if defined(FIXED_ADDRESS_MEMORY)
    #if defined(COMPILER_MPLAB_C18)
        #pragma udata DEVICE_AUDIO_MIDI_RX_DATA_BUFFER=DEVCE_AUDIO_MIDI_RX_DATA_BUFFER_ADDRESS
            static uint8_t ReceivedDataBuffer[64];
        #pragma udata DEVICE_AUDIO_MIDI_EVENT_DATA_BUFFER=DEVCE_AUDIO_MIDI_EVENT_DATA_BUFFER_ADDRESS
            static USB_AUDIO_MIDI_EVENT_PACKET midiData;
        #pragma udata
    #elif defined(__XC8)
        static uint8_t ReceivedDataBuffer[64] @ DEVCE_AUDIO_MIDI_RX_DATA_BUFFER_ADDRESS;
        static USB_AUDIO_MIDI_EVENT_PACKET midiData @ DEVCE_AUDIO_MIDI_EVENT_DATA_BUFFER_ADDRESS;
    #endif
#else
    static uint8_t ReceivedDataBuffer[64];
    static USB_AUDIO_MIDI_EVENT_PACKET midiData;
#endif

static USB_HANDLE USBTxHandle;
static USB_HANDLE USBRxHandle;

static uint8_t pitch;
static bool sentNoteOff;

static USB_VOLATILE uint8_t msCounter;

/*********************************************************************
* Function: void APP_DeviceAudioMIDIInitialize(void);
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
void APP_DeviceAudioMIDIInitialize()
{
    USBTxHandle = NULL;
    USBRxHandle = NULL;

    UART_init(9600);
    
    UART_sendString("TMax UsbMidi V1.0 ready", 1);
    
    pitch = 0x3C;
    sentNoteOff = true;

    msCounter = 0;

    //enable the HID endpoint
    USBEnableEndpoint(USB_DEVICE_AUDIO_MIDI_ENDPOINT,USB_OUT_ENABLED|USB_IN_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);

    //Re-arm the OUT endpoint for the next packet
    USBRxHandle = USBRxOnePacket(USB_DEVICE_AUDIO_MIDI_ENDPOINT,(uint8_t*)&ReceivedDataBuffer,64);
}

/*********************************************************************
* Function: void APP_DeviceAudioMIDIInitialize(void);
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
void APP_DeviceAudioMIDISOFHandler()
{
    if(msCounter != 0)
    {
        msCounter--;
    }
}


/*********************************************************************
* Function: void APP_DeviceAudioMIDITasks(void);
*
* Overview: Keeps the Custom HID demo running.
*
* PreCondition: The demo should have been initialized and started via
*   the APP_DeviceAudioMIDIInitialize() and APP_DeviceAudioMIDIStart() demos
*   respectively.
*
* Input: None
*
* Output: None
*
********************************************************************/
void APP_DeviceAudioMIDITasks()
{
    /* If the device is not configured yet, or the device is suspended, then
     * we don't need to run the demo since we can't send any data.
     */
    if( (USBGetDeviceState() < CONFIGURED_STATE) ||
        (USBIsDeviceSuspended() == true))
    {
        return;
    }

    if(!USBHandleBusy(USBRxHandle))
    {
        
        //UART_sendString("Data:", 1);
        
        //for(int b = 0; b < 64; b++){
        //    UART_sendHex(ReceivedDataBuffer[b], 1);
        //}
        
        //We have received a MIDI packet from the host, process it and then
        //  prepare to receive the next packet
        

                

        //Get ready for next packet (this will overwrite the old data)
        USBRxHandle = USBRxOnePacket(USB_DEVICE_AUDIO_MIDI_ENDPOINT,(uint8_t*)&ReceivedDataBuffer,64);
    }

    /* If the user button is pressed... 
    if(0)
    {
        if(msCounter == 0)
        {
            if(sentNoteOff == true)
            {
                if(!USBHandleBusy(USBTxHandle))
                {
                    //Then reset the 100ms counter
                    msCounter = 100;

                    midiData.Val = 0;   //must set all unused values to 0 so go ahead
                                        //  and set them all to 0

                    midiData.CableNumber = 0;
                    midiData.CodeIndexNumber = MIDI_CIN_NOTE_ON;
                    midiData.DATA_0 = 0x90;         //Note on
                    midiData.DATA_1 = pitch;         //pitch
                    midiData.DATA_2 = 0x7F;  //velocity

                    USBTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_MIDI_ENDPOINT,(uint8_t*)&midiData,4);

                    sentNoteOff = false;
                }
            }
        }
    }
    else
    {
        if(msCounter == 0)
        {
            if(sentNoteOff == false)
            {
                if(!USBHandleBusy(USBTxHandle))
                {
                    //Debounce counter for 100ms
                    msCounter = 100;

                    midiData.Val = 0;   //must set all unused values to 0 so go ahead
                                        //  and set them all to 0

                    midiData.CableNumber = 0;
                    midiData.CodeIndexNumber = MIDI_CIN_NOTE_ON;
                    midiData.DATA_0 = 0x90;     //Note off
                    midiData.DATA_1 = pitch++;     //pitch
                    midiData.DATA_2 = 0x00;        //velocity

                    if(pitch == 0x49)
                    {
                        pitch = 0x3C;
                    }

                    USBTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_MIDI_ENDPOINT,(uint8_t*)&midiData,4);
                    sentNoteOff = true;
                }
            }
        }
    }*/
}
