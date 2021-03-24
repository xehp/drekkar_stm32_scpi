

#include <stdint.h>
#include <ctype.h>

#include "stm32l4xx.h"
#include "stm32l432xx.h"
#include "stm32l4xx_nucleo_32.h"
#include "stm32l4xx_ll_adc.h"
#include "stm32l4xx_hal_gpio_ex.h"
#include "systemInit.h"

#include "mathi.h"
#include "fifo.h"
#include "SoftUart.h"

#ifdef SOFTUART1_BAUDRATE

#define timerFrequency TIM2_TICKS_PER_SEC

#ifndef SERIAL_SOFT_HARDCODED_BAUDRATE
static int32_t tickDivider = TIM2_TICKS_PER_SEC*2;
#else
#define tickDivider (TIM2_TICKS_PER_SEC*2)
#endif

/* 
Noise Filter
Noise that flips a single sample is filtered out.
It works like this. Input is shifted into a variable. 
Last 3 bits are considered.
The noiseFilter array has 8 positions giving the next output.
The last 3 received bits are input to it.

000 All last 3 input bits are 0, so its a zero out.
001 Majority is 0, so 0 out.
010 Majority is 0, so 0 out.
011 Majority is 1, so 1 out.
100 Majority is 0, so 0 out.
101 Majority is 1, so 1 out.
110 Majority is 1, so 1 out.
111 All last 3 input bits are 1, so 1 out.
*/
#ifdef SOFTUART1_RX_PIN
static const char noiseFilter[8] = {0,0,0,1,0,1,1,1};
#endif

// Integer division rounded (negative numbers might give wrong result)
#define MY_DIV(a,b) ( ((a)+((b)/2)) / (b) )


static int32_t tim2counter=0;

#ifdef SOFTUART1_TX_PIN
static void pinOutInit(GPIO_TypeDef* port, int pin)
{
	port->MODER &= ~(3U << (pin*2)); // Clear the 2 mode bits for this pin.
	port->MODER |= (1U << (pin*2)); // 1 for "General purpose output mode".
	port->OTYPER &= ~(0x1U << (pin*1)); // Clear output type, 0 for "Output push-pull".
	port->OSPEEDR &= ~(0x3U << (pin*2)); // Clear the 2 mode bits for this pin.
	port->OSPEEDR |= (0x1U << (pin*2)); // 1 for "Medium speed" or 2 for High speed.
	port->PUPDR &= ~(3U << (pin*2)); // Clear the 2 pull up/down bits. 0 for No pull-up, pull-down
}


static void pinOn(GPIO_TypeDef* port, int pin)
{
	port->ODR |= (0x1U << pin);
}

static void pinOff(GPIO_TypeDef* port, int pin)
{
	port->ODR &= ~(0x1U << pin);
}


static void pinWrite(GPIO_TypeDef* port, int pin, int bit)
{
	const uint32_t tmp = port->ODR & (~(0x1U << pin));
	port->ODR = tmp | (bit << pin);
}
#endif

#ifdef SOFTUART1_RX_PIN
static void pinInInit(GPIO_TypeDef* port, int pin)
{
	const uint32_t mode = 0; // 0 for "General purpose input mode".
	port->MODER &= ~(3U << (pin*2)); // Clear the 2 mode bits for this pin.
	port->MODER |= (mode << (pin*2));

	const uint32_t speed = 1; // 1 for "Medium speed"
	port->OSPEEDR &= ~(0x3U << (pin*2)); // Clear the 2 speed bits for this pin.
	port->OSPEEDR |= (speed << (pin*2));

	const uint32_t pupd = 1; // 1 for Pull-up
	port->PUPDR &= ~(3U << (pin*2)); // Clear the 2 pull up/down bits.
	port->PUPDR |= pupd << (pin*2);
}

static int pinRead(GPIO_TypeDef* port, int pin)
{
	return (port->IDR >> pin) & 0x1U;
}
#endif

