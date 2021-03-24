/*
 * Dbf.cpp
 *
 *  Created on: Apr 3, 2018
 *      Author: henrik
 */

#include <stdio.h>
#include <ctype.h>



#if ((defined __arm__) && (!defined __linux__))
#include "debugLog.h"
#elif defined __linux__ || defined __WIN32
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include "utime.h"
#endif

#include "cfg.h"
#include "crc32.h"
#include "Dbf.h"


#define LOG_PREFIX "Dbf:  "
#define LOG_SUFIX "\n"

/*
DrekkarBinaryFormat (DBF)

The purpose is to encode messages containing numbers and strings into a compact binary format.
Sort of like LEB128 but this encodes strings also and will tell if next value is string or int.
A requirement is easy debugging. It must be possible to unpack a message into its components
called codes. So some basic formatting information is also part of the encoding.
Small numbers and common ASCII characters must be encoded with a single byte.

The message is composed of codes.
A code is made up of a start sub code and zero or more extension sub codes.
The start code will tell if it is a positive or negative number or some special code.
The range depends on implementation (if 32 bit or 64 bit variables are used).
It is recommended to use 64 bits and that is assumed below.

Characters are sent as n = Unicode - 64. By subtracting the unicde code with 64
characters "@ABCDE...xyz~" etc are sent as 0-63 and characters space to '?' are sent
as -1 to -32. These are then coded same as numbers and for those ranges these fit
in a byte. Which was our requirement for compactness.



Below bits in a sub code are shown as 'b'. All bits together are called n.


When displayed as readable text numbers and strings etc are separated by ',' or space.
Strings are quoted. Example message:
0, 1, -1, 63, 64, -1000, 65536, "hello?  abc!", "one, more", 4711


If the data did not fit in one byte (what is left after formatting info)
then the least significant bits are written first and the more significant
are added in extension codes (as many as needed).

A "code" is made up of one or more sub codes. 
First a start code (any sub code except the extension code) followed
by zero or more extension codes.


Sub codes:

1bbbbbbb
	Extension code with 7 more bits of data.
	Sent after one of the other sub codes below if there was not room for all data.
	least significant is sent in first sub code and then more and more significant
	until only zeroes remain in the more significant part of the number.
	Those zeroes do not need to be sent.

01bbbbbb
	Number sub code. This is for a positive number n.
	the 6 least significant data bits of n are written to the bbbbbb bits.
	If the number was larger than 63 some Extension codes will be needed.
	The number can be an integer to be shown as decimal, hexadecimal or
	part of a string. Depending on any preceding format codes.

001bbbbb
	Number sub code. This is for a negative number n.
	Note that Unicode characters might also be encoded as negative numbers
	since they are subtracted with 64 before being encoded. This so that the most
	common 7 bit ascii characters will fit without using extension codes.

0001bbbb
	Format code or CRC
	If this appears last in a message it is a CRC. Otherwise it is format code.

	CRC
		Where n is the CRC, it is typically 32 bits using extension bits.
			The DbfBegin, DbfEnd & networking options are typically not included in the CRC.

	Format codes.
		0
			One or more signed integers follow. To be displayed as positive or negative decimal.
			This is also default in a each message.
		1
			A string with Unicode characters follow. To be displayed as quoted string.
			The string continues until next non number sub code.
			Display using same escape sequences as JSON if needed.
		3, 4, 5...
			Reserved, up to 2^64 special codes are possible so they should not run out.
			But only 0-15 can be encoded without extension sub codes so it is preferred to keep within that.
			Suggested additions:
				3
				  One or more unsigned numbers to be displayed in hexadecimal follows.
				  These can still be encoded using 001bbbbb. If all bits are one that can be
				  encoded as -1 and then displayed as 0xFFFFFFFFFFFFFFFF.
				4
				  embedded byte buffer follows, to be displayed in hex.
				  Perhaps first number after this shall be used to tell how
				  many bytes there is in the byte buffer. In each of the following
				  number codes 4 bytes of binary data is then stored.
				5
				  binary64 floating point format.
				  64-bit IEEE 754 floating point, 1 sign, 11 exponent and 52 mantissa bits
				  mantissa is perhaps better called coefficient or significand.
				  Probably it shall be encoded into 2 codes.
				  	  1) sign and significand/mantissa
				  	  2) exponent
				  Note that the exponent is then for power of 2 not power of 10 as in Scientific notation.
				  If that can be changed to 10 without to much complexity it would make a log so much more readable.
				6 and 7
				  Begin and end of a JSON style object '{' '}' respectively.
                                  Remember to diplay with the ':' as delimiter also.
				8 and 9
				  Begin and end of a JSON style array '[' ']' respectively.
				15
				  Do nothing. Do not change format.


00001bbb
	Currently reserved for future use. Perhaps to be used as below:

		If this code is received in the beginning of a message, before any
		of these: 01bbbbbb, 001bbbbb, 0001bbbb it is a version code. If this is
		received inside a message it is a repetition code.

		repetition code

	In future the plan is to use this for repeating codes.
	Such as if previous code is repeated a number of times then this is sent instead
	to say how many times. Only number codes (positive or negative) can be repeated
	this way. n will tell how many extra repetitions. The number of extra repetitions
	shall be n+1.

	Currently if "Extension code" or "repetition code" is found as the first code in a
			message (before any	of these: 01bbbbbb, 001bbbbb, 0001bbbb) then its an error
			(unless a future version of DBF say otherwise).

		version code

			In future this might be used to indicate DBF versions.
			If so it shall be the first code after "Begin of a DBF message" code.


000001bb
	Currently reserved for future use. Perhaps to be used as below:

			If this code is received in the beginning of a message, before any
			of these: 01bbbbbb, 001bbbbb, 0001bbbb it is a networking option. If this is
	received inside a message it is something else, not defined yet.

			Networking codes (mandatory)

	Networking options are not regarded do be part of the message body for
	which the CRC is calculated mentioned for 0001bbbb.

			Other
	If this code is received and is not known to the receiver its an error.
				The rest of the message shall be discarded. Unlike optional networking codes
				that shall be ignored if unknown to the receiver.

0000001b
	Currently reserved for future use. Perhaps to be used as below:

		If this code is received in the beginning of a message, before any
		of these: 01bbbbbb, 001bbbbb, 0001bbbb it is a networking option. If this is
	received inside a message it is something else, not defined yet.

		Networking codes (optional)

	Networking options are not regarded do be part of the message body for
	which the CRC is calculated mentioned for 0001bbbb.

	Perhaps this will be used for a Time To Live (TTL) counter used when routing/forwarding
	messages. Not being part of the CRC means it can be decremented without needed to
	recalculate the CRC.
		Other
	If this code is received in the middle of a message and it is unknown to the receiver
	then it may be ignored and shall not be displayed.

00000001
	End of DBF.	No extension subcode is allowed together with this sub code.
	Any data sent after this is considered non DBF until a "Begin of a message"
	is received.
	It is OK to repeat this code until there is a message send.
	Note that this is not same as "End of sub message", see above.

00000000
	Begin of a DBF message. Also used as separator between messages.
	No extension subcode is allowed together with this code.
	The message shall end with an "End of message" code.


*/

