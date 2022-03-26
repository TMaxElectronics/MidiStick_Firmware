
#ifndef __USB_DESCRIPTORS_C
#define __USB_DESCRIPTORS_C

#include "usb.h"
#include "usb_device_hid.h"
#include "usb_device_audio.h"
#include "ConfigManager.h"

const USB_DEVICE_DESCRIPTOR device_dsc=
{
   sizeof(USB_DEVICE_DESCRIPTOR),    // Size of this descriptor in bytes
   USB_DESCRIPTOR_DEVICE,            // DEVICE descriptor type
   0x0200,                 // USB Spec Release Number in BCD format
   0x00,                   // Class Code
   0x00,                   // Subclass code
   0x00,                   // Protocol code
    USB_EP0_BUFF_SIZE,          // Max packet size for EP0, see usb_config.h
    0x1d50,                 // Vendor ID
    0x0002,                 // Product ID: Custom HID device demo
    0x0101,                 // Device release number in BCD format
   0x01,                   // Manufacturer string index
   0x02,                   // Product string index
    0x03,                   // Device serial number string index
   0x01                    // Number of possible configurations
};


const uint8_t configDescriptor1[]={
    /* Configuration Descriptor */
    0x09,//sizeof(USB_CFG_DSC),    // Size of this descriptor in bytes
    USB_DESCRIPTOR_CONFIGURATION,                // CONFIGURATION descriptor type
    0x04,0x01,            // Total length of data for this cfg
    6,                      // Number of interfaces in this cfg
    1,                      // Index value of this configuration
    0,                      // Configuration string index
    _DEFAULT,               // Attributes, see usb_device.h
    50,                     // Max power consumption (2X mA)

    /* Interface Descriptor */
    0x09,//sizeof(USB_INTF_DSC),   // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,               // INTERFACE descriptor type
    DESCRIPTOR_INTERFACE_ID,                      // Interface Number
    0,                      // Alternate Setting Number
    0,                      // Number of endpoints in this interface
    1,                      // Class code
    1,                      // Subclass code
    0,                      // Protocol code
    2,                      // Interface string index
    
    /* MIDI Adapter Class-specific AC Interface Descriptor */
    0x09,       //bLength
    0x24,       //bDescriptorType - CS_INTERFACE
    0x01,       //bDescriptorSubtype - HEADER
    0x00,0x01,  //bcdADC
    0x09,0x00,  //wTotalLength
    0x01,       //bInCollection
    0x01,       //baInterfaceNr(1)

    /* MIDI Adapter Standard MS Interface Descriptor */
    0x09,       //bLength
    0x04,       //bDescriptorType
    AUDIO_MIDI_INTERFACE_ID,       //bInterfaceNumber
    0x00,       //bAlternateSetting
    0x01,       //bNumEndpoints
    0x01,       //bInterfaceClass
    0x03,       //bInterfaceSubclass
    0x00,       //bInterfaceProtocol
    0x00,       //iInterface

    /* MIDI Adapter Class-specific MS Interface Descriptor */
    0x07,       //bLength
    0x24,       //bDescriptorType - CS_INTERFACE
    0x01,       //bDescriptorSubtype - MS_HEADER
    0x00,0x01,  //BcdADC
    0x1b,0x00,  //wTotalLength
    
    /* MIDI Adapter MIDI IN Jack Descriptor (Embedded) */
    0x06,       //bLength
    0x24,       //bDescriptorType - CS_INTERFACE
    0x02,       //bDescriptorSubtype - MIDI_IN_JACK
    0x01,       //bJackType - EMBEDDED
    0x01,       //bJackID
    0x00,       //iJack

    /* MIDI Adapter Standard Bulk OUT Endpoint Descriptor */
    0x09,       //bLength
    0x05,       //bDescriptorType - ENDPOINT
    USB_DEVICE_AUDIO_MIDI_ENDPOINT | _EP_OUT,       //bEndpointAddress - OUT
    0x02,       //bmAttributes
    0x40,0x00,  //wMaxPacketSize
    0x00,       //bInterval
    0x00,       //bRefresh
    0x00,       //bSynchAddress

    /* MIDI Adapter Class-specific Bulk OUT Endpoint Descriptor */
    0x05,       //bLength
    0x25,       //bDescriptorType - CS_ENDPOINT
    0x01,       //bDescriptorSubtype - MS_GENERAL
    0x01,       //bNumEmbMIDIJack
    0x01,       //BaAssocJackID(1)



    /* Interface Descriptor */
    0x09,//sizeof(USB_INTF_DSC),   // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,               // INTERFACE descriptor type
    AUDIO_HID_INTERFACE_ID,                      // Interface Number
    0,                      // Alternate Setting Number
    2,                      // Number of endpoints in this intf
    HID_INTF,               // Class code
    0,     // Subclass code
    0,     // Protocol code
    2,                      // Interface string index
    
    //offset here = 72
    /* HID Class-Specific Descriptor */
    0x09,//sizeof(USB_HID_DSC)+3,    // Size of this descriptor in bytes
    DSC_HID,                // HID descriptor type
    0x11,0x01,                 // HID Spec Release Number in BCD format (1.11)
    0x00,                   // Country Code (0x00 for Not supported)
    HID_NUM_OF_DSC,         // Number of class descriptors, see usbcfg.h
    DSC_RPT,                // Report descriptor type
    HID_RPT01_SIZE,0x00,//sizeof(hid_rpt01),      // Size of the report descriptor

    
    /* Endpoint Descriptor */
    0x07,/*sizeof(USB_EP_DSC)*/
    USB_DESCRIPTOR_ENDPOINT,    //Endpoint Descriptor
    USB_DEVICE_AUDIO_CONFIG_ENDPOINT | _EP_IN,                   //EndpointAddress
    _INTERRUPT,                       //Attributes
    0x40,0x00,                  //size
    0x01,                        //Interval

    /* Endpoint Descriptor */
    0x07,/*sizeof(USB_EP_DSC)*/
    USB_DESCRIPTOR_ENDPOINT,    //Endpoint Descriptor
    USB_DEVICE_AUDIO_CONFIG_ENDPOINT | _EP_OUT,                   //EndpointAddress
    _INTERRUPT,                       //Attributes
    0x40,0x00,                  //size
    0x01,                        //Interval
    
    
    //Interface 0 audio control
    
    /* USB Microphone Standard AC Interface Descriptor	*/
    0x09,//sizeof(USB_INTF_DSC),    // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,       // INTERFACE descriptor type
    AUDIO_CONTROL_INTERFACE_ID,     // Interface Number
    0x00,                           // Alternate Setting Number
    0x00,                           // Number of endpoints in this intf
    AUDIO_DEVICE,                   // Class code
    AUDIOCONTROL,                   // Subclass code
    0x00,                           // Protocol code
    0x00,                           // Interface string index

        /* USB Microphone Class-specific AC Interface Descriptor  */
        0x0a,                           // Size of this descriptor, in bytes.
        CS_INTERFACE,                   // CS_INTERFACE Descriptor Type
        HEADER,                         // HEADER descriptor subtype
        0x00,0x01,                      // Audio Device compliant to the USB Audio specification version 1.00
        0x34,0x00,                      // Total number of bytes returned for the class-specific AudioControl interface descriptor.
                                        // Includes the combined length of this descriptor header and all Unit and Terminal descriptors.
        0x02,                           // The number of AudioStreaming interfaces in the Audio Interface Collection to which this AudioControl interface belongs
        AUDIO_IN_INTERFACE_ID,                           // AudioStreaming interface 1 belongs to this AudioControl interface.
        AUDIO_OUT_INTERFACE_ID,                           // AudioStreaming interface 1 belongs to this AudioControl interface.

        /* Output sample Input Terminal Descriptor */
        0x0C,                           // Size of the descriptor, in bytes
        CS_INTERFACE,                   // CS_INTERFACE Descriptor Type
        INPUT_TERMINAL,                 // INPUT_TERMINAL descriptor subtype
        AUDIO_SAMPLE_INPUT,             // ID of this Terminal.
        DIGITAL_AUDIO,                     // Terminal is Microphone (0x01,0x02)
        0x00,                           // No association
        0x01,                           // One channel
        0x00,0x00,                      // Mono sets no position bits
        0x00,                           // Unused.
        0x00,                           // Unused.

        /* USB data Input Terminal Descriptor */
        0x0C,                           // Size of the descriptor, in bytes
        CS_INTERFACE,                   // CS_INTERFACE Descriptor Type
        INPUT_TERMINAL,                 // INPUT_TERMINAL descriptor subtype
        AUDIO_USB_INPUT,                // ID of this Terminal.
        USB_STREAMING,                  // Terminal is Microphone (0x01,0x02)
        0x00,                           // No association
        0x01,                           // One channel
        0x00,0x00,                      // Mono sets no position bits
        0x00,                           // Unused.
        0x00,                           // Unused.

        /* USB data Output Terminal Descriptor */
        0x09,                           // Size of the descriptor, in bytes (bLength)
        CS_INTERFACE,                   // CS_INTERFACE Descriptor Type (bDescriptorType)
        OUTPUT_TERMINAL,                // OUTPUT_TERMINAL descriptor subtype (bDescriptorSubtype)
        AUDIO_USB_OUTPUT,                              // ID of this Terminal. (bTerminalID)
        USB_STREAMING,                  // USB Streaming. (wTerminalType
        0x00,                           // unused (bAssocTerminal)
        AUDIO_SAMPLE_INPUT,                              // From Input Terminal.(bSourceID)
        0x00,                           // unused  (iTerminal)

        /* Synthesizer data Terminal Descriptor */
        0x09,                           // Size of the descriptor, in bytes (bLength)
        CS_INTERFACE,                   // CS_INTERFACE Descriptor Type (bDescriptorType)
        OUTPUT_TERMINAL,                // OUTPUT_TERMINAL descriptor subtype (bDescriptorSubtype)
        AUDIO_COIL_OUTPUT,              // ID of this Terminal. (bTerminalID)
        SPEAKER,                        // USB Streaming. (wTerminalType
        0x00,                           // unused (bAssocTerminal)
        AUDIO_USB_INPUT,                              // From Input Terminal.(bSourceID)
        0x00,                           // unused  (iTerminal)

    //Interface 1 - AudioSteam output
    
    /* USB Microphone Standard AS Interface Descriptor (Alt. Set. 0) */
    0x09,                           // Size of the descriptor, in bytes (bLength)
    USB_DESCRIPTOR_INTERFACE,       // INTERFACE descriptor type (bDescriptorType)
    AUDIO_IN_INTERFACE_ID,          // Index of this interface. (bInterfaceNumber)
    0x00,                           // Index of this alternate setting. (bAlternateSetting)
    0x00,                           // 0 endpoints. (bNumEndpoints)
    AUDIO_DEVICE,                   // AUDIO (bInterfaceClass)
    AUDIOSTREAMING,                 // AUDIO_STREAMING (bInterfaceSubclass)
    0x00,                           // Unused. (bInterfaceProtocol)
    0x00,                           // Unused. (iInterface)

    /* USB Microphone Standard AS Interface Descriptor (Alt. Set. 1) */
    0x09,                           // Size of the descriptor, in bytes (bLength)
    USB_DESCRIPTOR_INTERFACE,       // INTERFACE descriptor type (bDescriptorType)
    AUDIO_IN_INTERFACE_ID,          // Index of this interface. (bInterfaceNumber)
    0x01,                           // Index of this alternate setting. (bAlternateSetting)
    0x01,                           // 1 endpoint (bNumEndpoints)
    AUDIO_DEVICE,                   // AUDIO (bInterfaceClass)
    AUDIOSTREAMING,                 // AUDIO_STREAMING (bInterfaceSubclass)
    0x00,                           // Unused. (bInterfaceProtocol)
    0x00,                           // Unused. (iInterface)

        /*  USB Microphone Class-specific AS General Interface Descriptor */
        0x07,                           // Size of the descriptor, in bytes (bLength)
        CS_INTERFACE,                   // CS_INTERFACE Descriptor Type (bDescriptorType)
        AS_GENERAL,                     // GENERAL subtype (bDescriptorSubtype)
        AUDIO_USB_INPUT,                              // Unit ID of the Output Terminal.(bTerminalLink)
        0x01,                           // Interface delay. (bDelay)
        0x01,0x00,                      // PCM Format (wFormatTag)

        /*  USB Microphone Type I Format Type Descriptor */
        0x0B,                           // Size of the descriptor, in bytes (bLength)
        CS_INTERFACE,                   // CS_INTERFACE Descriptor Type (bDescriptorType)
        FORMAT_TYPE,                    // FORMAT_TYPE subtype. (bDescriptorSubtype)
        0x01,                           // FORMAT_TYPE_I. (bFormatType)
        0x01,                           // One channel.(bNrChannels)
        0x02,                           // Two bytes per audio subframe.(bSubFrameSize)
        0x10,                           // 16 bits per sample.(bBitResolution)
        0x01,                           // bSamFreqType -> 0x02 = 2 sample frequencies supported
        0x80,0xBB,0x00,                 // tSamFreq -> 0xBB80 = 48000 Hz

        /*  USB Microphone Standard Endpoint Descriptor */
        0x09,                           // Size of the descriptor, in bytes (bLength)
        0x05,                           // ENDPOINT descriptor (bDescriptorType)
        USB_DEVICE_AUDIO_INPUT_ENDPOINT | _EP_IN,                  // Endpoint number. (bEndpointAddress)         
        0x09,                           // Isochronous, not shared. (bmAttributes)
        0x10,0x00,                      // 16 bytes per packet (wMaxPacketSize)
        0x01,                           // One packet per frame.(bInterval)
        0x00,                           // Unused. (bRefresh)
        0x00,                           // Unused. (bSynchAddress)

        /* USB Microphone Class-specific Isoc. Audio Data Endpoint Descriptor*/
        0x07,                           // Size of the descriptor, in bytes (bLength)
        CS_ENDPOINT,                    // CS_ENDPOINT Descriptor Type (bDescriptorType)
        AS_GENERAL,                     // GENERAL subtype. (bDescriptorSubtype)
        0x01,                           // No sampling frequency control, no pitch control, no packet padding.(bmAttributes)
        0x00,                           // Unused. (bLockDelayUnits)
        0x00,0x00,                       // Unused. (wLockDelay)
            
    

    //Interface 2 - AudioSteam input
            
    /* USB Microphone Standard AS Interface Descriptor (Alt. Set. 0) */
    0x09,                           // Size of the descriptor, in bytes (bLength)
    USB_DESCRIPTOR_INTERFACE,       // INTERFACE descriptor type (bDescriptorType)
    AUDIO_OUT_INTERFACE_ID,   // Index of this interface. (bInterfaceNumber)
    0x00,                           // Index of this alternate setting. (bAlternateSetting)
    0x00,                           // 0 endpoints. (bNumEndpoints)
    AUDIO_DEVICE,                   // AUDIO (bInterfaceClass)
    AUDIOSTREAMING,                 // AUDIO_STREAMING (bInterfaceSubclass)
    0x00,                           // Unused. (bInterfaceProtocol)
    0x00,                           // Unused. (iInterface)

    /* USB Microphone Standard AS Interface Descriptor (Alt. Set. 1) */
    0x09,                           // Size of the descriptor, in bytes (bLength)
    USB_DESCRIPTOR_INTERFACE,       // INTERFACE descriptor type (bDescriptorType)
    AUDIO_OUT_INTERFACE_ID,   // Index of this interface. (bInterfaceNumber)
    0x01,                           // Index of this alternate setting. (bAlternateSetting)
    0x01,                           // 1 endpoint (bNumEndpoints)
    AUDIO_DEVICE,                   // AUDIO (bInterfaceClass)
    AUDIOSTREAMING,                 // AUDIO_STREAMING (bInterfaceSubclass)
    0x00,                           // Unused. (bInterfaceProtocol)
    0x00,                           // Unused. (iInterface)

        /*  USB Microphone Class-specific AS General Interface Descriptor */
        0x07,                           // Size of the descriptor, in bytes (bLength)
        CS_INTERFACE,                   // CS_INTERFACE Descriptor Type (bDescriptorType)
        AS_GENERAL,                     // GENERAL subtype (bDescriptorSubtype)
        AUDIO_USB_OUTPUT,               // Unit ID of the Output Terminal.(bTerminalLink)
        0x01,                           // Interface delay. (bDelay)
        0x01,0x00,                      // PCM Format (wFormatTag)

        /*  USB Microphone Type I Format Type Descriptor */
        0x0B,                           // Size of the descriptor, in bytes (bLength)
        CS_INTERFACE,                   // CS_INTERFACE Descriptor Type (bDescriptorType)
        FORMAT_TYPE,                    // FORMAT_TYPE subtype. (bDescriptorSubtype)
        0x01,                           // FORMAT_TYPE_I. (bFormatType)
        0x01,                           // One channel.(bNrChannels)
        0x02,                           // Two bytes per audio subframe.(bSubFrameSize)
        0x10,                           // 16 bits per sample.(bBitResolution)
        0x01,                           // bSamFreqType -> 0x02 = 2 sample frequencies supported
        0x80,0xBB,0x00,                 // tSamFreq -> 0xBB80 = 48000 Hz

        /*  USB Microphone Standard Endpoint Descriptor */
        0x09,                           // Size of the descriptor, in bytes (bLength)
        0x05,                           // ENDPOINT descriptor (bDescriptorType)                              
        USB_DEVICE_AUDIO_OUTPUT_ENDPOINT | _EP_IN,     // Endpoint number. (bEndpointAddress)                      
        0x09,                           // Isochronous, not shared. (bmAttributes)
        0x60,0x00,                      // 16 bytes per packet (wMaxPacketSize)
        0x01,                           // One packet per frame.(bInterval)
        0x00,                           // Unused. (bRefresh)
        0x00,                           // Unused. (bSynchAddress)

        /* USB Microphone Class-specific Isoc. Audio Data Endpoint Descriptor*/
        0x07,                           // Size of the descriptor, in bytes (bLength)
        CS_ENDPOINT,                    // CS_ENDPOINT Descriptor Type (bDescriptorType)
        AS_GENERAL,                     // GENERAL subtype. (bDescriptorSubtype)
        0x01,                           // No sampling frequency control, no pitch control, no packet padding.(bmAttributes)
        0x00,                           // Unused. (bLockDelayUnits)
        0x00,0x00                       // Unused. (wLockDelay)
};


