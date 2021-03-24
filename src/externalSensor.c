
#include "eeprom.h"
#include "systemInit.h"
#include "log.h"
#include "machineState.h"
#include "externalSensor.h"
//#include "mainSeconds.h"

/*
The purpose of this is to process messages from an external voltage sensor.
*/

#if (defined SCPI_ON_USART2) || (defined SCPI_ON_SOFTUART1)

// In this configuration a SCPI capable multimeter is expected, such as this one:
// http://bkpmedia.s3.amazonaws.com/downloads/manuals/en-us/5492B_manual.pdf

// When staring up check if voltmeter is connected:
// Send a command:
// :func?
// The reply shall be:
// volt:dc
// Then just send fetch command once every 100 ms:
// :FETCh?

#include <ctype.h>
#include <inttypes.h>
#include "debugLog.h"
#include "messageUtilities.h"
#include "serialDev.h"


// Check that configuration make sense and select serial device to use.
#if (defined SCPI_ON_USART2)
#define SCPI_DEV DEV_USART2
#elif (defined SCPI_ON_SOFTUART1)
#define SCPI_DEV DEV_SOFTUART1
#else
#error
#endif

enum{
	initalState=0,
	waitForSetFuncRelpyState=1,
	verifyFunc=2,
	waitForFuncReply=3,
	fetchValue=4,
	waitFetchReply=5,
};


static int esState = initalState;
static int esCounter=0;

static int64_t voltage_mv2=0;
static int64_t voltage_mv1=0;
static int64_t voltage_mv0=0;


static int64_t voltage_mv=0;
//static int senderState=0;
//static int senderCounter=0;

static char rcvBuffer[256];
static int rcvCount=0;
static int rcvMessageReceived=0;

static int nOfvaluesAvailable=0;
static int noNeedToQueryFunc=0;
static int inStateCounter=0;

static int my_isspace(int ch)
{
	return (ch==' ') || (ch=='\n') || (ch=='\r') || (ch=='\t');
}

static int my_isdigit(int ch)
{
	return (ch>='0') && (ch <='9');
}

static int to_upper(int a)
{
	if ((a >= 'a' ) && (a <= 'z' ))
	{
		return a - ('a'-'A');
	}
	return a;
}


static int my_stricmp(const char *str1, const char *str2)
{
	for(;;)
	{
		const int ch1 = to_upper(*str1);
		const int ch2 = to_upper(*str2);

		if (ch1 != ch2)
		{
			return -1;
		}
		if (ch1 == 0)
		{
			return 0;
		}
		++str1;
		++str2;
	}
}


static const char * decodeDigits(const char *str, int64_t *result, int *nOfDigits)
{
    for(;;)
    {
    	int ch=*str;
    	if (my_isdigit(ch))
    	{
    		*result = (10 * (*result)) + (ch - '0');
    		++(*nOfDigits);
    		++str;
    	}
    	else
    	{
    		break;
    	}
    }
    return str;
}


static const char* decodeIntegerNumber(const char *str, int64_t *result)
{
	int isNegative = 0;

    if (*str == '-') {
    	isNegative = 1;
    	++str;
    }
    else if (*str == '+')
    {
	    ++str;
	}

    int64_t i = 0;
    for(;;)
    {
    	int ch=*str;
    	if (my_isdigit(ch))
    	{
    		i = (10 * i) + (ch - '0');
    		++str;
    	}
    	else
    	{
    		break;
    	}
    }

    if (isNegative)
    {
    	*result = -i;
    }
    else
    {
    	*result = i;
    }

    return str;
}


/**
This function decodes the SCPI "Numeric Representation format".

Give precision as:
	3 if millis are wanted
	6 if micros

Returns:
    0 : OK
   <0 : Not OK
*/
static int decodeScientific(const char *str, int precision, int64_t *result)
{
	// decode integer, decimals and exponent.
	int64_t i=0, d=0, e=0;
	int n=0;
	// Example for values of i,d,e
	// -5.263926e-1  then i = -5, d = 263926, e = -1, n = 6

	while(my_isspace(*str))
	{
		++str;
	}

	int isNegative = 0;

    if (*str == '-') {
    	isNegative = 1;
    	++str;
    }
    else if (*str == '+')
    {
	    ++str;
	}


	if ((!my_isdigit(*str)) && ((*str)!='.'))
	{
		return -1;
	}

	str=decodeIntegerNumber(str, &i);

	if (*str == '.')
	{
		++str;
		str= decodeDigits(str, &d, &n);
	}

	if ((*str == 'e') || (*str == 'E'))
	{
		++str;
		str=decodeIntegerNumber(str, &e);
	}

	e+=precision;

	if (e>16)
	{
		// Out of range for 64 bit integer.
		return -2;
	}

	if (e<-50)
	{
		// rounded to zero for an integer.
		*result = 0;
		return 0;
	}


	// from decimals keep only as many as e say.
	while (n>e)
	{
		d = d/10;
		--n;
	}


	// Or if number of digits is less then add some.
	while (n<e)
	{
		d = d*10;
		++n;
	}

	// Adjust integer part also
	while (e>0)
	{
		i *= 10;
		--e;
	}

	while (e<0)
	{
		i /= 10;
		++e;
	}


	if (isNegative)
	{
		*result = -(i + d);
	}
	else
	{
		*result = i + d;
	}

	return 0;
}


