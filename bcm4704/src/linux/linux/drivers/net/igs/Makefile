#
# Makefile for the Broadcom IGS Layer
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

O_TARGET	:= igs.o
IGS_OBJS	:= igsc.o igsc_sdb.o igs_linux.o osl_linux.o

# Search for sources under src/router/emf/igs
ifneq ($(wildcard $(SRCBASE)/router/emf/igs),)
	EXTRA_CFLAGS	+= -I$(SRCBASE)/router/emf/igs -I$(SRCBASE)/router/emf/emf
	vpath %.c $(SRCBASE)/router/emf/igs
	export-objs	:=
	obj-y		:= $(IGS_OBJS)
	obj-m		:= $(O_TARGET)
else
	obj-y		:= $(SRCBASE)/router/emf/igs.o
	obj-m		:= $(O_TARGET)
endif

include $(TOPDIR)/Rules.make
