#
# Linux NVRAM Makefile
#
# Copyright 2007, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.1.1.1 2010/03/05 07:31:39 reynolds Exp $
#

include $(TOP)/.config

ifeq ($(CONFIG_ACOS_MODULES),y)
ACOSTOPDIR= $(SRCBASE)/../ap/acos
ACOSNVRAM= $(ACOSTOPDIR)/nvram
endif

CFLAGS	+= -I. -I$(SRCBASE)/include -Wall -DINCLUDE_WAN_LAN_SUBNET
CFLAGS	+= -DINCLUDE_WDS -DINCLUDE_QOS
#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2
LDFLAGS += -L.

ifeq ($(CONFIG_ACOS_MODULES),y)
CFLAGS	+= -DACOS_MODULES_ENABLE
CFLAGS	+= -I$(ACOSTOPDIR)/nvram -I$(ACOSTOPDIR)/include
LDFLAGS	+= -L$(ACOSTOPDIR)/include -L$(ACOSTOPDIR)/nvram
endif

OBJS := nvram_linux.o
ifeq ($(CONFIG_ACOS_MODULES),y)
OBJS += $(ACOSNVRAM)/acosNvramConfig.o

# Foxconn modified pling 05/08/2007, Localisation
ifeq ($(FW_TYPE),NA)
OBJS += $(ACOSNVRAM)/defaults.o
else
ifeq ($(FW_TYPE),WW)
OBJS += $(ACOSNVRAM)/defaults.o
else
OBJS += $(ACOSTOPDIR)/../../project/acos/nvram/defaults_$(FW_TYPE).o
endif
endif

endif

vpath %.c $(SRCBASE)/shared/nvram

all: libnvram.so $(if $(CONFIG_NVRAM_UTILITY),nvram)

install: all
	install -D libnvram.so $(INSTALLDIR)/usr/lib/libnvram.so
	$(STRIP) $(INSTALLDIR)/usr/lib/libnvram.so
ifneq ($(CONFIG_NVRAM_UTILITY),)
	install -D nvram $(INSTALLDIR)/usr/sbin/nvram
	$(STRIP) $(INSTALLDIR)/usr/sbin/nvram
else
	# Utility not wanted: get rid of any leftover one
	rm -f $(INSTALLDIR)/usr/sbin/nvram
endif


clean:
	rm -f *.o *.a *.so nvram
ifeq ($(CONFIG_ACOS_MODULES),y)
	rm -f $(ACOSNVRAM)/*.o
endif	

libnvram.so: $(OBJS)
	$(LD) -shared -o $@ $^

nvram: main.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) -lnvram
