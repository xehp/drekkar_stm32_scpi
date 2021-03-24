/*
machineState.c


Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2019-03-15
Created, Some methods moved from ports.h to this new file.
Henrik Bjorkman

*/

#include "cfg.h"
#include "systemInit.h"
#include "eeprom.h"
#include "messageNames.h"
#include "log.h"
#include "machineState.h"
#include "debugLog.h"
#include "messageUtilities.h"
#include "serialDev.h"

DbfSerializer statusDbfMessage;

int errorReported = portsErrorOk; // REPORTED_ERROR_PAR
//int startCmd=cmdPauseCommand;

int64_t cyclesToDo = INT64_MAX; // CYCLES_TO_DO_PAR
int64_t totalCyclesPerformed = 0; // TOTAL_CYCLES_PERFORMED_PAR

uint32_t machineSessionNumber=0; // MACHINE_SESSION_NUMBER, Web server use this to detect a target reboot.
int32_t maxAllowedCurrent_mA=0;  // MAX_ALLOWED_CURRENT_MA_PAR
int32_t maxAllowedExternalVoltage_mV=0; // MAX_ALLOWED_EXT_VOLT_MV_PAR


/*
Extract from msg.c (if changed msg.c has precedence):
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
*/
static void sendLogAssertError(const char *msg, const char* file, int32_t line)
{
	logInitAndAddHeader(&messageDbfTmpBuffer, ERROR_ASSERT);
	DbfSerializerWriteString(&messageDbfTmpBuffer, msg);
	DbfSerializerWriteString(&messageDbfTmpBuffer, file);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, line);
	messageSendDbf(&messageDbfTmpBuffer);
}

/*
This is an extract from msg.c (things might change, that file has precedence if mismatching)

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
*/
void secAndLogInitStatusMessageAddHeader(DbfSerializer *dbfMessage, STATUS_MESSAGES msg)
{
	messageInitAndAddCategoryAndSender(dbfMessage, STATUS_CATEGORY);
	DbfSerializerWriteInt32(dbfMessage, msg);
}

void errorReportError(int errorCode)
{
	if (errorReported == portsErrorOk)
	{
		logInt3(ERROR_REPORT, errorCode, systemGetSysTimeMs());
		errorReported = errorCode;
	}
}

int errorGetReportedError()
{
	return errorReported;
}

void errorAssert(const char *msg, const char *file, int line)
{
	errorReportError(portsErrorAssert);

	sendLogAssertError(msg, file, line);


#if (defined __arm__)
	debug_print("\nassert fail ");
	debug_print(file);
	debug_print(" ");
	debug_print64(__LINE__);
	debug_print(" ");
	debug_print(msg);
	debug_print("\n");
#endif


	systemErrorHandler(SYSTEM_APPLICATION_ASSERT_ERROR);
}

// TODO perhaps we should require caller to provide the current error code to be reset?
// This to be sure that we do not miss an error.
void errorReset()
{
	errorReported = portsErrorOk;
}

void machineStateInit()
{
}





// This is called once per second or so from secAndLog.
void machineSupervise(void)
{
}




