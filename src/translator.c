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


int8_t translatorConvertionAvailable()
{
	return (ee.microAmpsPerUnitDc !=0) && (ee.microVoltsPerUnitDc !=0);
}


#ifdef INV_OUTPUT_AC_CURRENT_CHANNEL
int64_t translatorConvertFromAdcUnitsToMilliAmpsAc(int64_t currentInUnits)
{
	const int64_t uA = (int64_t)currentInUnits * (int64_t)ee.microAmpsPerUnitAc;
	return DIV_ROUND(uA, 1000);
}
#endif


int64_t translatorConvertFromAdcUnitsToMilliAmpsDc(int64_t currentInUnits)
{
	const int64_t uA = (int64_t)currentInUnits * (int64_t)ee.microAmpsPerUnitDc;
	return DIV_ROUND(uA, 1000);
}

/*
static int32_t measuring_convertToMicroVolts(uint16_t voltageInUnits)
{
	const int32_t uV = (uint32_t)voltageInUnits * ee.microVoltsPerUnit;
	return uV;
}
*/

static uint32_t convertFromMicroAmpsToAdcUnits(int64_t uA)
{
	uint64_t a = DIV_ROUND((uA), ee.microAmpsPerUnitDc);
	return MIN(a, uint32_tMaxValue);
}



uint32_t translatorConvertFromMilliAmpsToAdcUnitsDc(int32_t mA)
{
	return convertFromMicroAmpsToAdcUnits(((int64_t)mA)*1000LL);
}


int64_t translatorConvertFromAdcUnitsToMilliVoltsDc(int64_t voltageInUnits)
{
	if (voltageInUnits == NO_VOLT_VALUE)
	{
		return NO_VOLT_VALUE;
	}
	const int64_t probedMicrovoltsPerUnit = (int64_t)ee.microVoltsPerUnitDc;
	const int64_t uV = voltageInUnits * probedMicrovoltsPerUnit;
	return DIV_ROUND(uV, 1000L);
}

int32_t translatorConvertFromMilliVoltsToAdcUnitsDc(int32_t probeVolts)
{
	const int64_t probed_uV = ((int64_t)probeVolts) * 1000LL;
	const int64_t probedMicrovoltsPerUnit = (int64_t)ee.microVoltsPerUnitDc;
	const int64_t u = DIV_ROUND(probed_uV, probedMicrovoltsPerUnit);
	return MIN(u, 0x7FFFFFFF);
}

int32_t translatorConvertFromExternalToMilliVoltsAc(int32_t externalVoltageInAdcUnits)
{
	// TODO Not same translation as internal measurements.
	// For now assuming that external sensor give us voltage in mV or we use old way.
	return externalVoltageInAdcUnits;
	//return translatorConvertFromAdcUnitsToMilliVolts(externalVoltageInAdcUnits);
}



static int32_t interpolate10(int low, int high, int offset)
{
	return (low*(10-offset) + high*(offset))/10;
}

// Measured values
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
// Adjusted a little below
#define FREQ_AT_10C 46
#define FREQ_AT_20C 66
#define FREQ_AT_30C 93
#define FREQ_AT_40C 175
#define FREQ_AT_50C 252
#define FREQ_AT_60C 369
#define FREQ_AT_70C 445
#define FREQ_AT_80C 597
#define FREQ_AT_90C 769
#define FREQ_AT_100C 1000
#define FREQ_AT_110C 1360
#define FREQ_AT_120C 1700

static int32_t linearInterpolationCToFreqUnadj(int32_t temp)
{
	const int i = temp/10;
	const int o = temp%10;
	switch(i)
	{
	case 1:
		return interpolate10(FREQ_AT_10C, FREQ_AT_20C, o);
	case 2:
		return interpolate10(FREQ_AT_20C, FREQ_AT_30C, o);
	case 3:
		return interpolate10(FREQ_AT_30C, FREQ_AT_40C, o);
	case 4:
		return interpolate10(FREQ_AT_40C, FREQ_AT_50C, o);
	case 5:
		return interpolate10(FREQ_AT_50C, FREQ_AT_60C, o);
	case 6:
		return interpolate10(FREQ_AT_60C, FREQ_AT_70C, o);
	case 7:
		return interpolate10(FREQ_AT_70C, FREQ_AT_80C, o);
	case 8:
		return interpolate10(FREQ_AT_80C, FREQ_AT_90C, o);
	case 9:
		return interpolate10(FREQ_AT_90C, FREQ_AT_100C, o);
	case 10:
		return interpolate10(FREQ_AT_100C, FREQ_AT_110C, o);
	case 11:
		return interpolate10(FREQ_AT_110C, FREQ_AT_120C, o);
	default:
		if (temp<=10)
		{
			return interpolate10(FREQ_AT_10C, FREQ_AT_20C, temp-10);
		}
		else
		{
			return interpolate10(FREQ_AT_110C, FREQ_AT_120C, temp-110);
		}
	}
}

int32_t linearInterpolationCToFreq(int32_t temp)
{
	const int32_t f = linearInterpolationCToFreqUnadj(temp);


	// Adjustment factor such that if we read 61 Hz and
	// and our adjustment say 61 Hz is 20C then...
	// If temp is at 100C then that adjustment shall dominate.
	// TODO  Can probably remove this. Adjust linearInterpolationCToFreqUnadj instead.
	// If removing this then remove ee.freqAt20C and ee.freqAt100C also.
	const int32_t a = 100 - temp;
	const int32_t b = temp - 20;
	const int32_t c = (a * FREQ_AT_20C) + (b * FREQ_AT_100C);
	const int32_t d = (a * ee.freqAt20C) + (b * ee.freqAt100C);


	return (f*d) / c;
}


int32_t intervallHalvingFromFreqToC(int32_t freq)
{
	int32_t t=64;
	int32_t s=64;

	while(s>0)
	{
		const int f2=linearInterpolationCToFreq(t);

		if (f2<freq)
		{
			t+=s;
		}
		else if (f2>freq)
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
	const int32_t f3=linearInterpolationCToFreq(t);
	if (f3<freq)
	{
		t++;
	}

	return t;
}


// TODO Duplicate code, see also macro TRANSLATE_HALF_PERIOD_TO_FREQUENCY.
int32_t measuring_convertToHz(uint32_t halfPeriod)
{
	return DIV_ROUND(SysClockFrequencyHz,halfPeriod*2);
}



#if (defined TEMP1_ADC_CHANNEL) || (defined TEMP2_ADC_CHANNEL)

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

