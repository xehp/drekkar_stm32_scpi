/*
supervice.c

provide functions that happen once per second, such as logging.

Copyright (C) 2021 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2016-09-22
Created.
Henrik Bjorkman

*/

#include "cfg.h"

#if (defined __linux__)
#include <stdio.h>
#include <unistd.h>
#elif (defined __arm__)
#include "systemInit.h"
#include "timerDev.h"
#else
#error
#endif

#include "mathi.h"
#include "machineState.h"
#include "portsPwm.h"
#include "eeprom.h"
#include "translator.h"
#include "Dbf.h"
//#include "measuring.h"
#include "adcDev.h"
#include "measuringFilter.h"
#include "cmd.h"
#include "scan.h"
//#include "reg.h"
#include "fan.h"
#include "temp.h"
#include "messageNames.h"
#include "messageUtilities.h"
#include "log.h"
#include "compDev.h"
#include "relay.h"
#include "pwm1.h"
#include "supervise.h"
#include "externalSensor.h"
#include "externalTemp.h"
#include "externalCurrentLeakSensor.h"
#include "mainSeconds.h"





int32_t secAndLogSeconds = 0;
char superviceState = 0;
int32_t superviceSeconds = 0;
//static int16_t supervice_previous_ac_count=0;

static int32_t lastLoggedHalfPeriod = 0;
static int32_t lastLoggedInputVoltage = 0;
static int32_t lastLoggedExternalVoltage = 0;

static int32_t lastLoggedTimestamp = 0;
static int32_t lastLoggedDcCurrent_mA = 0;
static int32_t lastLoggedInternalVoltage = 0;
static int32_t lastLoggedTemp_Hz = 0;
static int32_t lastLoggedThrottle = 0;



// Log frequency current and voltage at least this often.
#define MIN_LOG_INTERVAL_SEC 60L

// But not more often than this.
#define MINIMUM_TIME_BETWEEN_LOG_S 5



int32_t secAndLogGetSeconds()
{
	return secAndLogSeconds;
}

/**
 * This is expected to be called from main loop once per second
 */
void secAndLogIncSeconds(void)
{
	secAndLogSeconds++;
/*#ifdef __linux__
	printf("incMainSeconds %d\n", mainSeconds);
	fflush(stdout);
#endif*/
}




/*static uint32_t my_abs32(int32_t v)
{
	const int32_t tmp = (v>=0) ? v : -v;
	return tmp;
}*/

/*static uint16_t my_abs16(int16_t v)
{
	return v<0 ? -v : v;
}*/


// A bitmask with possible errors;
static uint32_t getErrorMask()
{
	uint32_t errorMask=0;
	if (!temp1Ok())
	{
		errorMask |= 0x1;
	}
	#ifdef FAN1_APIN
	if (!fan1Ok())
	{
		errorMask |= 0x2;
	}
	#endif
	#ifdef USE_LPTMR2_FOR_FAN2
	if (!fan2Ok())
	{
		errorMask |= 0x4;
	}
	#endif
	#ifdef USE_LPTMR2_FOR_TEMP2
	if (!temp2Ok())
	{
		errorMask |= 0x8;
	}
	#endif
	#if (defined PWM_PORT)
	if (pwm_isBreakActive())
	{
		errorMask |= 0x10;
	}
	#endif
	/*if (!(getMeasuredVoltageIsAvailable() || (cmdGetWantedVoltage() == NO_VOLT_VALUE)))
	{
		errorMask |= 0x20;
	}*/

	if (errorGetReportedError() != portsErrorOk)
	{
		errorMask |= 0x80000000;
	}

	return errorMask;
}

