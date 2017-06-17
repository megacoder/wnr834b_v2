#
# Broadcom Linux Router Makefile
# 
# Copyright 2007, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.1.1.1 2010/03/05 07:31:36 reynolds Exp $
#

include .config

ifndef FW_TYPE
FW_TYPE = NA
endif
export FW_TYPE


BOARDID_FILE=compatible.txt
ifeq ($(FW_TYPE),TDC)
BOARDID_FILE=compatible_TDC.txt
endif

#
# Paths
#

# Source bases
export PLATFORM LINUXDIR LIBDIR USRLIBDIR
export TOP := $(shell pwd)
export SRCBASE := $(shell (cd $(TOP)/.. && pwd -P))

#
# Cross-compile environment variables
#

# Build platform
export BUILD := i386-pc-linux-gnu
export HOSTCC := gcc

# uClibc wrapper
ifeq ($(CONFIG_UCLIBC),y)
export PLATFORM := $(PLATFORM)-uclibc
endif

ifeq ($(PLATFORM),mipsel)
export CROSS_COMPILE := mipsel-linux-
export CONFIGURE := ./configure mipsel-linux --build=$(BUILD)
export TOOLCHAIN := $(shell cd $(dir $(shell which $(CROSS_COMPILE)gcc))/../mipsel-linux && pwd -P)
endif

ifeq ($(PLATFORM),mipsel-uclibc)
export CROSS_COMPILE := mipsel-uclibc-
export CONFIGURE := ./configure mipsel-linux --build=$(BUILD)
export TOOLCHAIN := $(shell cd $(dir $(shell which $(CROSS_COMPILE)gcc))/.. && pwd -P)
endif


CFLAGS	+= -DBCMVISTAROUTER

ifeq ($(CONFIG_BCMWPA2),y)
export CFLAGS += -DBCMWPA2
endif

ifeq ($(CONFIG_WSCCMD),y)
export CONFIG_WSCCMD
export CFLAGS += -DBCMWPS

endif

ifeq ($(CONFIG_EMF),y)
export CFLAGS += -D__CONFIG_EMF__
endif

ifeq ($(CONFIG_IGMP_PROXY),y)
export CFLAGS += -D__CONFIG_IGMP_PROXY__
endif

export CC := $(CROSS_COMPILE)gcc
export AR := $(CROSS_COMPILE)ar
export AS := $(CROSS_COMPILE)as
export LD := $(CROSS_COMPILE)ld
export NM := $(CROSS_COMPILE)nm
export RANLIB := $(CROSS_COMPILE)ranlib
export STRIP := $(CROSS_COMPILE)strip
export SIZE := $(CROSS_COMPILE)size
export OBJCOPY := $(CROSS_COMPILE)objcopy

#
# Install and target directories
#

export PLATFORMDIR := $(TOP)/$(PLATFORM)
export INSTALLDIR := $(PLATFORMDIR)/install
export TARGETDIR := $(PLATFORMDIR)/target

define STRIP_DEBUG_SYMBOLS
	@dbgsymf=$(basename $(1))_dbgsym$(suffix $(1)); \
	if [ "$(1)" -nt "$${dbgsymf}" ]; then \
	   echo "#- $0"; \
	   ls -ln $1 | awk '{printf "Orig  size: %10d bytes, %s\n",$$5,$$NF}'; \
	   cp -p -v $1 $$dbgsymf; $(STRIP) -d $(1); touch $$dbgsymf; \
	   ls -ln $1 | awk '{printf "Strip size: %10d bytes, %s\n",$$5,$$NF}'; \
	fi
endef

# always build libbcmcrypto
obj-y += libbcmcrypto

#
# Configuration
#

# obj-$(CONFIG_NVRAM) += nvram
# obj-$(CONFIG_SHARED) += shared
obj-$(CONFIG_LIBBCM) += libbcm

