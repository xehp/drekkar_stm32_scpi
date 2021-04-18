/*
portsPwm.c

Copyright (C) 2019 Henrik Björkman www.eit.se/hb.
All rights reserved etc etc...

provide functions to set up hardware

History

	2016-09-10
		Created.
		Henrik Bjorkman

	2019-04-11
		SW adapted to be used on this board:
			...CableAgeing2/kicad/controlboard_nucleo_4_inv/output.pdf
		Which in turn is used to drive drive this board:
			...CableAgeing2/kicad/mosfet_driver_alternating_logic/output.pdf
		Which in turn is used to drive drive this board:
			...CableAgeing2/kicad/igbt_inverter_only_4/output.pdf
		Henrik

*/

#include "cfg.h"



#if (defined __linux__)
#include <stdio.h>
#include <unistd.h>
//#include "simulated_hw.h"
//#include "linux_sim.h"
#elif (defined __arm__)
#include "systemInit.h"
#include "timerDev.h"
#else
#error
#endif

#include "supervise.h"
//#include "measuring.h"
#include "machineState.h"
#include "eeprom.h"
#include "scan.h"
#include "fan.h"
//#include "reg.h"
#include "relay.h"
#include "log.h"
#include "serialDev.h"
#include "messageUtilities.h"
#include "debugLog.h"
#include "portsPwm.h"


//const uint16_t MaxAllowedCurrent=0x6000; // Put twice EstimatedFullPower here

#define PORTS_LOG_DEBUG_INTERVALL 10L


// "inv_and_buck" Use one pin for both high and low,
// external HW will separate into high and low signals see:
// ...CableAgeing2/kicad/mosfet_driver_alternating_logic/output.pdf
// Pin PA_0 is used since channel TIM2_CH1 can be connected to this pin.
// See table 16 in stm32l431kc.pdf.
#define PORTS_BRIDGE_PORT GPIOA
//#define PORTS_BRIDGE_LEG1_HIGH_PIN 12
#define PORTS_BRIDGE_LEG1_HIGH_PIN 0


// An external D-latch is used for over current protection.
// At power on it shall be in the tripped state so try a reset.
// For pin number see controlboard_nucleo_4_inv, PB6
// The clear signal is active low.
#define PORTS_DLATCH_RESET_PORT GPIOB
#define PORTS_DLATCH_RESET_PIN 6
#define PORTS_DLATCH_STATE_PORT GPIOB
#define PORTS_DLATCH_STATE_PIN 7

// Very strange things, when PB6 is set high the PA6 is also set high.
// On Nucleo these pins seems to have been connected somehow. When starting to measure pins
// also PB7 and PA5 seems to be shorted together
// https://community.st.com/s/question/0D50X00009XkW7OSAV/l432kc-nucleo-unwanted-connection-between-pb6-spi1-and-spi3
// So there is a solder bridge SB16 between PA6 and PB6 on the Nucleo-L432KC
// And SB18 between PA5 and PB7. That was insane.




enum
{
	portsIdleState=0,
	portsStartState,
	portsWaitForTimeToIgniteHigh,
	portsWaitForEndOfHighHalfCycle,
	portsWaitForTimeToIgniteLow,
	portsWaitForEndOfLowHalfCycle,
	portsErrorReportedState
};


enum
{
	breakStateIdle=0,
	breakStateClearing2=1,
	breakStateClearing1=2,
	breakStateDcBreakLogged=3,
	breakStateDcBreakDetected=4,
};


//#define portsPwmSetLeg2Inactive() {systemOutPinHigh(PORTS_BRIDGE_PORT, PORTS_BRIDGE_LEG1_HIGH_PIN);}




int portsPwmState = portsIdleState;


static volatile int32_t tim2Count = 0;

static int reportedPortsBreakState=0;


static int breakState = breakStateIdle;


static void setState(const char newState)
{
	if (portsPwmState != newState)
	{
		logInt3(PORTS_STATE_CHANGED, portsPwmState, newState);
		portsPwmState=newState;
	}
}


static void inputPinInit(GPIO_TypeDef* port, int pin)
{
	// MODER
	// 00: Input mode
	// 01: General purpose output mode
	// 10: Alternate function mode
	// 11: Analog mode (reset state)
	const uint32_t m = 0U; // 0 for "General purpose input mode", 1 for "General purpose output mode".
	port->MODER &= ~(3U << (pin*2)); // Clear the 2 mode bits for this pin.
	port->MODER |= (m << (pin*2));

	// OTYPER
	// 0: Output push-pull (reset state)
	// 1: Output open-drain
	//port->OTYPER &= ~(0x1U << (pin*1)); // Clear output type, 0 for "Output push-pull".

	const uint32_t s = 1U; // 1 for "Medium speed" or 2 for High speed.
	port->OSPEEDR &= ~(0x3U << (pin*2)); // Clear the 2 mode bits for this pin.
	port->OSPEEDR |= (s << (pin*2));

	// Clear the 2 pull up/down bits. 0 for No pull-up, pull-down
	port->PUPDR &= ~(3U << (pin*2));
}



