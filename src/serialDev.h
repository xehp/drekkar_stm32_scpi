/*
Barebone blinky on nucleo L432KC / stm32l432kc using SysTick. No RTOS needed.
Copyright (C) Alexander & Henrik Bjorkman www.eit.se 2018
This may be used and redistributed as long as all copyright notices etc are retained .
http://www.eit.se/hb/misc/stm32/stm32l4xx/stm32_barebone/
*/

#ifndef USARTDEV_H
#define USARTDEV_H

// TODO Rename this file to "serialDev.h"

#ifndef __arm__
#error
#endif

// TODO Can we replace some of these with forward declarations?
#include "stm32l4xx.h"
#include "stm32l432xx.h"
#include "stm32l4xx_nucleo_32.h"
#include "stm32l4xx_ll_adc.h"
#include "systemInit.h"

// Support for Low Power Uart (LPUART) is not tested.

enum
{
  DEV_LPUART1 = 0,
  DEV_USART1 = 1, // OPTO
  DEV_USART2 = 2, // USB
  DEV_SOFTUART1 = 3,
};

void setupIoPinTx(GPIO_TypeDef *base, uint32_t pin, uint32_t alternateFunction);
void setupIoPinRx(GPIO_TypeDef *base, uint32_t pin, uint32_t alternateFunction);

int serialInit(int usartNr, uint32_t baud);
void serialPutChar(int usartNr, int ch);
int serialGetChar(int usartNr);
void serialWrite(int usartNr, const char *str, int msgLen);
void serialPrint(int usartNr, const char *str);
void serialPrintInt64(int usartNr, int64_t num);


#endif
