
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <xc.h>
#include <sys/attribs.h>
#include <string.h>

#include "usb lib/usb.h"
#include "MidiController.h"
#include "ConfigManager.h"

//Programm and coil configurations
MidiProgramm * Midi_currProgramm;
MidiProgramm * Midi_currOverrideProgramm;
CoilConfig * Midi_currCoil;
uint8_t Midi_currCoilIndex = 0xff;          //this is used by the pc software to figure out which configuration is active

//TODO make this dynamic
uint8_t Midi_currChannel = 0;

//USB comms stuff
static uint8_t ReceivedDataBuffer[64];          //data received from the midi endpoint
static uint8_t ConfigReceivedDataBuffer[64];    //data received from the config endpoint
static uint8_t ToSendDataBuffer[64];            //data to send to the config endpoint
static USB_HANDLE Midi_dataHandle;              //Micochip usb library handles
static USB_HANDLE Midi_configTxHandle;
static USB_HANDLE Midi_configRxHandle;

//Global midi settings
unsigned MIDI_programmOverrideEnabled = 0;  //sets wether data from Midi_currOverrideProgramm or Midi_currProgramm is used
uint8_t Midi_currVolume = 0;                
double Midi_currBendFactor = 1;             

//Voice 1 variables
uint8_t Midi_adsrState1 = 0;        //state of the ADST state machine
uint8_t Midi_currNote1 = 0xff;      //currently playing note, only used when the note is switched off again, and not used to dictate the frequency
uint16_t Midi_currReqNoteOT1 = 0;   //the on time requested by the controller
uint16_t Midi_lastReqNoteOT1 = 0;   //the last on time requested. used when the note decays, as Midi_currReqNoteOT1 = 0 in that case and it cant be used
uint16_t Midi_currNoteOT1 = 0;      //the OT output from the ADSR loop, used by the timers 2,3 / 4,5 to control the timer one period
float Midi_currOT1Mult = 0;         //the current multiplication factor for ADSR

//Voice 2 variables
uint8_t Midi_adsrState2 = 0;
uint8_t Midi_currNote2 = 0xff;
uint16_t Midi_currReqNoteOT2 = 0;
uint16_t Midi_lastReqNoteOT2 = 0;
uint16_t Midi_currNoteOT2 = 0;
float Midi_currOT2Mult = 0;

//ADSR coefficients
uint16_t Midi_currOT;
float Midi_attacCoef = 1;
float Midi_decayCoef = 0;
float Midi_releaseCoef = 1;

//LED Time variables
uint16_t currComsLEDTime = 0;

