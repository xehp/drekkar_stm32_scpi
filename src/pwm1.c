/*
ports.h

provide functions to set up PWM hardware

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

References
[1] en.DM00151940.pdf "RM0394 Reference manual"
[2] stm32l431kc.pdf "STM32L431xx Datasheet"

History

2019-09-10
Created.
Henrik Bjorkman

*/
#include "stm32l4xx.h"
#include "stm32l432xx.h"
#include "stm32l4xx_nucleo_32.h"
#include "stm32l4xx_ll_adc.h"
#include "stm32l4xx_hal_gpio_ex.h"
#include "systemInit.h"
#include "cfg.h"
#include "messageNames.h"
#include "log.h"
#include "pwm1.h"
#include "serialDev.h"


#if (defined PWM_PORT)

// If inverter is using PA6 then PWM1 can not also use it.
#ifdef INVERTER_OVER_CURRENT_APIN
#error
#endif


#define IS_BREAK_TRIPPED() (TIM1->SR & TIM_SR_BIF_Msk)


static uint32_t tim1UpdateCount = 0;
int pwm1State = 0;
static int swBreak=0;
static int debugLogCount=0;

//#define PWM_USE_TIM16

#ifndef DISABLE_PWM_BREAK_ON_PA6


static void setupIoPinAlternate(GPIO_TypeDef *base, uint32_t pin, uint32_t alternateFunction)
{
    uint32_t tmp = 0x00;

    /*
    Alternate function register.
    Set it to USART2 (or whatever). Remember to set the port mode to alternate mode also.
    */
    const uint32_t afrIndex = pin >> 3;
    const uint32_t afrOffset = (pin & 0x7) * 4;
    tmp = base->AFR[afrIndex];
    tmp &= ~(15U << afrOffset);
    tmp |= alternateFunction << afrOffset;
    base->AFR[afrIndex] = tmp;

    /*
    Configure IO Direction mode (Input, Output, Alternate or Analog)
    00: Input mode
    01: General purpose output mode
    10: Alternate function mode
    11: Analog mode (reset state)
    Need the alternate mode in this case. See also AFR.
    I think it shall be alternate mode but perhaps it shall be input mode?
    */
    const uint32_t mode = 2U; // 2=alternate function
    tmp = base->MODER;
    tmp &= ~(3U << (pin * 2));
    tmp |= (mode << (pin * 2));
    base->MODER = tmp;

    /*
    Configure the IO Speed
    00: Low speed (reset state for most IO pins)
    01: Medium speed
    10: High speed
    11: Very high speed (reset state for PA13)
    */
    const uint32_t speed = 2U;
    tmp = base->OSPEEDR;
    tmp &= ~(3U << (pin * 2));
    tmp |= (speed << (pin * 2));
    base->OSPEEDR = tmp;

    /*
    Configure the IO Output Type
    0: Output push-pull (reset state)
    1: Output open-drain
    Some examples use push-pull others open-drain, did not notice any difference.
    This pin is for input anyway so this should not matter.
    */
    //tmp = GPIOA->OTYPER;
    //tmp &= ~(1U << (pin * 1));
    //tmp |= (1U << (pin * 1));
    //GPIOA->OTYPER = tmp;

    /*
    Activate the Pull-up or Pull down resistor for the current IO
    00: No pull-up, pull-down (reset state for most IO pins)
    01: Pull-up
    10: Pull-down
    11: Reserved
    */
    //const uint32_t pupd = 1U;
    //tmp = GPIOA->PUPDR;
    //tmp &= ~(3U << (pin * 2));
    //tmp |= (pupd << (pin * 2));
    //GPIOA->PUPDR = tmp;
}
#endif

#ifdef USE_TIM1_INTERRUPT

void __attribute__ ((interrupt, used)) TIM1_UP_TIM16_IRQHandler(void)
{
	//const uint32_t sr = TIM1->SR;
	TIM1->SR = 0;
	tim1UpdateCount++;
}

#endif

/*
void __attribute__ ((interrupt, used)) TIM1_TRG_COM_IRQHandler(void)
{
	const uint32_t sr = TIM1->SR;
	TIM1->SR = 0;
    tim1UpdateCount--;
}
*/

/**
 * Ref [1] Chapter 26.3.11 PWM mode
 *
 * The PWM mode can be selected independently on each channel (one PWM per OCx
 * output) by writing ‘0110’ (PWM mode 1) or ‘0111’ (PWM mode 2) in the OCxM bits in the
 * TIMx_CCMRx register. You must enable the corresponding preload register by setting the
 * OCxPE bit in the TIMx_CCMRx register, and eventually the auto-reload preload register (in
 * upcounting or center-aligned modes) by setting the ARPE bit in the TIMx_CR1 register.
 */
