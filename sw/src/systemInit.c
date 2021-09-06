/*
Barebone blinky on nucleo L432KC / stm32l432kc using SysTick. No RTOS needed.
Copyright (C) Alexander & Henrik Bjorkman www.eit.se 2018
This may be used and redistributed as long as all copyright notices etc are retained .
http://www.eit.se/hb/misc/stm32/stm32l4xx/stm32_barebone/

Read more:
https://www.st.com/content/ccc/resource/technical/document/application_note/2e/79/0c/5a/7d/70/4a/c7/DM00049931.pdf/files/DM00049931.pdf/jcr:content/translations/en.DM00049931.pdf


References
[1] DocID027295 Rev 3, https://www.st.com/content/ccc/resource/technical/document/reference_manual/group0/b0/ac/3e/8f/6d/21/47/af/DM00151940/files/DM00151940.pdf/jcr:content/translations/en.DM00151940.pdf
[6] DS11451 Rev 4, https://www.st.com/resource/en/datasheet/stm32l432kb.pdf

*/

#ifdef __arm__

#include "stm32l4xx.h"
#include "stm32l432xx.h"
#include "stm32l4xx_nucleo_32.h"
#include "stm32l4xx_ll_adc.h"
#include "stm32l4xx_ll_rcc.h"
#include "systemInit.h"




volatile int64_t SysTickCountMs = 0;
volatile int32_t sysDummy = 0;

// Simplest possible SysTick ISR (interrupt survice routine).
// TODO This "__attribute__ ((interrupt, used))" what does it do? 
// It seems to work also without it.
void __attribute__ ((interrupt, used)) SysTick_Handler(void)
//void SysTick_Handler(void)
{
  SysTickCountMs++;
}

#define VECT_TAB_OFFSET  0x00 /*!< Vector Table base offset field.
                                   This value must be a multiple of 0x200. */


int64_t systemGetSysTimeMs()
{
  for(;;)
  {
    int64_t tmp1 = SysTickCountMs;
    int64_t tmp2 = SysTickCountMs;
    if (tmp1 == tmp2)
    {
      return tmp1;
    }
  }
}

/**
This can be used if a delay is needed before the SysTick is started.
Once sys tick is running use systemSleepMs instead.
Very roughly delay is measured in uS.
*/
void systemBusyWait(uint32_t delay)
{
  while (delay > 0)
  { 
    sysDummy++; // Change a volatile, just to be sure the compiler does not optimize this out.
    delay--;
  }
}

// Setup the port/pin that is connected to the LED (Using the Green LED, it works).
// It is assumed that the clock for the port is enabled already.
void systemPinOutInit(GPIO_TypeDef* port, int pin)
{
  port->MODER &= ~(3U << (pin*2)); // Clear the 2 mode bits for this pin.
  port->MODER |= (1U << (pin*2)); // 1 for "General purpose output mode".
  port->OTYPER &= ~(0x1U << (pin*1)); // Clear output type, 0 for "Output push-pull".
  port->OSPEEDR &= ~(0x3U << (pin*2)); // Clear the 2 mode bits for this pin.
  port->OSPEEDR |= (0x1U << (pin*2)); // 1 for "Medium speed" or 2 for High speed.
  port->PUPDR &= ~(3U << (pin*2)); // Clear the 2 pull up/down bits. 0 for No pull-up, pull-down
}

void systemPinOutSetHigh(GPIO_TypeDef* port, int pin)
{
  port->ODR |= (0x1U << pin);
}

void systemPinOutSetLow(GPIO_TypeDef* port, int pin)
{
  port->ODR &= ~(0x1U << pin);
}


/**
 * Use SysTick as time base source and configure 1ms tick (default clock after Reset is MSI)
 * Configure the SysTick to have interrupt once every 1ms.
 */
static void sysTickInit()
{
  //const uint32_t sysTickPriority = 0xfU; // 0-255, Lower number for sysTickPriority is higher priority.
  //const uint32_t SubPriority = 0;

  /* set reload register */
  SysTick->LOAD  = (uint32_t)((SysClockFrequencyHz/1000) - 1UL); // May need some calibration.
  SysTick->VAL = 0UL;

  /* Enable SysTick IRQ and SysTick Timer */
  SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
                   SysTick_CTRL_TICKINT_Msk   |
                   SysTick_CTRL_ENABLE_Msk; 

  // Set SysTick priority. Do not know what NVIC_GetPriorityGrouping does. HAL did it that way.
  //uint32_t prioritygroup = NVIC_GetPriorityGrouping();
  //NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(prioritygroup, sysTickPriority, SubPriority));
  NVIC_SetPriority (SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); /* set Priority for Systick Interrupt */


  // It can be good with a tiny delay here for clocks to start up.
  systemBusyWait(1);
}


