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
#ifndef IO_S390X_H
#define IO_S390X_H

#include <sys/types.h>

/*------------------------------------------------------*/
/* Channel Command Word...  				*/
/*------------------------------------------------------*/
typedef struct ccw1 {
	uchar_t		op;		/* Opcode	*/
	uchar_t		flags;		/* Flags	*/
	ushort_t	count;		/* Data count	*/
	uint_t		data;		/* Data address	*/
} __attribute__ ((packed,aligned(8))) ccw1;

#define CCW_FLAG_DC             0x80
#define CCW_FLAG_CC             0x40
#define CCW_FLAG_SLI            0x20
#define CCW_FLAG_SKIP           0x10
#define CCW_FLAG_PCI            0x08
#define CCW_FLAG_IDA            0x04
#define CCW_FLAG_SUSPEND        0x02
#define CCW_FLAG_MIDAW          0x01

#define CCW_CMD_WRITE		0x01
#define CCW_CMD_READ		0x02
#define CCW_CMD_READ_IPL        0x02
#define CCW_CMD_NOP	        0x03
#define CCW_CMD_SENSE		0x04
#define CCW_CMD_READ_ECKD	0x06
#define CCW_CMD_SEEK	 	0x07
#define CCW_CMD_TIC             0x08
#define CCW_CMD_READ_INQUIRY	0x0a
#define CCW_CMD_READ_INHIBIT	0x0e
#define CCW_CMD_STLCK           0x14
#define CCW_CMD_READ_FBA	0x42
#define CCW_CMD_LOCATE		0x43
#define CCW_CMD_LOCATE_REC	0x47
#define CCW_CMD_SENSE_PGID      0x34
#define CCW_CMD_SUSPEND_RECONN  0x5b
#define CCW_CMD_DEFEXT          0x63
#define CCW_CMD_RDC             0x64
#define CCW_CMD_RELEASE         0x94
#define CCW_CMD_SET_PGID        0xaf
#define CCW_CMD_SENSE_ID        0xe4
#define CCW_CMD_DCTL            0xf3

/*------------------------------------------------------*/
/* Modified Indirect Address Word...			*/
/*------------------------------------------------------*/
typedef struct _midaw {
	uchar_t		rsvd[5];	/* Reserved	*/
	uchar_t		last : 1;	/* Last midaw	*/
	uchar_t		skip : 1;	/* Skip		*/
	uchar_t		dtic : 1;	/* DTI control	*/
	ushort_t	count;		/* Data count	*/
	uint64_t	data;		/* Data address	*/
} __attribute__ ((packed,aligned(8))) midaw;

/*------------------------------------------------------*/
/* Subchannel Status Word...				*/
/*------------------------------------------------------*/
typedef struct scsw {
        uint_t  key   : 4;      /* Subchannel key 	*/
        uint_t  sCtl  : 1;      /* Suspend control 	*/
        uint_t  eswf  : 1;      /* ESW format 		*/
        uint_t  cc    : 2;      /* Deferred CC		*/
        uint_t  fmt   : 1;      /* Format 		*/
        uint_t  pref  : 1;      /* Prefetch 		*/
        uint_t  isic  : 1;      /* Initial-status ctl  	*/
        uint_t  alcc  : 1;      /* Address-limit ctl   	*/
        uint_t  ssi   : 1;      /* Supress-suspended	*/
        uint_t  zcc   : 1;      /* Zero condition code 	*/
        uint_t  eEtl  : 1;      /* Extended control    	*/
        uint_t  pnop  : 1;      /* Path not operational	*/
        uint_t  resv  : 1;      /* Reserved 		*/
        uint_t  fCtl  : 3;      /* Function control 	*/
        uint_t  aCtl  : 7;      /* Activity control 	*/
        uint_t  stCtl : 5;      /* Status control 	*/
        uint_t  cpa;            /* Channel program add 	*/
        uchar_t dstat;          /* Device status 	*/
        uchar_t cstat;          /* Subchannel status 	*/
        ushort_t count;		/* Residual count 	*/
} __attribute__ ((packed)) scsw;

#define SCSW_FCTL_START_FUNC     0x04
#define SCSW_FCTL_HALT_FUNC      0x02
#define SCSW_FCTL_CLEAR_FUNC     0x01

