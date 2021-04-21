/*
exteralTemp.h

Copyright (C) 2021 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...


History
2021-04-12 Moved some code from fan.h to this new file. Henrik

*/

#ifndef EXTERNAL_TEMP_H
#define EXTERNAL_TEMP_H

//#include <stdlib.h>
//#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include "messageNames.h"






typedef enum{
	EXT_TEMP_INITIAL = 0,
	EXT_TEMP_OUT_OF_RANGE = 1,
	EXT_TEMP_READY = 2,
	EXT_TEMP_TIMEOUT = 3,
} ExtTempStaes;


CmdResult fanProcessExtTempStatusMsg(DbfUnserializer *dbfPacket);
int getExtTemp1C();
int getExtTemp2C();
//int getExtTempAmbientC();
int getExtTempState();
int ExtTempAvailableOrNotRequired();
int ExtTempOk();


#endif