/* 
Set Interrupt Group Priority
ref [1] does not say anything about SCB->AIRCR.
It must be an ARM/Cortex thing and not an STM32 thing.
http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0553a/Cihehdge.html
*/
static void sysVectorInit()
{
  /* Configure the Vector Table location add offset address ------------------*/
#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#else
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
#endif


  uint32_t tmp =  SCB->AIRCR;
  const uint32_t VECTKEY = 0x5FAUL; // On writes, write 0x5FA to VECTKEY, otherwise the write is ignored.
  tmp &= ~((uint32_t)((0xFFFFUL << SCB_AIRCR_VECTKEY_Pos) | (7UL << SCB_AIRCR_PRIGROUP_Pos)));
  tmp |= ((uint32_t)VECTKEY << SCB_AIRCR_VECTKEY_Pos) |  (NVIC_PRIORITYGROUP_4 << SCB_AIRCR_PRIGROUP_Pos);
  SCB->AIRCR =  tmp;
}


// Ref [1] chapter 5.3.4 Sleep mode
void systemSleep()
{  
  /* Clear SLEEPDEEP bit of Cortex System Control Register */
  SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
  
  /* Request Wait For Interrupt */
  __WFI();
}


/**
 * Sleep given amount of milliseconds.
 */
void systemSleepMs(int32_t timeMs)
{
  const int32_t startTimeMs = systemGetSysTimeMs();
  for(;;)
  {
    systemSleep();
    const int32_t currentTimeMs = systemGetSysTimeMs();
    const int32_t timePassedMs = currentTimeMs - startTimeMs;
    if (timePassedMs>=timeMs)
    {
      break;
    }
  }
}

static int findFirstOne(int errorCode)
{
	for(int i=31; i>=0; i--)
	{
		if ((errorCode >> i) & 1)
		{
			return i;
		}
	}
	return -1;
}

static void flashLed(int base, int len)
{
#ifdef __arm__
	SYS_LED_ON();
	systemSleepMs(base*len);
	SYS_LED_OFF();
	systemSleepMs(base);
#else
	if (len == 1)
		printf(".");
	else
		printf("-");
#endif
}


/**
The error code will blink on the system debug LED.
The error code will be displayed in binary sort of morse code.
Leading zeroes are not flashed.
For example 8 (binary 1000) will be one one long and 3 short "-...", "daa di di di", morse code for 'B'.
Zero is one short
So positive numbers (>0) all begin with a long (the first bit flashed is a one).
Negative numbers will have a short first to tell that its a negative number.
 -14  .---  J
 -13  .--.  P
 -12  .-.-
 -11  .-..  L
 -10  ..--
  -9  ..-.  F
  -8  ...-  V
  -7  ....  H
  -6  .--   W
  -5  .-.   R
  -4  ..-   U
  -3  ...   S
  -2  .-    A
  -1  ..    I
   0  .     E
   1  -     T
   2  -.    N
   3  --    M
   4  -..   D
   5  -.-   K
   6  --.   G
   7  ---   O
   8  -...  B
   9  -..-  X
  10  -.-.  C
  11  -.--  Y
  12  --..  Z
  13  --.-  Q
  14  ---.
  15  ----
 */
void systemFlashCode(int code)
{
	const int beep = 200; // length of short beep in ms

	systemSleepMs(beep);

	if (code>0)
	{
		flashLed(beep, 3);
	}
	else
	{
		flashLed(beep, 1);
		if (code<0)
		{
			code=1-code;
		}
	}

	int n = findFirstOne(code);
	for(int i=n-1; i>=0; i--)
	{
		const uint32_t m = 1<<i;
		if ((code & m) == 0)
		{
			flashLed(beep, 1);
		}
		else
		{
			flashLed(beep, 3);
		}
	}
	systemSleepMs(beep);
}

/**
 * Call this if there is an assert error or such.
 * This code depends on system timer to be running.
 * If error happens before that use something else.
 */