#define SCSW_ACTL_RESUME_PEND    0x40
#define SCSW_ACTL_START_PEND     0x20
#define SCSW_ACTL_HALT_PEND      0x10
#define SCSW_ACTL_CLEAR_PEND     0x08
#define SCSW_ACTL_SCHACT         0x04
#define SCSW_ACTL_DEVACT         0x02
#define SCSW_ACTL_SUSPENDED      0x01

#define SCSW_STCTL_ALERT_STATUS  0x10
#define SCSW_STCTL_INTER_STATUS  0x08
#define SCSW_STCTL_PRIM_STATUS   0x04
#define SCSW_STCTL_SEC_STATUS    0x02
#define SCSW_STCTL_STATUS_PEND   0x01

#define DEV_STAT_ATTENTION       0x80
#define DEV_STAT_STAT_MOD        0x40
#define DEV_STAT_CU_END	         0x20
#define DEV_STAT_BUSY            0x10
#define DEV_STAT_CHN_END         0x08
#define DEV_STAT_DEV_END         0x04
#define DEV_STAT_UNIT_CHECK      0x02
#define DEV_STAT_UNIT_EXCEP      0x01

#define SCHN_STAT_PCI            0x80
#define SCHN_STAT_INCORR_LEN     0x40
#define SCHN_STAT_PROG_CHECK     0x20
#define SCHN_STAT_PROT_CHECK     0x10
#define SCHN_STAT_CHN_DATA_CHK   0x08
#define SCHN_STAT_CHN_CTRL_CHK   0x04
#define SCHN_STAT_INTF_CTRL_CHK  0x02
#define SCHN_STAT_CHAIN_CHECK    0x01

#define SNS0_CMD_REJECT         0x80
#define SNS0_INTERVENTION_REQ   0x40
#define SNS0_BUS_OUT_CHECK      0x20
#define SNS0_EQUIPMENT_CHECK    0x10
#define SNS0_DATA_CHECK         0x08
#define SNS0_OVERRUN            0x04
#define SNS0_INCOMPL_DOMAIN     0x01

#define SNS1_PERM_ERR           0x80
#define SNS1_INV_TRACK_FORMAT   0x40
#define SNS1_EOC                0x20
#define SNS1_MESSAGE_TO_OPER    0x10
#define SNS1_NO_REC_FOUND       0x08
#define SNS1_FILE_PROTECTED     0x04
#define SNS1_WRITE_INHIBITED    0x02
#define SNS1_IMPRECISE_END      0x01

#define SNS2_REQ_INH_WRITE      0x80
#define SNS2_CORRECTABLE        0x40
#define SNS2_FIRST_LOG_ERR      0x20
#define SNS2_ENV_DATA_PRESENT   0x10
#define SNS2_IMPRECISE_END      0x04

/*------------------------------------------------------*/
/* Subchannel Logout...     				*/
/*------------------------------------------------------*/
typedef struct _sublog {
	uint_t 	fill_1	:1;
	uint_t 	esf	:7;	/* Extended status flgs	*/
	uchar_t	lpum;		/* Last used path mask	*/
	uint_t 	ar	:1;	/* Anciliary report	*/
	uint_t 	fvf	:5;	/* Field validity flags	*/
	uint_t 	sac	:2;	/* Storage access code	*/
	uint_t 	tc	:2;	/* Termination code	*/
	uint_t 	dsc	:1;	/* Device status check	*/
	uint_t 	se	:1;	/* Secondary error	*/
	uint_t 	ioea	:1;	/* I/O error alert	*/
	uint_t 	sc	:3;	/* Sequence code	*/
} __attribute__ ((packed))sublog;

/*------------------------------------------------------*/
/* Extended Report Word...  				*/
/*------------------------------------------------------*/
typedef struct _erw {
	uint_t 	fill_1	:1;
	uint_t 	rlo	:1;	/* Request logging only	*/
	uint_t 	eslp	:1;	/* Extended sub logout	*/
	uint_t 	ac	:1;	/* Authorization check	*/
	uint_t 	pvr	:1;	/* Path verification rqd*/
	uint_t 	cpt	:1;	/* Channel path timeout */
	uint_t 	fsav	:1;	/* Failing stg verifictn*/
	uint_t 	cs	:1;	/* Concurrent sense	*/
	uint_t 	sccwav	:1;	/* 2ndry CCW access vfy */
	uint_t 	cscnt	:6;	/* Concurrent sense cnt */
	uchar_t fill_2[2];
} __attribute__ ((packed)) erw;

