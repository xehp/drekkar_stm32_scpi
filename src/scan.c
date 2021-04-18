/*
scan.c

provide functions to set up hardware

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2016-09-25
Created.
Henrik Bjorkman
*/

#include "cfg.h"



#if defined __arm__
#include "timerDev.h"
#include "systemInit.h"
#elif defined __linux__
#include <stdio.h>
#endif

#include "mathi.h"
#include "eeprom.h"
#include "adcDev.h"
#include "portsPwm.h"
#include "translator.h"
#include "measuringFilter.h"
#include "mainSeconds.h"
//#include "measuring.h"
#include "log.h"
#include "relay.h"
#include "fan.h"
#include "temp.h"
#include "externalSensor.h"
#include "externalTemp.h"
#include "supervise.h"
#include "scan.h"

// TODO Use the quota between current and internal voltage instead of just current.
// That way it will not matter if REG is a bit slow to react when current goes up or down.


// On ATMEGA CPU frequency is 20 MHz
// On Nucleo CPU frequency is 80 MHz
//
// Frequency is
// f = 20MHz/PeriodTime
// Or
// f = 10MHz/(HalfPeriod)
// To get an halfPeriod for a given frequency do:
// HalfPeriod = 10000000/f
//
// 0x1986 gives about 3.07 kHz
// 0x8000 gives about 305 Hz, don't need less than that. We should keep above 400 Hz if we use a 400 Hz transformer
// 0x0800 gives about 4.7 kHz, Do not need higher than that for now.
// 0x0700 gives about 6.3 kHz
//
// On Nucleo we typically have 80 MHz
// So frequency = SysClockFrequencyHz / (2*halfPeriod)
// Or halfPeriod = SysClockFrequencyHz / (2*frequency)




#define MEASURING_VS_WANTED_MARGIN_MV 1000

// Want it to take about 1 minute to increase or decrease 600V
#define VOLTAGE_INCREASE_MV_PER_MS 10
#define VOLTAGE_DECREASE_MV_PER_MS 10


// Use 0 or 1 here, 0 if tune only little, 1 if tune a little more (until current has dropped an extra unit)
#define TUNE_EXTRA 1


// How fast we do prescan, use higher value here for slower prescan.
#define PRE_SCAN_DELAY (ee.scanDelay_ms)

// How fast we examine the resonance peak, use higher value here for more accurate but slower scan.
#define SCAN_BOUNDRY_DELAY (ee.scanDelay_ms*2)

// How fast we do fine tuning, use higher value here for more accurate but slower scan.
#define FINE_TUNING_DELAY (ee.scanDelay_ms*4)

// How much current increase during fine tuning is needed to say it was correct direction.
#define STOP_MARGIN_MA 1

// Use this if some fine tuning shall be done after scanning, comment out if not needed.
// TODO This did not help so will remove support for this.
//#define NEEDED_TUNING_SAME_COUNT 2


// How often shall scan try to tune the frequency
//#define TUNE_DELAY_IN_TICKS 10000	// This was when tmr 0 was used, Time is probably scanCount / AVR_TMR0_TICKS_PER_SEC, so 5000 gives about one minute
//#ifdef NEEDED_TUNING_SAME_COUNT
//#define TUNE_DELAY_MS ((tuning_same_count==0)?(60*1000):(15*1000))
//#else
#define TUNE_DELAY_MS (60*1000)
//#endif


// Dead zone or tolerance zone, this gives an interval within which regulation is not activated.
// May be usefull or not. Set to zero to deactivate.
#define DEADZONE 2

// How much higher the resonance current shall be compared to mean current for all
// frequencies to be accepted
#define RESONANCE_FOUND_FACTOR 2

// If Q exceed this value its to good to be true (or to bad to use).
#define MAX_Q ee.maxQValue
#define MIN_Q ee.minQValue

// When inverter duty cycle is off the current goes via diodes to opposite rail instead.
// In effect "brakes" the current. If if brakes more than "thrusting" it will not run well.
#define LOWEST_USABLE_DUTY_CYCLE_PPB (ee.lowDutyCyclePercent * 10000000)


#define LOG_DEBUG_INTERVALL_MS 10000

#define HARMLESS_DC_VOLTAGE_MV 36000

const int minimumMeasurableCurrent_mA = 100;

// scanState
enum
{
	scanInit,
	scanIsReady,
	scanClearOverCurrentDLatch,
	scanDelayBeforePowerOn,
	scanWaitForVoltage,
	scanRampUpDutyCycle,
	scanFindThePreliminaryResonance,
	scanGotoPreliminaryResonance,
	scanDelayBeforeThoroughScan,
	scanFindUpperBoundry,
	//scanGotoPreliminaryAgain,
	scanFindLowerBoundry,
	scanGotoResonanceBeforeNormalOperation,
	scanNormalOperation,
	scanDelayBeforeTuning,
	scanMeasureBeforeTuning,
	scanTestingHigherFreq,
	scanTestingLowerFreq,  // Frequency down == period longer
	scanPowerDown,
	scanDischarge,
	scanPause,
	scanDone,
	scanError,
};



char scan_state = scanInit; // SCAN_STATE_PAR
int32_t scanCount = 0;

//uint32_t scanHalfPeriod = MAX_HALF_PERIOD;

#ifndef DONT_SCAN_FOR_RESONANCE

static uint32_t scanResonanceHalfPeriod = 0; // RESONANCE_HALF_PERIOD

static uint32_t scanTopPreliminaryCurrentFound_mA = 0;
static uint32_t scanTopPreliminaryHalfPeriodFound = 0;

static int32_t scanTopCurrentFoundUp = 0;
static int32_t scanTopCurrentFoundDown = 0;

static uint32_t scanTopHalfPeriodFoundUp = 0;
static uint32_t scanTopHalfPeriodFoundDown = 0;

static uint32_t scanUpperBoundryHalfPeriod = 0;
static uint32_t scanLowerBoundryHalfPeriod = 0;

uint32_t scanPeakWidthInPeriodTime = 0;  // SCAN_PEAK_WIDTH_IN_PERIOD_TIME_PAR
uint32_t scanQ = 0; // SCAN_Q_PAR

static uint64_t scanCurrentSum = 0;
static uint32_t scanSumNSamples = 0;

int64_t scanTuningStartCurrent=0;
int64_t scanTuningAlternativeCurrent=0;


//static int64_t timeOfLastWantedStatusMsgSent = 0;

#else
uint32_t scanResonanceHalfPeriod = START_HALF_PERIOD;

#endif

int64_t lastLogTime_ms = 0;

//#ifdef NEEDED_TUNING_SAME_COUNT
//int tuning_same_count = NEEDED_TUNING_SAME_COUNT;
//#endif

int nextTryUp=0;



// Scan will regulate voltage and current by adjusting duty cycle of the "500Hz" signal.
static uint32_t scanDutyCyclePpb = 0; // PPB (Parts per billion)


uint32_t previousMeasuringVoltage_mV = 0;


/*
void print_dec_number(const char* msg, int32_t n, char* unit)
{
	char str[32];
	hex_itoa(n, str, 10);

	uart_print_P(PSTR(LOG_PREFIX));
	uart_print(msg);
	uart_putchar(' ');
	uart_print(str);
	uart_putchar(' ');
	uart_print(unit);
	uart_print_P(PSTR(LOG_SUFIX));
}
*/


/*
void print_freq_from_half_period(const char* msg, uint32_t half_period)
{
	const int32_t f = DIV_ROUND(TMR_TICKS_PER_SEC,(half_period*2));
	print_dec_number(msg, f, "hz");
}
*/

static int32_t freq_from_half_period(uint32_t half_period)
{
	return DIV_ROUND(SysClockFrequencyHz,(half_period*2));
}



/*
char scanIsDone()
{
#ifdef DONT_SCAN_FOR_RESONANCE
	return 1;
#else
	return scan_state==scanDone;
#endif
}
*/

#ifndef DONT_SCAN_FOR_RESONANCE

// How much to change the period.
static uint32_t calcChangeStep(uint32_t scanHalfPeriod)
{
	//const uint32_t change=1+DIV_ROUND_DOWN(scanHalfPeriod,(MIN_HALF_PERIOD));
	//return change;
	return 1;
}

// When period is big (low frequency we need a shorter delay since it would take long time otherwise.
static uint32_t calcChangeDelay(uint32_t scanHalfPeriod, uint32_t fullDelay)
{
	return DIV_ROUND_UP(fullDelay*MIN_HALF_PERIOD, scanHalfPeriod);
}


/*static uint32_t calcChangeTarget(uint32_t scanHalfPeriod, int16_t speedFactor, uint32_t targetHalfPerion)
{
	const uint32_t d=targetHalfPerion-scanHalfPeriod;
	const uint32_t c=1+DIV_ROUND_DOWN(d,speedFactor);
	return c;
}*/


static int calculateScanningRange()
{
	return MY_ABS((int)ee.scanEnd_Hz - (int)ee.scanBegin_Hz);
}

static int calculateScanningCenterFrequency()
{
	return ((int)ee.scanEnd_Hz + (int)ee.scanBegin_Hz + 1)/2;
}

static void setState(const char newState)
{
	if (scan_state != newState)
	{
		logInt3(SCAN_CHANGE_STATE, scan_state, newState);
		scan_state=newState;
	}
}

#endif

static int32_t getInternalScanningVoltage_mV()
{
	return ee.wantedInternalScanningVoltage_mV;
}


static void logWaiting1(int msg)
{
	const int64_t sysTime = systemGetSysTimeMs();
	const int64_t timeSinceLastLog = sysTime - lastLogTime_ms;
	if (timeSinceLastLog > (LOG_DEBUG_INTERVALL_MS))
	{
		logInt1(msg);
		lastLogTime_ms = sysTime;
	}
}


static void logWaiting2(int msg, int64_t parameter)
{
	const int64_t sysTime = systemGetSysTimeMs();
	const int64_t timeSinceLastLog = sysTime - lastLogTime_ms;
	if (timeSinceLastLog > (LOG_DEBUG_INTERVALL_MS))
	{
		logInt2(msg, parameter);
		lastLogTime_ms = sysTime;
	}
}

