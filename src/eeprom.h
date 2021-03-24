/*
avr_eeprom.h

Handle stored parameters

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2017-06-04
Created.
Henrik Bjorkman

*/

#ifndef EEPROM_H_
#define EEPROM_H_

// TODO rename this to flashData

#include <stdint.h>


// This is used to tell if external voltage sensor it required or not.
// Sometimes the voltage value is multiplied by 10 so not using a little
// smaller value here than 0x7FFFFFFF.
#define NO_VOLT_VALUE 0x7FFFFFF




// A magic number so we can detect if expected format of stored eedata has been changed (don't use zero).
#define EEDATA_MAGIC_NR 0x1144



// Add scan start and scan end frequency
// If adding members here add them also in PARAMETER_CODES in msg.h.
// And perhaps web server also need it in DataTargetStruct?
// For best efficiency pay attention to word alignment if adding or removing something here.
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
	uint32_t spare_16;
	uint16_t spare_17;
	uint8_t spare_18;
	uint8_t spare_19;
	uint16_t spare_20;
	uint16_t spare_21;
	uint32_t checkSum;
} EeDataStruct;


extern EeDataStruct ee;


void eepromSave(void);
void eepromLoad(void);
void eepromSetDefault();
void eepromSaveIfPending();
void eepromSetSavePending();


#endif

