/*------------------------------------------------------------------*/
/* 								    */
/* Name        - ccwnex.c					    */
/* 								    */
/* Function    - CCW bus nexus driver.                              */
/* 								    */
/* Name	       - Leland Lucius					    */
/* 								    */
/* Date        - October, 2007  				    */
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

#define DC_ENDT	0xff
#define DT_ENDT	0xff
#define IOPIL_0	0
#define IOPIL_1	2
#define IOPIL_2	5
#define IOPIL_3	6
#define IOPIL_4	7

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
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
#include <sys/archsystm.h>
#include <sys/ios390x.h>
#include <sys/ccw.h>
#include <vm/vm_dep.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

typedef struct _devTable {
	uint8_t	 class;		/* Device class			    */
	uint8_t  type;		/* Device type			    */
	short	 count;		/* Count of compat entries	    */
	short	 isc;		/* Interrupt subclass	 	    */
	int32_t  *instance;	/* Instance number		    */
	char	 *name;		/* Device name			    */
	char	 *compat[2];	/* Driver compatability list	    */
} devTable;

typedef struct ccw_state {
	ddi_intr_handle_t *ih;	/* I/O interrupt                    */
	ddi_intr_handle_t *mh;	/* Machine check interrupt          */
	ddi_taskq_t *tq;        /* Machine check task               */

	uint_t flag_disable_io_intr : 1;
	uint_t flag_remove_io_handler : 1;
	uint_t flag_free_io_intr : 1;
	uint_t flag_free_io_handle : 1;

	uint_t flag_disable_mchk_intr : 1;
	uint_t flag_remove_mchk_handler : 1;
	uint_t flag_free_mchk_intr : 1;
	uint_t flag_free_mchk_handle : 1;
} ccw_state;

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

/*
 * The "dma31_arena" guarantees the returned virtual memory is backed
 * by contiguous, 31-bit (below the line) real memory.  This is used
 * for objects like CCWs and MIDAWS.
 */
extern vmem_t *dma31_arena;

/*
 * The "dma64_arena" guarantees the returned virtual memory is backed
 * by contiguous real memory.
 */
extern vmem_t *dma64_arena;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static int ccw_attach(dev_info_t *devi,
		      ddi_attach_cmd_t cmd);
static int ccw_detach(dev_info_t *devi,
		      ddi_detach_cmd_t cmd);

static int ccw_bus_ctlops(dev_info_t *dip,
			  dev_info_t *rdip,
			  ddi_ctl_enum_t ctlop,
			  void *arg,
			  void *result);
static int ccw_bus_config(dev_info_t *parent,
			  uint_t flags,
			  ddi_bus_config_op_t op,
			  void *arg,
			  dev_info_t **childp);
static int ccw_bus_unconfig(dev_info_t *parent,
			    uint_t flags,
			    ddi_bus_config_op_t op,
			    void *arg);

static uint_t ccw_bus_intr(caddr_t arg1,
			   caddr_t arg2);

static dev_info_t *ccw_find_dev_by_schid(dev_info_t *dip,
					 uint32_t schid);

static uint_t ccw_bus_mchk_intr(caddr_t arg1,
			        caddr_t arg2);
static void ccw_bus_mchk_task(void *arg);

static void ccw_build_nodes(dev_info_t *dip);
static int ccw_check_device(dev_info_t *dip,
			    uint32_t schid,
			    struct schib *sib);
static dev_info_t *ccw_create_device(dev_info_t *dip,
				     uint32_t schid,
				     struct schib *sib);
static int ccw_find_devmap(uint8_t class,
			   uint8_t type);

static int ccw_unitname(dev_info_t *dip, char *buf, int len);

void ccw_dump_irb(struct irb *irb);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

struct bus_ops ccw_bus_ops = {
	BUSO_REV,		/* busops_rev */
	NULL,			/* bus_map */
	NULL,			/* bus_get_intrspec */
	NULL,			/* bus_add_intrspec */
	NULL,			/* bus_remove_intrspec */
	NULL,			/* bus_map_fault */
	ddi_no_dma_map,		/* bus_dma_map */
	ddi_no_dma_allochdl,	/* bus_dma_allochdl */
	ddi_no_dma_freehdl,	/* bus_dma_freehdl */
	ddi_no_dma_bindhdl,	/* bus_dma_bindhdl */
	ddi_no_dma_unbindhdl,	/* bus_dma_unbindhdl */
	ddi_no_dma_flush,	/* bus_dma_flush */
	ddi_no_dma_win,		/* bus_dma_win */
	ddi_no_dma_mctl,	/* bus_dma_ctl */
	ccw_bus_ctlops,		/* bus_ctl */
	ddi_bus_prop_op,	/* bus_prop_op */
	NULL,			/* bus_get_eventcookie */
	NULL,			/* bus_add_eventcall */
	NULL,			/* bus_remove_eventcall */
	NULL,			/* bus_post_event */
	NULL,			/* bus_intr_ctl */
	ccw_bus_config,		/* bus_config */
	ccw_bus_unconfig,	/* bus_unconfig */
	NULL,			/* bus_fm_init */
	NULL,			/* bus_fm_fini */
	NULL,			/* bus_fm_access_enter */
	NULL,			/* bus_fm_access_exit */
	NULL,			/* bus_power */
	i_ddi_intr_ops,		/* bus_intr_op */
};

