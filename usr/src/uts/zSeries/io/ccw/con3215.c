/*------------------------------------------------------------------*/
/* 								    */
/* Name        - con3215.c					    */
/* 								    */
/* Function    - z/VM 3215 console driver.			    */
/* 								    */
/* Name	       - Leland Lucius					    */
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

#define	CON_MI_IDNUM		0x3215
#define CON_MI_MAXOSZ		1920
#define	CON_MI_MAXISZ		160
#define	CON_MI_HIWAT		80
#define	CON_MI_LOWAT		40

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/termios.h>
#include <sys/termio.h>
#include <sys/modctl.h>
#include <sys/kbio.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/strsun.h>
#include <sys/sysmacros.h>
#include <sys/promif.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/cyclic.h>
#include <sys/intr.h>
#include <sys/spl.h>
#include <sys/tty.h>
#include <sys/kstr.h>
#include <sys/ios390x.h>
#include <sys/ccw.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

/*
 * con driver's soft state structure
 */
typedef struct con_state {
	/* mutexes */
	kmutex_t lock;

	/* task queues */
	ddi_taskq_t *rq;	/* request queue */
	ddi_taskq_t *cq;	/* complete queue */

	/* stream queues */
	queue_t *writeq;
	queue_t	*readq;

	/* dev info */
	ccw_device *cd;
	dev_info_t *dip;

	/* for handling IOCTL messages */
	bufcall_id_t wbufcid;
	tty_common_t tty;

	/* for console output timeout */
	boolean_t stopped;

	/* active input block */
	mblk_t *readb;
	boolean_t readqueued;
	boolean_t readpending;

	/* read buffer */
	char rbuf[CON_MI_MAXOSZ + 1];

	/* write buffer */
	char wbuf[CON_MI_MAXOSZ + 1];	// +1 for newline
	int wcnt;
	boolean_t writequeued;

	/* alternative control character sequences */
	struct {
		char *seq;
		int cc;
	} cc[NCCS];
} con_state;

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

/* dev_ops and cb_ops for device driver */
static int con_getinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static int con_attach(dev_info_t *, ddi_attach_cmd_t);
static int con_detach(dev_info_t *, ddi_detach_cmd_t);
static int con_open(queue_t *, dev_t *, int, int, cred_t *);
static int con_close(queue_t *, int, cred_t *);
static int con_wput(queue_t *, mblk_t *);
static int con_wsrv(queue_t *);
static int con_rsrv(queue_t *);

/* other internal con routines */
static void con_ioctl(queue_t *, mblk_t *);
static void con_reioctl(void *);
static void con_ack(mblk_t *, mblk_t *, uint_t);
static void con_flush(con_state *qsp);

static void con_read(void *args);
static void con_read_done(void *args);
static void con_write_done(void *args);

static int con_intr(ccw_device *dev, ccw_device_req *req);

static void con_setterm(void);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

char _depends_on[] = "ccwnex";

static void *con_state_head;

extern struct mod_ops mod_driverops;

static char autooff[] = "TERM AUTOCR OFF";

static struct
{
	char *name;
	char *seq;
	int ndx;
}
cc_props[NCCS] =
{
	{	"intr",		"^c",	VINTR		},
	{	"quit",		"^\\",	VQUIT		},
	{	"erase",	"^?",	VERASE		},
	{	"kill",		"^u",	VKILL		},
	{	"eof",		"^d",	VEOF		},
	{	"eol",		"",	VEOL		},
	{	"eol2",		"",	VEOL2		},
	{	"swtch",	"",	VSWTCH		},
	{	"start",	"^q",	VSTART		},
	{	"stop",		"^s",	VSTOP		},
	{	"susp",		"^z",	VSUSP		},
	{	"dsusp",	"^y",	VDSUSP		},
	{	"rprnt",	"^r",	VREPRINT	},
	{	"flush",	"^o",	VDISCARD	},
	{	"werase",	"^w",	VWERASE		},
	{	"lnext",	"^v",	VLNEXT		}
};

/* streams structures */
static struct module_info minfo = {
	CON_MI_IDNUM,		/* mi_idnum		*/
	"con3215",		/* mi_idname		*/
	0,			/* mi_minpsz		*/
	CON_MI_MAXOSZ,		/* mi_maxpsz		*/
	CON_MI_HIWAT,		/* mi_hiwat		*/
	CON_MI_LOWAT		/* mi_lowat		*/
};

