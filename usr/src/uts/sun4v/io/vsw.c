/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
 */

/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/time.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/strlog.h>
#include <sys/strsubr.h>
#include <sys/cmn_err.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/ksynch.h>
#include <sys/stat.h>
#include <sys/kstat.h>
#include <sys/vtrace.h>
#include <sys/strsun.h>
#include <sys/dlpi.h>
#include <sys/ethernet.h>
#include <net/if.h>
#include <sys/varargs.h>
#include <sys/machsystm.h>
#include <sys/modctl.h>
#include <sys/modhash.h>
#include <sys/mac.h>
#include <sys/mac_ether.h>
#include <sys/taskq.h>
#include <sys/note.h>
#include <sys/mach_descrip.h>
#include <sys/mac.h>
#include <sys/mdeg.h>
#include <sys/ldc.h>
#include <sys/vsw_fdb.h>
#include <sys/vsw.h>
#include <sys/vio_mailbox.h>
#include <sys/vnet_mailbox.h>
#include <sys/vnet_common.h>
#include <sys/vio_util.h>
#include <sys/sdt.h>
#include <sys/atomic.h>
#include <sys/callb.h>
#include <sys/vlan.h>

/*
 * Function prototypes.
 */
static	int vsw_attach(dev_info_t *, ddi_attach_cmd_t);
static	int vsw_detach(dev_info_t *, ddi_detach_cmd_t);
static	int vsw_getinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static	int vsw_get_md_physname(vsw_t *, md_t *, mde_cookie_t, char *);
static	int vsw_get_md_smodes(vsw_t *, md_t *, mde_cookie_t, uint8_t *, int *);

/* MDEG routines */
static	int vsw_mdeg_register(vsw_t *vswp);
static	void vsw_mdeg_unregister(vsw_t *vswp);
static	int vsw_mdeg_cb(void *cb_argp, mdeg_result_t *);
static	int vsw_port_mdeg_cb(void *cb_argp, mdeg_result_t *);
static	int vsw_get_initial_md_properties(vsw_t *vswp, md_t *, mde_cookie_t);
static	int vsw_read_mdprops(vsw_t *vswp);
static	void vsw_vlan_read_ids(void *arg, int type, md_t *mdp,
	mde_cookie_t node, uint16_t *pvidp, uint16_t **vidspp,
	uint16_t *nvidsp, uint16_t *default_idp);
static	int vsw_port_read_props(vsw_port_t *portp, vsw_t *vswp,
	md_t *mdp, mde_cookie_t *node);
static	void vsw_read_pri_eth_types(vsw_t *vswp, md_t *mdp,
	mde_cookie_t node);
static	void vsw_update_md_prop(vsw_t *, md_t *, mde_cookie_t);
static void vsw_save_lmacaddr(vsw_t *vswp, uint64_t macaddr);

/* Mac driver related routines */
static int vsw_mac_register(vsw_t *);
static int vsw_mac_unregister(vsw_t *);
static int vsw_m_stat(void *, uint_t, uint64_t *);
static void vsw_m_stop(void *arg);
static int vsw_m_start(void *arg);
static int vsw_m_unicst(void *arg, const uint8_t *);
static int vsw_m_multicst(void *arg, boolean_t, const uint8_t *);
static int vsw_m_promisc(void *arg, boolean_t);
static mblk_t *vsw_m_tx(void *arg, mblk_t *);
void vsw_mac_rx(vsw_t *vswp, mac_resource_handle_t mrh,
    mblk_t *mp, vsw_macrx_flags_t flags);

/*
 * Functions imported from other files.
 */
extern void vsw_setup_switching_timeout(void *arg);
extern void vsw_stop_switching_timeout(vsw_t *vswp);
extern int vsw_setup_switching(vsw_t *);
extern void vsw_switch_frame_nop(vsw_t *vswp, mblk_t *mp, int caller,
    vsw_port_t *port, mac_resource_handle_t mrh);
extern int vsw_add_mcst(vsw_t *, uint8_t, uint64_t, void *);
extern int vsw_del_mcst(vsw_t *, uint8_t, uint64_t, void *);
extern void vsw_del_mcst_vsw(vsw_t *);
extern mcst_addr_t *vsw_del_addr(uint8_t devtype, void *arg, uint64_t addr);
extern int vsw_detach_ports(vsw_t *vswp);
extern int vsw_port_add(vsw_t *vswp, md_t *mdp, mde_cookie_t *node);
extern int vsw_port_detach(vsw_t *vswp, int p_instance);
static int vsw_port_update(vsw_t *vswp, md_t *curr_mdp, mde_cookie_t curr_mdex,
	md_t *prev_mdp, mde_cookie_t prev_mdex);
extern	int vsw_port_attach(vsw_port_t *port);
extern vsw_port_t *vsw_lookup_port(vsw_t *vswp, int p_instance);
extern int vsw_mac_attach(vsw_t *vswp);
extern void vsw_mac_detach(vsw_t *vswp);
extern int vsw_mac_open(vsw_t *vswp);
extern void vsw_mac_close(vsw_t *vswp);
extern int vsw_set_hw(vsw_t *, vsw_port_t *, int);
extern int vsw_unset_hw(vsw_t *, vsw_port_t *, int);
extern void vsw_reconfig_hw(vsw_t *);
extern void vsw_unset_addrs(vsw_t *vswp);
extern void vsw_set_addrs(vsw_t *vswp);
extern void vsw_create_vlans(void *arg, int type);
extern void vsw_destroy_vlans(void *arg, int type);
extern void vsw_vlan_add_ids(void *arg, int type);
extern void vsw_vlan_remove_ids(void *arg, int type);
extern void vsw_vlan_unaware_port_reset(vsw_port_t *portp);
extern uint32_t vsw_vlan_frame_untag(void *arg, int type, mblk_t **np,
	mblk_t **npt);
extern mblk_t *vsw_vlan_frame_pretag(void *arg, int type, mblk_t *mp);
extern void vsw_hio_cleanup(vsw_t *vswp);
extern void vsw_hio_start_ports(vsw_t *vswp);
void vsw_hio_port_update(vsw_port_t *portp, boolean_t hio_enabled);

/*
 * Internal tunables.
 */
int	vsw_num_handshakes = VNET_NUM_HANDSHAKES; /* # of handshake attempts */
int	vsw_wretries = 100;		/* # of write attempts */
int	vsw_desc_delay = 0;		/* delay in us */
int	vsw_read_attempts = 5;		/* # of reads of descriptor */
int	vsw_setup_switching_delay = 3;	/* setup sw timeout interval in sec */
int	vsw_mac_open_retries = 300;	/* max # of mac_open() retries */
					/* 300*3 = 900sec(15min) of max tmout */
int	vsw_ldc_tx_delay = 5;		/* delay(ticks) for tx retries */
int	vsw_ldc_tx_retries = 10;	/* # of ldc tx retries */
boolean_t vsw_ldc_rxthr_enabled = B_TRUE;	/* LDC Rx thread enabled */
boolean_t vsw_ldc_txthr_enabled = B_TRUE;	/* LDC Tx thread enabled */

uint32_t	vsw_fdb_nchains = 8;	/* # of chains in fdb hash table */
uint32_t	vsw_vlan_nchains = 4;	/* # of chains in vlan id hash table */
uint32_t	vsw_ethermtu = 1500;	/* mtu of the device */

/* sw timeout for boot delay only, in milliseconds */
int vsw_setup_switching_boot_delay = 100 * MILLISEC;

/* delay in usec to wait for all references on a fdb entry to be dropped */
uint32_t vsw_fdbe_refcnt_delay = 10;

/*
 * Default vlan id. This is only used internally when the "default-vlan-id"
 * property is not present in the MD device node. Therefore, this should not be
 * used as a tunable; if this value is changed, the corresponding variable
 * should be updated to the same value in all vnets connected to this vsw.
 */
uint16_t	vsw_default_vlan_id = 1;

/*
 * Workaround for a version handshake bug in obp's vnet.
 * If vsw initiates version negotiation starting from the highest version,
 * obp sends a nack and terminates version handshake. To workaround
 * this, we do not initiate version handshake when the channel comes up.
 * Instead, we wait for the peer to send its version info msg and go through
 * the version protocol exchange. If we successfully negotiate a version,
 * before sending the ack, we send our version info msg to the peer
 * using the <major,minor> version that we are about to ack.
 */
boolean_t vsw_obp_ver_proto_workaround = B_TRUE;

/*
 * In the absence of "priority-ether-types" property in MD, the following
 * internal tunable can be set to specify a single priority ethertype.
 */
uint64_t vsw_pri_eth_type = 0;

/*
 * Number of transmit priority buffers that are preallocated per device.
 * This number is chosen to be a small value to throttle transmission
 * of priority packets. Note: Must be a power of 2 for vio_create_mblks().
 */
uint32_t vsw_pri_tx_nmblks = 64;

/*
 * Number of RARP packets sent to announce macaddr to the physical switch,
 * after vsw's physical device is changed dynamically or after a guest (client
 * vnet) is live migrated in.
 */
uint32_t vsw_publish_macaddr_count = 3;

boolean_t vsw_hio_enabled = B_TRUE;	/* Enable/disable HybridIO */
int vsw_hio_max_cleanup_retries = 10;	/* Max retries for HybridIO cleanp */
int vsw_hio_cleanup_delay = 10000;	/* 10ms */

/*
 * External tunables.
 */
/*
 * Enable/disable thread per ring. This is a mode selection
 * that is done a vsw driver attach time.
 */
boolean_t vsw_multi_ring_enable = B_FALSE;
int vsw_mac_rx_rings = VSW_MAC_RX_RINGS;

/* Number of transmit descriptors -  must be power of 2 */
uint32_t vsw_ntxds = VSW_RING_NUM_EL;

/*
 * Max number of mblks received in one receive operation.
 */
uint32_t vsw_chain_len = (VSW_NUM_MBLKS * 0.6);

/*
 * Tunables for three different pools, that is, the size and
 * number of mblks for each pool.
 */
uint32_t vsw_mblk_size1 = VSW_MBLK_SZ_128;	/* size=128 for pool1 */
uint32_t vsw_mblk_size2 = VSW_MBLK_SZ_256;	/* size=256 for pool2 */
uint32_t vsw_mblk_size3 = VSW_MBLK_SZ_2048;	/* size=2048 for pool3 */
uint32_t vsw_num_mblks1 = VSW_NUM_MBLKS;	/* number of mblks for pool1 */
uint32_t vsw_num_mblks2 = VSW_NUM_MBLKS;	/* number of mblks for pool2 */
uint32_t vsw_num_mblks3 = VSW_NUM_MBLKS;	/* number of mblks for pool3 */

