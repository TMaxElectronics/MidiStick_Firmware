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
static uint32_t SigGen_holdOff = 0;
static uint32_t SigGen_watchDogCounter = 0;
uint32_t en = 1;

uint32_t SigGen_masterVol = SIGGEN_DEFAULTVOLUME;

void SigGen_init(){
    //enable DMA for hyperVoice (tm)
    DMACON = _DMACON_ON_MASK;
    
    //note on time timer @ 750 KHz
    T1CON = 0b00100000;
    IEC0bits.T1IE = 1;
    IPC1bits.T1IP = 7;
    T1CONbits.ON = 1;
    
    //Note one timer (F = 48MHz), because we can 
    T2CON = 0b01110000;
    IEC0bits.T2IE = 1;
    IPC2bits.T2IP = 5;
    T2CONbits.ON = 0;
    
    Midi_voice[0].hyperVoice_DCHXCONptr = &DCH0CON; 
    Midi_voice[0].hyperVoice_DCHXECONptr = &DCH0ECON; 
    DCH0CON  = 0b10010011;
    DCH0ECON = 0b00010000; DCH0ECONbits.CHSIRQ = _TIMER_2_IRQ;
    DCH0SSIZ = 4; DCH0SSA = KVA_TO_PA(Midi_voice[0].hyperVoice_timings);
    DCH0DSIZ = 2; DCH0DSA = KVA_TO_PA(&PR2);
    DCH0CSIZ = 2;
    
    //Note two timer (F = 48MHz), because we can 
    T3CON = 0b01110000;
    IEC0bits.T3IE = 1;
    IPC3bits.T3IP = 5;
    T2CONbits.ON = 0;
    
    Midi_voice[1].hyperVoice_DCHXCONptr = &DCH1CON; 
    Midi_voice[1].hyperVoice_DCHXECONptr = &DCH1ECON; 
    DCH1CON  = 0b10010010;
    DCH1ECON = 0b00010000; DCH1ECONbits.CHSIRQ = _TIMER_3_IRQ;
    DCH1SSIZ = 4; DCH1SSA = KVA_TO_PA((Midi_voice[1].hyperVoice_timings));
    DCH1DSIZ = 2; DCH1DSA = KVA_TO_PA(&PR3);
    DCH1CSIZ = 2;
    
    //Note three timer (F = 48MHz)
    T4CON = 0b01110000;
    IEC0bits.T4IE = 1;
    IPC4bits.T4IP = 5;
    T4CONbits.ON = 0;
    
    Midi_voice[2].hyperVoice_DCHXCONptr = &DCH2CON; 
    Midi_voice[2].hyperVoice_DCHXECONptr = &DCH2ECON; 
    DCH2CON  = 0b10010001;
    DCH2ECON = 0b00010000; DCH2ECONbits.CHSIRQ = _TIMER_4_IRQ;
    DCH2SSIZ = 4; DCH2SSA = KVA_TO_PA((Midi_voice[2].hyperVoice_timings));
    DCH2DSIZ = 2; DCH2DSA = KVA_TO_PA(&PR4);
    DCH2CSIZ = 2;
    
    //Note four timer (F = 48MHz), because we can 
    T5CON = 0b01110000;
    IEC0bits.T5IE = 1;
    IPC5bits.T5IP = 5;
    T2CONbits.ON = 0;
    
    Midi_voice[3].hyperVoice_DCHXCONptr = &DCH3CON;
    Midi_voice[3].hyperVoice_DCHXECONptr = &DCH3ECON; 
    DCH3CON  = 0b10010000;
    DCH3ECON = 0b00010000; DCH3ECONbits.CHSIRQ = _TIMER_5_IRQ;
    DCH3SSIZ = 4; DCH3SSA = KVA_TO_PA((Midi_voice[3].hyperVoice_timings));
    DCH3DSIZ = 2; DCH3DSA = KVA_TO_PA(&PR5);
    DCH3CSIZ = 2;
}

