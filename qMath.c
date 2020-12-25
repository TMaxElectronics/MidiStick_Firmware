#include <stdint.h>

#define SIN_MAX 1000000
#define SIN_PI 314159
const int32_t sinCoefficients[] = {0, SIN_MAX, 0, -(SIN_MAX/6), 0, SIN_MAX/120, 0, -(SIN_MAX/5040)};

int32_t qSin(int32_t x){
    if(x == 0) return 0;
    if(x < 0) x = -x;
    
    x %= SIN_PI;
    
    uint8_t deg = sizeof(sinCoefficients) / sizeof(sinCoefficients[0]);
    uint8_t c = 0;
    int32_t ret = 0;
    for(;c < deg; c++){
        
    }
}