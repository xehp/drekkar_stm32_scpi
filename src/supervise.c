/*
supervice.c

provide functions to check hardware

Copyright (C) 2019 Henrik Björkman www.eit.se/hb.
All rights reserved etc etc...

History

2016-09-22
Created.
Henrik Bjorkman

*/
#if (defined __linux__)
#include <stdio.h>
#include <unistd.h>
#endif


#include "systemInit.h"
#include "timerDev.h"


#include "eeprom.h"
#include "machineState.h"
#include "log.h"
#include "fan.h"
#include "temp.h"
#include "externalTemp.h"
#include "externalSensor.h"
#include "measuringFilter.h"
#include "portsPwm.h"
#include "scan.h"
#include "mathi.h"
#include "messageUtilities.h"
#include "supervise.h"

#if 1

enum
{
	SUP_INIT_STATE=0,
	SUP_STABILITY_STATE,
};



#define SDF ((uint32_t)ee.superviceDeviationFactor)
#define LESS_HELP(a) (((a)*(SDF-1))/SDF)
#define LESS(a, m) ((LESS_HELP(a)>m)?(LESS_HELP(a)-m):0)
#define MORE(a, m) ((((a)*(SDF+1))/SDF)+m)


// Wait some seconds before starting this supervision so that things are stable.
#define STABILITY_TIME_S (ee.stableOperationTimer_s)
int stableOperationTimer=0;
int64_t timeCounted_s=0; // Used to detect timeout in link to websuart.

const uint32_t marginDcCurrent_mA = 200;
const uint32_t marginAcCurrent_mA = 100;
const uint32_t marginDcVoltage_mV = 2000;

// We expect current to stay within limits given here.
static uint32_t superviseDcCurrentMin=0;
static uint32_t superviseDcCurrentMax=0;
static uint32_t superviseAcCurrentMin=0;
static uint32_t superviseAcCurrentMax=0;

static int superviseState=0;

static uint32_t prevWantedVoltage=0;
static uint32_t prevWantedCurrent=0;

static uint32_t superviceHalfPeriodMin =0;
static uint32_t superviceHalfPeriodMax =0;

static uint32_t superviseInternalVoltageMin=0;
static uint32_t superviseInternalVoltageMax=0;

int32_t previousCycleCount = 0;
uint8_t prevSuperviceDeviationFactor = 0;

static uint32_t prevMeasuredFilteredInternalVoltValue_mV = 0;
static int32_t superviseInternalVoltageEtc=0;
static uint32_t prevMeasuredFilteredAcCurrentValue_mV = 0;


static int64_t longTermInternalVoltage_mV = 0;
static int64_t longTermAcCurrent_mA = 0;

#if 0
static int maxExtTemp1C = -99;
static int maxExtTemp2C = -99;
#endif

static int32_t getInternalScanningVoltage_mV()
{
	return ee.wantedInternalScanningVoltage_mV;
}


static void sendTargetReachedLogMessage(int64_t cyclesPerformed, int64_t cyclesToDo)
{
	logInitAndAddHeader(&messageDbfTmpBuffer, CMD_TARGET_REACHED);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, cyclesPerformed);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, cyclesToDo);
	messageSendDbf(&messageDbfTmpBuffer);
}


static void enterIdleState()
{
	stableOperationTimer=STABILITY_TIME_S;
	superviseState=SUP_INIT_STATE;
}


