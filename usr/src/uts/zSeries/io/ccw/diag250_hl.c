/*------------------------------------------------------------------*/
/* 								    */
/* Name        - diag250_hl.c 					    */
/* 								    */
/* Function    - Interface diag250 DASD driver w/DDI stack.         */
/* 								    */
/* Name	       - Adam Thornton					    */
/*               Leland Lucius                                      */
/* 								    */
/* Date        - September, 2007				    */
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

#define D250_NAME "dasd"

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/open.h>
#include <sys/cred.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/ksynch.h>
#include <sys/modctl.h>
#include <sys/conf.h>
#include <sys/devops.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <sys/dktp/fdisk.h>
#include <sys/queue.h>
#include <sys/blockio.h>
#include <sys/ccw.h>
#include <sys/diag250_ll.h>
#include <sys/exts390x.h>
#include <sys/devinit.h>
#include <sys/machs390x.h>
#include <sys/archsystm.h>
#include <sys/intr.h>
#include <sys/vnode.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
/* Disk - Vol1 Label						    */
/*------------------------------------------------------------------*/
typedef struct _dsklabel {
	uint32_t ident;		/* Label identifier		    */
#define CMS1	0xc3d4e2f1	/* Disk label identifier we support */

	uchar_t  id[6];		/* Volume identifier		    */
	uint16_t vers;		/* Version level		    */
	uint32_t blocksize;	/* Blocksize			    */
	uint32_t origin;	/* Disk origin pointer		    */
	uint32_t cyl;		/* Number of formatted cylinders    */
	uint32_t max_cyl;	/* Max. no. of formatted cylinders  */
	uint32_t n_blocks;	/* Number of blocks assigned to disk*/
	uint32_t used;		/* Number of blocks used 	    */
	uint32_t fstsize;	/* Size of FST 			    */
	uint32_t n_fst;		/* Number of FSTs per block	    */
	uchar_t	 cre8date[6];	/* Date of creation 		    */
	uchar_t  flag;		/* Label flag			    */
#define ADCNTRY	0x01		/* Century for disk creation date   */
	uchar_t	 resvd;		/* Reserved			    */
	uint32_t offset;	/* Disk offset when reserved	    */
} dsklabel_t;

/*------------------------------------------------------------------*/
/* DIAG 0xa8 - I/O Parameter List				    */
/*------------------------------------------------------------------*/
typedef struct _sgiop {
	uint16_t devno;		/* Device number		    */
	uchar_t  key;		/* Storage key			    */
	uchar_t	 flags;		/* Flag				    */
#define F1CCW	0x80		/* Format 1 CCWs being used	    */
#define F2IDAW  0x02		/* Format 2 IDAW being used	    */
#define F22K	0x01		/* 2K blocksize format 2 IDAWs	    */
	uint32_t resv0;		/* Reserved			    */
	uint32_t cpa;		/* Channel program address	    */
	uint32_t resv1;		/* Reserved			    */
	uchar_t  devstat;	/* Device status		    */
	uchar_t  schstat;	/* Subchannel status		    */
	uint16_t resid;		/* Residual count		    */
	uchar_t  lpm;		/* Logical path mask		    */
	uchar_t  resv2[5];	/* Reserved			    */
	uint16_t sensect;	/* Sense byte count		    */
	uint32_t resv3[6];	/* Reserved			    */
	uchar_t	 sense[32];	/* Sense data			    */
} __attribute__ ((packed,aligned(8))) sgiop_t;

/*------------------------------------------------------------------*/
/* FBA - Define Extent Parameter List				    */
/*------------------------------------------------------------------*/
typedef struct _defExtFba {
	struct {
		uchar_t perm:2;	/* Extent permission		    */
#define DFXF_WRITE	0x00	/* Write permission		    */
#define DFXF_READ	0x01	/* Read permission		    */
#define DFXF_CTL	0x02	/* Control permission		    */
		uchar_t xxx:2;  /* Reserved			    */
		uchar_t da:1;	/* Device Access		    */
		uchar_t diag:1; /* Allow diagnose		    */
		uchar_t yyy:2;	/* Reserved			    */
	} __attribute__ ((packed)) mask;
	uint8_t	 xxxx;		/* Reserved			    */
	uint16_t blksize;	/* Blocksize			    */
	uint32_t extLoc;	/* Extent locator		    */
	uint32_t extBeg;	/* Logical block 0		    */
	uint32_t extEnd;	/* Logical block end		    */
} __attribute__ ((packed,aligned(8))) defExtFba_t;

/*------------------------------------------------------------------*/
/* FBA - Locate Parameter List       				    */
/*------------------------------------------------------------------*/
typedef struct _locateFba {
	struct {
		uchar_t xxx:4;	/* Reserved			    */
		uchar_t cmd:4;	/* Command			    */
	} __attribute__ ((packed)) op;
	uint8_t	aux;
	uint16_t blkCnt;	/* Block count			    */
	uint32_t blkNr;		/* Block number			    */
} __attribute__ ((packed)) locateFba_t;

/*------------------------------------------------------------------*/
/* ECKD - Cylinder Head (CCHH) Specification			    */
/*------------------------------------------------------------------*/
typedef struct _cchh {
	uint16_t cyl;		/* Cylinder			    */
	uint16_t head;		/* Head				    */
} __attribute__ ((packed)) cchh_t;

/*------------------------------------------------------------------*/
/* ECKD - Cylinder Head Record (CCHHR) Specification		    */
/*------------------------------------------------------------------*/
typedef struct _cchhr {
	uint16_t cyl;		/* Cylinder			    */
	uint16_t head;		/* Head				    */
	uint8_t  rec;		/* Record			    */
} __attribute__ ((packed)) cchhr_t;

