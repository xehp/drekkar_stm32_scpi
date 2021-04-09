/*
fan.c

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...



History

2017-01-27
Created for dielectric test equipment
Henrik

*/

#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include "timerDev.h"

#include "cfg.h"

#if (defined FAN1_APIN) || (defined INTERLOCKING_LOOP_PIN)
#include "stm32l4xx.h"
#include "stm32l432xx.h"
#include "stm32l4xx_nucleo_32.h"
#include "stm32l4xx_ll_adc.h"
#include "machineState.h"
#endif

#if (defined TEMP1_ADC_CHANNEL) || (defined TEMP2_ADC_CHANNEL)
#include "systemInit.h"
#include "adcDev.h"
#include "serialDev.h"
#endif

#include "debugLog.h"
#include "translator.h"
#include "portsGpio.h"
#include "eeprom.h"
#include "messageNames.h"
#include "messageUtilities.h"
#include "log.h"
#include "fan.h"


typedef enum{
	INITIAL_STATE = 0,
	NOT_OK_STATE = 1,
	OK_STATE = 2
} TempOrFanStateEnum;



#ifdef USE_LPTMR2_FOR_FAN2
static int16_t fan2Hz = 0;
static int16_t prevFan2Count = 0;
static int fan2State = 0;
#endif




#ifdef FAN1_APIN
static int fan1Count=0;
static int prevLoggedFan1Hz = -1;
static int fan1Hz=0;
#endif



#if (defined USE_LPTMR2_FOR_FAN2)
static inline void fan2Init()
{
	lptmr2Init();
	prevFan2Count = 0;
	fan2State = INITIAL_STATE;
	fan2Hz=0;
}
#endif

#ifdef FAN1_APIN
/**
 * TODO Fan1 could use lptmr1
 */
static void fan1Init()
{
	#ifdef USE_LPTMR1_FOR_FAN1
	// lptmr1 will typically count pulses on port PB5 (AKA PB_5)
	lptmr1Init();
	#error this is not tested.
	#else
	fan1Hz=0;
	portsGpioExtiInitA();
	#endif
	fan1Count=0;
	prevLoggedFan1Hz = -1;
}
#endif


#ifdef INTERLOCKING_LOOP_PIN
#ifndef INTERLOCKING_LOOP_PORT
#error
#endif

#define readInputPin(port, pin) (((port->IDR) >> (pin)) & 1)

static void inputPinPullUpInit(GPIO_TypeDef* port, int pin)
{
	// MODER
	// 00: Input mode
	// 01: General purpose output mode
	// 10: Alternate function mode
	// 11: Analog mode (reset state)
	const uint32_t m = 0U; // 0 for "General purpose input mode", 1 for "General purpose output mode".
	port->MODER &= ~(3U << (pin*2)); // Clear the 2 mode bits for this pin.
	port->MODER |= (m << (pin*2));

	// OTYPER
	// 0: Output push-pull (reset state)
	// 1: Output open-drain
	//port->OTYPER &= ~(0x1U << (pin*1)); // Clear output type, 0 for "Output push-pull".

	const uint32_t s = 1U; // 1 for "Medium speed" or 2 for High speed.
	port->OSPEEDR &= ~(0x3U << (pin*2)); // Clear the 2 mode bits for this pin.
	port->OSPEEDR |= (s << (pin*2));

	// Pull up/Pull down
	// Clear the 2 pull up/down bits.
        // 0: for No pull-up, pull-down, 
	// 1: for pull up.
	const uint32_t p = 1U;
	port->PUPDR &= ~(3U << (pin*2));
	port->PUPDR |= (p << (pin*2));
}


static void interlockingLoopInit()
{
	inputPinPullUpInit(INTERLOCKING_LOOP_PORT, INTERLOCKING_LOOP_PIN);
}

int interlockingLoopOk()
{
	return readInputPin(INTERLOCKING_LOOP_PORT, INTERLOCKING_LOOP_PIN) == 0;
}

#endif

void fanInit()
{
	logInt1(FAN_INIT);

	#ifdef USE_LPTMR2_FOR_FAN2
	fan2Init();
	#endif

	#ifdef FAN1_APIN
	fan1Init();
	#endif

	#ifdef INTERLOCKING_LOOP_PIN
	interlockingLoopInit();
	#endif
}

// Changed this from 40 to 20, since using a very slow (and quiet) fan.
#define MIN_FAN 20

#define MAX_FAN 400

#ifdef USE_LPTMR2_FOR_FAN2

// To be called once every second.
static inline void fan2MainSecondsTick()
{
  const int tmp = lptmr2GetCount();
  switch (fan2State)
  {
    default: 
    case INITIAL_STATE: 
      fan2Hz = 0;
      fan2State = NOT_OK_STATE;
      break;
	case NOT_OK_STATE:
	{
      fan2Hz = tmp - prevFan2Count;
	  if ((fan2Hz > MIN_FAN) && (fan2Hz < MAX_FAN))
	  {
	  	fan2State = OK_STATE;
	  }
      break;
	}
	case OK_STATE:
	{
      fan2Hz = tmp - prevFan2Count;
	  if ((fan2Hz < MIN_FAN) || (fan2Hz > MAX_FAN))
	  {
		logInt1(FAN_2_NOT_DETECTED);
	  	fan2State = NOT_OK_STATE;
	  }
      break;
	}
  }
  prevFan2Count = tmp;
}
#endif


#ifdef FAN1_APIN
static void fan1MainSecondsTick()
{
	int tmp = portsGpioGetCounterA();
	fan1Hz = tmp-fan1Count;
	const int diff = fan1Hz != prevLoggedFan1Hz;
	if ((diff >1) || (diff < 1))
	{
		logInt2(FAN_AND_TEMP_COUNT_FAN2, fan1Hz);
		prevLoggedFan1Hz = fan1Hz;
	}
	fan1Count=tmp;
}

int fan1Ok()
{
	return (fan1Hz>=20) && (fan1Hz<=200);
}

int fanGetFan1Measurement_Hz()
{
	return fan1Hz;
}
#endif


void fanMainSecondsTick()
{
	#ifdef USE_LPTMR2_FOR_FAN2
	fan2MainSecondsTick();
	#endif
	#ifdef FAN1_APIN
	fan1MainSecondsTick();
	if (!fan1Ok())
	{
		logInt1(FAN_1_NOT_DETECTED);
	}
	#endif
}


#ifdef USE_LPTMR2_FOR_FAN2
int fan2Ok()
{
	return (fan2State == OK_STATE);
}

int fanGetFan2Measurement_Hz()
{
  return fan2Hz;
}
#endif


/**
 * Return true if OK
 */
int fansOK()
{
#ifdef USE_LPTMR2_FOR_FAN2
	if (!fan2Ok())
	{
		return 0;
	}
#endif
#ifdef FAN1_APIN
	if (!fan1Ok())
	{
		return 0;
	}
#endif
#ifdef INTERLOCKING_LOOP_PIN
	if (!interlockingLoopOk())
	{
		return 0;
	}
#endif
  return 1;
}










