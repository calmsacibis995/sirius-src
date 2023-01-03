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
/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef MACH_S390_H
#define MACH_S390_H

#ifndef _ASM

#include <sys/types.h>
#include <sys/regset.h>

extern void a2e(const char *, size_t);
extern void e2a(const char *, size_t);

#ifdef __s390x__

#define GET_TRC_CR(cr)	__asm__ ("	stctg	12,12,%0\n"	\
				 : "=Q" (cr));

#define SET_TRC_CR(cr)	__asm__ ("	la	1,%0\n"		\
				 "	lctlg	12,12,0(1)\n"	\
				 : : "m" (cr));

static void __inline__
ptlb(void) 
{
	__asm__ ("ptlb");
}

static void __inline__
quiesce_cpu(void)
{
	__asm__ ("	larl	1,qscPSW\n"
		 "	lpswe	0(1)\n"
		 : : : "1", "cc");
}

static void __inline__
yield_cpu(int cpuid)
{
	__asm__ ("	diag	%0,0,0x9c\n"
		 : : "r" (cpuid) : "cc");
}

static void __inline__
xptlb(void) 
{
	long a = 0, b = 1, *c = &a;

	__asm__ ("	l	0,%0\n"
		 "	lgr	1,%1\n"
		 "	lgr	2,%2\n"
		 "	oill	0,1\n"
		 "	cspg 	1,0"
		 : "+m" (c) 
		 : "r" (a), "r" (b) 
		 : "0", "1", "2", "cc", "memory");
}

static long __inline__
cspg(void *val, long old, long new)
{
	__asm__ __volatile__ (	"	l	0,%0\n"
				"	lgr	1,%1\n" 
				"	lgr	2,%2\n" 
				"	oill	0,1\n"
				" 	cspg	1,0\n"
				"	lgr	%1,1\n"
				: "+m" (val), "+r" (old) 
				: "r" (new)
				: "0", "1", "2", "cc", "memory");	 
	return (old);
}

static long __inline__	
get_cr(int reg)
{				
	long cr,
	     creg;

	__asm__ ("	larl	1,getcr\n"
		 "	lgfr	2,%2\n"
		 "	sllg	2,2,4\n"
		 "	ogr	2,%2\n"
		 "	la	3,%0\n"
		 "	ex	2,0(1)\n"
		 "	lg	%1,%0\n"
		 : "+m" (cr), "=r" (creg) 
		 : "r" (reg) 
		 : "1", "2", "3", "cc", "memory");

	return (creg);
}

static void __inline__
set_cr(long creg, int reg) 
{
	__asm__ ("	larl	1,setcr\n"
		 "	lgfr	2,%1\n"
		 "	sllg	2,2,4\n"
		 "	ogr	2,%1\n"
		 "	la	3,%0\n"
		 "	ex	2,0(1)\n"
		 : : "m" (creg), "r" (reg) 
		 : "1", "2", "3", "cc", "memory");
}
		 
static void __inline__
clear_page(void *page)
{
	__asm__ __volatile__ ("	slgr	1,1\n"
			      "	lgr	2,%0\n"
			      "	lghi	3,4096\n"
			      "	mvcl	2,0"
			      : "+&a" (page) 
			      : : "0", "1", "2", "3", "cc", "memory");

}

static void __inline__
copy_page(void *to, void *from)
{
	__asm__ __volatile__ ("	lghi	0,0\n"
			      " mvpg	%1,%2"
			      : "+a" (to) : "a" (from) : "0", "cc", "memory");
}

static void __inline__
enable_ext()
{
	long mask;

	__asm__ ("	stosm	0(%0),0x01"
		 : : "a" (&mask));
}

static void __inline__
disable_ext()
{
	long mask;

	__asm__ ("	stnsm	0(%0),0xfe"
		 : : "a" (&mask));
}

