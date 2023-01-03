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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ident	"@(#)ip_ocsum.s	1.4	05/06/08 SMI"

#include <sys/asm_linkage.h>

#if defined(lint)
#include <sys/types.h>
#endif	/* lint */

/*
 * ip_ocsum(address, halfword_count, sum)
 * Do a 16 bit one's complement sum of a given number of (16-bit)
 * halfwords. The halfword pointer must not be odd.
 *	%r2 address
 *	%r3 count 
 *	%r4 sum accumulator
 *
 * (from @(#)ocsum.s 1.3 89/02/24 SMI)
 *
 */

#if defined(lint) 

/* ARGSUSED */
unsigned int
ip_ocsum(u_short *address, int halfword_count, unsigned int sum)
{ return (0); }

#else	/* lint */

	ENTRY(ip_ocsum)
	sllg	%r3,%r3,1	// Convert halfword count to bytes
0:
	cksm	%r4,%r2		// Form checksum
	jnz	0b		// Go until done

	lgr	%r2,%r4		// Get sum
	srdl	%r2,16		// Get top 16 bits
	alr	%r2,%r3		// Form composite
	alr	%r2,%r4		// ....
	srl	%r2,16		// Get tcpip checksum
	br	%r14
	SET_SIZE(ip_ocsum)

#endif 	/* lint */
