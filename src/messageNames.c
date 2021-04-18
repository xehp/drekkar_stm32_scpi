/*
messageNames.c

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

In this file message codes are defined.

Created 2018 by Henrik
*/

/*
Message format
	All messages begin with a message category code.
	<msg catagory code>
	It shall be one of the following:

		STATUS_CATEGORY
			Cyclic messages, only latest data is of interest,
			all is broadcasted. So no sequence numbering or receiver ID
			is needed, only sender ID.
			After this code the following fields (AKA codes) are expected:
			<sender ID>
			<status message type>
				see STATUS_MESSAGES
			<status message body>
				The rest of message depend on status message type

		LOG_CATEGORY
			Logging messages, only for logging, shall have no effect on operation.
			All log messages are broadcasted so need no receiver ID.
			After this code the following fields (AKA codes) are expected:
			<sender ID>
			<log sequence counter>
				A log sequence number 0-63, incremented by one for each message
				the sequence number is added so that a warning can be logged
				if some log message was lost.
			<log message type>
				see AVR_CFG_LOG_MESSAGES
			<log message body>
				The rest of message depend on log message type

		COMMAND_CATEGORY
			Messages for which a reply is required,
			A reply is expected for all command messages.
			It will have the following fields:
				<sender ID>
					ID of unit sending the command, also the unit to reply to.
				<cmdDestinationID>
					ID of unit the command is to be performed by.
				<reference number>
					Note it is not same sequence number as in log messages.
					This one can be but does not need to be limited to 0-63
					and it is not required to increment by one every time
					as long as it is different from the previous few.
				<command message type>
					see COMMAND_CODES
				<command message body>
					The rest of message depend on command message type,
					Not necessarily the same fields as in the OK reply message.

		REPLY_OK_CATEGORY
			OK reply to a command message.
			<sender ID>
				ID of unit sending the reply.
			<reply to ID>
				ID of unit that sent the command and now shall have the reply.
			<reference number>
				Same number as was given in the command.
			<command>
				See COMMAND_CODES in msg.h
			<ok reply body>
				The rest of message depend on command message type,
				Not necessarily the same fields as in the OK reply message.

		REPLY_NOK_CATEGORY
			Not OK reply to a command message.
			<sender ID>
				ID of unit sending the reply.
			<reply to ID>
				ID of unit that sent the command and now shall have the reply.
			<reference number>
				Same number as was given in the command.
			<reason>
				See NOK_REASON_CODES  in msg.h
			<command>
				See COMMAND_CODES in msg.h
			<nok reply body>
				The rest of message depend on command message type,
				Not necessarily the same fields as in the OK reply message.
*/

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#if (defined __linux__) || (defined __WIN32)
#include "log.h"
#else
#include "Dbf.h"
#include "eeprom.h"
#endif
#include "messageUtilities.h"
#include "messageNames.h"


#define LOG_PREFIX "log "
#define LOG_SUFIX "\n"


const char* getMessageCommandName(COMMAND_CODES messageTypeCode)
{
	switch(messageTypeCode)
	{
		#if (defined __linux__) || (defined __WIN32)
		case UNKNOWN_CMD: return "UNKNOWN_COMMAND";
		case ECHO_CMD: return "ECHO_CMD";
		case GET_CMD: return "GET_CMD";
		case SET_CMD: return "SET_CMD";
		case IGNORE_VOLTAGE_SENSOR_CMD: return "IGNORE_VOLTAGE_SENSOR";
		case REBOOT_CMD: return "REBOOT_CMD";
		case SAVE_CMD: return "SAVE_CMD";
		case STOP_CMD: return "STOP_CMD";
		case START_CMD: return "START_CMD";
		case SHOW_VERSION_CMD: return "SHOW_VERSION";
		case RESTORE_DEFAULT_CMD: return "RESTORE_DEFAULT_CMD";
		case SEND_ALL_CONFIG2_CMD: return "SEND_ALL_CONFIG2_CMD";
		case SEND_ALL_CONFIG3_CMD: return "SEND_ALL_CONFIG3_CMD";
		case SEND_ALL_CONFIG1_CMD: return "SEND_ALL_CONFIG1_CMD";
		#endif
		default: break;
	}
	return NULL;
}


const char *getPortsStatusMessageName(PORTS_STATUS_SUB_MESSAGES statusCode)
{
	switch(statusCode)
	{
		case FAN_NOT_OK_STATUS: return "FAN_NOT_OK";
		case INT_TEMP_NOT_OK_STATUS: return "INT_TEMP_NOT_OK";
		case EXT_TEMP_NOT_OK_STATUS: return "EXT_TEMP_NOT_OK";
		case EXTERNAL_SENSOR_VOLTAGE_STATUS: return "EXTERNAL_SENSOR_VOLTAGE";
		case DRIVE_ERROR_STATUS: return "DRIVE_ERROR";
		case PAUSED_STATUS: return "PAUSED";
		case POWERING_UP: return "POWERING_UP";
		case STARTING_STATUS: return "STARTING";
		case SCANNING_STATUS: return "SCANNING";
		case IDLE_STATUS: return "IDLE_STATUS";
		case RUNNING_STATUS: return "RUNNING";
		case UNKNOWN_STATUS: return "UNKNOWN";
		case COOL_DOWN_STATUS: return "COOL_DOWN";
		case WAITING_FOR_POWER_STATUS: return "WAITING_FOR_POWER";
		case TIMEOUT_STATUS: return "TIMEOUT";
		case NO_RESONANSE_FOUND: return "NO_RESONANSE_FOUND";
		case FINE_TUNING: return "FINE_TUNING";
		case EXT_TEMP_OUT_OF_RANGE_STATUS: return "EXT_TEMP_OUT_OF_RANGE_STATUS";
		case RESETTING_STATUS: return "RESETTING";
		case DISCHARGING_STATUS: return "DISCHARGING";
		case CHARGING_STATUS: return "CHARGING";
		case RAMPING_UP: return "RAMPING_UP";
		case INTERLOCKING_LOOP_OPEN_STATUS: return "LOOP_OPEN";
		case TARGET_REACHED_OK: return "TARGET_REACHED_OK";
		case READY_TO_RUN: return "READY_TO_RUN";
		default: break;
	}
	return NULL;
}


const char *getStatusMessagesName(STATUS_MESSAGES statusCode)
{
	switch(statusCode)
	{
		#if (defined __linux__) || (defined __WIN32)
		case UNKNOWN_STATUS_MSG: return "UNKNOWN_STATUS";
		case REBOOT_STATUS_MSG: return "REBOOT_STATUS";
		case VOLTAGE_STATUS_MSG: return "VOLTAGE_STATUS";
		case OVER_VOLTAGE_STATUS_MSG: return "OVER_VOLTAGE";
		case LEAK_CURRENT_STATUS_MSG: return "LEAK_CURRENT_STATUS";
		case PARAMETER_STATUS_MSG: return "PARAMETER_STATUS";
		case TEMP_STATUS_MSG: return "TEMP_STATUS_MSG";
		case WEB_SERVER_STATUS_MSG: return "WEB_SERVER_STATUS_MSG";
		#endif
		default: break;
	}
	return NULL;
}