/*------------------------------------------------------*/
/* Control Registers...  				*/
/*------------------------------------------------------*/
typedef struct _ctlr0_masks {
	uchar_t fill_1[4];		/* Top half of CR0		*/
	uchar_t	todCtl	:1;		/* Trace TOD-clock control	*/ 
	uchar_t	ssmCtl	:1;		/* SSM suppression control	*/
	uchar_t	todSyn	:1;		/* TOD-clock sync control	*/
	uchar_t	lowPrt	:1;		/* Low address protection	*/
	uchar_t	extCtl	:1;		/* Extraction authority control	*/
	uchar_t	secCtl	:1;		/* Secondary space control	*/
	uchar_t	ftcPrt	:1;		/* Fetch protection override	*/
	uchar_t	stgPrt	:1;		/* Storage protection override	*/
	uchar_t	fill_2	:4;		/*				*/
	uchar_t	AsnLx	:1;		/* ASN and LX reuse control	*/
	uchar_t	afpCtl	:1;		/* AFP-register control		*/
	uchar_t	fill_3	:2;		/*				*/
	uchar_t	malAlt	:1;		/* Malfunction alert subclass	*/
	uchar_t	emgSub	:1;		/* Emergency-signal subclass	*/
	uchar_t	extCall	:1;		/* External-call subclass mask	*/
	uchar_t	fill_4	:1;		/*				*/
	uchar_t	clkCmp	:1;		/* Clock-comparator subclass	*/
	uchar_t	cpuTmr	:1;		/* CPU-timer subclass mask	*/
	uchar_t	ssgSub	:1;		/* Service-signal subclass mask */
	uchar_t	fill_5	:1;		/*				*/
	uchar_t	unused1	:1;		/* Not used but set to '1'	*/
	uchar_t	intKey	:1;		/* Interrupt-key subclass mask	*/
	uchar_t	unused2	:1;		/* Not used but set to '1'	*/
	uchar_t	etrSub	:1;		/* ETR subclass mask		*/
	uchar_t	fill_6	:1;		/*				*/
	uchar_t	crpCtl	:1;		/* Crypto control		*/
	uchar_t	fill_7	:2;		/*				*/
} __attribute__ ((packed)) ctlr0_masks;

typedef union _ctlr0 {
	ctlr0_masks	mask;
	ulong_t		value;
} __attribute__ ((packed,aligned(8))) ctlr0;
	
#define CR0VALUE 0x00000000

typedef struct _ctlr1 {
	ulong_t to	:52;		/* Primary Region/Segment/Space */
	ulong_t	fill_1	:2;
	ulong_t	ssgCtl	:1;		/* Subspace group control	*/
	ulong_t	pspCtl	:1;		/* Private-space control	*/
	ulong_t	staCtl	:1;		/* Storage-alteration-event	*/
	ulong_t	sseCtl	:1;		/* Private-space-switch control	*/
	ulong_t	rspCtl	:1;		/* Real-space control		*/
	ulong_t	fill_2	:1;	
	ulong_t	dsgCtl	:2;		/* Designation-type control	*/
	ulong_t	tblLen	:2;		/* Table length			*/
} __attribute__ ((packed,aligned(8))) ctlr1;
	
typedef struct _ctlr2 {
	uint_t	fill_1;
	uint_t	ducto	:25;		/* Dispatchable unit ctl tbl 	*/
	uint_t	fill_2	:7;	
} __attribute__ ((packed,aligned(8))) ctlr2;

typedef struct _ctlr3 {
	uint_t		sasnIns;	/* 2ndary ASN-2nd-table entry	*/
	ushort_t	PSWKeyMask;	/* PSW-key mask			*/
	ushort_t	secASN;		/* Secondary ASN		*/
} __attribute__ ((packed,aligned(8))) ctlr3;

