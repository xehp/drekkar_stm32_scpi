/*
Barebone blinky on nucleo L432KC / stm32l432kc using SysTick. No RTOS needed.
Copyright (C) Alexander & Henrik Bjorkman www.eit.se 2018
This may be used and redistributed as long as all copyright notices etc are retained .
http://www.eit.se/hb/misc/stm32/stm32l4xx/stm32_barebone/

References
[1] DocID027295 Rev 3, http://www.st.com/content/ccc/resource/technical/document/reference_manual/group0/b0/ac/3e/8f/6d/21/47/af/DM00151940/files/DM00151940.pdf/jcr:content/translations/en.DM00151940.pdf
[2] DS11453 Rev 3 (stm32l431kc.pdf)
[3] Reference Manual RM0394 Rev 4 (en.DM00151940.pdf)
*/

#include "stm32l4xx.h"
#include "stm32l432xx.h"
#include "stm32l4xx_nucleo_32.h"
#include "stm32l4xx_ll_adc.h"
#include "systemInit.h"
#include "cfg.h"
#include "fifo.h"
#include "miscUtilities.h"
#ifdef SOFTUART1_BAUDRATE
#include "SoftUart.h"
#endif
#include "serialDev.h"


void* memset(void* bufptr, int value, unsigned int size);



#ifdef LPUART1_BAUDRATE
	//#define LPUART1_TX_PIN 2
	#define LPUART1_RX_PIN 3
	volatile struct Fifo lpuart1In = {0,0,{0}};
	#ifdef LPUART1_TX_PIN
	volatile struct Fifo lpuart1Out = {0,0,{0}};
	#endif
#endif


volatile struct Fifo usart1In = {0,0,{0}};
volatile struct Fifo usart1Out = {0,0,{0}};

#ifdef USART2_BAUDRATE
	// If PA2 is needed by LPUART comment the line below.
	//setupIoPinRx(GPIOA, 3, GPIO_AF7_USART2);
	// PA3 does not work via stlink (PA2 does work for sending), use PA_15 for usart2 RX instead.
	// https://community.st.com/s/question/0D50X00009XkYKK/usart-vcp-connections-on-nucleol432kc
	#define USART2_TX_PIN 2
	#define USART2_RX_PIN 15
	volatile struct Fifo usart2In = {0,0,{0}};
	#ifdef USART2_TX_PIN
	volatile struct Fifo usart2Out = {0,0,{0}};
	#endif
#endif


// NOTE Two uarts, usarts etc shall not use same pins.
// For example USART2 and LPUART1 can not both use PA2.
#if ((defined LPUART1_TX_PIN) && (defined USART2_TX_PIN)) && (LPUART1_TX_PIN==USART2_TX_PIN)
#error
#endif


#ifdef LPUART1_BAUDRATE
void __attribute__ ((interrupt, used)) LPUART1_IRQHandler(void)
{
  volatile uint32_t tmp = LPUART1->ISR;

  // RXNE (Receive not empty)
  if (tmp & USART_ISR_RXNE_Msk)
  {
    // Simple receive, this never happens :(
    //mainDummy = USART1->RDR;
    fifoPut(&lpuart1In, LPUART1->RDR);
  }

  #ifdef LPUART1_TX_PIN
  // TXE (transmit empty)
  if (tmp & USART_ISR_TXE_Msk)
  {
    if (!fifoIsEmpty(&lpuart1Out))
    {
      LPUART1->TDR = fifoTake(&lpuart1Out);
    }
    else
    {
      // We are done sending, disables further tx empty interrupts. 
      LPUART1->CR1 &= ~(USART_CR1_TXEIE_Msk);
    }
  }
  #endif
}
#endif

void __attribute__ ((interrupt, used)) USART1_IRQHandler(void)
{
  volatile uint32_t tmp = USART1->ISR;

  // RXNE (Receive not empty)
  if (tmp & USART_ISR_RXNE_Msk)
  {
    // Simple receive, this never happens :(
    //mainDummy = USART1->RDR;
    fifoPut(&usart1In, USART1->RDR);
  }

  // TXE (transmit empty)
  if (tmp & USART_ISR_TXE_Msk)
  {
    if (!fifoIsEmpty(&usart1Out))
    {
      USART1->TDR = fifoTake(&usart1Out);
    }
    else
    {
      // We are done sending, disables further tx empty interrupts. 
      USART1->CR1 &= ~(USART_CR1_TXEIE_Msk);
    }
  }
}

