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

#include <sys/stack.h>
#include <sys/regset.h>
#include <sys/frame.h>
#include <sys/sysmacros.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include "Pcontrol.h"
#include "Pstack.h"
#include "Pisadep.h"

#define	M_PLT_NRSV	4			/* reserved PLT entries */
#define	M_PLT_ENTSIZE	12			/* size of each PLT entry */

#define	SYSCALL32 0x0a00		/* 32-bit syscall (svc) instruction */

const char *
Ppltdest(struct ps_prochandle *P, uintptr_t pltaddr)
{
	map_info_t *mp = Paddr2mptr(P, pltaddr);

	uintptr_t r_addr;
	file_info_t *fp;
	Elf32_Rela r;
	size_t i;

	if (mp == NULL || (fp = mp->map_file) == NULL ||
	    fp->file_plt_base == 0 || pltaddr < fp->file_plt_base ||
	    pltaddr >= fp->file_plt_base + fp->file_plt_size) {
		errno = EINVAL;
		return (NULL);
	}

	i = (pltaddr - fp->file_plt_base -
	    M_PLT_NRSV * M_PLT_ENTSIZE) / M_PLT_ENTSIZE;

	r_addr = fp->file_jmp_rel + i * sizeof (Elf32_Rela);

	if (Pread(P, &r, sizeof (r), r_addr) == sizeof (r) &&
	    (i = ELF32_R_SYM(r.r_info)) < fp->file_dynsym.sym_symn) {

		Elf_Data *data = fp->file_dynsym.sym_data_pri;
		Elf32_Sym *symp = &(((Elf32_Sym *)data->d_buf)[i]);

		return (fp->file_dynsym.sym_strs + symp->st_name);
	}

	return (NULL);
}

int
Pissyscall(struct ps_prochandle *P, uintptr_t addr)
{
	instr_t sysinstr;
	instr_t instr;

	sysinstr = SYSCALL32;

	if (Pread(P, &instr, sizeof (instr), addr) != sizeof (instr) ||
	    instr != sysinstr)
		return (0);
	else
		return (1);
}

int
Pissyscall_prev(struct ps_prochandle *P, uintptr_t addr, uintptr_t *dst)
{
	uintptr_t prevaddr = addr - sizeof (instr_t);

	if (Pissyscall(P, prevaddr)) {
		if (dst)
			*dst = prevaddr;
		return (1);
	}

	return (0);
}

/* ARGSUSED */
int
Pissyscall_text(struct ps_prochandle *P, const void *buf, size_t buflen)
{
	instr_t sysinstr;

	sysinstr = SYSCALL32;

	if (buflen >= sizeof (instr_t) &&
	    memcmp(buf, &sysinstr, sizeof (instr_t)) == 0)
		return (1);
	else
		return (0);
}

static void
ucontext_n_to_prgregs(const ucontext_t *src, prgregset_t dst)
{
	const greg_t *gregs = &src->uc_mcontext.gregs[0];
	const pswg_t *psw = &src->uc_mcontext.psw;

	dst[R_PC] = psw->pc;

	dst[REG_G0]  = gregs[REG_G0];
	dst[REG_G1]  = gregs[REG_G1];
	dst[REG_G2]  = gregs[REG_G2];
	dst[REG_G3]  = gregs[REG_G3];
	dst[REG_G4]  = gregs[REG_G4];
	dst[REG_G5]  = gregs[REG_G5];
	dst[REG_G6]  = gregs[REG_G6];
	dst[REG_G7]  = gregs[REG_G7];
	dst[REG_G8]  = gregs[REG_G8];
	dst[REG_G9]  = gregs[REG_G9];
	dst[REG_G10] = gregs[REG_G10];
	dst[REG_G11] = gregs[REG_G11];
	dst[REG_G12] = gregs[REG_G12];
	dst[REG_G13] = gregs[REG_G13];
	dst[REG_G14] = gregs[REG_G14];
	dst[REG_G15] = gregs[REG_G15];
}

int
Pstack_iter(struct ps_prochandle *P, const prgregset_t regs,
	proc_stack_f *func, void *arg)
{
	prgreg_t *prevfp = NULL;
	uint_t pfpsize = 0;
	int nfp = 0;
	prgregset_t gregs;
	long args[5];
	prgreg_t fp;
	int i;
	int rv;
	uintptr_t sp;
	ssize_t n;
	uclist_t ucl;
	ucontext_t uc;
	struct frame *fr;

	init_uclist(&ucl, P);
	(void) memcpy(gregs, regs, sizeof (gregs));

	for (;;) {
		fp = gregs[R_SP];
		if (stack_loop(fp, &prevfp, &nfp, &pfpsize))
			break;

		fr = (struct frame *) fp;

		for (i = 0; i < 5; i++)
			args[i] = gregs[REG_G2 + i];
		if ((rv = func(arg, gregs, 5, args)) != 0)
			break;

		gregs[R_PC] = gregs[REG_G14];
		if ((sp = gregs[R_SP]) == 0)
			break;

		if (find_uclink(&ucl, sp + SA(sizeof (struct frame)))) {
			if (Pread(P, &uc, sizeof (uc), sp +
			    SA(sizeof (struct frame))) == sizeof (uc)) {
				ucontext_n_to_prgregs(&uc, gregs);
				sp = gregs[R_SP];
			}
		} else {
			if (fr->fr_bc != NULL) {
				Pread(P, gregs, sizeof(struct frame), fr->fr_bc);
				sp = gregs[R_SP];
			}
		}
	}

	if (prevfp)
		free(prevfp);

	free_uclist(&ucl);
	return (rv);
}

uintptr_t
Psyscall_setup(struct ps_prochandle *P, int nargs, int sysindex, uintptr_t sp)
{
	sp -= (nargs > 5) ?
		 sizeof (int32_t) * (1 + nargs) :
		 sizeof (int32_t) * (1 + 5);
	sp = PSTACK_ALIGN32(sp);

	P->status.pr_lwp.pr_reg[REG_G0] = sysindex;
	P->status.pr_lwp.pr_reg[R_SP] = sp;
	P->status.pr_lwp.pr_reg[R_PC] = P->sysaddr;

	return (sp + sizeof (int32_t));
}

int
Psyscall_copyinargs(struct ps_prochandle *P, int nargs, argdes_t *argp,
    uintptr_t ap)
{
	uint32_t arglist[MAXARGS+2];
	int i;
	argdes_t *adp;

	for (i = 0, adp = argp; i < nargs; i++, adp++) {
		arglist[i] = adp->arg_value;

		if (i < 6)
			(void) Pputareg(P, REG_G2+i, adp->arg_value);
	}

	if (nargs > 6 &&
	    Pwrite(P, &arglist[0], sizeof (int32_t) * nargs,
	    (uintptr_t)ap) != sizeof (int32_t) * nargs)
		return (-1);

	return (0);
}

/* ARGSUSED */
int
Psyscall_copyoutargs(struct ps_prochandle *P, int nargs, argdes_t *argp,
    uintptr_t ap)
{
	/* Do nothing */
	return (0);
}