ifeq ($(CONFIG_ACOS_MODULES),y)
obj-y += ../../ap/acos
obj-y += ../../ap/gpl
fw_cfg_file := ../../../project/acos/include/ambitCfg.h
else
obj-$(CONFIG_HTTPD) += httpd
obj-$(CONFIG_WWW) += www
endif

obj-$(CONFIG_RC) += rc
obj-$(CONFIG_GLIBC) += lib
obj-$(CONFIG_UCLIBC) += lib
obj-$(CONFIG_WLCONF) += wlconf
obj-$(CONFIG_BRIDGE) += bridge
obj-$(CONFIG_BUSYBOX) += busybox
obj-$(CONFIG_DNSMASQ) += dnsmasq
obj-$(CONFIG_IPTABLES) += iptables
obj-$(CONFIG_LIBIPT) += iptables
obj-$(CONFIG_NAS) += nas
obj-$(CONFIG_SES) += ses/ses
obj-$(CONFIG_SES_CL) += ses/ses_cl
obj-$(CONFIG_EZC) += ezc
obj-$(CONFIG_NETCONF) += netconf
obj-$(CONFIG_NTP) += ntpclient
obj-$(CONFIG_PPP) += ppp
obj-$(CONFIG_UDHCPD) += udhcpd
obj-$(CONFIG_UPNP) += upnp
obj-$(CONFIG_UTILS) += utils
obj-$(CONFIG_ETC) += etc
obj-$(CONFIG_VLAN) += vlan
obj-$(CONFIG_WSCCMD) += wsc/Wsccmd/src/linux
obj-$(CONFIG_EMF) += emf
obj-$(CONFIG_EMF) += igs
obj-$(CONFIG_IGMP_PROXY) += igmp
obj-$(CONFIG_LLD2D) += lltd/wrt54g-linux
obj-$(CONFIG_ACL_LOGD) += acl_log
obj-$(CONFIG_GPIO) += gpio
obj-$(CONFIG_SWRESETD) += swresetd

obj-clean := $(foreach obj,$(obj-y) $(obj-n),$(obj)-clean)
obj-install := $(foreach obj,$(obj-y),$(obj)-install)

#
# Basic rules
#

all: acos_link prebuilt $(obj-y) $(LINUXDIR)/.config
        # Also build kernel
	if ! grep -q "CONFIG_EMBEDDED_RAMDISK=y" $(LINUXDIR)/.config ; then \
	    $(MAKE) -C $(LINUXDIR) zImage ; \
	fi
	if grep -q "CONFIG_MODULES=y" $(LINUXDIR)/.config ; then \
	    $(MAKE) -C $(LINUXDIR) modules ; \
	fi
	# Preserve the debug versions of these and strip for release
	$(call STRIP_DEBUG_SYMBOLS,$(LINUXDIR)/vmlinux)
	$(call STRIP_DEBUG_SYMBOLS,$(LINUXDIR)/drivers/net/wl/wl.o)
	$(call STRIP_DEBUG_SYMBOLS,$(LINUXDIR)/drivers/net/et/et.o)
#	$(call STRIP_DEBUG_SYMBOLS,$(LINUXDIR)/drivers/net/bcm57xx/bcm57xx.o)
#	$(call STRIP_DEBUG_SYMBOLS,$(LINUXDIR)/drivers/net/emf/emf.o)
#	$(call STRIP_DEBUG_SYMBOLS,$(LINUXDIR)/drivers/net/igs/igs.o)


router-clean: $(obj-clean) config-clean
	rm -rf $(TARGETDIR)
	#rm -f $(PLATFORMDIR)/linux.trx $(PLATFORMDIR)/vmlinuz $(PLATFORMDIR)/target.cramfs
	rm -f $(PLATFORMDIR)/linux.trx $(PLATFORMDIR)/vmlinuz $(PLATFORMDIR)/target.squashfs
	rm -rf $(INSTALLDIR)/busybox

clean: router-clean
	$(MAKE) -C $(LINUXDIR) clean
	$(MAKE) -C $(SRCBASE)/cfe/build/broadcom/bcm947xx clean
	$(MAKE) -C $(SRCBASE)/tools/misc clean