const char *getLogMessageName(int logMessageType)
{
	switch(logMessageType)
	{
	#if (defined __linux__) || (defined __WIN32)
	case LOG_FREQ_CURRENT_VOLT_HBOX: return "HBOX_FREQ_CURRENT_VOLT";
	case MAIN_LOOP_BEGIN: return "MAIN_LOOP_BEGIN";
	case LOG_MEASUREMENT: return "LOG_MEASUREMENT";
	case LOG_FREQ_CURRENT_VOLT_BUCK: return "LOG_FREQ_CURRENT_VOLT_BUCK";
	case LOG_FREQ_CURRENT_VOLT_INV: return "LOG_FREQ_CURRENT_VOLT_INV";
	case CMD_VOLT_VALUE_IS_TO_OLD: return "CMD_VOLT_VALUE_IS_TO_OLD";
	case CMD_VOLTAGE_SENSOR_DETECTED: return "CMD_VOLTAGE_SENSOR_DETECTED";
	case CMD_TARGET_REACHED: return "CMD_TARGET_REACHED";
	case UNEXPECTED_STRING_MESSAGE: return "UNEXPECTED_STRING_MESSAGE";
	case UNEXPECTED_EMPTY_MESSAGE: return "UNEXPECTED_EMPTY_MESSAGE";
	case UNKNOWN_DBF_MESSAGE: return "UNKNOWN_DBF_MESSAGE";
	case CMD_INIT: return "CMD_INIT";
	case CMD_INCORRECT_DBF_RECEIVED: return "CMD_INCORRECT_DBF_RECEIVED";
	case CMD_RESETTING: return "CMD_RESETTING";
	case EEPROM_WRONG_MAGIC_NUMBER: return "EEPROM_WRONG_MAGIC_NUMBER";
	case EEPROM_WRONG_CHECKSUM: return "EEPROM_WRONG_CHECKSUM";
	case EEPROM_LOADED_OK: return "EEPROM_LOADED_OK";
	case EEPROM_TRYING_BACKUP: return "EEPROM_TRYING_BACKUP";
	case EEPROM_BACKUP_LOADED_OK: return "EEPROM_BACKUP_LOADED_OK";
	case EEPROM_FAILED: return "EEPROM_FAILED";
	case EEPROM_SAVE_RESULT: return "EEPROM_SAVE_RESULT";
	case SCAN_FAN1_NOT_READY_YET: return "SCAN_FAN1_NOT_READY_YET";
	case SCAN_FAN2_NOT_READY_YET: return "SCAN_FAN2_NOT_READY_YET";
	case FAN_1_DETECTED: return "FAN_1_DETECTED";
	case FAN_2_DETECTED: return "FAN_2_DETECTED";
	case FAN_1_NOT_DETECTED: return "FAN_1_NOT_DETECTED";
	case FAN_2_NOT_DETECTED: return "FAN_2_NOT_DETECTED";
	case TEMP_SENSOR_DETECTED: return "TEMP_SENSOR_DETECTED";
	case TEMP_SENSOR_FAIL: return "TEMP_SENSOR_FAIL";
	case TEMP_SENSOR_OVERHEAT: return "TEMP_SENSOR_OVERHEAT";
	case TEMP_SENSOR_OK: return "TEMP_SENSOR_OK";
	case FAN_AND_TEMP_INIT: return "FAN_AND_TEMP_INIT";
	case FAN_AND_TEMP_COUNT_FAN2: return "FAN_AND_TEMP_COUNT_FAN2";
	case MEASURING_ZERO_LEVEL_OK: return "MEASURING_ZERO_LEVEL_OK";
	case MEASURING_TIMEOUT: return "MEASURING_TIMEOUT";
	case MEASURING_MORE_SENSITIVE_RANGE: return "MEASURING_MORE_SENSITIVE_RANGE";
	case MEASURING_LESS_SENSITIVE_RANGE: return "MEASURING_LESS_SENSITIVE_RANGE";
	case MEASURING_OVERLOAD_LOG: return "MEASURING_OVERLOAD";
	case MEASURING_INIT: return "MEASURING_INIT";
	case MEASURING_UNSTABLE: return "MEASURING_UNSTABLE";
	case MEASURING_INITIALIZED: return "MEASURING_INITIALIZED";
	case MEASURING_INIT_ADC: return "MEASURING_INIT_ADC";
	case MEASURING_OVER_VOLTAGE: return "MEASURING_OVER_VOLTAGE";
	case PORTS_GO: return "PORTS_GO";
	case PORTS_OVER_CURRENT: return "PORTS_OVER_CURRENT";
	case PORTS_DLATCH_STATE: return "PORTS_DLATCH_STATE";
	case PORTS_ERROR: return "PORTS_ERROR";
	case PORTS_INIT: return "PORTS_INIT";
	case PORTS_WAITING_FOR_ISR: return "PORTS_WAITING_FOR_ISR";
	case PORTS_WAITING_FOR_MEASURING_TEMP_FAN: return "PORTS_WAITING_FOR_MEASURING_TEMP_FAN";
	case PORTS_MISSED_BEGIN: return "PORTS_MISSED_BEGIN";
	case PORTS_WAITING_FOR_START_ORDER: return "PORTS_WAITING_FOR_START_ORDER";
	case REG_START: return "REG_START";
	case REG_STOP: return "REG_STOP";
	//case REG_TEMP_FAIL_LOG: return "REG_TEMP_FAIL";
	//case REG_FAN1_FAIL_LOG: return "REG_FAN1_FAIL";
	//case REG_FAN2_FAIL_LOG: return "REG_FAN2_FAIL";
	case REG_WAITING_FOR_PORTS: return "REG_WAITING_FOR_PORTS";
	case REG_WAITING_POWER_UP_CMD: return "REG_WAITING_POWER_UP_CMD";
	case REG_PORTS_ON: return "REG_PORTS_ON";
	case REG_INC_LOG: return "REG_INC";
	case REG_DEC_LOG: return "REG_DEC";
	case REG_ON_TARGET_LOG: return "REG_ON_TARGET";
	case REG_ADJUSTED_STEP: return "REG_ADJUSTED_STEP";
	case REG_CHANGE_STATE: return "REG_CHANGE_STATE";
	case SUPERVICE_START: return "SUPERVICE_START";
	case SUPERVICE_TO_LITTLE_CURRENT: return "SUPERVICE_TO_LITTLE_CURRENT";
	case SUPERVICE_TO_MUCH_CURRENT: return "SUPERVICE_TO_MUCH_CURRENT";
	case SUPERVICE_FREQUENCY_DRIFT: return "SUPERVICE_FREQUENCY_DRIFT";
	case SUPERVICE_INTERNAL_VOLTAGE_DRIFT: return "SUPERVICE_INTERNAL_VOLTAGE_DRIFT";
	case SUPERVICE_NOT_ENABLED: return "SUPERVICE_NOT_ENABLED";
	case SUPERVICE_TIMEOUT_STATUS_MSG: return "SUPERVICE_TIMEOUT_STATUS_MSG";
	case SEC_AND_LOG_INIT: return "SEC_AND_LOG_INIT";
	case SEC_WAITING_FOR_MEASURING: return "SEC_WAITING_FOR_MEASURING";
	case REG_TO_LITTLE_INPUT_VOLTAGE: return "REG_TO_LITTLE_INPUT_VOLTAGE";
	case REG_DEBUG_STATE: return "REG_DEBUG_STATE";
	case SCAN_Q: return "SCAN_Q";
	case SCAN_START: return "SCAN_START";
	case SCAN_HIGHER_BOUNDRY_FOUND: return "SCAN_HIGHER_BOUNDRY_FOUND";
	case SCAN_DID_NOT_FIND_UPPER_BOUNDRY: return "SCAN_DID_NOT_FIND_UPPER_BOUNDRY";
	case SCAN_LOWER_BOUNDRY_FOUND: return "SCAN_LOWER_BOUNDRY_FOUND";
	case SCAN_DID_NOT_FIND_LOWER_BOUNDRY: return "SCAN_DID_NOT_FIND_LOWER_BOUNDRY";
	case SCAN_FAILED_TO_FIND_Q: return "SCAN_FAILED_TO_FIND_Q";
	case SCAN_SWITCHING_TO_FINE_TUNING: return "SCAN_SWITCHING_TO_FINE_TUNING";
	case SCAN_LEAVING_FINE_TUNING: return "SCAN_LEAVING_FINE_TUNING";
	case SCAN_START_TUNING: return "SCAN_START_TUNING";
	case SCAN_TUNING_DID_NOT_DETECT_DROP: return "SCAN_TUNING_DID_NOT_DETECT_DROP";
	case SCAN_TUNING_STOP_HIGH: return "SCAN_TUNING_STOP_HIGH";
	case SCAN_CHANGE_TO_FREQUENCY_REGULATION: return "SCAN_CHANGE_TO_FREQUENCY_REGULATION";
	case SCAN_CHANGE_STATE: return "SCAN_CHANGE_STATE";
	case SCAN_NEED_A_SAVED_CONFIGURATION: return "SCAN_NEED_A_SAVED_CONFIGURATION";
	case SCAN_PORTS_NOT_READY_YET: return "SCAN_PORTS_NOT_READY_YET";
	case SCAN_FANS_NOT_READY_YET: return "SCAN_FANS_NOT_READY_YET";
	case SCAN_VOLTAGE_SENSOR_NOT_READY_YET: return "SCAN_VOLTAGE_SENSOR_NOT_READY_YET";
	case SCAN_WANTED_CURRENT_IS_ZERO: return "SCAN_WANTED_CURRENT_IS_ZERO";
	case SCAN_WANTED_VOLTAGE_IS_ZERO: return "SCAN_WANTED_VOLTAGE_IS_ZERO";
	case SCAN_WAITING_FOR_START_COMMAND: return "SCAN_WAITING_FOR_START_COMMAND";
	case SCAN_DID_NOT_FIND_RESONANCE: return "SCAN_DID_NOT_FIND_RESONANCE";
	case SCAN_PRELIMINARY_RESONANCE_FOUND: return "SCAN_PRELIMINARY_RESONANCE_FOUND";
	case SCAN_NOW_AT_FREQ: return "SCAN_NOW_AT_FREQ";
	case SCAN_MEASURING_RESONANCE_WIDTH: return "SCAN_MEASURING_RESONANCE_WIDTH";
	case SCAN_STOP_LOW: return "SCAN_STOP_LOW";
	case SCAN_TUNE_TO_LOW: return "SCAN_TUNE_TO_LOW";
	case SCAN_TUNE_DOWN: return "SCAN_TUNE_DOWN";
	case SCAN_TUNE_UP: return "SCAN_TUNE_UP";
	case SCAN_TUNE_STAY: return "SCAN_TUNE_STAY";
	case SCAN_WAITING_FOR_MEASURING: return "SCAN_WAITING_FOR_MEASURING";
	case SCAN_TO_LOW_FREQUENY: return "SCAN_TO_LOW_FREQUENY";
	case SCAN_INC_PERIOD: return "SCAN_INC_PERIOD";
	case SCAN_DEC_PERIOD: return "SCAN_DEC_PERIOD";
	case SCAN_ON_TARGET: return "SCAN_ON_TARGET";
	case SCAN_Q_LARGER_THAN_EXPECTED: return "SCAN_Q_LARGER_THAN_EXPECTED";
	case SCAN_Q_LESS_THAN_EXPECTED: return "SCAN_Q_LESS_THAN_EXPECTED";
	case SCAN_ENOUGH_WHILE_SCANNING: return "SCAN_ENOUGH_WHILE_SCANNING";
	case SCAN_COOLING_DOWN_WILL_START_SOON: return "SCAN_COOLING_DOWN_WILL_START_SOON";
	case SCAN_WAITING_FOR_SOMETHING_ELSE: return "SCAN_WAITING_FOR_SOMETHING_ELSE";
	case SCAN_TEMP_NOT_READY_YET: return "SCAN_TEMP_NOT_READY_YET";
	case SCAN_WAITING_FOR_REG: return "SCAN_WAITING_FOR_REG";
	case SCAN_SWITCHING_TO_DUTY_CYCLE: return "SCAN_SWITCHING_TO_DUTY_CYCLE";
	case SCAN_WAITING_FOR_PSU: return "SCAN_WAITING_FOR_PSU";
	case SCAN_INTERNAL_ERROR: return "SCAN_INTERNAL_ERROR";
	case SCAN_TRIPPED_ALREADY_CLEAR: return "SCAN_TRIPPED_ALREADY_CLEAR";
	case REG_PORTS_OFF: return "REG_PORTS_OFF";
	case REG_COUNTER: return "REG_COUNTER";
	case REG_COOLING_DOWN: return "REG_COOLING_DOWN";
	case REG_COOLING_DOWN_MV: return "REG_COOLING_DOWN_MV";
	case REG_COOLING_DOWN_S: return "REG_COOLING_DOWN_S";
	case REG_NO_MEASURING: return "REG_NO_MEASURING";
	case REG_MAX_MIN: return "REG_MAX_MIN";
	case COMP_STATE_CHANGED: return "COMP_STATE_CHANGED";
	case REG_MEASURED_CURRENT_MORE_THAN_WANTED_MV: return "REG_MEASURED_CURRENT_MORE_THAN_WANTED_MV";
	case REG_WAITING_POWER_UP_VOLT_CMD: return "REG_WAITING_POWER_UP_VOLT_CMD";
	case REG_PWM_BREAK: return "REG_PWM_BREAK";
	case REG_WAITING_POWER_UP_CURRENT_CMD: return "REG_WAITING_POWER_UP_CURRENT_CMD";
	case COMP_LOGIC_POWER_OK: return "COMP_LOGIC_POWER_OK";
	case COMP_LOGIC_POWER_LOST: return "COMP_LOGIC_POWER_LOST";
	case REG_FAILED_TO_SET_PWM_DUTY: return "REG_FAILED_TO_SET_PWM_DUTY";
	case REG_DID_NOT_SET_WANTED_CURRENT: return "REG_DID_NOT_SET_WANTED_CURRENT";
	case REG_DID_NOT_SET_WANTED_VOLTAGE: return "REG_DID_NOT_SET_WANTED_VOLTAGE";
	case PWM_DID_NOT_SET_CCR1: return "PWM_DID_NOT_SET_CCR1";
	case PWM_STATE_CHANGE: return "PWM_STATE_CHANGE";
	case SCAN_WAITING_FOR_POWER_TO_SCAN_WITH: return "SCAN_WAITING_FOR_POWER_TO_SCAN_WITH";
	case SCAN_SCANNING_FOR_RESONANCE: return "SCAN_SCANNING_FOR_RESONANCE";
	case COMP_COUNT: return "COMP_COUNT";
	case SCAN_TEMP1_NOT_READY_YET: return "SCAN_TEMP1_NOT_READY_YET";
	case SCAN_EXT_TEMP_NOT_READY_YET: return "SCAN_EXT_TEMP_NOT_READY_YET";
	case REG_WAITING_FOR_INPUT_POWER: return "REG_WAITING_FOR_INPUT_POWER";
	case SCAN_TAGET_REACHED: return "SCAN_TAGET_REACHED";
	case MEASURING_TOP_INTERNAL_VOLTAGE_MV: return "MEASURING_TOP_INTERNAL_VOLTAGE_MV";
	case MEASURING_TOP_CURRENT_MA: return "MEASURING_TOP_CURRENT_MA";
	case ERROR_REPORT: return "ERROR_REPORT";
	case ERROR_ASSERT: return "ERROR_ASSERT";
	case SCAN_ALREADY_HAVE_RESONANCE_FREQUENCY: return "SCAN_ALREADY_HAVE_RESONANCE_FREQUENCY";
	case SCAN_TOO_SMALL_INTERVALL_FOR_SCANNING: return "SCAN_TOO_SMALL_INTERVALL_FOR_SCANNING";
	case SCAN_START_SCANNING_RESONANCE_FREQUENCY: return "SCAN_START_SCANNING_RESONANCE_FREQUENCY";
	case SCAN_PARAMETER_OUT_OF_RANGE: return "SCAN_PARAMETER_OUT_OF_RANGE";
	case REG_WANTED_CURRENT_MA: return "REG_WANTED_CURRENT_MA";
	case SCAN_ABORTING_RUN: return "SCAN_ABORTING_RUN";
	case REG_WANTED_VOLTAGE_MA: return "REG_WANTED_VOLTAGE_MA";
	case CMD_BUCK_TIMEOUT: return "CMD_BUCK_TIMEOUT";
	case REG_CHARGING_INTERNAL_CAPACITORS: return "REG_CHARGING_INTERNAL_CAPACITORS";
	case PORTS_STATE_CHANGED: return "PORTS_STATE_CHANGED";
	case SCAN_WAITING_FOR_RELAY: return "SCAN_WAITING_FOR_RELAY";
	case SCAN_WAITING_FOR_POWER_DOWN: return "SCAN_WAITING_FOR_POWER_DOWN";
	case MEASURING_OVER_CURRENT: return "MEASURING_OVER_CURRENT";
	case MEASURING_OVER_VOLTAGE_MV: return "MEASURING_OVER_VOLTAGE_MV";
	case MEASURING_OVER_VOLTAGE_INPUT_MV: return "MEASURING_OVER_VOLTAGE_INPUT_MV";
	case MEASURING_OVER_VOLTAGE_INTERNAL_MV: return "MEASURING_OVER_VOLTAGE_INTERNAL_MV";
	case CMD_OVER_VOLTAGE_EXTERNAL: return "CMD_OVER_VOLTAGE_EXTERNAL";
	case MEASURING_OVER_CURRENT_MA: return "MEASURING_OVER_CURRENT_MA";
	case RELAY_STATE: return "RELAY_STATE";
	case SCAN_WAITING_FOR_VOLTAGE_TO_SCAN_WITH: return "SCAN_WAITING_FOR_VOLTAGE_TO_SCAN_WITH";
	case SUPERVICE_INTERNAL_VOLTAGE_DROP: return "SUPERVICE_INTERNAL_VOLTAGE_DROP";
	case MEASURING_OVER_AC_CURRENT: return "MEASURING_OVER_AC_CURRENT";
	case MEASURING_TOP_AC_CURRENT_MA: return "MEASURING_TOP_AC_CURRENT_MA";
	case MEASURING_OVER_AC_CURRENT_MA: return "MEASURING_OVER_AC_CURRENT_MA";
	case RELAY_ORDER: return "RELAY_ORDER";
	case SUPERVICE_CURRENT_DROP: return "SUPERVICE_CURRENT_DROP";
	case SUPERVICE_TO_MUCH_VOLTAGE: return "SUPERVICE_TO_MUCH_VOLTAGE";
	case SCAN_NO_LOAD_DETECTED: return "SCAN_NO_LOAD_DETECTED";
	case INTERLOCKING_LOOP_IS_OPEN: return "INTERLOCKING_LOOP_IS_OPEN";
	case TO_FAST_CURRENT_INC: return "TO_FAST_CURRENT_INC";
	case EEPROM_SAVE_PENDING_FLAG_SET: return "EEPROM_SAVE_PENDING_FLAG_SET";
	case EEPROM_PERFOMING_PENDING_SAVE: return "EEPROM_PERFOMING_PENDING_SAVE";
	case VOLTAGE_SENSOR_FAIL: return "VOLTAGE_SENSOR_FAIL";
	case RECEIVED_AC_VOLTAGE: return "RECEIVED_AC_VOLTAGE";
	case EXT_TEMP_STATE_CHANGE: return "EXT_TEMP_STATE_CHANGE";
	//case TEMPERATURE_NOT_RISING: return "TEMPERATURE_NOT_RISING";
	//case TEMPERATURE_HAS_FALLEN: return "TEMPERATURE_HAS_FALLEN";
	case WEB_STATUS_RECEIVED: return "WEB_STATUS_RECEIVED";
	case CMD_CONFIG: return "CMD_CONFIG";
	case LEAK_DETECTOR_TIMEOUT: return "LEAK_DETECTOR_TIMEOUT";
	case LEAK_DETECTOR_NOW_AVAILABLE: return "LEAK_DETECTOR_NOW_AVAILABLE";
	case EEPROM_DATA_UPGRADED: return "EEPROM_DATA_UPGRADED";
	#endif
	default: break;
	}
	return NULL;
}

