/*
ports.h

provide functions to set up PWM hardware

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2019-09-10
Created.
Henrik Bjorkman

*/


#ifndef PWM1_H_
#define PWM1_H_


#include "cfg.h"

#if 0


#define PWM_PORT GPIOA
#define PWM_PIN 8


extern int pwm1State;

// Unit for the times here is 1/systFreq, typically 1/80000000 S.


void pwm_init();

uint32_t pwm_get_period_time(void);

void pwm_set_period_time(uint32_t period_time_t1);

// Range for duty cycle shall be 0 to pwm_get_period_time().
// 0 gives 0% duty cycle. To get near 100% give the value from pwm_get_period();
void pwm_set_duty_cycle(uint32_t puls_time_t1);

uint32_t pwm_get_duty_cycle(void);

void pwm_seconds_tick();

uint32_t pwm_getCounter();

int pwm_isBreakActive();

void pwm_break();
void pwm_clearBreak();

#endif
#endif
