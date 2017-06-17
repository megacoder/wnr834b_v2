include $(TOP)/.config

CFLAGS	+= -I. -I$(TOP)/shared -I$(SRCBASE)/include -Wall
#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2
LDFLAGS	+= -L$(TOP)/nvram -L$(INSTALLDIR)/nvram/usr/lib -lnvram -L$(TOP)/libbcmcrypto -L$(INSTALLDIR)/libbcmcrypto/usr/lib -lbcmcrypto -L$(TOP)/shared -L$(INSTALLDIR)/shared/usr/lib -lshared

OBJS := acl_logd.o


vpath %.c $(TOP)/shared

all: acl_logd

clean:
	rm -f *.o *~ acl_logd

install: all
	install -D acl_logd $(INSTALLDIR)/usr/sbin/acl_logd
	$(STRIP) $(INSTALLDIR)/usr/sbin/acl_logd

acl_logd: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