#if defined __linux__ || defined __WIN32
void dbfDebugLog(const char *str)
{
	fprintf(stdout, "%s\n",str);
	fflush(stdout);
}
#elif (defined __arm__)
void dbfDebugLog(const char *str)
{
	debug_print(str);
	debug_print("\n");
}
#else
#define dbfDebugLog(str)
#endif


//struct DbfSerializer dbfSerializer;
/*
static void logHex(FILE* stream, const char *prefix, const unsigned char *ptr, unsigned int len, const char *sufix)
{
	fprintf(stream, "%s", prefix);
	for(unsigned int i = 0; i < len; i++)
	{
		fprintf(stream, "%02x", ptr[i]);
	}
	fprintf(stream, "%s", sufix);
	fflush(stream);
}
*/

void DbfSerializerInit(DbfSerializer *dbfSerializer)
{
	//dbfDebugLog("DbfSerializerInit");
	dbfSerializer->pos = 0;
	dbfSerializer->encoderState = DBF_ENCODER_IDLE;
//#if defined __linux__ || defined __WIN32
//	dbfSerializer->debugState = 0;
//#endif
};

void DbfSerializerResetMesssage(DbfSerializer *dbfSerializer)
{
	//dbfDebugLog("DbfSerializerResetMesssage");
	dbfSerializer->pos = 0;
	dbfSerializer->encoderState = DBF_ENCODER_IDLE;
//#if defined __linux__ || defined __WIN32
//	dbfSerializer->debugState = 0;
//#endif
}

void DbfSerializerPutByte(DbfSerializer *dbfSerializer, char b)
{
//#if defined __linux__ || defined __WIN32
//	assert(dbfSerializer->debugState == 0);
//#endif

	if (dbfSerializer->pos<sizeof(dbfSerializer->buffer))
	{
		dbfSerializer->buffer[dbfSerializer->pos] = b;
		dbfSerializer->pos++;
	}
	else
	{
		dbfDebugLog("DbfSerializerPutByte full");
	}
}


/**
 * Parameters:
 * dbfSerializer: pointer to the struct that the data is written to.
 * c: Tells the type of data to be written.
 * n: The number of data bits that will fit in one byte together with c.
 * d: The actual data
 */
static void DbfSerializerEncodeData32(DbfSerializer *dbfSerializer, unsigned int c, unsigned int n, uint32_t d)
{
	const unsigned int m = (1<<n)-1; // mask for data to be written together with format code.

	// Send the type of code part and as many bits as will fit in first byte.
	DbfSerializerPutByte(dbfSerializer, c + (d & m));
	d = d >> n;

	// Then as many extension sub codes as needed for the more significant bits.
	while (d > 0)
	{
		DbfSerializerPutByte(dbfSerializer, DBF_EXT_CODEID + (d & DBF_EXT_DATAMASK));
		d = d >> DBF_EXT_DATANBITS;
	}
}

