/*------------------------------------------------------------------*/
/* 								    */
/* Name        - dis_s390x.c					    */
/* 								    */
/* Function    - Disassembler for s390x.                            */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - August, 2007  					    */
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

#define	MIN(a, b)	((a) < (b) ? (a) : (b))

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <libdisasm.h>
#include <stdlib.h>
#include <stdio.h>

#include "dis_tables.h"
#include "libdisasm_impl.h"

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

struct dis_handle {
	void		*dh_data;
	int		dh_flags;
	dis_lookup_f	dh_lookup;
	dis_read_f	dh_read;
	int		dh_mode;
	dis390x_t	dh_dis;
	uint64_t	dh_addr;
	uint64_t	dh_end;
};

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/


/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- check_func.                                       */
/*                                                                  */
/* Function	- Returns true if we are near the end of a function.*/  
/*		  This is a cheap hack at detecting BR padding 	    */
/*		  between functions.  If we're within a few bytes   */
/*		  of the next function, or past the start, then     */
/*		  return true.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
check_func(void *data)
{
	dis_handle_t *dhp = data;
	uint64_t start;
	size_t len;

	if (dhp->dh_lookup(dhp->dh_data, dhp->dh_addr, NULL, 0, &start, &len)
	    != 0)
		return (0);

	if (start < dhp->dh_addr)
		return (dhp->dh_addr > start + len - 0x10);

	return (1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- get_bytes.                                        */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
get_bytes(void *data, uchar_t *buf, int size)
{
	dis_handle_t *dhp = data;

	if (dhp->dh_read(dhp->dh_data, dhp->dh_addr, buf, size) != size)
		return (-1);

	dhp->dh_addr += size;

	return (size);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- do_lookup.                                        */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
do_lookup(void *data, uint64_t addr, char *buf, size_t buflen)
{
	dis_handle_t *dhp = data;

	return (dhp->dh_lookup(dhp->dh_data, addr, buf, buflen, NULL, NULL));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dis_handle_create.                                */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

dis_handle_t *
dis_handle_create(int flags, void *data, dis_lookup_f lookup_func,
    dis_read_f read_func)
{
	dis_handle_t *dhp;

	/*
	 * Validate architecture flags
	 */
	if (flags & ~(DIS_OCTAL | DIS_NOIMMSYM)) {
		(void) dis_seterrno(E_DIS_INVALFLAG);
		return (NULL);
	}

	/*
	 * Create and initialize the internal structure
	 */
	if ((dhp = dis_zalloc(sizeof (struct dis_handle))) == NULL) {
		(void) dis_seterrno(E_DIS_NOMEM);
		return (NULL);
	}

	dhp->dh_lookup	= lookup_func;
	dhp->dh_read	= read_func;
	dhp->dh_flags	= flags;
	dhp->dh_data	= data;

	if (flags & DIS_OCTAL)
		dhp->dh_dis.d390x_flags = DIS_F_OCTAL;

	dhp->dh_dis.d390x_sprintf_func 	= snprintf;
	dhp->dh_dis.d390x_get_bytes 	= get_bytes;
	dhp->dh_dis.d390x_sym_lookup 	= do_lookup;
	dhp->dh_dis.d390x_check_func 	= check_func;
	dhp->dh_dis.d390x_data 		= dhp;

	return (dhp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dis_disassemble.                                  */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
dis_disassemble(dis_handle_t *dhp, uint64_t addr, char *buf, size_t buflen)
{
	dhp->dh_addr = addr;

	/* DIS_NOIMMSYM might not be set until now, so update */
	if (dhp->dh_flags & DIS_NOIMMSYM)
		dhp->dh_dis.d390x_flags |= DIS_F_NOIMMSYM;
	else
		dhp->dh_dis.d390x_flags &= ~DIS_F_NOIMMSYM;

	if (dtrace_dis390x(&dhp->dh_dis, addr, buf, buflen) != 0)
		return (-1);

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dis_handle_destroy.                               */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
dis_handle_destroy(dis_handle_t *dhp)
{
	dis_free(dhp, sizeof (dis_handle_t));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dis_set_data.                                     */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
dis_set_data(dis_handle_t *dhp, void *data)
{
	dhp->dh_data = data;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dis_flags_set.                                    */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
dis_flags_set(dis_handle_t *dhp, int f)
{
	dhp->dh_flags |= f;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dis_flags_clear.                                  */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
dis_flags_clear(dis_handle_t *dhp, int f)
{
	dhp->dh_flags &= ~f;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dis_max_instrlen.                                 */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
int
dis_max_instrlen(dis_handle_t *dhp)
{
	return (47);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- dis_previnstr.                                    */
/*                                                                  */
/* Function	- Return the previous instruction.  On s390x, we    */
/*		  have no choice except to disassemble everything   */
/*		  from the start of the symbol, and stop when we    */
/*		  have reached our instruction address.  If we're   */
/*		  not in the middle of a known symbol, then we 	    */
/*		  return the same address to indicate failure.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

uint64_t
dis_previnstr(dis_handle_t *dhp, uint64_t pc, int n)
{
	uint64_t *hist, addr, start;
	int cur, nseen;
	uint64_t res = pc;

	if (n <= 0)
		return (pc);

	if (dhp->dh_lookup(dhp->dh_data, pc, NULL, 0, &start, NULL) != 0 ||
	    start == pc)
		return (res);

	hist = dis_zalloc(sizeof (uint64_t) * n);

	for (cur = 0, nseen = 0, addr = start; addr < pc; addr = dhp->dh_addr) {
		hist[cur] = addr;
		cur = (cur + 1) % n;
		nseen++;

		/* if we cannot make forward progress, give up */
		if (dis_disassemble(dhp, addr, NULL, 0) != 0)
			goto done;
	}

	if (addr != pc) {
		/*
		 * We scanned past %pc, but didn't find an instruction that
		 * started at %pc.  This means that either the caller specified
		 * an invalid address, or we ran into something other than code
		 * during our scan.  Virtually any combination of bytes can be
		 * construed as a valid Intel instruction, so any non-code bytes
		 * we encounter will have thrown off the scan.
		 */
		goto done;
	}

	res = hist[(cur + n - MIN(n, nseen)) % n];

done:
	dis_free(hist, sizeof (uint64_t) * n);
	return (res);
}

/*========================= End of Function ========================*/
