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
		case SEND_ALL_CONFIG_CMD: return "SEND_ALL_CONFIG";
		case IGNORE_VOLTAGE_SENSOR_CMD: return "IGNORE_VOLTAGE_SENSOR";
		case REBOOT_CMD: return "REBOOT_CMD";
		case SAVE_CMD: return "SAVE_CMD";
		case STOP_CMD: return "STOP_CMD";
		case START_CMD: return "START_CMD";
		case SHOW_VERSION_CMD: return "SHOW_VERSION";
		case RESTORE_DEFAULT_CMD: return "RESTORE_DEFAULT_CMD";
		#endif
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
	case CMD_INIT: return "CMD_INIT";
	case CMD_INCORRECT_DBF_RECEIVED: return "CMD_INCORRECT_DBF_RECEIVED";
	case CMD_RESETTING: return "CMD_RESETTING";
	case EEPROM_WRONG_MAGIC_NUMBER: return "EEPROM_WRONG_MAGIC_NUMBER";
	case EEPROM_WRONG_CHECKSUM: return "EEPROM_WRONG_CHECKSUM";
	case EEPROM_LOADED_OK: return "EEPROM_LOADED_OK";
	case EEPROM_TRYING_BACKUP: return "EEPROM_TRYING_BACKUP";
	case EEPROM_BACKUP_LOADED_OK: return "EEPROM_BACKUP_LOADED_OK";
	case EEPROM_FAILED: return "EEPROM_FAILED";
	case FAN_AND_TEMP_INIT: return "FAN_AND_TEMP_INIT";
	case SEC_AND_LOG_INIT: return "SEC_AND_LOG_INIT";
	case ERROR_REPORT: return "ERROR_REPORT";
	case ERROR_ASSERT: return "ERROR_ASSERT";
	case EEPROM_SAVE_PENDING_FLAG_SET: return "EEPROM_SAVE_PENDING_FLAG_SET";
	case EEPROM_PERFOMING_PENDING_SAVE: return "EEPROM_PERFOMING_PENDING_SAVE";
	case EEPROM_DATA_UPGRADED: return "EEPROM_DATA_UPGRADED";
	#endif
	default: break;
	}
	return NULL;
}


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
		case portsErrorUnexpecetedMessage: return "portsErrorUnexpecetedMessage";
		case portsErrorAssert: return "ErrorAssert";
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
		case DEVICE_ID: return "DEVICE_ID";
		case TARGET_TIME_S: return "TARGET_TIME_S";
		case TEMP1_C: return "TEMP1_C";
		case TEMP2_C: return "TEMP2_C";
		case REPORTED_ERROR: return "REPORTED_ERROR";
		case SCAN_DELAY_MS: return "SCAN_DELAY_MS";
		case SYS_TIME_MS: return "SYS_TIME_MS";
		#endif
		default: break;
	}
	return NULL;
}

#if defined __linux__ || defined __WIN32
PARAMETER_CODES getParameterCodeFromWebName(const char* dataName)
{
	assert(dataName!=NULL);
	// Slow but this was not called often.
	for (int i = 0; i< LOGGING_MAX_NOF_PARAMETERS; i++)
	{
		const char *tmp = getParameterName(i);
		if ((tmp!=NULL) && (strcmp(tmp, dataName) == 0))
		{
			return i;
		}
	}

	return UNKNOWN_PAR;
}
#endif


#if defined __linux__ || defined __WIN32
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

