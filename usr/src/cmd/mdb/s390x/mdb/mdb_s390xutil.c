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

/*
 * Routines common to the kvm target and the kmdb target that manipulate
 * registers.  This includes register dumps, registers as named variables,
 * and stack traces.
 */

#include <sys/types.h>
#include <sys/stack.h>
#include <sys/regset.h>

#ifndef	__s390xcpu
#define	__s390xcpu
#endif

#include <mdb/mdb_debug.h>
#include <mdb/mdb_modapi.h>
#include <mdb/mdb_s390xutil.h>
#include <mdb/mdb_target_impl.h>
#include <mdb/mdb_err.h>
#include <mdb/mdb.h>

#include <mdb/mdb_kreg_impl.h>

/*
 * We also define an array of register names and their corresponding
 * array indices.  This is used by the getareg and putareg entry points,
 * and also by our register variable discipline.
 */
const mdb_tgt_regdesc_t mdb_sparcs390x_kregs[] = {
	{ "g0",  KREG_G0,  MDB_TGT_R_EXPORT },
	{ "g1",  KREG_G1,  MDB_TGT_R_EXPORT },
	{ "g2",  KREG_G2,  MDB_TGT_R_EXPORT },
	{ "g3",  KREG_G3,  MDB_TGT_R_EXPORT },
	{ "g4",  KREG_G4,  MDB_TGT_R_EXPORT },
	{ "g5",  KREG_G5,  MDB_TGT_R_EXPORT },
	{ "g6",  KREG_G6,  MDB_TGT_R_EXPORT },
	{ "g7",  KREG_G7,  MDB_TGT_R_EXPORT },
	{ "g8",  KREG_G8,  MDB_TGT_R_EXPORT },
	{ "g9",  KREG_G9,  MDB_TGT_R_EXPORT },
	{ "g10", KREG_G10, MDB_TGT_R_EXPORT },
	{ "g11", KREG_G11, MDB_TGT_R_EXPORT },
	{ "g12", KREG_G12, MDB_TGT_R_EXPORT },
	{ "g13", KREG_G13, MDB_TGT_R_EXPORT },
	{ "g14", KREG_G14, MDB_TGT_R_EXPORT },
	{ "g15", KREG_G15, MDB_TGT_R_EXPORT },
	{ "psw", KREG_PSW, MDB_TGT_R_EXPORT },
	{ "c0",  KREG_C0,  MDB_TGT_R_EXPORT },
	{ "c1",  KREG_C1,  MDB_TGT_R_EXPORT },
	{ "c2",  KREG_C2,  MDB_TGT_R_EXPORT },
	{ "c3",  KREG_C3,  MDB_TGT_R_EXPORT },
	{ "c4",  KREG_C4,  MDB_TGT_R_EXPORT },
	{ "c5",  KREG_C5,  MDB_TGT_R_EXPORT },
	{ "c6",  KREG_C6,  MDB_TGT_R_EXPORT },
	{ "c7",  KREG_C7,  MDB_TGT_R_EXPORT },
	{ "c8",  KREG_C8,  MDB_TGT_R_EXPORT },
	{ "c9",  KREG_C9,  MDB_TGT_R_EXPORT },
	{ "c10", KREG_C10, MDB_TGT_R_EXPORT },
	{ "c11", KREG_C11, MDB_TGT_R_EXPORT },
	{ "c12", KREG_C12, MDB_TGT_R_EXPORT },
	{ "c13", KREG_C13, MDB_TGT_R_EXPORT },
	{ "c14", KREG_C14, MDB_TGT_R_EXPORT },
	{ "c15", KREG_C15, MDB_TGT_R_EXPORT },
	{ "a0",  KREG_A0,  MDB_TGT_R_EXPORT },
	{ "a1",  KREG_A1,  MDB_TGT_R_EXPORT },
	{ "a2",  KREG_A2,  MDB_TGT_R_EXPORT },
	{ "a3",  KREG_A3,  MDB_TGT_R_EXPORT },
	{ "a4",  KREG_A4,  MDB_TGT_R_EXPORT },
	{ "a5",  KREG_A5,  MDB_TGT_R_EXPORT },
	{ "a6",  KREG_A6,  MDB_TGT_R_EXPORT },
	{ "a7",  KREG_A7,  MDB_TGT_R_EXPORT },
	{ "a8",  KREG_A8,  MDB_TGT_R_EXPORT },
	{ "a9",  KREG_A9,  MDB_TGT_R_EXPORT },
	{ "a10", KREG_A10, MDB_TGT_R_EXPORT },
	{ "a11", KREG_A11, MDB_TGT_R_EXPORT },
	{ "a12", KREG_A12, MDB_TGT_R_EXPORT },
	{ "a13", KREG_A13, MDB_TGT_R_EXPORT },
	{ "a14", KREG_A14, MDB_TGT_R_EXPORT },
	{ "a15", KREG_A15, MDB_TGT_R_EXPORT },
	{ NULL, 0, 0 }
};