struct dev_ops ccw_devops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	ddi_no_info,		/* devo_getinfo */
	nulldev,		/* devo_identify */
	nulldev,		/* devo_probe */
	ccw_attach,		/* devo_attach */
	ccw_detach,		/* devo_detach */
	nulldev,		/* devo_reset */
	NULL,			/* devo_cb_ops */
	&ccw_bus_ops,		/* devo_bus_ops */
	nulldev			/* devo_power */
};

/*
 * Module linkage information for the kernel.
 */

static struct modldrv modldrv = {
	&mod_driverops,		/* type of module */
	"CCW nexus driver",
	&ccw_devops,		/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1,		/* module revision */
	&modldrv,		/* struct modldrv */
	NULL			/* must be NULL */
};

/*
 * Our instance states...will only ever have one
 */
static void *ccw_state_head;

/*
 * Level for controlling message load
 */
int ccw_msglevel = DCCW_L3;

/*------------------------------------------------------------------*/
/* Instance counters for various device classes			    */
/*------------------------------------------------------------------*/
static int32_t consinst = 0;
static int32_t grafinst = 0;
static int32_t tapeinst = 0;
static int32_t osadinst = 0;
static int32_t dasdinst = 0;

/*------------------------------------------------------------------*/
/* Device driver to device type lookup table			    */
/*------------------------------------------------------------------*/
static devTable devMap[] = {
	{DC_CONS, DT_3215, 1, IOPIL_4, &consinst, "cnsl", {"con3215", NULL}},
	{DC_CONS, DT_ENDT, 0, IOPIL_0, NULL, NULL, {NULL, NULL}},
	{DC_GRAF, DT_3277, 1, IOPIL_0, &grafinst, "graf", {"term3270", NULL}},
	{DC_GRAF, DT_3278, 1, IOPIL_0, &grafinst, "graf", {"term3270", NULL}},
	{DC_GRAF, DT_ENDT, 0, IOPIL_0, NULL, NULL, {NULL, NULL}},
	{DC_URIN, DT_ENDT, 0, IOPIL_0, NULL, NULL, {NULL, NULL}},
	{DC_UROT, DT_ENDT, 0, IOPIL_0, NULL, NULL, {NULL, NULL}},
	{DC_TAPE, DT_3480, 1, IOPIL_1, &tapeinst, "mt", {"tape3480", NULL}},
	{DC_TAPE, DT_3590, 1, IOPIL_1, &tapeinst, "mt", {"tape3590", NULL}},
	{DC_TAPE, DT_NTAP, 1, IOPIL_1, &tapeinst, "mt", {"tapenew", NULL}},
	{DC_TAPE, DT_ENDT, 0, IOPIL_0, NULL, NULL, {NULL, NULL}},
	{DC_DASD, DT_3390, 1, IOPIL_3, &dasdinst, "dasd", {"diag250", NULL}},
	{DC_DASD, DT_ENDT, 0, IOPIL_0, NULL, NULL, {NULL, NULL}},
	{DC_SPEC, DT_OSAD, 1, IOPIL_2, &osadinst, "osa", {"osa", NULL}},
	{DC_SPEC, DT_ENDT, 0, IOPIL_0, NULL, NULL, {NULL, NULL}},
	{DC_FBAD, DT_9336, 1, IOPIL_3, &dasdinst, "dasd", {"diag250", NULL}},
	{DC_SPEC, DT_ENDT, 0, IOPIL_0, NULL, NULL, {NULL, NULL}},
	{DC_ENDT, DT_ENDT, 0, IOPIL_0, NULL, NULL, {NULL, NULL}}
};

/*====================== End of Global Variables ===================*/

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
	DCCW(DCCW_L1, "ccwnex: _info()\n");

	return mod_info(&modlinkage, modinfop);
}

/*========================= End of Function ========================*/

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
	int rv;

	DCCW(DCCW_L1, "ccwnex: _init()\n");

	rv = ddi_soft_state_init(&ccw_state_head,
				 sizeof (ccw_state),
				 1);
	if (rv != 0) {
		cmn_err(CE_WARN,
			"Unable to initialize state");
		return (rv);
	}

	rv = mod_install(&modlinkage);
	if (rv != 0) {
		cmn_err(CE_WARN,
			"Unable to install module");
		ddi_soft_state_fini(&ccw_state_head);
		return -1;
	}

	return 0;
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
	int rv;

	DCCW(DCCW_L1, "ccwnex: _fini()\n");

	rv = mod_remove(&modlinkage);
	if (rv != 0) {
		return (rv);
	}

	ddi_soft_state_fini(&ccw_state_head);

	return 0;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_attach.                                       */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