distclean mrproper: clean
	rm -f .config $(LINUXDIR)/.config

prebuilt:
	install -d $(TARGETDIR)/usr/lib
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/libnvram.so $(TARGETDIR)/usr/lib
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/libshared.so $(TARGETDIR)/usr/lib
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/libacos_shared.so $(TARGETDIR)/usr/lib
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/libnat.so $(TARGETDIR)/usr/lib
install package: $(filter-out lib-install,$(obj-install)) $(LINUXDIR)/.config
        # Install binaries into target directory
	install -d $(TARGETDIR)
	for dir in $(wildcard $(patsubst %,$(INSTALLDIR)/%,$(obj-y))) ; do \
	    (cd $${dir} && tar cpf - .) | (cd $(TARGETDIR) && tar xpf -) \
	done
	# optimize the crypto library by removing unneeded symbols
	[ ! -d libbcmcrypto ] || $(MAKE) -C libbcmcrypto optimize
        # Install (and possibly optimize) C library
	$(MAKE) lib-install
        # Install modules into filesystem
	if grep -q "CONFIG_MODULES=y" $(LINUXDIR)/.config ; then \
	    $(MAKE) -C $(LINUXDIR) modules_install DEPMOD=/bin/true INSTALL_MOD_PATH=$(TARGETDIR) ; \
	fi
	find $(TARGETDIR) -name "wl_*.o"  | xargs rm -rf
        # Prepare filesystem
	cd $(TARGETDIR) && $(TOP)/misc/rootprep.sh
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/libc.so.6 $(TARGETDIR)/lib
	install -D $(SRCBASE)/prebuilt/$(FW_TYPE)/acos_nat.o $(TARGETDIR)/lib/modules/2.4.20/kernel/net/ipv4/acos_nat/acos_nat.o
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/cli $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/libnat.so $(TARGETDIR)/usr/lib/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/bd $(TARGETDIR)/sbin/
	cd $(TARGETDIR)/sbin && ln -sf bd burnethermac
	cd $(TARGETDIR)/sbin && ln -sf bd burnboardid
	cd $(TARGETDIR)/sbin && ln -sf bd burnrf
	cd $(TARGETDIR)/sbin && ln -sf bd burnsn
	cd $(TARGETDIR)/sbin && ln -sf bd burnpin
	cd $(TARGETDIR)/sbin && ln -sf bd reset_no_reboot
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/bpa_monitor $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/ddnsd $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/dnsRedirectReplyd $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/email $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/ftpc $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/heartbeat $(TARGETDIR)/usr/sbin/
	cd $(TARGETDIR)/usr/sbin && ln -sf heartbeat wanled
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/lld2d $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/wrt54g.large.ico $(TARGETDIR)/etc
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/wrt54g.small.ico $(TARGETDIR)/etc
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/icon.ico $(TARGETDIR)/etc
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/lld2d.conf $(TARGETDIR)/etc	
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/httpd $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/nvram $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/pot $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/outputimage $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/acos_service $(TARGETDIR)/sbin/
	cd $(TARGETDIR)/sbin && ln -sf acos_service acos_init
	cd $(TARGETDIR)/sbin && ln -sf acos_service version
	cd $(TARGETDIR)/sbin && ln -sf acos_service uptime
	cd $(TARGETDIR)/sbin && ln -sf acos_service read_bd	
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/libacos_shared.so $(TARGETDIR)/usr/lib/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/telnetenabled $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/timesync $(TARGETDIR)/usr/sbin/
	cd $(TARGETDIR)/etc && rm -f resolv.conf && ln -s /tmp/resolv.conf resolv.conf	
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/upnpd $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/wandetect $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/wan_debug $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/wlanconfigd $(TARGETDIR)/usr/sbin/
	cp $(SRCBASE)/prebuilt/$(FW_TYPE)/wpsd $(TARGETDIR)/usr/sbin/
	cp -R $(SRCBASE)/prebuilt/$(FW_TYPE)/www $(TARGETDIR)
	rm -f $(TARGETDIR)/usr/bin/dumpleases
	###########################################
	### Using cramfs filesystem #############
        ## Make sure mkcramfs-2.0 is used
	#$(MAKE) -C $(LINUXDIR)/scripts/cramfs mkcramfs	
	## '-u 0 -g 0' will set the uid and gid of all the files to 0 (root)
	## These options are currently only available on our version of mkcramfs
	#$(LINUXDIR)/scripts/cramfs/mkcramfs -u 0 -g 0 $(TARGETDIR) $(PLATFORMDIR)/target.cramfs
        ## Package kernel and filesystem
	#if grep -q "CONFIG_EMBEDDED_RAMDISK=y" $(LINUXDIR)/.config ; then \
	#    cp $(PLATFORMDIR)/target.cramfs $(LINUXDIR)/arch/mips/ramdisk/$${CONFIG_EMBEDDED_RAMDISK_IMAGE} ; \
	#    $(MAKE) -C $(LINUXDIR) zImage ; \
	#else \
	#    cp $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz $(PLATFORMDIR)/ ; \
	#    trx -o $(PLATFORMDIR)/linux.trx $(PLATFORMDIR)/vmlinuz $(PLATFORMDIR)/target.cramfs ; \
	#    addpattern -i $(PLATFORMDIR)/linux.trx -o $(PLATFORMDIR)/linux_lsys.bin ; \
	#fi
        ## Pad self-booting Linux to a 64 KB boundary
	#cp $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/zImage $(PLATFORMDIR)/
	#dd conv=sync bs=64k < $(PLATFORMDIR)/zImage > $(PLATFORMDIR)/linux.bin
        ## Append filesystem to self-booting Linux
	#cat $(PLATFORMDIR)/target.cramfs >> $(PLATFORMDIR)/linux.bin

	###########################################
	### Create Squashfs filesystem ############
        # Make sure Squashfs-2.2 is used
	#$(MAKE) -C $(LINUXDIR)/scripts/squashfs mksquashfs
	# Excute the mksquashfs, and generate the target.squashfs
	rm -f $(PLATFORMDIR)/target.squashfs
	rm -f $(TARGETDIR)/sbin/st*
	find $(TARGETDIR) -name ".svn" | xargs rm -rf
	$(LINUXDIR)/scripts/squashfs/mksquashfs $(TARGETDIR) $(PLATFORMDIR)/target.squashfs -noappend
        # Package kernel and filesystem
	if grep -q "CONFIG_EMBEDDED_RAMDISK=y" $(LINUXDIR)/.config ; then \
	    cp $(PLATFORMDIR)/target.squashfs $(LINUXDIR)/arch/mips/ramdisk/$${CONFIG_EMBEDDED_RAMDISK_IMAGE} ; \
	    $(MAKE) -C $(LINUXDIR) zImage ; \
	else \
	    cp $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz $(PLATFORMDIR)/ ; \
	    trx -o $(PLATFORMDIR)/linux.trx $(PLATFORMDIR)/vmlinuz $(PLATFORMDIR)/target.squashfs ; \
	fi
        # Pad self-booting Linux to a 64 KB boundary
	cp $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/zImage $(PLATFORMDIR)/
	dd conv=sync bs=64k < $(PLATFORMDIR)/zImage > $(PLATFORMDIR)/linux.bin
        # Append filesystem to self-booting Linux
	cat $(PLATFORMDIR)/target.squashfs >> $(PLATFORMDIR)/linux.bin

	###########################################
	### Create LZMA kernel ####################
	$(OBJCOPY) -O binary -g $(LINUXDIR)/vmlinux $(PLATFORMDIR)/vmlinux.bin
	../../tools/lzma e $(PLATFORMDIR)/vmlinux.bin $(PLATFORMDIR)/vmlinux.lzma
	trx -o $(PLATFORMDIR)/linux.trx $(PLATFORMDIR)/vmlinux.lzma $(PLATFORMDIR)/target.squashfs
	rm -f $(PLATFORMDIR)/vmlinux.bin $(PLATFORMDIR)/vmlinux.lzma

	###########################################
	### Create .chk files for Web UI upgrade ##
	cd $(PLATFORMDIR) && touch rootfs && \
	../../../tools/packet -k linux.trx -f rootfs -b $(BOARDID_FILE) \
	-ok kernel_image -oall kernel_rootfs_image -or rootfs_image \
	-i $(fw_cfg_file) && \
	rm -f rootfs && \
	cp kernel_rootfs_image.chk WNR834Bv2_`date +%m%d%H%M`.chk


