/*
Barebone blinky on nucleo L432KC / stm32l432kc using SysTick. No RTOS needed.
Copyright (C) Alexander & Henrik Bjorkman www.eit.se 2018
This may be used and redistributed as long as all copyright notices etc are retained .
http://www.eit.se/hb/misc/stm32/stm32l4xx/stm32_barebone/
*/

void comp_init(void);
int comp_getValue(void);
void compMediumTick(void);
void compSlowTick(void);