/**
 * Parameters: See DbfSerializerEncodeData32
 */
static void DbfSerializerEncodeData64(DbfSerializer *dbfSerializer, unsigned int c, unsigned int n, uint64_t d)
{
	const unsigned int m = (1<<n)-1; // mask for data to be written together with format code.

	// Send the type of code part and as many bits as will fit in first byte.
	DbfSerializerPutByte(dbfSerializer, c + (d & m));
	d = d >> n;

	// Then as many extension sub codes as needed for the more significant bits.
	while (d > 0)
	{
		DbfSerializerPutByte(dbfSerializer, DBF_EXT_CODEID + (d & DBF_EXT_DATAMASK));
		d = d >> DBF_EXT_DATANBITS;
	}
}

void DbfSerializerWriteCrc(DbfSerializer *dbfSerializer)
{
	const uint32_t crc = crc32_calculate((const unsigned char *)dbfSerializer->buffer, dbfSerializer->pos);
	DbfSerializerEncodeData32(dbfSerializer, DBF_FMTCRC_CODEID, DBF_FMTCRC_DATANBITS, crc);
}


static void DbfSerializerWriteCode16(DbfSerializer *dbfSerializer, int16_t i)
{
	if (i>=0)
	{
		DbfSerializerEncodeData32(dbfSerializer, DBF_PINT_CODEID, DBF_PINT_DATANBITS, i);
	}
	else
	{
		DbfSerializerEncodeData32(dbfSerializer, DBF_NINT_CODEID, DBF_NINT_DATANBITS, -1-i);
	}
}

static void DbfSerializerWriteCode32(DbfSerializer *dbfSerializer, int32_t i)
{
	if (i>=0)
	{
		DbfSerializerEncodeData32(dbfSerializer, DBF_PINT_CODEID, DBF_PINT_DATANBITS, i);
	}
	else
	{
		DbfSerializerEncodeData32(dbfSerializer, DBF_NINT_CODEID, DBF_NINT_DATANBITS, -1-i);
	}
}

static void DbfSerializerWriteCode64(DbfSerializer *dbfSerializer, int64_t i)
{
	if (i>=0)
	{
		DbfSerializerEncodeData64(dbfSerializer, DBF_PINT_CODEID, DBF_PINT_DATANBITS, i);
	}
	else
	{
		DbfSerializerEncodeData64(dbfSerializer, DBF_NINT_CODEID, DBF_NINT_DATANBITS, -1-i);
	}
}


void DbfSerializerWriteInt32(DbfSerializer *dbfSerializer, int32_t i)
{
	//printf("DbfSerializerWriteInt %d\n", i);
	// If format is not already numeric then send the code "INT_BEGIN_CODE".
	switch(dbfSerializer->encoderState)
	{
		case DBF_ENCODING_INT:
			// Do nothing
			break;
		case DBF_ENCODER_IDLE:
			// Int is default so no need to send formating code.

			// But due to some other bug it is needed. Probably its due to unserializer having initial state as ascii.
			// When that has been fixed in all units then the line here can be removed.
			//DbfSerializerEncodeData32(dbfSerializer, DBF_SPEC_CODEID, DBF_SPEC_DATANBITS, DBF_INT_BEGIN_CODE);

			dbfSerializer->encoderState = DBF_ENCODING_INT;
			break;
		default:
			// Previous code was not an integer so we must first send the format code.
			// Using the CRC code to send the format.
			DbfSerializerEncodeData32(dbfSerializer, DBF_FMTCRC_CODEID, DBF_FMTCRC_DATANBITS, DBF_INT_BEGIN_CODE);
			dbfSerializer->encoderState = DBF_ENCODING_INT;
			break;
	}
	DbfSerializerWriteCode32(dbfSerializer, i);
}

void DbfSerializerWriteInt64(DbfSerializer *dbfSerializer, int64_t i)
{
	//printf("DbfSerializerWriteInt %d\n", i);
	// If format is not already numeric then send the code "INT_BEGIN_CODE".
	switch(dbfSerializer->encoderState)
	{
		case DBF_ENCODING_INT:
			// Do nothing
			break;
		case DBF_ENCODER_IDLE:
			// Int is default so no need to send formating code.

			// But due to some other bug it is needed. Probably its due to unserializer having initial state as ascii.
			// When that has been fixed in all units then the line here can be removed.
			//DbfSerializerEncodeData32(dbfSerializer, DBF_SPEC_CODEID, DBF_SPEC_DATANBITS, DBF_INT_BEGIN_CODE);

			dbfSerializer->encoderState = DBF_ENCODING_INT;
			break;
		default:
			// Using DBF_CRC shall be used for format code.
			DbfSerializerEncodeData32(dbfSerializer, DBF_FMTCRC_CODEID, DBF_FMTCRC_DATANBITS, DBF_INT_BEGIN_CODE);
			dbfSerializer->encoderState = DBF_ENCODING_INT;
			break;
	}
	DbfSerializerWriteCode64(dbfSerializer, i);
}

