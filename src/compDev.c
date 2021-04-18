/*
Barebone blinky on nucleo L432KC / stm32l432kc using SysTick. No RTOS needed.
Copyright (C) Alexander & Henrik Bjorkman www.eit.se 2018
This may be used and redistributed as long as all copyright notices etc are retained .
http://www.eit.se/hb/misc/stm32/stm32l4xx/stm32_barebone/

[1] RM0394 Reference manual Rev 4, http://www.st.com/content/ccc/resource/technical/document/reference_manual/group0/b0/ac/3e/8f/6d/21/47/af/DM00151940/files/DM00151940.pdf/jcr:content/translations/en.DM00151940.pdf
[6] DS11451 Rev 4, https://www.st.com/resource/en/datasheet/stm32l432kb.pdf
[7] https://os.mbed.com/platforms/ST-Nucleo-L432KC/
*/


#include <stdint.h>
#include <ctype.h>

#include "stm32l4xx.h"
#include "stm32l432xx.h"
#include "stm32l4xx_nucleo_32.h"
#include "stm32l4xx_ll_adc.h"

#include "adcDev.h" // TODO remove this
#include "messageNames.h"
#include "log.h"
//#include "ports.h"
#include "compDev.h"
#include "machineState.h"


// TODO Perhaps built in feature can be used? See [1] Chapter 5.2 "Power supply supervisor"
// Would want to understand the comparator anyway so will look at this later.

// Using interrupt would be faster, COMP_IRQn.
// example see https://os.mbed.com/questions/77516/Interrupt-driven-comparator/

// Uncomment one of these to select how this is used.
// (PA1 is sometimes called PA_1)
#define USE_COMP1_PA1
//#define USE_PA1

#define COMP_USE_INTERRUPT


//#define ENABLE_COMP

enum {
	NO_LOGIC_POWER,
	NO_LOGIC_POWER_1_INDICATION,
	NO_LOGIC_POWER_2_INDICATIONS,
	LOGIC_POWER_OK_2_INDICATIONS,
	LOGIC_POWER_OK_1_INDICATION,
	LOGIC_POWER_OK,
};


static int state=NO_LOGIC_POWER;
static int count=0;
#ifdef ENABLE_COMP
static int loggedCount=0;
static int loggedValue=0;

#endif

#ifdef COMP_USE_INTERRUPT

#ifdef USE_PA1
// Be aware that portsGpio.c may have EXTI1_IRQHandler also. There may be only one.
void __attribute__ ((interrupt, used)) EXTI1_IRQHandler(void)
{
	volatile uint32_t tmp = EXTI->PR1;

	/* Make sure that interrupt flag is set */
	if (tmp & (1 << 1))
	{
		//portsBreak();
	}

	count++;

	// Clear interrupt flag
	//EXTI_ClearITPendingBit(EXTI_Line1);
	// [1] 13.5.6 Pending register 1 (EXTI_PR1)
	// It is cleared by writing one to it. So not: EXTI->PR1 &= ~2;
	//EXTI->PR1 = (EXTI->PR1 & 0x7DFFFF);
	EXTI->PR1 = (1 << 1);
}

#else

void __attribute__ ((interrupt, used)) COMP_IRQHandler(void)
{
	count++;
	EXTI->PR1 = (1<<21);
}

#endif

#endif



#if (defined USE_PA1) || (defined USE_PA1_AND_INTERRUPT)
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
#endif