volatile BufferedSerialSoft bufferedSerialSoft1;


void BufferedSerialSoft_init(volatile BufferedSerialSoft *bufferedSerialSoft);

//static volatile struct Fifo* BufferedSerialSoft_getInFifo(volatile BufferedSerialSoft *bufferedSerialSoft) {return &bufferedSerialSoft1.inBuffer;};

//#ifdef SOFT_TRANSMIT_ALSO
//static volatile struct Fifo* BufferedSerialSoft_getOutFifo(volatile BufferedSerialSoft *bufferedSerialSoft) {return &bufferedSerialSoft1.outBuffer;};
//#endif



void BufferedSerialSoft_init(volatile BufferedSerialSoft *bufferedSerialSoft)
{
	memset((void*)bufferedSerialSoft, 0, sizeof(*bufferedSerialSoft));
};


void __attribute__ ((interrupt, used)) TIM2_IRQHandler(void)
{
	#ifdef SOFTUART1_RX_PIN

	// Filter input. Shift new bit into filter buffer.
	// Then use a table to see what the majority of last 3 bits is.
	const int unfilteredInBit = pinRead(SOFTUART1_PORT, SOFTUART1_RX_PIN);
	bufferedSerialSoft1.inputFilter = (bufferedSerialSoft1.inputFilter << 1) + unfilteredInBit;
	const char inBit = noiseFilter[bufferedSerialSoft1.inputFilter & 0x7];

	switch(bufferedSerialSoft1.inState)
	{
		default:
			if (inBit==1)
			{
				bufferedSerialSoft1.inState = 1;
			}
			break;
		case 1:
			if (inBit==0)
			{
				// Start bit hopefully (or noise).
				bufferedSerialSoft1.inCounter = 0;
				bufferedSerialSoft1.inCh = 0;
				bufferedSerialSoft1.inState = 2;
				//startBitCounter++;
			}
			break;
		case 2:
			// Half bit later
			if (++bufferedSerialSoft1.inCounter >= (MY_DIV(1L*timerFrequency,tickDivider)) )
			{
				if (inBit==0)
				{
					// Start bit.
					bufferedSerialSoft1.inState = 3;
				}
				else
				{
					// No start bit, so it was just noise.
					bufferedSerialSoft1.inState = 1;
				}
			}
			break;
		case 3:
			// one and a half bit (3/2) after the falling edge of start bit, and so on.
			if (++bufferedSerialSoft1.inCounter >= (MY_DIV(3L*timerFrequency,tickDivider)) )
			{
				bufferedSerialSoft1.inCh = inBit; // Receive LSB first
				bufferedSerialSoft1.inState++;
			}
			break;
		case 4:
			if (++bufferedSerialSoft1.inCounter >= (MY_DIV(5L*timerFrequency,tickDivider)) )
			{
				bufferedSerialSoft1.inCh |= (inBit << 1);
				bufferedSerialSoft1.inState++;
			}
			break;
		case 5:
			if (++bufferedSerialSoft1.inCounter >= (MY_DIV(7L*timerFrequency,tickDivider)) )
			{
				bufferedSerialSoft1.inCh |= (inBit << 2);
				bufferedSerialSoft1.inState++;
			}
			break;
		case 6:
			if (++bufferedSerialSoft1.inCounter >= (MY_DIV(9L*timerFrequency,tickDivider)) )
			{
				bufferedSerialSoft1.inCh |= (inBit << 3);
				bufferedSerialSoft1.inState++;
			}
			break;
		case 7:
			if (++bufferedSerialSoft1.inCounter >= (MY_DIV(11L*timerFrequency,tickDivider)) )
			{
				bufferedSerialSoft1.inCh |= (inBit << 4);
				bufferedSerialSoft1.inState++;
			}
			break;
		case 8:
			if (++bufferedSerialSoft1.inCounter >= (MY_DIV(13L*timerFrequency,tickDivider)) )
			{
				bufferedSerialSoft1.inCh |= (inBit << 5);
				bufferedSerialSoft1.inState++;
			}
			break;
		case 9:
			if (++bufferedSerialSoft1.inCounter >= (MY_DIV(15L*timerFrequency,tickDivider)) )
			{
				bufferedSerialSoft1.inCh |= (inBit << 6);
				bufferedSerialSoft1.inState++;
			}
			break;
		case 10:
			if (++bufferedSerialSoft1.inCounter >= (MY_DIV(17L*timerFrequency,tickDivider)) )
			{
				bufferedSerialSoft1.inCh |= (inBit << 7);
				bufferedSerialSoft1.inState++;
			}
			break;
		case 11:
			// Stop bit could be checked here if break feature is needed.
			//if (++bufferedSerialSoft1.inCounter >= (MY_DIV(19L*timerFrequency,tickDivider)) )
			//{
			//	if (inBit!=0)
			//	{
			//		// this is the break bit;
			//	}
			//}
			fifoPut(&bufferedSerialSoft1.inBuffer, bufferedSerialSoft1.inCh);
			bufferedSerialSoft1.inState=1;
			tim2counter++;

			break;
	}
	#endif

	#ifdef SOFTUART1_TX_PIN
	#if (!defined SERIAL_SOFT_HARDCODED_BAUDRATE) || (TIM2_TICKS_PER_SEC != SOFTUART1_BAUDRATE)
	switch(bufferedSerialSoft1.outState)
	{
		default:
		case 0:
			if (!fifoIsEmpty(&bufferedSerialSoft1.outBuffer))
			{				
				bufferedSerialSoft1.outCh = fifoTake(&bufferedSerialSoft1.outBuffer);
				pinOff(SOFTUART1_PORT, SOFTUART1_TX_PIN); // Start bit
				bufferedSerialSoft1.outCounter = 0;
				bufferedSerialSoft1.outState++;
			}
			break;
		case 1:
			if (++bufferedSerialSoft1.outCounter >= (MY_DIV(2L*timerFrequency,tickDivider)) ) // *2L (instead of 1L) since tickDivider divides with an extra factor 2.
			{
				pinWrite(SOFTUART1_PORT, SOFTUART1_TX_PIN, bufferedSerialSoft1.outCh & 1); // Send LSB first
				bufferedSerialSoft1.outCh >>= 1; // prepare for sending next bit
				bufferedSerialSoft1.outState++;
			}
			break;
		case 2:
			if (++bufferedSerialSoft1.outCounter >= (MY_DIV(4L*timerFrequency,tickDivider)) )
			{
				pinWrite(SOFTUART1_PORT, SOFTUART1_TX_PIN, bufferedSerialSoft1.outCh & 1);
				bufferedSerialSoft1.outCh >>= 1;
				bufferedSerialSoft1.outState++;
			}
			break;
		case 3:
			if (++bufferedSerialSoft1.outCounter >= (MY_DIV(6L*timerFrequency,tickDivider)) )
			{
				pinWrite(SOFTUART1_PORT, SOFTUART1_TX_PIN, bufferedSerialSoft1.outCh & 1);
				bufferedSerialSoft1.outCh >>= 1;
				bufferedSerialSoft1.outState++;
			}
			break;
		case 4:
			if (++bufferedSerialSoft1.outCounter >= (MY_DIV(8L*timerFrequency,tickDivider)) )
			{
				pinWrite(SOFTUART1_PORT, SOFTUART1_TX_PIN, bufferedSerialSoft1.outCh & 1);
				bufferedSerialSoft1.outCh >>= 1;
				bufferedSerialSoft1.outState++;
			}
			break;
		case 5:
			if (++bufferedSerialSoft1.outCounter >= (MY_DIV(10L*timerFrequency,tickDivider)) )
			{
				pinWrite(SOFTUART1_PORT, SOFTUART1_TX_PIN, bufferedSerialSoft1.outCh & 1);
				bufferedSerialSoft1.outCh >>= 1;
				bufferedSerialSoft1.outState++;
			}
			break;
		case 6:
			if (++bufferedSerialSoft1.outCounter >= (MY_DIV(12L*timerFrequency,tickDivider)) )
			{
				pinWrite(SOFTUART1_PORT, SOFTUART1_TX_PIN, bufferedSerialSoft1.outCh & 1);
				bufferedSerialSoft1.outCh >>= 1;
				bufferedSerialSoft1.outState++;
			}
			break;
		case 7:
			if (++bufferedSerialSoft1.outCounter >= (MY_DIV(14L*timerFrequency,tickDivider)) )
			{
				pinWrite(SOFTUART1_PORT, SOFTUART1_TX_PIN, bufferedSerialSoft1.outCh & 1);
				bufferedSerialSoft1.outCh >>= 1;
				bufferedSerialSoft1.outState++;
			}
			break;
		case 8:
			if (++bufferedSerialSoft1.outCounter >= (MY_DIV(16L*timerFrequency,tickDivider)) )
			{
				pinWrite(SOFTUART1_PORT, SOFTUART1_TX_PIN, bufferedSerialSoft1.outCh & 1);
				bufferedSerialSoft1.outState++;
			}
			break;
		case 9:
			if (++bufferedSerialSoft1.outCounter >= (MY_DIV(18L*timerFrequency,tickDivider)) )
			{
				pinOn(SOFTUART1_PORT, SOFTUART1_TX_PIN); // Stop bit
				bufferedSerialSoft1.outState++;
			}
			break;
		case 10:
			if (++bufferedSerialSoft1.outCounter >= (MY_DIV(20L*timerFrequency,tickDivider)) )
			{
				// Stop bit has been sent by now, current character has been sent.
				bufferedSerialSoft1.outState=0;
			}
			break;

	}
	#else
	switch(bufferedSerialSoft1.outState)
	{
		case 0:
			if (!fifoIsEmpty(&bufferedSerialSoft1.outBuffer))
			{
				bufferedSerialSoft1.outCh = fifoTake(&bufferedSerialSoft1.outBuffer);
				pinOff(SOFTUART1_PORT, SOFTUART1_TX_PIN); // Start bit
				bufferedSerialSoft1.outState++;
			}
			break;
		default:
			{
				// Send LSB first
				pinWrite(SOFTUART1_PORT, SOFTUART1_TX_PIN, bufferedSerialSoft1.outCh & 1);
				bufferedSerialSoft1.outCh >>= 1; // prepare for sending next bit
				bufferedSerialSoft1.outState++;
			}
			break;
		case 8:
			{
				// Now send last bit, it was the MSB (Most Significant Bit) now it is LSB.
				pinWrite(SOFTUART1_PORT, SOFTUART1_TX_PIN, bufferedSerialSoft1.outCh & 1);
				bufferedSerialSoft1.outState++;
			}
			break;
		case 9:
			{
				pinOn(SOFTUART1_PORT, SOFTUART1_TX_PIN); // Stop bit
				bufferedSerialSoft1.outState=0;
			}
			break;
	}
	#endif
	#endif
	TIM2->SR = 0;
}