static void checkUart()
{
	if (rcvMessageReceived != 0)
	{
		rcvCount = 0;
		rcvMessageReceived = 0;
	}

	for(;;)
	{
		if (rcvCount >= sizeof(rcvBuffer))
		{
			rcvMessageReceived = 1;
			break;
		}
		const int ch = serialGetChar(SCPI_DEV);
		if (ch < 0)
		{
			break;
		}
		else if ((ch == '\r') || (ch == '\n'))
		{
			rcvBuffer[rcvCount]=0;
			rcvCount++;
			rcvMessageReceived = 1;

			// Just for debugging, comment this out later.
			debug_print("SCPI in ");
			debug_print(rcvBuffer);
			debug_print("\n");

			break;
		}
		else
		{
			rcvBuffer[rcvCount]=ch;
			rcvCount++;
		}
	}
}

static int isExpectedMessageReceived(const char *expectedMessage)
{
	if (!rcvMessageReceived) {return 0;}
	if (my_stricmp(rcvBuffer, expectedMessage)==0) {return 1;}
	return 0;
}


static void sendVoltageMessage(int64_t voltage_mv)
{
	// printing in ascii was used for debugging, can be removed later.
	// This should typically be sent on usart2 (USB)
	debug_print("Voltage ");
	debug_print64(voltage_mv);
	debug_print("mv\n");

	// This should typically be sent on usart1 (opto link)
	messageInitAndAddCategoryAndSender(&messageDbfTmpBuffer, STATUS_CATEGORY);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, VOLTAGE_STATUS_MSG);
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, systemGetSysTimeMs());
	DbfSerializerWriteInt64(&messageDbfTmpBuffer, voltage_mv);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, 0); // reserved for frequency
	messageSendDbf(&messageDbfTmpBuffer);
}

static void sendScpiMessage(const char* msg)
{
	// Just for debugging, comment this out later.
	debug_print("SCPI out ");
	debug_print(msg);
	debug_print("\n");

	serialPrint(SCPI_DEV, msg);
	serialPrint(SCPI_DEV, "\n");
}

static void resetRcv()
{
	rcvMessageReceived = 0;
	rcvCount = 0;
	rcvBuffer[0]=0;
}

static void enterInitalState()
{
	nOfvaluesAvailable = 0;
	voltage_mv2=0;
	voltage_mv1=0;
	voltage_mv0=0;
	voltage_mv = 0;
	esCounter = 2000;
	noNeedToQueryFunc=0;
	inStateCounter=0;
	esState = initalState;
	debug_print("SCPI initial state\n");
}

static void enterQueryFuncState()
{
	debug_print("SCPI verify state\n");
	esCounter = 10;
	esState = verifyFunc;
}

static void enterWaitForSetFuncRelpyState()
{
	nOfvaluesAvailable = 0;
	voltage_mv = 0;
	esState = waitForSetFuncRelpyState;
	esCounter = 1000;
}

static void enterFetchValueState()
{
	rcvMessageReceived = 0;
	rcvCount = 0;
	rcvBuffer[0]=0;

	esState = fetchValue;
	esCounter = 10;
}

static void enterWaitForFuncReply()
{
	esState = waitForFuncReply;
	esCounter = 500;
	voltage_mv = 0;
}

static void enterWaitFetchReply()
{
	esState = waitFetchReply;
	esCounter = 500;
}


void externalSensorInit()
{
	// usarts are initialized by main.

	systemSleepMs(200);

	enterInitalState();
}


int scientificMessageReceived()
{
	if (!rcvMessageReceived) {return 0;}

	int64_t tmpVoltage_mv;
	int r = decodeScientific(rcvBuffer, 3, &tmpVoltage_mv);
	if (r == 0)
	{
		// Filter out extreme values in case of transmission errors.
		// easy way is to take median value of last 3 values.

		voltage_mv2=voltage_mv1;
		voltage_mv1=voltage_mv0;
		voltage_mv0=tmpVoltage_mv;

		if (nOfvaluesAvailable>=2)
		{
			if ((voltage_mv0 >= voltage_mv1) && (voltage_mv0 <= voltage_mv2))
			{
				voltage_mv = voltage_mv0;
			}
			else if ((voltage_mv0 <= voltage_mv1) && (voltage_mv0 >= voltage_mv2))
			{
				voltage_mv = voltage_mv0;
			}
			else if ((voltage_mv1 >= voltage_mv2) && (voltage_mv1 <= voltage_mv0))
			{
				voltage_mv = voltage_mv1;
			}
			else if ((voltage_mv1 <= voltage_mv2) && (voltage_mv1 >= voltage_mv0))
			{
				voltage_mv = voltage_mv1;
			}
			else if ((voltage_mv2 >= voltage_mv1) && (voltage_mv2 <= voltage_mv0))
			{
				voltage_mv = voltage_mv2;
			}
			else if ((voltage_mv2 <= voltage_mv1) && (voltage_mv2 >= voltage_mv0))
			{
				voltage_mv = voltage_mv2;
			}
			else
			{
				// This should not happen
				debug_print("Median value of 3 failed.\n");
				voltage_mv = tmpVoltage_mv;
			}

			sendVoltageMessage(voltage_mv);
		}
		else
		{
			nOfvaluesAvailable++;
		}
		return 1;
	}

	return 0;
}


