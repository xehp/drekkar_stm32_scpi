/*
eeprom.c

Handle stored parameters

Copyright (C) 2021 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2017-06-04
Created.
Henrik Bjorkman

*/

// http://www.nongnu.org/avr-libc/user-manual/group__avr__eeprom.html#gac5c2be42eb170be7a26fe8b7cce4bc4d
// http://www.avrfreaks.net/forum/tut-c-using-eeprom-memory-avr-gcc?page=all
// http://www.nongnu.org/avr-libc/user-manual/group__avr__eeprom.html


#include <stdint.h>

#if (defined __arm__)
#include "systemInit.h"
#include "serialDev.h"
#elif (defined __linux__)
#include <stdio.h>
#endif
#include "debugLog.h"
#include "machineState.h"
#include "messageNames.h"
#include "log.h"
#include "crc32.h"
#include "flash.h"
#include "eeprom.h"

// Where in EEPROM the backup message shall be written.
#if (defined __arm__)
// [1] 3.2 FLASH main features
// Page erase (2 Kbyte) and mass erase.
// This numner must be atleast the size of a page.
#define EEPROM_PAGE_SIZE 2048U

// For now we will just take the highest part of flash memory and just hope 
// that the linker did not use it also.
// [1] 3.3.1 Flash memory organization
// This number must be a start address of a page. 
// TODO We should adjust the linking in flash.ld so that compiler knows to not 
// use this part of memory.
#define EEPROM_PAGE_BASE 126U

// This is in pages not bytes (as it was in AVR version).
#define BACKUP_EEPROM_OFFSET 1U

#elif (defined __linux__)
// In linux version this is just part of a file name.
#define BACKUP_EEPROM_OFFSET 1
#else
#error
#endif










/**
AC current is measured via 1:3000 current transformer
over 47 Ohm resistor.
One ADC unit is 3.3/65536 Volt.
So one ADC unit is 3000 * (3.3/65536)/47  Ampere => 3214 uA per ADC unit.
*/
#define DEFAULT_MICRO_AMPS_PER_UNIT_AC 4000


// In Hbox this was 10000
//#define DEFAULT_VOLTAGE_PROBE_FACTOR 10000

// Suggested codes: 0 = unknown, 1 = Websuart, 2 = hbox/inverter, 3 = buck, 4 = Volt sensor, 5 = hub, 6 = current leakage detector.
#define DEFAULT_DEVICE_ID 2




#define DEFAULT_SAVE_COUNTER 0

const EeDataStruct eeDeault={
	EEDATA_MAGIC_NR, // magicNumber
	0, // spare 0
	DEFAULT_MICRO_AMPS_PER_UNIT_AC,
	DEFAULT_DEVICE_ID, // deviceId
	0, // spare 2
	0, //       3
	0, //       4
	0, //       5
	0, //       6
	0, //       7
	0, //       8
	0, //       9
	0, //      10
	0, //      11
	0, //      12
	0, //      13
	0, //      14
	DEFAULT_SAVE_COUNTER,
	0 // checkSum
};

EeDataStruct ee;


int savePending = 0;


// Very simple checksum
/*
static uint16_t calcCSum(const char *ptr, uint8_t size)
{
	uint16_t s=0;
	uint8_t i;
	for (i=0; i<size; ++i)
	{
		const uint8_t v=ptr[i];
		s += v;
	}
	return s;
}
*/

void my_memcpy(void* dst, const void* src, size_t size)
{
	char *d = (char*)dst;
	const char *s = (const char*)src;
	while(size>0)
	{
		*d=*s;
		++d;
		++s;
	    --size;
	}
}

/*void my_memclr(void* dst, size_t size)
{
	char *d = (char*)dst;
	while(size>0)
	{
		*d=0;
		++d;
	    --size;
	}
}*/


static void eepromSetDefault()
{
	//ee = eeDeault;
	my_memcpy(&ee, &eeDeault, sizeof(ee));

}

