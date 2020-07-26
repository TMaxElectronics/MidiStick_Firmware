#include <stdint.h>
#include <xc.h>
#include "StripDriver.h"
#include "qTouchSPI.h"
#include "usb_hal_pic32mm.h"

//uint8_t latOut[29100];
//uint8_t led[300][4];

uint8_t dim = 255;

void initDriver(){
    PR3 = 17;
    T3CONbits.TCKPS = 0;        //prescaler 1:1
    
    unsigned int ch0startAddr = (unsigned int) latOut & 0x1FFFFFFF;
    unsigned int ch0destAddr = (unsigned int) &LATA & 0x1FFFFFFF;
    
    DMACON = 0x8000;                // dma module on
    DCRCCON = 0;                    // crc module off
    
    DCH0INT = 1<<21;                    // Disable interrupts on all channels
            
    DCH0SSA = ch0startAddr;
    DCH0DSA = ch0destAddr; 
    
    DCH0SSIZ = 29400;                   // source size - 2 bytes
    DCH0DSIZ = 1;                   // destination size - 1 byte
    DCH0CSIZ = 1;                   // cell size - 1 bytes
    
    DCH0CONbits.CHAEN = 0;
    DCH0CONbits.CHEN = 0;
    
    DCH0ECON = 0x0E10;              //Dma on Timer4 (int19)
    DCH0ECONbits.CHSIRQ = 14;
    
    T3CONbits.TON = 1;
}

void sendLEDData(){
    if(DCH0CONbits.CHEN == 1) return;
    generateOutputData();
    DCH0CONbits.CHEN = 1;
}

void setMaster(uint8_t val){ dim = val; };

void generateOutputData(){
    uint16_t currLED = 0;
    uint8_t bitPos = 0;
    uint16_t pos, pos2;
    uint8_t currColor;
    for(pos2 = 0; pos2 <= 300; pos2++) latOut[pos2] = 0;
    for(currLED = 0;currLED < 300; currLED++){
        pos = currLED * 96 + 300; //* 4 * 8 * 3;
        for(currColor = 0; currColor < 4; currColor++){
            for(bitPos = 0; bitPos < 8; bitPos++){
                pos2 = pos + currColor * 24 + bitPos * 3;
                latOut[pos2] = 0b10;
                latOut[pos2 + 1] = ((((led[currLED][currColor] * dim) / 256) >> (7-bitPos)) & 1) ? 0b10 : 0;// = (currColor == 3 | currColor == 2) ? 0b100 : 0;//
                latOut[pos2 + 2] = 0;
            }
        }
        
    }
    pos = 300 * 96; //break after the last LED
    for(pos2 = 0; pos2 < 300; pos2++) latOut[pos + pos2] = 0;
}

void setLed(uint16_t LED, uint8_t r, uint8_t g, uint8_t b, uint8_t w){
    led[LED][0] = g;
    led[LED][1] = r;
    led[LED][2] = b;
    led[LED][3] = w;
}

void setAllLEDs(uint8_t r, uint8_t g, uint8_t b, uint8_t w){
    uint16_t currLED = 0;
    for(; currLED < 300; currLED++){
        setLed(currLED, r, g, b, w);
    }
}

void setAllLEDsA(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint16_t end){
    uint16_t currLED = 0;
    for(; currLED < end; currLED++){
        setLed(currLED, r, g, b, w);
    }
    for(; currLED < 300; currLED++){
        setLed(currLED, 0, 0, 0, 0);
    }
}

/*  
 * This function has to be called whenever a new frame is desired to be shown. 
 *  
 * it sets generationPending to signal a new frame needs to be rendered, which should only be done on the main loop, to get out of the interrupt fast enough not to collide with USB data
 * 
*/

