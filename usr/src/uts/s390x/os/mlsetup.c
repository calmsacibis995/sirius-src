/*------------------------------------------------------------------*/
/* 								    */
/* Name        - mlsetup.c  					    */
/* 								    */
/* Function    -                                                    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - July, 2006  					    */
/* 								    */
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
/*                   L I C E N S E                                  */
/*------------------------------------------------------------------*/

/*==================================================================*/
/* 								    */
/* CDDL HEADER START						    */
/* 								    */
/* The contents of this file are subject to the terms of the	    */
/* Common Development and Distribution License                      */
/* (the "License").  You may not use this file except in compliance */
/* with the License.						    */
/* 								    */
/* You can obtain a copy of the license at: 			    */
/* - usr/src/OPENSOLARIS.LICENSE, or,				    */
/* - http://www.opensolaris.org/os/licensing.			    */
/* See the License for the specific language governing permissions  */
/* and limitations under the License.				    */
/* 								    */
/* When distributing Covered Code, include this CDDL HEADER in each */
/* file and include the License file at usr/src/OPENSOLARIS.LICENSE.*/
/* If applicable, add the following below this CDDL HEADER, with    */
/* the fields enclosed by brackets "[]" replaced with your own      */
/* identifying information: 					    */
/* Portions Copyright [yyyy] [name of copyright owner]		    */
/* 								    */
/* CDDL HEADER END						    */
/*                                                                  */
/* Copyright 2008 Sine Nomine Associates.                           */
/* All rights reserved.                                             */
/* Use is subject to license terms.                                 */
/* 								    */
/*==================================================================*/

/*------------------------------------------------------------------*/
/*                 D e f i n e s                                    */
/*------------------------------------------------------------------*/

#ifdef DEBUG
# define TRACE_ENABLE	0x00000000	// Enable TRACG to produce records
#else
# define TRACE_ENABLE	0x80000000	// Disable TRACG
#endif

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/bootconf.h>
#include <sys/bootvfs.h>
#include <sys/machs390x.h>
#include <sys/machsystm.h>
#include <sys/promif.h>
#include <sys/cpu.h>
#include <sys/disp.h>
#include <sys/stack.h>
#include <vm/as.h>
#include <sys/sysmacros.h>
#include <sys/cpupart.h>
#include <sys/copyops.h>
#include <sys/panic.h>
#include <sys/avintr.h>
#include <sys/smp_impldefs.h>

#include <sys/prom_debug.h>
#include <sys/debug.h>

#include <sys/lgrp.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern char 	t0stack[];
extern char	t0stacktop[];
extern struct 	classfuncs sys_classfuncs;
extern disp_t 	cpu0_disp;
extern cpu_t 	cpu0;
extern char	l0fpu[];
extern struct bootops *bop;
extern void	cpu_wait(void);
extern void	cpu_wakeup(struct cpu *, int);
extern char	traceTbl[];

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

