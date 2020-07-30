
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
#include "UART32.h"


//per Channel ADSR coefficients and other parameters
struct{
    float bendFactor;   
    unsigned sustainPedal;
    unsigned damperPedal;
    uint16_t bendMSB;   
    uint16_t bendLSB;
    uint16_t currOT;
    uint16_t currVolume;
    float attacCoef;
    float decayCoef;
    float releaseCoef;
    MidiProgramm currProgramm;
} ChannelInfo[16] = {[0 ... 15].bendFactor = 1, [0 ... 15].attacCoef = 1, [0 ... 15].decayCoef = 1, [0 ... 15].releaseCoef = 1};

//voice parameters and variables
#define MIDI_VOICECOUNT 4
SynthVoice Midi_voice[MIDI_VOICECOUNT] = {[0 ... 3].currNote = 0xff};

//Programm and coil configurations
MidiProgramm * Midi_currOverrideProgramm;
CoilConfig * Midi_currCoil;
uint8_t Midi_currCoilIndex = 0xff;          //this is used by the pc software to figure out which configuration is active
uint16_t Midi_minOnTimeVal = 0;
uint16_t Midi_maxOnTimeVal = 0;


//USB comms stuff
static uint8_t ReceivedDataBuffer[64];          //data received from the midi endpoint
static uint8_t ConfigReceivedDataBuffer[64];    //data received from the config endpoint
static uint8_t ToSendDataBuffer[64];            //data to send to the config endpoint
static USB_HANDLE Midi_dataHandle;              //Micochip usb library handles
static USB_HANDLE Midi_configTxHandle;
static USB_HANDLE Midi_configRxHandle;

//Global midi settings
unsigned MIDI_programmOverrideEnabled = 0;  //sets wether data from Midi_currOverrideProgramm or Midi_currProgramm is used        

//variables used for the limitation of signal parameters
unsigned noteHoldoff = 0;
uint16_t halfLimit = 0;
int32_t accumulatedOnTime = 0;
uint16_t resetCounter = 0;
uint16_t dutyLimiter = 0;
uint32_t noteOffTime = 1;
uint16_t holdOff = 0;

//LED Time variables
unsigned Midi_blinkLED = 0;
uint16_t Midi_blinkScaler = 0;
uint16_t Midi_currComsLEDTime = 0;

unsigned progMode = 0;
uint32_t FWUpdate_currPLOffset = 0;
uint8_t * payload = 0;
uint16_t currPage = 0;

uint16_t Midi_currRPN = 0xffff;
unsigned Midi_safe = 1;

void Midi_init(){
    
    //allocate ram for programm and coild configurations
    //Midi_currProgramm = malloc(sizeof(MidiProgramm));
    Midi_currOverrideProgramm = malloc(sizeof(MidiProgramm));
    Midi_currCoil = malloc(sizeof(CoilConfig));
    
    /*
     * What the hell is a NoteManager supposed to do??
     * 
     * Well, I am still experimenting with a proper decision algorithm to select the notes to be played
     * The notemanager keeps track of all the notes that are supposed to be playing at the moment
     * 
     * It is not used right now, so technically I could remove it, but it is a very hand tool to check if midi files exceed the available number of voices, and doesn't really
     * use too much CPU time, so I'll leave it here until it causes trouble :D
     */
    NoteManager_init();
    
    //load default values
    NVM_readProgrammConfig(Midi_currOverrideProgramm, 0xff);
    
    uint8_t currChannel = 0;
    for(;currChannel < 16; currChannel++) NVM_readProgrammConfig(&ChannelInfo[currChannel].currProgramm, 0xff);
    NVM_readCoilConfig(Midi_currCoil, 0xff);
    
    //calculate required coefficients
    //Midi_setNoteOnTime(Midi_currCoil->minOnTime + ((Midi_currCoil->maxOnTime * Midi_currVolume) / 127), 0);
    Midi_calculateADSR(16);
    holdOff = (Midi_currCoil->holdoffTime * 100) / 133;
    halfLimit = Midi_currCoil->ontimeLimit / 2;
    
    //random s(l)ee(t), used for the noise source
    srand(1337);
    
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
    
    //enable the power LED
    Midi_LED(LED_POWER, LED_ON);
}