// Half period in our application.
uint32_t ports_get_period(void)
{
  return TIM2->ARR;
}

// Smaller angle means bigger duty cycle.
uint32_t portsGetAngle()
{
	return TIM2->CCR1;
}

void sendLogPortsError(int32_t measuredCurrent)
{
	logInitAndAddHeader(&messageDbfTmpBuffer, PORTS_ERROR);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, errorGetReportedError());
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, measuredCurrent);
	DbfSerializerWriteInt32(&messageDbfTmpBuffer, ports_get_period());
	messageSendDbf(&messageDbfTmpBuffer);
}

/*char portsGetStateName(void)
{
	switch(portsState)
	{
	case portsIdleState:
	case portsReady:
		return IDLE_STATUS;
	case portsWaitForTimeToIgniteHigh:
	case portsWaitForEndOfHighHalfCycle:
	case portsWaitForTimeToIgniteLow:
	case portsWaitForEndOfLowHalfCycle:
		return RUNNING_STATUS;
	default:
		break;
	}
	return UNKNOWN_STATUS;
}*/



int32_t portsGetCycleCount(void)
{
	// On a 32 bit CPU this operation is assumed to be atomic.
	// So we do not need to turn of interrupts.
	return tim2Count;
}



void portsEnterIdleState(void)
{
	logInt1(PORTS_WAITING_FOR_START_ORDER);
}

static void portsDLatchClearSetInactive()
{
	systemPinOutSetHigh(PORTS_DLATCH_RESET_PORT, PORTS_DLATCH_RESET_PIN);
}


// debug parameter to read this is: IS_PORTS_BREAK_ACTIVE_PAR
int portsIsBreakActive()
{
	return (reportedPortsBreakState) /* || (breakState==breakStateDcBreakLogged)*/;
}

int portsGetPwmBrakeState()
{
	return breakState;
}


/*
 * periodTime_t1 is given in units of 1/SysClockFrequencyHz.
 * NOTE that duty cycle is affected if half period is changed.
 * So angle may need to be adjusted also.
 */
void portsSetHalfPeriod(uint32_t periodTime_t1)
{
	if ((periodTime_t1 < (PORTS_MIN_ANGLE*5)) || (periodTime_t1 >= PORTS_OFF_ANGLE-PORTS_MIN_ANGLE))
	{
		errorReportError(portsAttemptToSetPeriodOutOfRange);
	}
	else
	{
		TIM2->ARR = periodTime_t1;
	}
}

static void portsSetDutyOn(uint32_t angle_t1)
{
	TIM2->CCR1 = angle_t1;
	if (portsPwmState == portsIdleState)
	{
		setState(portsStartState);
		// Enable needed TIM2 interrupts
	}
}

/**
 * angle How long to wait after period start to wait before calling activating output.
 * Smaller angle means larger duty cycle.
 * If angle is larger than half period then duty cycle shall be 0%.
 * NOTE that duty cycle is affected if half period is changed.
 * So angle may need to be adjusted also.
 * If changed there may happen a cycle where only one is yet changed.
 */
void portsSetAngle(uint32_t angle_t1)
{
	if ((errorGetReportedError() != portsErrorOk) || (breakState!=breakStateIdle))
	{
		portsSetDutyOff();
	}
	else
	{
		const uint32_t maxAngle = TIM2->ARR - PORTS_MIN_ANGLE;
		if (angle_t1 < PORTS_MIN_ANGLE)
		{
			// Minimum angle is maximum duty cycle.
			portsSetDutyOn(PORTS_MIN_ANGLE);
		}
		else if (angle_t1 > (maxAngle))
		{
			if (angle_t1==PORTS_OFF_ANGLE)
			{
				portsSetDutyOff();
			}
			else
			{
				portsSetDutyOn(maxAngle);
			}
		}
		else
		{
			portsSetDutyOn(angle_t1);
		}

	}
}


void portsSetDutyOff()
{
	TIM2->CCR1 = PORTS_OFF_ANGLE;
	setState(portsIdleState);
}

char portsIsActive(void)
{
	return (TIM2->CCR1 != PORTS_OFF_ANGLE);
}

void __attribute__ ((interrupt, used)) TIM2_IRQHandler(void)
{
	TIM2->SR = 0;

	switch (portsPwmState)
	{
		case portsIdleState:
			break;
		case portsStartState:
			if (breakState==breakStateIdle)
			{
				portsPwmState = portsWaitForTimeToIgniteLow;
			}
			else
			{
				TIM2->CCR1 = PORTS_OFF_ANGLE;
			}
			break;
		case portsWaitForTimeToIgniteHigh:
		case portsWaitForEndOfHighHalfCycle:
			portsPwmState = portsWaitForTimeToIgniteLow;
			break;
		case portsWaitForTimeToIgniteLow:
		case portsWaitForEndOfLowHalfCycle:
			tim2Count++;
			portsPwmState = portsWaitForTimeToIgniteHigh;
			break;
		case portsErrorReportedState:
		default:
			break;
	}
}






