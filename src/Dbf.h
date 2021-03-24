/*
 * Dbf.h
 *
 *  Created on: Apr 3, 2018
 *      Author: henrik
 */

#ifndef DBF_H_
#define DBF_H_

#include <stdint.h>
#include <ctype.h>


#define DBF_EXT_CODEID 0x80
#define DBF_EXT_CODEMASK 0x80
#define DBF_EXT_DATANBITS 7
#define DBF_EXT_DATAMASK ((1 << 7) - 1)

#define DBF_PINT_CODEID 0x40
#define DBF_PINT_DATANBITS 6
#define DBF_PINT_DATAMASK ((1 << 6) - 1)

#define DBF_NINT_CODEID 0x20
#define DBF_NINT_DATANBITS 5
#define DBF_NINT_DATAMASK ((1 << 5) - 1)

#define DBF_FMTCRC_CODEID 0x10
#define DBF_FMTCRC_DATANBITS 4
#define DBF_FMTCRC_DATAMASK ((1 << 4) - 1)

#define DBF_SPEC_CODEID 0x08
#define DBF_SPEC_DATANBITS 3
#define DBF_SPEC_DATAMASK ((1 << 3) - 1)

#define DBF_END_CODEID 0x01
#define DBF_BEGIN_CODEID 0x00


void dbfDebugLog(const char *str);


enum
{
	DBF_INT_BEGIN_CODE = 0,
	DBF_STR_BEGIN_CODE = 1
};

enum
{
	DBF_ENCODER_IDLE = 0,
	DBF_ENCODING_INT = 1,
	DBF_ENCODING_STR = 2
};

typedef struct {
	char buffer[80];
	unsigned int pos;
	int encoderState;
} DbfSerializer;

// TODO Some way to know/check after if we tried to write more than there was room for in the message.

void DbfSerializerInit(DbfSerializer *dbfSerializer);

void DbfSerializerWriteInt32(DbfSerializer *dbfSerializer, int32_t i);
void DbfSerializerWriteInt64(DbfSerializer *dbfSerializer, int64_t i);

void DbfSerializerWriteString(DbfSerializer *dbfSerializer, const char *str);

void DbfSerializerResetMesssage(DbfSerializer *dbfSerializer);

void DbfSerializerWriteCrc(DbfSerializer *dbfSerializer);

const char* DbfSerializerGetMsgPtr(const DbfSerializer *dbfSerializer);

unsigned int DbfSerializerGetMsgLen(const DbfSerializer *dbfSerializer);



typedef enum
{
	DbfAsciiCodeState,
	DbfInitialCodeState,
	DbfIntegerCodeState,
	DbfStringCodeState,
	DbfEndCodeState,
	DbfErrorState,
} DbfCodeStateEnum;

typedef enum
{
	DbfNCT, // NOTHING_CODE_TYPE
	DbfEXT, // EXTENSION_CODE_TYPE
	DbfINT, // POSITIVE_NUMBER_CODE_TYPE
	DbfNEG, // NEGATIVE_NUMBER_CODE_TYPE
	DbfCRC, // FMTCRC_CODE_TYPE, format or CRC 
	DbfSCT, // SPECIAL_CODE_TYPE currently used for format, but will be used for repeat.
	DbfEMC, // ENDOFMSG_CODE_TYPE
	DbfECT  // ERROR_CODE_TYPE
} DbfCodeTypesEnum;


typedef enum
{
	DBF_OK_CRC=0,
	DBF_NO_CRC=1,
	DBF_BAD_CRC=-1
} DBF_CRC_RESULT;


typedef struct
{
	const unsigned char *msgPtr;

	unsigned int msgSize;

	DbfCodeStateEnum decodeState;

	unsigned int readPos;
} DbfUnserializer;

// TODO Some way to know/check after if we tried to read more than there was in the message.

DBF_CRC_RESULT DbfUnserializerInit(DbfUnserializer *dbfUnserializer, const unsigned char *msgPtr, unsigned int msgSize);

DbfCodeStateEnum DbfUnserializerReadCodeState(DbfUnserializer *dbfUnserializer);

int32_t DbfUnserializerReadInt32(DbfUnserializer *dbfUnserializer);
int64_t DbfUnserializerReadInt64(DbfUnserializer *dbfUnserializer);


int DbfUnserializerReadString(DbfUnserializer *dbfUnserializer, char* bufPtr, int bufLen);

int DbfUnserializerReadIsNextString(DbfUnserializer *dbfUnserializer);

int DbfUnserializerReadIsNextInt(DbfUnserializer *dbfUnserializer);

int DbfUnserializerReadIsNextEnd(DbfUnserializer *dbfUnserializer);

DBF_CRC_RESULT DbfUnserializerReadCrc(DbfUnserializer *dbfUnserializer);

DbfCodeTypesEnum DbfUnserializerGetNextType(const DbfUnserializer *dbfUnserializer, unsigned int idx);

int DbfUnserializerGetIntRev(const DbfUnserializer *dbfUnserializer, unsigned int e);

void DbfUnserializerReadCrcAndLog(DbfUnserializer *dbfUnserializer);

char DbfUnserializerIsOk(DbfUnserializer *dbfUnserializer);

void DbfUnserializerReadAllToString(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize);

#define DBF_RCV_TIMEOUT_MS 5000

// Buffer size, not in bytes but in number of 32bit words,
// buffer size shall be a full number of 32 bit words.
// We depend on that in other parts of the program
// when messages are copied.
#define DBF_BUFFER_SIZE_32BIT 32
#define BYTES_PER_32BIT_WORD 4

typedef enum
{
	DbfRcvInitialState,
	DbfRcvReceivingTxtState,
	DbfRcvReceivingMessageState,
	DbfRcvMessageReadyState,
	DbfRcvTxtReceivedState,
	DbfRcvDbfReceivedState,
	DbfRcvDbfReceivedMoreExpectedState,
	DbfRcvErrorState,
} DbfReveiverCodeStateEnum;


typedef struct
{
	unsigned char buffer[DBF_BUFFER_SIZE_32BIT*BYTES_PER_32BIT_WORD];
	unsigned int msgSize;
	DbfReveiverCodeStateEnum receiverState;
	int timeoutCounter;

	//uint64_t clear_time_stamp;
	uint64_t first_time_stamp;
	//uint64_t latest_time_stamp;

} DbfReceiver;

// This must be called any other DbfReceiver functions.
// Can also be used to reset the buffer so next message can be received.
void DbfReceiverInit(DbfReceiver *dbfReceiver);

// Call this at every character received. Returns >0 when there is a message to process.
int DbfReceiverProcessCh(DbfReceiver *dbfReceiver, unsigned char ch);

int DbfReceiverIsDbf(const DbfReceiver *dbfReceiver);
int DbfReceiverIsTxt(const DbfReceiver *dbfReceiver);

// This is intended to be called at a regular interval. It is used to check for timeouts in the receiving of messages.
void DbfReceiverTick(DbfReceiver *dbfReceiver, unsigned int deltaMs);

// Note, this is not same as DbfUnserializerReadString. This gives the entire message in ascii.
int DbfReceiverToString(DbfReceiver *dbfReceiver, const char* bufPtr, int bufLen);
int DbfReceiverLogRawData(const DbfReceiver *dbfReceiver);


#endif /* DBF_H_ */
