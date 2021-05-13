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

// Codes used in LOG_CATEGORY messages.
// In all log messages (LOG_MSG) the first field (after LOG_MSG) shall be one of these.
// Preferably unique numbers across the system so check also other SW what they use.
typedef enum
{
	CMD_INIT = 16,
	CMD_INCORRECT_DBF_RECEIVED = 17,
	CMD_RESETTING = 18,
	EEPROM_WRONG_MAGIC_NUMBER = 20,
	EEPROM_WRONG_CHECKSUM = 21,
	EEPROM_LOADED_OK = 22,
	EEPROM_TRYING_BACKUP = 23,
	EEPROM_BACKUP_LOADED_OK = 24,
	EEPROM_FAILED = 25,
	EEPROM_SAVE_RESULT = 26,
	FAN_1_NOT_DETECTED = 32,
	FAN_2_NOT_DETECTED = 33,
	FAN_INIT = 38,
	FAN_AND_TEMP_COUNT_FAN2 = 39,
	SEC_AND_LOG_INIT = 80,
	ERROR_REPORT = 180,
	ERROR_ASSERT = 181,
	EEPROM_SAVE_PENDING_FLAG_SET = 213,
	EEPROM_PERFOMING_PENDING_SAVE = 214,
	TEMP_INIT = 226,
	EEPROM_DATA_UPGRADED = 227,
} AVR_CFG_LOG_MESSAGES;

// Codes used in STATUS_MESSAGES messages.
typedef enum
{
	UNKNOWN_STATUS_MSG = 0,
	REBOOT_STATUS_MSG = 2,
	VOLTAGE_STATUS_MSG = 3,
	LEAK_CURRENT_STATUS_MSG = 5,
	PARAMETER_STATUS_MSG = 10,
	TEMP_STATUS_MSG = 11,
} STATUS_MESSAGES;

// Codes used in COMMAND_CATEGORY messages.
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
	portsErrorAssert=23,
} PORTS_ERROR_CODES;

// All parameters who's name ends with a number must be listed here in sequence.
// Is is for example assumed in other parts of SW that TARGET_CYCLES_x &
// REACHED_CYCLES_x are in sequence. So, things need to stay that way.
// Some extra numbers where reserved in case these "arrays" need to be larger.
typedef enum
{
	UNKNOWN_PAR = 0,
	DEVICE_ID = 3,                  // ee.deviceId
	TARGET_TIME_S = 26,
	REPORTED_EXT_AC_VOLTAGE_MV = 41,      // measured AC voltage from SCPI
	TEMP1_C = 47,
	TEMP2_C = 48,
	FAN1_HZ = 49, // Not used at the moment. Depends on macro FAN1_APIN.
	FAN2_HZ = 50, // Not used at the moment. Depends on macro USE_LPTMR2_FOR_FAN2.
	REPORTED_ERROR = 52,
	MICRO_AMPS_PER_UNIT_AC = 61,        // ee.microAmpsPerUnit
	SYS_TIME_MS = 74,
	MEASURED_LEAK_AC_CURRENT_MA = 106,
	// Remember to update LOGGING_MAX_NOF_PARAMETERS if a parameter is added here.
} PARAMETER_CODES;

// Set this value to the highest value used in PARAMETER_CODES+1.
#define LOGGING_MAX_NOF_PARAMETERS 107


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
//const char *getPortsStatusMessageName(PORTS_STATUS_SUB_MESSAGES statusCode);
const char *getStatusMessagesName(STATUS_MESSAGES statusCode);
const char *getLogMessageName(int logMessageType);
//const char* getVoltageLogMessageName(int logCode);
const char *getPortsErrorName(int errorCode);
const char * getParameterName(PARAMETER_CODES parameterCode);
PARAMETER_CODES getParameterCodeFromWebName(const char* dataName);
const char* getReasonCodeName(NOK_REASON_CODES reason);


#endif
