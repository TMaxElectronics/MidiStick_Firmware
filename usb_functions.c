
#include "usb lib/usb.h"
#include "usb lib/usb_device_hid.h"
#include "usb lib/usb_functions.h"

#include <string.h>

unsigned char ReceivedDataBuffer[64];
unsigned char ToSendDataBuffer[64];

volatile USB_HANDLE USBOutHandle;    
volatile USB_HANDLE USBInHandle;

typedef enum
{
    COMMAND_TOGGLE_LED = 0x80,
    COMMAND_GET_BUTTON_STATUS = 0x81,
    COMMAND_READ_POTENTIOMETER = 0x37
} CUSTOM_HID_DEMO_COMMANDS;


void APP_DeviceCustomHIDInitialize()
{
    //initialize the variable holding the handle for the last
    // transmission
    USBInHandle = 0;

    //enable the HID endpoint
    USBEnableEndpoint(CUSTOM_DEVICE_HID_EP, USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);

    //Re-arm the OUT endpoint for the next packet
    USBOutHandle = (volatile USB_HANDLE)HIDRxPacket(CUSTOM_DEVICE_HID_EP,(uint8_t*)&ReceivedDataBuffer[0],64);
}

void usbTasks()
{   
    USBDeviceTasks();
    /* If the USB device isn't configured yet, we can't really do anything
     * else since we don't have a host to talk to.  So jump back to the
     * top of the while loop. */
    if( USBGetDeviceState() < CONFIGURED_STATE )
    {
        return;
    }

    /* If we are currently suspended, then we need to see if we need to
     * issue a remote wakeup.  In either case, we shouldn't process any
     * keyboard commands since we aren't currently communicating to the host
     * thus just continue back to the start of the while loop. */
    if( USBIsDeviceSuspended()== true )
    {
        return;
    }
    
    //Check if we have received an OUT data packet from the host
    if(HIDRxHandleBusy(USBOutHandle) == false)
    {   
        //We just received a packet of data from the USB host.
        //Check the first uint8_t of the packet to see what command the host
        //application software wants us to fulfill.
        switch(ReceivedDataBuffer[0])				//Look at the data the host sent, to see what kind of application specific command it sent.
        {
            case COMMAND_TOGGLE_LED:  //Toggle LEDs command
                toggleState = !toggleState;
                setAllLEDs(toggleState * currColor[0], toggleState * currColor[1], toggleState * currColor[2], toggleState * currColor[3]);
                if(initDim) break;
                generationPending = 1;
                break;
            case COMMAND_GET_BUTTON_STATUS:  //Get push button state
                //Check to make sure the endpoint/buffer is free before we modify the contents
                if(!HIDTxHandleBusy(USBInHandle))
                {
                    ToSendDataBuffer[0] = 0x81;				//Echo back to the host PC the command we are fulfilling in the first uint8_t.  In this case, the Get Pushbutton State command.
                    if(false)	//pushbutton not pressed, pull up resistor on circuit board is pulling the PORT pin high
                    {
                            ToSendDataBuffer[1] = 0x01;
                    }
                    else									//sw3 must be == 0, pushbutton is pressed and overpowering the pull up resistor
                    {
                            ToSendDataBuffer[1] = 0x00;
                    }
                    //Prepare the USB module to send the data packet to the host
                    USBInHandle = HIDTxPacket(CUSTOM_DEVICE_HID_EP, (uint8_t*)&ToSendDataBuffer[0],64);
                }
                break;
                
            case 0x10:  //Set LED Data
                toggleState = 1;
                currColor[0] = ReceivedDataBuffer[1];
                currColor[1] = ReceivedDataBuffer[2];
                currColor[2] = ReceivedDataBuffer[3];
                currColor[3] = ReceivedDataBuffer[4];
                if(currAnimation != 0) break;
                setAllLEDsA(ReceivedDataBuffer[1], ReceivedDataBuffer[2], ReceivedDataBuffer[3], ReceivedDataBuffer[4], ReceivedDataBuffer[5] << 8 | ReceivedDataBuffer[6]);
                generationPending = 1;
                break;
                
            case 0x20:  //Set LED Animation
                if(ReceivedDataBuffer[1] == 0){ //stop animation
                    setCurrAnimation(ReceivedDataBuffer[1]);
                }else{
                    setCurrAnimation(ReceivedDataBuffer[1]);
                    setFPS(ReceivedDataBuffer[2]); 
                    animParam1 = ReceivedDataBuffer[3];
                }
                break;
        }
        //Re-arm the OUT endpoint, so we can receive the next OUT data packet 
        //that the host may try to send us.
        USBOutHandle = HIDRxPacket(CUSTOM_DEVICE_HID_EP, (uint8_t*)&ReceivedDataBuffer[0], 64);
    }
}