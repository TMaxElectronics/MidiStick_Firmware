#include <xc.h>
#include <stdint.h>

uint8_t latOut[29400];
uint8_t led[300][4];

#define numAnimations 6
#define stripLength 214

#define rgbF1 0
#define rgbF2 2
#define rgbF3 4

uint8_t rgbS[3] = {0, 2, 4};
uint8_t rgbES[6] = {0, 1, 2, 3, 4, 5};

uint16_t currFrame = 0;
uint8_t currAnimation = 0;
uint16_t frameCount[numAnimations];
unsigned toggleState = 0; //state of led toggle button
volatile unsigned generationPending = 0;
uint8_t currFPS = 0;
volatile unsigned initDim = 1;

uint8_t currColor[4] = {255, 40, 0, 255};   //color of the LEDS with no animation
uint8_t animParam1 = 3;

uint8_t Colors[10][4] = {{255, 0, 0, 0}, {255, 255, 0, 0}, {0, 255, 0, 0}, {0, 255, 255, 0}, {0, 0, 255, 0}, {255, 0, 255, 0}, {255, 130, 255, 255}, {255, 50, 0, 255}, {0, 25, 255, 255}, {0, 0, 0, 0}}; //R, RG, G, GB, B, BR, FW, WW, CW


void initDriver();

void sendLEDData();

void generateOutputData();

void setLed(uint16_t LED, uint8_t r, uint8_t g, uint8_t b, uint8_t w);

void setAllLEDs(uint8_t r, uint8_t g, uint8_t b, uint8_t w);

void setAllLEDsA(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint16_t end);

void animationTask();

void setFrame(uint16_t newFrame);

void setCurrAnimation(uint8_t animNum);

void setFPS(uint8_t FPS);

void setMaster(uint8_t val);