static uint32_t usableAngleRange()
{
	const uint32_t p = ports_get_period();
	const uint32_t m = 2 * PORTS_MIN_ANGLE;
	if (p < m)
	{
		return 0;
	}
	return p-m;
}



// Duty cycle is given in ppm but ports is working with steps
// out of a given period time, given in steps of SysClockFrequencyHz,
// typically 1/80000000 seconds. Since the period time is not fixed
// the duty cycle steps also vary.
// This gives the resolution of the duty cycle in ppm.
/*static uint32_t getDutyCycleStepsPpm()
{
	const uint32_t p = ports_get_period();
	const uint32_t d = DIV_ROUND_UP(1000000, p);
	return d;
}*/


// Calculate angle according to wanted duty cycle in PPM.
// NOTE that its not a direct translation, the usable angle range is considered.
// See also scanGetDutyCyclePpm.
static uint32_t calculateAngleFromDutyCyclePpb(const uint32_t dutyCyclePpb)
{
	const uint64_t ua = usableAngleRange();
	const uint64_t dc = dutyCyclePpb;
	const uint64_t revAngle = (ua * dc) / 1000000000LL;
	return PORTS_MIN_ANGLE + ua - revAngle;
}


// Calculate and set angle according to wanted duty cycle in PPM.
static void setInverterAngleAccordingToDutyCyclePpb()
{
	if (scanDutyCyclePpb)
	{
		const uint32_t a = calculateAngleFromDutyCyclePpb(scanDutyCyclePpb);
		portsSetAngle(a);
	}
	else
	{
		portsSetDutyOff();
	}
}

// Will also set ports angle (ports runs the inverter).
// Low duty cycle -> high angel and vice versa.
static void setInverterDutyCyclePpb(uint32_t dutyCycle)
{
	scanDutyCyclePpb = dutyCycle;
	setInverterAngleAccordingToDutyCyclePpb();
}

// Increment duty cycle (without passing 1000000).
// Decrement angle.
static void incInverterDutyCyclePpb(uint32_t deltaDutyCycle)
{
	if ((scanDutyCyclePpb+deltaDutyCycle)<1000000000)
	{
		setInverterDutyCyclePpb(scanDutyCyclePpb + deltaDutyCycle);
	}
	else
	{
		setInverterDutyCyclePpb(1000000000);
	}
}

// Decrement duty cycle (without passing 0).
// Increment angle.
static void decInverterDutyCyclePpb(uint32_t deltaDutyCycle)
{
	if (scanDutyCyclePpb>deltaDutyCycle)
	{
		setInverterDutyCyclePpb(scanDutyCyclePpb - deltaDutyCycle);
	}
	else
	{
		setInverterDutyCyclePpb(0);
	}
}

// Half period is what defines wanted output frequency of inverter.
// Unit for half period depends on SysClockFrequencyHz so typically
// 1/80000000 seconds. A full period is 2*halfPeriod.
// If period is changed the angle will also need to be adjusted
// to keep duty cycle the same.
static void setInverterHalfPeriod(uint32_t newHalfPeriod)
{
	const uint32_t currentHalfPeriod = ports_get_period();

	if (newHalfPeriod == currentHalfPeriod)
	{
		// same, no change
	}
	else if (newHalfPeriod < currentHalfPeriod)
	{
		// If period is reduced then adjust angle to it first.
		// This avoids missing the begin/end of a pulls in case new period time is less than active time.
		// At least that is the intention.
		setInverterAngleAccordingToDutyCyclePpb();
		portsSetHalfPeriod(newHalfPeriod);
	}
	else
	{
		// If period is increased then adjust angle to it after changing.
		portsSetHalfPeriod(newHalfPeriod);
		setInverterAngleAccordingToDutyCyclePpb();
	}
}


/*static void setWantedCurrentZero()
{
	//setInverterDutyCyclePpm(0);
}*/


/*static void setWantedCurrent_mA(int32_t wantedCurrent_mA)
{
	// Nothing
}*/


static void checkResonanceHalfPeriod()
{
	if (scanResonanceHalfPeriod)
	{
		if (scanResonanceHalfPeriod > MAX_HALF_PERIOD)
		{
			logInt4(SCAN_PARAMETER_OUT_OF_RANGE, scanResonanceHalfPeriod, MIN_HALF_PERIOD, MAX_HALF_PERIOD);
			scanResonanceHalfPeriod = MAX_HALF_PERIOD;
			errorReportError(portsErrorResonanceFrequencyToLow);
		}

		if (scanResonanceHalfPeriod < MIN_HALF_PERIOD)
		{
			logInt4(SCAN_PARAMETER_OUT_OF_RANGE, scanResonanceHalfPeriod, MIN_HALF_PERIOD, MAX_HALF_PERIOD);
			scanResonanceHalfPeriod = MIN_HALF_PERIOD;
			errorReportError(portsErrorResonanceFrequencyToHigh);
		}

		if ((ee.scanBegin_Hz == ee.scanEnd_Hz) && (scanPeakWidthInPeriodTime!=0))
		{
			errorReportError(portsInternalError);
		}
	}
}


static void enterStateIdle()
{
	scanCount = 1000;
	scanResonanceHalfPeriod = 0;
	scanTopPreliminaryCurrentFound_mA = 0;
	scanTopPreliminaryHalfPeriodFound = 0;
	scanTopCurrentFoundUp = 0;
	scanTopCurrentFoundDown = 0;
	scanTopHalfPeriodFoundUp = 0;
	scanTopHalfPeriodFoundDown = 0;
	scanUpperBoundryHalfPeriod = 0;
	scanLowerBoundryHalfPeriod = 0;
	scanPeakWidthInPeriodTime = 0;
	scanQ = 0;
	scanCurrentSum = 0;
	scanSumNSamples = 0;
	scanTuningStartCurrent=0;
	//regDisablePiRegulation();
	//regPowerDownCommand();
	//wantedBuckVoltage_mV = BUCK_LOW_VOLTAGE_MV;
	//wantedBuckCurrent_mA = BUCK_MEDIUM_CURRENT_MA;
	//#ifdef NEEDED_TUNING_SAME_COUNT
	//tuning_same_count = NEEDED_TUNING_SAME_COUNT+1;
	//#endif


	setInverterDutyCyclePpb(0);
	relayOff();
	setSuperviseInternalVoltage(0);
	portsSetDutyOff();
	setInverterHalfPeriod(START_HALF_PERIOD);
	setState(scanInit);
}


void enterStateReady()
{
	setState(scanIsReady);
}

static void enterStateClearOverCurrentDLatch()
{
	scanCount=10;

	setInverterDutyCyclePpb(0);

	portsDeactivateBreak();

	setState(scanClearOverCurrentDLatch);
}

static void enterStateDelayBeforePowerOn()
{
	logInt3(SCAN_START, START_HALF_PERIOD, STOP_HALF_PERIOD);

	//const uint32_t scanHalfPeriod = ports_get_period();
	const uint32_t scanHalfPeriod = START_HALF_PERIOD;
	scanCount=SCAN_BOUNDRY_DELAY*10;

	portsSetDutyOff();
	setInverterHalfPeriod(scanHalfPeriod);

	// To avoid current rush thru the transformer we start (due it possibly
	// being magnetized already) with the biggest angle we can (smallest
	// duty cycle) then increase it quite quickly.
	setInverterDutyCyclePpb(0);

	//portsSetFullDutyCycle();

	//setWantedCurrentZero();

	setState(scanDelayBeforePowerOn);
}

void enterClearOrDelay()
{
	if (portsIsBreakActive() || portsGetPwmBrakeState() != 0) // breakStateIdle
	{
		enterStateClearOverCurrentDLatch();
	}
	else
	{
		logInt1(SCAN_TRIPPED_ALREADY_CLEAR);
		enterStateDelayBeforePowerOn();
	}
}

/*
static void logMissingStartCondition()
{
	const uint32_t wantedVoltage = getWantedVoltage();
	//const uint32_t measuredVoltage = getMeasuredVoltage_rawUnits();
	const uint32_t wantedCurrent = getWantedCurrent();
	//const int32_t measuredCurrent = adc1GetSample(CURRENT_OUT_CHANNEL);;

	if (!translatorConvertionAvailable())
	{
		logInt1(SCAN_NEED_A_SAVED_CONFIGURATION);
	}
	else if (!tempOk())
	{
		logInt3(SCAN_TEMP_NOT_READY_YET, fanAndTempGetFanMeasurement_Hz(), fanAndTempGetTempMeasurement_Hz());
	}
	else if (!fansAndTempOK())
	{
		logInt3(SCAN_FANS_NOT_READY_YET, fanAndTempGetFanMeasurement_Hz(), fanAndTempGetTempMeasurement_Hz());
	}
	else if (!(getMeasuredVoltageIsAvailableOrNotRequired()))
	{
		logInt1(SCAN_VOLTAGE_SENSOR_NOT_READY_YET);
	}
	else if (regReadyToPowerUp())
	{
		logInt1(SCAN_WAITING_FOR_REG);
	}
	else if (machineGetRequestedState() != cmdRunCommand)
	{
		logInt1(SCAN_WAITING_FOR_START_COMMAND);
	}
	else if (wantedVoltage==0)
	{
		logInt1(SCAN_WANTED_VOLTAGE_IS_ZERO);
	}
	else if (wantedCurrent==0)
	{
		logInt1(SCAN_WANTED_CURRENT_IS_ZERO);
	}
	else if (errorGetReportedError() != portsErrorOk)
	{
		logInt2(PORTS_ERROR, errorGetReportedError());
	}
	else
	{
		logInt1(SCAN_WAITING_FOR_SOMETHING_ELSE);
	}
}
*/


static void checkPeriodThenEnterStateScanRegUsingBuck(void)
{
	//regSetWantedVoltage_mV(getMaxWantedInternalRunningVoltage_mV());
	scanCount=PRE_SCAN_DELAY;

	setState(scanGotoResonanceBeforeNormalOperation);
}