/*
#if (defined __linux__) || (defined __WIN32)
const char* getVoltageLogMessageName(int logCode)
{
	switch(logCode)
	{
		case VOLTAGE_INITIAL_LOG: return "VOLTAGE_INITIAL";
		case VOLTAGE_INITIAL_ZERO_LOG: return "INITIAL_ZERO";
		case VOLTAGE_APPROVED_ZERO_LOG: return "APPROVED_ZERO";
		case VOLTAGE_TIMEOUT_LOG: return "VOLTAGE_TIMEOUT";
		case VOLTAGE_DEBUG_LOG: return "VOLTAGE_DEBUG";
		default: break;
	}
	return NULL;
}
#endif
*/

const char* getMessageCategoryName(MESSAGE_CATEGORY messageCatagory)
{
	switch(messageCatagory)
	{
		case UNKNOWN_CATEGORY: return "UNKNOWN_CATEGORY";
		case STATUS_CATEGORY: return "STATUS_CATEGORY";
		case LOG_CATEGORY: return "LOG_CATEGORY";
		case COMMAND_CATEGORY: return "COMMAND_CATEGORY";
		case REPLY_OK_CATEGORY: return "REPLY_OK_CATEGORY";
		case REPLY_NOK_CATEGORY: return "REPLY_NOK_CATEGORY";
		default: break;
	}
	return NULL;
}


