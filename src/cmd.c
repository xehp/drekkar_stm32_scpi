/*
cmd.c

provide to handle commands/messages from other units/users

Copyright (C) 2021 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2016-09-22
Created.
Henrik Bjorkman

*/
#include "cfg.h"
//#include <string.h>

#include "main_loop.h"
#include "messageNames.h"
#include "Dbf.h"
#include "mathi.h"
#include "eeprom.h"
#include "fan.h"
#include "temp.h"
#include "scan.h"
#include "translator.h"
#include "version.h"
#include "log.h"
#include "externalSensor.h"
#include "externalTemp.h"
#include "externalCurrentLeakSensor.h"
#include "cmd.h"
#include "machineState.h"
#include "relay.h"
#include "portsPwm.h"
#include "pwm1.h"
#include "serialDev.h"
#include "debugLog.h"
#include "supervise.h"
#include "compDev.h"
#include "mainSeconds.h"
#include "measuringFilter.h"
#include "messageUtilities.h"


#ifdef COMMAND_ON_LPUART1
static DbfReceiver cmdLine0;
#endif

static DbfReceiver cmdLine1;

#ifdef COMMAND_ON_USART2
static DbfReceiver cmdLine2;
#endif


// Magic numbers so we can detect version incompatibilities.
// This needs to be same as magicNumberInAllConfig in target_maintenance.c
// All sorts of commands (except stop) shall be ignored unless web server gave
// correct magic number.
#define MAGIC_NUMBER_WEB_SERVER 8898878L
#define ALL_CONFIG_CMD_MAGIC_NR 0x0139


#define DBG_MODE 5

// See also DEFAULT_MAX_TEMP_HZ
#define CMD_MAX_TEMP_HZ 1360


static void forwardMessage(int usartDev, const DbfReceiver* dbfReceiver)
{
	const char *msgPtr=dbfReceiver->buffer;
	const int msgLen=dbfReceiver->msgSize;
	serialPutChar(usartDev, DBF_BEGIN_CODEID);
	serialWrite(usartDev, msgPtr, msgLen);
	serialPutChar(usartDev, DBF_END_CODEID);
}


static uint16_t latestForwardedCounter = 0;
static DbfReceiver latestForwarded[4]={0};

static int64_t magicNumberFromWebServer=0;
static int64_t idOfWebServer=0;


// Returns non zero if different.
static int bufcmp(const uint32_t *a, const uint32_t *b, int n)
{
	while(n)
	{
		n--;
		if (a[n] != b[n])
		{
			return -1;
		}
	}
	return 0;
}

// Returns non zero if different.
static int my_memcmp(const int8_t *a, const int8_t *b, int n)
{
	// First non word aligned in the end.
	while ((n%4) != 0)
	{
		if (a[n] != b[n])
		{
			return -1;
		}
		n--;
	}

	// Then compare all full 32 bit words.
	return bufcmp((const uint32_t*)a, (const uint32_t*)b, n/4);
}

static void bufcpy(uint32_t *dst, const uint32_t *src, int n)
{
	while(n)
	{
		*dst = *src;
		dst++;
		src++;
		n--;
	}
}

// Returns non zero if different.
static int dbfReceiverCompare(const DbfReceiver* a, const DbfReceiver* b)
{
	if (a->msgSize != b->msgSize)
	{
		// size is not same so its different.
		return -1;
	}
	return my_memcmp(a->buffer, b->buffer, a->msgSize);
}


static void dbfReceiverCopy(DbfReceiver* dbfReceiverDst, const DbfReceiver* dbfReceiverSrc)
{
	dbfReceiverDst->msgSize = dbfReceiverSrc->msgSize;
	// In this application it is OK to copy a few bytes extra in the end.
	// See comment for DBF_BUFFER_SIZE_32BIT.
	// So we do since that is much faster.
	bufcpy((uint32_t*)dbfReceiverDst->buffer, (const uint32_t*)dbfReceiverSrc->buffer, (dbfReceiverSrc->msgSize+3)/4);
}

// Returns 1 if message is already forwarded.
static int isAlreadyForwarded(const DbfReceiver* dbfReceiverSrc)
{
	int f = 0;
	int i = 0;
	while(i<SIZEOF_ARRAY(latestForwarded))
	{
		if (dbfReceiverCompare(&latestForwarded[i], dbfReceiverSrc)==0)
		{
			f = 1;
			break;
		}
		i++;
	}

	// Remember the forwarded message.
	ASSERT(latestForwardedCounter < SIZEOF_ARRAY(latestForwarded));
	dbfReceiverCopy(&latestForwarded[latestForwardedCounter], dbfReceiverSrc);
	latestForwardedCounter = (latestForwardedCounter >= (SIZEOF_ARRAY(latestForwarded)-1)) ? 0 : latestForwardedCounter+1;

	return f;
}

/**
 * Used to send a message received from one serial port to another.
 */
static void forwardMessageToOthers(int receivedFromUsartDev, const DbfReceiver* dbfReceiver)
{
	// Depending on which USART device the message was received on, forward to the other.

	if (isAlreadyForwarded(dbfReceiver))
	{
		// Don,t forward this, its same as a previous message.
	}
	else
	{
		switch(receivedFromUsartDev)
		{
			case DEV_USART1:
				forwardMessage(DEV_USART2, dbfReceiver);
				break;
			case DEV_USART2:
				forwardMessage(DEV_USART1, dbfReceiver);
				break;
			default:
				break;
		}
	}
}

// Returns zero if OK
static int checkSenderId(int cmd, int64_t senderId)
{
	if ((idOfWebServer ==0 ) || (senderId != idOfWebServer))
	{
		return 1;
	}
	return 0;
}

static int wrongSenderId(int cmd, int64_t senderId, int64_t replyToRef)
{
		messageReplyNOK(cmd, WRONG_SENDER_ID, -1, senderId, replyToRef);
		return CMD_SENDER_NOT_AUTHORIZED;
}

/**
When the WebSUart server starts it will send a SEND_ALL_CONFIG_CMD request message.
This function makes and sends the reply to it.

If the format of this or other messages is changed in a non backward compatible
way remember to change the "ALL_CONFIG_CMD_MAGIC_NR" that way we can avoid
mismatching (non compatible) programs trying to communicate with each other.
Appending fields can be backward compatible (receiver can see how long a message is)
so that is preferred compared to changing the meaning of a field.

See also process_all_config_reply in target_maintenance.c.
*/
static void sendAllConfig1Reply(int64_t replyToId, int64_t replyToRef)
{
	messageReplyOkInitAndAddHeader(SEND_ALL_CONFIG1_CMD, replyToId, replyToRef);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, ee.deviceId);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, ee.microAmpsPerUnitDc);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, ee.microVoltsPerUnitDc);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, ee.superviceDeviationFactor);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, tempMinFreq);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, ee.tempMaxFreq);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, ee.wallTimeReceived_ms);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, ee.microAmpsPerUnitAc);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, portsGetCycleCount());
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, ALL_CONFIG_CMD_MAGIC_NR);

	messageSendDbf(&messageDbfTmpBuffer);
}

