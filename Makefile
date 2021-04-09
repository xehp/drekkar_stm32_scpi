# Makefile
#
#src/stm32l4xx_nucleo_32.o src/stm32l4xx_it.o

# Use this if less output is wanted during compiling.
#ifndef V
#	Q := @
#endif

BINARY ?= binary
ST_FLASH ?= st-flash
GCC_TOOLCHAIN ?= arm-none-eabi-

CC = $(GCC_TOOLCHAIN)gcc
AS = $(GCC_TOOLCHAIN)as

LD = $(GCC_TOOLCHAIN)ld
OBJCOPY = $(GCC_TOOLCHAIN)objcopy

HAL_STM_SOURCES = $(wildcard src/stm32l4/Src/*.c)
HAL_STM_OBJS = $(HAL_STM_SOURCES:.c=.o)

OBJS = src/main.o
OBJS += src/stm32l4/startup_stm32l432xx.o
OBJS += src/systemInit.o
OBJS += src/serialDev.o
OBJS += src/timerDev.o
OBJS += src/adcDev.o
OBJS += src/crc32.o
OBJS += src/current.o
OBJS += src/Dbf.o
OBJS += src/debugLog.o
OBJS += src/messageNames.o
OBJS += src/log.o
OBJS += src/fan.o
OBJS += src/temp.o
OBJS += src/eeprom.o
OBJS += src/mathi.o
OBJS += src/machineState.o
OBJS += src/cmd.o
OBJS += src/main_loop.o
OBJS += src/mainSeconds.o
OBJS += src/translator.o
OBJS += src/scpi.o
OBJS += src/portsGpio.o
OBJS += src/messageUtilities.o
OBJS += src/SoftUart.o
#OBJS += src/stm32l4/system_stm32l4xx.o
#OBJS += src/stm32l4/stm32l4xx_hal_uart_ex.o
#OBJS += src/stm32l4/stm32l4xx_hal_uart.o
#OBJS += src/stm32l4/stm32l4xx_hal_dma.o
#OBJS += src/stm32l4/stm32l4xx_hal_gpio.o
#OBJS += src/stm32l4/stm32l4xx_hal_rcc.o
#OBJS += src/stm32l4/stm32l4xx_hal_pwr_ex.o
#OBJS += src/stm32l4/stm32l4xx_hal_pwr.o
#OBJS += src/stm32l4/stm32l4xx_hal_cortex.o
#OBJS += src/stm32l4/stm32l4xx_hal.o
OBJS += src/stm32l4/stm32l4xx_hal_flash.o
OBJS += src/stm32l4/stm32l4xx_hal_flash_ex.o
#OBJS += src/stm32l4/stm32l4xx_hal_flash_ramfunc.o
OBJS += src/stm32l4/libc_init.o
OBJS += src/stm32l4/libc_maths.o
OBJS += src/stm32l4/libc_maths_impl.o
OBJS += src/miscUtilities.o
OBJS += $(HAL_STM_OBJS)

INCPATH = -Isrc -Isrc/stm32l4 -Isrc/stm32l4/Inc
CFLAGS += -DSTM32L432xx=t -mfpu=fpv4-sp-d16 -mfloat-abi=soft -mcpu=cortex-m4 -DHAL_FLASH_MODULE_ENABLED -Werror -Wall -Wno-pointer-sign
ASFLAGS += -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=soft
LDFLAGS += -Lsrc/stm32l4/Lib -l:libarm_cortexM4l_math.a
LDSCRIPT ?= src/stm32l4/flash.ld

DEPENDENCIES = 
DEPENDENCIES += src/adcDev.h
DEPENDENCIES += src/cfg.h
DEPENDENCIES += src/cmd.h
DEPENDENCIES += src/crc32.h
DEPENDENCIES += src/Dbf.h
DEPENDENCIES += src/debugLog.h
DEPENDENCIES += src/eeprom.h
DEPENDENCIES += src/scpi.h
DEPENDENCIES += src/fan.h
DEPENDENCIES += src/temp.h
DEPENDENCIES += src/fifo.h
DEPENDENCIES += src/log.h
DEPENDENCIES += src/main_loop.h
DEPENDENCIES += src/mainSeconds.h
DEPENDENCIES += src/mathi.h
DEPENDENCIES += src/messageNames.h
DEPENDENCIES += src/miscUtilities.h
DEPENDENCIES += src/portsGpio.h
DEPENDENCIES += src/messageUtilities.h
DEPENDENCIES += src/serialDev.h
DEPENDENCIES += src/systemInit.h
DEPENDENCIES += src/SoftUart.h
DEPENDENCIES += src/timerDev.h
DEPENDENCIES += src/translator.h
DEPENDENCIES += src/current.h
DEPENDENCIES += src/version.h




# This will make the commited git version available to the compiler, if there are uncommited changes .dirty is added also.
#GIT_VERSION := $(shell git describe --abbrev=8 --dirty --always --tags)
GIT_VERSION := $(shell git describe --abbrev=8 --always --tags)
CFLAGS += $(INCPATH) -DGIT_VERSION=\"$(GIT_VERSION)\"


all: info $(BINARY).hex

%.o: %.c $(DEPENDENCIES)
	$(Q)$(CC) $(ARCH_FLAGS) $(COMMON_FLAGS) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(Q)$(AS) $(ARCH_FLAGS) $(COMMON_FLAGS) $(ASFLAGS) -o $@ $<

%.hex: %.elf
	$(Q)$(OBJCOPY) -Oihex $< $@

# We may later need these to be added to line below: src/stm32l4xx_it.o src/stm32l4xx_nucleo_32.o src/stm32l4/Src/*.o
$(BINARY).elf: $(OBJS) $(DEPENDENCIES)
	$(Q)$(LD) $(ARCH_FLAGS) $(LDFLAGS) $(OBJS) -T $(LDSCRIPT) -o $@


info:
	@echo "Objects:  $(OBJS)"
	@echo "Includes: $(INCPATH)"
	@echo ""

clean:
	rm $(OBJS) $(BINARY).hex $(BINARY).elf

flash: all
	$(ST_FLASH) --format ihex write $(BINARY).hex



# https://stackoverflow.com/questions/2394609/makefile-header-dependencies
#depend: .depend
#.depend: $(SRCS)
#	rm -f ./.depend
#	$(Q)$(CC) $(CFLAGS) -MM $^ -MF  ./.depend;
#include .depend