const char *getPortsErrorName(int errorCode)
{
	switch(errorCode)
	{
		#if (defined __linux__) || (defined __WIN32)
		case portsErrorOk: return "OK"; // No error detected
		case portsErrorUnexpecetedMessage: return "UnexpecetedMessage";
		case portsErrorOverCurrent: return "OverCurrent"; // To high current
		case portsErrorScanningCouldNotFinish: return "ScanningCouldNotFinish";
		case portsErrorNoResonance: return "NoResonance"; // Did not detect the resonance frequency.
		case portsErrorUnexpectedCurrentDecrease: return "UnexpectedDecrease"; // We had enough current but lost it.
		//case portsErrorUnexpectedStateCompb: return "UnexpectedStateCompb";
		//case portsErrorUnexpectedStateCapt: return "UnexpectedStateCapt";
		case portsErrorMeasuringTimeout: return "MeasuringTimeout"; // No voltage measurement is available
		//case portsErrorPossibleShortCircuit: return "PossibleShortCircuit";
		case portsCoilsOverheated: return "portsCoilsOverheated";
		case portsErrorUnexpectedCurrentIncrease: return "UnexpectedIncrease";
		case portsErrorOverheated: return "Overheated";
		case portsErrorTempFail: return "TempFail";
		case portsErrorFanFail: return "FanFail";
		//case portsErrorStopCmd: return "ErrorStopCmd";
		case portsErrorOverVoltage: return "ErrorOverVoltage";
		case portsInternalError: return "InternalError";
		case portsError12VoltLost: return "Error12VoltLost";
		case portsErrorAssert: return "ErrorAssert";
		case portsTimerBreak: return "TimerBreak";
		//case portsErrorOverVoltageOnInput: return "ErrorOverVoltageOnInput";
		//case portsFailedToSetupAdc: return "FailedToSetupAdc";
		//case portsSensorBreak: return "SensorBreak";
		//case portsUncompatibleDetected: return "UncompatibleDetected";
		case portsAttemptToSetPeriodOutOfRange: return "AttemptToSetPeriodOutOfRange";
		case portsTargetExceededWhileWarmingUp: return "TargetExceededWhileWarmingUp";
		//case portsDidNotExpectRegRequestMessage: return "DidNotExpectRegRequestMessage";
		//case portsDidNotExpectRegStatusMessage: return "DidNotExpectRegStatusMessage";
		//case portsRegPwmBreak: return "RegPwmBreak";
		case portsInputVoltageDecrease: return "InputVoltageDecrease";
		case portsExtTempDiff: return "portsExtTempDiff";
		case portsErrorResonanceFrequencyToLow: return "ResonanceFrequencyToLow";
		case portsErrorResonanceFrequencyToHigh: return "ResonanceFrequencyToHigh";
		case portsErrorFrequencyDrift: return "FrequencyDrift";
		case portsErrorInternalVoltageDrift: return "InternalVoltageDrift";
		case portsPwmPortsBreakIsActive: return "PwmPortsBreakIsActive";
		//case portsErrorOverWantedCurrent: return "ErrorOverWantedCurrent";
		//case portsErrorOverWantedVoltage: return "ErrorOverWantedVoltage";
		case portsErrorOverOutputAcCurrent: return "OverOutputAcCurrent";
		case portsErrorOverAcCurrent: return "OverAcCurrent";
		case portsErrorLoopOpen: return "LoopOpen";
		case portsErrorCurrentLessThanRequired: return "CurrentLessThanRequired";
		case portsErrorCurrentMoreThanExpected: return "CurrentMoreThanExpected";
		case portsErrorCurrentMoreThanAllowed: return "CurrentMoreThanAllowed";
		case portsErrorOverDcCurrentSlow: return "OverDcCurrentSlow";
		case portsErrorLeakCurrentExceeded: return "LeakCurrentExceeded";
		case portsVoltageMeterNotReacting: return "VoltageMeterNotReacting";
		#endif
		default: break;
	}
	return NULL;
}

const char * getParameterName(PARAMETER_CODES parameterCode)
{
	switch(parameterCode)
	{
		#if (defined __linux__) || (defined __WIN32)
		case UNKNOWN_PAR: return "UNKNOWN_PAR";
		case AUTOSTART_PAR: return "AUTOSTART_PAR";
		case CYCLES_TO_DO: return "CYCLES_TO_DO";
		case DEVICE_ID: return "DEVICE_ID";
		case MICRO_AMPS_PER_UNIT_DC: return "MICRO_AMPS_PER_UNIT_DC";
		case MICRO_VOLTS_PER_UNIT_DC: return "MICRO_VOLTS_PER_UNIT_DC";
		case MACHINE_SESSION_NUMBER: return "MACHINE_SESSION_NUMBER";
		case SUPERVICE_DEVIATION_FACTOR: return "SUPERVICE_DEVIATION_FACTOR";
		case FAIL_SCAN_IF_TO_MUCH_CURRENT: return "FAIL_SCAN_IF_TO_MUCH_CURRENT";
		case WANTED_CURRENT_MA: return "WANTED_CURRENT_MA"; // This is AC (but it has been DC at some point)
		case WANTED_VOLTAGE_MV: return "WANTED_VOLTAGE_MV";
		case TOTAL_CYCLES_PERFORMED: return "TOTAL_CYCLES_PERFORMED";
		case MIN_Q_VALUE: return "MIN_Q_VALUE";
		case MAX_Q_VALUE: return "MAX_Q_VALUE";
		case MIN_TEMP_HZ: return "MIN_TEMP_HZ";
		case MAX_TEMP_HZ: return "MAX_TEMP_HZ";
		case TEMP_FREQ_AT_20C: return "TEMP_FREQ_AT_20C";
		case TEMP_FREQ_AT_100C: return "TEMP_FREQ_AT_100C";
		case WANTED_SCANNING_VOLTAGE_MV: return "WANTED_SCANNING_VOLTAGE_MV";
		case SCAN_BEGIN_HZ: return "SCAN_BEGIN_HZ";
		case SCAN_END_HZ: return "SCAN_END_HZ";
		case RESONANCE_HALF_PERIOD: return "RESONANCE_HALF_PERIOD";
		case Q_VALUE: return "Q_VALUE";
		case PORTS_PWM_STATE: return "PORTS_PWM_STATE";
		case TARGET_TIME_S: return "TARGET_TIME_S";
		case PORTS_PWM_HALF_PERIOD: return "PORTS_PWM_HALF_PERIOD";
		case SCAN_PEAK_WIDTH_IN_PERIOD_TIME: return "SCAN_PEAK_WIDTH_IN_PERIOD_TIME";
		case PORTS_PWM_ANGLE: return "PORTS_PWM_ANGLE";
		case INV_FREQUENCY_HZ: return "INV_FREQUENCY_HZ";
		case PORTS_PWM_MIN_ANGLE: return "PORTS_PWM_MIN_ANGLE";
		case COOLDOWN_TIME_S: return "COOLDOWN_TIME_S";
		case FILTERED_CURRENT_DC_MA: return "FILTERED_CURRENT_DC_MA";
		case FILTERED_INTERNAL_VOLTAGE_DC_MV: return "FILTERED_INTERNAL_VOLTAGE_DC_MV";
		case SUPERVICE_MARGIN_MV: return "SUPERVICE_MARGIN_MV";
		case WALL_TIME_RECEIVED_MS: return "WALL_TIME_RECEIVED_MS";
		case TIME_COUNTED_S: return "TIME_COUNTED_S";
		case WEB_SERVER_TIMEOUT_S: return "WEB_SERVER_TIMEOUT_S";
		case REPORTED_EXT_AC_VOLTAGE_MV: return "REPORTED_EXT_AC_VOLTAGE_MV";
		case SCAN_STATE_PAR: return "SCAN_STATE_PAR";
		case IS_PORTS_BREAK_ACTIVE: return "IS_PORTS_BREAK_ACTIVE";
		case MAX_EXT_TEMP_DIFF_C: return "MAX_EXT_TEMP_DIFF_C";
		case RELAY_STATE_PAR: return "RELAY_STATE_PAR";
		case TEMP1_C: return "TEMP1_C";
		case TEMP2_C: return "TEMP2_C";
		case FAN1_HZ: return "FAN1_HZ";
		case FAN2_HZ: return "FAN2_HZ";
		case MACHINE_REQUESTED_OPERATION: return "MACHINE_REQUESTED_OPERATION";
		case REPORTED_ERROR: return "REPORTED_ERROR";
		case SCAN_DELAY_MS: return "SCAN_DELAY_MS";
		case MAX_ALLOWED_CURRENT_MA: return "MAX_ALLOWED_CURRENT_MA";
		case MAX_ALLOWED_EXT_VOLT_MV: return "MAX_ALLOWED_EXT_VOLT_MV";
		case STABLE_OPERATION_TIMER_S: return "STABLE_OPERATION_TIMER_S";
		case SUPERVICE_STATE: return "SUPERVICE_STATE";
		case COMP_VALUE: return "COMP_VALUE";
		case DUTY_CYCLE_PPM: return "DUTY_CYCLE_PPM";
		case PORTS_CYCLE_COUNT: return "PORTS_CYCLE_COUNT";
		case MICRO_AMPS_PER_UNIT_AC: return "MICRO_AMPS_PER_UNIT_AC";
		case MEASURED_AC_CURRENT_MA: return "MEASURED_AC_CURRENT_MA";
		case SUPERVICE_STABILITY_TIME_S: return "SUPERVICE_STBILITY_TIME_S";
		case SUPERVICE_DROP_FACTOR: return "SUPERVICE_DROP_FACTOR";
		case PORTS_PWM_BRAKE_STATE: return "PORTS_PWM_BRAKE_STATE";
		case VOLTAGE_PROBE_FACTOR: return "VOLTAGE_PROBE_FACTOR";
		case RELAY_COOL_DOWN_REMAINING_S: return "RELAY_COOL_DOWN_REMAINING_S";
		case MEASURING_MAX_AC_CURRENT_MA: return "MEASURING_MAX_AC_CURRENT_MA";
		case MEASURING_MAX_DC_CURRENT_MA: return "MEASURING_MAX_DC_CURRENT_MA";
		case MEASURING_MAX_DC_VOLTAGE_MA: return "MEASURING_MAX_DC_VOLTAGE_MA";
		case LOW_DUTY_CYCLE_PERCENT: return "LOW_DUTY_CYCLE_PERCENT";
		case MAX_TEMP_C: return "MAX_TEMP_C";
		case MACHINE_STATE: return "MACHINE_STATE";
		case SYS_TIME_MS: return "SYS_TIME_MS";
		case SLOW_INTERNAL_VOLTAGE_MV: return "SLOW_INTERNAL_VOLTAGE_MV";
		case SUPERVICE_INC_FACTOR: return "SUPERVICE_INC_FACTOR";
		case SLOW_AC_CURRENT_MA: return "SLOW_AC_CURRENT_MA";
		case TEMP1_IN_HZ: return "TEMP1_IN_HZ";
		case EXT_TEMP1_C: return "EXT_TEMP1_C";
		case EXT_TEMP2_C: return "EXT_TEMP2_C";
		case MAX_EXT_TEMP_C: return "MAX_EXT_TEMP_C";
		case EXT_TEMP_REQUIRED: return "EXT_TEMP_REQUIRED";
		case EXT_VOLT_REQUIRED: return "EXT_VOLT_REQUIRED";
		case EXT_TEMP_STATE: return "EXT_TEMP_STATE";
		case LOOP_REQUIRED: return "LOOP_REQUIRED";
		case TARGET_CYCLES_0: return "TARGET_CYCLES_0";
		case TARGET_CYCLES_1: return "TARGET_CYCLES_1";
		case TARGET_CYCLES_2: return "TARGET_CYCLES_2";
		case TARGET_CYCLES_3: return "TARGET_CYCLES_3";
		//case TARGET_CYCLES_4: return "TARGET_CYCLES_4";
		//case TARGET_CYCLES_5: return "TARGET_CYCLES_5";
		//case TARGET_CYCLES_6: return "TARGET_CYCLES_6";
		//case TARGET_CYCLES_7: return "TARGET_CYCLES_7";
		case REACHED_CYCLES_0: return "REACHED_CYCLES_0";
		case REACHED_CYCLES_1: return "REACHED_CYCLES_1";
		case REACHED_CYCLES_2: return "REACHED_CYCLES_2";
		case REACHED_CYCLES_3: return "REACHED_CYCLES_3";
		//case REACHED_CYCLES_4: return "REACHED_CYCLES_4";
		//case REACHED_CYCLES_5: return "REACHED_CYCLES_5";
		//case REACHED_CYCLES_6: return "REACHED_CYCLES_6";
		//case REACHED_CYCLES_7: return "REACHED_CYCLES_7";
		case MAX_CURRENT_STEP: return "MAX_CURRENT_STEP";
		case MAX_VOLTAGE_STEP: return "MAX_VOLTAGE_STEP";
		case MAX_LEAK_CURRENT_MA: return "MAX_LEAK_CURRENT_MA";
		case LEAK_CURRENT_MA: return "LEAK_CURRENT_MA";
		case MIN_MV_PER_MA_AC: return "MIN_MV_PER_MA_AC";
		#endif
		default: break;
	}
	return NULL;
}

