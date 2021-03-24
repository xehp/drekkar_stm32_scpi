#ifndef SOFTUART_H
#define SOFTUART_H


#include "cfg.h"
#include "fifo.h"



#ifdef SOFTUART1_BAUDRATE







// In our application we needed an extra receiver.
// The sender was only used for testing/debugging.
// If no transmit is needed do not define SOFTUART1_TX_PIN below
// Or if receive then do not define SOFTUART1_RX_PIN

// Choose here which IO pins to use
// Recommended is TX=PB6, RX=PB7
// If receiving or transmitting is not needed comment that pin out.
#define SOFTUART1_PORT GPIOA
#define SOFTUART1_TX_PIN 7
#define SOFTUART1_RX_PIN 8
// Note that PA3 can be used by USART2 or LPUART also.

// Using these to test, soft uart will replace usart1
//#define SOFTUART1_PORT GPIOA
//#define SOFTUART1_TX_PIN 9
//#define SOFTUART1_RX_PIN 10


// If TIM2 is only used for soft uart then set TIM2_TICKS_PER_SEC to
// an integer multiple of the max baudrate that needs to be supported.
// TIM2 can be used for some other timing also, if so then baudrate
// will need to be less than 1/3 of TIM2_TICKS_PER_SEC if receiving is
// is supported. That is TIM2_TICKS_PER_SEC below can be set to other
// values than x*baudrate if needed as long as its 3 times the baudrate
// or more.
#ifdef SOFTUART1_RX_PIN
// TIM2_TICKS_PER_SEC need to be at least 3 times baudrate for receiving to work.
// For noise filtering to work it needs a little more.
#define TIM2_TICKS_PER_SEC (SOFTUART1_BAUDRATE*5)
#else
// For receiving only ticks can be same as baudrate.
#define TIM2_TICKS_PER_SEC (SOFTUART1_BAUDRATE)
#endif


// If its OK to hardcode baudrate then use this option to
// save some CPU time and reduce code size.
// If this option is not used baudrate can be changed
// while the poregram is running.
//#define SERIAL_SOFT_HARDCODED_BAUDRATE



typedef struct
{
	#ifdef SOFTUART1_RX_PIN
	struct Fifo inBuffer;
	uint8_t inputFilter;
	char inState;
	int inCounter;
	int inCh;
	#endif

	#ifdef SOFTUART1_TX_PIN
	struct Fifo outBuffer;
	char outState;
	#if (!defined SERIAL_SOFT_HARDCODED_BAUDRATE) || (TIM2_TICKS_PER_SEC != SOFTUART1_BAUDRATE)
	int outCounter;
	#endif
	int outCh;
	#endif

} BufferedSerialSoft;

extern volatile BufferedSerialSoft bufferedSerialSoft1;

int softUart1Init(int baud);

#ifdef SOFTUART1_TX_PIN
inline static void softUart1PutCh(int ch) {fifoPut(&bufferedSerialSoft1.outBuffer, ch);}
#else
inline static void softUart1PutCh(int ch) {;}
#endif

inline static int softUart1GetCh()
{
	#ifdef SOFTUART1_RX_PIN
	if (!fifoIsEmpty(&bufferedSerialSoft1.inBuffer))
	{
		return fifoTake(&bufferedSerialSoft1.inBuffer);
	}
	return -1;
	#else
	return -1;
	#endif
};

//int32_t tim2GetCounter();



#endif
#endif


