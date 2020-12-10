#include <stdint.h>
#include <xc.h>

#include "ADSREngine.h"
#include "VMSRoutines.h"

const VMS_BLOCK IDLE = {
    .behavior = INVERTED,
    .offBlock = &SNAPON2,
    .target = onTime,
    .targetFactor = 0,
    .type = VMS_JUMP,
    .thresholdDirection = NONE,
    .period = 1000
};

const VMS_BLOCK SNAPON2 = {
    .nextBlocks[0] = &ATTAC,
    .nextBlocks[1] = &PORTAMENTO,
    .offBlock = &IDLE2,
    .behavior = NORMAL,
    
    .thresholdDirection = RISING,
    .target = onTime,
    .targetFactor = 0,
    .type = VMS_JUMP
};

const VMS_BLOCK PORTAMENTO = {
    .nextBlocks[0] = &FMOD,
    .behavior = NORMAL,
    
    .thresholdDirection = ANY,
    .target = frequency,
    .targetFactor = 1000000,
    .type = VMS_LIN,
    .param1 = pTime
};

const VMS_BLOCK ATTAC = {
    .nextBlocks[0] = &SUSTAIN,
    //.nextBlocks[1] = &NATTAC,
    .offBlock = &RELEASE,
    .behavior = NORMAL,
    
    .thresholdDirection = RISING,
    .target = onTime,
    .targetFactor = 1000000,
    .type = VMS_LIN,
    .param1 = 5000,
    .period = 1000
};

const VMS_BLOCK DECAY = {
    .nextBlocks[0] = &SUSTAIN,
    .offBlock = &RELEASE,
    .behavior = NORMAL,
    
    .thresholdDirection = FALLING,
    .target = onTime,
    .targetFactor = 750000,
    .type = VMS_LIN,
    .param1 = -1000,
    .period = 1000
};

const VMS_BLOCK NATTAC = {
    .nextBlocks[0] = &NSUS,
    .behavior = NORMAL,
    .offBlock = &NRELEASE,
    
    .thresholdDirection = RISING,
    .target = noise,
    .targetFactor = 100000,
    .type = VMS_LIN,
    .param1 = 1000,
    .period = 1000
};

const VMS_BLOCK NSUS = {
    .offBlock = &NRELEASE,
    .behavior = NORMAL,
    
    .thresholdDirection = NONE,
    .target = noise,
    .targetFactor = 100000,
    .type = VMS_JUMP,
    .period = 10000
};

const VMS_BLOCK NRELEASE = {
    .behavior = INVERTED,
    
    .thresholdDirection = FALLING,
    .target = noise,
    .targetFactor = 0,
    .type = VMS_LIN,
    .param1 = -10000,
    .period = 1000
};

const VMS_BLOCK SUSTAIN = {
    .nextBlocks[0] = VMS_DIE,
    .offBlock = &RELEASE,
    .behavior = NORMAL,
    
    .thresholdDirection = RISING,
    .target = onTime,
    .targetFactor = 750000,
    .type = VMS_JUMP
};

const VMS_BLOCK FMOD = {
    .behavior = NORMAL,
    
    .thresholdDirection = NONE,
    .target = frequency,
    .type = VMS_SIN,
    .param1 = 20,
    .param2 = 1000000,
    .param3 = 4,
    .period = 1000
};

const VMS_BLOCK RELEASE = {
    .nextBlocks[0] = &IDLE,
    .offBlock = &IDLE,
    .behavior = INVERTED,
    
    .thresholdDirection = FALLING,
    .target = onTime,
    .targetFactor = 10000,
    .type = VMS_EXP,
    .param1 = 993,
    .period = 1000
};

const VMS_BLOCK IDLE2 = {
    .behavior = INVERTED,
    .offBlock = &SNAPON,
    
    .target = onTime,
    .targetFactor = 0,
    .type = VMS_JUMP,
    .thresholdDirection = NONE,
    .period = 1000
};

const VMS_BLOCK SNAPON = {
    //.nextBlocks[1] = &ADECAY,
    .nextBlocks[0] = &FDECAY,
    .offBlock = &IDLE2,
    .behavior = NORMAL,
    
    .thresholdDirection = RISING,
    .target = onTime,
    .targetFactor = 1000000,
    .type = VMS_JUMP
};

const VMS_BLOCK SNAPFREQ = {
    .nextBlocks[0] = &ADECAY,
    //.nextBlocks[0] = &FDECAY,
    .offBlock = &IDLE2,
    .behavior = NORMAL,
    
    .thresholdDirection = RISING,
    .target = frequency,
    .targetFactor = 1000000,
    .type = VMS_JUMP
};

const VMS_BLOCK ADECAY = {
    .nextBlocks[0] = (VMS_BLOCK*) VMS_DIE,
    .behavior = NORMAL,
    .offBlock = &IDLE2,
    
    .thresholdDirection = FALLING,
    .target = frequency,
    .targetFactor = 1000000,
    .type = VMS_EXP,
    .param1 = 998,
    .period = 1000
};

const VMS_BLOCK FDECAY = {
    .behavior = NORMAL,
    .offBlock = &IDLE2,
    
    .thresholdDirection = NONE,
    .target = frequency,
    .type = VMS_SIN,
    .param1 = 50,
    .param2 = 1000000,
    .param3 = 4,
    .period = 1000
};



const VMS_BLOCK BD_IDLE = {
    .behavior = INVERTED,
    .offBlock = &BD_snapOn,
    
    .target = onTime,
    .targetFactor = 0,
    .type = VMS_JUMP,
    .thresholdDirection = NONE,
    .period = 1000
};

const VMS_BLOCK BD_snapOn = {
    .nextBlocks[0] = &BD_FDecay,
    .nextBlocks[1] = &BD_ADecay,
    .behavior = NORMAL,
    .offBlock = &BD_IDLE,
    
    .target = onTime,
    .targetFactor = 1000000,
    .type = VMS_JUMP,
    .thresholdDirection = RISING,
    .period = 1000
};

const VMS_BLOCK BD_FDecay = {
    .behavior = NORMAL,
    
    .thresholdDirection = FALLING,
    .targetFactor = 300000,
    .target = frequency,
    .type = VMS_EXP,
    .param1 = 990,
    .period = 1000
};

const VMS_BLOCK BD_ADecay = {
    .behavior = NORMAL,
    .offBlock = &BD_IDLE,
    
    .thresholdDirection = FALLING,
    .targetFactor = 0,
    .target = onTime,
    .type = VMS_LIN,
    .param1 = -10000,
    .period = 1000
};

const VMS_BLOCK DEF_IDLE = {
    .behavior = INVERTED,
    .offBlock = &DEF_ON,
    
    .target = onTime,
    .targetFactor = 0,
    .type = VMS_JUMP,
    .thresholdDirection = NONE,
    .period = 1000
};

const VMS_BLOCK DEF_ON = {
    .behavior = NORMAL,
    .offBlock = &DEF_IDLE,
    
    .target = onTime,
    .targetFactor = 1000000,
    .type = VMS_JUMP,
    .thresholdDirection = NONE,
    .period = 1000
};