/*
void BufferedSerialSoft::baud(int baudrate)
{
	this->baudrate = baudrate;
	this->tickDivider = (TICKER_RATE_US*2L)*baudrate;
}
*/




/**
[1] 27.4 TIM2/TIM3 registers
Pulse Width Modulation mode allows you to generate a signal with a frequency determined
by the value of the TIMx_ARR register and a duty cycle determined by the value of the
TIMx_CCRx register.

This code did generate interrupts but did not see any PWM output
(other than the one made in ISR).
*/
static void tmr2Init(int freq)
{
	TIM2->CR1 = 0;

	// Enable Timer clock.
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN_Msk;

	systemBusyWait(1);

	// 26.4.11 TIM1 prescaler (TIMx_PSC)
	// 0 For running fast, 0xFFFF for slow.
	TIM2->PSC = 0;


	// auto-reload register (TIMx_ARR)
	const uint32_t arr = SysClockFrequencyHz/freq;
	TIM2->ARR = arr-1;


	// enable TIM2 interrupt in NVIC

	// Set interrupt priority. Lower number is higher priority.
	// Lowest priority is: (1 << __NVIC_PRIO_BITS) - 1
	const uint32_t prio = (1 << __NVIC_PRIO_BITS) / 4;
	NVIC_SetPriority(TIM2_IRQn, prio);


	//NVIC->ISER[0] = (1 << TIM2_IRQn);
	//NVIC->ISER[(((uint32_t)(int32_t)IRQn) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)(int32_t)IRQn) & 0x1FUL));
	//NVIC->ISER[0] = 1 << 28;
	NVIC_EnableIRQ(TIM2_IRQn);

	TIM2->DIER |= /*TIM_DIER_CC1IE_Msk |*/ TIM_DIER_UIE_Msk;

	// ARPE: Auto-reload preload enable.
	// CEN: enable timer and update interrupt
	TIM2->CR1 = TIM_CR1_ARPE_Msk | TIM_CR1_CEN_Msk;
}


