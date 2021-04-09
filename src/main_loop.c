/*
main_loop.c

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...



History

2016-08-15
Created for dielectric test equipment
Henrik

*/

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
#include "mathi.h"
#include "scpi.h"
#include "cmd.h"
#include "fan.h"
#include "temp.h"
#include "current.h"
#include "log.h"
#include "version.h"
#include "main_loop.h"
#include "cfg.h"
#include "eeprom.h"
#include "mainSeconds.h"
#include "debugLog.h"
#include "serialDev.h"
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

	// Currently debug logging output.
	// Later for connection with volt sensor.
	#if (defined USART2_BAUDRATE)
	if (serialInit(DEV_USART2, USART2_BAUDRATE)!=0)
	{
		systemErrorHandler(SYSTEM_USART2_ERROR);
	}
	else
	{
		debug_print(LOG_PREFIX VERSION_STRING LOG_SUFIX);
	}
	#endif



    #ifdef LPUART1_BAUDRATE
	if (serialInit(DEV_LPUART1, LPUART1_BAUDRATE)!=0)
	{
		systemErrorHandler(SYSTEM_LPUART_ERROR);
	}
	else
	{
		serialPrint(DEV_LPUART1, LOG_PREFIX VERSION_STRING LOG_SUFIX);
	}
    #endif

	#ifdef SOFTUART1_BAUDRATE
	if (serialInit(DEV_SOFTUART1, SOFTUART1_BAUDRATE)!=0)
	{
		systemErrorHandler(SYSTEM_SOFT_UART_ERROR);
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
#error
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

	// Initialize Analog to Digital Converter
	#if (defined TEMP1_ADC_CHANNEL) || (defined TEMP2_ADC_CHANNEL) || (CURRENT_ADC_CHANNEL)
	//mainLog(LOG_PREFIX "init adc" LOG_SUFIX);
	adc1Init();
	#endif

	systemSleepMs(100);

	wdt_reset();

	#if (defined SCPI_ON_USART2 || defined SCPI_ON_LPUART1 || defined SCPI_ON_SOFTUART1)
	mainLog(LOG_PREFIX "init external sensor" LOG_SUFIX);
	externalSensorInit();
	#endif

	#if (defined USE_LPTMR2_FOR_FAN2) || (defined FAN1_APIN) || (defined INTERLOCKING_LOOP_PIN)
	// Initializing fan and interlocking supervision.
	mainLog(LOG_PREFIX "Initializing fan" LOG_SUFIX);
	fanInit();
	#endif

	#if (defined TEMP1_ADC_CHANNEL) || (defined TEMP2_ADC_CHANNEL) || (defined USE_LPTMR1_FOR_TEMP1)
	// Initializing temp supervision.
	mainLog(LOG_PREFIX "Initializing temp" LOG_SUFIX);
	tempInit();
	#endif

	#ifdef CURRENT_ADC_CHANNEL
	currentInit();
	#endif

	mainLog(LOG_PREFIX "Start command interpreter" LOG_SUFIX);
	cmdInit();

	// Initialize the processes that tick once per second and also does the logging.
	mainLog(LOG_PREFIX "secAndLogInit" LOG_SUFIX);
	secAndLogInit();

	wdt_reset();

	// Tell web server that we rebooted. It shall clear some stored values.
	secAndLogInitStatusMessageAddHeader(&messageDbfTmpBuffer, REBOOT_STATUS_MSG);
	messageSendDbf(&messageDbfTmpBuffer);

	mainLog(LOG_PREFIX "Enter main loop" LOG_SUFIX);

	// Initialize the timer used to know when to do medium tick.
	mainTimeInTicks=systemGetSysTimeMs()+1000;
	mainSecondsTimer=mainTimeInTicks;

	// main loop
	for(;;)
	{
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
			#ifdef CURRENT_ADC_CHANNEL
			currentMediumTick();
			#endif
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
			tickState++;
			break;
		}
		case 7:
		{
			tickState++;
			break;
		}
		case 8:
		{
			tickState++;
			break;
		}
		case 9:
		{
			#if (defined SCPI_ON_USART2 || defined SCPI_ON_LPUART1 || defined SCPI_ON_SOFTUART1)
			externalSensorMediumTick();
			#endif
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

