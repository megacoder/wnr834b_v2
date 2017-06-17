#
# Broadcom web pages
#
# Copyright 2007, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.1.1.1 2010/03/05 07:31:37 reynolds Exp $
#

all: pages.mk

ifneq ($(WIND_HOST_TYPE),x86-win32)
$(SRCBASE)/router/.config:
	cp $(SRCBASE)/router/config/defconfig $@

include $(SRCBASE)/router/.config
endif

PAGES := blur_new.jpg logo_new.gif repeat_edge_new.gif overlib.js style.css
PAGES += index.asp basic.gif
PAGES += lan.asp lan.gif
ifeq ($(CONFIG_NAT),y)
PAGES += wan.asp wan.gif
endif
PAGES += status.asp status.gif
ifeq ($(CONFIG_NAT),y)
PAGES += filter.asp filter.gif
PAGES += forward.asp forward.gif
endif
PAGES += ssid.asp ssid.gif
PAGES += radio.asp radio.gif
PAGES += security.asp security.gif
PAGES += firmware.asp firmware.gif
PAGES += internal.asp internal.gif

ifeq ($(CONFIG_WSCCMD),y)
PAGES += wps_add.asp 
endif

ifneq ($(WIND_HOST_TYPE),x86-win32)
pages.mk: $(SRCBASE)/router/.config
	echo "PAGES := $(PAGES)" > $@
else
pages.mk:
	echo PAGES := $(PAGES) > $@
endif

install:
	install -d $(INSTALLDIR)/www
	install -m 644 $(PAGES) $(INSTALLDIR)/www

clean:
	rm -f pages.mk
