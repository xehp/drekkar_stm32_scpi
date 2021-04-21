/*
msg.h

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

In this file message codes are defined.

Created 2018 by Henrik
*/

#ifndef SRC_MESSAGENAMES_H_
#define SRC_MESSAGENAMES_H_

#include "cfg.h"
#include "Dbf.h"

// In all log messages (LOG_MSG) the first field (after LOG_MSG) shall be one of these.
typedef enum
{
	LOG_FREQ_CURRENT_VOLT_HBOX = 1, // After this code, frequency, current, voltage etc shall follow.
	MAIN_LOOP_BEGIN = 2,
	LOG_MEASUREMENT = 3, 
	LOG_FREQ_CURRENT_VOLT_BUCK = 4,
	LOG_FREQ_CURRENT_VOLT_INV = 5,
	CMD_VOLT_VALUE_IS_TO_OLD = 10,
	CMD_VOLTAGE_SENSOR_DETECTED = 11,
	CMD_TARGET_REACHED = 12,
	UNEXPECTED_STRING_MESSAGE = 13,
	UNEXPECTED_EMPTY_MESSAGE = 14,
	UNKNOWN_DBF_MESSAGE = 15,
	CMD_INIT = 16,
	CMD_INCORRECT_DBF_RECEIVED = 17,
	CMD_RESETTING = 18,
	CMD_BUCK_DETECTED = 19,
	EEPROM_WRONG_MAGIC_NUMBER = 20,
	EEPROM_WRONG_CHECKSUM = 21,
	EEPROM_LOADED_OK = 22,
	EEPROM_TRYING_BACKUP = 23,
	EEPROM_BACKUP_LOADED_OK = 24,
	EEPROM_FAILED = 25,
	EEPROM_SAVE_RESULT = 26,
	SCAN_FAN1_NOT_READY_YET = 28,
	SCAN_FAN2_NOT_READY_YET = 29,
	FAN_1_DETECTED = 30,
	FAN_2_DETECTED = 31,
	FAN_1_NOT_DETECTED = 32,
	FAN_2_NOT_DETECTED = 33,
	TEMP_SENSOR_DETECTED = 34,
	TEMP_SENSOR_FAIL =35,
	TEMP_SENSOR_OVERHEAT = 36,
	TEMP_SENSOR_OK = 37,
	FAN_INIT = 38,
	FAN_AND_TEMP_COUNT_FAN2 = 39,
	MEASURING_ZERO_LEVEL_OK = 40,
	MEASURING_TIMEOUT = 41,
	MEASURING_MORE_SENSITIVE_RANGE = 42,
	MEASURING_LESS_SENSITIVE_RANGE = 43,
	MEASURING_OVERLOAD_LOG = 44,
	MEASURING_INIT = 45,
	MEASURING_UNSTABLE = 46,
	MEASURING_INITIALIZED = 47,
	MEASURING_INIT_ADC =48,
	MEASURING_OVER_VOLTAGE = 49,
	PORTS_GO = 50,
	PORTS_OVER_CURRENT = 51,
	PORTS_DLATCH_STATE = 52,
	PORTS_ERROR = 53,
	PORTS_INIT = 54,
	PORTS_WAITING_FOR_ISR = 55,
	PORTS_WAITING_FOR_MEASURING_TEMP_FAN = 56,
	PORTS_MISSED_BEGIN = 57,
	PORTS_WAITING_FOR_START_ORDER = 58,
	PORTS_BREAK_REPORTED = 59,
	REG_START = 60,
	REG_STOP = 61,
	REG_WAITING_FOR_PORTS = 62,
	REG_WAITING_POWER_UP_CMD = 63,
	REG_PORTS_ON = 64,
	REG_INC_LOG = 65,
	REG_DEC_LOG = 66,
	REG_ON_TARGET_LOG = 67,
	REG_ADJUSTED_STEP = 68,
	REG_CHANGE_STATE = 69,
	SUPERVICE_START = 70,
	SUPERVICE_TO_LITTLE_CURRENT = 71,
	SUPERVICE_TO_MUCH_CURRENT = 72,
	SUPERVICE_FREQUENCY_DRIFT = 73,
	SUPERVICE_INTERNAL_VOLTAGE_DRIFT = 74,
	SUPERVICE_NOT_ENABLED = 75,
	SUPERVICE_TIMEOUT_STATUS_MSG =76,
	SEC_AND_LOG_INIT = 80,
	SEC_WAITING_FOR_MEASURING = 81,
	REG_TO_LITTLE_INPUT_VOLTAGE = 82,
	REG_DEBUG_STATE = 83,
	SCAN_Q = 100,
	SCAN_START = 101,
	SCAN_HIGHER_BOUNDRY_FOUND = 102,
	SCAN_DID_NOT_FIND_UPPER_BOUNDRY = 103,
	SCAN_LOWER_BOUNDRY_FOUND = 104,
	SCAN_DID_NOT_FIND_LOWER_BOUNDRY = 105,
	SCAN_FAILED_TO_FIND_Q = 106,
	SCAN_SWITCHING_TO_FINE_TUNING = 107,
	SCAN_LEAVING_FINE_TUNING = 108,
	SCAN_START_TUNING = 109,
	SCAN_TUNING_DID_NOT_DETECT_DROP = 110,
	SCAN_TUNING_STOP_HIGH = 111,
	SCAN_CHANGE_TO_FREQUENCY_REGULATION = 112,
	SCAN_CHANGE_STATE = 113,
	SCAN_NEED_A_SAVED_CONFIGURATION = 114,
	SCAN_PORTS_NOT_READY_YET = 115,
	SCAN_FANS_NOT_READY_YET = 116,
	SCAN_VOLTAGE_SENSOR_NOT_READY_YET = 117,
	SCAN_WANTED_CURRENT_IS_ZERO = 118,
	SCAN_WANTED_VOLTAGE_IS_ZERO = 119,
	SCAN_WAITING_FOR_START_COMMAND = 120,
	SCAN_DID_NOT_FIND_RESONANCE = 121,
	SCAN_PRELIMINARY_RESONANCE_FOUND = 122,
	SCAN_NOW_AT_FREQ = 123,
	SCAN_MEASURING_RESONANCE_WIDTH = 124,
	SCAN_STOP_LOW = 125,
	SCAN_TUNE_TO_LOW = 126,
	SCAN_TUNE_DOWN = 127,
	SCAN_TUNE_UP = 128,
	SCAN_TUNE_STAY = 129,
	SCAN_WAITING_FOR_MEASURING = 130,
	SCAN_TO_LOW_FREQUENY = 131,
	SCAN_INC_PERIOD = 132,
	SCAN_DEC_PERIOD = 133,
	SCAN_ON_TARGET = 134,
	SCAN_Q_LARGER_THAN_EXPECTED = 135,
	SCAN_Q_LESS_THAN_EXPECTED = 136,
	SCAN_ENOUGH_WHILE_SCANNING = 137,
	SCAN_COOLING_DOWN_WILL_START_SOON = 138,
	SCAN_WAITING_FOR_SOMETHING_ELSE = 139,
	SCAN_TEMP_NOT_READY_YET = 140,
	SCAN_WAITING_FOR_REG = 141,
	SCAN_SWITCHING_TO_DUTY_CYCLE = 142,
	SCAN_WAITING_FOR_PSU = 143,
	SCAN_INTERNAL_ERROR = 144,
	SCAN_NO_TRIPPED_CLEAR = 145,
	SCAN_TRIPPED_ALREADY_CLEAR = 146,
	REG_PORTS_OFF = 150,
	REG_COUNTER = 151,
	REG_COOLING_DOWN = 152,
	REG_COOLING_DOWN_MV = 153,
	REG_COOLING_DOWN_S = 154,
	REG_MEASURED_VOLTAGE_MORE_THAN_WANTED_MV = 155,
	REG_WAITING_FOR_INVERTER = 156,
	REG_NO_MEASURING = 157,
	REG_MAX_MIN = 158,
	REG_WAITING_POWER_UP_CURRENT_CMD = 159,
	COMP_STATE_CHANGED = 160,
	REG_MEASURED_CURRENT_MORE_THAN_WANTED_MV = 161,
	REG_PWM_BREAK = 162,
	REG_WAITING_POWER_UP_VOLT_CMD = 163,
	COMP_LOGIC_POWER_OK = 164,
	COMP_LOGIC_POWER_LOST = 165,
	REG_FAILED_TO_SET_PWM_DUTY = 166,
	REG_DID_NOT_SET_WANTED_CURRENT =167,
	REG_DID_NOT_SET_WANTED_VOLTAGE=168,
	PWM_DID_NOT_SET_CCR1 = 169,
	PWM_STATE_CHANGE = 170,
	SCAN_WAITING_FOR_POWER_TO_SCAN_WITH = 171,
	SCAN_SCANNING_FOR_RESONANCE = 172,
	COMP_COUNT = 173,
	SCAN_TEMP1_NOT_READY_YET = 174,
	//SCAN_TEMP2_NOT_READY_YET = 175,
	REG_WAITING_FOR_INPUT_POWER = 176,
	SCAN_TAGET_REACHED = 177,
	MEASURING_TOP_INTERNAL_VOLTAGE_MV = 178,
	MEASURING_TOP_CURRENT_MA = 179,
	ERROR_REPORT = 180,
	ERROR_ASSERT = 181,
	SCAN_ALREADY_HAVE_RESONANCE_FREQUENCY = 182,
	SCAN_TOO_SMALL_INTERVALL_FOR_SCANNING = 183,
	SCAN_START_SCANNING_RESONANCE_FREQUENCY = 184,
	SCAN_PARAMETER_OUT_OF_RANGE = 185,
	REG_WANTED_CURRENT_MA = 186,
	SCAN_ABORTING_RUN = 187,
	REG_WANTED_VOLTAGE_MA = 188,
	CMD_BUCK_TIMEOUT = 190,
	REG_CHARGING_INTERNAL_CAPACITORS = 191,
	PORTS_STATE_CHANGED = 192,
	SCAN_WAITING_FOR_RELAY = 193,
	SCAN_WAITING_FOR_POWER_DOWN = 194,
	MEASURING_OVER_CURRENT = 195,
	MEASURING_OVER_VOLTAGE_MV = 196,
	MEASURING_OVER_VOLTAGE_INPUT_MV = 197,
	MEASURING_OVER_VOLTAGE_INTERNAL_MV = 198,
	CMD_OVER_VOLTAGE_EXTERNAL = 199,
	MEASURING_OVER_CURRENT_MA = 200,
	RELAY_STATE = 201,
	SCAN_WAITING_FOR_VOLTAGE_TO_SCAN_WITH = 202,
	SUPERVICE_INTERNAL_VOLTAGE_DROP = 203,
	MEASURING_OVER_AC_CURRENT = 204,
	MEASURING_TOP_AC_CURRENT_MA = 205,
	MEASURING_OVER_AC_CURRENT_MA = 206,
	RELAY_ORDER = 207,
	SUPERVICE_CURRENT_DROP = 208,
	SUPERVICE_TO_MUCH_VOLTAGE = 209,
	SCAN_NO_LOAD_DETECTED = 210,
	INTERLOCKING_LOOP_IS_OPEN = 211,
	TO_FAST_CURRENT_INC = 212,
	EEPROM_SAVE_PENDING_FLAG_SET = 213,
	EEPROM_PERFOMING_PENDING_SAVE = 214,
	VOLTAGE_SENSOR_FAIL = 215,
	RECEIVED_AC_VOLTAGE =216,
	EXT_TEMP_STATE_CHANGE = 217,
	#if 0
	TEMPERATURE_NOT_RISING = 218,
	TEMPERATURE_HAS_FALLEN = 219,
	#endif
	RECEIVED_TEMP = 220,
	SCAN_EXT_TEMP_NOT_READY_YET = 221,
	WEB_STATUS_RECEIVED = 222,
	CMD_CONFIG = 223,
	LEAK_DETECTOR_TIMEOUT = 224,
	LEAK_DETECTOR_NOW_AVAILABLE = 225,
	TEMP_INIT = 226,
	EEPROM_DATA_UPGRADED = 227,
} AVR_CFG_LOG_MESSAGES;


