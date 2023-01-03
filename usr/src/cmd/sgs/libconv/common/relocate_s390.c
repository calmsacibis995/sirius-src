/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/*
 * String conversion routine for relocation types.
 */
#include	<stdio.h>
#include	<sys/elf_s390.h>
#include	"_conv.h"
#include	"relocate_s390_msg.h"

/*
 * S390 specific relocations.
 */
static const Msg rels[R_390_NUM] = {
	MSG_R_390_NONE,		MSG_R_390_8,
	MSG_R_390_12,		MSG_R_390_16,
	MSG_R_390_32,		MSG_R_390_PC32,
	MSG_R_390_GOT12,	MSG_R_390_GOT32,
	MSG_R_390_PLT32,	MSG_R_390_COPY,
	MSG_R_390_GLOB_DAT,	MSG_R_390_JMP_SLOT,
	MSG_R_390_RELATIVE,	MSG_R_390_GOTOFF32,
	MSG_R_390_GOTPC,	MSG_R_390_GOT16,
	MSG_R_390_PC16,		MSG_R_390_PC16DBL,
	MSG_R_390_PLT16DBL,	MSG_R_390_PC32DBL,
	MSG_R_390_PLT32DBL,	MSG_R_390_GOTPCDBL,
	MSG_R_390_64,		MSG_R_390_PC64,
	MSG_R_390_GOT64,	MSG_R_390_PLT64,
	MSG_R_390_GOTENT,	MSG_R_390_GOTOFF16,
	MSG_R_390_GOTOFF64,	MSG_R_390_GOTPLT12,
	MSG_R_390_GOTPLT16,	MSG_R_390_GOTPLT32,
	MSG_R_390_GOTPLT64,	MSG_R_390_GOTPLTENT,
	MSG_R_390_PLTOFF16,	MSG_R_390_PLTOFF32,
	MSG_R_390_PLTOFF64,	MSG_R_390_TLS_LOAD,
	MSG_R_390_TLS_GDCALL,	MSG_R_390_TLS_LDCALL,
	MSG_R_390_TLS_GD32,	MSG_R_390_TLS_GD64,
	MSG_R_390_TLS_GOTIE12,	MSG_R_390_TLS_GOTIE32,
	MSG_R_390_TLS_GOTIE64,	MSG_R_390_TLS_LDM32,
	MSG_R_390_TLS_LDM64,	MSG_R_390_TLS_IE32,
	MSG_R_390_TLS_IE64,	MSG_R_390_TLS_IEENT,
	MSG_R_390_TLS_LE32,	MSG_R_390_TLS_LE64,
	MSG_R_390_TLS_LDO32,	MSG_R_390_TLS_LDO64,
	MSG_R_390_TLS_DTPMOD,	MSG_R_390_TLS_DTPOFF,
	MSG_R_390_TLS_TPOFF,	MSG_R_390_20,
	MSG_R_390_GOT20,	MSG_R_390_GOTPLT20,
	MSG_R_390_TLS_GOTIE20
};

#if	(R_390_NUM != (R_390_TLS_GOTIE20 + 1))
# error	"R_390_NUM has grown"
#endif

const char *
conv_reloc_s390_type(Word type, int fmt_flags, Conv_inv_buf_t *inv_buf)
{
	const char *tmp;

	if (type >= R_390_NUM) {
		switch(type) {
		case R_390_GNU_VTINHERIT :
			tmp = MSG_ORIG(MSG_R_390_GNU_VTINHERIT);
			break;
		case R_390_GNU_VTENTRY :
			tmp = MSG_ORIG(MSG_R_390_GNU_VTENTRY);
			break;
		default:
			return (conv_invalid_val(inv_buf, type, fmt_flags));
		}
	} else {
		tmp = MSG_ORIG(rels[type]);
	}

	return (tmp);
}