//Serial number descriptor is dynamically loaded, so this is not used anymore
const struct{uint8_t bLength;uint8_t bDscType;uint16_t string[1];}sd000={
sizeof(sd000),USB_DESCRIPTOR_STRING,{0x0410
}};

//Manufacturer string descriptor
const struct{uint8_t bLength;uint8_t bDscType;uint16_t string[21];}sd001={
sizeof(sd001),USB_DESCRIPTOR_STRING,
{'T','M','a','x','-','E','l','e','c','t',
'r','o','n','i','c','s',' ','I','n','c','.'
}};

//Array of configuration descriptors
const uint8_t *const USB_CD_Ptr[]=
{
    (const uint8_t *const)&configDescriptor1
};

//Array of string descriptors, &sd000, means that the data will be dynamically loaded in from ram
const uint8_t *const USB_SD_Ptr[]=
{
    (const uint8_t *const)&sd000,
    (const uint8_t *const)&sd001,
    (const uint8_t *const)&sd000
};

//Class specific descriptor - HID 
const struct{uint8_t report[HID_RPT01_SIZE];}hid_rpt01={
{
    0x06, 0x00, 0xFF,       // Usage Page = 0xFF00 (Vendor Defined Page 1)
    0x09, 0x01,             // Usage (Vendor Usage 1)
    0xA1, 0x01,             // Collection (Application)
    0x19, 0x01,             //      Usage Minimum 
    0x29, 0x40,             //      Usage Maximum   //64 input usages total (0x01 to 0x40)
    0x15, 0x00,             //      Logical Minimum (data bytes in the report may have minimum value = 0x00)
    0x26, 0xFF, 0x00,       //      Logical Maximum (data bytes in the report may have maximum value = 0x00FF = unsigned 255)
    0x75, 0x08,             //      Report Size: 8-bit field size
    0x95, 0x40,             //      Report Count: Make sixty-four 8-bit fields (the next time the parser hits an "Input", "Output", or "Feature" item)
    0x81, 0x00,             //      Input (Data, Array, Abs): Instantiates input packet fields based on the above report size, count, logical min/max, and usage.
    0x19, 0x01,             //      Usage Minimum 
    0x29, 0x40,             //      Usage Maximum 	//64 output usages total (0x01 to 0x40)
    0x91, 0x00,             //      Output (Data, Array, Abs): Instantiates output packet fields.  Uses same report size and count as "Input" fields, since nothing new/different was specified to the parser since the "Input" item.
    0xC0}                   // End Collection
};                 

/** EOF usb_descriptors.c ***************************************************/

#endif