void pwm_init()
{
	#ifndef PWM_USE_TIM16
	TIM1->CR1 = 0;

	// Enable Timer clock.
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN_Msk;

	systemBusyWait(2);

	// [1] chapter 26.3.10 Advanced-control timers (TIM1) Output compare mode

	// For testing set PA5 low, needed something to drive PA6 while testing break.
	// TODO remove this later we need PA5 for analog input.
	//systemPinInit(GPIOA, 5);
	//systemPinOff(GPIOA, 5);

	// Enable break feature
#ifndef DISABLE_PWM_BREAK_ON_PA6
	// Break polarity, Break enable
	// enable break to protect circuitry if a short circuit happens.
	TIM1->BDTR |= TIM_BDTR_BKP_Msk | TIM_BDTR_BKE_Msk;

	// Enable break input on PA6.
	setupIoPinAlternate(GPIOA, 6, GPIO_AF1_TIM1);
#else
#error
#endif

	// 1. Select the counter clock (internal, external, prescaler).
	// 0 For running fast, 0xFFFF for slow.
	// Need it fast.
	TIM1->PSC = 0;

	// 2. Write the desired data in the TIMx_ARR and TIMx_CCRx registers.
	// auto-reload register (TIMx_ARR)
	// If we want 50 kHz and system clock is at 80 MHz then reload shall be at:
	// arr = 80000 / 50 = 1600
	const uint16_t arr = 1600;
	TIM1->ARR = arr;

	// [1] 26.4.16 TIM1 capture/compare register 1 (TIM1_CCR1)
	TIM1->CCR1 = 0;

	// 3. Set the CCxIE bit if an interrupt request is to be generated.
	// no interrupt needed on this.

	// 4. Select the output mode.
	// capture/compare enable register
	// OC1PE: 1: Preload register on TIMx_CCR1 enabled.
	// Preload is important or else some pulses might get much longer than
	// intended, very rarely but things may get damaged and hard to understand
	// what went wrong.
	const uint16_t outputMode = 6;
	TIM1->CCMR1 |=  outputMode << TIM_CCMR1_OC1M_Pos | TIM_CCMR1_OC1PE_Msk;

	// Need only one channel.
	// Enable Capture compare channel 1 and its inverted channel.
	// TIM_CCER_CC1P_Msk & TIM_CCER_CC1NP_Msk can be one or zero as long as they are the same.
	// [1] 26.3.15 "Complementary outputs and dead-time insertion"
	// CC1E: Capture/Compare 1 output enable
	//       1: On - OC1 signal is output on the corresponding output pin depending on MOE, OSSI, OSSR, OIS1, OIS1N and CC1NE bits.
	TIM1->CCER |= TIM_CCER_CC1E_Msk | TIM_CCER_CC1NE_Msk;

	// Don't know how much dead time we need, will just take some value here.
	// Perhaps we do not need to set AOE if we have set MOE? One of them should be enough.
	const uint32_t dtg = 5; // Bits 7:0 DTG[7:0]: Dead-time generator setup, to see the dead time use something like 0x80.
	TIM1->BDTR |= TIM_BDTR_MOE_Msk /*| TIM_BDTR_AOE_Msk*/ | dtg << TIM_BDTR_DTG_Pos;





	// TODO set lock bit in TIMx_BDTR register


	// Enable the output pin in alternate mode.
	// [2] Table 15. STM32L431xx pin definitions
	setupIoPinTx(GPIOA, 8, GPIO_AF1_TIM1);

	// Use also the negative channel 1 on PA7
	setupIoPinTx(GPIOA, 7, GPIO_AF1_TIM1);
	// Or put it permanently low like this.
	//systemPinInit(GPIOA, 7);
	//systemPinOff(GPIOA, 7);


	// A dummy interrupt so that we can wake up the CPU each pulse.
	// TODO Let regBuck use system time in milli seconds instead and turn of TIM1 interrupts.
#ifdef USE_TIM1_INTERRUPT
	// Set interrupt priority. Lower number is higher priority.
	// Lowest priority is: (1 << __NVIC_PRIO_BITS) - 1
	const uint32_t prio = (1 << __NVIC_PRIO_BITS) / 2;
	NVIC_SetPriority(TIM1_TRG_COM_IRQn, prio);
	NVIC_EnableIRQ(TIM1_UP_TIM16_IRQn);
	// See also TIM1_TRG_COM_IRQn & TIM1_TRG_COM_IRQHandler etc.
	TIM1->DIER |= /*TIM_DIER_CC1IE_Msk |*/ TIM_DIER_UIE_Msk;
#endif

	// ARPE: Auto-reload preload enable
	// enable interrupt
	TIM1->CR1 = TIM_CR1_ARPE_Msk;


	// 5. Enable the counter by setting the CEN bit in the TIMx_CR1 register.
	// enable timer and update interrupt
	TIM1->CR1 |= TIM_CR1_CEN_Msk;


	// TODO lock registers?

	uart_print_P("pwm_init TIM1\n");

	#else

	// This code is not tested, review before use.
	#error

	TIM16->CR1 = 0;
	TIM16->CCER = 0;

	// Enable Timer clock.
	RCC->APB2ENR |= RCC_APB2ENR_TIM16EN_Msk;
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN_Msk | RCC_AHB2ENR_GPIOAEN_Msk ; // Not needed, GPIOA is enabled in systemInit.c.


	systemBusyWait(1);

	// [1] chapter 26.3.10 Advanced-control timers (TIM1) Output compare mode

	// 1. Select the counter clock (internal, external, prescaler).
	// 0 For running fast, 0xFFFF for slow.
	TIM16->PSC = 0;

	// 2. Write the desired data in the TIMx_ARR and TIMx_CCRx registers.
	// auto-reload register (TIMx_ARR)
	// If we want 50 kHz and sytsem clock is at 80 MHz then reload shall be at:
	// arr = 80000 / 50 = 1600
	const uint16_t arr = 1600;
	TIM16->ARR = arr;

	// [1] TIM16 capture/compare register 1 (TIM16_CCR1)
	// TODO It is loaded permanently if the preload feature is not selected in the TIMx_CCMR1 register (bit OC1PE)?
	// CCR1 = arr / 3, will give a duty cycle that is 1/3 of the time high.
	// CCR1 = 0; will be permanently low.
	// CCR1 = arr; gives almost 100% high (not 100%) there is still a short low.
	// auto-reload register (TIMx_ARR)
	TIM16->CCR1 = arr / 3;

	// 3. Set the CCxIE bit if an interrupt request is to be generated.
	// no interrupt needed on this.

	// 4. Select the output mode.
	// capture/compare enable register
	// CC1E: Capture/Compare 1 output enable
	//       1: On - OC1 signal is output on the corresponding output pin depending on MOE, OSSI, OSSR, OIS1, OIS1N and CC1NE bits.
	// TODO The below is not PWM only toggle, but if it works then we have that.
	const uint16_t outputMode = 6;
	TIM16->CCMR1 =  outputMode << TIM_CCMR1_OC1M_Pos | TIM_CCMR1_OC1PE_Msk;
	TIM16->CCER = TIM_CCER_CC1E_Msk;

	TIM16->BDTR |= TIM_BDTR_AOE_Msk;


	// 5. Enable the counter by setting the CEN bit in the TIMx_CR1 register.
	// ARPE: Auto-reload preload enable
	// enable timer and update interrupt
	TIM16->CR1 = /*TIM_CR1_ARPE_Msk |*/ TIM_CR1_CEN_Msk;

	// Enable the output pin in alternate mode.
	// [2] Table 15. STM32L431xx pin definitions
	setupIoPinRx(GPIOA, 6, GPIO_AF14_TIM16);

	uart_print_P("pwm_init TIM16\n");

	#endif
}

