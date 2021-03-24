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

	#if (defined __arm__)
	debug_print("logInt ");
	debug_print64(msg);
	debug_print(" ");
	debug_print(getLogMessageName(msg));
	debug_print(" ");
	#endif

	logInitAndAddHeader(&messageDbfTmpBuffer, msg);

    for(int i = 1; i < nParameters; i++) {
    	int64_t value = va_arg(argptr, int64_t);
    	DbfSerializerWriteInt64(&messageDbfTmpBuffer, value);
		#if (defined __arm__)
    	debug_print64(value);
		#endif
	}

	messageSendDbf(&messageDbfTmpBuffer);
	#if (defined __arm__)
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

static int logParameterIfChanged(PARAMETER_CODES par, int64_t value, int logInterval)
{
	const int64_t t = systemGetSysTimeMs();

	// Is value is changed?
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
		return logParameterIfChanged(par, value, 1000);
	}
	return 0;
}

void logResetTime()
{
	for (int i = 0; i< SIZEOF_ARRAY(loggedParameterTime); i++)
	{
		loggedParameterTime[i]=0;
	}
}


#if defined __linux__ || defined __WIN32
int logPortsErrorToString(char *bufPtr, int bufSize, uint16_t errorCode)
{
	const char* name=getPortsErrorName(errorCode);
	if (name!=NULL)
	{
		snprintf(bufPtr, bufSize, " %s", name);
	}
	else
	{
		snprintf(bufPtr, bufSize, " portsError_%d", errorCode);
	}
	return strlen(bufPtr);
}


void logStatusMessageToString(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	if ((bufPtr == NULL) || (bufSize<=0) || (bufSize >= 0x70000000))
	{
		return;
	}

	/*const int64_t senderId =*/ DbfUnserializerReadInt64(dbfUnserializer);
	const int statusMsgType = DbfUnserializerReadInt32(dbfUnserializer);

	const char* statusMsgTypeName = getStatusMessagesName(statusMsgType);

	switch(statusMsgType)
	{
		case VOLTAGE_STATUS_MSG:
		{
			/*long long int time_ms =*/ DbfUnserializerReadInt64(dbfUnserializer);
			long long int voltage_mV = DbfUnserializerReadInt64(dbfUnserializer);

			snprintf(bufPtr, bufSize, "AC voltage %lldmV",
				voltage_mV);

			break;
		}
		case TEMP_STATUS_MSG:
		{
			/*long long int time_ms =*/ DbfUnserializerReadInt64(dbfUnserializer);
			int sensorNumber = DbfUnserializerReadInt64(dbfUnserializer);
			long long int tempValue_C = DbfUnserializerReadInt64(dbfUnserializer);
			snprintf(bufPtr, bufSize, "Temp%d %lld", sensorNumber, tempValue_C);
			break;
		}
		case WEB_SERVER_STATUS_MSG:
		{
			long long int wallTime_ms = DbfUnserializerReadInt64(dbfUnserializer);
			snprintf(bufPtr, bufSize, "Web server is alive, time %lld ms", wallTime_ms);
			break;
		}
		default:
		{
			char tmpStr[4096];
			DbfUnserializerReadAllToString(dbfUnserializer, tmpStr, sizeof(tmpStr));

			if (statusMsgTypeName!=NULL)
			{
				snprintf(bufPtr, bufSize, "  %s %s", statusMsgTypeName, tmpStr);
			}
			else
			{
				snprintf(bufPtr, bufSize, "  STATUS_CODE_%d %s", statusMsgType, tmpStr);
			}
			break;
		}
	}
}

