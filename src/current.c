/*
current.c

Copyright (C) 2021 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...


Our current sensor measure top current (during a cycle) not RMS or even mean value,
so don't expect to much from it.


History

2021-04-06
Created for dielectric test equipment
Henrik Bjorkman
*/


#include "cfg.h"
#include "current.h"

#ifdef CURRENT_ADC_CHANNEL

#include "adcDev.h"
#include "debugLog.h"
#include "log.h"
#include "messageNames.h"
#include "messageUtilities.h"
#include "systemInit.h"
#include "translator.h"

typedef enum{
	INITIAL_STATE = 0,
	NOT_OK_STATE = 1,
	OK_STATE = 2
} TempOrFanStateEnum;

#define MAX_TIME_BETWEEN_MESSAGES_MS 1000LL
#define NOICE_LEVEL_MA 5LL

static int prevCount = 0;
static int64_t current_mA = 0;
static int64_t lastSentFilter_mA = 0;
static int64_t lastSent_ms = 0;

static void sendCurrentMessage(int cur_mA)
{
	// printing in ascii was used for debugging, can be removed later.
	// This should typically be sent on usart2 (USB)
	debug_print("current ");
	debug_print64(cur_mA);
	debug_print(" mA\n");

	// This will typically be sent on usart1 (opto link)
	// This needs to match the unpacking in externalLeakSensorProcessMsg.
	// If changes are made here check if that needs to be changed also.
	messageInitAndAddCategoryAndSender(&messageDbfTmpBuffer, STATUS_CATEGORY);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, LEAK_CURRENT_STATUS_MSG);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, systemGetSysTimeMs());
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, cur_mA);
	messageSendDbf(&messageDbfTmpBuffer);
}


void currentInit()
{
	// The ADC is initialized by adcDev. Just make sure the channel is included
	// among those that will be sampled (adcChannelSequence).
}

void currentMediumTick()
{
	const int n = adc1GetNOfSamples(CURRENT_ADC_CHANNEL);

	if (n != prevCount)
	{
		uint32_t v = adc1GetSample(CURRENT_ADC_CHANNEL);
		current_mA = translatorConvertFromAdcUnitsToMilliAmpsAc(v);

		const int64_t currentTime = systemGetSysTimeMs();
		const int64_t timeSinceLast = currentTime - lastSent_ms;

		if ((current_mA > (lastSentFilter_mA * 2LL + NOICE_LEVEL_MA)) || ((timeSinceLast-MAX_TIME_BETWEEN_MESSAGES_MS) > 0))
		{
			sendCurrentMessage(current_mA);
			lastSent_ms = currentTime;
			lastSentFilter_mA = current_mA;
		}
		else
		{
			lastSentFilter_mA = (lastSentFilter_mA*999LL) / 1000LL;
		}

		prevCount = n;
	}
}

void currentSecondsTick()
{
}

int64_t currentGetAcCurrent_mA()
{
	return current_mA;
}


#endif
