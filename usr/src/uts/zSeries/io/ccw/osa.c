/*------------------------------------------------------------------*/
/* 								    */
/* Name        - osa.c      					    */
/* 								    */
/* Function    - Network driver for OSA card using DIAG interface.  */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 	         Leland Lucius					    */
/* 								    */
/* Date        - August, 2007					    */
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

#define OSA_NAME "osa"
#define OSA_IDENT "osa"

/*
 * TRUE if frame contains a broadcast address
 */
#define IS_BROADCAST(ehp) \
        (ether_cmp(&((struct ether_header *)ehp)->ether_dhost, &OSA_broadcastaddr) == 0)

/*
 * TRUE if frame is a multicast address
 */
#define IS_MULTICAST(ehp) \
        ((((struct ether_header *)ehp)->ether_dhost.ether_addr_octet[0] & 01) == 1)

/*
 * Enable (1) or disable (0) debugging
 */
#define OSA_DEBUG 1

/*
 * Macro to define an area big enough to contain an aligned parameter list
 * so that it doesn't cross a page boundary.  This must be a power of 2 and
 * should be sufficient to hold the largest parameter list to use it.
 */
#define PLALIGN 128

/*
 * Macro to return an aligned address that does not cross a page boundary.
 */
#define OSAALIGN(a, b, c)							\
	char t_ ## b[2 * c];							\
	a *b = (a *) ((((uintptr_t) t_ ## b) + c) & ~(c - 1))

/*
 * Macro to return an aligned address for a paraemter list so that it doesn't
 * cross a page boundary.
 */
#define OSAPL(a, b)								\
	OSAALIGN(a, b, PLALIGN)

/*
 * Macro to return an aligned address for a packet used by the interrupt thread
 * so it doesn't have to call vmem_xalloc() and potentially sleep
 */
#define OSAPKT(a, b)								\
	OSAALIGN(a, b, PAGESIZE)

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/strsun.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/devops.h>
#include <sys/ksynch.h>
#include <sys/stat.h>
#include <sys/modctl.h>
#include <sys/debug.h>
#include <sys/dlpi.h>
#include <sys/ethernet.h>
#include <sys/gld.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/ios390x.h>
#include <sys/ccw.h>
#include <sys/osa.h>
#include <sys/sysmacros.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern vmem_t *dma31_arena;
extern vmem_t *static_alloc_arena;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

/* Required system entry points */
static int	OSA_probe(dev_info_t *);
static int	OSA_attach(dev_info_t *, ddi_attach_cmd_t);
static int	OSA_detach(dev_info_t *, ddi_detach_cmd_t);

/* Required driver entry points for GLD */
static int 	OSA_set_mac_addr(gld_mac_info_t *, unsigned char *);
static int	OSA_reset(gld_mac_info_t *);
static int	OSA_start(gld_mac_info_t *);
static int	OSA_stop(gld_mac_info_t *);
static int	OSA_set_multicast(gld_mac_info_t *, unsigned char *, int);
static int	OSA_set_promiscuous(gld_mac_info_t *, int);
static int	OSA_get_stats(gld_mac_info_t *, struct gld_stats *);
static int	OSA_send(gld_mac_info_t *, mblk_t *);
static int	OSA_intr(ccw_device *, ccw_device_req *);

/* Internal functions */
static int	OSA_query(gld_mac_info_t *);
static int	OSA_init(gld_mac_info_t *);
static void	OSA_uninit(gld_mac_info_t *);
static void	OSA_clear(OSA_state *);
static int	OSA_diag(OSA_state *, int, void *, int *);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

char _depends_on[] = "misc/gld ccwnex";

/*
 * Declarations and Module Linkage
 */

#if OSA_DEBUG
static int OSA_debug = 0x0;
#endif

static unsigned char OSA_broadcastaddr[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* Standard Streams initialization */
static struct module_info minfo = {
	0, OSA_NAME, 0, INFPSZ, OSAHIWAT, OSALOWAT
};

static struct qinit rinit = {	/* read queues */
	0, gld_rsrv, gld_open, gld_close, 0, &minfo, 0
};

static struct qinit winit = {	/* write queues */
	gld_wput, gld_wsrv, 0, 0, 0, &minfo, 0
};

static struct streamtab osa_info = {
	&rinit, &winit, NULL, NULL
};

/* Standard Module linkage initialization for a Streams driver */

static 	struct cb_ops cb_osa_ops = {
	nulldev,		/* cb_open */
	nulldev,		/* cb_close */
	nodev,			/* cb_strategy */
	nodev,			/* cb_print */
	nodev,			/* cb_dump */
	nodev,			/* cb_read */
	nodev,			/* cb_write */
	nodev,			/* cb_ioctl */
	nodev,			/* cb_devmap */
	nodev,			/* cb_mmap */
	nodev,			/* cb_segmap */
	nochpoll,		/* cb_chpoll */
	ddi_prop_op,		/* cb_prop_op */
	&osa_info,		/* cb_stream */
	(int)(D_MP)		/* cb_flag */
};

static struct dev_ops osa_ops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	gld_getinfo,		/* devo_getinfo */
	nulldev,		/* devo_identify */
	OSA_probe,		/* devo_probe */
	OSA_attach,		/* devo_attach */
	OSA_detach,		/* devo_detach */
	nodev,			/* devo_reset */
	&cb_osa_ops,		/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
};

static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module.  This one is a driver */
	OSA_IDENT " %I%",	/* short description */
	&osa_ops		/* driver specific ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _init.                                            */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
_init(void)
{
	return (mod_install(&modlinkage));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _fini.                                            */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
_fini(void)
{
	return (mod_remove(&modlinkage));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _info.                                            */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*========================= End of Function ========================*/

/*==================================================================*/
/* DDI Entry Points						    */
/*==================================================================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- probe.                                            */
/*                                                                  */
/* Function	- Determine if a device is present. See probe(9e).  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
static int
OSA_probe(dev_info_t *devinfo)
{
#if OSA_DEBUG
	if (OSA_debug & OSADDI) {
		prom_printf(
			OSA_NAME "_probe(%p)\n",
			devinfo);
	}
#endif

	return (DDI_PROBE_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- attach.                                           */
/*                                                                  */
/* Function	- Attach a device to the system. Called once for    */
/*		  each card successfully probed. See attach(9e).    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
OSA_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	ccw_device *cd;
	gld_mac_info_t *macinfo;
	OSA_state *os;
	int instance = ddi_get_instance(dip);
	int rc;

#if OSA_DEBUG
	if (OSA_debug & OSADDI) {
		prom_printf(
			OSA_NAME "_attach(%p)\n",
			dip);
	}
#endif

	/* We only allow ATTACH/DETACH */
	if (cmd != DDI_ATTACH) {
		return (DDI_FAILURE);
	}


	/* Register with the CCW layer */
	cd = ccw_device_register(dip);
	if (cd == NULL) {
		cmn_err(CE_WARN,
			OSA_NAME "%d: failed to register with ccw layer",
			instance);
		return (DDI_FAILURE);
	}

	/* Allocate GLD macinfo */
	macinfo = gld_mac_alloc(dip);
	if (macinfo == NULL) {
		cmn_err(CE_WARN,
			OSA_NAME "%d: unable to alloc mac",
			instance);
		goto error1;
	}

	/* Allocate our state */
	os = kmem_zalloc(sizeof(OSA_state), KM_NOSLEEP);
	if (os == NULL) {
		cmn_err(CE_WARN,
			OSA_NAME "%d: unable to alloc state",
			instance);
		goto error2;
	}

	/* Save a few things */
	os->cd = cd;
	os->dip = dip;
	os->macinfo = macinfo;
	os->devno = os->cd->rdc.vrdcdvno;

	/* Initialize active mutex */
	mutex_init(&os->activelock, NULL, MUTEX_DRIVER, NULL);

	/* Initialize function vectors */
	macinfo->gldm_reset		= OSA_reset;
	macinfo->gldm_start		= OSA_start;
	macinfo->gldm_stop		= OSA_stop;
	macinfo->gldm_set_mac_addr	= OSA_set_mac_addr;
	macinfo->gldm_set_multicast	= OSA_set_multicast;
	macinfo->gldm_set_promiscuous	= OSA_set_promiscuous;
	macinfo->gldm_get_stats		= OSA_get_stats;
	macinfo->gldm_send		= OSA_send;
	macinfo->gldm_intr		= NULL;
	macinfo->gldm_ioctl		= NULL;

	/* Intialize board characteristics */
	macinfo->gldm_ident		= OSA_IDENT;
	macinfo->gldm_type		= DL_ETHER;
	macinfo->gldm_minpkt		= 0;		/* z/VM will pad */
	macinfo->gldm_maxpkt		= OSAMAXPKT;
	macinfo->gldm_addrlen		= ETHERADDRL;
	macinfo->gldm_saplen		= -2;
	macinfo->gldm_ppa		= instance;
	macinfo->gldm_devinfo		= dip;
	macinfo->gldm_broadcast_addr	= OSA_broadcastaddr;
	macinfo->gldm_private		= (caddr_t) os;

	/* Enable the device */
	rc = ccw_device_enable(cd);
	if (rc != 0) {
		cmn_err(CE_WARN,
			OSA_NAME "%d: unable to enable device",
			instance);
		goto error4;
	}

	/* Query the NIC to verify it's valid and get the macaddr */
	rc = OSA_query(macinfo);
	if (rc != 0) {
		goto error5;
	}
	macinfo->gldm_vendor_addr = os->qi.macaddr;

	/* Add the interrupt handler */
	ccw_device_set_handler(cd, OSA_intr);

	/* Register ourselves with the GLD interface */
	rc = gld_register(dip, OSA_NAME, macinfo);
	if (rc != DDI_SUCCESS) {
		goto error5;
	}

	return (DDI_SUCCESS);

error5:
	ccw_device_disable(cd);

error4:
	mutex_destroy(&os->activelock);

error3:
	kmem_free(os, sizeof(*os));

error2:
	gld_mac_free(macinfo);

error1:
	ccw_device_unregister(cd);

	return (DDI_FAILURE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- detach.                                           */
/*                                                                  */
/* Function	- Detach a device from the system. See detach(9e).  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
OSA_detach(dev_info_t *devinfo, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t *macinfo = ddi_get_driver_private(devinfo);
	OSA_state *os = (OSA_state *) macinfo->gldm_private;
	ccw_device *cd = os->cd;
	int i;

#if OSA_DEBUG
	if (OSA_debug & OSADDI) {
		prom_printf(
			OSA_NAME "_detach(%p)\n",
			devinfo);
	}
#endif

	/* We only allow ATTACH/DETACH */
	if (cmd != DDI_DETACH) {
		return (DDI_FAILURE);
	}

	/* Make sure we're stopped */
	OSA_stop(macinfo);

	/* Unregister ourselves from the GLD interface */
	if (gld_unregister(macinfo) != DDI_SUCCESS) {
		return (DDI_FAILURE);
	}

	/* Don't allow any more interrupts */
	ccw_device_disable(cd);

	/* Kill the mutex */
	mutex_destroy(&os->activelock);

	/* State is no longer needed */
	kmem_free(os, sizeof(*os));

	/* And neither it the GLD macinfo */
	gld_mac_free(macinfo);

	/* Unregister from the CCW layer */
	ccw_device_unregister(cd);

	return (DDI_SUCCESS);
}

/*========================= End of Function ========================*/

/*==================================================================*/
/* GLD Entry Points						    */
/*==================================================================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- OSA_reset.                                        */
/*                                                                  */
/* Function	- Reset the card to its initial state.              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
OSA_reset(gld_mac_info_t *macinfo)
{
	OSA_state *os = (OSA_state *) macinfo->gldm_private;
	int rc;

#if OSA_DEBUG
	if (OSA_debug & OSATRACE) {
		prom_printf(
			OSA_NAME "_reset(%p)\n",
			macinfo);
	}
#endif

	/* Stop does all the work for us */
	rc = OSA_stop(macinfo);
	if (rc == GLD_SUCCESS) {
		rc = OSA_init(macinfo);
	}

	return (rc);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- OSA_start.                                        */
/*                                                                  */
/* Function	- Set the adapter receiving and allow transmits.    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
OSA_start(gld_mac_info_t *macinfo)
{
	OSA_state *os = (OSA_state *) macinfo->gldm_private;
	int rc;

#if OSA_DEBUG
	if (OSA_debug & OSATRACE) {
		prom_printf(
			OSA_NAME "_start(%p)\n",
			macinfo);
	}
#endif

	/* Already started? */
	if (os->active) {
		/* Something's amiss */
		return GLD_FAILURE;
	}

	/* Block */
	mutex_enter(&os->activelock);

	/* Get a new CCW request */
	os->active = ccw_device_alloc_req(os->cd);

	/* Add the command given to us */
	ccw_cmd_add(os->active,
		    os->qi.ccw.op,
		    os->qi.ccw.flags,
		    os->qi.ccw.count,
		    (void *) (uintptr_t) os->qi.ccw.data);

	/* Unblock */
	mutex_exit(&os->activelock);

	/* Start the (never ending) I/O */
	rc = ccw_device_start(os->active);
	if (rc != 0) {
		/* Free the request */
		mutex_enter(&os->activelock);
		ccw_device_free_req(os->active);
		os->active = NULL;
		mutex_exit(&os->activelock);

		cmn_err(CE_WARN,
			OSA_NAME "%d: Unable to start I/O, rc = %d",
			macinfo->gldm_ppa,
			rc);

		return GLD_FAILURE;
	}

	/* Drop any pending packets */
	OSA_clear(os);

	return GLD_SUCCESS;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- OSA_stop.                                         */
/*                                                                  */
/* Function	- Stop the adapter receiving.                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
OSA_stop(gld_mac_info_t *macinfo)
{
	OSA_state *os = (OSA_state *) macinfo->gldm_private;
	ccw_device_req *active;
	int rc;

#if OSA_DEBUG
	if (OSA_debug & OSATRACE) {
		prom_printf(
			OSA_NAME "_stop(%p)\n",
			macinfo);
	}
#endif
	/* Block */
	mutex_enter(&os->activelock);

	/* Terminate if we have one */
	if (os->active) {
		/* Clear */
		OSA_clear(os);

		/* Cancel the request */
		rc = ccw_device_clear(os->cd);
		if (rc == 0) {
			/* Wait for request to clear */
			mutex_enter(&os->active->lock);
			while (os->active->busy) {
				cv_wait(&os->active->done, &os->active->lock);
			}
			mutex_exit(&os->active->lock);
		}

		/* Delete the request and mark us inactive */
		ccw_device_free_req(os->active);

		/* No longer active */
		os->active = NULL;
	}

	/* Unblock */
	mutex_exit(&os->activelock);

	/* DDI manual says that we must never fail */
	return GLD_SUCCESS;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- OSA_set_mac_addr.                                 */
/*                                                                  */
/* Function	- Set the node's MAC address.                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
OSA_set_mac_addr(gld_mac_info_t *macinfo, unsigned char *macaddr)
{
#if defined(OSA_SET_MAC_ADDR_SUPPORTED)
	/*
	 * Not sure if diag 2a8 will allow us to supply our own
	 * macaddr or what the GLD will do if we do a stop/start
	 * behind the scenes since that's the only way we can set
	 * the macaddr using 2a8.
	 */

	OSA_state *os = (OSA_state *) macinfo->gldm_private;
	boolean_t active = (os->active != NULL);
	int rc;

	bcopy(macaddr, os->qi.macaddr, ETHERADDRL);
	rc = #OSA_reset(macinfo);
	if (rc == GLD_SUCCESS && active) {
		rc = OSA_start(macinfo);
	}

	return (GLD_FAILURE);
#else
	return (GLD_NOTSUPPORTED);
#endif
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- OSA_set_multicast.                                */
/*                                                                  */
/* Function	- Enable or disable a multicast address.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
OSA_set_multicast(gld_mac_info_t *macinfo, unsigned char *mcast, int op)
{
	OSA_state *os = (OSA_state *) macinfo->gldm_private;
	OSAPL(macpl, mpl);
	int rc;
	int cc;

#if OSA_DEBUG
	if (OSA_debug & OSATRACE) {
		prom_printf(
			OSA_NAME "_set_multicast(%p, %p, %s)\n",
			macinfo,
			mcast,
			(op == GLD_MULTI_ENABLE) ? "ON" : "OFF");
	}
#endif

	/* Make sure reserved fields are clear */
	bzero(mpl, sizeof(*mpl));

	/* Set function based on opcode */
	switch (op) {
	case GLD_MULTI_ENABLE:
		mpl->function = MPL_FUNC_ASSIGN;
		break;
	case GLD_MULTI_DISABLE:
		mpl->function = MPL_FUNC_UNASSIGN;
		break;
	}

	/* Set target device and macaddr */
	mpl->devno = os->devno;
	bcopy(mcast, mpl->macaddr, ETHERADDRL);

	/* Issue request */
	cc = OSA_diag(os, OSA_OP_MAR, mpl, &rc);

	if (cc != 0) {
#if OSA_DEBUG
		cmn_err(CE_WARN,
			OSA_NAME "%d: set_multicast failed %d-%d",
			macinfo->gldm_ppa,
			cc, rc);
#endif
		return (GLD_FAILURE);
	}

	return (GLD_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- OSA_set_promiscuous.                              */
/*                                                                  */
/* Function	- Set or reset promiscuous mode on the card.        */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
OSA_set_promiscuous(gld_mac_info_t *macinfo, int on)
{
	OSA_state *os = (OSA_state *) macinfo->gldm_private;
	OSAPL(dopl, dpl);
	int rc;
	int cc;

#if OSA_DEBUG
	if (OSA_debug & OSATRACE) {
		prom_printf(
			OSA_NAME "_promiscuous(%p, %s)\n",
			macinfo,
			(on != GLD_MAC_PROMISC_NONE) ? "ON" : "OFF");
	}
#endif

	/* Make sure reserved fields are clear */
	bzero(dpl, sizeof(*dpl));

	/* Set function based on opcode */
	switch (on) {
	case GLD_MAC_PROMISC_NONE:
		dpl->function = DPL_FUNC_PROMISC_OFF;
		break;
	default:
		dpl->function = DPL_FUNC_PROMISC_ON;
		break;
	}

	/* Set target device */
	dpl->devno = os->devno;

	/* Issue request */
	cc = OSA_diag(os, OSA_OP_NDO, dpl, &rc);

	if (rc != 0) {
#if OSA_DEBUG
		cmn_err(CE_WARN,
			OSA_NAME "%d: set_promiscuous failed %d-%d",
			macinfo->gldm_ppa,
			cc, rc);
#endif
		return (GLD_FAILURE);
	}

	return (GLD_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- OSA_get_stats.                                    */
/*                                                                  */
/* Function	- Update statistics.                                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
OSA_get_stats(gld_mac_info_t *macinfo, struct gld_stats *g_stats)
{
	OSA_state *os = (OSA_state *) macinfo->gldm_private;

#if OSA_DEBUG
	if (OSA_debug & OSATRACE) {
		prom_printf(
			OSA_NAME "_get_stat(%p)\n",
			macinfo);
	}
#endif

	/* Return current statistics...not much yet */
	g_stats->glds_intr = os->numintrs;
	g_stats->glds_speed = 100000000000;
	g_stats->glds_duplex = GLD_DUPLEX_FULL;
	g_stats->glds_media = GLDM_UNKNOWN;

	return (GLD_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- OSA_send.                                         */
/*                                                                  */
/* Function	- Send a packet.                                    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
OSA_send(gld_mac_info_t *macinfo, mblk_t *mp)
{
	OSA_state *os = (OSA_state *) macinfo->gldm_private;
	OSAPL(netpl, npl);
	caddr_t pkt;
	caddr_t	dest;
	mblk_t	*bp;
	int	rc;
	int	cc;
	size_t	n;

#if OSA_DEBUG
	if (OSA_debug & OSATRACE) {
		prom_printf(
			OSA_NAME "_send(%p, %p)\n",
			macinfo,
			mp);
	}
#endif

	/* Ensure reserved fields are clear */
	bzero(npl, sizeof(*npl));

	/* We only support 1 packet per request for now */
	npl->entries = 1;

	/* Set packet length and verify */
	npl->drbs[0].count = msgsize(mp);
	if (npl->drbs[0].count > ETHERMAX) {
		cmn_err(CE_WARN,
			OSA_NAME "%d: dropping oversize packet (%d)",
			macinfo->gldm_ppa,
			npl->drbs[0].count);
		return (GLD_BADARG);
	}

	/* Get memory for packet */
	pkt = vmem_xalloc(static_alloc_arena, npl->drbs[0].count,
			  PAGESIZE, 0, 0, NULL, NULL, VM_SLEEP);
	if (pkt == NULL) {
		return (GLD_NORESOURCES);
	}
	npl->drbs[0].addr = va_to_pa(pkt);

	/* Copy packet into allocated area */
	dest = pkt;
	for (bp = mp; bp != NULL; bp = bp->b_cont) {
		n = MBLKL(bp);
		if (n == 0) {
			/* Ignore zero-length segments */
			continue;
		}
		bcopy(bp->b_rptr, dest, n);
		dest += n;
	}

	/* Set destination type */
	if (IS_BROADCAST(pkt)) {
		npl->drbs[0].flags = OSA_DRB_BCAST;
	}
	else if (IS_MULTICAST(pkt)) {
		npl->drbs[0].flags = OSA_DRB_MCAST;
	}
	else {
		npl->drbs[0].flags = OSA_DRB_UCAST;
	}

	/* Issue request */
	cc = OSA_diag(os, OSA_OP_SDR, npl, &rc);

	/* Free packet memory */
	vmem_free(static_alloc_arena, pkt, npl->drbs[0].count);

	if (cc != 0) {
#if OSA_DEBUG
		cmn_err(CE_WARN,
			OSA_NAME "%d: send failed %d-%d",
			macinfo->gldm_ppa,
			cc, rc);
#endif
		return (GLD_FAILURE);
	}

	/* Only free message if send was successful */
	freemsg(mp);

	return (GLD_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- OSA_intr.                                         */
/*                                                                  */
/* Function	- Interrupt from card to inform us that a receive   */
/*		  or transmit has completed.   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
OSA_intr(ccw_device *dev, ccw_device_req *req)
{
	gld_mac_info_t *macinfo = ddi_get_driver_private(dev->dip);
	OSA_state *os = (OSA_state *) macinfo->gldm_private;
	irb *irb = ccw_device_get_irb(dev);
	OSAPL(netpl, npl);
	mblk_t	*mp;
	OSAPKT(char, pkt);
	int 	rc;
	int 	cc;

#if OSA_DEBUG
	if (OSA_debug & OSAINT) {
		prom_printf(
			OSA_NAME "_intr(%p)\n",
			macinfo);
	}
#endif

	/* Non-PCI interrupts are simply signaled and ignored */
	if (!(irb->scsw.cstat & SCHN_STAT_PCI)) {
		mutex_enter(&os->active->lock);
		os->active->busy = B_FALSE;
		cv_broadcast(&os->active->done);
		mutex_exit(&os->active->lock);
		return (0);
	}

	/* Update stats */
	os->numintrs++;

#if 0
	/* Get memory for incoming packet (allow for jumbo frames???) */
	pkt = vmem_xalloc(static_alloc_arena, PAGESIZE,
			  PAGESIZE, 0, 0, NULL, NULL, VM_SLEEP);
	if (pkt == NULL) {
		os->norcvbuf++;
		return (GLD_NORESOURCES);
	}
#endif

	/* Ensure reserved areas are clear */
	bzero(npl, sizeof(*npl));

	/* We only support 1 packet per request for now */
	npl->entries = 1;
	npl->drbs[0].addr = va_to_pa(pkt);

	/* Receive all queued packets from z/VM */
	do {
		/* Set maximum packet size */
		npl->drbs[0].count = PAGESIZE;

		/* Send request */
		cc = OSA_diag(os, OSA_OP_RDR, npl, &rc);
		if (cc != 0) {
#if OSA_DEBUG
			cmn_err(CE_WARN,
				OSA_NAME "%d: recv failed %d-%d",
				macinfo->gldm_ppa,
				cc, rc);
#endif
			break;
		}

		/* Allocate a message */
		mp = allocb(npl->drbs[0].count, 0);
		if (mp == NULL) {
			break;
		}

		/* Copy packet into it */
		bcopy(pkt, mp->b_wptr, npl->drbs[0].count);
		mp->b_wptr += npl->drbs[0].count;

		/* Send to GLD */
		gld_recv(macinfo, mp);
	} while (cc == 0 && rc == 4);

#if 0
	/* Free the packet memory */
	vmem_free(static_alloc_arena, pkt, PAGESIZE);
#endif

	return (0);
}

/*========================= End of Function ========================*/

/*==================================================================*/
/* Internal Entry Points					    */
/*==================================================================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- OSA_query.                                        */
/*                                                                  */
/* Function	- Retrieve interface information.                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
OSA_query(gld_mac_info_t *macinfo)
{
	OSA_state *os = (OSA_state *) macinfo->gldm_private;
	OSAPL(qipl, qpl);
	int s;
	int rc;
	int cc;

#if OSA_DEBUG
	if (OSA_debug & OSATRACE) {
		prom_printf(
			OSA_NAME "_query(%p)\n",
			macinfo);
	}
#endif

	/* Make sure reserved fields are clear */
	bzero(qpl, sizeof(*qpl));

	/* Send request */
	cc = OSA_diag(os, OSA_OP_QI, qpl, &rc);

	if (cc != 0) {
#if OSA_DEBUG
		cmn_err(CE_WARN,
			OSA_NAME "%d: query failed %d-%d",
			macinfo->gldm_ppa,
			cc, rc);
#endif
		return (GLD_FAILURE);
	}

	/* Store in state */
	bcopy(qpl, &os->qi, sizeof(*qpl));

	return (GLD_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- OSA_init.                                         */
/*                                                                  */
/* Function	- Initialize the device connection.                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
OSA_init(gld_mac_info_t *macinfo)
{
	OSA_state *os = (OSA_state *) macinfo->gldm_private;
	edcpl *edc;
	int i;
	int rc;
	int cc;
	uintptr_t page;

#if OSA_DEBUG
	if (OSA_debug & OSATRACE) {
		prom_printf(
			OSA_NAME "_init(%p)\n",
			macinfo);
	}
#endif

	/* Uninitialize if already initialized */
	if (os->edc) {
		OSA_uninit(macinfo);
	}

	/* Cal total size of parameter list */
	os->edclen = sizeof(edcpl) + ((os->qi.numpages - 1) * sizeof(uint64_t));

	/* Allocate parameter list (page boundary concerns here???) */
	os->edc = edc = kmem_zalloc(os->edclen, KM_SLEEP);
	if (edc == NULL) {
		return (GLD_NORESOURCES);
	}

	/* Calc size and allocate 64-bit work pages */
	os->edc64len = ptob(os->qi.numpages - os->qi.adrpages);
	os->edc64pages = vmem_xalloc(static_alloc_arena, os->edc64len,
				     PAGESIZE, 0, 0, NULL, NULL, VM_SLEEP);
	if (os->edc64pages == NULL) {
		OSA_uninit(macinfo);
		return (GLD_NORESOURCES);
	}

	/* Calc size and allocate 31-bit work pages */
	os->edc31len = ptob(os->qi.adrpages);
	os->edc31pages = vmem_xalloc(dma31_arena, os->edc31len,
				     PAGESIZE, 0, 0, NULL, NULL, VM_SLEEP);
	if (os->edc31pages == NULL) {
		OSA_uninit(macinfo);
		return (GLD_NORESOURCES);
	}

	/* Set target device and prepare page count */
	edc->devno = os->devno;
	edc->numpages = 0;

	/* Store pointers to 31-bit pages */
	page = (uintptr_t) os->edc31pages;
	while (edc->numpages < os->qi.adrpages) {
		edc->pages[edc->numpages++] = va_to_pa((void *)page);
		page += PAGESIZE;
	}

	/* Store pointers to 64-bit pages */
	page = (uintptr_t) os->edc64pages;
	while (edc->numpages < os->qi.numpages) {
		edc->pages[edc->numpages++] = va_to_pa((void *)page);
		page += PAGESIZE;
	}

	/* Issue request */
	cc = OSA_diag(os, OSA_OP_EDC, os->edc, &rc);

	if (cc != 0) {
		OSA_uninit(macinfo);
#if OSA_DEBUG
		cmn_err(CE_WARN,
			OSA_NAME "%d: init failed cc %d rc %d",
			macinfo->gldm_ppa,
			cc, rc);
#endif
		return (GLD_FAILURE);
	}

	return (GLD_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- OSA_uninit.                                       */
/*                                                                  */
/* Function	- Clean up a device connection.                     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
OSA_uninit(gld_mac_info_t *macinfo)
{
	OSA_state *os = (OSA_state *) macinfo->gldm_private;
	edcpl *edc;
	int rc;

#if OSA_DEBUG
	if (OSA_debug & OSATRACE) {
		prom_printf(
			OSA_NAME "_uninit(%p)\n",
			macinfo);
	}
#endif

	/* Stop device if it's active */
	if (os->active) {
		OSA_stop(macinfo);
	}

	/* Don't have anything to do if nothing's allocated */
	if (os->edc == NULL) {
		return;
	}

	/* Free 31-bit work pages */
	if (os->edc31pages != NULL) {
		vmem_free(dma31_arena,
			  os->edc31pages,
			  os->edc31len);
		os->edc31pages = NULL;
		os->edc31len = 0;
	}

	/* Free 64-bit work pages */
	if (os->edc64pages != NULL) {
		vmem_free(static_alloc_arena,
			  os->edc64pages,
			  os->edc64len);
		os->edc64pages = NULL;
		os->edc64len = 0;
	}

	/* Free parameter list */
	kmem_free(os->edc, os->edclen);
	os->edclen = 0;
	os->edc = NULL;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- OSA_clear.                                        */
/*                                                                  */
/* Function	- Clear out any queued packets			    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
OSA_clear(OSA_state *os)
{
	caddr_t pkt;
	OSAPL(netpl, npl);
	int 	rc;
	int 	cc;

#if OSA_DEBUG
	if (OSA_debug & OSATRACE) {
		prom_printf(
			OSA_NAME "_clear(%p)\n",
			os);
	}
#endif

	/* Get memory for incoming packet (allow for jumbo frames???) */
	pkt = vmem_xalloc(static_alloc_arena, PAGESIZE,
			  PAGESIZE, 0, 0, NULL, NULL, VM_SLEEP);
	if (pkt == NULL) {
		return;
	}

	/* Ensure reserved fields are clear */
	bzero(npl, sizeof(*npl));

	/* We only support 1 packet per request for now */
	npl->entries = 1;
	npl->drbs[0].addr = va_to_pa(pkt);

	/* Receive all queued packets and throw away */
	do {
		npl->drbs[0].count = PAGESIZE;
		cc = OSA_diag(os, OSA_OP_RDR, npl, &rc);
	} while (cc == 0 && rc == 4);

	/* Free the packet memory */
	vmem_free(static_alloc_arena, pkt, PAGESIZE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- OSA_diag.                                         */
/*                                                                  */
/* Function	- Call virtual NIC DIAG.                            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
OSA_diag(OSA_state *os, int op, void *pl, int *rc)
{
	int cc;

#if OSA_DEBUG
	if (OSA_debug & OSATRACE) {
		prom_printf(
			OSA_NAME "_diag(%p, %x, %p)\n",
			os, op, pl);
	}
#endif

	/* Issue DIAG 2A8 */
	__asm__("    lgr   1,%2\n"
		"    diag  1,%3,0x2a8\n"
		"    lgr   %0,2\n"
		"    lghi  %1,1\n"
		"    ipm   %1\n"
		"    srlg  %1,%1,28"
		: "=r" (*rc), "=r" (cc)
		: "r" (va_to_pa(pl)), "r" (op | os->devno)
		: "1", "2", "cc");

#if OSA_DEBUG
	if (OSA_debug & OSATRACE) {
		if (cc != 0) {
			prom_printf(
				OSA_NAME "%d: _diag rc %d cc %d\n",
				os->macinfo->gldm_ppa,
				*rc, cc);
		}
	}
#endif

	return (cc);
}

/*========================= End of Function ========================*/