// Status (STATUS_MSG) sub messages
// This is part of status messages, it tells what the rest of the message contains.
// See also get_state_name in "protocol.js".
// The lowest state name shall be higher then the highest error code in PORTS_ERROR_CODES.
// So let these start on 64.
typedef enum
{
	COOL_DOWN_STATUS = 67,
	READY_TO_RUN = 68,
	RESETTING_STATUS = 69,
	INTERLOCKING_LOOP_OPEN_STATUS = 76,
	RAMPING_UP = 77,
	POWERING_UP = 80,
	NO_RESONANSE_FOUND = 82,
	INT_TEMP_NOT_OK_STATUS = 83,
	EXT_TEMP_NOT_OK_STATUS = 84,
	FINE_TUNING = 85,
	EXT_TEMP_OUT_OF_RANGE_STATUS = 86,
	// 'c' 99 reserved for Connection lost.
	DISCHARGING_STATUS = 100,
	DRIVE_ERROR_STATUS = 101,
	FAN_NOT_OK_STATUS = 102,
	CHARGING_STATUS = 104,
	IDLE_STATUS = 105,
	EXTERNAL_SENSOR_VOLTAGE_STATUS = 109,
	TARGET_REACHED_OK = 111,
	PAUSED_STATUS = 112,
	RUNNING_STATUS = 114,
	SCANNING_STATUS = 115,
	TIMEOUT_STATUS = 116,
	UNKNOWN_STATUS = 117,
	WAITING_FOR_POWER_STATUS = 119,
	STARTING_STATUS = 122,
} PORTS_STATUS_SUB_MESSAGES;

