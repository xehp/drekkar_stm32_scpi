/*
main_loop.c

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...



History

2016-08-15
Created for dielectric test equipment
Henrik

*/

//#include <stdio.h>
//#include <unistd.h>
//#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "stm32l4xx.h"
#include "stm32l432xx.h"
#include "stm32l4xx_nucleo_32.h"
#include "stm32l4xx_ll_adc.h"

#include "systemInit.h"
#include "timerDev.h"
#include "adcDev.h"
#include "compDev.h"
#include "supervise.h"
#include "mathi.h"
#include "externalSensor.h"
#include "cmd.h"
#include "scan.h"
#include "fan.h"
#include "temp.h"
#include "log.h"
#include "relay.h"
#include "version.h"
#include "main_loop.h"
#include "cfg.h"
#include "eeprom.h"
#include "mainSeconds.h"
#include "measuringFilter.h"
#include "portsPwm.h"
#include "pwm1.h"
#include "serialDev.h"
#include "debugLog.h"
#include "messageUtilities.h"



// What shall the timeout be when waiting for current direction change
//const int16_t waitingTimeout=(DIV_ROUND_UP(AVR_TMR0_TICKS_PER_SEC(),25));

// Specify here a little delay before trying to detect current direction, This is given in the "unit" number of turns in the main loop. For how many turns per second we make see mainLoopCounter.
// So 1 here gives about 10 us
// 25 gives about 250 us


// 200 gives 2 ms which in turn gives about 250 Hz
const int16_t HalfCycleTimeout_mlt=10; // Unit is main loop turns (approx 10uS), So 100 here will give a frequenzy of about 500Hz, later we need to use something slightly less than 100 here since we wish to allow 300 - 3000 Hz.

const int16_t HalfCycleMinTime_mlt=2;







// Our current sensor measured top current not RMS or even mean value.
// If we drive a resistive load the top current is aproximately the same
// regardless of the actuall current so don't expect to much from it.

//const int16_t deadTime_us=1;  // Time between turning one MOSFET off and turning the other on. Unit is micro seconds.


int64_t mainTimeInTicks=0;

int32_t mainSecondsTimer=0;

// We do about 100000 turns per second on this. So for shorter time measurements use this instead of mainTimeInTicks

int32_t mainLoopCounter=0; // This increments about once per 10 uS
//char mainState=idleState;
//int16_t mainStateEnteredTicks=0;   // Keeps the ticks that we had when currect state was entered, can be used to calculate how int32_t state has been (given in ticks).
//int16_t mainLoopCounterInState=0; // Counts the number of turns since state was entered. See also mainLoopCounter.



//char debugString[120]={0};
//char *debugPtr=NULL;

char tickState=0;


// Variables and constants used for current measuring

const int16_t TimeToDetectCurrent=2048;



/*
void logLine(char* str)
{
#ifndef __linux__

	uart_print_P(PSTR(LOG_PREFIX));
	printHex16(mainSeconds);
	uart_putchar(' ');
	uart_print(str);
	uart_print_P(PSTR(LOG_SUFIX));

#else
	printf("logLine %d %s\n", mainTimeInTicks, str);
	fflush(stdout);
#endif
}
*/


#define mainLog(str) {serialPrint(DEV_LPUART1, str); serialPrint(DEV_USART1, str); serialPrint(DEV_USART2, str); systemSleepMs(100);}

// This was a test, remove later if OK
volatile int64_t one = 1;
static int64_t test(int64_t a)
{
	return a+one;
}


/*#ifdef __cplusplus
extern "C" {
#endif*/