void DbfSerializerWriteString(DbfSerializer *dbfSerializer, const char *str)
{
	//printf("DbfSerializerWriteString '%s'\n", str);
	// Send a string format code to tell receiver that it is a string that follows.
	// It is needed also if previous parameter was a string since this also separates strings.
	// Using DBF_CRC shall be used for format code.
	DbfSerializerEncodeData32(dbfSerializer, DBF_FMTCRC_CODEID, DBF_FMTCRC_DATANBITS, DBF_STR_BEGIN_CODE);
	dbfSerializer->encoderState = DBF_ENCODING_STR;
	while(*str)
	{
		int i = *str;
		DbfSerializerWriteCode16(dbfSerializer, i-64);
		str++;
	}
}

const char* DbfSerializerGetMsgPtr(const DbfSerializer *dbfSerializer)
{
//#if defined __linux__ || defined __WIN32
//	dbfSerializer->debugState = 1;
//#endif
	return dbfSerializer->buffer;
}

unsigned int DbfSerializerGetMsgLen(const DbfSerializer *dbfSerializer)
{
	return dbfSerializer->pos;
}



#if defined __linux__ || defined __WIN32 || __arm__
//#if 1

/*DbfCodeTypesEnum*/ uint8_t codeTypeTable[256] = {
		DbfNCT,DbfNCT,DbfNCT,DbfNCT,DbfNCT,DbfNCT,DbfNCT,DbfNCT,DbfSCT,DbfSCT,DbfSCT,DbfSCT,DbfSCT,DbfSCT,DbfSCT,DbfSCT,
		DbfCRC,DbfCRC,DbfCRC,DbfCRC,DbfCRC,DbfCRC,DbfCRC,DbfCRC,DbfCRC,DbfCRC,DbfCRC,DbfCRC,DbfCRC,DbfCRC,DbfCRC,DbfCRC,
		DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,
		DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,DbfNEG,
		DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,
		DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,
		DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,
		DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,DbfINT,
		DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,
		DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,
		DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,
		DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,
		DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,
		DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,
		DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,
		DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT,DbfEXT};

#define GET_CODE_TYPE(i) codeTypeTable[i]
#else
// Doing same as above but using more CPU time and using less memory.
static /*DbfCodeTypesEnum*/ uint8_t getCodeType(uint8_t i)
{
	if (i>=128)
	{
		return DbfEXT; // EXTENSION_CODE_TYPE
	}
	else if (i>=64)
	{
		return DbfINT; // Positive INTEGER_CODE_TYPE
	}
	else if (i>=32)
	{
		return DbfNEG; // NEGATIVE_CODE_TYPE
	}
	else if (i>=16)
	{
		return DbfCRC; // CRC_CODE_TYPE
	}
	else if (i>=8)
	{
		return DbfSCT; // SPECIAL_CODE_TYPE
	}
	return DbfNCT; // NOTHING_CODE_TYPE
}
#define GET_CODE_TYPE(i) getCodeType(i)
#endif
/*
static uint8_t GET_CODE_TYPE(uint8_t i)
{
	uint8_t c = getCodeType(i);
	uint8_t t = codeTypeTable[i];
	assert(c==t);
	return t;
}
*/


/**
 * Returns DBF_OK_CRC if OK. A non zero error code if not OK (there was a CRC error).
 */
DBF_CRC_RESULT DbfUnserializerInit(DbfUnserializer *dbfUnserializer, const unsigned char *msgPtr, unsigned int msgSize)
{
	dbfUnserializer->msgPtr = msgPtr;
	dbfUnserializer->msgSize = msgSize;
	dbfUnserializer->decodeState = DbfIntegerCodeState;
	dbfUnserializer->readPos = 0;

	DBF_CRC_RESULT r = DbfUnserializerReadCrc(dbfUnserializer);
	switch(r)
	{
		case DBF_OK_CRC:
			return DBF_OK_CRC;
		default:
		case DBF_BAD_CRC:
			//dbfDebugLog("Bad or missing CRC");
			dbfUnserializer->msgSize = 0;
			return r;
	}
}


unsigned int DbfUnserializerFindBeginOfCode(const DbfUnserializer *dbfUnserializer, unsigned int idx)
{
	for(;;)
	{
		if (idx==0)
		{
			break;
		}
		idx--;
		unsigned char ch = dbfUnserializer->msgPtr[idx];
		if ((ch & DBF_EXT_CODEMASK) == DBF_EXT_CODEID)
		{
			// This extends current code
		}
		else
		{
			// This is the beginning of code.
			break;
		}
	}
	return idx;
}

// The return value will point on the beginning of next code if any,
unsigned int DbfUnserializerFindNextCode(const DbfUnserializer *dbfUnserializer, unsigned int idx)
{
	for(;;)
	{
		idx++;
		if (idx >= dbfUnserializer->msgSize)
		{
			break;
		}
		const unsigned char ch = dbfUnserializer->msgPtr[idx];
		if ((ch & DBF_EXT_CODEMASK) == DBF_EXT_CODEID)
		{
			// This extends current code
		}
		else
		{
			// This is part of next code.
			break;
		}
	}
	return idx;
}



