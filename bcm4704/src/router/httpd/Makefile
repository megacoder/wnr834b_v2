#
# milli_httpd Makefile
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

CFLAGS	+= -I. -I$(TOP)/shared -I$(SRCBASE)/include -Wall
#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2
LDFLAGS	+= -L$(TOP)/nvram -L$(INSTALLDIR)/nvram/usr/lib -lnvram -L$(TOP)/libbcmcrypto -L$(INSTALLDIR)/libbcmcrypto/usr/lib -lbcmcrypto -L$(TOP)/shared -L$(INSTALLDIR)/shared/usr/lib -lshared

OBJS := cgi.o ej.o httpd.o broadcom.o

ifeq ($(CONFIG_EZC),y)
OBJS += ezc.o
endif

vpath %.c $(TOP)/shared
vpath %.o $(SRCBASE)/router/httpd/prebuilt

all: httpd

clean:
	rm -f *.o *~ httpd

install: all
	install -D httpd $(INSTALLDIR)/usr/sbin/httpd
	$(STRIP) $(INSTALLDIR)/usr/sbin/httpd

httpd: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