void animationTask(){
    switch(currAnimation){
        case 0:         //0 = no animation running      --      send data once every second incase strip is switched off
            generationPending = 1;
            break;
        case 1:         //all LEDS fading simultaneously R-G-B      --      255 frames/fade ->  255*3 = 765
            currFrame+= currFPS/64;
            if(currFrame >= 765) currFrame = 0;
            
            switch(currFrame / 255){
                case 0:
                    setAllLEDs((Colors[rgbF2][0] * (currFrame % 255) + Colors[rgbF1][0] * (255-(currFrame % 255)))/255, (Colors[rgbF2][1] * (currFrame % 255) + Colors[rgbF1][1] * (255-(currFrame % 255)))/255, (Colors[rgbF2][2] * (currFrame % 255) + Colors[rgbF1][2] * (255-(currFrame % 255)))/255, 0);
                    break;
                    
                case 1:
                    setAllLEDs((Colors[rgbF3][0] * (currFrame % 255) + Colors[rgbF2][0] * (255-(currFrame % 255)))/255, (Colors[rgbF3][1] * (currFrame % 255) + Colors[rgbF2][1] * (255-(currFrame % 255)))/255, (Colors[rgbF3][2] * (currFrame % 255) + Colors[rgbF2][2] * (255-(currFrame % 255)))/255, 0);
                    break;
                    
                case 2:
                    setAllLEDs((Colors[rgbF1][0] * (currFrame % 255) + Colors[rgbF3][0] * (255-(currFrame % 255)))/255, (Colors[rgbF1][1] * (currFrame % 255) + Colors[rgbF3][1] * (255-(currFrame % 255)))/255, (Colors[rgbF1][2] * (currFrame % 255) + Colors[rgbF3][2] * (255-(currFrame % 255)))/255, 0);
                    break;
            }
            generationPending = 1;
            break;
            
        case 2:         //all LEDS switching  R-G-B
            currFrame++;
            if(currFrame >= 3) currFrame = 0;
            setAllLEDs(Colors[rgbS[currFrame]][0], Colors[rgbS[currFrame]][1], Colors[rgbS[currFrame]][2], 0);
            generationPending = 1;
            break;
            
        case 3:         //all LEDS switching R-RG-G-GB-B-BR
            currFrame++;
            if(currFrame >= 6) currFrame = 0;
            setAllLEDs(Colors[rgbES[currFrame]][0], Colors[rgbES[currFrame]][1], Colors[rgbES[currFrame]][2], 0);
            generationPending = 1;
            break;
            
        case 4:;         //Color wave with variable width of each color R-G-B
            currFrame += 1;
            if(currFrame >= animParam1*3) currFrame = 0;
            uint16_t currOffset = 0;
            for(;currOffset < 300; currOffset ++){
                switch(((currFrame + currOffset)%(animParam1*3)) / animParam1){
                    case 0:
                        setLed(currOffset, (Colors[rgbF2][0] * ((currFrame + currOffset) % animParam1) + Colors[rgbF1][0] * (animParam1-((currFrame + currOffset) % animParam1)))/animParam1, (Colors[rgbF2][1] * ((currFrame + currOffset) % animParam1) + Colors[rgbF1][1] * (animParam1-((currFrame + currOffset) % animParam1)))/animParam1, (Colors[rgbF2][2] * ((currFrame + currOffset) % animParam1) + Colors[rgbF1][2] * (animParam1-((currFrame + currOffset) % animParam1)))/animParam1, 0);
                        break;

                    case 1:
                        setLed(currOffset, (Colors[rgbF3][0] * ((currFrame + currOffset) % animParam1) + Colors[rgbF2][0] * (animParam1-((currFrame + currOffset) % animParam1)))/animParam1, (Colors[rgbF3][1] * ((currFrame + currOffset) % animParam1) + Colors[rgbF2][1] * (animParam1-((currFrame + currOffset) % animParam1)))/animParam1, (Colors[rgbF3][2] * ((currFrame + currOffset) % animParam1) + Colors[rgbF2][2] * (animParam1-((currFrame + currOffset) % animParam1)))/animParam1, 0);
                        break;

                    case 2:
                        setLed(currOffset, (Colors[rgbF1][0] * ((currFrame + currOffset) % animParam1) + Colors[rgbF3][0] * (animParam1-((currFrame + currOffset) % animParam1)))/animParam1, (Colors[rgbF1][1] * ((currFrame + currOffset) % animParam1) + Colors[rgbF3][1] * (animParam1-((currFrame + currOffset) % animParam1)))/animParam1, (Colors[rgbF1][2] * ((currFrame + currOffset) % animParam1) + Colors[rgbF3][2] * (animParam1-((currFrame + currOffset) % animParam1)))/animParam1, 0);
                        break;
                }
            }
            
            generationPending = 1;
            break;
            
        case 5:         //Color wave with variable width of each color R-RG-G-GB-B-BR
            currFrame ++;
            if(currFrame >= 214) currFrame = 0;
            
            setAllLEDs(0, 0, 0, 0);
            
            uint8_t currLED = 0;
            
            if(currFrame + animParam1 > stripLength){   //we are at the end of the strip and need to show something on the beginning
                for(currLED = 0; currLED < (animParam1 + currFrame) - stripLength; currLED ++){
                    setLed(currLED, currColor[0], currColor[1], currColor[2], currColor[3]); 
                }
            }
            
            for(currLED = 0;currLED < animParam1;currLED++){
                if(currFrame+currLED > stripLength) break;
                setLed(currFrame + currLED, currColor[0], currColor[1], currColor[2], currColor[3]);
            }
            
            
            generationPending = 1;
            break;
    }
}

//sets the animation frame to the given one, if newFrame > frameCount[animation] -> the new frame will be % frameCount[animation]
void setFrame(uint16_t newFrame){
    currFrame = newFrame % frameCount[currAnimation];
}

//sets a different animation and resets the currFrame to 0
void setCurrAnimation(uint8_t animNum){
    if(animNum == currAnimation) return;
    if(animNum == 0){
        currAnimation = 0;
        setAllLEDs(currColor[0], currColor[1], currColor[2], currColor[3]);
        generationPending = 1;
        //setFPS(1);
    }
    if(animNum < numAnimations){
        currFrame = 0;
        currAnimation = animNum;
    }
}

void setFPS(uint8_t FPS){
    currFPS = FPS;
    if(FPS == 0) return;
    if(!T4CONbits.ON) T4CONbits.ON = 1;
    PR4 = (735 * (255 - FPS));
    TMR4 = 0;
}