typedef struct _ctlr4 {
	uint_t		pasnIns;	/* Primary ASN-2nd-table entry 	*/
	ushort_t	authIdx;	/* Authorization index		*/
	ushort_t	priASN;		/* Primary ASN			*/
} __attribute__ ((packed,aligned(8))) ctlr4;

typedef struct _ctlr5 {
	uint_t	fill_1;
	uint_t	priSecTo	:25;	/* Primary ASN-2nd-table org	*/
	uint_t	fill_2		:7;	
} __attribute__ ((packed,aligned(8))) ctlr5;

typedef struct _ctlr6 {
	uint_t	fill_1;
	uchar_t	intMask;		/* I/O interrupt subclass mask	*/
	uchar_t	fill_2[3];		
} __attribute__ ((packed,aligned(8))) ctlr6;

typedef struct _ctlr7 {
	ulong_t to	:52;		/* 2ndary Region/Segment/Space  */
	ulong_t	fill_1	:2;
	ulong_t	ssgCtl	:1;		/* Subspace group control	*/
	ulong_t	pspCtl	:1;		/* Private-space control	*/
	ulong_t	staCtl	:1;		/* Storage-alteration-event	*/
	ulong_t	sseCtl	:1;		/* Private-space-switch control	*/
	ulong_t	rspCtl	:1;		/* Real-space control		*/
	ulong_t	fill_2	:1;	
	ulong_t	dsgCtl	:2;		/* Designation-type control	*/
	ulong_t	tblLen	:2;		/* Table length			*/
} __attribute__ ((packed,aligned(8))) ctlr7;
	
typedef struct _ctlr8 {
	uint_t		fill_1;
	ushort_t	xAuthIdx;	/* Extended Authorization index	*/
	ushort_t	monMasks;	/* Monitor masks		*/
} __attribute__ ((packed,aligned(8))) ctlr8;

typedef struct _ctlr9 {
	int 	fill_1;
	uchar_t	br	:1;		/* Successful branch event mask	*/
	uchar_t	ifch	:1;		/* Intruction fetch event mask	*/
	uchar_t	fill_2	:1;
	uchar_t	sa	:1;		/* Storage alteration event mask*/
	uchar_t	sura	:1;		/* Store-using-real-address mask*/
	uchar_t	fill_3	:2;
	uchar_t	ifNull	:1;		/* Instruction-fetching-null	*/
	uchar_t	bac	:1;		/* Branch address control	*/
	uchar_t	fill_4	:1;
	uchar_t	sac	:1;		/* Storage alteration control	*/
	uchar_t	fill_5	:5;	
	short	fill_6;
} __attribute__ ((packed,aligned(8))) ctlr9;

typedef struct _ctlr10 {
	void 	*perStart;		/* PER starting address		*/
} ctlr10;

typedef struct _ctlr11 {
	void 	*perEnd;  		/* PER ending address		*/
} ctlr11;

typedef struct _ctlr12 {
	union {
		struct {
			ulong_t	brTrc	:1;	/* Branch trace control		*/
			ulong_t	mdTrc	:1;	/* Mode trace control		*/
			ulong_t	teAdd	:60;	/* Trace-entry address		*/
			ulong_t	asnTrc	:1;	/* ASN trace control		*/
			ulong_t	exTrc	:1;	/* Explicit trace control	*/
		} bits;
		ulong_t tbl;			/* Address of trace table	*/
	} data;
} __attribute__ ((packed,aligned(8))) ctlr12;

typedef struct _ctlr13 {
	ulong_t to	:52;		/* Home Region/Segment/Space    */
	ulong_t	fill_1	:2;
	ulong_t	ssgCtl	:1;		/* Subspace group control	*/
	ulong_t	pspCtl	:1;		/* Private-space control	*/
	ulong_t	staCtl	:1;		/* Storage-alteration-event	*/
	ulong_t	sseCtl	:1;		/* Private-space-switch control	*/
	ulong_t	rspCtl	:1;		/* Real-space control		*/
	ulong_t	fill_2	:1;	
	ulong_t	dsgCtl	:2;		/* Designation-type control	*/
	ulong_t	tblLen	:2;		/* Table length			*/
} __attribute__ ((packed,aligned(8))) ctlr13;
	
