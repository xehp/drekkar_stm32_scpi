/*
exteralTemp.c

Copyright (C) 2021 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...


History
2021-04-12 Moved some code from fan.h to this new file. Henrik

*/





#include "systemInit.h"
#include "eeprom.h"
#include "log.h"
#include "machineState.h"
#include "externalTemp.h"

#define TEMP_VALUE_UNAVAILABLE -99

static int extTemp1C=TEMP_VALUE_UNAVAILABLE;
static int extTemp2C=TEMP_VALUE_UNAVAILABLE;
//static int extTempAmbientC=TEMP_VALUE_UNAVAILABLE;
static int64_t extTempTime_ms=0;
static int extTempState=EXT_TEMP_INITIAL;

static const int maxTempDiff_C = 5;

#define MY_ABS(a) (((a)>=0) ? (a) : (-(a)))




static void extTempChageState(int newState)
{

	logInt5(EXT_TEMP_STATE_CHANGE, extTempState, newState, extTemp1C, extTemp2C);
	//extTempAmbientC
	extTempState = newState;
}


// To be called once every second.
static inline void tempExtMainSecondsTick() {
	switch(extTempState)
	{
		case EXT_TEMP_INITIAL:
			// Do not check timeout yet.
			break;
		case EXT_TEMP_TIMEOUT:
			// Already in timeout state
			break;
		default:
		{
			int64_t t1 = systemGetSysTimeMs() - extTempTime_ms;
			if ((t1 < -5000) || (t1 > 5000))
			{
				extTempTime_ms = 0;
				extTempChageState(EXT_TEMP_TIMEOUT);
				extTemp1C = TEMP_VALUE_UNAVAILABLE;
				extTemp2C = TEMP_VALUE_UNAVAILABLE;

				// Will keep whatever ambient value we have since for now
				// we use the first reading of extTemp1C & extTemp2C as ambient.
				// Hopefully the coils are cold at that time and we only need
				// ambient to know that coils get a bit warm.
				//extTempAmbientC = TEMP_VALUE_UNAVAILABLE;

				if ((ee.optionsBitMask >> EXT_TEMP_REQUIRED_BIT) & 1)
				{
					errorReportError(portsExtTempFail);
				}
			}
		}
	}
}


// This needs to match the packing in sendTempMessage.
// If changes are made here check if that needs to be changed also.
CmdResult fanProcessExtTempStatusMsg(DbfUnserializer *dbfPacket)
{
	/*int64_t time_ms =*/ DbfUnserializerReadInt64(dbfPacket);
	extTemp1C = DbfUnserializerReadInt64(dbfPacket);
	extTemp2C = DbfUnserializerReadInt64(dbfPacket);

	#if 0
	// Was ambient temp also provided?
	if (DbfUnserializerReadIsNextInt(dbfPacket))
	{
		extTempAmbientC = DbfUnserializerReadInt64(dbfPacket);
	}
	else if (extTempAmbientC == TEMP_VALUE_UNAVAILABLE)
	{
		// If there is no ambient value then take the mean value of first
		// temp readings that we get. Hopefully coils are at room temperature
		// at start and that is what we need it for.
		extTempAmbientC = (extTemp1C + extTemp2C) / 2;
	}
	#endif

	extTempTime_ms = systemGetSysTimeMs();

	switch(extTempState)
	{
		default:
		case EXT_TEMP_INITIAL:
		case EXT_TEMP_TIMEOUT:
		{
			if ((extTemp1C < -40) || (extTemp1C > 250) || (extTemp2C < -40) || (extTemp2C > 250))
			{
				// Out of range
				extTempChageState(EXT_TEMP_OUT_OF_RANGE);
			}
			else if (MY_ABS(extTemp1C-extTemp2C) > maxTempDiff_C)
			{
				// To big difference between the two sensors.
				extTempChageState(EXT_TEMP_OUT_OF_RANGE);
			}
			else
			{
				extTempChageState(EXT_TEMP_READY);
			}
			break;
		}
		case EXT_TEMP_OUT_OF_RANGE:
		{
			if ((extTemp1C < -40) || (extTemp1C > 250) || (extTemp2C < -40) || (extTemp2C > 250))
			{
				// Out of range, so not OK (yet).
			}
			else if (MY_ABS(extTemp1C-extTemp2C) > ee.maxExtTempDiff_C)
			{
				// To big difference between the two sensors, so not OK (yet).
			}
			else
			{
				extTempChageState(EXT_TEMP_READY);
			}
			break;
		}
		case EXT_TEMP_READY:
		{
			if ((extTemp1C < -40) || (extTemp1C > 250) || (extTemp2C < -40) || (extTemp2C > 250))
			{
				// Out of range, so not OK (anymore).
				extTempChageState(EXT_TEMP_OUT_OF_RANGE);
			}
			else if (MY_ABS(extTemp1C-extTemp2C) > maxTempDiff_C)
			{
				// To big difference between the two sensors. One is probably faulty.
				extTempChageState(EXT_TEMP_OUT_OF_RANGE);
			}
			break;
		}
	}

	if ((extTemp1C>ee.maxExternalTemp_C) || (extTemp1C>ee.maxExternalTemp_C))
	{
		if ((ee.optionsBitMask >> EXT_TEMP_REQUIRED_BIT) & 1)
		{
			errorReportError(portsCoilsOverheated);
		}
	}


	// Don't echo this, it will be sent frequently, but we do forward it.
	return CMD_OK_FORWARD_ALSO;
}

int getExtTemp1C()
{
	return extTemp1C;
}

int getExtTemp2C()
{
	return extTemp2C;
}

/*
int getExtTempAmbientC()
{
	return extTempAmbientC;
}
*/

int ExtTempAvailableOrNotRequired()
{
	if (((ee.optionsBitMask >> EXT_TEMP_REQUIRED_BIT) & 1) == 0)
	{
		// Not required so OK.
		return 1;
	}

	return (extTempState == EXT_TEMP_READY);
}

int ExtTempOk()
{
	if (extTemp1C > ee.maxExternalTemp_C)
	{
		return 0;
	}

	if (extTemp2C > ee.maxExternalTemp_C)
	{
		return 0;
	}

	if (extTempState == EXT_TEMP_OUT_OF_RANGE)
	{
		return 0;
	}

	if (((ee.optionsBitMask >> EXT_TEMP_REQUIRED_BIT) & 1) == 0)
	{
		// Not required so OK.
		return 1;
	}

	if (extTempState != EXT_TEMP_READY)
	{
		// Not available so not OK.
		return 0;
	}

	return 1;
}

int getExtTempState()
{
	return extTempState;
}