/*
 * vsw_max_tx_qcount is the maximum # of packets that can be queued
 * before the tx worker thread begins processing the queue. Its value
 * is chosen to be 4x the default length of tx descriptor ring.
 */
uint32_t vsw_max_tx_qcount = 4 * VSW_RING_NUM_EL;

/*
 * MAC callbacks
 */
static	mac_callbacks_t	vsw_m_callbacks = {
	0,
	vsw_m_stat,
	vsw_m_start,
	vsw_m_stop,
	vsw_m_promisc,
	vsw_m_multicst,
	vsw_m_unicst,
	vsw_m_tx,
	NULL,
	NULL,
	NULL
};

static	struct	cb_ops	vsw_cb_ops = {
	nulldev,			/* cb_open */
	nulldev,			/* cb_close */
	nodev,				/* cb_strategy */
	nodev,				/* cb_print */
	nodev,				/* cb_dump */
	nodev,				/* cb_read */
	nodev,				/* cb_write */
	nodev,				/* cb_ioctl */
	nodev,				/* cb_devmap */
	nodev,				/* cb_mmap */
	nodev,				/* cb_segmap */
	nochpoll,			/* cb_chpoll */
	ddi_prop_op,			/* cb_prop_op */
	NULL,				/* cb_stream */
	D_MP,				/* cb_flag */
	CB_REV,				/* rev */
	nodev,				/* int (*cb_aread)() */
	nodev				/* int (*cb_awrite)() */
};

static	struct	dev_ops	vsw_ops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	vsw_getinfo,		/* devo_getinfo */
	nulldev,		/* devo_identify */
	nulldev,		/* devo_probe */
	vsw_attach,		/* devo_attach */
	vsw_detach,		/* devo_detach */
	nodev,			/* devo_reset */
	&vsw_cb_ops,		/* devo_cb_ops */
	(struct bus_ops *)NULL,	/* devo_bus_ops */
	ddi_power		/* devo_power */
};

extern	struct	mod_ops	mod_driverops;
static struct modldrv vswmodldrv = {
	&mod_driverops,
	"sun4v Virtual Switch",
	&vsw_ops,
};

#define	LDC_ENTER_LOCK(ldcp)	\
				mutex_enter(&((ldcp)->ldc_cblock));\
				mutex_enter(&((ldcp)->ldc_rxlock));\
				mutex_enter(&((ldcp)->ldc_txlock));
#define	LDC_EXIT_LOCK(ldcp)	\
				mutex_exit(&((ldcp)->ldc_txlock));\
				mutex_exit(&((ldcp)->ldc_rxlock));\
				mutex_exit(&((ldcp)->ldc_cblock));

/* Driver soft state ptr  */
static void	*vsw_state;

/*
 * Linked list of "vsw_t" structures - one per instance.
 */
vsw_t		*vsw_head = NULL;
krwlock_t	vsw_rw;

/*
 * Property names
 */
static char vdev_propname[] = "virtual-device";
static char vsw_propname[] = "virtual-network-switch";
static char physdev_propname[] = "vsw-phys-dev";
static char smode_propname[] = "vsw-switch-mode";
static char macaddr_propname[] = "local-mac-address";
static char remaddr_propname[] = "remote-mac-address";
static char ldcids_propname[] = "ldc-ids";
static char chan_propname[] = "channel-endpoint";
static char id_propname[] = "id";
static char reg_propname[] = "reg";
static char pri_types_propname[] = "priority-ether-types";
static char vsw_pvid_propname[] = "port-vlan-id";
static char vsw_vid_propname[] = "vlan-id";
static char vsw_dvid_propname[] = "default-vlan-id";
static char port_pvid_propname[] = "remote-port-vlan-id";
static char port_vid_propname[] = "remote-vlan-id";
static char hybrid_propname[] = "hybrid";

/*
 * Matching criteria passed to the MDEG to register interest
 * in changes to 'virtual-device-port' nodes identified by their
 * 'id' property.
 */
static md_prop_match_t vport_prop_match[] = {
	{ MDET_PROP_VAL,    "id"   },
	{ MDET_LIST_END,    NULL    }
};

static mdeg_node_match_t vport_match = { "virtual-device-port",
						vport_prop_match };

/*
 * Matching criteria passed to the MDEG to register interest
 * in changes to 'virtual-device' nodes (i.e. vsw nodes) identified
 * by their 'name' and 'cfg-handle' properties.
 */
static md_prop_match_t vdev_prop_match[] = {
	{ MDET_PROP_STR,    "name"   },
	{ MDET_PROP_VAL,    "cfg-handle" },
	{ MDET_LIST_END,    NULL    }
};

static mdeg_node_match_t vdev_match = { "virtual-device",
						vdev_prop_match };


/*
 * Specification of an MD node passed to the MDEG to filter any
 * 'vport' nodes that do not belong to the specified node. This
 * template is copied for each vsw instance and filled in with
 * the appropriate 'cfg-handle' value before being passed to the MDEG.
 */
static mdeg_prop_spec_t vsw_prop_template[] = {
	{ MDET_PROP_STR,    "name",		vsw_propname },
	{ MDET_PROP_VAL,    "cfg-handle",	NULL	},
	{ MDET_LIST_END,    NULL,		NULL	}
};

#define	VSW_SET_MDEG_PROP_INST(specp, val)	(specp)[1].ps_val = (val);

#ifdef	DEBUG
/*
 * Print debug messages - set to 0x1f to enable all msgs
 * or 0x0 to turn all off.
 */
int vswdbg = 0x0;

/*
 * debug levels:
 * 0x01:	Function entry/exit tracing
 * 0x02:	Internal function messages
 * 0x04:	Verbose internal messages
 * 0x08:	Warning messages
 * 0x10:	Error messages
 */

void
vswdebug(vsw_t *vswp, const char *fmt, ...)
{
	char buf[512];
	va_list ap;

	va_start(ap, fmt);
	(void) vsprintf(buf, fmt, ap);
	va_end(ap);

	if (vswp == NULL)
		cmn_err(CE_CONT, "%s\n", buf);
	else
		cmn_err(CE_CONT, "vsw%d: %s\n", vswp->instance, buf);
}

#endif	/* DEBUG */

static struct modlinkage modlinkage = {
	MODREV_1,
	&vswmodldrv,
	NULL
};

int
_init(void)
{
	int status;

	rw_init(&vsw_rw, NULL, RW_DRIVER, NULL);

	status = ddi_soft_state_init(&vsw_state, sizeof (vsw_t), 1);
	if (status != 0) {
		return (status);
	}

	mac_init_ops(&vsw_ops, DRV_NAME);
	status = mod_install(&modlinkage);
	if (status != 0) {
		ddi_soft_state_fini(&vsw_state);
	}
	return (status);
}

