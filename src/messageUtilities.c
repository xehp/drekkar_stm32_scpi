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


#if (defined __linux__) || (defined __WIN32)

void DbfLogBuffer(const char *prefix, const unsigned char *bufPtr, int bufLen)
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