static void enterSuperviceState()
{
	// Shall we even do supervision?
	if (SDF!=0)
	{
		// superviceDeviationFactor is not 0 so yes.

		const uint32_t measuredDcCurrent_mA = measuringGetFilteredDcCurrentValue_mA();
		superviseDcCurrentMin = LESS(measuredDcCurrent_mA, marginDcCurrent_mA);
		superviseDcCurrentMax = MORE(measuredDcCurrent_mA, marginDcCurrent_mA);

		const uint32_t measuredAcCurrent_mA = measuringGetFilteredOutputAcCurrentValue_mA();
		superviseAcCurrentMin = LESS(measuredAcCurrent_mA, marginAcCurrent_mA);
		superviseAcCurrentMax = MORE(measuredAcCurrent_mA, marginAcCurrent_mA);

		const uint32_t p = ports_get_period();
		superviceHalfPeriodMin = LESS(p, 1);
		superviceHalfPeriodMax = MORE(p, 1);

		const uint32_t measuredDcVoltage_mA = measuringGetFilteredInternalDcVoltValue_mV();
		superviseInternalVoltageMin = LESS(measuredDcVoltage_mA, marginDcVoltage_mV);
		superviseInternalVoltageMax = MORE(measuredDcVoltage_mA, marginDcVoltage_mV);
		// TODO measured AC current?

		resetMaxAllowed();

		// If we have been running this long we will want to save to flash when machine stops.
		eepromSetSavePending();

		logInt5(SUPERVICE_START, superviseAcCurrentMin, superviseAcCurrentMax, superviceHalfPeriodMin, superviceHalfPeriodMax);

		#if 0
		// stableOperationTimer is used to know when its time to check that we
		// see coils warming up. If that does not happen then perhaps the temperature
		// sensors might have fallen off the coils. So we want to log a warning about that.
		stableOperationTimer = 3600;
		#endif

		superviseState = SUP_STABILITY_STATE;
	}
	else
	{
		errorReportError(portsErrorAssert);
	}
}


int superviseIsTargetReached()
{
	return (cyclesToDo <= 0) ;
}


/*static int currentDrift(uint32_t measuredAcCurrent_mA)
{
	if (ee.superviceSlowDeviation == 0)
	{
		return 0;
	}

	const uint32_t margin_mA = 3000 + (prevMeasuredFilteredAcCurrentValue_mV / (ee.superviceSlowDeviation+1));

	if ((measuredAcCurrent_mA + margin_mA) < prevMeasuredFilteredAcCurrentValue_mV)
	{
		logInt5(SUPERVICE_CURRENT_DROP, 3, prevMeasuredFilteredAcCurrentValue_mV, measuredAcCurrent_mA, margin_mA);
		return 1;
	}

	return 0;
}*/

// A state machine that aims to detect if current drops unexpectedly. That would indicate some error.
// So we need to keep track of current current and what current should be.

void superviseCurrentSecondsTick2(void)
{
	const int32_t currentCycleCount = portsGetCycleCount();
	const int64_t inc = 999LL;

	if (scanIsAtPercent(95))
	{
		const int32_t cyclesPerformedDuringLastSecond = currentCycleCount - previousCycleCount;
		totalCyclesPerformed += cyclesPerformedDuringLastSecond;

		if (cyclesPerformedDuringLastSecond < cyclesToDo)
		{
			cyclesToDo -= cyclesPerformedDuringLastSecond;
		}
		else
		{
			// Target reached, stopping.
			sendTargetReachedLogMessage(cyclesPerformedDuringLastSecond, cyclesToDo);
			if (cyclesToDo!=0)
			{
				eepromSetSavePending();
				cyclesToDo = 0;
			}
			machineRequestPause();
		}

		for (int i=0; i<NDEV; ++i)
		{
			ee.reachedCycles[i]+=cyclesPerformedDuringLastSecond;
			if ((ee.targetCycles[i] != 0) && (ee.reachedCycles[i] > ee.targetCycles[i]))
			{
				sendTargetReachedLogMessage(cyclesPerformedDuringLastSecond, cyclesToDo);
				if (cyclesToDo!=0)
				{
					eepromSetSavePending();
					cyclesToDo = 0;
				}
				machineRequestPause();
			}
		}

	}
	previousCycleCount = currentCycleCount;

	// Check if there is a timeout in the status messages from WEB server.
	++timeCounted_s;
	if (timeCounted_s >= ee.webServerTimeout_s)
	{
		if (timeCounted_s!=0xFFFFFFFFu)
		{
			logInt2(SUPERVICE_TIMEOUT_STATUS_MSG, timeCounted_s);
			eepromSetSavePending();
			machineRequestPause();
			timeCounted_s=0xFFFFFFFFu;
		}
	}

	// Increment wallTimeReceived_ms
	// We expect to get regular updates from websuart so we increment time a little slow.
	ee.wallTimeReceived_ms += inc;
}