// TODO Perhaps we should check if last characters are numbers and
// if so treat the parameter as an array? That is for REACHED_CYCLES_2
// we should find parameter REACHED_CYCLES_0 and then add 2.
#if defined __linux__ || defined __WIN32
PARAMETER_CODES getParameterCodeFromWebName(const char* dataName)
{
	assert(dataName!=NULL);
	switch(dataName[0]) {
	case 'A':
		if (strcmp(dataName, "AUTOSTART_PAR") == 0)
		{
			return AUTOSTART_PAR;
		}
		break;
	case 'C':
		if (strcmp(dataName, "CYCLES_TO_DO") == 0)
		{
			return CYCLES_TO_DO;
		}
		if (strcmp(dataName, "COMP_VALUE") == 0)
		{
			return COMP_VALUE;
		}
		if (strcmp(dataName, "COOLDOWN_TIME_S") == 0)
		{
			return COOLDOWN_TIME_S;
		}
		break;
	case 'D':
		if (strcmp(dataName, "DEVICE_ID") == 0)
		{
			return DEVICE_ID;
		}
		if (strcmp(dataName, "DUTY_CYCLE_PPM") == 0)
		{
			return DUTY_CYCLE_PPM;
		}
		break;
	case 'E':
		if (strcmp("EXT_TEMP1_C", dataName) == 0)
		{
			return EXT_TEMP1_C;
		}
		if (strcmp("EXT_TEMP2_C", dataName) == 0)
		{
			return EXT_TEMP2_C;
		}
		if (strcmp("EXT_TEMP_REQUIRED", dataName) == 0)
		{
			return EXT_TEMP_REQUIRED;
		}
		if (strcmp("EXT_VOLT_REQUIRED", dataName) == 0)
		{
			return EXT_VOLT_REQUIRED;
		}
		if (strcmp("EXT_TEMP_STATE", dataName) == 0)
		{
			return EXT_TEMP_STATE;
		}
		break;
	case 'F':
		if (strcmp(dataName, "FILTERED_INTERNAL_VOLTAGE_DC_MV") == 0)
		{
			return FILTERED_INTERNAL_VOLTAGE_DC_MV;
		}
		if (strcmp(dataName, "FILTERED_CURRENT_DC_MA") == 0)
		{
			return FILTERED_CURRENT_DC_MA;
		}
		if (strcmp(dataName, "FAIL_SCAN_IF_TO_MUCH_CURRENT") == 0)
		{
			return FAIL_SCAN_IF_TO_MUCH_CURRENT;
		}
		break;
	case 'I':
		if (strcmp(dataName, "INV_FREQUENCY_HZ") == 0)
		{
			return INV_FREQUENCY_HZ;
		}
		break;

	case 'L':
		if (strcmp(dataName, "LOW_DUTY_CYCLE_PERCENT") == 0)
		{
			return LOW_DUTY_CYCLE_PERCENT;
		}
		if (strcmp(dataName, "LEAK_CURRENT_MA") == 0)
		{
			return LEAK_CURRENT_MA;
		}
		break;
	case 'M':
		if (strcmp("MICRO_VOLTS_PER_UNIT_DC", dataName) == 0)
		{
			return MICRO_VOLTS_PER_UNIT_DC;
		}
		else if (strcmp("MICRO_AMPS_PER_UNIT_DC", dataName) == 0)
		{
			return MICRO_AMPS_PER_UNIT_DC;
		}
		if (strcmp("MACHINE_SESSION_NUMBER", dataName) == 0)
		{
			return MACHINE_SESSION_NUMBER;
		}
		if (strcmp("MIN_TEMP_HZ", dataName) == 0)
		{
			return MIN_TEMP_HZ;
		}
		if (strcmp("MAX_TEMP_HZ", dataName) == 0)
		{
			return MAX_TEMP_HZ;
		}
		else if (strcmp("MICRO_AMPS_PER_UNIT_AC", dataName) == 0)
		{
			return MICRO_AMPS_PER_UNIT_AC;
		}
		else if (strcmp("MAX_TEMP_C", dataName) == 0)
		{
			return MAX_TEMP_C;
		}
		else if (strcmp("MACHINE_STATE", dataName) == 0)
		{
			return MACHINE_STATE;
		}
		else if (strcmp("MEASURED_AC_CURRENT_MA", dataName) == 0)
		{
			return MEASURED_AC_CURRENT_MA;
		}
		else if (strcmp("MEASURING_MAX_AC_CURRENT_MA", dataName) == 0)
		{
			return MEASURING_MAX_AC_CURRENT_MA;
		}
		else if (strcmp("MEASURING_MAX_DC_CURRENT_MA", dataName) == 0)
		{
			return MEASURING_MAX_DC_CURRENT_MA;
		}
		else if (strcmp("MEASURING_MAX_DC_VOLTAGE_MA", dataName) == 0)
		{
			return MEASURING_MAX_DC_VOLTAGE_MA;
		}
		else if (strcmp("MAX_EXT_TEMP_C", dataName) == 0)
		{
			return MAX_EXT_TEMP_C;
		}
		else if (strcmp("MAX_LEAK_CURRENT_MA", dataName) == 0)
		{
			return MAX_LEAK_CURRENT_MA;
		}
		break;
	case 'P':
		if (strcmp("PORTS_PWM_BRAKE_STATE", dataName) == 0)
		{
			return PORTS_PWM_BRAKE_STATE;
		}
		if (strcmp("PORTS_CYCLE_COUNT", dataName) == 0)
		{
			return PORTS_CYCLE_COUNT;
		}
		if (strcmp("PORTS_PWM_ANGLE", dataName) == 0)
		{
			return PORTS_PWM_ANGLE;
		}
		if (strcmp("PORTS_PWM_HALF_PERIOD", dataName) == 0)
		{
			return PORTS_PWM_HALF_PERIOD;
		}
		break;
	case 'Q':
		if (strcmp("Q_VALUE", dataName) == 0)
		{
			return Q_VALUE;
		}
	case 'R':
		if (strcmp("RELAY_COOL_DOWN_REMAINING_S", dataName) == 0)
		{
			return RELAY_COOL_DOWN_REMAINING_S;
		}
		if (strcmp("REPORTED_EXT_AC_VOLTAGE_MV", dataName) == 0)
		{
			return REPORTED_EXT_AC_VOLTAGE_MV;
		}
		if (strcmp("REACHED_CYCLES_0", dataName) == 0)
		{
			return REACHED_CYCLES_0;
		}
		if (strcmp("REACHED_CYCLES_1", dataName) == 0)
		{
			return REACHED_CYCLES_1;
		}
		if (strcmp("REACHED_CYCLES_2", dataName) == 0)
		{
			return REACHED_CYCLES_2;
		}
		if (strcmp("REACHED_CYCLES_3", dataName) == 0)
		{
			return REACHED_CYCLES_3;
		}
		if (strcmp("RESONANCE_HALF_PERIOD", dataName) == 0)
		{
			return RESONANCE_HALF_PERIOD;
		}
		break;
	case 'S':
		if (strcmp(dataName, "SCAN_BEGIN_HZ") == 0)
		{
			return SCAN_BEGIN_HZ;
		}
		if (strcmp(dataName, "SCAN_END_HZ") == 0)
		{
			return SCAN_END_HZ;
		}
		if (strcmp(dataName, "SUPERVICE_DEVIATION_FACTOR") == 0)
		{
			return SUPERVICE_DEVIATION_FACTOR;
		}
		if (strcmp(dataName, "SCAN_DELAY_MS"))
		{
			return SCAN_DELAY_MS;
		}
		if (strcmp(dataName, "SUPERVICE_STABILITY_TIME_S"))
		{
			return SUPERVICE_STABILITY_TIME_S;
		}
		/*if (strcmp(dataName, "SEC_AND_LOG_SECONDS"))
		{
			return SEC_AND_LOG_SECONDS;
		}*/
		if (strcmp(dataName, "SLOW_AC_CURRENT_MA"))
		{
			return SLOW_AC_CURRENT_MA;
		}
		if (strcmp(dataName, "SLOW_INTERNAL_VOLTAGE_MV"))
		{
			return SLOW_INTERNAL_VOLTAGE_MV;
		}
		if (strcmp(dataName, "SCAN_STATE_PAR"))
		{
			return SCAN_STATE_PAR;
		}
		break;
	case 'T':
		if (strcmp("TOTAL_CYCLES_PERFORMED", dataName) == 0)
		{
			return TOTAL_CYCLES_PERFORMED;
		}
		/*if (strcmp("TEMP_MILLI_DEGREES_PER_HZ", dataName) == 0)
		{
			return TEMP_MILLI_DEGREES_PER_HZ_PAR;
		}
		if (strcmp("TEMP_MILLI_DEGREES_OFFSET", dataName) == 0)
		{
			return TEMP_MILLI_DEGREES_OFFSET_PAR;
		}*/
		/*if (strcmp("TEMP_FREQ_AT_20C", dataName) == 0)
		{
			return TEMP_FREQ_AT_20C_PAR;
		}
		if (strcmp("TEMP_FREQ_AT_100C", dataName) == 0)
		{
			return TEMP_FREQ_AT_100C_PAR;
		}*/
		if (strcmp("TEMP1_IN_HZ", dataName) == 0)
		{
			return TEMP1_IN_HZ;
		}
		if (strcmp("TEMP1_C", dataName) == 0)
		{
			return TEMP1_C;
		}
		if (strcmp("TEMP2_C", dataName) == 0)
		{
			return TEMP2_C;
		}
		if (strcmp("TARGET_TIME_S", dataName) == 0)
		{
			return TARGET_TIME_S;
		}
		break;
	/*case 't':
		if (strcmp("target_time_s", dataName) == 0)
		{
			return TARGET_TIME_S;
		}
		break;*/
	case 'V':
		if (strcmp("VOLTAGE_PROBE_FACTOR", dataName) == 0)
		{
			return VOLTAGE_PROBE_FACTOR;
		}
		break;
	case 'W':
		if (strcmp("WANTED_VOLTAGE_MV", dataName) == 0)
		{
			return WANTED_VOLTAGE_MV;
		}
		if (strcmp("WANTED_CURRENT_MA", dataName) == 0)
		{
			return WANTED_CURRENT_MA;
		}
		if (strcmp("WANTED_SCANNING_VOLTAGE_MV", dataName) == 0)
		{
			return WANTED_SCANNING_VOLTAGE_MV;
		}
		if (strcmp("WANTED_SCANING_VOLTAGE_MV", dataName) == 0) // Allow incorrect spelling for backward compatibility
		{
			return WANTED_SCANNING_VOLTAGE_MV;
		}
		if (strcmp("WALL_TIME_RECEIVED_MS", dataName) == 0)
		{
			return WALL_TIME_RECEIVED_MS;
		}
		if (strcmp("WEB_SERVER_TIMEOUT_S", dataName) == 0)
		{
			return WEB_SERVER_TIMEOUT_S;
		}
		break;
	default:
		break;
	}

	// We should not need this one but sometimes a parameter is forgotten above.
	for (int i = 0; i< LOGGING_MAX_NOF_PARAMETERS; i++)
	{
		const char *tmp = getParameterName(i);
		if ((tmp!=NULL) && (strcmp(tmp, dataName) == 0) && ((logWarnings++)<10))
		{
			printf("parameter '%s' found in linear search to be %d\n", tmp, i);
			return i;
		}
	}

	return UNKNOWN_PAR;
}


