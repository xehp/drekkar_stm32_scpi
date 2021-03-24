/*
supervice.c

provide functions that happen once per second, such as logging.

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
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
#include "eeprom.h"
#include "translator.h"
#include "Dbf.h"
//#include "measuring.h"
#include "fan.h"
#include "cmd.h"
#include "messageNames.h"
#include "log.h"
#include "externalSensor.h"
#include "mainSeconds.h"





int32_t secAndLogSeconds = 0;
char superviceState = 0;
int32_t superviceSeconds = 0;
//static int16_t supervice_previous_ac_count=0;



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






void secAndLogInit(void)
{
	logInt1(SEC_AND_LOG_INIT);

	//uart_print_P(PSTR(LOG_PREFIX "status seconds mainState measuredCycles measuredCurrent wantedCurrent offPeriod halfPeriod regState scanState timeoutEvents measuredVoltage wantedVoltage" LOG_SUFIX));
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
			superviceState++;
			break;
		}
		case 2:
		{
			machineSupervise();
			fanAndTempMainSecondsTick();
			superviceState++;
			break;
		}
		case 3:
		{
			superviceState++;
			break;
		}
		case 4:
		{
			superviceState++;
			break;
		}
		case 5:
		{
			superviceState++;
			break;
		}
		case 6:
		{
			superviceState++;
			break;
		}
		case 7:
		{
			superviceState++;
			break;
		}
		case 8:
		{
			superviceState++;
			break;
		}
		case 9:
		{
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