#ifdef USART2_BAUDRATE
void __attribute__ ((interrupt, used)) USART2_IRQHandler(void)
{
  volatile uint32_t tmp = USART2->ISR;

  // RXNE (Receive not empty)
  if (tmp & USART_ISR_RXNE_Msk)
  {
    // Simple receive, this never happens :(
    fifoPut(&usart2In, (int)(USART2->RDR & 0xFF));

    // For debugging count the RXNE interrupts.
    //mainCh++; // Remove this when things work.
  }

  #ifdef USART2_TX_PIN
  // TXE (transmit empty)
  if (tmp & USART_ISR_TXE_Msk)
  {
    if (!fifoIsEmpty(&usart2Out))
    {
      USART2->TDR = fifoTake(&usart2Out);
    }
    else
    {
      // We are done sending, disables further tx empty interrupts. 
      USART2->CR1 &= ~(USART_CR1_TXEIE_Msk);

      // This was just for testing, remove later...
      //USART2->CR1 |= (USART_CR1_RXNEIE_Msk);
    }
  }
  #endif
}
#endif

/* This seems to work fine, we do get what the Nucleo sends */
void setupIoPinTx(GPIO_TypeDef *base, uint32_t pin, uint32_t alternateFunction)
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
  
    /* [1]
    Configure IO Direction mode (Input, Output, Alternate or Analog)
    00: Input mode 
    01: General purpose output mode
    10: Alternate function mode
    11: Analog mode (reset state)
    Need the alternate mode in this case. See also AFR.
    */
    const uint32_t mode = 2U;
    tmp = base->MODER;
    tmp &= ~(3U << (pin * 2));
    tmp |= (mode << (pin * 2));
    base->MODER = tmp;

    /* [1]
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

    /* [1]
    Configure the IO Output Type
    0: Output push-pull (reset state)
    1: Output open-drain
    */
    //const uint32_t ot = 2U;
    //tmp = GPIOA->OTYPER;
    //tmp &= ~(1U << (pin * 1));
    //tmp |= (ot << (pin * 1));
    //GPIOA->OTYPER = tmp;

    /* [1]
    Activate the Pull-up or Pull down resistor for the current IO 
    00: No pull-up, pull-down (reset state for most IO pins)
    01: Pull-up
    10: Pull-down
    11: Reserved
    */
    //tmp = GPIOA->PUPDR;
    //tmp &= ~(3U << (pin * 2));
    //tmp |= (GPIO_NOPULL << (pin * 2));
    //GPIOA->PUPDR = tmp;
}

/**
 * Perhaps both TX and RX pins can be configured the same way.
 * If so only one method setupIoPinRxTx should be needed.
 * */
void setupIoPinRx(GPIO_TypeDef *base, uint32_t pin, uint32_t alternateFunction)
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
    const uint32_t mode = 2U;
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
    Seems resonable to activate Pull-up if open drain is used, but did not notice any improvement doing so.
    */
    const uint32_t pupd = 1U;
    tmp = GPIOA->PUPDR;
    tmp &= ~(3U << (pin * 2));
    tmp |= (pupd << (pin * 2));
    GPIOA->PUPDR = tmp;
}



USART_TypeDef *usartGetPtr(int usartNr)
{
  switch(usartNr)
  {
    case DEV_USART1: return USART1;
    case DEV_USART2: return USART2;
    default : break;
  }
  return NULL;
}

int usartGetIrqN(int usartNr)
{
  switch(usartNr)
  {
    case DEV_LPUART1: return LPUART1_IRQn;
    case DEV_USART1: return USART1_IRQn;
    case DEV_USART2: return USART2_IRQn;
    default : break;
  }
  return 0;
}

