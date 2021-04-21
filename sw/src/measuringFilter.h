/*
measuringFilter.h

provide functions to measure (AC) current.

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2019-04-16
Created.
Henrik Bjorkman
*/


#ifndef MEASURING_FILTER_H
#define MEASURING_FILTER_H

#include <inttypes.h>
#include "cfg.h"

// To be called once before any other measuring function is called.
void measuring_init(void);

// To be called regularly (as often as possible) from the main loop.
void measuring_process_fast_tick(void);

//uint32_t measuringGetFilteredCurrentValue_adcunits(void);
uint32_t measuringGetFilteredDcCurrentValue_mA(void);

//uint32_t measuringGetFilteredOutVolt_adcunits(void);
uint32_t measuringGetFilteredInternalDcVoltValue_mV(void);

#ifdef INV_OUTPUT_AC_CURRENT_CHANNEL
uint32_t measuringGetFilteredOutputAcCurrentValue_mA(void);
#endif

int measuringGetAcCurrentMax_mA(void);
int measuringGetDcCurrentMax_mA(void);
int measuringGetDcVoltMax_mV(void);

int measuringIsReady(void);

void measuringSlowTick();

#endif