static void printCurrentFreq(void)
{
	const int32_t mainSeconds = secAndLogGetSeconds();
	const int32_t time_since_log=mainSeconds-lastLoggedTimestamp;

	if (time_since_log>=MINIMUM_TIME_BETWEEN_LOG_S)
	{
		const uint32_t scanHalfPeriod = ports_get_period();
		#ifdef INV_OUTPUT_AC_CURRENT_CHANNEL
		const uint32_t measuredAcCurrent_mA = measuringGetFilteredOutputAcCurrentValue_mA();
		#else
		const uint32_t measuredAcCurrent_mA = 0;
		#endif
		const uint32_t measuredDcCurrent_mA = measuringGetFilteredDcCurrentValue_mA();
		const uint32_t measuredInternalDcVoltage_mV = measuringGetFilteredInternalDcVoltValue_mV();
		const int32_t measuredExternalAcVoltage_mV = getMeasuredExternalAcVoltage_mV();
		const int32_t temp1_celsius = tempGetTemp1Measurement_C();
		const uint32_t dcCurrentChange = MY_ABS((int32_t)measuredDcCurrent_mA-(int32_t)lastLoggedDcCurrent_mA);
		const uint32_t voltageInternalChange = MY_ABS((int32_t)measuredInternalDcVoltage_mV-(int32_t)lastLoggedInternalVoltage);
		const uint32_t voltageExternalChange = MY_ABS((int32_t)measuredExternalAcVoltage_mV-(int32_t)lastLoggedExternalVoltage);

		//const uint32_t temp_changed_Hz = MY_ABS(temp_Hz-lastLoggedTemp_Hz);

		const int32_t portsPwmBrakeState = portsGetPwmBrakeState();

		if (  (time_since_log>=(MIN_LOG_INTERVAL_SEC))
			|| ((scanHalfPeriod!=lastLoggedHalfPeriod))
			|| ((voltageInternalChange*20L)>(lastLoggedInternalVoltage+1000))
			|| ((voltageExternalChange*20L)>(lastLoggedExternalVoltage+200))
			|| ((dcCurrentChange*20L)>(lastLoggedDcCurrent_mA+200))
		   )
		{
			const int32_t frequency_Hz = measuring_convertToHz(scanHalfPeriod);
			const char relayState = relayGetState();
			const int32_t wantedVoltage_mV = getWantedVoltage_mV();
			const int32_t extTemp1_C = getExtTemp1C();
			const int32_t extTemp2_C = getExtTemp2C();
			const uint32_t angle = portsGetAngle();
			const uint32_t errorMask = getErrorMask();
			const uint32_t portsErrors=errorGetReportedError();
			const uint32_t wantedCurrent_mv = 0; //ee.wantedCurrent_mA;
			const uint32_t spare1 = 0;


			logInitAndAddHeader(&messageDbfTmpBuffer, LOG_FREQ_CURRENT_VOLT_INV);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, frequency_Hz);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, measuredAcCurrent_mA);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, measuredExternalAcVoltage_mV);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, temp1_celsius);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, extTemp1_C);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, extTemp2_C);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, measuredDcCurrent_mA);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, wantedVoltage_mV);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, measuredInternalDcVoltage_mV);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, relayState);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, scanGetState());
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, angle);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, scanHalfPeriod);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, wantedCurrent_mv);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, errorMask);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, portsErrors);
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, DIV_ROUND(scanGetDutyCyclePpm(),1000));
			DbfSerializerWriteInt32(&messageDbfTmpBuffer, portsPwmBrakeState);
			messageSendDbf(&messageDbfTmpBuffer);


			lastLoggedTimestamp=mainSeconds;
			lastLoggedHalfPeriod=scanHalfPeriod;
			lastLoggedInputVoltage=spare1;
			lastLoggedInternalVoltage=measuredInternalDcVoltage_mV;
			lastLoggedExternalVoltage=measuredExternalAcVoltage_mV;
			lastLoggedDcCurrent_mA=measuredDcCurrent_mA;
			lastLoggedTemp_Hz=temp1_celsius;
			lastLoggedThrottle = portsPwmBrakeState;
		}
	}
}



void secAndLogInit(void)
{
	logInt1(SEC_AND_LOG_INIT);

	//uart_print_P(PSTR(LOG_PREFIX "status seconds mainState measuredCycles measuredCurrent wantedCurrent offPeriod halfPeriod regState scanState timeoutEvents measuredVoltage wantedVoltage" LOG_SUFIX));
}


static int getStatusCode(void)
{
	PORTS_STATUS_SUB_MESSAGES s = scanGetStateName();
	return s;
}

/**
This function makes and sends a general status message.
Typically this shall be called once per second.
This message must match the unpacking in process_status_from_target in web server.
*/
static void sendGeneralStatus()
{
	const uint16_t statusCode=getStatusCode();
	const int32_t frequency_Hz = measuring_convertToHz(ports_get_period());

	const uint32_t measuredAcCurrent_mA = measuringGetFilteredOutputAcCurrentValue_mA();

	secAndLogInitStatusMessageAddHeader(&statusDbfMessage, TARGET_GENERAL_STATUS_MSG);
	DbfSerializerWriteInt64(&statusDbfMessage, superviceSeconds);
	DbfSerializerWriteInt32(&statusDbfMessage, statusCode);
	DbfSerializerWriteInt32(&statusDbfMessage, frequency_Hz);
	DbfSerializerWriteInt32(&statusDbfMessage, measuredAcCurrent_mA);
	DbfSerializerWriteInt32(&statusDbfMessage, portsGetCycleCount());
	DbfSerializerWriteInt64(&statusDbfMessage, ee.wallTimeReceived_ms);

	messageSendDbf(&statusDbfMessage);
}

/** 
 * This method shall be called once per tick.
 * TODO Move counting of seconds from main_loop to here.
 * Here we put things that need to be done only once per second. 
 * This to spread the CPU load over time. */
void secAndLogMediumTick(void)
{
	switch(superviceState)
	{
		case 0:
		if (secAndLogSeconds!=superviceSeconds)
		{
			// Seconds has ticked.

			SYS_LED_ON();

			superviceSeconds=secAndLogSeconds;

			// Start a seconds tick sequence.
			// Functions needing seconds tick will get it in the following few calls
			// to even out the CPU load.
			superviceState++;
		}
		break;

		case 1:
		{
			superviseCurrentSecondsTick1();
			superviceState++;
			break;
		}
		case 2:
		{
			machineSupervise();
			fanMainSecondsTick();
			tempMainSecondsTick();
			superviceState++;
			break;
		}
		case 3:
		{
			printCurrentFreq();
			superviceState++;
			break;
		}
		case 4:
		{
			measuringSlowTick();
			superviceState++;
			break;
		}
		case 5:
		{
			sendGeneralStatus();
			superviceState++;
			break;
		}
		case 6:
		{
			compSlowTick();
			superviceState++;
			break;
		}
		case 7:
		{
			scanSecondsTick();
			superviceState++;
			break;
		}
		case 8:
		{
			relaySecTick();
			superviceState++;
			break;
		}
		case 9:
		{
			portsSecondsTick();
			superviceState++;
			break;
		}
		case 10:
		{
			superviseCurrentSecondsTick2();
			superviceState++;
			break;
		}
		case 11:
		{
			externalLeakSensorSecondsTick();
			superviceState++;
			break;
		}
		default:
		{
			superviceState=0;
			SYS_LED_OFF();
			break;
		}
	}
}