static void superviceInitState()
{
	const uint32_t wantedVoltage_mV = getWantedVoltage_mV();
	const uint32_t wantedCurrent_mA = getWantedCurrent_mA();

	if (errorGetReportedError() != portsErrorOk)
	{
		// Already in error.
		stableOperationTimer=STABILITY_TIME_S;
	}
	else if (prevWantedVoltage != wantedVoltage_mV)
	{
		// Wanted voltage has been changed, now we must wait for stability again.
		prevWantedVoltage=wantedVoltage_mV;
		stableOperationTimer=STABILITY_TIME_S;
	}
	else if (prevWantedCurrent!=wantedCurrent_mA)
	{
		// Wanted current has been changed, now we must wait for stability again.
		prevWantedCurrent = wantedCurrent_mA;
		stableOperationTimer=STABILITY_TIME_S;
	}
	else if (!scanIsNormalOperation())
	{
		// Waiting for SCAN to be ready.
		stableOperationTimer=STABILITY_TIME_S;
	}
	else if (prevSuperviceDeviationFactor != ee.superviceDeviationFactor)
	{
		// If this is changes then restart timer, wait for stability again.
		stableOperationTimer=STABILITY_TIME_S;
		prevSuperviceDeviationFactor = ee.superviceDeviationFactor;
	}
	else if (stableOperationTimer>0)
	{
		--stableOperationTimer;
	}
	else if ((SDF == 0) || (STABILITY_TIME_S == 0))
	{
		logInt3(SUPERVICE_NOT_ENABLED, SDF, STABILITY_TIME_S);
		enterIdleState();
		stableOperationTimer=150;
	}
	else
	{
		enterSuperviceState();
	}
}

/**
 * When the machine has been running a while and everything is warmed up not much
 * trimming shall be needed after that. If something changes it may be that something is wrong.
 **/