void Midi_init()
{
    //allocate ram for programm and coild configurations
    Midi_currProgramm = malloc(sizeof(MidiProgramm));
    Midi_currOverrideProgramm = malloc(sizeof(MidiProgramm));
    Midi_currCoil = malloc(sizeof(CoilConfig));
    
    //load default values
    NVM_readProgrammConfig(Midi_currOverrideProgramm, 0xff);
    NVM_readProgrammConfig(Midi_currProgramm, 0xff);
    NVM_readCoilConfig(Midi_currCoil, 0xff);
    
    //calculate required coefficients
    Midi_setNoteOnTime(Midi_currCoil->minOnTime + ((Midi_currCoil->maxOnTime * Midi_currVolume) / 127));
    Midi_calculateADSR();
    
    //get note timers ready
    Midi_initTimers();
    
    //Do usb stuff (code mostly from the Midi device example code)
    Midi_dataHandle = NULL;
    Midi_configRxHandle = NULL;
    Midi_configTxHandle = NULL;
    USBEnableEndpoint(USB_DEVICE_AUDIO_MIDI_ENDPOINT,USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
    USBEnableEndpoint(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
    Midi_dataHandle = USBRxOnePacket(USB_DEVICE_AUDIO_MIDI_ENDPOINT,(uint8_t*)&ReceivedDataBuffer,64);
    Midi_configRxHandle = USBRxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT,(uint8_t*)&ConfigReceivedDataBuffer,64);
}

//As long as the USB device is not suspended this function will be called every ms
void Midi_SOFHandler(){
    
    //Do adsr calculations 
    switch(Midi_adsrState1){
        case NOTE_OFF:
            Midi_currNoteOT1 = 0;
            if(Midi_currReqNoteOT1 == 0) break;
            
            Midi_adsrState1 = ATTAC;    //note is on -> proceed to attac
            Midi_currOT1Mult = 0.001;   //reset the ADSR multiplier. if we don't do this, the ADSR will resume from the last set point (if a note was still at 100% while it was overwritten, no attac would take place)
                                        //the mult != 0 requirement is because the attac is exponential. So multiplying by zero would make it not work
        case ATTAC:
            if(Midi_currReqNoteOT1 > 0){
                Midi_currOT1Mult *= Midi_attacCoef;
                if(Midi_currOT1Mult > 1 || Midi_attacCoef == 1){    //note attac has finished -> proceed to decay
                    Midi_currOT1Mult = 1;
                    Midi_currNoteOT1 = Midi_currCoil->minOnTime + Midi_currReqNoteOT1;
                    Midi_adsrState1 = DECAY;
                }else{
                    Midi_currNoteOT1 = Midi_currCoil->minOnTime + (uint16_t) ((float) Midi_currReqNoteOT1 * Midi_currOT1Mult);  //the requested on time is always what needs to be added to the minimum on time settings of the current coil
                }
                break;
            }else{  //note is off again -> fall through to release
                Midi_adsrState1 = RELEASE;
            }
        case DECAY:
            if(Midi_currReqNoteOT1 > 0){
                if(MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->sustainEnabled : Midi_currProgramm->sustainEnabled){
                    Midi_currOT1Mult *= Midi_decayCoef;
                    if((Midi_currOT1Mult * 100 < (MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->sustainPower : Midi_currProgramm->sustainPower)) || Midi_decayCoef == 1){ //sustain power level has been reached -> go to sustain
                        Midi_currOT1Mult = (float) (MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->sustainPower : Midi_currProgramm->sustainPower) / 100.0;
                        Midi_adsrState1 = SUSTAIN;
                    }
                    Midi_currNoteOT1 = Midi_currCoil->minOnTime + (uint16_t) ((float) Midi_currReqNoteOT1 * Midi_currOT1Mult);
                }
                break;
            }else{  //note is off again -> fall through to release
                Midi_adsrState1 = RELEASE;
            }
        case SUSTAIN:
            if(Midi_currReqNoteOT1 > 0){
                break;
            }else{  //note is off again -> fall through to release
                Midi_adsrState1 = RELEASE;
            }
        case RELEASE:
            if(Midi_currReqNoteOT1 > 0){    //note is on again -> continue attac
                Midi_adsrState1 = ATTAC;
                break;
            }else{
                Midi_currOT1Mult *= Midi_releaseCoef;
                Midi_currNoteOT1 = Midi_currCoil->minOnTime + (uint16_t) ((float) Midi_lastReqNoteOT1 * Midi_currOT1Mult);
                if(Midi_currNoteOT1 == Midi_currCoil->minOnTime || Midi_releaseCoef == 1){  //note has decayed -> wait for beginning of next note
                    Midi_currOT1Mult = 0;
                    Midi_currNoteOT1 = 0;
                    Midi_adsrState1 = NOTE_OFF;
                }
                break;
            }
    }
    
    switch(Midi_adsrState2){
        case 0:
            Midi_currNoteOT2 = 0;
            if(Midi_currReqNoteOT2 == 0) break;
            
            Midi_adsrState2 = ATTAC;    //note is on -> proceed to attac
            Midi_currOT2Mult = 0.001;   //0*attacCoefficeient is still zero
        case 1:
            if(Midi_currReqNoteOT2 > 0){
                Midi_currOT2Mult *= Midi_attacCoef;
                if(Midi_currOT2Mult > 1 || Midi_attacCoef == 1){    //note attac has finished -> proceed to decay
                    Midi_currOT2Mult = 1;
                    Midi_currNoteOT2 = Midi_currCoil->minOnTime + Midi_currReqNoteOT2;
                    Midi_adsrState2 = DECAY;
                }else{
                    Midi_currNoteOT2 = Midi_currCoil->minOnTime + (uint16_t) ((float) Midi_currReqNoteOT2 * Midi_currOT2Mult);
                }
                break;
            }else{  //note is off again -> fall through to release
                Midi_adsrState2 = RELEASE;
            }
        case DECAY:
            if(Midi_currReqNoteOT2 > 0){
                if(MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->sustainEnabled : Midi_currProgramm->sustainEnabled){
                    Midi_currOT2Mult *= Midi_decayCoef;
                    if((Midi_currOT2Mult * 100 < (MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->sustainPower : Midi_currProgramm->sustainPower)) || Midi_decayCoef == 1){ //sustain power level has been reached -> go to sustain
                        Midi_currOT2Mult = (float) (MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->sustainPower : Midi_currProgramm->sustainPower) / 100.0;
                        Midi_adsrState2 = SUSTAIN;
                    }
                    Midi_currNoteOT2 = Midi_currCoil->minOnTime + (uint16_t) ((float) Midi_currReqNoteOT2 * Midi_currOT2Mult);
                }
                break;
            }else{  //note is off again -> fall through to release
                Midi_adsrState2 = RELEASE;
            }
        case 3:
            if(Midi_currReqNoteOT2 > 0){
                break;
            }else{  //note is off again -> fall through to release
                Midi_adsrState2 = RELEASE;
            }
        case RELEASE:
            if(Midi_currReqNoteOT2 > 0){    //note is on again -> continue attac
                Midi_adsrState2 = ATTAC;
                break;
            }else{
                Midi_currOT2Mult *= Midi_releaseCoef;
                Midi_currNoteOT2 = Midi_currCoil->minOnTime + (uint16_t) ((float) Midi_lastReqNoteOT2 * Midi_currOT2Mult);
                if(Midi_currNoteOT2 == Midi_currCoil->minOnTime || Midi_releaseCoef == 1){  //note has decayed -> wait for beginning of next note
                    Midi_currOT2Mult = 0;
                    Midi_currNoteOT2 = 0;
                    Midi_adsrState2 = 0;
                }
                break;
            }
    }
    
    //Turn the LED on if any voices are active
    if(Midi_currNoteOT2 > 0 || Midi_currNoteOT1 > 0){   //enable signal LED
        LATBCLR = _LATB_LATB7_MASK;
    }else{                      //disable signal LED
        LATBSET = _LATB_LATB7_MASK;
    }
    
    //count down comms led on time, to make it turn off if no data was received for 100ms
    if(currComsLEDTime > 0){
        currComsLEDTime --;
        if(currComsLEDTime == 0){
            LATBSET = _LATB_LATB9_MASK;
        }
    }
}

void Midi_run(){
    /* If the device is not configured yet, or the device is suspended, then
     * we don't need to run the demo since we can't send any data.
     */
    if( (USBGetDeviceState() < CONFIGURED_STATE) || USBIsDeviceSuspended() == true) {
        return;
    }
    
    if(!USBHandleBusy(Midi_dataHandle)){
        //a new data packet was received
        uint8_t cmd = 0;
        if(((ReceivedDataBuffer[1] & 0xf) ^ Midi_currChannel) == 0 && (ReceivedDataBuffer[0] & 0xf0) == 0){    //our channel is addressed and the correct cable is selected
            LATBINV = _LATB_LATB9_MASK; //blink coms led
            currComsLEDTime = 100;

            cmd = ReceivedDataBuffer[1] & 0xf0;
            if(cmd == MIDI_CMD_NOTE_OFF){
                if(Midi_currNote1 == ReceivedDataBuffer[2]){
                    Midi_currNote1 = 0xff;
                    Midi_lastReqNoteOT1 = Midi_currReqNoteOT1;
                    Midi_currReqNoteOT1 = 0;
                }
                if(Midi_currNote2 == ReceivedDataBuffer[2]){
                    Midi_currNote2 = 0xff;
                    Midi_lastReqNoteOT2 = Midi_currReqNoteOT2;
                    Midi_currReqNoteOT2 = 0;
                }
            }else if(cmd == MIDI_CMD_NOTE_ON){
                if(ReceivedDataBuffer[3] > 0){  //velocity is > 0 -> turn note on
                    //decide which voice should play the new note
                    if((Midi_adsrState1 == NOTE_OFF && ReceivedDataBuffer[2] != Midi_currNote2 && Midi_currReqNoteOT1 == 0) || ReceivedDataBuffer[2] == Midi_currNote1){     
                        Midi_setNote(1, ReceivedDataBuffer[2], ReceivedDataBuffer[3]);
                    }else if(Midi_adsrState2 == NOTE_OFF || Midi_currReqNoteOT2 == 0){         
                        Midi_setNote(2, ReceivedDataBuffer[2], ReceivedDataBuffer[3]);
                    }else{  // || (Midi_adsrState1 > Midi_adsrState2 && )
                        if(ReceivedDataBuffer[2] > Midi_currNote1 || ReceivedDataBuffer[2] > Midi_currNote2){
                            if(Midi_currReqNoteOT1 == 0){         //note is decaying, so overwrite it
                                Midi_setNote(1, ReceivedDataBuffer[2], ReceivedDataBuffer[3]);
                            }else if(Midi_currReqNoteOT2 == 0){         //note is decaying, so overwrite it   
                                Midi_setNote(2, ReceivedDataBuffer[2], ReceivedDataBuffer[3]);
                            }else if(Midi_currNote1 > Midi_currNote2){          
                                Midi_setNote(2, ReceivedDataBuffer[2], ReceivedDataBuffer[3]);
                            }else{
                                Midi_setNote(1, ReceivedDataBuffer[2], ReceivedDataBuffer[3]);
                            }
                        }
                    }
                }else{  //velocity is == 0 -> turn note off (some software uses this instead of the note off command)
                    if(Midi_currNote1 == ReceivedDataBuffer[2]){
                    Midi_currNote1 = 0xff;
                    Midi_lastReqNoteOT1 = Midi_currReqNoteOT1;
                    Midi_currReqNoteOT1 = 0;
                    }
                    if(Midi_currNote2 == ReceivedDataBuffer[2]){
                        Midi_currNote2 = 0xff;
                        Midi_lastReqNoteOT2 = Midi_currReqNoteOT2;
                        Midi_currReqNoteOT2 = 0;
                    }
                }
                
            }else if(cmd == MIDI_CMD_CONTROLLER_CHANGE){
                if(ReceivedDataBuffer[2] == MIDI_CC_ALL_SOUND_OFF || ReceivedDataBuffer[2] == MIDI_CC_ALL_NOTES_OFF || ReceivedDataBuffer[2] == MIDI_CC_RESET_ALL_CONTROLLERS){ //Midi panic, and sometimes used by programms when you press the "stop" button
                    Midi_currNote2 = 0xff;
                    Midi_setNote2TPR(0);
                    Midi_currNote1 = 0xff;
                    Midi_setNote1TPR(0);
                    Midi_currBendFactor = 1;
                    Midi_currReqNoteOT1 = 0;
                    Midi_currReqNoteOT2 = 0;
                }else if(ReceivedDataBuffer[2] == MIDI_CC_VOLUME){
                    Midi_currVolume = ReceivedDataBuffer[3];
                    Midi_setNoteOnTime(Midi_currCoil->minOnTime + ((Midi_currCoil->maxOnTime * Midi_currVolume) / 127));
                }
                    
            }else if(cmd == MIDI_CMD_PITCH_BEND){
                //calculate the current bend factor as 2^(bend_range * requested_bend)
                Midi_currBendFactor = pow(2, ((double) (((ReceivedDataBuffer[3] << 7 ) | ReceivedDataBuffer[2]) - 8192) * (MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->bendRange : Midi_currProgramm->bendRange)) / 8192.0);
                if(Midi_adsrState1 != NOTE_OFF) Midi_setNote1TPR(Midi_NoteFreq[Midi_currNote1]);    //update notes if any are playing
                if(Midi_adsrState2 != NOTE_OFF) Midi_setNote2TPR(Midi_NoteFreq[Midi_currNote2]);
            }else if(cmd == MIDI_CMD_PROGRAM_CHANGE){
                NVM_readProgrammConfig(Midi_currProgramm, ReceivedDataBuffer[2]);
                Midi_calculateADSR();   //recalculate coefficients
            }
        }

        //Get ready for next packet (this will overwrite the old data). thanks microchip, very cool
        Midi_dataHandle = USBRxOnePacket(USB_DEVICE_AUDIO_MIDI_ENDPOINT,(uint8_t*)&ReceivedDataBuffer,64);
    }
    
    if(!USBHandleBusy(Midi_configRxHandle))
    {
        //New data is available at the configuration endpoint
        //TODO create structs for the data packets, so we don't have to do this ugly stuff
        if(ConfigReceivedDataBuffer[0] == USB_CMD_LIVE_PREV){
            LATBbits.LATB8 = ConfigReceivedDataBuffer[1];
            MIDI_programmOverrideEnabled = ConfigReceivedDataBuffer[1];
        }else if(ConfigReceivedDataBuffer[0] == USB_CMD_SET_ADSR_PREV){
            Midi_currOverrideProgramm->noteOffset = ConfigReceivedDataBuffer[1];
            Midi_currOverrideProgramm->bendRange = ConfigReceivedDataBuffer[2];
            Midi_currOverrideProgramm->AttacTime = (ConfigReceivedDataBuffer[6] << 24) | (ConfigReceivedDataBuffer[5] << 16) | (ConfigReceivedDataBuffer[4] << 8) | ConfigReceivedDataBuffer[3];
            Midi_currOverrideProgramm->DecayTime = (ConfigReceivedDataBuffer[10] << 24) | (ConfigReceivedDataBuffer[9] << 16) | (ConfigReceivedDataBuffer[8] << 8) | ConfigReceivedDataBuffer[7];   //TODO change to release
            Midi_currOverrideProgramm->releaseTime = (ConfigReceivedDataBuffer[14] << 24) | (ConfigReceivedDataBuffer[13] << 16) | (ConfigReceivedDataBuffer[12] << 8) | ConfigReceivedDataBuffer[11];   //TODO change to release
            Midi_currOverrideProgramm->sustainPower = ConfigReceivedDataBuffer[15];
            Midi_currOverrideProgramm->sustainEnabled = ConfigReceivedDataBuffer[16];
            
        }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_PROGRAMM){
            MidiProgramm requested;
            if(NVM_readProgrammConfig(&requested, ConfigReceivedDataBuffer[1])){    //returns zero if voice is not from flash
                ToSendDataBuffer[0] = 0x20;
                ToSendDataBuffer[1] = ConfigReceivedDataBuffer[1];
                ToSendDataBuffer[2] = requested.noteOffset;
                ToSendDataBuffer[3] = requested.bendRange;
                ToSendDataBuffer[7] = (requested.AttacTime >> 24);
                ToSendDataBuffer[6] = (requested.AttacTime >> 16);
                ToSendDataBuffer[5] = (requested.AttacTime >> 8);
                ToSendDataBuffer[4] = (requested.AttacTime);
                ToSendDataBuffer[11] = (requested.DecayTime >> 24);
                ToSendDataBuffer[10] = (requested.DecayTime >> 16);
                ToSendDataBuffer[9] =  (requested.DecayTime >> 8);
                ToSendDataBuffer[8] =  (requested.DecayTime);
                ToSendDataBuffer[15] = (requested.releaseTime >> 24);
                ToSendDataBuffer[14] = (requested.releaseTime >> 16);
                ToSendDataBuffer[13] = (requested.releaseTime >> 8);
                ToSendDataBuffer[12] = (requested.releaseTime);
                ToSendDataBuffer[16] = requested.sustainPower;   //sustain level
                ToSendDataBuffer[17] = requested.sustainEnabled;   //sustain level
                memcpy(&ToSendDataBuffer[32], requested.name, 24);
            }else{
                memset(ToSendDataBuffer, 0, 64);
            }
            Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
            
        }else if(ConfigReceivedDataBuffer[0] == USB_CMD_SET_PROGRAMM){
            MidiProgramm * newData = malloc(sizeof(MidiProgramm));
            
            newData->noteOffset = ConfigReceivedDataBuffer[2];
            newData->bendRange = (double) ConfigReceivedDataBuffer[3];
            newData->AttacTime = (ConfigReceivedDataBuffer[7] << 24) | (ConfigReceivedDataBuffer[6] << 16) | (ConfigReceivedDataBuffer[5] << 8) | ConfigReceivedDataBuffer[4];
            newData->DecayTime = (ConfigReceivedDataBuffer[11] << 24) | (ConfigReceivedDataBuffer[10] << 16) | (ConfigReceivedDataBuffer[9] << 8) | ConfigReceivedDataBuffer[8];   //TODO change to release
            newData->releaseTime = (ConfigReceivedDataBuffer[15] << 24) | (ConfigReceivedDataBuffer[14] << 16) | (ConfigReceivedDataBuffer[13] << 8) | ConfigReceivedDataBuffer[12];   //TODO change to release
            newData->sustainPower = ConfigReceivedDataBuffer[16];
            newData->sustainEnabled = ConfigReceivedDataBuffer[17];
            memcpy(newData->name, &ConfigReceivedDataBuffer[31], 24);
            
            NVM_writeProgrammConfig(newData, (uint8_t) ConfigReceivedDataBuffer[1]);
            
            free(newData);
            
        }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_COIL_CONFIG){
            CoilConfig requested;
            if(NVM_readCoilConfig(&requested, ConfigReceivedDataBuffer[1])){    //returns zero if voice is not from flash
                ToSendDataBuffer[0] = 0x22;
                ToSendDataBuffer[1] = ConfigReceivedDataBuffer[1];
                ToSendDataBuffer[5] = (requested.maxOnTime >> 24);
                ToSendDataBuffer[4] = (requested.maxOnTime >> 16);
                ToSendDataBuffer[3] = (requested.maxOnTime >> 8);
                ToSendDataBuffer[2] = requested.maxOnTime;
                ToSendDataBuffer[9] = (requested.minOnTime >> 24);
                ToSendDataBuffer[8] = (requested.minOnTime >> 16);
                ToSendDataBuffer[7] = (requested.minOnTime >> 8);
                ToSendDataBuffer[6] = (requested.minOnTime);
                ToSendDataBuffer[10] = requested.maxDuty;
                ToSendDataBuffer[11] = requested.limiterMode;
                memcpy(&ToSendDataBuffer[32], requested.name, 24);
            }else{
                memset(ToSendDataBuffer, 0, 64);
            }
            Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
            
        }else if(ConfigReceivedDataBuffer[0] == USB_CMD_SET_COIL_CONFIG){
            CoilConfig * newData = malloc(sizeof(CoilConfig));
            
            newData->maxOnTime = (ConfigReceivedDataBuffer[5] << 24) | (ConfigReceivedDataBuffer[4] << 16) | (ConfigReceivedDataBuffer[3] << 8) | ConfigReceivedDataBuffer[2];
            newData->minOnTime = (ConfigReceivedDataBuffer[9] << 24) | (ConfigReceivedDataBuffer[8] << 16) | (ConfigReceivedDataBuffer[7] << 8) | ConfigReceivedDataBuffer[6];
            newData->maxDuty = ConfigReceivedDataBuffer[10];
            newData->limiterMode = ConfigReceivedDataBuffer[11];
            memcpy(newData->name, &ConfigReceivedDataBuffer[31], 24);
            
            NVM_writeCoilConfig(newData, (uint8_t) ConfigReceivedDataBuffer[1]);
            
            free(newData);
            
        }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_DEVICE_NAME){
            ToSendDataBuffer[0] = USB_CMD_GET_DEVICE_NAME;
            memcpy(&ToSendDataBuffer[1], NVM_getDeviceName(), 24);
            Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
            
        }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_COILCONFIG_INDEX){
            ToSendDataBuffer[0] = USB_CMD_GET_COILCONFIG_INDEX;
            ToSendDataBuffer[1] = NVM_isCoilConfigValid(Midi_currCoilIndex) ? Midi_currCoilIndex : 0xff;
            Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
            
        }else if(ConfigReceivedDataBuffer[0] == USB_CMD_SET_COILCONFIG_INDEX){
            if(!Midi_isNotePlaing()){
                Midi_currCoilIndex = ConfigReceivedDataBuffer[1];
                NVM_readCoilConfig(Midi_currCoil, Midi_currCoilIndex);
                Midi_setNoteOnTime(Midi_currCoil->minOnTime + ((Midi_currCoil->maxOnTime * Midi_currVolume) / 127));
            }
            ToSendDataBuffer[0] = USB_CMD_SET_COILCONFIG_INDEX;
            ToSendDataBuffer[1] = NVM_isCoilConfigValid(Midi_currCoilIndex) ? Midi_currCoilIndex : 0xff;
            Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
            
        }else if(ConfigReceivedDataBuffer[0] == USB_CMD_PING){
            //the software checked the connection... Do nothing
        }
        
        Midi_configRxHandle = USBRxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT,(uint8_t*)&ConfigReceivedDataBuffer,64);
        Midi_calculateADSR();   //recalculate ADSR coefficients, as it is likely some changed
    }
}

