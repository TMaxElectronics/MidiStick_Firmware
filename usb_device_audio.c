/********************************************************************************
  File Information:
    FileName:       usb_function_audio.c
    Dependencies:   See INCLUDES section
    Processor:      Microchip USB Microcontrollers
    Hardware:       Please see help file in "<install directory>\Microchip\Help"
	                for details.
    Complier:   	Microchip C18 (for PIC18) or C30 (for PIC24)
    Company:        Microchip Technology, Inc.

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

  Summary:
    This file contains all of functions, macros, definitions, variables,
    datatypes, etc. that are required for usage with the Audio function
    driver. This file should be included in projects that use the Audio
    \function driver.



    This file is located in the "\<Install Directory\>\\Microchip\\USB\\Audio
    Device Driver" directory.
  Description:
    USB Audio Function Driver File

    This file contains all of functions, macros, definitions, variables,
    datatypes, etc. that are required for usage with the Audio function
    driver. This file should be included in projects that use the Audio
    \function driver.

    This file is located in the "\<Install Directory\>\\Microchip\\USB\\Audio
    Device Driver" directory.

    When including this file in a new project, this file can either be
    referenced from the directory in which it was installed or copied
    directly into the user application folder. If the first method is
    chosen to keep the file located in the folder in which it is installed
    then include paths need to be added so that the library and the
    application both know where to reference each others files. If the
    application folder is located in the same folder as the Microchip
    folder (like the current demo folders), then the following include
    paths need to be added to the application's project:

    ..\\Include

    .

    If a different directory structure is used, modify the paths as
    required. An example using absolute paths instead of relative paths
    would be the following:

    C:\\Microchip Solutions\\Microchip\\Include

    C:\\Microchip Solutions\\My Demo Application
  ********************************************************************************/

 /** I N C L U D E S *******************************************************/
#include "usb_config.h"
#include "usb.h"
#include "usb_device_audio.h"

#ifdef USB_USE_AUDIO_CLASS

static void USBHandleGET_CUR();

#if defined USB_AUDIO_INPUT_TERMINAL_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_INPUT_TERMINAL_CONTROL_REQUESTS_HANDLER(void);
#endif 

#if defined USB_AUDIO_OUTPUT_TERMINAL_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_OUTPUT_TERMINAL_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_MIXER_UNIT_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_MIXER_UNIT_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_SELECTOR_UNIT_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_SELECTOR_UNIT_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_FEATURE_UNIT_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_FEATURE_UNIT_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_PROCESSING_UNIT_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_PROCESSING_UNIT_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_EXTENSION_UNIT_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_EXTENSION_UNIT_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_INTRFACE_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_INTRFACE_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_ENDPOINT_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_ENDPOINT_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_MEMORY_REQUESTS_HANDLER
    void USB_AUDIO_MEMORY_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_STATUS_REQUESTS_HANDLER
    void USB_AUDIO_STATUS_REQUESTS_HANDLER(void);
#endif


    
void USBCheckAudioRequest(void)
{
    /*
     * If request recipient is not an interface then return
     */
    if(SetupPkt.Recipient != USB_SETUP_RECIPIENT_INTERFACE_BITFIELD) return;
    /*
     * If request type is not class-specific then return
     */
    if(SetupPkt.RequestType != USB_SETUP_TYPE_CLASS_BITFIELD) return;
    /*
     * Interface ID must match interface numbers associated with
     * Audio class, else return
     */
    if((SetupPkt.bIntfID != AUDIO_CONTROL_INTERFACE_ID) && (SetupPkt.bIntfID != AUDIO_IN_INTERFACE_ID) && (SetupPkt.bIntfID != AUDIO_OUT_INTERFACE_ID)) return;
    
    uint8_t unitID = SetupPkt.wIndex >> 8;
    uint8_t controlSelect = SetupPkt.wValue >> 8;
    uint32_t data = 0x7fff;
    uint32_t minVol = 100;
    uint32_t maxVol = 0x7fff;
    uint32_t resVol = 1;
    
    switch(SetupPkt.bRequest){
        case SET_CUR:
            switch(controlSelect){
                case MUTE_CONTROL:
                    break;
                case VOLUME_CONTROL:
                    break;
            }
            USBEP0Transmit(USB_EP0_NO_DATA);
            break;
        case GET_CUR:
            
            switch(controlSelect){
                case MUTE_CONTROL:
                    USBEP0SendRAMPtr((uint8_t*) &data, 1, USB_EP0_INCLUDE_ZERO);
                    break;
                case VOLUME_CONTROL:
                    USBEP0SendRAMPtr((uint8_t*) &data, 2, USB_EP0_INCLUDE_ZERO);
                    break;
            }
            
            break;
        case GET_MIN:
            
            switch(controlSelect){
                case VOLUME_CONTROL:
                    USBEP0SendRAMPtr((uint8_t*) &minVol, 2, USB_EP0_INCLUDE_ZERO);
                    break;
            }
            break;
        case GET_MAX:
            
            switch(controlSelect){
                case VOLUME_CONTROL:
                    USBEP0SendRAMPtr((uint8_t*) &maxVol, 2, USB_EP0_INCLUDE_ZERO);
                    break;
            }
            break;
        case GET_RES:
            
            switch(controlSelect){
                case VOLUME_CONTROL:
                    USBEP0SendRAMPtr((uint8_t*) &resVol, 2, USB_EP0_INCLUDE_ZERO);
                    break;
            }
            break;
        default:
            LATAINV = 0;
            break;
            
    }//end switch(SetupPkt.bRequest
    USBEP0Transmit(USB_EP0_NO_DATA);
}//end USBCheckAudioRequest


#endif
