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
 *	Copyright (c) 1988 AT&T
 *	  All Rights Reserved
 *
 *
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 * Global include file for all sgs S390x machine dependent macros, constants
 * and declarations.
 */

#ifndef	_MACHDEP_S390_H
#define	_MACHDEP_S390_H

#include <link.h>
#include <sys/machelf.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Elf header information.
 */
#define	M_MACH			EM_S390
#define	M_MACH_32		EM_S390
#define	M_MACH_64		EM_S390
#ifdef _ELF64
#define	M_CLASS			ELFCLASS64
#else
#define	M_CLASS			ELFCLASS32
#endif

#define	M_MACHPLUS		M_MACH
#define	M_DATA			ELFDATA2MSB
#define	M_FLAGSPLUS		0

/*
 * Page boundary Macros: truncate to previous page boundary and round to
 * next page boundary (refer to generic macros in ../sgs.h also).
 */
#define	M_PTRUNC(X)	((X) & ~(syspagsz - 1))
#define	M_PROUND(X)	(((X) + syspagsz - 1) & ~(syspagsz - 1))

/*
 * Segment boundary macros: truncate to previous segment boundary and round
 * to next page boundary.
 */
#ifndef	M_SEGSIZE
# define	M_SEGSIZE	4096
#endif
#define	M_STRUNC(X)	((X) & ~(M_SEGSIZE - 1))
#define	M_SROUND(X)	(((X) + M_SEGSIZE - 1) & ~(M_SEGSIZE - 1))


/*
 * TLS static segments must be rounded to the following requirements,
 * due to libthread stack allocation.
 */
#define	M_TLSSTATALIGN	0x8

#define	M_BIND_ADJ	4		/* adjustment for end of */
					/*	elf_rtbndr() address */

#ifdef _ELF64
#define	M_WORD_ALIGN	8
#define	M_SEGM_ORIGIN	(Addr)0x08000000  /* default first segment offset */
#else

#define	M_WORD_ALIGN	4

#define	M_SEGM_ORIGIN	(Addr)0x00400000 /* default first segment offset */
#endif
/*
 * Plt and Got information; the first few .got and .plt entries are reserved
 *	PLT[0]	jump to dynamic linker
 *	GOT[0]	address of _DYNAMIC
 */
#define	M_GOT_XDYNAMIC	0		/* got index for _DYNAMIC */
#define	M_GOT_XLINKMAP	1		/* got index for link map */
#define	M_GOT_XRTLD	2		/* got index for rtbinder */
#define	M_GOT_XNumber	3		/* reserved no. of got entries */

/*
 * ELF PLT constants
 */
#define	M_PLT_ENTSIZE	32		/* plt entry size in bytes */
#define	M_PLT_RESERVSZ	M_PLT_ENTSIZE	/* PLT[0] reserved */
#define M_PLT_INSSIZE 	6		/* Single PLT instruction size */
#define	M_PLT_XNumber	3		/* reserved no. of GOT entries */
#define	M_PLT_ALIGN	8		/* alignment of .plt section */

#ifdef _ELF64
# define	M_GOT_ENTSIZE	8		/* got entry size in bytes */
#else
# define	M_GOT_ENTSIZE	4		/* got entry size in bytes */
#endif

/*
 * Other machine dependent entities
 */
#define	M_SEGM_ALIGN	4096

/*
 * Make common relocation information transparent to the common code
 */
#define	M_REL_DT_TYPE	DT_RELA		/* .dynamic entry */
#define	M_REL_DT_SIZE	DT_RELASZ	/* .dynamic entry */
#define	M_REL_DT_ENT	DT_RELAENT	/* .dynamic entry */
#define	M_REL_DT_COUNT	DT_RELACOUNT	/* .dynamic entry */
#define	M_REL_SHT_TYPE	SHT_RELA	/* section header type */
#define	M_REL_ELF_TYPE	ELF_T_RELA	/* data buffer type */

#define	M_REL_CONTYPSTR	conv_reloc_S390_type_str

/*
 * Make common relocation types transparent to the common code
 */
