/*
eeprom.h

Handle stored parameters

Copyright (C) 2021 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2017-06-04
Created.
Henrik Bjorkman

*/

#ifndef EEPROM_H_
#define EEPROM_H_


#include <stdint.h>


// This is used to tell if external voltage sensor it required or not.
// Sometimes the voltage value is multiplied by 10 so not using a little
// smaller value here than 0x7FFFFFFF.
#define NO_VOLT_VALUE 0x7FFFFFF




// A magic number so we can detect if expected format of stored eedata has been changed (don't use zero).
// Remember to increment this number if struct below is changed in a non backward compatible way.
#define EEDATA_MAGIC_NR 0x1044

// To simplify upgrade we support one legacy magic number.
// Comment this macro out if legacy feature is not needed.
#define EEDATA_LEGACY_MAGIC_NR 0x1144


// Add scan start and scan end frequency
// If adding members here add them also in PARAMETER_CODES in msg.h.
// And perhaps web server also need it in DataTargetStruct?
// For best efficiency pay attention to word alignment if adding or removing something here.
typedef struct
{
	uint16_t magicNumber;
	uint16_t spare_0;
	uint32_t microAmpsPerUnitAc;			   // MICRO_AMPS_PER_UNIT_AC_PAR
	uint64_t deviceId;
	uint64_t spare_2;
	uint64_t spare_3;
	uint64_t spare_4;
	uint64_t spare_5;
	uint64_t spare_6;
	uint64_t spare_7;
	uint64_t spare_8;
	uint64_t spare_9;
	uint64_t spare_10;
	uint64_t spare_11;
	uint64_t spare_12;
	uint64_t spare_13;
	uint64_t spare_14;
	int32_t saveCounter;
	uint32_t checkSum;
} EeDataStruct;


// data size in bytes
// In version 0x1144 this was: 27*4 or 13.5*8 -> 108 bytes
// In next it will need to be 128 bytes, so 20 more.


#ifdef EEDATA_LEGACY_MAGIC_NR
typedef struct
{
	uint16_t magicNumber;
	uint8_t spare_1;
	uint8_t spare_2;
	uint32_t spare_3;
	uint32_t spare_4;
	uint32_t spare_5;
	uint32_t spare_6;
	uint32_t spare_7;
	uint64_t deviceId;
	uint16_t spare_8;
	uint16_t spare_9;
	uint16_t spare_10;
	uint16_t spare_11;
	uint16_t spare_12;
	uint16_t spare_13;
	uint16_t spare_14;
	uint16_t spare_15;
	uint32_t microAmpsPerUnitAc;			   // MICRO_AMPS_PER_UNIT_AC_PAR
	uint16_t spare_17;
	uint8_t spare_18;
	uint8_t spare_19;
	uint16_t spare_20;
	uint16_t spare_21;
	uint32_t checkSum;
} EeDataLegacyStruct;
#endif



extern EeDataStruct ee;


void eepromSave(void);
void eepromLoad(void);
void eepromSaveIfPending();
void eepromSetSavePending();


#endif

