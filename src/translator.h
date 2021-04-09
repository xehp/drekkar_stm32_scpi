/*
translator.h

Translate between internal units and SI units.

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.

History
2019-03-16 Created by Henrik Bjorkman

*/

#include "cfg.h"


// This returns true if ee data is avaliable so that the
// convert functions below can be used.




int32_t measuring_convertToHz(uint32_t halfPeriod); // TODO rename

int32_t translateAdcToC(uint32_t adcSample);

#ifdef TEMP_INTERNAL_ADC_CHANNEL
int translateInternalTempToC(uint32_t adcSample);
#endif


#ifdef CURRENT_ADC_CHANNEL
int64_t translatorConvertFromAdcUnitsToMilliAmpsAc(int64_t currentInUnits);
#endif