int
_fini(void)
{
	int status;

	status = mod_remove(&modlinkage);
	if (status != 0)
		return (status);
	mac_fini_ops(&vsw_ops);
	ddi_soft_state_fini(&vsw_state);

	rw_destroy(&vsw_rw);

	return (status);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

static int
vsw_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	vsw_t		*vswp;
	int		instance;
	char		hashname[MAXNAMELEN];
	char		qname[TASKQ_NAMELEN];
	enum		{ PROG_init = 0x00,
				PROG_locks = 0x01,
				PROG_readmd = 0x02,
				PROG_fdb = 0x04,
				PROG_mfdb = 0x08,
				PROG_taskq = 0x10,
				PROG_swmode = 0x20,
				PROG_macreg = 0x40,
				PROG_mdreg = 0x80}
			progress;

	progress = PROG_init;
	int		rv;

	switch (cmd) {
	case DDI_ATTACH:
		break;
	case DDI_RESUME:
		/* nothing to do for this non-device */
		return (DDI_SUCCESS);
	case DDI_PM_RESUME:
	default:
		return (DDI_FAILURE);
	}

	instance = ddi_get_instance(dip);
	if (ddi_soft_state_zalloc(vsw_state, instance) != DDI_SUCCESS) {
		DERR(NULL, "vsw%d: ddi_soft_state_zalloc failed", instance);
		return (DDI_FAILURE);
	}
	vswp = ddi_get_soft_state(vsw_state, instance);

	if (vswp == NULL) {
		DERR(NULL, "vsw%d: ddi_get_soft_state failed", instance);
		goto vsw_attach_fail;
	}

	vswp->dip = dip;
	vswp->instance = instance;
	ddi_set_driver_private(dip, (caddr_t)vswp);

	mutex_init(&vswp->hw_lock, NULL, MUTEX_DRIVER, NULL);
	mutex_init(&vswp->mca_lock, NULL, MUTEX_DRIVER, NULL);
	mutex_init(&vswp->swtmout_lock, NULL, MUTEX_DRIVER, NULL);
	rw_init(&vswp->if_lockrw, NULL, RW_DRIVER, NULL);
	rw_init(&vswp->mac_rwlock, NULL, RW_DRIVER, NULL);
	rw_init(&vswp->mfdbrw, NULL, RW_DRIVER, NULL);
	rw_init(&vswp->plist.lockrw, NULL, RW_DRIVER, NULL);

	progress |= PROG_locks;

	rv = vsw_read_mdprops(vswp);
	if (rv != 0)
		goto vsw_attach_fail;

	progress |= PROG_readmd;

	/* setup the unicast forwarding database  */
	(void) snprintf(hashname, MAXNAMELEN, "vsw_unicst_table-%d",
	    vswp->instance);
	D2(vswp, "creating unicast hash table (%s)...", hashname);
	vswp->fdb_nchains = vsw_fdb_nchains;
	vswp->fdb_hashp = mod_hash_create_ptrhash(hashname, vswp->fdb_nchains,
	    mod_hash_null_valdtor, sizeof (void *));
	vsw_create_vlans((void *)vswp, VSW_LOCALDEV);
	progress |= PROG_fdb;

	/* setup the multicast fowarding database */
	(void) snprintf(hashname, MAXNAMELEN, "vsw_mcst_table-%d",
	    vswp->instance);
	D2(vswp, "creating multicast hash table %s)...", hashname);
	vswp->mfdb = mod_hash_create_ptrhash(hashname, vsw_fdb_nchains,
	    mod_hash_null_valdtor, sizeof (void *));

	progress |= PROG_mfdb;

	/*
	 * Create the taskq which will process all the VIO
	 * control messages.
	 */
	(void) snprintf(qname, TASKQ_NAMELEN, "vsw_taskq%d", vswp->instance);
	if ((vswp->taskq_p = ddi_taskq_create(vswp->dip, qname, 1,
	    TASKQ_DEFAULTPRI, 0)) == NULL) {
		cmn_err(CE_WARN, "!vsw%d: Unable to create task queue",
		    vswp->instance);
		goto vsw_attach_fail;
	}

	progress |= PROG_taskq;

	/* prevent auto-detaching */
	if (ddi_prop_update_int(DDI_DEV_T_NONE, vswp->dip,
	    DDI_NO_AUTODETACH, 1) != DDI_SUCCESS) {
		cmn_err(CE_NOTE, "!Unable to set \"%s\" property for "
		    "instance %u", DDI_NO_AUTODETACH, instance);
	}

	/*
	 * The null switching function is set to avoid panic until
	 * switch mode is setup.
	 */
	vswp->vsw_switch_frame = vsw_switch_frame_nop;

	/*
	 * Setup the required switching mode,
	 * based on the mdprops that we read earlier.
	 * schedule a short timeout (0.1 sec) for the first time
	 * setup and avoid calling mac_open() directly here,
	 * others are regular timeout 3 secs.
	 */
	mutex_enter(&vswp->swtmout_lock);

	vswp->swtmout_enabled = B_TRUE;
	vswp->swtmout_id = timeout(vsw_setup_switching_timeout, vswp,
	    drv_usectohz(vsw_setup_switching_boot_delay));

	mutex_exit(&vswp->swtmout_lock);

	progress |= PROG_swmode;

	/* Register with mac layer as a provider */
	rv = vsw_mac_register(vswp);
	if (rv != 0)
		goto vsw_attach_fail;

	progress |= PROG_macreg;

	/*
	 * Now we have everything setup, register an interest in
	 * specific MD nodes.
	 *
	 * The callback is invoked in 2 cases, firstly if upon mdeg
	 * registration there are existing nodes which match our specified
	 * criteria, and secondly if the MD is changed (and again, there
	 * are nodes which we are interested in present within it. Note
	 * that our callback will be invoked even if our specified nodes
	 * have not actually changed).
	 *
	 */
	rv = vsw_mdeg_register(vswp);
	if (rv != 0)
		goto vsw_attach_fail;

	progress |= PROG_mdreg;

	WRITE_ENTER(&vsw_rw);
	vswp->next = vsw_head;
	vsw_head = vswp;
	RW_EXIT(&vsw_rw);

	ddi_report_dev(vswp->dip);
	return (DDI_SUCCESS);

vsw_attach_fail:
	DERR(NULL, "vsw_attach: failed");

	if (progress & PROG_mdreg) {
		vsw_mdeg_unregister(vswp);
		(void) vsw_detach_ports(vswp);
	}

	if (progress & PROG_macreg)
		(void) vsw_mac_unregister(vswp);

	if (progress & PROG_swmode) {
		vsw_stop_switching_timeout(vswp);
		vsw_hio_cleanup(vswp);
		WRITE_ENTER(&vswp->mac_rwlock);
		vsw_mac_detach(vswp);
		vsw_mac_close(vswp);
		RW_EXIT(&vswp->mac_rwlock);
	}

	if (progress & PROG_taskq)
		ddi_taskq_destroy(vswp->taskq_p);

	if (progress & PROG_mfdb)
		mod_hash_destroy_hash(vswp->mfdb);

	if (progress & PROG_fdb) {
		vsw_destroy_vlans(vswp, VSW_LOCALDEV);
		mod_hash_destroy_hash(vswp->fdb_hashp);
	}

	if (progress & PROG_readmd) {
		if (VSW_PRI_ETH_DEFINED(vswp)) {
			kmem_free(vswp->pri_types,
			    sizeof (uint16_t) * vswp->pri_num_types);
		}
		(void) vio_destroy_mblks(vswp->pri_tx_vmp);
	}

	if (progress & PROG_locks) {
		rw_destroy(&vswp->plist.lockrw);
		rw_destroy(&vswp->mfdbrw);
		rw_destroy(&vswp->mac_rwlock);
		rw_destroy(&vswp->if_lockrw);
		mutex_destroy(&vswp->swtmout_lock);
		mutex_destroy(&vswp->mca_lock);
		mutex_destroy(&vswp->hw_lock);
	}

	ddi_soft_state_free(vsw_state, instance);
	return (DDI_FAILURE);
}

static int
vsw_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	vio_mblk_pool_t		*poolp, *npoolp;
	vsw_t			**vswpp, *vswp;
	int 			instance;

	instance = ddi_get_instance(dip);
	vswp = ddi_get_soft_state(vsw_state, instance);

	if (vswp == NULL) {
		return (DDI_FAILURE);
	}

	switch (cmd) {
	case DDI_DETACH:
		break;
	case DDI_SUSPEND:
	case DDI_PM_SUSPEND:
	default:
		return (DDI_FAILURE);
	}

	D2(vswp, "detaching instance %d", instance);

	/* Stop any pending timeout to setup switching mode. */
	vsw_stop_switching_timeout(vswp);

	if (vswp->if_state & VSW_IF_REG) {
		if (vsw_mac_unregister(vswp) != 0) {
			cmn_err(CE_WARN, "!vsw%d: Unable to detach from "
			    "MAC layer", vswp->instance);
			return (DDI_FAILURE);
		}
	}

	vsw_mdeg_unregister(vswp);

	/* remove mac layer callback */
	WRITE_ENTER(&vswp->mac_rwlock);
	if ((vswp->mh != NULL) && (vswp->mrh != NULL)) {
		mac_rx_remove(vswp->mh, vswp->mrh, B_TRUE);
		vswp->mrh = NULL;
	}
	RW_EXIT(&vswp->mac_rwlock);

	if (vsw_detach_ports(vswp) != 0) {
		cmn_err(CE_WARN, "!vsw%d: Unable to unconfigure ports",
		    vswp->instance);
		return (DDI_FAILURE);
	}

	rw_destroy(&vswp->if_lockrw);

	/* cleanup HybridIO */
	vsw_hio_cleanup(vswp);

	mutex_destroy(&vswp->hw_lock);

	/*
	 * Now that the ports have been deleted, stop and close
	 * the physical device.
	 */
	WRITE_ENTER(&vswp->mac_rwlock);

	vsw_mac_detach(vswp);
	vsw_mac_close(vswp);

	RW_EXIT(&vswp->mac_rwlock);

	rw_destroy(&vswp->mac_rwlock);
	mutex_destroy(&vswp->swtmout_lock);

	/*
	 * Destroy any free pools that may still exist.
	 */
	poolp = vswp->rxh;
	while (poolp != NULL) {
		npoolp = vswp->rxh = poolp->nextp;
		if (vio_destroy_mblks(poolp) != 0) {
			vswp->rxh = poolp;
			return (DDI_FAILURE);
		}
		poolp = npoolp;
	}

	/*
	 * Remove this instance from any entries it may be on in
	 * the hash table by using the list of addresses maintained
	 * in the vsw_t structure.
	 */
	vsw_del_mcst_vsw(vswp);

	vswp->mcap = NULL;
	mutex_destroy(&vswp->mca_lock);

	/*
	 * By now any pending tasks have finished and the underlying
	 * ldc's have been destroyed, so its safe to delete the control
	 * message taskq.
	 */
	if (vswp->taskq_p != NULL)
		ddi_taskq_destroy(vswp->taskq_p);

	/*
	 * At this stage all the data pointers in the hash table
	 * should be NULL, as all the ports have been removed and will
	 * have deleted themselves from the port lists which the data
	 * pointers point to. Hence we can destroy the table using the
	 * default destructors.
	 */
	D2(vswp, "vsw_detach: destroying hash tables..");
	vsw_destroy_vlans(vswp, VSW_LOCALDEV);
	mod_hash_destroy_hash(vswp->fdb_hashp);
	vswp->fdb_hashp = NULL;

	WRITE_ENTER(&vswp->mfdbrw);
	mod_hash_destroy_hash(vswp->mfdb);
	vswp->mfdb = NULL;
	RW_EXIT(&vswp->mfdbrw);
	rw_destroy(&vswp->mfdbrw);

	/* free pri_types table */
	if (VSW_PRI_ETH_DEFINED(vswp)) {
		kmem_free(vswp->pri_types,
		    sizeof (uint16_t) * vswp->pri_num_types);
		(void) vio_destroy_mblks(vswp->pri_tx_vmp);
	}

	ddi_remove_minor_node(dip, NULL);

	rw_destroy(&vswp->plist.lockrw);
	WRITE_ENTER(&vsw_rw);
	for (vswpp = &vsw_head; *vswpp; vswpp = &(*vswpp)->next) {
		if (*vswpp == vswp) {
			*vswpp = vswp->next;
			break;
		}
	}
	RW_EXIT(&vsw_rw);
	ddi_soft_state_free(vsw_state, instance);

	return (DDI_SUCCESS);
}

static int
vsw_getinfo(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg, void **result)
{
	_NOTE(ARGUNUSED(dip))

	vsw_t	*vswp = NULL;
	dev_t	dev = (dev_t)arg;
	int	instance;

	instance = getminor(dev);

	switch (infocmd) {
	case DDI_INFO_DEVT2DEVINFO:
		if ((vswp = ddi_get_soft_state(vsw_state, instance)) == NULL) {
			*result = NULL;
			return (DDI_FAILURE);
		}
		*result = vswp->dip;
		return (DDI_SUCCESS);

	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *)(uintptr_t)instance;
		return (DDI_SUCCESS);

	default:
		*result = NULL;
		return (DDI_FAILURE);
	}
}

/*
 * Get the value of the "vsw-phys-dev" property in the specified
 * node. This property is the name of the physical device that
 * the virtual switch will use to talk to the outside world.
 *
 * Note it is valid for this property to be NULL (but the property
 * itself must exist). Callers of this routine should verify that
 * the value returned is what they expected (i.e. either NULL or non NULL).
 *
 * On success returns value of the property in region pointed to by
 * the 'name' argument, and with return value of 0. Otherwise returns 1.
 */
