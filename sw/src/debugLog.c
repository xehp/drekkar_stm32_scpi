

#include "serialDev.h"
#include "cfg.h"
#include "fifo.h"
#include "miscUtilities.h"
#include "debugLog.h"

#ifdef DEBUG_DEV

// Do not write strings longer than 255 bytes at a time.
void debug_print(const char* str)
{
	if (serialGetFreeSpaceWriteBuffer(DEBUG_DEV)>(FIFO_BUFFER_SIZE/2))
	{
		serialPrint(DEBUG_DEV, str);
	}
	else
	{
		serialPutChar(DEBUG_DEV, '#');
	}
}

void debug_print64(int64_t num)
{
	if (serialGetFreeSpaceWriteBuffer(DEBUG_DEV)>(FIFO_BUFFER_SIZE/2))
	{
		char str[64];
		misc_lltoa(num, str, 10);
		debug_print(str);
	}
	else
	{
		serialPutChar(DEBUG_DEV, '#');
	}
}

static const int hex[]={'0','1','2','3','4','5','6','7', '8','9','A','B','C','D','E','F'};

void debug_print_hex_4(uint32_t i)
{
	serialPutChar(DEBUG_DEV, hex[i&0xf]);
}

void debug_print_hex_8(uint32_t i)
{
	debug_print_hex_4(i>>4);
	debug_print_hex_4(i&0xf);
}

void debug_print_hex_16(uint32_t i)
{
	debug_print_hex_8(i>>8);
	debug_print_hex_8(i&0xff);
}

void debug_print_hex_32(uint32_t i)
{
	if (serialGetFreeSpaceWriteBuffer(DEBUG_DEV)>(FIFO_BUFFER_SIZE/2))
	{
		debug_print_hex_16(i>>16);
		debug_print_hex_16(i&0xffff);
	}
	else
	{
		serialPutChar(DEBUG_DEV, '#');
	}
}

void debug_print_hex_64(uint64_t i)
{
	if (serialGetFreeSpaceWriteBuffer(DEBUG_DEV)>(FIFO_BUFFER_SIZE/2))
	{
		debug_print_hex_32(i>>32);
		debug_print_hex_32(i&0xffffffff);
	}
	else
	{
		serialPutChar(DEBUG_DEV, '#');
	}
}

void debug_putchar(int ch)
{
	serialPutChar(DEBUG_DEV, ch);
}

#endif
