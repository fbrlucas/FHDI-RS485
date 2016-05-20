# STM32 application builder
#
# Author: Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
# License: BSD
# 

include ../globals.inc
include sources.inc

APP_NAME = $(notdir $(shell pwd))

G_LIBS:= $(G_LIBS) -lstm32f4
G_LDFILE:= $(LD_SRC)
G_LDFLAGS:= $(G_LDFLAGS) -Wl,-T,"$(G_SRCDIR)/$(G_LDFILE)" -Wl,-Map,"$(G_BLDDIR)/$(APP_NAME).map"

STATICLIB:=
EXEC:=../$(G_BLDDIR)/$(APP_NAME).elf
FLASH:=../$(G_BLDDIR)/$(APP_NAME).bin

include ../makefile.inc