typedef struct _c14_mask {
	uchar_t		fill_3	:2;
	uchar_t		todOvr	:1;	/* TOD-clock-control-overide	*/
	uchar_t		fill_4	:1;
	uchar_t		asnTc	:1;	/* ASN-translation control	*/
	uchar_t		fill_5	:4;
	ushort_t	fill_6;
} __attribute__ ((packed)) c14_mask;

typedef struct _c14_asn {
	uint_t	fill_1	:5;
	uint_t	asnTo	:27;		/* ASN-first-table origin	*/
} __attribute__ ((packed)) c14_asn;

typedef struct _ctlr14 {
	uint_t	fill_1;
	uchar_t	unused_1	:1;	/* Unused but set to '1'	*/
	uchar_t	unused_2	:1;	/* Unused but set to '1'	*/
	uchar_t	fill_2		:1;
	uchar_t	crp		:1;	/* Channel report pending mask	*/
	uchar_t	rec		:1;	/* Recovery subclass mask	*/
	uchar_t	deg		:1;	/* Degredation subclass mask	*/
	uchar_t	extDmg		:1;	/* External damage subclass mask*/
	uchar_t	warn		:1;	/* Warning subclass mask	*/
	union {
		c14_mask	masks;
		c14_asn		asn;
	} asnDat;
} __attribute__ ((packed,aligned(8))) ctlr14;

typedef struct _ctlr15 {
	void	*lse;			/* Linkage stack entry address	*/
} __attribute__ ((aligned(8))) ctlr15;

#endif

/*--------------------------------------------------------*/
/* System Information Blocks (SYSIB)			  */
/*--------------------------------------------------------*/

//
// Basic Machine
//
typedef struct _sysib111 {
	char	resv000[32];	// Reserved
	char	mfg[16];	// Manufacturer
	char	type[4];	// Type
	char	resv001[12];	// Reserved
	char	mdlCapp[16];	// Model Capacity
	char	seqCode[16];	// Sequence Code
	char	plant[4];	// Plant of manufacture
	char	model[16];	// Model
} sysib_1_1_1;