#ifdef LPUART1_BAUDRATE
int lpuartInit(uint32_t baud)
{

  /* Disable the Peripheral, should be the reset state but just in case. */
  LPUART1->CR1 &= ~USART_CR1_UE_Msk;

  /*
  USART1_TX on PA9, USART1_RX on PA10. 
  But it is Usart2 that is connected to USB so using that one. 
  USART2 GPIO Configuration: USART2_TX on PA2, USART2_RX on PA3
  */

  // Clear the in and out FIFOs
  fifoInit(&lpuart1In);
  #ifdef LPUART1_TX_PIN
  fifoInit(&lpuart1Out);
  #endif

  // Enable Uart clock LPUART1EN
  RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN_Msk;

  #if LPUART1_BAUDRATE == 9600
  // Select clock LPUART1SEL
  {
	// [3] 6.4.27 Peripherals independent clock configuration register (RCC_CCIPR)
	// Bits 11:10 LPUART1SEL[1:0]: LPUART1 clock source selection
	// These bits are set and cleared by software to select the LPUART1 clock source.
	// 00: PCLK selected as LPUART1 clock
	// 01: System clock (SYSCLK) selected as LPUART1 clock
	// 10: HSI16 clock selected as LPUART1 clock
	// 11: LSE clock selected as LPUART1 clock
    uint32_t tmp = RCC->CCIPR;
    uint32_t sel = 2u;
    tmp &= ~(3 << RCC_CCIPR_LPUART1SEL_Pos);
    tmp |= (sel << RCC_CCIPR_LPUART1SEL_Pos);
    RCC->CCIPR = tmp;
  }
  #endif


  // On Nucleo L432KC we use Serial2 on PA2 & PA15
  // So PA3 is free to use for LPUART1 but we have no transmit pin available.
  // TODO Use LPUART1 for receiving and SoftUart1 for transmitting.

  // Use PA2 for TX & PA3 for RX
  #ifdef LPUART1_TX_PIN
  setupIoPinTx(GPIOA, LPUART1_TX_PIN, GPIO_AF8_LPUART1);
  #endif
  #ifdef LPUART1_RX_PIN
  setupIoPinRx(GPIOA, LPUART1_RX_PIN, GPIO_AF8_LPUART1);
  #endif



  /*
  Select the desired baud rate using the USART_BRR register.
  https://community.st.com/thread/46664-setting-the-baudrate-for-usart-in-stm32f103rb
  Ref [1] chapter 36.5.4 USART baud rate generation 
  */
  #if LPUART1_BAUDRATE == 115200
  uint32_t clockDiv = 0x2B671;  // according to Table 203 in ref [3].
  #elif LPUART1_BAUDRATE == 38400
  uint32_t clockDiv = 0x82355;
  #elif LPUART1_BAUDRATE == 9600
  //uint32_t clockDiv = 0x369;  // according to Table 202 in ref [3].
  uint32_t clockDiv = 426667; // (256*16000000LL)/9600;
  #else
  #error
  #endif

  // LPUART1->BRR is a 20 bit register, no use setting larger than 0xfffff.
  // Less than 0x300 is not allowed according to [3] 39.4.4 LPUART baud rate generation
  if ((clockDiv<0x300) || (clockDiv>0xfffff))
  {
    return -1;
  }

  LPUART1->BRR = clockDiv;

  /* Enable uart transmitter and receiver */
  LPUART1->CR1 |= USART_CR1_TE_Msk | USART_CR1_RE_Msk;


  // Bit 1 UESM: USART enable in Stop mode 
  // just testing, this should not be needed in this example.
  //LPUART1->CR1 |= USART_CR1_UESM_Msk;

  /*
  In asynchronous mode, the following bits must be kept cleared:
  - LINEN and CLKEN bits in the USART_CR2 register,
  - SCEN, HDSEL and IREN  bits in the USART_CR3 register.
  */
  //LPUART1->CR2 &= ~(USART_CR2_LINEN_Msk | USART_CR2_CLKEN_Msk);
  //LPUART1->CR3 &= ~(USART_CR3_SCEN_Msk | USART_CR3_HDSEL_Msk | USART_CR3_IREN_Msk);
  LPUART1->CR2 &= ~(USART_CR2_CLKEN_Msk);



  //Set Usart1 interrupt priority. Lower number is higher priority.
  //uint32_t prioritygroup = NVIC_GetPriorityGrouping();
  //NVIC_SetPriority(USART2_IRQn, NVIC_EncodePriority(prioritygroup, 45, 0));
  const int irqN = usartGetIrqN(DEV_LPUART1);
  NVIC_SetPriority (irqN, (1UL << __NVIC_PRIO_BITS) - 1UL);

  // This one helped.
  // https://community.st.com/thread/19735
  // I was missing to do NVIC_EnableIRQ.
  NVIC_EnableIRQ(irqN);

  /* Enable the Peripheral */
  LPUART1->CR1 |= USART_CR1_UE_Msk;

  /* 
  Shall interrupt be enabled before or after usart is enabled? 
  Did not notice any difference doing this before or after.
  RXNEIE
  0: Interrupt is inhibited
  1: A USART interrupt is generated whenever ORE=1 or RXNE=1 in the USART_ISR 
  */
  LPUART1->CR1 |= USART_CR1_RXNEIE_Msk;

  return 0;
}
#endif