/*------------------------------------------------------------------*/
/* ECKD - Define Extent Parameter List				    */
/*------------------------------------------------------------------*/
typedef struct _defExtEckd {
	struct {
		uchar_t perm:2;	/* Extent permissions		    */
#define DFXE_NORM	0x00	/* Normal permissions		    */
#define DFXE_WRITE	0x02	/* Write permission		    */
#define DFXE_CTL	0x03	/* Control permission		    */
		uchar_t xxx:1;	/* Reserved			    */
		uchar_t seek:2; /* Seek control			    */
		uchar_t auth:2; /* Access authorization		    */
		uchar_t pci:1;	/* PCI fetch mode		    */
	} __attribute__ ((packed)) mask;
	struct {
		uchar_t mode:2;	/* Architecture mode		    */
#define DFX_ECKD	0x03	/* ECKD mode			    */
		uchar_t ckdc:1;	/* CKD conversion		    */
		uchar_t	op:3;	/* Operation mode		    */
		uchar_t cfw:1;	/* Cache fast write		    */
		uchar_t dfw:1;	/* Disk fast write		    */
	} __attribute__ ((packed)) attr;
	uint16_t blkSize;	/* Block size			    */
	uint16_t fwId;		/* Fast write identifier	    */
	uint8_t  ga_add;	/* Global attributes - additional   */
	uint8_t  ga_ext;	/* Global attributes - extended     */
	cchh_t	 begExt;	/* Beginning Extent		    */
	cchh_t	 endExt;	/* Ending Extent		    */
} __attribute__ ((packed)) defExtEckd_t;

/*------------------------------------------------------------------*/
/* ECKD - Locate Record Parameter List 				    */
/*------------------------------------------------------------------*/
typedef struct _locateRec {
	struct {
		uchar_t ornt:2;	/* Orientation			    */
		uchar_t code:6;	/* Operation			    */
#define LRDOCOR	0x00		/* Orient			    */
#define LRDOCWD 0x01		/* Write data			    */
#define LRDOCFW 0x03		/* Format write			    */
#define LRDOCRD 0x06		/* Read data			    */
#define LRD0CWT 0x0b		/* Write track			    */
#define LRDOCRT 0x0c		/* Read track			    */
#define LRD0CR  0x16		/* Read				    */
	} __attribute__ ((packed)) op;
	struct {
		uchar_t	last:1;	/* Last bytes used		    */
		uchar_t xxx:6;	/* Reserved			    */
		uchar_t rcsf:1; /* Read count suffix		    */
	} __attribute__ ((packed)) aux;
	uint8_t  xxx;		/* Reserved			    */
	uint8_t  count;		/* Count			    */
	cchh_t   seek;		/* Seek argument		    */
	cchhr_t  search;	/* Search argument		    */
	uint8_t	 sector;	/* Sector			    */
	uint16_t length;	/* Length 			    */
} __attribute__ ((packed)) locateRec_t;

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern vmem_t *static_alloc_arena;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static int  diag250_getinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static int  diag250_attach(dev_info_t *, ddi_attach_cmd_t);
static int  diag250_detach(dev_info_t *, ddi_detach_cmd_t);
static int  diag250_open(dev_t *, int, int, cred_t *);
static int  diag250_close(dev_t, int, int, cred_t *);
static int  diag250_strategy(struct buf *);
static int  diag250_print(dev_t, char *);
static int  diag250_read(dev_t, struct uio *, cred_t *);
static int  diag250_write(dev_t, struct uio *, cred_t *);
static int  diag250_aread(dev_t, struct aio_req *, cred_t *);
static int  diag250_awrite(dev_t, struct aio_req *, cred_t *);
static int  diag250_ioctl(dev_t, int, intptr_t, int, cred_t *, int *);
static boolean_t diag250_init_queues(diag250_dev_t *dp);
static void diag250_free_queues(diag250_dev_t *dp);
static int  diag250_get_label(ccw_device *, diag250_dev_t *);
static void diag250_fake_geometry(diag250_dev_t *);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

static void *diag250_state_head;

static struct cb_ops diag250_cb_ops = {
	diag250_open, 		/* cb_open */
	diag250_close, 		/* cb_close */
	diag250_strategy,	/* cb_strategy */
	diag250_print,		/* cb_print */
	nodev, 			/* cb_dump */
	diag250_read, 		/* cb_read */
	diag250_write, 		/* cb_write */
	diag250_ioctl, 		/* cb_ioctl */
	nodev, 			/* cb_devmap */
	nodev, 			/* cb_mmap */
	nodev, 			/* cb_segmap */
	nochpoll, 		/* cb_chpoll */
	ddi_prop_op, 		/* cb_prop_op */
	(struct streamtab *)NULL,/*cb_str */
	D_MP | D_NEW |  D_64BIT,/* cb_flag */
	CB_REV, 		/* cb_rev */
	diag250_aread, 		/* cb_aread */
	diag250_awrite		/* cb_awrite */
};

static struct dev_ops diag250_dev_ops = {
	DEVO_REV, 		/* devo_rev */
	0, 			/* devo_refcnt */
	diag250_getinfo, 	/* devo_getinfo */
	nulldev, 		/* devo_identify */
	nulldev, 		/* devo_probe */
	diag250_attach, 	/* devo_attach */
	diag250_detach, 	/* devo_detach */
	nodev, 			/* devo_reset */
	&diag250_cb_ops, 	/* devo_cb_ops */
	(struct bus_ops *)NULL, /* devo_bus_ops */
	nulldev 		/* devo_power */
};

static struct modldrv modldrv = {
	&mod_driverops,
	"DIAG 250 1.0",
	&diag250_dev_ops};

static struct modlinkage modlinkage = {
	MODREV_1,
	(void *)&modldrv,
	NULL
};

int diag250_debug = 0;

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
	int rc;

	rc = ddi_soft_state_init(&diag250_state_head,
				 sizeof(diag250_dev_t),
				 1);
	if (rc != 0) {
		return (rc);
	}

	rc = mod_install(&modlinkage);
	if (rc != 0) {
		ddi_soft_state_fini(&diag250_state_head);
	}

	return (rc);
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
	int rc;

	rc = mod_remove(&modlinkage);
	if (rc != 0) {
		return (rc);
	}

	ddi_soft_state_fini(&diag250_state_head);

	return (rc);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_getinfo.                                  */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static int