#ifdef __GNUC__
/*--------------------------------------------------------*/
/* Absolute and Real Storage Layout...			  */
/*--------------------------------------------------------*/
typedef struct _pfxPage {
	//
	//	Private to CPU's prefix page
	//
	long	 __lc_ipl_psw;			/* 0x0000 */
	long	 __lc_ipl_ccw1;			/* 0x0008 */
	long	 __lc_ipl_ccw2;			/* 0x0010 */
	char	 fill_01[0x80-0x18];		/* 0x0018 */
	uint32_t __lc_ext_intparm;		/* 0x0080 */
	uint16_t __lc_cpu_addr;			/* 0x0084 */
	uint16_t __lc_ext_intcode;		/* 0x0086 */
	char	 fill_02[0x89-0x88];		/* 0x0088 */
	uint8_t	 __lc_svc_ilc;			/* 0x0089 */
	uint16_t __lc_svc_intcode;		/* 0x008a */
	char	 __lc_pgm_null;			/* 0x008c */
	uint8_t  __lc_pgm_ilc;			/* 0x008d */
	uint16_t __lc_pgm_intcode;		/* 0x008e */
	uint32_t __lc_dxc;			/* 0x0090 */
	uint16_t __lc_monitor_class;		/* 0x0094 */
	uint8_t	 __lc_per_code;			/* 0x0096 */
#define _per_evt_br	0x80
#define _per_evt_fetch	0x40
#define _per_evt_sa	0x20
#define _per_evt_stura	0x08
#define _per_evt_null	0x01
	uint8_t	 __lc_per_atmid;		/* 0x0097 */
#define _per_atm_bit31  0x80
#define _per_atm_valid  0x40
#define _per_atm_bit32  0x20
#define _per_atm_bit5   0x10
#define _per_atm_bit16  0x08
#define _per_atm_bit17  0x04
#define _per_asce_pri	0x00
#define _per_asce_ar	0x01
#define _per_asce_sec	0x02
#define _per_asce_home	0x03
	uint64_t __lc_per_addr;			/* 0x0098 */
	uint8_t	 __lc_exc_accid;		/* 0x00a0 */
	uint8_t	 __lc_per_accid;		/* 0x00a1 */
	uint8_t	 __lc_op_accid;			/* 0x00a2 */
	uint8_t	 __lc_ss_accid;			/* 0x00a3 */
#define __lc_mc_accid	_lc_ss_accid
	char	 fill_04[0xa8-0xa4];		/* 0x00a4 */
	uint64_t __lc_xlt_excid;		/* 0x00a8 */
	uint64_t __lc_monitor_code;		/* 0x00b0 */
	uint16_t __lc_schid;			/* 0x00b8 */
	uint16_t __lc_sch_nr;			/* 0x00ba */
	uint32_t __lc_io_intparm;		/* 0x00bc */
	uint32_t __lc_io_idw;			/* 0x00c0 */
	char	 fill_05[0xc8-0xc4];		/* 0x00c4 */
	uint32_t __lc_stfl_list;		/* 0x00c8 */
	char	 fill_06[0xe8-0xcc];		/* 0x00cc */
	uint64_t __lc_mc_intcode;		/* 0x00e8 */
	char	 fill_07[0xf4-0xf0];		/* 0x00f0 */
	uint32_t __lc_extdmg_code;		/* 0x00f4 */
	uint64_t __lc_failstor_addr;		/* 0x00f8 */
	char	 fill_08[0x110-0x100];		/* 0x0100 */
	uint64_t __lc_brkevt_addr;		/* 0x0110 */
	char	 fill_09[0x120-0x118];		/* 0x0118 */
	pswg_t	 __lc_rst_old_psw;		/* 0x0120 */
	pswg_t	 __lc_ext_old_psw;		/* 0x0130 */
	pswg_t	 __lc_svc_old_psw;		/* 0x0140 */
	pswg_t	 __lc_pgm_old_psw;		/* 0x0150 */
	pswg_t	 __lc_mc_old_psw;		/* 0x0160 */
	pswg_t	 __lc_io_old_psw;		/* 0x0170 */
	char	 fill_10[0x1a0-0x180];		/* 0x0180 */
	pswg_t	 __lc_rst_new_psw;		/* 0x01a0 */
	pswg_t	 __lc_ext_new_psw;		/* 0x01b0 */
	pswg_t	 __lc_svc_new_psw;		/* 0x01c0 */
	pswg_t	 __lc_pgm_new_psw;		/* 0x01d0 */
	pswg_t	 __lc_mc_new_psw;		/* 0x01e0 */
	pswg_t	 __lc_io_new_psw;		/* 0x01f0 */
	char	 fill_11[0x280-0x200];		/* 0x0208 */
	pswg_t	 __lc_run_psw;			/* 0x0280 */
	uint64_t __lc_timer_syn_end;  		/* 0x0290 */
	uint64_t __lc_timer_asy_end;  		/* 0x0298 */
	uint64_t __lc_timer_start;		/* 0x02a0 */
	uint64_t __lc_timer_user; 		/* 0x02a8 */
	uint64_t __lc_timer_last; 		/* 0x02b0 */
	uint64_t __lc_timer_sys;  		/* 0x02b8 */
	uint64_t __lc_cpu;        		/* 0x02c0 */
	uint64_t __lc_scratch;			/* 0x02c8 */
	uint32_t __lc_parmreg[16];		/* 0x02d0 */
	uint32_t fill_12[16];			/* 0x0310 */
	uint32_t __lc_rsm_trace;		/* 0x0350 */
	uint32_t __lc_svc_trace;		/* 0x0354 */
	uint32_t __lc_pgm_trace;		/* 0x0358 */
	uint32_t __lc_hat_trace;		/* 0x035c */
	uint32_t __lc_any_trace;		/* 0x0360 */
	char	 fill_13[0x400-0x364];		/* 0x0364 */
	void	 *__lc_kstack;			/* 0x0400 */
	uint64_t __lc_syn_save_area;		/* 0x0408 */
	uint64_t __lc_asy_save_area;		/* 0x0410 */
	void	 *__lc_bootops;         	/* 0x0418 */
	char	 fill_14[0x11b8-0x420];		/* 0x0420 */
	uint64_t __lc_blkio_parm;		/* 0x11b8 */
	char	 fill_15[0x1200-0x11c0];	/* 0x11c0 */
	double	 __lc_ssmc_fp_area[16];		/* 0x1200 */
	uint64_t __lc_ssmc_gr_area[16];		/* 0x1280 */
	pswg_t	 __lc_ssmc_psw_area;		/* 0x1300 */
	char	 fill_16[0x1318-0x1310];	/* 0x1310 */
	uint32_t __lc_ssmc_pfx_area;		/* 0x1318 */
	uint32_t __lc_ssmc_fpc_area;		/* 0x131c */
	char	 fill_17[0x1324-0x1320];	/* 0x1320 */
	uint32_t __lc_ssmc_tod_area;		/* 0x1324 */
	uint64_t __lc_ssmc_tmr_area;		/* 0x1328 */
	uint64_t fill_18 :8;			/* 0x1330 */
	uint64_t __lc_ssmc_ckc_area :56;	/* 0x1331 */
	char	 fill_19[0x1340-0x1338];	/* 0x1338 */
	uint64_t __lc_ssmc_cr_area[16];		/* 0x1340 */
	uint32_t __lc_ssmc_ar_area[16];		/* 0x1380 */
	char	 fill_20[0x2000-0x1400];	/* 0x1400 */
} __attribute__ ((packed)) _pfxPage;