void systemErrorHandler(int errorCode)
{
	// Stop PWM outputs etc here.
	// And/or make sure application calls its own error handler before this is called.
	TIM1->EGR |= TIM_EGR_BG_Msk;
	TIM2->EGR |= TIM_EGR_BG_Msk;
	SYS_LED_OFF();

	systemSleepMs(1000);

	// Do not turn system timer off, systemFlashCode uses it.

	// Disable external interrupts.
	NVIC->ICER[0]=0xFFFFFFFFUL;
	NVIC->ICER[1]=0xFFFFFFFFUL;
	NVIC->ICER[2]=0xFFFFFFFFUL;

	// Eternal loop
	for(;;)
	{
		systemFlashCode(errorCode);
	}
}


// TODO Would want to use the 8 MHz crystal and not the less accurate RC oscillator.
// https://community.st.com/s/question/0D50X00009XkgFp/usart-or-serial-trhough-stlinkv2-for-nucleo-stm32l152re
// The Nucleo boards don't have an external crystal, and the 8 MHz source from the ST-LINK must be made through the MCO (SB16 and SB50), which are normally not soldered.




static void sysClockStart()
{
  /* Disable all interrupts */
  RCC->CIER = 0x00000000U;


  /*
  AHB2 peripheral clock enable register (RCC_AHB2ENR)
  If other ports than GPIOA and GPIOB are to be used those also need 
  their clocks to be enabled like this.
  */
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN_Msk | RCC_AHB2ENR_GPIOAEN_Msk ;

  
  // Initialize the HSI clock
  // TODO This is probably not needed here.
  RCC->CR |= RCC_CR_HSION; // Enable HSI
  while (!(RCC->CR & RCC_CR_HSIRDY)); // Wait until HSI stable


#if SysClockFrequencyHz == 80000000U

  // Setup PLL so we can have 80 MHz

  // TODO:
  // This uses 16 HSI as base, it has 1% accuracy.
  // But we really want HSE from ST-Link.
  // If that does not work perhapås MSI can give 0.25% accuracy?

  // https://community.st.com/thread/41606-stm32-pll-clock-source-doesnt-work
  // Also note, that FLASH latency has to be set properly prior switching to a higher system clock, if running from FLASH.
  // [1] 3.3.3 Read access latency
  // [1] Bits 2:0 LATENCY[2:0]: Latency
  FLASH->ACR |= (4 << FLASH_ACR_LATENCY_Pos);


  /* HSEBYP, external clock (from ST-Link, not chrystal) */
  /* 
  [1] Bit 18 HSEBYP: HSE crystal oscillator bypass
  Set and cleared by software to bypass the oscillator with an external clock. The external 
  clock must be enabled with the HSEON bit set, to be used by the device. The HSEBYP bit 
  can be written only if the H
  SE oscillator is disabled. 
  0: HSE crystal oscillator not bypassed
  1: HSE crystal oscillator by
  passed with external clock
  */
  //RCC->CR &= ~(RCC_CR_HSEBYP_Msk);
  //RCC->CR |= RCC_CR_HSEBYP_Msk;
  // I have not gotten it to work with the external (HSE) clock.


  // First turn PLL off
  // And other clocks on.
  RCC->CR &= ~RCC_CR_PLLON_Msk; 
  RCC->CR |= RCC_CR_HSEON_Msk | RCC_CR_HSION_Msk | RCC_CR_MSION_Msk;
  //LL_RCC_HSI_Enable();


  // Set the RCC_PLLCFGR
  {
    uint32_t tmpRCC_PLLCFGR = 0x00001000U; // (RCC_PLLCFGR register reset settings

    //uint32_t tmp = RCC->PLLCFGR;
    //tmpRCC_PLLCFGR &= ~(RCC_PLLCFGR_PLLREN_Msk | (0x7f << RCC_PLLCFGR_PLLN_Pos) | (3 <<RCC_PLLCFGR_PLLSRC_Pos));

    // TODO, Is this one needed? Or instead are more of these needed?
    // [1] Bit 24 PLLREN: Main PLL PLLCLK output enable
    tmpRCC_PLLCFGR |= RCC_PLLCFGR_PLLREN_Msk;

    const uint32_t wantedPLLN = 20U;
    tmpRCC_PLLCFGR |= wantedPLLN << RCC_PLLCFGR_PLLN_Pos;
    // Probably I want 10x PLL with External HSE 8 MHz.

    // Divide by 2
    const uint32_t wantedPLLM = 1;
    tmpRCC_PLLCFGR |= wantedPLLM << RCC_PLLCFGR_PLLM_Pos;

    /*
    [1] 6.4.4 PLL configuration register (RCC_PLLCFGR)
    Bits 1:0 PLLSRC: Main PLL and PLLSAI1 entry clock source
    Set and cleared by software to select PLL and PLLSAI1 clock source. These bits can be 
    written only when PLL and PLLSAI1 are disabled.
    00: No clock sent to PLL and PLLSAI1
    01: MSI clock selected as PLL and PLLSAI1 clock entry
    10: HSI16 clock selected as PLL and PLLSAI1 clock entry
    11: HSE clock selected as PLL and PLLSAI1 clock entry
    */
    const uint32_t wantedPllSrc = 0x2U;
    tmpRCC_PLLCFGR |= (wantedPllSrc << RCC_PLLCFGR_PLLSRC_Pos);

    // Using 16 MHz HSI, x20 /2 the system frequency should be:
    // 16 * 20 / 2 MHz = 160 MHz
    // But it seems I get the wanted 80 MHz with the setting.    
    // Did I miss some additional divide by 2 somewhere?

    RCC->PLLCFGR = tmpRCC_PLLCFGR;
  }


  // Turn PLL on
  RCC->CR |= RCC_CR_PLLON;

  systemBusyWait(10000);

  // Set system clock source to PLL.
  {
    uint32_t tmpRCC_CFGR = 0;

    // PLL selected as system clock
    const uint32_t wantedClock = RCC_CFGR_SW_PLL;
    tmpRCC_CFGR &= ~(3 << RCC_CFGR_SW_Pos);
    tmpRCC_CFGR |= (wantedClock << RCC_CFGR_SW_Pos); 
    RCC->CFGR = tmpRCC_CFGR; 

    // Wait for system clock to be on.
    //while (((RCC->CFGR & (3 << RCC_CFGR_SWS_Pos)) != (wantedClock << RCC_CFGR_SW_Pos) ) {};
  }


  systemBusyWait(10000);


    // Disable unused clocks.
    // This might have no effect on clocks still used?
    //RCC->CR &= ~(RCC_CR_HSION_Msk | RCC_CR_MSION_Msk);
    RCC->CR &= ~(RCC_CR_MSION_Msk | RCC_CR_HSEON_Msk);

#elif SysClockFrequencyHz == 16000000U
    // Use HSI, 16 MHz, no PLL

  LL_RCC_HSI_Enable();
  while (LL_RCC_HSI_IsReady() != 1) {/* Wait for HSI ready */}

  /* Set HSI as SYSCLCK source */
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
  //LL_RCC_SetAHBPrescaler(RCC_CFGR_HPRE_DIV1);
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {}

  LL_RCC_MSI_Disable();

#elif SysClockFrequencyHz == 4000000U

  LL_RCC_MSI_Enable();
  while (LL_RCC_MSI_IsReady() != 1) {/* Wait for MSI ready */}

  /* Set HSI as SYSCLCK source */
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);
  //LL_RCC_SetAHBPrescaler(RCC_CFGR_HPRE_DIV1);
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSI) {}

  LL_RCC_HSI_Disable();

#else
#error // Not implemented
#endif

  // It seems to be good with a tiny delay here and there for clocks etc to start up.
  systemBusyWait(5);
}




/**
This is called from "startup_stm32l432xx.s" before main is called.
*/
void SystemInit(void)
{
  /* FPU settings */
  // TODO perhaps move this to sysVectorInit?
  #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
  #endif

  sysClockStart();

  sysVectorInit();

  // Initialize the system debug LED
  systemPinOutInit(SYS_LED_PORT, SYS_LED_PIN);

  sysTickInit();


  // The following shall be commented out when code works.
  // If something is not working? Try enable everything.
  // Doing this or not should not be needed so comment this out normally.
  /*RCC->AHB1ENR |= 0x00011103U;
  RCC->AHB2ENR |= 0x0005209FU;
  RCC->AHB3ENR |= 0x00000100U;
  RCC->APB1ENR1 |= 0xF7EEC633U;
  RCC->APB1ENR2 |= 0x00000003U;
  RCC->APB2ENR |= 0x01235C01U;*/
}

uint32_t HAL_GetTick()
{
  return systemGetSysTimeMs();
}

#endif
