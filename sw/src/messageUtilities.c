/*
messageUtilities.c


Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2019-03-15
Created, Some methods moved from ports.h to this new file.
Henrik Bjorkman

*/

#include "cfg.h"
#include "miscUtilities.h"
#include "systemInit.h"
#include "Dbf.h"
#include "eeprom.h"
#include "messageNames.h"
#include "debugLog.h"
#include "log.h"
#include "serialDev.h"
#include "messageUtilities.h"


DbfSerializer messageDbfTmpBuffer =
{
	{0},
	0,
	0
//#ifdef __linux__
//	,0
//#endif
};


#if (defined __linux__) || (defined __WIN32)
// Not implemented
#else
void messageInitAndAddCategoryAndSender(DbfSerializer *dbfSerializer, MESSAGE_CATEGORY category)
{
	DbfSerializerInit(dbfSerializer);
	DbfSerializerWriteInt32(dbfSerializer, category);
	DbfSerializerWriteInt64(dbfSerializer, ee.deviceId);
}
#endif



void messageReplyOkInitAndAddHeader(COMMAND_CODES cmd, int64_t replyToId, int64_t replyToRef)
{
	messageInitAndAddCategoryAndSender(&messageDbfTmpBuffer, REPLY_OK_CATEGORY);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, replyToId);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, replyToRef);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, cmd);

}

void messageReplyOK(COMMAND_CODES cmd, int64_t replyToId, int64_t replyToRef)
{
	messageReplyOkInitAndAddHeader(cmd, replyToId, replyToRef);
	messageSendDbf(&messageDbfTmpBuffer);
}

void messageReplyNOK(COMMAND_CODES cmd, NOK_REASON_CODES reasonCode, int parameter, int64_t replyToId, int64_t replyToRef)
{
	messageInitAndAddCategoryAndSender(&messageDbfTmpBuffer, REPLY_NOK_CATEGORY);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, replyToId);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, replyToRef);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, reasonCode);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, cmd);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, parameter);
	messageSendDbf(&messageDbfTmpBuffer);
}


void messageReplyToSetCommand(int16_t parameterId, int64_t value, int64_t replyToId, int64_t replyToRef)
{
	messageReplyOkInitAndAddHeader(SET_CMD, replyToId, replyToRef);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, parameterId);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, value);
	messageSendDbf(&messageDbfTmpBuffer);
}


void messageReplyToGetCommand(int32_t parameterId, int64_t value, int64_t replyToId, int64_t replyToRef)
{
	messageReplyOkInitAndAddHeader(GET_CMD, replyToId, replyToRef);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, parameterId);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, value);
	messageSendDbf(&messageDbfTmpBuffer);
}


#if (defined __linux__) || (defined __WIN32) || (defined DEBUG_DECODE_DBF)

int decodeCommandMessageToString(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	int c = 0;

	if ((bufSize<=0) || (bufSize >= 0x70000000))
	{
		return c;
	}

	const long long int senderId = DbfUnserializerReadInt64(dbfUnserializer);
	const long long int destId = DbfUnserializerReadInt64(dbfUnserializer);
	const long long int refNr = DbfUnserializerReadInt64(dbfUnserializer);
	const int cmdMsgType = DbfUnserializerReadInt32(dbfUnserializer);

	const char* statusMsgTypeName = getMessageCommandName(cmdMsgType);
	if (statusMsgTypeName!=NULL)
	{
		c += utility_strccpy(bufPtr, statusMsgTypeName, bufSize-c);
	}
	else
	{
		c += utility_strccpy(bufPtr+c, statusMsgTypeName, bufSize-c);
		c += utility_lltoa(cmdMsgType, bufPtr+c, 10, bufSize-c);
	}

	c += utility_strccpy(bufPtr+c, "from ", bufSize-c);
	c += utility_lltoa(senderId, bufPtr+c, 10, bufSize-c);

	if (destId==-1)
	{
		c += utility_strccpy(bufPtr+c, ", to all, #", bufSize-c);
	}
	else
	{
		c += utility_strccpy(bufPtr+c, ", to ", bufSize-c);
		c += utility_lltoa(destId, bufPtr+c, 10, bufSize-c);
		c += utility_strccpy(bufPtr+c, ", ", bufSize-c);
	}

	c += utility_lltoa(refNr, bufPtr+c, 10, bufSize-c);

	switch(cmdMsgType)
	{
		case SET_CMD:
		{
			const int parId = DbfUnserializerReadInt32(dbfUnserializer);
			const char* parameterName = getParameterName(parId);
			if (parameterName)
			{
				c += utility_strccpy(bufPtr+c, ", ", bufSize-c);
				c += utility_strccpy(bufPtr+c, parameterName, bufSize-c);
			}
			else
			{
				c += utility_strccpy(bufPtr+c, ", parameter", bufSize-c);
				c += utility_lltoa(cmdMsgType, bufPtr+c, 10, bufSize-c);
			}
			break;
		}
		default:
		{
			c += utility_strccpy(bufPtr+c, ",", bufSize-c);
			break;
		}
	}

	c += utility_strccpy(bufPtr, " ", bufSize-c);
	c += DbfUnserializerReadAllToString(dbfUnserializer, bufPtr+c, bufSize-c);

	return c;
}


