/*
Barebone blinky on nucleo L432KC / stm32l432kc using SysTick. No RTOS needed.
Copyright (C) Alexander & Henrik Bjorkman www.eit.se 2018
This may be used and redistributed as long as all copyright notices etc are retained .
http://www.eit.se/hb/misc/stm32/stm32l4xx/stm32_barebone/

References
[1] https://www.st.com/content/ccc/resource/technical/document/reference_manual/group0/b0/ac/3e/8f/6d/21/47/af/DM00151940/files/DM00151940.pdf/jcr:content/translations/en.DM00151940.pdf
[6] DS11451 Rev 4, https://www.st.com/resource/en/datasheet/stm32l432kb.pdf
*/

#include "stm32l4xx.h"
#include "stm32l432xx.h"
#include "stm32l4xx_nucleo_32.h"
#include "stm32l4xx_ll_adc.h"
#include "stm32l4xx_hal_gpio_ex.h"
#include "systemInit.h"
#include "timerDev.h"
#include "serialDev.h"




/**
 * In this example the LPTIM1 is used as Pulse Counter.
 * Gpio pin PB5, PB6 or PB7.
 * [1] 30 Low-power timer (LPTIM)
 * 
 */
void lptmr1Init()
{
	LPTIM1->CR = 0;

	// Enable Timer clock (perhaps not needed when using external input?).
	RCC->APB1ENR1 |= RCC_APB1ENR1_LPTIM1EN_Msk;
	systemBusyWait(1);

	// [1] 30.7.4 LPTIM configuration register (LPTIM_CFGR)
	// Programming the CKSEL and COUNTMODE bits allows controlling
	// whether the LPTIM will use an external clock source or an internal one.
	LPTIM1->CFGR = /*LPTIM_CFGR_COUNTMODE_Msk |*/ LPTIM_CFGR_CKSEL_Msk;

	// [1] 30.4.4 Glitch filter.
    // CKFLT
	// 01: external clock signal level change must be stable for at least 2 clock periods before it is
	// considered as valid transition.
	const int ckflt = 1;
	LPTIM1->CFGR |= ckflt << LPTIM_CFGR_CKFLT_Pos;

	// [1] 30.7.9 LPTIM1 option register (LPTIM1_OR)
	LPTIM1->OR = 0;

	// Set PB5, PB6 or PB7 as alternate function 1. [6]
	// Ports also known as PB_5, PB_6, PB_7, PB-5, PB-6, PB-7.
	setupIoPinRx(GPIOB, 5, GPIO_AF1_LPTIM);
	//setupIoPinRx(GPIOB, 6, GPIO_AF1_LPTIM);
	//setupIoPinRx(GPIOB, 7, GPIO_AF1_LPTIM);

	// enable timer and start
	// [1] "The timer must be enabled before setting the SNGSTRT/CNTSTRT bits.
	// Any write on these bits when the timer is disabled will be discarded
	// by hardware."
	// [1] "To enable the continuous counting, the CNTSTRT bit must be set."
	LPTIM1->CR = LPTIM_CR_ENABLE_Msk;
	systemBusyWait(10);

	// [1] The LPTIM_ARR register’s content must only be modified when the LPTIM
	// is enabled (ENABLE bit is set to ‘1’).
	// The reset value for this one is strangely 1. So it only counts to 1!
	//LPTIM1->ARR = 10000-1;
	// Or shall it be LPTIM1->ARR = 0 ?
	LPTIM1->ARR = 0xFFFF;

	LPTIM1->CR |= LPTIM_CR_CNTSTRT_Msk;
}

/* [1]
When the LPTIM is running with an asynchronous clock, reading the LPTIM_CNT register may 
return unreliable values. So in this case it is 
necessary to perform two consecutive read accesses 
and verify that the two returned values are identical.
It should be noted that for a reliable LPTIM_CNT
register read access, two consecutive read 
accesses must be performed and compared. A read access can be considered reliable when the 
values of the two consecutive read accesses are equal.
*/
uint16_t lptmr1GetCount()
{
	volatile uint16_t read1;
	volatile uint16_t read2;
	for(;;)
	{
		read1 = LPTIM1->CNT;
		read2 = LPTIM1->CNT;
		if (read1 == read2)
		{
			break;
		}
	}
	return read2;
}



/**
 * In this example the LPTIM2 is used as Pulse Counter.
 * Gpio pin PB6. Or is it PB5 or PB7 to use? Or whatever, PB1 perhaps.
 * [1] 30 Low-power timer (LPTIM)
 * 
 * Regarding [1] 30.4.4 Glitch filter.
 * We may want it for fan and temp input but not voltage inputs.
 * Current plan is to use lptmr2 (AKA LPTIM2) for voltage.
 */
void lptmr2Init()
{
	LPTIM2->CR = 0;

	// Enable Timer clock (perhaps not needed when using external input)
	RCC->APB1ENR2 |= RCC_APB1ENR2_LPTIM2EN_Msk;
	systemBusyWait(1);

	// [1] 30.7.4 LPTIM configuration register (LPTIM_CFGR)
	// Programming the CKSEL and COUNTMODE bits allows controlling
	// whether the LPTIM will use an external clock source or an internal one.
	LPTIM2->CFGR = LPTIM_CFGR_CKSEL_Msk;

	// CKFLT
	// [1] 30.4.4 Glitch filter.
	// 01: external clock signal level change must be stable for at least 2 clock periods before it is
	// considered as valid transition.
	const int ckflt = 1;
	LPTIM1->CFGR |= ckflt << LPTIM_CFGR_CKFLT_Pos;


	// [1] 30.7.9 LPTIM1 option register (LPTIM1_OR)
	LPTIM2->OR = 0;


	// Set PB1 as alternate function 14. [6]
	setupIoPinRx(GPIOB, 1, GPIO_AF14_LPTIM2);

	// enable timer and start
	// [1] "The timer must be enabled before setting the SNGSTRT/CNTSTRT bits.
	// Any write on these bits when the timer is disabled will be discarded
	// by hardware."
	// [1] "To enable the continuous counting, the CNTSTRT bit must be set."
	LPTIM2->CR = LPTIM_CR_ENABLE_Msk;
	systemBusyWait(10);

	// [1] The LPTIM_ARR register’s content must only be modified when the LPTIM
	// is enabled (ENABLE bit is set to ‘1’).
	// The reset value for this one is strangely 1. So it only counts to 1!
	//LPTIM1->ARR = 10000-1;
	LPTIM2->ARR = 0xFFFF;

	// start counting
	LPTIM2->CR |= LPTIM_CR_CNTSTRT_Msk;
}

/* [1]
When the LPTIM is running with an asynchronous clock, reading the LPTIM_CNT register may 
return unreliable values. So in this case it is 
necessary to perform two consecutive read accesses 
and verify that the two returned values are identical.
It should be noted that for a reliable LPTIM_CNT
register read access, two consecutive read 
accesses must be performed and compared. A read access can be considered reliable when the 
values of the two consecutive read accesses are equal.
*/
uint16_t lptmr2GetCount()
{
	volatile uint16_t read1;
	volatile uint16_t read2;
	for(;;)
	{
		read1 = LPTIM2->CNT;
		read2 = LPTIM2->CNT;
		if (read1 == read2)
		{
			break;
		}
	}
	return read2;
}