int logErrorMaskToString(char *bufPtr, int bufSize, uint16_t errorMask)
{
	if (errorMask&1)
	{
		snprintf(bufPtr, bufSize, " Fan1_fail");
		int n = strlen(bufPtr);
		bufPtr += n;
		bufSize -= n;
	}
	if (errorMask&2)
	{
		snprintf(bufPtr, bufSize, " Fan2_fail");
		int n = strlen(bufPtr);
		bufPtr += n;
		bufSize -= n;
	}
	if (errorMask&4)
	{
		snprintf(bufPtr, bufSize, " Temp_fail");
		int n = strlen(bufPtr);
		bufPtr += n;
		bufSize -= n;
	}
	if (errorMask&8)
	{
		snprintf(bufPtr, bufSize, " VoltSensor_fail");
		int n = strlen(bufPtr);
		bufPtr += n;
		bufSize -= n;
	}
	return strlen(bufPtr);
}

int logPortsErrorToString(char *bufPtr, int bufSize, uint16_t errorCode)
{
	const char* name=getPortsErrorName(errorCode);
	if (name!=NULL)
	{
		snprintf(bufPtr, bufSize, " %s", name);
	}
	else
	{
		snprintf(bufPtr, bufSize, " portsError_%d", errorCode);
	}
	return strlen(bufPtr);
}


