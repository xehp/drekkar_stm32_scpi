/*
measuringFilter.c

provide functions to measure (AC) current.

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2019-04-16
Created.
Henrik Bjorkman
*/

#include "adcDev.h"
#include "mathi.h"
#include "translator.h"
#include "log.h"
#include "measuringFilter.h"



// To be called once before any other measuring function is called.
void measuring_init(void)
{

}

#define MEASURING_FILTER_SCALE 100000


#define MEASURING_FILTER_TIME_CURRENT 1000

static const int64_t maxAllowedVoltageOverall_mV = MAX_INTERMITTENT_DC_VOLTAGE_MV;
static const int64_t maxAllowedAcCurrentOverall_mA = MAX_INTERMITTENT_AC_CURRENT_MA;
static const int64_t maxAllowedDcCurrentOverall_mA = MAX_INTERMITTENT_DC_CURRENT_MA;

static uint64_t measuringFilteredDcCurrentValueScaled = 0;
static int prevNOfDcCurrentSamples = 0;
static int prevPrevNOfDcCurrentSamples = 0;

#ifdef INV_OUTPUT_AC_CURRENT_CHANNEL
static uint64_t measuringFilteredOutputAcCurrentValueScaled = 0;
static int prevNOfOutputAcCurrentSamples = 0;
static int prevPrevNOfOutputAcCurrentSamples = 0;
static int prevPrevNOfOutputAcVoltSamples = 0;
#endif



#define MEASURING_FILTER_TIME_VOLTAGE 100
static uint64_t measuringFilteredInternalDcVoltValueScaled = 0;
static int prevNOfInternalDcVoltSamples = 0;
static int prevPrevNOfInternalDcVoltSamples = 0;


static int measuringState=0;
static uint32_t previousMaxDcCurrent=0;
static uint32_t previousMaxInternalDcVoltage=0;
static uint32_t loggedPreviousMaxInternalDcVoltage=0;
static uint32_t loggedPreviousMaxDcCurrent=0;

#ifdef INV_OUTPUT_AC_CURRENT_CHANNEL
static uint32_t previousMaxAcCurrent=0;
static uint32_t loggedPreviousMaxAcCurrent=0;
#endif


static void calcDcCurrentRollingMeanValue(uint32_t latestValue)
{
	// Calculate a rolling mean value
	const uint64_t latestValueScaled = latestValue*MEASURING_FILTER_SCALE;
	const uint64_t filteredChangeScaledXTime = (measuringFilteredDcCurrentValueScaled * (MEASURING_FILTER_TIME_CURRENT-1)) + latestValueScaled;
	measuringFilteredDcCurrentValueScaled = filteredChangeScaledXTime / MEASURING_FILTER_TIME_CURRENT;
}

#ifdef INV_OUTPUT_AC_CURRENT_CHANNEL
static void calcOutputAcCurrentRollingMeanValue(uint32_t latestValue)
{
	// Calculate a rolling mean value
	const uint64_t latestValueScaled = latestValue*MEASURING_FILTER_SCALE;
	const uint64_t filteredChangeScaledXTime = (measuringFilteredOutputAcCurrentValueScaled * (MEASURING_FILTER_TIME_CURRENT-1)) + latestValueScaled;
	measuringFilteredOutputAcCurrentValueScaled = filteredChangeScaledXTime / MEASURING_FILTER_TIME_CURRENT;
}

uint32_t measuringGetFilteredOutputAcCurrentValue_mA(void)
{
	// Multiply or divide first depends on how large value we have.
	const int64_t tmp = translatorConvertFromAdcUnitsToMilliAmpsAc(measuringFilteredOutputAcCurrentValueScaled);
	return DIV_ROUND(tmp, MEASURING_FILTER_SCALE);
}
#endif

// Returns current (internal units)
// Function measuring_convertFromAdcUnitsToMilliAmps can be used to get milli Amps
/*uint32_t measuringGetFilteredCurrentValue_adcunits(void)
{
	return DIV_ROUND(measuringFilteredCurrentValueScaled, MEASURING_FILTER_SCALE);
}*/

uint32_t measuringGetFilteredDcCurrentValue_mA(void)
{
	// Multiply or divide first depends on how large value we have.
	const int64_t tmp = translatorConvertFromAdcUnitsToMilliAmpsDc(measuringFilteredDcCurrentValueScaled);
	return DIV_ROUND(tmp, MEASURING_FILTER_SCALE);
}

int measuringGetDcCurrentCounter(void)
{
	return prevNOfDcCurrentSamples; // ignoring prevNOfOutVoltSamples for now
}

static void calcInternalDcVoltRollingMeanValue(uint32_t latestValue)
{
	// Calculate a rolling mean value
	const uint64_t latestValueScaled = latestValue*MEASURING_FILTER_SCALE;
	const uint64_t filteredChangeScaledXTime = (measuringFilteredInternalDcVoltValueScaled * (MEASURING_FILTER_TIME_VOLTAGE-1)) + latestValueScaled;
	measuringFilteredInternalDcVoltValueScaled = filteredChangeScaledXTime / MEASURING_FILTER_TIME_VOLTAGE;
}