void logCommandMessageToString(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	if ((bufSize<=0) || (bufSize >= 0x70000000))
	{
		return;
	}

	const long long int senderId = DbfUnserializerReadInt64(dbfUnserializer);
	const long long int destId = DbfUnserializerReadInt64(dbfUnserializer);
	const long long int refNr = DbfUnserializerReadInt64(dbfUnserializer);
	const int cmdMsgType = DbfUnserializerReadInt32(dbfUnserializer);

	const char* statusMsgTypeName = getMessageCommandName(cmdMsgType);
	char name[256];
	if (statusMsgTypeName!=NULL)
	{
		snprintf(name, sizeof(name), "%s", statusMsgTypeName);
	}
	else
	{
		snprintf(name, sizeof(name), "cmd%d", cmdMsgType);
	}

	char tmp[512];
	if (destId==-1)
	{
		snprintf(tmp, sizeof(tmp), "from %lld, to all, #%lld, %s", senderId, refNr, name);
	}
	else
	{
		snprintf(tmp, sizeof(tmp), "from %lld, to %lld, #%lld, %s", senderId, destId, refNr, name);
	}

	switch(cmdMsgType)
	{
		case SET_CMD:
		{
			const int parId = DbfUnserializerReadInt32(dbfUnserializer);
			const char* parameterName = getParameterName(parId);
			if (parameterName)
			{
				snprintf(bufPtr, bufSize, "%s, %s", tmp, parameterName);
			}
			else
			{
				snprintf(bufPtr, bufSize, "%s, parameter%d", tmp, cmdMsgType);
			}
			break;
		}
		default:
		{
			snprintf(bufPtr, bufSize, "%s,", tmp);
			break;
		}
	}
}