#if 0
static void setupIoPinAlternate(GPIO_TypeDef *base, uint32_t pin, uint32_t alternateFunction)
{
    uint32_t tmp = 0x00;

    /*
    Alternate function register.
    Set it to USART2. Remember to set the port mode to alternate mode also.
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

void comp_init(void)
{
#if defined USE_COMP1_PA1
	// Enable the needed clocks
	// is some clock needed? Did not find.
	// The COMP clock provided by the clock controller is synchronous with the APB2 clock.
	// There is no clock enable control bit provided in the RCC controller. Reset and clock enable
	// bits are common for COMP and SYSCFG.
	// Still not working? Try enable everything.
	// Doing this or not should not be needed so comment this out later.
	// Remove this when code works.
	/*RCC->AHB1ENR |= 0x00011103U;
	RCC->AHB2ENR |= 0x0005209FU;
	RCC->AHB3ENR |= 0x00000100U;
	RCC->APB1ENR1 |= 0xF7EEC633U;
	RCC->APB1ENR2 |= 0x00000003U;
	RCC->APB2ENR |= 0x01235C01U;*/


	// Ref [1] Chapter 19.3 "COMP functional description"

	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN_Msk;

	// Analog should be default so we should not need to do this?
	//const int pin = 1; // 1 for PA1
	//const uint32_t m = 3U; // 3U is for analog mode
	//GPIOA->MODER |= (m << (pin*2));
	// Or shall it be in alternate mode?
	//setupIoPinRx(GPIOA, 1, GPIO_AF7_USART1); // But which alternate function to use?
	//inputPinInit(GPIOA, 1);


	COMP1->CSR = 0;


	const int INPSEL = 2; // 2 = PA1, COMP1_INPSEL, [1] chapter 19.3.2 "COMP pins and internal signals".
	const int INMSEL = 3; // 2 = V_REFINT * 3/4, 3 = V_REFINT
	const int HYST = 1; // 00: No hysteresis, 2 = Medium hysteresis
	const int EN = 1; // 1 = on
	COMP1->CSR = (HYST << COMP_CSR_HYST_Pos)  + (INPSEL<<COMP_CSR_INPSEL_Pos) + (INMSEL<<COMP_CSR_INMSEL_Pos) + (EN<<COMP_CSR_EN_Pos);


	#ifdef COMP_USE_INTERRUPT

	// Need falling edge.
	EXTI->FTSR1 |= (1<<21);

	// Enable interrupts See Table 47 in [1]. EXTI line 21 is COMP1
	EXTI->EMR1 |= (1<<21);

	// Tell system to use PA1 for EXTI_Line1
	// Connect EXTI Line1 to PA1 pin
	// Configure the corresponding mask bit in the EXTI_IMR register
	// Interrupt mask line 1
	EXTI->IMR1 |= (1<<21);

	NVIC_SetPriority(COMP_IRQn, 0UL);
	NVIC_EnableIRQ(COMP_IRQn);
	#endif

	// LOCK register.
	//COMP1->CSR |= COMP_CSR_LOCK_Msk;

#elif (defined USE_PA1)

	// Enable needed clocks
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN_Msk;
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN_Msk;

	// Configure PA1 pin as input
	inputPinInit(GPIOA,1);

	#ifdef COMP_USE_INTERRUPT

	// NOTE line 1 (EXTI1) is for PA1, PB1, PC1, PD1, PE1.
	// Its not line 1 for GPIOA and line 2 for GPIOB.
	// [1] Figure 29. External interrupt/event GPIO mapping


	// Need falling edge on line 1.
	EXTI->FTSR1 |= 2;

	// Do not know if this is also needed.
	EXTI->EMR1 |= 2;

	// Tell system to use PA1 for EXTI_Line1
	// Connect EXTI Line1 to PA1 pin
	// Configure the corresponding mask bit in the EXTI_IMR register
	// Interrupt mask line 1
	EXTI->IMR1 |= 2;


	// Enable and set EXTI Line1 Interrupt, high prio
	// Set interrupt priority. Lower number is higher priority. Need highest possible for this.
	NVIC_SetPriority(EXTI1_IRQn, 0UL);
	NVIC_EnableIRQ(EXTI1_IRQn);
	#endif

#else
	#error
#endif

	//logInt3(COMP_STATE_CHANGED, 0, state1);
}

void compChangeState(int newState)
{
	//logInt4(COMP_STATE_CHANGED, state, newState, count);
	state = newState;
}


int comp_getValue(void)
{
#ifdef USE_COMP1_PA1
	// Does this work? It seems we get interrupts OK but this value not so much.
	return ((COMP1->CSR >> COMP_CSR_VALUE_Pos ) & 1 );
#elif (defined USE_PA1)
	return (GPIOA->IDR >> 1) & 1; // >> 1 for PA1
#else
#error
#endif
}

/**
 * This shall be called once every system tick.
 * Typically once per milli second.
 */
void compMediumTick(void)
{
	// TODO, enable this
#ifdef ENABLE_COMP
	const int value = comp_getValue();

	switch(state)
	{
	default:
	case NO_LOGIC_POWER:
		// Starting state.
		if (value)
		{
			state=NO_LOGIC_POWER_1_INDICATION;
		}
		break;
	case NO_LOGIC_POWER_1_INDICATION:
		// In this state one power OK indication has happened. Perhaps just noise?
		if (value)
		{
			compChangeState(NO_LOGIC_POWER_2_INDICATIONS);
		}
		else
		{
			state=NO_LOGIC_POWER;
		}
		break;
	case NO_LOGIC_POWER_2_INDICATIONS:
		// In this state two power OK indication has happened.
		if (value)
		{
			logInt2(COMP_LOGIC_POWER_OK, count);
			compChangeState(LOGIC_POWER_OK);
		}
		else
		{
			compChangeState(NO_LOGIC_POWER);
		}
		break;
	case LOGIC_POWER_OK_2_INDICATIONS:
		// In this state two power loss indication has happened.
		if (value==0)
		{
			logInt2(COMP_LOGIC_POWER_LOST, count);
			// TODO Enable error if logic power is lost.
			//errorReportError(portsError12VoltLost);
			compChangeState(NO_LOGIC_POWER);
		}
		else
		{
			compChangeState(LOGIC_POWER_OK);
		}
		break;
	case LOGIC_POWER_OK_1_INDICATION:
		// In this state one power loss indication has happened. Perhaps just noise?
		if (value==0)
		{
			compChangeState(LOGIC_POWER_OK_2_INDICATIONS);
		}
		else
		{
			state=LOGIC_POWER_OK;
		}
		break;
	case LOGIC_POWER_OK:
		// In this state all is fine. Logic power is OK.
		if (value==0)
		{
			state=4;
		}
		break;
	}

#endif
}

void compSlowTick(void)
{
#ifdef ENABLE_COMP
	const int value = comp_getValue();

	if ((loggedCount!=count) || (value!=loggedValue))
	{

	#ifdef MEASURE_COMP_AND_REF
		// Ref voltage is on channel 0, PA1 (comp1) is on channel 6.
		logInt6(COMP_COUNT, count, state, value, adc1GetSample(0), adc1GetSample(6));
	#else
		logInt4(COMP_COUNT, count, state, value);
	#endif

		loggedCount=count;
		loggedValue=value;
	}
#endif
}
