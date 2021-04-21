/*
fan.h

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...



History

2017-01-27
Created for dielectric test equipment
Henrik

*/

#ifndef FAN_H
#define FAN_H

//#include <stdlib.h>
//#include <string.h>
#include <ctype.h>
#include <inttypes.h>






// returns zero if fan or temp has failed, non zero if OK.
int fansOK(void);

#ifdef USE_LPTMR2_FOR_FAN2
int fan2Ok(void);
int fanGetFan2Measurement_Hz(void);
#endif

#ifdef FAN1_APIN
int fan1Ok();
int fanGetFan1Measurement_Hz();
#endif

void fanInit(void);
void fanMainSecondsTick(void);

//int fanAndTempTranslateHzToCelcius(int freq);

#ifdef INTERLOCKING_LOOP_PIN
int interlockingLoopOk();
#endif

#endif
