/*
temp.h

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...



History

2017-01-27
Created for dielectric test equipment
Henrik

*/

#ifndef TEMP_H
#define TEMP_H

//#include <stdlib.h>
//#include <string.h>
#include <ctype.h>
#include <inttypes.h>



#define TEMP_MEASURING


// returns zero if some temp has failed, non zero if OK.
int tempOK(void);

#if (defined USE_LPTMR1_FOR_TEMP1) || (defined TEMP1_ADC_CHANNEL)
int temp1Ok(void);
int tempGetTemp1Measurement_C(void);
#endif


#ifdef TEMP2_ADC_CHANNEL
int temp2Ok(void);
int tempGetTemp2Measurement_C(void);
#endif



void tempInit(void);
void tempMainSecondsTick(void);


#endif
