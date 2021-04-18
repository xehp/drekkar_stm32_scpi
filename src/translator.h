/*
translator.h

Translate between internal units and SI units.

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.

History
2019-03-16 Created by Henrik Bjorkman

*/



// This returns true if ee data is avaliable so that the
// convert functions below can be used.
int8_t translatorConvertionAvailable();

int64_t translatorConvertFromAdcUnitsToMilliAmpsDc(int64_t currentInUnits);
uint32_t translatorConvertFromMilliAmpsToAdcUnitsDc(int32_t mA);
int64_t translatorConvertFromAdcUnitsToMilliVoltsDc(int64_t voltageInUnits);
int32_t translatorConvertFromMilliVoltsToAdcUnitsDc(int32_t v); // ADC units AKA raw voltage

int32_t translatorConvertFromExternalToMilliVoltsAc(int32_t externalVoltageInAdcUnits);

#ifdef INV_OUTPUT_AC_CURRENT_CHANNEL
int64_t translatorConvertFromAdcUnitsToMilliAmpsAc(int64_t currentInUnits);
#endif

int32_t linearInterpolationCToFreq(int32_t temp);
int32_t intervallHalvingFromFreqToC(int32_t freq);

int32_t measuring_convertToHz(uint32_t halfPeriod); // TODO rename

int32_t translateAdcToC(uint32_t adcSample);

#ifdef TEMP_INTERNAL_ADC_CHANNEL
int translateInternalTempToC(uint32_t adcSample);
#endif