typedef struct __storKey {
	uint8_t	acc   : 4;	// Access key
	uint8_t fetch : 1;	// Fetch protected
	uint8_t ref   : 1;	// Referenced
	uint8_t mod   : 1;	// Modified
} storKey;
#endif

#define SK_FETCH	0x08
#define SK_REF  	0x04
#define SK_MOD  	0x02

/*
 * Processor facility list - 
 *	Bit 	Meaning When Bit Is One
 *	0 	The instructions marked N3 in the instruction summary are installed
 *	1 	The z/Architecture architectural mode is installed.
 *	2 	The z/Architecture architectural mode is active.
 *		When this bit is zero, the ESA/390 architectural
 *		mode is active.
 *	3 	The DAT-enhancement facility is installed in the
 *		z/Architecture architectural mode. The DAT-enhancement
 *		facility includes the INVALIDATE DAT TABLE ENTRY (IDTE) 
 *		and COMPARE AND SWAP AND PURGE (CSPG)
 *	4	INVALIDATE DAT TABLE ENTRY (IDTE) performs the 
 *		invalidation-and-clearing operation by selectively clearing 
 *		combined region-and-segment table entries when a segment-table 
 *		entry or entries are invalidated. IDTE also performs the clearing-by-
 *		ASCE operation. Unless bit 4 is one, IDTE simply purges all 
 *		TLBs. Bit 3 is one if bit 4 is one.
 *	5 	INVALIDATE DAT TABLE ENTRY (IDTE) performs the invalidation-and-
 *		clearing operation by selectively clearing combined region-and-
 *		segment table entries when a region-table entry or entries are 
 *		invalidated. Bits 3 and 4 are ones if bit 5 is one.
 *	6 	The ASN-and-LX reuse facility is installed in the z/Architecture 
 *		architectural mode.
 *	7 	The store-facility-list-extended facility is installed.
 *	9 	The sense-running-status facility is installed in the z/Architecture 
 *		architectural mode.
 *	10 	The conditional-SSKE facility is installed in the z/Architecture 
 *		architectural mode.
 *	16 	The extended-translation facility 2 is installed.
 *	17 	The message-security assist is installed.
 *	18 	The long-displacement facility is installed in the z/Architecture 
 *		architectural mode.
 *	19 	The long-displacement facility has high performance. Bit 18 is one 
 *		if bit 19 is one.
 *	20 	The HFP-multiply-and-add/subtract facility is installed.
 *	21 	The extended-immediate facility is installed in the z/Architecture 
 *		architectural mode.
 *	22 	The extended-translation facility 3 is installed in the z/Architecture 
 *		architectural mode.
 *	23 	The HFP-unnormalized-extension facility is installed in the 
 *		z/Architecture architectural mode.
 *	24 	The ETF2-enhancement facility is installed.
 *	25 	The store-clock-fast facility is installed in the z/Architecture 
 *		architectural mode.
 *	28 	The TOD-clock-steering facility is installed in the z/Architecture 
 *		architectural mode.
 *	30 	The ETF3-enhancement facility is installed in the z/Architecture 
 *		architectural mode.
 *	31 	The extract-CPU-time facility is installed in the z/Architecture 
 *		architectural mode.
 *	32 	The compare-and-swap-and-store facility is installed in the 
 *		z/Architecture architectural mode.
 *	41 	The floating-point-support-enhancement facilities (FPR-GR-loading, 
 *		FPS-sign-handling, and DFProunding) are installed in the z/Architecture
 *		architectural mode.
 *	42 	The DFP (decimal-floating-point) facility is installed in the 
 *		z/Architecture architectural mode.
 *	43 	The DFP (decimal-floating-point) facility has high performance. 
 *		Bit 42 is one if bit 43 is one.
 *	44 	The PFPO instruction is installed in the z/Architecture architectural mode.
 *
 */

