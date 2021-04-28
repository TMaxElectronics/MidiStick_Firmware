/*
    Copyright (C) 2020,2021 TMax-Electronics.de
   
    This file is part of the MidiStick Firmware.

    the MidiStick Firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    the MidiStick Firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the MidiStick Firmware.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <xc.h>
#include <stdint.h>

#include "MidiController.h"
#include "SignalGenerator.h"
#include "UART32.h"
#include "ADSREngine.h"

static unsigned SigGen_outputOn = 0;
static uint32_t SigGen_maxOTScaled = 0;

static uint32_t SigGen_trOT = 0;
static uint32_t SigGen_trPulses = 0;
static uint32_t SigGen_trPulseCount = 0;

static uint32_t SigGen_watchDogCounter = 0;

static SigGen_masterVol = SIGGEN_DEFAULTVOLUME;

void SigGen_init(){
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

void SigGen_setTR(uint32_t freq, uint32_t ot, uint32_t burstLength, uint32_t burstDelay){ 
    if(SigGen_genMode != SIGGEN_TR) return;
    
    if(freq == 0 || ot == 0){
        SigGen_kill();
        return;
    }
    
    SigGen_maxOTScaled = (Midi_currCoil->maxOnTime * 100) / 133;
    
    if(freq * ot > 10000 * Midi_currCoil->maxDuty){
        uint32_t scale = (10000000 * Midi_currCoil->maxDuty) / (freq * ot);
        SigGen_trOT = (ot * scale) / 1330;
        Midi_LED(LED_DUTY_LIMITER, LED_ON);
    }else{
        SigGen_trOT = (ot * 100) / 133;
        Midi_LED(LED_DUTY_LIMITER, LED_OFF);
    }
    
    SigGen_trPulses = burstLength;
    
    PR2 = (187500 / freq) & 0xffff;
    if(TMR2 > PR2) TMR2 = PR2 - 1;
    T2CONSET = _T2CON_ON_MASK;
    IEC0SET = _IEC0_T2IE_MASK;
    
    if(burstLength == 0 || burstDelay == 0){
        T4CONCLR = _T4CON_ON_MASK;
        T5CONCLR = _T5CON_ON_MASK;
        SigGen_trPulses = 0;
    }else{
        uint32_t divider = (187 * burstDelay);
        T4CONSET = _T4CON_ON_MASK;
        T5CONSET = _T5CON_ON_MASK;
        IEC0SET = _IEC0_T5IE_MASK;
        PR4 = divider & 0xffff;
        PR5 = (divider >> 16) & 0xffff;
        TMR4 = 0;
        TMR5 = 0;
    }
    
    SigGen_resetWatchDog();
}

void SigGen_setMode(GENMODE newMode){
    SigGen_kill();
    SigGen_genMode = newMode;
    
    switch(newMode){
        case SIGGEN_music4V:
        case SIGGEN_musicSID:
            T4CON = 0b01110000;
            T5CON = 0b01110000;
            break;
             
        case SIGGEN_TR:
            T4CON = 0b01111000;
            T5CON = 0b01111000;
            IEC0SET = _IEC0_T2IE_MASK | _IEC0_T5IE_MASK; 
            
            T3CONSET = _T3CON_ON_MASK;
            IEC0SET = _IEC0_T3IE_MASK;
            PR3 = 187500 / 10;
            TMR3 = 0;
            break;
    }
}

void SigGen_kill(){
    T1CONCLR = _T1CON_ON_MASK;
    
    IEC0CLR = _IEC0_T2IE_MASK | _IEC0_T3IE_MASK | _IEC0_T4IE_MASK | _IEC0_T5IE_MASK; 
    
    T2CONCLR = _T2CON_ON_MASK;
    T3CONCLR = _T3CON_ON_MASK;
    T4CONCLR = _T4CON_ON_MASK;
    T5CONCLR = _T5CON_ON_MASK;
    
    LATBCLR = _LATB_LATB15_MASK | _LATB_LATB5_MASK; //turn off the output
}

//set the timer period register to the appropriate value for the required frequency. To allow for smooth pitch bend commands the frequency is in 1/10th Hz
void SigGen_setNoteTPR(uint8_t voice, uint32_t freqTenths){
    if(SigGen_genMode != SIGGEN_music4V && SigGen_genMode != SIGGEN_musicSID) return;
    uint32_t divVal = 69;
    if(freqTenths != 0) divVal = 1875000 / freqTenths;
    
    Midi_voice[voice].periodCurrent = divVal;
    Midi_voice[voice].freqCurrent = freqTenths;
    
    SigGen_maxOTScaled = (Midi_currCoil->maxOnTime * 100) / 133;
    
    SigGen_limit();
    //turn off the timer if the frequency is too low, otherwise set the scaler register to the appropriate value
    switch(voice){
        case 0:
            if(freqTenths == 0){
                T2CONCLR = _T2CON_ON_MASK;
                IEC0CLR = _IEC0_T2IE_MASK;
            }else{
                PR2 = divVal & 0xffff;
                if(TMR2 > PR2) TMR2 = PR2 - 1;
                T2CONSET = _T2CON_ON_MASK;
                IEC0SET = _IEC0_T2IE_MASK;
            }
            break;
            
        case 1:
            if(freqTenths == 0){
                T3CONCLR = _T3CON_ON_MASK;
                IEC0CLR = _IEC0_T3IE_MASK;
            }else{
                PR3 = divVal & 0xffff;
                if(TMR3 > PR3) TMR3 = PR3 - 1;
                T3CONSET = _T3CON_ON_MASK;
                IEC0SET = _IEC0_T3IE_MASK;
            }
            break;
            
        case 2:
            if(freqTenths == 0){
                T4CONCLR = _T4CON_ON_MASK;
                IEC0CLR = _IEC0_T4IE_MASK;
            }else{
                PR4 = divVal & 0xffff;
                if(TMR4 > PR4) TMR4 = PR4 - 1;
                T4CONSET = _T4CON_ON_MASK;
                IEC0SET = _IEC0_T4IE_MASK;
            }
            break;
            
        case 3:
            if(freqTenths == 0){
                T5CONCLR = _T5CON_ON_MASK;
                IEC0CLR = _IEC0_T5IE_MASK;
            }else{
                PR5 = divVal & 0xffff;
                if(TMR5 > PR5) TMR5 = PR5 - 1;
                T5CONSET = _T5CON_ON_MASK;
                IEC0SET = _IEC0_T5IE_MASK;
            }
            break;
            
    }
}

void SigGen_resetWatchDog(){
    SigGen_watchDogCounter = 0;
}

void SigGen_resetMasterVol(){
    SigGen_masterVol = SIGGEN_DEFAULTVOLUME;
}

void SigGen_setMasterVol(uint32_t newVol){
    if(newVol > 0xff) return;
    SigGen_masterVol = newVol;
}

void SigGen_limit(){
    uint32_t totalDuty = 0;
    uint32_t scale = 1000;
    //TODO remove that dumb /10 and make the duty cycle *10 larger
    uint8_t c = 0;
    for(; c < MIDI_VOICECOUNT; c++){
        totalDuty += (Midi_voice[c].otCurrent * Midi_voice[c].freqCurrent * SigGen_masterVol) / 2550; //TODO preemtively increase the frequency used for calculation if noise is on
    }
    
    if(totalDuty > 10000 * Midi_currCoil->maxDuty){
        scale = (10000000 * Midi_currCoil->maxDuty) / totalDuty;
        Midi_LED(LED_DUTY_LIMITER, LED_ON);
    }else{
        scale = 1000;
        Midi_LED(LED_DUTY_LIMITER, LED_OFF);
    }
    //UART_print("duty = %d%% -> scale = %d%%m\r\n", totalDuty / 10000, scale);
    
    //limit noise frequency change
    for(c = 0; c < MIDI_VOICECOUNT; c++){
        if(Midi_voice[c].noiseCurrent > (Midi_voice[c].periodCurrent >> 1)){
            Midi_voice[c].noiseRaw = Midi_voice[c].periodCurrent >> 1;
            //UART_print("limiting noise to %d\r\n", Midi_voice[c].noiseRaw);
        }else{
            Midi_voice[c].noiseRaw = Midi_voice[c].noiseCurrent;
        }
    }
    
    for(c = 0; c < MIDI_VOICECOUNT; c++){
        Midi_voice[c].outputOT = (Midi_voice[c].otCurrent * scale * SigGen_masterVol) / 339150;
    }
}   

void SigGen_writeRaw(RAW_WRITE_NOTES_Cmd * raw){
    if(SigGen_genMode != SIGGEN_musicSID) return;
    int32_t ot1 = (raw->v1OT == 0) ? 0 : (((raw->v1OT * Midi_currCoil->maxOnTime) >> 16) + Midi_currCoil->minOnTime);
    int32_t ot2 = (raw->v2OT == 0) ? 0 : (((raw->v2OT * Midi_currCoil->maxOnTime) >> 16) + Midi_currCoil->minOnTime);
    int32_t ot3 = (raw->v3OT == 0) ? 0 : (((raw->v3OT * Midi_currCoil->maxOnTime) >> 16) + Midi_currCoil->minOnTime);
    int32_t ot4 = (raw->v3OT == 0) ? 0 : (((raw->v4OT * Midi_currCoil->maxOnTime) >> 16) + Midi_currCoil->minOnTime);
    
    Midi_voice[0].otCurrent = ot1;
    Midi_voice[1].otCurrent = ot2;
    Midi_voice[2].otCurrent = ot3;
    Midi_voice[3].otCurrent = ot4;
    
    Midi_voice[0].noiseCurrent = raw->v1NoiseAmp;
    Midi_voice[1].noiseCurrent = raw->v2NoiseAmp;
    Midi_voice[2].noiseCurrent = raw->v3NoiseAmp;
    Midi_voice[3].noiseCurrent = raw->v4NoiseAmp;
    
    SigGen_setNoteTPR(0, (raw->v1OT == 0) ? 0 : (raw->v1Freq * 10));
    SigGen_setNoteTPR(1, (raw->v2OT == 0) ? 0 : (raw->v2Freq * 10));
    SigGen_setNoteTPR(2, (raw->v3OT == 0) ? 0 : (raw->v3Freq * 10));
    SigGen_setNoteTPR(3, (raw->v4OT == 0) ? 0 : (raw->v4Freq * 10));
    
    //UART_print("write raw: f1 = %d ot1 = %d f2 = %d ot2 = %d f3 = %d ot3 = %d f4 = %d\r\n", Midi_voice[0].freqCurrent, Midi_voice[0].outputOT, raw->v2Freq, raw->v2OT, raw->v3Freq, raw->v3OT, raw->v4Freq);
}

/*
 * How the notes are played:
 * 
 *  The timer 2,3,4 and 5 are overflowing at the note frequency, 
 *  once the interrupt triggers, we reset the counter for Timer 1 to zero, enable it and turn the output on
 *  once timer 1 overflows, we turn the output off again and disable the timer again (effectively one shot mode)
 *  
 * How the transient mode works
 * 
 *  timer 2 handles the pulse frequency
 *  timer 3 handles the burst frequency
 *  timer 1 handles the pulse onTime
 */