//As long as the USB device is not suspended this function will be called every 1 ms
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
                        Midi_voice[currVoice].currNoteOT = Midi_minOnTimeVal + (uint16_t) ((float) Midi_voice[currVoice].lastReqNoteOT * Midi_voice[currVoice].currOTMult);
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
                if(ChannelInfo[currChannel].sustainPedal) break;
                
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
    
    //turn on the duty limiter indicator, if it is on
    Midi_LED(LED_DUTY_LIMITER, (accumulatedOnTime < halfLimit) ? LED_OFF : LED_ON);
    
    
    //Turn the note on LED on if any voices are active, and the E_STOP is closed (or disabled)
    Midi_LED(LED_OUT_ON, anyNoteOn & NVM_getConfig()->auxMode != AUX_E_STOP || (PORTB & _PORTB_RB5_MASK));
    
    //count down comms led on time, to make it turn off if no data was received for a while
    if(Midi_currComsLEDTime > 0){
        Midi_currComsLEDTime --;
        if(Midi_currComsLEDTime == 0){
            Midi_LED(LED_DATA, LED_OFF);
        }
    }
    
    if(Midi_blinkLED){
        if(++Midi_blinkScaler > 500){
            LATBINV = _LATB_LATB8_MASK;
            Midi_blinkScaler = 0;
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
    
    //a new Midi data packet was received
    if(!USBHandleBusy(Midi_dataHandle)){
        if(!progMode){
            Midi_LED(LED_DATA, LED_BLINK);
            Midi_currComsLEDTime = 100;
            
            //prevent config data form being written, as it causes audio glitches
            Midi_safe = 0;

            uint8_t channel = ReceivedDataBuffer[1] & 0xf;
            uint8_t cmd = ReceivedDataBuffer[1] & 0xf0;

            if(cmd == MIDI_CMD_NOTE_OFF){
                //remove the note from the list
                NoteManager_removeNote(ReceivedDataBuffer[2]);
                
                //step through all voices and check if the note that was just switched off is playing on any, if it is and the channel matches, we switch it off
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
                    //add note to the list with its velocity
                    NoteManager_addNote(ReceivedDataBuffer[2], ReceivedDataBuffer[3]);
                    
                    //decide which voice should play the new note
                    uint8_t currVoice = 0;
                    unsigned skip = 0;
                    //if a voice is already playing the note, we can skip it. But we do need to reset its adsr
                    for(;currVoice < MIDI_VOICECOUNT; currVoice++){
                        if(Midi_voice[currVoice].currNote == ReceivedDataBuffer[2] && Midi_voice[currVoice].currNoteOrigin == channel){
                            Midi_setNote(currVoice, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                            skip = 1;
                            break;
                        }
                    }
                    
                    //if we need to use a new voice, use the first off one we find. If there are none, we overwrite the oldest note
                    if(!skip){
                        if(Midi_voice[0].currReqNoteOT == NOTE_OFF){
                            Midi_setNote(0, ReceivedDataBuffer[2], ReceivedDataBuffer[3], channel);
                        }else if(Midi_voice[1].currReqNoteOT == NOTE_OFF){ 
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

                    //step through all voices and switch them off
                    uint8_t currVoice = 0;
                    for(;currVoice < MIDI_VOICECOUNT; currVoice++){
                        Midi_voice[currVoice].currNote = 0xff;
                        Midi_voice[currVoice].adsrState = NOTE_OFF;
                        Midi_voice[currVoice].lastReqNoteOT = 0;
                        Midi_voice[currVoice].currReqNoteOT = 0;
                    }

                    //stop the timers
                    T2CONCLR = _T2CON_ON_MASK;
                    T3CONCLR = _T3CON_ON_MASK;
                    T4CONCLR = _T4CON_ON_MASK;
                    T5CONCLR = _T5CON_ON_MASK;
                    
                    //reset all bend factors
                    uint8_t currChannel = 0;
                    for(;currChannel < 16; currChannel++){
                        ChannelInfo[currChannel].bendFactor = 1;
                    }
                    
                    //enable NVM operations again
                    Midi_safe = 1;
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_VOLUME){
                    ChannelInfo[channel].currVolume = ReceivedDataBuffer[3];
                    Midi_setNoteOnTime(Midi_currCoil->minOnTime + ((Midi_currCoil->maxOnTime * ChannelInfo[channel].currVolume) / 127), channel);
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_RPN_LSB){             //we are getting the address for the next RPN, remember it until the data comes in
                    Midi_currRPN &= 0xff00;
                    Midi_currRPN |= ReceivedDataBuffer[3];
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_RPN_MSB){
                    Midi_currRPN &= 0x00ff;
                    Midi_currRPN |= ReceivedDataBuffer[3] << 8;
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_DATA_FINE){           //RPN data -> check the address we received before, to select which parameter to change
                    if(Midi_currRPN == MIDI_RPN_BENDRANGE){     //get the 100ths semitones for the bendrange
                        ChannelInfo[channel].bendLSB = ReceivedDataBuffer[3];
                        ChannelInfo[channel].currProgramm.bendRange = (float) ChannelInfo[channel].bendMSB + (float) ChannelInfo[channel].bendLSB / 100.0;
                    }
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_DATA_COARSE){         //RPN data -> check the address we received before, to select which parameter to change
                    if(Midi_currRPN == MIDI_RPN_BENDRANGE){     //get the full semitones for the bendrange
                        ChannelInfo[channel].bendMSB = ReceivedDataBuffer[3];
                        ChannelInfo[channel].currProgramm.bendRange = (float) ChannelInfo[channel].bendMSB + (float) ChannelInfo[channel].bendLSB / 100.0;
                    }
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_NRPN_LSB || ReceivedDataBuffer[2] == MIDI_CC_NRPN_MSB){   //we don't currently support any NRPNs, but we need to make sure, that we don't confuse the data sent for them as being a RPN, so reset the current RPN address
                    Midi_currRPN = 0xffff;
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_SUSTAIN_PEDAL){
                    ChannelInfo[channel].sustainPedal = ReceivedDataBuffer[3] > 63;
                    
                }else if(ReceivedDataBuffer[2] == MIDI_CC_DAMPER_PEDAL){
                    ChannelInfo[channel].damperPedal = ReceivedDataBuffer[3] > 63;
                }


            }else if(cmd == MIDI_CMD_PITCH_BEND){
                //calculate the factor, by which we will bend the note
                float bendOffset = (float) (((ReceivedDataBuffer[3] << 7) | ReceivedDataBuffer[2]) - 8192) / 8192.0;
                ChannelInfo[channel].bendFactor = powf(2, (bendOffset * (float) (MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->bendRange : ChannelInfo[channel].currProgramm.bendRange)) / 12.0);

                //apply the new bending to all voices, that are playing a not from the current channel
                uint8_t currVoice = 0;
                for(;currVoice < MIDI_VOICECOUNT; currVoice++){
                    if(Midi_voice[currVoice].currNote != 0xff){ 
                        int16_t currNote = Midi_voice[currVoice].currNote + (int8_t) (MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->noteOffset : ChannelInfo[channel].currProgramm.noteOffset);
                        uint16_t nextNoteFreq = (uint16_t) ((double) Midi_NoteFreq[currNote] * ChannelInfo[Midi_voice[currVoice].currNoteOrigin].bendFactor);
                        Midi_setNoteTPR(currVoice, nextNoteFreq); 
                    }
                }
                
            }else if(cmd == MIDI_CMD_PROGRAM_CHANGE){
                //load the program from NVM
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
            //yeah this is still a TODO...
            
            if(ConfigReceivedDataBuffer[0] == USB_CMD_LIVE_PREV){
                MIDI_programmOverrideEnabled = ConfigReceivedDataBuffer[1];
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_SET_ADSR_PREV){     //sets the ADST parameters for the live preview
                Midi_currOverrideProgramm->noteOffset = ConfigReceivedDataBuffer[1];
                Midi_currOverrideProgramm->bendRange = (float) ConfigReceivedDataBuffer[2];
                Midi_currOverrideProgramm->AttacTime = (ConfigReceivedDataBuffer[6] << 24) | (ConfigReceivedDataBuffer[5] << 16) | (ConfigReceivedDataBuffer[4] << 8) | ConfigReceivedDataBuffer[3];
                Midi_currOverrideProgramm->DecayTime = (ConfigReceivedDataBuffer[10] << 24) | (ConfigReceivedDataBuffer[9] << 16) | (ConfigReceivedDataBuffer[8] << 8) | ConfigReceivedDataBuffer[7];   //TODO change to release
                Midi_currOverrideProgramm->releaseTime = (ConfigReceivedDataBuffer[14] << 24) | (ConfigReceivedDataBuffer[13] << 16) | (ConfigReceivedDataBuffer[12] << 8) | ConfigReceivedDataBuffer[11];   //TODO change to release
                Midi_currOverrideProgramm->sustainPower = ConfigReceivedDataBuffer[15];
                Midi_currOverrideProgramm->sustainEnabled = ConfigReceivedDataBuffer[16];
                Midi_calculateADSR(16);   //recalculate ADSR coefficients

            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_PROGRAMM){      //reads programm parameters
                memset(ToSendDataBuffer, 0, 64);
                MidiProgramm requested;
                if(NVM_readProgrammConfig(&requested, ConfigReceivedDataBuffer[1])){    //returns zero if the configuration at the provided index is not valid (empty)
                    ToSendDataBuffer[0] = 0x20;
                    ToSendDataBuffer[1] = ConfigReceivedDataBuffer[1];
                    ToSendDataBuffer[2] = requested.noteOffset;
                    ToSendDataBuffer[3] = (uint8_t) requested.bendRange;
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
                    ToSendDataBuffer[16] = requested.sustainPower;
                    ToSendDataBuffer[17] = requested.sustainEnabled;
                    memcpy(&ToSendDataBuffer[32], requested.name, 24);
                }
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);

            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_SET_PROGRAMM){      //write new program data
                MidiProgramm * newData = malloc(sizeof(MidiProgramm));
                    //TODO: load new data into ram if the programm is currently in use
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

            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_COIL_CONFIG){   //read coil configurations
                memset(ToSendDataBuffer, 0, 64);
                CoilConfig requested;
                if(NVM_readCoilConfig(&requested, ConfigReceivedDataBuffer[1])){    //returns zero if config at index is invalid
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
                }
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);

            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_SET_COIL_CONFIG){   //write coil configuration data
                CoilConfig * newData = malloc(sizeof(CoilConfig));

                newData->maxOnTime = (ConfigReceivedDataBuffer[5] << 24) | (ConfigReceivedDataBuffer[4] << 16) | (ConfigReceivedDataBuffer[3] << 8) | ConfigReceivedDataBuffer[2];
                newData->minOnTime = (ConfigReceivedDataBuffer[9] << 24) | (ConfigReceivedDataBuffer[8] << 16) | (ConfigReceivedDataBuffer[7] << 8) | ConfigReceivedDataBuffer[6];
                newData->maxDuty = ConfigReceivedDataBuffer[10];
                newData->ontimeLimit = (ConfigReceivedDataBuffer[12] << 8) | ConfigReceivedDataBuffer[11];
                newData->holdoffTime = (ConfigReceivedDataBuffer[14] << 8) | ConfigReceivedDataBuffer[13];
                memcpy(newData->name, &ConfigReceivedDataBuffer[31], 22);
                NVM_writeCoilConfig(newData, (uint8_t) ConfigReceivedDataBuffer[1]);

                free(newData);

            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_DEVCFG){        //read device configuration parameters
                ToSendDataBuffer[0] = USB_CMD_GET_DEVCFG;
                
                memcpy(&ToSendDataBuffer[1], NVM_getConfig(), sizeof(CFGData));
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);

            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_SET_DEVCFG){        //write device configuration parameters
                CFGData * newData = malloc(sizeof(CFGData));
                memcpy(newData, &ConfigReceivedDataBuffer[1], sizeof(CFGData));
                NVM_writeCFG(newData);
                free(newData);
                Midi_LED(LED_POWER, LED_ON);    //turn on the power LED again, as it might be a different one from before

            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_COILCONFIG_INDEX){  //read which coil preset is in use
                ToSendDataBuffer[0] = USB_CMD_GET_COILCONFIG_INDEX;
                ToSendDataBuffer[1] = NVM_isCoilConfigValid(Midi_currCoilIndex) ? Midi_currCoilIndex : 0xff;
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);

            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_SET_COILCONFIG_INDEX){  //set which coil preset is in use
                if(Midi_safe){  //only allow for changing the coil if we are not playing any music
                    Midi_currCoilIndex = ConfigReceivedDataBuffer[1];
                    NVM_readCoilConfig(Midi_currCoil, Midi_currCoilIndex);
                    halfLimit = Midi_currCoil->ontimeLimit / 2;
                    holdOff = (Midi_currCoil->holdoffTime * 100) / 133;
                    uint8_t currChannel = 0;
                    for(;currChannel < 16; currChannel++){                      //update the on time values, incase we have a different min/max from before
                        Midi_setNoteOnTime(Midi_currCoil->minOnTime + ((Midi_currCoil->maxOnTime * ChannelInfo[currChannel].currVolume) / 127), currChannel);
                    }
                    ToSendDataBuffer[0] = USB_CMD_SET_COILCONFIG_INDEX;
                    ToSendDataBuffer[1] = NVM_isCoilConfigValid(Midi_currCoilIndex) ? Midi_currCoilIndex : 0xff;
                }else{
                    ToSendDataBuffer[0] = USB_CMD_SET_COILCONFIG_INDEX;
                    ToSendDataBuffer[1] = USB_CMD_COILCONFIG_NOT_CHANGED;
                }
                
                //return either the new coil configuration index, or an error flag if we didn't change it, so the software can inform the user
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);

            }else if(ConfigReceivedDataBuffer[0] == USB_GET_CURR_NOTES){        //dump all notes from the noteManager
                memset(ToSendDataBuffer, 0, 64);
                NoteManager_dumpNotes(ToSendDataBuffer);
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_ENTER_PROGMODE){    //enter firmware update mode. In this mode only FWUPDATE commands will be processed, and any form of midi data ignored
                if(Midi_safe){
                    progMode = 1;
                }
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_CLEAR_ALL_PROGRAMMS){   //batch erase of all program presets, used for bulk writing to limit the overall re-write count
                NVM_clearAllProgramms();
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_CLEAR_ALL_COILS){   //batch erase of all coil configurations, used for bulk writing to limit the overall re-write count
                NVM_clearAllCoils();
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_FWVERSION){     //read the firmware version string
                memset(ToSendDataBuffer, 0, 64);
                memcpy(&ToSendDataBuffer[1], NVM_getFWVersionString(), 24);
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_BLVERSION){     //read the bootloader version string
                memset(ToSendDataBuffer, 0, 64);
                char * blVer = NVM_getBootloaderVersionString();
                memcpy(&ToSendDataBuffer[1], blVer, 24);
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                free(blVer);
                
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_GET_SERIAL_NR){     //read the devices serial number. this is stored in the boot flash, and will not be changed when updating the firmware
                ToSendDataBuffer[0] = USB_CMD_GET_SERIAL_NR;
                uint32_t sn = NVM_getSerialNumber();
                ToSendDataBuffer[4] = (sn >> 24);
                ToSendDataBuffer[3] = (sn >> 16);
                ToSendDataBuffer[2] = (sn >> 8);
                ToSendDataBuffer[1] = (sn);
                
                Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_BLINK){
                //we are supposed to blink an led to indentify us
                Midi_blinkLED = ConfigReceivedDataBuffer[1];
                if(!Midi_blinkLED) LATBSET =_LATB_LATB8_MASK;
            }else if(ConfigReceivedDataBuffer[0] == USB_CMD_PING){
                //called by the software every 250ms to check for device presence... no response or action required
            }

        }else{
            if(payload != 0){ //we are in the process of receiving data packets for a firmware update
                memcpy(payload + FWUpdate_currPLOffset, ConfigReceivedDataBuffer, 64);  //copy the data into the buffer at the correct location and increase the offset
                FWUpdate_currPLOffset += 64;
                
                if(FWUpdate_currPLOffset >= PAGE_SIZE){ //we have received one page of data -> write it to the NVM at the offset given by the packetcount * packet Size
                    NVM_writeFWUpdate(payload, currPage);
                    free(payload);
                    
                    //return to normal parsing of packets
                    payload = 0;
                    FWUpdate_currPLOffset = 0;
                    
                    //inform the software that we are ready for another packet, this is necessary as flash write speed is slow and not consistent enough to just use a wait in the software after writing one packet
                    ToSendDataBuffer[0] = USB_CMD_FWUPDATE_BULK_WRITE_FINISHED;
                    Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64); 
                }
            }else{
                
                if(ConfigReceivedDataBuffer[0] == USB_CMD_EXIT_PROGMODE){       //go back to normal operation
                    progMode = 0;
                    
                }else if(ConfigReceivedDataBuffer[0] == USB_CMD_FWUPDATE_ERASE){//erase any possible previous firmware updates
                    NVM_eraseFWUpdate();
                    memset(ToSendDataBuffer, 0, 64);
                    //inform the software that the erase operation is complete, this is necessary as flash erase speed is relatively slow and not consistent enough to just use a wait in the software after writing one packet
                     ToSendDataBuffer[0] = USB_CMD_FWUPDATE_ERASE;
                    Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                    
                }else if(ConfigReceivedDataBuffer[0] == USB_CMD_FWUPDATE_COMMIT){   //write the appropriate value to the fwStatus byte, to tell the bootloader we mean business ;)
                    NVM_commitFWUpdate(ConfigReceivedDataBuffer[1]);
                    Midi_blinkLED = 1;
                    
                }else if(ConfigReceivedDataBuffer[0] == USB_CMD_FWUPDATE_REBOOT){   //to a software reset
                    __pic32_software_reset();
                    
                }else if(ConfigReceivedDataBuffer[0] == USB_CMD_FWUPDATE_GET_CRC){  //calculate the CRC of the update firmware in flash
                    uint32_t crc = NVM_getUpdateCRC();
                    ToSendDataBuffer[0] = USB_CMD_FWUPDATE_GET_CRC;
                    ToSendDataBuffer[1] = (crc >> 24);
                    ToSendDataBuffer[2] = (crc >> 16);
                    ToSendDataBuffer[3] = (crc >> 8);
                    ToSendDataBuffer[4] = (crc);
                    Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64);
                    
                }else if(ConfigReceivedDataBuffer[0] == USB_CMD_FWUPDATE_START_BULK_WRITE){ //prepare to receive civil judgment, wait no we prepare to receive packet data
                    payload = malloc(PAGE_SIZE);
                    FWUpdate_currPLOffset = 0;
                    currPage = (ConfigReceivedDataBuffer[1] << 8) | (ConfigReceivedDataBuffer[2]);
                    
                    memset(ToSendDataBuffer, 0, 64);                            //return the size of the page to be written
                    ToSendDataBuffer[0] = USB_CMD_FWUPDATE_COMMIT;
                    ToSendDataBuffer[1] = PAGE_SIZE >> 8;
                    ToSendDataBuffer[2] = PAGE_SIZE;
                    Midi_configTxHandle = USBTxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT, ToSendDataBuffer,64); 
                }
            }
        }
        
        //get ready to receive more data
        Midi_configRxHandle = USBRxOnePacket(USB_DEVICE_AUDIO_CONFIG_ENDPOINT,(uint8_t*)&ConfigReceivedDataBuffer,64);
    }
}

