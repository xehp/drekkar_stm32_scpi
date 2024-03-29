/*
2016-08-08 Adapted for atmega328p
2018-07-11 Adapted for STM32
2021-04-06 Used for current leak sensor also. /Henrik
2021-09-06 Changed format
*/


// Enter name and version numbers here:
#define VERSION_NAME "EIT Dielectric test system SCPI"
#define VERSION_MAJOR 5
#define VERSION_MINOR 3
#define VERSION_DEBUG 1

// Don't edit these (only used internally in version.h):
#define VER_XSTR(s) VER_STR(s)
#define VER_STR(s) #s

#define VERSION_STRING VERSION_NAME " " VER_XSTR(VERSION_MAJOR) "." VER_XSTR(VERSION_MINOR) "." VER_XSTR(VERSION_DEBUG)