static int
vsw_get_md_physname(vsw_t *vswp, md_t *mdp, mde_cookie_t node, char *name)
{
	int		len = 0;
	int		instance;
	char		*physname = NULL;
	char		*dev;
	const char	*dev_name;
	char		myname[MAXNAMELEN];

	dev_name = ddi_driver_name(vswp->dip);
	instance = ddi_get_instance(vswp->dip);
	(void) snprintf(myname, MAXNAMELEN, "%s%d", dev_name, instance);

	if (md_get_prop_data(mdp, node, physdev_propname,
	    (uint8_t **)(&physname), &len) != 0) {
		cmn_err(CE_WARN, "!vsw%d: Unable to get name(s) of physical "
		    "device(s) from MD", vswp->instance);
		return (1);
	} else if ((strlen(physname) + 1) > LIFNAMSIZ) {
		cmn_err(CE_WARN, "!vsw%d: %s is too long a device name",
		    vswp->instance, physname);
		return (1);
	} else if (strcmp(myname, physname) == 0) {
		/*
		 * Prevent the vswitch from opening itself as the
		 * network device.
		 */
		cmn_err(CE_WARN, "!vsw%d: %s is an invalid device name",
		    vswp->instance, physname);
		return (1);
	} else {
		(void) strncpy(name, physname, strlen(physname) + 1);
		D2(vswp, "%s: using first device specified (%s)",
		    __func__, physname);
	}

#ifdef DEBUG
	/*
	 * As a temporary measure to aid testing we check to see if there
	 * is a vsw.conf file present. If there is we use the value of the
	 * vsw_physname property in the file as the name of the physical
	 * device, overriding the value from the MD.
	 *
	 * There may be multiple devices listed, but for the moment
	 * we just use the first one.
	 */
	if (ddi_prop_lookup_string(DDI_DEV_T_ANY, vswp->dip, 0,
	    "vsw_physname", &dev) == DDI_PROP_SUCCESS) {
		if ((strlen(dev) + 1) > LIFNAMSIZ) {
			cmn_err(CE_WARN, "vsw%d: %s is too long a device name",
			    vswp->instance, dev);
			ddi_prop_free(dev);
			return (1);
		} else {
			cmn_err(CE_NOTE, "vsw%d: Using device name (%s) from "
			    "config file", vswp->instance, dev);

			(void) strncpy(name, dev, strlen(dev) + 1);
		}

		ddi_prop_free(dev);
	}
#endif

	return (0);
}

/*
 * Read the 'vsw-switch-mode' property from the specified MD node.
 *
 * Returns 0 on success and the number of modes found in 'found',
 * otherwise returns 1.
 */
static int
vsw_get_md_smodes(vsw_t *vswp, md_t *mdp, mde_cookie_t node,
						uint8_t *modes, int *found)
{
	int		len = 0;
	int		smode_num = 0;
	char		*smode = NULL;
	char		*curr_mode = NULL;

	D1(vswp, "%s: enter", __func__);

	/*
	 * Get the switch-mode property. The modes are listed in
	 * decreasing order of preference, i.e. prefered mode is
	 * first item in list.
	 */
	len = 0;
	smode_num = 0;
	if (md_get_prop_data(mdp, node, smode_propname,
	    (uint8_t **)(&smode), &len) != 0) {
		/*
		 * Unable to get switch-mode property from MD, nothing
		 * more we can do.
		 */
		cmn_err(CE_WARN, "!vsw%d: Unable to get switch mode property"
		    " from the MD", vswp->instance);
		*found = 0;
		return (1);
	}

	curr_mode = smode;
	/*
	 * Modes of operation:
	 * 'switched'	 - layer 2 switching, underlying HW in
	 *			programmed mode.
	 * 'promiscuous' - layer 2 switching, underlying HW in
	 *			promiscuous mode.
	 * 'routed'	 - layer 3 (i.e. IP) routing, underlying HW
	 *			in non-promiscuous mode.
	 */
	while ((curr_mode < (smode + len)) && (smode_num < NUM_SMODES)) {
		D2(vswp, "%s: curr_mode = [%s]", __func__, curr_mode);
		if (strcmp(curr_mode, "switched") == 0) {
			modes[smode_num++] = VSW_LAYER2;
		} else if (strcmp(curr_mode, "promiscuous") == 0) {
			modes[smode_num++] = VSW_LAYER2_PROMISC;
		} else if (strcmp(curr_mode, "routed") == 0) {
			modes[smode_num++] = VSW_LAYER3;
		} else {
			DWARN(vswp, "%s: Unknown switch mode %s, "
			    "setting to default 'switched' mode",
			    __func__, curr_mode);
			modes[smode_num++] = VSW_LAYER2;
		}
		curr_mode += strlen(curr_mode) + 1;
	}
	*found = smode_num;

	D2(vswp, "%s: %d modes found", __func__, smode_num);

	D1(vswp, "%s: exit", __func__);

	return (0);
}

/*
 * Register with the MAC layer as a network device, so we
 * can be plumbed if necessary.
 */
static int
vsw_mac_register(vsw_t *vswp)
{
	mac_register_t	*macp;
	int		rv;

	D1(vswp, "%s: enter", __func__);

	if ((macp = mac_alloc(MAC_VERSION)) == NULL)
		return (EINVAL);
	macp->m_type_ident = MAC_PLUGIN_IDENT_ETHER;
	macp->m_driver = vswp;
	macp->m_dip = vswp->dip;
	macp->m_src_addr = (uint8_t *)&vswp->if_addr;
	macp->m_callbacks = &vsw_m_callbacks;
	macp->m_min_sdu = 0;
	macp->m_max_sdu = vsw_ethermtu;
	macp->m_margin = VLAN_TAGSZ;
	rv = mac_register(macp, &vswp->if_mh);
	mac_free(macp);
	if (rv != 0) {
		/*
		 * Treat this as a non-fatal error as we may be
		 * able to operate in some other mode.
		 */
		cmn_err(CE_NOTE, "!vsw%d: Unable to register as "
		    "a provider with MAC layer", vswp->instance);
		return (rv);
	}

	vswp->if_state |= VSW_IF_REG;

	vswp->max_frame_size = vsw_ethermtu + sizeof (struct ether_header)
	    + VLAN_TAGSZ;

	D1(vswp, "%s: exit", __func__);

	return (rv);
}

static int
vsw_mac_unregister(vsw_t *vswp)
{
	int		rv = 0;

	D1(vswp, "%s: enter", __func__);

	WRITE_ENTER(&vswp->if_lockrw);

	if (vswp->if_state & VSW_IF_REG) {
		rv = mac_unregister(vswp->if_mh);
		if (rv != 0) {
			DWARN(vswp, "%s: unable to unregister from MAC "
			    "framework", __func__);

			RW_EXIT(&vswp->if_lockrw);
			D1(vswp, "%s: fail exit", __func__);
			return (rv);
		}

		/* mark i/f as down and unregistered */
		vswp->if_state &= ~(VSW_IF_UP | VSW_IF_REG);
	}
	RW_EXIT(&vswp->if_lockrw);

	D1(vswp, "%s: exit", __func__);

	return (rv);
}

static int
vsw_m_stat(void *arg, uint_t stat, uint64_t *val)
{
	vsw_t			*vswp = (vsw_t *)arg;

	D1(vswp, "%s: enter", __func__);

	WRITE_ENTER(&vswp->mac_rwlock);
	if (vswp->mh == NULL) {
		RW_EXIT(&vswp->mac_rwlock);
		return (EINVAL);
	}

	/* return stats from underlying device */
	*val = mac_stat_get(vswp->mh, stat);

	RW_EXIT(&vswp->mac_rwlock);

	return (0);
}

static void
vsw_m_stop(void *arg)
{
	vsw_t		*vswp = (vsw_t *)arg;

	D1(vswp, "%s: enter", __func__);

	WRITE_ENTER(&vswp->if_lockrw);
	vswp->if_state &= ~VSW_IF_UP;
	RW_EXIT(&vswp->if_lockrw);

	mutex_enter(&vswp->hw_lock);

	(void) vsw_unset_hw(vswp, NULL, VSW_LOCALDEV);

	if (vswp->recfg_reqd)
		vsw_reconfig_hw(vswp);

	mutex_exit(&vswp->hw_lock);

	D1(vswp, "%s: exit (state = %d)", __func__, vswp->if_state);
}

static int
vsw_m_start(void *arg)
{
	vsw_t		*vswp = (vsw_t *)arg;

	D1(vswp, "%s: enter", __func__);

	WRITE_ENTER(&vswp->if_lockrw);

	vswp->if_state |= VSW_IF_UP;

	if (vswp->switching_setup_done == B_FALSE) {
		/*
		 * If the switching mode has not been setup yet, just
		 * return. The unicast address will be programmed
		 * after the physical device is successfully setup by the
		 * timeout handler.
		 */
		RW_EXIT(&vswp->if_lockrw);
		return (0);
	}

	/* if in layer2 mode, program unicast address. */
	if (vswp->mh != NULL) {
		mutex_enter(&vswp->hw_lock);
		(void) vsw_set_hw(vswp, NULL, VSW_LOCALDEV);
		mutex_exit(&vswp->hw_lock);
	}

	RW_EXIT(&vswp->if_lockrw);

	D1(vswp, "%s: exit (state = %d)", __func__, vswp->if_state);
	return (0);
}

/*
 * Change the local interface address.
 *
 * Note: we don't support this entry point. The local
 * mac address of the switch can only be changed via its
 * MD node properties.
 */
static int
vsw_m_unicst(void *arg, const uint8_t *macaddr)
{
	_NOTE(ARGUNUSED(arg, macaddr))

	return (DDI_FAILURE);
}

static int
vsw_m_multicst(void *arg, boolean_t add, const uint8_t *mca)
{
	vsw_t		*vswp = (vsw_t *)arg;
	mcst_addr_t	*mcst_p = NULL;
	uint64_t	addr = 0x0;
	int		i, ret = 0;

	D1(vswp, "%s: enter", __func__);

	/*
	 * Convert address into form that can be used
	 * as hash table key.
	 */
	for (i = 0; i < ETHERADDRL; i++) {
		addr = (addr << 8) | mca[i];
	}

	D2(vswp, "%s: addr = 0x%llx", __func__, addr);

	if (add) {
		D2(vswp, "%s: adding multicast", __func__);
		if (vsw_add_mcst(vswp, VSW_LOCALDEV, addr, NULL) == 0) {
			/*
			 * Update the list of multicast addresses
			 * contained within the vsw_t structure to
			 * include this new one.
			 */
			mcst_p = kmem_zalloc(sizeof (mcst_addr_t), KM_NOSLEEP);
			if (mcst_p == NULL) {
				DERR(vswp, "%s unable to alloc mem", __func__);
				(void) vsw_del_mcst(vswp,
				    VSW_LOCALDEV, addr, NULL);
				return (1);
			}
			mcst_p->addr = addr;
			ether_copy(mca, &mcst_p->mca);

			/*
			 * Call into the underlying driver to program the
			 * address into HW.
			 */
			WRITE_ENTER(&vswp->mac_rwlock);
			if (vswp->mh != NULL) {
				ret = mac_multicst_add(vswp->mh, mca);
				if (ret != 0) {
					cmn_err(CE_NOTE, "!vsw%d: unable to "
					    "add multicast address",
					    vswp->instance);
					RW_EXIT(&vswp->mac_rwlock);
					(void) vsw_del_mcst(vswp,
					    VSW_LOCALDEV, addr, NULL);
					kmem_free(mcst_p, sizeof (*mcst_p));
					return (ret);
				}
				mcst_p->mac_added = B_TRUE;
			}
			RW_EXIT(&vswp->mac_rwlock);

			mutex_enter(&vswp->mca_lock);
			mcst_p->nextp = vswp->mcap;
			vswp->mcap = mcst_p;
			mutex_exit(&vswp->mca_lock);
		} else {
			cmn_err(CE_NOTE, "!vsw%d: unable to add multicast "
			    "address", vswp->instance);
		}
		return (ret);
	}

	D2(vswp, "%s: removing multicast", __func__);
	/*
	 * Remove the address from the hash table..
	 */
	if (vsw_del_mcst(vswp, VSW_LOCALDEV, addr, NULL) == 0) {

		/*
		 * ..and then from the list maintained in the
		 * vsw_t structure.
		 */
		mcst_p = vsw_del_addr(VSW_LOCALDEV, vswp, addr);
		ASSERT(mcst_p != NULL);

		WRITE_ENTER(&vswp->mac_rwlock);
		if (vswp->mh != NULL && mcst_p->mac_added) {
			(void) mac_multicst_remove(vswp->mh, mca);
			mcst_p->mac_added = B_FALSE;
		}
		RW_EXIT(&vswp->mac_rwlock);
		kmem_free(mcst_p, sizeof (*mcst_p));
	}

	D1(vswp, "%s: exit", __func__);

	return (0);
}

