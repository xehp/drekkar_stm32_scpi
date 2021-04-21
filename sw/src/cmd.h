/*
cmd.h

provide to handle commands/messages from other units/users

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2016-09-22
Created.
Henrik Bjorkman

*/

#ifndef CMD_H
#define CMD_H

#include <inttypes.h>
#include "messageNames.h"

int64_t getParameterValue(PARAMETER_CODES parId, NOK_REASON_CODES *result);


// This shall be called before any of the other cmd functions are called.
void cmdInit(void);

// This will take input from serial port.
// It is to be called as often as possible from the main loop.
void cmdFastTick(void);

// This shall be called once per second.
void cmdMediumTick(void);


#endif
