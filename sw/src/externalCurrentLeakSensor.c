/*
externalCurrentLeakSensor.cpp


Copyright (C) 2021 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2021-04-06
Created, Some methods moved from externalSensor to this new file.
Henrik Bjorkman

*/

#include "eeprom.h"
#include "systemInit.h"
#include "log.h"
#include "machineState.h"
#include "externalCurrentLeakSensor.h"

#include "messageUtilities.h"

enum{
	leakStateInitial = 0,
	leakStateAvailable = 1,
	leakStateTimeout = 2,
};

#define LEAK_DETECTOR_TIMEOUT_MS 2200

static int64_t measuredExternalLeakCurrent_mV=NO_VALUE;
static int64_t measuredLeakCurrentTimeMs=0;
static int measuredLeakCurrentState = leakStateInitial;

CmdResult externalLeakSensorProcessMsg(DbfUnserializer *dbfPacket)
{
	// Sending code in voltageMeter looks like this:
	//DbfSerializerWriteInt64(&dbfSerializer, systime_ms);
	//DbfSerializerWriteInt64(&dbfSerializer, current_mA);

	/*time_ms =*/ DbfUnserializerReadInt64(dbfPacket);
	const int64_t tmp1 = DbfUnserializerReadInt64(dbfPacket);
	if ((tmp1 >=-1) && (tmp1 < 0x7FFFFFF))
	{
		// Value was approved.
		measuredExternalLeakCurrent_mV = tmp1;
		measuredLeakCurrentTimeMs = systemGetSysTimeMs();

		if ((ee.maxLeakCurrentStep_mA !=0) && (measuredExternalLeakCurrent_mV > ee.maxLeakCurrentStep_mA))
		{
			// Max allowed leak current was exceeded.
			errorReportError(portsErrorLeakCurrentExceeded);
		}

		// Run the state machine (parts of it).
		switch(measuredLeakCurrentState)
		{
			default:
			{
				logInt2(LEAK_DETECTOR_NOW_AVAILABLE, tmp1);
				measuredLeakCurrentState = leakStateAvailable;
				break;
			}
			case leakStateAvailable:
				// nothing, this state is handled in externalLeakSensorSecondsTick.
				break;
		}
	}
	else
	{
		measuredExternalLeakCurrent_mV = NO_VALUE;
	}

	// Don't echo this, it will be sent frequently
	return CMD_OK_FORWARD_ALSO;
}

int64_t getMeasuredLeakSensor_mA()
{
	return measuredExternalLeakCurrent_mV;
}

void externalLeakSensorSecondsTick()
{
	// Run the state machine (parts of it).
	switch(measuredLeakCurrentState)
	{
		case leakStateAvailable:
		{
			int64_t t = systemGetSysTimeMs();
			int64_t d = t - measuredLeakCurrentTimeMs;
			if ((d-LEAK_DETECTOR_TIMEOUT_MS) > 0)
			{
				// Timeout
				logInt2(LEAK_DETECTOR_TIMEOUT, measuredExternalLeakCurrent_mV);
				measuredLeakCurrentState = leakStateTimeout;
				measuredExternalLeakCurrent_mV = NO_VALUE;
			}
			break;
		}
		default:
			// nothing, these states are handled in externalLeakSensorProcessMsg.
			break;
	}
}