typedef struct __facList {
	uint8_t facN3	  :1;
	uint8_t facZarch  :1;
	uint8_t facZact	  :1;
	uint8_t facDATe	  :1;
	uint8_t facIDTErs :1;
	uint8_t facIDTEr  :1;
	uint8_t facLX	  :1;
	uint8_t facSTFLE  :1;
	uint8_t facSRSF	  :1;
	uint8_t facSSKE	  :1;
	uint8_t facETF2	  :1;
	uint8_t facMSA	  :1;
	uint8_t facLDF	  :1;
	uint8_t facLDFHP  :1;
	uint8_t facMASF	  :1;
	uint8_t facXIMF	  :1;
	uint8_t facETF3	  :1;
	uint8_t facUXF	  :1;
	uint8_t facETF2E  :1;
	uint8_t facSTCKF  :1;
	uint8_t facETF3E  :1;
	uint8_t facXCPU	  :1;
	uint8_t facCSST	  :1;
	uint8_t facFPSE	  :1;
	uint8_t facDFP	  :1;
	uint8_t facDFPHP  :1;
	uint8_t facPFPO	  :1;
} facList_t;

extern facList_t facilities;

#endif

/*
 *	Private to CPU's prefix area
 */
#define __LC_IPL_PSW		0x00
#define __LC_IPL_CCW1		0x08
#define __LC_IPL_CCW2		0x10
#define __LC_EXT_INTPARM	0x80
#define __LC_CPU_ADDR		0x84
#define __LC_EXT_SUBCODE	0x84
#define __LC_EXT_INTCODE	0x86
#define	__LC_SVC_ILC		0x89
#define __LC_SVC_INTCODE	0x8a
#define __LC_PGM_NULL		0x8c
#define __LC_PGM_ILC		0x8d
#define __LC_PGM_INTCODE	0x8e
#define __LC_DXC		0x90
#define __LC_MONITOR_CLASS	0x94
#define __LC_PER_CODE		0x96
#define __LC_PER_ATMID		0x97
#define __LC_PER_ADDR		0x98
#define	__LC_EXC_ACCID		0xa0
#define __LC_PER_ACCID		0xa1
#define	__LC_OP_ACCID		0xa2
#define __LC_SS_ACCID		0xa3
#define __LC_MC_ACCID		0xa3
#define	__LC_XLT_EXCID		0xa8
#define __LC_MONITOR_CODE	0xb0
#define __LC_SCHID		0xb8
#define __LC_SCH_NR		0xba
#define	__LC_IO_INTPARM		0xbc
#define	__LC_IO_IDW		0xc0
#define	__LC_STFL_LIST		0xc8
#define	__LC_MC_INTCODE		0xe8
#define __LC_EXTDMG_CODE	0xf4
#define	__LC_FAILSTOR_ADDR	0xf8
#define	__LC_BRKEVT_ADDR	0x110
#define	__LC_RST_OLD_PSW	0x120
#define	__LC_EXT_OLD_PSW	0x130
#define	__LC_SVC_OLD_PSW	0x140
#define __LC_PGM_OLD_PSW	0x150
#define	__LC_MC_OLD_PSW		0x160
#define	__LC_IO_OLD_PSW		0x170
#define	__LC_RST_NEW_PSW	0x1a0
#define	__LC_EXT_NEW_PSW	0x1b0
#define	__LC_SVC_NEW_PSW	0x1c0
#define __LC_PGM_NEW_PSW	0x1d0
#define	__LC_MC_NEW_PSW		0x1e0
#define	__LC_IO_NEW_PSW		0x1f0
#define __LC_RUN_PSW		0x280
#define __LC_TIMER_SYN_END  	0x290
#define __LC_TIMER_ASY_END  	0x298
#define __LC_TIMER_START	0x2a0
#define __LC_TIMER_USER 	0x2a8
#define __LC_TIMER_LAST 	0x2b0
#define __LC_TIMER_SYS  	0x2b8
#define __LC_CPU		0x2c0
#define __LC_SCRATCH		0x2c8
#define __LC_PARMREG		0x2d0
#define __LC_RSM_TRACE		0x350
#define __LC_SVC_TRACE		0x354
#define __LC_PGM_TRACE		0x358
#define __LC_HAT_TRACE		0x35c
#define __LC_ANY_TRACE		0x364
#define	__LC_KSTACK		0x400
#define	__LC_SYN_SAVE_AREA	0x408
#define	__LC_ASY_SAVE_AREA	0x410
#define	__LC_BOOTOPS  		0x418
#define __LC_BLKIO_PARM		0x11b8
#define	__LC_SSMC_FP_AREA	0x1200
#define	__LC_SSMC_GR_AREA	0x1280
#define	__LC_SSMC_PSW_AREA	0x1300
#define	__LC_SSMC_PFX_AREA	0x1318
#define	__LC_SSMC_FPC_AREA	0x131c
#define	__LC_SSMC_TOD_AREA	0x1324
#define	__LC_SSMC_TMR_AREA	0x1328
#define	__LC_SSMC_CKC_AREA	0x1331
#define	__LC_SSMC_CR_AREA	0x1340
#define	__LC_SSMC_AR_AREA	0x1380

/*
 *	Visible to all CPUs
 */
#define	__LC_PSTATE  		0x2000
#define	__LC_WSTATE  		0x2008
#define   LC_PRIORITY		0x2010

/*
 * Set Address Control settings
 */
#define AC_PRIMARY	0x000
#define AC_SECONDARY	0x100
#define AC_ACCESS	0x200
#define AC_HOME		0x300

#endif 