/*
typedef enum
{
	VOLTAGE_INITIAL_LOG=1,
	VOLTAGE_INITIAL_ZERO_LOG=2,
	VOLTAGE_APPROVED_ZERO_LOG=3,
	VOLTAGE_TIMEOUT_LOG=4,
	VOLTAGE_DEBUG_LOG=5
} VOLTAGE_LOG_MESSAGES;
*/

typedef enum
{
	UNKNOWN_STATUS_MSG = 0,
	REBOOT_STATUS_MSG = 2,
	VOLTAGE_STATUS_MSG = 3,
	OVER_VOLTAGE_STATUS_MSG = 4,
	LEAK_CURRENT_STATUS_MSG = 5,
	PARAMETER_STATUS_MSG = 10,
	TEMP_STATUS_MSG = 11,
	WEB_SERVER_STATUS_MSG = 12,
	TARGET_GENERAL_STATUS_MSG = 13,
} STATUS_MESSAGES;


typedef enum
{
	UNKNOWN_CMD = 0,
	ECHO_CMD = 20,
	GET_CMD = 21,
	SET_CMD = 22,
	IGNORE_VOLTAGE_SENSOR_CMD = 24,
	REBOOT_CMD = 25,
	SAVE_CMD = 26,
	STOP_CMD = 27,
	START_CMD = 28,
	SHOW_VERSION_CMD = 29,
	RESTORE_DEFAULT_CMD = 30,
	SEND_ALL_CONFIG2_CMD = 31,
	SEND_ALL_CONFIG3_CMD = 32,
	SEND_ALL_CONFIG1_CMD = 33,
} COMMAND_CODES;