void Midi_setNote(uint8_t voice, uint8_t note, uint8_t velocity, uint8_t channel){
    int16_t currNote = note + (int8_t) (MIDI_programmOverrideEnabled ? Midi_currOverrideProgramm->noteOffset : ChannelInfo[channel].currProgramm.noteOffset);
    if(currNote > 127 || currNote < 1) return;
    uint16_t currNoteFreq = (uint16_t) ((double) Midi_NoteFreq[currNote] * ChannelInfo[channel].bendFactor);    //get the required note frequency, including any bender offset
    
    Midi_voice[voice].currNoteOrigin = channel;
    
    Midi_setNoteTPR(voice, currNoteFreq);
    Midi_voice[voice].noteAge = 0;
    Midi_voice[voice].currNote = note;
    Midi_voice[voice].adsrState = NOTE_OFF;    //reset the ADSR state to note off, so attac will work, even if the note was still on
    Midi_voice[voice].currReqNoteOT = (((ChannelInfo[Midi_voice[voice].currNoteOrigin].currOT * velocity * (ChannelInfo[channel].damperPedal ? 6 : 10)) / 1270)) - Midi_minOnTimeVal;      //currReqNoteOT dictates the on time, added to the minimum setting
    
    //TODO implement velocity based ADSR
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
    
    //transform on times, to match the actual timer frequency
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
    uint32_t divVal = 187500 / freq;
    //turn off the timer if the frequency is too low, otherwise set the scaler register to the appropriate value
    switch(voice){
        case 0:
            if(freq == 0){
                T2CONCLR = _T2CON_ON_MASK;
                return;
            }else{
                T2CONSET = _T2CON_ON_MASK;
            }
            PR2 = divVal & 0xffff;
            if(TMR2 > PR2) TMR2 = 0;
            break;
        case 1:
            if(freq == 0){
                T3CONCLR = _T3CON_ON_MASK;
                return;
            }else{
                T3CONSET = _T3CON_ON_MASK;
            }
            PR3 = divVal & 0xffff;
            if(TMR3 > PR3) TMR3 = 0;
            break;
        case 2:
            if(freq == 0){
                T4CONCLR = _T4CON_ON_MASK;
                return;
            }else{
                T4CONSET = _T4CON_ON_MASK;
            }
            PR4 = divVal & 0xffff;
            if(TMR4 > PR4) TMR4 = 0;
            break;
        case 3:
            if(freq == 0){
                T5CONCLR = _T5CON_ON_MASK;
                return;
            }else{
                T5CONSET = _T5CON_ON_MASK;
            }
            PR5 = divVal & 0xffff;
            if(TMR5 > PR5) TMR5 = 0;
            break;
    }
}