/*------------------------------------------------------*/
/* Extended Status Word - Format 0			*/
/*------------------------------------------------------*/
typedef struct esw0 {
	sublog	logout;		/* Subchannel logout	*/
	erw	extRep;		/* Extended Report Word */
	void	*fail;		/* Failing stg address  */
	uint_t	ccwad;		/* 2ndry CCW address	*/	
} __attribute__ ((packed)) esw0;

/*------------------------------------------------------*/
/* Extended Status Word - Format 1			*/
/*------------------------------------------------------*/
typedef struct esw1 {
	uchar_t	fill_1;	
	uchar_t lpum;		/* Last Path Used Mask	*/
	uchar_t	fill_2[2];
	erw	extRep;		/* Extended Report Word */
	uint_t	fill_3[3];
} __attribute__ ((packed)) esw1;
	
/*------------------------------------------------------*/
/* Extended Status Word - Format 2			*/
/*------------------------------------------------------*/
typedef struct esw2 {
	uchar_t	 fill_1;	
	uchar_t  lpum;		/* Last Path Used Mask	*/
	ushort_t dcti;		/* Device Connect Time	*/
	erw	 extRep;	/* Extended Report Word */
	uint_t	 fill_2[3];
} __attribute__ ((packed)) esw2;
	
/*------------------------------------------------------*/
/* Extended Status Word - Format 3			*/
/*------------------------------------------------------*/
typedef struct esw3 {
	uchar_t	fill_1;	
	uchar_t lpum;		/* Last Path Used Mask	*/
	uchar_t fill_2[2];
	erw	extRep;		/* Extended Report Word */
	uint_t	fill_3[3];
} __attribute__ ((packed)) esw3;

/*------------------------------------------------------*/
/* Extended Measurement Word...        			*/
/*------------------------------------------------------*/
typedef struct emw {
	uint_t	dct;		/* Device connect time	*/
	uint_t	fpt;		/* Function pending time*/
	uint_t	ddt;		/* Device disc. time	*/
	uint_t	cuqt;		/* CU queueing time	*/
	uint_t	daot;		/* Device active only tm*/
	uint_t	dt;		/* Device busy time	*/
	uint_t	icrt;		/* Initial cmd resp time*/
	uint_t	fill_1;		
} __attribute__ ((packed)) emw;

/*------------------------------------------------------*/
/* Interruption Response Block...      			*/
/*------------------------------------------------------*/
typedef struct irb {
	scsw	scsw;		/* Subchannel status 	*/
	union {
		esw0 esw0;	/* Extended status fmt0 */
		esw1 esw1;	/* Extended status fmt1 */
		esw2 esw2;	/* Extended status fmt2 */
		esw3 esw3;	/* Extended status fmt3 */
	} esw;
	uint_t	ecw;		/* Extended ctl word	*/
	emw	emw;		/* Extended measurement */
} __attribute__ ((packed,aligned(4))) irb;

/*------------------------------------------------------*/
/* Command Information Word...         			*/
/*------------------------------------------------------*/
typedef struct ciw {
	uint_t   et	:  2;	/* Entry type 		*/
	uint_t   fill_1	:  2;	/* Reserved 		*/
	uint_t   ct	:  4;	/* Command type 	*/
	uchar_t  cmd;		/* Command 		*/
	ushort_t count;		/* Count		*/
} __attribute__ ((packed)) ciw;

#define CIW_TYPE_RCD    0x0	/* Read cfg data	*/
#define CIW_TYPE_SII    0x1	/* Set interface id	*/
#define CIW_TYPE_RNI    0x2	/* Read node identifier */

