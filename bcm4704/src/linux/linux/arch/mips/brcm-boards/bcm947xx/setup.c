/*
 * HND MIPS boards setup routines
 *
 * Copyright 2007, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: setup.c,v 1.1.1.1 2010/03/05 07:31:14 reynolds Exp $
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/serialP.h>
#include <linux/ide.h>
#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <asm/time.h>
#include <asm/reboot.h>

#ifdef CONFIG_MTD_PARTITIONS
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/minix_fs.h>
#include <linux/ext2_fs.h>
#include <linux/romfs_fs.h>
#include <linux/cramfs_fs.h>
#include <linux/squashfs_fs.h>
#endif

#include <typedefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmnvram.h>
#include <sbutils.h>
#include <hndcpu.h>
#include <sbhndmips.h>
#include <hndmips.h>
#include <sbchipc.h>
#include <hndchipc.h>
#include <trxhdr.h>
#include "bcm947xx.h"

extern void bcm947xx_time_init(void);
extern void bcm947xx_timer_setup(struct irqaction *irq);

#ifdef CONFIG_REMOTE_DEBUG
extern void set_debug_traps(void);
extern void rs_kgdb_hook(struct serial_state *);
extern void breakpoint(void);
#endif

#if defined(CONFIG_BLK_DEV_IDE) || defined(CONFIG_BLK_DEV_IDE_MODULE)
extern struct ide_ops std_ide_ops;
#endif

/* Global SB handle */
sb_t *bcm947xx_sbh = NULL;
spinlock_t bcm947xx_sbh_lock = SPIN_LOCK_UNLOCKED;
EXPORT_SYMBOL(bcm947xx_sbh);
EXPORT_SYMBOL(bcm947xx_sbh_lock);

/* Convenience */
#define sbh bcm947xx_sbh
#define sbh_lock bcm947xx_sbh_lock

/* Kernel command line */
char arcs_cmdline[CL_SIZE] __initdata = CONFIG_CMDLINE;

void
bcm947xx_machine_restart(char *command)
{
	printk("Please stand by while rebooting the system...\n");

	/* Set the watchdog timer to reset immediately */
	__cli();
	hnd_cpu_reset(sbh);
}

void
bcm947xx_machine_halt(void)
{
	printk("System halted\n");

	/* Disable interrupts and watchdog and spin forever */
	__cli();
	sb_watchdog(sbh, 0);
	while (1);
}

#ifdef CONFIG_SERIAL

static struct serial_struct rs = {
	line: 0,
	flags: ASYNC_BOOT_AUTOCONF,
	io_type: SERIAL_IO_MEM,
};

static void __init
serial_add(void *regs, uint irq, uint baud_base, uint reg_shift)
{
	rs.iomem_base = regs;
	rs.irq = irq + 2;
	rs.baud_base = baud_base / 16;
	rs.iomem_reg_shift = reg_shift;

	early_serial_setup(&rs);

	rs.line++;
}

static void __init
serial_setup(sb_t *sbh)
{
	sb_serial_init(sbh, serial_add);

#ifdef CONFIG_REMOTE_DEBUG
	/* Use the last port for kernel debugging */
	if (rs.iomem_base)
		rs_kgdb_hook(&rs);
#endif
}

#endif /* CONFIG_SERIAL */

void __init
brcm_setup(void)
{
	char *value;

	/* Get global SB handle */
	sbh = sb_kattach(SB_OSH);

	/* Initialize clocks and interrupts */
	sb_mips_init(sbh, SBMIPS_VIRTIRQ_BASE);

	if (BCM330X(mips_cpu.processor_id) &&
		(read_c0_diag() & BRCM_PFC_AVAIL)) {
		/* 
		 * Now that the sbh is inited set the  proper PFC value 
		 */	
		printk("Setting the PFC to its default value\n");
		enable_pfc(PFC_AUTO);
	}


#ifdef CONFIG_SERIAL
	/* Initialize UARTs */
	serial_setup(sbh);
#endif

#if defined(CONFIG_BLK_DEV_IDE) || defined(CONFIG_BLK_DEV_IDE_MODULE)
	ide_ops = &std_ide_ops;
#endif

	/* Override default command line arguments */
	value = nvram_get("kernel_args");
	if (value && strlen(value) && strncmp(value, "empty", 5))
		strncpy(arcs_cmdline, value, sizeof(arcs_cmdline));


	/* Generic setup */
	_machine_restart = bcm947xx_machine_restart;
	_machine_halt = bcm947xx_machine_halt;
	_machine_power_off = bcm947xx_machine_halt;

	board_time_init = bcm947xx_time_init;
	board_timer_setup = bcm947xx_timer_setup;
}

const char *
get_system_type(void)
{
	static char s[32];

	if (bcm947xx_sbh) {
		sprintf(s, "Broadcom BCM%X chip rev %d", sb_chip(bcm947xx_sbh),
			sb_chiprev(bcm947xx_sbh));
		return s;
	}
	else
		return "Broadcom BCM947XX";
}

void __init
bus_error_init(void)
{
}

#ifdef CONFIG_MTD_PARTITIONS

