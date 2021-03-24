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



#if (defined USE_LPTMR1_FOR_TEMP1) || (defined TEMP1_ADC_CHANNEL)
#ifdef USE_LPTMR1_FOR_TEMP1
static int16_t temp1Hz = 0;
#endif
static int temperature1State = 0;
static int16_t prevTemp1Count = 0;
#if (defined TEMP1_ADC_CHANNEL)
static int32_t temp1C = 0;
#endif
#endif

#ifdef USE_LPTMR2_FOR_FAN2
static int16_t fan2Hz = 0;
static int16_t prevFan2Count = 0;
static int fan2State = 0;
#endif

#if (defined USE_LPTMR2_FOR_TEMP2) || (defined TEMP2_ADC_CHANNEL)
#ifdef USE_LPTMR2_FOR_TEMP2
static int16_t temp2Hz = 0;
#endif
static int temperature2State = 0;
static int16_t prevTemp2Count = 0;
#if (defined TEMP2_ADC_CHANNEL)
static int32_t temp2C = 0;
#endif
#endif


#if (defined USE_LPTMR2_FOR_VOLTAGE2) || defined (VOLTAGE2_APIN)
static int16_t voltage2Hz = 0;
static int16_t prevVoltage2Count = 0;
static int voltage2State = INITIAL_STATE;
#endif




#ifdef FAN1_APIN
static int fan1Count=0;
static int prevLoggedFan1Hz = -1;
static int fan1Hz=0;
#endif


#ifdef TEMP_INTERNAL_ADC_CHANNEL
	int tempInternal_C;
#endif


// Measured when using 0.1 uF
//  25C   673Hz
//  30C   802Hz
//  90C  5050Hz
// 100C  6600Hz
// Its not linear but we don't need exact value between those temperatures so we use linear interpolation.
// Measured when using 1 uF
//  Temp(C)  Freq(Hz)
//  10       54
//  20       78
//  30      105
//  40      194
//  50      260
//  60      380
//  70      450
//  80      600
//  90      770
// 100     1000
// 110     1360
// 120     1700
/*int fanAndTempTranslateHzToCelcius(int freq)
{
	//const int32_t milliDegrees = ee.milliCOffset+(freq*ee.milliCPerHz);
	//const int32_t degrees = milliDegrees/1000;
	const int32_t tempDiff_20_100_C = 80;
	const int32_t freqAt20C = ee.freqAt20C;
	const int32_t freqAt100C = ee.freqAt100C;
	const int32_t f = freq;

	const int32_t degrees = 20L + ((f - freqAt20C) * tempDiff_20_100_C) / (freqAt100C - freqAt20C);
	return degrees;
}*/

// No use to compile this if LPTMR2 is used for more than one input.
#if (defined USE_LPTMR2_FOR_VOLTAGE2) && (defined USE_LPTMR2_FOR_TEMP2)
#error
#elif (defined USE_LPTMR2_FOR_FAN2) && (defined USE_LPTMR2_FOR_VOLTAGE2)
#error
#elif (defined USE_LPTMR2_FOR_FAN2) && (defined USE_LPTMR2_FOR_TEMP2)
#error
#endif


#if (defined TEMP1_ADC_CHANNEL) || (defined TEMP2_ADC_CHANNEL)
static void sendTempMessage(int temp1_C, int temp2_C)
{
	// printing in ascii was used for debugging, can be removed later.
	// This should typically be sent on usart2 (USB)
	debug_print("Temp ");
	debug_print64(temp1_C);
	debug_print(" ");
	debug_print64(temp2_C);
	debug_print("C\n");

	// This will typically be sent on usart1 (opto link)
	// This needs to match the unpacking in fanProcessExtTempStatusMsg.
	// If changes are made here check if that needs to be changed also.
	messageInitAndAddCategoryAndSender(&messageDbfTmpBuffer, STATUS_CATEGORY);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, TEMP_STATUS_MSG);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, systemGetSysTimeMs());
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, temp1_C);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, temp2_C);
#ifdef TEMP_INTERNAL_ADC_CHANNEL
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, tempInternal_C);
#endif
	messageSendDbf(&messageDbfTmpBuffer);
}
#endif



#if (defined USE_LPTMR2_FOR_FAN2)
static inline void fan2Init()
{
	lptmr2Init();
	prevFan2Count = 0;
	fan2State = INITIAL_STATE;
}
#endif

static inline void temp2Init()
{
	#ifdef USE_LPTMR2_FOR_TEMP2
	lptmr2Init();
	#endif
	prevTemp2Count = 0;
	temperature2State = INITIAL_STATE;
}

#if (defined USE_LPTMR2_FOR_VOLTAGE2)
static inline void voltage2Init()
{
	lptmr2Init();
	prevVoltage2Count = 0;
	voltage2State = INITIAL_STATE;
}
#endif


