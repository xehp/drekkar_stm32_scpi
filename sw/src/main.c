/*
main.c

Copyright (C) 2019 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2016-08-15
Created for dielectric test equipment
Henrik

*/


#include "systemInit.h"
#include "main_loop.h"



int main(void)
{
	systemSleepMs(100);
	main_loop();
	return(0);
}
