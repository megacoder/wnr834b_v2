#
# Makefile for Broadcom BCM947XX boards
#
# Copyright 2007, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.1.1.1 2010/03/05 07:31:40 reynolds Exp $
#

CFE_MAXSIZE = 262144
ifeq ($(strip $(CFG_BCM57XX)),1)
CFEZ_MAXSIZE = 262144
else
CFEZ_MAXSIZE = 131072
endif


BSPOBJS += bcm947xx_init.o bcm947xx_devs.o ui_bcm947xx.o

ALLOBJS += sbsdram.o hndmips.o sbutils.o hndpmu.o hndchipc.o hndpci.o \
	sromstubs.o bcmutils.o cfe_osl.o nvram.o nvram_rw.o bcmstdlib.o

CFLAGS += -DBCM4710 -DBCM4704 -DBCMDRIVER -DBCM5354

CFLAGS += -I$(SRCBASE)/include

vpath %.c $(SRCBASE)/shared $(SRCBASE)/shared/nvram
vpath %.S $(SRCBASE)/shared

ifeq ($(strip $(CFG_LITTLE)),0)
CFLAGS += -DIL_BIGENDIAN -D_MIPSEB_DATA_INVARIANT_
endif

ifeq ($(strip $(CFG_EMBEDDED_NVRAM)),1)
CFLAGS += -DCFG_EMBEDDED_NVRAM=1
endif

ifeq ($(strip ${CFG_MINIMAL_SIZE}),1)
CFLAGS += -DCFG_MINIMAL_SIZE=1
else
endif

ifeq ($(strip $(CFG_FLASH)),1)
CFLAGS += -DCFG_FLASH=1
CFLAGS += -D_FLASH_BROKEN_BYTEREAD_ -DINCLUDE_FLASH_DRIVERS
ALLOBJS += ui_test_flash.o
endif

ifeq ($(strip ${CFG_SFLASH}),1)
CFLAGS += -DCFG_SFLASH=1
ALLOBJS += dev_sflash.o sflash.o
endif

ifeq ($(strip $(CFG_ET)),1)
ETOBJS := etc_adm.o etc47xx.o etc.o et_cfe.o bcmrobo.o
CFLAGS += -DCFG_ET=1
$(ETOBJS): ETFLAGS := $(CFLAGS) -I$(SRCBASE)/et/sys -DBCM47XX_CHOPS -DETROBO -DETADM -DDMA
CFLAGS += $(ETFLAGS)
HNDDMA := 1
# Search for sources under src/et/sys or objects under src/et/cfe
ifneq ($(wildcard $(SRCBASE)/et/sys),)
vpath %.c $(SRCBASE)/et/sys
ALLOBJS += $(ETOBJS)
else
ALLOBJS += $(foreach obj,$(ETOBJS),$(SRCBASE)/et/cfe/$(obj))
endif
endif

ifeq ($(strip $(CFG_WL)),1)
CFLAGS += -DCFG_WL=1

# get wl driver source files and flags, 
# macros are defined in build/broadcom/bcm947xx/Makefile
# WLCFE is to pick wl_cfe.c from wl.mk
WLCFE=1
include $(WLCFGDIR)/$(WLCONFFILE)
include $(WLCFGDIR)/wl.mk

ifneq ($(WLFILES),)
WLOBJS := $(patsubst %.c,%.o,$(WLFILES))
else
$(error WLFILES is undefined in $(CONFIG_WL_CONF))
endif
WL_DFLAGS       := $(WLFLAGS)

ifeq ($(strip $(CFG_WLU)),1)
WLOBJS += wlu.o wlu_cmd.o wlu_iov.o bcmwifi.o
CFLAGS += -DCFG_WLU=1
endif

# add these path for WLOBJS only
$(WLOBJS): WL_CFLAGS := -I$(SRCBASE)/wl/sys -I$(SRCBASE)/wl/exe $(WL_DFLAGS)
CFLAGS += $(WL_CFLAGS) 

# Search for sources under src/wl/sys or objects under src/wl/cfe
ifneq ($(wildcard $(SRCBASE)/wl/sys/wlc.h),)
vpath %.c $(SRCBASE)/wl/sys $(SRCBASE)/wl/exe $(SRCBASE)/bcmcrypto $(SRCBASE)/shared
ALLOBJS += $(WLOBJS)
else
ALLOBJS += $(foreach obj,$(WLOBJS),$(SRCBASE)/wl/cfe/$(obj))
endif

HNDDMA := 1
endif # CFG_WL


# Search for sources under src/shared or objects under src/shared/cfe
ifeq ($(strip $(HNDDMA)),1)
ifneq ($(wildcard $(SRCBASE)/shared/hnddma.c),)
ALLOBJS += hnddma.o
else
ALLOBJS += $(SRCBASE)/shared/cfe/hnddma.o
endif
endif

ifeq ($(strip $(CFG_PCI)),1)
BSPOBJS += bcm947xx_pci_machdep.o
ifeq ($(strip $(CFG_BCM57XX)),1)
CFLAGS += -DCFG_BCM57XX=1
ALLOBJS += dev_bcm5700.o hndgige.o
# Add robo if not already done by ET
ifeq ($(strip $(CFG_ET)),0)
ALLOBJS += bcmrobo.o
endif
endif
endif

