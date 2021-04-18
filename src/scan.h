/*
scan.h

provide functions to set up hardware

Copyright (C) 2019 Henrik Björkman www.eit.se/hb.
All rights reserved etc etc...

History

2016-09-25
Created.
Henrik Bjorkman

*/

#ifndef SCAN_H
#define SCAN_H

#include "cfg.h"



#include "timerDev.h"

// Preferably at least 400 Hz or so since the intermediate trafo will be rated at 400 Hz.
#define MIN_FREQUENCY 400
#define MAX_FREQUENCY 2000

#define TRANSLATE_FREQUENCY_TO_HALF_PERIOD(f) (SysClockFrequencyHz / (2*(f)))

// TODO Duplicate code, see also measuring_convertToHz.
#define TRANSLATE_HALF_PERIOD_TO_FREQUENCY(hp) (SysClockFrequencyHz / (2*(hp)))


#define MIN_HALF_PERIOD TRANSLATE_FREQUENCY_TO_HALF_PERIOD(MAX_FREQUENCY)
#define MAX_HALF_PERIOD TRANSLATE_FREQUENCY_TO_HALF_PERIOD(MIN_FREQUENCY)


#define START_FREQUENCY MIN_MAX_RANGE(ee.scanBegin_Hz, MIN_FREQUENCY, MAX_FREQUENCY)
#define STOP_FREQUENCY MIN_MAX_RANGE(ee.scanEnd_Hz, MIN_FREQUENCY, MAX_FREQUENCY)

// Scan between these periods
#define START_HALF_PERIOD TRANSLATE_FREQUENCY_TO_HALF_PERIOD(START_FREQUENCY)
#define STOP_HALF_PERIOD TRANSLATE_FREQUENCY_TO_HALF_PERIOD(STOP_FREQUENCY)


extern uint32_t scanPeakWidthInPeriodTime;
extern uint32_t scanQ;



void scan_init();

void scanMediumTick(void);

//char scanIsDone(void);

//uint32_t scanGetHalfPeriod(void); // Use ports_get_period() instead.
uint32_t scanGetResonanceHalfPeriod(void);
void scanSetResonanceHalfPeriod(uint32_t);

char scanGetState(void);
char scanIsStarting(void);
char scanIsScanning(void);
char scanIsNormalOperation(void);
char scanIdleOrPauseEtc(void);
int scanIsAtPercent(int requiredPercent);

PORTS_STATUS_SUB_MESSAGES scanGetStateName(void);
uint32_t scanGetDutyCyclePpm();

//int32_t scanGetWantedBuckVoltage_mV();
//void scanPowerDownCommand();
void scanSecondsTick(void);
void scanBreak();

#endif
