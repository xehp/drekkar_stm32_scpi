/*
avr_eeprom.c

Handle stored parameters

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
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
#include "stm32l4xx.h"
#include "stm32l432xx.h"
#include "stm32l4xx_nucleo_32.h"
#include "stm32l4xx_ll_adc.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_flash.h"
#include "stm32l4xx_hal_flash_ex.h"
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

// This is in pages not bytes (as in avr).
#define BACKUP_EEPROM_OFFSET 1U

#elif (defined __linux__)
// In linux version this is just part of a file name.
#define BACKUP_EEPROM_OFFSET 1
#else
#error
#endif



#define DEFAULT_WANTED_CURRENT_MA 1000




/**
Voltage is measured over a voltage divider. 33 Ohm in series with 200 kOhm to rail.
Voltage to ADC will be 33/200033 = 0.000164973 times the voltage.
ADC have 65536 steps for 3.3 Volts.
One step on ADC is 3.3V/65536 = 50.354 micro volt per step at ADC.
That is 305226 micro volt from rail per ADC unit.
Calibration at 30 Volt gave 400000 uV/step
*/
#define DEFAULT_MICRO_VOLTS_PER_UNIT_DC 330000

/**
Current is measured over 0.005 Ohm (sort of, at such
low resistance the cables and connections matter).
ADC have 65536 steps for 3.3 Volts.
To get 3.3 Volt the current needs to be 660 Ampere.
600/65536 so 10000 uA per ADC unit.
But there are some resustance in cables etc also,
Calibration at 1 amp gave 6900 uA/step
*/
#define DEFAULT_MICRO_AMPS_PER_UNIT_DC 6900


/**
AC current is measured via 1:3000 current transformer
over 47 Ohm resistor.
One ADC unit is 3.3/65536 Volt.
So one ADC unit is 3000 * (3.3/65536)/47  Ampere => 3214 uA per ADC unit.
*/
#define DEFAULT_MICRO_AMPS_PER_UNIT_AC 4000

// Typical value is 10, to disable feature use 0
#define DEFAULT_DEVIATION_FACTOR 0

// See also bit positions within optionsBitMask in header file.
// Preferably default values are zero if adding more bits.
#define DEFAULT_OPTIONS (2+4+8)

// In Hbox this was 10000
//#define DEFAULT_VOLTAGE_PROBE_FACTOR 10000

#define DEFAULT_WANTED_VOLTAGE_MV NO_VOLT_VALUE
//#define DEFAULT_WANTED_VOLTAGE (12000000 / DEFAULT_MICRO_VOLTS_PER_UNIT)

// Suggested codes: 1 = Websuart, 2 = hbox/inverter, 3 = buck, 4 = Volt sensor, 5 = hub
#define DEFAULT_DEVICE_ID 2

#define DEFAULT_SCANNING_VOLTAGE_MV 24000


// See also CMD_MAX_TEMP_HZ in cmd.c.
#define DEFAULT_MIN_TEMP_HZ 54
#define DEFAULT_MAX_TEMP_HZ 1000

#define DEFAULT_FREQ_AT_20C 78
#define DEFAULT_FREQ_AT_100C 1000

// This tells SCAN over which frequencies it shall scan for resonance.
#define DEFAULT_SCAN_BEGIN_HZ 425
#define DEFAULT_SCAN_END_HZ 1800

// If PWM angle is too small then the inverting logic will not get time to reset its frozen signal detection.
// The result is that over current latch will get tripped.
#define DEFAULT_PORTS_PWM_MIN_ANGLE 256

// NTCs need some time to cool down. This tells the relay handler how much is minimum.
#define DEFAULT_COOLDOWN_TIME_S 30

// Time unit for interpolationTime depends on switching frequency (typically 50 KHz)
// or on regulation interval which is 1 ms.
// This is used as a prediction of voltage changes in REG. If this is zero interpolation
// will be same as non interpolated value so sort of disabling this feature.
#define DEFAULT_REG_INTERPOLATION_TIME 5

// When current is more or less than wanted it is compared with this value to give a relative value.
// A larger value here slows down regulation based on current.
// If this is doubled then regulation based on current will be half as fast.
#define DEFAULT_REFERENCE_CURRENT_A 30

// A higher value makes regulation slower. Lower value then REG changes faster.
// If this is doubled regulation (both based on voltage and current) will be half as fast.
#define DEFAULT_REG_TIME_BASE_MS 1000

