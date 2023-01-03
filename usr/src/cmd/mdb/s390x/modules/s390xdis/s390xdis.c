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
/*                                                                  */
/* Copyright 2008 Sine Nomine Associates.                           */
/* All rights reserved.                                             */
/* Use is subject to license terms.                                 */
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/* S390X FIXME */

#include "dis.h"

extern mdb_tgt_addr_t s390xdis_ins2str(mdb_disasm_t *, mdb_tgt_t *,
    mdb_tgt_as_t, char *, size_t, mdb_tgt_addr_t);

/*ARGSUSED*/
static void
s390xdis_destroy(mdb_disasm_t *dp)
{
	/* Nothing to do here */
}

/*ARGSUSED*/
static mdb_tgt_addr_t
s390xdis_previns(mdb_disasm_t *dp, mdb_tgt_t *t, mdb_tgt_as_t as,
    mdb_tgt_addr_t pc, uint_t n)
{
	mdb_tgt_addr_t res = (pc < n * 4 ? 0 : pc - n * 4);
	uint32_t buf;

	/*
	 * Probe the address to make sure we can read from it - we want the
	 * address we return to actually contain something.
	 */
	while (res < pc && mdb_tgt_aread(t, as, &buf, sizeof (buf), res) !=
	    sizeof (buf))
		res += 4;

	return (res);
}

/*ARGSUSED*/
static mdb_tgt_addr_t
s390xdis_nextins(mdb_disasm_t *dp, mdb_tgt_t *t, mdb_tgt_as_t as,
    mdb_tgt_addr_t pc)
{
	mdb_tgt_addr_t npc = pc + 4;
	uint32_t buf;

	/*
	 * Probe the address to make sure we can read from it - we want the
	 * address we return to actually contain something.
	 */
	if (mdb_tgt_aread(t, as, &buf, sizeof (buf), npc) != sizeof (buf))
		return (pc);

	return (npc);
}

static const mdb_dis_ops_t s390xdis_ops = {
	s390xdis_destroy,
	s390xdis_ins2str,
	s390xdis_previns,
	s390xdis_nextins
};

int
s390x_create(mdb_disasm_t *dp)
{
	dp->dis_name = "s390x";
	dp->dis_ops  = &s390xdis_ops;
	dp->dis_data = 0;
	dp->dis_desc = "s390x disassembler";
	return (0);
}


static struct {
	mdb_dis_ctor_f *ctor;
	mdb_disasm_t *hdl;
} s390x_dis[] = {
	{ s390x_create }
};

static const mdb_modinfo_t modinfo = { MDB_API_VERSION, NULL, NULL };

const mdb_modinfo_t *
_mdb_init(void)
{
	int i;

	for (i = 0; i < sizeof (s390x_dis) / sizeof (s390x_dis[0]); i++) {
		s390x_dis[i].hdl = mdb_dis_create(s390x_dis[i].ctor);
	}

	return (&modinfo);
}

void
_mdb_fini(void)
{
	int i;

	for (i = 0; i < sizeof (s390x_dis) / sizeof (s390x_dis[0]); i++) {
		if (s390x_dis[i].hdl != NULL)
			mdb_dis_destroy(s390x_dis[i].hdl);
	}
}