void __ISR(_TIMER_2_VECTOR) SigGen_tmr2ISR(){
    IFS0CLR = _IFS0_T2IF_MASK;
    if(SigGen_genMode == SIGGEN_music4V || SigGen_genMode == SIGGEN_musicSID){ 
        SigGen_timerHandler(0); 
        
    }else if(SigGen_genMode == SIGGEN_TR){
        if(SigGen_trPulses != 0 && ++SigGen_trPulseCount >= SigGen_trPulses){    //we have made enough pulses for the burst... turn ourselves off
            T2CONCLR = _T2CON_ON_MASK;
        }
        
        if(SigGen_trOT > SigGen_maxOTScaled) return;    //make double sure that the maximum OT is not exceeded       && SigGen_watchDogCounter < 10
        
        //start the NoteOT timer
        PR1 = SigGen_trOT;
        TMR1 = 0;
        T1CONSET = _T1CON_ON_MASK;
        IEC0CLR = _IEC0_T2IE_MASK | _IEC0_T4IE_MASK | _IEC0_T5IE_MASK; 
        
        //switch on the output
        SigGen_outputOn = 1;
        if(NVM_getConfig()->auxMode != AUX_E_STOP || (PORTB & _PORTB_RB5_MASK)) LATBSET = (_LATB_LATB15_MASK | ((NVM_getConfig()->auxMode == AUX_AUDIO) ? _LATB_LATB5_MASK : 0));
    }
}