static void temp1Init()
{
    #ifdef USE_LPTMR1_FOR_TEMP1
	// lptmr1 will typically count pulses on port PB5 (AKA PB_5)
	lptmr1Init();
	prevTemp1Count = 0;
	temperature1State = INITIAL_STATE;
    #endif
}

#ifdef FAN1_APIN
static void fan1Init()
{
	portsGpioExtiInitA();
	fan1Count=0;
	prevLoggedFan1Hz = -1;
	fan1Hz=0;
}
#endif

#if (defined VOLTAGE2_APIN)
static inline void voltage2Init()
{
	portsGpioExtiInitA();
	prevVoltage2Count = 0;
	voltage2State = INITIAL_STATE;
}
#endif


#ifdef INTERLOCKING_LOOP_PIN

#ifndef INTERLOCKING_LOOP_PORT
#error
#endif

#if (INTERLOCKING_LOOP_PIN == 1) /*&& (INTERLOCKING_LOOP_PORT == GPIOB)*/
#if (defined USE_LPTMR2_FOR_VOLTAGE2) || (defined USE_LPTMR2_FOR_TEMP2) || (defined USE_LPTMR2_FOR_FAN2)
#error
#endif
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

/**
 * Fan and temp sensor inputs use counters:
 * Fan uses lptmr1
 * Temp uses lptmr2
 * 
 * (Current drawing have it the other way around but will change that.)
 */
