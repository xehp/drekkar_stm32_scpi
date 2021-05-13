/*
errorReporter.h


Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2019-03-15
Created, Some methods moved from ports.h to this new file.
Henrik Bjorkman

*/
#ifndef MACHINE_STATE_H
#define MACHINE_STATE_H

#include "messageNames.h"

typedef enum {
    cmdPauseCommand = 0,
    cmdRunCommand = 1,
    cmdResetCommand = 2
} machineRequestedStateEnum;

extern int64_t cyclesToDo; // TODO remove this extern declaration, use get/set CyclesToDo instead.
extern int64_t totalCyclesPerformed;
extern uint32_t machineSessionNumber;

extern DbfSerializer statusDbfMessage;
extern int32_t maxAllowedCurrent_mA;
extern int32_t maxAllowedExternalVoltage_mV;


void secAndLogInitStatusMessageAddHeader(DbfSerializer *statusDbfMessage, STATUS_MESSAGES msg);



void machineStateInit();

// This shall be called regularly, it can be less than once per second.
// but more than once per minute.
void machineSupervise(void);

/*
DbfCodeStateEnum machineStateParseCyclesToDo(DbfUnserializer *dbfPacket, int64_t replyToId, int64_t replyToRef);
DbfCodeStateEnum parseMachineSessionNumber(DbfUnserializer *dbfPacket, int64_t replyToId, int64_t replyToRef);
int machineReplyCyclesToDo(DbfUnserializer *dbfPacket, int64_t replyToId, int64_t replyToRef);
int machineReplySessionNumber(DbfUnserializer *dbfPacket, int64_t replyToId, int64_t replyToRef);
int machineReplyTotalCyclesPerformed(DbfUnserializer *dbfPacket, int64_t replyToId, int64_t replyToRef);
*/


void errorReportError(int errorCode);
int errorGetReportedError();
void errorAssert(const char *msg, const char *file, int line);
void errorReset();

#ifndef ASSERT
#if (defined __linux__)
#define ASSERT assert
#else
#define ASSERT(c) {if (!(c)) {errorAssert(#c, __FILE__ ,__LINE__);}}
#endif
#endif

#endif