void Midi_setNoteOnTime(uint16_t onTimeUs, uint8_t ch){
    if(onTimeUs > Midi_currCoil->maxOnTime || onTimeUs < Midi_currCoil->minOnTime) return;
    //Convert the requested time in us, to a timer divider value
    ChannelInfo[ch].currOT = (onTimeUs * 100) / 133;
}

/*
 * How the notes are played:
 * 
 *  The timer 2,3 & 4,5 are overflowing at the note frequency, 
 *  once the interrupt triggers, we reset the counter for Timer 1 to zero, enable it and turn the output on
 *  once timer 1 overflows, we turn the output off again and disable the timer again (effectively one shot mode)
 * 
 */
int32_t offset = 0;
unsigned dir = 0;

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
    
    if(!(LATB & _LATB_LATB5_MASK)){
        noteHoldoff = 0;
        IEC0SET = _IEC0_T2IE_MASK | _IEC0_T3IE_MASK | _IEC0_T4IE_MASK | _IEC0_T5IE_MASK; 
        T1CONCLR = _T1CON_ON_MASK;
        PR1 = 10;
        TMR1 = 0;
    }else{
        LATBCLR = (_LATB_LATB15_MASK | ((NVM_getConfig()->auxMode == AUX_AUDIO) ? _LATB_LATB5_MASK : 0));  //RB15
        noteOffTime = _CP0_GET_COUNT();     //core timer runs at 1/2 CPU freq = 24MHz, so 1 TMR count = 32 CoreTimer count
        noteHoldoff = 1;
        TMR1 = 0;
        if(holdOff < 10){
            noteHoldoff = 0;
            IEC0SET = _IEC0_T2IE_MASK | _IEC0_T3IE_MASK | _IEC0_T4IE_MASK | _IEC0_T5IE_MASK; 
            T1CONCLR = _T1CON_ON_MASK;
            PR1 = 10;
            TMR1 = 0;
        }else{
            PR1 = holdOff;
        }
        
    }
}