void Midi_setNote(uint8_t voice, uint8_t note, uint8_t velocity){
    int16_t currNote = note + (int8_t) (MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->noteOffset : Midi_currProgramm->noteOffset);
    if(currNote > 127 || currNote < 0) return;
    uint16_t currNoteFreq = (uint16_t) ((double) Midi_NoteFreq[currNote] * Midi_currBendFactor);    //get the required note frequency, including any bender offset
    
    if((Midi_getNoteOnTime() * currNoteFreq) / 10000 > Midi_currCoil->maxDuty) return;  //don't change the note if the duty cycle would be > maxDuty. Duty = onTime * frequency / ({time units coefficient, here uS -> 10^6} / 100)
    
    if(voice == 1){  
        Midi_currNote1 = note;
        Midi_adsrState1 = NOTE_OFF;    //reset the ADSR state to note off, so attac will work, even if the note was still on
        Midi_setNote1TPR(currNoteFreq);
        Midi_currReqNoteOT1 = Midi_currOT - Midi_currCoil->minOnTime;      //currReqNoteOT dictates the on time, added to the minimum setting
    }else if(voice == 2){
        Midi_currNote2 = note;
        Midi_adsrState2 = NOTE_OFF;
        Midi_setNote2TPR(currNoteFreq);
        Midi_currReqNoteOT2 = Midi_currOT - Midi_currCoil->minOnTime;
    }
    //TODO implement velocity based ADSR and volume
}