void logFreqCurrVoltToStringInverter(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	int64_t frequency_Hz = DbfUnserializerReadInt64(dbfUnserializer);
	int64_t measuredAcCurrent_mA = DbfUnserializerReadInt64(dbfUnserializer);
	int64_t measuredExternalAcVoltage_mV = DbfUnserializerReadInt64(dbfUnserializer);
	int64_t temp1_C = DbfUnserializerReadInt64(dbfUnserializer);
	int64_t extTemp1_C = DbfUnserializerReadInt64(dbfUnserializer);
	int64_t extTemp2_C = DbfUnserializerReadInt64(dbfUnserializer);
	int64_t measuredDcCurrent_mA = DbfUnserializerReadInt64(dbfUnserializer);
	/*int64_t wantedVoltage_mV =*/ DbfUnserializerReadInt64(dbfUnserializer);
	int64_t measuredInternalDcVoltage_mV = DbfUnserializerReadInt64(dbfUnserializer);

	int64_t portsRegulationState = DbfUnserializerReadInt64(dbfUnserializer);
	int64_t scanGetState = DbfUnserializerReadInt64(dbfUnserializer);

	int64_t angle = DbfUnserializerReadInt64(dbfUnserializer);
	int64_t scanGetHalfPeriod = DbfUnserializerReadInt64(dbfUnserializer);

	/*int64_t wantedCurrent_mv =*/ DbfUnserializerReadInt64(dbfUnserializer);
	int64_t errorMask = DbfUnserializerReadInt64(dbfUnserializer);
	int64_t portsErrors = DbfUnserializerReadInt64(dbfUnserializer);

	/*int64_t spare1 =*/ DbfUnserializerReadInt64(dbfUnserializer);
	int64_t throttle = DbfUnserializerReadInt64(dbfUnserializer);

	#ifndef __WIN32
	snprintf(bufPtr, bufSize, "%lldHz %lldmA %lldmV %lldC %lldC %lldC %lld.%lld %lld:%lld/%lld %lldmA %lldmV",
	#else
	snprintf(bufPtr, bufSize, "%I64dHz %I64dmA %I64dmV %I64dC %I64d.%I64d %I64d:%I64d/%I64d %I64dmA %I64dmV",
	#endif
			(long long int)frequency_Hz,
			(long long int)measuredAcCurrent_mA,
			(long long int)measuredExternalAcVoltage_mV,
			(long long int)temp1_C,
			(long long int)extTemp1_C,
			(long long int)extTemp2_C,
			(long long int)scanGetState,
			(long long int)portsRegulationState,
			(long long int)throttle,
			(long long int)angle,
			(long long int)scanGetHalfPeriod,
			(long long int)measuredDcCurrent_mA,
			(long long int)measuredInternalDcVoltage_mV);
	int n = strlen(bufPtr);
	bufPtr += n;
	bufSize -= n;

	n=logErrorMaskToString(bufPtr, bufSize, errorMask);
	bufPtr += n;
	bufSize -= n;

	n=logPortsErrorToString(bufPtr, bufSize, portsErrors);
	bufPtr += n;
	bufSize -= n;

	// All that is left is unpacked and logged in raw ascii format, if any.
	if (!DbfUnserializerReadIsNextEnd(dbfUnserializer))
	{
		char tmp[4096];
		DbfUnserializerReadAllToString(dbfUnserializer, tmp, sizeof(tmp));
		snprintf(bufPtr, bufSize, " debugInfo='%s'", tmp);
	}
}

void logStatusMessageToString(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	if ((bufPtr == NULL) || (bufSize<=0) || (bufSize >= 0x70000000))
	{
		return;
	}

	/*const int64_t senderId =*/ DbfUnserializerReadInt64(dbfUnserializer);
	const int statusMsgType = DbfUnserializerReadInt32(dbfUnserializer);

	const char* statusMsgTypeName = getStatusMessagesName(statusMsgType);

	switch(statusMsgType)
	{
		case TARGET_GENERAL_STATUS_MSG:
		{
			long long int superviceSeconds = DbfUnserializerReadInt64(dbfUnserializer);
			long long int statusCode = DbfUnserializerReadInt64(dbfUnserializer);
			long long int frequency_Hz = DbfUnserializerReadInt64(dbfUnserializer);
			long long int measuredAcCurrent_mA = DbfUnserializerReadInt64(dbfUnserializer);
			long long int portsCycleCount = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t wallTimeReceived_ms = DbfUnserializerReadInt64(dbfUnserializer);

			snprintf(bufPtr, bufSize, "tgt %lldS, %lld, %lldHz, %lldmA, %lld, %lldms",
					superviceSeconds,
					statusCode,
					frequency_Hz,
					measuredAcCurrent_mA,
					portsCycleCount,
					(long long int)wallTimeReceived_ms);

			break;
		}
		case VOLTAGE_STATUS_MSG:
		{
			/*long long int time_ms =*/ DbfUnserializerReadInt64(dbfUnserializer);
			int64_t voltage_mV = DbfUnserializerReadInt64(dbfUnserializer);

			snprintf(bufPtr, bufSize, "AC voltage %lldmV", (long long int)voltage_mV);

			break;
		}
		case TEMP_STATUS_MSG:
		{
			/*long long int time_ms =*/ DbfUnserializerReadInt64(dbfUnserializer);
			int64_t sensorNumber = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t tempValue_C = DbfUnserializerReadInt64(dbfUnserializer);
			snprintf(bufPtr, bufSize, "Temp%lld %lld", (long long int)sensorNumber, (long long int)tempValue_C);
			break;
		}
		case LEAK_CURRENT_STATUS_MSG:
		{
			/*long long int time_ms =*/ DbfUnserializerReadInt64(dbfUnserializer);
			int64_t current_mV = DbfUnserializerReadInt64(dbfUnserializer);
			snprintf(bufPtr, bufSize, "leak current %lldmA", (long long int)current_mV);
			break;
		}
		default:
		{
			char tmpStr[4096];
			DbfUnserializerReadAllToString(dbfUnserializer, tmpStr, sizeof(tmpStr));

			if (statusMsgTypeName!=NULL)
			{
				snprintf(bufPtr, bufSize, "  %s %s", statusMsgTypeName, tmpStr);
			}
			else
			{
				snprintf(bufPtr, bufSize, "  STATUS_CODE_%d %s", statusMsgType, tmpStr);
			}
			break;
		}
	}
}

void logCommandMessageToString(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	if ((bufSize<=0) || (bufSize >= 0x70000000))
	{
		return;
	}

	const long long int senderId = DbfUnserializerReadInt64(dbfUnserializer);
	const long long int destId = DbfUnserializerReadInt64(dbfUnserializer);
	const long long int refNr = DbfUnserializerReadInt64(dbfUnserializer);
	const int cmdMsgType = DbfUnserializerReadInt32(dbfUnserializer);

	const char* statusMsgTypeName = getMessageCommandName(cmdMsgType);
	char name[256];
	if (statusMsgTypeName!=NULL)
	{
		snprintf(name, sizeof(name), "%s", statusMsgTypeName);
	}
	else
	{
		snprintf(name, sizeof(name), "cmd%d", cmdMsgType);
	}

	char tmp[512];
	if (destId==-1)
	{
		snprintf(tmp, sizeof(tmp), "from %lld, to all, #%lld, %s", senderId, refNr, name);
	}
	else
	{
		snprintf(tmp, sizeof(tmp), "from %lld, to %lld, #%lld, %s", senderId, destId, refNr, name);
	}

	switch(cmdMsgType)
	{
		case SET_CMD:
		{
			const int parId = DbfUnserializerReadInt32(dbfUnserializer);
			const char* parameterName = getParameterName(parId);
			if (parameterName)
			{
				snprintf(bufPtr, bufSize, "%s, %s", tmp, parameterName);
			}
			else
			{
				snprintf(bufPtr, bufSize, "%s, parameter%d", tmp, cmdMsgType);
			}
			break;
		}
		default:
		{
			snprintf(bufPtr, bufSize, "%s,", tmp);
			break;
		}
	}
}

void log_message_to_string(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	if ((bufPtr == NULL) || (bufSize<=0) || (bufSize >= 0x70000000))
	{
		return;
	}

	/*const int senderId =*/ DbfUnserializerReadInt64(dbfUnserializer);
	/*const int sequenceNr =*/ DbfUnserializerReadInt64(dbfUnserializer);
	// TODO check if we miss a log message by looking at sequence number.

	const int logMessageType = DbfUnserializerReadInt64(dbfUnserializer);

	//printf("log_message_to_string %d\n", logMessageType);

	char tmp[80];
	const char* tmpPtr=getLogMessageName(logMessageType);
	if (tmpPtr!=NULL)
	{
		snprintf(tmp, sizeof(tmp), "%s", tmpPtr);
	}
	else
	{
		snprintf(tmp, sizeof(tmp), "LogMsg%d", logMessageType);
	}


	switch(logMessageType)
	{
		/*case LOG_FREQ_CURRENT_VOLT_HBOX:
		case LOG_FREQ_CURRENT_VOLT_BUCK:
			logOldFormatsFreqCurrVoltToString(dbfUnserializer, bufPtr, bufSize);
			break;*/
		case LOG_FREQ_CURRENT_VOLT_INV:
			logFreqCurrVoltToStringInverter(dbfUnserializer, bufPtr, bufSize);
			break;

		case SCAN_DID_NOT_FIND_RESONANCE:
		{
			int64_t scanTopPreliminaryCurrentFound = DbfUnserializerReadInt64(dbfUnserializer);
			DbfUnserializerReadInt64(dbfUnserializer);
			int64_t meanValue = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: top current %lld (proprietary units), mean current %lld (proprietary units). Perhaps load is not correctly connected or no input power?", tmp, (long long int)scanTopPreliminaryCurrentFound, (long long int)meanValue);
			#else
			snprintf(bufPtr, bufSize, "%s: top current %I64d (proprietary units), mean current %I64d (proprietary units). Perhaps load is not correctly connected or no input power?", tmp, (long long int)scanTopPreliminaryCurrentFound, (long long int)meanValue);
			#endif
			break;
		}
		case SCAN_PRELIMINARY_RESONANCE_FOUND:
		{
			int64_t f = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t cTop_mA = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t cMean_mA = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t halfPeriod = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: Frequency %lld Hz, top current %lld mA, mean current %lld mA, halfPeriod %lld.", tmp, (long long int)f, (long long int)cTop_mA, (long long int)cMean_mA, (long long int)halfPeriod);
			#else
			snprintf(bufPtr, bufSize, "%s: Frequency %I64d Hz, top current %I64d mA, mean current %I64d mA, halfPeriod %I64d.", tmp, (long long int)f, (long long int)cTop_mA, (long long int)cMean_mA, (long long int)halfPeriod);
			#endif
			break;
		}
		case SCAN_NOW_AT_FREQ:
		{
			int64_t f_Hz = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: Frequency %lld Hz", tmp, (long long int)f_Hz);
			#else
			snprintf(bufPtr, bufSize, "%s: Frequency %I64d Hz", tmp, (long long int)f_Hz);
			#endif
			break;
		}
		case SCAN_HIGHER_BOUNDRY_FOUND:
		{
			int64_t scanTopCurrentFoundUp = DbfUnserializerReadInt64(dbfUnserializer);
			DbfUnserializerReadInt64(dbfUnserializer);
			int64_t f_Hz = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t measuredCurrent = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: Frequency %lld Hz, scanTopCurrent %lld, measuredCurrent %lld", tmp, (long long int)f_Hz, (long long int)scanTopCurrentFoundUp, (long long int)measuredCurrent);
			#else
			snprintf(bufPtr, bufSize, "%s: Frequency %I64d Hz, scanTopCurrent %I64d, measuredCurrent %I64d", tmp, (long long int)f_Hz, (long long int)scanTopCurrentFoundUp, (long long int)measuredCurrent);
			#endif
			break;
		}
		case SCAN_LOWER_BOUNDRY_FOUND:
		{
			int64_t scanTopCurrentFoundDown = DbfUnserializerReadInt64(dbfUnserializer);
			DbfUnserializerReadInt64(dbfUnserializer);
			int64_t f_Hz = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t measuredCurrent = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: Frequency %lld Hz, scanTopCurrent %lld, measuredCurrent %lld", tmp, (long long int)f_Hz, (long long int)scanTopCurrentFoundDown, (long long int)measuredCurrent);
			#else
			snprintf(bufPtr, bufSize, "%s: Frequency %I64d Hz, scanTopCurrent %I64d, measuredCurrent %I64d", tmp, (long long int)f_Hz, (long long int)scanTopCurrentFoundDown, (long long int)measuredCurrent);
			#endif
			break;
		}
		case SCAN_Q:
		{
			int64_t q = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "Q: %lld", (long long int)q);
			#else
			snprintf(bufPtr, bufSize, "Q: %I64d", (long long int)q);
			#endif
			break;
		}
		case SCAN_FAILED_TO_FIND_Q:
			snprintf(bufPtr, bufSize, " %s. Perhaps load is not correctly connected or no input power?", tmp);
			break;
		case SCAN_Q_LARGER_THAN_EXPECTED:
		case SCAN_Q_LESS_THAN_EXPECTED:
		{
			int64_t q = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: Q=%lld. Perhaps load is not correctly connected or no input power?", tmp, (long long int)q);
			#else
			snprintf(bufPtr, bufSize, "%s: Q=%I64d. Perhaps load is not correctly connected or no input power?", tmp, (long long int)q);
			#endif
			break;
		}
		case SCAN_CHANGE_TO_FREQUENCY_REGULATION:
		{
			int64_t halfPeriod = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t f_Hz = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: %lld Hz, halfPeriod %lld.", tmp, (long long int)f_Hz, (long long int)halfPeriod);
			#else
			snprintf(bufPtr, bufSize, "%s: %I64d Hz, halfPeriod %I64d.", tmp, (long long int)f_Hz, (long long int)halfPeriod);
			#endif
			break;
		}
		case SCAN_START_TUNING:
		case SCAN_TUNE_UP:
		case SCAN_TUNE_DOWN:
		case SCAN_TUNE_STAY:
		case SCAN_TUNING_STOP_HIGH:
		case SCAN_STOP_LOW:
		{
			int64_t f_Hz = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t newHalfPeriod = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t scanTuningStartCurrent = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t scanTuningAlternativeCurrent = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t prevHalfPeriod = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: %lld Hz, period %lld, %lld %lld %lld", tmp, (long long int)f_Hz, (long long int)newHalfPeriod, (long long int)prevHalfPeriod, (long long int)scanTuningStartCurrent, (long long int)scanTuningAlternativeCurrent);
			#else
			snprintf(bufPtr, bufSize, "%s: %I64d Hz, period %I64d, %I64d %I64d %I64d", tmp, (long long int)f_Hz, (long long int)newHalfPeriod, (long long int)prevHalfPeriod, (long long int)scanTuningStartCurrent, (long long int)scanTuningAlternativeCurrent);
			#endif
			break;
		}
		case SCAN_COOLING_DOWN_WILL_START_SOON:
		{
			int64_t starting_in_s = DbfUnserializerReadInt64(dbfUnserializer);
			#ifndef __WIN32
			snprintf(bufPtr, bufSize, "%s: %lld ms", tmp, (long long int)starting_in_s);
			#else
			snprintf(bufPtr, bufSize, "%s: %I64d ms", tmp, (long long int)starting_in_s);
			#endif
			break;
		}
		case PORTS_ERROR:
		{
			int64_t errorCode = DbfUnserializerReadInt64(dbfUnserializer);
			const char *errorName = getPortsErrorName(errorCode);
			if (errorName !=NULL)
			{
				snprintf(bufPtr, bufSize, "%s: %s", tmp, errorName);
			}
			else
			{
				#ifndef __WIN32
				snprintf(bufPtr, bufSize, "%s: %lld", tmp, (long long int)errorCode);
				#else
				snprintf(bufPtr, bufSize, "%s: %I64d", tmp, (long long int)errorCode);
				#endif
			}
			break;
		}
		case REG_WAITING_FOR_INPUT_POWER:
		{
			int64_t measuredInputVoltage_mV = DbfUnserializerReadInt64(dbfUnserializer);
			int64_t neededInputVoltage_mV = DbfUnserializerReadInt64(dbfUnserializer);
			snprintf(bufPtr, bufSize, "%s: input voltage %lld mV DC, need %lld", tmp, (long long int)measuredInputVoltage_mV, (long long int)neededInputVoltage_mV);
			break;
		}
		default:
		{
			snprintf(bufPtr, bufSize, "%s: ", tmp);
			const int n = strlen(bufPtr);
			DbfUnserializerReadAllToString(dbfUnserializer, bufPtr+n, bufSize-n);
			break;
		}
	}
}

