/*
 * NVRAM variable manipulation (Linux user mode half)
 *
 * Copyright 2007, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: nvram_linux.c,v 1.1.1.1 2010/03/05 07:31:39 reynolds Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <typedefs.h>
#include <bcmnvram.h>

#define PATH_DEV_NVRAM "/dev/nvram"

/* wklin added, 12/13/2006 */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define LOCK            -1
#define UNLOCK          1
#define NVRAM_SAVE_KEY  0x12345678
static int lock_shm (int semkey, int op)
{
    struct sembuf lockop = { 0, 0, SEM_UNDO } /* sem operation */ ;
    int semid;
        
    if (semkey == 0)
        return -1;

    /* create/init sem */ 
    if ((semid = semget (semkey, 1, IPC_CREAT | IPC_EXCL | 0666)) >= 0) 
    {
        /* initialize the sem vaule to 1 */
        if (semctl (semid, 0, SETVAL, 1) < 0)
        {
            return -1;
        }
    }
    else
    {
        /* sem maybe has createdAget the semid */
        if ((semid = semget (semkey, 1, 0666)) < 0)
        {
            return -1;
        }
    }

    lockop.sem_op = op;
    if (semop (semid, &lockop, 1) < 0)
        return -1;
    
    return 0;
}

#define NVRAM_LOCK()    lock_shm(NVRAM_SAVE_KEY, LOCK)
#define NVRAM_UNLOCK()    lock_shm(NVRAM_SAVE_KEY, UNLOCK)
/* wklin added, 12/13/2006 */



/* Globals */
static int nvram_fd = -1;
static char *nvram_buf = NULL;

int
nvram_init(void *unused)
{
	if ((nvram_fd = open(PATH_DEV_NVRAM, O_RDWR)) < 0)
		goto err;

	/* Map kernel string buffer into user space */
	if ((nvram_buf = mmap(NULL, NVRAM_SPACE, PROT_READ, MAP_SHARED, nvram_fd, 0)) == MAP_FAILED) {
		close(nvram_fd);
		nvram_fd = -1;
		goto err;
	}

	return 0;

 err:
	perror(PATH_DEV_NVRAM);
	return errno;
}

char *
nvram_get(const char *name)
{
	size_t count = strlen(name) + 1;
	char tmp[100], *value;
	unsigned long *off = (unsigned long *) tmp;

    NVRAM_LOCK();

	if (nvram_fd < 0)
		if (nvram_init(NULL)) {
            NVRAM_UNLOCK();
			return NULL;
        }

	if (count > sizeof(tmp)) {
		if (!(off = malloc(count))) {
            NVRAM_UNLOCK();
			return NULL;
        }
	}

	/* Get offset into mmap() space */
	strcpy((char *) off, name);

	count = read(nvram_fd, off, count);

	if (count == sizeof(unsigned long))
		value = &nvram_buf[*off];
	else
		value = NULL;

	if (count < 0)
		perror(PATH_DEV_NVRAM);

	if (off != (unsigned long *) tmp)
		free(off);
            
    NVRAM_UNLOCK();

	return value;
}

int
nvram_getall(char *buf, int count)
{
	int ret;

    NVRAM_LOCK();

	if (nvram_fd < 0)
		if ((ret = nvram_init(NULL))) {
            NVRAM_UNLOCK();
			return ret;
        }

	if (count == 0) {
        NVRAM_UNLOCK();
		return 0;
    }

	/* Get all variables */
	*buf = '\0';

	ret = read(nvram_fd, buf, count);

	if (ret < 0)
		perror(PATH_DEV_NVRAM);

    NVRAM_UNLOCK();

	return (ret == count) ? 0 : ret;
}

static int
_nvram_set(const char *name, const char *value)
{
	size_t count = strlen(name) + 1;
	char tmp[100], *buf = tmp;
	int ret;

	if (nvram_fd < 0)
		if ((ret = nvram_init(NULL)))
			return ret;

	/* Unset if value is NULL */
	if (value)
		count += strlen(value) + 1;

	if (count > sizeof(tmp)) {
		if (!(buf = malloc(count)))
			return -ENOMEM;
	}

	if (value)
		sprintf(buf, "%s=%s", name, value);
	else
		strcpy(buf, name);

	ret = write(nvram_fd, buf, count);

	if (ret < 0)
		perror(PATH_DEV_NVRAM);

	if (buf != tmp)
		free(buf);

	return (ret == count) ? 0 : ret;
}

int
nvram_set(const char *name, const char *value)
{
    int ret;

    NVRAM_LOCK();
	ret = _nvram_set(name, value);
    NVRAM_UNLOCK();

    return ret;
}

int
nvram_unset(const char *name)
{
    int ret;

    NVRAM_LOCK();    
	ret = _nvram_set(name, NULL);
    NVRAM_UNLOCK();

    return ret;
}

int
nvram_commit(void)
{
	int ret;
    
    NVRAM_LOCK();

	if (nvram_fd < 0)
		if ((ret = nvram_init(NULL))) {
            NVRAM_UNLOCK();
			return ret;
        }

	ret = ioctl(nvram_fd, NVRAM_MAGIC, NULL);

	if (ret < 0)
		perror(PATH_DEV_NVRAM);
    
    NVRAM_UNLOCK();

	return ret;
}

/* Foxconn added start Peter Ling 12/05/2005 */
#ifdef ACOS_MODULES_ENABLE
extern struct nvram_tuple router_defaults[];

int nvram_loaddefault (void)
{
    system("rm /tmp/ppp/ip-down"); /* added by EricHuang, 01/12/2007 */
    
    /* NVRAM partition is mtd/7 */
    system("erase mtd7");
    
    return (0);
}
#endif
/* Foxconn added end Peter Ling 12/05/2005 */
