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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SYS_ARCHSYSTM_H
#define	_SYS_ARCHSYSTM_H

/*
 * A selection of ISA-dependent interfaces
 */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_KERNEL) && !defined(_ASM)

#include <sys/types.h>
#include <sys/regset.h>
#include <sys/model.h>
#if defined(__GNUC__) && defined(_ASM_INLINES)
#include <asm/flush.h>
#endif

extern greg_t getfp(void);
extern greg_t getpsr(void);
extern uint_t getpil(void);
extern void setpil(uint_t);
extern greg_t gettbr(void);
extern void realsigprof(int, int);

extern uintptr_t shm_alignment;

struct proc;
struct _klwp;
extern void xregrestore(struct _klwp *, int);
extern int  copy_return_window(int);

extern void setgregs(struct _klwp *, gregset_t);
extern void getgregs(struct _klwp *, gregset_t);
extern void setfpregs(struct _klwp *, fpregset_t *);
extern void getfpregs(struct _klwp *, fpregset_t *);
extern void setgregs32(struct _klwp *, gregset32_t);
extern void getgregs32(struct _klwp *, gregset32_t);
extern void getpsw(struct _klwp *, pswg_t *);

extern void vac_flushall(void);

extern void bind_hwcap(void);

extern u_longlong_t gettick(void);
extern uint64_t	gettick_counter(void);
extern int xcopyin_little(const void *, void *, size_t);
extern int xcopyout_little(const void *, void *, size_t);

struct ucontext;

extern void doflush(void *);

extern uint_t cpu_hwcap_flags;

/* Defined in zSeries/os/archdep.c */
extern int diag_24(int devno, uint32_t *vdevinfo, uint32_t *rdevinfo);
extern int diag_a8(void *sgio, int *retcode);
extern int diag_210(void *rdc);

#endif /* _KERNEL && !_ASM */


#if defined(_KERNEL)

/*
 * For binary compatability with SPARC/Solaris 1.  Needed in the
 * sparc assembly files.
 */
#define	OSYS_mmap	71

#endif /* _KERNEL */

#ifdef __cplusplus
}
#endif

#endif	/* _SYS_ARCHSYSTM_H */
