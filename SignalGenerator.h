#include <xc.h>
#include <stdint.h>

#include "MidiController.h"

#define SIGGEN_MAX_OTLIMIT 1000 
#define SIGGEN_MAX_MAXOT 400 
#define SIGGEN_MIN_MINOT 5          //we need this to prvent the note off timer from firing while we are still in the note on interrupt
#define SIGGEN_MIN_HOLDOFF 0 

void SigGen_init();

//set the timer period register to the appropriate value for the required frequency. To allow for smooth pitch bend commands the frequency is in 1/10th Hz
void SigGen_setNoteTPR(uint8_t voice, uint16_t freqTenths);

inline void SigGen_timerHandler(uint8_t voice);

void SigGen_setOTLimit(uint16_t limit);

void SigGen_setMaxOT(uint16_t max);

void SigGen_setMinOT(uint16_t min);

void SigGen_setHoldOff(uint16_t ho);

void SigGen_limit();