DbfCodeTypesEnum DbfUnserializerGetNextType(const DbfUnserializer *dbfUnserializer, unsigned int idx)
{
	if (idx >= dbfUnserializer->msgSize)
	{
		return DbfEMC; // ENDOFMSG_CODE_TYPE
	}
	else
	{
		const unsigned char ch = dbfUnserializer->msgPtr[idx];

		return GET_CODE_TYPE(ch);
	}
	return DbfECT; // ERROR_CODE_TYPE
}

// The code is decoded backwards, from its end to its beginning.
// codeEndIndex shall be the index of the first character in next code.
static int32_t DbfUnserializerDecodeData32(const DbfUnserializer *dbfUnserializer, int codeEndIndex)
{
	int32_t i = 0;

	while (codeEndIndex>0)
	{
		codeEndIndex--;
		unsigned char ch = dbfUnserializer->msgPtr[codeEndIndex];

		const /*DbfCodeTypesEnum*/ uint8_t ct = GET_CODE_TYPE(ch);

		switch (ct)
		{
			case DbfEXT:
				i = (i << DBF_EXT_DATANBITS) | (ch & DBF_EXT_DATAMASK);
				break;
			case DbfINT:
				i = (i << DBF_PINT_DATANBITS) | (ch & DBF_PINT_DATAMASK);
				return i;
			case DbfNEG:
				i = (i << DBF_NINT_DATANBITS) | (ch & DBF_NINT_DATAMASK);
				i = -1 - i;
				return i;
			case DbfCRC:
				i = (i << DBF_FMTCRC_DATANBITS) | (ch & DBF_FMTCRC_DATAMASK);
				return i;
			case DbfSCT:
				i = (i << DBF_SPEC_DATANBITS) | (ch & DBF_SPEC_DATAMASK);
				return i;
			default:
				#if defined __linux__ || defined __WIN32
				printf("Unknown code 0x%x\n", ch);
				#endif
				return 0;
		}
	}
	return i;
}

// The code is decoded backwards, from its end to its beginning.
// codeEndIndex shall be the index of the first character in next code.
static int64_t DbfUnserializerDecodeData64(const DbfUnserializer *dbfUnserializer, int codeEndIndex)
{
	int64_t i = 0;

	while (codeEndIndex>0)
	{
		codeEndIndex--;
		unsigned char ch = dbfUnserializer->msgPtr[codeEndIndex];

		const /*DbfCodeTypesEnum*/ uint8_t ct = GET_CODE_TYPE(ch);

		switch (ct)
		{
			case DbfEXT:
				i = (i << DBF_EXT_DATANBITS) | (ch & DBF_EXT_DATAMASK);
				break;
			case DbfINT:
				i = (i << DBF_PINT_DATANBITS) | (ch & DBF_PINT_DATAMASK);
				return i;
			case DbfNEG:
				i = (i << DBF_NINT_DATANBITS) | (ch & DBF_NINT_DATAMASK);
				i = -1 - i;
				return i;
			case DbfCRC:
				i = (i << DBF_FMTCRC_DATANBITS) | (ch & DBF_FMTCRC_DATAMASK);
				return i;
			case DbfSCT:
				i = (i << DBF_SPEC_DATANBITS) | (ch & DBF_SPEC_DATAMASK);
				return i;
			default:
				#if defined __linux__ || defined __WIN32
				printf("Unknown code 0x%x\n", ch);
				#endif
				return 0;
		}
	}
	return i;
}

/**
 * Get the last code in the buffer.
 */
int DbfUnserializerDecodeDataRev64(const DbfUnserializer *dbfUnserializer, unsigned int endIdx, int *nextCodeType, int64_t *nextCodeData)
{
	unsigned int beginIdx = DbfUnserializerFindBeginOfCode(dbfUnserializer, endIdx);
	*nextCodeType = DbfUnserializerGetNextType(dbfUnserializer, beginIdx);
	*nextCodeData = DbfUnserializerDecodeData64(dbfUnserializer, endIdx);
	return beginIdx;
}



/**
 * Translate from one enum to another.
 */
static DbfCodeStateEnum DbfUnserializerEvaluateSpecCode(int c)
{
	switch(c)
	{
		case DBF_INT_BEGIN_CODE: return DbfIntegerCodeState;
		case DBF_STR_BEGIN_CODE: return DbfStringCodeState;
		default: return DbfInitialCodeState;
	}
}

/**
 * This will check if next code is a formating code. One that
 * tells the type of following data, if it is a number or string.
 */