void SigGen_setTR(uint32_t freq, uint32_t ot, uint32_t burstLength, uint32_t burstDelay){ 
    if(SigGen_genMode != SIGGEN_TR) return;
    
    if(freq == 0 || ot == 0){
        SigGen_kill();
        return;
    }
    
    SigGen_maxOTScaled = (Midi_currCoil->maxOnTime * 100) / 133;
    SigGen_holdOff = (Midi_currCoil->holdoffTime * 100) / 133;
    
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
        IEC0CLR = _IEC0_T5IE_MASK;
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
            DMACONbits.ON = 1;
            break;
             
        case SIGGEN_TR:
            T4CON = 0b01111000;
            T5CON = 0b01111000;
            IEC0SET = _IEC0_T2IE_MASK | _IEC0_T5IE_MASK; 
            
            T3CONSET = _T3CON_ON_MASK;
            IEC0SET = _IEC0_T3IE_MASK;
            PR3 = 187500 / 10;
            TMR3 = 0;
            DMACONbits.ON = 0;
            break;
             
        case SIGGEN_AUDIO_ZCD:
            IEC0CLR = _IEC0_T2IE_MASK | _IEC0_T3IE_MASK | _IEC0_T4IE_MASK | _IEC0_T5IE_MASK; 
            en = 1;
            DMACONbits.ON = 0;
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
    
    Midi_voice[0].hyperVoiceCount = 0;
    Midi_voice[1].hyperVoiceCount = 0;
    Midi_voice[2].hyperVoiceCount = 0;
    Midi_voice[3].hyperVoiceCount = 0;
    
    en = 1;
    
    SigGen_outputOn = 0;
}

