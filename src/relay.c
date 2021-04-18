/*
relay.c

provide functions to set up hardware

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2019-03-09
Created.
Henrik Bjorkman
*/

#include "cfg.h"

#if (defined __linux__)
#include <stdio.h>
#include <unistd.h>
#elif (defined __arm__)
#include "systemInit.h"
#include "timerDev.h"
#else
#error
#endif


#include "machineState.h"
#include "messageNames.h"
#include "log.h"
#include "eeprom.h"
#include "relay.h"

// Give NTCs some time to cool down. They typically need 30 seconds.
#define MIN_COOL_DOWN_TIME_S (ee.minCooldownTime_s)

#define DELAY_BETWEEN_1_AND_2_S 6

enum{
	coolingDownRelayState = 0,
	coolRelayState,
	poweringUpRelayState,
	onRelayState
};


static int relayCounter=0;
static int relay1WantedOn=0;
static int relay2WantedOn=0;
static int relayWantedOff=0;
static int relayState=coolingDownRelayState;
static int64_t ntcHotTimeStamp = 0;

static int pinOutGet(GPIO_TypeDef* port, int pin)
{
	return port->ODR & (0x1U << pin);
}


static void allRelaysOff()
{
	systemPinOutSetLow(CONTACTOR1_PORT, CONTACTOR1_PIN);
	#ifdef CONTACTOR2_PIN
	systemPinOutSetLow(CONTACTOR2_PORT, CONTACTOR2_PIN);
	#endif
}

static void relaySetState(int newState)
{
	logInt3(RELAY_STATE, relayState, newState);
	relayState = newState;
}

static void enterStateCoolingDown()
{
	allRelaysOff();
	relay1WantedOn = 0;
	relay2WantedOn = 0;
	relayWantedOff = 0;
	relayCounter = DELAY_BETWEEN_1_AND_2_S;
	relaySetState(coolingDownRelayState);
}

static void enterStatePoweringUp()
{
	systemPinOutSetHigh(CONTACTOR1_PORT, CONTACTOR1_PIN);
	relayCounter = DELAY_BETWEEN_1_AND_2_S;
	relaySetState(poweringUpRelayState);
}

int relayGetState()
{
	return relayState;
}

void relayInit(void)
{
	allRelaysOff();
	systemPinOutInit(CONTACTOR1_PORT, CONTACTOR1_PIN);
	#ifdef CONTACTOR2_PIN
	systemPinOutInit(CONTACTOR2_PORT, CONTACTOR2_PIN);
	#endif
	allRelaysOff();
	enterStateCoolingDown();
	ntcHotTimeStamp = systemGetSysTimeMs();
}

void relay1On(void)
{
	relay1WantedOn = 1;
	relay2WantedOn = 0;
	relayWantedOff = 0;
	logInt7(RELAY_ORDER, 1, relayState, relayCounter, relay1WantedOn, relay2WantedOn, relayWantedOff);
}


void relay1Off2On(void)
{
	relay1WantedOn = 0;
	relay2WantedOn = 1;
	relayWantedOff = 0;
	logInt7(RELAY_ORDER, 2, relayState, relayCounter, relay1WantedOn, relay2WantedOn, relayWantedOff);
}


void relayOff(void)
{
	// Turn relay/contactor off.
	allRelaysOff();
	relay1WantedOn = 0;
	relay2WantedOn = 0;
	relayWantedOff = 1;
	logInt7(RELAY_ORDER, 3, relayState, relayCounter, relay1WantedOn, relay2WantedOn, relayWantedOff);

	// When Relay has been opened we can do a save to flash if a save is pending.
	eepromSaveIfPending();
}



int relayCoolDownTimeRemaining_S()
{
	const int64_t minCooldDownTime_ms = (MIN_COOL_DOWN_TIME_S * 1000);
	const int64_t SysTime_ms = systemGetSysTimeMs();
	const int64_t coolingTimeRelay1_ms = SysTime_ms - ntcHotTimeStamp;
	if (coolingTimeRelay1_ms < minCooldDownTime_ms)
	{
		return (minCooldDownTime_ms - coolingTimeRelay1_ms )/ 1000;
	}
	if (relayState == coolingDownRelayState)
	{
		return relayCounter;
	}
	return 0;
}


void relaySecTick()
{
	//logInt7(RELAY_ORDER, 5, relayState, relayCounter, relay1WantedOn, relay2WantedOn, relayWantedOff);
	switch(relayState)
	{
	    default:
		case coolingDownRelayState:
		{
			const int64_t minCooldDownTime_ms = (MIN_COOL_DOWN_TIME_S * 1000);
			const int64_t SysTime_ms = systemGetSysTimeMs();
			const int64_t coolingTimeRelay1_ms = SysTime_ms - ntcHotTimeStamp;
			// In this state there is a delay so that NTCs cool down.
			allRelaysOff();
			if (relayCounter>0)
			{
				// A little extra delay
				relayCounter--;
			}
			else if (coolingTimeRelay1_ms < minCooldDownTime_ms)
			{
				// Wait for NTCs to cool.
			}
			else
			{
				relayWantedOff = 0;
				relaySetState(coolRelayState);
				relayCounter = 0;
			}
			break;
		}
		case coolRelayState:
			// In this state the NTCs should be cool.
			allRelaysOff();
			if (errorGetReportedError() != portsErrorOk)
			{
				// stay in state, wait for error to be cleared.
			}
			else if ((relay1WantedOn) || (relay2WantedOn))
			{
				// Turn relay/contactor on.
				relay1WantedOn = 0;
				enterStatePoweringUp();
			}
			break;
		case poweringUpRelayState:
			if ((errorGetReportedError() != portsErrorOk) || (relayWantedOff))
			{
				enterStateCoolingDown();
			}
			else if (relayCounter>0)
			{
				relayCounter--;
			}
			else
			{
				// If we have 2 relays turn off first and turn on second.
				// If not go to next state.
				#ifdef CONTACTOR2_PIN
				if (relay2WantedOn)
				{
					systemPinOutSetLow(CONTACTOR1_PORT, CONTACTOR1_PIN);
					systemPinOutSetHigh(CONTACTOR2_PORT, CONTACTOR2_PIN);
					relaySetState(onRelayState);
					relay2WantedOn = 0;
				}
				#else
				relaySetState(onRelayState);
				relay2WantedOn = 0;
				#endif
			}
			break;
		case onRelayState:
			// If we have 2 relays turn off first one now.
			if ((errorGetReportedError() != portsErrorOk) || (relayWantedOff))
			{
				relayWantedOff = 0;
				enterStateCoolingDown();
			}
			break;
	}

	if (pinOutGet(CONTACTOR1_PORT, CONTACTOR1_PIN))
	{
		ntcHotTimeStamp = systemGetSysTimeMs();
	}
}

int relayIsReadyToScan()
{
	return (relayState == onRelayState);
}
