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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include	<proc_service.h>
#include	<link.h>
#include	<rtld_db.h>
#include	<_rtld_db.h>
#include	<msg.h>

#define OFFSET_TO_BR 14
#define OFFSET_TO_GOTENT 28

/*
 * A un-initialized S390X PLT look like so
 *
typedef struct __plt1 {
	char	larl[2];	larl	%r1,			    
	long	gotent;		ep@GOTENT              	    
	char	lgot[6];	lg	%r1,0(%r1)		    
	char	br[2];		br	%r1			    
	char	basr[2];	basr	%r1,0			    
	char	loff[6];	lgf	%r1,12(%r1)		    
	char	jg[2];		jg				    
	long	plt0;		offset(plt0)			    
	long	gotoff;		offset(symbol table)		    
} __attribute__ ((packed)) plt1_t;
 */
/* ARGSUSED 2 */
rd_err_e
plt64_resolution(rd_agent_t *rap, psaddr_t pc, lwpid_t lwpid,
	psaddr_t pltbase, rd_plt_info_t *rpi)
{
	long	gotaddr, destaddr;
	psaddr_t	pltoff, pltaddr;


	if (rtld_db_version >= RD_VERSION3) {
		rpi->pi_flags = 0;
		rpi->pi_baddr = 0;
	}

	pltoff = pc - pltbase;
	pltaddr = pltbase +
		((pltoff / M_PLT_ENTSIZE) * M_PLT_ENTSIZE);
	/*
	 * This is the target of the jmp instruction
	 */
	if (ps_pread(rap->rd_psp, pltaddr + OFFSET_TO_GOTENT, (char *)&gotaddr,
	    sizeof (gotaddr)) != PS_OK) {
		LOG(ps_plog(MSG_ORIG(MSG_DB_READFAIL_2), EC_ADDR(pltaddr + 2)));
		return (RD_ERR);
	}

	/*
	 * Find out what's pointed to by @OFFSET_INTO_GOT
	 */
	if (ps_pread(rap->rd_psp, gotaddr, (char *)&destaddr,
	    sizeof (destaddr)) != PS_OK) {
		LOG(ps_plog(MSG_ORIG(MSG_DB_READFAIL_2), EC_ADDR(destaddr)));
		return (RD_ERR);
	}
	if (destaddr == (pltaddr + OFFSET_TO_BR)) {
		rd_err_e	rerr;
		/*
		 * If GOT[ind] points to PLT+offset (branch) then this is the first
		 * time through this PLT.
		 */
		if ((rerr = rd_binder_exit_addr(rap, MSG_ORIG(MSG_SYM_RTBIND),
		    &(rpi->pi_target))) != RD_OK) {
			return (rerr);
		}
		rpi->pi_skip_method = RD_RESOLVE_TARGET_STEP;
		rpi->pi_nstep = 1;
	} 
	else {
		/*
		 * This is the n'th time through and GOT[ind] points
		 * to the final destination.
		 */
		rpi->pi_skip_method = RD_RESOLVE_STEP;
		rpi->pi_nstep = 1;
		rpi->pi_target = 0;
		if (rtld_db_version >= RD_VERSION3) {
			rpi->pi_flags |= RD_FLG_PI_PLTBOUND;
			rpi->pi_baddr = destaddr;
		}
	}

	return (RD_OK);
}
