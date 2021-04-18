/*
Barebone blinky on nucleo L432KC / stm32l432kc using SysTick. No RTOS needed.
Copyright (C) Alexander & Henrik Bjorkman www.eit.se 2018
This may be used and redistributed as long as all copyright notices etc are retained .
http://www.eit.se/hb/misc/stm32/stm32l4xx/stm32_barebone/

References
[1] DocID027295 Rev 3, http://www.st.com/content/ccc/resource/technical/document/reference_manual/group0/b0/ac/3e/8f/6d/21/47/af/DM00151940/files/DM00151940.pdf/jcr:content/translations/en.DM00151940.pdf
[2] https://community.st.com/thread/51219-stm32l4-adc-module-using-the-ll#comments
[3] https://community.st.com/message/204166-re-stm32l4-adc-module-using-the-ll?commentID=204166#comment-204166
[4] http://www.micromouseonline.com/2009/05/26/simple-adc-use-on-the-stm32/
[5] http://homepage.cem.itesm.mx/carbajal/Microcontrollers/SLIDES/STM32F3%20ADC.pdf
[6] DS11451 Rev 4, https://www.st.com/resource/en/datasheet/stm32l432kb.pdf
*/


#include "stm32l4xx.h"
#include "stm32l432xx.h"
#include "stm32l4xx_nucleo_32.h"
#include "stm32l4xx_ll_adc.h"
#include "systemInit.h"
#include "timerDev.h"
#include "adcDev.h"
#include "serialDev.h"
#include "mathi.h"


#ifndef ASSERT
#define ASSERT(c) {if (!c) {systemErrorHandler(SYSTEM_APPLICATION_ASSERT_ERROR);}}
#endif

// TODO supposedly there is a better way: http://www.cplusplus.com/faq/sequences/arrays/sizeof-array/#cpp
#ifndef SIZEOF_ARRAY
#define SIZEOF_ARRAY( a ) (sizeof( a ) / sizeof( a[ 0 ] ))
#endif


#if (defined ADC_DIFFERENTIAL)
#if ((ADC_CHANNEL >= 16) || (ADC_CHANNEL <= 0))
#error // Differential input can't be used with all channels).
#endif 
#endif 


// More important signals can be listed more than once here.
const uint32_t adcChannelSequence[] = {
#ifdef MEASURE_COMP_AND_REF
		0, 6,
#endif
#ifdef INPUT_DC_VOLTAGE_CHANNEL
	INPUT_DC_VOLTAGE_CHANNEL,
#endif
#ifdef INTERNAL_DC_VOLTAGE_CHANNEL
	INTERNAL_DC_VOLTAGE_CHANNEL,
#endif
#ifdef INV_OUTPUT_AC_CURRENT_CHANNEL
	INV_OUTPUT_AC_CURRENT_CHANNEL,
#endif
#ifdef INTERNAL_DC_CURRENT_CHANNEL
	INTERNAL_DC_CURRENT_CHANNEL,
#endif
#ifdef INV_OUTPUT_AC_CURRENT_CHANNEL
	INV_OUTPUT_AC_CURRENT_CHANNEL,
#endif
};


volatile uint32_t adcSequenceCounter = 0;

// Results per channel
volatile uint32_t adcSamples[19] = {0};
volatile int adcCounter[19] = {0};
volatile uint32_t adcCurrentChannel = 0;

// ADC

//volatile uint16_t adc1Data = 0;

void __attribute__ ((interrupt, used)) ADC1_IRQHandler(void)
{
	// Are there any flags we must reset here?
	// This bit is set by hardware at the end of the conversions of a
	// regular sequence of channels. It is cleared by software writing 1 to it
	// So write 1 to make it zero?
	//ADC1_ClearITPendingBit(ADC1_IT_EOS);
	//ADC1->ISR = tmp | ADC_ISR_EOS_Msk;
	const uint32_t tmp = ADC1->ISR;
	ADC1->ISR = tmp | ADC_ISR_JQOVF_Msk | ADC_ISR_AWD3_Msk | ADC_ISR_AWD2_Msk | ADC_ISR_AWD1_Msk | ADC_ISR_JEOS_Msk | ADC_ISR_JEOC_Msk | ADC_ISR_OVR_Msk | ADC_ISR_EOS_Msk | ADC_ISR_EOC_Msk /*| ADC_ISR_EOSMP_Msk | ADC_ISR_ADRDY_Msk*/;

	// Bit 3 EOS: End of regular sequence flag
	// Only interrupt that is enabled so perhaps we do not need to check the flag?
	if (tmp & ADC_ISR_EOC_Msk)
	{
		adcSamples[adcCurrentChannel] = ADC1->DR;
		adcCounter[adcCurrentChannel]++;

		// Select next channel to sample.
		adcSequenceCounter++;
		if (adcSequenceCounter>=SIZEOF_ARRAY(adcChannelSequence))
		{
			adcSequenceCounter = 0;
		}
		adcCurrentChannel = adcChannelSequence[adcSequenceCounter];
		ADC1->SQR1 = (adcCurrentChannel << ADC_SQR1_SQ1_Pos);

		// Since we used non continuous mode start a new sequence.
		// We could program the ADC in continuous mode instead.
		// See ADC_CFGR_DISCEN_Msk below.
		if (tmp & ADC_ISR_EOS_Msk)
		{
			LL_ADC_REG_StartConversion(ADC1);
		}
	}
}