diag250_getinfo(dev_info_t *dip, ddi_info_cmd_t cmd, void *arg, void **resultp)
{
	int instance = getminor((dev_t) arg);
	diag250_dev_t *dp;
	int rc = DDI_FAILURE;

#if DASD_DEBUG
	if (diag250_debug & D_D250MISC)
		prom_printf("diag250_getinfo called for %p with cmd: %x\n",
			    dip, cmd);
#endif

	ASSERT(resultp != NULL);

	switch (cmd) {
	case DDI_INFO_DEVT2DEVINFO:
		dp = ddi_get_soft_state(diag250_state_head, instance);
		if (dp != NULL) {
			*resultp = dp->devi;
			rc = DDI_SUCCESS;
		}
		else {
			*resultp = NULL;
		}
		break;
	case DDI_INFO_DEVT2INSTANCE:
		*resultp = (void *) (uintptr_t) instance;
		rc = DDI_SUCCESS;
		break;
	}

	return (rc);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_attach.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
diag250_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	int instance = ddi_get_instance(dip);
	ccw_device *cd;
	diag250_dev_t *dp;
	int rc;
	int count;
	int64_t size;
	dev_t fulldev = makedevice(ddi_driver_major(dip), instance);

#if DASD_DEBUG
	if (diag250_debug & D_D250MISC)
		prom_printf("diag250_attach called for instance %d with cmd: %x\n",
			    instance, cmd);
#endif

	cd = ccw_device_register(dip);
	if (cd == NULL) {
		prom_printf("NULL device ptr in diag250 %d instance\n",
			instance);
		return (DDI_FAILURE);
	}

	switch (cmd) {
	case DDI_ATTACH:
		break;
	case DDI_RESUME:
		return (DDI_SUCCESS);
	default:
		return (DDI_FAILURE);
	}

	rc = ddi_soft_state_zalloc(diag250_state_head, instance);
	if (rc != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Unable to allocate state for %d",
			instance);
		return (DDI_FAILURE);
	}

	dp = ddi_get_soft_state(diag250_state_head, instance);
	if (dp == NULL) {
		cmn_err(CE_WARN,
			"Unable to obtain state for %d",
			instance);
		goto error;
	}
	dp->cd = cd;

	rc = diag250_get_label(cd, dp);
	if (rc != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Disk %d is not valid for I/O\n",
			instance);
		goto error;
	}

	dp->ih = kmem_zalloc(sizeof(*dp->ih), KM_SLEEP);
	if (dp->ih == NULL) {
		cmn_err(CE_WARN,
			"Unable to allocate interrupt handle");
		goto error;
	}

	rc = ddi_intr_alloc(dip,
			    dp->ih,
			    DDI_INTR_TYPE_FIXED,
			    S390_INTR_EXT,
			    1,
			    &count,
			    DDI_INTR_ALLOC_STRICT);
	if (rc != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Unable to allocate external interrupt");
		goto error;
	}
	dp->flags |= DIDINTR;

	rc = ddi_intr_add_handler(dp->ih[0],
				  diag250_ll_intr,
				  (caddr_t) dip,
				  NULL);
	if (rc != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Unable to add interrupt handler");
		goto error;
	}
	dp->flags |= DIDADDH;

	rc = ddi_intr_enable(dp->ih[0]);
	if (rc != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Unable to enable interrupt");
		goto error;
	}
	dp->flags |= DIDENAB;

	/* Create block node */
	rc = ddi_create_minor_node(dip, "dasd",  S_IFBLK, instance,
				   DDI_NT_BLOCK, 0);
	if (rc != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Unable to create block node for %d",
			instance);
		goto error;
	}
	dp->flags |= DIDMINOR;

	/* Create character node */
	rc = ddi_create_minor_node(dip, "dasd,raw",  S_IFCHR, instance,
				   DDI_NT_BLOCK, 0);
	if (rc != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Unable to create character node for %d",
			instance);
		goto error;
	}

	/* Create task queue */
	dp->tq = ddi_taskq_create(dip,
				  "request queue",
				  1,
				  TASKQ_DEFAULTPRI,
				  0);
	if (dp->tq == NULL) {
		goto error;
	}

	/* Get minidisk cache setting */
	dp->usemdc = ddi_prop_get_int(DDI_DEV_T_ANY, dip,
				      DDI_PROP_DONTPASS | DDI_PROP_NOTPROM,
				      "use-minidisk-cache", 1);
	if (dp->usemdc != 0 && dp->usemdc != 1) {
		cmn_err(CE_WARN,
			"use-minidisk-cache %d not 0 or 1, using default",
			dp->usemdc);
		dp->usemdc = 1;
	}

	/* Get queue limit setting */
	dp->qlimit = ddi_prop_get_int(DDI_DEV_T_ANY, dip,
				      DDI_PROP_DONTPASS | DDI_PROP_NOTPROM,
				      "io-queue-limit", D250_QLIMIT);
	if (dp->qlimit < 1 || dp->qlimit > 255) {
		cmn_err(CE_WARN,
			"io-queue-limit %d out of range, using default",
			dp->qlimit);
		dp->qlimit = D250_QLIMIT;
	}

	/* Get bounce buffer limit setting */
	dp->bblimit = ddi_prop_get_int(DDI_DEV_T_ANY, dip,
				       DDI_PROP_DONTPASS | DDI_PROP_NOTPROM,
				       "bounce-buffer-limit", D250_BBLIMIT);
	if (dp->bblimit < 1 || dp->bblimit > 255) {
		cmn_err(CE_WARN,
			"bounce-buffer-limit %d out of range, using default",
			dp->bblimit);
		dp->bblimit = D250_BBLIMIT;
	}

	/* Init the mutexes and condition vars */
	mutex_init(&dp->dotex, NULL, MUTEX_DRIVER, NULL);
	mutex_init(&dp->statex, NULL, MUTEX_DRIVER, NULL);
	mutex_init(&dp->waitex, NULL, MUTEX_DRIVER, NULL);
	cv_init(&dp->waitcv, NULL, CV_DRIVER, NULL);

	dp->flags |= DIDMUTEX;

	/* Save a few things */
	dp->devno	 = cd->rdc.vrdcdvno;
	dp->instance	 = instance;
	dp->devi    	 = dip;

	/* Init the wait queue */
	dp->whead = NULL;
	dp->wtail = &dp->whead;

	/* Init the active queue */
	dp->ahead = NULL;

	/* Create statistics block */
	dp->iostat = kstat_create(ddi_get_name(dip),
				  instance,
				  ddi_get_name_addr(dip),
				  "disk",
				  KSTAT_TYPE_IO,
				  1,
				  KSTAT_FLAG_PERSISTENT);
	if (dp->iostat == NULL) {
		cmn_err(CE_WARN,
			"Insufficient memory...not keeping statistics");
		goto error;
	}
	dp->iostat->ks_lock = &dp->statex;
	kstat_install(dp->iostat);

	/* Pre-init the read/write parameter list */
	dp->syncio.bents = ccw_alloc64(sizeof(blkent_t), VM_NOSLEEP);
	if (dp->syncio.bents == NULL) {
		prom_printf("Unable to alloc bents for syncio\n");
		goto error;
	}
	mutex_init(&dp->syncio.lock, NULL, MUTEX_DRIVER, NULL);
	cv_init(&dp->syncio.wait, NULL, CV_DRIVER, NULL);
	dp->syncio.dp = dp;
	dp->syncio.bcnt = 1;
	dp->syncio.done = B_FALSE;
	dp->syncio.sync = B_TRUE;
	dp->syncio.bents->belrqtyp = BELREAD;
	dp->syncio.rwpl.bioiparm = (uintptr_t) &dp->syncio;
	dp->syncio.rwpl.bioladdr = va_to_pa(dp->syncio.bents);
	dp->syncio.rwpl.bioplhd.biodevn = dp->devno;
	dp->syncio.rwpl.bioplhd.biomode = BIOZAR;
	dp->syncio.rwpl.bioflag = BIOASYN;
	if (!dp->usemdc) {
		dp->syncio.rwpl.bioflag |= BIOBYPAS;
	}

	rc = diag250_init_queues(dp);
	if (rc == B_TRUE) {
		goto error;
	}
	
	rc = diag250_ll_initio(dp);
	if (rc != 0) {
		goto error;
	}

	/*
	 * Expose our block size and count.
	 * (Do this AFTER diag250_ll_initio!)
	 */
	size = (dp->endblock + 1) * dp->blksize;
	rc = ddi_prop_update_int64(fulldev, dip, "Size", size); 
	if (rc != DDI_PROP_SUCCESS) {
		goto error;
	}

	size = (dp->endblock + 1);
	rc = ddi_prop_update_int64(fulldev, dip, "Nblocks", size);
	if (rc != DDI_PROP_SUCCESS) {
		goto error;
	}

	rc = ddi_prop_update_int(fulldev, dip, "blksize", dp->blksize);
	if (rc != DDI_PROP_SUCCESS) {
		goto error;
	}

	rc = ddi_prop_update_int(fulldev, dip, "device-blksize", dp->blksize);
	if (rc != DDI_PROP_SUCCESS) {
		goto error;
	}

	rc = ddi_taskq_dispatch(dp->tq, diag250_ll_iotask, dp, DDI_NOSLEEP);
	if (rc != 0) {
		goto error;
	}

	diag250_fake_geometry(dp);

	ddi_report_dev(dip);

	return (DDI_SUCCESS);

