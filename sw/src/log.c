/*
log.c

Created 2018 by Henrik

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...
*/

#include<stdarg.h>
#include<stdio.h>

#include "Dbf.h"
#include "eeprom.h"
#include "cmd.h"
#include "log.h"
#include "serialDev.h"
#include "debugLog.h"
#include "mathi.h"
#include "messageNames.h"
#include "messageUtilities.h"


int32_t logSequenceNumber=0;


/*
Extract from msg.c (if changed msg.c has precedence):
 	LOG_CATEGORY
		Logging messages, only for logging, shall have no effect on operation.
		All log messages are broadcasted so need no receiver ID.
		After this code the following fields (AKA codes) are expected:
		<sender ID>
		<log sequence counter>
			A log sequence number 0-63, incremented by one for each message
			the sequence number is added so that a warning can be logged
			if some log message was lost.
		<log message type>
			see AVR_CFG_LOG_MESSAGES
		<log message body>
			The rest of message depend on log message type
*/
void logInitAndAddHeader(DbfSerializer *dbfSerializer, AVR_CFG_LOG_MESSAGES msg)
{
	const int32_t tmp = logSequenceNumber;
	logSequenceNumber = (logSequenceNumber+1) & 0x3F;

	messageInitAndAddCategoryAndSender(dbfSerializer, LOG_CATEGORY);
	DbfSerializerWriteInt64(dbfSerializer, tmp);
	DbfSerializerWriteInt32(dbfSerializer, msg);
}



/**
First argument (after nParameters) must be an int.
After that all arguments must be int64_t.
*/
static void logInt(int nParameters, ...)
{
	va_list argptr;
	va_start(argptr, nParameters);
	int msg = va_arg(argptr, int);

	#if (defined DEBUG_UART) && (!defined DEBUG_DECODE_DBF)
	debug_print("logInt ");
	debug_print64(msg);
	debug_print(" ");
	debug_print(getLogMessageName(msg));
	#endif

	logInitAndAddHeader(&messageDbfTmpBuffer, msg);

	for(int i = 1; i < nParameters; i++) {
		int64_t value = va_arg(argptr, int64_t);
		DbfSerializerWriteInt64(&messageDbfTmpBuffer, value);
		#if (defined DEBUG_UART) && (!defined DEBUG_DECODE_DBF)
		debug_print(" ");
		debug_print64(value);
		#endif
	}

	#if (defined DEBUG_UART) && (!defined DEBUG_DECODE_DBF)
	debug_print("\n");
	#endif

	messageSendDbf(&messageDbfTmpBuffer);

	va_end(argptr);
}



void logInt1(int msg)
{
	logInt(1, msg);
}

void logInt2(int msg, int64_t value)
{
	logInt(2, msg, value);
}

void logInt3(int msg, int64_t value1, int64_t value2)
{
	logInt(3, msg, value1, value2);
}

void logInt4(int msg, int64_t value1, int64_t value2, int64_t value3)
{
	logInt(4, msg, value1, value2, value3);
}

void logInt5(int msg, int64_t value1, int64_t value2, int64_t value3, int64_t value4)
{
	logInt(5, msg, value1, value2, value3, value4);
}

void logInt6(int msg, int64_t value1, int64_t value2, int64_t value3, int64_t value4, int64_t value5)
{
	logInt(6, msg, value1, value2, value3, value4, value5);
}

void logInt7(int msg, int64_t value1, int64_t value2, int64_t value3, int64_t value4, int64_t value5, int64_t value6)
{
	logInt(7, msg, value1, value2, value3, value4, value5, value6);
}

void logInt8(int msg, int64_t value1, int64_t value2, int64_t value3, int64_t value4, int64_t value5, int64_t value6, int64_t value7)
{
	logInt(8, msg, value1, value2, value3, value4, value5, value6, value7);
}


static int64_t loggedParameterValues[MAX_NOF_PARAMETER_CODES];
//static int64_t loggedParameterTime[MAX_NOF_PARAMETER_CODES];