void fanAndTempInit()
{
	logInt1(FAN_AND_TEMP_INIT);
	#ifdef USE_LPTMR2_FOR_FAN2
	fan2Init();
	#endif
	temp2Init();
	temp1Init();
	#ifdef FAN1_APIN
	fan1Init();
	#endif
	#ifdef INTERLOCKING_LOOP_PIN
	interlockingLoopInit();
	#endif
	#if (defined USE_LPTMR2_FOR_VOLTAGE2) || (defined VOLTAGE2_APIN)
	voltage2Init();
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

#if (defined USE_LPTMR2_FOR_TEMP2)
// To be called once every second.
static inline void temp2MainSecondsTick()
{
	const int16_t tmp = lptmr2GetCount();
	switch (temperature2State)
	{
		default:
		case INITIAL_STATE:
			temp2Hz = 0;
			temperature2State = NOT_OK_STATE;
			break;
		case NOT_OK_STATE:
		{
			temp2Hz = tmp - prevTemp2Count;
			if ((temp2Hz > ee.tempMinFreq) && (temp2Hz < ee.tempMaxFreq))
			{
				temperature2State = OK_STATE;
			}
			break;
		}
		case OK_STATE:
		{
			temp2Hz = tmp - prevTemp2Count;
			if (temp2Hz < ee.tempMinFreq)
			{
				logInt2(TEMP_SENSOR_FAIL, 2);
				temperature2State = NOT_OK_STATE;
			}
			if (temp2Hz > ee.tempMaxFreq)
			{
				logInt2(TEMP_SENSOR_OVERHEAT, 2);
				temperature2State = NOT_OK_STATE;
			}
			break;
		}
	}
	prevTemp2Count = tmp;
}
#endif


#if (defined USE_LPTMR2_FOR_VOLTAGE2) || (defined VOLTAGE2_APIN)
// To be called once every second.
static inline void voltage2MainSecondsTick()
{
	#if defined USE_LPTMR2_FOR_VOLTAGE2
	const int16_t tmp = lptmr2GetCount();
	#elif defined VOLTAGE2_APIN
	const int16_t tmp = portsGpioGetCounterA();
	#else
	#error
	#endif

	switch (voltage2State)
	{
		default:
		case INITIAL_STATE:
			voltage2Hz = 0;
			voltage2State = NOT_OK_STATE;
			break;
		case NOT_OK_STATE:
		{
			voltage2Hz = tmp - prevVoltage2Count;
			if ((voltage2Hz > ee.tempMinFreq) && (voltage2Hz < ee.tempMaxFreq))
			{
				voltage2State = OK_STATE;
			}
			break;
		}
		case OK_STATE:
		{
			voltage2Hz = tmp - prevVoltage2Count;
			if (voltage2Hz < ee.tempMinFreq)
			{
				logInt2(VOLTAGE_SENSOR_FAIL, 2);
				voltage2State = NOT_OK_STATE;
			}
			if (voltage2Hz > ee.tempMaxFreq)
			{
				logInt2(VOLTAGE_SENSOR_FAIL, 2);
				voltage2State = NOT_OK_STATE;
			}
			break;
		}
	}
	prevVoltage2Count = tmp;
}
#endif



static inline void temp1MainSecondsTick()
{
	#ifdef USE_LPTMR1_FOR_TEMP1
	int16_t tmp = lptmr1GetCount();
	switch (temperature1State)
	{
	default: 
	case INITIAL_STATE:
	  temp1Hz = 0;
	  temperature1State = NOT_OK_STATE;
	  break;
	case NOT_OK_STATE:
	{
	  temp1Hz = tmp - prevTemp1Count;
	  if ((temp1Hz > ee.tempMinFreq) && (temp1Hz < ee.tempMaxFreq))
	  {
		temperature1State = OK_STATE;
	  }
	  break;
	}
	case OK_STATE:
	{
	  temp1Hz = tmp - prevTemp1Count;
	  if (temp1Hz < ee.tempMinFreq)
	  {
		logInt2(TEMP_SENSOR_FAIL, 1);
		temperature1State = NOT_OK_STATE;
	  }
	  else if (temp1Hz > ee.tempMaxFreq)
	  {
		logInt2(TEMP_SENSOR_OVERHEAT, 1);
		temperature1State = NOT_OK_STATE;
	  }
	  break;
	}
	}
	prevTemp1Count = tmp;
	#endif
}

#if (defined TEMP1_ADC_CHANNEL) || (defined TEMP2_ADC_CHANNEL)
static inline void tempSendMainSecondsTick()
{
	const int n1 = adc1GetNOfSamples(TEMP1_ADC_CHANNEL);
	const int n2 = adc1GetNOfSamples(TEMP2_ADC_CHANNEL);

	if ((n1 != prevTemp1Count) && (n2 != prevTemp2Count)) {
		uint32_t v1 = adc1GetSample(TEMP1_ADC_CHANNEL);
		uint32_t v2 = adc1GetSample(TEMP2_ADC_CHANNEL);
		#ifdef TEMP_INTERNAL_ADC_CHANNEL
		uint32_t vi = adc1GetSample(TEMP_INTERNAL_ADC_CHANNEL);
		tempiC = translateInternalTempToC(vi);
		#endif

		temp1C = translateAdcToC(v1);
		temp2C = translateAdcToC(v2);
		sendTempMessage(temp1C, temp2C);
		prevTemp1Count = n1;
		prevTemp2Count = n2;
		temperature1State = OK_STATE;
		temperature2State = OK_STATE;
	}
	else
	{
		temperature1State = NOT_OK_STATE;
		temperature2State = NOT_OK_STATE;
	}
}
#else
#error
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

int fanAndTempGetFan1Measurement_Hz()
{
	return fan1Hz;
}
#endif


void fanAndTempMainSecondsTick()
{
	#ifdef USE_LPTMR2_FOR_FAN2
	fan2MainSecondsTick();
	#endif
	#ifdef USE_LPTMR2_FOR_TEMP2
	temp2MainSecondsTick();
	#endif
	temp1MainSecondsTick();
	#ifdef FAN1_APIN
	fan1MainSecondsTick();
	if (!fan1Ok())
	{
		logInt1(FAN_1_NOT_DETECTED);
	}
	#endif
	#if (defined USE_LPTMR2_FOR_VOLTAGE2) || (defined VOLTAGE2_APIN)
	voltage2MainSecondsTick();
	#endif
	tempSendMainSecondsTick();
}

#ifdef USE_LPTMR1_FOR_FAN1
int temp1Ok()
{
	return (temperature1State == OK_STATE);
}

int fanAndTempGetTemp1Measurement_Hz()
{
  return temp1Hz;
}

int fanAndTempGetTemp1Measurement_C()
{
	//return fanAndTempTranslateHzToCelcius(temp1Hz);
	return intervallHalvingFromFreqToC(temp1Hz);
}
#elif (defined TEMP1_ADC_CHANNEL)

int fanAndTempGetTemp1Measurement_C()
{
	//return fanAndTempTranslateHzToCelcius(temp1Hz);
	return temp1C;
}

#endif



#ifdef USE_LPTMR2_FOR_FAN2
int fan2Ok()
{
	return (fan2State == OK_STATE);
}

int fanAndTempGetFan2Measurement_Hz()
{
  return fan2Hz;
}

#endif

int temp2Ok()
{
	return (temperature2State == OK_STATE);
}

#ifdef USE_LPTMR2_FOR_TEMP2

int fanAndTempGetTemp2Measurement_Hz()
{
  return temp2Hz;
}

int fanAndTempGetTemp2Measurement_C()
{
  return fanAndTempTranslateHzToCelcius(temp2Hz);
}
#elif (defined TEMP2_ADC_CHANNEL)
int fanAndTempGetTemp2Measurement_C()
{
  return temp2C;
}
#endif


#if (defined USE_LPTMR2_FOR_VOLTAGE2) || (defined VOLTAGE2_APIN)
int voltage2Ok(void)
{
	return (voltage2State == OK_STATE);
}
int fanAndTempGetvoltage2Measurement_Hz(void)
{
	return voltage2Hz;
}
#endif


/**
 * Return true if OK
 */
int fansAndTempOK()
{
#ifdef USE_LPTMR1_FOR_TEMP1
	if (!temp1Ok())
	{
		return 0;
	}
#endif
#ifdef USE_LPTMR2_FOR_FAN2
	if (!fan2Ok())
	{
		return 0;
	}
#endif
#ifdef USE_LPTMR2_FOR_TEMP2
	if (!temp2Ok())
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