void supervice_stability()
{
	const uint32_t wantedVoltage_mV = getWantedVoltage_mV();
	const uint32_t wantedCurrent_mA = getWantedCurrent_mA();
	const uint32_t measuredDcVoltage_mV = measuringGetFilteredInternalDcVoltValue_mV();
	const uint32_t measuredAcCurrent_mA = measuringGetFilteredOutputAcCurrentValue_mA();
	const uint32_t measuredDcCurrent_mA = measuringGetFilteredDcCurrentValue_mA();

	if (errorGetReportedError() != portsErrorOk)
	{
		// An error has been reported so no more supervision is needed.
		enterIdleState();
	}
	else if (prevWantedVoltage != wantedVoltage_mV)
	{
		// Wanted voltage has been changed, now we must wait for stability again.
		prevWantedVoltage=wantedVoltage_mV;
		enterIdleState();
	}
	else if (prevWantedCurrent!=wantedCurrent_mA)
	{
		// Wanted current has been changed, now we must wait for stability again.
		prevWantedCurrent = wantedCurrent_mA;
		enterIdleState();
	}
	else if (machineGetRequestedState() != cmdRunCommand)
	{
		enterIdleState();
	}
	/*else if (!scanIsNormalOperation())
	{
		// Don't supervise when scan is doing something
	}*/
	else if (prevSuperviceDeviationFactor != ee.superviceDeviationFactor)
	{
		// superviceDeviationFactor has been changed so go to idle and start over.
		enterIdleState();
	}
	else if ((ee.superviceDeviationFactor==0) || (STABILITY_TIME_S == 0))
	{
		// No more supervision is wanted, go to idle.
		enterIdleState();
	}
	else if (measuredAcCurrent_mA > superviseAcCurrentMax)
	{
		// To much current
		logInt5(SUPERVICE_TO_MUCH_CURRENT, 4, measuredAcCurrent_mA, superviseAcCurrentMin, superviseAcCurrentMax);
		errorReportError(portsErrorCurrentMoreThanExpected);
		enterIdleState();
	}
	else if (measuredAcCurrent_mA < superviseAcCurrentMin)
	{
		// To little current, current has drifted away from what it was.
		logInt5(SUPERVICE_TO_LITTLE_CURRENT, 3, measuredAcCurrent_mA, superviseAcCurrentMin, superviseAcCurrentMax);
		errorReportError(portsErrorCurrentLessThanRequired);
		enterIdleState();
	}
	/*else if (currentDrift(measuredAcCurrent_mA))
	{
		// If current drops more than margin_mA over a second something is wrong.
		errorReportError(portsErrorUnexpectedCurrentDecrease);
	}*/
	else if (measuredDcCurrent_mA  < superviseDcCurrentMin)
	{
		// To little current
		logInt5(SUPERVICE_TO_LITTLE_CURRENT, 1, measuredDcCurrent_mA, superviseDcCurrentMin, superviseDcCurrentMax);
		errorReportError(portsErrorCurrentLessThanRequired);
		enterIdleState();
	}
	else if (measuredDcCurrent_mA > superviseDcCurrentMax)
	{
		// To much current
		logInt5(SUPERVICE_TO_MUCH_CURRENT, 2, measuredDcCurrent_mA, superviseDcCurrentMin, superviseDcCurrentMax);
		errorReportError(portsErrorCurrentMoreThanExpected);
		enterIdleState();
	}
	else if (ports_get_period()<superviceHalfPeriodMin)
	{
		logInt5(SUPERVICE_FREQUENCY_DRIFT, 1, ports_get_period(), superviceHalfPeriodMin, superviceHalfPeriodMax);
		errorReportError(portsErrorFrequencyDrift);
		enterIdleState();
	}
	else if (ports_get_period()>superviceHalfPeriodMax)
	{
		logInt5(SUPERVICE_FREQUENCY_DRIFT, 2, ports_get_period(), superviceHalfPeriodMin, superviceHalfPeriodMax);
		errorReportError(portsErrorFrequencyDrift);
		enterIdleState();
	}
	else if (measuredDcVoltage_mV  < superviseInternalVoltageMin)
	{
		logInt5(SUPERVICE_INTERNAL_VOLTAGE_DRIFT, 1, measuredDcVoltage_mV, superviseInternalVoltageMin, superviseInternalVoltageMax);
		errorReportError(portsErrorInternalVoltageDrift);
		enterIdleState();
	}
	else if (measuredDcVoltage_mV > superviseInternalVoltageMax)
	{
		logInt5(SUPERVICE_INTERNAL_VOLTAGE_DRIFT, 2, measuredDcVoltage_mV, superviseInternalVoltageMin, superviseInternalVoltageMax);
		errorReportError(portsErrorInternalVoltageDrift);
		enterIdleState();
	}
	#if 0
	else if (stableOperationTimer>0)
	{
		--stableOperationTimer;
	}
	else
	{
		const int margin_C = 2;
		const int a = getExtTempAmbientC() + margin_C;
		const int e1 = getExtTemp1C();
		const int e2 = getExtTemp2C();
		if ((e1 <= a) || (e2 <= a))
		{
			// The coils should be warm by now, perhaps temp sensors are not mounted?
			// Log a warning about it.
			logInt4(TEMPERATURE_NOT_RISING, getExtTemp1C(), getExtTemp2C(), getExtTempAmbientC());

			// Log again in 1 min (if still not rising).
			stableOperationTimer=60;
		}
		// Remember highest temperature
		if (e1 > maxExtTemp1C)
		{
			maxExtTemp1C = e1;
		}
		if (e2 > maxExtTemp2C)
		{
			maxExtTemp2C = e2;
		}

		// If temperature is falling then something might be wrong.
		if ((e1 + margin_C) < maxExtTemp1C)
		{
			logInt4(TEMPERATURE_HAS_FALLEN, getExtTemp1C(), getExtTemp2C(), getExtTempAmbientC());
			--maxExtTemp1C;
			// Log again in 1 min (if still falling).
			stableOperationTimer=60;
		}

		if ((e2 + margin_C) < maxExtTemp2C)
		{
			logInt4(TEMPERATURE_HAS_FALLEN, getExtTemp1C(), getExtTemp2C(), getExtTempAmbientC());
			--maxExtTemp2C;
			// Log again in 1 min (if still falling).
			stableOperationTimer=60;
		}
	}
	#endif
}

