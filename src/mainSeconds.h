/*
mainSeconds.h

provide functions that happen once per second, such as logging.

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2016-09-22
Created.
Henrik Bjorkman

*/

#ifndef MAINSECANDLOG_H
#define MAINSECANDLOG_H

#include <inttypes.h>
#include "Dbf.h"
#include "messageNames.h"




int32_t secAndLogGetSeconds(void);
void secAndLogIncSeconds(void);

void secAndLogInit(void);
void secAndLogMediumTick(void);

#endif