#
# Configuration rules
#

conf mconf:
	$(MAKE) -C config
	@./config/$@ ./config/Config
        # Also configure kernel
	$(MAKE) k$@

oldconf: .config
	$(MAKE) -C config
	@./config/conf -o ./config/Config
        # Also configure kernel
	$(MAKE) k$@

kconf:
	$(MAKE) -C $(LINUXDIR) config

kmconf:
	$(MAKE) -C $(LINUXDIR) menuconfig

koldconf: $(LINUXDIR)/.config
	$(MAKE) -C $(LINUXDIR) oldconfig

# Convenience
config: conf

menuconfig: mconf

oldconfig: oldconf

# Default configurations
.config:
	cp config/defconfig $@
	$(MAKE) oldconfig

$(LINUXDIR)/.config:
	cp $(LINUXDIR)/arch/mips/defconfig-bcm947xx $@

#
# Overrides
#

OLDBBCFG=$(shell basename $$(readlink busybox/Config.h) 2> /dev/null)
CURBBCFG=$(CONFIG_BUSYBOX_CONFIG).h

busybox/Config.h: dummy
ifneq ($(OLDBBCFG),$(CURBBCFG))
	cd busybox && rm -f Config.h && ln -sf configs/$(CURBBCFG) Config.h && touch Config.h
