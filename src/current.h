/*
current.h

Copyright (C) 2021 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...



History

2021-04-06
Created for dielectric test equipment
Henrik Bjorkman
*/

#ifndef CURRENT_H
#define CURRENT_H

//#include <stdlib.h>
//#include <string.h>
#include <ctype.h>
#include <inttypes.h>





void currentInit();
void currentMediumTick();
void currentSecondsTick();

int64_t currentGetAcCurrent_mA();

#endif