typedef enum
{
	UNKNOWN_CATEGORY = 0,
	STATUS_CATEGORY = 1,
	LOG_CATEGORY = 2,
	COMMAND_CATEGORY = 3,
	REPLY_OK_CATEGORY = 4,
	REPLY_NOK_CATEGORY = 5,
} MESSAGE_CATEGORY;


// Error codes
// The highest error code shall be less then the lowest code in PORTS_STATUS_SUB_MESSAGES.
// The lowest of PORTS_STATUS_SUB_MESSAGES is currently 64. So up to 63 is OK here.
// If adding a code here add it in getPortsErrorName and get_state_name in protocol.js also.
enum
{
	portsErrorOk=0,							// No error detected
	portsErrorUnexpecetedMessage=1,
	portsErrorOverDcCurrentFast=2,				// To high current
	portsErrorScanningCouldNotFinish=3,
	portsErrorNoResonance=4,				// Did not detect the resonance frequency.
	portsErrorUnexpectedCurrentDecrease=5,
	portsErrorMeasuringTimeout=11,			// No voltage measurement is available
	portsCoilsOverheated=14,
	portsErrorUnexpectedCurrentIncrease=15,             // Supervice feature reports unexpected current increase
	portsErrorOverheated=16,	
	portsErrorTempFail=17,
	portsErrorFanFail=18,
	portsExtTempFail=19,
	portsErrorOverVoltage=20,
	portsInternalError=21,
	portsError12VoltLost=22,
	portsErrorAssert=23,
	portsTimerBreak=24,
	//portsErrorOverVoltageOnInput=25,
	//portsFailedToSetupAdc=26,
	//portsSensorBreak = 27,
	//portsUncompatibleDetected=28,
	portsAttemptToSetPeriodOutOfRange=29,
	portsTargetExceededWhileWarmingUp=30,
	//portsDidNotExpectRegRequestMessage=31,
	//portsDidNotExpectRegStatusMessage=32,
	//portsRegPwmBreak=33,
	portsInputVoltageDecrease=34,
	portsExtTempDiff=35,
	portsErrorResonanceFrequencyToLow=36,
	portsErrorResonanceFrequencyToHigh=37,
	portsErrorFrequencyDrift=38,
	portsErrorInternalVoltageDrift=39,
	portsPwmPortsBreakIsActive=40,
	//portsErrorOverWantedCurrent=41,
	//portsErrorOverWantedVoltage=42,
	portsErrorOverOutputAcCurrent=43,
	portsErrorOverAcCurrentSlow=44,
	portsErrorLoopOpen=45,
	portsErrorCurrentLessThanRequired=46,
	portsErrorCurrentMoreThanExpected=47,
	portsErrorCurrentMoreThanAllowed=48,
	portsErrorOverDcCurrentSlow=49,
	portsErrorLeakCurrentExceeded=50,
	portsVoltageMeterNotReacting=51,
} PORTS_ERROR_CODES;

