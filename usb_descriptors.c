
#ifndef __USB_DESCRIPTORS_C
#define __USB_DESCRIPTORS_C

#include "usb lib/usb.h"
#include "usb lib/usb_device_hid.h"

/* Device Descriptor */
const USB_DEVICE_DESCRIPTOR device_dsc=
{
    0x12,    // Size of this descriptor in bytes
    USB_DESCRIPTOR_DEVICE,                // DEVICE descriptor type
    0x0200,                 // USB Spec Release Number in BCD format
    0x00,                   // Class Code
    0x00,                   // Subclass code
    0x00,                   // Protocol code
    USB_EP0_BUFF_SIZE,          // Max packet size for EP0, see usb_config.h
    0x0888,                 // Vendor ID
    0x0002,                 // Product ID: Custom HID device demo
    0x0100,                 // Device release number in BCD format
    0x01,                   // Manufacturer string index
    0x02,                   // Product string index
    0x00,                   // Device serial number string index
    0x01                    // Number of possible configurations
};


const uint8_t configDescriptor1[]={
    /* Configuration Descriptor */
    0x09,//sizeof(USB_CFG_DSC),    // Size of this descriptor in bytes
    USB_DESCRIPTOR_CONFIGURATION,                // CONFIGURATION descriptor type
    0x5f,0x00,            // Total length of data for this cfg
    3,                      // Number of interfaces in this cfg
    1,                      // Index value of this configuration
    0,                      // Configuration string index
    _DEFAULT,               // Attributes, see usb_device.h
    50,                     // Max power consumption (2X mA)

    /* Interface Descriptor */
    0x09,//sizeof(USB_INTF_DSC),   // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,               // INTERFACE descriptor type
    0,                      // Interface Number
    0,                      // Alternate Setting Number
    0,                      // Number of endpoints in this interface
    1,                      // Class code
    1,                      // Subclass code
    0,                      // Protocol code
    0,                      // Interface string index

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
    0x01,       //bInterfaceNumber
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
    2,                      // Interface Number
    0,                      // Alternate Setting Number
    2,                      // Number of endpoints in this intf
    HID_INTF,               // Class code
    0,     // Subclass code
    0,     // Protocol code
    0,                      // Interface string index
    
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
    0x01                        //Interval
};


//Language code string descriptor
const struct{uint8_t bLength;uint8_t bDscType;uint16_t string[1];}sd000={
sizeof(sd000),USB_DESCRIPTOR_STRING,{0x0409
}};

//Manufacturer string descriptor
const struct{uint8_t bLength;uint8_t bDscType;uint16_t string[21];}sd001={
sizeof(sd001),USB_DESCRIPTOR_STRING,
{'T','M','a','x','-','E','l','e','c','t',
'r','o','n','i','c','s',' ','I','n','c','.'
}};

//Product string descriptor
const struct{uint8_t bLength;uint8_t bDscType;uint16_t string[14];}sd002={
sizeof(sd002),USB_DESCRIPTOR_STRING,
{'M','i','d','i','S','t','i','c','k',' ',' ',
' ',' ',' '
}}; 
  


//Array of configuration descriptors
const uint8_t *const USB_CD_Ptr[]=
{
    (const uint8_t *const)&configDescriptor1
};

//Array of string descriptors
const uint8_t *const USB_SD_Ptr[]=
{
    (const uint8_t *const)&sd000,
    (const uint8_t *const)&sd001,
    (const uint8_t *const)&sd002
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