static void DbfUnserializerReadSpecial(DbfUnserializer *dbfUnserializer)
{
	const DbfCodeTypesEnum t = DbfUnserializerGetNextType(dbfUnserializer, dbfUnserializer->readPos);

	switch(t)
	{
		case DbfCRC:
		case DbfSCT:
		{
			// This was a format code.
			unsigned int nextIndex = DbfUnserializerFindNextCode(dbfUnserializer, dbfUnserializer->readPos);
			int64_t specialCode = DbfUnserializerDecodeData64(dbfUnserializer, nextIndex);
			dbfUnserializer->decodeState = DbfUnserializerEvaluateSpecCode(specialCode);
			dbfUnserializer->readPos = nextIndex;
			break;
		}
		case DbfEMC:
		{
			dbfUnserializer->decodeState = DbfEndCodeState;
			break;
		}
		default:
			// This was not a special etc code
			// positive and negative integer codes are to be left.
			break;
	}
}


DbfCodeStateEnum DbfUnserializerReadCodeState(DbfUnserializer *dbfUnserializer)
{
	DbfUnserializerReadSpecial(dbfUnserializer);
	return dbfUnserializer->decodeState;
}

int32_t DbfUnserializerReadInt32(DbfUnserializer *dbfUnserializer)
{
	// Read and skip next code if it is a special code for message formating etc.
	DbfUnserializerReadSpecial(dbfUnserializer);

	const unsigned int nextCodeIndex = DbfUnserializerFindNextCode(dbfUnserializer, dbfUnserializer->readPos);
	const int32_t i = DbfUnserializerDecodeData32(dbfUnserializer, nextCodeIndex);
	dbfUnserializer->readPos = nextCodeIndex;
	return i;
}

int64_t DbfUnserializerReadInt64(DbfUnserializer *dbfUnserializer)
{
	// Read and skip next code if it is a special code for message formating etc.
	DbfUnserializerReadSpecial(dbfUnserializer);

	const unsigned int nextCodeIndex = DbfUnserializerFindNextCode(dbfUnserializer, dbfUnserializer->readPos);
	const int64_t i = DbfUnserializerDecodeData64(dbfUnserializer, nextCodeIndex);
	dbfUnserializer->readPos = nextCodeIndex;
	return i;
}


int DbfUnserializerReadString(DbfUnserializer *dbfUnserializer, char* bufPtr, int bufLen)
{
	// Read and skip next code if it is a special code for message formating etc.
	DbfUnserializerReadSpecial(dbfUnserializer);

	int n = 0;
	while(n<(bufLen-1))
	{
		// Read as long as it is a code that represents characters (that is positive or negative numbers)
		int t = DbfUnserializerGetNextType(dbfUnserializer, dbfUnserializer->readPos);
		if ((t == DbfINT) || (t == DbfNEG))
		{
			unsigned int e = DbfUnserializerFindNextCode(dbfUnserializer, dbfUnserializer->readPos);
			int i = DbfUnserializerDecodeData32(dbfUnserializer, e);
			*bufPtr++ = i+64;
			n++;
			dbfUnserializer->readPos = e;
		}
		else
		{
			// Any other code means the string has ended.
			// Perhaps we want to have a special code for repeated characters?
			// If so something more is needed here.
			break;
		}
	}
	*bufPtr = 0;
	return n;
}

int DbfUnserializerReadIsNextString(DbfUnserializer *dbfUnserializer)
{
	return (DbfUnserializerReadCodeState(dbfUnserializer) == DbfStringCodeState);
}

int DbfUnserializerReadIsNextInt(DbfUnserializer *dbfUnserializer)
{
	return (DbfUnserializerReadCodeState(dbfUnserializer) == DbfIntegerCodeState);
}

int DbfUnserializerReadIsNextEnd(DbfUnserializer *dbfUnserializer)
{
	if (dbfUnserializer->readPos>=dbfUnserializer->msgSize)
	{
		return 1;
	}
	return (DbfUnserializerReadCodeState(dbfUnserializer) == DbfEndCodeState);
}

/**
 * This will take the last code in the buffer, decode and check if CRC is OK.
 */
DBF_CRC_RESULT DbfUnserializerReadCrc(DbfUnserializer *dbfUnserializer)
{
	if (dbfUnserializer->msgSize==0)
	{
		// No message
		return DBF_NO_CRC;
	}

	int nextCodeType;
	int64_t nextCodeData;
	int lastCodePos = DbfUnserializerDecodeDataRev64(dbfUnserializer, dbfUnserializer->msgSize, &nextCodeType, &nextCodeData);
	// TODO Shall we allow using numbers code to send the CRC?
	//if ((nextCodeType != DbfCRC) && (nextCodeType != DbfINT) && (nextCodeType != DbfNEG))
	if (nextCodeType != DbfCRC)
	{
		//dbfDebugLog("Last code was not a CRC");
		return DBF_NO_CRC;
	}
	else
	{
		// the CRC has been read, shorten the message.
		dbfUnserializer->msgSize = lastCodePos;
		uint32_t receivedCrc=nextCodeData;
		uint32_t calculatedCrc = crc32_calculate(dbfUnserializer->msgPtr, dbfUnserializer->msgSize);
		if (receivedCrc != calculatedCrc)
		{
			/*#if defined __linux__ || defined __WIN32
			printf(LOG_PREFIX "Wrong CRC: got %04x, expected %04x\n", receivedCrc, calculatedCrc);
			#else
			dbfDebugLog("bad CRC");
			#endif*/
			return DBF_BAD_CRC;
		}
	}

	return DBF_OK_CRC;
}