void log_message_to_string(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	if ((bufPtr == NULL) || (bufSize<=0) || (bufSize >= 0x70000000))
	{
		return;
	}

	/*const int senderId =*/ DbfUnserializerReadInt64(dbfUnserializer);
	/*const int sequenceNr =*/ DbfUnserializerReadInt64(dbfUnserializer);
	// TODO check if we miss a log message by looking at sequence number.

	const int logMessageType = DbfUnserializerReadInt64(dbfUnserializer);

	//printf("log_message_to_string %d\n", logMessageType);

	char tmp[80];
	const char* tmpPtr=getLogMessageName(logMessageType);
	if (tmpPtr!=NULL)
	{
		snprintf(tmp, sizeof(tmp), "%s", tmpPtr);
	}
	else
	{
		snprintf(tmp, sizeof(tmp), "LogMsg%d", logMessageType);
	}


	switch(logMessageType)
	{
		/*case LOG_FREQ_CURRENT_VOLT_HBOX:
		case LOG_FREQ_CURRENT_VOLT_BUCK:
			logOldFormatsFreqCurrVoltToString(dbfUnserializer, bufPtr, bufSize);
			break;*/
		case LOG_FREQ_CURRENT_VOLT_INV:
			logFreqCurrVoltToStringInverter(dbfUnserializer, bufPtr, bufSize);
			break;

		case SCAN_DID_NOT_FIND_RESONANCE:
		{
			int64_t scanTopPreliminaryCurrentFound = DbfUnserializerReadInt64(dbfUnserializer);
			DbfUnserializerReadInt64(dbfUnserializer);
			int64_t meanValue = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: top current %lld (proprietary units), mean current %lld (proprietary units). Perhaps load is not correctly connected or no input power?", tmp, (long long int)scanTopPreliminaryCurrentFound, (long long int)meanValue);
			#else
			snprintf(bufPtr, bufSize, "%s: top current %I64d (proprietary units), mean current %I64d (proprietary units). Perhaps load is not correctly connected or no input power?", tmp, (long long int)scanTopPreliminaryCurrentFound, (long long int)meanValue);
			#endif
			break;
		}
		case SCAN_PRELIMINARY_RESONANCE_FOUND:
		{
			int64_t f = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t cTop_mA = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t cMean_mA = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t halfPeriod = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: Frequency %lld Hz, top current %lld mA, mean current %lld mA, halfPeriod %lld.", tmp, (long long int)f, (long long int)cTop_mA, (long long int)cMean_mA, (long long int)halfPeriod);
			#else
			snprintf(bufPtr, bufSize, "%s: Frequency %I64d Hz, top current %I64d mA, mean current %I64d mA, halfPeriod %I64d.", tmp, (long long int)f, (long long int)cTop_mA, (long long int)cMean_mA, (long long int)halfPeriod);
			#endif
			break;
		}
		case SCAN_NOW_AT_FREQ:
		{
			int64_t f_Hz = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: Frequency %lld Hz", tmp, (long long int)f_Hz);
			#else
			snprintf(bufPtr, bufSize, "%s: Frequency %I64d Hz", tmp, (long long int)f_Hz);
			#endif
			break;
		}
		case SCAN_HIGHER_BOUNDRY_FOUND:
		{
			int64_t scanTopCurrentFoundUp = DbfUnserializerReadInt64(dbfUnserializer);
			DbfUnserializerReadInt64(dbfUnserializer);
			int64_t f_Hz = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t measuredCurrent = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: Frequency %lld Hz, scanTopCurrent %lld, measuredCurrent %lld", tmp, (long long int)f_Hz, (long long int)scanTopCurrentFoundUp, (long long int)measuredCurrent);
			#else
			snprintf(bufPtr, bufSize, "%s: Frequency %I64d Hz, scanTopCurrent %I64d, measuredCurrent %I64d", tmp, (long long int)f_Hz, (long long int)scanTopCurrentFoundUp, (long long int)measuredCurrent);
			#endif
			break;
		}
		case SCAN_LOWER_BOUNDRY_FOUND:
		{
			int64_t scanTopCurrentFoundDown = DbfUnserializerReadInt64(dbfUnserializer);
			DbfUnserializerReadInt64(dbfUnserializer);
			int64_t f_Hz = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t measuredCurrent = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: Frequency %lld Hz, scanTopCurrent %lld, measuredCurrent %lld", tmp, (long long int)f_Hz, (long long int)scanTopCurrentFoundDown, (long long int)measuredCurrent);
			#else
			snprintf(bufPtr, bufSize, "%s: Frequency %I64d Hz, scanTopCurrent %I64d, measuredCurrent %I64d", tmp, (long long int)f_Hz, (long long int)scanTopCurrentFoundDown, (long long int)measuredCurrent);
			#endif
			break;
		}
		case SCAN_Q:
		{
			int64_t q = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "Q: %lld", (long long int)q);
			#else
			snprintf(bufPtr, bufSize, "Q: %I64d", (long long int)q);
			#endif
			break;
		}
		case SCAN_FAILED_TO_FIND_Q:
			snprintf(bufPtr, bufSize, " %s. Perhaps load is not correctly connected or no input power?", tmp);
			break;
		case SCAN_Q_LARGER_THAN_EXPECTED:
		case SCAN_Q_LESS_THAN_EXPECTED:
		{
			int64_t q = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: Q=%lld. Perhaps load is not correctly connected or no input power?", tmp, (long long int)q);
			#else
			snprintf(bufPtr, bufSize, "%s: Q=%I64d. Perhaps load is not correctly connected or no input power?", tmp, (long long int)q);
			#endif
			break;
		}
		case SCAN_CHANGE_TO_FREQUENCY_REGULATION:
		{
			int64_t halfPeriod = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t f_Hz = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: %lld Hz, halfPeriod %lld.", tmp, (long long int)f_Hz, (long long int)halfPeriod);
			#else
			snprintf(bufPtr, bufSize, "%s: %I64d Hz, halfPeriod %I64d.", tmp, (long long int)f_Hz, (long long int)halfPeriod);
			#endif
			break;
		}
		case SCAN_START_TUNING:
		case SCAN_TUNE_UP:
		case SCAN_TUNE_DOWN:
		case SCAN_TUNE_STAY:
		case SCAN_TUNING_STOP_HIGH:
		case SCAN_STOP_LOW:
		{
			int64_t f_Hz = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t newHalfPeriod = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t scanTuningStartCurrent = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t scanTuningAlternativeCurrent = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t prevHalfPeriod = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: %lld Hz, period %lld, %lld %lld %lld", tmp, (long long int)f_Hz, (long long int)newHalfPeriod, (long long int)prevHalfPeriod, (long long int)scanTuningStartCurrent, (long long int)scanTuningAlternativeCurrent);
			#else
			snprintf(bufPtr, bufSize, "%s: %I64d Hz, period %I64d, %I64d %I64d %I64d", tmp, (long long int)f_Hz, (long long int)newHalfPeriod, (long long int)prevHalfPeriod, (long long int)scanTuningStartCurrent, (long long int)scanTuningAlternativeCurrent);
			#endif
			break;
		}
		case SCAN_COOLING_DOWN_WILL_START_SOON:
		{
			int64_t starting_in_s = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: %lld ms", tmp, (long long int)starting_in_s);
			#else
			snprintf(bufPtr, bufSize, "%s: %I64d ms", tmp, (long long int)starting_in_s);
			#endif
			break;
		}
		case PORTS_ERROR:
		{
			int64_t errorCode = DbfUnserializerReadInt64(dbfUnserializer);
			const char *errorName = getPortsErrorName(errorCode);
			if (errorName !=NULL)
			{
				snprintf(bufPtr, bufSize, "%s: %s", tmp, errorName);
			}
			else
			{
				#ifndef __WIN32
				snprintf(bufPtr, bufSize, "%s: %lld", tmp, (long long int)errorCode);
				#else
				snprintf(bufPtr, bufSize, "%s: %I64d", tmp, (long long int)errorCode);
				#endif
			}
			break;
		}
		case REG_WAITING_FOR_INPUT_POWER:
		{
			int64_t measuredInputVoltage_mV = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t neededInputVoltage_mV = DbfUnserializerReadInt64(dbfUnserializer);
			snprintf(bufPtr, bufSize, "%s: input voltage %lld mV DC, need %lld", tmp, (long long int)measuredInputVoltage_mV, (long long int)neededInputVoltage_mV);
			break;
		}
		default:
		{
			snprintf(bufPtr, bufSize, "%s: ", tmp);
			const int n = strlen(bufPtr);
			DbfUnserializerReadAllToString(dbfUnserializer, bufPtr+n, bufSize-n);
			break;
		}
	}
}