/*------------------------------------------------------*/
/* Path Management Control Word...     			*/
/*------------------------------------------------------*/
typedef struct pmcw {
	uint_t	ip;		/* Interrupt parameter  */
	uint_t  fill_1	:2;
	uint_t 	isc	:3;	/* Interrupt subclass	*/
	uint_t 	fill_2	:3;
	uint_t 	enabled :1;	/* Subchannel enabled	*/
	uint_t 	lm	:2;	/* Limit mode		*/
	uint_t 	mm	:2;	/* Measurement mode	*/
	uint_t 	mp	:1;	/* Multipath mode	*/
	uint_t 	tf	:1;	/* Timing facility	*/
	uint_t 	dnv	:1;	/* Device no. valid	*/
	ushort_t dev;		/* Device number	*/
	uchar_t	lpm;		/* Logical Path Mask	*/
	uchar_t	pnom;		/* Path Not Op Mask	*/
	uchar_t	lpum;		/* Last Path Used Mask	*/
	uchar_t	pim;		/* Path Installed Mask	*/
	ushort_t mbi;		/* Measurement blk idx	*/
	uchar_t pom;		/* Path Operational Msk */
	uchar_t	pam;		/* Path Available Mask  */
	ushort_t chpid[8];	/* Channel Paths	*/
	uchar_t fill_3[3];
	uint_t 	fill_4	:5;	
	uint_t 	mbfc	:1;	/* Measurement blk fmt  */
	uint_t 	emwme	:1;	/* Ext Msrmnt Wrd Mode	*/
	uint_t 	consns  :1;	/* Concurrent sense	*/
} __attribute__ ((packed)) pmcw;

/*------------------------------------------------------*/
/* Subchannel Information Block...     			*/
/*------------------------------------------------------*/
typedef struct schib {
	pmcw	pmcw;		/* Path mgt control wrd */
	scsw	scsw;		/* Subchannel status	*/
	union {
		uint_t	mda[3];	/* Model dependent area */
		ulong_t mba;	/* Measurement blk addr */
	} mdamba;
	uint_t	mda;		/* Model dependent area */
} __attribute__ ((packed,aligned(8))) schib;

/*------------------------------------------------------*/
/* Operation Request Block...          			*/
/*------------------------------------------------------*/
typedef struct orb {
	uint_t	iop;		/* Interrupt parameter  */
	uint_t 	key	:4;	/* Storage key		*/
	uint_t 	suspend :1;	/* Suspend flag		*/
	uint_t 	stream	:1;	/* Streaming mode flag	*/
	uint_t 	mc	:1;	/* Modification control */
	uint_t 	sc	:1;	/* Synchronization ctl  */
	uint_t 	fc	:1;	/* Format control	*/
	uint_t 	pfc	:1;	/* Prefetch control	*/
	uint_t 	isic	:1;	/* Init status int ctl	*/
	uint_t 	alcc	:1;	/* Access limit ctl 	*/
	uint_t 	ssic	:1;	/* Supress suspend	*/
	uint_t 	fill_1	:1;	
	uint_t 	fmt2	:1;	/* Format-2-IDAW ctl	*/
	uint_t 	idawctl :1;	/* 2K-IDAW control	*/
	uchar_t	lpm;		/* Logical Path Mask	*/
	uint_t 	ilsm	:1;	/* Incorrect Len Supprs */
	uint_t 	midaw	:1;	/* MIDAW control	*/
	uint_t 	fill_2	:5;
	uint_t 	orbx	:1;	/* ORB extension ctl	*/
	uint_t	cpa;		/* Channel Program addr */
	uchar_t	csspri;		/* CSS priority		*/
	uchar_t	fill_3;
	uchar_t	cupri;		/* CU priority		*/
	uchar_t	fill_4;
	uint_t	fill_5[4];
} __attribute__ ((packed,aligned(8))) orb;

