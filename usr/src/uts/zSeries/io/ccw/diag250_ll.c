/*------------------------------------------------------------------*/
/* 								    */
/* Name        - diag250_ll.c                                       */
/* 								    */
/* Function    - DIAG 250 low-level disk driver.                    */
/* 								    */
/* Name	       - Adam Thornton					    */
/*               Leland Lucius                                      */
/* 								    */
/* Date        - September, 2007 				    */
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

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/cmn_err.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <sys/queue.h>
#include <sys/blockio.h>
#include <sys/ccw.h>
#include <sys/diag250_ll.h>
#include <sys/exts390x.h>
#include <sys/devinit.h>
#include <sys/machs390x.h>
#include <sys/archsystm.h>
#include <sys/intr.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern caddr_t hat_kpm_pfn2va(pfn_t pfn);

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static __inline__ int diag250(void *biopl, int opcode, int *rc);

static boolean_t diag250_ll_is_aligned(diag250_dev_t *dp, struct buf *bp);
static boolean_t diag250_ll_add_aligned(diag250_dev_t *dp, struct buf *bp);
static boolean_t diag250_ll_add_unaligned(diag250_dev_t *dp, struct buf *bp);
static boolean_t diag250_ll_sync_read(diag250_dev_t *dp, daddr_t blk, void *addr);
static boolean_t diag250_ll_sync_write(diag250_dev_t *dp, daddr_t blk, void *addr);
static boolean_t diag250_ll_add_block(diag250_dev_t *dp, struct buf *bp, daddr_t blk, void *addr, int rw);
static boolean_t diag250_ll_flushio(diag250_dev_t *dp);
static boolean_t diag250_ll_startio(diag250_io_t *io);
static void diag250_ll_checkio(diag250_io_t *io);

static void diag250_showdev(char *,diag250_dev_t *);
static void diag250_showio(char *,bioplrw_t *,blkent_t *);
static void diag250_showbuf(char *,struct buf *);
static void diag250_showblkent(char *,blkent_t *);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

char _depends_on[] = "ccwnex";

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250                                           */
/*                                                                  */
/* Function	- Perform the actual DIAG 250                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
static __inline__ int
diag250(void *biopl, int opcode, int *rc)
{
	uint64_t cc;
	
	__asm__("    lgfr  0,%2\n"
		"    diag  0,%3,0x250\n"
		"    lgfr  %0,1\n"
		"    lghi  %1,0\n"
		"    ipm   %1\n"
		"    srlg  %1,%1,28"
		: "=r" (*rc), "=r" (cc) 
		: "r" (va_to_pa(biopl)), "r" (opcode)
		: "0", "1", "cc");
	
#if DASD_DEBUG
	if (diag250_debug & (D_D250WRIT | D_D250READ))
	{
		bioplhd_t *hdr;
		bioplrw_t *bpl;
		blkent_t  *bio;
		int i;
		
		hdr = (bioplhd_t *) biopl;
		bpl = (bioplrw_t *) biopl;
if (hdr->biodevn == 0x201) {
		bio = (blkent_t *) bpl->bioladdr;
		if (((diag250_debug & D_D250WRIT) && (bio[0].belrqtyp == BELWRITE)) ||
		    ((diag250_debug & D_D250READ) && (bio[0].belrqtyp != BELWRITE))) {
msgnoh("DIAG250 request - op: %d dev: %x cc: %d rc: %d\n",
				    opcode, hdr->biodevn, cc, *rc);
			if (diag250_debug & D_D250DATA) {
				for (i = 0; i < bpl->biolentn; i++) {
msgnoh("     op: %s blk: %06d buf: %lx\n",
						    (bio[i].belrqtyp == BELWRITE ? "write" : "read "),
						    bio[i].belbknum, bio[i].belbufad);
				}	
			}
		}
}
	}	
#endif
 
	return(cc);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ll_initio                                 */