/*
void log_voltage_message_to_string(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	if ((bufPtr == NULL) || (bufSize<=0) || (bufSize >= 0x70000000))
	{
		return;
	}

	int logMessageCode = DbfUnserializerReadInt64(dbfUnserializer);

	const char* tmp=getVoltageLogMessageName(logMessageCode);
	if (tmp!=NULL)
	{
		snprintf(bufPtr, bufSize, "%s: ", tmp);
	}
	else
	{
		snprintf(bufPtr, bufSize, "VoltageLogMsg_%d: ", (int)logMessageCode);
	}
	const int n = strlen(bufPtr);
	DbfUnserializerReadAllToString(dbfUnserializer, bufPtr+n, bufSize-n);
}
*/

void log_reply_message_to_string(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	if ((bufPtr == NULL) || (bufSize<=0) || (bufSize >= 0x70000000))
	{
		return;
	}

	long cmd = DbfUnserializerReadInt64(dbfUnserializer);
	const char *commandName = getMessageCommandName(cmd);
	if (commandName != NULL)
	{
		snprintf(bufPtr, bufSize, "%s", commandName);
	}
	else
	{
		snprintf(bufPtr, bufSize, "CMD%ld", cmd);
	}
}

void logDbfAsHexAndText(const unsigned char *msgPtr, int msgLen)
{
	// Logging data in hex.
	printf(LOG_PREFIX "hex: ");
	for(int i=0;i<msgLen;i++)
	{
		printf(" %02x", msgPtr[i]);
	}
	printf(LOG_SUFIX);

	// Log as raw text.
	char str[4096];
	DbfUnserializer dbfUnserializer;
	DbfUnserializerInit(&dbfUnserializer, msgPtr, msgLen);
	DbfUnserializerReadAllToString(&dbfUnserializer, str, sizeof(str));
	printf(LOG_PREFIX "raw: %s" LOG_SUFIX,str);

	// Log in clear text as much as known
	char buf[4096];
	//DbfLogBuffer("", msgPtr, msgLen);
	decodeDbfToText(msgPtr, msgLen, buf, sizeof(buf));
	printf(LOG_PREFIX "txt: %s" LOG_SUFIX, buf);
}