/**
[1] 27.4 TIM2/TIM3 registers
Pulse Width Modulation mode allows you to generate a signal with a frequency determined
by the value of the TIMx_ARR register and a duty cycle determined by the value of the
TIMx_CCRx register.
*/
static void tmr2Init()
{
	TIM2->CR1 = 0;

	// Enable Timer clock.
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN_Msk;

	systemBusyWait(2);


	// 1. Select the counter clock (internal, external, prescaler).
	// 0 For running fast, 0xFFFF for slow.
	// Need it to run fast.
	TIM2->PSC = 0;

	// 2. Write the desired data in the TIMx_ARR and TIMx_CCRx registers.
	// auto-reload register (TIMx_ARR)
	// If we want 500 Hz and system clock is at 80 MHz then reload shall be at:
	const uint32_t arr = 160000;
	TIM2->ARR = arr;

	TIM2->CCR1 = PORTS_OFF_ANGLE;

	// 3. Set the CCxIE bit if an interrupt request is to be generated.
	// no interrupt needed on this.

	// 4. Select the output mode.
	// capture/compare enable register
	// OC1PE: 1: Preload register on TIMx_CCR1 enabled.
	// Preload is very important or else some pulses might get much longer than
	// intended, very rarely but things may get damaged and it will be hard to
	// understand what went wrong.
	const uint16_t outputMode = 6;
	TIM2->CCMR1 |=  (outputMode << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE_Msk;

	// Need only one channel.
	// Enable Capture compare channel 1 and its inverted channel.
	// TIM_CCER_CC1P_Msk & TIM_CCER_CC1NP_Msk can be one or zero as long as they are the same.
	// [1] 26.3.15 "Complementary outputs and dead-time insertion"
	// CC1E: Capture/Compare 1 output enable
	//       1: On - OC1 signal is output on the corresponding output pin depending on MOE, OSSI, OSSR, OIS1, OIS1N and CC1NE bits.
	TIM2->CCER |= TIM_CCER_CC1E_Msk | TIM_CCER_CC1NE_Msk;





	// Enable the output pin in alternate mode.
	// [2] Table 15. STM32L431xx pin definitions
	setupIoPinTx(GPIOA, 0, GPIO_AF1_TIM2);



	// Set interrupt priority. Lower number is higher priority.
	// Lowest priority is: (1 << __NVIC_PRIO_BITS) - 1
	const uint32_t prio = (1 << __NVIC_PRIO_BITS) / 2;
	NVIC_SetPriority(TIM2_IRQn, prio);
	NVIC_EnableIRQ(TIM2_IRQn);
	TIM2->DIER |= /*TIM_DIER_CC1IE_Msk |*/ TIM_DIER_UIE_Msk;


	// ARPE: Auto-reload preload enable
	// This one is very important.
	// enable interrupt
	TIM2->CR1 = TIM_CR1_ARPE_Msk;

	// 5. Enable the counter by setting the CEN bit in the TIMx_CR1 register.
	// enable timer and update interrupt
	TIM2->CR1 |= TIM_CR1_CEN_Msk;


	// TODO Is there a brake pin for timer 2, as for TIM1->BDTR


	debug_print("tmr2Init\n");
}




void portsPwmInit(void)
{
	logInt1(PORTS_INIT);

	// Init DLatch Feedback Pin
	inputPinInit(PORTS_DLATCH_STATE_PORT, PORTS_DLATCH_STATE_PIN);

	// enable interrupts on PA6 for over current input
	// see portsGpioExti1Init instead.

	// Init DLatch Clear Pin
	portsDLatchClearSetInactive();
	systemPinOutInit(PORTS_DLATCH_RESET_PORT, PORTS_DLATCH_RESET_PIN);

	tmr2Init();

	portsEnterIdleState();
}







void portsSecondsTick()
{
	// TODO Currently only logging changes in DLatch feedback.
	// We could take an error action if feedback is not as expected.
	const int d = ((PORTS_DLATCH_STATE_PORT->IDR >> PORTS_DLATCH_STATE_PIN) & 1);

	if (reportedPortsBreakState != d)
	{
		logInt2(PORTS_DLATCH_STATE, d);
		reportedPortsBreakState = d;
	}

	switch(breakState)
	{
	case breakStateIdle:
		break;
	case breakStateClearing2:
		portsSetDutyOff();
		portsDLatchClearSetInactive();
		breakState = breakStateIdle;
		break;
	case breakStateClearing1:
		portsSetDutyOff();
		breakState = breakStateClearing2;
		break;
	case breakStateDcBreakLogged:
		portsSetDutyOff();
		break;
	case breakStateDcBreakDetected:
		portsSetDutyOff();
		logInt2(PORTS_DLATCH_STATE, d);
		breakState = breakStateDcBreakLogged;
		break;
	default:
		break;
	}
}


// To be called if ports shall turn off ASAP (some error detected).
void portsBreak()
{
	TIM2->CCR1 = PORTS_OFF_ANGLE;
	//tmr2Init();
	breakState = breakStateDcBreakDetected;
}


void portsDeactivateBreak()
{
	// Set DLatch Clear Active
	systemPinOutSetLow(PORTS_DLATCH_RESET_PORT, PORTS_DLATCH_RESET_PIN);
	breakState = breakStateClearing1;
}





