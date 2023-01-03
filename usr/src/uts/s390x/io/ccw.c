/*------------------------------------------------------------------*/
/*                                                                  */
/* Name        - ccw.c                                              */
/*                                                                  */
/* Function    - CCW common I/O framework.                          */
/*                                                                  */
/* Name        - Leland Lucius                                      */
/*                                                                  */
/* Date        - October, 2007                                      */
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
/*------------------------------------------------------------------*/
/*                 D e f i n e s                                    */
/*------------------------------------------------------------------*/

#define CCW_CMDCOUNT	32	/* Arbitrary */
#define CCW_IDAWCOUNT	17	/* Enough to handle 65K of data */

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/cmn_err.h>
#include <sys/conf.h>
#include <sys/modctl.h>
#include <sys/autoconf.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/kmem.h>
#include <sys/ddidmareq.h>
#include <sys/ddi_impldefs.h>
#include <sys/dma_engine.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/sunndi.h>
#include <sys/id32.h>
#include <vm/seg_kmem.h>
#include <sys/ios390x.h>
#include <sys/machs390x.h>
#include <sys/ccw.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/


/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static void ccw_dump_req(ccw_device_req *req);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_register.                              */
/*                                                                  */
/* Function	- Register a CCW device                             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

ccw_device *
ccw_device_register(dev_info_t *dip)
{
	ccw_device *cd;
	int isc;
	int len;
	int cc;

	DCCW(DCCW_L1, "ccw_device_register(%p)\n", dip);

	cd = kmem_zalloc(sizeof(*cd), KM_NOSLEEP);
	if (cd == NULL) {
		return (NULL);
	}

	cd->schid = ddi_prop_get_int(DDI_DEV_T_ANY,
				     dip,
				     DDI_PROP_DONTPASS,
				     "subchannel-id",
				     -1);
	if (cd->schid == -1) {
		kmem_free(cd, sizeof(*cd));
		return (NULL);
	}

	isc = ddi_prop_get_int(DDI_DEV_T_ANY,
			       dip,
			       DDI_PROP_DONTPASS,
			       "interrupt-subclass",
			       -1);
	if (isc == -1) {
		kmem_free(cd, sizeof(*cd));
		return (NULL);
	}

	len = sizeof(cd->sib);
	cc = ddi_getlongprop_buf(DDI_DEV_T_ANY,
				 dip,
				 0,
				 "initial-state",
				 (caddr_t)&cd->sib,
				 &len);
	if (cc != DDI_PROP_SUCCESS || len != sizeof(cd->sib)) {
		kmem_free(cd, sizeof(*cd));
		return (NULL);
	}

	len = sizeof(cd->rdc);
	cc = ddi_getlongprop_buf(DDI_DEV_T_ANY,
				 dip,
				 0,
				 "device-characteristics",
				 (caddr_t)&cd->rdc,
				 &len);
	if (cc != DDI_PROP_SUCCESS || len != sizeof(cd->rdc)) {
		kmem_free(cd, sizeof(*cd));
		return (NULL);
	}

	mutex_init(&cd->intrlock, NULL, MUTEX_DRIVER, NULL);
	cd->dip = dip;
	cd->id32 = id32_alloc(cd, KM_SLEEP);

	cd->sib.pmcw.ip = cd->id32;
	cd->sib.pmcw.isc = isc;
	cd->sib.pmcw.enabled = 0;

	ccw_device_modify(cd);

	return (cd);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_unregister.                            */
/*                                                                  */
/* Function	- Unregister a CCW device                           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
ccw_device_unregister(ccw_device *cd)
{
	id32_free(cd->id32);
	mutex_destroy(&cd->intrlock);
	kmem_free(cd, sizeof(*cd));

	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_set_private.                           */
/*                                                                  */
/* Function	- Set driver private data for device                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
ccw_device_set_private(ccw_device *cd, void *data)
{
	cd->devpriv = data;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_get_private.                           */