void Midi_calculateADSR(){
    if(((MIDI_programmOverrideEnabled) ? Midi_currOverrideProgramm->AttacTime : Midi_currProgramm->AttacTime) != 0){
        //Attac time is the doubling time. So attacCoefficient = nthRoot(2) ; with n = attacTime. (nthRoot(2) = 2^(^/n))
        Midi_attacCoef = (float) pow(2.0, 1.0 / (double) ((MIDI_programmOverrideEnabled) ? Midi_currOverrideProgramm->AttacTime : Midi_currProgramm->AttacTime));
    }else{
        //if a time setting is zero, we cant use the calculation (because of 1/0), so we just set the coefficient to one. (The ADSR routine interprets this as no time for the corresponding parameter)
        Midi_attacCoef = 1;
    }
    if(((MIDI_programmOverrideEnabled) ? Midi_currOverrideProgramm->DecayTime : Midi_currProgramm->DecayTime) != 0){
        //Decay time is the HalfLife. So attacCoefficient = nthRoot(0.5) ; with n = decayTime. (nthRoot(0.5) = 0.5^(^/n))
        Midi_decayCoef = (float) pow(0.5, 1.0 / (double) ((MIDI_programmOverrideEnabled) ? Midi_currOverrideProgramm->DecayTime : Midi_currProgramm->DecayTime));
    }else{
        Midi_decayCoef = 1;
    }
    if(((MIDI_programmOverrideEnabled) ? Midi_currOverrideProgramm->releaseTime : Midi_currProgramm->releaseTime) != 0){
        //same as decay
        Midi_releaseCoef = (float) pow(0.5, 1.0 / (double) ((MIDI_programmOverrideEnabled) ? Midi_currOverrideProgramm->releaseTime : Midi_currProgramm->releaseTime));
    }else{
        Midi_releaseCoef = 1;
    }
}

