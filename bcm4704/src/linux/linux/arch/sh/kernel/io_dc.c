/*
 *	$Id: io_dc.c,v 1.1.1.1 2010/03/05 07:31:15 reynolds Exp $
 *	I/O routines for SEGA Dreamcast
 */

#include <asm/io.h>
#include <asm/machvec.h>

unsigned long dreamcast_isa_port2addr(unsigned long offset)
{
	return offset + 0xa0000000;
}