error:	
	(void) diag250_detach(dip, DDI_DETACH);

	return (DDI_FAILURE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_detach.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
diag250_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	int instance = ddi_get_instance(dip);
	diag250_dev_t *dp;
	ccw_device *cd;

#if DASD_DEBUG
	if (diag250_debug & D_D250MISC)
		prom_printf("diag250_detach called for %p with cmd: %x\n",
			    ddi_get_soft_state(diag250_state_head, instance), cmd);
#endif

	switch (cmd) {
	case DDI_DETACH:
		break;
	case DDI_SUSPEND:
		return (DDI_SUCCESS);
	default:
		return (DDI_FAILURE);
	}

	ddi_prop_remove_all(dip);

	dp = ddi_get_soft_state(diag250_state_head, instance);
	if (dp == NULL) {
		return (DDI_SUCCESS);
	}

	if (dp->flags & DIDBUSY) {
		return (EBUSY);
	}

	if (dp->tq != NULL) {
		/* Tell task to end */
		dp->taskstop = B_TRUE;
	
		/* Wake up the I/O task */
		mutex_enter(&dp->waitex);
		dp->wakeup = B_TRUE;
		cv_signal(&dp->waitcv);
		mutex_exit(&dp->waitex);
	
		/* Wait for task to end */
		ddi_taskq_wait(dp->tq);

		ddi_taskq_destroy(dp->tq);
	}

	if (dp->flags & DIDINIT) {
		diag250_ll_termio(dp);
	}

	diag250_free_queues(dp);
	
	if (dp->syncio.bents) {
		cv_destroy(&dp->syncio.wait);
		mutex_destroy(&dp->syncio.lock);
		ccw_free64(dp->syncio.bents, sizeof(blkent_t));
	}

	if (dp->iostat != NULL) {
		kstat_delete(dp->iostat);
	}

	if (dp->flags & DIDMUTEX) {
		cv_destroy(&dp->waitcv);
		mutex_destroy(&dp->waitex);
		mutex_destroy(&dp->statex);
		mutex_destroy(&dp->dotex);
	}

	/* Remove all minor nodes for this instance */
	if (dp->flags & DIDMINOR) {
		ddi_remove_minor_node(dip, NULL);
	}

	if (dp->flags & DIDENAB) {
		ddi_intr_disable(dp->ih[0]);
	}

	if (dp->flags & DIDADDH) {
		ddi_intr_remove_handler(dp->ih[0]);
	}

	if (dp->flags & DIDINTR) {
		ddi_intr_free(dp->ih[0]);
	}

	if (dp->ih != NULL ) {
		kmem_free(dp->ih, sizeof(*dp->ih));
	}

	cd = dp->cd;
	ddi_soft_state_free(diag250_state_head, instance);

	ccw_device_unregister(cd);

	return (DDI_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_open.                                     */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static int
diag250_open(dev_t *devp, int flag, int otyp, cred_t *credp)
{
	int instance = getminor(*devp);
	diag250_dev_t *dp;
	int rc;

	if ((dp = ddi_get_soft_state(diag250_state_head, instance)) == NULL)
		return (ENXIO);

#if DASD_DEBUG
	if (diag250_debug & D_D250MISC)
		prom_printf("diag250_open called for %p with type: %x\n",
			    dp, otyp);
#endif

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_close.                                    */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static int
diag250_close(dev_t dev, int flag, int otyp, cred_t *credp)
{
	int instance = getminor(dev);
	diag250_dev_t *dp;

	if ((dp = ddi_get_soft_state(diag250_state_head, instance)) == NULL)
		return (ENXIO);

#if DASD_DEBUG
	if (diag250_debug & D_D250MISC)
		prom_printf("diag250_close called for %p with type: %x\n",
			    dp, otyp);
#endif

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_minphys                                   */
/*                                                                  */
/* Function	- Impose limits on the transfer size                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
diag250_minphys(struct buf *bp)
{
	if (bp->b_bcount > 4096 * D250_BLKCNT) {
		prom_printf("####### minphys too big... %ld\n", 
			bp->b_bcount, 4096 * D250_BLKCNT);
		bp->b_bcount = 4096 * D250_BLKCNT;
	}

	/* We can handle anything we're given. */
	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_strategy                                  */
/*                                                                  */
/* Function	- actually do all the I/O                           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static int
diag250_strategy(struct buf *bp)
{
	diag250_dev_t	*dp;
	struct buf	*tp;
	struct buf	*lp;
	daddr_t		sblock = bp->b_lblkno;
	daddr_t		eblock = bp->b_lblkno + (bp->b_bcount / DEV_BSIZE) - 1;
	int       	rc = 0;

	dp = ddi_get_soft_state(diag250_state_head,
				getminor(bp->b_edev));

#if DASD_DEBUG
	if (diag250_debug & D_D250HIIO) {
		struct vnode *vp = bp->b_vp;

		if ((vp != NULL) &&
		    (vp->v_path != NULL))
			prom_printf("DIAG250 strategy - path: %s\n",vp->v_path);

		prom_printf("DIAG250 strategy - bp: %p blocks: %ld-%ld (%ld) "
			"bytes: %ld flags: %08x\n",
			bp, sblock, eblock, eblock - sblock + 1, bp->b_bcount, bp->b_flags);
	}
#endif

	/* Return buf if it attempts access outside of extent */
	if ((sblock < 0) || ((eblock / dp->lpp) > dp->endblock)) {
		cmn_err(CE_WARN,
			"Block request [%ld - %ld] out of range [0 - %ld]!\n",
			sblock / dp->lpp, eblock / dp->lpp,
			dp->endblock);
		bioerror(bp, EINVAL);
		biodone(bp);

		return(EINVAL);
	}

	/* Return buf if it attempts access outside of extent */
	if (((bp->b_flags & B_READ) == 0) && dp->rdonly) {
		cmn_err(CE_WARN,
			"Write attempt on readonly disk");
		bioerror(bp, EROFS);
		biodone(bp);

		return(EINVAL);
	}

	/* Maintain stats */
	mutex_enter(&dp->statex);
	kstat_waitq_enter(KSTAT_IO_PTR(dp->iostat));
	mutex_exit(&dp->statex);

	/* Block */
	mutex_enter(&dp->waitex);

	/* Add buf to end of wait queue */
	bp->av_forw = NULL;
	*dp->wtail = bp;
	dp->wtail = &bp->av_forw;

	/* Wake up the I/O task */
	dp->wakeup = B_TRUE;
	cv_signal(&dp->waitcv);

	/* Unblock */
	mutex_exit(&dp->waitex);

	return (rc);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_print                                     */
/*                                                                  */
/* Function	- emit error/debug message                          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static int
diag250_print(dev_t dev, char *str)
{
	cmn_err(CE_CONT, "diag250: %s\n",str);
	return (DDI_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_read                                      */
/*                                                                  */
/* Function	- read (for char device, calls strategy())          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static int
diag250_read(dev_t dev, struct uio *uio, cred_t *cred)
{
#if DASD_DEBUG
	if (diag250_debug & D_D250HIIO)
		prom_printf("diag250_read for %p\n",dev);
#endif
	return (physio(diag250_strategy, NULL, dev, B_READ, diag250_minphys, uio));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_write                                     */
/*                                                                  */
/* Function	- write (for char device, calls strategy())         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static int
diag250_write(dev_t dev, struct uio *uio, cred_t *cred)
{
#if DASD_DEBUG
	if (diag250_debug & D_D250HIIO)
		prom_printf("diag250_write for %p\n",dev);
#endif
	return (physio(diag250_strategy, NULL, dev, B_WRITE, diag250_minphys, uio));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_aread                                     */
/*                                                                  */
/* Function	- aio read (for char device, calls strategy())      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static int
diag250_aread(dev_t dev, struct aio_req *aio, cred_t *cred)
{
#if DASD_DEBUG
	if (diag250_debug & D_D250HIIO)
		prom_printf("diag250_aread for %p\n",dev);
#endif
	return (aphysio(diag250_strategy, anocancel, dev, B_READ, diag250_minphys, aio));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_awrite                                    */
/*                                                                  */
/* Function	- aio write (for char device, calls strategy())     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static int
diag250_awrite(dev_t dev, struct aio_req *aio, cred_t *cred)
{
#if DASD_DEBUG
	if (diag250_debug & D_D250HIIO)
		prom_printf("diag250_awrite for %p\n",dev);
#endif
	return (aphysio(diag250_strategy, anocancel, dev, B_WRITE, diag250_minphys, aio));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_ioctl                                     */
/*                                                                  */
/* Function	- ioctls (set/unset MDC, for instance)              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static int
diag250_ioctl(dev_t dev, int cmd, intptr_t arg, int mode,
	      cred_t *cred_p, int *rval_p)
{
	diag250_dev_t *dp;
	int instance = getminor(dev);
	enum dkio_state state;
	int rc;

	dp = ddi_get_soft_state(diag250_state_head, instance);
	if (dp == NULL) {
		return (ENXIO);
	}

#if DASD_DEBUG
	if (diag250_debug & D_D250HIIO)
		prom_printf("diag250_ioctl called for %p with cmd: %x, mode %x\n",
			    dp, cmd, mode);
#endif
	switch (cmd) {

	/*
	 * These are for faking out utilities like newfs.
	 */
	case DKIOCGVTOC:
		switch (ddi_model_convert_from(mode & FMODELS)) {
		case DDI_MODEL_ILP32: {
			struct vtoc32 vtoc32;

			vtoctovtoc32(dp->vtoc, vtoc32);
			if (ddi_copyout(&vtoc32, (void *)arg,
			    sizeof (struct vtoc32), mode))
				return (EFAULT);
				break;
			}

		case DDI_MODEL_NONE:
		default:
			if (ddi_copyout(&dp->vtoc, (void *)arg,
			    sizeof (struct vtoc), mode))
				return (EFAULT);
			break;
		}
		break;

	case DKIOCINFO:
		rc = ddi_copyout(&dp->cinfo, (void *)arg, sizeof(dp->cinfo), mode);
		if (rc) {
			return (EFAULT);
		}
		break;

	case DKIOCG_VIRTGEOM:
	case DKIOCG_PHYGEOM:
	case DKIOCGGEOM:
		rc = ddi_copyout(&dp->geom, (void *)arg, sizeof(dp->geom), mode);
		if (rc) {
			return (EFAULT);
		}
		break;

	case DKIOCSTATE:
		/* the file is always there */
		state = DKIO_INSERTED;
		rc = ddi_copyout(&state, (void *)arg, sizeof(state), mode);
		if (rc) {
			return (EFAULT);
		}
		break;

	case DKIOCGMEDIAINFO: {
		struct dk_minfo	media_info;

		media_info.dki_lbsize     = dp->blksize;
		media_info.dki_capacity   = dp->endblock * dp->blksize;
		media_info.dki_media_type = DK_FIXED_DISK;

		if (ddi_copyout(&media_info, (void *)arg,
		    sizeof (struct dk_minfo), mode)) {
			return (EFAULT);
		} else {
			return (0);
		}
	}

	case DKIOCGMBOOT: {
		char *blk;
		int rc;
#if 0
		// FIXME: We need to flush any pending I/O requests

		blk = diag250_alloc_static(dp->blksize);
		if (blk == NULL) {
			return (ENOMEM);
		}

		rc = diag250_ll_read_one(dp, dp->startblock + 1, blk);
		if (rc) {
			diag250_free_static(blk, dp->blksize);
			return (EIO);
		}

		rc = ddi_copyout(blk, (void *)arg, sizeof(struct mboot), mode);
		if (rc) {
			diag250_free_static(blk, dp->blksize);
			return (EFAULT);
		}
		
		diag250_free_static(blk, dp->blksize);
#endif
		break;
	}

	case DKIOCSMBOOT: {
		char *blk;
		int rc;
#if 0
		// FIXME: We need to flush any pending I/O requests

		blk = diag250_alloc_static(dp->blksize);
		if (blk == NULL) {
			return (ENOMEM);
		}

		rc = diag250_ll_read_one(dp, dp->startblock + 1, blk);
		if (rc) {
			diag250_free_static(blk, dp->blksize);
			return (EIO);
		}

		rc = ddi_copyin((void *)arg, blk, sizeof(struct mboot), mode);
		if (rc) {
			diag250_free_static(blk, dp->blksize);
			return (EFAULT);
		}
		
		rc = diag250_ll_write_one(dp, 1, blk);
		if (rc) {
			diag250_free_static(blk, dp->blksize);
			return (EIO);
		}

		diag250_free_static(blk, dp->blksize);
#endif
		break;
	}

	default:
		return (ENOTTY);
	}

	return(DDI_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_init_queues.                              */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static boolean_t
diag250_init_queues(diag250_dev_t *dp)
{
	diag250_io_t *io = NULL;
	diag250_bbuf_t *bbuf = NULL;
	boolean_t failed = B_TRUE;
	int i;

	sema_init(&dp->freeq.sema, dp->qlimit, NULL, SEMA_DRIVER, NULL);
	dp->freeq.head = NULL;
	dp->freeq.aba = 0;
	sema_init(&dp->bbufq.sema, dp->qlimit, NULL, SEMA_DRIVER, NULL);
	dp->bbufq.head = NULL;
	dp->bbufq.aba = 0;

	for (i = 0; i < dp->qlimit; i++) {
		/* Must not cross page boundary */
		io = vmem_xalloc(static_alloc_arena,
				 sizeof(diag250_io_t), PAGESIZE,
				 0, 0, NULL, NULL, VM_NOSLEEP);
		if (io == NULL) {
			prom_printf("Unable to alloc io\n");
			goto error;
		}
		bzero(io, sizeof(*io));
	
		mutex_init(&io->lock, NULL, MUTEX_DRIVER, NULL);
		cv_init(&io->wait, NULL, CV_DRIVER, NULL);

		io->link.next = (diag250_node_t *) dp->freeq.head;
		dp->freeq.head = (diag250_node_t *) io;

		io->bents = ccw_alloc64(sizeof(blkent_t) * D250_BLKCNT, VM_NOSLEEP);
		if (io->bents == NULL) {
			prom_printf("Unable to alloc bents for io\n");
			goto error;
		}
		
		io->dp = dp;
		io->bcnt = 0;
		io->done = B_FALSE;
		io->rwpl.bioiparm = (uintptr_t) io;
		io->rwpl.bioladdr = va_to_pa(io->bents);
		io->rwpl.bioplhd.biodevn = dp->devno;
		io->rwpl.bioplhd.biomode = BIOZAR;
		io->rwpl.bioflag = BIOASYN;
		if (!dp->usemdc) {
			io->rwpl.bioflag |= BIOBYPAS;
		}
	}

	for (i = 0; i < dp->bblimit; i++) {
		bbuf = kmem_zalloc(sizeof(diag250_bbuf_t), KM_NOSLEEP);
		if (bbuf == NULL) {
			prom_printf("Unable to alloc buf\n");
			goto error;
		}

		bbuf->link.next = (diag250_node_t *) dp->bbufq.head;
		dp->bbufq.head = (diag250_node_t *) bbuf;

		bbuf->buf = vmem_xalloc(static_alloc_arena,
					4096 * (D250_BLKCNT + 2), PAGESIZE,
					0, 0, NULL, NULL, VM_NOSLEEP);
		if (bbuf->buf == NULL) {
			prom_printf("Unable to alloc buf for bbuf\n");
			goto error;
		}
	}

	failed = B_FALSE;

error:

	return failed;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_free_queues.                              */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
diag250_free_queues(diag250_dev_t *dp)
{
	diag250_io_t *io;
	diag250_bbuf_t *bbuf, *nbbuf;

	/* Empty out the queue */
	bbuf = (diag250_bbuf_t *) dp->bbufq.head;
	while (bbuf != NULL) {
		dp->bbufq.head = bbuf->link.next;

		/* Free the buffer */
		if (bbuf->buf != NULL) {
			vmem_free(static_alloc_arena,
				  bbuf->buf,
				  4096 * (D250_BLKCNT + 2));
		}

		/* And the bounce buffer itself */
		kmem_free(bbuf, sizeof(diag250_bbuf_t));

		/* Get next node */
		bbuf = (diag250_bbuf_t *) dp->bbufq.head;
	};

	/* Empty out the queue */
	io = (diag250_io_t *) dp->freeq.head;
	while (io != NULL) {
		dp->freeq.head = io->link.next;

		/* Free the block entries array */
		if (io->bents != NULL) {
			ccw_free64(io->bents, sizeof(blkent_t) * D250_BLKCNT);
		}

		/* Free cv and mutext resources */
		cv_destroy(&io->wait);
		mutex_destroy(&io->lock);

		/* And the request itself */
		vmem_free(static_alloc_arena,
			  io,
			  sizeof(*io));

		io = (diag250_io_t *) dp->freeq.head;
	};

	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_get_label.                                */
/*                                                                  */
/* Function	- Get and validate label information.               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int 
diag250_get_label(ccw_device *dvc, diag250_dev_t *dp)
{
	dsklabel_t   label;
	defExtEckd_t eckdDef;
	locateRec_t  eckdLoc;
	defExtFba_t  fbaDef;
	locateFba_t  fbaLoc;
	sgiop_t	     sgio;
	int	     cc;
	int	     rc;

	static struct ccw1 eckd[4] = {
		{.op = CCW_CMD_DEFEXT, .flags = CCW_FLAG_CC,
		 .count = sizeof(eckdDef) },
		{.op = CCW_CMD_LOCATE_REC, .flags = CCW_FLAG_CC,
		 .count = sizeof(eckdLoc) },
		{.op = CCW_CMD_READ_ECKD, .flags = CCW_FLAG_SLI,
		 .count = sizeof(label) }
	};

	static struct ccw1 fba[4] = {
		{.op = CCW_CMD_DEFEXT, .flags = CCW_FLAG_CC,
		 .count = sizeof(fbaDef) },
		{.op = CCW_CMD_LOCATE, .flags = CCW_FLAG_CC,
		 .count = sizeof(fbaLoc) },
		{.op = CCW_CMD_READ_FBA, .flags = CCW_FLAG_SLI,
		 .count = sizeof(label) }
	};

	memset(&sgio, 0, sizeof(sgio));
	sgio.devno = dvc->rdc.vrdcdvno;
	sgio.flags = F1CCW;

	if (dvc->rdc.vrdcvcla == DC_DASD) {
		sgio.cpa	    = va_to_pa(&eckd);
		eckd[0].data	    = va_to_pa(&eckdDef);
		eckd[1].data	    = va_to_pa(&eckdLoc);
		eckd[2].data	    = va_to_pa(&label);
		memset(&eckdDef, 0, sizeof(eckdDef));
		eckdDef.mask.perm   = DFXE_NORM;
		eckdDef.attr.mode   = DFX_ECKD;
		eckdDef.endExt.head = 1;
		memset(&eckdLoc, 0, sizeof(eckdLoc));
		eckdLoc.count	    = 1;
		eckdLoc.search.rec  = 3;
		eckdLoc.sector	    = 0x29;
		eckdLoc.op.code	    = LRDOCRD;
	} else {
		sgio.cpa	= va_to_pa(&fba);
		fba[0].data	= va_to_pa(&fbaDef);
		fba[1].data	= va_to_pa(&fbaLoc);
		fba[2].data	= va_to_pa(&label);
		memset(&fbaDef, 0, sizeof(fbaDef));
		fbaDef.mask.perm  = DFXF_READ;
		fbaDef.extEnd   = 1;
		memset(&fbaLoc, 0, sizeof(fbaLoc));
		fbaLoc.op.cmd   = LRDOCRD;
		fbaLoc.blkCnt   = 1;
		fbaLoc.blkNr    = 1;
	}

	cc = diag_a8(&sgio, &rc);

	if (cc != 0) {
		cmn_err(CE_WARN,
			"Cannot read label of 0x%x\n",
			dvc->rdc.vrdcdvno);
		return (DDI_FAILURE);
	}

	e2a((char *)&label.id, sizeof(label.id));
	if (label.ident != CMS1) {
		cmn_err(CE_WARN,
			"Volume %s is not of correct format\n",
			label.id);
		return (DDI_FAILURE);
	}

	switch(label.blocksize)
	{
	case 512:
	case 1024:
	case 2048:
	case 4096:
		break;
	default:
		cmn_err(CE_WARN,
			"Blocksize %d not 512, 1024, 2048, or 4096!\n",
			label.blocksize);
		return (DDI_FAILURE);
	}
	
	if (label.offset <= 0) {
		cmn_err(CE_WARN,
			"Volume %s has not been reserved\n",
			label.id);
		return (DDI_FAILURE);
	}

	dp->blksize = label.blocksize;
	dp->blkmask = dp->blksize - 1;
	dp->lpp	    = dp->blksize / DEV_BSIZE;
	dp->lppmask = dp->lpp - 1;
	dp->offset  = label.offset + 1;

	cmn_err(CE_NOTE, "Volume %s discovered at "
		"0%x with blockize %d and offset %ld\n",
		label.id, dvc->rdc.vrdcdvno, 
		dp->blksize, dp->offset);

	return (DDI_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- diag250_fake_geometry                             */
/*                                                                  */
/* Function	- Create a fake geometry                            */
/*		  (taken from lofi.c)                               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static void
diag250_fake_geometry(diag250_dev_t *dp)
{
	size_t size = dp->endblock * dp->blksize;

	/* dk_geom - see dkio(7I) */

	/*
	 * dkg_ncyl _could_ be set to one here (one big cylinder with gobs
	 * of sectors), but that breaks programs like fdisk which want to
	 * partition a disk by cylinder. With one cylinder, you can't create
	 * an fdisk partition and put pcfs on it for testing (hard to pick
	 * a number between one and one).
	 *
	 * The cheezy floppy test is an attempt to not have too few cylinders
	 * for a small file, or so many on a big file that you waste space
	 * for backup superblocks or cylinder group structures.
	 */
	if (size < (2 * 1024 * 1024)) { /* floppy? */
		dp->geom.dkg_ncyl = size / (100 * 1024);
	}
	else {
		dp->geom.dkg_ncyl = size / (300 * 1024);
	}

	/* in case file is < 128k */
	if (dp->geom.dkg_ncyl == 0) {
		dp->geom.dkg_ncyl = 1;
	}
	dp->geom.dkg_acyl = 0;
	dp->geom.dkg_bcyl = 0;
	dp->geom.dkg_nhead = 1;
	dp->geom.dkg_obs1 = 0;
	dp->geom.dkg_intrlv = 0;
	dp->geom.dkg_obs2 = 0;
	dp->geom.dkg_obs3 = 0;
	dp->geom.dkg_apc = 0;
	dp->geom.dkg_rpm = 7200;
	dp->geom.dkg_pcyl = dp->geom.dkg_ncyl + dp->geom.dkg_acyl;
	dp->geom.dkg_nsect = size / (DEV_BSIZE * dp->geom.dkg_ncyl);
	dp->geom.dkg_write_reinstruct = 0;
	dp->geom.dkg_read_reinstruct = 0;
#if DASD_DEBUG
	if (diag250_debug & D_D250GEOM)
		prom_printf("diag250: Size: %d = Cyl: %d (%d), Sec: %d (bsize: %d)\n",
		size, dp->geom.dkg_ncyl, dp->geom.dkg_pcyl, dp->geom.dkg_nsect, dp->blksize);
#endif

	/* vtoc - see dkio(7I) */

	dp->vtoc.v_sanity = VTOC_SANE;
	dp->vtoc.v_version = V_VERSION;
	bcopy(D250_NAME, dp->vtoc.v_volume, min(LEN_DKL_VVOL, strlen(D250_NAME)));
	dp->vtoc.v_sectorsz = DEV_BSIZE;
	dp->vtoc.v_nparts = 1;
	dp->vtoc.v_part[0].p_tag = V_UNASSIGNED;
	dp->vtoc.v_part[0].p_flag = V_UNMNT;
	dp->vtoc.v_part[0].p_start = (daddr_t)0;

	/*
	 * The partition size cannot just be the number of sectors, because
	 * that might not end on a cylinder boundary. And if that's the case,
	 * newfs/mkfs will print a scary warning. So just figure the size
	 * based on the number of cylinders and sectors/cylinder.
	 */
	dp->vtoc.v_part[0].p_size = dp->geom.dkg_pcyl *
	    dp->geom.dkg_nsect * dp->geom.dkg_nhead;
#if DASD_DEBUG
	if (diag250_debug & D_D250GEOM)
		prom_printf("diag250: Part. Size: %d (diff. %d)\n", 
			dp->vtoc.v_part[0].p_size * DEV_BSIZE, size-(dp->vtoc.v_part[0].p_size * DEV_BSIZE));
#endif

	/* dk_cinfo - see dkio(7I) */

	/* controller information */
	strncpy(dp->cinfo.dki_cname, ddi_get_name(ddi_get_parent(dp->devi)), DK_DEVLEN);
	dp->cinfo.dki_ctype = DKC_MD;
	dp->cinfo.dki_cnum = ddi_get_instance(dp->devi);
	dp->cinfo.dki_addr = dp->devno;
	dp->cinfo.dki_space = 0;

	/* Unit Information */
	strncpy(dp->cinfo.dki_dname, ddi_driver_name(dp->devi), DK_DEVLEN);
	dp->cinfo.dki_unit = ddi_get_instance(dp->devi);
	dp->cinfo.dki_slave = 0;
	dp->cinfo.dki_partition = 0;

	dp->cinfo.dki_flags = DKI_FMTVOL;
	dp->cinfo.dki_maxtransfer = maxphys / DEV_BSIZE;
	dp->cinfo.dki_prio = 0;
	dp->cinfo.dki_vec = 0;
}

/*========================= End of Function ========================*/