/**
This sets up the Usart 1 or 2. 

USART1_TX on PA9, USART1_RX on PA10. 
Usart2 sends on USB (perhaps also on PA2), it receives on PA3.

Returns 0 if OK.
*/
static int initUsart1or2(int usartNr, uint32_t baud)
{
	USART_TypeDef *usartPtr = usartGetPtr(usartNr);
	if (usartPtr == NULL) 
	{
		return -1;
	}

	/* Disable the Peripheral, should be the reset state but just in case. */
	usartPtr->CR1 &= ~USART_CR1_UE_Msk;

	// Clear the in and out FIFOs
	// Enable Usart clock
	switch (usartNr)
	{
	case DEV_USART1:
		// If higher baud rate is needed change clock source.
		// Select clock LPUART1SEL
		/*{
		 uint32_t tmp = RCC->CCIPR;
		 tmp &= ~(3 << 0);
		 tmp |= ~(1 << 0);
		 RCC->CCIPR = tmp;
		 }*/
		fifoInit(&usart1In);
		fifoInit(&usart1Out);
		RCC->APB2ENR |= RCC_APB2ENR_USART1EN_Msk;

		// Configure IO pins.
		#if 1
		setupIoPinTx(GPIOA, 9, GPIO_AF7_USART1);
		setupIoPinRx(GPIOA, 10, GPIO_AF7_USART1);
		break;
		#else
		// If PA9 & PA10 are needed by something else try these instead:
		setupIoPinTx(GPIOB, 6, GPIO_AF7_USART1);
		setupIoPinRx(GPIOB, 7, GPIO_AF7_USART1);
		#endif

		break;
#ifdef USART2_BAUDRATE
	case DEV_USART2:
		// If higher baud rate is needed change clock source.
		// Select clock LPUART1SEL
		/*{
		 uint32_t tmp = RCC->CCIPR;
		 tmp &= ~(3 << 2);
		 tmp |= ~(1 << 2);
		 RCC->CCIPR = tmp;
		 }*/
		fifoInit(&usart2In);
		#ifdef USART2_TX_PIN
		fifoInit(&usart2Out);
		#endif
		RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN_Msk;  // bit 17

		// Configure IO pins, to find pins and alternate function codes see table 16 in ref [2].

		// Transmit pin
        #ifdef USART2_TX_PIN
		setupIoPinTx(GPIOA, USART2_TX_PIN, GPIO_AF7_USART2);
        #endif

		// Receive pin
		setupIoPinRx(GPIOA, USART2_RX_PIN, GPIO_AF3_USART2);

		break;
#endif
	default:
		return -1;
	}


	/*
	 [1] chapter 36.8.1 Control register 1 (USART_CR1)
	 36.8.12 USART register map
	 8-bit character length: M[1:0] = 00, that is default.
	 1 stop bit is the default value of number of stop bits.
	 That is what we want. So skipping those settings.
	 */

	/*
	 OVER8
	 0: Oversampling by 16 (default)
	 1: Oversampling by 8
	 */
	//usartPtr->CR1 |= (1 << 15);
	/*
	 Select the desired baud rate using the USART_BRR register.
	 https://community.st.com/thread/46664-setting-the-baudrate-for-usart-in-stm32f103rb
	 Ref [1] chapter 36.5.4 USART baud rate generation
	 */
	uint32_t divider = SysClockFrequencyHz / baud;
	/*if (divider<0x300)
	 {
	 return -1;
	 }*/
	usartPtr->BRR = divider;

	/* Enable uart transmitter and receiver */
	usartPtr->CR1 |= USART_CR1_TE_Msk | USART_CR1_RE_Msk;

	// Bit 1 UESM: USART enable in Stop mode
	// just testing, this should not be needed in this example.
	//usartPtr->CR1 |= USART_CR1_UESM_Msk;

	// We may need to invert RX line if we use a simple optocoupler
	// That is done in CR2 Bit 16 RXINV like this:
	// usartPtr->CR2 |= USART_CR2_RXINV_Msk;
	// No we may need to invert TX line:
	// usartPtr->CR2 |= USART_CR2_TXINV_Msk;

	/*
	 In asynchronous mode, the following bits must be kept cleared:
	 - LINEN and CLKEN bits in the USART_CR2 register,
	 - SCEN, HDSEL and IREN  bits in the USART_CR3 register.
	 */
	//USART2->CR2 &= ~(USART_CR2_LINEN_Msk | USART_CR2_CLKEN_Msk);
	//USART2->CR3 &= ~(USART_CR3_SCEN_Msk | USART_CR3_HDSEL_Msk | USART_CR3_IREN_Msk);
	usartPtr->CR2 &= ~(USART_CR2_CLKEN_Msk);

	systemBusyWait(1);

	//Set Usart1 interrupt priority. Lower number is higher priority.
	//uint32_t prioritygroup = NVIC_GetPriorityGrouping();
	//NVIC_SetPriority(USART2_IRQn, NVIC_EncodePriority(prioritygroup, 45, 0));
	const int irqN = usartGetIrqN(usartNr);
	NVIC_SetPriority(irqN, (1UL << __NVIC_PRIO_BITS) - 1UL);

	// This one helped.
	// https://community.st.com/thread/19735
	// I was missing to do NVIC_EnableIRQ.
	NVIC_EnableIRQ(irqN);

	/* Enable the Peripheral */
	usartPtr->CR1 |= USART_CR1_UE_Msk;

	/*
	 Shall interrupt be enabled before or after usart is enabled?
	 Did not notice any difference doing this before or after.
	 RXNEIE
	 0: Interrupt is inhibited
	 1: A USART interrupt is generated whenever ORE=1 or RXNE=1 in the USART_ISR
	 */
	usartPtr->CR1 |= USART_CR1_RXNEIE_Msk;

	return 0;
}