static int
vsw_m_promisc(void *arg, boolean_t on)
{
	vsw_t		*vswp = (vsw_t *)arg;

	D1(vswp, "%s: enter", __func__);

	WRITE_ENTER(&vswp->if_lockrw);
	if (on)
		vswp->if_state |= VSW_IF_PROMISC;
	else
		vswp->if_state &= ~VSW_IF_PROMISC;
	RW_EXIT(&vswp->if_lockrw);

	D1(vswp, "%s: exit", __func__);

	return (0);
}

static mblk_t *
vsw_m_tx(void *arg, mblk_t *mp)
{
	vsw_t		*vswp = (vsw_t *)arg;

	D1(vswp, "%s: enter", __func__);

	mp = vsw_vlan_frame_pretag(vswp, VSW_LOCALDEV, mp);

	if (mp == NULL) {
		return (NULL);
	}

	vswp->vsw_switch_frame(vswp, mp, VSW_LOCALDEV, NULL, NULL);

	D1(vswp, "%s: exit", __func__);

	return (NULL);
}

/*
 * Register for machine description (MD) updates.
 *
 * Returns 0 on success, 1 on failure.
 */
static int
vsw_mdeg_register(vsw_t *vswp)
{
	mdeg_prop_spec_t	*pspecp;
	mdeg_node_spec_t	*inst_specp;
	mdeg_handle_t		mdeg_hdl, mdeg_port_hdl;
	size_t			templatesz;
	int			rv;

	D1(vswp, "%s: enter", __func__);

	/*
	 * Allocate and initialize a per-instance copy
	 * of the global property spec array that will
	 * uniquely identify this vsw instance.
	 */
	templatesz = sizeof (vsw_prop_template);
	pspecp = kmem_zalloc(templatesz, KM_SLEEP);

	bcopy(vsw_prop_template, pspecp, templatesz);

	VSW_SET_MDEG_PROP_INST(pspecp, vswp->regprop);

	/* initialize the complete prop spec structure */
	inst_specp = kmem_zalloc(sizeof (mdeg_node_spec_t), KM_SLEEP);
	inst_specp->namep = "virtual-device";
	inst_specp->specp = pspecp;

	D2(vswp, "%s: instance %d registering with mdeg", __func__,
	    vswp->regprop);
	/*
	 * Register an interest in 'virtual-device' nodes with a
	 * 'name' property of 'virtual-network-switch'
	 */
	rv = mdeg_register(inst_specp, &vdev_match, vsw_mdeg_cb,
	    (void *)vswp, &mdeg_hdl);
	if (rv != MDEG_SUCCESS) {
		DERR(vswp, "%s: mdeg_register failed (%d) for vsw node",
		    __func__, rv);
		goto mdeg_reg_fail;
	}

	/*
	 * Register an interest in 'vsw-port' nodes.
	 */
	rv = mdeg_register(inst_specp, &vport_match, vsw_port_mdeg_cb,
	    (void *)vswp, &mdeg_port_hdl);
	if (rv != MDEG_SUCCESS) {
		DERR(vswp, "%s: mdeg_register failed (%d)\n", __func__, rv);
		(void) mdeg_unregister(mdeg_hdl);
		goto mdeg_reg_fail;
	}

	/* save off data that will be needed later */
	vswp->inst_spec = inst_specp;
	vswp->mdeg_hdl = mdeg_hdl;
	vswp->mdeg_port_hdl = mdeg_port_hdl;

	D1(vswp, "%s: exit", __func__);
	return (0);

mdeg_reg_fail:
	cmn_err(CE_WARN, "!vsw%d: Unable to register MDEG callbacks",
	    vswp->instance);
	kmem_free(pspecp, templatesz);
	kmem_free(inst_specp, sizeof (mdeg_node_spec_t));

	vswp->mdeg_hdl = NULL;
	vswp->mdeg_port_hdl = NULL;

	return (1);
}

static void
vsw_mdeg_unregister(vsw_t *vswp)
{
	D1(vswp, "vsw_mdeg_unregister: enter");

	if (vswp->mdeg_hdl != NULL)
		(void) mdeg_unregister(vswp->mdeg_hdl);

	if (vswp->mdeg_port_hdl != NULL)
		(void) mdeg_unregister(vswp->mdeg_port_hdl);

	if (vswp->inst_spec != NULL) {
		if (vswp->inst_spec->specp != NULL) {
			(void) kmem_free(vswp->inst_spec->specp,
			    sizeof (vsw_prop_template));
			vswp->inst_spec->specp = NULL;
		}

		(void) kmem_free(vswp->inst_spec, sizeof (mdeg_node_spec_t));
		vswp->inst_spec = NULL;
	}

	D1(vswp, "vsw_mdeg_unregister: exit");
}

/*
 * Mdeg callback invoked for the vsw node itself.
 */
static int
vsw_mdeg_cb(void *cb_argp, mdeg_result_t *resp)
{
	vsw_t		*vswp;
	md_t		*mdp;
	mde_cookie_t	node;
	uint64_t	inst;
	char		*node_name = NULL;

	if (resp == NULL)
		return (MDEG_FAILURE);

	vswp = (vsw_t *)cb_argp;

	D1(vswp, "%s: added %d : removed %d : curr matched %d"
	    " : prev matched %d", __func__, resp->added.nelem,
	    resp->removed.nelem, resp->match_curr.nelem,
	    resp->match_prev.nelem);

	/*
	 * We get an initial callback for this node as 'added'
	 * after registering with mdeg. Note that we would have
	 * already gathered information about this vsw node by
	 * walking MD earlier during attach (in vsw_read_mdprops()).
	 * So, there is a window where the properties of this
	 * node might have changed when we get this initial 'added'
	 * callback. We handle this as if an update occured
	 * and invoke the same function which handles updates to
	 * the properties of this vsw-node if any.
	 *
	 * A non-zero 'match' value indicates that the MD has been
	 * updated and that a virtual-network-switch node is
	 * present which may or may not have been updated. It is
	 * up to the clients to examine their own nodes and
	 * determine if they have changed.
	 */
	if (resp->added.nelem != 0) {

		if (resp->added.nelem != 1) {
			cmn_err(CE_NOTE, "!vsw%d: number of nodes added "
			    "invalid: %d\n", vswp->instance, resp->added.nelem);
			return (MDEG_FAILURE);
		}

		mdp = resp->added.mdp;
		node = resp->added.mdep[0];

	} else if (resp->match_curr.nelem != 0) {

		if (resp->match_curr.nelem != 1) {
			cmn_err(CE_NOTE, "!vsw%d: number of nodes updated "
			    "invalid: %d\n", vswp->instance,
			    resp->match_curr.nelem);
			return (MDEG_FAILURE);
		}

		mdp = resp->match_curr.mdp;
		node = resp->match_curr.mdep[0];

	} else {
		return (MDEG_FAILURE);
	}

	/* Validate name and instance */
	if (md_get_prop_str(mdp, node, "name", &node_name) != 0) {
		DERR(vswp, "%s: unable to get node name\n",  __func__);
		return (MDEG_FAILURE);
	}

	/* is this a virtual-network-switch? */
	if (strcmp(node_name, vsw_propname) != 0) {
		DERR(vswp, "%s: Invalid node name: %s\n",
		    __func__, node_name);
		return (MDEG_FAILURE);
	}

	if (md_get_prop_val(mdp, node, "cfg-handle", &inst)) {
		DERR(vswp, "%s: prop(cfg-handle) not found\n",
		    __func__);
		return (MDEG_FAILURE);
	}

	/* is this the right instance of vsw? */
	if (inst != vswp->regprop) {
		DERR(vswp, "%s: Invalid cfg-handle: %lx\n",
		    __func__, inst);
		return (MDEG_FAILURE);
	}

	vsw_update_md_prop(vswp, mdp, node);

	return (MDEG_SUCCESS);
}

/*
 * Mdeg callback invoked for changes to the vsw-port nodes
 * under the vsw node.
 */
static int
vsw_port_mdeg_cb(void *cb_argp, mdeg_result_t *resp)
{
	vsw_t		*vswp;
	int		idx;
	md_t		*mdp;
	mde_cookie_t	node;
	uint64_t	inst;
	int		rv;

	if ((resp == NULL) || (cb_argp == NULL))
		return (MDEG_FAILURE);

	vswp = (vsw_t *)cb_argp;

	D2(vswp, "%s: added %d : removed %d : curr matched %d"
	    " : prev matched %d", __func__, resp->added.nelem,
	    resp->removed.nelem, resp->match_curr.nelem,
	    resp->match_prev.nelem);

	/* process added ports */
	for (idx = 0; idx < resp->added.nelem; idx++) {
		mdp = resp->added.mdp;
		node = resp->added.mdep[idx];

		D2(vswp, "%s: adding node(%d) 0x%lx", __func__, idx, node);

		if ((rv = vsw_port_add(vswp, mdp, &node)) != 0) {
			cmn_err(CE_WARN, "!vsw%d: Unable to add new port "
			    "(0x%lx), err=%d", vswp->instance, node, rv);
		}
	}

	/* process removed ports */
	for (idx = 0; idx < resp->removed.nelem; idx++) {
		mdp = resp->removed.mdp;
		node = resp->removed.mdep[idx];

		if (md_get_prop_val(mdp, node, id_propname, &inst)) {
			DERR(vswp, "%s: prop(%s) not found in port(%d)",
			    __func__, id_propname, idx);
			continue;
		}

		D2(vswp, "%s: removing node(%d) 0x%lx", __func__, idx, node);

		if (vsw_port_detach(vswp, inst) != 0) {
			cmn_err(CE_WARN, "!vsw%d: Unable to remove port %ld",
			    vswp->instance, inst);
		}
	}

	for (idx = 0; idx < resp->match_curr.nelem; idx++) {
		(void) vsw_port_update(vswp, resp->match_curr.mdp,
		    resp->match_curr.mdep[idx],
		    resp->match_prev.mdp,
		    resp->match_prev.mdep[idx]);
	}

	D1(vswp, "%s: exit", __func__);

	return (MDEG_SUCCESS);
}