uint32_t measuringGetFilteredInternalDcVoltValue_mV(void)
{
	// Multiply or divide first depends on how large value we have.
	const int64_t tmp = translatorConvertFromAdcUnitsToMilliVoltsDc(measuringFilteredInternalDcVoltValueScaled);
	return DIV_ROUND(tmp, MEASURING_FILTER_SCALE);
}


/*int measuringGetOutVoltCounter(void)
{
	return prevNOfOutVoltSamples;
}*/





// To be called regularly (as often as possible) from the main loop.
void measuring_process_fast_tick(void)
{
	// DC current
	{
		const int nc = adc1GetNOfSamples(INTERNAL_DC_CURRENT_CHANNEL);
		if (nc != prevNOfDcCurrentSamples)
		{
			uint32_t s = adc1GetSample(INTERNAL_DC_CURRENT_CHANNEL);

			if (s > previousMaxDcCurrent)
			{
				if (s > ADC_MAX)
				{
					if (errorGetReportedError() == portsErrorOk)
					{
						logInt3(MEASURING_OVER_CURRENT, s, ADC_MAX);
					}
					errorReportError(portsErrorOverDcCurrentFast);
				}
				previousMaxDcCurrent = s;
			}

			// TODO RMS, for now just mean value
			calcDcCurrentRollingMeanValue(s);

			prevNOfDcCurrentSamples = nc;
		}
	}

	// Internal DC voltage
	{
		const int nu = adc1GetNOfSamples(INTERNAL_DC_VOLTAGE_CHANNEL);
		if (nu != prevNOfInternalDcVoltSamples)
		{
			uint32_t s = adc1GetSample(INTERNAL_DC_VOLTAGE_CHANNEL);

			if (s > previousMaxInternalDcVoltage)
			{
				if (s > ADC_MAX)
				{
					if (errorGetReportedError() == portsErrorOk)
					{
						logInt3(MEASURING_OVER_VOLTAGE, s, ADC_MAX);
					}
					errorReportError(portsErrorOverVoltage);
				}
				previousMaxInternalDcVoltage = s;
			}

			calcInternalDcVoltRollingMeanValue(s);

			prevNOfInternalDcVoltSamples = nu;
		}
	}

	// Output AC current
	#ifdef INV_OUTPUT_AC_CURRENT_CHANNEL
	{
		const int nc = adc1GetNOfSamples(INV_OUTPUT_AC_CURRENT_CHANNEL);
		if (nc != prevNOfOutputAcCurrentSamples)
		{
			uint32_t s = adc1GetSample(INV_OUTPUT_AC_CURRENT_CHANNEL);

			if (s > previousMaxAcCurrent)
			{
				if (s > ADC_MAX)
				{
					if (errorGetReportedError() == portsErrorOk)
					{
						logInt3(MEASURING_OVER_AC_CURRENT, s, ADC_MAX);
					}
					errorReportError(portsErrorOverOutputAcCurrent);
				}
				previousMaxAcCurrent = s;
			}

			// TODO RMS, for now just mean value
			calcOutputAcCurrentRollingMeanValue(s);

			prevNOfOutputAcCurrentSamples = nc;
		}
	}
	#endif
}



int measuringIsReady(void)
{
	return measuringState == 1;
}

