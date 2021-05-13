/*
timerDev.h

Copyright (C) 2021 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...
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
