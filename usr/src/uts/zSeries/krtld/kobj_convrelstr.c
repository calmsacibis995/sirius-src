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
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 * All rights reserved.
 */
#include	<sys/types.h>
#include	"reloc.h"


#ifdef	KOBJ_DEBUG
static const char	*rels[] = {
	"R_390_NONE",
	"R_390_8",
	"R_390_12",
	"R_390_16",
	"R_390_32",
	"R_390_PC32",
	"R_390_GOT12",
	"R_390_GOT32",
	"R_390_PLT32",
	"R_390_COPY",
	"R_390_GLOB_DAT",
	"R_390_JMP_SLOT",
	"R_390_RELATIVE",
	"R_390_GOTOFF32",
	"R_390_GOTPC",
	"R_390_GOT16",
	"R_390_PC16",
	"R_390_PC16DBL",
	"R_390_PLT16DBL",
	"R_390_PC32DBL",
	"R_390_PLT32DBL",
	"R_390_GOTPCDBL",
	"R_390_64",
	"R_390_PC64",
	"R_390_GOT64",
	"R_390_PLT64",
	"R_390_GOTENT",
	"R_390_GOTOFF16",
	"R_390_GOTOFF64",
	"R_390_GOTPLT12",
	"R_390_GOTPLT16",
	"R_390_GOTPLT32",
	"R_390_GOTPLT64",
	"R_390_GOTPLTENT",
	"R_390_PLTOFF16",
	"R_390_PLTOFF32",
	"R_390_PLTOFF64",
	"R_390_TLS_LOAD",
	"R_390_TLS_GDCALL",
	"R_390_TLS_LDCALL",
	"R_390_TLS_GD32",
	"R_390_TLS_GD64",
	"R_390_TLS_GOTIE12",
	"R_390_TLS_GOTIE32",
	"R_390_TLS_GOTIE64",
	"R_390_TLS_LDM32",
	"R_390_TLS_LDM64",
	"R_390_TLS_IE32",
	"R_390_TLS_IE64",
	"R_390_TLS_IEENT",
	"R_390_TLS_LE32",
	"R_390_TLS_LE64",
	"R_390_TLS_LDO32",
	"R_390_TLS_LDO64",
	"R_390_TLS_DTPMOD",
	"R_390_TLS_DTPOFF",
	"R_390_TLS_TPOFF",
	"R_390_20",
	"R_390_GOT20",
	"R_390_GOTPLT20",
	"R_390_TLS_GOTIE20"
};
#endif

/*
 * This is a 'stub' of the orignal version defined in liblddbg.so
 * This stub just returns the 'int string' of the relocation in question
 * instead of converting it to it's full syntax.
 */
const char *
conv_reloc_S390X_type(Word rtype)
{
#ifdef	KOBJ_DEBUG
	if (rtype < R_390_NUM)
		return (rels[rtype]);
	else {
#endif
		static char 	strbuf[32];
		int		ndx = 31;
		strbuf[ndx--] = '\0';
		do {
			strbuf[ndx--] = '0' + (rtype % 10);
			rtype = rtype / 10;
		} while ((ndx >= (int)0) && (rtype > (Word)0));
		return (&strbuf[ndx + 1]);
#ifdef	KOBJ_DEBUG
	}
#endif
}
