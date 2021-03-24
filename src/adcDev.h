/*
Barebone blinky on nucleo L432KC / stm32l432kc using SysTick. No RTOS needed.
Copyright (C) Alexander & Henrik Bjorkman www.eit.se 2018
This may be used and redistributed as long as all copyright notices etc are retained .
http://www.eit.se/hb/misc/stm32/stm32l4xx/stm32_barebone/

[1] DocID027295 Rev 3, http://www.st.com/content/ccc/resource/technical/document/reference_manual/group0/b0/ac/3e/8f/6d/21/47/af/DM00151940/files/DM00151940.pdf/jcr:content/translations/en.DM00151940.pdf
[6] DS11451 Rev 4, https://www.st.com/resource/en/datasheet/stm32l432kb.pdf
*/

#ifndef ADCDEV_H
#define ADCDEV_H

#include <stdint.h>
#include <ctype.h>
#include "cfg.h"




// The ADC values are expected to be between 0 and this.
// The average value of AC signals are expected to be half of this.
// The ADC is 12 bit but with 16 times oversampling the sum is 16 bits.
// If not using oversampling the result is instead left aligned.
#define ADC_RANGE 0x10000LU
#define ADC_MAX 0xFF00LU



// Take the average of some samples (typically 16 samples).
#define ADC_USE_OVERSAMPLING



void adc1Init();


int adc1GetNOfSamples(uint32_t channel);
uint32_t adc1GetSample(uint32_t channel);



#endif
