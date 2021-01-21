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

int32_t SigGen_accumulatedOT = 0;
uint32_t SigGen_unblockTime = 0;
unsigned SigGen_outputOn = 0;
uint8_t lastTimer = 0xff;
uint32_t SigGen_maxOTScaled = 0;

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

//set the timer period register to the appropriate value for the required frequency. To allow for smooth pitch bend commands the frequency is in 1/10th Hz
void SigGen_setNoteTPR(uint8_t voice, uint32_t freqTenths){
    uint32_t divVal = 69;
    if(freqTenths != 0) divVal = 1875000 / freqTenths;
    
    Midi_voice[voice].periodCurrent = divVal;
    Midi_voice[voice].freqCurrent = freqTenths;
    
    SigGen_maxOTScaled = (Midi_currCoil->maxOnTime * 100) / 133;
    //turn off the timer if the frequency is too low, otherwise set the scaler register to the appropriate value
    switch(voice){
        case 0:
            if(freqTenths == 0){
                T2CONCLR = _T2CON_ON_MASK;
                IEC0CLR = _IEC0_T2IE_MASK;
            }else{
                T2CONSET = _T2CON_ON_MASK;
                IEC0SET = _IEC0_T2IE_MASK;
                PR2 = divVal & 0xffff;
                if(TMR2 > PR2) TMR2 = PR2 - 1;
            }
            break;
            
        case 1:
            if(freqTenths == 0){
                T3CONCLR = _T3CON_ON_MASK;
                IEC0CLR = _IEC0_T3IE_MASK;
            }else{
                T3CONSET = _T3CON_ON_MASK;
                IEC0SET = _IEC0_T3IE_MASK;
                PR3 = divVal & 0xffff;
                if(TMR3 > PR3) TMR3 = PR3 - 1;
            }
            break;
            
        case 2:
            if(freqTenths == 0){
                T4CONCLR = _T4CON_ON_MASK;
                IEC0CLR = _IEC0_T4IE_MASK;
            }else{
                T4CONSET = _T4CON_ON_MASK;
                IEC0SET = _IEC0_T4IE_MASK;
                PR4 = divVal & 0xffff;
                if(TMR4 > PR4) TMR4 = PR4 - 1;
            }
            break;
            
        case 3:
            if(freqTenths == 0){
                T5CONCLR = _T5CON_ON_MASK;
                IEC0CLR = _IEC0_T5IE_MASK;
            }else{
                T5CONSET = _T5CON_ON_MASK;
                IEC0SET = _IEC0_T5IE_MASK;
                PR5 = divVal & 0xffff;
                if(TMR5 > PR5) TMR5 = PR5 - 1;
            }
            break;
            
    }
    
    SigGen_limit();
}


uint8_t outCounter = 0;
void SigGen_limit(){
    uint32_t totalDuty = 0;
    uint32_t scale = 1000;
    
    uint8_t c = 0;
    for(; c < MIDI_VOICECOUNT; c++){
        totalDuty += (Midi_voice[c].otCurrent * Midi_voice[c].freqCurrent) / 10; //TODO preemtively increase the frequency used for calculation if noise is on
    }
    
    if(totalDuty > 10000 * Midi_currCoil->maxDuty){
        scale = (10000000 * Midi_currCoil->maxDuty) / totalDuty;
        Midi_LED(LED_DUTY_LIMITER, LED_ON);
    }else{
        scale = 1000;
        Midi_LED(LED_DUTY_LIMITER, LED_OFF);
    }
    
    for(c = 0; c < MIDI_VOICECOUNT; c++){
        Midi_voice[c].outputOT = (Midi_voice[c].otCurrent * scale) / 1330;
    }
    
    //if(!outCounter++) UART_print("OT[0]: totalDuty = %d, scale = %d, OT = %d\r\n", totalDuty / 10000, scale, Midi_voice[0].outputOT);
}   

/*
 * How the notes are played:
 * 
 *  The timer 2,3,4 and 5 are overflowing at the note frequency, 
 *  once the interrupt triggers, we reset the counter for Timer 1 to zero, enable it and turn the output on
 *  once timer 1 overflows, we turn the output off again and disable the timer again (effectively one shot mode)
 * 
 */

void __ISR(_TIMER_2_VECTOR) SigGen_note1ISR(){
    IFS0CLR = _IFS0_T2IF_MASK;
    SigGen_timerHandler(0);
}

void __ISR(_TIMER_3_VECTOR) SigGen_note2ISR(){
    IFS0CLR = _IFS0_T3IF_MASK;
    SigGen_timerHandler(1);
}

void __ISR(_TIMER_4_VECTOR) SigGen_note3ISR(){
    IFS0CLR = _IFS0_T4IF_MASK;
    SigGen_timerHandler(2);
}

void __ISR(_TIMER_5_VECTOR) SigGen_note4ISR(){
    IFS0CLR = _IFS0_T5IF_MASK;
    SigGen_timerHandler(3);
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
    
    if(Midi_voice[voice].noiseCurrent > 0){
        int32_t nextPeriod = Midi_voice[voice].periodCurrent + (rand() / (RAND_MAX / Midi_voice[voice].noiseCurrent)) - (Midi_voice[voice].noiseCurrent >> 1);
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