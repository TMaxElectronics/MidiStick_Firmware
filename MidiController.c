
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <xc.h>
#include <sys/attribs.h>
#include <string.h>
#include <GenericTypeDefs.h>

#include "usb lib/usb.h"
#include "MidiController.h"
#include "ConfigManager.h"
#include "NoteManager.h"

struct{
    float bendFactor;   
    float bendRange;    
    uint16_t currOT;
    uint16_t currVolume;
    float attacCoef;
    float decayCoef;
    float releaseCoef;
    MidiProgramm currProgramm;
} ChannelInfo[16] = {[0 ... 15].bendFactor = 1, [0 ... 15].attacCoef = 1, [0 ... 15].decayCoef = 1, [0 ... 15].releaseCoef = 1};

#define MIDI_VOICECOUNT 4

SynthVoice Midi_voice[MIDI_VOICECOUNT] = {[0 ... 3].currNote = 0xff};

unsigned noteHoldoff = 0;

uint16_t halfLimit = 0;

//Programm and coil configurations
MidiProgramm * Midi_currOverrideProgramm;
CoilConfig * Midi_currCoil;
uint8_t Midi_currCoilIndex = 0xff;          //this is used by the pc software to figure out which configuration is active
uint16_t Midi_minOnTimeVal = 0;
uint16_t Midi_maxOnTimeVal = 0;

//TODO make this dynamic
//uint8_t Midi_currChannel = 0;

//USB comms stuff
static uint8_t ReceivedDataBuffer[64];          //data received from the midi endpoint
static uint8_t ConfigReceivedDataBuffer[64];    //data received from the config endpoint
static uint8_t ToSendDataBuffer[64];            //data to send to the config endpoint
static USB_HANDLE Midi_dataHandle;              //Micochip usb library handles
static USB_HANDLE Midi_configTxHandle;
static USB_HANDLE Midi_configRxHandle;

//Global midi settings
unsigned MIDI_programmOverrideEnabled = 0;  //sets wether data from Midi_currOverrideProgramm or Midi_currProgramm is used        

int32_t accumulatedOnTime = 0;
uint16_t resetCounter = 0;
uint16_t dutyLimiter = 0;
uint32_t noteOffTime = 1;
uint16_t holdOff = 0;

//ADSR coefficients

//LED Time variables
uint16_t currComsLEDTime = 0;

unsigned progMode = 0;
uint32_t FWUpdate_currPLOffset = 0;
uint8_t * payload = 0;
uint16_t currPage = 0;

