/*
ports.h

provide functions to set up hardware

Copyright (C) 2019 Henrik Björkman www.eit.se/hb.
All rights reserved etc etc...

History

2016-09-10
Created.
Henrik Bjorkman

*/

// TODO rename this to portsPwm?


#ifndef PORTS_H
#define PORTS_H

#include <inttypes.h>

#include "cfg.h"


#include "messageNames.h"

// Don't try to set min angle to less than 50 since then ISR will struggle to separate the two interrupts.
// max angle gives min power.
// Min angle is also restricted by the inverter board which can not handle more than 99% duty cycle
// before it considers the signal to be faulty and stops the inverter.
#define PORTS_OFF_ANGLE 0xFFFFFFFFUL
#define PORTS_MIN_ANGLE (ee.portsPwmMinAngle)


extern int portsPwmState;


void portsDeactivateBreak();
int portsIsBreakActive();
int portsGetPwmBrakeState();

uint32_t ports_get_period(void); // Half period in our application.
uint32_t portsGetAngle();

int32_t portsGetCycleCount(void);

void portsBreak();


void portsSetHalfPeriod(uint32_t periodTime_t1);
void portsSetAngle(uint32_t angle_t1);
//void portsSetFullDutyCycle();
void portsSetDutyOff();

char portsIsActive(void);
uint32_t portsGetHalfCycleTimeout(void);

void portsPwmInit(void);
//void portsMediumTick(); // typically milli seconds
void portsSecondsTick();


#endif