/*------------------------------------------------------*/
/* ECKD Device Characteristics...			*/ 
/*------------------------------------------------------*/
struct eckdchar {
	short	vrdcprim;	/* No. primary cyls	*/
	short	vrdctrkc;	/* Tracks per cylinder	*/
	int  	vrdcsect : 8;	/* Number of sectors	*/
	int 	vrdctotr : 24;	/* Total track length	*/
	short	vrdcha;		/* Length for R0 & HA	*/
	char	vrdcmode;	/* Capacity calc mode	*/
	char	vrdcmdfr;	/* Capacity calc chgd	*/
	short	vrdcnkov;	/* Non-keyed overhead	*/
	short	vrdckovh;	/* Keyed overhead	*/
	short	vrdcaltc;	/* 1st alternate cyl	*/
	short	vrdcaltr;	/* No. alternate cyls	*/
	short	vrdcdig;	/* 1st diagnostic cyl	*/
	short	vrdcdign;	/* No. diagnostic cyls	*/
	short	vrdcdvcy;	/* 1st dev support cyl	*/
	short	vrdcdvtr;	/* No. dev support cyls	*/
	char	vrdcmdr;	/* MDR record id	*/
	char	vrdcobr;	/* OBR record id	*/
	char	vrdccuid;	/* Control Unit ID	*/
	char	resv00[21];	/* Unused		*/
	char	vrdcpgid[11];	/* Path group id	*/
	char	resv01[5];	/* Unused		*/
} __attribute__ ((packed));

/*------------------------------------------------------*/
/* FBA Device Characteristics...			*/ 
/*------------------------------------------------------*/
struct fbachar {
	char	vrdcoper;	/* Operation modes	*/
	char	vrdcfbaf;	/* Device features	*/
	char  	vrdcfbac;	/* Device class		*/
	char	vrdcfbat;	/* Device type		*/
	short	vrdcrcsz;	/* Physical record size	*/
	int	vrdcbkcg;	/* Blocks/track		*/
	int	vrdcbkap;	/* Blocks/access pos	*/
	int	vrdcbkma;	/* Blks under mv access	*/
	int	vrdcbkfa;	/* Blks under fx access */
	short	vrdcbkaa;	/* Blks in alt area	*/
	short	vrdcbkce;	/* Blks in CE area	*/
	short	vrdcbflg;	/* No. buffered log byts*/
	short	vrdcatmi;	/* Min. access time	*/
	short	vrdcatma;	/* Max. access time	*/
} __attribute__ ((packed));

/*------------------------------------------------------*/
/* Device Characteristics...				*/ 
/*------------------------------------------------------*/
struct rdvchar {
	uint16_t vrdccuty;	/* Control Unit type	*/
	uint8_t  vrdccumd;	/* Control Unit model	*/
	uint16_t vrdcdvty;	/* Device type      	*/
	uint8_t  vrdcdvmd;	/* Device model		*/
	uint8_t  vrdcdvfe[3];	/* Features supported	*/
	uint8_t  vrdcsdfe;	/* Subsystem features	*/
	uint8_t  vrdcdvcl;	/* Device class		*/
	uint8_t  vrdcdvco;	/* Device code 		*/
	union {
	  struct eckdchar eckd;	/* ECKD device chars	*/
	  struct fbachar fba; 	/* FBA device chars	*/
	} ch;
} __attribute__ ((packed));

/*------------------------------------------------------*/
/* Virtual/Real Device Block...        			*/
/*------------------------------------------------------*/
struct vrdcblok {
	uint16_t vrdcdvno;	/* Device number	*/
	uint16_t vrdclen;	/* Length of block	*/
	uint8_t  vrdcvcla;	/* Virtual Device class	*/
#define DC_CONS		0x80	/* Console type device  */
#define DC_GRAF 	0x40	/* 3270 type device	*/
#define DC_URIN 	0x20	/* Unit Record input    */
#define DC_UROT 	0x10	/* Unit Record output   */
#define DC_TAPE		0x08	/* Tape type device	*/
#define DC_DASD		0x04	/* ECKD Disk type device*/
#define DC_SPEC 	0x02	/* Special type device  */
#define DC_FBAD 	0x01	/* FBA disk type device */