inline void Midi_timerHandler(uint8_t voice){
    IEC0CLR = _IEC0_T2IE_MASK | _IEC0_T3IE_MASK | _IEC0_T4IE_MASK | _IEC0_T5IE_MASK;
    if(Midi_voice[voice].currNoteOT == 0) return;
    
    //scale down the on time as we approach the set maximum (including the on time that will be added by the current note)
    uint16_t newLimitTime = accumulatedOnTime + Midi_voice[voice].currNoteOT;
    int16_t nextOnTime = Midi_voice[voice].currNoteOT - ((Midi_voice[voice].currNoteOT * ((newLimitTime > halfLimit) ? (newLimitTime - halfLimit) : 0)) / halfLimit);
    
    if(noteOffTime != 0){
        //count down the accumulator by the off time
        accumulatedOnTime -= ((_CP0_GET_COUNT() - noteOffTime) * Midi_currCoil->maxDuty) / 3200;
        if(accumulatedOnTime < 0) accumulatedOnTime = 0;
        accumulatedOnTime += nextOnTime;
        if(accumulatedOnTime > Midi_currCoil->ontimeLimit) accumulatedOnTime = Midi_currCoil->ontimeLimit;
        noteOffTime = 0;
    }
    
    //start the on-time timer
    T1CONSET = _T1CON_ON_MASK;
    
    //if out new on time is too small ignore it
    if(nextOnTime <= 0){
        PR1 = 5;   //wait a bit until firing the off-interrupt, even if no on time is desired
        return;
    }
    PR1 = nextOnTime;
    
    //switch on the output if the E-STOP is clear, or not enabled. Switch on the secondary output if aux out is desired
    if(NVM_getConfig()->auxMode != AUX_E_STOP || (PORTB & _PORTB_RB5_MASK)) LATBSET = _LATB_LATB15_MASK | ((NVM_getConfig()->auxMode == AUX_AUDIO) ? _LATB_LATB5_MASK : 0);  //RB15

    //make sure the timer is not past its rollover point, now that we have changed the period. It would run for 65536 - delta(TMR1,PR1) clock cycles otherwise
    if(TMR1 > PR1-10) TMR1 = PR1-10;
}


//"HAL" for the leds, takes the settings for led usage into account
void Midi_LED(uint8_t type, uint8_t state){
    if(NVM_getConfig()->ledMode1 == type) {   //blue
        if(state == LED_ON) LATBCLR = _LATB_LATB9_MASK; else if(state == LED_OFF) LATBSET = _LATB_LATB9_MASK; else if(state == LED_BLINK) LATBINV = _LATB_LATB9_MASK; //blink coms led
    }
    if(NVM_getConfig()->ledMode2 == type && ! Midi_blinkLED) {   //white
        if(state == LED_ON) LATBCLR = _LATB_LATB8_MASK; else if(state == LED_OFF) LATBSET = _LATB_LATB8_MASK; else if(state == LED_BLINK) LATBINV = _LATB_LATB8_MASK; //blink coms led
    }
    if(NVM_getConfig()->ledMode3 == type) {   //green
        if(state == LED_ON) LATBCLR = _LATB_LATB7_MASK; else if(state == LED_OFF) LATBSET = _LATB_LATB7_MASK; else if(state == LED_BLINK) LATBINV = _LATB_LATB7_MASK; //blink coms led
    }
}