static void enterStateWaitForVoltage()
{
	relay1On();
	scanCount = 100;
	setState(scanWaitForVoltage);
}

static void enterRampUpDutyCycle()
{
	scanCount = 100;
	relay1Off2On();
	setState(scanRampUpDutyCycle);
}

static void enterStateScanNormalOperation()
{
	scanCount=100;
	setState(scanNormalOperation);
}

static void enterScanningOrRegulation()
{
	const int diff = calculateScanningRange();
	const int medium = calculateScanningCenterFrequency();

	setSuperviseInternalVoltage(1);
	scanCount=1000;

	if (scanResonanceHalfPeriod != 0)
	{
		// Already have a resonance frequency so use that
		logInt6(SCAN_ALREADY_HAVE_RESONANCE_FREQUENCY, TRANSLATE_HALF_PERIOD_TO_FREQUENCY(scanResonanceHalfPeriod), ee.scanBegin_Hz, ee.scanEnd_Hz, medium, diff);
		setInverterHalfPeriod(scanResonanceHalfPeriod);
		checkPeriodThenEnterStateScanRegUsingBuck();
	}
	else if (diff*25 <= medium)
	{
		// Can't scan for resonance if this interval is too small.
		logInt5(SCAN_TOO_SMALL_INTERVALL_FOR_SCANNING, ee.scanBegin_Hz, ee.scanEnd_Hz, medium, diff);
		scanResonanceHalfPeriod = TRANSLATE_FREQUENCY_TO_HALF_PERIOD(medium);
		setInverterHalfPeriod(scanResonanceHalfPeriod);
		checkPeriodThenEnterStateScanRegUsingBuck();
	}
	else
	{
		logInt5(SCAN_START_SCANNING_RESONANCE_FREQUENCY, ee.scanBegin_Hz, ee.scanEnd_Hz, medium, diff);
		scanTopPreliminaryCurrentFound_mA = 0;
		setState(scanFindThePreliminaryResonance);
	}
}

static void enterStateDelayBeforeTuning()
{
	scanCount=5000;
	scanTuningStartCurrent=0;
	scanTuningAlternativeCurrent=0;
	setState(scanDelayBeforeTuning);
}

static void enterStateMeasureBeforeTuning()
{
	scanCount=TUNE_DELAY_MS;
	scanTuningStartCurrent=0;
	scanTuningAlternativeCurrent=0;
	setState(scanMeasureBeforeTuning);
}

static void enterStateTestingHigherFrequency()
{
	const uint32_t scanHalfPeriod = ports_get_period();
	setInverterHalfPeriod(scanHalfPeriod-1);
	scanCount=TUNE_DELAY_MS;
	scanTuningAlternativeCurrent=0;
	setState(scanTestingHigherFreq);
}

static void enterStateTestingLowerFrequency()
{
	const uint32_t scanHalfPeriod = ports_get_period();
	setInverterHalfPeriod(scanHalfPeriod+1);
	scanCount=TUNE_DELAY_MS;
	scanTuningAlternativeCurrent=0;
	setState(scanTestingLowerFreq);
}


static void logTuningResult(AVR_CFG_LOG_MESSAGES msgCode, const uint32_t halfPeriod, int64_t scanTuningStartCurrent, int64_t scanTuningAlternativeCurrent)
{
	const int32_t f_hz = freq_from_half_period(halfPeriod);
	const uint32_t scanHalfPeriod = ports_get_period();
	logInt6(msgCode, f_hz, halfPeriod, scanTuningStartCurrent, scanTuningAlternativeCurrent, scanHalfPeriod);
}

static void evaluateTuningHigherEnterStateReg()
{
	if (scanTuningAlternativeCurrent > scanTuningStartCurrent)
	{
		logTuningResult(SCAN_TUNE_UP, scanResonanceHalfPeriod, scanTuningStartCurrent, scanTuningAlternativeCurrent);
		if (scanResonanceHalfPeriod>MIN_HALF_PERIOD)
		{
			scanResonanceHalfPeriod--;
		}
		//#ifdef NEEDED_TUNING_SAME_COUNT
		//if (tuning_same_count)
		//{
		//	tuning_same_count=NEEDED_TUNING_SAME_COUNT;
		//}
		//#endif
	}
	else
	{
		logTuningResult(SCAN_TUNE_STAY, scanResonanceHalfPeriod, scanTuningStartCurrent, scanTuningAlternativeCurrent);
		//#ifdef NEEDED_TUNING_SAME_COUNT
		//if (tuning_same_count)
		//{
		//	tuning_same_count--;
		//}
		//#endif
	}

	checkPeriodThenEnterStateScanRegUsingBuck();
}

static void evaluateTuningLowerEnterStateReg()
{
	if (scanTuningAlternativeCurrent > scanTuningStartCurrent)
	{
		logTuningResult(SCAN_TUNE_DOWN, scanResonanceHalfPeriod, scanTuningStartCurrent, scanTuningAlternativeCurrent);
		if (scanResonanceHalfPeriod<MAX_HALF_PERIOD)
		{
			scanResonanceHalfPeriod++;
		}
		//#ifdef NEEDED_TUNING_SAME_COUNT
		//if (tuning_same_count)
		//{
		//	tuning_same_count=NEEDED_TUNING_SAME_COUNT;
		//}
		//#endif
	}
	else
	{
		logTuningResult(SCAN_TUNE_STAY, scanResonanceHalfPeriod, scanTuningStartCurrent, scanTuningAlternativeCurrent);
		//#ifdef NEEDED_TUNING_SAME_COUNT
		//if (tuning_same_count)
		//{
		//	tuning_same_count--;
		//}
		//#endif
	}

	checkPeriodThenEnterStateScanRegUsingBuck();
}


static void enterStateDischarge()
{
	scanCount = 1000;

	setSuperviseInternalVoltage(0);
	relayOff();

	setState(scanDischarge);
}

static void enterStatePause()
{
	scanCount = 1000;

	//setWantedCurrentZero();

	setInverterDutyCyclePpb(0);
	setSuperviseInternalVoltage(0);
	portsSetDutyOff(); // setInverterDutyCyclePpm(0) will do this also.
	relayOff();
	setState(scanPause);
}

static void enterStatePowerDown()
{
	scanCount = 0;
	setState(scanPowerDown);
}

static void reportErrorAndEnterStateScanDone()
{
	setInverterDutyCyclePpb(0);
	errorReportError(portsErrorNoResonance);
	scanCount=0;
	setState(scanDone);
}

static void enterStateError()
{
	scanCount=0;
	setInverterDutyCyclePpb(0);
	relayOff();
	setSuperviseInternalVoltage(0);
	setState(scanError);
}

char scanGetState(void)
{
	return scan_state;
}

char scanIsStarting(void)
{
	return scan_state == scanInit;
}

int scanIsAtPercent(int requiredPercent)
{
	const int64_t wantedExternalVoltage_mV = getWantedVoltage_mV();
	const int64_t wantedCurrent_mA = getWantedCurrent_mA();
	const int64_t measuredCurrent_mA = measuringGetFilteredOutputAcCurrentValue_mA();
	const int64_t measuredExternalVoltage_mV = getMeasuredExternalAcVoltage_mV();
	if ((measuredExternalVoltage_mV * 100LL) >= (wantedExternalVoltage_mV*requiredPercent))
	{
		return 1;
	}
	else if ((measuredCurrent_mA * 100LL) >= (wantedCurrent_mA * requiredPercent))
	{
		return 1;
	}
	return 0;
}

char scanIsScanning(void)
{
	switch(scan_state)
	{
		case scanDelayBeforePowerOn:
		case scanWaitForVoltage:
		case scanRampUpDutyCycle:
		case scanFindThePreliminaryResonance:
		case scanGotoPreliminaryResonance:
		case scanDelayBeforeThoroughScan:
		case scanFindUpperBoundry:
		case scanFindLowerBoundry:
			return 1;
		default: break;
	}
	return 0;
}