// The format here must match that in process_all_config2_reply in web server.
// See also processAllConfig3Cmd?
static void sendAllConfig2Reply(int64_t replyToId, int64_t replyToRef)
{
	messageReplyOkInitAndAddHeader(SEND_ALL_CONFIG2_CMD, replyToId, replyToRef);

	DbfSerializerWriteInt64(&messageDbfTmpBuffer, ee.wantedCurrent_mA);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, ee.wantedVoltage_mV);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, ee.scanBegin_Hz);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, ee.scanEnd_Hz);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, ee.voltageProbeFactor);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, ee.maxExternalTemp_C);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, ee.wantedInternalScanningVoltage_mV);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, getParameterValue(MAX_TEMP_C, NULL));
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, getParameterValue(AUTOSTART_PAR, NULL));
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, getParameterValue(FAIL_SCAN_IF_TO_MUCH_CURRENT, NULL));

	for(int i=0; i<NDEV; i++)
	{
		DbfSerializerWriteInt64(&messageDbfTmpBuffer, ee.targetCycles[i]);
		DbfSerializerWriteInt64(&messageDbfTmpBuffer, ee.reachedCycles[i]);
	}

	messageSendDbf(&messageDbfTmpBuffer);

	logInt5(CMD_CONFIG, 2, ee.scanBegin_Hz, ee.targetCycles[0], ee.reachedCycles[0]);
}

// The format here must match that in process_all_config2_reply in web server.
static void sendAllConfig3Reply(int64_t replyToId, int64_t replyToRef)
{
	messageReplyOkInitAndAddHeader(SEND_ALL_CONFIG3_CMD, replyToId, replyToRef);
	messageSendDbf(&messageDbfTmpBuffer);
}

void sendVersionReply(int64_t replyToId, int64_t replyToRef)
{
	messageReplyOkInitAndAddHeader(SHOW_VERSION_CMD, replyToId, replyToRef);
	DbfSerializerWriteString(&messageDbfTmpBuffer, VERSION_STRING);
	messageSendDbf(&messageDbfTmpBuffer);
}



