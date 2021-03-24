/*
machineState.c


Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2019-03-15
Created, Some methods moved from ports.h to this new file.
Henrik Bjorkman

*/

#include "cfg.h"
#include "systemInit.h"
#include "eeprom.h"
#include "messageNames.h"
#include "log.h"
#include "messageUtilities.h"
//#include "flashMemory.h"
#include "serialDev.h"


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


#if defined __linux__ || defined __WIN32

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


#if defined __linux__ || defined __WIN32

void messageLogBuffer(const char *prefix, const unsigned char *bufPtr, int bufLen)
{
	char str[4096];
	DbfUnserializer dbfUnserializer;
	DbfUnserializerInit(&dbfUnserializer, bufPtr, bufLen);
	DbfUnserializerReadAllToString(&dbfUnserializer, str, sizeof(str));
	printf("%s\n",prefix);
	printf("    dbf: %s\n",str);

	// Logging data in hex also.
	printf("    hex:");
	for(int i=0;i<bufLen;i++)
	{
		printf(" %02x", (unsigned char)bufPtr[i]);
	}
	printf("\n");
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

#elif (defined __arm__)

void messageSendDbf(DbfSerializer *bytePacket)
{
	DbfSerializerWriteCrc(bytePacket);
	const char *msgPtr=DbfSerializerGetMsgPtr(bytePacket);
	const int msgLen=DbfSerializerGetMsgLen(bytePacket);
	serialPutChar(DEV_USART1, DBF_BEGIN_CODEID);
	serialWrite(DEV_USART1, msgPtr, msgLen);
	serialPutChar(DEV_USART1, DBF_END_CODEID);
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
	uart_print("dbf ");
	uart_print(buf);
	uart_print("\n");
	#endif
	DbfSerializerInit(bytePacket);
}

void messageSendShortDbf(int32_t code)
{
	DbfSerializerInit(&messageDbfTmpBuffer);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, code);
	messageSendDbf(&messageDbfTmpBuffer);
}

#else
#error
#endif