#define	M_R_NONE	R_390_NONE
#define	M_R_GLOB_DAT	R_390_GLOB_DAT
#define	M_R_COPY	R_390_COPY
#define	M_R_RELATIVE	R_390_RELATIVE
#define	M_R_JMP_SLOT	R_390_JMP_SLOT
#define	M_R_REGISTER	R_390_NONE
#define	M_R_FPTR	R_390_NONE
#define	M_R_NUM		R_390_NUM

/*
 * Length of R_390_
 */
#define	M_R_STR_LEN	6

#ifdef _ELF64
# define	M_R_ARRAYADDR	R_390_64
#else
# define	M_R_ARRAYADDR	R_390_32
#endif

#define	M_R_DTPMOD	R_390_TLS_DTPMOD
#define	M_R_DTPOFF	R_390_TLS_DTPOFF
#define	M_R_TPOFF	R_390_TLS_TPOFF

/*
 * DT_REGISTER is not valid on s390
 */
#define	M_DT_REGISTER	0xffffffff

/*
 * PLTRESERVE is not relevant on s390
 */
#define	M_DT_PLTRESERVE	0xfffffffe


/*
 * Make plt section information transparent to the common code.
 */
#define	M_PLT_SHF_FLAGS	(SHF_ALLOC | SHF_WRITE | SHF_EXECINSTR)

/*
 * Make data segment information transparent to the common code.
 */
#define	M_DATASEG_PERM	(PF_R | PF_W | PF_X)

/*
 * Define a set of identifies for special sections.  These allow the sections
 * to be ordered within the output file image.  These values should be
 * maintained consistently, where appropriate, in each platform specific header
 * file.
 *
 *  o	null identifies that this section does not need to be added to the
 *	output image (ie. shared object sections or sections we're going to
 *	recreate (sym tables, string tables, relocations, etc.)).
 *
 *  o	any user defined section will be first in the associated segment.
 *
 *  o	interp and capabilities sections are next, as these are accessed
 *	immediately the first page of the image is mapped.
 *
 *  o	the syminfo, hash, dynsym, dynstr and rel's are grouped together as
 *	these will all be accessed first by ld.so.1 to perform relocations.
 *
 *  o	the got, dynamic, and plt are grouped together as these may also be
 *	accessed first by ld.so.1 to perform relocations, fill in DT_DEBUG
 *	(executables only), and .plt[0].
 *
 *  o	unknown sections (stabs, comments etc.) go at the end.
 *
 * Note that .tlsbss/.bss are given the largest identifiers.  This insures that
 * if any unknown sections become associated to the same segment as the .bss,
 * the .bss sections are always the last section in the segment.
 */
#define	M_ID_NULL	0x00
#define	M_ID_USER	0x01

#define	M_ID_INTERP	0x02			/* SHF_ALLOC */
#define	M_ID_CAP	0x03
#define	M_ID_SYMINFO	0x04
#define	M_ID_HASH	0x05
#define	M_ID_DYNSYM	0x06
#define	M_ID_DYNSTR	0x07
#define	M_ID_VERSION	0x08
#define	M_ID_REL	0x09
#define	M_ID_TEXT	0x0b			/* SHF_ALLOC + SHF_EXECINSTR */
#define	M_ID_DATA	0x0c

/*	M_ID_USER	0x01			dual entry - listed above */
#define	M_ID_GOTDATA	0x02			/* SHF_ALLOC + SHF_WRITE */
#define	M_ID_GOT	0x03
#define	M_ID_PLT	0x04
#define	M_ID_DYNAMIC	0x05
#define	M_ID_ARRAY	0x06

#define	M_ID_UNKNOWN	0xfc			/* just before TLS */

#define	M_ID_TLS	0xfd			/* just before bss */
#define	M_ID_TLSBSS	0xfe
#define	M_ID_BSS	0xff

#define	M_ID_SYMTAB_NDX	0x02			/* ! SHF_ALLOC */
#define	M_ID_SYMTAB	0x03
#define	M_ID_STRTAB	0x04
#define	M_ID_DYNSYM_NDX	0x05
#define	M_ID_NOTE	0x06

#define	M_ID_LDYNSYM	M_ID_DYNSYM		/* TODO Verify correctness */
#define	M_ID_DYNSORT	0x0a			/* TODO Verify correctness */

#ifdef	__cplusplus
}
#endif

#endif /* _MACHDEP_H_S390 */
