/*
mathi.c

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2016-08-15
Created for dielectric test equipment
Henrik

*/

#include "mathi.h"
#include "machineState.h"

#if 0
// http://stackoverflow.com/questions/1100090/looking-for-an-efficient-integer-square-root-algorithm-for-arm-thumb2
// Henriks benchmark test 1: Lap time 30 ms (less is better)
uint16_t sqrti(uint32_t x)
{
    uint16_t res=0;
    uint16_t add=0x8000;
    int i;
    for(i=0;i<16;i++)
    {
        uint16_t temp=res | add;
        uint32_t g2=temp*temp;
        if (x>=g2)
        {
            res=temp;
        }
        add>>=1;
    }
    return res;
}


#elif 0
// This algorithm is small and looks good but is terribly slow for large values.
// Henriks benchmark test 1: Lap time 9450 ms
uint16_t sqrti(uint32_t a)
{
	uint32_t square = 1;
	uint32_t delta = 3;
	while(square <= a){
		square += delta;
		delta +=2;
	}
	uint32_t result=(delta/2 -1);
	return result;
}

#else
// http://www.embedded.com/electronics-blogs/programmer-s-toolbox/4219659/Integer-Square-Roots
uint16_t sqrti(uint32_t a)
{
	uint32_t rem=0;
	uint32_t root=0;
	int i=0;
	for(i=0; i<16; i++){
		root <<=1;
		rem = ((rem << 2)+(a >> 30));
		a <<= 2;
		root++;
		if (root <=rem){
			rem -= root;
			root++;
		}
		else
			root--;
	}
	return (uint16_t)(root >> 1);
}
#endif


uint32_t sqrti64_32(uint64_t a)
{
	if (a<0xFFFFFFFF)
	{
		// The value was small enough to fit in 32 bits, so no problem.
		return sqrti(a);
	}

	uint64_t r;

	const uint64_t a1 = a >> 8;
	if (a1<0xFFFFFFFF)
	{
		// The value a1 will now fit in 32 bits, so we can use the 32 bit function.
		const uint32_t a32 = a1; 
		const uint32_t r32 = sqrti(a32);
		r = r32 << 4;
	}
	else
	{
		const uint64_t a2 = a >> 16;
		if (a2<0xFFFFFFFF)
		{
			// The value a2 will now fit in 32 bits.
			const uint32_t a32 = a2; 
			const uint32_t r32 = sqrti(a32);
			r = r32 << 8;
		}
		else
		{
			const uint64_t a3 = a >> 32;
			// The value a3 must now fit in 32 bits.
			const uint32_t a32 = a3;
			const uint32_t r32 = sqrti(a32);
			r = r32 << 16;
		}
	}

	// The calculation above only gives an approximate result, so now just adjust it a little here.

	while (SQRI(r)>a)
	{
    		// result was to big
    		r--;
	}

	while (SQRI(r+1)<=a)
	{
    		// result was not big enough
    		r++;
	}

	return (uint32_t)r;
}


#ifdef AVR_TIMER1_TONE_GENERATOR


static const char sin_table[] PROGMEM={  0,   3,   6,   9,  12,  16,  19,  22, 
                                          25,  28,  31,  34,  37,  40,  43,  46, 
                                          49,  51,  54,  57,  60,  63,  65,  68, 
                                          71,  73,  76,  78,  81,  83,  85,  88, 
                                          90,  92,  94,  96,  98, 100, 102, 104, 
                                         106, 107, 109, 111, 112, 113, 115, 116, 
                                         117, 118, 120, 121, 122, 122, 123, 124, 
                                         125, 125, 126, 126, 126, 127, 127, 127, 
                                         127};

// a is the angle in steps of 2*pi/256, that is 256 is a full turn.
// returns sin(a*(2*pi*/256))*127, that is 127 for 1.
int8_t my_sin(uint8_t a)
{
  if (a<64)
  {
    return pgm_read_byte(sin_table+a);
  }
  else if (a<128)
  {
    return pgm_read_byte(sin_table+(128-a));
  }
  else if (a<192)
  {
    return -pgm_read_byte(sin_table+(a-128));
  }
  else
  {
    return -pgm_read_byte(sin_table+(256-a));
  }
}



#endif