PORTS_STATUS_SUB_MESSAGES scanGetStateName(void)
{
	switch(scan_state)
	{
		case scanInit:
			if (scanCount>0)
			{
				return IDLE_STATUS;
			}
			#ifdef FAN1_APIN
			else if (!fan1Ok())
			{
				return FAN_NOT_OK_STATUS;
			}
			#endif
			else if (!temp1Ok())
			{
				return INT_TEMP_NOT_OK_STATUS;
			}
			else if (getExtTempState() == EXT_TEMP_OUT_OF_RANGE)
			{
				return EXT_TEMP_OUT_OF_RANGE_STATUS;
			}
			else if (!ExtTempAvailableOrNotRequired())
			{
				return EXT_TEMP_NOT_OK_STATUS;
			}
			#ifdef USE_LPTMR2_FOR_FAN2
			else if (!fan2Ok())
			{
				return FAN_NOT_OK_STATUS;
			}
			#endif
			#ifdef USE_LPTMR2_FOR_TEMP2
			else if (!temp2Ok())
			{
				return TEMP_NOT_OK_STATUS;
			}
			#endif
			#ifdef INTERLOCKING_LOOP_PIN
			else if (!interlockingLoopOk())
			{
				return INTERLOCKING_LOOP_OPEN_STATUS;
			}
			#endif
			else if (!(getMeasuredExtAcVoltageIsAvailableOrNotRequired()))
			{
				return EXTERNAL_SENSOR_VOLTAGE_STATUS;
			}
			else if (superviseIsTargetReached())
			{
				return TARGET_REACHED_OK;
			}
			else if (relayCoolDownTimeRemaining_S()>0)
			{
				if (machineGetRequestedState() == cmdRunCommand)
				{
					return STARTING_STATUS;
				}
				return COOL_DOWN_STATUS;
			}
			else if (measuringGetFilteredInternalDcVoltValue_mV() > getInternalScanningVoltage_mV())
			{
				return DISCHARGING_STATUS;
			}
			return IDLE_STATUS;
		case scanIsReady:
			if (superviseIsTargetReached() || (getCyclesToDo() < CYCLES_TO_DO_MARGIN))
			{
				return TARGET_REACHED_OK;
			}
			else if (getWantedVoltage_mV()==0)
			{
				return READY_TO_RUN;
			}
			else if (getWantedCurrent_mA()==0)
			{
				return READY_TO_RUN;
			}
			return PAUSED_STATUS;
		case scanClearOverCurrentDLatch:
			return RESETTING_STATUS;
		case scanDelayBeforePowerOn:
		case scanWaitForVoltage:
		case scanRampUpDutyCycle:
			if (measuringGetFilteredInternalDcVoltValue_mV() <= getInternalScanningVoltage_mV())
			{
				return WAITING_FOR_POWER_STATUS;
			}
			else if (!relayIsReadyToScan())
			{
				return WAITING_FOR_POWER_STATUS;
			}
			return POWERING_UP;
		case scanFindThePreliminaryResonance:
		case scanGotoPreliminaryResonance:
		case scanDelayBeforeThoroughScan:
		case scanFindUpperBoundry:
		case scanFindLowerBoundry:
			return SCANNING_STATUS;
		case scanGotoResonanceBeforeNormalOperation:
		case scanNormalOperation:
		{
			if (scanIsAtPercent(95))
			{
				return RUNNING_STATUS;
			}
			else
			{
				return RAMPING_UP;
			}
		}
		case scanDelayBeforeTuning:
		case scanMeasureBeforeTuning:
		case scanTestingLowerFreq:
		case scanTestingHigherFreq:
		{
			if (scanIsAtPercent(99))
			{
				return RUNNING_STATUS;
			}
			else
			{
				return FINE_TUNING;
			}
		}
		case scanPowerDown:
		case scanDischarge:
		case scanPause:
		{
			uint32_t m = measuringGetFilteredInternalDcVoltValue_mV();
			if (m > HARMLESS_DC_VOLTAGE_MV)
			{
				return DISCHARGING_STATUS;
			}
			if (m > (getInternalScanningVoltage_mV()/2))
			{
				return DISCHARGING_STATUS;
			}
			if (superviseIsTargetReached() || (getCyclesToDo() < CYCLES_TO_DO_MARGIN))
			{
				return TARGET_REACHED_OK;
			}
			return PAUSED_STATUS;
		}
		case scanDone:
			return NO_RESONANSE_FOUND;
		case scanError:
		{
			const int e = errorGetReportedError();
			if ((e>=0) && (e<64))
			{
				return e;
			}
			return DRIVE_ERROR_STATUS;
		}
		default: break;
	}
	return UNKNOWN_STATUS;
}


/*static uint32_t scanGetHalfPeriod(void)
{
	return ports_get_period();
}*/



uint32_t scanGetResonanceHalfPeriod(void)
{
	return scanResonanceHalfPeriod;
}

void scanSetResonanceHalfPeriod(uint32_t halfPeriod)
{
	scanResonanceHalfPeriod=halfPeriod;
	checkResonanceHalfPeriod();
}

#if 0
#else
#define logIncOrDec(a,b)
#define logIncOrDecMainTick()
#endif



static int dontRun()
{
	if (machineGetRequestedState() != cmdRunCommand)
	{
		logInt3(SCAN_ABORTING_RUN, scan_state, 2);
		return 2;
	}

	/*if (!fansAndTempOK())
	{
		logInt3(SCAN_ABORTING_RUN, scan_state, 3);
		return 3;
	}*/

	return 0;
}


void scan_init()
{
	enterStateIdle();
}

void static evaluateLowerBoundryEnterNextState(const uint32_t scanHalfPeriod, const int32_t measuredCurrent_mA)
{
	scanLowerBoundryHalfPeriod=scanHalfPeriod;
	scanSetResonanceHalfPeriod((scanTopHalfPeriodFoundUp+scanTopHalfPeriodFoundDown)/2);
	scanPeakWidthInPeriodTime = scanLowerBoundryHalfPeriod - scanUpperBoundryHalfPeriod;

	// Q = Omega / delta Omega

	const int32_t f_hz = freq_from_half_period(scanHalfPeriod);
	logInt6(SCAN_LOWER_BOUNDRY_FOUND, scanTopCurrentFoundDown, scanLowerBoundryHalfPeriod, f_hz, measuredCurrent_mA, scanTopPreliminaryCurrentFound_mA);

	if ((scanUpperBoundryHalfPeriod!=0) && (scanPeakWidthInPeriodTime!=0))
	{
		// We have period.
		// Omega = 2*pi*1/period
		// Hopefully this will also give Q
		scanQ = DIV_ROUND(scanResonanceHalfPeriod, scanPeakWidthInPeriodTime);
		if (scanQ > MAX_Q)
		{
			logInt2(SCAN_Q_LARGER_THAN_EXPECTED, scanQ);
			reportErrorAndEnterStateScanDone();
		}
		else if (scanQ < MIN_Q)
		{
			logInt2(SCAN_Q_LESS_THAN_EXPECTED, scanQ);
			reportErrorAndEnterStateScanDone();
		}
		else
		{
			logInt2(SCAN_Q, scanQ);

			scanCount=100;
			// We will use the mean value of high and low limits
			setInverterHalfPeriod(scanHalfPeriod);
			checkPeriodThenEnterStateScanRegUsingBuck();
		}
	}
	else
	{
		logInt1(SCAN_FAILED_TO_FIND_Q);
		reportErrorAndEnterStateScanDone();
	}
}


/**
 * This is to be called once per tick.
 * That is typically once per millisecond.
 */
