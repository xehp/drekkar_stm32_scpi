/*
flash.h

Copyright (C) 2021 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

History

2021-04-11 Created by splitting eeprom.h into two files. Henrik Bjorkman
*/

#ifndef FLASH_H_
#define FLASH_H_

#include <stdint.h>


int8_t flashLoad(char* dataPtr, uint16_t dataSize, uint16_t offset);
int8_t flashSave(const char* dataPtr, uint16_t dataSize, uint16_t offset);


#endif
