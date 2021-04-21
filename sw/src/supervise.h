/*
supervice.h

provide functions to set up hardware

Copyright (C) 2019 Henrik Björkman www.eit.se/hb.
All rights reserved etc etc...

History

2016-09-22
Created.
Henrik Bjorkman

*/

#ifndef SUPERVICE_H
#define SUPERVICE_H

#include <inttypes.h>
#include "messageNames.h"

extern int stableOperationTimer;
extern int64_t timeCounted_s;

int superviseGetState();

int superviseIsTargetReached();

void superviseCurrentSecondsTick1(void);
void superviseCurrentSecondsTick2(void);

void superviseCurrentMilliSecondsTick(void);

void setSuperviseInternalVoltage(int32_t requireVoltage);

int64_t superviceLongTermInternalVoltage_uV();
int64_t superviceLongTermAcCurrent_uA();

// see also enum in target_maintenance.h
enum {
	INITIAL_CONFIG = 0,
	ALL_CONFIG_DONE = 1,
};

CmdResult processWebServerStatusMsg(DbfUnserializer *dbfPacket);
void setConnectionState(int intconnectionState);

#endif