/*                                                                  */
/* Function	- Initializes a device to allow I/O via DIAG250.    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
int
diag250_ll_initio(diag250_dev_t *dp)
{
	bioplin_t biopl;
	int rc;
	int cc;

	/* No need to reinit ... this really shouldn't happen though */
	if (dp->flags & DIDINIT) {
		return (DDI_SUCCESS);
	}

	/* Block */
	mutex_enter(&dp->dotex);

	/* Show that we're busy */
	dp->flags |= DIDBUSY;

	/* Build init parameter list */
	bzero(&biopl, sizeof(biopl));
	biopl.bioplhd.biodevn = dp->devno;
	biopl.bioplhd.biomode = BIOZAR;
	biopl.bioblksz = dp->blksize;
	biopl.biooffst = dp->offset;

	/* Initialize environment */
	cc = diag250(&biopl, BIO_OP_IN, &rc);

	/* Check result */
	switch (rc) {
	case 4:
		/* Mark disk readonly */
		dp->rdonly = B_TRUE;

		/* FALLTHRU */
	case 0:
		/* Capture boundaries */
		dp->startblock = biopl.biostart;
		dp->endblock   = biopl.bioend;
		dp->flags |= DIDINIT;
		rc = DDI_SUCCESS;
		break;
	default:
		/* Init failed */
		rc = DDI_FAILURE;
	}
	
#if DASD_DEBUG
	if (diag250_debug & D_D250MISC)
		diag250_showdev("DIAG250 init",dp);
#endif	

	/* No longer busy */
	dp->flags &= ~DIDBUSY;

	/* Unblock */
	mutex_exit(&dp->dotex);

	return (rc);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ll_remove                                 */
/*                                                                  */
/* Function	- Removes block device from diag250 eligibility     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
diag250_ll_termio(diag250_dev_t *dp)
{
        bioplrm_t biopl;
        int rc;
	int cc;

	/* Shouldn't happen, but allow it anyway */
	if (!(dp->flags & DIDINIT)) {
		return (DDI_SUCCESS);
	}

	/* Block */
	mutex_enter(&dp->dotex);

	/* Show that we're busy */
	dp->flags |= DIDBUSY;

	/* Build removal parameter list */
	bzero(&biopl, sizeof(biopl));
	biopl.bioplhd.biodevn = dp->devno;

	/* Remove environment */
	cc = diag250(&biopl, BIO_OP_RM, &rc);
	
	/* Check result */
	if ((rc) || (cc)) {
		/* Removal failed ... leave initialized */
		rc = DDI_FAILURE;
	}
	else {
		/* No longer initialized */
		dp->flags &= ~DIDINIT;
		rc = DDI_SUCCESS;
	}

	/* No longer busy */
	dp->flags &= ~DIDBUSY;

	/* Unblock */
	mutex_exit(&dp->dotex);

	return (rc);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ll_iotask                                 */
/*                                                                  */
/* Function	- Marshals parameters to do io                      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
void
diag250_ll_iotask(void *args)
{
	diag250_dev_t *dp = (diag250_dev_t *) args;
	struct buf *bp;

	/* Process until we're told to quit */
	while (!dp->taskstop) {

		/* Wait for more work */
		mutex_enter(&dp->waitex);
		while (dp->wakeup == B_FALSE) {
			cv_wait(&dp->waitcv, &dp->waitex);
		}
		dp->wakeup = B_FALSE;

		/* Move bufs to active queue */
		dp->ahead = dp->whead;

		/* Empty wait queue */
		dp->whead = NULL;
		dp->wtail = &dp->whead;
	
		/* Release hold to allow more bufs to be queued */
		mutex_exit(&dp->waitex);

		/* Now process each buf */
		while (dp->ahead != NULL) {
	
			/* Maintain stats */
			mutex_enter(&dp->statex);
			kstat_waitq_to_runq(KSTAT_IO_PTR(dp->iostat));
			mutex_exit(&dp->statex);

			/* Get first entry */
			bp = dp->ahead;
	
			/* Remove from queue */
			dp->ahead = bp->av_forw;
	
			/* Speed things up a bit if the data is aligned */
			if (diag250_ll_is_aligned(dp, bp)) {
				diag250_ll_add_aligned(dp, bp);
			}
			else {
				diag250_ll_add_unaligned(dp, bp);
			}

			/* Flush queued blocks */
			diag250_ll_flushio(dp);
		}
	}

	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ll_intr.                                  */