unsigned Midi_isNotePlaing(){
    return (Midi_adsrState1 != NOTE_OFF) && (Midi_adsrState2 != NOTE_OFF);
}

void Midi_initTimers(){
    //maybe I should have writte nthe timer speeds here when i calculated them... -.- I'll do that when i do so again
    T1CON = 0b00100000;
    IEC0bits.T1IE = 1;
    IPC1bits.T1IP = 7;
    T1CONbits.ON = 1;
    
    //Note one timer (F = 48MHz), because we can 
    T2CON = 0b00001000;
    IEC0bits.T3IE = 1;
    IPC3bits.T3IP = 7;
    T2CONbits.ON = 1;
    
    //Note two timer (F = 48MHz)
    T4CON = 0b00001000;
    IEC0bits.T5IE = 1;
    IPC5bits.T5IP = 7;
    T4CONbits.ON = 1;
}

void Midi_setNote1TPR(uint16_t freq){
    if(freq == 0){
        T2CONCLR = _T2CON_ON_MASK;
        return;
    }else{
        T2CONSET = _T2CON_ON_MASK;
    }
    uint32_t divVal = 48000000/freq;
    PR2 = divVal;
    if(TMR2 > PR2) TMR2 = 0;
}

void Midi_setNote2TPR(uint16_t freq){
    if(freq == 0){
        T4CONCLR = _T4CON_ON_MASK;
        return;
    }else{
        T4CONSET = _T4CON_ON_MASK;
    }
    uint32_t divVal = 48000000 / freq;
    PR4 = divVal;
    if(TMR4 > PR4) TMR4 = 0;
}