// Returns zero if OK.
int serialInit(int usartNr, uint32_t baud)
{
	switch(usartNr)
	{
		#ifdef LPUART1_BAUDRATE
		case DEV_LPUART1: return lpuartInit(baud);
		#endif
		case DEV_USART1: return initUsart1or2(usartNr, baud);
		#ifdef USART2_BAUDRATE
		case DEV_USART2: return initUsart1or2(usartNr, baud);
		#endif
		#ifdef SOFTUART1_BAUDRATE
		case DEV_SOFTUART1: return softUart1Init(baud);
		#endif
		default: break;
	}
	return -1;
}


#ifdef LPUART1_TX_PIN
static inline void lpuart1Put(int ch)
{
	fifoPut(&lpuart1Out, ch);
	LPUART1->CR1 |= USART_CR1_TXEIE_Msk;
}
#endif


static inline void usart1Put(int ch)
{
	fifoPut(&usart1Out, ch);
	/*
	Now we need to trigger the ISR. Its done by enabling transmitter empty interrupt (it is empty so).
	TXEIE
	0: Interrupt is inhibited
	1: A USART interrupt is generated whenever TXE=1 in the USART_ISR register
	*/
	USART1->CR1 |= USART_CR1_TXEIE_Msk;
}


#ifdef USART2_TX_PIN
static inline void usart2Put(int ch)
{
	// If FIFO is full shall we wait or not?
	// Comment out lines below if not.
	//while (fifoIsFull(usart2Out))
	//{
	//	systemSleep();
	//}

	fifoPut(&usart2Out, ch);
	// Now we need to trigger the ISR. Its done by enabling transmitter empty interrupt (it is empty so).
	USART2->CR1 |= USART_CR1_TXEIE_Msk;
}
#endif


// Ref [1] chapter 36.5.2 USART transmitter
void serialPutChar(int usartNr, int ch)
{
	switch(usartNr)
	{
		#ifdef LPUART1_TX_PIN
		case DEV_LPUART1:
			lpuart1Put(ch);
			break;
		#endif
		case DEV_USART1:
			usart1Put(ch);
			break;
		#ifdef USART2_TX_PIN
		case DEV_USART2:
			usart2Put(ch);
			break;
		#endif
		#ifdef SOFTUART1_BAUDRATE
		case DEV_SOFTUART1:
			softUart1PutCh(ch);
			break;
		#endif
		default:
			// Ignore this.
		break;
	}
}

