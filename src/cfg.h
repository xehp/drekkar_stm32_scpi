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

// PWM break shall be used, not disabled, this option is just for debugging
//#define DISABLE_PWM_BREAK_ON_PA6

// This was a test only
//#define MEASURE_COMP_AND_REF



#define USART1_BAUDRATE 115200
#define USART2_BAUDRATE 115200
#define SOFTUART1_BAUDRATE 9600

// Support for Low Power Uart (LPUART)
//#define LPUART1_BAUDRATE 9600



// Usart 2 can also be used for commands.
// Currently if probaly only recives commands un usart2
// it does not send replys to it.
#define COMMAND_ON_USART2


// SCPI interface can use USART2 or SOFTUART1
// If not using SCPI then use proprietary
// which will expect commands with voltage.
//#define SCPI_ON_USART2
//#define SCPI_ON_LPUART1
#define SCPI_ON_SOFTUART1
//#define DBF_VOLTAGE_SENSOR
//#define DIAC_VOLTAGE_SENSOR


// lptmr1 will typically be used to measure temperature
// pulses on port PB5 (AKA PB_5) but check in lptmr1Init() to be sure.
//#define USE_LPTMR1_FOR_TEMP1


// Port PB_1 (AKA PB1) is a timer input.
// It can be used to measure pulses from a fan or a temp sensor.
// NOTE Not both simultaneously.
// Fan can be measured with FAN1_APIN.
// If there is only one fan and only one temp sensor its better
// to use this for fan since the FAN1_APIN uses interrupts.
// LPTMR2 is also known as LPTIM2.
//#define USE_LPTMR2_FOR_TEMP2
//#define USE_LPTMR2_FOR_FAN2
//#define USE_LPTMR2_FOR_VOLTAGE2


// Enable if fan 1 shall be supervised on PA_11 (AKA PA11)
// This will tell which pin on port A that the fan input is connected to.
// This option one uses interrupts so only port A pins is currently supported.
// If LPTMR2 was not needed for temperature preferably use that one instead.
// Do not use 11 if it was used for relay 2 output.
// It should be possible to use some other A pin such as PA3 or PA12 (have not tested that).
//#define FAN1_APIN 11


// If voltage 2 sensor is not connected to LPTMR2 then it can be on any other Apin
// Voltage 2 is the sensor that provides the external AC voltage. That is
// voltage in the cable/coil resonance circuit.
// such as PA3 (AKA PA_3)
//#define VOLTAGE2_APIN 3





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
#define TEMP1_ADC_CHANNEL 9
#define TEMP2_ADC_CHANNEL 10

// Shall we read the STM32 internal temp sensor.
//#define TEMP_INTERNAL_ADC_CHANNEL 18


// Can not have 2 inputs for one sensor.
#if (defined USE_LPTMR2_FOR_VOLTAGE2) && (defined VOLTAGE2_APIN)
#error
#endif


// Currently PORTS_GPIO support only one A pin.
#if (defined FAN1_APIN) && (defined VOLTAGE2_APIN)
#error
#elif (defined FAN1_APIN)
#define PORTS_GPIO_APIN FAN1_APIN
#elif (defined VOLTAGE2_APIN)
#define PORTS_GPIO_APIN VOLTAGE2_APIN
#endif


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


#if (defined COMMAND_ON_USART2) || (defined SCPI_ON_USART2)
#ifndef USART2_BAUDRATE
#error
#endif
#endif

#if (defined COMMAND_ON_LPUART1) && (!defined LPUART1_BAUDRATE)
#error
#endif

// Can't read external voltage in more than one way.
#if (defined USE_LPTMR2_FOR_VOLTAGE2) && (defined SCPI_ON_USART2)
#error
#endif

#if (defined USE_LPTMR2_FOR_VOLTAGE2) && (defined VOLTAGE2_APIN)
#error
#endif

#if (defined USE_LPTMR2_FOR_VOLTAGE2) || (defined VOLTAGE2_APIN)
#ifndef DIAC_VOLTAGE_SENSOR
#error
#endif
#endif



#endif