void Midi_init()
{
    //allocate ram for programm and coild configurations
    //Midi_currProgramm = malloc(sizeof(MidiProgramm));
    Midi_currOverrideProgramm = malloc(sizeof(MidiProgramm));
    Midi_currCoil = malloc(sizeof(CoilConfig));
    NoteManager_init();
    
    //load default values
    NVM_readProgrammConfig(Midi_currOverrideProgramm, 0xff);
    
    uint8_t currChannel = 0;
    for(;currChannel < 16; currChannel++) NVM_readProgrammConfig(&ChannelInfo[currChannel].currProgramm, 0xff);
    NVM_readCoilConfig(Midi_currCoil, 0xff);
    //dutyLimiter = (Midi_currCoil->maxDuty * 4000) / 133;
    holdOff = (Midi_currCoil->holdoffTime * 100) / 133;
    halfLimit = Midi_currCoil->ontimeLimit / 2;
    
    //calculate required coefficients
    //Midi_setNoteOnTime(Midi_currCoil->minOnTime + ((Midi_currCoil->maxOnTime * Midi_currVolume) / 127), 0);
    Midi_calculateADSR(16);
    
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
    
    unsigned anyNoteOn = 0;
    
    uint8_t currVoice = 0;
    for(;currVoice < MIDI_VOICECOUNT; currVoice++){
        //Do adsr calculations 
        uint8_t currChannel = Midi_voice[currVoice].currNoteOrigin;
        if(Midi_voice[currVoice].currNoteOT > 0) anyNoteOn = 1;
        if(Midi_voice[currVoice].adsrState != NOTE_OFF) Midi_voice[currVoice].noteAge++;
        
        switch(Midi_voice[currVoice].adsrState){
            case NOTE_OFF:
                Midi_voice[currVoice].currNoteOT = 0;
                if(Midi_voice[currVoice].currReqNoteOT == 0) break;

                Midi_voice[currVoice].adsrState = ATTAC;    //note is on -> proceed to attac
                Midi_voice[currVoice].currOTMult = 0.001;   //reset the ADSR multiplier. if we don't do this, the ADSR will resume from the last set point (if a note was still at 100% while it was overwritten, no attac would take place)
                                            //the mult != 0 requirement is because the attac is exponential. So multiplying by zero would make it not work
            case ATTAC:
                if(Midi_voice[currVoice].currReqNoteOT > 0){
                    Midi_voice[currVoice].currOTMult *= ChannelInfo[currChannel].attacCoef;
                    if(Midi_voice[currVoice].currOTMult > 1 || ChannelInfo[currChannel].attacCoef == 1){    //note attac has finished -> proceed to decay
                        Midi_voice[currVoice].currOTMult = 1;
                        Midi_voice[currVoice].currNoteOT = Midi_minOnTimeVal + Midi_voice[currVoice].currReqNoteOT;
                        Midi_voice[currVoice].adsrState = DECAY;
                    }else{
                        Midi_voice[currVoice].currNoteOT = Midi_minOnTimeVal + (uint16_t) ((float) Midi_voice[currVoice].currReqNoteOT * Midi_voice[currVoice].currOTMult);  //the requested on time is always what needs to be added to the minimum on time settings of the current coil
                    }
                    break;
                }else{  //note is off again -> fall through to release
                    Midi_voice[currVoice].adsrState = RELEASE;
                }
            case DECAY:
                if(Midi_voice[currVoice].currReqNoteOT > 0){
                    if(MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->sustainEnabled : ChannelInfo[currChannel].currProgramm.sustainEnabled){
                        Midi_voice[currVoice].currOTMult *= ChannelInfo[currChannel].decayCoef;
                        if((Midi_voice[currVoice].currOTMult * 100 < (MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->sustainPower : ChannelInfo[currChannel].currProgramm.sustainPower)) || ChannelInfo[currChannel].decayCoef == 1){ //sustain power level has been reached -> go to sustain
                            Midi_voice[currVoice].currOTMult = (float) (MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->sustainPower : ChannelInfo[currChannel].currProgramm.sustainPower) / 100.0;
                            Midi_voice[currVoice].adsrState = SUSTAIN;
                        }
                        Midi_voice[currVoice].currNoteOT = Midi_minOnTimeVal + (uint16_t) ((float) Midi_voice[currVoice].currReqNoteOT * Midi_voice[currVoice].currOTMult);
                    }
                    break;
                }else{  //note is off again -> fall through to release
                    Midi_voice[currVoice].adsrState = RELEASE;
                }
            case SUSTAIN:
                if(Midi_voice[currVoice].currReqNoteOT > 0){
                    break;
                }else{  //note is off again -> fall through to release
                    Midi_voice[currVoice].adsrState = RELEASE;
                }
            case RELEASE:
                if(Midi_voice[currVoice].currReqNoteOT > 0){    //note is on again -> continue attac
                    Midi_voice[currVoice].adsrState = ATTAC;
                    break;
                }else{
                    Midi_voice[currVoice].currOTMult *= ChannelInfo[currChannel].releaseCoef;
                    Midi_voice[currVoice].currNoteOT = Midi_minOnTimeVal + (uint16_t) ((float) Midi_voice[currVoice].lastReqNoteOT * Midi_voice[currVoice].currOTMult);
                    if(Midi_voice[currVoice].currNoteOT == Midi_minOnTimeVal || ChannelInfo[currChannel].releaseCoef == 1){  //note has decayed -> wait for beginning of next note
                        Midi_voice[currVoice].currOTMult = 0;
                        Midi_voice[currVoice].currNoteOT = 0;
                        Midi_voice[currVoice].adsrState = NOTE_OFF;
                    }
                    break;
                }
                break;
        }
    }
    
    LATBbits.LATB8 = accumulatedOnTime < halfLimit;
    
    //if(++resetCounter > 4){
        //resetCounter = 0;
    //}
    
    //Turn the LED on if any voices are active
    if(anyNoteOn){   //enable signal LED
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
        //if(((ReceivedDataBuffer[1] & 0xf) ^ Midi_currChannel) == 0 && (ReceivedDataBuffer[0] & 0xf0) == 0){    //our channel is addressed and the correct cable is selected
        if(!progMode){
            LATBINV = _LATB_LATB9_MASK; //blink coms led
            currComsLEDTime = 100;

            uint8_t channel = ReceivedDataBuffer[1] & 0xf;
            uint8_t cmd = ReceivedDataBuffer[1] & 0xf0;

            if(cmd == MIDI_CMD_NOTE_OFF){
                NoteManager_removeNote(ReceivedDataBuffer[2]);

                uint8_t currVoice = 0;
                for(;currVoice < MIDI_VOICECOUNT; currVoice++){
                    if(Midi_voice[currVoice].currNote == ReceivedDataBuffer[2] && Midi_voice[currVoice].currNoteOrigin == channel){
                        Midi_voice[currVoice].currNote = 0xff;
                        Midi_voice[currVoice].lastReqNoteOT = Midi_voice[currVoice].currReqNoteOT;
                        Midi_voice[currVoice].currReqNoteOT = 0;
                    }
                }
            }else if(cmd == MIDI_CMD_NOTE_ON){
                if(ReceivedDataBuffer[3] > 0){  //velocity is > 0 -> turn note on
                    NoteManager_addNote(ReceivedDataBuffer[2], ReceivedDataBuffer[3]);
                    //decide which voice should play the new note
                    uint8_t currVoice = 0;
                    unsigned skip = 0;
                    for(;currVoice < MIDI_VOICECOUNT; currVoice++){
                        if(Midi_voice[currVoice].currNote == ReceivedDataBuffer[2] && Midi_voice[currVoice].currNoteOrigin == channel){
                            Midi_setNote(currVoice, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                            skip = 1;
                            break;
                        }
                    }
                    if(!skip){
                        if(Midi_voice[0].currReqNoteOT == NOTE_OFF){         //note is decaying, so overwrite it
                            Midi_setNote(0, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                        }else if(Midi_voice[1].currReqNoteOT == NOTE_OFF){         //note is decaying, so overwrite it   
                            Midi_setNote(1, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                        }else if(Midi_voice[2].currReqNoteOT == NOTE_OFF){          
                            Midi_setNote(2, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                        }else if(Midi_voice[3].currReqNoteOT == NOTE_OFF){
                            Midi_setNote(3, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                        }else{
                            uint8_t oldestAge = Midi_voice[0].noteAge;
                            uint8_t oldestIndex = 0;
                            if(Midi_voice[1].noteAge > oldestAge){ oldestAge = Midi_voice[1].noteAge; oldestIndex = 1; }
                            if(Midi_voice[2].noteAge > oldestAge){ oldestAge = Midi_voice[2].noteAge; oldestIndex = 2; }
                            if(Midi_voice[3].noteAge > oldestAge){ oldestAge = Midi_voice[3].noteAge; oldestIndex = 3; }
                            Midi_setNote(oldestIndex, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                            Midi_voice[oldestIndex].currNoteOrigin = channel;
                        }
                    }

                }else{  //velocity is == 0 -> turn note off (some software uses this instead of the note off command)
                    NoteManager_removeNote(ReceivedDataBuffer[2]);
                    uint8_t currVoice = 0;
                    for(;currVoice < MIDI_VOICECOUNT; currVoice++){
                        if(Midi_voice[currVoice].currNote == ReceivedDataBuffer[2] && Midi_voice[currVoice].currNoteOrigin == channel){
                            Midi_voice[currVoice].currNote = 0xff;
                            Midi_voice[currVoice].lastReqNoteOT = Midi_voice[currVoice].currReqNoteOT;
                            Midi_voice[currVoice].currReqNoteOT = 0;
                        }
                    }
                }

            }else if(cmd == MIDI_CMD_CONTROLLER_CHANGE){
                if(ReceivedDataBuffer[2] == MIDI_CC_ALL_SOUND_OFF || ReceivedDataBuffer[2] == MIDI_CC_ALL_NOTES_OFF || ReceivedDataBuffer[2] == MIDI_CC_RESET_ALL_CONTROLLERS){ //Midi panic, and sometimes used by programms when you press the "stop" button
                    NoteManager_clearList();

                    uint8_t currVoice = 0;
                    for(;currVoice < MIDI_VOICECOUNT; currVoice++){
                        Midi_voice[currVoice].currNote = 0xff;
                        Midi_voice[currVoice].adsrState = NOTE_OFF;
                        Midi_voice[currVoice].lastReqNoteOT = 0;
                        Midi_voice[currVoice].currReqNoteOT = 0;
                    }

                    T2CONCLR = _T2CON_ON_MASK;
                    T3CONCLR = _T3CON_ON_MASK;
                    T4CONCLR = _T4CON_ON_MASK;
                    T5CONCLR = _T5CON_ON_MASK;

                    uint8_t currChannel = 0;
                    for(;currChannel < 16; currChannel++){
                        ChannelInfo[currChannel].bendFactor = 1;
                    }
                }else if(ReceivedDataBuffer[2] == MIDI_CC_VOLUME){
                    ChannelInfo[channel].currVolume = ReceivedDataBuffer[3];
                    Midi_setNoteOnTime(Midi_currCoil->minOnTime + ((Midi_currCoil->maxOnTime * ChannelInfo[channel].currVolume) / 127), channel);
                }

            }else if(cmd == MIDI_CMD_PITCH_BEND){
                //calculate the current bend factor as 2^(bend_range * requested_bend)
                double bendOffset = (double) (((ReceivedDataBuffer[3] << 7) | ReceivedDataBuffer[2]) - 8192) / 8192.0;
                ChannelInfo[channel].bendFactor = pow(2, (bendOffset * (double) (MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->bendRange : ChannelInfo[channel].currProgramm.bendRange)));

                uint8_t currVoice = 0;
                for(;currVoice < MIDI_VOICECOUNT; currVoice++){
                    if(Midi_voice[currVoice].currNote != 0xff){ 
                        int16_t currNote = Midi_voice[currVoice].currNote + (int8_t) (MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->noteOffset : ChannelInfo[channel].currProgramm.noteOffset);
                        uint16_t nextNoteFreq = (uint16_t) ((double) Midi_NoteFreq[currNote] * ChannelInfo[Midi_voice[currVoice].currNoteOrigin].bendFactor);
                        Midi_setNoteTPR(currVoice, nextNoteFreq); 
                    }
                }
            }else if(cmd == MIDI_CMD_PROGRAM_CHANGE){
                NVM_readProgrammConfig(&ChannelInfo[channel].currProgramm, ReceivedDataBuffer[2]);
                Midi_calculateADSR(channel);   //recalculate coefficients
            }
        }

        //Get ready for next packet (this will overwrite the old data). thanks microchip, very cool
        Midi_dataHandle = USBRxOnePacket(USB_DEVICE_AUDIO_MIDI_ENDPOINT,(uint8_t*)&ReceivedDataBuffer,64);
    }
    
    if(!USBHandleBusy(Midi_configRxHandle))
    {
        if(!progMode){
            //New data is available at the configuration endpoint
            //TODO create structs for the data packets, so we don't have to do this ugly stuff
            if(ConfigReceivedDataBuffer[0] == USB_CMD_LIVE_PREV){
                //LATBbits.LATB8 = ConfigReceivedDataBuffer[1];
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
                    ToSendDataBuffer[12] = requested.ontimeLimit >> 8;
                    ToSendDataBuffer[11] = requested.ontimeLimit;
                    ToSendDataBuffer[14] = requested.holdoffTime >> 8;
                    ToSendDataBuffer[13] = requested.holdoffTime;
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
                newData->ontimeLimit = (ConfigReceivedDataBuffer[12] << 8) | ConfigReceivedDataBuffer[11];
                newData->holdoffTime = (ConfigReceivedDataBuffer[14] << 8) | ConfigReceivedDataBuffer[13];
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
                    halfLimit = Midi_currCoil->ontimeLimit / 2;
                    holdOff = (Midi_currCoil->holdoffTime * 100) / 133;
                    uint8_t currChannel = 0;
                    for(;currChannel < 16; currChannel++){
                        Midi_setNoteOnTime(Midi_currCoil->minOnTime + ((Midi_currCoil->maxOnTime * ChannelInfo[currChannel].currVolume) / 127), currChannel);
                    }
                }
                ToSendDataBuffer[0] = USB_CMD_SET_COILCONFIG_INDEX;
                ToSendDataBuffer[1] = NVM_isCoilConfigValid(Midi_currCoilIndex) ? Midi_currCoilIndex : 0xff;
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);

            }else if(ConfigReceivedDataBuffer[0] == USB_GET_CURR_NOTES){
                memset(ToSendDataBuffer, 0, 64);
                NoteManager_dumpNotes(ToSendDataBuffer);
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_ENTER_PROGMODE){
                if(!Midi_isNotePlaing()){
                    progMode = 1;
                }
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_PING){
                //the software checked the connection... Do nothing
            }

            Midi_configRxHandle = USBRxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT,(uint8_t*)&ConfigReceivedDataBuffer,64);
            Midi_calculateADSR(16);   //recalculate ADSR coefficients, as it is likely some changed
        }else{
            if(payload != 0){ //we are in the process of receiving data packets for a firmware update
                memcpy(payload + FWUpdate_currPLOffset, ConfigReceivedDataBuffer, 64);
                FWUpdate_currPLOffset += 64;
                if(FWUpdate_currPLOffset > BYTE_PAGE_SIZE){
                    NVM_writeFWUpdate(payload, currPage);
                    free(payload);
                    payload = 0;
                    FWUpdate_currPLOffset = 0;
                }
            }else{
                if(ConfigReceivedDataBuffer[0] == USB_CMD_EXIT_PROGMODE){
                    progMode = 0;
                }else if(ConfigReceivedDataBuffer[0] == USB_CMD_FWUPDATE_ERASE){
                    NVM_eraseFWUpdate();
                }else if(ConfigReceivedDataBuffer[0] == USB_CMD_FWUPDATE_COMMIT){
                    NVM_memcpy128(0x9d020000, 0x9d010000, BYTE_PAGE_SIZE * 4);
                }else if(ConfigReceivedDataBuffer[0] == USB_CMD_FWUPDATE_START_BULK_WRITE){
                    payload = malloc(BYTE_PAGE_SIZE);
                    FWUpdate_currPLOffset = 0;
                    currPage = ConfigReceivedDataBuffer[1] | (ConfigReceivedDataBuffer[2] << 8);
                }
            }
        }
    }
}

void Midi_setNote(uint8_t voice, uint8_t note, uint8_t velocity, uint8_t channel){
    int16_t currNote = note + (int8_t) (MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->noteOffset : ChannelInfo[channel].currProgramm.noteOffset);
    if(currNote > 127 || currNote < 1) return;
    uint16_t currNoteFreq = (uint16_t) ((double) Midi_NoteFreq[currNote] * ChannelInfo[channel].bendFactor);    //get the required note frequency, including any bender offset
    
    Midi_voice[voice].currNoteOrigin = channel;
    //if((Midi_getNoteOnTime() * currNoteFreq) / 10000 > Midi_currCoil->maxDuty) return;  //don't change the note if the duty cycle would be > maxDuty. Duty = onTime * frequency / ({time units coefficient, here uS -> 10^6} / 100)
    
    Midi_setNoteTPR(voice, currNoteFreq);
    Midi_voice[voice].noteAge = 0;
    Midi_voice[voice].currNote = note;
    Midi_voice[voice].adsrState = NOTE_OFF;    //reset the ADSR state to note off, so attac will work, even if the note was still on
    Midi_voice[voice].currReqNoteOT = (ChannelInfo[Midi_voice[voice].currNoteOrigin].currOT * velocity) / 127 - Midi_minOnTimeVal;      //currReqNoteOT dictates the on time, added to the minimum setting
    //TODO implement velocity based ADSR and volume
}

void Midi_calculateADSR(uint8_t channel){
    int i = (channel > 15) ? 0 : channel;
    for(; i < 15; i++){
        if(((MIDI_programmOverrideEnabled) ? Midi_currOverrideProgramm->AttacTime : ChannelInfo[i].currProgramm.AttacTime) != 0){
            //Attac time is the doubling time. So attacCoefficient = nthRoot(2) ; with n = attacTime. (nthRoot(2) = 2^(^/n))
            ChannelInfo[i].attacCoef = (float) pow(2.0, 1.0 / (double) ((MIDI_programmOverrideEnabled) ? Midi_currOverrideProgramm->AttacTime : ChannelInfo[i].currProgramm.AttacTime));
        }else{
            //if a time setting is zero, we cant use the calculation (because of 1/0), so we just set the coefficient to one. (The ADSR routine interprets this as no time for the corresponding parameter)
            ChannelInfo[i].attacCoef = 1;
        }

        if(((MIDI_programmOverrideEnabled) ? Midi_currOverrideProgramm->DecayTime : ChannelInfo[i].currProgramm.DecayTime) != 0){
            //Decay time is the HalfLife. So attacCoefficient = nthRoot(0.5) ; with n = decayTime. (nthRoot(0.5) = 0.5^(^/n))
            ChannelInfo[i].decayCoef = (float) pow(0.5, 1.0 / (double) ((MIDI_programmOverrideEnabled) ? Midi_currOverrideProgramm->DecayTime : ChannelInfo[i].currProgramm.DecayTime));
        }else{
            ChannelInfo[i].decayCoef = 1;
        }

        if(((MIDI_programmOverrideEnabled) ? Midi_currOverrideProgramm->releaseTime : ChannelInfo[i].currProgramm.releaseTime) != 0){
            //same as decay
            ChannelInfo[i].releaseCoef = (float) pow(0.5, 1.0 / (double) ((MIDI_programmOverrideEnabled) ? Midi_currOverrideProgramm->releaseTime : ChannelInfo[i].currProgramm.releaseTime));
        }else{
            ChannelInfo[i].releaseCoef = 1;
        }
        
        if(channel <= 15) break;
    }

    Midi_maxOnTimeVal = (Midi_currCoil->maxOnTime * 100) / 133;
    Midi_minOnTimeVal = (Midi_currCoil->minOnTime * 100) / 133;
}

unsigned Midi_isNotePlaing(){
    uint8_t currVoice = 0;
    for(;currVoice < MIDI_VOICECOUNT; currVoice++){
        if(Midi_voice[currVoice].adsrState != NOTE_OFF) return 1;
    }
    return 0;
}

void Midi_initTimers(){
    //note on time timer @ 750 KHz
    T1CON = 0b00100000;
    IEC0bits.T1IE = 1;
    IPC1bits.T1IP = 7;
    T1CONbits.ON = 1;
    
    //Note one timer (F = 48MHz), because we can 
    T2CON = 0b01110000;
    IEC0bits.T2IE = 1;
    IPC2bits.T2IP = 6;
    T2CONbits.ON = 0;
    
    //Note two timer (F = 48MHz), because we can 
    T3CON = 0b01110000;
    IEC0bits.T3IE = 1;
    IPC3bits.T3IP = 6;
    T2CONbits.ON = 0;
    
    //Note three timer (F = 48MHz)
    T4CON = 0b01110000;
    IEC0bits.T4IE = 1;
    IPC4bits.T4IP = 6;
    T4CONbits.ON = 0;
    
    //Note four timer (F = 48MHz), because we can 
    T5CON = 0b01110000;
    IEC0bits.T5IE = 1;
    IPC5bits.T5IP = 6;
    T2CONbits.ON = 0;
}

void Midi_setNoteTPR(uint8_t voice, uint16_t freq){
    if(freq > 2000) return;
    uint32_t divVal = 187500 / freq;
    switch(voice){
        case 0:
            if(freq == 0){
                T2CONCLR = _T2CON_ON_MASK;
                return;
            }else{
                T2CONSET = _T2CON_ON_MASK;
            }
            PR2 = divVal & 0xffff;    //lower 16 bits of the divider are done in hardware
            if(TMR2 > PR2) TMR2 = 0;
            break;
        case 1:
            if(freq == 0){
                T3CONCLR = _T3CON_ON_MASK;
                return;
            }else{
                T3CONSET = _T3CON_ON_MASK;
            }
            PR3 = divVal & 0xffff;    //lower 16 bits of the divider are done in hardware
            if(TMR3 > PR3) TMR3 = 0;
            break;
        case 2:
            if(freq == 0){
                T4CONCLR = _T4CON_ON_MASK;
                return;
            }else{
                T4CONSET = _T4CON_ON_MASK;
            }
            PR4 = divVal & 0xffff;    //lower 16 bits of the divider are done in hardware
            if(TMR4 > PR4) TMR4 = 0;
            break;
        case 3:
            if(freq == 0){
                T5CONCLR = _T5CON_ON_MASK;
                return;
            }else{
                T5CONSET = _T5CON_ON_MASK;
            }
            PR5 = divVal & 0xffff;    //lower 16 bits of the divider are done in hardware
            if(TMR5 > PR5) TMR5 = 0;
            break;
    }
}

void Midi_setNoteOnTime(uint16_t onTimeUs, uint8_t ch){
    if(onTimeUs > Midi_currCoil->maxOnTime || onTimeUs < Midi_currCoil->minOnTime) return;
    //Convert the requested time in us, to a timer divider value
    ChannelInfo[ch].currOT = (onTimeUs * 100) / 133;
}

/*uint16_t Midi_getNoteOnTime(){
    //Convert the timer divider value, to the time in us
    return (Midi_channelOTs[Midi_voice[voice].currNoteOrigin] * 133) / 100;
}*/

/*
 * How the notes are played:
 * 
 *  The timer 2,3 & 4,5 are overflowing at the note frequency, 
 *  once the interrupt triggers, we reset the counter for Timer 1 to zero, enable it and turn the output on
 *  once timer 1 overflows, we turn the output off again and disable the timer again (effectively one shot mode)
 * 
 */

void __ISR(_TIMER_2_VECTOR) Midi_note1ISR(){
    IFS0CLR = _IFS0_T2IF_MASK;
    Midi_timerHandler(0);
}

void __ISR(_TIMER_3_VECTOR) Midi_note2ISR(){
    IFS0CLR = _IFS0_T3IF_MASK;
    Midi_timerHandler(1);
}

void __ISR(_TIMER_4_VECTOR) Midi_note3ISR(){
    IFS0CLR = _IFS0_T4IF_MASK;
    Midi_timerHandler(2);
}

void __ISR(_TIMER_5_VECTOR) Midi_note4ISR(){
    IFS0CLR = _IFS0_T5IF_MASK;
    Midi_timerHandler(3);
}

void __ISR(_TIMER_1_VECTOR) Midi_noteOffISR(){
    IFS0bits.T1IF = 0;
    LATBCLR = (_LATB_LATB15_MASK | _LATB_LATB5_MASK);  //RB15
    
    if(noteHoldoff){
        noteHoldoff = 0;
        IEC0SET = _IEC0_T2IE_MASK | _IEC0_T3IE_MASK | _IEC0_T4IE_MASK | _IEC0_T5IE_MASK;
        T1CONCLR = _T1CON_ON_MASK;
        PR1 = 0;
        TMR1 = 0;
    }else{
        noteOffTime = _CP0_GET_COUNT();     //core timer runs at 1/2 CPU freq = 24MHz, so 1 TMR count = 32 CoreTimer count
        noteHoldoff = 1;
        TMR1 = 0;
        PR1 = holdOff;
    }
}

inline void Midi_timerHandler(uint8_t voice){
    if(Midi_voice[voice].currNoteOT == 0) return;
    if(noteHoldoff) return;
    IEC0CLR = _IEC0_T2IE_MASK | _IEC0_T3IE_MASK | _IEC0_T4IE_MASK | _IEC0_T5IE_MASK;
    uint16_t newLimitTime = accumulatedOnTime + Midi_voice[voice].currNoteOT;
    int16_t nextOnTime = Midi_voice[voice].currNoteOT - ((Midi_voice[voice].currNoteOT * ((newLimitTime > halfLimit) ? (newLimitTime - halfLimit) : 0)) / halfLimit);
    
    if(noteOffTime != 0){    //Note is already on
        accumulatedOnTime -= ((_CP0_GET_COUNT() - noteOffTime) * Midi_currCoil->maxDuty) / 3200;
        if(accumulatedOnTime < 0) accumulatedOnTime = 0;
        accumulatedOnTime += nextOnTime;
        if(accumulatedOnTime > Midi_currCoil->ontimeLimit) accumulatedOnTime = Midi_currCoil->ontimeLimit;
        noteOffTime = 0;
    }
    
    T1CONSET = _T1CON_ON_MASK;
    //if(accumulatedOnTime > dutyLimiter) return;
    if(nextOnTime <= 0) return;
    PR1 = nextOnTime;
    LATBSET = _LATB_LATB15_MASK | _LATB_LATB5_MASK;  //RB15

    if(TMR1 > PR1-10) TMR1 = PR1-10;
}