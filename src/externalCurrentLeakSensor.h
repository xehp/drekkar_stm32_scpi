/*
externalCurrentLeakSensor.h


Copyright (C) 2021 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2021-04-06
Created, Some methods moved from externalSensor to this new file.
Henrik Bjorkman

*/
#ifndef EXTERNAL_CURRENT_LEAK_SENSOR_H
#define EXTERNAL_CURRENT_LEAK_SENSOR_H


#include "messageNames.h"

CmdResult externalLeakSensorProcessMsg(DbfUnserializer *dbfPacket);
int64_t getMeasuredLeakSensor_mA();

void externalLeakSensorSecondsTick();


#endif