// Gives current period time.
// Unit is probably in 1/SysClockFrequencyHz.
uint32_t pwm_get_period_time(void)
{
#ifndef PWM_USE_TIM16
  return TIM1->ARR;
#else
  return TIM16->ARR;
#endif
}

//uint16_t avr_tmr1_get_counter(void);

// How long a period shall be.
void pwm_set_period_time(uint32_t period_time_t1)
{
#ifndef PWM_USE_TIM16
	TIM1->ARR = period_time_t1;
#else
	TIM16->ARR = period_time_t1;
#endif
}


// How long the pulse shall be.
// 0 for no pulse.
// Same as pwm_set_period_time for max pulse.
void pwm_set_duty_cycle(uint32_t puls_time_t1)
{
	if ((swBreak) || IS_BREAK_TRIPPED())
	{
		// set to zero if break is active.
		TIM1->CCR1 = 0;
		if (debugLogCount==0)
		{
			logInt5(PWM_DID_NOT_SET_CCR1, TIM1->CCR1, puls_time_t1, swBreak, IS_BREAK_TRIPPED());
			debugLogCount++;
		}
	}
	else
	{
		TIM1->CCR1 = puls_time_t1;
	}
}

uint32_t pwm_get_duty_cycle(void)
{
#ifndef PWM_USE_TIM16
	return TIM1->CCR1;
#else
	return TIM16->CCR1;
#endif
}


void pwm_seconds_tick()
{
	// TODO const int b = pwm_isBreakActive();
	const int b = swBreak;

	switch(pwm1State)
	{
	case 0:
		if ((b) || (swBreak))
		{
			pwm_set_duty_cycle(0);
			errorReportError(portsTimerBreak);
			pwm1State = 1;
			logInt2(PWM_STATE_CHANGE, pwm1State);
		}
		break;
	default:
		pwm_set_duty_cycle(0);
		if (!b)
		{
			pwm1State = 0;
			logInt2(PWM_STATE_CHANGE, pwm1State);
		}
		break;
	}

}

uint32_t pwm_getCounter()
{
	return tim1UpdateCount;
}

// IS_PWM1_BREAK_ACTIVE_PAR
int pwm_isBreakActive()
{
	return IS_BREAK_TRIPPED() || swBreak;
}

void pwm_break()
{
	// TODO: TIM1->EGR |= TIM_EGR_BG_Msk;
	swBreak = 1;
	pwm_set_duty_cycle(0);
}

void pwm_clearBreak()
{
	// TODO: How do we clear HW break?
	// Can we do:
	// TIM1->EGR &= ~TIM_EGR_BG_Msk;
	swBreak = 0;
}
#endif
