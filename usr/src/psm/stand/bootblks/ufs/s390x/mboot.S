/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 * ident	"@(#)mboot.S	1.20	05/06/08 SMI"
 */

/*
 * SOLARIS MASTER BOOT:
 *
 * PURPOSE: loads the primary boot from the active fdisk partition.
 *          in effect, this routine mimics the functionality of INT 0x19.
 *
 * resides on the first physical sector of the hard drive media.
 * loaded by INT 0x19 (ROM bootstrap loader) at address 0x7C00
 * limited to 512 bytes total, including embedded fdisk table.
 *
 * for compatibility with the ROM BIOS, we contain standard DOS structures:
 *
 *	the fdisk partition table (at offset 0x1BE-0x1FE)
 *	boot signature bytes (0x55, 0xAA at 0x1FE, 0x1FF)
 *
 * the above two entities are required in order to be compatible with
 * the manner in which the DOS BIOS has always performed its boot operation.
 * In the event that our master boot record is inadvertently replaced by
 * a standard DOS boot sector, the booting operation will still succeed!
 *
 * This master boot record uses the relsect/numsect fields of the partition
 * table entry, to compute the start of the active partition; therefore,
 * it is geometry independent.  This means that the drive could be "built"
 * on a system with a disk controller that uses a given disk geometry, but
 * would run on any other controller.
 *
 * SYNOPSIS:
 *     begins execution at 0:0x7C00
 *     relocates to 0:0x600 (to get out of the way!)
 *     reads fdisk table to locate bootable partition
 *     load boot record from the active fdisk partition at 0x7C00
 *     verify boot record signature bytes
 *     jump to/execute the SOLARIS PARTITION PRIMARY BOOT
 *     error handler - can either reboot, or invoke INT 0x18.
 *
 * interface from DOS INT 0x19:  BootDev in DL
 * (this fails sometimes, so we look for a signature to determine whether
 *  to rely on DL from the floppy boot, or if we should assume 0x80 from
 *  the BIOS)
 *
 * interface to partition boot: BootDev in DL
 *
 *=============================================================================
 * Master boot record: resides on first physical sector of device
 */

/*
 * This file is written in GNU as syntax using Intel assembler syntax.  The
 * startup label _start will be executed at address PBOOT_ADDR (0x7C00), but
 * the text section must be set at address RELOC_ADDR (0x600).  With GNU ld
 * this can be done using the "-Ttext 600" option.
 */


#define	PBOOT_ADDR	0x7C00
#define	RELOC_ADDR	0x600

#define	FDISK_START	0x1BE
#define	BOOT_SIG	0x55aa
#define	N_RETRIES	5

#define	FD_NUMPART	4
#define	FD_PTESIZE	0x10
#define	ACTIVE		0x80

/*
 * A convenience macro for declaring a message string (using .ascii directive--
 * NOT nul-terminated) surrounded by two labels, which can then be used with
 * the SIZEOF() macro to get its length.
 */
#define	MSG(label, string)	label: .ascii string; label##_end:

/*
 * Returns the length of some consecutive bytes.  These bytes must be placed
 * between two labels.  The ending label must be the same as the starting label
 * but with a suffix "_end".
 */
#define	SIZEOF(label)	(offset label##_end - offset label)


	.title	"Solaris_Master_Boot"

	.text				/* code segment begins here */

	.global	BootDev
	.global _start

_start:					/* _start is loaded at PBOOT_ADDR */
	j	bootrun

Version:
	.ascii	"M3.0"			/* ident string */

bootrun:
	j	bootrun			/* don't bother me now! */

/*
 * For debugging:  Here's a representative FDISK table entry
 *
 * .org   0x1BE
 * .byte  0x80,1,1,0,0x82,0xfe,0x7f,4,0x3f,0,0,0,0x86,0xfa,0x3f,0
 */
	.org 	0x1FE

	.word	BOOT_SIG