void scanMediumTick()
{
	logIncOrDecMainTick();

	const uint32_t scanHalfPeriod = ports_get_period();

	#ifndef DONT_SCAN_FOR_RESONANCE

	const int32_t wantedExternalVoltage_mV = getWantedVoltage_mV();
	const int32_t wantedCurrent_mA = getWantedCurrent_mA();

	#if (defined INV_OUTPUT_AC_CURRENT_CHANNEL)
	const int32_t measuredCurrent_mA = measuringGetFilteredOutputAcCurrentValue_mA();
	#else
	#error
	const int32_t measuredCurrent_mA = measuringGetFilteredDcCurrentValue_mA();
	#endif

	const int32_t measuredExternalVoltage_mV = getMeasuredExternalAcVoltage_mV();


	switch(scan_state)
	{
		case scanInit:
		{
			// In this state scan is waiting for other parts of the program to be ready and for start command.

			//setWantedCurrentZero();

			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (!translatorConvertionAvailable())
			{
				logWaiting2(SCAN_NEED_A_SAVED_CONFIGURATION, translatorConvertionAvailable());
			}
			else if (scanCount>0)
			{
				// Wait a little;
				scanCount--;
			}
			else if (!measuringIsReady())
			{
				logWaiting2(SCAN_WAITING_FOR_MEASURING, measuringIsReady());
			}
			#ifdef FAN1_APIN
			else if (!fan1Ok())
			{
				logWaiting2(SCAN_FAN1_NOT_READY_YET, fanAndTempGetFan1Measurement_Hz());
			}
			#endif
			else if (relayCoolDownTimeRemaining_S()>0)
			{
				logWaiting2(SCAN_WAITING_FOR_RELAY, relayCoolDownTimeRemaining_S());
			}
			else if (!temp1Ok())
			{
				logWaiting2(SCAN_TEMP1_NOT_READY_YET, tempGetTemp1Measurement_C());
			}
			else if (!ExtTempAvailableOrNotRequired())
			{
				logWaiting2(SCAN_EXT_TEMP_NOT_READY_YET, getExtTempState());
			}
			#ifdef USE_LPTMR2_FOR_FAN2
			else if (!fan2Ok())
			{
				logWaiting2(SCAN_FAN2_NOT_READY_YET, fanAndTempGetFan2Measurement_Hz());
			}
			#endif
			#ifdef USE_LPTMR2_FOR_TEMP2
			else if (!temp2Ok())
			{
				logWaiting2(SCAN_TEMP2_NOT_READY_YET, fanAndTempGetTemp2Measurement_C());
			}
			#endif
			#ifdef INTERLOCKING_LOOP_PIN
			else if (!interlockingLoopOk())
			{
				logWaiting1(INTERLOCKING_LOOP_IS_OPEN);
			}
			#endif
			else if (!(getMeasuredExtAcVoltageIsAvailableOrNotRequired()))
			{
				logWaiting2(SCAN_VOLTAGE_SENSOR_NOT_READY_YET, getMeasuredExtAcVoltageIsAvailableOrNotRequired());
			}
			else if (superviseIsTargetReached())
			{
				logWaiting2(SCAN_TAGET_REACHED, 0);
			}
			else
			{
				// Everything is ready so far.
				enterStateReady();
			}
			break;
		}
		case scanIsReady:
		{
			//setWantedCurrentZero();

			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (superviseIsTargetReached())
			{
				logWaiting2(SCAN_TAGET_REACHED, 0);
			}
			else if (machineGetRequestedState() != cmdRunCommand)
			{
				logWaiting2(SCAN_WAITING_FOR_START_COMMAND, cmdRunCommand);
			}
			else if (wantedExternalVoltage_mV==0)
			{
				logWaiting2(SCAN_WANTED_VOLTAGE_IS_ZERO, wantedExternalVoltage_mV);
			}
			else if (wantedCurrent_mA==0)
			{
				logWaiting2(SCAN_WANTED_CURRENT_IS_ZERO, wantedCurrent_mA);
			}
			else
			{
				// Everything is ready to go.
				enterClearOrDelay();
			}
			break;
		}
		case scanClearOverCurrentDLatch:
		{
			//setWantedCurrentZero();

			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (dontRun())
			{
				enterStateIdle();
			}
			else if (portsIsBreakActive())
			{
				// No feedback that DLatch is clear yet, wait more.
				logWaiting1(SCAN_NO_TRIPPED_CLEAR);
			}
			else
			{
				enterStateDelayBeforePowerOn();
			}
			break;
		}
		case scanDelayBeforePowerOn:
		{
			//setWantedCurrentZero();

			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (dontRun())
			{
				enterStateIdle();
			}
			else if (measuredCurrent_mA>wantedCurrent_mA)
			{
				// No need to continue we already have enough.
				logInt4(SCAN_ENOUGH_WHILE_SCANNING, 1, wantedCurrent_mA, measuredCurrent_mA);

				// This should not happen so something is wrong.
				errorReportError(portsTargetExceededWhileWarmingUp);

				enterStateIdle();
			}
			else if ((getMeasuredExtAcVoltageUse()) && (measuredExternalVoltage_mV*10L>wantedExternalVoltage_mV*9L))
			{
				// No need to continue we already have enough.
				logInt4(SCAN_ENOUGH_WHILE_SCANNING, 2, wantedExternalVoltage_mV, measuredExternalVoltage_mV);

				// This should not happen so something is wrong.
				errorReportError(portsTargetExceededWhileWarmingUp);

				enterStateIdle();
			}
			else if (scanCount>0)
			{
				// Wait a little
				scanCount--;
			}
			else
			{
				enterStateWaitForVoltage();
			}
			break;
		}
		case scanWaitForVoltage:
		{
			// In this state waiting for relay 1 to be ready (we get internal voltage).

			//setWantedCurrent_mA(wantedCurrent_mA);

			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (dontRun())
			{
				enterStateIdle();
			}
			else if (measuredCurrent_mA>wantedCurrent_mA)
			{
				// No need to continue we already have enough.
				logInt4(SCAN_ENOUGH_WHILE_SCANNING, 3, wantedCurrent_mA, measuredCurrent_mA);

				// This should not happen so something is wrong.
				errorReportError(portsTargetExceededWhileWarmingUp);

				enterStateIdle();
			}
			else if ((getMeasuredExtAcVoltageUse()) && (measuredExternalVoltage_mV*10L>wantedExternalVoltage_mV*9L))
			{
				// No need to continue we already have enough.
				logInt4(SCAN_ENOUGH_WHILE_SCANNING, 4, wantedExternalVoltage_mV, measuredExternalVoltage_mV);

				// This should not happen so something is wrong.
				errorReportError(portsTargetExceededWhileWarmingUp);

				enterStateIdle();
			}
			else if (measuringGetFilteredInternalDcVoltValue_mV() <= getInternalScanningVoltage_mV())
			{
				logWaiting1(SCAN_WAITING_FOR_VOLTAGE_TO_SCAN_WITH);
			}
			else if (measuringGetFilteredInternalDcVoltValue_mV() > previousMeasuringVoltage_mV)
			{
				previousMeasuringVoltage_mV = measuringGetFilteredInternalDcVoltValue_mV();
				if (scanCount < 100)
				{
					scanCount = 100;
				}
				logWaiting1(SCAN_WAITING_FOR_VOLTAGE_TO_SCAN_WITH);
			}
			else if (scanCount>0)
			{
				// Wait a little
				scanCount--;
			}
			else
			{
				// REG is now at desired power, continue with scanning.
				logInt3(SCAN_SCANNING_FOR_RESONANCE, ee.scanBegin_Hz, ee.scanEnd_Hz);
				enterRampUpDutyCycle();
			}
			break;
		}
		case scanRampUpDutyCycle:
		{
			// In this state we wait for second relay to be on and increase duty cycle.
			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (dontRun())
			{
				enterStateIdle();
			}
			else if (measuredCurrent_mA>wantedCurrent_mA)
			{
				// No need to continue we already have enough.
				logInt4(SCAN_ENOUGH_WHILE_SCANNING, 1, wantedCurrent_mA, measuredCurrent_mA);

				// This should not happen so something is wrong.
				errorReportError(portsTargetExceededWhileWarmingUp);

				enterStateIdle();
			}
			else if ((getMeasuredExtAcVoltageUse()) && (measuredExternalVoltage_mV*10L>wantedExternalVoltage_mV*9L))
			{
				// No need to continue we already have enough.
				logInt4(SCAN_ENOUGH_WHILE_SCANNING, 2, wantedExternalVoltage_mV, measuredExternalVoltage_mV);

				// This should not happen so something is wrong.
				errorReportError(portsTargetExceededWhileWarmingUp);

				enterStateIdle();
			}
			else if (!relayIsReadyToScan())
			{
				logWaiting2(SCAN_WAITING_FOR_POWER_TO_SCAN_WITH, relayGetState());
				//relay1Off2On(); // TODO This call should not be needed.
			}
			else if (scanCount>0)
			{
				// Wait a little
				scanCount--;
			}
			else if (scanDutyCyclePpb<LOWEST_USABLE_DUTY_CYCLE_PPB)
			{
				// increase portPwm duty cycle (Reduce angle) until low duty cycle is reached.
				// This step will do 0 to lowest usable duty cycle in 5s
				//const int step = DIV_ROUND_UP(LOWEST_USABLE_DUTY_CYCLE_PPM, 5000);
				//incInverterDutyCyclePpm(step);
				incInverterDutyCyclePpb(100000);
			}
			else
			{
				enterScanningOrRegulation();
			}
			break;
		}
		case scanFindThePreliminaryResonance:
		{
			//setWantedCurrent_mA(wantedCurrent_mA);

			// Scan from start freq to max freq (shorter period)
			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (dontRun())
			{
				enterStateIdle();
			}
			else if ((measuredCurrent_mA*10L>wantedCurrent_mA*9L) || ((getMeasuredExtAcVoltageUse()) && (measuredExternalVoltage_mV*10L>wantedExternalVoltage_mV*9L)))
			{
				// No need to continue we already have enough
				logInt6(SCAN_ENOUGH_WHILE_SCANNING, 5, wantedCurrent_mA, measuredCurrent_mA, wantedExternalVoltage_mV, measuredExternalVoltage_mV);
				scanTopPreliminaryHalfPeriodFound=0;

				if ((ee.optionsBitMask >> FAIL_SCAN_IF_TO_MUCH_CURRENT_BIT) & 1)
				{
					errorReportError(portsErrorScanningCouldNotFinish);
				}
				else
				{
					scanSetResonanceHalfPeriod(scanHalfPeriod);
					checkPeriodThenEnterStateScanRegUsingBuck();
				}
			}
			else if (scanCount!=0)
			{
				// Wait a little.
				scanCount--;
			}
			else
			{
				if (measuredCurrent_mA>scanTopPreliminaryCurrentFound_mA)
				{
					// A new top current has been found, remember it.
					scanTopPreliminaryCurrentFound_mA = measuredCurrent_mA;
					scanTopPreliminaryHalfPeriodFound = scanHalfPeriod;
				}

				// Having the mean value for all scanned frequencies would be nice
				scanCurrentSum += measuredCurrent_mA;
				scanSumNSamples++;

				if ((ee.scanBegin_Hz > ee.scanEnd_Hz) && (scanHalfPeriod<STOP_HALF_PERIOD))
				{
					setInverterHalfPeriod(scanHalfPeriod+calcChangeStep(scanHalfPeriod));
					scanCount=calcChangeDelay(scanHalfPeriod, PRE_SCAN_DELAY);
				}
				else if (scanHalfPeriod>STOP_HALF_PERIOD)
				{
					setInverterHalfPeriod(scanHalfPeriod-calcChangeStep(scanHalfPeriod));
					scanCount=calcChangeDelay(scanHalfPeriod, PRE_SCAN_DELAY);
				}
				else
				{
					// We reached highest frequency.

					const uint32_t meanValue = DIV_ROUND_UP(scanCurrentSum, scanSumNSamples);

					const int scanRange = calculateScanningRange();
					const int scanCenter = calculateScanningCenterFrequency();
					const int scanRangePercent = MIN((RESONANCE_FOUND_FACTOR*100), ((scanRange+scanCenter)*100)/scanCenter);


					if (scanTopPreliminaryCurrentFound_mA < minimumMeasurableCurrent_mA)
					{
						logInt3(SCAN_NO_LOAD_DETECTED, minimumMeasurableCurrent_mA, scanTopPreliminaryCurrentFound_mA);
						reportErrorAndEnterStateScanDone();
					}
					else if (((meanValue*scanRangePercent) >= (scanTopPreliminaryCurrentFound_mA*100)) || (scanTopPreliminaryHalfPeriodFound==0))
					{
						// Did not find resonance
						logInt7(SCAN_DID_NOT_FIND_RESONANCE, scanTopPreliminaryCurrentFound_mA, scanTopPreliminaryHalfPeriodFound, meanValue, scanRange, scanCenter, scanRangePercent);
						reportErrorAndEnterStateScanDone();
					}
					else
					{
						// A resonance was found, next is to find how wide it is.
						const int32_t f = freq_from_half_period(scanTopPreliminaryHalfPeriodFound);
						const int32_t cTop_mA = scanTopPreliminaryCurrentFound_mA;
						const int32_t cMean_mA = meanValue;
						logInt7(SCAN_PRELIMINARY_RESONANCE_FOUND, f, cTop_mA, cMean_mA, scanTopPreliminaryHalfPeriodFound,scanTopPreliminaryCurrentFound_mA, meanValue);

						/*#ifdef __linux__
						// half period is in units of 1/AVR_FOSC
						const int32_t f= TMR_TICKS_PER_SEC/(scanTopPreliminaryHalfPeriodFound*2);
						printf("scan: preliminary resonance at %d\n",f);
						fflush(stdout);
						#endif*/

						//enterGotoPreliminaryResonance
						scanCount=0;
						setState(scanGotoPreliminaryResonance);
					}
				}
			}
			break;
		}
		case scanGotoPreliminaryResonance:
		{
			//setWantedCurrent_mA(wantedCurrent_mA);

			// Go down to the preliminary frequency and a little lower (increase period length)
			const uint32_t targetHalfPeriod = scanTopPreliminaryHalfPeriodFound + scanTopPreliminaryHalfPeriodFound/16;
			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (dontRun())
			{
				enterStateIdle();
			}
			else if (measuredCurrent_mA>wantedCurrent_mA)
			{
				// No need to continue we already have enough
				logInt4(SCAN_ENOUGH_WHILE_SCANNING, 7, wantedCurrent_mA, measuredCurrent_mA);
				scanSetResonanceHalfPeriod(scanHalfPeriod);
				checkPeriodThenEnterStateScanRegUsingBuck();
			}
			else if ((getMeasuredExtAcVoltageUse()) && (measuredExternalVoltage_mV>wantedExternalVoltage_mV))
			{
				// No need to continue we already have enough
				logInt4(SCAN_ENOUGH_WHILE_SCANNING, 8, wantedExternalVoltage_mV, measuredExternalVoltage_mV);
				scanSetResonanceHalfPeriod(scanHalfPeriod);
				checkPeriodThenEnterStateScanRegUsingBuck();
			}
			else if (scanCount!=0)
			{
				// Wait a little.
				scanCount--;
			}
			else if (scanHalfPeriod<targetHalfPeriod)
			{
				setInverterHalfPeriod(scanHalfPeriod + calcChangeStep(scanHalfPeriod));
				scanCount=calcChangeDelay(scanHalfPeriod, PRE_SCAN_DELAY/2);
			}
			else if (scanHalfPeriod>targetHalfPeriod)
			{
				// This will probably never happen.
				setInverterHalfPeriod(scanHalfPeriod - 1);
				scanCount=calcChangeDelay(scanHalfPeriod, PRE_SCAN_DELAY/2);
			}
			else
			{
				const int32_t f = freq_from_half_period(scanHalfPeriod);
				logInt3(SCAN_NOW_AT_FREQ, f, scanHalfPeriod);
				setInverterHalfPeriod(scanHalfPeriod);

				scanCount=SCAN_BOUNDRY_DELAY*100;
				setState(scanDelayBeforeThoroughScan);

			}
			break;
		}

		case scanDelayBeforeThoroughScan:
		{
			//setWantedCurrent_mA(wantedCurrent_mA);

			// This state is just a little delay so that things are stable

			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (dontRun())
			{
				enterStateIdle();
			}
			else if ((measuredCurrent_mA>wantedCurrent_mA) || ((getMeasuredExtAcVoltageUse()) && (measuredExternalVoltage_mV>wantedExternalVoltage_mV)))
			{
				// No need to continue we already have enough
				logInt5(SCAN_ENOUGH_WHILE_SCANNING, wantedCurrent_mA, measuredCurrent_mA, wantedExternalVoltage_mV, measuredExternalVoltage_mV);
				scanSetResonanceHalfPeriod(scanHalfPeriod);
				checkPeriodThenEnterStateScanRegUsingBuck();
			}
			else if (scanCount!=0)
			{
				// Wait a little
				scanCount--;
			}
			else
			{
				logInt1(SCAN_MEASURING_RESONANCE_WIDTH);
				scanCount=SCAN_BOUNDRY_DELAY;
				scanTopCurrentFoundUp = 0;
				setState(scanFindUpperBoundry);
			}
			break;
		}
		case scanFindUpperBoundry:
		{
			//setWantedCurrent_mA(wantedCurrent_mA);

			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (dontRun())
			{
				enterStateIdle();
			}
			else if ((measuredCurrent_mA>wantedCurrent_mA) || ((getMeasuredExtAcVoltageUse()) && (measuredExternalVoltage_mV>wantedExternalVoltage_mV)))
			{
				// No need to continue we already have enough
				logInt5(SCAN_ENOUGH_WHILE_SCANNING, wantedCurrent_mA, measuredCurrent_mA, wantedExternalVoltage_mV, measuredExternalVoltage_mV);
				scanSetResonanceHalfPeriod(scanHalfPeriod);
				checkPeriodThenEnterStateScanRegUsingBuck();
			}
			else if (scanCount!=0)
			{
				scanCount--;
			}
			else
			{
				if (measuredCurrent_mA>scanTopCurrentFoundUp)
				{
					// Highest current found so far
					scanTopCurrentFoundUp = measuredCurrent_mA;
					scanTopHalfPeriodFoundUp = scanHalfPeriod;

				}

				// Approximate square root of 2. Is 14/10
				if (measuredCurrent_mA*14L<scanTopCurrentFoundUp*10L)
				{
					// We just passed the top
					// So this is the high limit of resonance, now we need the low end
					scanUpperBoundryHalfPeriod=scanHalfPeriod;
					const int32_t f_hz = freq_from_half_period(scanHalfPeriod);
					logInt6(SCAN_HIGHER_BOUNDRY_FOUND, scanTopCurrentFoundUp, scanUpperBoundryHalfPeriod, f_hz, measuredCurrent_mA, scanTopPreliminaryCurrentFound_mA);
					scanCount=100;
					scanTopCurrentFoundDown = 0;
					setState(scanFindLowerBoundry);
				}
				else
				{
					if (scanHalfPeriod>MIN_HALF_PERIOD)
					{
						// We can increase frequency more (reduce period time)
						setInverterHalfPeriod(scanHalfPeriod-1);
						scanCount=SCAN_BOUNDRY_DELAY;
					}
					else
					{
						// We reached highest frequency. Did not find upper boundary.
						setInverterHalfPeriod(MIN_HALF_PERIOD);

						logInt2(SCAN_DID_NOT_FIND_UPPER_BOUNDRY, scanTopCurrentFoundUp);

						scanCount=100;
						reportErrorAndEnterStateScanDone();
					}
				}
			}
			break;
		}
		case scanFindLowerBoundry:
		{
			//setWantedCurrent_mA(wantedCurrent_mA);

			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (dontRun())
			{
				enterStateIdle();
			}
			else if ((measuredCurrent_mA>wantedCurrent_mA) || ((getMeasuredExtAcVoltageUse()) && (measuredExternalVoltage_mV>wantedExternalVoltage_mV)))
			{
				// No need to continue we already have enough
				logInt5(SCAN_ENOUGH_WHILE_SCANNING, wantedCurrent_mA, measuredCurrent_mA, wantedExternalVoltage_mV, measuredExternalVoltage_mV);
				scanSetResonanceHalfPeriod(scanHalfPeriod);
				checkPeriodThenEnterStateScanRegUsingBuck();
			}
			else if (scanCount!=0)
			{
				scanCount--;
			}
			else
			{
				if (measuredCurrent_mA>scanTopCurrentFoundDown)
				{
					// A new all time high for finding lower boundary of resonance
					scanTopCurrentFoundDown = measuredCurrent_mA;
					scanTopHalfPeriodFoundDown = scanHalfPeriod;
				}

				// lower as in lower frequency (longer half period)
				// Approximate square root of 2. Is 14/10
				if (measuredCurrent_mA*14L<scanTopCurrentFoundDown*10L)
				{
					// We passed the top, so this is the low limit of resonance
					evaluateLowerBoundryEnterNextState(scanHalfPeriod, measuredCurrent_mA);
				}
				else
				{

					if (scanHalfPeriod<MAX_HALF_PERIOD)
					{
						const uint32_t t = scanTopPreliminaryHalfPeriodFound - scanTopPreliminaryHalfPeriodFound/8;

						// We can reduce frequency further (by incrementing period length)
						if (scanHalfPeriod<t)
						{
							scanCount=SCAN_BOUNDRY_DELAY/2;
						}
						else
						{
							scanCount=SCAN_BOUNDRY_DELAY;
						}

						setInverterHalfPeriod(scanHalfPeriod+1);
					}
					else
					{
						// We reached lowest frequency. Did not find lower boundary
						logInt3(SCAN_DID_NOT_FIND_LOWER_BOUNDRY, scanTopCurrentFoundUp, scanTopCurrentFoundDown);

						reportErrorAndEnterStateScanDone();
					}
				}
			}
			break;
		}
		case scanGotoResonanceBeforeNormalOperation:
		{
			// In this state adjusting the frequency (and perhaps adjusting duty cycle) before normal operation.

			//setWantedCurrent_mA(wantedCurrent_mA);

			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (machineGetRequestedState() != cmdRunCommand)
			{
				enterStatePowerDown();
			}
			else if ((measuredCurrent_mA>wantedCurrent_mA) || ((getMeasuredExtAcVoltageUse()) && (measuredExternalVoltage_mV>wantedExternalVoltage_mV)))
			{
				// More current and/or voltage than wanted while changing to resonance frequency. So reduce duty cycle.
				if (scanDutyCyclePpb > (LOWEST_USABLE_DUTY_CYCLE_PPB+100000))
				{
					decInverterDutyCyclePpb(100000);
				}
				else
				{
					// There is more current or voltage than wanted so no need to go closer to resonance.
					decInverterDutyCyclePpb(LOWEST_USABLE_DUTY_CYCLE_PPB);
					enterStateScanNormalOperation();
				}
			}
			else if (scanCount>0)
			{
				scanCount--;
			}
			else if (scanHalfPeriod>scanResonanceHalfPeriod)
			{
				setInverterHalfPeriod(scanHalfPeriod-1);
				// Short delay so that period is not changed to quickly.
				scanCount=calcChangeDelay(scanHalfPeriod, PRE_SCAN_DELAY);
			}
			else if (scanHalfPeriod<scanResonanceHalfPeriod)
			{
				setInverterHalfPeriod(scanHalfPeriod+1);
				// Short delay so that period is not changed to quickly.
				scanCount=calcChangeDelay(scanHalfPeriod, PRE_SCAN_DELAY);
			}
			else if (scanDutyCyclePpb < LOWEST_USABLE_DUTY_CYCLE_PPB)
			{
				// increase portPwm duty cycle (Reduce angle) until lowest usable duty cycle is reached.
				if ((scanDutyCyclePpb+100000) < LOWEST_USABLE_DUTY_CYCLE_PPB)
				{
					incInverterDutyCyclePpb(100000);
				}
				else
				{
					scanDutyCyclePpb = LOWEST_USABLE_DUTY_CYCLE_PPB;
				}
			}
			//#ifdef NEEDED_TUNING_SAME_COUNT
			//else if (tuning_same_count)
			//{
			//	enterStateDelayBeforeTuning();
			//}
			//#endif
			else
			{
				enterStateScanNormalOperation();
			}
			break;
		}
		case scanNormalOperation:
		{
			//setWantedCurrent_mA(wantedCurrent_mA);

			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (machineGetRequestedState() != cmdRunCommand)
			{
				enterStatePowerDown();
			}
			else if ((measuredCurrent_mA > wantedCurrent_mA) ||
				 ((getMeasuredExtAcVoltageUse()) && (measuredExternalVoltage_mV > wantedExternalVoltage_mV)))
			{
				// To much current and/or voltage

				// If more current than wanted, reduce duty cycle.
				const int32_t bigDiffFactor = 2;
				const int32_t maxCurrentStep_ppb = ee.maxCurrentStep_ppb;
				const int32_t currentDiff_mA = measuredCurrent_mA - wantedCurrent_mA;
				const int32_t bigDiff_mA = (wantedCurrent_mA/bigDiffFactor)+100;
				int32_t current_step_ppb = 0;

				if (currentDiff_mA <= 0)
				{
					// Not to much current so no change needed for it.
					current_step_ppb = 0;
				}
				else if (currentDiff_mA > bigDiff_mA)
				{
					// much more current than wanted (well more than 50% to much).
					current_step_ppb = maxCurrentStep_ppb;
				}
				else
				{
					// Only a little to much current.
					const int64_t mega = 1000000LL;
					const int64_t currentDiff_ppm = (mega * currentDiff_mA) / wantedCurrent_mA;
					const int64_t bigDiff_ppm_ppm = mega / bigDiffFactor; // 50 percent but in ppm

					// When diff is big we want step to be max step.
					current_step_ppb = (currentDiff_ppm * maxCurrentStep_ppb) / bigDiff_ppm_ppm;

					if (current_step_ppb<=1)
					{
						current_step_ppb = 1;
					}
					else if (current_step_ppb > maxCurrentStep_ppb)
					{
						current_step_ppb = maxCurrentStep_ppb;
					}
				}

				// If more voltage than wanted, reduce duty cycle.

				if (getMeasuredExtAcVoltageUse())
				{
					const int32_t voltageDiff_mV = measuredExternalVoltage_mV - wantedExternalVoltage_mV;
					const int32_t bigDiff_mV = (wantedExternalVoltage_mV/bigDiffFactor)+100;
					const int32_t maxVoltageStep_ppb = ee.maxVoltageStep_ppb;
					int32_t voltage_step_ppb = 0;

					if (voltageDiff_mV <= 0)
					{
						// Not to much voltage so no change needed for it.
						voltage_step_ppb = 0;
					}
					else if (voltageDiff_mV > bigDiff_mV)
					{
						// Big difference, use max step.
						voltage_step_ppb = maxVoltageStep_ppb;
					}
					else
					{
						// Only a little to much.
						const int64_t mega = 1000000LL;				
						const int64_t voltageDiff_ppm = (mega * voltageDiff_mV) / wantedExternalVoltage_mV;
						const int64_t bigDiff_ppm_ppm = mega/bigDiffFactor; // 50 percent but in ppm

						// When diff is 10% we want step to be "maxStep_ppb".
						voltage_step_ppb = (voltageDiff_ppm * maxVoltageStep_ppb) / bigDiff_ppm_ppm;

						if (voltage_step_ppb<=1)
						{
							voltage_step_ppb = 1;
						}
						else if (voltage_step_ppb > maxVoltageStep_ppb)
						{
							voltage_step_ppb = maxVoltageStep_ppb;
						}
					}

 					if (currentDiff_mA <= 0)
 					{
 						// There was not to much current so use voltage step.
 						decInverterDutyCyclePpb(voltage_step_ppb);
 					}
 					else if (voltageDiff_mV <= 0)
 					{
						// There was not to much voltage so use current step.
 						decInverterDutyCyclePpb(current_step_ppb);
 					}
 					else
 					{
 						// Both to much voltage and current,
 						// Use the bigger step when decreasing since both current and voltage shall be below wanted.
						if (current_step_ppb > voltage_step_ppb)
						{
							decInverterDutyCyclePpb(current_step_ppb);
						}
						else
						{
							decInverterDutyCyclePpb(voltage_step_ppb);
						}
					}
				}
				else
				{
					decInverterDutyCyclePpb(current_step_ppb);
				}

			}
			else if (scanCount>0)
			{
				// Avoid changing very often. So do nothing until scanCount has counted down to zero.
				scanCount--;
			}
			else if (scanHalfPeriod>scanResonanceHalfPeriod)
			{
				setInverterHalfPeriod(scanHalfPeriod-1);
				scanCount=calcChangeDelay(scanHalfPeriod, PRE_SCAN_DELAY);
			}
			else if (scanHalfPeriod<scanResonanceHalfPeriod)
			{
				setInverterHalfPeriod(scanHalfPeriod+1);
				scanCount=calcChangeDelay(scanHalfPeriod, PRE_SCAN_DELAY);
			}
			else if ((scanDutyCyclePpb >= 1000000000) && (scanPeakWidthInPeriodTime!=0))
			{
				// We have ordered full power so try some fine tuning.
				enterStateDelayBeforeTuning();
			}
			else if ((measuredCurrent_mA<wantedCurrent_mA) && 
				((wantedExternalVoltage_mV == NO_VOLT_VALUE) || (measuredExternalVoltage_mV<wantedExternalVoltage_mV)))
			{
				// Both to little current and voltage.

				// Check how much to little the current is.
				const int32_t bigDiffFactor = 2;
				const int32_t maxCurrentStep_ppb = ee.maxCurrentStep_ppb;
				const int32_t currentDiff_mA = wantedCurrent_mA - measuredCurrent_mA;
				const int32_t bigDiff_mA = (wantedCurrent_mA/bigDiffFactor)+100;
				int32_t current_step_ppb = 0;

				if (currentDiff_mA <= 0)
				{
					// Not to little current so no change needed for it.
				}
				else if (currentDiff_mA > bigDiff_mA)
				{
					// much to little current than wanted (well more than 50% to little).
					current_step_ppb = maxCurrentStep_ppb;
				}
				else
				{
					// Only a little to little current.
					const int64_t mega = 1000000LL;				
					const int64_t currentDiff_ppm = (mega * currentDiff_mA) / wantedCurrent_mA;
					const int64_t bigDiff_ppm = mega/bigDiffFactor;

					// When diff is big we want step to be maxStep_ppb.
					current_step_ppb = (currentDiff_ppm * maxCurrentStep_ppb) / bigDiff_ppm;

					if (current_step_ppb <= 1)
					{
						current_step_ppb = 1;
					}
					else if (current_step_ppb > maxCurrentStep_ppb)
					{
						current_step_ppb = maxCurrentStep_ppb;
					}
				}

				// Check how much to little the voltage is.

				if (getMeasuredExtAcVoltageUse())
				{
					const int32_t voltageDiff_mV = wantedExternalVoltage_mV - measuredExternalVoltage_mV;
					const int32_t bigDiff_mV = (wantedExternalVoltage_mV/bigDiffFactor)+100;
					const int32_t maxVoltageStep_ppb = ee.maxVoltageStep_ppb;
					int32_t voltage_step_ppb = 0;
 					if (voltageDiff_mV <= 0)
					{
						// Not to little voltage so no change needed for it.
					}
					else if (voltageDiff_mV > bigDiff_mV)
					{
						// much to little (well more than 50% or so to little).
						voltage_step_ppb = maxVoltageStep_ppb;
					}
					else
					{
						// Only a little to little.
						const int64_t mega = 1000000LL;				
						const int64_t voltageDiff_ppm = (mega * voltageDiff_mV) / wantedExternalVoltage_mV;
						const int64_t bigDiff_ppm = mega/bigDiffFactor;

						// When diff is big we want step to be "maxStep_ppb".
						voltage_step_ppb = (voltageDiff_ppm * maxVoltageStep_ppb) / bigDiff_ppm;

						if (voltage_step_ppb<=1)
						{
							voltage_step_ppb = 1;
						}
						else if (voltage_step_ppb > maxVoltageStep_ppb)
						{
							voltage_step_ppb = maxVoltageStep_ppb;
						}
					}

 					if (currentDiff_mA <= 0)
 					{
 						// There was not to little current so use voltage step.
 						// This should not happen.
						incInverterDutyCyclePpb(voltage_step_ppb);
 					}
 					else if (voltageDiff_mV <= 0)
 					{
						// There was not to little voltage so use current step.
 						// This should not happen.
						incInverterDutyCyclePpb(current_step_ppb);
 					}
 					else
 					{
 						// Use whatever is the smaller step when incrementing.
 						if (current_step_ppb < voltage_step_ppb)
 						{
 							incInverterDutyCyclePpb(current_step_ppb);
 						}
 						else
 						{
 							incInverterDutyCyclePpb(voltage_step_ppb);
 						}
 					}
				}
				else
				{
					incInverterDutyCyclePpb(current_step_ppb);
				}
			}
			break;
		}
		case scanDelayBeforeTuning:
		{
			//setWantedCurrent_mA(wantedCurrent_mA);

			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (machineGetRequestedState() != cmdRunCommand)
			{
				enterStatePowerDown();
			}
			else if ((measuredCurrent_mA>wantedCurrent_mA) || ((getMeasuredExtAcVoltageUse()) && (measuredExternalVoltage_mV>wantedExternalVoltage_mV)))
			{
				// More current or voltage than wanted.
				decInverterDutyCyclePpb(1000);
				checkPeriodThenEnterStateScanRegUsingBuck();
			}
			else if (scanCount>0)
			{
				scanCount--;
			}
			else
			{
				enterStateMeasureBeforeTuning();
			}
			break;
		}

		case scanMeasureBeforeTuning:
		{
			//setWantedCurrent_mA(wantedCurrent_mA);

			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (machineGetRequestedState() != cmdRunCommand)
			{
				enterStatePowerDown();
			}
			else if ((measuredCurrent_mA>wantedCurrent_mA) || ((getMeasuredExtAcVoltageUse()) && (measuredExternalVoltage_mV>wantedExternalVoltage_mV)))
			{
				// More current or voltage than wanted.
				decInverterDutyCyclePpb(1000);
				checkPeriodThenEnterStateScanRegUsingBuck();
			}
			else if (scanCount>0)
			{
				scanTuningStartCurrent += measuredCurrent_mA;
				scanCount--;
			}
			else
			{
				if (nextTryUp)
				{
					nextTryUp=0;
					enterStateTestingHigherFrequency();
				}
				else
				{
					nextTryUp=1;
					enterStateTestingLowerFrequency();
				}
			}
			break;
		}

		case scanTestingHigherFreq:
		{
			//setWantedCurrent_mA(wantedCurrent_mA);

			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (machineGetRequestedState() != cmdRunCommand)
			{
				enterStatePowerDown();
			}
			else if ((measuredCurrent_mA>wantedCurrent_mA) || ((getMeasuredExtAcVoltageUse()) && (measuredExternalVoltage_mV>wantedExternalVoltage_mV)))
			{
				// We now have more current or voltage than wanted.
				decInverterDutyCyclePpb(100);
				if (scanResonanceHalfPeriod>MIN_HALF_PERIOD)
				{
					scanResonanceHalfPeriod--;
				}
				logTuningResult(SCAN_TUNE_UP, scanResonanceHalfPeriod, scanTuningStartCurrent, scanTuningAlternativeCurrent);
				checkPeriodThenEnterStateScanRegUsingBuck();
			}
			else if (scanCount>0)
			{
				scanTuningAlternativeCurrent += measuredCurrent_mA;
				scanCount--;
			}
			else
			{
				evaluateTuningHigherEnterStateReg();
			}
			break;
		}
		case scanTestingLowerFreq:
		{
			//setWantedCurrent_mA(wantedCurrent_mA);

			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (machineGetRequestedState() != cmdRunCommand)
			{
				enterStatePowerDown();
			}
			else if ((measuredCurrent_mA>wantedCurrent_mA) || ((getMeasuredExtAcVoltageUse()) && (measuredExternalVoltage_mV>wantedExternalVoltage_mV)))
			{
				// We now have more current or voltage than wanted.
				decInverterDutyCyclePpb(100);
				if (scanResonanceHalfPeriod<MAX_HALF_PERIOD)
				{
					scanResonanceHalfPeriod++;
				}
				logTuningResult(SCAN_TUNE_DOWN, scanResonanceHalfPeriod, scanTuningStartCurrent, scanTuningAlternativeCurrent);
				checkPeriodThenEnterStateScanRegUsingBuck();
			}
			else if (scanCount>0)
			{
				scanTuningAlternativeCurrent += measuredCurrent_mA;
				scanCount--;
			}
			else
			{
				evaluateTuningLowerEnterStateReg();
			}
			break;
		}
		case scanPowerDown:
			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (scanDutyCyclePpb>LOWEST_USABLE_DUTY_CYCLE_PPB)
			{
				// We know the coils have a typical Q value of 25 so current and voltage
				// will drop quite fast. Like halved in 25 cycles or so.
				// Decrementing with 500000 ppb per millisecond will reduce duty cycle from full
				// to half in 1000 milliseconds.
				decInverterDutyCyclePpb(500000);
			}
			else
			{
				enterStateDischarge();
			}
			break;
		case scanDischarge:
		{
			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (machineGetRequestedState() == cmdResetCommand)
			{
				enterStateIdle();
			}
			else if (measuringGetFilteredInternalDcVoltValue_mV() > getInternalScanningVoltage_mV())
			{
				logWaiting1(SCAN_WAITING_FOR_POWER_DOWN);
			}
			else if (machineGetRequestedState() == cmdRunCommand)
			{
				enterClearOrDelay();
			}
			else if (scanDutyCyclePpb>0)
			{
				decInverterDutyCyclePpb(250000);
			}
			else
			{
				enterStatePause();
			}
			break;
		}
		case scanPause:
		{
			if (errorGetReportedError() != portsErrorOk)
			{
				enterStateError();
			}
			else if (machineGetRequestedState() == cmdResetCommand)
			{
				enterStateIdle();
			}
			else if (measuringGetFilteredInternalDcVoltValue_mV() > getInternalScanningVoltage_mV())
			{
				logWaiting1(SCAN_WAITING_FOR_POWER_DOWN);
			}
			else if (machineGetRequestedState() == cmdRunCommand)
			{
				enterClearOrDelay();
			}
			break;
		}
		case scanError:
		case scanDone:
		default:
			//setWantedCurrentZero();
			setInverterDutyCyclePpb(0);
			portsSetDutyOff();

			if (errorGetReportedError() != portsErrorOk)
			{
				logWaiting2(PORTS_ERROR, errorGetReportedError());
			}
			else if (machineGetRequestedState() == cmdResetCommand)
			{
				enterStateIdle();
			}

			// Do nothing
			break;
	}

	#else
	#error
	#endif // DONT_SCAN_FOR_RESONANCE
}