static const char *
pstate_mm_to_str(kreg_t pstate)
{
	if (KREG_PSTATE_MM_TSO(pstate))
		return ("TSO");

	if (KREG_PSTATE_MM_PSO(pstate))
		return ("PSO");

	if (KREG_PSTATE_MM_RMO(pstate))
		return ("RMO");

	return ("???");
}

void
mdb_s390xprintregs(const mdb_tgt_gregset_t *gregs)
{
	const kreg_t *kregs = gregs->kregs;

#define	GETREG2(x) ((uintptr_t)kregs[(x)]), ((uintptr_t)kregs[(x)])

	mdb_printf("%%g0  = 0x%0?p %15A %%a0  = 0x%0?p %A\n",
	    GETREG2(KREG_G0), GETREG2(KREG_A0));

	mdb_printf("%%g1  = 0x%0?p %15A %%a1  = 0x%0?p %A\n",
	    GETREG2(KREG_G1), GETREG2(KREG_A1));

	mdb_printf("%%g2  = 0x%0?p %15A %%a2  = 0x%0?p %A\n",
	    GETREG2(KREG_G2), GETREG2(KREG_A2));

	mdb_printf("%%g3  = 0x%0?p %15A %%a3  = 0x%0?p %A\n",
	    GETREG2(KREG_G3), GETREG2(KREG_A3));

	mdb_printf("%%g4  = 0x%0?p %15A %%a4  = 0x%0?p %A\n",
	    GETREG2(KREG_G4), GETREG2(KREG_A4));

	mdb_printf("%%g5  = 0x%0?p %15A %%a5  = 0x%0?p %A\n",
	    GETREG2(KREG_G5), GETREG2(KREG_A5));

	mdb_printf("%%g6  = 0x%0?p %15A %%a6  = 0x%0?p %A\n",
	    GETREG2(KREG_G6), GETREG2(KREG_A6));

	mdb_printf("%%g7  = 0x%0?p %15A %%a7  = 0x%0?p %A\n\n",
	    GETREG2(KREG_G7), GETREG2(KREG_A7));

	mdb_printf("%%g8  = 0x%0?p %15A %%a8  = 0x%0?p %A\n",
	    GETREG2(KREG_G8), GETREG2(KREG_A8));

	mdb_printf("%%g9  = 0x%0?p %15A %%a9  = 0x%0?p %A\n",
	    GETREG2(KREG_G9), GETREG2(KREG_A9));

	mdb_printf("%%g10 = 0x%0?p %15A %%a10 = 0x%0?p %A\n",
	    GETREG2(KREG_G10), GETREG2(KREG_A10));

	mdb_printf("%%g11 = 0x%0?p %15A %%a11 = 0x%0?p %A\n",
	    GETREG2(KREG_G11), GETREG2(KREG_A11));

	mdb_printf("%%g12 = 0x%0?p %15A %%a12 = 0x%0?p %A\n",
	    GETREG2(KREG_G12), GETREG2(KREG_A12));

	mdb_printf("%%g13 = 0x%0?p %15A %%a13 = 0x%0?p %A\n",
	    GETREG2(KREG_G13), GETREG2(KREG_A13));

	mdb_printf("%%g14 = 0x%0?p %15A %%a14 = 0x%0?p %A\n",
	    GETREG2(KREG_G14), GETREG2(KREG_A14));

	mdb_printf("%%g15 = 0x%0?p %15A %%a15 = 0x%0?p %A\n\n",
	    GETREG2(KREG_G15), GETREG2(KREG_A15));


}

