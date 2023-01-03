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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	_SYS_INTR_H
#define	_SYS_INTR_H

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Each cpu allocates two arrays, intr_head[] and intr_tail[], with the
 * size of PIL_LEVELS each. The entry 0 of these arrays are reserved.
 *
 * The entries 1-15 of the arrays are the head and the tail of interrupt
 * level 1-15 request queues.
 */
#define	PIL_LEVELS	16	/* 0	: reserved */
				/* 1-15 : for the pil level 1-15 */

#define	PIL_1	1
#define	PIL_2	2
#define	PIL_3	3
#define	PIL_4	4
#define	PIL_5	5
#define	PIL_6	6
#define	PIL_7	7
#define	PIL_8	8
#define	PIL_9	9
#define	PIL_10	10
#define	PIL_11	11
#define	PIL_12	12
#define	PIL_13	13
#define	PIL_14	14
#define	PIL_15	15

#define S390_INTR_MCHK		0
#define S390_INTR_SVC		1
#define S390_INTR_PGM		2
#define S390_INTR_EXT		3
#define S390_INTR_IO		4
#define S390_INTR_RESTART	5

#ifndef _ASM
struct cpu;
extern uint64_t poke_cpu_inum;
extern void intr_init(struct cpu *);
extern int intr_restore(int);
extern int intr_clear(void);
extern int intr_enable(void);
extern void sti(void);
extern void cli(void);

typedef struct _mcic {
	uint_t	systemDmg	: 1;	/* system damage		      00*/
	uint_t	instDmg		: 1;	/* instruction processing damage      01*/
	uint_t	sysRec		: 1;	/* system recovery		      02*/
	uint_t	fill1		: 1;
	uint_t	timingDmg	: 1;	/* timing facility damage	      04*/
	uint_t	extDmg		: 1;	/* external damage		      05*/
	uint_t	fill2		: 1;
	uint_t	degrad		: 1;	/* degradation			      07*/
	uint_t	warning		: 1;	/* warning			      08*/
	uint_t	chanRpt		: 1;	/* channel report pending	      09*/
	uint_t	servDmg		: 1;	/* service processor damage	      10*/
	uint_t	chanDmg		: 1;	/* channel subsystem damage	      11*/
	uint_t	fill3		: 2;
	uint_t	backup		: 1;	/* backed up			      14*/
	uint_t	fill4		: 1;
	uint_t	storUncorr	: 1;	/* storage error uncorrected	      16*/
	uint_t	storCorr	: 1;	/* storage error corrected	      17*/
	uint_t	keyUncorr	: 1;	/* storage key error uncorrected      18*/
	uint_t	storDeg		: 1;	/* storage degradation		      19*/
	uint_t	mwpVal		: 1;	/* PSW MWP validity		      20*/
	uint_t	mkeyVal		: 1;	/* PSW mask and key validity	      21*/
	uint_t	pmccVal		: 1;	/* PSW pgm mask, cond code validity   22*/
	uint_t	instVal		: 1;	/* PSW instrution address validity    23*/
	uint_t	fsaVal		: 1;	/* failing storage address validity   24*/
	uint_t	fill5		: 1;
	uint_t	extdmgVal	: 1;	/* external damage code validity      26*/
	uint_t	fpregVal	: 1;	/* floating point register validity   27*/
	uint_t	gregVal		: 1;	/* general register validity	      28*/
	uint_t	cregVal		: 1;	/* control register validity	      29*/
	uint_t	fill6		: 1;
	uint_t	slogVal		: 1;	/* storage logical validity	      31*/
	uint_t	istorVal	: 1;	/* indirect storage validity	      32*/
	uint_t	aregVal		: 1;	/* access register validity	      33*/
	uint_t	delayAcc	: 1;	/* delayed access exception	      34*/
	uint_t	fill7		: 7;
	uint_t	todVal		: 1;	/* TOD programmable register validity 42*/
	uint_t	fpcregVal	: 1;	/* floating point ctrl reg validity   43*/
	uint_t	ancRpt		: 1;	/* ancillary report		      44*/
	uint_t	fill8		: 1;
	uint_t	timerVal	: 1;	/* CPU timer validity		      46*/
	uint_t	clkVal		: 1;	/* clock comparator validity	      47*/
	uint_t	fill9		: 16;
} mcic;	

typedef struct _intparms
{
	int vector;
	caddr_t arg2;
	union {
		struct {
			uint32_t intparm;
			uint16_t intcode;
			uint16_t subcode;
			uint64_t extparm;
		} ext;
		struct {
			uint32_t intparm;
			uint32_t idw;
			uint32_t schid;
		} io;
		struct {
			union {
				uint64_t intcode;
				mcic mcic;
			} u;
		} mch;
	} u;
} intparms;

#endif	/* !_ASM */

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_INTR_H */