static struct qinit rinit = {
	putq,			/* qi_putp		*/
	con_rsrv,		/* qi_srvp		*/
	con_open,		/* qi_qopen		*/
	con_close,		/* qi_qclose		*/
	NULL,			/* qi_qadmin		*/
	&minfo,			/* qi_minfo		*/
	NULL			/* qi_mstat		*/
};

static struct qinit winit = {
	con_wput,		/* qi_putp		*/
	con_wsrv,		/* qi_srvp		*/
	NULL,			/* qi_qopen		*/
	NULL,			/* qi_qclose		*/
	NULL,			/* qi_qadmin		*/
	&minfo,			/* qi_minfo		*/
	NULL			/* qi_mstat		*/
};

static struct streamtab constrinfo = {
	&rinit,
	&winit,
	NULL,
	NULL
};

/* standard device driver structures */
static struct cb_ops con_cb_ops = {
	nulldev,		/* open()		*/
	nulldev,		/* close()		*/
	nodev,			/* strategy()		*/
	nodev,			/* print()		*/
	nodev,			/* dump()		*/
	nodev,			/* read()		*/
	nodev,			/* write()		*/
	nodev,			/* ioctl()		*/
	nodev,			/* devmap()		*/
	nodev,			/* mmap()		*/
	nodev,			/* segmap()		*/
	nochpoll,		/* poll()		*/
	ddi_prop_op,		/* prop_op()		*/
	&constrinfo,		/* cb_str		*/
	D_NEW | D_MP		/* cb_flag		*/
};

static struct dev_ops con_ops = {
	DEVO_REV,
	0,			/* refcnt		*/
	con_getinfo,		/* getinfo()		*/
	nulldev,		/* identify()		*/
	nulldev,		/* probe()		*/
	con_attach,		/* attach()		*/
	con_detach,		/* detach()		*/
	nodev,			/* reset()		*/
	&con_cb_ops,		/* cb_ops		*/
	(struct bus_ops *)NULL,	/* bus_ops		*/
	NULL			/* power()		*/
};

static struct modldrv modldrv = {
	&mod_driverops,
	"3215 console driver v%I%",
	&con_ops
};

static struct modlinkage modlinkage = {
	MODREV_1,
	(void*)&modldrv,
	NULL
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
	int rv;

	DCCW(DCCW_L1, "con3215 _init()\n");

	rv = ddi_soft_state_init(&con_state_head,
				 sizeof (con_state),
				 1);
	if (rv != 0) {
		return (rv);
	}

	rv = mod_install(&modlinkage);
	if (rv != 0) {
		ddi_soft_state_fini(&con_state_head);
		return (rv);
	}

	return (0);
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
	DCCW(DCCW_L1, "con3215 _fini()\n");

	/* can't remove console driver */
	return (EBUSY);
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
	DCCW(DCCW_L1, "con3215 _info()\n");

	return (mod_info(&modlinkage, modinfop));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_attach.                                       */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
con_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	int instance = ddi_get_instance(dip);
	ccw_device *cd;
	con_state *qsp;
	int rv;

	DCCW(DCCW_L1, "con_attach(%p, %d)\n",
		dip, cmd);

	cd = ccw_device_register(dip);
	if (cd == NULL) {
		return (DDI_FAILURE);
	}

	if (cmd != DDI_ATTACH) {
		return (DDI_FAILURE);
	}

