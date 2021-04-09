/*
avr_cfg.h

provide functions to set up hardware

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...


History

2005-03-23
Created by Henrik Bjorkman

2018-07-15
Adapted for using STM32.

*/


#ifndef CFG_HDR_H_
#define CFG_HDR_H_



#define USART1_BAUDRATE 115200
#define USART2_BAUDRATE 115200
#define SOFTUART1_BAUDRATE 9600

// Support for Low Power Uart (LPUART)
//#define LPUART1_BAUDRATE 9600

// Usart1 is the one connected to our opto link
// It is used for receiving commands.
#define COMMAND_ON_USART1


// Usart 2 can also be used for commands.
// Currently it probably only receives commands on usart2
// it might not send replies to it (due to missing HW).
#define COMMAND_ON_USART2


// SCPI interface can use USART2 or SOFTUART1
// If not using SCPI then use proprietary
// which will expect commands with voltage.
// Uncomment one (not 2 or 3) below to use SCPI.
//#define SCPI_ON_USART2
//#define SCPI_ON_LPUART1
#define SCPI_ON_SOFTUART1




// Port PB_1 (AKA PB1) is a timer input.
// LPTMR2 is also known as LPTIM2.
//#define USE_LPTMR2_FOR_FAN2


// Enable if fan 1 shall be supervised on PA_11 (AKA PA11)
// This will tell which pin on port A that the fan input is connected to.
// This option uses interrupts so only port A pins is currently supported.
// TODO Use LPTMR1 instead see fan 2 how to do that.
//#define FAN1_APIN 11




/*
NOTE on selecting channels
Some channels are faster than others. See [1] chapter 16.4.4 "ADC1/2 connectivity".
Sadly only one fast channel is available on Nucleo.
To find the external pins available see [6] chapter 4 "Pinouts and pin description".
Fast channels
ADC1_IN1  PC0
ADC1_IN2  PC1
ADC1_IN3  PC2
ADC1_IN4  PC3
ADC1_IN5  PA0

Slow channels
ADC1_IN6  PA1
ADC1_IN7  PA2
ADC1_IN8  PA3
ADC1_IN9  PA4
ADC1_IN10 PA5
ADC1_IN11 PA6
ADC1_IN12 PA7
ADC1_IN13 PC4
ADC1_IN14 PC5
ADC1_IN15 PB0
ADC1_IN16 PB1
*/


// If temperatures shall be measured also
// channel 9 is on PA4
// channel 10 is on PA5
#define TEMP1_ADC_CHANNEL 9
#define TEMP2_ADC_CHANNEL 10

// Shall we read the STM32 internal temp sensor.
//#define TEMP_INTERNAL_ADC_CHANNEL 18


// Measure current on PA1, channel ADC1_IN6
//#define CURRENT_ADC_CHANNEL 6



// Sometimes we run out of ports on the main box.
// So we daisy chain the sensors. Here we configure how
// messages are forwarded.
#define FORWARD_USART1_TO_USART1
#define FORWARD_USART1_TO_USART2
//#define FORWARD_USART1_TO_SOFTUART1
//#define FORWARD_USART1_TO_LPUART1
#define FORWARD_USART2_TO_USART1
#define FORWARD_USART2_TO_USART2
//#define FORWARD_USART2_TO_SOFTUART1
//#define FORWARD_USART2_TO_LPUART1
//#define FORWARD_SOFTUART1_TO_USART1
//#define FORWARD_SOFTUART1_TO_USART2
//#define FORWARD_SOFTUART1_TO_SOFTUART1
//#define FORWARD_SOFTUART1_TO_LPUART1
//#define FORWARD_LPUART1_TO_USART1
//#define FORWARD_LPUART1_TO_USART2
//#define FORWARD_LPUART1_TO_SOFTUART1
//#define FORWARD_LPUART1_TO_LPUART1



// If interlocking uses PA3 then usart2 can not also be used.
#ifdef USART2_BAUDRATE
#if INTERLOCKING_LOOP_PIN == 3
#error
#endif
#endif

// Usart can not be used for both commands and SCPI.
#if (defined COMMAND_ON_USART2) && (defined SCPI_ON_USART2)
#error
#endif

// If usart2 is used its baudrate must be set.
#if (defined COMMAND_ON_USART2) || (defined SCPI_ON_USART2)
#ifndef USART2_BAUDRATE
#error
#endif
#endif

// If soft uart is used its baudrate must be set.
#ifdef SCPI_ON_SOFTUART1
#ifndef SOFTUART1_BAUDRATE
#error
#endif
#endif

// If commands shall be sent/recieved on lpuart its baudrate must be set.
#if (defined COMMAND_ON_LPUART1) && (!defined LPUART1_BAUDRATE)
#error
#endif


#endif