	uint8_t	 vrdcvtyp;	/* Virtual Device type	*/
/*
 * Device types defined in DC_CONS
 */
#define DT_3215		0x00	/* 3215 console		*/
/*
 * Device types defined in DC_GRAF
 */
#define DT_5080 	0xc0	/* 5080 terminal	*/
#define DT_2250 	0x80	/* 2250 terminal	*/
#define DT_3277 	0x04	/* 3277 terminal	*/
#define DT_3278		0x01	/* 3278 terminal	*/
#define DT_3279		DT_3278	/* 3279 terminal	*/
/*
 * Device types defined in DC_URIN
 */
#define DT_2520 	0x90	/* 1442 card reader	*/
#define DT_1442 	0x88	/* 1442 card reader	*/
#define DT_3505 	0x84	/* 3505 card reader	*/
#define DT_2540 	0x82	/* 2540 card reader	*/
#define DT_2501 	0x81	/* 2501 card reader	*/
/*
 * Device types defined in DC_UROT
 */
#define DT_3525 	0x84	/* 3525 card punch	*/
#define DT_1403 	0x41	/* 1403 printer   	*/
#define DT_3211 	0x42	/* 3211 printer   	*/
#define DT_3203 	0x43	/* 3203 printer   	*/
#define DT_VAFP		0x48	/* VAFP printer 	*/
#define DT_AFP1		0x4e	/* 3820 printer 	*/
#define DT_AFP2		0x4f	/* 3820 printer 	*/
#define DT_AFP3		0x45	/* 3800 printer 	*/
#define DT_AFP4		0x49	/* 3800-03 printer 	*/
#define DT_AFP5		0x4d	/* 3800-08 printer 	*/
#define DT_3262 	0x47	/* 3262 printer		*/
#define DT_4245 	0x4a	/* 4245 printer		*/
#define DT_4248 	0x4b	/* 4248 printer		*/
/*
 * Device types defined in DC_TAPE
 */
#define DT_NTAP 	0x90	/* New tape product	*/
#define DT_3590 	0x83	/* 3590 tape		*/
#define DT_3422 	0x82	/* 3422 tape		*/
#define DT_3490 	0x81	/* 3490 tape		*/
#define DT_2401 	0x80	/* 2401 tape		*/
#define DT_9348 	0x44	/* 9348 tape		*/
#define DT_3424 	0x42	/* 3424 tape		*/
#define DT_3420 	0x10	/* 3420 tape		*/
#define DT_3410 	0x08	/* 3480 tape		*/
#define DT_3411 	DT_3410	/* 3480 tape		*/
#define DT_8890 	0x04	/* 8809 tape		*/
#define DT_3430 	0x02	/* 3430 tape		*/
#define DT_3480 	0x01	/* 3480 tape		*/
/*
 * Device types defined in DC_DASD
 */
#define DT_3390 	0x82   	/* 3390 disk		*/
#define DT_9345 	0x81   	/* 9345 disk		*/
#define DT_2311 	0x80	/* 2311 disk		*/
#define DT_2301 	DT_2311	/* 2301 disk		*/
#define DT_2303 	DT_2311	/* 2303 disk		*/
#define DT_2321 	DT_2311	/* 2321 disk		*/
#define DT_2314 	0x40	/* 2314 disk		*/
#define DT_2319 	DT_2314	/* 2319 disk		*/
#define DT_3380 	0x20   	/* 3380 disk		*/
#define DT_3330 	0x10   	/* 3330 disk		*/
#define DT_3333 	DT_3330	/* 3330 disk		*/
#define DT_3375 	0x04   	/* 3375 disk		*/
#define DT_2305 	0x02   	/* 2305 disk		*/
#define DT_3340 	0x01   	/* 3340 disk		*/
/*
 * Device types defined in DC_SPEC
 */
#define DT_CTCA		0x80	/* CTCA/3088 device	*/
#define DT_37XX		0x40	/* 3704/3705/3725 	*/
#define DT_OSAD		0x20	/* OSA device		*/
/*
 * Device types defined in DC_FBAD
 */
#define DT_9336		0x40	/* 9336 fba disk	*/
#define DT_0671		0x20	/* 0671 fba disk	*/
#define DT_9313		0x10	/* 9313 fba disk	*/
#define DT_9332		0x08	/* 9332 fba disk	*/
#define DT_9335		0x04	/* 9335 fba disk	*/
#define DT_3370		0x02	/* 3370 fba disk	*/
#define DT_3310		0x01	/* 3310 fba disk	*/