static void checkForErrors()
{
	const uint32_t measuredDcVoltage_mV = measuringGetFilteredInternalDcVoltValue_mV();
	const uint32_t measuredDcCurrent_mA = measuringGetFilteredDcCurrentValue_mA();

	// This is also checked in measuring filter so perhaps not needed here.
	if (measuredDcCurrent_mA > MAX_FILTERED_DC_CURRENT_MA)
	{
		logInt5(SUPERVICE_TO_MUCH_CURRENT, 5, measuredDcCurrent_mA, superviseDcCurrentMin, superviseDcCurrentMax);
		errorReportError(portsErrorCurrentMoreThanAllowed);
		return;
	}

	// This is also checked in measuring filter so perhaps not needed here.
	if (measuredDcVoltage_mV > MAX_FILTERED_DC_VOLTAGE_MV)
	{
		logInt6(SUPERVICE_TO_MUCH_VOLTAGE, 5, measuredDcVoltage_mV, superviseInternalVoltageMin, superviseInternalVoltageMax, MAX_FILTERED_DC_VOLTAGE_MV);
		errorReportError(portsErrorOverVoltage);
		return;
	}

	if (getExtTempState() == EXT_TEMP_OUT_OF_RANGE)
	{
		errorReportError(portsExtTempDiff);
		return;
	}

	if (superviseInternalVoltageEtc)
	{
		const uint32_t measuredFilteredInternalVoltValue_mV = measuringGetFilteredInternalDcVoltValue_mV();
		const uint32_t internalMargin_mV = ee.superviceMargin_mV;
		if ((measuredFilteredInternalVoltValue_mV  + internalMargin_mV) < getInternalScanningVoltage_mV())
		{
			// If this happens we assume input power is out. We need to turn off our input relay.
			// Its not an error with our machine.
			logInt5(SUPERVICE_INTERNAL_VOLTAGE_DROP, 2, measuredFilteredInternalVoltValue_mV, internalMargin_mV, getInternalScanningVoltage_mV());
			errorReportError(portsInputVoltageDecrease);
			return;
		}

		// TODO Perhaps we do not need this one since we check against longTermInternalVoltage_uV
		// in superviseCurrentMilliSecondsTick.
		if (ee.superviceDropFactor != 0)
		{
			// If internal voltage drops more than 5V + 4% (1/25) over a second something is wrong.
			// (assuming superviceDropFactor is 25)
			const uint32_t margin_mV = ee.superviceMargin_mV + (prevMeasuredFilteredInternalVoltValue_mV / ee.superviceDropFactor);
			if ((measuredDcVoltage_mV + margin_mV) < prevMeasuredFilteredInternalVoltValue_mV)
			{
				logInt5(SUPERVICE_INTERNAL_VOLTAGE_DROP, 1, prevMeasuredFilteredInternalVoltValue_mV, measuredDcVoltage_mV, margin_mV);
				errorReportError(portsInputVoltageDecrease);
				return;
			}
		}	
	}
	else
	{
		// superviseInternalVoltageEtc is not yet activated.
	}


	if (!temp1Ok())
	{
		errorReportError(portsErrorOverheated);
		return;
	}

	#ifdef USE_LPTMR2_FOR_TEMP2
	if (!temp2Ok())
	{
		errorReportError(portsErrorOverheated);
	}
	#endif

	if (!ExtTempOk())
	{
		errorReportError(portsCoilsOverheated);
		return;
	}

	#ifdef FAN1_APIN
	if (!fan1Ok())
	{
		errorReportError(portsErrorFanFail);
		return;
	}
	#endif

	#ifdef USE_LPTMR2_FOR_FAN2
	if (!fan2Ok())
	{
		errorReportError(portsErrorFanFail);
		return;
	}
	#endif

	#ifdef INTERLOCKING_LOOP_PIN
	if (!interlockingLoopOk())
	{
		// trigger a save to flash if this was the first/only error.
		if (errorGetReportedError()==portsErrorOk)
		{
			eepromSetSavePending();
		}
		errorReportError(portsErrorLoopOpen);
		return;
	}
	#endif

	if ((scanIsNormalOperation()) && (ee.min_mV_per_mA_ac != 0))
	{
		const int32_t measuredAcCurrent_mA = measuringGetFilteredOutputAcCurrentValue_mA();

		// A check that voltage is not less than a minimum per current unit.
		// If this happens then the voltage meter is perhaps not connected.
		if (measuredAcCurrent_mA > 10)
		{
			const int32_t meauredAcVoltage_mV = getMeasuredExternalAcVoltage_mV();
			const int32_t factor = meauredAcVoltage_mV / measuredAcCurrent_mA;
			if (factor < ee.min_mV_per_mA_ac)
			{
				errorReportError(portsVoltageMeterNotReacting);
			}
		}
	}
}
 
