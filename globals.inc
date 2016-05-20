# STM32 application builder
#
# Author: Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
# License: BSD
# 

# Choose your processor here, see stm32f4/Inc/stm32f4xx_hal.h for a complete list
PROCESSOR = STM32F407xx

GCC_TARGET_PREFIX=~/bin/gcc/bin/arm-none-eabi

CC  = $(GCC_TARGET_PREFIX)-gcc
CPP = $(GCC_TARGET_PREFIX)-g++
AR  = $(GCC_TARGET_PREFIX)-ar
SIZE= $(GCC_TARGET_PREFIX)-size
DUMP= $(GCC_TARGET_PREFIX)-objdump
COPY= $(GCC_TARGET_PREFIX)-objcopy

CC_ARM_OPT = -fmessage-length=0 -mthumb -mcpu=cortex-m4 -mfloat-abi=softfp \
             -mfpu=fpv4-sp-d16 -fdata-sections  -ffunction-sections
LD_ARM_OPT = -mthumb  -mcpu=cortex-m4 --specs=nano.specs  -Wl,--gc-sections 

G_CFLAGS   = -std=c99 -Wall -D$(PROCESSOR) $(CC_ARM_OPT)
G_CPPFLAGS = -std=c99 -Wall -D$(PROCESSOR) $(CC_ARM_OPT)

ifeq ($(RELEASE),ON)
    G_CFLAGS   := -O2 $(G_CFLAGS)
    G_CPPFLAGS := -O2 $(G_CPPFLAGS)
else
    G_CFLAGS   := -g3 -O0 $(G_CFLAGS)
    G_CPPFLAGS := -g3 -O0 $(G_CPPFLAGS)
endif

SHELL = /bin/bash

# all symbols starting by G_ are global 

# global flags for tools
G_LDFLAGS = -Wl,-static $(LD_ARM_OPT)
G_ARFLAGS =
G_LDFILE =

# global libs
G_LIBS = 

# global libs
G_INCS = \
    -Iinclude \
    -I../cmsis/Include \
    -I../stm32f4/Inc \
    -I../cmsis/Device/ST/STM32F4xx/Include \

G_LIBDIR = -L../output
G_BLDDIR = output
G_SRCDIR = source
