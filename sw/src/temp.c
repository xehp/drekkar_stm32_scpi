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
#include "debugLog.h"
#include "messageNames.h"
#include "log.h"


#if (defined TEMP1_ADC_CHANNEL) || (defined TEMP2_ADC_CHANNEL) || (defined USE_LPTMR1_FOR_TEMP1)

/*#if (defined FAN1_APIN) || (defined INTERLOCKING_LOOP_PIN)
#include "stm32l4xx.h"
#include "stm32l432xx.h"
#include "stm32l4xx_nucleo_32.h"
#include "stm32l4xx_ll_adc.h"
#include "machineState.h"
#endif*/

#include "systemInit.h"
#include "adcDev.h"
#include "serialDev.h"

#include "translator.h"
#include "portsGpio.h"
#include "eeprom.h"
#include "messageUtilities.h"
#include "temp.h"


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

#ifdef TEMP2_ADC_CHANNEL
static int temperature2State = 0;
static int16_t prevTemp2Count = 0;
static int32_t temp2C = 0;
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

// The ADC is initialized by adcDev. Just make sure the channel is included
// among those that will be sampled (adcChannelSequence).
static inline void temp2Init()
{
	prevTemp2Count = 0;
	temperature2State = INITIAL_STATE;
}

// The ADC is initialized by adcDev. Just make sure the channel is included
// among those that will be sampled (adcChannelSequence).
static void temp1Init()
{
    #ifdef USE_LPTMR1_FOR_TEMP1
	// lptmr1 will typically count pulses on port PB5 (AKA PB_5)
	lptmr1Init();
	#endif
	prevTemp1Count = 0;
	temperature1State = INITIAL_STATE;
}






#ifdef USE_LPTMR1_FOR_TEMP1
static inline void temp1MainSecondsTick()
{
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
}
#endif

#if (defined TEMP1_ADC_CHANNEL) || (defined TEMP2_ADC_CHANNEL)
static inline void tempSendMainSecondsTick()
{
	const int n1 = adc1GetNOfSamples(TEMP1_ADC_CHANNEL);
	const int n2 = adc1GetNOfSamples(TEMP2_ADC_CHANNEL);

	if ((n1 != prevTemp1Count) && (n2 != prevTemp2Count))
	{
		const uint32_t v1 = adc1GetSample(TEMP1_ADC_CHANNEL);
		const uint32_t v2 = adc1GetSample(TEMP2_ADC_CHANNEL);
		#if 0
		if ((v1<0x20) && (v2<0x20))
		{
			// No temp sensors seems to be connected.
			debug_print("No temp sensors ");
			debug_print_hex_16(v1);
			debug_print(" ");
			debug_print_hex_16(v2);
			debug_print("\n");
			temperature1State = NOT_OK_STATE;
			temperature2State = NOT_OK_STATE;
		}
		else
		{
			temp1C = translateAdcToC(v1);
			temp2C = translateAdcToC(v2);
			if (temperature1State == OK_STATE)
			{
				sendTempMessage(temp1C, temp2C);
			}
			else
			{
				temperature1State = OK_STATE;
				temperature2State = OK_STATE;
			}
		}
		#else
		temp1C = translateAdcToC(v1);
		temp2C = translateAdcToC(v2);
		sendTempMessage(temp1C, temp2C);
		#endif
		prevTemp1Count = n1;
		prevTemp2Count = n2;
	}
	else
	{
		debug_print("No temp value\n");
		temperature1State = NOT_OK_STATE;
		temperature2State = NOT_OK_STATE;
	}
}
#else
#error
#endif





int tempGetTemp1Measurement_C()
{
	return temp1C;
}

int temp1Ok()
{
	return (temperature1State == OK_STATE);
}


int temp2Ok()
{
	return (temperature2State == OK_STATE);
}

int tempGetTemp2Measurement_C()
{
  return temp2C;
}

#endif

/**
 * Fan and temp sensor inputs use counters:
 * Fan uses lptmr1
 * Temp uses lptmr2
 *
 * (Current drawing have it the other way around but will change that.)
 */
void tempInit()
{
	logInt1(TEMP_INIT);

	#if (defined TEMP1_ADC_CHANNEL) || (defined USE_LPTMR1_FOR_TEMP1)
	temp1Init();
	#endif

	#ifdef TEMP2_ADC_CHANNEL
	temp2Init();
	#endif
}



void tempMainSecondsTick()
{
	#ifdef USE_LPTMR1_FOR_TEMP1
	temp1MainSecondsTick();
	#endif

	#if (defined TEMP1_ADC_CHANNEL) || (defined TEMP2_ADC_CHANNEL) || (defined USE_LPTMR1_FOR_TEMP1)
	tempSendMainSecondsTick();
	#endif
}


/**
 * Return true if OK
 */
int tempOK()
{
	#if (defined TEMP1_ADC_CHANNEL) || (defined USE_LPTMR1_FOR_TEMP1)
	if (!temp1Ok())
	{
		return 0;
	}
	#endif

	#ifdef TEMP2_ADC_CHANNEL
	if (!temp2Ok())
	{
		return 0;
	}
	#endif

	return 1;
}


