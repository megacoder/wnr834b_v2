#***************************************************************************
#***
#***    Copyright 2005  Hon Hai Precision Ind. Co. Ltd.
#***    All Rights Reserved.
#***    No portions of this material shall be reproduced in any form without the
#***    written permission of Hon Hai Precision Ind. Co. Ltd.
#***
#***    All information contained in this document is Hon Hai Precision Ind.  
#***    Co. Ltd. company private, proprietary, and trade secret property and 
#***    are protected by international intellectual property laws and treaties.
#***
#****************************************************************************

# Makefile for swresetd

include $(TOP)/.config

CFLAGS	+= -I. -I$(TOP)/shared -I$(TOP)/busybox -I$(SRCBASE)/include -I$(TOP)/libbcm -Wall
#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2
LDFLAGS	+= -L$(TOP)/nvram -L$(INSTALLDIR)/nvram/usr/lib -lnvram -L$(TOP)/shared -L$(INSTALLDIR)/shared/usr/lib -lshared
LDFLAGS	+= -L$(TOP)/libbcm -L$(INSTALLDIR)/libbcm/usr/lib -lbcm

OBJS := swresetd.o
BIN  := swresetd

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $(BIN) $(OBJS)

install: $(BIN)
	install -d $(TARGETDIR)/usr/sbin
	install -m 755 $(BIN) $(TARGETDIR)/usr/sbin
	$(STRIP) $(TARGETDIR)/usr/sbin/$(BIN)

clean:
	rm -f *~ $(BIN)
	rm -f *.o

$(OBJS): %.o: %.c
	@echo "Making $<"
	$(CC) $(CFLAGS) -c $<