#if defined __linux__ || defined __WIN32
void DbfUnserializerReadCrcAndLog(DbfUnserializer *dbfUnserializer)
{
	DBF_CRC_RESULT r = DbfUnserializerReadCrc(dbfUnserializer);
	switch(r)
	{
		case DBF_BAD_CRC: printf("Bad CRC\n");break;
		case DBF_OK_CRC: printf("OK CRC\n");break;
		default: printf("No CRC\n");break;
	}
}
#endif


static int8_t DbfReceiverIsFull(DbfReceiver * dbfReceiver)
{
	return (dbfReceiver->msgSize>=sizeof(dbfReceiver->buffer));
}

// This is an internal helper function. Not intended for users to call.
// Returns 0 if OK
static int8_t DbfReceiverStoreByte(DbfReceiver * dbfReceiver, char b)
{
	if (dbfReceiver->msgSize<sizeof(dbfReceiver->buffer))
	{
		dbfReceiver->buffer[dbfReceiver->msgSize] = b;
		dbfReceiver->msgSize++;
		return 0;
	}
	return -1;
}




void DbfReceiverInit(DbfReceiver *dbfReceiver)
{
	//dbfDebugLog("DbfReceiverInit");

	#if defined __linux__ || defined __WIN32
	//dbfReceiver->clear_time_stamp = buf_time_us();
	dbfReceiver->first_time_stamp = 0;
	//dbfReceiver->latest_time_stamp = 0;
	#endif

	dbfReceiver->msgSize = 0;
	dbfReceiver->receiverState = DbfRcvInitialState;
	dbfReceiver->timeoutCounter = 0;
}


/**
 * Returns
 * -1 : Something wrong
 *  0 : Nothing to report, not yet a full message.
 *  >0 : A full message has been received (number of bytes received).
 */
int DbfReceiverProcessCh(DbfReceiver *dbfReceiver, unsigned char ch)
{
	#if defined __linux__ || defined __WIN32
	const int64_t currentTimeUs = buf_time_us();
	//dbfReceiver->latest_time_stamp = currentTimeUs;
	#endif

	switch (dbfReceiver->receiverState)
	{
		case DbfRcvInitialState:
		{
			// In this state: waiting for the first character of a message, DBF or ascii.
			switch(ch)
			{
				case DBF_BEGIN_CODEID:
					// A DBF message begin.
					dbfReceiver->msgSize = 0;
					dbfReceiver->timeoutCounter = 0;
					dbfReceiver->receiverState = DbfRcvReceivingMessageState;
					#if defined __linux__ || defined __WIN32
					dbfReceiver->first_time_stamp = currentTimeUs;
					#endif
					//dbfDebugLog("DBF begin");
					break;
				case DBF_END_CODEID:
					// this was the end of a BDF message.
					dbfReceiver->msgSize = 0;
					dbfReceiver->timeoutCounter = 0;
					#if defined __linux__ || defined __WIN32
					dbfReceiver->first_time_stamp = currentTimeUs;
					#endif
					break;
				case '\r':
				case '\n':
					// This was the end of an ascii text line.
					dbfReceiver->msgSize = 0;
					dbfReceiver->timeoutCounter = 0;
					#if defined __linux__ || defined __WIN32
					dbfReceiver->first_time_stamp = currentTimeUs;
					#endif
					break;
				default:
					dbfReceiver->timeoutCounter = 0;
					dbfReceiver->msgSize = 0;
					if ((ch>=' ') && (ch<='~'))
					{
						// This looks like the begin of an ascii string.
						DbfReceiverStoreByte(dbfReceiver, ch);
						dbfReceiver->receiverState = DbfRcvReceivingTxtState;
						#if defined __linux__ || defined __WIN32
						dbfReceiver->first_time_stamp = currentTimeUs;
						#endif
						//dbfDebugLog("txt begin");
					}
					else
					{
						// Not DBF and not regular 7 bit ascii, ignore.
						// TODO we should count these, perhaps report some useful error message?
						//dbfDebugLog("unexpected char");
					}
					break;
			}
			break;
		}
		case DbfRcvReceivingTxtState:
		{
			switch(ch)
			{
				case DBF_BEGIN_CODEID:
					// A DBF message begin while receiving ascii line?
					// Ascii lines are expected to end with CR or LF.
					dbfDebugLog("DBF inside txt");
					dbfReceiver->msgSize = 0;
					dbfReceiver->receiverState = DbfRcvReceivingMessageState;
					dbfReceiver->timeoutCounter = 0;
					break;
				case '\r':
				case '\n':
					// Adding a terminating zero (instead of the cr or lf)
					if (DbfReceiverStoreByte(dbfReceiver, 0) == 0)
					{
						dbfReceiver->receiverState = DbfRcvTxtReceivedState;
						//dbfDebugLog("txt end");
					}
					else
					{
						dbfDebugLog("txt buffer full");
					}
					return dbfReceiver->msgSize;
				default:
					DbfReceiverStoreByte(dbfReceiver, ch);
					if (DbfReceiverIsFull(dbfReceiver))
					{
						dbfReceiver->receiverState = DbfRcvTxtReceivedState;
						return dbfReceiver->msgSize;
					}
					break;
			}
			break;
		}
		case DbfRcvReceivingMessageState:
		{
			dbfReceiver->timeoutCounter = 0;
			switch(ch)
			{
				case DBF_BEGIN_CODEID:
				{
					if (dbfReceiver->msgSize == 0)
					{
						// Stay in this state
						//dbfReceiver->receiverState = DbfRcvReceivingMessageState;
					}
					else
					{
						dbfReceiver->receiverState = DbfRcvDbfReceivedMoreExpectedState;
						//dbfDebugLog("dbf end and begin");
						return dbfReceiver->msgSize;
					}
					break;
				}
				case DBF_END_CODEID:
				{
					if (dbfReceiver->msgSize == 0)
					{
						dbfReceiver->receiverState = DbfRcvInitialState;
					}
					else
					{
						dbfReceiver->receiverState = DbfRcvDbfReceivedState;
						//dbfDebugLog("dbf end");
						return dbfReceiver->msgSize;
					}
					break;
				}
				default:
				{
					if (DbfReceiverStoreByte(dbfReceiver, ch) != 0)
					{
						// Discard the message, it was too long.
						dbfDebugLog("dbf buffer full");
						dbfReceiver->msgSize = 0;
						dbfReceiver->timeoutCounter = 0;
						dbfReceiver->receiverState = DbfRcvInitialState;
					}
					break;
				}
			}
			break;
		}
		default:
		case DbfRcvTxtReceivedState:
		case DbfRcvMessageReadyState:
		case DbfRcvErrorState:
			dbfDebugLog("msg cleared");
			dbfReceiver->msgSize = 0;
			return -1;
	}
	return 0;
}