void superviseCurrentSecondsTick1(void)
{
	const uint32_t measuredDcVoltage_mV = measuringGetFilteredInternalDcVoltValue_mV();
	const uint32_t measuredAcCurrent_mA = measuringGetFilteredOutputAcCurrentValue_mA();

	switch(superviseState)
	{
		case SUP_INIT_STATE:
		{
			superviceInitState();
			break;
		}
		default:
		case SUP_STABILITY_STATE:
		{
			supervice_stability();
			break;
		}
	}
	
	// Things that are to be checked in all states
	if (errorGetReportedError() == portsErrorOk)
	{
		checkForErrors();
	}

	prevMeasuredFilteredInternalVoltValue_mV = measuredDcVoltage_mV;
	prevMeasuredFilteredAcCurrentValue_mV = measuredAcCurrent_mA;
}

#define LONG_TERM_TIME_CONST_VOLT 10000
#define LONG_TERM_TIME_CONST_CURR 1000

void superviseCurrentMilliSecondsTick(void)
{
	const int32_t measuredAcCurrent_mA = measuringGetFilteredOutputAcCurrentValue_mA();
	const int32_t measuredDcVoltage_mV = measuringGetFilteredInternalDcVoltValue_mV();

	if (errorGetReportedError() == portsErrorOk)
	{
		if (superviseInternalVoltageEtc)
		{
			if (ee.superviceIncFactor != 0)
			{
				const uint32_t wantedCurrent_mA = getWantedCurrent_mA();
				if (measuredAcCurrent_mA > wantedCurrent_mA)
				{
					const int32_t margin_mA = 500 + (longTermAcCurrent_mA / ee.superviceIncFactor);
					if (measuredAcCurrent_mA > (longTermAcCurrent_mA+margin_mA))
					{
						// To fast current increase, perhaps a short somewhere?
						logInt6(TO_FAST_CURRENT_INC, 0, measuredAcCurrent_mA, superviseAcCurrentMin, superviseAcCurrentMax, ee.superviceIncFactor);
						errorReportError(portsErrorUnexpectedCurrentIncrease);
						enterIdleState();
					}
				}
			}

			if (ee.superviceDropFactor != 0)
			{
				// If internal voltage drops more than 1/superviceDropFactor from the long term medium value then something is wrong.
				const int32_t margin_mV = ee.superviceMargin_mV + (longTermInternalVoltage_mV / ee.superviceDropFactor);
				if ((measuredDcVoltage_mV + margin_mV) < longTermInternalVoltage_mV)
				{
					// To fast voltage drop. Input power is probably lost.
					logInt6(SUPERVICE_INTERNAL_VOLTAGE_DROP, 3, measuredDcVoltage_mV, longTermInternalVoltage_mV, margin_mV, ee.superviceDropFactor);
					errorReportError(portsInputVoltageDecrease);
				}
			}

		}
	}


	longTermInternalVoltage_mV = (((LONG_TERM_TIME_CONST_VOLT-1)*longTermInternalVoltage_mV) + (int64_t)(measuredDcVoltage_mV + LONG_TERM_TIME_CONST_VOLT/2)) / LONG_TERM_TIME_CONST_VOLT;
	longTermAcCurrent_mA = (((LONG_TERM_TIME_CONST_CURR-1)*longTermAcCurrent_mA) + (int64_t)(measuredAcCurrent_mA + LONG_TERM_TIME_CONST_CURR/2)) / LONG_TERM_TIME_CONST_CURR;
}

