/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License                  
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
 *                                                               
 * Copyright 2008 Sine Nomine Associates.                         
 * All rights reserved.                                        
 * Use is subject to license terms.                             
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SYS_MACHPARAM_H
#define	_SYS_MACHPARAM_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef _ASM
#define	ADDRESS_C(c)    c ## ul
#else   /* _ASM */
#define	ADDRESS_C(c)    (c)
#endif	/* _ASM */

/*
 * Machine dependent parameters and limits - System z version.
 */

/*
 * Maximum cpuid value that we support.  NCPU can be defined in a platform's
 * makefile.
 */
#ifndef NCPU
# define	NCPU	64
#endif

/*
 * MMU_PAGES* describes the physical page size used by the mapping hardware.
 * PAGES* describes the logical page size used by the system.
 */
#define	MMU_PAGE_SIZES		1	/* max s390x mmu-supported page sizes */
#define	DEFAULT_MMU_PAGE_SIZES	1	/* default s390x supported page sizes */

/*
 * XXX make sure the MMU_PAGESHIFT definition here is
 * consistent with the one in param.h
 */
#define	MMU_PAGESHIFT	12
#define	MMU_PAGESIZE	(1<<MMU_PAGESHIFT)
#define	MMU_PAGEOFFSET	(MMU_PAGESIZE - 1)
#define	MMU_PAGEMASK	(~MMU_PAGEOFFSET)

#define	PAGESHIFT	12
#define	PAGESIZE	(1<<PAGESHIFT)
#define	PAGEOFFSET	(PAGESIZE - 1)
#define	PAGEMASK	(~PAGEOFFSET)

#define	MMU_SEGMSHIFT	20
#define	MMU_SEGMSIZE	(1<<MMU_SEGMSHIFT)
#define	MMU_SEGMOFFSET	(MMU_SEGMSIZE - 1)
#define	MMU_SEGMMASK	(~MMU_SEGMOFFSET)

#define	SEGMSHIFT	20
#define	SEGMSIZE	(1<<SEGMSHIFT)
#define	SEGMOFFSET	(SEGMSIZE - 1)
#define	SEGMMASK	(~SEGMOFFSET)

#define CPU_ALLOC_SIZE	MMU_PAGESIZE

/*
 * DATA_ALIGN is used to define the alignment of the Unix data segment.
 */
#define	DATA_ALIGN	ADDRESS_C(0x2000)

/*
 * DEFAULT KERNEL THREAD stack size.
 */

#define	DEFAULTSTKSZ	(8*PAGESIZE)

/*
 * DEFAULT initial thread stack size.
 */
#define	T0STKSZ		(16*PAGESIZE)

/*
 * KERNELBASE is the virtual address which
 * the kernel text/data mapping starts in all contexts.
 */
#define	KERNELBASE	ADDRESS_C(0x01000000)

/*
 * Virtual address range available to the debugger
 */
#define	SEGDEBUGBASE	ADDRESS_C(0xedd00000)
#define	SEGDEBUGSIZE	(ADDRESS_C(0xf0000000) - SEGDEBUGBASE)

/*
 * Define the userlimits
 */

// #define	USERLIMIT	ADDRESS_C(0xFFFFFFFF80000000)
#define	USERLIMIT	ADDRESS_C(0x80000000000)
#define	USERLIMIT32	ADDRESS_C(0x7FFF0000)

/*
 * Define SEGKPBASE, start of the segkp segment.
 */

#define	SEGKPBASE	ADDRESS_C(0x2a100000000)

/*
 * Define SEGKPMBASE, start of the segkpm segment.
 */

#define	SEGKPMBASE	ADDRESS_C(0xFFFFFFFF80000000)

/*
 * Define SEGMAPBASE, start of the segmap segment.
 */

#define	SEGMAPBASE	ADDRESS_C(0x2a750000000)

/*
 * SYSBASE is the virtual address which the kernel allocated memory
 * mapping starts in all contexts.  SYSLIMIT is the end of the Sysbase segment.
 */

#define	SYSBASE		ADDRESS_C(0x30000000000)
#define	SYSBASE32	ADDRESS_C(0x70000000)
#define	SYSLIMIT	ADDRESS_C(0x70000000000)
#define	SYSLIMIT32	ADDRESS_C(0x80000000)

