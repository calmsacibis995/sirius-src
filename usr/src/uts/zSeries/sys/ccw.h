/*------------------------------------------------------------------*/
/*                                                                  */
/* Name        - ccw.h                                              */
/*                                                                  */
/* Function    - CCW bus nexus driver.                              */
/*                                                                  */
/* Name        - Leland Lucius                                      */
/*                                                                  */
/* Date        - September, 2007                                    */
/*                                                                  */
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
/*                   L I C E N S E                                  */
/*------------------------------------------------------------------*/

/*==================================================================*/
/*                                                                  */
/* CDDL HEADER START                                                */
/*                                                                  */
/* The contents of this file are subject to the terms of the        */
/* Common Development and Distribution License                      */
/* (the "License").  You may not use this file except in compliance */
/* with the License.                                                */
/*                                                                  */
/* You can obtain a copy of the license at:                         */
/* - usr/src/OPENSOLARIS.LICENSE, or,                               */
/* - http://www.opensolaris.org/os/licensing.                       */
/* See the License for the specific language governing permissions  */
/* and limitations under the License.                               */
/*                                                                  */
/* When distributing Covered Code, include this CDDL HEADER in each */
/* file and include the License file at usr/src/OPENSOLARIS.LICENSE.*/
/* If applicable, add the following below this CDDL HEADER, with    */
/* the fields enclosed by brackets "[]" replaced with your own      */
/* identifying information:                                         */
/* Portions Copyright [yyyy] [name of copyright owner]              */
/*                                                                  */
/* CDDL HEADER END                                                  */
/*                                                                  */
/* Copyright 2008 Sine Nomine Associates.                           */
/* All rights reserved.                                             */
/* Use is subject to license terms.                                 */
/*                                                                  */
/*==================================================================*/

#include <sys/ios390x.h>

/* Forward reference */
typedef struct ccw_device ccw_device;

/*
 * Device extension used for managing CCW devices.  Pointer is stored
 * in the dev_info structure, so CCW drivers must use ccw_set_private()
 * to store driver specific data.
 */
typedef struct ccw_device_req {
	dev_info_t	*dip;		/* pointer to issuing device */
	ccw_device	*cd;		/* pointer to ccw device */
	void		*user;		/* reserved for users use */
	kmutex_t	lock;		/* acquire to protect cv */
	kcondvar_t	done;		/* posted when request completes */
	boolean_t	busy;		/* B_TRUE if I/O still busy */
	irb		irb;		/* interrupt request block */
	uint_t		id;		/* id32 pointer back to self */
	int		cmdcount;	/* number of ccw1 slots */
	int		cmdnext;	/* index of next ccw slot */
	struct ccw1	*ccws;		/* ccw program */
	caddr_t		*idas;		/* IDA pointers */
} ccw_device_req;

typedef int (*ccw_dev_handler)(ccw_device *dev, ccw_device_req *req);

struct ccw_device {
	uint32_t	schid;		/* subchannel id */
	schib		sib;		/* subchannel information block */
	irb		irb;		/* interrupt request block */
	struct vrdcblok	rdc;		/* device charactistics */
	dev_info_t	*dip;		/* pointer back to device */
	uint32_t	id32;		/* 32-bit ID for this ccw_device */
	void		*devpriv;	/* driver private data */ 
	kmutex_t	intrlock;	/* acquire to single thread handler */
	ccw_dev_handler	handler;	/* pointer to IRQ handler function */
	ccw_device_req	*active;	/* active request */
};

/*
 * Registers device and returns pointer to ccw_device
 */

ccw_device *
ccw_device_register(dev_info_t *dip);

/*
 * Unregisters device and frees ccw_device
 */

void
ccw_device_unregister(ccw_device *cd);

/*
 * Returns pointer to devices private data
 */

void *
ccw_device_get_private(ccw_device *cd);

/*
 * Set private data for device
 */

void
ccw_device_set_private(ccw_device *cd, void *data);

/*
 * Returns pointer to interruption response block
 */
 
irb *
ccw_device_get_irb(ccw_device *cd);

/*
 * Returns pointer to read device characteristics block
 */

struct vrdcblok *
ccw_device_get_rdc(ccw_device *cd);

/*
 * Returns subchannel ID
 */
 
uint32_t
ccw_device_get_schid(ccw_device *cd);

/*
 * Returns pointer to subchannel information block
 */
 
struct schib *
ccw_device_get_schib(ccw_device *cd);

/*
 * Set I/O completion handler
 */

void
ccw_device_set_handler(ccw_device *cd, ccw_dev_handler handler);

/*
 * Enable device for I/O
 */

int
ccw_device_enable(ccw_device *cd);

/*
 * Disable device for I/O
 */

int
ccw_device_disable(ccw_device *cd);

/*
 * Cancel subchannel
 */
 
int
ccw_device_cancel(ccw_device *cd);

/*
 * Clears subchannel
 */
 
int
ccw_device_clear(ccw_device *cd);

/*
 * Halt subchannel
 */
 
int
ccw_device_halt(ccw_device *cd);

/*
 * Modify subchannel
 */
 
int
ccw_device_modify(ccw_device *cd);

/*
 * Resume subchannel
 */
 
int
ccw_device_resume(ccw_device *cd);

/*
 * Starts an I/O
 */
 
int
ccw_device_start(ccw_device_req *req);

/*
 * Store subchannel
 */
 
int
ccw_device_store(ccw_device *cd);

/*
 * Test subchannel
 */
 
int
ccw_device_test(ccw_device *cd);

/*
 * Alloc a new I/O request
 */

ccw_device_req *
ccw_device_alloc_req(ccw_device *cd);

/*
 * Free an I/O request
 */

void
ccw_device_free_req(ccw_device_req *req);

/*
 * Add a new command to list of CCWs within a request
 */
 
int
ccw_cmd_add(ccw_device_req *req,
	    uchar_t op,
	    uchar_t flags,
	    ushort_t count,
	    void *data);

/*
 * Set flags in CCW at given index
 */
 
int
ccw_cmd_set_flags(ccw_device_req *req, int ccw, uchar_t flags);

/*
 * clear flags in CCW at given index
 */
 
int
ccw_cmd_clear_flags(ccw_device_req *req, int ccw, uchar_t flags);

/*
 * Reset CCW associated with a request (for reuse)
 */

void
ccw_cmd_reset(ccw_device_req *req);

/*
 * Allocate virtual, backed by contiguous 31-bit real
 */
 
void *
ccw_alloc31(size_t size, int flags);

/*
 * Free memory allocated by ccw_alloc31()
 */
 
void
ccw_free31(void *mem, size_t size);

/*
 * Allocate virtual, backed by contiguous real memory
 */
 
void *
ccw_alloc64(size_t size, int flags);

/*
 * Free memory allocated by ccw_alloc64()
 */
 
void
ccw_free64(void *mem, size_t size);

/*
 * Debug information
 * Severity levels for printing
 */

#define	DCCW_L0	0	/* print every message */
#define	DCCW_L1	1	/* debug */
#define	DCCW_L2	2	/* minor errors */
#define	DCCW_L3	3	/* major errors */
#define	DCCW_L4	4	/* catastrophic errors */

#define	DCCW ccw_dprintf

int ccw_msglevel;

void
ccw_dprintf(int, const char *, ...) __KPRINTFLIKE(2);

