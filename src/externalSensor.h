/*
externalSensor.h


Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2019-03-15
Created, Some methods moved from ports.h to this new file.
Henrik Bjorkman

*/
#ifndef EXTERNAL_SENSOR_H
#define EXTERNAL_SENSOR_H

#include "messageNames.h"




// requires external HW to deliver this via serial port command 'v'
//int32_t getMeasuredVoltage_rawUnits(void);
int32_t getMeasuredExternalAcVoltage_mV();
int getMeasuredExtAcVoltageIsAvailable();

CmdResult externalSensorProcessStatusMsg(DbfUnserializer *dbfPacket);
void externalSensorMediumTick();
void externalSensorInit();


#endif
