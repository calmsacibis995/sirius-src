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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _MM_S390X_H
#define	_MM_S390X_H



#ifdef	__cplusplus
extern "C" {
#endif

#ifndef _ASM

#include <sys/types.h>
#include <sys/sysmacros.h>

#define ASPACE_LIMIT20	256       	// Number of pages in 2**20
#define ASPACE_LIMIT31	524288    	// Number of pages in 2**31
#define ASPACE_LIMIT36	16777216  	// Number of pages in 2**36
#define ASPACE_LIMIT42	1073741824	// Number of pages in 2**42
#define ASPACE_LIMIT53	2199023255552	// Number of pages in 2**53

#define ASPACE_CHUNK12	0x1000  	// Number of bytes in 2**12
#define ASPACE_CHUNK20	0x100000  	// Number of bytes in 2**20
#define ASPACE_CHUNK31	0x80000000	// Number of bytes in 2**31
#define ASPACE_CHUNK42	0x40000000000	// Number of bytes in 2**42
#define ASPACE_CHUNK53	0x20000000000000   // Number of bytes in 2**53

#define COUNT_RSP(mem, limit) ((mem) / (limit)) + (((mem) % (limit) != 0))

#define R1_MASK		0xffe0000000000000
#define R2_MASK		0x001ffc0000000000
#define R3_MASK		0x000003ff80000000
#define ST_MASK		0x000000007ff00000
#define PT_MASK		0x00000000000ff000

#define R12_MASK	0xfffffc0000000000
#define R123_MASK	0xffffffff80000000
#define R123S_MASK	0xfffffffffff00000
#define R123SP_MASK	0xfffffffffffff000

#define R1_SINGLE	2048		// Number of entries in a Region 1 table
#define R2_SINGLE	2048		// Number of entries in a Region 2 table
#define R3_SINGLE	2048		// Number of entries in a Region 3 table
#define ST_SINGLE	2048		// Number of entries in a Segment table 
#define PT_SINGLE	256 		// Number of entries in a Page table 
#define BOOT_SCRATCH_SZ	0x1000000

#define CLR_KEY(ra) 					\
	__asm__ ("	lghi	0,0\n"			\
		 "	sske	0,%0\n"			\
		 : : "r" (ra) : "cc", "0")		\

#define SET_KEY(ra, key)				\
	__asm__ ("	sske	%0,%1\n"		\
		 : : "r" (key), "r" (ra) : "cc", "0")	

#define	GET_KEY(ra, key)				\
	__asm__ ("	lghi	%0,0\n"			\
		 "	iske	%0,%1\n"		\
		 : "+r" (key), "+r" (ra))		

/*------------------------------------------------------*/
/* Region entry table (1st, 2nd or 3rd level)		*/
/*------------------------------------------------------*/
typedef struct _rte_{
	u_longlong_t	rsto	:52;	/* Origin	*/
	u_longlong_t	fill_1	:4;
	u_longlong_t	offset	:2;	/* Next Tbl Off	*/
	u_longlong_t	invalid :1;	/* Region invld	*/
	u_longlong_t	fill_2	:1;
	u_longlong_t	type	:2;	/* Entry type	*/
#define RTT1	0x03
#define RTT2	0x02
#define RTT3	0x01
	u_longlong_t	len	:2;	/* Next tbl len	*/
} __attribute__ ((packed)) rte; 

/*------------------------------------------------------*/
/* Segment table entry					*/
/*------------------------------------------------------*/
typedef struct _ste_ {
	u_longlong_t	pto	:53;	/* Page tbl org	*/
	u_longlong_t 	fill_1	:1;
	u_longlong_t	protect	:1;	/* Page protect	*/
	u_longlong_t	fill_2	:3;
	u_longlong_t	invalid	:1;	/* Segt invalid	*/
	u_longlong_t	common	:1;	/* Common seg	*/
	u_longlong_t	type	:2;	/* Entry type	*/
#define STT 	0x00
	u_longlong_t	fill_3	:2;
} __attribute__ ((packed)) ste;

/*------------------------------------------------------*/
/* Page table entry					*/
/*------------------------------------------------------*/
typedef struct _pte_ {
	u_longlong_t	pfra	:52;	/* PF Real Addr	*/
	u_longlong_t	fill_1	:1;
	u_longlong_t	invalid :1;	/* Page invalid	*/
	u_longlong_t	protect :1;	/* Page protect	*/
	u_longlong_t	fill_2	:9;	
} __attribute__ ((packed)) pte;
	
/*------------------------------------------------------*/
/* Virtual address					*/
/*------------------------------------------------------*/
typedef struct _va_t {
	u_longlong_t	ri1	:11;	/* Region 1	*/
	u_longlong_t	ri2	:11;	/* Region 2	*/
	u_longlong_t	ri3	:11;	/* Region 3	*/
	u_longlong_t	si 	:11;	/* Segment index*/
	u_longlong_t	pi	:8;	/* Page index	*/
	u_longlong_t	offset	:12;    /* Byte offset  */
} va_t;
	
/*------------------------------------------------------*/
/* Table managing Region/Segment/Page Tables		*/
/*------------------------------------------------------*/
typedef struct _rspList {
	uint64_t covers;		// Area covered - start
	uint64_t covere;		// Area covered - end
	void	*origin;		// Table origin
	void	*next;			// Next list entry
	void	*prev;			// Previous list entry
	size_t	length;			// Length of table
	size_t	nents; 			// Number of entries
} rspList;

/*------------------------------------------------------*/
/* Track early memory allocations              		*/
/*------------------------------------------------------*/
typedef struct __earlyAlloc {
	void	*addr;		// Address of allocation
	size_t	size;		// Size of allocation
} earlyAlloc;

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _MM_S390X_H */