#ifdef EEDATA_LEGACY_MAGIC_NR
static void eepromDataUpgrade(EeDataStruct* e)
{
	if (e->magicNumber == EEDATA_LEGACY_MAGIC_NR)
	{
		EeDataLegacyStruct* l = (EeDataLegacyStruct*)e;

		// save checksum
		const uint32_t csumLoaded = l->checkSum;

		// Set checksum to zero while calculating it. Since that is what we did when saving
		l->checkSum = 0;

		// Check CRC with old size of data. NOTE new size must not be less than old!
		const uint32_t csumCalculated = crc32_calculate((const unsigned char *)l, sizeof(EeDataLegacyStruct));

	    // Apply any translation needed here. For example if another
	    // field was added set it to its default value.
		e->magicNumber = EEDATA_MAGIC_NR;
		e->microAmpsPerUnitAc = l->microAmpsPerUnitAc;
		e->deviceId = l->deviceId;
		e->saveCounter = 0;

		if (csumCalculated == csumLoaded)
		{
			// Checksum was OK so update it also to new format.
			e->checkSum = 0;
			const uint32_t newCsum = crc32_calculate((const unsigned char *)e, sizeof(EeDataStruct));
			e->checkSum = newCsum;
		}

		logInt3(EEPROM_DATA_UPGRADED, EEDATA_LEGACY_MAGIC_NR, EEDATA_MAGIC_NR);
	}
}
#endif







void eepromSave()
{
	ee.saveCounter++;

	// Set checksum to zero while calculating it.
	ee.checkSum = 0;
    const uint32_t calculatedCSum = crc32_calculate((const unsigned char *)&ee, sizeof(EeDataStruct));
	ee.checkSum = calculatedCSum;

	flashSave((const char*)&ee, sizeof(ee), 0);
	flashSave((const char*)&ee, sizeof(ee), 1);

	savePending = 0;
}




// Returns 0 if OK
static int8_t eepromTryLoad(uint16_t offset)
{
	EeDataStruct tmp;

	const int8_t r = flashLoad((char*)(&tmp), sizeof(tmp), offset);

	#ifdef EEDATA_LEGACY_MAGIC_NR
	eepromDataUpgrade(&tmp);
	#endif

	// save checksum
	const uint32_t csumLoaded = tmp.checkSum;

	// Set checksum to zero while calculating it. Since that is what we did when saving
	tmp.checkSum = 0;
    const uint32_t csumCalculated = crc32_calculate((const unsigned char *)&tmp, sizeof(EeDataStruct));

	if(tmp.magicNumber != EEDATA_MAGIC_NR)
	{
		logInt3(EEPROM_WRONG_MAGIC_NUMBER, tmp.magicNumber, EEDATA_MAGIC_NR);
    	return -1;
	}

	if(csumCalculated != csumLoaded)
	{
		logInt3(EEPROM_WRONG_CHECKSUM, csumCalculated, csumLoaded);
        return -1;
	}

	// Loaded data was approved so copy to active buffer,
	//ee = tmp;
	my_memcpy(&ee, &tmp, sizeof(ee));

	//logInt2(EEPROM_LOADED_OK, ee.magicNumber/*, ee.wallTimeReceived_ms*/);

	return r;
}


// If EEProm is available we want to set wantedCurrent and/or wantedVoltage
// We do that by reading EEPROM as if those where commands an pass these commands to
void eepromLoad(void)
{
	debug_print("eepromLoad\n");

	int8_t r = eepromTryLoad(0);

	if (r!=0)
	{

		logInt1(EEPROM_TRYING_BACKUP);

		r = eepromTryLoad(1);
		if (r==0)
		{
			logInt2(EEPROM_BACKUP_LOADED_OK, ee.magicNumber /*,ee.wallTimeReceived_ms*/);
		}
		else
		{
			eepromSetDefault();
			logInt1(EEPROM_FAILED);
		}
	}
	else
	{
		logInt2(EEPROM_LOADED_OK, ee.magicNumber /*,ee.wallTimeReceived_ms*/);
	}

	// TODO This should not be needed. Or if it is move it to flashLoad function.
	systemSleepMs(100);

	debug_print("\n" "magicNumber "); debug_print64(ee.magicNumber);
	debug_print("\n" "deviceId "); debug_print64(ee.deviceId);
	//debug_print("\n" "checkSum "); debug_print64(ee.checkSum);
	debug_print("\n");


	// TODO This should not be needed.
	systemSleepMs(100);
}

void eepromSetSavePending()
{
	if (savePending == 0)
	{
		logInt1(EEPROM_SAVE_PENDING_FLAG_SET);
		savePending = 1;
	}
}

void eepromSaveIfPending()
{
	if (savePending)
	{
		logInt1(EEPROM_PERFOMING_PENDING_SAVE);
		eepromSave();
		savePending = 0;
	}
}



