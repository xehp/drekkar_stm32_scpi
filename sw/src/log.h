/*
log.h

Created 2018 by Henrik

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...
*/

#ifndef LOG_H_
#define LOG_H_

#include "Dbf.h"
#include "messageNames.h"


#define LOG_PREFIX "l "
#define LOG_SUFIX "\r\n"


// msg shall be one of the values in enum AVR_CFG_LOG_MESSAGES.
void logInt1(int msg);
void logInt2(int msg, int64_t value);
void logInt3(int msg, int64_t value1, int64_t value2);
void logInt4(int msg, int64_t value1, int64_t value2, int64_t value3);
void logInt5(int msg, int64_t value1, int64_t value2, int64_t value3, int64_t value4);
void logInt6(int msg, int64_t value1, int64_t value2, int64_t value3, int64_t value4, int64_t value5);
void logInt7(int msg, int64_t value1, int64_t value2, int64_t value3, int64_t value4, int64_t value5, int64_t value6);
void logInt8(int msg, int64_t value1, int64_t value2, int64_t value3, int64_t value4, int64_t value5, int64_t value6, int64_t value7);

void logInitAndAddHeader(DbfSerializer *dbfSerializer, AVR_CFG_LOG_MESSAGES msg);

// Returns true if parameter was changed and therefore logged.
int logIfParameterChanged(PARAMETER_CODES par, int64_t value);
void logResetTime();

#if (defined __linux__) || (defined __WIN32) || (defined DEBUG_DECODE_DBF)
void logScanningAndRunningStateToString(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize);
void logOldFormatsFreqCurrVoltToString(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize);
//void logStatusMessageToString(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize);
void log_reply_message_to_string(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize);
//void log_voltage_message_to_string(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize);
void logDbfAsHexAndText(const unsigned char *msgPtr, int msgLen);
//void decodeDbfToText(const unsigned char *msgPtr, int msgLen, char *bufPtr, int bufSize);
#endif

#endif