// Unit for max steps is ppm of throttle per milli second.
// So a THROTTLE_MAX_BIG_STEPS_UP of 2500 means it takes
// at least 1000000/2500 ms that is 400 ms to change from 0 to full throttle.
// A lower value here slows down the max rate of throttle changes.
#define DEFAULT_REG_MAX_SETP_UP_PPMPMS 2500
#define DEFAULT_REG_MAX_STEP_DOWN_PPMPMS 20000

// This affect how fast scanning is done. Smaller value for faster scanning.
#define DEFAULT_SCAN_DELAY_MS 6

#define DEFAULT_STABILITY_TIME_S (5*60)
#define DEFAULT_SUPERVICE_DROP_FACTOR 30

#define DEFAULT_LOW_DUTY_CYCLE_PERCENT 45

#define DEFAULT_SUPERVICE_INC_FACTOR 7


const EeDataStruct eeDeault={
	EEDATA_MAGIC_NR, // magicNumber
	DEFAULT_OPTIONS, // autoStart;
	DEFAULT_DEVIATION_FACTOR, // superviceDeviationFactor;
	DEFAULT_MICRO_AMPS_PER_UNIT_DC, // microAmpsPerUnit;
	DEFAULT_MICRO_VOLTS_PER_UNIT_DC, // microVoltsPerUnit;
	DEFAULT_SCANNING_VOLTAGE_MV, // wantedInternalScanningVoltage_mV
	DEFAULT_WANTED_CURRENT_MA, // wantedCurrent;
	DEFAULT_WANTED_VOLTAGE_MV, // wantedVoltage;
	DEFAULT_DEVICE_ID, // deviceId
	DEFAULT_MIN_TEMP_HZ,
	DEFAULT_MAX_TEMP_HZ,
	DEFAULT_FREQ_AT_20C,
	DEFAULT_FREQ_AT_100C,
	DEFAULT_SCAN_BEGIN_HZ,
	DEFAULT_SCAN_END_HZ,
	DEFAULT_PORTS_PWM_MIN_ANGLE,
	DEFAULT_COOLDOWN_TIME_S,
	DEFAULT_MICRO_AMPS_PER_UNIT_AC,
	DEFAULT_SUPERVICE_DROP_FACTOR,
	DEFAULT_LOW_DUTY_CYCLE_PERCENT,
	DEFAULT_SUPERVICE_INC_FACTOR,
	DEFAULT_STABILITY_TIME_S,
	DEFAULT_SCAN_DELAY_MS,
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


void eepromSetDefault()
{
	ee = eeDeault;
}

#if (defined __linux__)


static int8_t saveBytePacker(EeDataStruct* eedataStruct, uint16_t offset)
{
	#if (defined __AVR_ATmega328P__)
	eeprom_write_block(eedataStruct, (void*)offset, sizeof(EeDataStruct));
	#else
	char fileName[80];
	snprintf(fileName ,sizeof(fileName), "eeprom%d.bin", offset);
	FILE *fp = fopen(fileName, "w");
	if (fp!=NULL)
	{
		const char *ptr=(const char *)eedataStruct;
		int len=sizeof(EeDataStruct);
		for(int i=0; i<len; ++i)
		{
			fputc(ptr[i], fp);
		}
		fclose(fp);
	}
	else
	{
		printf("saveBytePacker could not write '%s'\n", fileName);
		return -1;
	}
	#endif
	return  0;
}

static int8_t loadBytePacker(EeDataStruct* eedataStruct, uint16_t offset)
{
	char fileName[80];
	snprintf(fileName ,sizeof(fileName), "eeprom%d.bin", offset);
	FILE *fp = fopen(fileName, "r");
	if (fp!=NULL)
	{
		char *ptr=(char*)eedataStruct;
		int len=sizeof(EeDataStruct);
		for(int i=0; i<len; ++i)
		{
			ptr[i] = fgetc(fp);
		}
		fclose(fp);
	}
	else
	{
		printf("loadBytePacker did not find '%s'\n", fileName);
		return -1;
	}
	return 0;
}



void eepromSave()
{

	// Set checksum to zero while calculating it.
	ee.checkSum = 0;
    const uint32_t calculatedCSum = crc32_calculate((const unsigned char *)&ee, sizeof(EeDataStruct));
	ee.checkSum = calculatedCSum;

	saveBytePacker(&ee, 0);
	saveBytePacker(&ee, BACKUP_EEPROM_OFFSET);
}




// Returns 0 if OK
static int8_t eepromTryLoad(uint16_t offset)
{
	EeDataStruct tmp;



	const int8_t r = loadBytePacker(&tmp, offset);

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

	ee = tmp;

	logInt2(EEPROM_LOADED_OK, ee.magicNumber);

	return r;
}


// If EEProm is available we want to set wantedCurrent and/or wantedVoltage
// We do that by reading EEPROM as if those where commands an pass these commands to
void eepromLoad(void)
{
	int8_t r = eepromTryLoad(0);

	if (r!=0)
	{

		logInt1(EEPROM_TRYING_BACKUP);

		r = eepromTryLoad(BACKUP_EEPROM_OFFSET);
		if (r==0)
		{
			logInt1(EEPROM_BACKUP_LOADED_OK);
		}
		else
		{
			eepromSetDefault();
			logInt1(EEPROM_FAILED);
		}
	}
}



#elif (defined __arm__)


static void waitIfBusy()
{
	// Wait for any ongoinf flash operation to end.
	while (FLASH->SR & FLASH_SR_BSY_Msk)
	{
		//systemSleep();
	}
}

static void waitAndCheckFlashErrors(int line)
{
	waitIfBusy();
	const uint32_t tmp = FLASH->SR;
	const uint32_t msk = FLASH_SR_OPTVERR_Msk | FLASH_SR_RDERR_Msk | FLASH_SR_FASTERR_Msk | FLASH_SR_MISERR_Msk | FLASH_SR_PGSERR_Msk | FLASH_SR_SIZERR_Msk | FLASH_SR_PGAERR_Msk | FLASH_SR_WRPERR_Msk | FLASH_SR_PROGERR_Msk | FLASH_SR_OPERR_Msk;
	if ((tmp & msk) != 0)
	{
	 	debug_print("flash error: ");
		debug_print64(line);
		debug_print(" ");
		debug_print64(tmp);
	 	debug_print("\n");
		systemErrorHandler(SYSTEM_FLASH_DRIVER_ERROR);
	}
	else
	{
		debug_print("Flash OK ");
		debug_print64(line);
		debug_print("\n");
	}

	FLASH->SR |= msk;
}

/**
 * pages are memory in blocks of 2048 bytes.
 * To translate page into memory address there is also an offset to consider.
 */
static void flashErase(uint32_t pageToErase)
{
	debug_print("flashErase\n");


	ASSERT(IS_FLASH_PAGE(pageToErase));

#if 1

	//uint32_t bank = 2; // Bank 1 or 2 how do I know?
	//FLASH_PageErase(page, bank);
	//FLASH_Erase_Sector()

	// Unlock flash 
	#if 1
    //HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
	#else
	waitAndCheckFlashErrors(__LINE__);
	FLASH->KEYR = 0x45670123U;
	FLASH->KEYR = 0xCDEF89ABU;
	#endif

	// First page erase

	// [1] 1. Check that no Flash memory operation is ongoing
	// [1] 2. Check and clear all error programming flags due to a previous programming.
	waitAndCheckFlashErrors(__LINE__);

	// [1] 3. Set the PER bit and select the page you wish to erase (PNB)  in the
	// Flash control register (FLASH_CR).
	FLASH->CR = pageToErase << FLASH_CR_PNB_Pos | FLASH_CR_PER_Msk;

	// [1] 4. Set the STRT bit in the FLASH_CR register.
	FLASH->CR |= FLASH_CR_STRT_Msk;

	// [1] 5. Wait for the BSY bit to be cleared in the FLASH_SR register.
	waitAndCheckFlashErrors(__LINE__);

	// https://stackoverflow.com/questions/28498191/cant-write-to-flash-memory-after-erase
	FLASH->CR &= ~FLASH_CR_PER_Msk;

    /* Flush the caches to be sure of the data consistency */
    //FLASH_FlushCaches();

    // Lock flash again
	#if 1
    //HAL_FLASH_Lock();
	#else
	waitAndCheckFlashErrors(__LINE__);
	FLASH->CR |= FLASH_CR_LOCK_Msk;
	#endif
#else

	//FLASH_EraseInitTypeDef eraseInit={FLASH_TYPEERASE_PAGES, 2, pageToErase-64, 1};
	FLASH_EraseInitTypeDef eraseInit={FLASH_TYPEERASE_PAGES, 1, pageToErase, 1};
	uint32_t pageError;

	HAL_StatusTypeDef s = HAL_FLASHEx_Erase(&eraseInit, &pageError);

	ASSERT(s == HAL_OK);

#endif
}

static void flashLogData(const uint64_t* src, const uint32_t n64BitsWords)
{
	for(int i=0; i< n64BitsWords; i++)
	{
		debug_print(" ");
		debug_print64(src[i]);
	}
	debug_print("\n");
}

/**
 * nBytes must be full 8 bytes. Like 8, 16, 24 and so on.
 */
static void flashData(const uint64_t* src, const uint32_t nBytes, const uint32_t dstAdr)
{
	ASSERT(nBytes<=2048U);
	ASSERT(nBytes%8 == 0);
    //ASSERT(IS_FLASH_PROGRAM_ADDRESS(dstAdr));
    ASSERT(IS_FLASH_PROGRAM_ADDRESS(dstAdr));

    uint32_t n64BitsWords = nBytes / 8;


	// Show whats there first.
	debug_print("flashData before writing\n");
	flashLogData((uint64_t*)dstAdr, n64BitsWords);


#if 0

	// [1] 3.3.5 Flash program and erase operations

	// Unlock flash 
	waitAndCheckFlashErrors(__LINE__);
	FLASH->KEYR = 0x45670123U;
	FLASH->KEYR = 0xCDEF89ABU;

	// First page erase

	// [1] 1. Check that no Flash memory operation is ongoing
	// [1] 2. Check and clear all error programming flags due to a previous programming.
	waitAndCheckFlashErrors(__LINE__);

	// [1] 3. Set the PER bit and select the page you wish to erase (PNB)  in the
	// Flash control register (FLASH_CR).
	const uint32_t pageToErase = EEPROM_PAGE_BASE+offset;
	FLASH->CR = pageToErase << FLASH_CR_PNB_Pos | FLASH_CR_PER_Msk;

	// [1] 4. Set the STRT bit in the FLASH_CR register.
	FLASH->CR |= FLASH_CR_STRT_Msk;

	// [1] 5. Wait for the BSY bit to be cleared in the FLASH_SR register.
	waitAndCheckFlashErrors(__LINE__);


	volatile uint32_t dstAdr = FLASH_BASE+((EEPROM_PAGE_BASE+offset)*EEPROM_PAGE_SIZE);
	volatile uint64_t data = 0;

    ASSERT(IS_FLASH_PROGRAM_ADDRESS(dstAdr));

	// [1] 1. Check that no Flash main memory operation is ongoing.
	// [1] 2. Check and clear all error programming flags due to a previous programming.
	waitAndCheckFlashErrors(__LINE__);



	// [1] 3. Set the PG bit in the Flash control register (FLASH_CR).
	FLASH->CR |= FLASH_CR_PG_Msk;

	//__disable_irq();

	// [1] 4. Perform the data write operation at the desired memory address,
	// inside main memory block or OTP area. 

	// Program the 64 bit (double) word.
    *(__IO uint32_t*)dstAdr = (uint32_t)data;
    *(__IO uint32_t*)(dstAdr+4) = (uint32_t)(data >> 32);

	/*for(int i=0; i< dataSize64BitWords; i++)
	{
		dst[i] = src[i];

		// Perhaps not needed?
        //waitAndCheckFlashErrors();
	}*/
    //__enable_irq();

	// [1] 5. Wait until the BSY bit is cleared in the FLASH_SR register.
	waitAndCheckFlashErrors(__LINE__);

    // [1] 6. Check that EOP flag is set in the FLASH_SR register (meaning that the programming 
    // operation has succeed), and clear it by software
	if (FLASH->SR & FLASH_SR_EOP_Msk)
	{
		// OK
		FLASH->SR &= ~FLASH_SR_EOP_Msk;
	}
	else
	{
		debug_print("Flash failed\n");
	}

	// [1] 7. Clear the PG bit in the FLASH_SR register 
	FLASH->CR &= ~FLASH_CR_PG_Msk;



    // Lock flash again
	waitAndCheckFlashErrors(__LINE__);
	FLASH->CR |= FLASH_CR_LOCK_Msk;
#else

    //FLASH_ProcessTypeDef pFlash;

    //HAL_FLASH_Unlock();

    //FLASH_WaitForLastOperation();


	for(int i =0; i<n64BitsWords; i++)
	{
		uint64_t data64 = src[i];
		//uint64_t data64 = 0;
		{
			int e1 = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, dstAdr+i*8, data64);
			if (e1 != HAL_OK)
			{
				debug_print("e1 ");
				debug_print64(e1);
				debug_print("\n");
			}
		}

		{
			int e2 = HAL_FLASH_GetError();
			if (e2 != HAL_OK)
			{
				debug_print("e2 ");
				debug_print64(e2);
				debug_print("\n");
			}
		}
	}

    //HAL_FLASH_Lock();

    debug_print("flashData done\n");
	flashLogData((uint64_t*)dstAdr, n64BitsWords);

	waitAndCheckFlashErrors(__LINE__);

#endif
}