int decode_log_message_to_string(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	int c = 0;

	if ((bufPtr == NULL) || (bufSize<=0) || (bufSize >= 0x70000000))
	{
		return c;
	}

	/*const int senderId =*/ DbfUnserializerReadInt64(dbfUnserializer);
	/*const int sequenceNr =*/ DbfUnserializerReadInt64(dbfUnserializer);
	// TODO check if we miss a log message by looking at sequence number.

	const int logMessageTypeCode = DbfUnserializerReadInt64(dbfUnserializer);

	//printf("log_message_to_string %d\n", logMessageType);

	const char* tmpPtr = getLogMessageName(logMessageTypeCode);
	if (tmpPtr != NULL)
	{
		c += utility_strccpy(bufPtr+c, tmpPtr, bufSize-c);
	}
	else
	{
		c += utility_strccpy(bufPtr+c, "LogMsg", bufSize-c);
		c += utility_lltoa(logMessageTypeCode, bufPtr+c, 10, bufSize-c);
	}

	c += utility_strccpy(bufPtr+c, " ", bufSize-c);
	c += DbfUnserializerReadAllToString(dbfUnserializer, bufPtr+c, bufSize-c);

	return c;
}

int decodeStatusMessageToString(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	int c = 0;

	if ((bufPtr == NULL) || (bufSize<=0) || (bufSize >= 0x70000000))
	{
		return c;
	}

	/*const int64_t senderId =*/ DbfUnserializerReadInt64(dbfUnserializer);
	const int statusMsgTypeCode = DbfUnserializerReadInt32(dbfUnserializer);

	const char* statusMsgTypeName = getStatusMessagesName(statusMsgTypeCode);

	if (statusMsgTypeName != NULL)
	{
		c += utility_strccpy(bufPtr+c, statusMsgTypeName, bufSize-c);
	}
	else
	{
		c += utility_strccpy(bufPtr+c, "StatusMsg", bufSize-c);
		c += utility_lltoa(statusMsgTypeCode, bufPtr+c, 10, bufSize-c);
	}

	c += utility_strccpy(bufPtr+c, " ", bufSize-c);
	c += DbfUnserializerReadAllToString(dbfUnserializer, bufPtr+c, bufSize-c);

	return c;
}