void adc1Init()
{
	// Enable the needed clocks
	RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN_Msk;
	//RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN_Msk;
	//RCC->APB2ENR |= RCC_APB2ENR_ADC1EN_Msk;
	systemSleepMs(2);

	// First set it to default but not in deep power down mode (in case this is not first time called).
	ADC1->CR = 0x00000000; // Not in deep power down. AKA ADC_CR_DEEPPWD_Msk
	ADC1->CFGR = 0x80000000;
	LL_ADC_DisableDeepPowerDown(ADC1); // [2]
	systemSleepMs(2);



	// [1] Before performing any operation such as launching a calibration or enabling the ADC, the
	// ADC voltage regulator must first be enabled and the software must wait for the regulator
	// start-up time.
	// ADVREGEN: ADC voltage regulator enable
	// ADCALDIF=0 is the reset value already.
	//ADC1->CR |= ADC_CR_ADVREGEN_Msk;
	//if ((ADC1->CR & ADC_CR_ADVREGEN_Msk) == 0)
	//{
	//  systemErrorHandler(4);
	//}
	LL_ADC_EnableInternalRegulator(ADC1);
	systemSleepMs(2);


	// Do we need to configure ADC_SMPR1 and ADC_SMPR2?
	// Ref [1] explains how to set it but nothing about what values should be used
	// or even why this setting exist. Perhaps ref [5] explain something?
	// So it seems to be something about input resistance and capacitance.
	// Perhaps if input signal is week it needs more time to charge the hold capacitor?
	// Then there is also [1] table 69 "DELAY bits versus ADC resolution".
	// Each channel have 3 bits in these registers.

	// This will put all channels on 6.5 ADC clock cycles instead of default 2.5.
	// This is fast but gives low accuracy
	//ADC1->SMPR1=0x9249249U;
	//ADC1->SMPR2=0x1249249U;

	// 010: 12.5 ADC clock cycles
	// This will put all channels on 12.5 ADC clock cycles instead of default 2.5.
	//ADC1->SMPR1=0x12492492U;
	//ADC1->SMPR2=0x2492492U;

	// 100: 47.5 ADC clock cycles
	ADC1->SMPR1=0x24924924U;
	ADC1->SMPR2=0x4924924U;

	// 101: 92.5 ADC clock cycles
	// With 470 Ohm impedance signal this was enough but not with a source of 10 kOhm.
	//ADC1->SMPR1=0x2db6db6dU;
	//ADC1->SMPR2=0x5b6db6dU;

	// 110: 247.5 ADC clock cycles
	// This gave acceptable result with 10 kOhm signal source
	//ADC1->SMPR1=0x36db6db6U;
	//ADC1->SMPR2=0x6db6db6U;

	// 111: 640.5 ADC clock cycles
	// This gave (no surprise) the best result using a weak signal source.
	// But sampling takes much time so we may need to speed up the stronger signal sources
	//ADC1->SMPR1=0x3FFFFFFFU;
	//ADC1->SMPR2=0x07FFFFFFU;

	// Giving channel 15 some extra time
	//ADC1->SMPR1=0x2db6db6dU;
	//ADC1->SMPR2=0x5b75b6dU;



	// According to ref [4] the ADC clock shall be in a certain range.
	// The code below gives 20 MHz (I would think)? That is a little above the range
	// (at least for STM32F). Perhaps something like this?
	// ADC1_COMMON->CCR |= (4 << ADC_CCR_PRESC_Pos);
	// But doing that did not seem change anything anyway.

	// 16.7.2 ADC common control register (ADC_CCR)
	// Using the system clock divided by 2 or 4 seems to work.
	// 10: HCLK/2 (Synchronous clock mode)
	// 11: HCLK/2 (Synchronous clock mode)
	const uint32_t CLOCK_MODE=3U;
	ADC1_COMMON->CCR |= (CLOCK_MODE << ADC_CCR_CKMODE_Pos);
	systemSleepMs(2);


	// Start calibration and wait for it to be done.
	/* [1]
	Software procedure to calibrate the ADC
	1.     Ensure DEEPPWD=0, ADVREGEN=1 and that ADC voltage regulator
		 startup time has elapsed.
	2.     Ensure that ADEN=0.
	3.     Select the input mode for this calibration by setting A
		 DCALDIF=0 (single-ended input) or ADCALDIF=1 (differential input).
	4.     Set ADCAL=1.
	5.     Wait until ADCAL=0.
	6.     The calibration factor can be read from ADC_CALFACT register
	*/
	/*ADC1->CR |= ADC_CR_ADCAL_Msk;
	while((ADC1->CR & ADC_CR_ADCAL_Msk) && (adc1Count<1000))
	{
	adc1Count++;
	systemSleepMs(10);
	}*/
	LL_ADC_StartCalibration(ADC1, LL_ADC_SINGLE_ENDED);
	while (LL_ADC_IsCalibrationOnGoing(ADC1))
	{
		systemSleep();
	}
	systemSleepMs(2);




	// [1] 16.6.4 ADC configuration register (ADC_CFGR)
	// Bit 16 DISCEN: Discontinuous mode for regular channels
	// Bit 5 ALIGN: Data alignment
	//   0: Right alignment
	//   1: Left alignment
#ifndef ADC_USE_OVERSAMPLING
	// Left align if not oversampling, this way the value is in same range as the oversampled one.
	// Not using oversampling is not much tested. We may get too much data to process if we do not use oversampling.
	ADC1->CFGR |= ADC_CFGR_DISCEN_Msk |  ADC_CFGR_ALIGN_Msk;
#else
	// Don't left align if oversampling, we should get a sum that use all 16 bits.
	ADC1->CFGR |= ADC_CFGR_DISCEN_Msk;
	ADC1->CFGR2 |= (3U << ADC_CFGR2_OVSR_Pos) | ADC_CFGR2_ROVSE_Msk;
#endif

#ifdef TEMP_INTERNAL_ADC_CHANNEL
#error // Need to enable something here, Not implemented yet
#endif



	// Select channel(s).
	// We will sample only one channel at a time, that is the reset setting.
	// So we do not need to change ADC_SQR1_L, it shall have the value 0
	// which is for 1 sample as reset setting.
	// We must also configure those pins as analog inputs? It should be the reset state.
	adcCurrentChannel = adcChannelSequence[adcSequenceCounter];
	ADC1->SQR1 = (adcCurrentChannel << ADC_SQR1_SQ1_Pos);
	systemSleepMs(2);


	// Set interrupt priority and enable. Lower number is higher priority.
	// We just want this to wake up the CPU from sleep.
	NVIC_SetPriority(ADC1_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL);
	NVIC_EnableIRQ(ADC1_IRQn);

	// Interrupt enable.
	// TODO Probably we need only one of these and then we can skip the if in the ISR.
	// If interrupts don't work, try enable all of them.
	//ADC1->IER = 0x7FF;
	ADC1->IER |= ADC_IER_EOSIE_Msk | ADC_IER_EOCIE_Msk;


	// Added this for debugging, can be removed later.
	if (ADC1->CR & ADC_CR_ADEN_Msk)
	{
		systemErrorHandler(STSTEM_ADC_DRIVER_ERROR);
	}


	/*
	[1] Software procedure to enable the ADC
	1. Clear the ADRDY bit in the ADC_ISR register by writing ‘1’.
	2. Set ADEN=1.
	3. Wait until ADRDY=1 (ADRDY is set after the ADC startup time).
	 This can be done using the associated interrupt (setting ADRDYIE=1).
	4. Clear the ADRDY bit in the ADC_ISR register by writing ‘1’ (optional).
	*/
	ADC1->ISR |= ADC_ISR_ADRDY_Msk;
	LL_ADC_Enable(ADC1);
	while ((ADC1->ISR & ADC_ISR_ADRDY_Msk) == 0)
	{
		systemSleep();
	}
	systemSleepMs(2);
	ADC1->ISR |= ADC_ISR_ADRDY_Msk;

	systemSleepMs(2);
	LL_ADC_REG_StartConversion(ADC1);
}

// Note, this means ADC is ready to start, not that its done a conversion and that the data from that is ready.
uint32_t adc1IsReady()
{
  return ADC1->ISR & ADC_ISR_ADRDY_Msk;
  //return !LL_ADC_REG_IsConversionOngoing(ADC1);
}




int adc1GetNOfSamples(uint32_t channel)
{
	ASSERT(channel < SIZEOF_ARRAY(adcCounter));
	return adcCounter[channel];
}

uint32_t adc1GetSample(uint32_t channel)
{
	ASSERT(channel < SIZEOF_ARRAY(adcSamples));
	return adcSamples[channel];
}