/*
 * Scan the machine description for this instance of vsw
 * and read its properties. Called only from vsw_attach().
 * Returns: 0 on success, 1 on failure.
 */
static int
vsw_read_mdprops(vsw_t *vswp)
{
	md_t		*mdp = NULL;
	mde_cookie_t	rootnode;
	mde_cookie_t	*listp = NULL;
	uint64_t	inst;
	uint64_t	cfgh;
	char		*name;
	int		rv = 1;
	int		num_nodes = 0;
	int		num_devs = 0;
	int		listsz = 0;
	int		i;

	/*
	 * In each 'virtual-device' node in the MD there is a
	 * 'cfg-handle' property which is the MD's concept of
	 * an instance number (this may be completely different from
	 * the device drivers instance #). OBP reads that value and
	 * stores it in the 'reg' property of the appropriate node in
	 * the device tree. We first read this reg property and use this
	 * to compare against the 'cfg-handle' property of vsw nodes
	 * in MD to get to this specific vsw instance and then read
	 * other properties that we are interested in.
	 * We also cache the value of 'reg' property and use it later
	 * to register callbacks with mdeg (see vsw_mdeg_register())
	 */
	inst = ddi_prop_get_int(DDI_DEV_T_ANY, vswp->dip,
	    DDI_PROP_DONTPASS, reg_propname, -1);
	if (inst == -1) {
		cmn_err(CE_NOTE, "!vsw%d: Unable to read %s property from "
		    "OBP device tree", vswp->instance, reg_propname);
		return (rv);
	}

	vswp->regprop = inst;

	if ((mdp = md_get_handle()) == NULL) {
		DWARN(vswp, "%s: cannot init MD\n", __func__);
		return (rv);
	}

	num_nodes = md_node_count(mdp);
	ASSERT(num_nodes > 0);

	listsz = num_nodes * sizeof (mde_cookie_t);
	listp = (mde_cookie_t *)kmem_zalloc(listsz, KM_SLEEP);

	rootnode = md_root_node(mdp);

	/* search for all "virtual_device" nodes */
	num_devs = md_scan_dag(mdp, rootnode,
	    md_find_name(mdp, vdev_propname),
	    md_find_name(mdp, "fwd"), listp);
	if (num_devs <= 0) {
		DWARN(vswp, "%s: invalid num_devs:%d\n", __func__, num_devs);
		goto vsw_readmd_exit;
	}

	/*
	 * Now loop through the list of virtual-devices looking for
	 * devices with name "virtual-network-switch" and for each
	 * such device compare its instance with what we have from
	 * the 'reg' property to find the right node in MD and then
	 * read all its properties.
	 */
	for (i = 0; i < num_devs; i++) {

		if (md_get_prop_str(mdp, listp[i], "name", &name) != 0) {
			DWARN(vswp, "%s: name property not found\n",
			    __func__);
			goto vsw_readmd_exit;
		}

		/* is this a virtual-network-switch? */
		if (strcmp(name, vsw_propname) != 0)
			continue;

		if (md_get_prop_val(mdp, listp[i], "cfg-handle", &cfgh) != 0) {
			DWARN(vswp, "%s: cfg-handle property not found\n",
			    __func__);
			goto vsw_readmd_exit;
		}

		/* is this the required instance of vsw? */
		if (inst != cfgh)
			continue;

		/* now read all properties of this vsw instance */
		rv = vsw_get_initial_md_properties(vswp, mdp, listp[i]);
		break;
	}

vsw_readmd_exit:

	kmem_free(listp, listsz);
	(void) md_fini_handle(mdp);
	return (rv);
}

/*
 * Read the initial start-of-day values from the specified MD node.
 */
static int
vsw_get_initial_md_properties(vsw_t *vswp, md_t *mdp, mde_cookie_t node)
{
	int		i;
	uint64_t 	macaddr = 0;

	D1(vswp, "%s: enter", __func__);

	if (vsw_get_md_physname(vswp, mdp, node, vswp->physname) != 0) {
		return (1);
	}

	/* mac address for vswitch device itself */
	if (md_get_prop_val(mdp, node, macaddr_propname, &macaddr) != 0) {
		cmn_err(CE_WARN, "!vsw%d: Unable to get MAC address from MD",
		    vswp->instance);
		return (1);
	}

	vsw_save_lmacaddr(vswp, macaddr);

	if (vsw_get_md_smodes(vswp, mdp, node, vswp->smode, &vswp->smode_num)) {
		DWARN(vswp, "%s: Unable to read %s property from MD, "
		    "defaulting to 'switched' mode",
		    __func__, smode_propname);

		for (i = 0; i < NUM_SMODES; i++)
			vswp->smode[i] = VSW_LAYER2;

		vswp->smode_num = NUM_SMODES;
	} else {
		ASSERT(vswp->smode_num != 0);
	}

	/* read vlan id properties of this vsw instance */
	vsw_vlan_read_ids(vswp, VSW_LOCALDEV, mdp, node, &vswp->pvid,
	    &vswp->vids, &vswp->nvids, &vswp->default_vlan_id);

	/* read priority-ether-types */
	vsw_read_pri_eth_types(vswp, mdp, node);

	D1(vswp, "%s: exit", __func__);
	return (0);
}

/*
 * Read vlan id properties of the given MD node.
 * Arguments:
 *   arg:          device argument(vsw device or a port)
 *   type:         type of arg; VSW_LOCALDEV(vsw device) or VSW_VNETPORT(port)
 *   mdp:          machine description
 *   node:         md node cookie
 *
 * Returns:
 *   pvidp:        port-vlan-id of the node
 *   vidspp:       list of vlan-ids of the node
 *   nvidsp:       # of vlan-ids in the list
 *   default_idp:  default-vlan-id of the node(if node is vsw device)
 */
static void
vsw_vlan_read_ids(void *arg, int type, md_t *mdp, mde_cookie_t node,
	uint16_t *pvidp, uint16_t **vidspp, uint16_t *nvidsp,
	uint16_t *default_idp)
{
	vsw_t		*vswp;
	vsw_port_t	*portp;
	char		*pvid_propname;
	char		*vid_propname;
	uint_t		nvids = 0;
	uint32_t	vids_size;
	int		rv;
	int		i;
	uint64_t	*data;
	uint64_t	val;
	int		size;
	int		inst;

	if (type == VSW_LOCALDEV) {

		vswp = (vsw_t *)arg;
		pvid_propname = vsw_pvid_propname;
		vid_propname = vsw_vid_propname;
		inst = vswp->instance;

	} else if (type == VSW_VNETPORT) {

		portp = (vsw_port_t *)arg;
		vswp = portp->p_vswp;
		pvid_propname = port_pvid_propname;
		vid_propname = port_vid_propname;
		inst = portp->p_instance;

	} else {
		return;
	}

	if (type == VSW_LOCALDEV && default_idp != NULL) {
		rv = md_get_prop_val(mdp, node, vsw_dvid_propname, &val);
		if (rv != 0) {
			DWARN(vswp, "%s: prop(%s) not found", __func__,
			    vsw_dvid_propname);

			*default_idp = vsw_default_vlan_id;
		} else {
			*default_idp = val & 0xFFF;
			D2(vswp, "%s: %s(%d): (%d)\n", __func__,
			    vsw_dvid_propname, inst, *default_idp);
		}
	}

	rv = md_get_prop_val(mdp, node, pvid_propname, &val);
	if (rv != 0) {
		DWARN(vswp, "%s: prop(%s) not found", __func__, pvid_propname);
		*pvidp = vsw_default_vlan_id;
	} else {

		*pvidp = val & 0xFFF;
		D2(vswp, "%s: %s(%d): (%d)\n", __func__,
		    pvid_propname, inst, *pvidp);
	}

	rv = md_get_prop_data(mdp, node, vid_propname, (uint8_t **)&data,
	    &size);
	if (rv != 0) {
		D2(vswp, "%s: prop(%s) not found", __func__, vid_propname);
		size = 0;
	} else {
		size /= sizeof (uint64_t);
	}
	nvids = size;

	if (nvids != 0) {
		D2(vswp, "%s: %s(%d): ", __func__, vid_propname, inst);
		vids_size = sizeof (uint16_t) * nvids;
		*vidspp = kmem_zalloc(vids_size, KM_SLEEP);
		for (i = 0; i < nvids; i++) {
			(*vidspp)[i] = data[i] & 0xFFFF;
			D2(vswp, " %d ", (*vidspp)[i]);
		}
		D2(vswp, "\n");
	}

	*nvidsp = nvids;
}

/*
 * This function reads "priority-ether-types" property from md. This property
 * is used to enable support for priority frames. Applications which need
 * guaranteed and timely delivery of certain high priority frames to/from
 * a vnet or vsw within ldoms, should configure this property by providing
 * the ether type(s) for which the priority facility is needed.
 * Normal data frames are delivered over a ldc channel using the descriptor
 * ring mechanism which is constrained by factors such as descriptor ring size,
 * the rate at which the ring is processed at the peer ldc end point, etc.
 * The priority mechanism provides an Out-Of-Band path to send/receive frames
 * as raw pkt data (VIO_PKT_DATA) messages over the channel, avoiding the
 * descriptor ring path and enables a more reliable and timely delivery of
 * frames to the peer.
 */