int decodeDbfToText(const unsigned char *msgPtr, int msgLen, char *bufPtr, int bufSize)
{
	int n = 0;
	if ((bufPtr == NULL) || (bufSize<=0) || (bufSize >= 0x70000000))
	{
		return n;
	}

	DbfUnserializer dbfUnserializer;
	DbfUnserializerInit(&dbfUnserializer, msgPtr, msgLen);

	if (DbfUnserializerReadIsNextEnd(&dbfUnserializer))
	{
		n += utility_strccpy(bufPtr+n, "empty dbf", bufSize-n);
	}
	else if (DbfUnserializerReadIsNextString(&dbfUnserializer))
	{
		n += utility_strccpy(bufPtr+n, "unknown dbf", bufSize-n);
	}
	else if (DbfUnserializerReadIsNextInt(&dbfUnserializer))
	{
		int messageCategory = DbfUnserializerReadInt64(&dbfUnserializer);

		switch(messageCategory)
		{
			case LOG_CATEGORY:
			{
				n += utility_strccpy(bufPtr+n, "log, ", bufSize-n);
				n += decode_log_message_to_string(&dbfUnserializer, bufPtr+n, bufSize-n);
			}	break;
			case STATUS_CATEGORY:
			{
				n += utility_strccpy(bufPtr+n, "status, ", bufSize-n);
				n += decodeStatusMessageToString(&dbfUnserializer, bufPtr+n, bufSize-n);
				break;
			}
			case COMMAND_CATEGORY:
			{
				n += utility_strccpy(bufPtr+n, "command, ", bufSize-n);
				n += decodeCommandMessageToString(&dbfUnserializer, bufPtr+n, bufSize-n);
				break;
			}
			default:
			{
				const char* messageCategoryName = getMessageCategoryName(messageCategory);
				if (messageCategoryName!=NULL)
				{
					n += utility_strccpy(bufPtr+n, "cat, ", bufSize-n);
					utility_strccpy(bufPtr+n, messageCategoryName, bufSize-n);
				}
				else
				{
					n += utility_strccpy(bufPtr+n, "unknown", bufSize-n);
					utility_lltoa(messageCategory, bufPtr+n, 10, bufSize-n);
				}
				break;
			}
		}
	}
	else
	{
		n += utility_strccpy(bufPtr+n, LOG_PREFIX "unsupported dbf" LOG_SUFIX, bufSize-n);
	}

	if (!DbfUnserializerReadIsNextEnd(&dbfUnserializer))
	{
		if(n<bufSize)
		{
			n += utility_strccpy(bufPtr+n, ", ", bufSize-n);
			DbfUnserializerReadAllToString(&dbfUnserializer, bufPtr+n, bufSize-n);
		}
	}

	SYSTEM_ASSERT(n<bufSize);
	bufPtr[n] = 0;
	return n;
}

// This shall only log the message to console. Not process it.
int decodeMessageToText(const DbfReceiver *dbfReceiver, char *bufPtr, int bufSize)
{
	int n = 0;
	if (DbfReceiverIsTxt(dbfReceiver))
	{
		//DbfReceiverLogRawData(dbfReceiver);
		n += utility_strccpy(bufPtr+n, "txt: ", bufSize-n);
		n += utility_strccpy(bufPtr+n, dbfReceiver->buffer, bufSize-n);
	}
	else if (DbfReceiverIsDbf(dbfReceiver))
	{
		n += decodeDbfToText(dbfReceiver->buffer, dbfReceiver->msgSize, bufPtr+n, bufSize-n);
	}
	else
	{
		n+=utility_strccpy(bufPtr+n, "unknown message", bufSize-n);
	}
	return n;
}
#endif

#if (defined __linux__) || (defined __WIN32)

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


#if (defined __linux__) || (defined __WIN32) || (defined DEBUG_DECODE_DBF)

void messageLogBuffer(const char *prefix, const unsigned char *bufPtr, int bufLen)
{
	char str[4096];
	DbfUnserializer dbfUnserializer;
	DbfUnserializerInit(&dbfUnserializer, bufPtr, bufLen);
	DbfUnserializerReadAllToString(&dbfUnserializer, str, sizeof(str));
	debug_print(prefix);
	debug_print("\n    dbf: ");
	debug_print(str);
	debug_print("\n");

	// Logging data in hex also.
	debug_print("    hex:");
	for(int i=0;i<bufLen;i++)
	{
		debug_print_hex_8((unsigned char)bufPtr[i]);
	}
	debug_print("\n");
}

#endif


#if (defined __linux__) || (defined __WIN32)

void messageSendDbf(DbfSerializer *bytePacket)
{
	// TODO
}

void messageSendShortDbf(int32_t code)
{
	// TODO
}
#error

#else