// All parameters who's name ends with a number must be listed here in sequence.
// Is is for example assumed in other parts of SW that TARGET_CYCLES_x &
// REACHED_CYCLES_x are in sequence. So, things need to stay that way.
// Some extra numbers where reserved in case these "arrays" need to be larger.
typedef enum
{
	UNKNOWN_PAR = 0,
	AUTOSTART_PAR = 1,                  // ee.autoStart
	CYCLES_TO_DO = 2,                   // cyclesToDo
	DEVICE_ID = 3,                  // ee.deviceId
	MICRO_AMPS_PER_UNIT_DC = 4,         // ee.microAmpsPerUnit
	MICRO_VOLTS_PER_UNIT_DC = 5,        // ee.microVoltsPerUnit
	MACHINE_SESSION_NUMBER = 6,         // machineSessionNumber
	SUPERVICE_DEVIATION_FACTOR = 7,     // ee.superviceDeviationFactor
	VOLTAGE_PROBE_FACTOR = 8,           // ee.voltageProbeFactor
	FAIL_SCAN_IF_TO_MUCH_CURRENT = 9,
	// spare 10
	WANTED_CURRENT_MA = 11,             // ee.wantedCurrent_mA
	WANTED_VOLTAGE_MV = 12,             // ee.wantedExternalVoltage_mV
	TOTAL_CYCLES_PERFORMED = 13,
	MIN_Q_VALUE = 14,               // ee.minQValue
	MAX_Q_VALUE = 15,               // ee.maxQValue
	MIN_TEMP_HZ = 16,               // ee.tempMinFreq
	MAX_TEMP_HZ =17,                // ee.tempMaxFreq
	TEMP_FREQ_AT_20C = 18,          // ee.freqAt20C
	TEMP_FREQ_AT_100C =19,          // ee.freqAt100C
	WANTED_SCANNING_VOLTAGE_MV = 20,    // ee.wantedInternalScanningVoltage_mV
	SCAN_BEGIN_HZ = 21,             // ee.scanBegin_Hz
	SCAN_END_HZ = 22,               // ee.scanEnd_Hz
	RESONANCE_HALF_PERIOD = 23,
	Q_VALUE = 24,                    // scanQ
	PORTS_PWM_STATE = 25,
	TARGET_TIME_S = 26,
	PORTS_PWM_HALF_PERIOD = 27,
	SCAN_PEAK_WIDTH_IN_PERIOD_TIME =28,
	PORTS_PWM_ANGLE = 29,
	INV_FREQUENCY_HZ = 30,
	PORTS_PWM_MIN_ANGLE = 31,       // ee.portsPwmMinAngle
	COOLDOWN_TIME_S = 32,           // ee.minCooldownTime_s
	WEB_SERVER_TIMEOUT_S = 33,
	WALL_TIME_RECEIVED_MS = 34,
	TIME_COUNTED_S = 35,
	// spare 36 - 37
	FILTERED_CURRENT_DC_MA = 38,		  // measured DC current
	FILTERED_INTERNAL_VOLTAGE_DC_MV = 39, // measured DC voltage
	SUPERVICE_MARGIN_MV = 40,             // ee.superviceMargin_mV
	REPORTED_EXT_AC_VOLTAGE_MV = 41,      // measured AC voltage
	// spare 42
	SCAN_STATE_PAR = 43,
	IS_PORTS_BREAK_ACTIVE = 44,
	MAX_EXT_TEMP_DIFF_C = 45,  // maxExtTempDiff_C
	RELAY_STATE_PAR = 46,
	TEMP1_C = 47,
	TEMP2_C = 48,
	FAN1_HZ = 49, // Not used at the moment. Depends on macro FAN1_APIN.
	FAN2_HZ = 50, // Not used at the moment. Depends on macro USE_LPTMR2_FOR_FAN2.
	MACHINE_REQUESTED_OPERATION = 51,
	REPORTED_ERROR = 52,
	SCAN_DELAY_MS = 53,             // ee.scanDelay_ms
	MAX_ALLOWED_CURRENT_MA = 54,
	MAX_ALLOWED_EXT_VOLT_MV = 55,
	STABLE_OPERATION_TIMER_S =56,
	SUPERVICE_STATE = 57,
	COMP_VALUE = 58,
	DUTY_CYCLE_PPM = 59,
	PORTS_CYCLE_COUNT = 60,
	MICRO_AMPS_PER_UNIT_AC = 61,        // ee.microAmpsPerUnit
	MEASURED_AC_CURRENT_MA = 62,        // measured AC current
	SUPERVICE_STABILITY_TIME_S = 63,
	SUPERVICE_DROP_FACTOR = 64,
	PORTS_PWM_BRAKE_STATE = 65,
	RELAY_COOL_DOWN_REMAINING_S = 67,
	MEASURING_MAX_AC_CURRENT_MA = 68,
	MEASURING_MAX_DC_CURRENT_MA = 69,
	MEASURING_MAX_DC_VOLTAGE_MA = 70,
	LOW_DUTY_CYCLE_PERCENT = 71,
	MAX_TEMP_C = 72,
	MACHINE_STATE = 73,
	SYS_TIME_MS = 74,
	SLOW_INTERNAL_VOLTAGE_MV = 75,
	SUPERVICE_INC_FACTOR = 76,
	SLOW_AC_CURRENT_MA = 77,
	TEMP1_IN_HZ = 78,
	EXT_TEMP1_C = 79,
	EXT_TEMP2_C = 80,
	MAX_EXT_TEMP_C = 81,
	EXT_TEMP_REQUIRED = 82,
	EXT_VOLT_REQUIRED = 83,
	EXT_TEMP_STATE = 84,
	LOOP_REQUIRED = 85,
	TARGET_CYCLES_0 = 86,
	TARGET_CYCLES_1 = 87,
	TARGET_CYCLES_2 = 88,
	TARGET_CYCLES_3 = 89,
	//TARGET_CYCLES_4 = 90,
	//TARGET_CYCLES_5 = 91,
	//TARGET_CYCLES_6 = 92,
	//TARGET_CYCLES_7 = 93,
	REACHED_CYCLES_0 = 94,
	REACHED_CYCLES_1 = 95,
	REACHED_CYCLES_2 = 96,
	REACHED_CYCLES_3 = 97,
	//REACHED_CYCLES_4 = 98,
	//REACHED_CYCLES_5 = 99,
	//REACHED_CYCLES_6 = 100,
	//REACHED_CYCLES_7 = 101,
	MAX_CURRENT_STEP = 102,
	MAX_VOLTAGE_STEP = 103,
	MAX_LEAK_CURRENT_MA = 104,
	LEAK_CURRENT_MA = 105,
	// MEASURED_LEAK_AC_CURRENT_MA = 106,
	MIN_MV_PER_MA_AC = 107,
	// Remember to update LOGGING_MAX_NOF_PARAMETERS if a parameter is added here.
} PARAMETER_CODES;

