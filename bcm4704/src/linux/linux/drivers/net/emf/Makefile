#
# Makefile for the Broadcom EMF Layer
#
# Copyright 2007, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
#

O_TARGET	:= emf.o
EMF_OBJS	:= emfc.o emf_linux.o

# Search for sources under src/router/emf/emf
ifneq ($(wildcard $(SRCBASE)/router/emf/emf),)
	EXTRA_CFLAGS	+= -I$(SRCBASE)/router/emf/emf -I$(SRCBASE)/router/emf/igs
	vpath %.c $(SRCBASE)/router/emf/emf
	export-objs	:=
	obj-y		:= $(EMF_OBJS)
	obj-m		:= $(O_TARGET)
else
	obj-y		:= $(SRCBASE)/router/emf/emf.o
	obj-m		:= $(O_TARGET)
endif

include $(TOPDIR)/Rules.make