void __ISR(_TIMER_3_VECTOR) SigGen_tmr3ISR(){
    IFS0CLR = _IFS0_T3IF_MASK;
    if(SigGen_genMode == SIGGEN_music4V || SigGen_genMode == SIGGEN_musicSID) SigGen_timerHandler(1);
    if(SigGen_genMode == SIGGEN_TR && SigGen_trPulses != 0){
        if(SigGen_watchDogCounter < 10) SigGen_watchDogCounter++;
    }
}

void __ISR(_TIMER_4_VECTOR) SigGen_tmr4ISR(){
    IFS0CLR = _IFS0_T4IF_MASK;
    if(SigGen_genMode == SIGGEN_music4V || SigGen_genMode == SIGGEN_musicSID) SigGen_timerHandler(2);
}

void __ISR(_TIMER_5_VECTOR) SigGen_tmr5ISR(){
    IFS0CLR = _IFS0_T5IF_MASK;
    if(SigGen_genMode == SIGGEN_music4V || SigGen_genMode == SIGGEN_musicSID) SigGen_timerHandler(3);
    
    if(SigGen_genMode == SIGGEN_TR && SigGen_trPulses != 0){
        T2CONSET = _T2CON_ON_MASK;
        SigGen_trPulseCount = 0;
    }
}

