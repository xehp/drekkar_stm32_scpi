/*
relay.c

provide functions to set up hardware

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2019-03-09
Created.
Henrik Bjorkman
*/


#ifndef RELAY_H
#define RELAY_H

#include <inttypes.h>


void relayInit(void);
void relayOff(void);
void relay1On(void);
void relaySecTick();
int relayCoolDownTimeRemaining_S();
int relayGetState();
int relayIsReadyToScan();
void relay1Off2On(void);

#endif