/*                                                                  */
/* Function	- Get driver private data for device                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void *
ccw_device_get_private(ccw_device *cd)
{
	return cd->devpriv;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_get_schid.                             */
/*                                                                  */
/* Function	- Get subchannel id                                 */
/*                                                                  */
/*------------------------------------------------------------------*/

uint32_t
ccw_device_get_schid(ccw_device *cd)
{
	return cd->schid;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_get_schib.                             */
/*                                                                  */
/* Function	- Get subchannel information block                  */
/*                                                                  */
/*------------------------------------------------------------------*/

struct schib *
ccw_device_get_schib(ccw_device *cd)
{
	return &cd->sib;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_get_irb.                               */
/*                                                                  */
/* Function	- Get interruption response block                   */
/*                                                                  */
/*------------------------------------------------------------------*/

irb *
ccw_device_get_irb(ccw_device *cd)
{
	return &cd->irb;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_get_rdc.                               */
/*                                                                  */
/* Function	- Get read device characteristics block             */
/*                                                                  */
/*------------------------------------------------------------------*/

struct vrdcblok *
ccw_device_get_rdc(ccw_device *cd)
{
	return &cd->rdc;
}

/*========================= End of Function ========================*/

void
showmem(char *str, void *mem, int len)
{
	
	int i,j,k;
	uchar_t *p = (uchar_t *) mem;

	DCCW(DCCW_L4, "%s: (%p:%d)  \n", str, mem, len);
	for (i = 0; i < len; ) {
		DCCW(DCCW_L4, "%04x:", i);
		for (j = 0; j < 4 && i < len; j++) {
			DCCW(DCCW_L4, "  ");
			for (k = 0; k < 4 && i < len; k++, i++) {
				DCCW(DCCW_L4, "%02x", *p++);
			}
		}
		DCCW(DCCW_L4, "\n");
	}
	DCCW(DCCW_L4, "\n");
}

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_set_handler.                           */
/*                                                                  */
/* Function	- Set I/O completion handler                        */
/*                                                                  */
/*------------------------------------------------------------------*/

void
ccw_device_set_handler(ccw_device *cd, ccw_dev_handler handler)
{
	DCCW(DCCW_L1, "ccw_device_set_handler(%p, %p)\n",
		cd, handler);

	cd->handler = handler;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_enable.                                */
/*                                                                  */
/* Function	- Enable subchannel                                 */
/*                                                                  */
/*------------------------------------------------------------------*/

int
ccw_device_enable(ccw_device *cd)
{
	struct schib *sib = ccw_device_get_schib(cd);
	int cc;

	DCCW(DCCW_L1, "ccw_device_enable(%p)\n", cd);

	cc = ccw_device_store(cd);
	if (cc == 0) {
		sib->pmcw.ip = cd->id32;
		sib->pmcw.enabled = 1;
		cc = ccw_device_modify(cd);
	}

	return cc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_disable.                               */
/*                                                                  */
/* Function	- Disable subchannel                                */
/*                                                                  */
/*------------------------------------------------------------------*/

int
ccw_device_disable(ccw_device *cd)
{
	struct schib *sib = ccw_device_get_schib(cd);
	int cc;

	DCCW(DCCW_L1, "ccw_device_disable(%p)\n", cd);

	cc = ccw_device_store(cd);
	if (cc == 0) {
		sib->pmcw.ip = cd->id32;
		sib->pmcw.enabled = 0;
		cc = ccw_device_modify(cd);
	}

	return cc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_cancel.                                */
/*                                                                  */
/* Function	- Cancel subchannel                                 */
/*                                                                  */
/*------------------------------------------------------------------*/

int
ccw_device_cancel(ccw_device *cd)
{
	uint32_t schid = cd->schid;
	int cc;

	DCCW(DCCW_L1, "ccw_device_cancel(%p)\n", cd);

	schid |= 0x00010000;
	__asm__ ("	lgr	1,%1\n"
		 "	xsch	\n"
		 "	lghi	%0,0\n"
		 "	ipm	%0\n"
		 "	srlg	%0,%0,28\n"
		 : "=r" (cc)
		 : "r" (schid)
		 : "1", "cc");

	return cc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_clear.                                 */
/*                                                                  */
/* Function	- Clear a subchannel                                */
/*                                                                  */
/*------------------------------------------------------------------*/

int
ccw_device_clear(ccw_device *cd)
{
	uint32_t schid = cd->schid;
	int cc;

	DCCW(DCCW_L1, "ccw_device_clear(%p)\n", cd);

	schid |= 0x00010000;
	__asm__ ("	lgr	1,%1\n"
		 "	csch	\n"
		 "	lghi	%0,0\n"
		 "	ipm	%0\n"
		 "	srlg	%0,%0,28\n"
		 : "=r" (cc)
		 : "r" (schid)
		 : "1", "cc");

	return cc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_halt.                                  */
/*                                                                  */
/* Function	- Halt subchannel                                   */
/*                                                                  */
/*------------------------------------------------------------------*/

int
ccw_device_halt(ccw_device *cd)
{
	uint32_t schid = cd->schid;
	int cc;

	DCCW(DCCW_L1, "ccw_device_halt(%p)\n", cd);

	schid |= 0x00010000;
	__asm__ ("	lgr	1,%1\n"
		 "	hsch	\n"
		 "	lghi	%0,0\n"
		 "	ipm	%0\n"
		 "	srlg	%0,%0,28\n"
		 : "=r" (cc)
		 : "r" (schid)
		 : "1", "cc");

	return cc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_modify.                                */
/*                                                                  */
/* Function	- Modify subchannel                                 */
/*                                                                  */
/*------------------------------------------------------------------*/

int
ccw_device_modify(ccw_device *cd)
{
	uint32_t schid = cd->schid;
	int cc;

	DCCW(DCCW_L1, "ccw_device_modify(%p)\n",cd);

	schid |= 0x00010000;
	__asm__ ("	lgr	1,%1\n"
		 "	msch	0(%2)\n"
		 "	lghi	%0,0\n"
		 "	ipm	%0\n"
		 "	srlg	%0,%0,28\n"
		 : "=r" (cc)
		 : "r" (schid), "r" (ccw_device_get_schib(cd))
		 : "1", "cc", "memory");

//	showmem("MSCH SCHIB", ccw_device_get_schib(cd), sizeof(schib));

	return cc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_resume.                                */
/*                                                                  */
/* Function	- Resume subchannel                                 */
/*                                                                  */
/*------------------------------------------------------------------*/

int
ccw_device_resume(ccw_device *cd)
{
	uint32_t schid = cd->schid;
	int cc;

	DCCW(DCCW_L1, "ccw_device_resume(%p)\n", cd);

	schid |= 0x00010000;
	__asm__ ("	lgr	1,%1\n"
		 "	rsch	\n"
		 "	lghi	%0,0\n"
		 "	ipm	%0\n"
		 "	srlg	%0,%0,28\n"
		 : "=r" (cc)
		 : "r" (schid)
		 : "1", "cc");

	return cc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_start.                                 */
/*                                                                  */
/* Function	- Start an I/O request                              */
/*                                                                  */
/*------------------------------------------------------------------*/

int
ccw_device_start(ccw_device_req *req)
{
	ccw_device *cd = req->cd;
	uint32_t schid = cd->schid;
	struct orb orb;
	irb irb;
	int cc;

	DCCW(DCCW_L1, "ccw_device_start(%p)\n", req);

	memset(&orb, 0, sizeof(orb));

	orb.iop = cd->id32;			/* need cd when interrupt fires */
	orb.fc = 1;				/* use format-1 CCWs */
	orb.pfc = 0;				/* don't prefetch CCWs */
	orb.key = 0;				/* storage key is always zero */
	orb.fmt2 = 1;				/* use format-2 (64-bit) IDAWs */
	orb.idawctl = 0;			/* use 4k page size for IDAWs */
	orb.lpm = 0xff;				/* use all available paths */
	orb.midaw = 0;				/* standard IDAWs */
	if (cd->rdc.vrdcvfla & VRDFLMID) {
		orb.midaw = 1;			/* modified IDAWs */
	}
	orb.cpa = (uint_t) va_to_pa(req->ccws);	/* */

//	showmem("SSCH ORB", &orb, sizeof(orb));

	req->busy = B_TRUE;

	cd->active = req;

	schid |= 0x00010000;
	__asm__ ("	lgr	1,%1\n"
		 "	ssch	%2\n"
		 "	lghi	%0,0\n"
		 "	ipm	%0\n"
		 "	srlg	%0,%0,28\n"
		 : "=r" (cc)
		 : "r" (schid), "Q" (orb)
		 : "1", "cc");

	return cc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_store.                                 */
/*                                                                  */
/* Function	- Store subchannel                                  */
/*                                                                  */
/*------------------------------------------------------------------*/

int
ccw_device_store(ccw_device *cd)
{
	uint32_t schid = cd->schid;
	int cc;

	DCCW(DCCW_L1, "ccw_device_store(%p)\n", cd);

	schid |= 0x00010000;
	__asm__ ("	lgr	1,%1\n"
		 "	stsch	0(%2)\n"
		 "	lghi	%0,0\n"
		 "	ipm	%0\n"
		 "	srlg	%0,%0,28\n"
		 : "=r" (cc)
		 : "r" (schid), "r" (ccw_device_get_schib(cd))
		 : "1", "cc");

//	showmem("STSCH SCHIB", ccw_device_get_schib(cd), sizeof(schib));

	return cc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_test.                                  */
/*                                                                  */
/* Function	- Test subchannel                                   */
/*                                                                  */
/*------------------------------------------------------------------*/

int
ccw_device_test(ccw_device *cd)
{
	uint32_t schid = cd->schid;
	int cc;

	DCCW(DCCW_L1, "ccw_device_test(%p)\n", cd);

	schid |= 0x00010000;
	__asm__ ("	lgr	1,%1\n"
		 "	tsch	0(%2)\n"
		 "	lghi	%0,0\n"
		 "	ipm	%0\n"
		 "	srlg	%0,%0,28\n"
		 : "=r" (cc)
		 : "r" (schid), "r" (ccw_device_get_irb(cd))
		 : "1", "cc");

//	showmem("TSCH IRB", ccw_device_get_irb(cd), sizeof(irb));

	return cc;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_alloc_req.                             */
/*                                                                  */
/* Function	- Allocate an I/O request                           */
/*                                                                  */
/*------------------------------------------------------------------*/

ccw_device_req *
ccw_device_alloc_req(ccw_device *cd)
{
	ccw_device_req *req;

	DCCW(DCCW_L1, "ccw_device_alloc_req(%p)\n", cd);

	req = kmem_zalloc(sizeof(*req), KM_SLEEP);
	mutex_init(&req->lock, NULL, MUTEX_DRIVER, NULL);
	cv_init(&req->done, NULL, CV_DRIVER, NULL);
	req->id = id32_alloc(req, KM_SLEEP);
	req->cd = cd;
	req->dip = cd->dip;

	return req;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_device_free_req.                              */
/*                                                                  */
/* Function	- Free an I/O request                               */
/*                                                                  */
/*------------------------------------------------------------------*/

void
ccw_device_free_req(ccw_device_req *req)
{
	int i;

	DCCW(DCCW_L1, "ccw_device_free_req(%p)\n", req);

	/* XXX Probaby should check for outstanding I/O and cancel */
	if (req->ccws) {
		if (req->idas) {
			for (i = 0; i < req->cmdnext; i++) {
				if (req->idas[i] == NULL) {
					continue;
				}
				if (req->ccws[i].flags & CCW_FLAG_MIDAW) {
					ccw_free31(req->idas[i],
						   CCW_IDAWCOUNT * sizeof(midaw));
				}
				else if (req->ccws[i].flags & CCW_FLAG_IDA) {
					ccw_free31(req->idas[i],
						   CCW_IDAWCOUNT * sizeof(uint64_t));
				}
			}

			kmem_free(req->idas, req->cmdcount * sizeof(*req->idas));
		}
	
		ccw_free31(req->ccws, req->cmdcount * sizeof(*req->ccws));
	}

	if (req->id) {
		id32_free(req->id);
	}

	cv_destroy(&req->done);
	mutex_destroy(&req->lock);

	kmem_free(req, sizeof(*req));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_cmd_add.                                      */
/*                                                                  */
/* Function	- Add a new CCW to the channel program              */
/*                                                                  */
/* Note		- Data pages should be locked already               */
/*                                                                  */
/*------------------------------------------------------------------*/

int
ccw_cmd_add(ccw_device_req *req,
	    uchar_t op,
	    uchar_t flags,
	    ushort_t count,
	    void *data)
{
	struct ccw1 *ccw;
	caddr_t *idas;
	uint64_t pa;
	int ndx;

	DCCW(DCCW_L1, "ccw_cmd_add(%p, %02x, %02x, %d, %p\n",
		req, op, flags, count, data);

	if (req->cmdnext == req->cmdcount) {
		int osize = req->cmdcount * sizeof(*req->ccws);
		int nsize = osize + (CCW_CMDCOUNT * sizeof(*req->ccws));

		ccw = ccw_alloc31(nsize, VM_SLEEP);

		if (req->ccws && osize) {
			memcpy(ccw, req->ccws, osize);
			ccw_free31(req->ccws, osize);
		}

		req->ccws = ccw;

		osize = req->cmdcount * sizeof(*req->idas);
		nsize = osize + (CCW_CMDCOUNT * sizeof(*req->idas));

		idas = kmem_zalloc(nsize, KM_SLEEP);

		if (req->idas && osize) {
			memcpy(idas, req->idas, osize);
			kmem_free(req->idas, osize);
		}

		req->idas = idas;

		req->cmdcount += CCW_CMDCOUNT;
	}

	pa = va_to_pa(data);
	ccw = &req->ccws[req->cmdnext];
	ccw->op = op;
	ccw->flags = flags;
	ccw->count = count;
	ccw->data = (uint_t) pa;

	if (req->cmdnext > 0) {
		ccw[-1].flags |= CCW_FLAG_CC;
	}

	if ((pa + count) >= (1 << 31)) {
		ccw_device *cd = (ccw_device *)
			ddi_get_driver_private(req->dip);
		int icount;

		if (cd->rdc.vrdcvfla & VRDFLMID) {
			midaw *idaws;

			ccw->flags |= CCW_FLAG_MIDAW;

			idaws = ccw_alloc31(CCW_IDAWCOUNT * sizeof(*idaws), VM_SLEEP);
			req->idas[req->cmdnext] = (caddr_t) idaws;

			ccw->data = (uint_t) va_to_pa(idaws);

			icount = MIN(count, (4096 - (pa & 0xfff)));
			while (icount != 0) {
				idaws->count = icount;
				idaws->data = pa;

				count -= icount;
				pa += icount;

				if (count == 0) {
					idaws->last = 1;
				}

				idaws++;
	
				icount = MIN(count, 4096);
			}
		}
		else {
			uint64_t *idaws;

			ccw->flags |= CCW_FLAG_IDA;

			idaws = ccw_alloc31(CCW_IDAWCOUNT * sizeof(*idaws), VM_SLEEP);
			req->idas[req->cmdnext] = (caddr_t) idaws;

			ccw->data = (uint_t) va_to_pa(idaws);

			icount = MIN(count, (4096 - (pa & 0xfff)));
			while (icount != 0) {
				*idaws = pa;
				idaws++;
				count -= icount;
				pa += icount;
	
				icount = MIN(count, 4096);
			}
		}
	}

	ndx = req->cmdnext++;

	ccw_dump_req(req);

	return ndx;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_cmd_set_flags.                                */
/*                                                                  */
/* Function	- Set flags in CCW at given index                   */
/*                                                                  */
/*------------------------------------------------------------------*/

int
ccw_cmd_set_flags(ccw_device_req *req, int ccw, uchar_t flags)
{
	DCCW(DCCW_L1, "ccw_cmd_set_flags(%p, %d, %d)\n",
		req, ccw, flags);

	if (ccw >= req->cmdcount) {
		return -1;
	}

	flags &= ~(CCW_FLAG_IDA|CCW_FLAG_MIDAW);

	req->ccws[ccw].flags |= flags;

	return req->ccws[ccw].flags;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_cmd_clear_flags.                              */
/*                                                                  */
/* Function	- Clear flags in CCW at given index                 */
/*                                                                  */
/*------------------------------------------------------------------*/

int
ccw_cmd_clear_flags(ccw_device_req *req, int ccw, uchar_t flags)
{
	DCCW(DCCW_L1, "ccw_cmd_clear_flags(%p, %d, %d)\n",
		req, ccw, flags);

	if (ccw >= req->cmdcount) {
		return -1;
	}

	flags |= CCW_FLAG_IDA|CCW_FLAG_MIDAW;

	req->ccws[ccw].flags &= (~flags);

	return req->ccws[ccw].flags;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_cmd_reset.		                    */
/*                                                                  */
/* Function	- Reset list of CCWs to allow reusing request.      */
/*                                                                  */
/*------------------------------------------------------------------*/

void
ccw_cmd_reset(ccw_device_req *req)
{
	DCCW(DCCW_L1, "ccw_cmd_reset(%p)\n", req);

	req->cmdnext = 0;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_dump_req.                                     */
/*                                                                  */
/* Function	- Display contents of I/O request                   */
/*                                                                  */
/*------------------------------------------------------------------*/

static void
ccw_dump_req(ccw_device_req *req)
{
	if (ccw_msglevel > DCCW_L1) {
		return;
	}

	DCCW(DCCW_L1, "dump_req(%p)\n", req);

	DCCW(DCCW_L1, "    cd:            %p\n", req->cd);
	DCCW(DCCW_L1, "    cmdcount:      %d\n", req->cmdcount);
	DCCW(DCCW_L1, "    cmdnext:       %d\n", req->cmdnext);
	DCCW(DCCW_L1, "    ccws:          %p\n", req->ccws);

	for (int i = 0; i < req->cmdnext; i++) {
		DCCW(DCCW_L1, "        ccw:       %d\n", i);
		DCCW(DCCW_L1, "            op:    %02x\n", req->ccws[i].op);
		DCCW(DCCW_L1, "            flags: %02x\n", req->ccws[i].flags);
		DCCW(DCCW_L1, "            count: %d\n", req->ccws[i].count);
		DCCW(DCCW_L1, "            data:  %08x\n", req->ccws[i].data);

		if (req->ccws[i].flags & CCW_FLAG_MIDAW) {
			midaw *mp = (midaw *) req->idas[i];

			DCCW(DCCW_L1, "        midaws:       %p\n", mp);
			do {
				DCCW(DCCW_L1, "            last:  %d\n", mp->last);
				DCCW(DCCW_L1, "            skip:  %d\n", mp->skip);
				DCCW(DCCW_L1, "            dtic:  %d\n", mp->dtic);
				DCCW(DCCW_L1, "            count: %d\n", mp->count);
				DCCW(DCCW_L1, "            data:  %lx\n", mp->data);
			} while (!(mp++)->last);
		}
		else if (req->ccws[i].flags & CCW_FLAG_IDA) {
			uint64_t *ip = (uint64_t *) req->idas[i];
			int count = req->ccws[i].count;

			DCCW(DCCW_L1, "        idaws:        %p\n", ip);
			while (count > 0) {
				DCCW(DCCW_L1, "            data:  %lx\n", *ip);
				count -= 4096;
				ip++;
			}
		}
	}
}

/*========================= End of Function ========================*/
