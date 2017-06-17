/*
 * Frontend command-line utility for Linux NVRAM layer
 *
 * Copyright 2007, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: main.c,v 1.1.1.1 2010/03/05 07:31:39 reynolds Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <typedefs.h>
#include <bcmnvram.h>

void
usage(void)
{
	fprintf(stderr,
	        "usage: nvram [get name] [set name=value] "
	        "[unset name] [show] [commit] ...\n");
	exit(0);
}

/* NVRAM utility */
int
main(int argc, char **argv)
{
	char *name, *value, buf[NVRAM_SPACE];
	int size;

	/* Skip program name */
	--argc;
	++argv;

	if (!*argv)
		usage();

	/* Process the arguments */
	for (; *argv; *++argv) {
		if (!strcmp(*argv, "get")) {
			if (*++argv) {
				if ((value = nvram_get(*argv)))
					puts(value);
			}
		} else if (!strcmp(*argv, "set")) {
			if (*++argv) {
				strncpy(value = buf, *argv, sizeof(buf));
				name = strsep(&value, "=");
				nvram_set(name, value);
			}
		} else if (!strcmp(*argv, "unset")) {
			if (*++argv)
				nvram_unset(*argv);
        } else if (!strcmp (*argv, "commit") || !strcmp (*argv, "save")) { /*Foxconn add, Jasmine Yang, 03/30/2006 */
			nvram_commit();
		} else if (!strcmp(*argv, "show") ||
		           !strcmp(*argv, "dump")) {
			nvram_getall(buf, sizeof(buf));
			for (name = buf; *name; name += strlen(name) + 1)
				puts(name);
			size = sizeof(struct nvram_header) + (int) name - (int) buf;
			if (**argv != 'd')
				fprintf(stderr, "size: %d bytes (%d left)\n",
				        size, NVRAM_SPACE - size);
        }
#ifdef ACOS_MODULES_ENABLE
        else if (!strncmp (*argv, "loaddefault", 11))   /*Foxconn add, Jasmine Yang, 03/30/2006 */
        {
            /* Write only NVRAM header to flash */
            extern int nvram_loaddefault(void);
            nvram_loaddefault ();
        }
        else if (!strncmp (*argv, "version", 7))        /*Foxconn add, Jasmine Yang, 03/30/2006 */
        {
            memset (buf, '\0', sizeof (buf));
            if ((value = nvram_safe_get ("pmon_ver")))
            {
                strcpy (buf, "Boot Loader Version : CFE ");
                strcat (buf, value);
                strcat (buf, "\n");
            }
            if ((value = nvram_safe_get ("os_version")))
            {
                strcat (buf, "OS Version : Linux ");
                strcat (buf, value);
                strcat (buf, "\n");
            }
            puts (buf);
        }
#endif        
        else
			usage();
	}

	return 0;
}