/*
void log_voltage_message_to_string(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	if ((bufPtr == NULL) || (bufSize<=0) || (bufSize >= 0x70000000))
	{
		return;
	}

	int logMessageCode = DbfUnserializerReadInt64(dbfUnserializer);

	const char* tmp=getVoltageLogMessageName(logMessageCode);
	if (tmp!=NULL)
	{
		snprintf(bufPtr, bufSize, "%s: ", tmp);
	}
	else
	{
		snprintf(bufPtr, bufSize, "VoltageLogMsg_%d: ", (int)logMessageCode);
	}
	const int n = strlen(bufPtr);
	DbfUnserializerReadAllToString(dbfUnserializer, bufPtr+n, bufSize-n);
}
*/

void log_reply_message_to_string(DbfUnserializer *dbfUnserializer, char *bufPtr, int bufSize)
{
	if ((bufPtr == NULL) || (bufSize<=0) || (bufSize >= 0x70000000))
	{
		return;
	}

	long cmd = DbfUnserializerReadInt64(dbfUnserializer);
	const char *commandName = getMessageCommandName(cmd);
	if (commandName != NULL)
	{
		snprintf(bufPtr, bufSize, "%s", commandName);
	}
	else
	{
		snprintf(bufPtr, bufSize, "CMD%ld", cmd);
	}
}

void decodeDbfToText(const unsigned char *msgPtr, int msgLen, char *bufPtr, int bufSize)
{
	if ((bufPtr == NULL) || (bufSize<=0) || (bufSize >= 0x70000000))
	{
		return;
	}

	DbfUnserializer dbfUnserializer;
	DbfUnserializerInit(&dbfUnserializer, msgPtr, msgLen);

	if (DbfUnserializerReadIsNextEnd(&dbfUnserializer))
	{
		snprintf(bufPtr, bufSize, "empty dbf");
	}
	else if (DbfUnserializerReadIsNextString(&dbfUnserializer))
	{
		snprintf(bufPtr, bufSize, "unknown dbf");
	}
	else if (DbfUnserializerReadIsNextInt(&dbfUnserializer))
	{
		int messageCategory = DbfUnserializerReadInt64(&dbfUnserializer);

		switch(messageCategory)
		{
			case LOG_CATEGORY:
			{
				char tmpStr[4096];
				log_message_to_string(&dbfUnserializer, tmpStr, sizeof(tmpStr));
				snprintf(bufPtr, bufSize, "log, %s", tmpStr);
			}	break;
			case STATUS_CATEGORY:
			{
				char tmpStr[4096];
				logStatusMessageToString(&dbfUnserializer, tmpStr, sizeof(tmpStr));
				snprintf(bufPtr, bufSize, "status, %s", tmpStr);
				break;
			}
			case COMMAND_CATEGORY:
			{
				char tmpStr[4096];
				logCommandMessageToString(&dbfUnserializer, tmpStr, sizeof(tmpStr));
				snprintf(bufPtr, bufSize, "command, %s", tmpStr);
				break;
			}
			default:
			{
				const char* messageCategoryName = getMessageCategoryName(messageCategory);
				if (messageCategoryName!=NULL)
				{
					snprintf(bufPtr, bufSize,"%s", messageCategoryName);
				}
				else
				{
					snprintf(bufPtr, bufSize,"msgCategory%d", messageCategory);
				}
				break;
			}
		}
	}
	else
	{
		snprintf(bufPtr, bufSize,LOG_PREFIX "unsupported dbf" LOG_SUFIX);
	}

	if (!DbfUnserializerReadIsNextEnd(&dbfUnserializer))
	{
		int n = strlen(bufPtr);
		if(n<bufSize)
		{
			char tmpStr[4096];
			DbfUnserializerReadAllToString(&dbfUnserializer, tmpStr, sizeof(tmpStr));
			snprintf(bufPtr+n, bufSize-n, ", '%s'", tmpStr);
		}
	}
}


// This shall only log the message to console. Not process it.
void decodeMessageToText(const DbfReceiver *dbfReceiver, char *buf, int bufSize)
{
	if (DbfReceiverIsTxt(dbfReceiver))
	{
		//DbfReceiverLogRawData(dbfReceiver);
		snprintf(buf, bufSize, "txt: '%s'", dbfReceiver->buffer);
	}
	else if (DbfReceiverIsDbf(dbfReceiver))
	{
		decodeDbfToText(dbfReceiver->buffer, dbfReceiver->msgSize, buf, bufSize);
	}
	else
	{
		printf("unknown message");
	}

}

// This shall only log the message to console. Not process it.
void linux_sim_log_message_from_target(const DbfReceiver *dbfReceiver)
{
	// Logging data in hex.
	printf(LOG_PREFIX "hex: ");
	for(int i=0;i<dbfReceiver->msgSize;i++)
	{
		printf(" %02x", (unsigned char)dbfReceiver->buffer[i]);
	}
	printf(LOG_SUFIX);

	// Log as raw text.
	if (DbfReceiverIsDbf(dbfReceiver))
	{
		char str[4096];
		DbfUnserializer dbfUnserializer;
		DbfUnserializerInit(&dbfUnserializer, dbfReceiver->buffer, dbfReceiver->msgSize);
		DbfUnserializerReadAllToString(&dbfUnserializer, str, sizeof(str));
		printf(LOG_PREFIX "raw: %s" LOG_SUFIX,str);
	}

	// Log in clear text as much as known
	char buf[4096];
	//DbfLogBuffer("", dbfReceiver->buffer, dbfReceiver->msgSize);
	decodeMessageToText(dbfReceiver, buf, sizeof(buf));
	printf(LOG_PREFIX "msg: %s" LOG_SUFIX, buf);
}

void logDbfAsHexAndText(const unsigned char *msgPtr, int msgLen)
{
	// Logging data in hex.
	printf(LOG_PREFIX "hex: ");
	for(int i=0;i<msgLen;i++)
	{
		printf(" %02x", msgPtr[i]);
	}
	printf(LOG_SUFIX);

	// Log as raw text.
	char str[4096];
	DbfUnserializer dbfUnserializer;
	DbfUnserializerInit(&dbfUnserializer, msgPtr, msgLen);
	DbfUnserializerReadAllToString(&dbfUnserializer, str, sizeof(str));
	printf(LOG_PREFIX "raw: %s" LOG_SUFIX,str);

	// Log in clear text as much as known
	char buf[4096];
	//DbfLogBuffer("", msgPtr, msgLen);
	decodeDbfToText(msgPtr, msgLen, buf, sizeof(buf));
	printf(LOG_PREFIX "txt: %s" LOG_SUFIX, buf);
}

const char* getReasonCodeName(NOK_REASON_CODES reason)
{
	switch (reason)
	{
		case REASON_OK: return "OK";
		case CANT_SAVE_WHILE_RUNNING: return "CANT_SAVE_WHILE_RUNNING";
		case NOK_UNKOWN_PARAMETER: return "NOK_UNKOWN_PARAMETER";
		case NOK_UNKNOwN_COMMAND: return "NOK_UNKNOwN_COMMAND";
		case IGNORED_SETTING_PARAMETER: return "IGNORED_SETTING_PARAMETER";
		case WRONG_MAGIC_NUMBER: return "WRONG_MAGIC_NUMBER";
		case WRONG_SENDER_ID: return "WRONG_SENDER_ID";
		case PARAMETER_OUT_OF_RANGE: return "PARAMETER_OUT_OF_RANGE";
		case READ_ONLY_PARAMETER: return "READ_ONLY_PARAMETER";
		case INCOMPLETE_MESSAGE: return "INCOMPLETE_MESSAGE";
		case UNKNOWN_OR_READ_ONLY_PARAMETER: return "UNKNOWN_OR_READ_ONLY_PARAMETER";
		default: break;
	}
	return NULL;
}
#endif

