# $Id: Makefile,v 1.1.1.1 2010/03/05 07:31:38 reynolds Exp $

CFLAGS = -Wall -g -O2
ifeq ($(strip $(DOSTATIC)),true)
    CFLAGS += --static
endif


all:		bidi tap

clean:
		rcsclean *
		rm -f *~
		rm -f bidi
		rm -f tap

bidi:		bidi.c
		$(CC) $(CFLAGS) -o bidi bidi.c

tap:		tap.c
		$(CC) $(CFLAGS) -o tap tap.c