static int8_t eedataSave(const char* dataPtr, uint16_t dataSize, uint16_t offset)
{

    debug_print("eedataSave\n");

	// TODO To have less wear on the flash we should circulate where we save data,
	// not saving att same place every time. This way we could do less erase. Just
	// overwrite previous save with zeros until block is used up and only then do 
	// erase.

	uint32_t page = EEPROM_PAGE_BASE+offset;
	uint32_t dstAdr = FLASH_BASE+((page)*EEPROM_PAGE_SIZE);

	ASSERT((offset == 0) || (offset == 1));

    HAL_FLASH_Unlock();
	systemSleepMs(2);

    debug_print("before flash erase\n");
    flashLogData((uint64_t*)dstAdr, sizeof(EeDataStruct)/8);

    flashErase(page);
	systemSleepMs(2);
    debug_print("after flash erase\n");
    flashLogData((uint64_t*)dstAdr, sizeof(EeDataStruct)/8);

	flashData((const void*)dataPtr, dataSize, dstAdr);

    debug_print("after flash writing\n");
    flashLogData((uint64_t*)dstAdr, sizeof(EeDataStruct)/8);
	systemSleepMs(2);
    HAL_FLASH_Lock();

	return  0;
}

static int8_t eedataLoad(const char* dataPtr, uint16_t dataSize, uint16_t offset)
{
	debug_print("eedata size ");
	debug_print64(dataSize);
	debug_print("\n");

	const uint32_t dataSize64BitWords = (dataSize / 8);
	const uint32_t srcAdr = (FLASH_BASE+((EEPROM_PAGE_BASE+offset)*EEPROM_PAGE_SIZE));

	ASSERT(dataSize<EEPROM_PAGE_SIZE);
	ASSERT(dataSize%8 == 0);
	ASSERT((offset == 0) || (offset == 1));
	ASSERT(IS_FLASH_PROGRAM_ADDRESS(srcAdr));

	volatile uint64_t *dst = (volatile uint64_t *)dataPtr;
	volatile uint64_t *src = (volatile uint64_t *)srcAdr;

	for(int i=0; i< dataSize64BitWords; i++)
	{
		debug_print(" ");
		debug_print64(*src);
		*dst = *src;
		dst++;
		src++;
	}
	debug_print("\n");

	return 0;
}