endif

busybox: busybox/Config.h
	$(MAKE) -C busybox STRIPTOOL=$(STRIP)

busybox-install:
	$(MAKE) -C busybox STRIPTOOL=$(STRIP) PREFIX=$(INSTALLDIR)/busybox install

# rc: netconf nvram shared
rc: nvram shared
ifneq ($(CONFIG_BUSYBOX),)
rc: busybox/Config.h
endif

www www-%:
	$(MAKE) -C www/$(CONFIG_VENDOR) $* INSTALLDIR=$(INSTALLDIR)/www


bridge:
	$(MAKE) -C bridge brctl/brctl

bridge-install:
	install -D bridge/brctl/brctl $(INSTALLDIR)/bridge/usr/sbin/brctl
	$(STRIP) $(INSTALLDIR)/bridge/usr/sbin/brctl

dnsmasq-install:
	install -D dnsmasq/dnsmasq $(INSTALLDIR)/dnsmasq/usr/sbin/dnsmasq
	$(STRIP) $(INSTALLDIR)/dnsmasq/usr/sbin/dnsmasq

iptables:
	$(MAKE) -C iptables BINDIR=/usr/sbin LIBDIR=/usr/lib KERNEL_DIR=$(LINUXDIR)

iptables-install:
ifeq ($(CONFIG_IPTABLES),y)
	install -d $(INSTALLDIR)/iptables/usr/lib/iptables
	install iptables/extensions/*.so $(INSTALLDIR)/iptables/usr/lib/iptables
	$(STRIP) $(INSTALLDIR)/iptables/usr/lib/iptables/*.so
	install -D iptables/iptables $(INSTALLDIR)/iptables/usr/sbin/iptables
	$(STRIP) $(INSTALLDIR)/iptables/usr/sbin/iptables
else
        # So that generic rule does not take precedence
	@true
endif

iptables-clean:
	-$(MAKE) -C iptables KERNEL_DIR=$(LINUXDIR) clean


netconf: iptables

ntpclient-install:
	install -D ntpclient/ntpclient $(INSTALLDIR)/ntpclient/usr/sbin/ntpclient
	$(STRIP) $(INSTALLDIR)/ntpclient/usr/sbin/ntpclient

ppp ppp-%:
	$(MAKE) -C ppp/pppoecd $* INSTALLDIR=$(INSTALLDIR)/ppp

udhcpd-install:
	install -D udhcpd/udhcpd $(INSTALLDIR)/udhcpd/usr/sbin/udhcpd
	$(STRIP) $(INSTALLDIR)/udhcpd/usr/sbin/udhcpd
	cd $(INSTALLDIR)/udhcpd/usr/sbin && ln -sf udhcpd udhcpc

upnp: netconf nvram shared

wlconf: nvram shared

vlan:
	$(MAKE) -C vlan CROSS=$(CROSS_COMPILE) STRIPTOOL=$(STRIP)

vlan-install:
	$(MAKE) -C vlan CROSS=$(CROSS_COMPILE) STRIPTOOL=$(STRIP) INSTALLDIR=$(INSTALLDIR) install

vlan-clean:
	$(MAKE) -C vlan clean


emf:
	$(MAKE) -C emf/emfconf CROSS=$(CROSS_COMPILE)

emf-install:
ifeq ($(CONFIG_EMF),y)
	install -d $(TARGETDIR)
	$(MAKE) -C emf/emfconf CROSS=$(CROSS_COMPILE) INSTALLDIR=$(INSTALLDIR) install
endif

emf-clean:
	$(MAKE) -C emf/emfconf clean

igs:
	$(MAKE) -C emf/igsconf CROSS=$(CROSS_COMPILE)

igs-install:
ifeq ($(CONFIG_EMF),y)
	install -d $(TARGETDIR)
	$(MAKE) -C emf/igsconf CROSS=$(CROSS_COMPILE) INSTALLDIR=$(INSTALLDIR) install
endif

igs-clean:
	$(MAKE) -C emf/igsconf clean

igmp:
	$(MAKE) -C igmp CROSS=$(CROSS_COMPILE)

igmp-install:
ifeq ($(CONFIG_IGMP_PROXY),y)
	install -d $(TARGETDIR)
	$(MAKE) -C igmp CROSS=$(CROSS_COMPILE) INSTALLDIR=$(INSTALLDIR) install
endif

igmp-clean:
	$(MAKE) -C igmp clean

acos_link:
	cd ../../project/acos/include; rm -f ambitCfg.h; ln -s ambitCfg_$(FW_TYPE).h ambitCfg.h

acos:
	$(MAKE) -C acos CROSS=$(CROSS_COMPILE) STRIPTOOL=$(STRIP)

acos-install:
	$(MAKE) -C acos CROSS=$(CROSS_COMPILE) STRIPTOOL=$(STRIP) INSTALLDIR=$(INSTALLDIR) install

acos-clean:
	$(MAKE) -C acos clean

gpl:
	$(MAKE) -C gpl CROSS=$(CROSS_COMPILE) STRIPTOOL=$(STRIP)

gpl-install:
	$(MAKE) -C gpl CROSS=$(CROSS_COMPILE) STRIPTOOL=$(STRIP) INSTALLDIR=$(INSTALLDIR) install

gpl-clean:
	$(MAKE) -C gpl clean

#
# Generic rules
#

%:
	[ ! -d $* ] || $(MAKE) -C $*

%-clean:
	[ ! -d $* ] || $(MAKE) -C $* clean

%-install:
	[ ! -d $* ] || $(MAKE) -C $* install INSTALLDIR=$(INSTALLDIR)/$*

$(obj-y) $(obj-n) $(obj-clean) $(obj-install): dummy

.PHONY: all clean distclean mrproper install package
.PHONY: conf mconf oldconf kconf kmconf config menuconfig oldconfig
.PHONY: dummy
