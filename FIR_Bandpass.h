#ifndef FIR_INC
#define FIR_INC

#include <stdint.h>

typedef struct{
    int32_t * buff;
    int32_t * buffStart;
    int32_t * buffEnd;
    int32_t * buffTopLimit;
    int32_t * coefficients;
    uint32_t elementCount,f1,f2,fs;
} FIR_HANDLE_t;

#define FIR_ELEMENTS 100

FIR_HANDLE_t * FIR_create(uint32_t f1, uint32_t f2, uint32_t fs);
int32_t FIR_filter(FIR_HANDLE_t * handle, int32_t value);
void FIR_dispose(FIR_HANDLE_t * handle);
void FIR_updateFilterConfig(FIR_HANDLE_t * handle, uint32_t f1, uint32_t f2);

#endif