// Returns true if normal operation.
// False if in startup scanning or failed/done state.
// This is used by supervise to check for unexpected drops or surges, those would indicate some error and then machine shall stop.
char scanIsNormalOperation(void)
{
	switch(scan_state)
	{
		case scanInit:
		case scanDelayBeforePowerOn:
		case scanWaitForVoltage:
		case scanRampUpDutyCycle:
		case scanFindThePreliminaryResonance:
		case scanGotoPreliminaryResonance:
		case scanDelayBeforeThoroughScan:
		case scanFindUpperBoundry:
		case scanFindLowerBoundry:
		case scanGotoResonanceBeforeNormalOperation:
			return 0;
		case scanNormalOperation:
			return 1;
		case scanDelayBeforeTuning:
		case scanMeasureBeforeTuning:
		case scanTestingLowerFreq:
		case scanTestingHigherFreq:
		case scanPowerDown:
		case scanDischarge:
		case scanPause:
		case scanDone:
			return 0;
		default:
			break;
	}
	return 0;
}

char scanIsRunning(void)
{
	switch(scan_state)
	{
		case scanInit:
		case scanIsReady:
		case scanClearOverCurrentDLatch:
			return 0;
		case scanDelayBeforePowerOn:
		case scanWaitForVoltage:
		case scanRampUpDutyCycle:
		case scanFindThePreliminaryResonance:
		case scanGotoPreliminaryResonance:
		case scanDelayBeforeThoroughScan:
		case scanFindUpperBoundry:
		case scanFindLowerBoundry:
		case scanGotoResonanceBeforeNormalOperation:
		case scanNormalOperation:
		case scanDelayBeforeTuning:
		case scanMeasureBeforeTuning:
		case scanTestingLowerFreq:
		case scanTestingHigherFreq:
		case scanPowerDown:
			return 1;
		case scanPause:
		case scanDischarge:
		case scanDone:
		case scanError:
			return 0;
		default:
			break;
	}
	return 1;
}