/*                                                                  */
/* Function	- External interrupt handler for DIAG 250.          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
uint_t
diag250_ll_intr(caddr_t arg1, caddr_t arg2)
{
	intparms *ip = (intparms *)arg2;
	diag250_io_t *io;
	int rc;

	/* We want Service Signals only */
	if (ip->u.ext.intcode != EXT_SSIG) {
		return (DDI_INTR_UNCLAIMED);
	}

	/* And only 64-bit block I/O ones */
	if (ip->u.ext.subcode >> 8 != SSG_BIOZ) {
		return (DDI_INTR_UNCLAIMED);
	}

#if 0
prom_printf("2 intcode %04x subcode %02x rc %02x parm %016lx\n",
	ip->u.ext.intcode,
	ip->u.ext.subcode >> 8,
	ip->u.ext.subcode & 0xff,
	*((diag250_dev_t **)__LC_BLKIO_PARM));
#endif

	/* Get I/O request pointer and save status */
	io = (diag250_io_t *)ip->u.ext.extparm;
	io->sc = ip->u.ext.subcode & 0xff;

	/* Was this a synchronous request? */
	if (io->sync) {
		/* Signal waiters that the request is complete */
		mutex_enter(&io->lock);
		io->done = B_TRUE;
		cv_broadcast(&io->wait);
		mutex_exit(&io->lock);
	}
	else {
		/* Check status of request */
		diag250_ll_checkio(io);
	}

	return (DDI_INTR_CLAIMED);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ll_is_aligned.                            */
/*                                                                  */
/* Function	- Determines if buf is aligned to and multiple of   */
/*                the disk block size                               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
static boolean_t
diag250_ll_is_aligned(diag250_dev_t *dp, struct buf *bp)
{
	caddr_t addr;
	daddr_t offset;
	size_t count;

	/* Data must be at a block boundary in memory */
	addr = (caddr_t)bp->b_un.b_addr;
	if (addr && (((uintptr_t)addr & dp->blkmask) != 0)) {
		return B_FALSE;
	}

	/* And, must be at a block boundary on disk */
	offset = bp->b_lblkno * DEV_BSIZE;
	if (offset & dp->blkmask) {
		return B_FALSE;
	}

	/* And, it must be a multiple of the block size */
	count = bp->b_bcount;
	if (count & dp->blkmask) {
		return B_FALSE;
	}

	return B_TRUE;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ll_add_aligned.                           */