int serialGetChar(int usartNr)
{
	switch(usartNr)
	{
		#ifdef LPUART1_BAUDRATE
		case DEV_LPUART1:
			if (!fifoIsEmpty(&lpuart1In))
			{
				return fifoTake(&lpuart1In);
			}
			return -1;
		#endif
		case DEV_USART1:
			if (!fifoIsEmpty(&usart1In))
			{
				return fifoTake(&usart1In);
			}
			return -1;
		#ifdef USART2_BAUDRATE
		case DEV_USART2:
			if (!fifoIsEmpty(&usart2In))
			{
				return fifoTake(&usart2In);
			}
			return -1;
		#endif
		#ifdef SOFTUART1_BAUDRATE
		case DEV_SOFTUART1:
			return softUart1GetCh();
		#endif
		default:
			return -1;
		break;
	}
}

#ifdef LPUART1_TX_PIN
static inline void usartWriteLpuart1(const char *str, int msgLen)
{
	while (msgLen>0)
	{
		lpuart1Put(*str++);
	    msgLen--;
	}
}
#endif

static inline void usartWriteUart1(const char *str, int msgLen)
{
	while (msgLen>0)
	{
		usart1Put(*str++);
	    msgLen--;
	}
}

#ifdef USART2_TX_PIN
static inline void usartWriteUart2(const char *str, int msgLen)
{
	while (msgLen>0)
	{
		usart2Put(*str++);
	    msgLen--;
	}
}
#endif

#ifdef SOFTUART1_BAUDRATE
static inline void usartWriteSoftUart1(const char *str, int msgLen)
{
	while (msgLen>0)
	{
		softUart1PutCh(*str++);
	    msgLen--;
	}
}
#endif

void serialWrite(int usartNr, const char *str, int msgLen)
{
	switch(usartNr)
	{
		#ifdef LPUART1_TX_PIN
		case DEV_LPUART1:
			usartWriteLpuart1(str, msgLen);
			break;
		#endif
		case DEV_USART1:
			usartWriteUart1(str, msgLen);
			break;
		#ifdef USART2_TX_PIN
		case DEV_USART2:
			usartWriteUart2(str, msgLen);
			break;
		#endif
		#ifdef SOFTUART1_BAUDRATE
		case DEV_SOFTUART1:
			usartWriteSoftUart1(str, msgLen);
			break;
		#endif
		default:
			while (msgLen>0)
			{
			    serialPutChar(usartNr, *str++);
			    msgLen--;
			}
			break;
	}
}

void serialPrint(int usartNr, const char *str)
{
  while(*str)
  {
    serialPutChar(usartNr, *str++);
  }
}

#if 0
/* A utility function to reverse a string  */
static void my_reverse(char str[], int length)
{
    int start = 0;
    int end = length -1;
    while (start < end)
    {
    	// swap(*(str+start), *(str+end));
    	char tmp=*(str+start);
    	*(str+start)=*(str+end);
    	*(str+end)=tmp;

        start++;
        end--;
    }
}

static char* my_lltoa(int64_t num, char* str, int base)
{
	int64_t i = 0;
	char isNegative = 0;

	/* Handle 0 explicitly, otherwise empty string is printed for 0 */
	if (num == 0)
	{
		str[i++] = '0';
		str[i] = '\0';
		return str;
	}

	// In standard itoa(), negative numbers are handled only with
	// base 10. Otherwise numbers are considered unsigned.
	if (num < 0 && base == 10)
	{
		isNegative = 1;
		num = -num;
	}

	// Process individual digits
	while (num != 0)
	{
		int64_t rem = num % base;
		str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
		num = num/base;
	}

	// If number is negative, append '-'
	if (isNegative)
	str[i++] = '-';

	str[i] = '\0'; // Append string terminator

	// Reverse the string
	my_reverse(str, i);

	return str;
}
#endif

void serialPrintInt64(int usartNr, int64_t num)
{
	char str[64];
	misc_lltoa(num, str, 10);
	serialPrint(usartNr, str);
}