	rv = ddi_soft_state_zalloc(con_state_head, instance);
	if (rv != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Unable to allocate state for %d",
			instance);
		return (DDI_FAILURE);
	}

	qsp = ddi_get_soft_state(con_state_head, instance);
	if (qsp == NULL) {
		cmn_err(CE_WARN,
			"Unable to obtain state for %d",
			instance);
		ddi_soft_state_free(con_state_head, instance);
		return (DDI_FAILURE);
	}

	qsp->cd = cd;
	qsp->dip = dip;
	qsp->wcnt = 0;

	rv = ddi_create_minor_node(dip,
				   ddi_get_name(dip),
				   S_IFCHR,
				   instance,
				   DDI_NT_DISPLAY,
				   0);
	if (rv != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"Unable to create minor node for %d",
			instance);
		ddi_soft_state_free(con_state_head, instance);
		return (DDI_FAILURE);
	}

	qsp->rq = ddi_taskq_create(dip,
				   "con3215 request queue",
				   1,
				   TASKQ_DEFAULTPRI,
				   0);
	if (qsp->rq == NULL) {
		cmn_err(CE_WARN,
			"Unable to create task queue for %d",
			instance);
		ddi_soft_state_free(con_state_head, instance);
		return (DDI_FAILURE);
	}
	

	qsp->cq = ddi_taskq_create(dip,
				   "con3215 completion queue",
				   1,
				   TASKQ_DEFAULTPRI,
				   0);
	if (qsp->cq == NULL) {
		cmn_err(CE_WARN,
			"Unable to create task queue for %d",
			instance);
		ddi_soft_state_free(con_state_head, instance);
		return (DDI_FAILURE);
	}
	
	con_setterm();

	mutex_init(&qsp->lock, NULL, MUTEX_DRIVER, NULL);

	ccw_device_set_private(cd, qsp);

	ccw_device_set_handler(cd, con_intr);

	return (DDI_SUCCESS);

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_detach.                                       */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
con_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
        int instance = ddi_get_instance(dip);
	ccw_device *cd;
        con_state *qsp;

	DCCW(DCCW_L1, "con_detach(%p, %d)\n",
		dip, cmd);

	if (cmd != DDI_DETACH)
		return (DDI_FAILURE);

	qsp = ddi_get_soft_state(con_state_head, instance);
	if (qsp == NULL) {
		return (DDI_SUCCESS);
	}
	cd = qsp->cd;

	ccw_device_set_handler(cd, NULL);

	ddi_taskq_destroy(qsp->cq);

	ddi_taskq_destroy(qsp->rq);

	mutex_destroy(&qsp->lock);

	ddi_soft_state_free(con_state_head, instance);

	ccw_device_unregister(cd);

	return (DDI_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_getinfo.                                      */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
con_getinfo(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg, void **result)
{
	con_state *qsp;
	int retval = DDI_FAILURE;

	DCCW(DCCW_L1, "con_getinfo(%p, %d, %p, %p)\n",
		dip, infocmd, arg, result);

	switch (infocmd) {
	case DDI_INFO_DEVT2DEVINFO:
		qsp = ddi_get_soft_state(con_state_head,
					 getminor((dev_t) arg));
		if (qsp != NULL) {
			*result = (void *) qsp->dip;
			retval = DDI_SUCCESS;
		}
		else {
			*result = NULL;
		}
		break;

	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *) (uintptr_t) getminor((dev_t) arg);
		retval = DDI_SUCCESS;
		break;
	}

	return (retval);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_open.                                         */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
con_open(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *credp)
{
	int instance = getminor(*devp);
	con_state *qsp;
	tty_common_t *tty;
	struct termios *termiosp;
	char *str;
	int len;
	int rv;
	int ndx;
	
	DCCW(DCCW_L1, "con_open(%p, %p, %d, %d, %p)\n",
	     q, devp, oflag, sflag, credp);

	/* stream already open */
	if (q->q_ptr != NULL) {
		return (DDI_SUCCESS);
	}

	qsp = ddi_get_soft_state(con_state_head, instance);
	if (qsp == NULL) {
		cmn_err(CE_WARN, "con_open: console was not configured by "
		    "autoconfig");
		return (ENXIO);
	}

	mutex_enter(&qsp->lock);

	tty = &(qsp->tty);

	tty->t_readq = q;
	tty->t_writeq = WR(q);

	/* Link the RD and WR Q's */
	RD(q)->q_ptr = (caddr_t) qsp;
	WR(q)->q_ptr = (caddr_t) qsp;
	qsp->readq = RD(q);
	qsp->writeq = WR(q);

	rv = ddi_getlongprop(DDI_DEV_T_ANY,
			     ddi_root_node(),
			     0,
			     "ttymodes",
			     (caddr_t)&termiosp,
			     &len);
	if (rv == DDI_PROP_SUCCESS && len == sizeof(struct termios)) {
		for (ndx = 0; ndx < NCCS; ndx++) {
			qsp->cc[ndx].cc = termiosp->c_cc[ndx];
		}
		kmem_free(termiosp, len);
	}

	/* Get intr setting */
	for (ndx = 0; ndx < NCCS; ndx++) {
		rv = ddi_prop_lookup_string(DDI_DEV_T_ANY,
					    qsp->dip,
					    DDI_PROP_DONTPASS | DDI_PROP_NOTPROM,
					    cc_props[ndx].name,
					    &str);
		if (rv == DDI_PROP_SUCCESS) {
			qsp->cc[ndx].seq = i_ddi_strdup(str, KM_SLEEP);
			ddi_prop_free(str);
		}
		else {
			qsp->cc[ndx].seq = i_ddi_strdup(cc_props[ndx].seq, KM_SLEEP);
		}
	}

	/* Start up the queues */
	qprocson(q);

	/* Set subchannel parms and enable */
	ccw_device_enable(qsp->cd);

	/*
	 * Start a read task in case an attention interrupt was fielded
	 * while the driver wasn't open.
	 */
	if (qsp->readpending == B_TRUE) {
		rv = ddi_taskq_dispatch(qsp->rq, con_read, qsp, DDI_NOSLEEP);
		if (rv == DDI_FAILURE) {
			cmn_err(CE_WARN,
				"con3215: unable to start read task");
			mutex_exit(&qsp->lock);
			return (DDI_FAILURE);
		}
		qsp->readpending = B_FALSE;
	}
		
	mutex_exit(&qsp->lock);

	return (DDI_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_close.                                        */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
con_close(queue_t *q, int flag, cred_t *credp)
{
	con_state *qsp = RD(q)->q_ptr;
	int ndx;

	DCCW(DCCW_L1, "con_close(%p, %d, %p)\n",
		q, flag, credp);

	if (qsp == NULL) {
		return (ENXIO);
	}

	qprocsoff(q);

	for (ndx = 0; ndx < NCCS; ndx++) {
		if (qsp->cc[ndx].seq) {
			kmem_free(qsp->cc[ndx].seq, strlen(qsp->cc[ndx].seq) + 1);
			qsp->cc[ndx].seq = NULL;
		}
	}

	if (qsp->wbufcid != 0) {
		unbufcall(qsp->wbufcid);
	}

	ttycommon_close(&qsp->tty);

	RD(q)->q_ptr = NULL;
	WR(q)->q_ptr = NULL;
	qsp->readq = NULL;
	qsp->writeq = NULL;

	ccw_device_disable(qsp->cd);

	return (DDI_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_wput.                                         */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
con_wput(queue_t *q, mblk_t *mp)
{
	con_state *qsp = RD(q)->q_ptr;

	DCCW(DCCW_L1, "con_wput(%p, %p)\n",
		q, mp);

	if (qsp == NULL) {
		return (ENXIO);
	}


	if (!mp->b_datap) {
		cmn_err(CE_PANIC, "con_wput: null datap");
	}

	mutex_enter(&qsp->lock);

	switch (mp->b_datap->db_type) {
	case M_IOCTL:
	case M_CTL:
		switch (((struct iocblk *)mp->b_rptr)->ioc_cmd) {
		case TCSETSW:
		case TCSETSF:
		case TCSETAW:
		case TCSETAF:
		case TCSBRK:
			/*
			 * The changes do not take effect until all
			 * output queued before them is drained.
			 * Put this message on the queue, so that
			 * "con_start" will see it when it's done
			 * with the output before it. Poke the start
			 * routine, just in case.
			 */
			putq(q, mp);
			qenable(q);
			break;
		default:
			mutex_exit(&qsp->lock);
			con_ioctl(q, mp);
			mutex_enter(&qsp->lock);
		}
		break;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW) {
			flushq(q, FLUSHDATA);
			*mp->b_rptr &= ~FLUSHW;
		}
		if (*mp->b_rptr & FLUSHR) {
			flushq(RD(q), FLUSHDATA);
			qreply(q, mp);
		} else {
			freemsg(mp);
		}
		break;

	case M_STOP:
		qsp->stopped = B_TRUE;
		freemsg(mp);
		break;

	case M_START:
		qsp->stopped = B_FALSE;
		freemsg(mp);
		qenable(q);	/* Start up delayed messages */
		break;

	case M_DATA:
		/*
		 * Queue the message up to be transmitted,
		 * and poke the start routine.
		 */
		putq(q, mp);
		qenable(q);
		break;

	default:
		freemsg(mp);
	}

	mutex_exit(&qsp->lock);

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_ioctl.                                        */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
con_ioctl(queue_t *q, mblk_t *mp)
{
	con_state *qsp = RD(q)->q_ptr;
	struct iocblk	*iocp;
	tty_common_t	*tty;
	mblk_t		*datamp;
	int		data_size;
	int		error = 0;

	DCCW(DCCW_L1, "con_ioctl(%p, %p)\n",
		q, mp);

	if (qsp == NULL) {
		return; // (ENXIO);
	}

	iocp = (struct iocblk *)mp->b_rptr;
	tty = &(qsp->tty);

	if (tty->t_iocpending != NULL) {
		freemsg(tty->t_iocpending);
		tty->t_iocpending = NULL;
	}
	data_size = ttycommon_ioctl(tty, q, mp, &error);
	if (data_size != 0) {
		if (qsp->wbufcid) {
			unbufcall(qsp->wbufcid);
		}
		/* call con_reioctl() */
		qsp->wbufcid = bufcall(data_size, BPRI_HI, con_reioctl, qsp);
		return;
	}

	mutex_enter(&qsp->lock);

	if (error == 0) {
		switch (iocp->ioc_cmd) {
		case TCSETSW:
		case TCSETS:
			error = miocpullup(mp, sizeof(struct termios));
			if (error == 0) {
				struct termios *cb;
				int ndx;
	
				cb = (struct termios *) mp->b_cont->b_rptr;
				for (ndx = 0; ndx < NCCS; ndx++) {
					if (cb->c_cc[ndx]) {
						qsp->cc[ndx].cc = cb->c_cc[ndx];
					}
				}
			}
			break;
	
		case TCSETAW:
		case TCSETA:
			error = miocpullup(mp, sizeof(struct termios));
			if (error == 0) {
				struct termio *cb;
				int ndx;
	
				cb = (struct termio *) mp->b_cont->b_rptr;
				for (ndx = 0; ndx < _NCC; ndx++) {
					if (cb->c_cc[ndx]) {
						qsp->cc[ndx].cc = cb->c_cc[ndx];
					}
				}
			}
			break;
		}
			
	}
	else if (error < 0) {
		iocp = (struct iocblk *)mp->b_rptr;
		/*
		 * "ttycommon_ioctl" didn't do anything; we process it here.
		 */
		error = 0;
		switch (iocp->ioc_cmd) {
		case TCSBRK:
		case TIOCSBRK:
		case TIOCCBRK:
		case TIOCMSET:
		case TIOCMBIS:
		case TIOCMBIC:
			if (iocp->ioc_count != TRANSPARENT)
				con_ack(mp, NULL, 0);
			else
				mcopyin(mp, NULL, sizeof (int), NULL);
			break;

		case TIOCMGET:
			datamp = allocb(sizeof (int), BPRI_MED);
			if (datamp == NULL) {
				error = EAGAIN;
				break;
			}

			*(int *)datamp->b_rptr = 0;

			if (iocp->ioc_count != TRANSPARENT)
				con_ack(mp, datamp, sizeof (int));
			else
				mcopyout(mp, NULL, sizeof (int), NULL, datamp);
			break;

		default:
			error = EINVAL;
			break;
		}
	}

	if (error != 0) {
		iocp->ioc_count = 0;
		iocp->ioc_error = error;
		mp->b_datap->db_type = M_IOCNAK;
	}

	mutex_exit(&qsp->lock);

	qreply(q, mp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_reioctl.                                      */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
con_reioctl(void *unit)
{
	con_state *qsp = (con_state *) unit;
	queue_t *q;
	mblk_t *mp;

	DCCW(DCCW_L1, "con_reioctl(%p)\n",
		unit);

	if (!qsp->wbufcid) {
		return;
	}

	qsp->wbufcid = 0;
	q = qsp->tty.t_writeq;
	if (q == NULL) {
		return;
	}

	mp = qsp->tty.t_iocpending;
	if (mp == NULL) {
		return;
	}

	qsp->tty.t_iocpending = NULL;
	con_ioctl(q, mp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_ack.                                          */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
con_ack(mblk_t *mp, mblk_t *dp, uint_t size)
{
	struct iocblk  *iocp = (struct iocblk *)mp->b_rptr;

	DCCW(DCCW_L1, "con_ack(%p, %p, %d)\n",
		mp, dp, size);

	mp->b_datap->db_type = M_IOCACK;
	iocp->ioc_count = size;
	iocp->ioc_error = 0;
	iocp->ioc_rval = 0;

	if (mp->b_cont != NULL) {
		freeb(mp->b_cont);
	}

	mp->b_cont = dp;
	if (dp != NULL) {
		dp->b_wptr += size;
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_read.                                         */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
con_read(void *args)
{
	con_state *qsp = (con_state *) args;
	ccw_device_req *rreq;
	int rv;

	DCCW(DCCW_L1, "con_read(%p)\n",
		args);

	if (qsp->readqueued || qsp->writequeued) {
		qsp->readpending = B_TRUE;
		return;
	}

	ASSERT((MUTEX_NOT_HELD(&qsp->lock)));

	mutex_enter(&qsp->lock);

	rreq = ccw_device_alloc_req(qsp->cd);
	if (rreq == NULL) {
		cmn_err(CE_WARN,
			"con_read: allocb failed (console input dropped)");
		mutex_exit(&qsp->lock);
		return;
	}

	rreq->user = (void *) con_read_done;

	ccw_cmd_add(rreq,
		    CCW_CMD_READ_INQUIRY,
		    CCW_FLAG_SLI,
		    CON_MI_MAXISZ,
		    qsp->rbuf);
	ccw_cmd_add(rreq,
		    CCW_CMD_NOP,
		    0,
		    0,
		    NULL);

	qsp->readqueued = B_TRUE;

	rv = ccw_device_start(rreq);
	if (rv != 0) {
		qsp->readqueued = B_FALSE;
		ccw_device_free_req(rreq);
		mutex_exit(&qsp->lock);

		cmn_err(CE_WARN,
			"con3215: Unable to start read IO");
		return;
	}

	qsp->readpending = B_FALSE;

	mutex_exit(&qsp->lock);

	DCCW(DCCW_L1, "con_read leaving\n");
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_read_done.                                    */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
con_read_done(void *args)
{
	ccw_device_req *req = (ccw_device_req *) args;
	int instance = ddi_get_instance(req->dip);
	con_state *qsp = ddi_get_soft_state(con_state_head, instance);
	mblk_t *readb;
	char *rbuf;
	unsigned char *wptr;
	int rv;
	int len;
	int i;
	int seqcnts[NCCS];
	int seqndx;

	DCCW(DCCW_L1, "con_read_done(%p)\n",
		args);

	ASSERT((MUTEX_NOT_HELD(&qsp->lock)));

	mutex_enter(&qsp->lock);

	readb = allocb(CON_MI_MAXISZ + 1, BPRI_MED);  // +1 for newline
	if (readb == NULL) {
		cmn_err(CE_WARN,
			"con_read: allocb failed (console input dropped)");
		mutex_exit(&qsp->lock);
		return;
	}

	len = 0;
	if (req->irb.scsw.count > 0) {
		len = CON_MI_MAXISZ - req->irb.scsw.count;
	}

	e2a((const char *)qsp->rbuf, len);

	rbuf = qsp->rbuf;
	wptr = readb->b_wptr;

	/* Reset sequence counts */
	bzero(seqcnts, sizeof(seqcnts));

	/* Copy input while searching for control sequences */
	for (i = 0; i < len; i++) {
		char c = rbuf[i];

		/* Chech character against control character sequences */
		for (seqndx = 0; seqndx < NCCS; seqndx++) {
			int cnt = seqcnts[seqndx];

			if (c == qsp->cc[seqndx].seq[cnt]) {
				cnt++;
				if (qsp->cc[seqndx].seq[cnt] == '\0') {
					c = qsp->cc[seqndx].cc;
					wptr -= --cnt;
					cnt = 0;
				}
			}
			else {
				cnt = 0;
			}

			seqcnts[seqndx] = cnt;
		}

		*(wptr++) = c;
	}

	*(wptr++) = '\n';

	readb->b_wptr = wptr;

	qsp->readqueued = B_FALSE;

	mutex_exit(&qsp->lock);

	/* Any further QSP references should be read only */

	if (qsp->readpending) {
		con_read(qsp);
	}

	/* Kick the write queue */
	qenable(qsp->writeq);

	ccw_device_free_req(req);

	if (canputnext(qsp->readq)) {
		putnext(qsp->readq, readb);
	}
	else {
		freeb(readb);
		cmn_err(CE_WARN, "con_read_done: canputnext "
			"failed (console input dropped)");
	}
	DCCW(DCCW_L1, "con_read_done leaving\n");
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_write_done.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
con_write_done(void *args)
{
	ccw_device_req *req = (ccw_device_req *) args;
	int instance = ddi_get_instance(req->dip);
	con_state *qsp = ddi_get_soft_state(con_state_head, instance);

	DCCW(DCCW_L1, "con_write_done(%p)\n",
		args);

	ASSERT((MUTEX_NOT_HELD(&qsp->lock)));

	mutex_enter(&qsp->lock);
	
	qsp->writequeued = B_FALSE;

	mutex_exit(&qsp->lock);

	/* Any further QSP references should be read only */

	if (qsp->readpending) {
		con_read(qsp);
	}

	qenable(qsp->writeq);

	ccw_device_free_req(req);

	DCCW(DCCW_L1, "con_write_done leaving\n");
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_flush.                                        */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
con_flush(con_state *qsp)
{
	queue_t *q;
	mblk_t *mp;

	DCCW(DCCW_L1, "con_flush(%p)\n",
		qsp);

	ASSERT(MUTEX_HELD(&qsp->lock));

	q = qsp->writeq;

	DCCW(DCCW_L1, "con_flush(): WARNING console output is dropped time=%lx\n",
	    gethrestime_sec());

	while (mp = getq(q)) {
		freemsg(mp);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_rsrv.                                         */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
con_rsrv(queue_t *q)
{
	con_state *qsp = RD(q)->q_ptr;
	mblk_t *mp;

	DCCW(DCCW_L1, "con_rsrv(%p)\n",
		q);

	if (qsp->stopped == B_TRUE) {
		return (0);
	}

	mutex_enter(&qsp->lock);

	while ((mp = getq(q)) != NULL) {
		if (canputnext(q)) {
			putnext(q, mp);
		}
		else if (mp->b_datap->db_type >= QPCTL) {
			putbq(q, mp);
		}
	}

	mutex_exit(&qsp->lock);

	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_wsrv.                                         */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
con_wsrv(queue_t *q)
{
	con_state *qsp = WR(q)->q_ptr;
	mblk_t *mp;
	int avail;
	int icnt;
	int ocnt;
	int rv;
	int lfcnt;
	char c;

	DCCW(DCCW_L1, "con_wsrv(%p)\n",
		q);

	if (qsp->stopped || qsp->readqueued || qsp->writequeued) {
		return (0);
	}

	ASSERT((MUTEX_NOT_HELD(&qsp->lock)));

	mutex_enter(&qsp->lock);

	/*
	 * read stream queue and remove data from the queue and
	 * transmit them if possible
	 */
	while (mp = getq(q)) {

		/* Combine mblks */
		pullupmsg(mp, -1);

		if (mp->b_datap->db_type == M_IOCTL) {
			/*
			 * These are those IOCTLs queued up
			 * do it now
			 */
			mutex_exit(&qsp->lock);
			con_ioctl(q, mp);
			mutex_enter(&qsp->lock);
			continue;
		}

		/*
		 * M_DATA
		 */
/* Need to check for 50 linefeeds to satisfy VM */
		avail = MIN(msgdsize(mp), CON_MI_MAXOSZ - qsp->wcnt);
		for (icnt = 0, ocnt = 0; ocnt < avail; ocnt++) {
			c = mp->b_rptr[icnt++];

			if (c == '\n') {
/* FIXME - a2e issue */		c = 0xc5;
			}

			qsp->wbuf[qsp->wcnt++] = c;
		}

		avail -= icnt;
		mp->b_rptr += icnt;

		if (msgdsize(mp) != 0) {
			putbq(q, mp);
		}
		else {
			freeb(mp);
		}

		if (qsp->wcnt == CON_MI_MAXOSZ) {
			break;
		}
	}

	if (qsp->wcnt != 0) {
		ccw_device_req *wreq;

		a2e(qsp->wbuf, qsp->wcnt);
	
		wreq = ccw_device_alloc_req(qsp->cd);
		wreq->user = (void *) con_write_done;
	
		ccw_cmd_add(wreq,
			    CCW_CMD_WRITE,
			    CCW_FLAG_SLI,
			    qsp->wcnt,
			    qsp->wbuf);
		ccw_cmd_add(wreq,
			    CCW_CMD_NOP,
			    0,
			    0,
			    NULL);

		qsp->writequeued = B_TRUE;

		rv = ccw_device_start(wreq);
		if (rv != 0) {
			qsp->writequeued = B_FALSE;
			ccw_device_free_req(wreq);
			mutex_exit(&qsp->lock);
			cmn_err(CE_WARN,
				"con3215: Unable to start write IO");
			return (0);
		}
		qsp->wcnt = 0;
	}

	mutex_exit(&qsp->lock);

	DCCW(DCCW_L1, "con_wsrv returning %d\n", qsize(q));
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_intr.                                         */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
con_intr(ccw_device *dev, ccw_device_req *req)
{
	irb *irb = ccw_device_get_irb(dev);
	con_state *qsp = (con_state *) ccw_device_get_private(dev);
	int rv;

	DCCW(DCCW_L1, "###*** con_intr(%p, %p, %p) ***###\n",
		dev, req, irb);

	DCCW(DCCW_L1, "dstat = %08x\n", irb->scsw.dstat);

	/* Can happen if interrupt occurs before device is opened */
	if (qsp->readq == NULL || qsp->writeq == NULL) {
		/*
		 * For attention interrupts, we need to remember that it 
		 * happened so when the device is eventually opened the
		 * pending data can be read.  If it is not, the console
		 * will no longer accept any input.
		 *
		 * All other interrupts are ignored since the driver isn't
		 * open.
		 */
		if (irb->scsw.dstat == DEV_STAT_ATTENTION) {
			qsp->readpending = B_TRUE;
		}
	}
	else if (irb->scsw.dstat == DEV_STAT_ATTENTION) {
		rv = ddi_taskq_dispatch(qsp->rq, con_read, qsp, DDI_NOSLEEP);
		if (rv == DDI_FAILURE) {
			cmn_err(CE_WARN,
				"con3215: unable to start read task");
		}
	}
	else if (req) {
		rv = ddi_taskq_dispatch(qsp->cq, req->user, req, DDI_NOSLEEP);
		if (rv == DDI_FAILURE) {
			cmn_err(CE_WARN,
				"con3215: unable to start completion task");
		}
	}
	else {
		cmn_err(CE_WARN,
			"con3215: Unknown interrupt reason: "
			"stCtl %08x dstat %08x\n",
			irb->scsw.stCtl, irb->scsw.dstat);
	}

	DCCW(DCCW_L1, "con_intr() returning\n");

	return 0;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- con_setterm.                                      */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
con_setterm(void)
{
	char *buf;
	int len = strlen(autooff);
	int rc;
	int cc;

	buf = ccw_alloc64(len + 1, VM_SLEEP);
	if (buf != NULL) {
		strcpy(buf, autooff);

		a2e(buf,  len);

		__asm__("	lgr	1,%2\n"
			"	lgr	3,%3\n"
			"	lghi	4,0\n"
			"	diag	1,3,0x8\n"
			"	lgfr	%1,4\n"
			"	lghi	%0,0\n"
			"	ipm	%0\n"
			"	srlg	%0,%0,28"
			: "=r" (rc), "=r" (cc)
			: "r" (va_to_pa(buf)), "r" (len)
			: "1", "2", "3", "4");

		ccw_free64(buf, len + 1);
	}
}

/*========================= End of Function ========================*/
