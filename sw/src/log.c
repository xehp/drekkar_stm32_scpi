/*
log.c

Created 2018 by Henrik

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...
*/

#include<stdarg.h>
#include<stdio.h>

#include "miscUtilities.h"
#include "Dbf.h"
#include "eeprom.h"
#include "messageNames.h"
#include "log.h"
#include "serialDev.h"
#include "machineState.h"
#include "mathi.h"
#include "debugLog.h"
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

	#ifdef LOG_IN_DBF
	logInitAndAddHeader(&messageDbfTmpBuffer, msg);
	#else
	debug_print("logInt ");
	debug_print64(msg);
	debug_print(" ");
	#endif

	for(int i = 1; i < nParameters; i++)
	{
		int64_t value = va_arg(argptr, int64_t);
		#ifdef LOG_IN_DBF
		DbfSerializerWriteInt64(&messageDbfTmpBuffer, value);
		#else
		debug_print64(value);
		#endif
	}

	#ifdef LOG_IN_DBF
	messageSendDbf(&messageDbfTmpBuffer);
	#else
	debug_print("\n");
	#endif

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


static int64_t loggedParameterValues[LOGGING_MAX_NOF_PARAMETERS];
static int64_t loggedParameterTime[LOGGING_MAX_NOF_PARAMETERS];


static void sendParameterStatus(PARAMETER_CODES par, int64_t value)
{
	secAndLogInitStatusMessageAddHeader(&statusDbfMessage, PARAMETER_STATUS_MSG);
	DbfSerializerWriteInt32(&statusDbfMessage, par);
	DbfSerializerWriteInt64(&statusDbfMessage, value);
	messageSendDbf(&statusDbfMessage);
}

static void logParameter(PARAMETER_CODES par, int64_t value, int time_ms)
{
	sendParameterStatus(par, value);
	loggedParameterValues[par] = value;
	loggedParameterTime[par] = time_ms;
}

// Returns -1 if no status update for the parameter is wanted.
static int getMinLoggingIntervall_ms(PARAMETER_CODES par)
{
	switch (par)
	{
		case DEVICE_ID: return 5000;
		case TARGET_TIME_S:
		case REPORTED_EXT_AC_VOLTAGE_MV:
		case TEMP1_C:
		case TEMP2_C:
		case FAN1_HZ:
		case FAN2_HZ:
		case REPORTED_ERROR:
		case MICRO_AMPS_PER_UNIT_AC:
		case SYS_TIME_MS:
		case MEASURED_LEAK_AC_CURRENT_MA:
		default: return -1;
	}
}

static int logParameterIfChanged(PARAMETER_CODES par, int64_t value, int logInterval_ms)
{
	const int64_t t = systemGetSysTimeMs();
	const int64_t timeSincePreviousLogging = t - loggedParameterTime[par];

	// Has it been logged?
	if ((timeSincePreviousLogging >= logInterval_ms) || (loggedParameterTime[par] == 0))
	{
		// It has not been logged for a while or not at all so log it now.
		logParameter(par, value, t);
		return 1;
	}

	// Is value changed since last status update?
	if (value == loggedParameterValues[par])
	{
		// Unchanged, so no need to log it now.
		return 0;
	}

	logParameter(par, value, t);
	return 1;
}

/*
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
*/

int logIfParameterChanged(PARAMETER_CODES par, int64_t value)
{
	if (par < 0)
	{
		// Shall not happen
		return 0;
	}

	const int t = getMinLoggingIntervall_ms(par);
	if( t < 0)
	{
		// No status update wanted for this one.
		return 0;
	}

	if (par >= SIZEOF_ARRAY(loggedParameterValues))
	{
		// Do not have a previous logged value for these.
		sendParameterStatus(par, value);
		return 1;
	}

	return logParameterIfChanged(par, value, t);
}

void logResetTime()
{
	for (int i = 0; i< SIZEOF_ARRAY(loggedParameterTime); i++)
	{
		loggedParameterTime[i]=0;
	}
}
