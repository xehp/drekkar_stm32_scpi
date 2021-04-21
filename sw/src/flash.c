/*
flash.c

Copyright (C) 2021 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2021-04-11 Created by splitting eeprom.c into two files. Henrik Bjorkman
*/

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








#if (defined __linux__)

int8_t flashSave(const char* dataPtr, uint16_t dataSize, uint16_t offset);
{
	char fileName[80];
	snprintf(fileName ,sizeof(fileName), "eeprom%d.bin", offset);
	FILE *fp = fopen(fileName, "w");
	if (fp!=NULL)
	{
		int len=dataSize;
		for(int i=0; i<dataSize; ++i)
		{
			fputc(dataPtr[i], fp);
		}
		fclose(fp);
	}
	else
	{
		printf("saveBytePacker could not write '%s'\n", fileName);
		return -1;
	}
	return  0;
}

int8_t flashLoad(char* dataPtr, uint16_t dataSize, uint16_t offset);
{
	char fileName[80];
	snprintf(fileName ,sizeof(fileName), "eeprom%d.bin", offset);
	FILE *fp = fopen(fileName, "r");
	if (fp!=NULL)
	{
		for(int i=0; i<dataSize; ++i)
		{
			dataPtr[i] = fgetc(fp);
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


#elif (defined __arm__)



void flashWaitIfBusy()
{
	// Wait for any ongoinf flash operation to end.
	while (FLASH->SR & FLASH_SR_BSY_Msk)
	{
		//systemSleep();
	}
}

void flashWaitAndCheckFlashErrors(int line)
{
	flashWaitIfBusy();
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
void flashErase(uint32_t pageToErase)
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
	flashWaitAndCheckFlashErrors(__LINE__);

	// [1] 3. Set the PER bit and select the page you wish to erase (PNB)  in the
	// Flash control register (FLASH_CR).
	FLASH->CR = pageToErase << FLASH_CR_PNB_Pos | FLASH_CR_PER_Msk;

	// [1] 4. Set the STRT bit in the FLASH_CR register.
	FLASH->CR |= FLASH_CR_STRT_Msk;

	// [1] 5. Wait for the BSY bit to be cleared in the FLASH_SR register.
	flashWaitAndCheckFlashErrors(__LINE__);

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
 * Returns 0 if OK.
 */
static int flashWriteData(const uint64_t* src, const uint32_t nBytes, const uint32_t dstAdr)
{
	ASSERT(nBytes<=2048U);
	ASSERT(nBytes%8 == 0);
    //ASSERT(IS_FLASH_PROGRAM_ADDRESS(dstAdr));
    ASSERT(IS_FLASH_PROGRAM_ADDRESS(dstAdr));

    uint32_t n64BitsWords = nBytes / 8;


	// Show whats there first.
	debug_print("flashWriteData before writing\n");
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
		return -1;
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
				return -2;
			}
		}

		{
			int e2 = HAL_FLASH_GetError();
			if (e2 != HAL_OK)
			{
				debug_print("e2 ");
				debug_print64(e2);
				debug_print("\n");
				return -3;
			}
		}
	}

    //HAL_FLASH_Lock();

    debug_print("flashWriteData done\n");
	flashLogData((uint64_t*)dstAdr, n64BitsWords);

	flashWaitAndCheckFlashErrors(__LINE__);

#endif
	return 0;
}


int8_t flashSave(const char* dataPtr, uint16_t dataSize, uint16_t offset)
{

    debug_print("eedataSave\n");

	// TODO To have less wear on the flash we should circulate where we save data,
	// not saving at same place every time. This way we could do less erase. Just
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

	const int r = flashWriteData((const void*)dataPtr, dataSize, dstAdr);

	logInt2(EEPROM_SAVE_RESULT, r /*,ee.wallTimeReceived_ms*/);

    debug_print("after flash writing\n");
    flashLogData((uint64_t*)dstAdr, sizeof(EeDataStruct)/8);
	systemSleepMs(2);
    HAL_FLASH_Lock();

	return  0;
}

int8_t flashLoad(char* dataPtr, uint16_t dataSize, uint16_t offset)
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

	// Just copy form data flash location to wanted buffer.
	for(int i=0; i< dataSize64BitWords; i++)
	{
		debug_print(" ");
		debug_print64(*src);
		*dst = *src;
		dst++;
		src++;
	}

	//systemSleepMs(100);

	debug_print("\n");

	return 0;
}




#endif
