/*
Barebone blinky on nucleo L432KC / stm32l432kc using SysTick. No RTOS needed.
Copyright (C) Alexander & Henrik Bjorkman www.eit.se 2018
This may be used and redistributed as long as all copyright notices etc are retained .
http://www.eit.se/hb/misc/stm32/stm32l4xx/stm32_barebone/

This module holds the SysLib. Some functions to administrate the system itself.
Provides milli seconds tick timer, sleep and error handler.
It will use one IO pin to show status and error.
*/

#ifndef SYSTEMINIT_H
#define SYSTEMINIT_H

// TODO Can we replace some of these with forward declarations?
#include "stm32l4xx.h"
#include "stm32l432xx.h"
#include "stm32l4xx_nucleo_32.h"
#include "stm32l4xx_ll_adc.h"

#ifndef __arm__
#error
#endif



// Depending on clock configuration in SystemInit
// Unit for SysClockFrequencyHz is Hz.
// Uncomment one of the lines below.
//#define SysClockFrequencyHz 4000000U
//#define SysClockFrequencyHz 16000000U
#define SysClockFrequencyHz 80000000U


enum
{                                        // Morse code
	SYSTEM_ASSERT_ERROR = 3,             // M --
	SYSTEM_USART1_ERROR = 4,             // D -..
	SYSTEM_USART2_ERROR = 5,             // K -.-
	SYSTEM_LPUART_ERROR = 6,             // G --.
	SYSTEM_FLASH_DRIVER_ERROR = 7,       // O ---
	STSTEM_ADC_DRIVER_ERROR = 8,         // B -...
	SYSTEM_APPLICATION_ASSERT_ERROR = 9, // X -..-
	SYSTEM_SOFT_UART_ERROR = 10,         // C -.-.
} SystemErrorCodes;



/**
 * These are part of system library since the system library error handler
 * needs them also. It will use this to operate the green debug LED.
 * These functions can also be used to operate other output pins.
 *
 * On nucleo l432kc the Green LED is on port B pin 3
 * If changing port remember to also activate the clock for that port.
 * For port B that is the line: RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
 * It will be initiated by system library to be used by error handler if needed.
 * This also needs to be changed if the application is to use this pin 
 * for something else.
 */
#define SYS_LED_PORT GPIOB
#define SYS_LED_PIN 3

#define SYS_LED_ON() SYS_LED_PORT->ODR |= (0x1U << SYS_LED_PIN);
#define SYS_LED_OFF() SYS_LED_PORT->ODR &= ~(0x1U << SYS_LED_PIN);

void systemPinOutInit(GPIO_TypeDef* port, int pin);
void systemPinOutSetHigh(GPIO_TypeDef* port, int pin); // For Green system LED this is On
void systemPinOutSetLow(GPIO_TypeDef* port, int pin); // For Green system LED this is Off

/**
 * Application functions can call this to get the system tick counter.
 * It is supposed to be one tick per milli second.
 */
int64_t systemGetSysTimeMs();

/**
 * For very short delays the systemBusyWait can be used.
 * However the delay length of systemBusyWait is unspecified.
 * 
 * For longer delays it is much better to use the systemSleepMs. 
 * 
 * It is recommended that the main loop or other long
 * lasting loops calls systemSleep() once per turn.
 */
void systemBusyWait(uint32_t delay);
void systemSleepMs(int32_t timeMs);
void systemSleep();

/**
 * Device handlers or applications can call this if they encounter an
 * error after which system shall be halted. Typically SysLib will
 * go into an eternal loop flashing the green LED depending on error code.
 */
void systemErrorHandler(int errorCode);



#define SYSTEM_ASSERT(c) {if (!c) {systemErrorHandler(SYSTEM_ASSERT_ERROR);}}


#ifdef __AVR_ATmega328P__
#define system_disable_interrupts() cli()
#define system_enable_interrupts() sei()
#else
#define system_disable_interrupts() __disable_irq()
#define system_enable_interrupts() __enable_irq()
#endif

#endif