char scanIdleOrPauseEtc(void)
{
	switch(scan_state)
	{
		case scanInit:
		case scanIsReady:
			return 1;
		case scanClearOverCurrentDLatch:
		case scanDelayBeforePowerOn:
		case scanWaitForVoltage:
		case scanRampUpDutyCycle:
		case scanFindThePreliminaryResonance:
		case scanGotoPreliminaryResonance:
		case scanDelayBeforeThoroughScan:
		case scanFindUpperBoundry:
		case scanFindLowerBoundry:
		case scanGotoResonanceBeforeNormalOperation:
		case scanNormalOperation:
		case scanDelayBeforeTuning:
		case scanMeasureBeforeTuning:
		case scanTestingLowerFreq:
		case scanTestingHigherFreq:
		case scanPowerDown:
		case scanDischarge:
			return 0;
		case scanPause:
		case scanDone:
			return 1;
		default:
			break;
	}
	return 1;

}



// This is called once per second or so from secAndLog.
// TODO We can go to pause/idle instead.

/*int32_t scanGetWantedBuckVoltage_mV()
{
	return wantedBuckVoltage_mV;
}*/

void scanBreak()
{
	portsBreak();
}


void scanSecondsTick()
{
	//logInt5(REG_COUNTER, regState, currentThrottle, regCounter, pwmCounter);

	checkResonanceHalfPeriod();

	if (scanIsRunning())
	{
		if (!temp1Ok())
		{
			//setWantedCurrentZero();
			logInt3(SCAN_TEMP_NOT_READY_YET, 1, tempGetTemp1Measurement_C());
			errorReportError(portsErrorTempFail);
		}
		#ifdef USE_LPTMR2_FOR_FAN2
		if (!fan2Ok())
		{
			//setWantedCurrentZero();
			logInt3(SCAN_FANS_NOT_READY_YET, 2, fanAndTempGetFan2Measurement_Hz());
			errorReportError(portsErrorFanFail);
		}
		#endif
		#ifdef USE_LPTMR2_FOR_TEMP2
		if (!temp2Ok())
		{
			//setWantedCurrentZero();
			logInt3(SCAN_TEMP_NOT_READY_YET, 2, fanAndTempGetTemp2Measurement_C());
			errorReportError(portsErrorTempFail);
		}
		#endif
		#ifdef FAN1_APIN
		if (!fan1Ok())
		{
			//setWantedCurrentZero();
			logInt3(SCAN_FANS_NOT_READY_YET, 1, fanAndTempGetFan1Measurement_Hz());
			errorReportError(portsErrorFanFail);
		}
		#endif

		if (portsIsBreakActive())
		{
			errorReportError(portsPwmPortsBreakIsActive);
		}

		if (!(getMeasuredExtAcVoltageIsAvailableOrNotRequired()))
		{
			// No voltage measurement is available
			logInt2(MEASURING_TIMEOUT, 7);
			errorReportError(portsErrorMeasuringTimeout);
		}

	}

}

uint32_t scanGetDutyCyclePpm()
{
	return DIV_ROUND(scanDutyCyclePpb,1000);
}
