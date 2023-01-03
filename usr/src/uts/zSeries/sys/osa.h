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
 * Copyright (c) 1998, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef	_OSA_H
#define	_OSA_H

/*
 * Driver declarations for the OSA driver
 */

#ifdef	__cplusplus
extern "C" {
#endif

/* DIAG 2A8 function codes */
#define OSA_OP_QI	0x00000000	/* Query Interface */
#define OSA_OP_EDC	0x01000000	/* Establish Device Connection */
#define OSA_OP_SDR	0x02000000	/* Send Datagram Request */
#define OSA_OP_RDR	0x03000000	/* Receive Datagram Request */
#define OSA_OP_MAR	0x04000000	/* Multicast Address Registration */
#define OSA_OP_NDO	0x05000000	/* Network Device Options */

/* Query Interface parameter list */
typedef struct _qipl {
	uint8_t		macaddr[6];
	uint8_t		rsvd1;
	uint8_t		features;
	uint16_t	numpages;
	uint16_t	adrpages;
	uint16_t	rsvd2;
	uint16_t	maclimit;
	ccw1		ccw;
	uint8_t		rsvd3[40];
} __attribute__ ((packed,aligned(8))) qipl;

/* Establish Device Connection parameter list */
typedef struct _edcpl
{
	uint16_t	numpages;
	uint16_t	index;
	uint16_t	devno;
	uint8_t		rsvd1[26];
	uint64_t	pages[1];
} __attribute__ ((packed,aligned(8))) edcpl;

/* Data Request Block */
typedef struct _drb
{
	uint8_t		flags;
	uint8_t		key : 4;
	uint8_t		rsvd1 : 4;
	uint8_t		rsvd2[4];
	uint16_t	count;
	uint64_t	addr;
} __attribute__ ((packed,aligned(8))) drb;

#define OSA_DRB_MCAST	0x01
#define OSA_DRB_BCAST	0x02
#define OSA_DRB_UCAST	0x04

/* Network parameter list */
typedef struct _netpl
{
	uint8_t		entries;
	uint8_t		index;
	uint8_t		rsvd1[30];
	drb		drbs[1];
} __attribute__ ((packed,aligned(8))) netpl;

/* Multicast MAC Address Registration parameter list */
typedef struct _macpl
{
	uint8_t		function;
	uint8_t		rsvd1[3];
	uint16_t	devno;
	uint8_t		rsvd2[2];
	uint8_t		macaddr[6];
	uint8_t		rsvd3[18];
} __attribute__ ((packed,aligned(8))) macpl;

#define MPL_FUNC_ASSIGN		0x01
#define MPL_FUNC_UNASSIGN	0x02

/* Device Options parameter list */
typedef struct _dopl
{
	uint8_t		function;
	uint8_t		rsvd1[3];
	uint16_t	devno;
	uint8_t		rsvd2[26];
	uint32_t	options;
	uint8_t		rsvd3[28];
} __attribute__ ((packed,aligned(8))) dopl;

#define DPL_FUNC_QUERY		0x00
#define DPL_FUNC_PROMISC_ON	0x01
#define DPL_FUNC_PROMISC_OFF	0x02

/* debug flags */
#define	OSATRACE	0x01
#define	OSAERRS		0x02
#define	OSARECV		0x04
#define	OSADDI		0x08
#define	OSASEND		0x10
#define	OSAINT		0x20
#define	OSAALAN		0x40

/* Misc	*/
#define	OSAHIWAT	65536		/* driver flow control high water */
#define	OSALOWAT	16384		/* driver flow control low water */
#define	OSAMAXPKT	1500		/* maximum media frame size */

/* Device state */
typedef
struct osa_state
{
	dev_info_t		*dip;
	ccw_device		*cd;
	gld_mac_info_t		*macinfo;
	int			devno;
	qipl			qi;
	ccw_device_req		*active;
	kmutex_t		activelock;

	edcpl			*edc;
	int			edclen;
	int			edc64len;
	caddr_t			edc64pages;
	int			edc31len;
	caddr_t			edc31pages;
	int			numpages;

	/* Storage for statistics */
	uint64_t		numintrs;
	uint64_t		norcvbuf;
} OSA_state;

#ifdef	__cplusplus
}
#endif

#endif	/* _OSA_H */