/*
static void sendParameterStatus(PARAMETER_CODES par, int64_t value)
{
	secAndLogInitStatusMessageAddHeader(&statusDbfMessage, PARAMETER_STATUS_MSG);
	DbfSerializerWriteInt32(&statusDbfMessage, par);
	DbfSerializerWriteInt64(&statusDbfMessage, value);
	dbfSendMessage(&statusDbfMessage);
}

static void logParameter(PARAMETER_CODES par, int64_t value, int time_ms)
{
	sendParameterStatus(par, value);
	loggedParameterValues[par] = value;
	loggedParameterTime[par] = time_ms;
}

static int logParameterIfChanged(PARAMETER_CODES par, int64_t value, int logInterval)
{
	const int64_t t = systemGetSysTimeMs();

	if (loggedParameterTime[par] == 0)
	{
		// It has not been logged at all so log it first.
		logParameter(par, value, t);
		return 1;
	}
	else if (value != loggedParameterValues[par])
	{
		const int64_t timeSincePreviousLogging = t - loggedParameterTime[par];
		// But not too often
		if (timeSincePreviousLogging >= logInterval)
		{
			logParameter(par, value, t);
			return 1;
		}
	}
	return 0;
}

int logIfChangedMuch(PARAMETER_CODES par, int64_t value, int logInterval, int changeFactor, int margin)
{
	const int64_t diff = MY_ABS(value - loggedParameterValues[par]);
	if (diff*changeFactor > (loggedParameterValues[par]+margin))
	{
		// If changed much log it more often.
		return logParameterIfChanged(par, value, logInterval);
	}
	else
	{
		// If changed only a little log it less often.
		return logParameterIfChanged(par, value, 60*logInterval);
	}
}

int logIfParameterChanged(PARAMETER_CODES par, int64_t value)
{
	if (par < 0)
	{
		// Shall not happen
	}
	else if (par >= SIZEOF_ARRAY(loggedParameterValues))
	{
		// Do not have a previous logged value for these.
		sendParameterStatus(par, value);
		return 1;
	}
	else
	{
		switch(par)
		{
			case SYS_TIME_MS:
			case CYCLES_TO_DO:
			case TOTAL_CYCLES_PERFORMED:
			case TARGET_TIME_S:
			case TIME_COUNTED_S:
			case REACHED_CYCLES_0:
			case REACHED_CYCLES_1:
			case REACHED_CYCLES_2:
			case REACHED_CYCLES_3:
			case WALL_TIME_RECEIVED_MS:
			case PORTS_CYCLE_COUNT:
			case STABLE_OPERATION_TIMER_S:
				return logParameterIfChanged(par, value, 300000);
				break;
			case PORTS_PWM_STATE:
				return logIfChangedMuch(par, value, 10000, 10, 100);
				break;
			case SCAN_STATE_PAR:
				return logParameterIfChanged(par, value, 0);
				break;
			case FILTERED_CURRENT_DC_MA:
			case MEASURED_AC_CURRENT_MA:
			case REPORTED_EXT_AC_VOLTAGE_MV:
			case PORTS_PWM_ANGLE:
			case TEMP1_C:
			case TEMP2_C:
			case FAN1_HZ:
			case FAN2_HZ:
			case DUTY_CYCLE_PPM:
			case TEMP1_IN_HZ:
			case SLOW_AC_CURRENT_MA:
			case SLOW_INTERNAL_VOLTAGE_MV:
			case FILTERED_INTERNAL_VOLTAGE_DC_MV:
			case PORTS_PWM_HALF_PERIOD:
			case INV_FREQUENCY_HZ:
			{
				return logIfChangedMuch(par, value, 1000, 25, 100);
			}
			case MEASURING_MAX_AC_CURRENT_MA:
			case MEASURING_MAX_DC_CURRENT_MA:
			case MEASURING_MAX_DC_VOLTAGE_MA:
			{
				return logIfChangedMuch(par, value, 1000, 25, 100);
			}

			default:
				return logParameterIfChanged(par, value, 0);
		}
	}
	return 0;
}
*/
// This will tell target SW that all parameters shall be sent to Web Server.
// Typically used after a reset/reboot.
void logResetTime()
{
	/*for (int i = 0; i< SIZEOF_ARRAY(loggedParameterTime); i++)
	{
		loggedParameterTime[i]=0;
	}*/
}

static unsigned int incAndWrap(unsigned int i)
{
	if (i>=(MAX_NOF_PARAMETER_CODES-1))
	{
		return 0;
	}
	return i+1;
}

static const int64_t minTimeBetweenParameterLogging_ms = 250; // To avoid saturating the link.
//static int logParameterCounter = 0;
static unsigned int logParameterRegardlessCounter = 0;
static unsigned int logChangedParameterCounter = 0;
static int64_t LastParameterLogTime_ms = 0;

/**
 * Returns 1 if a parameter was added. 0 if not.
 */
static int addNextParameterToMsg(DbfSerializer *dbfMessage, const int64_t time_ms)
{
	int n=0;
	NOK_REASON_CODES readBackResult = REASON_OK;
	const int64_t value = getParameterValue(logParameterRegardlessCounter, &readBackResult);
	if (readBackResult == REASON_OK)
	{
		DbfSerializerWriteInt32(&statusDbfMessage, logParameterRegardlessCounter);
		DbfSerializerWriteInt64(&statusDbfMessage, value);

		loggedParameterValues[logParameterRegardlessCounter] = value;
		//loggedParameterTime[logParameterRegardlessCounter] = time_ms;
		n++;
	}
	logParameterRegardlessCounter = incAndWrap(logParameterRegardlessCounter);
	return n;
}

/**
 * Returns 1 if a parameter was added. 0 if not.
 */
static int addNextParameterToMsgIfChanged(DbfSerializer *dbfMessage, const int64_t time_ms)
{
	int n=0;
	NOK_REASON_CODES readBackResult = REASON_OK;
	const int64_t value = getParameterValue(logChangedParameterCounter, &readBackResult);
	if (readBackResult == REASON_OK)
	{
		if (value != loggedParameterValues[logChangedParameterCounter])
		{
			DbfSerializerWriteInt32(&statusDbfMessage, logChangedParameterCounter);
			DbfSerializerWriteInt64(&statusDbfMessage, value);

			loggedParameterValues[logChangedParameterCounter] = value;
			//loggedParameterTime[logChangedParameterCounter] = time_ms;
			n++;
		}
	}
	logChangedParameterCounter = incAndWrap(logChangedParameterCounter);
	return n;
}

void logMediumTick(void)
{

	const int64_t systime_ms = systemGetSysTimeMs();
	const int64_t timeSinceLastParameterLog = systime_ms - LastParameterLogTime_ms;
	if (timeSinceLastParameterLog > minTimeBetweenParameterLogging_ms)
	{
		int n=0;
		int i=0;

		secAndLogInitStatusMessageAddHeader(&statusDbfMessage, PARAMETER_STATUS_MSG);

		// There are two counters for which parameter to log next.
		// First one will log the parameter regardless if it is changed or not.
		// Next one will check a few more and log those if they have been changed.
		n+=addNextParameterToMsg(&statusDbfMessage, systime_ms);

		while ((n<8) && (i<32))
		{
			n+=addNextParameterToMsgIfChanged(&statusDbfMessage, systime_ms);
			i++;
		}

		if (n>0)
		{
			messageSendDbf(&statusDbfMessage);
		}

		LastParameterLogTime_ms = systime_ms;
	}
}