int	vac_size;		// cache size in bytes 
uint_t	vac_mask;		// VAC alignment consistency mask 
int	vac_shift;		// log2(vac_size) for ppmapout() 
int	vac = 0;		// virtual address cache type (none == 0) 

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mlsetup.                                          */
/*                                                                  */
/* Function	- Setup routine called right before main(). Inter-  */
/*		  posing this function before main() allows us to   */
/*		  call it in a machine-indepedent fashion.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
mlsetup(void *ksp)
{
	unsigned long long pa;
	_pfxPage *pfx = NULL;
	ctlr12	 cr12;

	bop->bsys_printf  = &prom_printf;
	bop->bsys_vprintf = &prom_vprintf;

	/*
	 * initialize lbolt
	 */
	tod2ticks();

	/*
	 * initialize cpu_self
	 */
	cpu0.cpu_self     = &cpu0;

	/*
	 * initialize t0
	 */
	t0.t_stk 	= t0stacktop;
	t0.t_sp 	= (uint64_t) ksp;
	t0.t_stkbase 	= t0stack;
	t0.t_pri 	= maxclsyspri - 3;
	t0.t_schedflag 	= TS_LOAD | TS_DONT_SWAP;
	t0.t_procp 	= &p0;
	t0.t_plockp 	= &p0lock.pl_lock;
	t0.t_lwp 	= &lwp0;
	t0.t_forw 	= &t0;
	t0.t_back 	= &t0;
	t0.t_next 	= &t0;
	t0.t_prev 	= &t0;
	t0.t_cpu 	= &cpu0;			/* loaded by _start */
	t0.t_disp_queue = &cpu0_disp;
	t0.t_bind_cpu 	= PBIND_NONE;
	t0.t_bind_pset 	= PS_NONE;
	t0.t_cpupart 	= &cp_default;
	t0.t_clfuncs 	= &sys_classfuncs.thread;
	t0.t_copyops 	= NULL;
	THREAD_ONPROC(&t0, CPU);

	lwp0.lwp_thread = &t0;
	lwp0.lwp_procp 	= &p0;
	lwp0.lwp_regs 	= (void *) ksp + STACK_REGS;
	lwp0.lwp_fpu  	= &l0fpu;
	t0.t_tid 	= p0.p_lwpcnt = p0.p_lwprcnt = p0.p_lwpid = 1;

	p0.p_exec 	= NULL;
	p0.p_stat 	= SRUN;
	p0.p_flag 	= SSYS;
	p0.p_tlist 	= &t0;
	p0.p_stksize 	= 2*PAGESIZE;
	p0.p_stkpageszc = 0;
	p0.p_as 	= &kas;
	p0.p_lockp 	= &p0lock;
	p0.p_utraps 	= NULL;
	p0.p_brkpageszc = 0;
	sigorset(&p0.p_ignore, &ignoredefault);

	CPU->cpu_thread 	  = &t0;
	CPU->cpu_dispthread 	  = &t0;
	bzero(&cpu0_disp, sizeof (disp_t));
	CPU->cpu_disp 		  = &cpu0_disp;
	CPU->cpu_disp->disp_cpu	  = CPU;
	CPU->cpu_idle_thread 	  = &t0;
	CPU->cpu_flags 		  = CPU_READY | CPU_RUNNING | CPU_EXISTS | CPU_ENABLE;
	CPU->cpu_id 	 	  = getprocessorid();
	CPU->cpu_dispatch_pri 	  = t0.t_pri;
	CPU->cpu_m.idling 	  = 0;
	CPU->cpu_m.traceTbl	  = &traceTbl;
	CPU->cpu_m.lTraceTbl	  = TRACETBL_SIZE * MMU_PAGESIZE;
	cr12.data.tbl		  = (uint64_t) &traceTbl;
	cr12.data.bits.exTrc	  = 1;
	cr12.data.bits.brTrc	  = 0;
	cr12.data.bits.mdTrc	  = 0;
	cr12.data.bits.asnTrc	  = 0;
	SET_TRC_CR(cr12);
	pfx->__lc_rsm_trace	  = TRACE_ENABLE;
	pfx->__lc_svc_trace	  = 1 | TRACE_ENABLE;
	pfx->__lc_pgm_trace	  = 2 | TRACE_ENABLE;
	pfx->__lc_hat_trace	  = 3 | TRACE_ENABLE;
	pfx->__lc_any_trace	  = 4 | TRACE_ENABLE;
	bzero(&CPU->cpu_ftrace, sizeof(ftrace_data_t));
	
	idle_cpu 	= cpu_wait;
	disp_enq_thread = cpu_wakeup;

	/*
	 * Initialize thread/cpu microstate accounting here
	 */
	init_mstate(&t0, LMS_SYSTEM);
	init_cpu_mstate(CPU, CMS_SYSTEM);

	/*
	 * Initialize lists of available and active CPUs.
	 */
	cpu_list_init(CPU);

	cpu_vm_data_init(CPU);

	spl0();

	prom_init("kernel", NULL);
	/*
	 * lgroup framework initialization. This must be done prior
	 * to devices being mapped.
	 */
	lgrp_init();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mach_modpath.                                     */
/*                                                                  */
/* Function	- Construct the directy path from the filename.     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
mach_modpath(char *path, const char *filename)
{
	int len;
	char *p;
	const char isastr[] = "/s390x";
	size_t isalen = strlen(isastr);

	if ((p = strrchr(filename, '/')) == NULL)
		return;

	while (p > filename && *(p - 1) == '/')
		p--;	/* remove trailing '/' characters */
	if (p == filename)
		p++;	/* so "/" -is- the modpath in this case */

	/*
	 * Remove optional isa-dependent directory name - the module
	 * subsystem will put this back again (!)
	 */
	len = p - filename;
	if (len > isalen &&
	    strncmp(&filename[len - isalen], isastr, isalen) == 0)
		p -= isalen;

	/*
	 * "/platform/mumblefrotz" + " " + MOD_DEFPATH
	 */
	len += (p - filename) + 1 + strlen(MOD_DEFPATH) + 1;
	(void) strncpy(path, filename, p - filename);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- bop_compinfo.                                     */
/*                                                                  */
/* Function	- Fake information about a compressed image.        */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
boot_compinfo(int fd, struct compinfo *cbp)
{
	cbp->iscmp = 0;
	cbp->blksize = MAXBSIZE;
	return (0);
}

/*========================= End of Function ========================*/
