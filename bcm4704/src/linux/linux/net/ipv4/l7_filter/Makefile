################################################################################
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
# $Id: Makefile,v 1.1.1.1 2010/03/05 07:31:11 reynolds Exp $
#
O_TARGET +=l7_filter.o

L7FILTER := $(SRCBASE)/../project/gpl/l7filter

EXTRA_CFLAGS += -D__LINUX__ -D__KERNEL__ -DMODULE \
	-DEXPORT_SYMTAB \
	-DINCLUDE_L7_FILTER \
	-I$(L7FILTER) 

obj-y := $(L7FILTER)/l7_filter_main.o 

obj-m := $(O_TARGET)

include $(TOPDIR)/Rules.make
