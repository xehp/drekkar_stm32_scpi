/*
cmd.c

provide to handle commands/messages from other units/users

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2016-09-22
Created.
Henrik Bjorkman

*/
#include "cfg.h"
//#include <string.h>

#include "main_loop.h"
#include "messageNames.h"
#include "Dbf.h"
#include "mathi.h"
#include "eeprom.h"
#include "fan.h"
#include "translator.h"
#include "version.h"
#include "log.h"
#include "externalSensor.h"
#include "cmd.h"
#include "debugLog.h"
#include "serialDev.h"
#include "mainSeconds.h"



#ifdef COMMAND_ON_LPUART1
static DbfReceiver cmdLine0;
#endif

static DbfReceiver cmdLine1;

#ifdef COMMAND_ON_USART2
static DbfReceiver cmdLine2;
#endif


// Magic numbers so we can detect version incompatibilities.
// This needs to be same as magicNumberInAllConfig in target_maintenance.c
// All sorts of commands (except stop) shall be ignored unless web server gave
// correct magic number.
#define MAGIC_NUMBER_WEB_SERVER 8898878L
#define ALL_CONFIG_CMD_MAGIC_NR 0x0139


#define DBG_MODE 5

// See also DEFAULT_MAX_TEMP_HZ
#define CMD_MAX_TEMP_HZ 1360


static void forwardMessage(int usartDev, const DbfReceiver* dbfReceiver)
{
	const char *msgPtr=dbfReceiver->buffer;
	const int msgLen=dbfReceiver->msgSize;
	serialPutChar(usartDev, DBF_BEGIN_CODEID);
	serialWrite(usartDev, msgPtr, msgLen);
	serialPutChar(usartDev, DBF_END_CODEID);
}




static int64_t logParameterCounter = 0;

static const int64_t minTimeBeteenParameterLogging_ms = 50;
static int64_t LastParameterLogTime = 0;



/**
 * Used to send a message received from one serial port to another.
 */
static void forwardMessageToOthers(int receivedFromUsartDev, const DbfReceiver* dbfReceiver)
{
	// Depending on which USART device the message was received on, forward to the other.

	/*if (isAlreadyForwarded(dbfReceiver))
	{
		// Don,t forward this, its same as a previous message.
	}
	else*/
	{
		switch(receivedFromUsartDev)
		{
			case DEV_USART1:
				forwardMessage(DEV_USART2, dbfReceiver);
				break;
			case DEV_USART2:
				forwardMessage(DEV_USART1, dbfReceiver);
				break;
			default:
				break;
		}
	}
}






// Returns 0 if OK, nonzero otherwise.
int64_t getParameterValue(PARAMETER_CODES parId, NOK_REASON_CODES *result)
{
	switch(parId)
	{
		case DEVICE_ID:
		{
			return ee.deviceId;
		}
		case TARGET_TIME_S:
		{
			return secAndLogGetSeconds();
		}
		case REPORTED_EXT_AC_VOLTAGE_MV:
		{
			return getMeasuredExternalAcVoltage_mV();
		}
		case TEMP1_C:
		{
			return fanAndTempGetTemp1Measurement_C();
		}
		#if (defined USE_LPTMR2_FOR_TEMP2) || (defined TEMP2_ADC_CHANNEL)
		case TEMP2_C:
		{
			return fanAndTempGetTemp2Measurement_C();
		}
		#endif
		#ifdef USE_LPTMR2_FOR_FAN2
		case FAN2_HZ:
		{
			return fanAndTempGetFan2Measurement_Hz();
		}
		#endif
		#ifdef FAN1_APIN
		case FAN1_HZ:
		{
			return fanAndTempGetFan1Measurement_Hz();
		}
		#endif
		case REPORTED_ERROR:
		{
			return errorGetReportedError();
		}
		case SYS_TIME_MS:
		{
			return systemGetSysTimeMs();
		}
		default:
			if (result!=NULL)
			{
				*result = NOK_UNKOWN_PARAMETER;
			}
			break;
	}
	return 0;
}


static void processReceivedMessage(int usartDev, const DbfReceiver* dbfReceiver)
{
	forwardMessageToOthers(usartDev, dbfReceiver);
}

void cmdCheckSerialPort(int usartDev, DbfReceiver* dbfReceiver)
{
	// Check for input from serial port
	const int16_t ch = serialGetChar(usartDev);
	if (ch>=0)
	{
		//#if (defined __arm__)
		//usartPrint(DEV_USART2, "ch ");
		//usartPrintInt64(DEV_USART2, ch);
		//usartPrint(DEV_USART2, "\n");
		//#endif

		const char r = DbfReceiverProcessCh(dbfReceiver, ch);

		if (r > 0)
		{
			// A full message or line has been received.

			processReceivedMessage(usartDev, dbfReceiver);

			DbfReceiverInit(dbfReceiver);
		}
		else if (r == 0)
		{
			// We do not yet have a full message.
		}
		else
		{
			debug_print(LOG_PREFIX "something wrong" LOG_SUFIX);
			logInt1(CMD_INCORRECT_DBF_RECEIVED);
			DbfReceiverInit(dbfReceiver);
		}

	}
}

// This shall be called from main loop as often as possible.
// It is used to check for incoming messages.
void cmdFastTick(void)
{
	cmdCheckSerialPort(DEV_USART1 , &cmdLine1);

	#ifdef COMMAND_ON_LPUART1
	cmdCheckSerialPort(DEV_LPUART1 , &cmdLine0);
	#endif
	#ifdef COMMAND_ON_USART2
	cmdCheckSerialPort(DEV_USART2 , &cmdLine2);
	#endif
}

// This shall be called from main loop once per tick, typically once per millisecond.
// It is used for timeouts receiving messages.
void cmdMediumTick(void)
{
	DbfReceiverTick(&cmdLine1, 1);

	#ifdef COMMAND_ON_LPUART1
	DbfReceiverTick(&cmdLine0, 1);
	#endif
	#ifdef COMMAND_ON_USART2
	DbfReceiverTick(&cmdLine2, 1);
	#endif

	// TODO Perhaps Move this to log.c?
	const int64_t t = systemGetSysTimeMs();
	const int64_t timeSinceLastParameterLog = t - LastParameterLogTime;
	if (timeSinceLastParameterLog> minTimeBeteenParameterLogging_ms)
	{
		NOK_REASON_CODES readBackResult = REASON_OK;
		const int64_t v = getParameterValue(logParameterCounter, &readBackResult);
		if (readBackResult == REASON_OK)
		{
			if (logIfParameterChanged(logParameterCounter, v))
			{
				LastParameterLogTime = t;
			}
		}
		logParameterCounter++;
		if (logParameterCounter>=LOGGING_MAX_NOF_PARAMETERS)
		{
			logParameterCounter = 0;
		}
	}
}





void cmdInit(void)
{
	logInt1(CMD_INIT);
	DbfReceiverInit(&cmdLine1);

	#ifdef COMMAND_ON_LPUART1
	DbfReceiverInit(&cmdLine0);
	#endif
	#ifdef COMMAND_ON_USART2
	DbfReceiverInit(&cmdLine2);
	#endif

	// cmdInit must be called after eepromLoad for this to work.
	// if not result may be unpredictable.
}