void Midi_setNoteOnTime(uint16_t onTimeUs){
    if(onTimeUs > Midi_currCoil->maxOnTime || onTimeUs < Midi_currCoil->minOnTime) return;
    //Convert the requested time in us, to a timer divider value
    Midi_currOT = (onTimeUs * 100) / 133;
}

uint16_t Midi_getNoteOnTime(){
    //Convert the timer divider value, to the time in us
    return (Midi_currOT * 133) / 100;
}

/*
 * How the notes are played:
 * 
 *  The timer 2,3 & 4,5 are overflowing at the note frequency, 
 *  once the interrupt triggers, we reset the counter for Timer 1 to zero, enable it and turn the output on
 *  once timer 1 overflows, we turn the output off again and disable the timer again (effectively one shot mode)
 * 
 */

void __ISR(_TIMER_5_VECTOR) Midi_note2ISR(){
    IFS0bits.T5IF = 0;
    if(Midi_currNoteOT2 == 0) return;
    T1CONSET = _T1CON_ON_MASK;
    TMR1 = 0;
    PR1 = Midi_currNoteOT2;
    LATBSET = (Midi_currNoteOT2 > 0) ? (_LATB_LATB15_MASK | _LATB_LATB5_MASK) : 0;  //RB15
}

void __ISR(_TIMER_3_VECTOR) Midi_note1ISR(){
    IFS0bits.T3IF = 0;
    if(Midi_currNoteOT1 == 0) return;
    T1CONSET = _T1CON_ON_MASK;
    TMR1 = 0;
    PR1 = Midi_currNoteOT1;
    LATBSET = (Midi_currNoteOT1 > 0) ? (_LATB_LATB15_MASK | _LATB_LATB5_MASK) : 0;  //RB15
}

void __ISR(_TIMER_1_VECTOR) Midi_noteOffISR(){
    IFS0bits.T1IF = 0;
    T1CONCLR = _T1CON_ON_MASK;
    LATBCLR = (_LATB_LATB15_MASK | _LATB_LATB5_MASK);  //RB15
}