static void
vsw_read_pri_eth_types(vsw_t *vswp, md_t *mdp, mde_cookie_t node)
{
	int		rv;
	uint16_t	*types;
	uint64_t	*data;
	int		size;
	int		i;
	size_t		mblk_sz;

	rv = md_get_prop_data(mdp, node, pri_types_propname,
	    (uint8_t **)&data, &size);
	if (rv != 0) {
		/*
		 * Property may not exist if we are running pre-ldoms1.1 f/w.
		 * Check if 'vsw_pri_eth_type' has been set in that case.
		 */
		if (vsw_pri_eth_type != 0) {
			size = sizeof (vsw_pri_eth_type);
			data = &vsw_pri_eth_type;
		} else {
			D3(vswp, "%s: prop(%s) not found", __func__,
			    pri_types_propname);
			size = 0;
		}
	}

	if (size == 0) {
		vswp->pri_num_types = 0;
		return;
	}

	/*
	 * we have some priority-ether-types defined;
	 * allocate a table of these types and also
	 * allocate a pool of mblks to transmit these
	 * priority packets.
	 */
	size /= sizeof (uint64_t);
	vswp->pri_num_types = size;
	vswp->pri_types = kmem_zalloc(size * sizeof (uint16_t), KM_SLEEP);
	for (i = 0, types = vswp->pri_types; i < size; i++) {
		types[i] = data[i] & 0xFFFF;
	}
	mblk_sz = (VIO_PKT_DATA_HDRSIZE + ETHERMAX + 7) & ~7;
	(void) vio_create_mblks(vsw_pri_tx_nmblks, mblk_sz, &vswp->pri_tx_vmp);
}

/*
 * Check to see if the relevant properties in the specified node have
 * changed, and if so take the appropriate action.
 *
 * If any of the properties are missing or invalid we don't take
 * any action, as this function should only be invoked when modifications
 * have been made to what we assume is a working configuration, which
 * we leave active.
 *
 * Note it is legal for this routine to be invoked even if none of the
 * properties in the port node within the MD have actually changed.
 */
static void
vsw_update_md_prop(vsw_t *vswp, md_t *mdp, mde_cookie_t node)
{
	char		physname[LIFNAMSIZ];
	char		drv[LIFNAMSIZ];
	uint_t		ddi_instance;
	uint8_t		new_smode[NUM_SMODES];
	int		i, smode_num = 0;
	uint64_t 	macaddr = 0;
	enum		{MD_init = 0x1,
				MD_physname = 0x2,
				MD_macaddr = 0x4,
				MD_smode = 0x8,
				MD_vlans = 0x10} updated;
	int		rv;
	uint16_t	pvid;
	uint16_t	*vids;
	uint16_t	nvids;

	updated = MD_init;

	D1(vswp, "%s: enter", __func__);

	/*
	 * Check if name of physical device in MD has changed.
	 */
	if (vsw_get_md_physname(vswp, mdp, node, (char *)&physname) == 0) {
		/*
		 * Do basic sanity check on new device name/instance,
		 * if its non NULL. It is valid for the device name to
		 * have changed from a non NULL to a NULL value, i.e.
		 * the vsw is being changed to 'routed' mode.
		 */
		if ((strlen(physname) != 0) &&
		    (ddi_parse(physname, drv,
		    &ddi_instance) != DDI_SUCCESS)) {
			cmn_err(CE_WARN, "!vsw%d: physical device %s is not"
			    " a valid device name/instance",
			    vswp->instance, physname);
			goto fail_reconf;
		}

		if (strcmp(physname, vswp->physname)) {
			D2(vswp, "%s: device name changed from %s to %s",
			    __func__, vswp->physname, physname);

			updated |= MD_physname;
		} else {
			D2(vswp, "%s: device name unchanged at %s",
			    __func__, vswp->physname);
		}
	} else {
		cmn_err(CE_WARN, "!vsw%d: Unable to read name of physical "
		    "device from updated MD.", vswp->instance);
		goto fail_reconf;
	}

	/*
	 * Check if MAC address has changed.
	 */
	if (md_get_prop_val(mdp, node, macaddr_propname, &macaddr) != 0) {
		cmn_err(CE_WARN, "!vsw%d: Unable to get MAC address from MD",
		    vswp->instance);
		goto fail_reconf;
	} else {
		uint64_t maddr = macaddr;
		READ_ENTER(&vswp->if_lockrw);
		for (i = ETHERADDRL - 1; i >= 0; i--) {
			if (vswp->if_addr.ether_addr_octet[i]
			    != (macaddr & 0xFF)) {
				D2(vswp, "%s: octet[%d] 0x%x != 0x%x",
				    __func__, i,
				    vswp->if_addr.ether_addr_octet[i],
				    (macaddr & 0xFF));
				updated |= MD_macaddr;
				macaddr = maddr;
				break;
			}
			macaddr >>= 8;
		}
		RW_EXIT(&vswp->if_lockrw);
		if (updated & MD_macaddr) {
			vsw_save_lmacaddr(vswp, macaddr);
		}
	}

	/*
	 * Check if switching modes have changed.
	 */
	if (vsw_get_md_smodes(vswp, mdp, node,
	    new_smode, &smode_num)) {
		cmn_err(CE_WARN, "!vsw%d: Unable to read %s property from MD",
		    vswp->instance, smode_propname);
		goto fail_reconf;
	} else {
		ASSERT(smode_num != 0);
		if (smode_num != vswp->smode_num) {
			D2(vswp, "%s: number of modes changed from %d to %d",
			    __func__, vswp->smode_num, smode_num);
		}

		for (i = 0; i < smode_num; i++) {
			if (new_smode[i] != vswp->smode[i]) {
				D2(vswp, "%s: mode changed from %d to %d",
				    __func__, vswp->smode[i], new_smode[i]);
				updated |= MD_smode;
				break;
			}
		}
	}

	/* Read the vlan ids */
	vsw_vlan_read_ids(vswp, VSW_LOCALDEV, mdp, node, &pvid, &vids,
	    &nvids, NULL);

	/* Determine if there are any vlan id updates */
	if ((pvid != vswp->pvid) ||		/* pvid changed? */
	    (nvids != vswp->nvids) ||		/* # of vids changed? */
	    ((nvids != 0) && (vswp->nvids != 0) &&	/* vids changed? */
	    bcmp(vids, vswp->vids, sizeof (uint16_t) * nvids))) {
		updated |= MD_vlans;
	}

	/*
	 * Now make any changes which are needed...
	 */

	if (updated & (MD_physname | MD_smode)) {

		/*
		 * Stop any pending timeout to setup switching mode.
		 */
		vsw_stop_switching_timeout(vswp);

		/* Cleanup HybridIO */
		vsw_hio_cleanup(vswp);

		/*
		 * Remove unicst, mcst addrs of vsw interface
		 * and ports from the physdev.
		 */
		vsw_unset_addrs(vswp);

		/*
		 * Stop, detach and close the old device..
		 */
		WRITE_ENTER(&vswp->mac_rwlock);

		vsw_mac_detach(vswp);
		vsw_mac_close(vswp);

		RW_EXIT(&vswp->mac_rwlock);

		/*
		 * Update phys name.
		 */
		if (updated & MD_physname) {
			cmn_err(CE_NOTE, "!vsw%d: changing from %s to %s",
			    vswp->instance, vswp->physname, physname);
			(void) strncpy(vswp->physname,
			    physname, strlen(physname) + 1);
		}

		/*
		 * Update array with the new switch mode values.
		 */
		if (updated & MD_smode) {
			for (i = 0; i < smode_num; i++)
				vswp->smode[i] = new_smode[i];

			vswp->smode_num = smode_num;
			vswp->smode_idx = 0;
		}

		/*
		 * ..and attach, start the new device.
		 */
		rv = vsw_setup_switching(vswp);
		if (rv == EAGAIN) {
			/*
			 * Unable to setup switching mode.
			 * As the error is EAGAIN, schedule a timeout to retry
			 * and return. Programming addresses of ports and
			 * vsw interface will be done when the timeout handler
			 * completes successfully.
			 */
			mutex_enter(&vswp->swtmout_lock);

			vswp->swtmout_enabled = B_TRUE;
			vswp->swtmout_id =
			    timeout(vsw_setup_switching_timeout, vswp,
			    (vsw_setup_switching_delay *
			    drv_usectohz(MICROSEC)));

			mutex_exit(&vswp->swtmout_lock);

			return;

		} else if (rv) {
			goto fail_update;
		}

		/*
		 * program unicst, mcst addrs of vsw interface
		 * and ports in the physdev.
		 */
		vsw_set_addrs(vswp);

		/* Start HIO for ports that have already connected */
		vsw_hio_start_ports(vswp);

	} else if (updated & MD_macaddr) {
		/*
		 * We enter here if only MD_macaddr is exclusively updated.
		 * If MD_physname and/or MD_smode are also updated, then
		 * as part of that, we would have implicitly processed
		 * MD_macaddr update (above).
		 */
		cmn_err(CE_NOTE, "!vsw%d: changing mac address to 0x%lx",
		    vswp->instance, macaddr);

		READ_ENTER(&vswp->if_lockrw);
		if (vswp->if_state & VSW_IF_UP) {

			mutex_enter(&vswp->hw_lock);
			/*
			 * Remove old mac address of vsw interface
			 * from the physdev
			 */
			(void) vsw_unset_hw(vswp, NULL, VSW_LOCALDEV);
			/*
			 * Program new mac address of vsw interface
			 * in the physdev
			 */
			rv = vsw_set_hw(vswp, NULL, VSW_LOCALDEV);
			mutex_exit(&vswp->hw_lock);
			if (rv != 0) {
				cmn_err(CE_NOTE,
				    "!vsw%d: failed to program interface "
				    "unicast address\n", vswp->instance);
			}
			/*
			 * Notify the MAC layer of the changed address.
			 */
			mac_unicst_update(vswp->if_mh,
			    (uint8_t *)&vswp->if_addr);

		}
		RW_EXIT(&vswp->if_lockrw);

	}

	if (updated & MD_vlans) {
		/* Remove existing vlan ids from the hash table. */
		vsw_vlan_remove_ids(vswp, VSW_LOCALDEV);

		/* save the new vlan ids */
		vswp->pvid = pvid;
		if (vswp->nvids != 0) {
			kmem_free(vswp->vids, sizeof (uint16_t) * vswp->nvids);
			vswp->nvids = 0;
		}
		if (nvids != 0) {
			vswp->nvids = nvids;
			vswp->vids = vids;
		}

		/* add these new vlan ids into hash table */
		vsw_vlan_add_ids(vswp, VSW_LOCALDEV);
	} else {
		if (nvids != 0) {
			kmem_free(vids, sizeof (uint16_t) * nvids);
		}
	}

	return;

fail_reconf:
	cmn_err(CE_WARN, "!vsw%d: configuration unchanged", vswp->instance);
	return;

fail_update:
	cmn_err(CE_WARN, "!vsw%d: re-configuration failed",
	    vswp->instance);
}

/*
 * Read the port's md properties.
 */