int
mdb_kvm_s390xstack_iter(mdb_tgt_t *t, const mdb_tgt_gregset_t *gsp,
    mdb_tgt_stack_f *func, void *arg)
{
	mdb_tgt_gregset_t gregs;
	kreg_t *kregs = &gregs.kregs[0];
	int got_pc = (gsp->kregs[KREG_PC] != 0);

	struct rwindow rwin;
	uintptr_t sp;
	long argv[6];
	int i;

	/*
	 * - If we got a pc, invoke the call back function starting
	 *   with gsp.
	 * - If we got a saved pc (%i7), invoke the call back function
	 *   starting with the first register window.
	 * - If we got neither a pc nor a saved pc, invoke the call back
	 *   function starting with the second register window.
	 */

	bcopy(gsp, &gregs, sizeof (gregs));

	for (;;) {
		for (i = 0; i < 6; i++)
			argv[i] = kregs[KREG_I0 + i];

		if (got_pc && func(arg, kregs[KREG_PC], 6, argv, &gregs) != 0)
			break;

		kregs[KREG_PC] = kregs[KREG_I7];
		kregs[KREG_NPC] = kregs[KREG_PC] + 4;

		bcopy(&kregs[KREG_I0], &kregs[KREG_O0], 8 * sizeof (kreg_t));
		got_pc |= (kregs[KREG_PC] != 0);

		if ((sp = kregs[KREG_FP] + STACK_BIAS) == STACK_BIAS || sp == 0)
			break; /* Stop if we're at the end of the stack */

		if (sp & (STACK_ALIGN - 1))
			return (set_errno(EMDB_STKALIGN));

		if (mdb_tgt_vread(t, &rwin, sizeof (rwin), sp) != sizeof (rwin))
			return (-1); /* Failed to read frame */

		for (i = 0; i < 8; i++)
			kregs[KREG_L0 + i] = (uintptr_t)rwin.rw_local[i];
		for (i = 0; i < 8; i++)
			kregs[KREG_I0 + i] = (uintptr_t)rwin.rw_in[i];
	}

	return (0);
}

/*ARGSUSED*/
int
mdb_kvm_s390xframe(void *arglim, uintptr_t pc, uint_t argc, const long *argv,
    const mdb_tgt_gregset_t *gregs)
{
	argc = MIN(argc, (uint_t)(uintptr_t)arglim);
	mdb_printf("%a(", pc);

	if (argc != 0) {
		mdb_printf("%lr", *argv++);
		for (argc--; argc != 0; argc--)
			mdb_printf(", %lr", *argv++);
	}

	mdb_printf(")\n");
	return (0);
}

int
mdb_kvm_s390xframev(void *arglim, uintptr_t pc, uint_t argc, const long *argv,
    const mdb_tgt_gregset_t *gregs)
{
	argc = MIN(argc, (uint_t)(uintptr_t)arglim);
	mdb_printf("%0?llr %a(", gregs->kregs[KREG_SP], pc);

	if (argc != 0) {
		mdb_printf("%lr", *argv++);
		for (argc--; argc != 0; argc--)
			mdb_printf(", %lr", *argv++);
	}

	mdb_printf(")\n");
	return (0);
}

int
mdb_kvm_s390xframer(void *arglim, uintptr_t pc, uint_t argc, const long *argv,
    const mdb_tgt_gregset_t *gregs)
{
	char buf[BUFSIZ];
	const kreg_t *kregs = &gregs->kregs[0];

	argc = MIN(argc, (uint_t)(uintptr_t)arglim);

	if (pc == PC_FAKE)
		mdb_printf("%<b>%0?llr% %s%</b>(", kregs[KREG_SP], "?");
	else
		mdb_printf("%<b>%0?llr% %a%</b>(", kregs[KREG_SP], pc);

	if (argc != 0) {
		mdb_printf("%lr", *argv++);
		for (argc--; argc != 0; argc--)
			mdb_printf(", %lr", *argv++);
	}

	mdb_printf(")\n");

	(void) mdb_inc_indent(2);

	mdb_printf("%%l0-%%l3: %?lr %?lr %?lr %?lr\n",
	    kregs[KREG_L0], kregs[KREG_L1], kregs[KREG_L2], kregs[KREG_L3]);

	mdb_printf("%%l4-%%l7: %?lr %?lr %?lr %?lr\n",
	    kregs[KREG_L4], kregs[KREG_L5], kregs[KREG_L6], kregs[KREG_L7]);

	if (kregs[KREG_FP] != 0 && (kregs[KREG_FP] + STACK_BIAS) != 0)
		if (mdb_dis_ins2str(mdb.m_disasm, mdb.m_target, MDB_TGT_AS_VIRT,
		    buf, sizeof (buf), kregs[KREG_I7]) != kregs[KREG_I7])
			mdb_printf("%-#25a%s\n", kregs[KREG_I7], buf);

	(void) mdb_dec_indent(2);
	mdb_printf("\n");

	return (0);
}
