#
# Router rc control scripts Makefile
#
# Copyright 2007, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.1.1.1 2010/03/05 07:31:38 reynolds Exp $
#

include $(TOP)/.config

CFLAGS	+= -I. -I$(TOP)/shared -I$(TOP)/busybox -I$(SRCBASE)/include -Wall
#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2

ifeq ($(FW_TYPE),TWC)
CFLAGS += -DBUILD_TWC
endif

LDFLAGS	+= -L$(TOP)/nvram -L$(INSTALLDIR)/nvram/usr/lib -lnvram -L$(TOP)/shared -L$(INSTALLDIR)/shared/usr/lib \
	-lshared -L$(TOP)/libbcmcrypto -L$(INSTALLDIR)/libbcmcrypto/usr/lib -lbcmcrypto

OBJS := rc.o init.o interface.o network.o services.o udhcpc.o http.o stats.o bcmutils.o mtd.o

ifeq ($(CONFIG_NAT),y)
OBJS += firewall.o
LDFLAGS += -L$(TOP)/netconf -L$(INSTALLDIR)/netconf/usr/lib -lnetconf
endif

ifeq ($(CONFIG_NAT),y)
OBJS += ppp.o
endif

ifeq ($(CONFIG_WCN),y)
OBJS += usb_hotplug.o
endif

vpath %.c $(TOP)/shared $(SRCBASE)/shared

all: rc

clean:
	rm -f *.o rc .*.depend

install: all
	install -d $(INSTALLDIR)/sbin
	install rc $(INSTALLDIR)/sbin
	$(STRIP) $(INSTALLDIR)/sbin/rc
	cd $(INSTALLDIR)/sbin && ln -sf rc init
	cd $(INSTALLDIR)/sbin && ln -sf rc erase
	cd $(INSTALLDIR)/sbin && ln -sf rc write
	cd $(INSTALLDIR)/sbin && ln -sf rc stats
	cd $(INSTALLDIR)/sbin && ln -sf rc hotplug

rc: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: .%.depend
.%.depend: %.c
	$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