void __ISR(_TIMER_1_VECTOR) SigGen_noteOffISR(){
    IFS0CLR = _IFS0_T1IF_MASK;
    
    LATBCLR = _LATB_LATB15_MASK | _LATB_LATB5_MASK; //turn off the output
    if(SigGen_outputOn){
        SigGen_outputOn = 0;
        PR1 = (Midi_currCoil->holdoffTime * 100) / 133;
        TMR1 = 0;
    }else{
        T1CONCLR = _T1CON_ON_MASK;
        IEC0SET = _IEC0_T2IE_MASK | _IEC0_T3IE_MASK | _IEC0_T4IE_MASK | _IEC0_T5IE_MASK; 
    }
}

inline void SigGen_timerHandler(uint8_t voice){
    if(SigGen_outputOn) return;
    if(Midi_voice[voice].otCurrent < Midi_currCoil->minOnTime || Midi_voice[voice].otCurrent > Midi_currCoil->maxOnTime) return;
    if(Midi_voice[voice].outputOT == 0 || Midi_voice[voice].outputOT > SigGen_maxOTScaled) return;
    SigGen_outputOn = 1;
    
    uint16_t otLimit = Midi_currCoil->ontimeLimit * 4;
    
    if(Midi_voice[voice].noiseRaw > 0){
        int32_t nextPeriod = Midi_voice[voice].periodCurrent + (rand() / (RAND_MAX / Midi_voice[voice].noiseRaw)) - (Midi_voice[voice].noiseRaw >> 1);
        if(nextPeriod > 10){
            //nextOT >>= 1;

            switch(voice){
                case 0:
                    PR2 = nextPeriod;
                    break;

                case 1:
                    PR3 = nextPeriod;
                    break;

                case 2:
                    PR4 = nextPeriod;
                    break;

                case 3:
                    PR5 = nextPeriod;
                    break;
            }
        }
    }
    
    
    PR1 = Midi_voice[voice].outputOT;
    TMR1 = 0;
    T1CONSET = _T1CON_ON_MASK;
    IEC0CLR = _IEC0_T2IE_MASK | _IEC0_T3IE_MASK | _IEC0_T4IE_MASK | _IEC0_T5IE_MASK; 
    
    //at this point everything is set up to turn the output off after a safe on time, so we can turn on the output. We have to make sure that the E-Stop is not triggered if it is enabled
    if(NVM_getConfig()->auxMode != AUX_E_STOP || (PORTB & _PORTB_RB5_MASK)) LATBSET = (_LATB_LATB15_MASK | ((NVM_getConfig()->auxMode == AUX_AUDIO) ? _LATB_LATB5_MASK : 0));
}