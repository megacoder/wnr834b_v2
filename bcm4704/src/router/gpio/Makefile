#
# Makefile GPIO
#

include $(TOP)/.config

CFLAGS	+= -I. -I$(TOP)/shared -I$(TOP)/busybox -I$(SRCBASE)/include -I$(TOP)/libbcm -Wall
#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2
LDFLAGS	+= -L$(TOP)/nvram -L$(INSTALLDIR)/nvram/usr/lib -lnvram -L$(TOP)/shared -L$(INSTALLDIR)/shared/usr/lib -lshared
LDFLAGS	+= -L$(TOP)/libbcm -L$(INSTALLDIR)/libbcm/usr/lib -lbcm

OBJS := gpio.o

all: gpio

clean:
	rm -f *.o gpio .*.depend

install: all
	install -d $(INSTALLDIR)/sbin
	install gpio $(INSTALLDIR)/sbin
	$(STRIP) $(INSTALLDIR)/sbin/gpio

gpio: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