	uint8_t  vrdcvsta;	/* Virtual Device status*/
#define VRDSTBSY	0x20	/* Device is busy	*/
#define VRDSTNRY	0x04	/* Device is not ready	*/
#define VRDSTDED	0x01	/* Device is dedicated	*/
	uint8_t  vrdcvfla;	/* Virtual Device flags	*/
#define VRDFLDRO	0x80	/* DASD - read only	*/
#define VRDFLLEN	0x80	/* 270x - line enabled	*/
#define VRDFLTSP	0x40	/* DASD - stor alloc'd	*/
#define VRDFLLCO	0x40	/* 270x - line connected*/
#define	VRDFL1ST	0x10	/* Proc'ing 1st CCW	*/
#define VRDFLDRR	0x02	/* DASD - can res/rel	*/
#define VRDFLMID	0x01	/* MIDAW supported	*/
	uint8_t  vrdcrcca;	/* Real Device class	*/
	uint8_t  vrdccrty;	/* Real Device type	*/
	uint8_t  vrdccrmd;	/* Real Device model 	*/
	uint8_t  vrdccrft;	/* Real Device features */
	uint8_t  vrdcundv;	/* Underlying dev code	*/
#define VRDCTNAT	0x00	/* Native non-emulated  */
#define	VRDCT120	0x01	/* 3590/3592(128)->3490E*/
#define VRDCTVTS	0x02	/* 3490E within 3494	*/
#define VRDCT121	0x03	/* 3590/3591(128)->3490E*/
#define VRDCT255	0x09	/* 3590/3592(256)->3590 */
#define VRDCT254	0x0a	/* 3590/3592(256)->3490E*/
#define VRDCT384	0x0b	/* 3590/3592(384)->3590 */
#define VRDCT383	0x0c	/* 3590/3592(384)->3490E*/
#define VRDCT512	0x10	/* 3590/3592(512)->3590 */
#define VRDCT511	0x11	/* 3590/3592(512)->3490E*/
#define VRDCTUNK	0xff	/* Unknown		*/
	uint8_t  vrdcrdaf;	/* Additional features	*/
#define VRDCFCDS	0x80	/* Dataset level flash	*/
#define VRDCFCFV	0x20	/* Full volume flash  	*/
	struct rdvchar	dvc;	/* Device characteristic*/
} __attribute__ ((packed));

/*------------------------------------------------------*/
/* Channel Report Word				        */
/*------------------------------------------------------*/
typedef struct _crw {
	uint_t	unused1	: 1;		/* Unused		*/
	uint_t	sol	: 1;		/* Solicited		*/
	uint_t	over	: 1;		/* Overflow		*/
	uint_t	chain	: 1;		/* Chaining		*/
	uint_t	rsc	: 4;		/* Reporting-Source code*/
	uint_t	anc	: 1;		/* Ancillary report	*/
	uint_t	unused2	: 1;		/* Unused		*/
	uint_t	erc	: 6;		/* Error-Recovery code	*/
	uint_t	rsid	: 16;		/* Reporting-Source ID	*/
} __attribute__ ((packed,aligned(8))) crw;

/*
 * Definitions for Reporting-Source code
 */
#define CRW_RSC_MON		0x02	/* Monitoring facility	*/
#define CRW_RSC_SCHN		0x03	/* Subchannel		*/
#define CRW_RSC_PATH		0x04	/* Channel path		*/
#define CRW_RSC_CONF		0x09	/* Config-alert fac	*/
#define CRW_RSC_SUBS		0x0b	/* Channel subsystem	*/

/*
 * Definitions for Error-Recovery code
 */
#define CRW_ERC_PEND		0x00	/* Event info pending	*/
#define CRW_ERC_AVAIL		0x01	/* Available		*/
#define CRW_ERC_INIT		0x02	/* Initialized		*/
#define CRW_ERC_TEMP		0x03	/* Temporary error	*/
#define CRW_ERC_INSTI		0x04	/* Installed parm initd	*/
#define CRW_ERC_TERM		0x05	/* Terminal		*/
#define CRW_ERC_PERMN		0x06	/* Perm Error not initd	*/
#define CRW_ERC_PERMI		0x07	/* Perm Error initd	*/
#define CRW_ERC_INSTM		0x08	/* Installed parm modd	*/

#endif
