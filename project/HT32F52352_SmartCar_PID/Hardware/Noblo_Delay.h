// File: delay.h
#ifndef __NOBLO_DELAY_H
#define __NOBLO_DELAY_H


#include "ht32.h"
#include "BFTM0.h"

typedef struct 
{
    vu32 start_time;
    vu32 delay_ms;
    uint8_t is_running;
} DelayTimer;

void Delay_Init(void);
uint8_t Delay_NonBlocking(DelayTimer *timer, vu32 delay_ms);

#endif