void externalSensorMediumTick()
{
	checkUart();

	// Run the state machine once per 100 ms or so.
	switch(esState)
	{
		default:
		case initalState:
			if (esCounter>0)
			{
				esCounter--;
			}
			else
			{
				enterWaitForSetFuncRelpyState();
			}
			break;
		case waitForSetFuncRelpyState:
			// It seems there is not reply on the "FUNC VOLT:AC" command
			// so we just wait a little here (a second or so).
			if (isExpectedMessageReceived("FUNC VOLT:AC"))
			{
				resetRcv();
			}
			else if (isExpectedMessageReceived("VOLTage:AC:NPLCycles 10"))
			{
				resetRcv();
			}
			else if (esCounter>0)
			{
				esCounter--;
			}
			else
			{
				switch(inStateCounter)
				{
				default:
				case 0:
					// Sending the set function command.
					sendScpiMessage("FUNC VOLT:AC");
					enterWaitForSetFuncRelpyState();
					inStateCounter = 1;
					break;
				case 1:
					// Perhaps try "VOLTage:AC:NPLCycles 10" also?
					sendScpiMessage("VOLTage:AC:NPLCycles 10");
					enterWaitForSetFuncRelpyState();
					inStateCounter = 2;
					break;
				case 2:
					enterQueryFuncState();
					break;
				}
			}
			break;
		case verifyFunc:
			if (esCounter>0)
			{
				esCounter--;
			}
			else
			{
				sendScpiMessage("FUNC?");
				enterWaitForFuncReply();
			}
			break;
		case waitForFuncReply:
			// Waiting for the "volt:dc" reply message.
			if (isExpectedMessageReceived("VOLT:AC"))
			{
				// The multimeter is set to desired function.
				// We can continue.
				// set a short delay and ask for e measurement.
				resetRcv();
				inStateCounter = 0;
				noNeedToQueryFunc = 16;
				enterFetchValueState();
			}
			else if (isExpectedMessageReceived("FUNC?"))
			{
				// Ignore this, its just an echoing of our message.
				resetRcv();
			}
			else if (esCounter>0)
			{
				esCounter--;
			}
			else
			{
				// Timeout, go back to try sending ":func?" etc again.
				debug_print("SCPI func timeout\n");
				if (++inStateCounter>3)
				{
					nOfvaluesAvailable=0;
					enterInitalState();
				}
				else
				{
					enterQueryFuncState();
				}
			}
			break;
		case fetchValue:
			if (esCounter>0)
			{
				esCounter--;
			}
			else
			{
				sendScpiMessage("FETC?");
				enterWaitFetchReply();
			}
			break;
		case waitFetchReply:
		{
			if (scientificMessageReceived())
			{
				// Good we got a reading, set a short delay and ask for more.
				resetRcv();
				if (--noNeedToQueryFunc>0)
				{
					enterFetchValueState();
				}
				else
				{
					enterQueryFuncState();
				}
			}
			else if (isExpectedMessageReceived("FETC?"))
			{
				// Ignore this, its just an echoing of our message.
				resetRcv();
			}
			else if (esCounter>0)
			{
				esCounter--;
			}
			else
			{
				// Timeout
				debug_print("SCPI fetch timeout\n");
				enterQueryFuncState();
			}
			break;
		}
	}

	if (rcvMessageReceived)
	{
		debug_print("SCPI ignored ");
		debug_print(rcvBuffer);
		debug_print("\n");

		resetRcv();
	}
}

// This is in case a voltage reading is received on the other usart.
// That is an error if it happens in this configuration.
CmdResult externalSensorProcessStatusMsg(DbfUnserializer *dbfPacket)
{
	errorReportError(portsErrorUnexpecetedMessage);
	// Don't echo this, it might be sent frequently
	return CMD_RECEIVED_UNKNOWN_MESSAGE;
}


int32_t getMeasuredExternalAcVoltage_mV()
{
	return voltage_mv;
}

int getMeasuredExtAcVoltageIsAvailable()
{
	return (nOfvaluesAvailable>=3);
}

// Check that configuration make sense.
#if (defined SCPI_ON_USART2)
	#ifndef USART2_BAUDRATE
	#error
	#endif
#elif (defined SCPI_ON_SOFTUART1)
	#ifndef SOFTUART1_BAUDRATE
	#error
	#endif
#elif (defined SCPI_ON_LPUART1)
#ifndef LPUART1_BAUDRATE
#error
#endif
#else
#error
#endif


#elif (defined DIAC_VOLTAGE_SENSOR)

// Not implemented yet
#error

#else

#error

#endif


