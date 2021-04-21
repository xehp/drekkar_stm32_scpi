/*
avr_eeprom.h

Handle stored parameters

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2017-06-04
Created.
Henrik Bjorkman

*/

#ifndef EEPROM_H_
#define EEPROM_H_


#include <stdint.h>


// This is used to tell if external voltage sensor it required or not.
// Sometimes the voltage value is multiplied by 10 so not using a little
// smaller value here than 0x7FFFFFFF.
#define NO_VOLT_VALUE -1
#define NO_VALUE -1



// A magic number so we can detect if expected format of stored eedata has been changed (don't use zero).
// Remember to increment this number if struct below is changed in a non backward compatible way.
#define EEDATA_MAGIC_NR 0x1145

// To simplify upgrade we support one legacy magic number.
// Comment this macro out if legacy feature is not needed.
#define EEDATA_LEGACY_MAGIC_NR 0x1144


// Bit positions within optionsBitMask
#define AUTOSTART_BIT 0
#define FAIL_SCAN_IF_TO_MUCH_CURRENT_BIT 1
#define EXT_TEMP_REQUIRED_BIT 2
#define EXT_VOLT_REQUIRED_BIT 3
#define LOOP_REQUIRED_BIT 4


#define tempMinFreq 40

// This shall be same as NDEV in web server file "data.h".
#define NDEV 4



// Add scan start and scan end frequency
// If adding members here add them also in PARAMETER_CODES in msg.h.
// And perhaps web server also need it in DataTargetStruct?
// For best efficiency pay attention to word alignment if adding or removing something here.
typedef struct
{
	uint16_t magicNumber;
	uint8_t optionsBitMask;                    // AUTOSTART_PAR, FAIL_SCAN_IF_TO_MUCH_CURRENT etc
	uint8_t superviceDeviationFactor;          // SUPERVICE_DEVIATION_FACTOR_PAR
	uint32_t microAmpsPerUnitDc;               // MICRO_AMPS_PER_UNIT_PAR
	uint32_t microVoltsPerUnitDc;              // MICRO_VOLTS_PER_UNIT_PAR
	uint32_t wantedInternalScanningVoltage_mV; // WANTED_SCANNING_VOLTAGE_MV
	uint32_t wantedCurrent_mA;                 // WANTED_CURRENT_PAR
	int32_t wantedVoltage_mV;                 // WANTED_VOLTAGE_PAR
	int64_t deviceId;                         // DEVICE_ID_PAR
	uint16_t voltageProbeFactor;               // VOLTAGE_PROBE_FACTOR
	uint16_t tempMaxFreq;                      // MAX_TEMP_HZ_PAR
	uint16_t freqAt20C;                        // TEMP_FREQ_AT_20C
	uint16_t freqAt100C;                       // TEMP_FREQ_AT_100C
	uint16_t scanBegin_Hz;                     // SCAN_BEGIN_HZ_PAR
	uint16_t scanEnd_Hz;                       // SCAN_END_HZ_PAR
	uint16_t portsPwmMinAngle;                 // PORTS_PWM_MIN_ANGLE_PAR
	uint16_t minCooldownTime_s;                // COOLDOWN_TIME_S_PAR
	uint32_t microAmpsPerUnitAc;	           // MICRO_AMPS_PER_UNIT_AC_PAR
	uint8_t maxExternalTemp_C;                 // MAX_EXT_TEMP_C
	uint8_t superviceDropFactor;               // SUPERVICE_DROP_FACTOR
	uint8_t lowDutyCyclePercent;               // LOW_DUTY_CYCLE_PERCENT
	uint8_t superviceIncFactor;                // SUPERVICE_INC_FACTOR_PAR
	uint16_t stableOperationTimer_s;           // SUPERVICE_STABILITY_TIME_S_PAR
	uint16_t scanDelay_ms;                     // SCAN_DELAY_MS_PAR
	uint16_t min_mV_per_mA_ac;                 // To detect unconnected multimeter.
	uint16_t spare0;
	uint16_t minQValue;                        // MIN_Q_VALUE
	uint16_t maxQValue;                        // MAX_Q_VALUE
	uint32_t webServerTimeout_s;               // How long machine will run without contact with web server
	int64_t wallTimeReceived_ms;	           // Latest wall time given from server and internally counted up.
	int64_t targetCycles[NDEV];                // cycles to per bath
	int64_t reachedCycles[NDEV];               // cycles performed per bath
	uint16_t superviceMargin_mV;
	uint8_t maxExtTempDiff_C;                  // MAX_EXT_TEMP_DIFF_C
	uint8_t spare;
	int32_t maxCurrentStep_ppb;
	int32_t maxVoltageStep_ppb;
	int16_t maxLeakCurrentStep_mA;
	int16_t spare2;
	int64_t spare3;
	int32_t saveCounter;
	uint32_t checkSum;
} EeDataStruct;