int main_loop(void)
{
	systemSleepMs(100);

	// For connection with websuart,
	// this is input from web server via opto link.
	if (serialInit(DEV_USART1, USART1_BAUDRATE)!=0)
	{
		systemErrorHandler(SYSTEM_USART1_ERROR);
	}
	else
	{
		serialPrint(DEV_USART1, "USART1\n");
	}

	// Currently debug logging output.
	#if (defined USART2_BAUDRATE)
	if (serialInit(DEV_USART2, USART2_BAUDRATE)!=0)
	{
		systemErrorHandler(SYSTEM_USART2_ERROR);
	}
	else
	{
		serialPrint(DEV_USART2, "USART2\n");
	}
	#endif

	// For connection with volt sensor.
        #ifdef LPUART1_BAUDRATE
	if (serialInit(DEV_LPUART1, LPUART1_BAUDRATE)!=0)
	{
		systemErrorHandler(SYSTEM_LPUART_ERROR);
	}
	else
	{
		serialPrint(DEV_LPUART1, "LPUART1\n");
	}
        #endif


	systemSleepMs(100);


	// Print some message, so we know the program is running.
	mainLog(LOG_PREFIX LOG_SUFIX);
	mainLog(LOG_PREFIX VERSION_STRING LOG_SUFIX);
	mainLog(LOG_PREFIX "build " __DATE__ " " __TIME__ LOG_SUFIX);
	#ifdef GIT_VERSION
	mainLog(LOG_PREFIX "git " GIT_VERSION LOG_SUFIX);
	#endif

	mainLog(LOG_PREFIX "deviceId "); debug_print64(ee.deviceId); mainLog(LOG_SUFIX);

	mainLog(LOG_PREFIX "Copyright 2020 (C) Henrik Bjorkman www.eit.se/hb" LOG_SUFIX);


#if (defined __linux__)
	mainLog(LOG_PREFIX "linux" LOG_SUFIX);
#elif __arm__
	mainLog(LOG_PREFIX "arm" LOG_SUFIX);
#else
#error Only Linux or embedded ARM is currently supported.
#endif


#ifdef DONT_SCAN
	mainLog(LOG_PREFIX "DONT_SCAN" LOG_SUFIX);
#endif


	wdt_reset();

#if (defined __linux__)
	linux_sim_init();
	simulated_init();
#endif

	systemSleepMs(100);

	mainLog(LOG_PREFIX "loading from EEPROM" LOG_SUFIX);
	eepromLoad();

	mainLog(LOG_PREFIX "init machine state" LOG_SUFIX);
	machineStateInit();


	//pwm_init();

	// Pin used for contactor/relay
	relayInit();

	// Comp is used to detect input power loss.
	comp_init();

	// Initialize Analog to Digital Converter
	adc1Init();


	measuring_init();

	//for(int i=0; i<10;i++) {
	//	systemSleepMs(1000);
	//	mainLog(LOG_PREFIX "eternal loop" LOG_SUFIX);
	//}

	portsPwmInit();

	#if (defined PWM_PORT)
	pwm_init();
	#endif


	wdt_reset();

	externalSensorInit();


	// Initializing fan and temp supervision.
	mainLog(LOG_PREFIX "Initializing fan and temp" LOG_SUFIX);
	fanInit();
	tempInit();


	mainLog(LOG_PREFIX "Initialize resonance scanner" LOG_SUFIX);
	scan_init();


	mainLog(LOG_PREFIX "Start command interpreter" LOG_SUFIX);
	cmdInit();

	// Initialize the processes that tick once per second and also does the logging.
	mainLog(LOG_PREFIX "secAndLogInit" LOG_SUFIX);
	secAndLogInit();

	wdt_reset();

	// Tell web server that we rebooted. It shall clear some stored values.
	secAndLogInitStatusMessageAddHeader(&messageDbfTmpBuffer, REBOOT_STATUS_MSG);
	messageSendDbf(&messageDbfTmpBuffer);

	// This was a test, remove later if OK
	const int64_t a = 1234567891234567LL;
	const int64_t b = 1234567891234568LL;
	if (test(a) !=b)
	{
		mainLog(LOG_PREFIX "int64_t failed" LOG_SUFIX);
		return 0;
	}

	mainLog(LOG_PREFIX "Enter main loop" LOG_SUFIX);

	// Initialize the timer used to know when to do medium tick.
	mainTimeInTicks=systemGetSysTimeMs()+1000;
	mainSecondsTimer=mainTimeInTicks;

	// main loop
	for(;;)
	{
		measuring_process_fast_tick();

		// TODO This shall not be needed if we can get it done with interrupt
		//comp_mainTick();

		// command interpreter needs to be polled quite often so it does not miss input from serial ports.
		cmdFastTick();

		const int64_t timerTicks_ms=systemGetSysTimeMs();

		// In the switch we put things that need to be done medium frequently, this too tries to spread CPU load over time.

		switch(tickState)
		{
		case 0:
			if (mainTimeInTicks != timerTicks_ms)
			{

				// Timer has been incremented,
				// count ticks and start the tickState sequence,
				// typically this happens once per milli second.
				mainTimeInTicks = timerTicks_ms;
				tickState++;
			}
			break;
		case 1:
		{
			// Call those that need tick more often than once per second in the following cases.
			tickState++;
			break;
		}
		case 2:
		{
			const int16_t t = timerTicks_ms-mainSecondsTimer;
			if (t>=0)
			{
				// One second has passed since last second tick. Increment seconds timer.
				secAndLogIncSeconds();
				mainSecondsTimer += 1000;
				//logInt1(MAIN_LOOP_TICK);
			}
			tickState++;
			break;
		}
		case 3:
		{
			//portsMainTick();
			tickState++;
			break;
		}
		case 4:
		{
			secAndLogMediumTick();
			tickState++;
			break;
		}
		case 5:
		{
			cmdMediumTick();
			tickState++;
			break;
		}
		case 6:
		{
			scanMediumTick();
			tickState++;
			break;
		}
		case 7:
		{
			compMediumTick();
			tickState++;
			break;
		}
		case 8:
		{
			superviseCurrentMilliSecondsTick();
			tickState++;
			break;
		}
		case 9:
		{
			externalSensorMediumTick();
			tickState++;
			break;
		}
		case 10:
		{
			logMediumTick();
			tickState++;
			break;
		}
		default:
			// Set CPU in idle mode to save energy, it will wake up next time there is an interrupt
			systemSleep();
			tickState=0;
			break;
		}

		mainLoopCounter++;

		wdt_reset();
	}



	return(0);
} // end main()


/*#ifdef __cplusplus
} // extern "C"
#endif*/

