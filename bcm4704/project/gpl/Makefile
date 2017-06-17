#
# Copyright 2005  Hon Hai Precision Ind. Co. Ltd.
#  All Rights Reserved.
# No portions of this material shall be reproduced in any form without the
# written permission of Hon Hai Precision Ind. Co. Ltd.
#
# All information contained in this document is Hon Hai Precision Ind.
# Co. Ltd. company private, proprietary, and trade secret property and
# are protected by international intellectual property laws and treaties.
#
# $Id: Makefile,v 1.1.1.1 2010/03/05 07:31:02 reynolds Exp $
#

include config.in
include config.mk

#
# Paths
#

SUBDIRS := 

#SUBDIRS += dnsmasq
SUBDIRS += dnsmasq-2.15
SUBDIRS += udhcpd
#SUBDIRS += ppp/pppoecd
SUBDIRS += ppp-2.4.4/pppd pppoecd
SUBDIRS += ntpclient

ifeq ($(CONFIG_RIP),y)
SUBDIRS	+= zebra
endif

ifeq ($(CONFIG_PPTP),y)
#SUBDIRS	+= ppp-2.4.4/pppd pptp
# SUBDIRS	+= ppp/pppd pptp
SUBDIRS	+= pptp
endif

ifeq ($(CONFIG_BPA),y)
SUBDIRS	+= bpalogin
endif

SUBDIRS += iproute2

all:
	for i in ${SUBDIRS}; do \
		(cd $$i; make) || exit 1; \
	done
	

install:
	for i in ${SUBDIRS}; do \
		(cd $$i; make install) || exit 1; \
	done
	
clean:
	for i in ${SUBDIRS}; do \
		(cd $$i; make clean); \
	done

.PHONY: $(SUBDIRS)
