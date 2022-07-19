
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <xc.h>
#include <cp0defs.h>

#include "FIR_Bandpass.h"

static uint32_t shiftTime = 0;
static uint32_t filterTime = 0;

static void updateCoefficients(FIR_HANDLE_t * handle){
    if(handle == 0) return;
    UART_print("calculating new Coefficients\r\n");
    //FIR filter coefficients are the odd coefficients of the fourier transformed filter function
    //
    //b_0 = f2/fs - f1/fs
    //we want a bandpass (gain = 1 for f1<f<f2, 0 otherwise) so the integral simplifies to:
    //b_n = -(2/(pi*n)) * (cos(n*pi*(f2/fs)) - cos(n*pi*(f1/fs)));
    
    //coefficients are stored as 16,384'ths, allowing for maximum values of 1+1/16383 for the coefficient and 32767 for the value to be filtered
    
    float f1Frac = (float) ((float) handle->f1 / (float) handle->fs);
    float f2Frac = (float) ((float) handle->f2 / (float) handle->fs);
    
    uint32_t i = 0;
    for(i = 0; i < handle->elementCount; i++){
        uint32_t n = i+1;
        
        float constant = (float) - (float) ((float) 2 / (M_PI * (float) n));
        float cosVal1 = (float) n * 2.0 * M_PI * f1Frac;
        float cosVal2 = (float) n * 2.0 * M_PI * f2Frac;
        float c = constant * (cos(cosVal2)  - cos(cosVal1));
        //UART_print("f1Frac = %.5f, f2Frac = %.5f, constant = %.5f, cosVal1 = %.5f, cosVal2 = %.5f => ", f1Frac, f2Frac, constant, cosVal1, cosVal2);
        
        if(c > 1.999) c = 1.999;
        if(c < -1.999) c = -1.999;
        handle->coefficients[i] = (int32_t) ((float) 0x8000 * c);
        //UART_print("c[%d] = %.5f = %d\r\n", i, c, handle->coefficients[i]);
    }
    
    UART_print("coefficients done!\r\n");
    
    //clear any previous data from the buffer
    memset(handle->buffStart, 0, sizeof(int32_t) * handle->elementCount);
}   


static void shift(FIR_HANDLE_t * handle, int32_t newVal){
    if(handle == 0) return;
    handle->buff-=1;
    
    if(handle->buff < handle->buffStart) handle->buff = handle->buffEnd;
    
    handle->buff[0] = newVal;
}

static int32_t getBufferValue(FIR_HANDLE_t * handle, uint32_t index){
    if(handle == 0) return;
    int32_t* actualPtr = (int32_t*) ((uint32_t) handle->buff + (index*4));
    if(actualPtr > handle->buffEnd) actualPtr = actualPtr - (handle->elementCount*4);
    return *actualPtr;
}

FIR_HANDLE_t * FIR_create(uint32_t f1, uint32_t f2, uint32_t fs){
    FIR_HANDLE_t * ret = malloc(sizeof(FIR_HANDLE_t));
    
    ret->elementCount = FIR_ELEMENTS;
    ret->f1 = f1;
    ret->f2 = f2;
    ret->fs = fs;
    
    ret->buffStart = malloc(sizeof(int32_t) * ret->elementCount);
    ret->buff = ret->buffStart;
    ret->buffEnd = &(ret->buffStart[FIR_ELEMENTS-1]);
    ret->buffTopLimit = &(ret->buffStart[FIR_ELEMENTS]);
    ret->coefficients = malloc(sizeof(int32_t) * ret->elementCount);
    
    UART_print("start 0x%08x end 0x%08x\r\n", ret->buff, ret->buffEnd);
    UART_print("last timings: shift=%d filter=%d\r\n", shiftTime, filterTime);
    
    memset(ret->buff, 0, sizeof(uint32_t) * ret->elementCount);
    updateCoefficients(ret);
    
    return ret;
}

void FIR_dispose(FIR_HANDLE_t * handle){
    if(handle == 0) return;
    free(handle->buffStart);
    free(handle->coefficients);
    free(handle);
}

int32_t FIR_filter(FIR_HANDLE_t * handle, int32_t value){
    if(handle == 0) return;
    shift(handle, value);
   
    int32_t ret = 0;
    
    uint32_t n;
    int32_t* actualPtr = handle->buff;
    int32_t* cPtr = handle->coefficients;
    int32_t * limit = handle->buffTopLimit;
    
    for(n = 0; n < handle->elementCount; n++){
        ret += (*cPtr * (*actualPtr)) >> 15;
        
        actualPtr+=1;
        cPtr+=1;
        if(actualPtr == limit) actualPtr = handle->buffStart;
    }
    return ret;
}

void FIR_updateFilterConfig(FIR_HANDLE_t * handle, uint32_t f1, uint32_t f2){
    if(handle == 0) return;
    handle->f1 = f1;
    handle->f2 = f2;
    updateCoefficients(handle);
}