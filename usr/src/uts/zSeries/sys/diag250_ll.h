/*------------------------------------------------------------------*/
/* 								    */
/* Name        - diag250.h                                          */
/* 								    */
/* Function    - DIAG 250 disk driver headers                       */
/* 								    */
/* Name	       - Adam Thornton					    */
/*               Leland Lucius                                      */
/* 								    */
/* Date        - July, 2007  					    */
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

#ifndef __DIAG250H__
#define __DIAG250H__

#ifdef DEBUG
# ifndef DASD_DEBUG
#  define DASD_DEBUG 1
# endif
#endif

/*
 * Number of blkents that will fill in a 4k page.  We limit it to a
 * 4k page to remove the need to allocate contiguous pages.
 */
#define D250_BLKCNT (PAGESIZE / sizeof(blkent_t))

/*
 * Default number of I/O requests to allocate.  Can be overridden
 * via diag250.conf setting.
 */
#define D250_QLIMIT 16

/*
 * Default number of bounce buffers to allocate.  Can be overridden
 * via diag250.conf setting.
 */
#define D250_BBLIMIT 16

typedef struct diag250_node
{
	struct diag250_node *next;
} diag250_node_t;

typedef struct diag250_lifo
{
	/* DO NOT CHANGE THE ORDER */
	diag250_node_t	*head;
	uint64_t	aba;
	ksema_t		sema;
}  __attribute__ ((packed,aligned(16))) diag250_lifo_t;
	
/*
 * 
 */
typedef struct diag250_dev diag250_dev_t;
typedef struct diag250_io
{
	diag250_node_t link;	// MUST BE FIRST!!!
	diag250_dev_t *dp;
	struct buf *bufs[D250_BLKCNT];
	blkent_t *bents;
	bioplrw_t rwpl;
	blkcnt_t bcnt;
	kmutex_t lock;
	kcondvar_t wait;
	boolean_t done;
	boolean_t sync;
	int cc;
	int rc;
	int sc;
} diag250_io_t;

typedef struct diag250_bbuf
{
	diag250_node_t link;	// MUST BE FIRST!!!
	caddr_t buf;
} diag250_bbuf_t;

struct diag250_dev
{
	dev_info_t *devi;	// DIP
	ccw_device *cd;		// CCW device 
	ddi_intr_handle_t *ih;  // Interrupt handle
	uint16_t devno;		// Device number
	uint16_t flags;		// Status flags
#define	DIDMINOR 0x0080		// Minor node has been allocated
#define	DIDMUTEX 0x0040		// Mutex allocated
#define DIDCV	 0x0020		// Conditional variable allocated
#define DIDBUSY	 0x0010		// I/O Operation is active
#define DIDINIT	 0x0008		// I/O environment initialized
#define DIDINTR  0x0004		// Interrupt allocated
#define DIDENAB  0x0002		// Interrupt enabled
#define DIDADDH  0x0001		// Interrupt added
	int instance;		// Instance of this minor 
	int usemdc;		// Use minidisk cache or not
	int qlimit;		// # of I/O requests to allocate
	int bblimit;		// # of bounce buffers to allocate
	int blksize;		// Block size
	int blkmask;		// Block size - 1
	int lpp;		// Logical blocks (DEV_BSIZE) per physical
	int lppmask;		// Logical blocks - 1
	int64_t offset;	        // Minidisk block Offset
	int64_t startblock;	// Start block
	int64_t endblock;	// End block
	boolean_t rdonly;	// Readonly = B_TRUE
	kmutex_t dotex;		// Protects I/O processing

	/* Wait queue */
	struct buf *whead;	// Head of wait queue
	struct buf **wtail;	// Tail of wait queue

	/* Active queue */
	struct buf *ahead;	// Head of active queue

	/* I/O task */
	ddi_taskq_t *tq;	// Task queue
	boolean_t taskstop;	// B_TRUE when task should exit
	kmutex_t waitex;	// Acquire to protect cv
	kcondvar_t waitcv;	// Posted when new I/O arrives
	boolean_t wakeup;	// B_TRUE to process more I/Os

	diag250_lifo_t freeq;	// Free I/O queue

	diag250_lifo_t bbufq;	// Bounce buffer queue

	diag250_io_t syncio;	// Synchronous I/O

	diag250_io_t *curio;	// I/O currently being built
	
	kmutex_t statex;	// acquire to protect iostat
	kstat_t *iostat;	// I/O statistics

	struct dk_cinfo cinfo;	// Fake controller info
	struct dk_geom geom;	// Fake geometry
	struct vtoc vtoc;	// Fake vtoc
};

extern int diag250_debug;
#define D_D250MISC	0x00000001		// Trace miscellaneous operations
#define D_D250HIIO	0x00000002		// Trace high-level I/O operations
#define D_D250GEOM	0x00000004		// Trace geometry related calls
#define D_D250READ	0x00000008		// Trace low-level read I/O operations
#define D_D250WRIT	0x00000010		// Trace low-level write I/O operations
#define D_D250DIAG	0x00000020		// Trace diag250 operations
#define D_D250FLSH	0x00000040		// Trace flush requests
#define D_D250DATA	0x00000080		// Trace data 
#define D_D250SWRT	0x00000100		// Trace synchronous writes
#define D_D250SRED	0x00000200		// Trace synchronous reads

extern int diag250_ll_initio(diag250_dev_t *dp);
extern int diag250_ll_termio(diag250_dev_t *dp);
extern void diag250_ll_iotask(void *args);
extern uint_t diag250_ll_intr(caddr_t arg1, caddr_t arg2);
extern diag250_node_t *diag250_ll_pop(diag250_lifo_t *lifo);
extern void diag250_ll_push(diag250_lifo_t *lifo, diag250_node_t *node);

#endif /* __DIAG250H__ */