int DbfReceiverIsDbf(const DbfReceiver *dbfReceiver)
{
	return ((dbfReceiver->receiverState == DbfRcvDbfReceivedState) || (dbfReceiver->receiverState == DbfRcvDbfReceivedMoreExpectedState));
}

int DbfReceiverIsTxt(const DbfReceiver *dbfReceiver)
{
	return (dbfReceiver->receiverState == DbfRcvTxtReceivedState);
}

// TODO Use system time instead of needing delta time as parameter.
void DbfReceiverTick(DbfReceiver *dbfReceiver, unsigned int deltaMs)
{
	switch (dbfReceiver->receiverState)
	{
		case DbfRcvReceivingMessageState:
		{
			dbfReceiver->timeoutCounter += deltaMs;
			if (dbfReceiver->timeoutCounter>DBF_RCV_TIMEOUT_MS)
			{
				if (dbfReceiver->msgSize != 0)
				{
					dbfDebugLog("timeout");
					dbfReceiver->msgSize = 0;
				}
				dbfReceiver->receiverState = DbfRcvInitialState;
				dbfReceiver->timeoutCounter = 0;
			}
			break;
		}
		default:
			break;
	}
}


#if defined __linux__ || defined __WIN32

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
void DbfUnserializerReadAllToString(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	if ((bufPtr == NULL) || (bufSize<=0) || (bufSize >= 0x70000000))
	{
		return;
	}

	const char* separator="";
	*bufPtr=0;
	while(!DbfUnserializerReadIsNextEnd(dbfUnserializer) && bufSize>0)
	{
		if (DbfUnserializerReadIsNextString(dbfUnserializer))
		{
			char tmp[80];
			DbfUnserializerReadString(dbfUnserializer, tmp, sizeof(tmp));
			snprintf(bufPtr, bufSize, "%s\"%s\"", separator, tmp);
		}
		else if (DbfUnserializerReadIsNextInt(dbfUnserializer))
		{
			int64_t i = DbfUnserializerReadInt64(dbfUnserializer);
			snprintf(bufPtr, bufSize, "%s%lld", separator, (long long int)i);
		}
		else
		{
			snprintf(bufPtr, bufSize, " ?");
		}
		const int n = strlen(bufPtr);
		bufPtr += n;
		bufSize -=n;
		separator=" ";
	}
}


int DbfReceiverLogRawData(const DbfReceiver *dbfReceiver)
{
	if (DbfReceiverIsTxt(dbfReceiver))
	{
		printf(LOG_PREFIX "DbfReceiverLogRawData: ");
		const unsigned char *ptr = dbfReceiver->buffer;
		int n = dbfReceiver->msgSize;
		for(int i=0; i<n; ++i)
		{
			int ch = ptr[i];
			if (isgraph(ch))
			{
				printf("%c", ch);
			}
			else if (isprint(ch))
			{
				printf("%c", ch);
			}
			else if (ch == 0)
			{
				// do nothing
			}
			else
			{
				printf("<%02x>", ch);
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