// data size in bytes
// In version 0x1144 this was: 27*4 or 13.5*8 -> 108 bytes
// In next it will need to be 128 bytes, so 20 more.


#ifdef EEDATA_LEGACY_MAGIC_NR
typedef struct
{
	uint16_t magicNumber;
	uint8_t optionsBitMask;                    // AUTOSTART_PAR, FAIL_SCAN_IF_TO_MUCH_CURRENT etc
	uint8_t superviceDeviationFactor;          // SUPERVICE_DEVIATION_FACTOR_PAR
	uint32_t microAmpsPerUnitDc;               // MICRO_AMPS_PER_UNIT_PAR
	uint32_t microVoltsPerUnitDc;              // MICRO_VOLTS_PER_UNIT_PAR
	uint32_t wantedInternalScanningVoltage_mV; // WANTED_SCANNING_VOLTAGE_MV
	uint32_t wantedCurrent_mA;                 // WANTED_CURRENT_PAR
	int32_t wantedVoltage_mV;                 // WANTED_VOLTAGE_PAR
	int64_t deviceId;                         // DEVICE_ID_PAR
	uint16_t voltageProbeFactor;               // VOLTAGE_PROBE_FACTOR
	uint16_t tempMaxFreq;                      // MAX_TEMP_HZ_PAR
	uint16_t freqAt20C;                        // TEMP_FREQ_AT_20C
	uint16_t freqAt100C;                       // TEMP_FREQ_AT_100C
	uint16_t scanBegin_Hz;                     // SCAN_BEGIN_HZ_PAR
	uint16_t scanEnd_Hz;                       // SCAN_END_HZ_PAR
	uint16_t portsPwmMinAngle;                 // PORTS_PWM_MIN_ANGLE_PAR
	uint16_t minCooldownTime_s;                // COOLDOWN_TIME_S_PAR
	uint32_t microAmpsPerUnitAc;			   // MICRO_AMPS_PER_UNIT_AC_PAR
	uint8_t maxExternalTemp_C;                 // MAX_EXT_TEMP_C
	uint8_t superviceDropFactor;               // SUPERVICE_DROP_FACTOR
	uint8_t lowDutyCyclePercent;               // LOW_DUTY_CYCLE_PERCENT
	uint8_t superviceIncFactor;                // SUPERVICE_INC_FACTOR_PAR
	uint16_t stableOperationTimer_s;           // SUPERVICE_STABILITY_TIME_S_PAR
	uint16_t scanDelay_ms;                     // SCAN_DELAY_MS_PAR
	uint32_t checkSum;
	uint16_t minQValue;                        // MIN_Q_VALUE
	uint16_t maxQValue;                        // MAX_Q_VALUE
	uint32_t webServerTimeout_s;               // How long machine will run without contact with web server
	int64_t wallTimeReceived_ms;	           // Latest wall time given from server and internally counted up.
	int64_t targetCycles[NDEV];                // cycles to per bath
	int64_t reachedCycles[NDEV];               // cycles performed per bath
	uint16_t superviceMargin_mV;
	uint8_t maxExtTempDiff_C;                   // MAX_EXT_TEMP_DIFF_C
	uint8_t spare;
	int32_t maxCurrentStep_ppb;
	int32_t maxVoltageStep_ppb;
} EeDataLegacyStruct;
#endif



extern EeDataStruct ee;


void eepromSave(void);
void eepromLoad(void);
void eepromSetDefault();
void eepromSaveIfPending();
void eepromSetSavePending();


#endif