ccw_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	ccw_state *state = NULL;
	int regs[] = {0, 0, 0};
	int intr[] = {IOPIL_0, IOPIL_1, IOPIL_2, IOPIL_3, IOPIL_4};
	int instance = ddi_get_instance(dip);
	int count;
	int rv;

	DCCW(DCCW_L1, "ccw_attach(%p, %d)\n", dip, cmd);

	if (cmd != DDI_ATTACH) {
		goto error;
	}

	rv = ddi_soft_state_zalloc(ccw_state_head, instance);
	if (rv != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Unable to allocate state for %d",
			instance);
		goto error;
	}

	state = ddi_get_soft_state(ccw_state_head, instance);
	if (state == NULL) {
		cmn_err(CE_WARN,
			"Unable to obtain state for %d",
			instance);
		goto error;
	}

	ddi_set_driver_private(dip, state);

	ndi_prop_update_int_array(DDI_DEV_T_NONE,
				  dip,
				  "reg", 
				  (int *)regs,
				  sizeof(regs) / sizeof(int));
	ndi_prop_update_int_array(DDI_DEV_T_NONE,
				  dip,
				  "interrupts",
				  (int *)intr, 
				  sizeof(intr) / sizeof(int));

	state->ih = kmem_zalloc(sizeof(*state->ih), KM_SLEEP);
	if (state->ih == NULL) {
		cmn_err(CE_WARN,
			"Unable to allocate i/o handle\n");
		goto error;
	}
	state->flag_free_io_handle = 1;

	/* Create task queue */
	state->tq = ddi_taskq_create(dip,
				     "task queue",
				     1,
				     TASKQ_DEFAULTPRI,
				     0);
	if (state->tq == NULL) {
		goto error;
	}

	rv = ddi_intr_alloc(dip,
			    state->ih,
			    DDI_INTR_TYPE_FIXED,
			    S390_INTR_IO,
			    1,
			    &count,
			    DDI_INTR_ALLOC_STRICT);
	if (rv != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Unable to allocate i/o interrupt\n");
		goto error;
	}
	state->flag_free_io_intr = 1;

	rv = ddi_intr_add_handler(state->ih[0],
				  ccw_bus_intr,
				  NULL,
				  NULL);
	if (rv != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Unable to add i/o handler\n");
		goto error;
	}
	state->flag_remove_io_handler = 1;

	rv = ddi_intr_enable(state->ih[0]);
	if (rv != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Unable to enable interrupt\n");
		goto error;
	}
	state->flag_disable_io_intr = 1;

	/*
	 * Setup interrupt to filter machine checks
	 */
	state->mh = kmem_zalloc(sizeof(*state->mh), KM_SLEEP);
	if (state->mh == NULL) {
		cmn_err(CE_WARN,
			"Unable to allocate mchk handle\n");
		goto error;
	}
	state->flag_free_mchk_handle = 1;

	rv = ddi_intr_alloc(dip,
			    state->mh,
			    DDI_INTR_TYPE_FIXED,
			    S390_INTR_MCHK,
			    1,
			    &count,
			    DDI_INTR_ALLOC_STRICT);
	if (rv != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Unable to allocate mchk interrupt\n");
		goto error;
	}
	state->flag_free_mchk_intr = 1;

	rv = ddi_intr_add_handler(state->mh[0],
				  ccw_bus_mchk_intr,
				  (caddr_t) dip,
				  NULL);
	if (rv != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Unable to add mchk handler\n");
		goto error;
	}
	state->flag_remove_mchk_handler = 1;

	rv = ddi_intr_enable(state->mh[0]);
	if (rv != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Unable to enable mchk interrupt\n");
		goto error;
	}
	state->flag_disable_mchk_intr = 1;

	ccw_build_nodes(dip);

	return DDI_SUCCESS;