static int
vsw_port_read_props(vsw_port_t *portp, vsw_t *vswp,
	md_t *mdp, mde_cookie_t *node)
{
	uint64_t		ldc_id;
	uint8_t			*addrp;
	int			i, addrsz;
	int			num_nodes = 0, nchan = 0;
	int			listsz = 0;
	mde_cookie_t		*listp = NULL;
	struct ether_addr	ea;
	uint64_t		macaddr;
	uint64_t		inst = 0;
	uint64_t		val;

	if (md_get_prop_val(mdp, *node, id_propname, &inst)) {
		DWARN(vswp, "%s: prop(%s) not found", __func__,
		    id_propname);
		return (1);
	}

	/*
	 * Find the channel endpoint node(s) (which should be under this
	 * port node) which contain the channel id(s).
	 */
	if ((num_nodes = md_node_count(mdp)) <= 0) {
		DERR(vswp, "%s: invalid number of nodes found (%d)",
		    __func__, num_nodes);
		return (1);
	}

	D2(vswp, "%s: %d nodes found", __func__, num_nodes);

	/* allocate enough space for node list */
	listsz = num_nodes * sizeof (mde_cookie_t);
	listp = kmem_zalloc(listsz, KM_SLEEP);

	nchan = md_scan_dag(mdp, *node, md_find_name(mdp, chan_propname),
	    md_find_name(mdp, "fwd"), listp);

	if (nchan <= 0) {
		DWARN(vswp, "%s: no %s nodes found", __func__, chan_propname);
		kmem_free(listp, listsz);
		return (1);
	}

	D2(vswp, "%s: %d %s nodes found", __func__, nchan, chan_propname);

	/* use property from first node found */
	if (md_get_prop_val(mdp, listp[0], id_propname, &ldc_id)) {
		DWARN(vswp, "%s: prop(%s) not found\n", __func__,
		    id_propname);
		kmem_free(listp, listsz);
		return (1);
	}

	/* don't need list any more */
	kmem_free(listp, listsz);

	D2(vswp, "%s: ldc_id 0x%llx", __func__, ldc_id);

	/* read mac-address property */
	if (md_get_prop_data(mdp, *node, remaddr_propname,
	    &addrp, &addrsz)) {
		DWARN(vswp, "%s: prop(%s) not found",
		    __func__, remaddr_propname);
		return (1);
	}

	if (addrsz < ETHERADDRL) {
		DWARN(vswp, "%s: invalid address size", __func__);
		return (1);
	}

	macaddr = *((uint64_t *)addrp);
	D2(vswp, "%s: remote mac address 0x%llx", __func__, macaddr);

	for (i = ETHERADDRL - 1; i >= 0; i--) {
		ea.ether_addr_octet[i] = macaddr & 0xFF;
		macaddr >>= 8;
	}

	/* now update all properties into the port */
	portp->p_vswp = vswp;
	portp->p_instance = inst;
	portp->addr_set = VSW_ADDR_UNSET;
	ether_copy(&ea, &portp->p_macaddr);
	if (nchan > VSW_PORT_MAX_LDCS) {
		D2(vswp, "%s: using first of %d ldc ids",
		    __func__, nchan);
		nchan = VSW_PORT_MAX_LDCS;
	}
	portp->num_ldcs = nchan;
	portp->ldc_ids =
	    kmem_zalloc(sizeof (uint64_t) * nchan, KM_SLEEP);
	bcopy(&ldc_id, (portp->ldc_ids), sizeof (uint64_t) * nchan);

	/* read vlan id properties of this port node */
	vsw_vlan_read_ids(portp, VSW_VNETPORT, mdp, *node, &portp->pvid,
	    &portp->vids, &portp->nvids, NULL);

	/* Check if hybrid property is present */
	if (md_get_prop_val(mdp, *node, hybrid_propname, &val) == 0) {
		D1(vswp, "%s: prop(%s) found\n", __func__, hybrid_propname);
		portp->p_hio_enabled = B_TRUE;
	} else {
		portp->p_hio_enabled = B_FALSE;
	}
	/*
	 * Port hio capability determined after version
	 * negotiation, i.e., when we know the peer is HybridIO capable.
	 */
	portp->p_hio_capable = B_FALSE;
	return (0);
}

/*
 * Add a new port to the system.
 *
 * Returns 0 on success, 1 on failure.
 */
int
vsw_port_add(vsw_t *vswp, md_t *mdp, mde_cookie_t *node)
{
	vsw_port_t	*portp;
	int		rv;

	portp = kmem_zalloc(sizeof (vsw_port_t), KM_SLEEP);

	rv = vsw_port_read_props(portp, vswp, mdp, node);
	if (rv != 0) {
		kmem_free(portp, sizeof (*portp));
		return (1);
	}

	rv = vsw_port_attach(portp);
	if (rv != 0) {
		DERR(vswp, "%s: failed to attach port", __func__);
		return (1);
	}

	return (0);
}

static int
vsw_port_update(vsw_t *vswp, md_t *curr_mdp, mde_cookie_t curr_mdex,
	md_t *prev_mdp, mde_cookie_t prev_mdex)
{
	uint64_t	cport_num;
	uint64_t	pport_num;
	vsw_port_list_t	*plistp;
	vsw_port_t	*portp;
	boolean_t	updated_vlans = B_FALSE;
	uint16_t	pvid;
	uint16_t	*vids;
	uint16_t	nvids;
	uint64_t	val;
	boolean_t	hio_enabled = B_FALSE;

	/*
	 * For now, we get port updates only if vlan ids changed.
	 * We read the port num and do some sanity check.
	 */
	if (md_get_prop_val(curr_mdp, curr_mdex, id_propname, &cport_num)) {
		return (1);
	}

	if (md_get_prop_val(prev_mdp, prev_mdex, id_propname, &pport_num)) {
		return (1);
	}
	if (cport_num != pport_num)
		return (1);

	plistp = &(vswp->plist);

	READ_ENTER(&plistp->lockrw);

	portp = vsw_lookup_port(vswp, cport_num);
	if (portp == NULL) {
		RW_EXIT(&plistp->lockrw);
		return (1);
	}

	/* Read the vlan ids */
	vsw_vlan_read_ids(portp, VSW_VNETPORT, curr_mdp, curr_mdex, &pvid,
	    &vids, &nvids, NULL);

	/* Determine if there are any vlan id updates */
	if ((pvid != portp->pvid) ||		/* pvid changed? */
	    (nvids != portp->nvids) ||		/* # of vids changed? */
	    ((nvids != 0) && (portp->nvids != 0) &&	/* vids changed? */
	    bcmp(vids, portp->vids, sizeof (uint16_t) * nvids))) {
		updated_vlans = B_TRUE;
	}

	if (updated_vlans == B_TRUE) {

		/* Remove existing vlan ids from the hash table. */
		vsw_vlan_remove_ids(portp, VSW_VNETPORT);

		/* save the new vlan ids */
		portp->pvid = pvid;
		if (portp->nvids != 0) {
			kmem_free(portp->vids,
			    sizeof (uint16_t) * portp->nvids);
			portp->nvids = 0;
		}
		if (nvids != 0) {
			portp->vids = kmem_zalloc(sizeof (uint16_t) *
			    nvids, KM_SLEEP);
			bcopy(vids, portp->vids, sizeof (uint16_t) * nvids);
			portp->nvids = nvids;
			kmem_free(vids, sizeof (uint16_t) * nvids);
		}

		/* add these new vlan ids into hash table */
		vsw_vlan_add_ids(portp, VSW_VNETPORT);

		/* reset the port if it is vlan unaware (ver < 1.3) */
		vsw_vlan_unaware_port_reset(portp);
	}

	/* Check if hybrid property is present */
	if (md_get_prop_val(curr_mdp, curr_mdex, hybrid_propname, &val) == 0) {
		D1(vswp, "%s: prop(%s) found\n", __func__, hybrid_propname);
		hio_enabled = B_TRUE;
	}

	if (portp->p_hio_enabled != hio_enabled) {
		vsw_hio_port_update(portp, hio_enabled);
	}

	RW_EXIT(&plistp->lockrw);

	return (0);
}

/*
 * vsw_mac_rx -- A common function to send packets to the interface.
 * By default this function check if the interface is UP or not, the
 * rest of the behaviour depends on the flags as below:
 *
 *	VSW_MACRX_PROMISC -- Check if the promisc mode set or not.
 *	VSW_MACRX_COPYMSG -- Make a copy of the message(s).
 *	VSW_MACRX_FREEMSG -- Free if the messages cannot be sent up the stack.
 */
void
vsw_mac_rx(vsw_t *vswp, mac_resource_handle_t mrh,
    mblk_t *mp, vsw_macrx_flags_t flags)
{
	mblk_t		*mpt;

	D1(vswp, "%s:enter\n", __func__);
	READ_ENTER(&vswp->if_lockrw);
	/* Check if the interface is up */
	if (!(vswp->if_state & VSW_IF_UP)) {
		RW_EXIT(&vswp->if_lockrw);
		/* Free messages only if FREEMSG flag specified */
		if (flags & VSW_MACRX_FREEMSG) {
			freemsgchain(mp);
		}
		D1(vswp, "%s:exit\n", __func__);
		return;
	}
	/*
	 * If PROMISC flag is passed, then check if
	 * the interface is in the PROMISC mode.
	 * If not, drop the messages.
	 */
	if (flags & VSW_MACRX_PROMISC) {
		if (!(vswp->if_state & VSW_IF_PROMISC)) {
			RW_EXIT(&vswp->if_lockrw);
			/* Free messages only if FREEMSG flag specified */
			if (flags & VSW_MACRX_FREEMSG) {
				freemsgchain(mp);
			}
			D1(vswp, "%s:exit\n", __func__);
			return;
		}
	}
	RW_EXIT(&vswp->if_lockrw);
	/*
	 * If COPYMSG flag is passed, then make a copy
	 * of the message chain and send up the copy.
	 */
	if (flags & VSW_MACRX_COPYMSG) {
		mp = copymsgchain(mp);
		if (mp == NULL) {
			D1(vswp, "%s:exit\n", __func__);
			return;
		}
	}

	D2(vswp, "%s: sending up stack", __func__);

	mpt = NULL;
	(void) vsw_vlan_frame_untag(vswp, VSW_LOCALDEV, &mp, &mpt);
	if (mp != NULL) {
		mac_rx(vswp->if_mh, mrh, mp);
	}
	D1(vswp, "%s:exit\n", __func__);
}

/* copy mac address of vsw into soft state structure */
static void
vsw_save_lmacaddr(vsw_t *vswp, uint64_t macaddr)
{
	int	i;

	WRITE_ENTER(&vswp->if_lockrw);
	for (i = ETHERADDRL - 1; i >= 0; i--) {
		vswp->if_addr.ether_addr_octet[i] = macaddr & 0xFF;
		macaddr >>= 8;
	}
	RW_EXIT(&vswp->if_lockrw);
}