/*                                                                  */
/* Function	- Break up buf and add blocks to I/O stream         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
static boolean_t
diag250_ll_add_aligned(diag250_dev_t *dp, struct buf *bp)
{
	page_t *pp;
	page_t **pplist;
	caddr_t addr;
	size_t soff;
	size_t eoff;
	size_t offset;
	daddr_t blk;
	int bsize;
	int rc;

	/* Cache */
	offset = bp->b_lblkno * DEV_BSIZE;
	bsize = dp->blksize;

	/* Calc start and end offsets */
	soff = offset & (~dp->blkmask);
	eoff = (offset + bp->b_bcount + dp->blkmask) & (~dp->blkmask);

	/* Remember start and end blocks */
	bp->av_forw = (void *) (soff / bsize);
	bp->av_back = (void *) (eoff / bsize) - 1;

	/* Initialize work fields */
	bp->b_private = NULL;
	bp->b_resid = bp->b_bcount;

	/* Get page ptrs */
	if (bp->b_flags & B_PAGEIO) {
		pp = bp->b_pages;
		pplist = NULL;
	} else if (bp->b_flags & B_SHADOW) {
		pp = NULL;
		pplist = bp->b_shadow;
	} else {
		pp = NULL;
		pplist = NULL;
	}

	/* Add each block in this request */
	addr = (caddr_t) bp->b_un.b_addr;
	blk = (daddr_t) bp->av_forw;
	while (soff < eoff) {

		/*
		 * Get address of next block if the first time through
		 * or at a page boundary.
		 */
		if (addr == NULL || (((uintptr_t)addr & PAGEOFFSET) == 0)) {
			if (pp) {
				addr = hat_kpm_page2va(pp, 0);
				pp = pp->p_next;
			} else if (pplist != NULL) {
				addr = hat_kpm_page2va(*pplist, 0);
				pplist++;
			}
		}

		/* Add the block */
		rc = diag250_ll_add_block(dp,
					  bp,
					  blk,
					  addr,
					  bp->b_flags & B_READ ? BELREAD : BELWRITE);
		if (rc != 0) {
			prom_printf("badrc %d\n", rc);
			return (B_FALSE);
		}

		/* Bump to next block */
		addr += bsize;
		soff += bsize;
		blk  += 1;
	}

	return (B_TRUE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ll_add_unaligned.                         */
/*                                                                  */
/* Function	- Break up buf and add blocks to I/O stream for the */
/*		  unaligned case.             		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
static boolean_t
diag250_ll_add_unaligned(diag250_dev_t *dp, struct buf *bp)
{
	diag250_bbuf_t *bbuf;
	size_t soff;
	size_t eoff;
	size_t offset;
	size_t count;
	caddr_t addr;
	daddr_t blk;
	size_t len;
	int bsize;
	int rw;
	int rc;

	/* Map user pages into kernel space */
	if (bp_mapin_common(bp, VM_NOSLEEP) == NULL) {
		bioerror(bp, ENOMEM);
		biodone(bp);
		return (B_FALSE);
	}

	/* Cache */
	offset = bp->b_lblkno * DEV_BSIZE;
	count = bp->b_bcount;
	rw = bp->b_flags & B_READ ? BELREAD : BELWRITE;
	bsize = dp->blksize;

	/* Calc start and end offsets */
	soff = offset & (~dp->blkmask);
	eoff = (offset + count + dp->blkmask) & (~dp->blkmask);
	len = eoff - soff;

	/* Should never happen, but... */
	ASSERT((count < 4096 * D250_BLKCNT));

	/* Remember start and end blocks */
	bp->av_forw = (void *) (soff / bsize);
	bp->av_back = (void *) (eoff / bsize) - 1;

	/* Get bounce buffer ... may wait */
	bbuf = (diag250_bbuf_t *) diag250_ll_pop(&dp->bbufq);
	ASSERT((bbuf != NULL));
	addr = bbuf->buf;

	/* Initialize work fields */
	bp->b_resid = count;
	bp->b_private = bbuf;

	/* For writes, copy input to bounce buffer*/
	if (rw == BELWRITE) {

		/* Need to preload partial blocks */
		if (dp->blksize > DEV_BSIZE) {

			/* Unaligned block at beginning? */
			if (soff < offset) {

				/* Synchronously read the block */
				rc = diag250_ll_sync_read(dp,
							  (daddr_t) bp->av_forw,
							  addr);
				if (rc != 0) {
					panic("getunaligned failed\n");
				}
			}
	
			/* Partial block at end? */
			if ((eoff > offset + count) &&
			   (soff == offset || bp->av_back > bp->av_forw)) {

				/* Synchronously read the block */
				rc = diag250_ll_sync_read(dp,
							  (daddr_t) bp->av_back,
							  &addr[len - bsize]);
				if (rc != 0) {
					panic("getunaligned failed\n");
				}
			}
		}

		/* Copy user data to bounce buffer */
		bcopy(bp->b_un.b_addr,
		      &addr[offset - soff],
		      count);
	}

	/* Add blkents for full blocks only */
	blk = (daddr_t) bp->av_forw;
	while (soff < eoff) {
		rc = diag250_ll_add_block(dp,
					  bp,
					  blk,
					  addr,
					  rw);
		if (rc != 0) {
			prom_printf("badrc %d\n", rc);
			return (B_FALSE);
		}

		/* Adjust buffer start and length */
		addr += bsize;
		soff += bsize;
		blk  += 1;
	}

	return (B_TRUE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ll_sync_read.                             */
/*                                                                  */
/* Function	- Reads one block synchronously                     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
static boolean_t
diag250_ll_sync_read(diag250_dev_t *dp, daddr_t blk, void *addr)
{
	diag250_io_t *io = &dp->syncio;
	blkent_t *be = io->bents;
	int cc;

	/*
	 * If this routine is used outside of the iotask, then
	 * a mutex needs to be used to protect syncio.
	 */

#if DASD_DEBUG
	if (diag250_debug & D_D250SRED) {
		bioplhd_t *hdr;
		
		hdr = (bioplhd_t *) &io->rwpl;
		prom_printf("DIAG250 sync_read - dev: %x buf: %p\n",
			    hdr->biodevn,addr);
	}
#endif

	/*
	 * Only need to set only block # and data address.  All
	 * other fields have been initialized at allocation.
	 */
	be->belbknum = blk;
	be->belbufad = va_to_pa(addr);

	/* Start (and wait for) the I/O */
	cc = diag250_ll_startio(io);

	return (cc);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ll_sync_write.                            */
/*                                                                  */
/* Function	- Writes one block synchronously                    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
static boolean_t
diag250_ll_sync_write(diag250_dev_t *dp, daddr_t blk, void *addr)
{
	diag250_io_t *io = &dp->syncio;
	blkent_t *be = io->bents;
	int cc;

	/*
	 * If this routine is used outside of the iotask, then
	 * a mutex needs to be used to protect syncio.
	 */

#if DASD_DEBUG
	if (diag250_debug & D_D250SWRT) {
		bioplhd_t *hdr;
		
		hdr = (bioplhd_t *) &io->rwpl;
		prom_printf("DIAG250 sync_write - dev: %x buf: %p\n",
			    hdr->biodevn,addr);
	}
#endif

		/* Start the request */
	/*
	 * Only need to set only block # and data address.  All
	 * other fields have been initialized at allocation.
	 */
	be->belbknum = blk;
	be->belbufad = va_to_pa(addr);

	/* Start (and wait for) the I/O */
	cc = diag250_ll_startio(io);

	return (cc);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ll_add_block.                             */
/*                                                                  */
/* Function	- Adds a block to current or new I/O request.  If   */
/*                the request fills, it will be flushed.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
static boolean_t
diag250_ll_add_block(diag250_dev_t *dp, struct buf *bp, daddr_t blk, void *addr, int rw)
{
	diag250_io_t *io;
	blkent_t *be;
	int ndx;
	int rc = 0;

	/* Get current or new I/O request */
	io = dp->curio;
	if (io == NULL) {
		io = (diag250_io_t *) diag250_ll_pop(&dp->freeq);
		ASSERT((io != NULL));
		dp->curio = io;
	}

	/* Get current index and bump for next time round */
	ndx = io->bcnt++;

	/* Remember buf pointer */
	io->bufs[ndx] = bp;

	/* Add blkent */
	be = &io->bents[ndx];
	be->belrqtyp = rw;
	be->belbknum = blk;
	be->belbufad = va_to_pa(addr);

	/* Flush the I/O if the block entries have filled */
	if (io->bcnt == D250_BLKCNT) {
		rc = diag250_ll_flushio(dp);
	}

	return (rc);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ll_flushio.                               */
/*                                                                  */
/* Function	- Starts a pending I/O request, if any              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
static boolean_t
diag250_ll_flushio(diag250_dev_t *dp)
{
	diag250_io_t *io;
	int rc = B_FALSE;

	/* Need to flush? */
	io = dp->curio;

	if (io != NULL) {
		/* No longer pending */
		dp->curio = NULL;

#if DASD_DEBUG
		if (diag250_debug & D_D250FLSH) {
			bioplhd_t *hdr;
			
			hdr = (bioplhd_t *) &io->rwpl;
			prom_printf("DIAG250 flush - dev: %x\n",
				    hdr->biodevn);
		}
#endif

		/* Start the request */
		rc = diag250_ll_startio(io);

		/*
		 * If start failed or if the request was satisfied
		 * from minidisk cache, call checkio() directly
		 * since the interrupt routine will not fire.
		 */
		if (rc != 0 || (io->rc == 0)) {
			diag250_ll_checkio(io);
		}
	}

	return (rc != 0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ll_startio.                               */
/*                                                                  */
/* Function	- Starts the I/O request and, possibly, wait for    */
/*                completion if it's synchronous.                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
static boolean_t
diag250_ll_startio(diag250_io_t *io)
{
	boolean_t sync = io->sync;

	/* Start the I/O.
	 *
	 * If the I/O completes before returning from the diag250() function,
	 * the interrupt will have fired (if the request couldn't be satisfied
	 * from minidisk cache) and the request will have been put back on the
	 * free queue.
	 *
	 * It's still safe to store the CC and RC within the I/O request in
	 * this case since the request will not be reused until later.
	 */
	io->rwpl.biolentn = io->bcnt;
	io->sc = 0;
	io->cc = diag250((void *) &io->rwpl, BIO_OP_RW, &io->rc);
	if (io->cc == 0) {
		if (io->rc == 8) {
			if (sync) {
				mutex_enter(&io->lock);
				while (!io->done) {
					cv_wait(&io->wait, &io->lock);
				}
				io->done = B_FALSE;
				mutex_exit(&io->lock);

				if (io->sc != 0) {
					prom_printf("io->rc = %d\n", io->sc);
					diag250_showio("BAD #1", &io->rwpl, io->bents);
					io->cc = 3;
				}
			}
		}
		else if (io->rc != 0) {
			prom_printf("di250 cc %d rc %d\n", io->cc, io->rc);
			diag250_showio("BAD #2", &io->rwpl, io->bents);
		}
	}
	else {
		prom_printf("di250 cc %d rc %d\n", io->cc, io->rc);
		diag250_showio("BAD #3", &io->rwpl, io->bents);
	}

	return (io->cc != 0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ll_checkio                                */
/*                                                                  */
/* Function	- Check I/O, copy unaligned/partial blocks and      */
/*                return bufs.					    */
/*		                               		 	    */
/*		  Called from task and interrupt state any access   */
/*                to diag250_dev_t should be protected or readonly. */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
static void
diag250_ll_checkio(diag250_io_t *io)
{
	diag250_dev_t *dp = io->dp;
	struct buf *bp;
	blkent_t *be;
	size_t offset;
	size_t count;
	daddr_t sblk;
	daddr_t eblk;
	int ndx;
	int len;

	for (ndx = 0; ndx < io->bcnt; ndx++) {

		bp = io->bufs[ndx];
		be = &io->bents[ndx];

		offset = bp->b_lblkno * DEV_BSIZE;
		sblk = (size_t) (uintptr_t) bp->av_forw;
		eblk = (size_t) (uintptr_t) bp->av_back;

		/* Calc length of this, possibly partial, block */
		len = dp->blksize;
		if (be->belbknum == eblk) {
			len = (offset + bp->b_bcount) - (be->belbknum * dp->blksize);
		}

		if (be->belbknum == sblk) {
			len = len - (offset & dp->blkmask);
		}

		mutex_enter(&dp->statex);

		if (be->belstat == BELOK) {

			bp->b_resid -= len;

			if (be->belrqtyp == BELREAD) {
				KSTAT_IO_PTR(dp->iostat)->reads++;
				KSTAT_IO_PTR(dp->iostat)->nread += len;
			}
			else {
				KSTAT_IO_PTR(dp->iostat)->writes++;
				KSTAT_IO_PTR(dp->iostat)->nwritten += len;
			}
		}
		else {
			if (be->belrqtyp == BELREAD) {
				KSTAT_IO_PTR(dp->iostat)->reads++;
			}
			else {
				KSTAT_IO_PTR(dp->iostat)->writes++;
			}
		}

		mutex_exit(&dp->statex);

		/* We've hit the end */
		if (be->belbknum == eblk) {
			size_t soff = sblk * dp->blksize;
			size_t eoff = (eblk + 1) * dp->blksize;

			/* Done with the bounce buffer */
			if (bp->b_private) {
				diag250_bbuf_t *bbuf = (diag250_bbuf_t *) bp->b_private;

				if (be->belrqtyp == BELREAD) {
					bcopy(&bbuf->buf[offset - soff],
					      bp->b_un.b_addr,
					      bp->b_bcount);
				}

				diag250_ll_push(&dp->bbufq, &bbuf->link);

				bp->b_private = NULL;
			}

			bp->av_forw = NULL;
			bp->av_back = NULL;

			if (bp->b_resid != 0) {
				bioerror(bp, EIO);
			}
			else {
				bioerror(bp, 0);
			}

			biodone(bp);

			mutex_enter(&dp->statex);
			kstat_runq_exit(KSTAT_IO_PTR(dp->iostat));
			mutex_exit(&dp->statex);
		}
	}

	/* Put the IO request back on the free queue */
	io->bcnt = 0;
	diag250_ll_push(&dp->freeq, &io->link);

	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ll_push.                                  */
/*                                                                  */
/* Function	- Return a node to a LIFO stack and increment the   */
/*		  semaphore to wake up any waiters.  The stack is   */
/*		  updated atomically, so no locks are required.     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
diag250_ll_push(diag250_lifo_t *lifo, diag250_node_t *node)
{
	__asm__("	lg	0,0(%0)\n"
		"0:\n"
		"	stg	0,0(%1)\n"
		"	csg	0,%1,0(%0)\n"
		"	jnz	0b\n"
		:
		: "r" (&lifo->head), "r" (node)
		: "0", "cc", "memory");

	sema_v(&lifo->sema);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ll_pop.                                   */
/*                                                                  */
/* Function	- Decrement the semaphore, possibly waiting until a */
/*		  node is placed onto the LIFO stack, and remove    */
/*		  and return the top node of the stack.  The stack  */
/*		  is updated atomically, so no locks are required.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

diag250_node_t *
diag250_ll_pop(diag250_lifo_t *lifo)
{
	diag250_node_t *node;

	do {
		sema_p(&lifo->sema);

		__asm__("	lmg	2,3,0(%1)\n"
			"0:\n"
			"	ltr	2,2\n"
			"	jz	1f\n"
			"	lg	4,0(2)\n"
			"	la	5,1(3)\n"
			"	cdsg	2,4,0(%1)\n"
			"	jnz	0b\n"
			"1:\n"
			"	lgr	%0,2\n"
			: "=r" (node)
			: "r" (&lifo->head)
			: "2", "3", "4", "5", "cc", "memory");
	} while (node == NULL);

	return node;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_showdev.                                  */
/*                                                                  */
/* Function	- Dump instance info.                               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
diag250_showdev(char *msg, diag250_dev_t *dp)
{
	/*  cmn_err(CE_NOTE,*/
	prom_printf(
		"DIAG250:showdev: %s\n  Devno:%x  Blocksize:%d\n  Offset:%ld  Startblock:%ld  Endblock:%ld\n",
		msg,
		dp->devno,
		dp->blksize,
		dp->offset,
		dp->startblock,
		dp->endblock);
	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_showio.                                   */
/*                                                                  */
/* Function	- Dump BIOPL rw info.                               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
diag250_showio(char *msg, bioplrw_t *b, blkent_t *e)
{
	int i=0;

	prom_printf("DIAG250:showio  biopl=0x%p\n", b);
	prom_printf(
		"DIAG250:showio: %s\n  Device: 0x%x   Archmode: 0x%x  Subchannel key:0x%x "
		"IOflag 0x%x\n  BELBLK entries: %d   ALET: %d   Interrupt parm: 0x%lx\n "
		"BELBLK start: 0x%p\n",
		msg,
		b->bioplhd.biodevn,
		b->bioplhd.biomode,
		b->biokey,
		b->bioflag,
		b->biolentn,
		b->biolalet,
		b->bioiparm,
		b->bioladdr);

	showmem("RWPL", b, sizeof(*b));

	for (i=0; i<b->biolentn; i++) {
		diag250_showblkent("First Block Entry",&e[i]);
		showmem("BENT", &e[i], sizeof(*e));
	}
	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_showblk.                                  */
/*                                                                  */
/* Function	- Dump block entry info                             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
diag250_showblkent(char *msg, blkent_t *b) 
{
	prom_printf("DIAG250:showblkent  blkent=0x%p\n",b);
	prom_printf(
		"DIAG250:showblkent: %s\n  stat: %d type: %d blknum: %ld buffer: 0x%p\n",
		msg,
		b->belstat,
		b->belrqtyp,
		b->belbknum,
		b->belbufad);
	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_showbuf.                                  */
/*                                                                  */
/* Function	- Display info about buf(9S) structure.             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void 
diag250_showbuf(char *msg, struct buf *b)
{
	prom_printf(
		"DIAG250 showbuf: %s\n  Flags 0x%lx ; Bytes %ld; blknum %ld\n  "
		"Bufsize %ld ; Bufaddr 0x%p\n  Not-transferred %ld ; biodone() 0x%p error %ld\n",
		msg,
		b->b_flags,
		b->b_bcount,
		b->b_lblkno,
		b->b_bufsize,
		b->b_un.b_addr,
		b->b_resid,
		b->b_iodone,
		b->b_error);

	return;
}

/*========================= End of Function ========================*/