//set the timer period register to the appropriate value for the required frequency. To allow for smooth pitch bend commands the frequency is in 1/10th Hz
void SigGen_setNoteTPR(uint8_t voice, uint32_t freqTenths){
    if(SigGen_genMode != SIGGEN_music4V && SigGen_genMode != SIGGEN_musicSID) return;
    uint32_t divVal = 69;
    if(freqTenths != 0) divVal = 1875000 / freqTenths;
    
    Midi_voice[voice].periodCurrent = divVal;
    Midi_voice[voice].freqCurrent = freqTenths;
    
    SigGen_limit();
    
    if(freqTenths != 0){
        if(Midi_voice[voice].hyperVoiceCount == 2 && Midi_voice[voice].noiseCurrent == 0 && Midi_voice[voice].outputOT > 10){  // && Midi_voice[voice].hyperVoicePhase > 0

            //calculate the timings for the two pulses
            uint32_t t0 = (divVal * Midi_voice[voice].hyperVoicePhase) >> 10;
            uint32_t t1 = divVal - t0;

            //UART_print("calcing HyperV on %d: dma = 0x%08x TMR = %d PR = %d CON = 0x%08x\r\n", voice, *Midi_voice[voice].hyperVoice_DCHXCONptr, *Midi_voice[voice].tmrTMR, *Midi_voice[voice].tmrPR, *Midi_voice[voice].tmrCON);
            
            uint32_t ot = (Midi_voice[voice].outputOT >> 2) + (SigGen_holdOff >> 2) + 10;
            if(t1 < ot){
                Midi_voice[voice].hyperVoice_timings[0] = divVal - ot;
                Midi_voice[voice].hyperVoice_timings[1] = ot;
                //UART_print("WRONG2 ot = %d\t", ot);
            }else if(t0 < ot){ 
                Midi_voice[voice].hyperVoice_timings[0] = ot;
                Midi_voice[voice].hyperVoice_timings[1] = divVal - ot;
                //UART_print("WRONG3 ot = %d\t", ot); 
            }else{
                //UART_print("gud\t", ot); 
                Midi_voice[voice].hyperVoice_timings[0] = t0;
                Midi_voice[voice].hyperVoice_timings[1] = t1;
            }
            
            //UART_print("calcing HyperV on %d: dma = 0x%08x TMR = %d PR = %d CON = 0x%08x phase = %d period total = %d t[0]=%d t[1]=%d\r\n", voice, *Midi_voice[voice].hyperVoice_DCHXCONptr, *Midi_voice[voice].tmrTMR, *Midi_voice[voice].tmrPR, *Midi_voice[voice].tmrCON, Midi_voice[voice].hyperVoicePhase, Midi_voice[voice].periodCurrent, Midi_voice[voice].hyperVoice_timings[0], Midi_voice[voice].hyperVoice_timings[1]);

            *(Midi_voice[voice].hyperVoice_DCHXCONptr) |= _DCH0CON_CHEN_MASK;
        }else{
            if(Midi_voice[voice].noiseCurrent == 0){
                Midi_voice[voice].hyperVoice_timings[0] = divVal;
                Midi_voice[voice].hyperVoice_timings[1] = divVal;
                //UART_print("WRONG\r\n");
                *(Midi_voice[voice].hyperVoice_DCHXCONptr) |= _DCH0CON_CHEN_MASK;
            }else{
                *(Midi_voice[voice].tmrPR) = divVal;
                if(*(Midi_voice[voice].tmrTMR) >= divVal) *(Midi_voice[voice].tmrTMR) = divVal - 5;
                *(Midi_voice[voice].hyperVoice_DCHXCONptr) &=  ~_DCH0CON_CHEN_MASK;
            }
            //UART_print("calcing nonHyperV on %d: dma = 0x%08x TMR = %d PR = %d CON = 0x%08x period total = %d t[0]=%d t[1]=%d ot = %d on = 0x%04x\r\n", voice, *Midi_voice[voice].hyperVoice_DCHXCONptr, *Midi_voice[voice].tmrTMR, *Midi_voice[voice].tmrPR, *Midi_voice[voice].tmrCON, Midi_voice[voice].periodCurrent, Midi_voice[voice].hyperVoice_timings[0], Midi_voice[voice].hyperVoice_timings[1], Midi_voice[voice].outputOT, LATB & 0xffff);

        }

        if((*(Midi_voice[voice].tmrCON) & _T2CON_ON_MASK) == 0){
            //UART_print("jumpstarting DMA\r\n");
            
            *(Midi_voice[voice].tmrPR) = divVal;
            *(Midi_voice[voice].tmrTMR) = 0;
            
            /*if(Midi_voice[voice].noiseCurrent == 0){
                *(Midi_voice[voice].hyperVoice_DCHXECONptr) |= _DCH0ECON_CFORCE_MASK;
            }else{
            }*/
            
            *(Midi_voice[voice].tmrCON) |= _T2CON_ON_MASK;
            IEC0SET = Midi_voice[voice].iecMask;
        }
    }else{
        //UART_print("WRONG %d\r\n", freqTenths);
        *(Midi_voice[voice].tmrCON) &= ~_T2CON_ON_MASK;
        IEC0CLR = Midi_voice[voice].iecMask;
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
    uint8_t c = 0;
    
    for(; c < MIDI_VOICECOUNT; c++){
        uint32_t ourDuty = (Midi_voice[c].otCurrent * Midi_voice[c].freqCurrent) / 10; //TODO preemtively increase the frequency used for calculation if noise is on
        if(Midi_voice[c].hyperVoiceCount == 2 && Midi_voice[c].noiseCurrent == 0) ourDuty *= 2;
        totalDuty += ourDuty;
    }
    
    totalDuty = (totalDuty * SigGen_masterVol) / 0xff;
    totalDuty = (totalDuty * COMP_getGain()) >> 10;
    
    SigGen_holdOff = (Midi_currCoil->holdoffTime * 100) / 133;
    if(SigGen_holdOff < 2) SigGen_holdOff = 2;
    SigGen_maxOTScaled = (Midi_currCoil->maxOnTime * 100) / 133;
    
    //scale newMaxDuty with the hard limit override. Default maxDutyOffset=64 => maximum factor = 256/64 = 4
    uint32_t newMaxDuty = (Midi_currCoil->maxDuty * NVM_getExpConfig()->maxDutyOffset) >> 6; 
    
    //add a safety limit
    if(newMaxDuty > 90) newMaxDuty = 90;
    
    if(totalDuty > 10000 * newMaxDuty){
        scale = (10000000 * newMaxDuty) / totalDuty;
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
        uint32_t ot;
        ot = (Midi_voice[c].otCurrent * scale) / 1330;
        ot = (ot * SigGen_masterVol) / 255;
        ot = (ot * COMP_getGain()) >> 10;
        
        Midi_voice[c].outputOT = ot;
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
    
    Midi_voice[0].hyperVoiceCount = raw->v1HPVCount; Midi_voice[0].hyperVoicePhase = raw->v1HPVPhase;
    Midi_voice[1].hyperVoiceCount = raw->v2HPVCount; Midi_voice[1].hyperVoicePhase = raw->v2HPVPhase;
    Midi_voice[2].hyperVoiceCount = raw->v3HPVCount; Midi_voice[2].hyperVoicePhase = raw->v3HPVPhase;
    Midi_voice[3].hyperVoiceCount = raw->v4HPVCount; Midi_voice[3].hyperVoicePhase = raw->v4HPVPhase;
    
    SigGen_setNoteTPR(0, (raw->v1OT == 0) ? 0 : (raw->v1Freq * 10));
    SigGen_setNoteTPR(1, (raw->v2OT == 0) ? 0 : (raw->v2Freq * 10));
    SigGen_setNoteTPR(2, (raw->v3OT == 0) ? 0 : (raw->v3Freq * 10));
    SigGen_setNoteTPR(3, (raw->v4OT == 0) ? 0 : (raw->v4Freq * 10));
    
    //UART_print("write raw: f1 = %d ot1 = %d f2 = %d ot2 = %d f3 = %d ot3 = %d f4 = %d\r\n", Midi_voice[0].freqCurrent, Midi_voice[0].outputOT, raw->v2Freq, raw->v2OT, raw->v3Freq, raw->v3OT, raw->v4Freq);
}

int16_t lastSample = 0;
int16_t lowPass = 0;

int16_t thresholdC = 0;
int16_t thresholdH = 0;
int16_t thresholdL = 0;
int16_t lastPol = 0;
int16_t currPol = 0;
int32_t lowPassC = 0;
uint32_t sampleE = 0;

int32_t currPeakSelect = 0;
int32_t targetOT = 0;

void SigGen_setZCD(int16_t threshold, int16_t thresholdWidth, uint32_t lowpass, uint32_t holdoff, uint32_t sampleEn){
    lowPassC = lowpass;
    thresholdC = threshold;
    thresholdH = threshold+thresholdWidth;
    thresholdL = threshold-thresholdWidth;
    sampleE = sampleEn;
    
    SigGen_holdOff = (holdoff * 100) / 133;
    targetOT = (Midi_currCoil->maxOnTime * 100) / 133;
    targetOT = (targetOT * SigGen_masterVol) / 0xff;
}

void SigGen_handleAudioSample(int16_t sample){
    //return;

    if(SigGen_genMode != SIGGEN_AUDIO_ZCD) return;
    
    if(abs(sample) > SigGen_currAudioPeak[0]) SigGen_currAudioPeak[0] = abs(sample);
    
    lowPass = ((lowPass*lowPassC) + (sample * (64-lowPassC))) >> 6;
    
    currPol = lastPol ? (lowPass > thresholdL) : (lowPass > thresholdH);
    
    LATBbits.LATB5 = currPol;
    
    if(lastPol && !currPol){
        int32_t truePeak = SigGen_currAudioPeak[0];
        //find the peak out of the last 5
        uint32_t cp = 0;
        //for(cp = 0; cp < 5; cp++) if(truePeak < currPeak[cp]) truePeak = currPeak[cp];
        
        //start the NoteOT timer
        truePeak *= targetOT;
        truePeak = truePeak >> 15;
        
        if((truePeak > 3) && en){
            PR1 = truePeak;
            TMR1 = 0;
            en = 0;
            T1CONSET = _T1CON_ON_MASK;

            //switch on the output
            SigGen_outputOn = 1;
            if(NVM_getConfig()->auxMode != AUX_E_STOP || (PORTB & _PORTB_RB5_MASK)) LATBSET = (_LATB_LATB15_MASK | ((NVM_getConfig()->auxMode == AUX_AUDIO) ? _LATB_LATB5_MASK : 0));
        }
            UART_print("P=%d", truePeak);
        //if(++currPeakSelect > 4) currPeakSelect = 0;
        //SigGen_currAudioPeak[currPeakSelect] = 0;
    }
    lastPol = currPol;
    if(sampleE) Audio_sendSample(lowPass);
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
        
        if(SigGen_trOT == 0 || SigGen_trOT > SigGen_maxOTScaled) return;    //make double sure that the maximum OT is not exceeded       || SigGen_watchDogCounter >= 10
        
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
    
    LATBCLR = (_LATB_LATB15_MASK | ((NVM_getConfig()->auxMode == AUX_AUDIO) ? _LATB_LATB5_MASK : 0)); //turn off the output
    if(SigGen_outputOn){
        SigGen_outputOn = 0;
        PR1 = SigGen_holdOff;
        //IFS0CLR = _IFS0_T2IF_MASK | _IFS0_T3IF_MASK | _IFS0_T4IF_MASK | _IFS0_T5IF_MASK;
        TMR1 = 0;
    }else{
        en = 1;
        T1CONCLR = _T1CON_ON_MASK;
        IEC0SET = _IEC0_T2IE_MASK | _IEC0_T3IE_MASK | _IEC0_T4IE_MASK | _IEC0_T5IE_MASK; 
    }
}

inline void SigGen_timerHandler(uint8_t voice){
    if(SigGen_outputOn) return;
    if(Midi_voice[voice].otCurrent < Midi_currCoil->minOnTime || Midi_voice[voice].otCurrent > Midi_currCoil->maxOnTime) { return;} //UART_print("\r\not1 error!\r\n"); 
    if(Midi_voice[voice].outputOT == 0 || Midi_voice[voice].outputOT > SigGen_maxOTScaled) { return;} //UART_print("\r\not2 error!\r\n"); 
    SigGen_outputOn = 1;
    
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