/**
 * Returns 0 if OK.
 */
int softUart1Init(int baud)
{
	// Setup IO (those pins that are to be used).
	#ifdef SOFTUART1_TX_PIN
	pinOn(SOFTUART1_PORT, SOFTUART1_TX_PIN);
	pinOutInit(SOFTUART1_PORT, SOFTUART1_TX_PIN);
	#endif
	#ifdef SOFTUART1_RX_PIN
	pinInInit(SOFTUART1_PORT, SOFTUART1_RX_PIN);
	#endif
	BufferedSerialSoft_init(&bufferedSerialSoft1);

	#ifndef SERIAL_SOFT_HARDCODED_BAUDRATE
	// Calculate tickDivider based on wanted baud rate.
	tickDivider = baud * 2;
	tmr2Init(TIM2_TICKS_PER_SEC);
	// Check that wanted baud rate is at most 1/3 of the timer tick frequency.
	// It works at 1/3 but the noise filter will not filter noise so well.
	if (baud*3 <= TIM2_TICKS_PER_SEC)
	{
		return 0;
	}
	#if (!defined SOFTUART1_RX_PIN)
	// If only transmitting data then a baud rate same as timer tick frequency is also OK
	// (but something like 34 to 99% is not).
	if (baud == TIM2_TICKS_PER_SEC)
	{
		return 0;
	}
	#endif
	return -1;
	#else
	// when baud rate is hard coded tickDivider is calculated at compile time, not here.
	tmr2Init(TIM2_TICKS_PER_SEC);
	// Wanted baud rate must be same as the hard coded value.
	return (baud == SOFTUART1_BAUDRATE) ? 0 : -1;
	#endif
}

/*
volatile struct Fifo* BufferedSerialSoft_getInFifoDev(int dev)
{
	return &bufferedSerialSoft1.inBuffer;
}

volatile struct Fifo* BufferedSerialSoft_getOutFifoDev(int dev)
{
	return &bufferedSerialSoft1.outBuffer;
}
*/

/*
void softUart1PutCh(int ch)
{
	fifoPut(&bufferedSerialSoft1.inBuffer, ch);
}


int softUart1GetCh(int ch)
{
	if (!fifoIsEmpty(&bufferedSerialSoft1.inBuffer))
	{
		return fifoTake(&bufferedSerialSoft1.inBuffer);
	}
	return -1;
}
*/

int32_t tim2GetCounter()
{
	return tim2counter;
}

#endif