void messageSendDbf(DbfSerializer *bytePacket)
{
	DbfSerializerWriteCrc(bytePacket);
	const char *msgPtr=DbfSerializerGetMsgPtr(bytePacket);
	const int msgLen=DbfSerializerGetMsgLen(bytePacket);
	#ifdef COMMAND_ON_USART1
	serialPutChar(DEV_USART1, DBF_BEGIN_CODEID);
	serialWrite(DEV_USART1, msgPtr, msgLen);
	serialPutChar(DEV_USART1, DBF_END_CODEID);
	#endif
	#ifdef COMMAND_ON_LPUART1
	serialPutChar(DEV_LPUART1, DBF_BEGIN_CODEID);
	serialWrite(DEV_LPUART1, msgPtr, msgLen);
	serialPutChar(DEV_LPUART1, DBF_END_CODEID);
	#endif
	#ifdef COMMAND_ON_USART2
	serialPutChar(DEV_USART2, DBF_BEGIN_CODEID);
	serialWrite(DEV_USART2, msgPtr, msgLen);
	serialPutChar(DEV_USART2, DBF_END_CODEID);
	#endif

	#ifdef DEBUG_DECODE_DBF
	char buf[1024];
	decodeDbfToText(msgPtr, msgLen, buf, sizeof(buf));
	debug_print("DBF ");
	debug_print(buf);
	debug_print("\n");
	#endif

	DbfSerializerInit(bytePacket);
}

void messageSendShortDbf(int32_t code)
{
	DbfSerializerInit(&messageDbfTmpBuffer);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, code);
	messageSendDbf(&messageDbfTmpBuffer);
}

#endif


#if (defined __linux__) || (defined __WIN32) || (defined DEBUG_DECODE_DBF)

void DbfUnserializerReadCrcAndLog(DbfUnserializer *dbfUnserializer)
{
	DBF_CRC_RESULT r = DbfUnserializerReadCrc(dbfUnserializer);
	switch(r)
	{
		case DBF_BAD_CRC: debug_print("Bad CRC\n");break;
		case DBF_OK_CRC: debug_print("OK CRC\n");break;
		default: debug_print("No CRC\n");break;
	}
}


/*int DbfReceiverToString(DbfReceiver *dbfReceiver, const char* bufPtr, int bufLen)
{
	if (DbfReceiverIsTxt(dbfReceiver))
	{
		int n = ((bufLen-1) > dbfReceiver->msgSize) ? dbfReceiver->msgSize : (bufLen-1);
		memcpy(bufPtr, dbfReceiver->buffer, n);
		bufPtr[n] = 0;
	}
	else if (DbfReceiverIsDbf(dbfReceiver))
	{
		// Not implemented yet
		bufPtr[0] = 0;
	}
	return 0;
}*/


// Reads the remaining message and logs its contents.
int DbfUnserializerReadAllToString(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	int c = 0;

	if ((bufPtr == NULL) || (bufSize<=0) || (bufSize >= 0x70000000))
	{
		return c;
	}

	const char* separator="";
	*bufPtr=0;
	while(!DbfUnserializerReadIsNextEnd(dbfUnserializer) && bufSize>0)
	{
		if (DbfUnserializerReadIsNextString(dbfUnserializer))
		{
			c += utility_strccpy(bufPtr+c, separator, bufSize-c);
			c += utility_strccpy(bufPtr+c, "\"", bufSize-c);
			c += DbfUnserializerReadString(dbfUnserializer, bufPtr+c, bufSize-c);
			c += utility_strccpy(bufPtr+c, "\"", bufSize-c);
		}
		else if (DbfUnserializerReadIsNextInt(dbfUnserializer))
		{
			int64_t i = DbfUnserializerReadInt64(dbfUnserializer);
			c += utility_strccpy(bufPtr+c, separator, bufSize);
			c += utility_lltoa(i, bufPtr+c, 10, bufSize-c);
		}
		else
		{
			c += utility_strccpy(bufPtr, " ?", bufSize);
		}
		separator=" ";
	}

	return c;
}


int DbfReceiverLogRawData(const DbfReceiver *dbfReceiver)
{
	if (DbfReceiverIsTxt(dbfReceiver))
	{
		debug_print(LOG_PREFIX "DbfReceiverLogRawData: ");
		const unsigned char *ptr = dbfReceiver->buffer;
		int n = dbfReceiver->msgSize;
		for(int i=0; i<n; ++i)
		{
			int ch = ptr[i];
			/*if (utility_isgraph(ch))
			{
				debug_putchar(ch);
			}
			else*/ if (utility_isprint(ch))
			{
				debug_putchar(ch);
			}
			else if (ch == 0)
			{
				// do nothing
			}
			else
			{
				debug_print_hex_8(ch);
			}
		}
	}
	else if (DbfReceiverIsDbf(dbfReceiver))
	{
		messageLogBuffer("", dbfReceiver->buffer, dbfReceiver->msgSize);
	}
	return 0;
}
#endif
