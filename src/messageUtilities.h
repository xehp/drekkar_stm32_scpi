/*
messageUtilities.h


Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2019-03-15
Created, Some methods moved from ports.h to this new file.
Henrik Bjorkman

*/
#ifndef MESSAGE_UTILITIES_H
#define MESSAGE_UTILITIES_H

#include "messageNames.h"

void messageInitAndAddCategoryAndSender(DbfSerializer *dbfSerializer, MESSAGE_CATEGORY category);

void messageReplyOkInitAndAddHeader(COMMAND_CODES cmd, int64_t replyToId, int64_t replyToRef);

void messageReplyOK(COMMAND_CODES cmd, int64_t replyToId, int64_t replyToRef);

void messageReplyNOK(COMMAND_CODES cmd, NOK_REASON_CODES reasonCode, int parameter, int64_t replyToId, int64_t replyToRef);

void messageReplyToSetCommand(int16_t parameterId, int64_t value, int64_t replyToId, int64_t replyToRef);

void messageReplyToGetCommand(int32_t parameterId, int64_t value, int64_t replyToId, int64_t replyToRef);

void messageLogBuffer(const char* prefix, const unsigned char *bufPtr, int bufLen);

extern DbfSerializer messageDbfTmpBuffer;

void messageSendDbf(DbfSerializer *bytePacket);

void messageSendShortDbf(int32_t code);


#if defined __linux__ || defined __WIN32
void decodeDbfToText(const unsigned char *msgPtr, int msgLen, char *buf, int bufSize);
void linux_sim_log_message_from_target(const DbfReceiver *dbfReceiver);
#endif


#endif