NOK_REASON_CODES setParameterValue(PARAMETER_CODES parId, const int64_t value)
{
	switch(parId)
	{
		case AUTOSTART_PAR:
		{
			if (value==0)
			{
				ee.optionsBitMask &= ~(1 << AUTOSTART_BIT);
			}
			else if (value==1)
			{
				ee.optionsBitMask |= (1 << AUTOSTART_BIT);
			}
			else
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			break;
		}
		case CYCLES_TO_DO:
		{
			cyclesToDo = value;
			break;
		}
		case DEVICE_ID:
		{
			ee.deviceId = value;
			break;
		}
		case MICRO_AMPS_PER_UNIT_DC:
		{
			if ((value<0) || (value>0xFFFFFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.microAmpsPerUnitDc = value;
			break;
		}
		case MICRO_VOLTS_PER_UNIT_DC:
		{
			if ((value<0) || (value>0xFFFFFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.microVoltsPerUnitDc = value;
			break;
		}
		case MACHINE_SESSION_NUMBER:
		{
			if ((value<0) || (value>0xFFFFFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			machineSessionNumber = value;
		}
		case SUPERVICE_DEVIATION_FACTOR:
		{
			if ((value<0) || (value>0xFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.superviceDeviationFactor = value;
			break;
		}
		case FAIL_SCAN_IF_TO_MUCH_CURRENT:
		{
			if (value==0)
			{
				ee.optionsBitMask &= ~(1 << FAIL_SCAN_IF_TO_MUCH_CURRENT_BIT);
			}
			else if (value==1)
			{
				ee.optionsBitMask |= (1 << FAIL_SCAN_IF_TO_MUCH_CURRENT_BIT);
			}
			else
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			break;
		}
		case WEB_SERVER_TIMEOUT_S:
		{
			ee.webServerTimeout_s = value;;
		}
		case WANTED_SCANNING_VOLTAGE_MV:
		{
			if ((value<0) || (value>0xFFFFFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.wantedInternalScanningVoltage_mV = value;
			break;
		}
		case WANTED_CURRENT_MA:
		{
			if ((value<0) || (value>0xFFFFFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.wantedCurrent_mA = value;

			// Adjust also maxAllowedCurrent_mA
			if (((value*11)/10) > maxAllowedCurrent_mA)
			{
				maxAllowedCurrent_mA = (value*11)/10;
			}
			break;
		}
		case WANTED_VOLTAGE_MV:
		{
			if ((value<-1) || (value>0x7FFFFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			else
			{
				ee.wantedVoltage_mV = value;
				if (((value*11)/10) > maxAllowedExternalVoltage_mV)
				{
					maxAllowedExternalVoltage_mV = (value*11)/10;
				}
			}
			break;
		}
		case MIN_TEMP_HZ:
		{
			if ((value<0) || (value>0xFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			else if (value<=CMD_MAX_TEMP_HZ)
			{
				ee.tempMaxFreq = value;
				return REASON_OK;
			}
			else
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			break;
		}
		case MAX_TEMP_HZ:
		{
			if ((value<0) || (value>0xFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			else if (value<=CMD_MAX_TEMP_HZ)
			{
				ee.tempMaxFreq = value;
				return REASON_OK;
			}
			else
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			break;
		}
		case TEMP_FREQ_AT_20C:
		{
			if ((value<0) || (value>0xFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.freqAt20C = value;
			break;
		}
		case TEMP_FREQ_AT_100C:
		{
			if ((value<0) || (value>0xFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.freqAt100C = value;
			break;
		}
		case TOTAL_CYCLES_PERFORMED: // This parameter TOTAL_CYCLES_PERFORMED_PAR can not be set, only get.
		{
			return READ_ONLY_PARAMETER;
			break;
		}
		case SCAN_BEGIN_HZ:
		{
			if ((value<MIN_FREQUENCY) || (value>MAX_FREQUENCY))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			else
			{
				ee.scanBegin_Hz = value;
			}
			break;
		}
		case SCAN_END_HZ:
		{
			if ((value<MIN_FREQUENCY) || (value>MAX_FREQUENCY))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			else
			{
				ee.scanEnd_Hz = value;
			}
			break;
		}
		case RESONANCE_HALF_PERIOD:
		{
			if ((value >= MIN_HALF_PERIOD) && (value <= MAX_HALF_PERIOD))
			{
				scanSetResonanceHalfPeriod(value);
			}
			else
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			break;
		}
		case PORTS_PWM_HALF_PERIOD:
		{
			if ((value<0) || (value>0xFFFFFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			portsSetHalfPeriod(value);
			break;
		}
		case SCAN_PEAK_WIDTH_IN_PERIOD_TIME:
		{
			if ((value<0) || (value>0xFFFFFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			scanPeakWidthInPeriodTime = value;
			break;
		}
		case PORTS_PWM_ANGLE:
		{
			if ((value<0) || (value>0xFFFFFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			portsSetAngle(value);
			break;
		}
		case INV_FREQUENCY_HZ:
		{
			if ((value<0) || (value>0xFFFFFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			portsSetHalfPeriod(TRANSLATE_FREQUENCY_TO_HALF_PERIOD(value));
			break;
		}
		case PORTS_PWM_MIN_ANGLE:
		{
			if ((value<0) || (value>0xFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.portsPwmMinAngle = value;
			break;
		}
		case COOLDOWN_TIME_S:
		{
			if ((value<0) || (value>0xFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.minCooldownTime_s = value;
			break;
		}
		case MIN_Q_VALUE:
		{
			if ((value<0) || (value>0xFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.minQValue = value;
			break;
		}
		case MAX_Q_VALUE:
		{
			if ((value<0) || (value>0xFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.maxQValue = value;
			break;
		}
		case SCAN_DELAY_MS:
		{
			if ((value<0) || (value>0xFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.scanDelay_ms = value;
			break;
		}
		case STABLE_OPERATION_TIMER_S:
		{
			if ((value<0) || (value>0xFFFFFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			stableOperationTimer = value;
			break;
		}
		case MICRO_AMPS_PER_UNIT_AC:
		{
			if ((value<0) || (value>0xFFFFFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.microAmpsPerUnitAc = value;
			break;
		}
		case SUPERVICE_STABILITY_TIME_S:
		{
			if ((value<0) || (value>0xFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.stableOperationTimer_s = value;
			break;
		}
		case SUPERVICE_DROP_FACTOR:
		{
			if ((value<=0) || (value>0xFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.superviceDropFactor = value;
			break;
		}
		case PORTS_PWM_BRAKE_STATE:
		{
			return READ_ONLY_PARAMETER;
		}
		case VOLTAGE_PROBE_FACTOR:
		{
			ee.voltageProbeFactor = value;
			break;
		}
		case LOW_DUTY_CYCLE_PERCENT:
		{
			if ((value<10) || (value>90))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.lowDutyCyclePercent = value;
			break;
		}
		/*case TEMP_FREQ_AT_60C_PAR:
		{
			if ((value<0) || (value>0xFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.freqAt60C = value;
			break;
		}*/
		case MAX_TEMP_C:
		{
			int32_t f = linearInterpolationCToFreq(value);
			if ((f<0) || (f>0xFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.tempMaxFreq = f;
			break;
		}
		case TARGET_TIME_S:
		case MEASURED_AC_CURRENT_MA:
		case MACHINE_STATE:
		case SYS_TIME_MS:
			return READ_ONLY_PARAMETER;
		case SUPERVICE_INC_FACTOR:
		{
			if ((value<=0) || (value>255))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.superviceIncFactor = value;
			break;
		}
		case MAX_EXT_TEMP_C:
		{
			if ((value<=0) || (value>150))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.maxExternalTemp_C=value;
			break;
		}
		case EXT_TEMP_REQUIRED:
		{
			if (value==0)
			{
				ee.optionsBitMask &= ~(1 << EXT_TEMP_REQUIRED_BIT);
			}
			else if (value==1)
			{
				ee.optionsBitMask |= (1 << EXT_TEMP_REQUIRED_BIT);
			}
			else
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			break;
		}
		case EXT_VOLT_REQUIRED:
		{
			if (value==0)
			{
				ee.optionsBitMask &= ~(1 << EXT_VOLT_REQUIRED_BIT);
			}
			else if (value==1)
			{
				ee.optionsBitMask |= (1 << EXT_VOLT_REQUIRED_BIT);
			}
			else
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			break;
		}
		case LOOP_REQUIRED:
		{
			if (value==0)
			{
				ee.optionsBitMask &= ~(1 << LOOP_REQUIRED_BIT);
			}
			else if (value==1)
			{
				ee.optionsBitMask |= (1 << LOOP_REQUIRED_BIT);
			}
			else
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			break;
		}
		case TARGET_CYCLES_0:
		{
			ee.targetCycles[0] = value;
			break;
		}
		case TARGET_CYCLES_1:
		{
			ee.targetCycles[1] = value;
			break;
		}
		case TARGET_CYCLES_2:
		{
			ee.targetCycles[2] = value;
			break;
		}
		case TARGET_CYCLES_3:
		{
			ee.targetCycles[3] = value;
			break;
		}
		#if (NDEV >= 8)
		case TARGET_CYCLES_4:
		{
			ee.targetCycles[4] = value;
		}
		case TARGET_CYCLES_5:
		{
			ee.targetCycles[5] = value;
		}
		case TARGET_CYCLES_6:
		{
			ee.targetCycles[6] = value;
		}
		case TARGET_CYCLES_7:
		{
			ee.targetCycles[7] = value;
		}
		#endif
		case REACHED_CYCLES_0:
		{
			ee.reachedCycles[0] = value;
			break;
		}
		case REACHED_CYCLES_1:
		{
			ee.reachedCycles[1] = value;
			break;
		}
		case REACHED_CYCLES_2:
		{
			ee.reachedCycles[2] = value;
			break;
		}
		case REACHED_CYCLES_3:
		{
			ee.reachedCycles[3] = value;
			break;
		}
		#if (NDEV >= 8)
		case REACHED_CYCLES_4:
		{
			ee.reachedCycles[4] = value;
		}
		case REACHED_CYCLES_5:
		{
			ee.reachedCycles[5] = value;
		}
		case REACHED_CYCLES_6:
		{
			ee.reachedCycles[6] = value;
		}
		case REACHED_CYCLES_7:
		{
			ee.reachedCycles[7] = value;
		}
		#endif
		case SUPERVICE_MARGIN_MV:
		{
			if ((value<=0) || (value>65000))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.superviceMargin_mV = value;
			break;
		}

		case MAX_EXT_TEMP_DIFF_C:
		{
			if ((value<=0) || (value>255))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.maxExtTempDiff_C = value;
			break;
		}
		case MAX_CURRENT_STEP:
		{
			if ((value<=0) || (value>1000000))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.maxCurrentStep_ppb = value;
			break;
		}
		case MAX_VOLTAGE_STEP:
		{
			if ((value<=0) || (value>100000))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.maxVoltageStep_ppb = value;
			break;
		}
		case MAX_LEAK_CURRENT_MA:
		{
			if ((value<=0) || (value>0x7FFFFFFF))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.maxLeakCurrentStep_mA = value;
			break;
		}
		case MIN_MV_PER_MA_AC:
		{
			if ((value<=0) || (value>0xffff))
			{
				return PARAMETER_OUT_OF_RANGE;
			}
			ee.min_mV_per_mA_ac = value;
			break;
		}
		default:
		{
			return UNKNOWN_OR_READ_ONLY_PARAMETER;
			break;
		}
	}
	return REASON_OK;
}


// Returns 0 if OK, nonzero otherwise.
static int interpretSetCmd(DbfUnserializer *dbfPacket, int64_t replyToId, int64_t replyToRef)
{
	// The command set wanted current looks like this: "set <variable name> <variable value>".

	const int32_t parId = DbfUnserializerReadInt32(dbfPacket);
	const int64_t value = DbfUnserializerReadInt64(dbfPacket);

	if (dbfPacket->decodeState == DbfErrorState)
	{
		messageReplyNOK(SET_CMD, INCOMPLETE_MESSAGE, parId, replyToId, replyToRef);
		return INCOMPLETE_MESSAGE;
	}

	const NOK_REASON_CODES r = setParameterValue(parId, value);
	if (r == 0)
	{
		const int64_t readBackValue = getParameterValue(parId, NULL);
		messageReplyToSetCommand(parId, readBackValue, replyToId, replyToRef);
	}
	else
	{
		messageReplyNOK(SET_CMD, r, parId, replyToId, replyToRef);
	}

	return r;
}


int64_t getParameterValue(PARAMETER_CODES parId, NOK_REASON_CODES *result)
{
	switch(parId)
	{
		case AUTOSTART_PAR:
		{
			return (ee.optionsBitMask >> AUTOSTART_BIT) & 1;
		}
		case CYCLES_TO_DO:
		{
			return getCyclesToDo();
		}
		case DEVICE_ID:
		{
			return ee.deviceId;
		}
		case MICRO_AMPS_PER_UNIT_DC:
		{
			return ee.microAmpsPerUnitDc;
		}
		case MICRO_VOLTS_PER_UNIT_DC:
		{
			return ee.microVoltsPerUnitDc;
		}
		case MACHINE_SESSION_NUMBER:
		{
			return machineSessionNumber;
		}
		case SUPERVICE_DEVIATION_FACTOR:
		{
			return ee.superviceDeviationFactor;
		}
		case FAIL_SCAN_IF_TO_MUCH_CURRENT:
		{
			return (ee.optionsBitMask >> FAIL_SCAN_IF_TO_MUCH_CURRENT_BIT) & 1;
		}
		case TIME_COUNTED_S:
		{
			return timeCounted_s;
		}
		case WALL_TIME_RECEIVED_MS:
		{
			return ee.wallTimeReceived_ms;
		}
		case WEB_SERVER_TIMEOUT_S:
		{
			return ee.webServerTimeout_s;
		}
		case TOTAL_CYCLES_PERFORMED:
		{
			return totalCyclesPerformed;
		}
		case WANTED_SCANNING_VOLTAGE_MV:
		{
			return ee.wantedInternalScanningVoltage_mV;
		}
		case WANTED_CURRENT_MA:
		{
			return ee.wantedCurrent_mA;
		}
		case WANTED_VOLTAGE_MV:
		{
			return ee.wantedVoltage_mV;
		}
		case MIN_TEMP_HZ:
		{
			return tempMinFreq;
		}
		case MAX_TEMP_HZ:
		{
			return ee.tempMaxFreq;
		}
		case TEMP_FREQ_AT_20C:
		{
			return ee.freqAt20C;
		}
		case TEMP_FREQ_AT_100C:
		{
			return ee.freqAt100C;
		}
		case SCAN_BEGIN_HZ:
		{
			return ee.scanBegin_Hz;
		}
		case SCAN_END_HZ:
		{
			return ee.scanEnd_Hz;
		}
		case RESONANCE_HALF_PERIOD:
		{
			return scanGetResonanceHalfPeriod();
		}
		case Q_VALUE:
		{
			return scanQ;
		}
		case PORTS_PWM_STATE:
		{
			return portsPwmState;
		}
		#if (defined PWM_PORT)
		case PWM1_STATE_PAR:
		{
			return pwm1State;
		}
		#endif
		case TARGET_TIME_S:
		{
			return secAndLogGetSeconds();
		}
		case PORTS_PWM_HALF_PERIOD:
		{
			return ports_get_period();
		}
		case SCAN_PEAK_WIDTH_IN_PERIOD_TIME:
		{
			return scanPeakWidthInPeriodTime;
		}
		case PORTS_PWM_ANGLE:
		{
			return portsGetAngle();
		}
		case INV_FREQUENCY_HZ:
		{
			//return TRANSLATE_HALF_PERIOD_TO_FREQUENCY(ports_get_period());
			return measuring_convertToHz(ports_get_period());
		}
		case PORTS_PWM_MIN_ANGLE:
		{
			return ee.portsPwmMinAngle;
		}
		case COOLDOWN_TIME_S:
		{
			return ee.minCooldownTime_s;
		}
		/*case REG_INTERPOLATION_TIME_PAR:
		{
			return ee.interpolationTimeDummy;
		}
		case REFERENCE_CURRENT_A_PAR:
		{
			return ee.referenceCurrent_A;
		}
		case REG_TIME_BASE_MS_PAR:
		{
			return ee.regTimeBase_msDummy;
		}
		case REG_MAX_STEP_UP_PPMPMS_PAR:
		{
			return ee.regMaxStepUp_ppmpmsDummy;
		}
		case REG_MAX_STEP_DOWN_PPMPMS_PAR:
		{
			return ee.regMaxStepDown_ppmpmsDummy;
		}*/
		case FILTERED_CURRENT_DC_MA:
		{
			return measuringGetFilteredDcCurrentValue_mA();
		}
		case FILTERED_INTERNAL_VOLTAGE_DC_MV:
		{
			return measuringGetFilteredInternalDcVoltValue_mV();
		}
		case MIN_Q_VALUE:
		{
			return ee.minQValue;
		}
		case MAX_Q_VALUE:
		{
			return ee.maxQValue;
		}
		case REPORTED_EXT_AC_VOLTAGE_MV:
		{
			return getMeasuredExternalAcVoltage_mV();
		}
		case SCAN_STATE_PAR:
		{
			return scanGetState();
		}
		case IS_PORTS_BREAK_ACTIVE:
		{
			return portsIsBreakActive();
		}
		#if (defined PWM_PORT)
		case IS_PWM1_BREAK_ACTIVE_PAR:
		{
			return pwm_isBreakActive();
		}
		#endif
		case RELAY_STATE_PAR:
		{
			return relayGetState();
		}
		case TEMP1_C:
		{
			return tempGetTemp1Measurement_C();
		}
		#ifdef USE_LPTMR2_FOR_TEMP2
		case TEMP2_C:
		{
			return tempGetTemp2Measurement_C();
		}
		#endif
		#ifdef USE_LPTMR2_FOR_FAN2
		case FAN2_HZ:
		{
			return fanGetFan2Measurement_Hz();
		}
		#endif
		#ifdef FAN1_APIN
		case FAN1_HZ:
		{
			return fanGetFan1Measurement_Hz();
		}
		#endif
		case MACHINE_REQUESTED_OPERATION:
		{
			return machineGetRequestedState();
		}
		case REPORTED_ERROR:
		{
			return errorGetReportedError();
		}
		case SCAN_DELAY_MS:
		{
			return ee.scanDelay_ms;
		}
		case MAX_ALLOWED_CURRENT_MA:
		{
			return maxAllowedCurrent_mA;
		}
		case MAX_ALLOWED_EXT_VOLT_MV:
		{
			return maxAllowedExternalVoltage_mV;
		}
		case STABLE_OPERATION_TIMER_S:
		{
			return stableOperationTimer;
		}
		case SUPERVICE_STATE:
		{
			return superviseGetState();
		}
		case COMP_VALUE:
		{
			return comp_getValue();
		}
		case DUTY_CYCLE_PPM:
		{
			return scanGetDutyCyclePpm();
		}
		case PORTS_CYCLE_COUNT:
		{
			return portsGetCycleCount();
		}
		case MICRO_AMPS_PER_UNIT_AC:
		{
			return ee.microAmpsPerUnitAc;
		}
		case MEASURED_AC_CURRENT_MA:
		{
			return measuringGetFilteredOutputAcCurrentValue_mA();
		}
		case SUPERVICE_STABILITY_TIME_S:
		{
			return ee.stableOperationTimer_s;
		}
		case SUPERVICE_DROP_FACTOR:
		{
			return ee.superviceDropFactor;
		}
		case PORTS_PWM_BRAKE_STATE:
		{
			return portsGetPwmBrakeState();
		}
		case VOLTAGE_PROBE_FACTOR:
		{
			return ee.voltageProbeFactor;
		}
		case RELAY_COOL_DOWN_REMAINING_S:
		{
			return relayCoolDownTimeRemaining_S();
		}
		case MEASURING_MAX_AC_CURRENT_MA:
		{
			return measuringGetAcCurrentMax_mA();
		}
		case MEASURING_MAX_DC_CURRENT_MA:
		{
			return measuringGetDcCurrentMax_mA();
		}
		case MEASURING_MAX_DC_VOLTAGE_MA:
		{
			return measuringGetDcVoltMax_mV();
		}
		case LOW_DUTY_CYCLE_PERCENT:
		{
			return ee.lowDutyCyclePercent;
		}
		/*case TEMP_FREQ_AT_60C_PAR:
		{
			return ee.freqAt60C;
		}*/
		case MAX_TEMP_C:
		{
			return intervallHalvingFromFreqToC(ee.tempMaxFreq);
		}
		case MACHINE_STATE:
		{
			return scanGetStateName();
		}
		case SYS_TIME_MS:
		{
			return systemGetSysTimeMs();
		}
		case SLOW_INTERNAL_VOLTAGE_MV:
		{
			return superviceLongTermInternalVoltage_uV();
		}
		case SUPERVICE_INC_FACTOR:
		{
			return ee.superviceIncFactor;
		}
		case SLOW_AC_CURRENT_MA:
		{
			return superviceLongTermAcCurrent_uA();
		}
		case EXT_TEMP1_C:
		{
			return getExtTemp1C();
		}
		case EXT_TEMP2_C:
		{
			return getExtTemp2C();
		}
		case MAX_EXT_TEMP_C:
		{
			return ee.maxExternalTemp_C;
		}
		case EXT_TEMP_REQUIRED:
		{
			return (ee.optionsBitMask >> EXT_TEMP_REQUIRED_BIT) & 1;
		}
		case EXT_VOLT_REQUIRED:
		{
			return (ee.optionsBitMask >> EXT_VOLT_REQUIRED_BIT) & 1;
		}
		case EXT_TEMP_STATE:
		{
			return getExtTempState();
		}
		case LOOP_REQUIRED:
		{
			return (ee.optionsBitMask >> LOOP_REQUIRED_BIT) & 1;
		}
		case TARGET_CYCLES_0:
		{
			return ee.targetCycles[0];
		}
		case TARGET_CYCLES_1:
		{
			return ee.targetCycles[1];
		}
		case TARGET_CYCLES_2:
		{
			return ee.targetCycles[2];
		}
		case TARGET_CYCLES_3:
		{
			return ee.targetCycles[3];
		}
		#if (NDEV >= 8)
		case TARGET_CYCLES_4:
		{
			return ee.targetCycles[4];
		}
		case TARGET_CYCLES_5:
		{
			return ee.targetCycles[5];
		}
		case TARGET_CYCLES_6:
		{
			return ee.targetCycles[6];
		}
		case TARGET_CYCLES_7:
		{
			return ee.targetCycles[7];
		}
		#endif
		case REACHED_CYCLES_0:
		{
			return ee.reachedCycles[0];
		}
		case REACHED_CYCLES_1:
		{
			return ee.reachedCycles[1];
		}
		case REACHED_CYCLES_2:
		{
			return ee.reachedCycles[2];
		}
		case REACHED_CYCLES_3:
		{
			return ee.reachedCycles[3];
		}
		#if (NDEV >= 8)
		case REACHED_CYCLES_4:
		{
			return ee.reachedCycles[4];
		}
		case REACHED_CYCLES_5:
		{
			return ee.reachedCycles[5];
		}
		case REACHED_CYCLES_6:
		{
			return ee.reachedCycles[6];
		}
		case REACHED_CYCLES_7:
		{
			return ee.reachedCycles[7];
		}
		#endif
		case SUPERVICE_MARGIN_MV:
		{
			return ee.superviceMargin_mV;
		}
		case MAX_EXT_TEMP_DIFF_C:
		{
			return ee.maxExtTempDiff_C;
		}
		case MAX_CURRENT_STEP:
		{
			return ee.maxCurrentStep_ppb;
		}
		case MAX_VOLTAGE_STEP:
		{
			return ee.maxVoltageStep_ppb;
		}
		case MAX_LEAK_CURRENT_MA:
		{
			return ee.maxLeakCurrentStep_mA;
		}
		case LEAK_CURRENT_MA:
		{
			return getMeasuredLeakSensor_mA();
		}
		case MIN_MV_PER_MA_AC:
		{
			return ee.min_mV_per_mA_ac;
		}

		default:
			if (result!=NULL)
			{
				*result = NOK_UNKOWN_PARAMETER;
			}
			else
			{
				// TODO log unknown parameter
			}
			break;
	}
	return 0;
}

// Returns 0 if OK, nonzero if not.
static int interpretGetCmd(DbfUnserializer *dbfPacket, int64_t replyToId, int64_t replyToRef)
{
	// The command get wanted current looks like this: "<get-command> <variable>".
	// The reply: <reply msg><variable><value>

	const int32_t parId = DbfUnserializerReadInt32(dbfPacket);

	NOK_REASON_CODES readBackResult = REASON_OK;
	const int64_t v = getParameterValue(parId, &readBackResult);
	if (readBackResult == REASON_OK)
	{
		messageReplyToGetCommand(parId, v, replyToId, replyToRef);
	}
	else
	{
		messageReplyNOK(GET_CMD, readBackResult, parId, replyToId, replyToRef);
	}

	return readBackResult;
}


//#if (defined __AVR_ATmega328P__) || (defined __linux__)
volatile char escCounter;
static void cmdReboot(void)
{
	scanBreak();

	//portsResetCommand();

	debug_print("\r\nresetting...\r\n");
	logInt1(CMD_RESETTING);

	// reset by triggering wdt, this way we can test that WDT works also.
	// TODO I have not yet gotten WDT to work on arm (AKA stm32).
    // https://stackoverflow.com/questions/25121107/nvic-systemreset-not-working-for-stm32f4
#ifdef __AVR_ATmega328P__
	startCmd = cmdPauseCommand;
	for(;;)
	{
		escCounter++;
	}
#else
	portsSetDutyOff();
	relayOff();
	//measuring_init();
	#if (defined PWM_PORT)
	pwm_clearBreak();
	#endif
	//portsDeactivateBreak(); // Not needed if scan will do it.
	scan_init();
	fanInit();
	tempInit();
	errorReset();
	machineRequestReset();
#endif
}
/*#else
// https://electronics.stackexchange.com/questions/31603/stm32-performing-a-software-reset
// That was complicated, tried to call 0 instead. No that did not work.
// https://stackoverflow.com/questions/8915797/calling-a-function-through-its-address-in-memory-in-c-c
typedef int func(void);
static void cmdReboot(void)
{
	uart_print_P(PSTR("\r\nresetting...\r\n"));
	systemSleepMs(100);
	func* f = (func*)0x0;
	int i = f();
}
#endif*/


static void cmdStop(void)
{
	machineRequestPause();
}

static void cmdStart(void)
{
	errorReset();
	machineRequestRun();
}

// Message format must match that in target_maintenance_sendRequestAllConfig3.
// Target will use wanted current etc sent from web server.
static void processAllConfig3Cmd(DbfUnserializer *dbfPacket)
{
	ee.wantedCurrent_mA = DbfUnserializerReadInt64(dbfPacket);
	ee.wantedVoltage_mV = DbfUnserializerReadInt64(dbfPacket);
	ee.scanBegin_Hz = DbfUnserializerReadInt64(dbfPacket);
	ee.scanEnd_Hz = DbfUnserializerReadInt64(dbfPacket);

	ee.voltageProbeFactor = DbfUnserializerReadInt64(dbfPacket);
	ee.maxExternalTemp_C = DbfUnserializerReadInt64(dbfPacket);
	ee.wantedInternalScanningVoltage_mV = DbfUnserializerReadInt64(dbfPacket);
	setParameterValue(MAX_TEMP_C, DbfUnserializerReadInt64(dbfPacket));
	setParameterValue(AUTOSTART_PAR, DbfUnserializerReadInt64(dbfPacket));
	setParameterValue(FAIL_SCAN_IF_TO_MUCH_CURRENT, DbfUnserializerReadInt64(dbfPacket));

	for(int i=0; i<NDEV;++i)
	{
		ee.targetCycles[i] = DbfUnserializerReadInt64(dbfPacket);
		ee.reachedCycles[i] = DbfUnserializerReadInt64(dbfPacket);
	}
	logInt5(CMD_CONFIG, 3, ee.scanBegin_Hz, ee.targetCycles[0], ee.reachedCycles[0]);
}

/*
This is an extract from msg.c (things might change, that file has precedence if mismatching)

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

Returns <0 if command not found. >=0 if OK.
See separate file protocoll.txt for more about these commands.
*/
static CmdResult cmdProcessCommand(DbfUnserializer *dbfPacket)
{
	const int64_t senderId = DbfUnserializerReadInt64(dbfPacket);
	const int64_t cmdDestId = DbfUnserializerReadInt64(dbfPacket);
	const int64_t replyToRef = DbfUnserializerReadInt64(dbfPacket);

	if ((ee.deviceId != 0) && (senderId == ee.deviceId))
	{
		// This message was sent by us.
		return CMD_RECEIVED_OWN_MESSAGE;
	}

	if ((cmdDestId != 0) && (cmdDestId != -1) && (senderId != ee.deviceId))
	{
		// This message was not to us.
		return CMD_RECEIVED_MESSAGE_NOT_TO_US;
	}

	// TODO We should require the device issuing commands to register itself before
	// accepting any commands. This to ensure that:
	//     There is only one device issuing commands.
	//     That if this device reboots the other end will have noticed that.
	// This can probably be done together with SEND_ALL_CONFIG_CMD.

	const int32_t cmd = DbfUnserializerReadInt32(dbfPacket);

	#if (defined DEBUG_UART)
	serialPrint(DEBUG_UART, "cmd ");
	serialPrintInt64(DEBUG_UART, cmd);
	serialPrint(DEBUG_UART, "\n");
	#endif

	switch(cmd)
	{
		case SEND_ALL_CONFIG1_CMD:
		{
			// This command will also establish the server-client connection.
			// If server identifies itself with correct magic number then
			// the the ID of server is remembered and only that server ID
			// is allowed to give some of the commands.

			magicNumberFromWebServer = DbfUnserializerReadInt64(dbfPacket);
			if (magicNumberFromWebServer != MAGIC_NUMBER_WEB_SERVER)
			{
				messageReplyNOK(cmd, WRONG_MAGIC_NUMBER, MAGIC_NUMBER_WEB_SERVER, senderId, replyToRef);
				return CMD_OK;
			}
			idOfWebServer = senderId;
			sendAllConfig1Reply(senderId, replyToRef);
			setConnectionState(INITIAL_CONFIG);

			// reset log times moved to SEND_ALL_CONFIG2_CMD and SEND_ALL_CONFIG3_CMD.
			//logResetTime();
			return CMD_OK;
		}
		case SEND_ALL_CONFIG2_CMD:
		{
			sendAllConfig2Reply(senderId, replyToRef);
			logResetTime();
			setConnectionState(ALL_CONFIG_DONE);
			// TODO Do we need a parameter to tell if configuration is OK? webServerConnected=1;
			return CMD_OK;
		}
		case SEND_ALL_CONFIG3_CMD:
		{
			processAllConfig3Cmd(dbfPacket);
			sendAllConfig3Reply(senderId, replyToRef);
			logResetTime();
			setConnectionState(ALL_CONFIG_DONE);
			// TODO Do we need a parameter to tell if configuration is OK? webServerConnected=1;
			return CMD_OK;
		}
		/*case ECHO_COMMAND:
		{
			// Echo command, the string following the "e" command is sent in the reply.
			uart_print_P(PSTR(REPLY_PREFIX  "echo "));
			//uart_print(cmdPtr);
			uart_print_P(PSTR(REPLY_SUFIX));
			return 0;
		}*/
		case GET_CMD:
		{
			// Expected format of a get command is: get <variable name>
			interpretGetCmd(dbfPacket, senderId, replyToRef);
			return CMD_OK;
		}
		case IGNORE_VOLTAGE_SENSOR_CMD:
		{
			if (checkSenderId(cmd, senderId))
			{
				return wrongSenderId(cmd, senderId, replyToRef);
			}

			// The "ignoreVoltageSensor" command tells machine to ignore voltage sensor (it will only use current sensor).
			ee.wantedVoltage_mV = NO_VOLT_VALUE;
			messageReplyOK(cmd, senderId, replyToRef);

			//uart_print_P(PSTR(REPLY_PREFIX  "ignoring voltage sensor" REPLY_SUFIX));
			return CMD_OK;
		}
		case REBOOT_CMD:
		{
			if (checkSenderId(cmd, senderId))
			{
				return wrongSenderId(cmd, senderId, replyToRef);
			}

			// The "reboot" command will have the machine reboot. "re" is deprecated, use "reboot" instead.
			// If autoStart is set then it will also start scanning for resonance and run.
			messageReplyOK(cmd, senderId, replyToRef);
			cmdReboot();
			return CMD_OK;
		}

		case SAVE_CMD:
			if (checkSenderId(cmd, senderId))
			{
				return wrongSenderId(cmd, senderId, replyToRef);
			}

			// The "save" command saves current settings to EEPROM/flash (non volatile memory).
			// Avoid doing this while machine is running, since this may take a little time, during which pulses can not be made.
			// pause/stop it first.
			if (!scanIdleOrPauseEtc())
			{
				messageReplyNOK(cmd, CANT_SAVE_WHILE_RUNNING, 0, senderId, replyToRef);
				eepromSetSavePending();
			}
			else
			{
				eepromSave();
				messageReplyOK(cmd, senderId, replyToRef);
			}
			return CMD_OK;
		case SET_CMD:
		{
			if (checkSenderId(cmd, senderId))
			{
				return wrongSenderId(cmd, senderId, replyToRef);
			}

			// Expected format of a set command is: set <variable name> <variable value in decimal>
			interpretSetCmd(dbfPacket, senderId, replyToRef);
			return 0;
		}
		case STOP_CMD:
			cmdStop();
			messageReplyOK(cmd, senderId, replyToRef);
			return CMD_OK;
		case START_CMD:
			// Command starts the machine (if not already auto started)
			// Command to start the machine. If the autoStart is set to 1 then
			// issuing this command is not needed. See "a" command.

			if (checkSenderId(cmd, senderId))
			{
				return wrongSenderId(cmd, senderId, replyToRef);
			}

			cmdStart();
			messageReplyOK(cmd, senderId, replyToRef);
			return CMD_OK;

		case SHOW_VERSION_CMD:
		{
			sendVersionReply(senderId, replyToRef);
			return CMD_OK;
		}
		case RESTORE_DEFAULT_CMD:
		{
			eepromSetDefault();
			messageReplyOK(RESTORE_DEFAULT_CMD, senderId, replyToRef);
			return CMD_OK;
		}
		default:
			//uart_print_P(PSTR(LOG_PREFIX "nok" LOG_SUFIX));
			messageReplyNOK(cmd, NOK_UNKNOwN_COMMAND, cmd, senderId, replyToRef);
			break;
	}

	return CMD_RECEIVED_UNKNOWN_MESSAGE;
}

static CmdResult checkSenderOnly(DbfUnserializer *dbfPacket)
{
	const int64_t senderId = DbfUnserializerReadInt64(dbfPacket);
	if ((ee.deviceId !=0) && (senderId == ee.deviceId))
	{
		// This message was sent by us.
		return CMD_RECEIVED_OWN_MESSAGE;
	}
	return CMD_OK;
}



/*
This is an extract from msg.c for convenience, if not consistent msg.c has precedence.
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

Returns <0 if something was wrong, command not found. >=0 if OK.
See separate file protocoll.txt for more about these commands.
*/
static CmdResult cmdProcessStatusMessage(DbfUnserializer *dbfPacket)
{
	/*const int64_t senderId =*/ DbfUnserializerReadInt64(dbfPacket);
	const int32_t typeOfStatusMsg = DbfUnserializerReadInt32(dbfPacket);

	/*#if (defined DEBUG_UART)
	usartPrint(DEBUG_UART, "msg ");
	usartPrintInt64(DEBUG_UART, typeOfStatusMsg);
	usartPrint(DEBUG_UART, "\n");
	#endif*/


	/*if ((ee.deviceId !=0) && (senderId == ee.deviceId))
	{
		// This message was sent by us.
		logInt2(RECEIVED_OWN_MESSAGE, senderId);
		return CMD_RECEIVED_OWN_MESSAGE;
	}*/


	switch(typeOfStatusMsg)
	{
		case VOLTAGE_STATUS_MSG:
		{
			return externalSensorProcessStatusMsg(dbfPacket);
		}
		case TEMP_STATUS_MSG:
		{
			return fanProcessExtTempStatusMsg(dbfPacket);
		}
		case LEAK_CURRENT_STATUS_MSG:
		{
			return externalLeakSensorProcessMsg(dbfPacket);
		}
		case OVER_VOLTAGE_STATUS_MSG:
		{
			// This means that the voltage sensor is out of range.
			// If it is far out of range the sensor may get damaged so stop machine.

			if (errorGetReportedError() == portsErrorOk)
			{
				logInt1(CMD_OVER_VOLTAGE_EXTERNAL);
			}

			errorReportError(portsErrorOverVoltage);
			return CMD_OK_FORWARD_ALSO;
		}
		case WEB_SERVER_STATUS_MSG:
		{
			return processWebServerStatusMsg(dbfPacket);
		}
		default:
			break;
	}
	return CMD_RECEIVED_UNKNOWN_MESSAGE;
}

// Returns non zero if there was something wrong with the message.
static CmdResult cmdProcessDbfMessage(DbfUnserializer *dbfPacket)
{
	const int32_t category = DbfUnserializerReadInt32(dbfPacket);

	#if (defined DEBUG_UART)
	serialPrint(DEBUG_UART, "category ");
	serialPrintInt64(DEBUG_UART, category);
	serialPrint(DEBUG_UART, "\n");
	#endif

	switch(category)
	{
		case STATUS_CATEGORY:
			return cmdProcessStatusMessage(dbfPacket);
		case COMMAND_CATEGORY:
			return cmdProcessCommand(dbfPacket);
		case REPLY_OK_CATEGORY:
		case REPLY_NOK_CATEGORY:
		case LOG_CATEGORY:
			// Ignore all these logs and this device never sends commands so ignore all replies.
			// TODO We need to forward messages received from one port to the others.
			return checkSenderOnly(dbfPacket);
		case UNKNOWN_CATEGORY:
		default:
			//uart_print_P(PSTR(LOG_PREFIX "nok" LOG_SUFIX));
			//replyNOK(cmd, NOK_UNKNOwN_CATEGORY, cmd);
			break;
	}

	return CMD_RECEIVED_UNKNOWN_MESSAGE;
}


static void processReceivedMessage(int usartDev, const DbfReceiver* dbfReceiver)
{
	DbfUnserializer u;
	const DBF_CRC_RESULT cr = DbfUnserializerInit(&u, dbfReceiver->buffer, dbfReceiver->msgSize);

	switch(cr)
	{
		case DBF_OK_CRC:
		{
			if (DbfUnserializerReadIsNextEnd(&u))
			{
				//uart_print_P(PSTR(LOG_PREFIX "empty" LOG_SUFIX));
				logInt1(UNEXPECTED_EMPTY_MESSAGE);
			}
			else if (DbfUnserializerReadIsNextString(&u))
			{
				//uart_print_P(PSTR(LOG_PREFIX "unexpected string" LOG_SUFIX));
				logInt1(UNEXPECTED_STRING_MESSAGE);
			}
			else if (DbfUnserializerReadIsNextInt(&u))
			{
				const CmdResult r = cmdProcessDbfMessage(&u);
				switch(r)
				{
					case CMD_OK:
						break;
					case CMD_RECEIVED_OWN_MESSAGE:
						break;
					case CMD_OK_FORWARD_ALSO:
					case CMD_SENDER_NOT_AUTHORIZED:
					case CMD_RECEIVED_MESSAGE_NOT_TO_US:
					default:
						forwardMessageToOthers(usartDev, dbfReceiver);
						break;
				}
			}
			else
			{
				logInt1(UNKNOWN_DBF_MESSAGE);
				//uart_print_P(PSTR(LOG_PREFIX "unknown DBF" LOG_SUFIX));
			}
			break;
		}
		case DBF_NO_CRC:
		case DBF_BAD_CRC:
		default:
			// Ignore these
			break;
	}
}

void cmdCheckSerialPort(int usartDev, DbfReceiver* dbfReceiver)
{
	// Check for input from serial port
	const int16_t ch = serialGetChar(usartDev);
	if (ch>=0)
	{
		//#if (defined DEBUG_UART)
		//usartPrint(DEBUG_UART, "ch ");
		//usartPrintInt64(DEBUG_UART, ch);
		//usartPrint(DEV_USART2, "\n");
		//#endif

		const char r = DbfReceiverProcessCh(dbfReceiver, ch);

		if (r > 0)
		{
			// A full message or line has been received.

			//usartPrint(DEV_USART2, "opto cmd\n");

			/*uart_print_P(PSTR(LOG_PREFIX));
			unsigned int i;
			for(i=0; i<cmdLine.msgSize; ++i)
			{
				printHex8(cmdLine.buffer[i]);
			}
			uart_print_P(PSTR(LOG_SUFIX));*/

			processReceivedMessage(usartDev, dbfReceiver);

			DbfReceiverInit(dbfReceiver);
		}
		else if (r == 0)
		{
			// We do not yet have a full message.
		}
		else
		{
			debug_print(LOG_PREFIX "something wrong" LOG_SUFIX);
			logInt1(CMD_INCORRECT_DBF_RECEIVED);
			DbfReceiverInit(dbfReceiver);
		}

	}
}

// This shall be called from main loop as often as possible.
// It is used to check for incoming messages.
void cmdFastTick(void)
{
	cmdCheckSerialPort(DEV_USART1 , &cmdLine1);

	#ifdef COMMAND_ON_LPUART1
	cmdCheckSerialPort(DEV_LPUART1 , &cmdLine0);
	#endif
	#ifdef COMMAND_ON_USART2
	cmdCheckSerialPort(DEV_USART2 , &cmdLine2);
	#endif
}

// This shall be called from main loop once per tick, typically once per millisecond.
// It is used for timeouts receiving messages.
void cmdMediumTick(void)
{
	DbfReceiverTick(&cmdLine1, 1);

	#ifdef COMMAND_ON_LPUART1
	DbfReceiverTick(&cmdLine0, 1);
	#endif
	#ifdef COMMAND_ON_USART2
	DbfReceiverTick(&cmdLine2, 1);
	#endif

}





void cmdInit(void)
{
	logInt1(CMD_INIT);

	setConnectionState(INITIAL_CONFIG);

	DbfReceiverInit(&cmdLine1);

	#ifdef COMMAND_ON_LPUART1
	DbfReceiverInit(&cmdLine0);
	#endif

	#ifdef COMMAND_ON_USART2
	DbfReceiverInit(&cmdLine2);
	#endif

	// cmdInit must be called after eepromLoad for this to work.
	// if not result may be unpredictable.
}




