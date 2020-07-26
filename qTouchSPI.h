#include<xc.h>
#include<stdint.h>

struct RE0{
    unsigned CTL : 1;
    unsigned PM : 1;
    unsigned LPS : 1;
    unsigned KeyError : 1;
    unsigned SlideError : 1;
    unsigned KeyCal : 1;
    unsigned SlideCal : 1;
};

typedef struct{
    unsigned Color : 1;
    unsigned Scene2 : 1;
    unsigned CTRL : 1;
    unsigned Fade : 1;
    unsigned Scene1 : 1;
    unsigned Custom : 1;
    unsigned Power : 1;
    unsigned SliderTouched : 1;
} Buttons;

union{
   Buttons buttons;
   uint8_t integer;
} UButton;

union{
   struct RE0 re;
   uint8_t integer;
} URE0;

uint8_t sliderPos;

void initSPI(uint32_t freq);

void updateQTouch();

Buttons getButtons();

uint8_t getSliderPos();

void sendData(uint8_t data);