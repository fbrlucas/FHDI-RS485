# STM32 application builder
#
# Author: Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
# License: BSD
# 

LD_SRC = stm32f407_flash.ld

ASM_SRCS = startup_stm32f407.S 

C_SRCS = \
main.c \
stm32f4xx_it.c \
system_stm32f4xx.c \
syscalls_stm32f4xx.c \
stm32f4xx_hal_msp.c \
circ_buffer.c \
crc16.c

CPP_SRCS = 