// Set this value to the highest value used in PARAMETER_CODES+1.
#define MAX_NOF_PARAMETER_CODES 108


// When A message NOK_REPLY_MSG is sent one of these codes shall be used to tell what when wrong.
typedef enum
{
	REASON_OK = 0,
	CANT_SAVE_WHILE_RUNNING = 1,
	NOK_UNKOWN_PARAMETER = 2,
	NOK_UNKNOwN_COMMAND =3,
	IGNORED_SETTING_PARAMETER =4,
	WRONG_MAGIC_NUMBER =5,
	WRONG_SENDER_ID =6,
	PARAMETER_OUT_OF_RANGE = 7,
	READ_ONLY_PARAMETER = 8,
	INCOMPLETE_MESSAGE = 9,
	UNKNOWN_OR_READ_ONLY_PARAMETER = 10,
} NOK_REASON_CODES;

typedef enum{
	CMD_OK = 0,
	CMD_OK_FORWARD_ALSO = -1,
	CMD_RECEIVED_OWN_MESSAGE = -2,
	CMD_RECEIVED_UNKNOWN_MESSAGE = -3,
	CMD_RECEIVED_MESSAGE_NOT_TO_US = -4,
	CMD_SENDER_NOT_AUTHORIZED = -5,
} CmdResult;


const char* getMessageCommandName(COMMAND_CODES messageTypeCode);
const char* getMessageCategoryName(MESSAGE_CATEGORY messageTypeCode);
const char *getPortsStatusMessageName(PORTS_STATUS_SUB_MESSAGES statusCode);
const char *getStatusMessagesName(STATUS_MESSAGES statusCode);
const char *getLogMessageName(int logMessageType);
//const char* getVoltageLogMessageName(int logCode);
const char *getPortsErrorName(int errorCode);
const char * getParameterName(PARAMETER_CODES parameterCode);
PARAMETER_CODES getParameterCodeFromWebName(const char* dataName);
const char* getReasonCodeName(NOK_REASON_CODES reason);

void logScanningAndRunningStateToString(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize);
void logOldFormatsFreqCurrVoltToString(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize);
void logStatusMessageToString(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize);
void log_message_to_string(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize);
void log_reply_message_to_string(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize);
//void log_voltage_message_to_string(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize);

void decodeDbfToText(const unsigned char *msgPtr, int msgLen, char *buf, int bufSize);
void linux_sim_log_message_from_target(const DbfReceiver *dbfReceiver);
void logDbfAsHexAndText(const unsigned char *msgPtr, int msgLen);



#endif
