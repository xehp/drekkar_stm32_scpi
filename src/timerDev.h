/*
Barebone blinky on nucleo L432KC / stm32l432kc using SysTick. No RTOS needed.
Copyright (C) Alexander & Henrik Bjorkman www.eit.se 2018
This may be used and redistributed as long as all copyright notices etc are retained .
http://www.eit.se/hb/misc/stm32/stm32l4xx/stm32_barebone/
*/

#ifndef TIMERDEV_H
#define TIMERDEV_H



#include <stdint.h>
#include <ctype.h>




// Low power timer 1 & 2 are used to count pulses from fan and temperature sensors.
void lptmr1Init();
uint16_t lptmr1GetCount();

void lptmr2Init();
uint16_t lptmr2GetCount();





#endif
