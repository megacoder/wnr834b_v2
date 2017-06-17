#
# Makefile for the Broadcom wl driver
#
# Copyright 2007, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.1.1.1 2010/03/05 07:31:24 reynolds Exp $
#

O_TARGET       := wl.o

# Generate list of supported variants
VLIST_BASE     ?= AP STA APSTA

VLIST          += $(VLIST_BASE)
ifeq ($(BUILD_WL_ONCHIP_G),true)
  VLIST        += $(foreach base,$(VLIST_BASE),$(base)_ONCHIP_G)
endif # BUILD_WL_ONCHIP_G


# Set suffix and config based on the selected variant
WL_SUFFIX      := $(strip $(foreach v,$(VLIST),$(if $(CONFIG_WL_USE_$(v)),$(v))))
CONFIG_WL_CONF := $(CONFIG_WL_$(WL_SUFFIX))

# Module file names use lowercase of the suffix
wl_suffix := $(shell echo "$(WL_SUFFIX)" | tr A-Z a-z)


include ./wl_generic.mk