#define	COREHEAP_BASE	ADDRESS_C(0x60000000)
#define	COREHEAP_END	ADDRESS_C(0x70000000)

/*
 * ARGSBASE is the base virtual address of the range which
 * the kernel uses to map the arguments for exec.
 */
#define	ARGSBASE	(0x2a000000000 - NCARGS)

/*
 * PPMAPBASE is the base virtual address of the range which
 * the kernel uses to quickly map pages for operations such
 * as ppcopy, pagecopy, pagezero, and pagesum.
 */
#define	PPMAPSIZE	(512 * 1024)
#define	PPMAPBASE	(ARGSBASE - PPMAPSIZE)

#define	MAXPP_SLOTS	ADDRESS_C(16)
#define	PPMAP_FAST_SIZE	(MAXPP_SLOTS * PAGESIZE * NCPU)
#define	PPMAP_FAST_BASE	(PPMAPBASE - PPMAP_FAST_SIZE)

/*
 * PIOMAPBASE is the base virtual address at which programmable I/O registers
 * are mapped.  This allows such memory -- which may induce side effects when
 * read -- to be cordoned off from the system at-large.
 */
#define	PIOMAPSIZE	(1024 * 1024 * 1024 * (uintptr_t)5)
#define	PIOMAPBASE	(PPMAP_FAST_BASE - PIOMAPSIZE)

/*
 * Allocate space for kernel modules on nucleus pages
 */
#define	MODDATA	1024 * 256

/*
 * On systems with <MODTEXT_SM_SIZE MB available physical memory,
 * cap the in-nucleus module text to MODTEXT_SM_CAP bytes.  The
 * cap must be a multiple of the base page size.  Also see startup.c.
 */
#define	MODTEXT_SM_CAP		(0x200000)		/* bytes */
#define	MODTEXT_SM_SIZE		(256)			/* MB */

/*
 * The heap has a region allocated from it specifically for module text that
 * cannot fit on the nucleus page.  This region -- which starts at address
 * HEAPTEXT_BASE and runs for HEAPTEXT_SIZE bytes -- has virtual holes
 * punched in it: for every HEAPTEXT_MAPPED bytes of available virtual, there
 * is a virtual hole of size HEAPTEXT_UNMAPPED bytes sitting beneath it.  This
 * assures that any text address is within HEAPTEXT_MAPPED of an unmapped
 * region.  The unmapped regions themselves are managed with the routines
 * kobj_texthole_alloc() and kobj_texthole_free().
 */
#define	HEAPTEXT_SIZE		(128 * 1024 * 1024)	/* bytes */
#define	HEAPTEXT_OVERSIZE	(64 * 1024 * 1024)	/* bytes */
#define	HEAPTEXT_BASE		(SYSLIMIT32 - HEAPTEXT_SIZE)
#define	HEAPTEXT_MAPPED		(2 * 1024 * 1024)
#define	HEAPTEXT_UNMAPPED	(2 * 1024 * 1024)

#define	HEAPTEXT_NARENAS	\
	(HEAPTEXT_SIZE / (HEAPTEXT_MAPPED + HEAPTEXT_UNMAPPED) + 2)

/*
 * Preallocate an area for setting up the user stack during
 * the exec(). This way we have a faster allocator and also
 * make sure the stack is always VAC aligned correctly. see
 * get_arg_base() in startup.c.
 */
#define	ARG_SLOT_SIZE	(0x8000)
#define	ARG_SLOT_SHIFT	(15)
#define	N_ARG_SLOT	(0x80)

#define	NARG_BASE	(PIOMAPBASE - (ARG_SLOT_SIZE * N_ARG_SLOT))

/*
 * ktextseg+kvalloc should not use space beyond KERNEL_LIMIT32.
 */

/*
 * For 64-bit kernels, rename KERNEL_LIMIT to KERNEL_LIMIT32 to more accurately
 * reflect the fact that it's actually the limit for 32-bit kernel virtual
 * addresses.
 */
#define	KERNEL_LIMIT32	(SYSBASE32)

#define	BUSTYPE_TO_PFN(btype, pfn)			\
	(((btype) << 19) | ((pfn) & 0x7FFFF))
/*
 * Number of pages to use for trace tables
 */
#define TRACETBL_SIZE	4

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_MACHPARAM_H */