int superviseGetState()
{
	return superviseState;
}

/*
Scan shall activate this supervision once it has detected power to be present.
requireVoltage:
0: Not required
1: Supervise the internal voltage, report error if it decreases,
*/
void setSuperviseInternalVoltage(int32_t requireVoltage)
{
	if (superviseInternalVoltageEtc != requireVoltage)
	{
		const uint32_t measuredDcVoltage_mV = measuringGetFilteredInternalDcVoltValue_mV();
		//const uint64_t measuredAcCurrent_mA = measuringGetFilteredOutputAcCurrentValue_mA();

		if (longTermInternalVoltage_mV < measuredDcVoltage_mV)
		{
			longTermInternalVoltage_mV = measuredDcVoltage_mV;
		}

		/*if (longTermAcCurrent_mA > measuredAcCurrent_uA)
		{
			longTermAcCurrent_mA = measuredAcCurrent_uA;
		}*/

		superviseInternalVoltageEtc = requireVoltage;
	}
}

int64_t superviceLongTermInternalVoltage_uV()
{
	return longTermInternalVoltage_mV;
}

int64_t superviceLongTermAcCurrent_uA()
{
	return longTermAcCurrent_mA;
}


static int32_t internalConnectionState = INITIAL_CONFIG;

// The message must match that in target_maintenance_sendWebServerStatusMsg.
CmdResult processWebServerStatusMsg(DbfUnserializer *dbfPacket)
{
	int64_t wallTimeReceived_ms = DbfUnserializerReadInt64(dbfPacket);
	int32_t connectionStateReceived = DbfUnserializerReadInt32(dbfPacket);
	int64_t diff_ms = ABS(wallTimeReceived_ms - ee.wallTimeReceived_ms);

	if ((connectionStateReceived != ALL_CONFIG_DONE) || (internalConnectionState != ALL_CONFIG_DONE))
	{
		logInt7(WEB_STATUS_RECEIVED, 1, diff_ms, connectionStateReceived, timeCounted_s, ee.wallTimeReceived_ms, wallTimeReceived_ms);
	}
	else
	{
		if (diff_ms>12000)
		{
			logInt7(WEB_STATUS_RECEIVED, 2, diff_ms, connectionStateReceived, timeCounted_s, ee.wallTimeReceived_ms, wallTimeReceived_ms);
		}
		ee.wallTimeReceived_ms = wallTimeReceived_ms;

	}
	timeCounted_s = 0;
	return CMD_OK;
}

void setConnectionState(int intconnectionState)
{
	internalConnectionState = intconnectionState;
}


#else
void superviseCurrentSecondsTick1(void)
{
}
#endif
