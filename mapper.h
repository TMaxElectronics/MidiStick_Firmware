#include <xc.h>
#include <stdint.h>
#include "sys/attribs.h"

#define MAP_ENA_PITCHBEND   0x80
#define MAP_ENA_STEREO      0x40
#define MAP_ENA_VOLUME      0x20
#define MAP_ENA_DAMPER      0x10
#define MAP_ENA_PORTAMENTO  0x08
#define MAP_FREQ_MODE       0x01

#define FREQ_MODE_OFFSET    1
#define FREQ_MODE_FIXED     0

#define MAPPER_FLASHSIZE    16384
extern uint8_t MAPTABLE[];

void MAPPER_map(uint8_t voice, uint8_t note, uint8_t velocity, uint8_t channel);
MAPTABLE_HEADER * MAPPER_findHeader(uint8_t prog);
void MAPPER_programChangeHandler(uint8_t channel, uint8_t program);
MAPTABLE_DATA * MAPPER_getMap(uint8_t note, MAPTABLE_HEADER * table);
void MAPPER_bendHandler(uint8_t channel);
void MAPPER_volumeHandler(uint8_t channel);