error:
	if (state == NULL) {
		return (DDI_FAILURE);
	}

	if (state->flag_disable_mchk_intr) {
		ddi_intr_disable(state->ih[0]);
	}

	if (state->flag_remove_mchk_handler) {
		ddi_intr_remove_handler(state->ih[0]);
	}

	if (state->flag_free_mchk_intr) {
		ddi_intr_free(state->ih[0]);
	}

	if (state->flag_free_mchk_handle) {
		kmem_free(state->mh, sizeof(*state->mh));
	}

	if (state->flag_disable_io_intr) {
		ddi_intr_disable(state->ih[0]);
	}

	if (state->flag_remove_io_handler) {
		ddi_intr_remove_handler(state->ih[0]);
	}

	if (state->flag_free_io_intr) {
		ddi_intr_free(state->ih[0]);
	}

	if (state->flag_free_io_handle) {
		kmem_free(state->ih, sizeof(*state->ih));
	}

	if (state->tq) {
		ddi_taskq_wait(state->tq);
		ddi_taskq_destroy(state->tq);
	}

	ddi_set_driver_private(dip, NULL);

	ddi_soft_state_free(state, instance);

	return (DDI_FAILURE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_detach.                                       */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
ccw_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	ccw_state *state = NULL;
	int instance = ddi_get_instance(dip);

	DCCW(DCCW_L1, "ccw_detach(%p, %d)", dip, cmd);

	state = ddi_get_soft_state(ccw_state_head, instance);
	if (state == NULL) {
		return DDI_SUCCESS;
	}

	if (state->flag_disable_mchk_intr) {
		ddi_intr_disable(state->mh[0]);
	}

	if (state->flag_remove_mchk_handler) {
		ddi_intr_remove_handler(state->mh[0]);
	}

	if (state->flag_free_mchk_intr) {
		ddi_intr_free(state->mh[0]);
	}

	if (state->flag_disable_io_intr) {
		ddi_intr_disable(state->ih[0]);
	}

	if (state->flag_remove_io_handler) {
		ddi_intr_remove_handler(state->ih[0]);
	}

	if (state->flag_free_io_intr) {
		ddi_intr_free(state->ih[0]);
	}

	if (state->tq) {
		ddi_taskq_wait(state->tq);
		ddi_taskq_destroy(state->tq);
	}

	ddi_set_driver_private(dip, NULL);

	ddi_soft_state_free(state, instance);

	return DDI_SUCCESS;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name         - ccw_bus_ctlops.                                   */
/*                                                                  */
/* Function	- Implement the ctlops function for the driver.     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static int
ccw_bus_ctlops(dev_info_t *dip,
	       dev_info_t *rdip,
               ddi_ctl_enum_t ctlop,
	       void *arg,
	       void *result)
{
	DCCW(DCCW_L1, "ccw_bus_ctlops(%p, %p, %d, %p, %p)\n",
			 dip, rdip, ctlop, arg, result);

	switch (ctlop) {
	case DDI_CTLOPS_REPORTDEV:
		if (rdip == (dev_info_t *)0) {
			return (DDI_FAILURE);
		}
		cmn_err(CE_CONT, "?CCW-device: %s@%s, %s%d\n",
			ddi_node_name(rdip),
			ddi_get_name_addr(rdip),
			ddi_driver_name(rdip),
			ddi_get_instance(rdip));
		return DDI_SUCCESS;

	case DDI_CTLOPS_INITCHILD:
	{
		dev_info_t *cdip = (dev_info_t *)arg;
		char unit[MAXNAMELEN];
		int rc;

		rc = ccw_unitname(cdip, unit, sizeof(unit));
		if (rc != DDI_SUCCESS) {
			return DDI_FAILURE;
		}

		ddi_set_name_addr(cdip, unit);

		if (ndi_dev_is_persistent_node(cdip) == 0) {
			rc = ndi_merge_node(cdip, ccw_unitname);
			if (rc == DDI_SUCCESS) {
				return DDI_FAILURE;
			}
		}

		return DDI_SUCCESS;
	}

	case DDI_CTLOPS_UNINITCHILD:
	{
		dev_info_t *cdip = (dev_info_t *)arg;

		ddi_set_name_addr(cdip, NULL);
		impl_ddi_sunbus_removechild(cdip);

		return DDI_SUCCESS;
	}

	case DDI_CTLOPS_SIDDEV:
		return DDI_SUCCESS;

	default:
		return ddi_ctlops(dip, rdip, ctlop, arg, result);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_bus_config.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
ccw_bus_config(dev_info_t *parent,
	       uint_t flags,
	       ddi_bus_config_op_t op,
	       void *arg,
	       dev_info_t **childp)
{
	DCCW(DCCW_L1, "ccw_bus_config(%p, %d, %d, %p, %p)\n",
			 parent, flags, op, arg, childp);

	return (ndi_busop_bus_config(parent, flags, op, arg, childp, 0));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_bus_unconfig.                                 */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
ccw_bus_unconfig(dev_info_t *parent,
		 uint_t flags,
		 ddi_bus_config_op_t op,
		 void *arg)
{
	DCCW(DCCW_L1, "ccw_bus_unconfig(%p, %d, %d, %p)\n",
			 parent, flags, op, arg);

	return ndi_busop_bus_unconfig(parent, flags, op, arg);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_bus_intr.                                     */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uint_t
ccw_bus_intr(caddr_t arg1, caddr_t arg2)
{
	intparms *ip = (intparms *)arg2;
	uint32_t schid = ip->u.io.schid;
	dev_info_t *dip;
	ccw_device_req *req;
	ccw_device *cd;
	irb *irb;
	boolean_t unsol;
	int cc;

	DCCW(DCCW_L1, "ccw_bus_intr(%p, %p)\n",
		arg1, arg2);

	DCCW(DCCW_L1, "schid %08x, intparm %08x, %08x)\n",
		schid, ip->u.io.intparm, ip->u.io.idw);

	cd = (ccw_device *) id32_lookup(ip->u.io.intparm);
	if (!cd) {
		return DDI_INTR_UNCLAIMED;
	}
	dip = cd->dip;

	/*
	 * A little explanation of why this lock is needed.
	 *
	 * This routine is called with interrupts enabled, but the CPU that we're
	 * currently running on has had the interrupt priority raised to one step
	 * above the interrupt priority for the device we're servicing.  Therefore,
	 * no interrupts for the device will be serviced on this CPU until we return.
	 *
	 * However, If we're running on a multi-processor system, then it is possible
	 * for this routine to get called multiple times at once since the other
	 * processors can still field interrupts from the same devices.
	 *
	 * And since we want to relieve the driver's handler from dealing with this
	 * burden, we will single thread the interrupt using a mutex.
	 *
	 * Even though we are processing interrupts, it is safe for one processor to
	 * wait for the mutex while the current holder is processing a previous one
	 * since this lock isn't used anywhere outside the handler.
	 *
	 * The mutex must be obtained before calling ccw_device_test() since any
	 * pending interrupts will fire as soon as the function issues the TSCH
	 * instruction.  Obtaining the mutex after calling the function will not
	 * guarantee proper ordering.
	 *
	 * An alternative to this might be to use a taskq to queue the interrupt
	 * for processing after this handler returns.  Since the interrupt handling
	 * would then run at normal dispatching priority and within a separate
	 * thread, there'd be less of a concern about blocking in the device's
	 * interrupt handler.  And a single thread taskq (per device) would
	 * guarantee proper ordering of interrupt delivery.
	 */
	mutex_enter(&cd->intrlock);

	cc = ccw_device_test(cd);

	irb = ccw_device_get_irb(cd);

	DCCW(DCCW_L1, "tsch cc = %d stCtl %08x dstat %08x\n", cc, irb->scsw.stCtl, irb->scsw.dstat);

	ccw_dump_irb(irb);

	unsol = (irb->scsw.stCtl == (SCSW_STCTL_ALERT_STATUS | SCSW_STCTL_STATUS_PEND));

	req = NULL;
	if (!unsol) {
		req = cd->active;
		if (req) {
			memcpy(&req->irb, irb, sizeof(*irb));
		}
		cd->active = NULL;
	}

	DCCW(DCCW_L1, "req = %p, dev = %p\n",
		req, cd);

	DCCW(DCCW_L1, "handler %p\n", cd->handler);
	if (cd->handler != NULL) {
		cd->handler(cd, req);
	}

	mutex_exit(&cd->intrlock);

	return DDI_INTR_CLAIMED;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_find_dev_by_schid.                            */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static dev_info_t *
ccw_find_dev_by_schid(dev_info_t *dip, uint32_t schid)
{
	dev_info_t *cdip;
	int cschid;
	int circ;

	ndi_devi_enter(dip, &circ);

	cdip = ddi_get_child(dip);
	while (cdip != NULL) {
		cschid = ddi_prop_get_int(DDI_DEV_T_ANY,
					  cdip,
					  DDI_PROP_DONTPASS,
					  "subchannel-id",
					  -1);
		if (cschid == schid) {
			break;
		}
		cdip = ddi_get_next_sibling(cdip);
	}

	ndi_devi_exit(dip, circ);

	return cdip;
}

/*========================= End of Function ========================*/


/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_bus_mchk_intr.                                */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static uint_t
ccw_bus_mchk_intr(caddr_t arg1, caddr_t arg2)
{
	intparms *ip = (intparms *)arg2;
	dev_info_t *dip = (dev_info_t *) arg1;
	ccw_state *state;
	int instance;
	int cc;

	DCCW(DCCW_L1, "ccw_bus_mchk_intr(%p, %p)\n", arg1, arg2);

	/* We only care about channel reports */
	if (!ip->u.mch.u.mcic.chanRpt) {
		return DDI_INTR_UNCLAIMED;
	}

	instance = ddi_get_instance(dip);

	state = ddi_get_soft_state(ccw_state_head, instance);
	if (state == NULL) {
		cmn_err(CE_WARN,
			"Unable to obtain state for %d",
			instance);
		return DDI_INTR_UNCLAIMED;
	}

	cc = ddi_taskq_dispatch(state->tq,
				ccw_bus_mchk_task,
				dip,
				DDI_NOSLEEP);
	if (cc != 0) {
		return DDI_INTR_UNCLAIMED;
	}



	return DDI_INTR_CLAIMED;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_bus_mchk_task.                                */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
ccw_bus_mchk_task(void *arg)
{
	dev_info_t *dip = (dev_info_t *) arg;

	DCCW(DCCW_L1, "ccw_bus_mchk_task(%p)\n", arg);

	/* Process all reports until no more exist */
	for (;;) {
		dev_info_t *cdip;
		ccw_device *cd;
		crw crw;
		schib sib;
		uint32_t schid;
		int devno;
		int cc;

		/* Retrieve the channel report information */
		__asm__	("	stcrw	%1\n"
			 "	lghi	%0,0\n"
			 "	ipm	%0\n"
			 "	srlg	%0,%0,28\n"
			 : "=r" (cc), "=m" (crw)
			 :
			 : "memory", "cc");

		/* No more channel reports...leave */
		if (cc != 0) {
			break;
		}

		/* Let someone know about it */
		cmn_err(CE_WARN,
			"Channel Report:\n"
			"  Solicited:     %d\n"
			"  Overflow:      %d\n"
			"  Chain:         %d\n"
			"  Source Code:   %02x\n"
			"  Ancilliary:    %d\n"
			"  Recovery Code: %02x\n"
			"  Source ID:     %04x\n",
			crw.sol, crw.over, crw.chain,
			crw.rsc, crw.anc, crw.erc,
			crw.rsid);

		/* We only no how to deal with subchannel reports */
		if (crw.rsc != CRW_RSC_SCHN) {
			cmn_err(CE_WARN,
				"Unrecognized CRW\n");
			continue;
		}

		/* Generate the subchannel ID */
		schid = crw.rsid | 0x00010000;

		/* Find the device node associated with this schid */
		cdip = ccw_find_dev_by_schid(dip, schid);

		/* Get device number */
		devno = -1;
		if (cdip != NULL) {
			devno = ddi_prop_get_int(DDI_DEV_T_ANY,
						 cdip,
						 DDI_PROP_DONTPASS,
						 "unit-address",
						 -1);
		}

		/* Determine status of schid in report */
		cc = ccw_check_device(dip, schid, &sib);

		/*
		 * Determine action to take
		 *
		 * Should we be checking "erc" here to determine exactly
		 * what happened?
		 */
		if (cdip && !cc) {
			/* Device is already defined and status is good */

			/* Subchannel now has a different device address ... can this happen? */
			if (sib.pmcw.dev != devno) {

				/* Remove old device */
				cc = ndi_devi_offline(cdip, NDI_DEVI_REMOVE);
				if (cc != NDI_SUCCESS) {
					cmn_err(CE_WARN,
						"Unable to remove device %04x\n",
						devno);
				}
				else {
					cmn_err(CE_WARN,
						"Device %04x removed\n",
						devno);

					/* Define device with new address */
					cdip = ccw_create_device(dip, schid, &sib);
					if (cdip == NULL) {
						cmn_err(CE_WARN,
							"Unable to define device %04x\n",
							sib.pmcw.dev);
					}
					else {
						cc = ndi_devi_online(cdip, NDI_ONLINE_ATTACH);
						if (cc != NDI_SUCCESS) {
							ndi_devi_offline(cdip, NDI_DEVI_REMOVE);
							cmn_err(CE_WARN,
								"Unable to bring device %04x online\n",
								sib.pmcw.dev);
						}
						else {
							cmn_err(CE_WARN,
								"New device %04x online\n",
								sib.pmcw.dev);
						}
					}
				}
			}
			else {
				cmn_err(CE_WARN,
					"Existing device %04x has become available\n",
					sib.pmcw.dev);
			}
		}
		else if (!cdip && !cc) {
			/* Device was not already defined and status is good. */
			cdip = ccw_create_device(dip, schid, &sib);
			if (cdip == NULL) {
				cmn_err(CE_WARN,
					"Unable to define device %04x\n",
					sib.pmcw.dev);
			}
			else {
				cc = ndi_devi_online(cdip, NDI_ONLINE_ATTACH);
				if (cc != NDI_SUCCESS) {
					ndi_devi_offline(cdip, NDI_DEVI_REMOVE);
					cmn_err(CE_WARN,
						"Unable to bring device %04x online\n",
						sib.pmcw.dev);
				}
				else {
					cmn_err(CE_WARN,
						"New device %04x online\n",
						sib.pmcw.dev);
				}
			}
		}
		else if (cdip && cc) {
			/* Device already defined but status isn't good. */
			cc = ndi_devi_offline(cdip, NDI_DEVI_REMOVE);
			if (cc != NDI_SUCCESS) {
				cmn_err(CE_WARN,
					"Unable to remove device %04x\n",
					devno);
			}
			else {
				cmn_err(CE_WARN,
					"Device %04x removed\n",
					devno);
			}
		}
		else {
			/* Device not yet defined and status isn't good. */
			cmn_err(CE_WARN,
				"Subchannel %04x reported unexpected status\n",
				crw.rsid);
		}
	}

	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_build_nodes.                                  */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
ccw_build_nodes(dev_info_t *dip)
{
	struct schib sib;
	uint32_t schid;
	int sch;
	int cc;

	DCCW(DCCW_L1, "build_nodes(%p)\n", dip);

	for (sch = 0, cc = 0; cc < 3; sch++) {
		schid = sch | 0x00010000;
		cc = ccw_check_device(dip, schid, &sib);
		if (cc == 0) {
			ccw_create_device(dip, schid, &sib);
		}
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_check_device.                                 */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
ccw_check_device(dev_info_t *dip, uint32_t schid, struct schib *sib)
{
	int cc;

	DCCW(DCCW_L1, "check_device(%p, %08x, %p)\n", dip, schid, sib);

	schid |= 0x00010000;

	__asm__ ("	lgr	1,%1\n"
		 "	stsch	0(%2)\n"
		 "	lghi	%0,0\n"
		 "	ipm	%0\n"
		 "	srlg	%0,%0,28\n"
		 : "=r" (cc)
		 : "r" (schid), "r" (sib)
		 : "1", "memory", "cc");

	if (cc == 0) {
		if (!sib->pmcw.dnv && !sib->pmcw.lpm) {
			cc = 1;
		}
	}

	return (cc);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_create_device.                                */
/*                                                                  */
/* Function	- Determine if this is a supported device and if so */
/*		  create a node for it and add some propositions.   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static dev_info_t *
ccw_create_device(dev_info_t *dip, uint32_t schid, struct schib *sib)
{
	struct vrdcblok *rdc;
	dev_info_t *cdip = NULL;
	int iMap;
	int cc;

	DCCW(DCCW_L1, "create_device(%p, %p)\n", dip, sib);

	if (!sib->pmcw.dnv) {
		return NULL;
	}

	rdc = ccw_alloc31(sizeof(*rdc), VM_NOSLEEP);
	if (rdc == NULL) {
		return NULL;
	}

	bzero(rdc, sizeof(*rdc));
	rdc->vrdcdvno = sib->pmcw.dev; 
	rdc->vrdclen  = sizeof(*rdc);

	cc = diag_210(rdc);

	if (cc != 0 && cc != 2) {
		ccw_free31(rdc, sizeof(*rdc));
		return NULL;
	}

	iMap  = ccw_find_devmap(rdc->vrdcvcla, rdc->vrdcvtyp);
	if (iMap == -1) {
		ccw_free31(rdc, sizeof(*rdc));
		return NULL;
	}

	cc = ndi_devi_alloc(dip, devMap[iMap].name, DEVI_SID_NODEID, &cdip);
	if (cdip == NULL) {
		ccw_free31(rdc, sizeof(*rdc));
		return NULL;
	}

	ndi_prop_update_int(DDI_DEV_T_NONE,
			    cdip,
			    "subchannel-id",
			    schid);
	ndi_prop_update_int(DDI_DEV_T_NONE,
			    cdip,
			    "unit-address",
			    rdc->vrdcdvno);
	ndi_prop_update_int(DDI_DEV_T_NONE,
			    cdip,
			    "device-class",
			    rdc->vrdcvcla);
	ndi_prop_update_int(DDI_DEV_T_NONE,
			    cdip,
			    "device-type",
			    rdc->vrdcvtyp);
	ndi_prop_update_int(DDI_DEV_T_NONE,
			    cdip,
			    "interrupt-subclass",
			    devMap[iMap].isc);
	ndi_prop_update_byte_array(DDI_DEV_T_NONE,
				   cdip,
				   "initial-state",
				   (uchar_t *) sib,
				   sizeof(*sib));
	ndi_prop_update_byte_array(DDI_DEV_T_NONE,
				   cdip,
				   "device-characteristics",
				   (uchar_t *) rdc,
				   sizeof(*rdc));

	if (devMap[iMap].count) {
		ndi_prop_update_string_array(DDI_DEV_T_NONE,
					     cdip,
					     "compatible", 
					     (char **)devMap[iMap].compat,
					     devMap[iMap].count);
	}

	ndi_devi_bind_driver(cdip, 0); 

	ccw_free31(rdc, sizeof(*rdc));

	return cdip;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_find_devmap.                                  */
/*                                                                  */
/* Function	- Locate an entry in the devMap table that matches  */
/*		  the device class and type.   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int 
ccw_find_devmap(uint8_t class, uint8_t type)
{
	int iMap;

	DCCW(DCCW_L1, "ccw_find_devmap(%02x, %02x)\n",
		class, type);

	for (iMap = 0; devMap[iMap].class != DC_ENDT; iMap++) {
		if (devMap[iMap].class == class) {
			for (; devMap[iMap].type != DT_ENDT; iMap++) {
				if (devMap[iMap].type == type)
					return(iMap);
			}
			return(-1);
		}
	}
	return(-1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name         - ccw_unitname.                                     */
/*                                                                  */
/* Function	- Generate the unit address string.                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/
static int
ccw_unitname(dev_info_t *dip, char *buf, int len)
{
	int devno;
	int rc;

	DCCW(DCCW_L1, "ccw_unitname(%p, %p, %d)\n",
		dip, buf, len);

	devno = ddi_prop_get_int(DDI_DEV_T_ANY,
				 dip,
				 DDI_PROP_DONTPASS,
				 "unit-address",
				 -1);

	if (devno == -1) {
		return DDI_FAILURE;
	}

	snprintf(buf, len, "0x%04x", devno);

	return DDI_SUCCESS;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_alloc31.                                      */
/*                                                                  */
/* Function	- Allocate virtual memory that's backed by below    */
/*		  the line contiguous real storage.  We bypass the  */
/*		  DDI DMA routines and use the DMA arena directly   */
/*		  as we do not require DMA handles.                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void *
ccw_alloc31(size_t size, int flags)
{
	void *mem;

	DCCW(DCCW_L0, "ccw_alloc31(%ld, %08x)\n",
		size, flags);

	mem = vmem_xalloc(dma31_arena,
			  size,
			  PAGESIZE,
			  0,
			  0,
			  (void *) 0,
			  (void *) 0,
			  flags | VM_NORELOC);

	if (mem != NULL) {
		bzero(mem, size);
	}

	return mem;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_free31.                                       */
/*                                                                  */
/* Function	- Release memory allocated using ccw_alloc31.       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
ccw_free31(void *mem, size_t size)
{
	DCCW(DCCW_L0, "ccw_free31(%p, %ld)\n",
		mem, size);

	vmem_xfree(dma31_arena, mem, size);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_alloc64.                                      */
/*                                                                  */
/* Function	- Allocate virtual memory that's backed by page     */
/*		  aligned contiguous real storage.  We bypass the   */
/*		  DDI DMA routines and use the DMA arena directly   */
/*		  as we do not require DMA handles.                 */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void *
ccw_alloc64(size_t size, int flags)
{
	void *mem;

	DCCW(DCCW_L0, "ccw_alloc64(%ld, %08x)\n",
		size, flags);

	mem = vmem_xalloc(dma64_arena,
			  size,
			  PAGESIZE,
			  0,
			  0,
			  (void *) 0,
			  (void *) 0,
			  flags | VM_NORELOC);

	if (mem != NULL) {
		bzero(mem, size);
	}

	return mem;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_free65.                                       */
/*                                                                  */
/* Function	- Release memory allocated using ccw_alloc64.       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
ccw_free64(void *mem, size_t size)
{
	DCCW(DCCW_L0, "ccw_free64(%p, %ld)\n",
		mem, size);

	vmem_xfree(dma64_arena, mem, size);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _ccw_dprint                                       */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*PRINTFLIKE2*/
void
ccw_dprintf(int l, const char *fmt, ...)
{
	va_list ap;

#ifndef DEBUG
	if (!l) {
		return;
	}
#endif /* DEBUG */
	if (l < ccw_msglevel) {
		return;
	}

	va_start(ap, fmt);
	(void) prom_vprintf(fmt, ap);
	va_end(ap);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_dump_irb.                                     */
/*                                                                  */
/* Function	- Dump contents of an IRB                           */
/*                                                                  */
/*------------------------------------------------------------------*/

void
ccw_dump_irb(struct irb *irb)
{
	int i, j, k;
        uchar_t *p = (uchar_t *) irb;
	erw *erw = &irb->esw.esw1.extRep;

	DCCW(DCCW_L1, "IRB: (%ld)  \n", sizeof(*irb));
	for (i = 0; i < sizeof(*irb); ) {
		DCCW(DCCW_L1, "%04x:", i);
		for (j = 0; j < 4 && i < sizeof(*irb); j++) {
			DCCW(DCCW_L1, "  ");
			for (k = 0; k < 4 && i < sizeof(*irb); k++, i++) {
				DCCW(DCCW_L1, "%02x", *p++);
			}
		}
		DCCW(DCCW_L1, "\n");
	}
	DCCW(DCCW_L1, "\n");

	DCCW(DCCW_L1, "    scsw:\n");
	DCCW(DCCW_L1, "        key:    %d\n", irb->scsw.key);
	DCCW(DCCW_L1, "        sctl:   %d\n", irb->scsw.sCtl);
	DCCW(DCCW_L1, "        eswf:   %d\n", irb->scsw.eswf);
	DCCW(DCCW_L1, "        cc:     %d\n", irb->scsw.cc);
	DCCW(DCCW_L1, "        fmt:    %d\n", irb->scsw.fmt);
	DCCW(DCCW_L1, "        pref:   %d\n", irb->scsw.pref);
	DCCW(DCCW_L1, "        isic:   %d\n", irb->scsw.isic);
	DCCW(DCCW_L1, "        alcc:   %d\n", irb->scsw.alcc);
	DCCW(DCCW_L1, "        ssi:    %d\n", irb->scsw.ssi);
	DCCW(DCCW_L1, "        zcc:    %d\n", irb->scsw.zcc);
	DCCW(DCCW_L1, "        eetl:   %d\n", irb->scsw.eEtl);
	DCCW(DCCW_L1, "        pnop:   %d\n", irb->scsw.pnop);
	DCCW(DCCW_L1, "        fctl:   %02x\n", irb->scsw.fCtl);
	DCCW(DCCW_L1, "        actl:   %02x\n", irb->scsw.aCtl);
	DCCW(DCCW_L1, "        stctl:  %02x\n", irb->scsw.stCtl);
	DCCW(DCCW_L1, "        cpa:    %08x\n", irb->scsw.cpa);
	DCCW(DCCW_L1, "        dstat:  %02x\n", irb->scsw.dstat);
	DCCW(DCCW_L1, "        cstat:  %02x\n", irb->scsw.cstat);
	DCCW(DCCW_L1, "        count:  %d\n", irb->scsw.count);

	DCCW(DCCW_L1, "    erw:\n");
	DCCW(DCCW_L1, "        rlo:    %d\n", erw->rlo);
	DCCW(DCCW_L1, "        eslp:   %d\n", erw->eslp);
	DCCW(DCCW_L1, "        ac:     %d\n", erw->ac);
	DCCW(DCCW_L1, "        prv:    %d\n", erw->pvr);
	DCCW(DCCW_L1, "        cpt:    %d\n", erw->cpt);
	DCCW(DCCW_L1, "        fsav:   %d\n", erw->fsav);
	DCCW(DCCW_L1, "        cs:     %d\n", erw->cs);
	DCCW(DCCW_L1, "        sccwav: %d\n", erw->sccwav);
	DCCW(DCCW_L1, "        cscnt:  %d\n", erw->cscnt);
}

/*========================= End of Function ========================*/

