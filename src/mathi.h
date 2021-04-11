/*
mathi.h

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2016-08-15
Created for dielectric test equipment
Henrik

*/
#ifndef MATHI_H
#define MATHI_H

#include <inttypes.h>
#include <limits.h>

#include "machineState.h"



// TODO supposedly there is a better way: http://www.cplusplus.com/faq/sequences/arrays/sizeof-array/#cpp
#define SIZEOF_ARRAY( a ) (sizeof( a ) / sizeof( a[ 0 ] ))


#ifndef LLONG_MAX
#define LLONG_MAX  9223372036854775807LL
#endif

#define uint32_tMaxValue 0xFFFFFFFF


// This shall give a/b but result is rounded up not down as normal
// Negative numbers? TODO is this correct for negative values?
#define DIV_ROUND_UP(a,b) (((a)+((b)-1))/(b))
#define DIV_ROUND(a,b) (((a)+((b)/2))/(b))
#define DIV_ROUND_DOWN(a,b) ((a)/(b)) // In C this should truncating towards zero (so not down if negative).

#define ABS(a) (((a)<0)?-(a):(a))
#define MY_ABS(d) (((d)>=0)?(d):(-(d)))
//#define MY_ABS(a) ((a)<0?-(a):(a))
#define MIN(a,b) (a<b?a:b)
#define MAX(a,b) (a>b?a:b)

#define MIN_MAX_BAD_RANGE(min, max) (((min)/2)+((max)/2))
#define MIN_MAX_HELPER(v, min, max) (((v)<(min)) ? (min) : (max))
#define MIN_MAX_RANGE(v, min, max) (((v) >= (min)) && ((v) <= (max)) ? (v) : (((min)>(max)) ? MIN_MAX_BAD_RANGE(min,max) : MIN_MAX_HELPER(v, min, max)))

#define SQRI(a) ((a)*(a))
#define SQRI64(a) (((uint64_t)(a))*((uint64_t)(a)))


uint16_t sqrti(uint32_t a);
uint16_t sqrti_using_previous(uint32_t a, uint16_t previous);
uint16_t sqrti_using_previous_with_assert(uint32_t n, uint32_t r);

uint32_t sqrti64_32(uint64_t a);
//#if !(defined __AVR_ATmega328P__)
//uint32_t sqrti64_32_using_previous64(uint64_t a, uint64_t previous);
//uint32_t sqrti64_32_using_previous_with_assert(uint64_t n, uint64_t r);
//#endif

#ifdef AVR_TIMER1_TONE_GENERATOR
int8_t my_sin(uint8_t a);
#endif

#endif



