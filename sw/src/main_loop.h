/*
main_loop.h

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2017-01-08
created using code from main.c
Henrik Bjorkman
*/

#ifdef __cplusplus
extern "C" {
#endif

//#include <inttypes.h>

#ifndef __AVR_ATmega328P__
#define wdt_reset()
#endif

int main_loop(void);



#ifdef __cplusplus
} // extern "C"
#endif







