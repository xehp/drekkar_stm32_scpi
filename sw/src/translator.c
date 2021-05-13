/*
translator.c

Translate between internal units and SI units.

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.

History
2019-03-16 Created by Henrik Bjorkman

*/

#include "cfg.h"
#include "systemInit.h"
#include "mathi.h"
#include "eeprom.h"
#include "adcDev.h"
#include "translator.h"


#ifdef CURRENT_ADC_CHANNEL
int64_t translatorConvertFromAdcUnitsToMilliAmpsAc(int64_t currentInUnits)
{
	const int64_t uA = (int64_t)currentInUnits * (int64_t)ee.microAmpsPerUnitAc;
	return DIV_ROUND(uA, 1000);
}
#endif



#if (defined TEMP1_ADC_CHANNEL) || (defined TEMP2_ADC_CHANNEL)



static int32_t interpolate10(int low, int high, int offset)
{
	return (low*(10-offset) + high*(offset))/10;
}



// TODO Duplicate code, see also macro TRANSLATE_HALF_PERIOD_TO_FREQUENCY.
int32_t measuring_convertToHz(uint32_t halfPeriod)
{
	return DIV_ROUND(SysClockFrequencyHz,halfPeriod*2);
}




// The ADC sample represent a value between 0 and ADC_RANGE (0x10000).
// Depending on voltage divider R1 and R2.
// v = R2 / (R1+R2)
// Where temp sensor is R1 and R2 is 1k Ohm.
// The table
// https://www.tdk-electronics.tdk.com/inf/50/db/ntc/NTC_Probe_ass_M703.pdf
// give resistance as quota of nominal at 25C which is 10 kOhm
// So expressed in ADC Samples it is:
//
// sample = 65536 / ( quota * 10 + 1 )
//
#define SAMPLE_AT_N50C 98
#define SAMPLE_AT_N40C 194
#define SAMPLE_AT_N30C 368
#define SAMPLE_AT_N20C 668
#define SAMPLE_AT_N10C 1163
#define SAMPLE_AT_0C 1948
#define SAMPLE_AT_10C 3136
#define SAMPLE_AT_20C 4858
#define SAMPLE_AT_30C 7236
#define SAMPLE_AT_40C 10358
#define SAMPLE_AT_50C 14238
#define SAMPLE_AT_60C 18789
#define SAMPLE_AT_70C 23814
#define SAMPLE_AT_80C 29024
#define SAMPLE_AT_90C 34174
#define SAMPLE_AT_100C 39010
#define SAMPLE_AT_110C 43367
#define SAMPLE_AT_120C 48845
#define SAMPLE_AT_130C 50377
#define SAMPLE_AT_140C 51791
#define SAMPLE_AT_150C 55291

/**
 * This function will give the expected ADC value for a given temperature.
 */
static uint32_t interpolateCelciusToAdcSample(int32_t temp_C)
{
	if (temp_C<0)
	{
		temp_C = -temp_C;
		const int i = temp_C/10;
		const int o = temp_C%10;
		switch(i)
		{
		case 0:
			return interpolate10(SAMPLE_AT_0C, SAMPLE_AT_N10C, o);
		case 1:
			return interpolate10(SAMPLE_AT_N10C, SAMPLE_AT_N20C, o);
		case 2:
			return interpolate10(SAMPLE_AT_N20C, SAMPLE_AT_N30C, o);
		case 3:
			return interpolate10(SAMPLE_AT_N30C, SAMPLE_AT_N40C, o);
		case 4:
			return interpolate10(SAMPLE_AT_N40C, SAMPLE_AT_N50C, o);
		default:
			return interpolate10(SAMPLE_AT_N40C, SAMPLE_AT_N50C, temp_C-40);
		}
	}
	else
	{
		const int i = temp_C/10;
		const int o = temp_C%10;
		switch(i)
		{
		case 0:
			return interpolate10(SAMPLE_AT_0C, SAMPLE_AT_10C, o);
		case 1:
			return interpolate10(SAMPLE_AT_10C, SAMPLE_AT_20C, o);
		case 2:
			return interpolate10(SAMPLE_AT_20C, SAMPLE_AT_30C, o);
		case 3:
			return interpolate10(SAMPLE_AT_30C, SAMPLE_AT_40C, o);
		case 4:
			return interpolate10(SAMPLE_AT_40C, SAMPLE_AT_50C, o);
		case 5:
			return interpolate10(SAMPLE_AT_50C, SAMPLE_AT_60C, o);
		case 6:
			return interpolate10(SAMPLE_AT_60C, SAMPLE_AT_70C, o);
		case 7:
			return interpolate10(SAMPLE_AT_70C, SAMPLE_AT_80C, o);
		case 8:
			return interpolate10(SAMPLE_AT_80C, SAMPLE_AT_90C, o);
		case 9:
			return interpolate10(SAMPLE_AT_90C, SAMPLE_AT_100C, o);
		case 10:
			return interpolate10(SAMPLE_AT_100C, SAMPLE_AT_110C, o);
		case 11:
			return interpolate10(SAMPLE_AT_110C, SAMPLE_AT_120C, o);
		case 12:
			return interpolate10(SAMPLE_AT_120C, SAMPLE_AT_130C, o);
		case 13:
			return interpolate10(SAMPLE_AT_130C, SAMPLE_AT_140C, o);
		case 14:
			return interpolate10(SAMPLE_AT_140C, SAMPLE_AT_150C, o);
		default:
			return interpolate10(SAMPLE_AT_140C, SAMPLE_AT_150C, temp_C-140);
		}
	}
}


static int32_t intervallHalvingFromSampleToC(int32_t sampleValue)
{
	int32_t t=64;
	int32_t s=64;

	while(s>0)
	{
		const int f2=interpolateCelciusToAdcSample(t);

		if (f2<sampleValue)
		{
			t+=s;
		}
		else if (f2>sampleValue)
		{
			t-=s;
		}
		else
		{
			break;
		}
		s=s/2;
	}

	// One extra step since otherwise we got very few even numbers and a lot of odd ones
	// or was it the other way around?
	const int32_t f3=interpolateCelciusToAdcSample(t);
	if (f3<sampleValue)
	{
		t++;
	}

	return t;
}



int32_t translateAdcToC(uint32_t adcSample)
{
	return intervallHalvingFromSampleToC(adcSample);
}

#ifdef TEMP_INTERNAL_ADC_CHANNEL
int translateInternalTempToC(uint32_t adcSample)
{
	// TODO not implemented.
	// https://electronics.stackexchange.com/questions/324321/reading-internal-temperature-sensor-stm32
	return 20;
}
#endif

#endif