void measuringSlowTick()
{
	switch(measuringState)
	{
	case 0:
		if (prevNOfDcCurrentSamples == prevPrevNOfDcCurrentSamples)
		{
			// Waiting for current measurements
			logInt4(MEASURING_TIMEOUT, 1, prevNOfDcCurrentSamples, prevPrevNOfDcCurrentSamples);
		}
		else if (prevNOfInternalDcVoltSamples == prevPrevNOfInternalDcVoltSamples)
		{
			// Waiting for measurements
			logInt4(MEASURING_TIMEOUT, 2, prevNOfInternalDcVoltSamples, prevPrevNOfInternalDcVoltSamples);
		}
		#ifdef INV_OUTPUT_AC_CURRENT_CHANNEL
		else if (prevNOfOutputAcCurrentSamples == prevPrevNOfOutputAcCurrentSamples)
		{
			// Waiting for current measurements
			logInt4(MEASURING_TIMEOUT, 3, prevNOfOutputAcCurrentSamples, prevPrevNOfOutputAcVoltSamples);
		}
		#endif
		else
		{
			logInt1(MEASURING_ZERO_LEVEL_OK);
			measuringState = 1;
		}
		break;
	case 1:
		if (prevNOfDcCurrentSamples == prevPrevNOfDcCurrentSamples)
		{
			logInt4(MEASURING_TIMEOUT, 4, prevNOfDcCurrentSamples, prevPrevNOfDcCurrentSamples);
			errorReportError(portsErrorMeasuringTimeout);
			measuringState = 2;
		}
		else if (prevNOfInternalDcVoltSamples == prevPrevNOfInternalDcVoltSamples)
		{
			logInt4(MEASURING_TIMEOUT, 5, prevNOfInternalDcVoltSamples, prevPrevNOfInternalDcVoltSamples);
			errorReportError(portsErrorMeasuringTimeout);
			measuringState = 2;
		}
		#ifdef INV_OUTPUT_AC_CURRENT_CHANNEL
		else if (prevNOfOutputAcCurrentSamples == prevPrevNOfOutputAcCurrentSamples)
		{
			logInt4(MEASURING_TIMEOUT, 6, prevNOfOutputAcCurrentSamples, prevPrevNOfOutputAcVoltSamples);
			errorReportError(portsErrorMeasuringTimeout);
			measuringState = 2;
		}
		#endif

		break;
	default:
		break;
	}

	prevPrevNOfDcCurrentSamples = prevNOfDcCurrentSamples;
	prevPrevNOfInternalDcVoltSamples = prevNOfInternalDcVoltSamples;
	#ifdef INV_OUTPUT_AC_CURRENT_CHANNEL
	prevPrevNOfOutputAcCurrentSamples = prevNOfOutputAcCurrentSamples;
	#endif


	/*
	const int inputDiff = previousMaxInputVoltage - loggedPreviousMaxInputVoltage;
	if ((inputDiff*50)>loggedPreviousMaxInputVoltage)
	{
		const int64_t max_mV = translatorConvertFromAdcUnitsToMilliVolts(previousMaxInputVoltage);
		logInt2(MEASURING_TOP_INPUT_VOLTAGE_MV, max_mV);
		loggedPreviousMaxInputVoltage = previousMaxInputVoltage;

		if (max_mV > maxAllowedVoltageOverall_mV)
		{
			if (errorGetReportedError() == portsErrorOk)
			{
				logInt3(MEASURING_OVER_VOLTAGE_INPUT_MV, max_mV, maxAllowedVoltageOverall_mV);
			}
			errorReportError(portsErrorOverVoltage);
		}
	}
	*/

	const int internalDiff = previousMaxInternalDcVoltage - loggedPreviousMaxInternalDcVoltage;
	if ((internalDiff*50) > loggedPreviousMaxInternalDcVoltage)
	{
		const int64_t max_mV = translatorConvertFromAdcUnitsToMilliVoltsDc(previousMaxInternalDcVoltage);
		logInt2(MEASURING_TOP_INTERNAL_VOLTAGE_MV, max_mV);
		loggedPreviousMaxInternalDcVoltage = previousMaxInternalDcVoltage;
		if (max_mV > maxAllowedVoltageOverall_mV)
		{
			if (errorGetReportedError() == portsErrorOk)
			{
				logInt3(MEASURING_OVER_VOLTAGE_INTERNAL_MV, max_mV, maxAllowedVoltageOverall_mV);
			}
			errorReportError(portsErrorOverVoltage);
		}
	}

	const int currentDiff = previousMaxDcCurrent - loggedPreviousMaxDcCurrent;
	if ((currentDiff*25) > loggedPreviousMaxDcCurrent)
	{
		const int64_t max_mA = translatorConvertFromAdcUnitsToMilliAmpsDc(previousMaxDcCurrent);

		logInt2(MEASURING_TOP_CURRENT_MA, max_mA);
		if (max_mA > maxAllowedDcCurrentOverall_mA)
		{
			if (errorGetReportedError() == portsErrorOk)
			{
				logInt3(MEASURING_OVER_CURRENT_MA, max_mA, maxAllowedDcCurrentOverall_mA);
			}
			errorReportError(portsErrorOverDcCurrentSlow);
		}


		loggedPreviousMaxDcCurrent = previousMaxDcCurrent;
	}

	#ifdef INV_OUTPUT_AC_CURRENT_CHANNEL
	const int currentDiffAc = previousMaxAcCurrent - loggedPreviousMaxAcCurrent;
	if ((currentDiffAc*25) > loggedPreviousMaxAcCurrent)
	{
		const int64_t max_mA = translatorConvertFromAdcUnitsToMilliAmpsAc(previousMaxAcCurrent);

		logInt2(MEASURING_TOP_AC_CURRENT_MA, max_mA);
		if (max_mA > maxAllowedAcCurrentOverall_mA)
		{
			if (errorGetReportedError() == portsErrorOk)
			{
				logInt3(MEASURING_OVER_AC_CURRENT_MA, max_mA, maxAllowedAcCurrentOverall_mA);
			}
			errorReportError(portsErrorOverAcCurrentSlow);
		}


		loggedPreviousMaxAcCurrent = previousMaxAcCurrent;
	}
	#endif
}

int measuringGetAcCurrentMax_mA(void)
{
	const int64_t max_mA = translatorConvertFromAdcUnitsToMilliAmpsAc(previousMaxAcCurrent);
	return max_mA;
}

int measuringGetDcCurrentMax_mA(void)
{
	const int64_t max_mA = translatorConvertFromAdcUnitsToMilliAmpsDc(previousMaxDcCurrent);
	return max_mA;
}

int measuringGetDcVoltMax_mV(void)
{
	const int64_t max_mV = translatorConvertFromAdcUnitsToMilliVoltsDc(previousMaxInternalDcVoltage);
	return max_mV;
}

