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



#define TEMP_MEASURING


// returns zero if fan or temp has failed, non zero if OK.
int fansAndTempOK(void);

#if (defined USE_LPTMR1_FOR_TEMP1) || (defined TEMP1_ADC_CHANNEL)
int temp1Ok(void);
int fanAndTempGetTemp1Measurement_C(void);
#if (defined USE_LPTMR1_FOR_TEMP1)
int fanAndTempGetTemp1Measurement_Hz(void);
#endif
#endif

#ifdef USE_LPTMR2_FOR_FAN2
int fan2Ok(void);
int fanAndTempGetFan2Measurement_Hz(void);
#endif

#if (defined USE_LPTMR2_FOR_TEMP2) || (defined TEMP2_ADC_CHANNEL)
int temp2Ok(void);
int fanAndTempGetTemp2Measurement_C(void);
#if (defined USE_LPTMR2_FOR_TEMP2)
//int fanAndTempGetTemp2Measurement_Hz(void);
#endif
#endif

#if (defined USE_LPTMR2_FOR_VOLTAGE2) || (defined VOLTAGE2_APIN)
int voltage2Ok(void);
int fanAndTempGetvoltage2Measurement_Hz(void);
#endif


#ifdef FAN1_APIN
int fan1Ok();
int fanAndTempGetFan1Measurement_Hz();
#endif

void fanAndTempInit(void);
void fanAndTempMainSecondsTick(void);

//int fanAndTempTranslateHzToCelcius(int freq);

#ifdef INTERLOCKING_LOOP_PIN
int interlockingLoopOk();
#endif

#endif