void decodeDbfToText(const unsigned char *msgPtr, int msgLen, char *bufPtr, int bufSize)
{
	if ((bufPtr == NULL) || (bufSize<=0) || (bufSize >= 0x70000000))
	{
		return;
	}

	DbfUnserializer dbfUnserializer;
	DbfUnserializerInit(&dbfUnserializer, msgPtr, msgLen);

	if (DbfUnserializerReadIsNextEnd(&dbfUnserializer))
	{
		snprintf(bufPtr, bufSize, "empty dbf");
	}
	else if (DbfUnserializerReadIsNextString(&dbfUnserializer))
	{
		snprintf(bufPtr, bufSize, "unknown dbf");
	}
	else if (DbfUnserializerReadIsNextInt(&dbfUnserializer))
	{
		int messageCategory = DbfUnserializerReadInt64(&dbfUnserializer);

		switch(messageCategory)
		{
			case LOG_CATEGORY:
			{
				char tmpStr[4096];
				log_message_to_string(&dbfUnserializer, tmpStr, sizeof(tmpStr));
				snprintf(bufPtr, bufSize, "log, %s", tmpStr);
			}	break;
			case STATUS_CATEGORY:
			{
				char tmpStr[4096];
				logStatusMessageToString(&dbfUnserializer, tmpStr, sizeof(tmpStr));
				snprintf(bufPtr, bufSize, "status, %s", tmpStr);
				break;
			}
			case COMMAND_CATEGORY:
			{
				char tmpStr[4096];
				logCommandMessageToString(&dbfUnserializer, tmpStr, sizeof(tmpStr));
				snprintf(bufPtr, bufSize, "command, %s", tmpStr);
				break;
			}
			default:
			{
				const char* messageCategoryName = getMessageCategoryName(messageCategory);
				if (messageCategoryName!=NULL)
				{
					snprintf(bufPtr, bufSize,"%s", messageCategoryName);
				}
				else
				{
					snprintf(bufPtr, bufSize,"msgCategory%d", messageCategory);
				}
				break;
			}
		}
	}
	else
	{
		snprintf(bufPtr, bufSize,LOG_PREFIX "unsupported dbf" LOG_SUFIX);
	}

	if (!DbfUnserializerReadIsNextEnd(&dbfUnserializer))
	{
		int n = strlen(bufPtr);
		if(n<bufSize)
		{
			char tmpStr[4096];
			DbfUnserializerReadAllToString(&dbfUnserializer, tmpStr, sizeof(tmpStr));
			snprintf(bufPtr+n, bufSize-n, ", '%s'", tmpStr);
		}
	}
}


// This shall only log the message to console. Not process it.
void decodeMessageToText(const DbfReceiver *dbfReceiver, char *buf, int bufSize)
{
	if (DbfReceiverIsTxt(dbfReceiver))
	{
		//DbfReceiverLogRawData(dbfReceiver);
		snprintf(buf, bufSize, "txt: '%s'", dbfReceiver->buffer);
	}
	else if (DbfReceiverIsDbf(dbfReceiver))
	{
		decodeDbfToText(dbfReceiver->buffer, dbfReceiver->msgSize, buf, bufSize);
	}
	else
	{
		printf("unknown message");
	}

}

// This shall only log the message to console. Not process it.
void linux_sim_log_message_from_target(const DbfReceiver *dbfReceiver)
{
	// Logging data in hex.
	printf(LOG_PREFIX "hex: ");
	for(int i=0;i<dbfReceiver->msgSize;i++)
	{
		printf(" %02x", (unsigned char)dbfReceiver->buffer[i]);
	}
	printf(LOG_SUFIX);

	// Log as raw text.
	if (DbfReceiverIsDbf(dbfReceiver))
	{
		char str[4096];
		DbfUnserializer dbfUnserializer;
		DbfUnserializerInit(&dbfUnserializer, dbfReceiver->buffer, dbfReceiver->msgSize);
		DbfUnserializerReadAllToString(&dbfUnserializer, str, sizeof(str));
		printf(LOG_PREFIX "raw: %s" LOG_SUFIX,str);
	}

	// Log in clear text as much as known
	char buf[4096];
	//DbfLogBuffer("", dbfReceiver->buffer, dbfReceiver->msgSize);
	decodeMessageToText(dbfReceiver, buf, sizeof(buf));
	printf(LOG_PREFIX "msg: %s" LOG_SUFIX, buf);
}

#endif