static struct mtd_partition bcm947xx_parts[] = {
	{ name: "boot",	offset: 0, size: 0, mask_flags: MTD_WRITEABLE, },
	{ name: "linux", offset: 0, size: 0, },
	{ name: "rootfs", offset: 0, size: 0, mask_flags: MTD_WRITEABLE, },
	{ name: "T_Meter1", offset: 0, size: 0, },
	{ name: "T_Meter2", offset: 0, size: 0, },
	{ name: "POT", offset: 0, size: 0, },
	{ name: "board_data", offset: 0, size: 0, },
	{ name: "nvram", offset: 0, size: 0, },
	{ name: NULL, },
};

struct mtd_partition * __init
init_mtd_partitions(struct mtd_info *mtd, size_t size)
{
	struct minix_super_block *minixsb;
	struct ext2_super_block *ext2sb;
	struct romfs_super_block *romfsb;
	struct cramfs_super *cramfsb;
	squashfs_super_block *squashfsb;
	struct trx_header *trx;
	unsigned char buf[512];
	int off;
	size_t len;

	minixsb = (struct minix_super_block *) buf;
	ext2sb = (struct ext2_super_block *) buf;
	romfsb = (struct romfs_super_block *) buf;
	cramfsb = (struct cramfs_super *) buf;
	squashfsb = (squashfs_super_block *) buf;
	trx = (struct trx_header *) buf;

	/* Look at every 64 KB boundary */
	for (off = 0; off < size; off += (64 * 1024)) {
		memset(buf, 0xe5, sizeof(buf));

		/*
		 * Read block 0 to test for romfs and cramfs superblock
		 */
		if (MTD_READ(mtd, off, sizeof(buf), &len, buf) ||
		    len != sizeof(buf))
			continue;

		/* Try looking at TRX header for rootfs offset */
		if (le32_to_cpu(trx->magic) == TRX_MAGIC) {
			bcm947xx_parts[1].offset = off;
			if (le32_to_cpu(trx->offsets[1]) > off)
				off = le32_to_cpu(trx->offsets[1]);
			continue;
		}

		/* romfs is at block zero too */
		if (romfsb->word0 == ROMSB_WORD0 &&
		    romfsb->word1 == ROMSB_WORD1) {
			printk(KERN_NOTICE
			       "%s: romfs filesystem found at block %d\n",
			       mtd->name, off / BLOCK_SIZE);
			goto done;
		}

		/* so is cramfs */
		if (cramfsb->magic == CRAMFS_MAGIC) {
			printk(KERN_NOTICE
			       "%s: cramfs filesystem found at block %d\n",
			       mtd->name, off / BLOCK_SIZE);
			goto done;
		}

		if(squashfsb->s_magic == SQUASHFS_MAGIC)	{
			printk(KERN_NOTICE
			       "%s: squashfs filesystem found at block %d\n",
			       mtd->name, off / BLOCK_SIZE);
			goto done;
		}
		/*
		 * Read block 1 to test for minix and ext2 superblock
		 */
		if (MTD_READ(mtd, off + BLOCK_SIZE, sizeof(buf), &len, buf) ||
		    len != sizeof(buf))
			continue;

		/* Try minix */
		if (minixsb->s_magic == MINIX_SUPER_MAGIC ||
		    minixsb->s_magic == MINIX_SUPER_MAGIC2) {
			printk(KERN_NOTICE
			       "%s: Minix filesystem found at block %d\n",
			       mtd->name, off / BLOCK_SIZE);
			goto done;
		}

		/* Try ext2 */
		if (ext2sb->s_magic == cpu_to_le16(EXT2_SUPER_MAGIC)) {
			printk(KERN_NOTICE
			       "%s: ext2 filesystem found at block %d\n",
			       mtd->name, off / BLOCK_SIZE);
			goto done;
		}
	}

	printk(KERN_NOTICE
	       "%s: Couldn't find valid ROM disk image\n",
	       mtd->name);

 done:
	/* Find and size nvram */
	bcm947xx_parts[7].offset = size - ROUNDUP(NVRAM_SPACE, mtd->erasesize);
	bcm947xx_parts[7].size = size - bcm947xx_parts[7].offset;

    /* Add Board data and POT partition */
	bcm947xx_parts[6].offset = bcm947xx_parts[7].offset - mtd->erasesize;
	bcm947xx_parts[6].size = mtd->erasesize;

	bcm947xx_parts[5].offset = bcm947xx_parts[6].offset - mtd->erasesize;
	bcm947xx_parts[5].size = mtd->erasesize;

    /* Add traffic meter partition */
	bcm947xx_parts[4].offset = bcm947xx_parts[5].offset - mtd->erasesize;
	bcm947xx_parts[4].size = mtd->erasesize;

	bcm947xx_parts[3].offset = bcm947xx_parts[4].offset - mtd->erasesize;
	bcm947xx_parts[3].size = mtd->erasesize;    
    
	/* Find and size rootfs */
	if (off < size) {
		bcm947xx_parts[2].offset = off;
		bcm947xx_parts[2].size = bcm947xx_parts[3].offset - bcm947xx_parts[2].offset;
	}

	/* Size linux (kernel and rootfs) */
	bcm947xx_parts[1].size = bcm947xx_parts[3].offset - bcm947xx_parts[1].offset;

	/* Size pmon */
	bcm947xx_parts[0].size = bcm947xx_parts[1].offset - bcm947xx_parts[0].offset;

	return bcm947xx_parts;
}

EXPORT_SYMBOL(init_mtd_partitions);

#endif