void eepromSave()
{
	// Set checksum to zero while calculating it.
	ee.checkSum = 0;
    const uint32_t calculatedCSum = crc32_calculate((const unsigned char *)&ee, sizeof(EeDataStruct));
	ee.checkSum = calculatedCSum;

	eedataSave((const char*)&ee, sizeof(ee), 0);
	eedataSave((const char*)&ee, sizeof(ee), 1);

	savePending = 0;
}




// Returns 0 if OK
static int8_t eepromTryLoad(uint16_t offset)
{
	EeDataStruct tmp;


	const int8_t r = eedataLoad((char*)(&tmp), sizeof(tmp), offset);

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

	ee = tmp;

	logInt1(EEPROM_LOADED_OK);

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
			logInt1(EEPROM_BACKUP_LOADED_OK);
		}
		else
		{
			eepromSetDefault();
			logInt1(EEPROM_FAILED);
		}
	}

	// TODO This should not be needed.
	systemSleepMs(100);

	debug_print("\n" "magicNumber "); debug_print64(ee.magicNumber);
	debug_print("\n" "deviceId "); debug_print64(ee.deviceId);
	//debug_print("\n" "freqAt20C "); debug_print64(ee.freqAt20C);
	//debug_print("\n" "freqAt100C "); debug_print64(ee.freqAt100C);
	debug_print("\n" "checkSum "); debug_print64(ee.checkSum);
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



#else
#error
#endif



