/*
portsGpio.c

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...



History

2019-04-27
Created for dielectric test equipment
Henrik

*/


#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include "timerDev.h"

#include "stm32l4xx.h"
#include "stm32l432xx.h"
#include "stm32l4xx_nucleo_32.h"
#include "stm32l4xx_ll_adc.h"

#include "cfg.h"
#include "log.h"
#ifdef INVERTER_OVER_CURRENT_APIN
#include "portsPwm.h"
#endif
#include "portsGpio.h"

#ifdef PORTS_GPIO_H


#if (defined FAN1_APIN)

#define GPIOA_LINE1 1

static volatile int counterA=0;
//static volatile int prevInput=0;

#define readInputPin(port, pin) (((port->IDR) >> (pin)) & 1)


static void inputPinInit(GPIO_TypeDef* port, int pin)
{
	// MODER
	// 00: Input mode
	// 01: General purpose output mode
	// 10: Alternate function mode
	// 11: Analog mode (reset state)
	const uint32_t m = 0U; // 0 for "General purpose input mode", 1 for "General purpose output mode".
	port->MODER &= ~(3U << (pin*2)); // Clear the 2 mode bits for this pin.
	port->MODER |= (m << (pin*2));

	// OTYPER
	// 0: Output push-pull (reset state)
	// 1: Output open-drain
	//port->OTYPER &= ~(0x1U << (pin*1)); // Clear output type, 0 for "Output push-pull".

	const uint32_t s = 1U; // 1 for "Medium speed" or 2 for High speed.
	port->OSPEEDR &= ~(0x3U << (pin*2)); // Clear the 2 mode bits for this pin.
	port->OSPEEDR |= (s << (pin*2));

	// Clear the 2 pull up/down bits. 0 for No pull-up, pull-down
	port->PUPDR &= ~(3U << (pin*2));
}


void __attribute__ ((interrupt, used)) EXTI15_10_IRQHandler(void)
{
	volatile uint32_t tmp = EXTI->PR1;

	#ifdef INVERTER_OVER_CURRENT_APIN
	// Check if over current is set
	if (tmp & (1 << INVERTER_OVER_CURRENT_APIN))
	{
		portsBreak();
	}
	#endif


	/* Check if interrupt flag is set */
	if (tmp & (1 << FAN1_APIN))
	{
		// If FAN1_APIN is the only interrupt for EXTI15_10_IRQHandler
		// then the flag does not need to be checked.
		#if 0
		int tmp = readInputPin(GPIOA, FAN1_APIN);
		if (tmp!=prevInput)
		{
			counterA++;
			prevInput = tmp;
		}
		#else
		counterA++;
		#endif
	}

	// Clear interrupt flags
	//EXTI_ClearITPendingBit(EXTI_Line1);
	// [1] 13.5.6 Pending register 1 (EXTI_PR1)
	// It is cleared by writing one to it. So not: EXTI->PR1 &= ~2;
	//EXTI->PR1 = (EXTI->PR1 & 0x7DFFFF);
	EXTI->PR1 = 0xFC00;
}


// TODO Take pin as parameter.
void portsGpioExtiInitA()
{
	// Enable needed clocks
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN_Msk;
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN_Msk;

	// Configure fan input pin, typically PA_11
	// Used for fan input, see fan1Init().
	inputPinInit(GPIOA, FAN1_APIN);

	#ifdef INVERTER_OVER_CURRENT_APIN
	// Configure over current input pin, typically PA_6
	inputPinInit(GPIOA, INVERTER_OVER_CURRENT_APIN);
	#endif

	// Fan pulses can use falling or rising edge but over current needs rising edge
	EXTI->FTSR1 |= (1 << FAN1_APIN);
	#ifdef INVERTER_OVER_CURRENT_APIN
	EXTI->RTSR1 |= (1 << INVERTER_OVER_CURRENT_APIN);
	#endif

	// Do not know if this is also needed.
	EXTI->EMR1 |= (1 << FAN1_APIN);
	#ifdef INVERTER_OVER_CURRENT_APIN
	EXTI->EMR1 |= (1 << INVERTER_OVER_CURRENT_APIN);
	#endif

	// Tell system to enable FAN1_APIN for EXTI_Line1
	// Connect EXTI Line1 to PA1 pin
	// Configure the corresponding mask bit in the EXTI_IMR register
	// Interrupt mask line
	EXTI->IMR1 |= (1 << FAN1_APIN);
	#ifdef INVERTER_OVER_CURRENT_APIN
	EXTI->IMR1 |= (1 << INVERTER_OVER_CURRENT_APIN);
	#endif


	// Enable and set EXTI Line1 Interrupt, high prio
	// Set interrupt priority. Lower number is higher priority. Don't need highest possible for this.
	// TODO So line 1 goes to "External Line[15:10] Interrupts"?
	NVIC_SetPriority(EXTI15_10_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL);
	NVIC_EnableIRQ(EXTI15_10_IRQn);
}

int portsGpioGetCounterA()
{
	return counterA;
}

#endif
#endif
