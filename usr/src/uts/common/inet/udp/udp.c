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
/* Copyright (c) 1990 Mentat Inc. */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

const char udp_version[] = "%Z%%M%	%I%	%E% SMI";

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/dlpi.h>
#include <sys/pattr.h>
#include <sys/stropts.h>
#include <sys/strlog.h>
#include <sys/strsun.h>
#include <sys/time.h>
#define	_SUN_TPI_VERSION 2
#include <sys/tihdr.h>
#include <sys/timod.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/strsubr.h>
#include <sys/suntpi.h>
#include <sys/xti_inet.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/policy.h>
#include <sys/ucred.h>
#include <sys/zone.h>

#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/vtrace.h>
#include <sys/sdt.h>
#include <sys/debug.h>
#include <sys/isa_defs.h>
#include <sys/random.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet/udp.h>
#include <net/if.h>
#include <net/route.h>

#include <inet/common.h>
#include <inet/ip.h>
#include <inet/ip_impl.h>
#include <inet/ip6.h>
#include <inet/ip_ire.h>
#include <inet/ip_if.h>
#include <inet/ip_multi.h>
#include <inet/ip_ndp.h>
#include <inet/mi.h>
#include <inet/mib2.h>
#include <inet/nd.h>
#include <inet/optcom.h>
#include <inet/snmpcom.h>
#include <inet/kstatcom.h>
#include <inet/udp_impl.h>
#include <inet/ipclassifier.h>
#include <inet/ipsec_impl.h>
#include <inet/ipp_common.h>

/*
 * The ipsec_info.h header file is here since it has the definition for the
 * M_CTL message types used by IP to convey information to the ULP. The
 * ipsec_info.h needs the pfkeyv2.h, hence the latter's presence.
 */
#include <net/pfkeyv2.h>
#include <inet/ipsec_info.h>

#include <sys/tsol/label.h>
#include <sys/tsol/tnet.h>
#include <rpc/pmap_prot.h>

/*
 * Synchronization notes:
 *
 * UDP is MT and uses the usual kernel synchronization primitives. There are 2
 * locks, the fanout lock (uf_lock) and the udp endpoint lock udp_rwlock.
 * We also use conn_lock when updating things that affect the IP classifier
 * lookup.
 * The lock order is udp_rwlock -> uf_lock and is udp_rwlock -> conn_lock.
 *
 * The fanout lock uf_lock:
 * When a UDP endpoint is bound to a local port, it is inserted into
 * a bind hash list.  The list consists of an array of udp_fanout_t buckets.
 * The size of the array is controlled by the udp_bind_fanout_size variable.
 * This variable can be changed in /etc/system if the default value is
 * not large enough.  Each bind hash bucket is protected by a per bucket
 * lock.  It protects the udp_bind_hash and udp_ptpbhn fields in the udp_t
 * structure and a few other fields in the udp_t. A UDP endpoint is removed
 * from the bind hash list only when it is being unbound or being closed.
 * The per bucket lock also protects a UDP endpoint's state changes.
 *
 * The udp_rwlock:
 * This protects most of the other fields in the udp_t. The exact list of
 * fields which are protected by each of the above locks is documented in
 * the udp_t structure definition.
 *
 * Plumbing notes:
 * UDP is always a device driver. For compatibility with mibopen() code
 * it is possible to I_PUSH "udp", but that results in pushing a passthrough
 * dummy module.
 *
 * The above implies that we don't support any intermediate module to
 * reside in between /dev/ip and udp -- in fact, we never supported such
 * scenario in the past as the inter-layer communication semantics have
 * always been private.
 */

/* For /etc/system control */
uint_t udp_bind_fanout_size = UDP_BIND_FANOUT_SIZE;

#define	NDD_TOO_QUICK_MSG \
	"ndd get info rate too high for non-privileged users, try again " \
	"later.\n"
#define	NDD_OUT_OF_BUF_MSG	"<< Out of buffer >>\n"

/* Option processing attrs */
typedef struct udpattrs_s {
	union {
		ip6_pkt_t	*udpattr_ipp6;	/* For V6 */
		ip4_pkt_t 	*udpattr_ipp4;	/* For V4 */
	} udpattr_ippu;
#define	udpattr_ipp6 udpattr_ippu.udpattr_ipp6
#define	udpattr_ipp4 udpattr_ippu.udpattr_ipp4
	mblk_t		*udpattr_mb;
	boolean_t	udpattr_credset;
} udpattrs_t;

static void	udp_addr_req(queue_t *q, mblk_t *mp);
static void	udp_bind(queue_t *q, mblk_t *mp);
static void	udp_bind_hash_insert(udp_fanout_t *uf, udp_t *udp);
static void	udp_bind_hash_remove(udp_t *udp, boolean_t caller_holds_lock);
static void	udp_bind_result(conn_t *, mblk_t *);
static void	udp_bind_ack(conn_t *, mblk_t *mp);
static void	udp_bind_error(conn_t *, mblk_t *mp);
static int	udp_build_hdrs(udp_t *udp);
static void	udp_capability_req(queue_t *q, mblk_t *mp);
static int	udp_close(queue_t *q);
static void	udp_connect(queue_t *q, mblk_t *mp);
static void	udp_disconnect(queue_t *q, mblk_t *mp);
static void	udp_err_ack(queue_t *q, mblk_t *mp, t_scalar_t t_error,
		    int sys_error);
static void	udp_err_ack_prim(queue_t *q, mblk_t *mp, int primitive,
		    t_scalar_t tlierr, int unixerr);
static int	udp_extra_priv_ports_get(queue_t *q, mblk_t *mp, caddr_t cp,
		    cred_t *cr);
static int	udp_extra_priv_ports_add(queue_t *q, mblk_t *mp,
		    char *value, caddr_t cp, cred_t *cr);
static int	udp_extra_priv_ports_del(queue_t *q, mblk_t *mp,
		    char *value, caddr_t cp, cred_t *cr);
static void	udp_icmp_error(queue_t *q, mblk_t *mp);
static void	udp_icmp_error_ipv6(queue_t *q, mblk_t *mp);
static void	udp_info_req(queue_t *q, mblk_t *mp);
static void	udp_input(void *, mblk_t *, void *);
static mblk_t	*udp_ip_bind_mp(udp_t *udp, t_scalar_t bind_prim,
		    t_scalar_t addr_length);
static void	udp_lrput(queue_t *, mblk_t *);
static void	udp_lwput(queue_t *, mblk_t *);
static int	udp_open(queue_t *q, dev_t *devp, int flag, int sflag,
		    cred_t *credp, boolean_t isv6);
static int	udp_openv4(queue_t *q, dev_t *devp, int flag, int sflag,
		    cred_t *credp);
static int	udp_openv6(queue_t *q, dev_t *devp, int flag, int sflag,
		    cred_t *credp);
static  int	udp_unitdata_opt_process(queue_t *q, mblk_t *mp,
		    int *errorp, udpattrs_t *udpattrs);
static boolean_t udp_opt_allow_udr_set(t_scalar_t level, t_scalar_t name);
static int	udp_param_get(queue_t *q, mblk_t *mp, caddr_t cp, cred_t *cr);
static boolean_t udp_param_register(IDP *ndp, udpparam_t *udppa, int cnt);
static int	udp_param_set(queue_t *q, mblk_t *mp, char *value, caddr_t cp,
		    cred_t *cr);
static void	udp_report_item(mblk_t *mp, udp_t *udp);
static int	udp_rinfop(queue_t *q, infod_t *dp);
static int	udp_rrw(queue_t *q, struiod_t *dp);
static int	udp_status_report(queue_t *q, mblk_t *mp, caddr_t cp,
		    cred_t *cr);
static void	udp_send_data(udp_t *, queue_t *, mblk_t *, ipha_t *);
static void	udp_ud_err(queue_t *q, mblk_t *mp, uchar_t *destaddr,
		    t_scalar_t destlen, t_scalar_t err);
static void	udp_unbind(queue_t *q, mblk_t *mp);
static in_port_t udp_update_next_port(udp_t *udp, in_port_t port,
    boolean_t random);
static mblk_t	*udp_output_v4(conn_t *, mblk_t *, ipaddr_t, uint16_t, uint_t,
		    int *, boolean_t);
static mblk_t	*udp_output_v6(conn_t *connp, mblk_t *mp, sin6_t *sin6,
		    int *error);
static void	udp_wput_other(queue_t *q, mblk_t *mp);
static void	udp_wput_iocdata(queue_t *q, mblk_t *mp);
static size_t	udp_set_rcv_hiwat(udp_t *udp, size_t size);

static void	*udp_stack_init(netstackid_t stackid, netstack_t *ns);
static void	udp_stack_fini(netstackid_t stackid, void *arg);

static void	*udp_kstat_init(netstackid_t stackid);
static void	udp_kstat_fini(netstackid_t stackid, kstat_t *ksp);
static void	*udp_kstat2_init(netstackid_t, udp_stat_t *);
static void	udp_kstat2_fini(netstackid_t, kstat_t *);
static int	udp_kstat_update(kstat_t *kp, int rw);

static void	udp_rcv_enqueue(queue_t *q, udp_t *udp, mblk_t *mp,
		    uint_t pkt_len);
static void	udp_rcv_drain(queue_t *q, udp_t *udp, boolean_t closing);
static void	udp_xmit(queue_t *, mblk_t *, ire_t *ire, conn_t *, zoneid_t);

#define	UDP_RECV_HIWATER	(56 * 1024)
#define	UDP_RECV_LOWATER	128
#define	UDP_XMIT_HIWATER	(56 * 1024)
#define	UDP_XMIT_LOWATER	1024

static struct module_info udp_mod_info =  {
	UDP_MOD_ID, UDP_MOD_NAME, 1, INFPSZ, UDP_RECV_HIWATER, UDP_RECV_LOWATER
};

/*
 * Entry points for UDP as a device.
 * We have separate open functions for the /dev/udp and /dev/udp6 devices.
 */
static struct qinit udp_rinitv4 = {
	NULL, NULL, udp_openv4, udp_close, NULL,
	&udp_mod_info, NULL, udp_rrw, udp_rinfop, STRUIOT_STANDARD
};

static struct qinit udp_rinitv6 = {
	NULL, NULL, udp_openv6, udp_close, NULL,
	&udp_mod_info, NULL, udp_rrw, udp_rinfop, STRUIOT_STANDARD
};

static struct qinit udp_winit = {
	(pfi_t)udp_wput, (pfi_t)ip_wsrv, NULL, NULL, NULL,
	&udp_mod_info, NULL, NULL, NULL, STRUIOT_NONE
};

/*
 * UDP needs to handle I_LINK and I_PLINK since ifconfig
 * likes to use it as a place to hang the various streams.
 */
static struct qinit udp_lrinit = {
	(pfi_t)udp_lrput, NULL, udp_openv4, udp_close, NULL,
	&udp_mod_info
};

static struct qinit udp_lwinit = {
	(pfi_t)udp_lwput, NULL, udp_openv4, udp_close, NULL,
	&udp_mod_info
};

/* For AF_INET aka /dev/udp */
struct streamtab udpinfov4 = {
	&udp_rinitv4, &udp_winit, &udp_lrinit, &udp_lwinit
};

/* For AF_INET6 aka /dev/udp6 */
struct streamtab udpinfov6 = {
	&udp_rinitv6, &udp_winit, &udp_lrinit, &udp_lwinit
};

static	sin_t	sin_null;	/* Zero address for quick clears */
static	sin6_t	sin6_null;	/* Zero address for quick clears */

#define	UDP_MAXPACKET_IPV4 (IP_MAXPACKET - UDPH_SIZE - IP_SIMPLE_HDR_LENGTH)

/* Default structure copied into T_INFO_ACK messages */
static struct T_info_ack udp_g_t_info_ack_ipv4 = {
	T_INFO_ACK,
	UDP_MAXPACKET_IPV4,	/* TSDU_size. Excl. headers */
	T_INVALID,	/* ETSU_size.  udp does not support expedited data. */
	T_INVALID,	/* CDATA_size. udp does not support connect data. */
	T_INVALID,	/* DDATA_size. udp does not support disconnect data. */
	sizeof (sin_t),	/* ADDR_size. */
	0,		/* OPT_size - not initialized here */
	UDP_MAXPACKET_IPV4,	/* TIDU_size.  Excl. headers */
	T_CLTS,		/* SERV_type.  udp supports connection-less. */
	TS_UNBND,	/* CURRENT_state.  This is set from udp_state. */
	(XPG4_1|SENDZERO) /* PROVIDER_flag */
};

#define	UDP_MAXPACKET_IPV6 (IP_MAXPACKET - UDPH_SIZE - IPV6_HDR_LEN)

static	struct T_info_ack udp_g_t_info_ack_ipv6 = {
	T_INFO_ACK,
	UDP_MAXPACKET_IPV6,	/* TSDU_size.  Excl. headers */
	T_INVALID,	/* ETSU_size.  udp does not support expedited data. */
	T_INVALID,	/* CDATA_size. udp does not support connect data. */
	T_INVALID,	/* DDATA_size. udp does not support disconnect data. */
	sizeof (sin6_t), /* ADDR_size. */
	0,		/* OPT_size - not initialized here */
	UDP_MAXPACKET_IPV6,	/* TIDU_size. Excl. headers */
	T_CLTS,		/* SERV_type.  udp supports connection-less. */
	TS_UNBND,	/* CURRENT_state.  This is set from udp_state. */
	(XPG4_1|SENDZERO) /* PROVIDER_flag */
};

/* largest UDP port number */
#define	UDP_MAX_PORT	65535

/*
 * Table of ND variables supported by udp.  These are loaded into us_nd
 * in udp_open.
 * All of these are alterable, within the min/max values given, at run time.
 */
/* BEGIN CSTYLED */
udpparam_t udp_param_arr[] = {
 /*min		max		value		name */
 { 0L,		256,		32,		"udp_wroff_extra" },
 { 1L,		255,		255,		"udp_ipv4_ttl" },
 { 0,		IPV6_MAX_HOPS,	IPV6_DEFAULT_HOPS, "udp_ipv6_hoplimit"},
 { 1024,	(32 * 1024),	1024,		"udp_smallest_nonpriv_port" },
 { 0,		1,		1,		"udp_do_checksum" },
 { 1024,	UDP_MAX_PORT,	(32 * 1024),	"udp_smallest_anon_port" },
 { 1024,	UDP_MAX_PORT,	UDP_MAX_PORT,	"udp_largest_anon_port" },
 { UDP_XMIT_LOWATER, (1<<30), UDP_XMIT_HIWATER,	"udp_xmit_hiwat"},
 { 0,		     (1<<30), UDP_XMIT_LOWATER, "udp_xmit_lowat"},
 { UDP_RECV_LOWATER, (1<<30), UDP_RECV_HIWATER,	"udp_recv_hiwat"},
 { 65536,	(1<<30),	2*1024*1024,	"udp_max_buf"},
 { 100,		60000,		1000,		"udp_ndd_get_info_interval"},
};
/* END CSTYLED */

/* Setable in /etc/system */
/* If set to 0, pick ephemeral port sequentially; otherwise randomly. */
uint32_t udp_random_anon_port = 1;

/*
 * Hook functions to enable cluster networking.
 * On non-clustered systems these vectors must always be NULL
 */

void (*cl_inet_bind)(uchar_t protocol, sa_family_t addr_family,
    uint8_t *laddrp, in_port_t lport) = NULL;
void (*cl_inet_unbind)(uint8_t protocol, sa_family_t addr_family,
    uint8_t *laddrp, in_port_t lport) = NULL;

typedef union T_primitives *t_primp_t;

/*
 * Return the next anonymous port in the privileged port range for
 * bind checking.
 *
 * Trusted Extension (TX) notes: TX allows administrator to mark or
 * reserve ports as Multilevel ports (MLP). MLP has special function
 * on TX systems. Once a port is made MLP, it's not available as
 * ordinary port. This creates "holes" in the port name space. It
 * may be necessary to skip the "holes" find a suitable anon port.
 */
static in_port_t
udp_get_next_priv_port(udp_t *udp)
{
	static in_port_t next_priv_port = IPPORT_RESERVED - 1;
	in_port_t nextport;
	boolean_t restart = B_FALSE;
	udp_stack_t *us = udp->udp_us;

retry:
	if (next_priv_port < us->us_min_anonpriv_port ||
	    next_priv_port >= IPPORT_RESERVED) {
		next_priv_port = IPPORT_RESERVED - 1;
		if (restart)
			return (0);
		restart = B_TRUE;
	}

	if (is_system_labeled() &&
	    (nextport = tsol_next_port(crgetzone(udp->udp_connp->conn_cred),
	    next_priv_port, IPPROTO_UDP, B_FALSE)) != 0) {
		next_priv_port = nextport;
		goto retry;
	}

	return (next_priv_port--);
}

/* UDP bind hash report triggered via the Named Dispatch mechanism. */
/* ARGSUSED */
static int
udp_bind_hash_report(queue_t *q, mblk_t *mp, caddr_t cp, cred_t *cr)
{
	udp_fanout_t	*udpf;
	int		i;
	zoneid_t	zoneid;
	conn_t		*connp;
	udp_t		*udp;
	udp_stack_t	*us;

	connp = Q_TO_CONN(q);
	udp = connp->conn_udp;
	us = udp->udp_us;

	/* Refer to comments in udp_status_report(). */
	if (cr == NULL || secpolicy_ip_config(cr, B_TRUE) != 0) {
		if (ddi_get_lbolt() - us->us_last_ndd_get_info_time <
		    drv_usectohz(us->us_ndd_get_info_interval * 1000)) {
			(void) mi_mpprintf(mp, NDD_TOO_QUICK_MSG);
			return (0);
		}
	}
	if ((mp->b_cont = allocb(ND_MAX_BUF_LEN, BPRI_HI)) == NULL) {
		/* The following may work even if we cannot get a large buf. */
		(void) mi_mpprintf(mp, NDD_OUT_OF_BUF_MSG);
		return (0);
	}

	(void) mi_mpprintf(mp,
	    "UDP     " MI_COL_HDRPAD_STR
	/*   12345678[89ABCDEF] */
	    " zone lport src addr        dest addr       port  state");
	/*    1234 12345 xxx.xxx.xxx.xxx xxx.xxx.xxx.xxx 12345 UNBOUND */

	zoneid = connp->conn_zoneid;

	for (i = 0; i < us->us_bind_fanout_size; i++) {
		udpf = &us->us_bind_fanout[i];
		mutex_enter(&udpf->uf_lock);

		/* Print the hash index. */
		udp = udpf->uf_udp;
		if (zoneid != GLOBAL_ZONEID) {
			/* skip to first entry in this zone; might be none */
			while (udp != NULL &&
			    udp->udp_connp->conn_zoneid != zoneid)
				udp = udp->udp_bind_hash;
		}
		if (udp != NULL) {
			uint_t print_len, buf_len;

			buf_len = mp->b_cont->b_datap->db_lim -
			    mp->b_cont->b_wptr;
			print_len = snprintf((char *)mp->b_cont->b_wptr,
			    buf_len, "%d\n", i);
			if (print_len < buf_len) {
				mp->b_cont->b_wptr += print_len;
			} else {
				mp->b_cont->b_wptr += buf_len;
			}
			for (; udp != NULL; udp = udp->udp_bind_hash) {
				if (zoneid == GLOBAL_ZONEID ||
				    zoneid == udp->udp_connp->conn_zoneid)
					udp_report_item(mp->b_cont, udp);
			}
		}
		mutex_exit(&udpf->uf_lock);
	}
	us->us_last_ndd_get_info_time = ddi_get_lbolt();
	return (0);
}

/*
 * Hash list removal routine for udp_t structures.
 */
static void
udp_bind_hash_remove(udp_t *udp, boolean_t caller_holds_lock)
{
	udp_t	*udpnext;
	kmutex_t *lockp;
	udp_stack_t *us = udp->udp_us;

	if (udp->udp_ptpbhn == NULL)
		return;

	/*
	 * Extract the lock pointer in case there are concurrent
	 * hash_remove's for this instance.
	 */
	ASSERT(udp->udp_port != 0);
	if (!caller_holds_lock) {
		lockp = &us->us_bind_fanout[UDP_BIND_HASH(udp->udp_port,
		    us->us_bind_fanout_size)].uf_lock;
		ASSERT(lockp != NULL);
		mutex_enter(lockp);
	}
	if (udp->udp_ptpbhn != NULL) {
		udpnext = udp->udp_bind_hash;
		if (udpnext != NULL) {
			udpnext->udp_ptpbhn = udp->udp_ptpbhn;
			udp->udp_bind_hash = NULL;
		}
		*udp->udp_ptpbhn = udpnext;
		udp->udp_ptpbhn = NULL;
	}
	if (!caller_holds_lock) {
		mutex_exit(lockp);
	}
}

static void
udp_bind_hash_insert(udp_fanout_t *uf, udp_t *udp)
{
	udp_t	**udpp;
	udp_t	*udpnext;

	ASSERT(MUTEX_HELD(&uf->uf_lock));
	ASSERT(udp->udp_ptpbhn == NULL);
	udpp = &uf->uf_udp;
	udpnext = udpp[0];
	if (udpnext != NULL) {
		/*
		 * If the new udp bound to the INADDR_ANY address
		 * and the first one in the list is not bound to
		 * INADDR_ANY we skip all entries until we find the
		 * first one bound to INADDR_ANY.
		 * This makes sure that applications binding to a
		 * specific address get preference over those binding to
		 * INADDR_ANY.
		 */
		if (V6_OR_V4_INADDR_ANY(udp->udp_bound_v6src) &&
		    !V6_OR_V4_INADDR_ANY(udpnext->udp_bound_v6src)) {
			while ((udpnext = udpp[0]) != NULL &&
			    !V6_OR_V4_INADDR_ANY(
			    udpnext->udp_bound_v6src)) {
				udpp = &(udpnext->udp_bind_hash);
			}
			if (udpnext != NULL)
				udpnext->udp_ptpbhn = &udp->udp_bind_hash;
		} else {
			udpnext->udp_ptpbhn = &udp->udp_bind_hash;
		}
	}
	udp->udp_bind_hash = udpnext;
	udp->udp_ptpbhn = udpp;
	udpp[0] = udp;
}

/*
 * This routine is called to handle each O_T_BIND_REQ/T_BIND_REQ message
 * passed to udp_wput.
 * It associates a port number and local address with the stream.
 * The O_T_BIND_REQ/T_BIND_REQ is passed downstream to ip with the UDP
 * protocol type (IPPROTO_UDP) placed in the message following the address.
 * A T_BIND_ACK message is passed upstream when ip acknowledges the request.
 * (Called as writer.)
 *
 * Note that UDP over IPv4 and IPv6 sockets can use the same port number
 * without setting SO_REUSEADDR. This is needed so that they
 * can be viewed as two independent transport protocols.
 * However, anonymouns ports are allocated from the same range to avoid
 * duplicating the us->us_next_port_to_try.
 */
static void
udp_bind(queue_t *q, mblk_t *mp)
{
	sin_t		*sin;
	sin6_t		*sin6;
	mblk_t		*mp1;
	in_port_t	port;		/* Host byte order */
	in_port_t	requested_port;	/* Host byte order */
	struct T_bind_req *tbr;
	int		count;
	in6_addr_t	v6src;
	boolean_t	bind_to_req_port_only;
	int		loopmax;
	udp_fanout_t	*udpf;
	in_port_t	lport;		/* Network byte order */
	zoneid_t	zoneid;
	conn_t		*connp;
	udp_t		*udp;
	boolean_t	is_inaddr_any;
	mlp_type_t	addrtype, mlptype;
	udp_stack_t	*us;

	connp = Q_TO_CONN(q);
	udp = connp->conn_udp;
	us = udp->udp_us;
	if ((mp->b_wptr - mp->b_rptr) < sizeof (*tbr)) {
		(void) mi_strlog(q, 1, SL_ERROR|SL_TRACE,
		    "udp_bind: bad req, len %u",
		    (uint_t)(mp->b_wptr - mp->b_rptr));
		udp_err_ack(q, mp, TPROTO, 0);
		return;
	}
	if (udp->udp_state != TS_UNBND) {
		(void) mi_strlog(q, 1, SL_ERROR|SL_TRACE,
		    "udp_bind: bad state, %u", udp->udp_state);
		udp_err_ack(q, mp, TOUTSTATE, 0);
		return;
	}
	/*
	 * Reallocate the message to make sure we have enough room for an
	 * address and the protocol type.
	 */
	mp1 = reallocb(mp, sizeof (struct T_bind_ack) + sizeof (sin6_t) + 1, 1);
	if (!mp1) {
		udp_err_ack(q, mp, TSYSERR, ENOMEM);
		return;
	}

	mp = mp1;
	tbr = (struct T_bind_req *)mp->b_rptr;
	switch (tbr->ADDR_length) {
	case 0:			/* Request for a generic port */
		tbr->ADDR_offset = sizeof (struct T_bind_req);
		if (udp->udp_family == AF_INET) {
			tbr->ADDR_length = sizeof (sin_t);
			sin = (sin_t *)&tbr[1];
			*sin = sin_null;
			sin->sin_family = AF_INET;
			mp->b_wptr = (uchar_t *)&sin[1];
		} else {
			ASSERT(udp->udp_family == AF_INET6);
			tbr->ADDR_length = sizeof (sin6_t);
			sin6 = (sin6_t *)&tbr[1];
			*sin6 = sin6_null;
			sin6->sin6_family = AF_INET6;
			mp->b_wptr = (uchar_t *)&sin6[1];
		}
		port = 0;
		break;

	case sizeof (sin_t):	/* Complete IPv4 address */
		sin = (sin_t *)mi_offset_param(mp, tbr->ADDR_offset,
		    sizeof (sin_t));
		if (sin == NULL || !OK_32PTR((char *)sin)) {
			udp_err_ack(q, mp, TSYSERR, EINVAL);
			return;
		}
		if (udp->udp_family != AF_INET ||
		    sin->sin_family != AF_INET) {
			udp_err_ack(q, mp, TSYSERR, EAFNOSUPPORT);
			return;
		}
		port = ntohs(sin->sin_port);
		break;

	case sizeof (sin6_t):	/* complete IPv6 address */
		sin6 = (sin6_t *)mi_offset_param(mp, tbr->ADDR_offset,
		    sizeof (sin6_t));
		if (sin6 == NULL || !OK_32PTR((char *)sin6)) {
			udp_err_ack(q, mp, TSYSERR, EINVAL);
			return;
		}
		if (udp->udp_family != AF_INET6 ||
		    sin6->sin6_family != AF_INET6) {
			udp_err_ack(q, mp, TSYSERR, EAFNOSUPPORT);
			return;
		}
		port = ntohs(sin6->sin6_port);
		break;

	default:		/* Invalid request */
		(void) mi_strlog(q, 1, SL_ERROR|SL_TRACE,
		    "udp_bind: bad ADDR_length length %u", tbr->ADDR_length);
		udp_err_ack(q, mp, TBADADDR, 0);
		return;
	}

	requested_port = port;

	if (requested_port == 0 || tbr->PRIM_type == O_T_BIND_REQ)
		bind_to_req_port_only = B_FALSE;
	else			/* T_BIND_REQ and requested_port != 0 */
		bind_to_req_port_only = B_TRUE;

	if (requested_port == 0) {
		/*
		 * If the application passed in zero for the port number, it
		 * doesn't care which port number we bind to. Get one in the
		 * valid range.
		 */
		if (udp->udp_anon_priv_bind) {
			port = udp_get_next_priv_port(udp);
		} else {
			port = udp_update_next_port(udp,
			    us->us_next_port_to_try, B_TRUE);
		}
	} else {
		/*
		 * If the port is in the well-known privileged range,
		 * make sure the caller was privileged.
		 */
		int i;
		boolean_t priv = B_FALSE;

		if (port < us->us_smallest_nonpriv_port) {
			priv = B_TRUE;
		} else {
			for (i = 0; i < us->us_num_epriv_ports; i++) {
				if (port == us->us_epriv_ports[i]) {
					priv = B_TRUE;
					break;
				}
			}
		}

		if (priv) {
			cred_t *cr = DB_CREDDEF(mp, connp->conn_cred);

			if (secpolicy_net_privaddr(cr, port,
			    IPPROTO_UDP) != 0) {
				udp_err_ack(q, mp, TACCES, 0);
				return;
			}
		}
	}

	if (port == 0) {
		udp_err_ack(q, mp, TNOADDR, 0);
		return;
	}

	/*
	 * The state must be TS_UNBND. TPI mandates that users must send
	 * TPI primitives only 1 at a time and wait for the response before
	 * sending the next primitive.
	 */
	rw_enter(&udp->udp_rwlock, RW_WRITER);
	if (udp->udp_state != TS_UNBND || udp->udp_pending_op != -1) {
		rw_exit(&udp->udp_rwlock);
		(void) mi_strlog(q, 1, SL_ERROR|SL_TRACE,
		    "udp_bind: bad state, %u", udp->udp_state);
		udp_err_ack(q, mp, TOUTSTATE, 0);
		return;
	}
	udp->udp_pending_op = tbr->PRIM_type;
	/*
	 * Copy the source address into our udp structure. This address
	 * may still be zero; if so, IP will fill in the correct address
	 * each time an outbound packet is passed to it. Since the udp is
	 * not yet in the bind hash list, we don't grab the uf_lock to
	 * change udp_ipversion
	 */
	if (udp->udp_family == AF_INET) {
		ASSERT(sin != NULL);
		ASSERT(udp->udp_ipversion == IPV4_VERSION);
		udp->udp_max_hdr_len = IP_SIMPLE_HDR_LENGTH + UDPH_SIZE +
		    udp->udp_ip_snd_options_len;
		IN6_IPADDR_TO_V4MAPPED(sin->sin_addr.s_addr, &v6src);
	} else {
		ASSERT(sin6 != NULL);
		v6src = sin6->sin6_addr;
		if (IN6_IS_ADDR_V4MAPPED(&v6src)) {
			/*
			 * no need to hold the uf_lock to set the udp_ipversion
			 * since we are not yet in the fanout list
			 */
			udp->udp_ipversion = IPV4_VERSION;
			udp->udp_max_hdr_len = IP_SIMPLE_HDR_LENGTH +
			    UDPH_SIZE + udp->udp_ip_snd_options_len;
		} else {
			udp->udp_ipversion = IPV6_VERSION;
			udp->udp_max_hdr_len = udp->udp_sticky_hdrs_len;
		}
	}

	/*
	 * If udp_reuseaddr is not set, then we have to make sure that
	 * the IP address and port number the application requested
	 * (or we selected for the application) is not being used by
	 * another stream.  If another stream is already using the
	 * requested IP address and port, the behavior depends on
	 * "bind_to_req_port_only". If set the bind fails; otherwise we
	 * search for any an unused port to bind to the the stream.
	 *
	 * As per the BSD semantics, as modified by the Deering multicast
	 * changes, if udp_reuseaddr is set, then we allow multiple binds
	 * to the same port independent of the local IP address.
	 *
	 * This is slightly different than in SunOS 4.X which did not
	 * support IP multicast. Note that the change implemented by the
	 * Deering multicast code effects all binds - not only binding
	 * to IP multicast addresses.
	 *
	 * Note that when binding to port zero we ignore SO_REUSEADDR in
	 * order to guarantee a unique port.
	 */

	count = 0;
	if (udp->udp_anon_priv_bind) {
		/*
		 * loopmax = (IPPORT_RESERVED-1) -
		 *    us->us_min_anonpriv_port + 1
		 */
		loopmax = IPPORT_RESERVED - us->us_min_anonpriv_port;
	} else {
		loopmax = us->us_largest_anon_port -
		    us->us_smallest_anon_port + 1;
	}

	is_inaddr_any = V6_OR_V4_INADDR_ANY(v6src);
	zoneid = connp->conn_zoneid;

	for (;;) {
		udp_t		*udp1;
		boolean_t	found_exclbind = B_FALSE;

		/*
		 * Walk through the list of udp streams bound to
		 * requested port with the same IP address.
		 */
		lport = htons(port);
		udpf = &us->us_bind_fanout[UDP_BIND_HASH(lport,
		    us->us_bind_fanout_size)];
		mutex_enter(&udpf->uf_lock);
		for (udp1 = udpf->uf_udp; udp1 != NULL;
		    udp1 = udp1->udp_bind_hash) {
			if (lport != udp1->udp_port)
				continue;

			/*
			 * On a labeled system, we must treat bindings to ports
			 * on shared IP addresses by sockets with MAC exemption
			 * privilege as being in all zones, as there's
			 * otherwise no way to identify the right receiver.
			 */
			if (!(IPCL_ZONE_MATCH(udp1->udp_connp, zoneid) ||
			    IPCL_ZONE_MATCH(connp,
			    udp1->udp_connp->conn_zoneid)) &&
			    !connp->conn_mac_exempt && \
			    !udp1->udp_connp->conn_mac_exempt)
				continue;

			/*
			 * If UDP_EXCLBIND is set for either the bound or
			 * binding endpoint, the semantics of bind
			 * is changed according to the following chart.
			 *
			 * spec = specified address (v4 or v6)
			 * unspec = unspecified address (v4 or v6)
			 * A = specified addresses are different for endpoints
			 *
			 * bound	bind to		allowed?
			 * -------------------------------------
			 * unspec	unspec		no
			 * unspec	spec		no
			 * spec		unspec		no
			 * spec		spec		yes if A
			 *
			 * For labeled systems, SO_MAC_EXEMPT behaves the same
			 * as UDP_EXCLBIND, except that zoneid is ignored.
			 */
			if (udp1->udp_exclbind || udp->udp_exclbind ||
			    udp1->udp_connp->conn_mac_exempt ||
			    connp->conn_mac_exempt) {
				if (V6_OR_V4_INADDR_ANY(
				    udp1->udp_bound_v6src) ||
				    is_inaddr_any ||
				    IN6_ARE_ADDR_EQUAL(&udp1->udp_bound_v6src,
				    &v6src)) {
					found_exclbind = B_TRUE;
					break;
				}
				continue;
			}

			/*
			 * Check ipversion to allow IPv4 and IPv6 sockets to
			 * have disjoint port number spaces.
			 */
			if (udp->udp_ipversion != udp1->udp_ipversion) {

				/*
				 * On the first time through the loop, if the
				 * the user intentionally specified a
				 * particular port number, then ignore any
				 * bindings of the other protocol that may
				 * conflict. This allows the user to bind IPv6
				 * alone and get both v4 and v6, or bind both
				 * both and get each seperately. On subsequent
				 * times through the loop, we're checking a
				 * port that we chose (not the user) and thus
				 * we do not allow casual duplicate bindings.
				 */
				if (count == 0 && requested_port != 0)
					continue;
			}

			/*
			 * No difference depending on SO_REUSEADDR.
			 *
			 * If existing port is bound to a
			 * non-wildcard IP address and
			 * the requesting stream is bound to
			 * a distinct different IP addresses
			 * (non-wildcard, also), keep going.
			 */
			if (!is_inaddr_any &&
			    !V6_OR_V4_INADDR_ANY(udp1->udp_bound_v6src) &&
			    !IN6_ARE_ADDR_EQUAL(&udp1->udp_bound_v6src,
			    &v6src)) {
				continue;
			}
			break;
		}

		if (!found_exclbind &&
		    (udp->udp_reuseaddr && requested_port != 0)) {
			break;
		}

		if (udp1 == NULL) {
			/*
			 * No other stream has this IP address
			 * and port number. We can use it.
			 */
			break;
		}
		mutex_exit(&udpf->uf_lock);
		if (bind_to_req_port_only) {
			/*
			 * We get here only when requested port
			 * is bound (and only first  of the for()
			 * loop iteration).
			 *
			 * The semantics of this bind request
			 * require it to fail so we return from
			 * the routine (and exit the loop).
			 *
			 */
			udp->udp_pending_op = -1;
			rw_exit(&udp->udp_rwlock);
			udp_err_ack(q, mp, TADDRBUSY, 0);
			return;
		}

		if (udp->udp_anon_priv_bind) {
			port = udp_get_next_priv_port(udp);
		} else {
			if ((count == 0) && (requested_port != 0)) {
				/*
				 * If the application wants us to find
				 * a port, get one to start with. Set
				 * requested_port to 0, so that we will
				 * update us->us_next_port_to_try below.
				 */
				port = udp_update_next_port(udp,
				    us->us_next_port_to_try, B_TRUE);
				requested_port = 0;
			} else {
				port = udp_update_next_port(udp, port + 1,
				    B_FALSE);
			}
		}

		if (port == 0 || ++count >= loopmax) {
			/*
			 * We've tried every possible port number and
			 * there are none available, so send an error
			 * to the user.
			 */
			udp->udp_pending_op = -1;
			rw_exit(&udp->udp_rwlock);
			udp_err_ack(q, mp, TNOADDR, 0);
			return;
		}
	}

	/*
	 * Copy the source address into our udp structure.  This address
	 * may still be zero; if so, ip will fill in the correct address
	 * each time an outbound packet is passed to it.
	 * If we are binding to a broadcast or multicast address then
	 * udp_bind_ack will clear the source address when it receives
	 * the T_BIND_ACK.
	 */
	udp->udp_v6src = udp->udp_bound_v6src = v6src;
	udp->udp_port = lport;
	/*
	 * Now reset the the next anonymous port if the application requested
	 * an anonymous port, or we handed out the next anonymous port.
	 */
	if ((requested_port == 0) && (!udp->udp_anon_priv_bind)) {
		us->us_next_port_to_try = port + 1;
	}

	/* Initialize the O_T_BIND_REQ/T_BIND_REQ for ip. */
	if (udp->udp_family == AF_INET) {
		sin->sin_port = udp->udp_port;
	} else {
		int error;

		sin6->sin6_port = udp->udp_port;
		/* Rebuild the header template */
		error = udp_build_hdrs(udp);
		if (error != 0) {
			udp->udp_pending_op = -1;
			rw_exit(&udp->udp_rwlock);
			mutex_exit(&udpf->uf_lock);
			udp_err_ack(q, mp, TSYSERR, error);
			return;
		}
	}
	udp->udp_state = TS_IDLE;
	udp_bind_hash_insert(udpf, udp);
	mutex_exit(&udpf->uf_lock);
	rw_exit(&udp->udp_rwlock);

	if (cl_inet_bind) {
		/*
		 * Running in cluster mode - register bind information
		 */
		if (udp->udp_ipversion == IPV4_VERSION) {
			(*cl_inet_bind)(IPPROTO_UDP, AF_INET,
			    (uint8_t *)(&V4_PART_OF_V6(udp->udp_v6src)),
			    (in_port_t)udp->udp_port);
		} else {
			(*cl_inet_bind)(IPPROTO_UDP, AF_INET6,
			    (uint8_t *)&(udp->udp_v6src),
			    (in_port_t)udp->udp_port);
		}

	}

	connp->conn_anon_port = (is_system_labeled() && requested_port == 0);
	if (is_system_labeled() && (!connp->conn_anon_port ||
	    connp->conn_anon_mlp)) {
		uint16_t mlpport;
		cred_t *cr = connp->conn_cred;
		zone_t *zone;

		zone = crgetzone(cr);
		connp->conn_mlp_type = udp->udp_recvucred ? mlptBoth :
		    mlptSingle;
		addrtype = tsol_mlp_addr_type(zone->zone_id, IPV6_VERSION,
		    &v6src, us->us_netstack->netstack_ip);
		if (addrtype == mlptSingle) {
			rw_enter(&udp->udp_rwlock, RW_WRITER);
			udp->udp_pending_op = -1;
			rw_exit(&udp->udp_rwlock);
			udp_err_ack(q, mp, TNOADDR, 0);
			connp->conn_anon_port = B_FALSE;
			connp->conn_mlp_type = mlptSingle;
			return;
		}
		mlpport = connp->conn_anon_port ? PMAPPORT : port;
		mlptype = tsol_mlp_port_type(zone, IPPROTO_UDP, mlpport,
		    addrtype);
		if (mlptype != mlptSingle &&
		    (connp->conn_mlp_type == mlptSingle ||
		    secpolicy_net_bindmlp(cr) != 0)) {
			if (udp->udp_debug) {
				(void) strlog(UDP_MOD_ID, 0, 1,
				    SL_ERROR|SL_TRACE,
				    "udp_bind: no priv for multilevel port %d",
				    mlpport);
			}
			rw_enter(&udp->udp_rwlock, RW_WRITER);
			udp->udp_pending_op = -1;
			rw_exit(&udp->udp_rwlock);
			udp_err_ack(q, mp, TACCES, 0);
			connp->conn_anon_port = B_FALSE;
			connp->conn_mlp_type = mlptSingle;
			return;
		}

		/*
		 * If we're specifically binding a shared IP address and the
		 * port is MLP on shared addresses, then check to see if this
		 * zone actually owns the MLP.  Reject if not.
		 */
		if (mlptype == mlptShared && addrtype == mlptShared) {
			/*
			 * No need to handle exclusive-stack zones since
			 * ALL_ZONES only applies to the shared stack.
			 */
			zoneid_t mlpzone;

			mlpzone = tsol_mlp_findzone(IPPROTO_UDP,
			    htons(mlpport));
			if (connp->conn_zoneid != mlpzone) {
				if (udp->udp_debug) {
					(void) strlog(UDP_MOD_ID, 0, 1,
					    SL_ERROR|SL_TRACE,
					    "udp_bind: attempt to bind port "
					    "%d on shared addr in zone %d "
					    "(should be %d)",
					    mlpport, connp->conn_zoneid,
					    mlpzone);
				}
				rw_enter(&udp->udp_rwlock, RW_WRITER);
				udp->udp_pending_op = -1;
				rw_exit(&udp->udp_rwlock);
				udp_err_ack(q, mp, TACCES, 0);
				connp->conn_anon_port = B_FALSE;
				connp->conn_mlp_type = mlptSingle;
				return;
			}
		}
		if (connp->conn_anon_port) {
			int error;

			error = tsol_mlp_anon(zone, mlptype, connp->conn_ulp,
			    port, B_TRUE);
			if (error != 0) {
				if (udp->udp_debug) {
					(void) strlog(UDP_MOD_ID, 0, 1,
					    SL_ERROR|SL_TRACE,
					    "udp_bind: cannot establish anon "
					    "MLP for port %d", port);
				}
				rw_enter(&udp->udp_rwlock, RW_WRITER);
				udp->udp_pending_op = -1;
				rw_exit(&udp->udp_rwlock);
				udp_err_ack(q, mp, TACCES, 0);
				connp->conn_anon_port = B_FALSE;
				connp->conn_mlp_type = mlptSingle;
				return;
			}
		}
		connp->conn_mlp_type = mlptype;
	}

	/* Pass the protocol number in the message following the address. */
	*mp->b_wptr++ = IPPROTO_UDP;
	if (!V6_OR_V4_INADDR_ANY(udp->udp_v6src)) {
		/*
		 * Append a request for an IRE if udp_v6src not
		 * zero (IPv4 - INADDR_ANY, or IPv6 - all-zeroes address).
		 */
		mp->b_cont = allocb(sizeof (ire_t), BPRI_HI);
		if (!mp->b_cont) {
			rw_enter(&udp->udp_rwlock, RW_WRITER);
			udp->udp_pending_op = -1;
			rw_exit(&udp->udp_rwlock);
			udp_err_ack(q, mp, TSYSERR, ENOMEM);
			return;
		}
		mp->b_cont->b_wptr += sizeof (ire_t);
		mp->b_cont->b_datap->db_type = IRE_DB_REQ_TYPE;
	}
	if (udp->udp_family == AF_INET6)
		mp = ip_bind_v6(q, mp, connp, NULL);
	else
		mp = ip_bind_v4(q, mp, connp);

	/* The above return NULL if the bind needs to be deferred */
	if (mp != NULL)
		udp_bind_result(connp, mp);
	else
		CONN_INC_REF(connp);
}

/*
 * This is called from ip_wput_nondata to handle the results of a
 * deferred UDP bind. It is called once the bind has been completed.
 */
void
udp_resume_bind(conn_t *connp, mblk_t *mp)
{
	ASSERT(connp != NULL && IPCL_IS_UDP(connp));

	udp_bind_result(connp, mp);

	CONN_OPER_PENDING_DONE(connp);
}

/*
 * This routine handles each T_CONN_REQ message passed to udp.  It
 * associates a default destination address with the stream.
 *
 * This routine sends down a T_BIND_REQ to IP with the following mblks:
 *	T_BIND_REQ	- specifying local and remote address/port
 *	IRE_DB_REQ_TYPE	- to get an IRE back containing ire_type and src
 *	T_OK_ACK	- for the T_CONN_REQ
 *	T_CONN_CON	- to keep the TPI user happy
 *
 * The connect completes in udp_bind_result.
 * When a T_BIND_ACK is received information is extracted from the IRE
 * and the two appended messages are sent to the TPI user.
 * Should udp_bind_result receive T_ERROR_ACK for the T_BIND_REQ it will
 * convert it to an error ack for the appropriate primitive.
 */
static void
udp_connect(queue_t *q, mblk_t *mp)
{
	sin6_t	*sin6;
	sin_t	*sin;
	struct T_conn_req	*tcr;
	in6_addr_t v6dst;
	ipaddr_t v4dst;
	uint16_t dstport;
	uint32_t flowinfo;
	mblk_t	*mp1, *mp2;
	udp_fanout_t	*udpf;
	udp_t	*udp, *udp1;
	ushort_t	ipversion;
	udp_stack_t	*us;
	conn_t		*connp = Q_TO_CONN(q);

	udp = connp->conn_udp;
	tcr = (struct T_conn_req *)mp->b_rptr;
	us = udp->udp_us;

	/* A bit of sanity checking */
	if ((mp->b_wptr - mp->b_rptr) < sizeof (struct T_conn_req)) {
		udp_err_ack(q, mp, TPROTO, 0);
		return;
	}

	if (tcr->OPT_length != 0) {
		udp_err_ack(q, mp, TBADOPT, 0);
		return;
	}

	/*
	 * Determine packet type based on type of address passed in
	 * the request should contain an IPv4 or IPv6 address.
	 * Make sure that address family matches the type of
	 * family of the the address passed down
	 */
	switch (tcr->DEST_length) {
	default:
		udp_err_ack(q, mp, TBADADDR, 0);
		return;

	case sizeof (sin_t):
		sin = (sin_t *)mi_offset_param(mp, tcr->DEST_offset,
		    sizeof (sin_t));
		if (sin == NULL || !OK_32PTR((char *)sin)) {
			udp_err_ack(q, mp, TSYSERR, EINVAL);
			return;
		}
		if (udp->udp_family != AF_INET ||
		    sin->sin_family != AF_INET) {
			udp_err_ack(q, mp, TSYSERR, EAFNOSUPPORT);
			return;
		}
		v4dst = sin->sin_addr.s_addr;
		dstport = sin->sin_port;
		IN6_IPADDR_TO_V4MAPPED(v4dst, &v6dst);
		ASSERT(udp->udp_ipversion == IPV4_VERSION);
		ipversion = IPV4_VERSION;
		break;

	case sizeof (sin6_t):
		sin6 = (sin6_t *)mi_offset_param(mp, tcr->DEST_offset,
		    sizeof (sin6_t));
		if (sin6 == NULL || !OK_32PTR((char *)sin6)) {
			udp_err_ack(q, mp, TSYSERR, EINVAL);
			return;
		}
		if (udp->udp_family != AF_INET6 ||
		    sin6->sin6_family != AF_INET6) {
			udp_err_ack(q, mp, TSYSERR, EAFNOSUPPORT);
			return;
		}
		v6dst = sin6->sin6_addr;
		dstport = sin6->sin6_port;
		if (IN6_IS_ADDR_V4MAPPED(&v6dst)) {
			IN6_V4MAPPED_TO_IPADDR(&v6dst, v4dst);
			ipversion = IPV4_VERSION;
			flowinfo = 0;
		} else {
			ipversion = IPV6_VERSION;
			flowinfo = sin6->sin6_flowinfo;
		}
		break;
	}
	if (dstport == 0) {
		udp_err_ack(q, mp, TBADADDR, 0);
		return;
	}

	rw_enter(&udp->udp_rwlock, RW_WRITER);

	/*
	 * This UDP must have bound to a port already before doing a connect.
	 * TPI mandates that users must send TPI primitives only 1 at a time
	 * and wait for the response before sending the next primitive.
	 */
	if (udp->udp_state == TS_UNBND || udp->udp_pending_op != -1) {
		rw_exit(&udp->udp_rwlock);
		(void) mi_strlog(q, 1, SL_ERROR|SL_TRACE,
		    "udp_connect: bad state, %u", udp->udp_state);
		udp_err_ack(q, mp, TOUTSTATE, 0);
		return;
	}
	udp->udp_pending_op = T_CONN_REQ;
	ASSERT(udp->udp_port != 0 && udp->udp_ptpbhn != NULL);

	if (ipversion == IPV4_VERSION) {
		udp->udp_max_hdr_len = IP_SIMPLE_HDR_LENGTH + UDPH_SIZE +
		    udp->udp_ip_snd_options_len;
	} else {
		udp->udp_max_hdr_len = udp->udp_sticky_hdrs_len;
	}

	udpf = &us->us_bind_fanout[UDP_BIND_HASH(udp->udp_port,
	    us->us_bind_fanout_size)];

	mutex_enter(&udpf->uf_lock);
	if (udp->udp_state == TS_DATA_XFER) {
		/* Already connected - clear out state */
		udp->udp_v6src = udp->udp_bound_v6src;
		udp->udp_state = TS_IDLE;
	}

	/*
	 * Create a default IP header with no IP options.
	 */
	udp->udp_dstport = dstport;
	udp->udp_ipversion = ipversion;
	if (ipversion == IPV4_VERSION) {
		/*
		 * Interpret a zero destination to mean loopback.
		 * Update the T_CONN_REQ (sin/sin6) since it is used to
		 * generate the T_CONN_CON.
		 */
		if (v4dst == INADDR_ANY) {
			v4dst = htonl(INADDR_LOOPBACK);
			IN6_IPADDR_TO_V4MAPPED(v4dst, &v6dst);
			if (udp->udp_family == AF_INET) {
				sin->sin_addr.s_addr = v4dst;
			} else {
				sin6->sin6_addr = v6dst;
			}
		}
		udp->udp_v6dst = v6dst;
		udp->udp_flowinfo = 0;

		/*
		 * If the destination address is multicast and
		 * an outgoing multicast interface has been set,
		 * use the address of that interface as our
		 * source address if no source address has been set.
		 */
		if (V4_PART_OF_V6(udp->udp_v6src) == INADDR_ANY &&
		    CLASSD(v4dst) &&
		    udp->udp_multicast_if_addr != INADDR_ANY) {
			IN6_IPADDR_TO_V4MAPPED(udp->udp_multicast_if_addr,
			    &udp->udp_v6src);
		}
	} else {
		ASSERT(udp->udp_ipversion == IPV6_VERSION);
		/*
		 * Interpret a zero destination to mean loopback.
		 * Update the T_CONN_REQ (sin/sin6) since it is used to
		 * generate the T_CONN_CON.
		 */
		if (IN6_IS_ADDR_UNSPECIFIED(&v6dst)) {
			v6dst = ipv6_loopback;
			sin6->sin6_addr = v6dst;
		}
		udp->udp_v6dst = v6dst;
		udp->udp_flowinfo = flowinfo;
		/*
		 * If the destination address is multicast and
		 * an outgoing multicast interface has been set,
		 * then the ip bind logic will pick the correct source
		 * address (i.e. matching the outgoing multicast interface).
		 */
	}

	/*
	 * Verify that the src/port/dst/port is unique for all
	 * connections in TS_DATA_XFER
	 */
	for (udp1 = udpf->uf_udp; udp1 != NULL; udp1 = udp1->udp_bind_hash) {
		if (udp1->udp_state != TS_DATA_XFER)
			continue;
		if (udp->udp_port != udp1->udp_port ||
		    udp->udp_ipversion != udp1->udp_ipversion ||
		    dstport != udp1->udp_dstport ||
		    !IN6_ARE_ADDR_EQUAL(&udp->udp_v6src, &udp1->udp_v6src) ||
		    !IN6_ARE_ADDR_EQUAL(&v6dst, &udp1->udp_v6dst) ||
		    !(IPCL_ZONE_MATCH(udp->udp_connp,
		    udp1->udp_connp->conn_zoneid) ||
		    IPCL_ZONE_MATCH(udp1->udp_connp,
		    udp->udp_connp->conn_zoneid)))
			continue;
		mutex_exit(&udpf->uf_lock);
		udp->udp_pending_op = -1;
		rw_exit(&udp->udp_rwlock);
		udp_err_ack(q, mp, TBADADDR, 0);
		return;
	}
	udp->udp_state = TS_DATA_XFER;
	mutex_exit(&udpf->uf_lock);

	/*
	 * Send down bind to IP to verify that there is a route
	 * and to determine the source address.
	 * This will come back as T_BIND_ACK with an IRE_DB_TYPE in rput.
	 */
	if (udp->udp_family == AF_INET)
		mp1 = udp_ip_bind_mp(udp, O_T_BIND_REQ, sizeof (ipa_conn_t));
	else
		mp1 = udp_ip_bind_mp(udp, O_T_BIND_REQ, sizeof (ipa6_conn_t));
	if (mp1 == NULL) {
bind_failed:
		mutex_enter(&udpf->uf_lock);
		udp->udp_state = TS_IDLE;
		udp->udp_pending_op = -1;
		mutex_exit(&udpf->uf_lock);
		rw_exit(&udp->udp_rwlock);
		udp_err_ack(q, mp, TSYSERR, ENOMEM);
		return;
	}

	rw_exit(&udp->udp_rwlock);
	/*
	 * We also have to send a connection confirmation to
	 * keep TLI happy. Prepare it for udp_bind_result.
	 */
	if (udp->udp_family == AF_INET)
		mp2 = mi_tpi_conn_con(NULL, (char *)sin,
		    sizeof (*sin), NULL, 0);
	else
		mp2 = mi_tpi_conn_con(NULL, (char *)sin6,
		    sizeof (*sin6), NULL, 0);
	if (mp2 == NULL) {
		freemsg(mp1);
		rw_enter(&udp->udp_rwlock, RW_WRITER);
		goto bind_failed;
	}

	mp = mi_tpi_ok_ack_alloc(mp);
	if (mp == NULL) {
		/* Unable to reuse the T_CONN_REQ for the ack. */
		freemsg(mp2);
		rw_enter(&udp->udp_rwlock, RW_WRITER);
		mutex_enter(&udpf->uf_lock);
		udp->udp_state = TS_IDLE;
		udp->udp_pending_op = -1;
		mutex_exit(&udpf->uf_lock);
		rw_exit(&udp->udp_rwlock);
		udp_err_ack_prim(q, mp1, T_CONN_REQ, TSYSERR, ENOMEM);
		return;
	}

	/* Hang onto the T_OK_ACK and T_CONN_CON for later. */
	linkb(mp1, mp);
	linkb(mp1, mp2);

	mblk_setcred(mp1, connp->conn_cred);
	if (udp->udp_family == AF_INET)
		mp1 = ip_bind_v4(q, mp1, connp);
	else
		mp1 = ip_bind_v6(q, mp1, connp, NULL);

	/* The above return NULL if the bind needs to be deferred */
	if (mp1 != NULL)
		udp_bind_result(connp, mp1);
	else
		CONN_INC_REF(connp);
}

static int
udp_close(queue_t *q)
{
	conn_t	*connp = (conn_t *)q->q_ptr;
	udp_t	*udp;

	ASSERT(connp != NULL && IPCL_IS_UDP(connp));
	udp = connp->conn_udp;

	udp_quiesce_conn(connp);
	ip_quiesce_conn(connp);
	/*
	 * Disable read-side synchronous stream
	 * interface and drain any queued data.
	 */
	udp_rcv_drain(q, udp, B_TRUE);
	ASSERT(!udp->udp_direct_sockfs);

	qprocsoff(q);

	ASSERT(udp->udp_rcv_cnt == 0);
	ASSERT(udp->udp_rcv_msgcnt == 0);
	ASSERT(udp->udp_rcv_list_head == NULL);
	ASSERT(udp->udp_rcv_list_tail == NULL);

	udp_close_free(connp);

	/*
	 * Now we are truly single threaded on this stream, and can
	 * delete the things hanging off the connp, and finally the connp.
	 * We removed this connp from the fanout list, it cannot be
	 * accessed thru the fanouts, and we already waited for the
	 * conn_ref to drop to 0. We are already in close, so
	 * there cannot be any other thread from the top. qprocsoff
	 * has completed, and service has completed or won't run in
	 * future.
	 */
	ASSERT(connp->conn_ref == 1);
	inet_minor_free(connp->conn_minor_arena, connp->conn_dev);
	connp->conn_ref--;
	ipcl_conn_destroy(connp);

	q->q_ptr = WR(q)->q_ptr = NULL;
	return (0);
}

/*
 * Called in the close path to quiesce the conn
 */
void
udp_quiesce_conn(conn_t *connp)
{
	udp_t	*udp = connp->conn_udp;

	if (cl_inet_unbind != NULL && udp->udp_state == TS_IDLE) {
		/*
		 * Running in cluster mode - register unbind information
		 */
		if (udp->udp_ipversion == IPV4_VERSION) {
			(*cl_inet_unbind)(IPPROTO_UDP, AF_INET,
			    (uint8_t *)(&(V4_PART_OF_V6(udp->udp_v6src))),
			    (in_port_t)udp->udp_port);
		} else {
			(*cl_inet_unbind)(IPPROTO_UDP, AF_INET6,
			    (uint8_t *)(&(udp->udp_v6src)),
			    (in_port_t)udp->udp_port);
		}
	}

	udp_bind_hash_remove(udp, B_FALSE);

}

void
udp_close_free(conn_t *connp)
{
	udp_t *udp = connp->conn_udp;

	/* If there are any options associated with the stream, free them. */
	if (udp->udp_ip_snd_options != NULL) {
		mi_free((char *)udp->udp_ip_snd_options);
		udp->udp_ip_snd_options = NULL;
		udp->udp_ip_snd_options_len = 0;
	}

	if (udp->udp_ip_rcv_options != NULL) {
		mi_free((char *)udp->udp_ip_rcv_options);
		udp->udp_ip_rcv_options = NULL;
		udp->udp_ip_rcv_options_len = 0;
	}

	/* Free memory associated with sticky options */
	if (udp->udp_sticky_hdrs_len != 0) {
		kmem_free(udp->udp_sticky_hdrs,
		    udp->udp_sticky_hdrs_len);
		udp->udp_sticky_hdrs = NULL;
		udp->udp_sticky_hdrs_len = 0;
	}

	ip6_pkt_free(&udp->udp_sticky_ipp);

	/*
	 * Clear any fields which the kmem_cache constructor clears.
	 * Only udp_connp needs to be preserved.
	 * TBD: We should make this more efficient to avoid clearing
	 * everything.
	 */
	ASSERT(udp->udp_connp == connp);
	bzero(udp, sizeof (udp_t));
	udp->udp_connp = connp;
}

/*
 * This routine handles each T_DISCON_REQ message passed to udp
 * as an indicating that UDP is no longer connected. This results
 * in sending a T_BIND_REQ to IP to restore the binding to just
 * the local address/port.
 *
 * This routine sends down a T_BIND_REQ to IP with the following mblks:
 *	T_BIND_REQ	- specifying just the local address/port
 *	T_OK_ACK	- for the T_DISCON_REQ
 *
 * The disconnect completes in udp_bind_result.
 * When a T_BIND_ACK is received the appended T_OK_ACK is sent to the TPI user.
 * Should udp_bind_result receive T_ERROR_ACK for the T_BIND_REQ it will
 * convert it to an error ack for the appropriate primitive.
 */
static void
udp_disconnect(queue_t *q, mblk_t *mp)
{
	udp_t	*udp;
	mblk_t	*mp1;
	udp_fanout_t *udpf;
	udp_stack_t *us;
	conn_t	*connp = Q_TO_CONN(q);

	udp = connp->conn_udp;
	us = udp->udp_us;
	rw_enter(&udp->udp_rwlock, RW_WRITER);
	if (udp->udp_state != TS_DATA_XFER || udp->udp_pending_op != -1) {
		rw_exit(&udp->udp_rwlock);
		(void) mi_strlog(q, 1, SL_ERROR|SL_TRACE,
		    "udp_disconnect: bad state, %u", udp->udp_state);
		udp_err_ack(q, mp, TOUTSTATE, 0);
		return;
	}
	udp->udp_pending_op = T_DISCON_REQ;
	udpf = &us->us_bind_fanout[UDP_BIND_HASH(udp->udp_port,
	    us->us_bind_fanout_size)];
	mutex_enter(&udpf->uf_lock);
	udp->udp_v6src = udp->udp_bound_v6src;
	udp->udp_state = TS_IDLE;
	mutex_exit(&udpf->uf_lock);

	/*
	 * Send down bind to IP to remove the full binding and revert
	 * to the local address binding.
	 */
	if (udp->udp_family == AF_INET)
		mp1 = udp_ip_bind_mp(udp, O_T_BIND_REQ, sizeof (sin_t));
	else
		mp1 = udp_ip_bind_mp(udp, O_T_BIND_REQ, sizeof (sin6_t));
	if (mp1 == NULL) {
		udp->udp_pending_op = -1;
		rw_exit(&udp->udp_rwlock);
		udp_err_ack(q, mp, TSYSERR, ENOMEM);
		return;
	}
	mp = mi_tpi_ok_ack_alloc(mp);
	if (mp == NULL) {
		/* Unable to reuse the T_DISCON_REQ for the ack. */
		udp->udp_pending_op = -1;
		rw_exit(&udp->udp_rwlock);
		udp_err_ack_prim(q, mp1, T_DISCON_REQ, TSYSERR, ENOMEM);
		return;
	}

	if (udp->udp_family == AF_INET6) {
		int error;

		/* Rebuild the header template */
		error = udp_build_hdrs(udp);
		if (error != 0) {
			udp->udp_pending_op = -1;
			rw_exit(&udp->udp_rwlock);
			udp_err_ack_prim(q, mp, T_DISCON_REQ, TSYSERR, error);
			freemsg(mp1);
			return;
		}
	}

	rw_exit(&udp->udp_rwlock);
	/* Append the T_OK_ACK to the T_BIND_REQ for udp_bind_ack */
	linkb(mp1, mp);

	if (udp->udp_family == AF_INET6)
		mp1 = ip_bind_v6(q, mp1, connp, NULL);
	else
		mp1 = ip_bind_v4(q, mp1, connp);

	/* The above return NULL if the bind needs to be deferred */
	if (mp1 != NULL)
		udp_bind_result(connp, mp1);
	else
		CONN_INC_REF(connp);
}

/* This routine creates a T_ERROR_ACK message and passes it upstream. */
static void
udp_err_ack(queue_t *q, mblk_t *mp, t_scalar_t t_error, int sys_error)
{
	if ((mp = mi_tpi_err_ack_alloc(mp, t_error, sys_error)) != NULL)
		qreply(q, mp);
}

/* Shorthand to generate and send TPI error acks to our client */
static void
udp_err_ack_prim(queue_t *q, mblk_t *mp, int primitive, t_scalar_t t_error,
    int sys_error)
{
	struct T_error_ack	*teackp;

	if ((mp = tpi_ack_alloc(mp, sizeof (struct T_error_ack),
	    M_PCPROTO, T_ERROR_ACK)) != NULL) {
		teackp = (struct T_error_ack *)mp->b_rptr;
		teackp->ERROR_prim = primitive;
		teackp->TLI_error = t_error;
		teackp->UNIX_error = sys_error;
		qreply(q, mp);
	}
}

/*ARGSUSED*/
static int
udp_extra_priv_ports_get(queue_t *q, mblk_t *mp, caddr_t cp, cred_t *cr)
{
	int i;
	udp_t		*udp = Q_TO_UDP(q);
	udp_stack_t *us = udp->udp_us;

	for (i = 0; i < us->us_num_epriv_ports; i++) {
		if (us->us_epriv_ports[i] != 0)
			(void) mi_mpprintf(mp, "%d ", us->us_epriv_ports[i]);
	}
	return (0);
}

/* ARGSUSED */
static int
udp_extra_priv_ports_add(queue_t *q, mblk_t *mp, char *value, caddr_t cp,
    cred_t *cr)
{
	long	new_value;
	int	i;
	udp_t		*udp = Q_TO_UDP(q);
	udp_stack_t *us = udp->udp_us;

	/*
	 * Fail the request if the new value does not lie within the
	 * port number limits.
	 */
	if (ddi_strtol(value, NULL, 10, &new_value) != 0 ||
	    new_value <= 0 || new_value >= 65536) {
		return (EINVAL);
	}

	/* Check if the value is already in the list */
	for (i = 0; i < us->us_num_epriv_ports; i++) {
		if (new_value == us->us_epriv_ports[i]) {
			return (EEXIST);
		}
	}
	/* Find an empty slot */
	for (i = 0; i < us->us_num_epriv_ports; i++) {
		if (us->us_epriv_ports[i] == 0)
			break;
	}
	if (i == us->us_num_epriv_ports) {
		return (EOVERFLOW);
	}

	/* Set the new value */
	us->us_epriv_ports[i] = (in_port_t)new_value;
	return (0);
}

/* ARGSUSED */
static int
udp_extra_priv_ports_del(queue_t *q, mblk_t *mp, char *value, caddr_t cp,
    cred_t *cr)
{
	long	new_value;
	int	i;
	udp_t		*udp = Q_TO_UDP(q);
	udp_stack_t *us = udp->udp_us;

	/*
	 * Fail the request if the new value does not lie within the
	 * port number limits.
	 */
	if (ddi_strtol(value, NULL, 10, &new_value) != 0 ||
	    new_value <= 0 || new_value >= 65536) {
		return (EINVAL);
	}

	/* Check that the value is already in the list */
	for (i = 0; i < us->us_num_epriv_ports; i++) {
		if (us->us_epriv_ports[i] == new_value)
			break;
	}
	if (i == us->us_num_epriv_ports) {
		return (ESRCH);
	}

	/* Clear the value */
	us->us_epriv_ports[i] = 0;
	return (0);
}

/* At minimum we need 4 bytes of UDP header */
#define	ICMP_MIN_UDP_HDR	4

/*
 * udp_icmp_error is called by udp_input to process ICMP msgs. passed up by IP.
 * Generates the appropriate T_UDERROR_IND for permanent (non-transient) errors.
 * Assumes that IP has pulled up everything up to and including the ICMP header.
 */
static void
udp_icmp_error(queue_t *q, mblk_t *mp)
{
	icmph_t *icmph;
	ipha_t	*ipha;
	int	iph_hdr_length;
	udpha_t	*udpha;
	sin_t	sin;
	sin6_t	sin6;
	mblk_t	*mp1;
	int	error = 0;
	udp_t	*udp = Q_TO_UDP(q);

	ipha = (ipha_t *)mp->b_rptr;

	ASSERT(OK_32PTR(mp->b_rptr));

	if (IPH_HDR_VERSION(ipha) != IPV4_VERSION) {
		ASSERT(IPH_HDR_VERSION(ipha) == IPV6_VERSION);
		udp_icmp_error_ipv6(q, mp);
		return;
	}
	ASSERT(IPH_HDR_VERSION(ipha) == IPV4_VERSION);

	/* Skip past the outer IP and ICMP headers */
	iph_hdr_length = IPH_HDR_LENGTH(ipha);
	icmph = (icmph_t *)&mp->b_rptr[iph_hdr_length];
	ipha = (ipha_t *)&icmph[1];

	/* Skip past the inner IP and find the ULP header */
	iph_hdr_length = IPH_HDR_LENGTH(ipha);
	udpha = (udpha_t *)((char *)ipha + iph_hdr_length);

	switch (icmph->icmph_type) {
	case ICMP_DEST_UNREACHABLE:
		switch (icmph->icmph_code) {
		case ICMP_FRAGMENTATION_NEEDED:
			/*
			 * IP has already adjusted the path MTU.
			 */
			break;
		case ICMP_PORT_UNREACHABLE:
		case ICMP_PROTOCOL_UNREACHABLE:
			error = ECONNREFUSED;
			break;
		default:
			/* Transient errors */
			break;
		}
		break;
	default:
		/* Transient errors */
		break;
	}
	if (error == 0) {
		freemsg(mp);
		return;
	}

	/*
	 * Deliver T_UDERROR_IND when the application has asked for it.
	 * The socket layer enables this automatically when connected.
	 */
	if (!udp->udp_dgram_errind) {
		freemsg(mp);
		return;
	}

	switch (udp->udp_family) {
	case AF_INET:
		sin = sin_null;
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = ipha->ipha_dst;
		sin.sin_port = udpha->uha_dst_port;
		mp1 = mi_tpi_uderror_ind((char *)&sin, sizeof (sin_t), NULL, 0,
		    error);
		break;
	case AF_INET6:
		sin6 = sin6_null;
		sin6.sin6_family = AF_INET6;
		IN6_IPADDR_TO_V4MAPPED(ipha->ipha_dst, &sin6.sin6_addr);
		sin6.sin6_port = udpha->uha_dst_port;

		mp1 = mi_tpi_uderror_ind((char *)&sin6, sizeof (sin6_t),
		    NULL, 0, error);
		break;
	}
	if (mp1)
		putnext(q, mp1);
	freemsg(mp);
}

/*
 * udp_icmp_error_ipv6 is called by udp_icmp_error to process ICMP for IPv6.
 * Generates the appropriate T_UDERROR_IND for permanent (non-transient) errors.
 * Assumes that IP has pulled up all the extension headers as well as the
 * ICMPv6 header.
 */
static void
udp_icmp_error_ipv6(queue_t *q, mblk_t *mp)
{
	icmp6_t		*icmp6;
	ip6_t		*ip6h, *outer_ip6h;
	uint16_t	iph_hdr_length;
	uint8_t		*nexthdrp;
	udpha_t		*udpha;
	sin6_t		sin6;
	mblk_t		*mp1;
	int		error = 0;
	udp_t		*udp = Q_TO_UDP(q);
	udp_stack_t	*us = udp->udp_us;

	outer_ip6h = (ip6_t *)mp->b_rptr;
	if (outer_ip6h->ip6_nxt != IPPROTO_ICMPV6)
		iph_hdr_length = ip_hdr_length_v6(mp, outer_ip6h);
	else
		iph_hdr_length = IPV6_HDR_LEN;
	icmp6 = (icmp6_t *)&mp->b_rptr[iph_hdr_length];
	ip6h = (ip6_t *)&icmp6[1];
	if (!ip_hdr_length_nexthdr_v6(mp, ip6h, &iph_hdr_length, &nexthdrp)) {
		freemsg(mp);
		return;
	}
	udpha = (udpha_t *)((char *)ip6h + iph_hdr_length);

	switch (icmp6->icmp6_type) {
	case ICMP6_DST_UNREACH:
		switch (icmp6->icmp6_code) {
		case ICMP6_DST_UNREACH_NOPORT:
			error = ECONNREFUSED;
			break;
		case ICMP6_DST_UNREACH_ADMIN:
		case ICMP6_DST_UNREACH_NOROUTE:
		case ICMP6_DST_UNREACH_BEYONDSCOPE:
		case ICMP6_DST_UNREACH_ADDR:
			/* Transient errors */
			break;
		default:
			break;
		}
		break;
	case ICMP6_PACKET_TOO_BIG: {
		struct T_unitdata_ind	*tudi;
		struct T_opthdr		*toh;
		size_t			udi_size;
		mblk_t			*newmp;
		t_scalar_t		opt_length = sizeof (struct T_opthdr) +
		    sizeof (struct ip6_mtuinfo);
		sin6_t			*sin6;
		struct ip6_mtuinfo	*mtuinfo;

		/*
		 * If the application has requested to receive path mtu
		 * information, send up an empty message containing an
		 * IPV6_PATHMTU ancillary data item.
		 */
		if (!udp->udp_ipv6_recvpathmtu)
			break;

		udi_size = sizeof (struct T_unitdata_ind) + sizeof (sin6_t) +
		    opt_length;
		if ((newmp = allocb(udi_size, BPRI_MED)) == NULL) {
			BUMP_MIB(&us->us_udp_mib, udpInErrors);
			break;
		}

		/*
		 * newmp->b_cont is left to NULL on purpose.  This is an
		 * empty message containing only ancillary data.
		 */
		newmp->b_datap->db_type = M_PROTO;
		tudi = (struct T_unitdata_ind *)newmp->b_rptr;
		newmp->b_wptr = (uchar_t *)tudi + udi_size;
		tudi->PRIM_type = T_UNITDATA_IND;
		tudi->SRC_length = sizeof (sin6_t);
		tudi->SRC_offset = sizeof (struct T_unitdata_ind);
		tudi->OPT_offset = tudi->SRC_offset + sizeof (sin6_t);
		tudi->OPT_length = opt_length;

		sin6 = (sin6_t *)&tudi[1];
		bzero(sin6, sizeof (sin6_t));
		sin6->sin6_family = AF_INET6;
		sin6->sin6_addr = udp->udp_v6dst;

		toh = (struct T_opthdr *)&sin6[1];
		toh->level = IPPROTO_IPV6;
		toh->name = IPV6_PATHMTU;
		toh->len = opt_length;
		toh->status = 0;

		mtuinfo = (struct ip6_mtuinfo *)&toh[1];
		bzero(mtuinfo, sizeof (struct ip6_mtuinfo));
		mtuinfo->ip6m_addr.sin6_family = AF_INET6;
		mtuinfo->ip6m_addr.sin6_addr = ip6h->ip6_dst;
		mtuinfo->ip6m_mtu = icmp6->icmp6_mtu;
		/*
		 * We've consumed everything we need from the original
		 * message.  Free it, then send our empty message.
		 */
		freemsg(mp);
		putnext(q, newmp);
		return;
	}
	case ICMP6_TIME_EXCEEDED:
		/* Transient errors */
		break;
	case ICMP6_PARAM_PROB:
		/* If this corresponds to an ICMP_PROTOCOL_UNREACHABLE */
		if (icmp6->icmp6_code == ICMP6_PARAMPROB_NEXTHEADER &&
		    (uchar_t *)ip6h + icmp6->icmp6_pptr ==
		    (uchar_t *)nexthdrp) {
			error = ECONNREFUSED;
			break;
		}
		break;
	}
	if (error == 0) {
		freemsg(mp);
		return;
	}

	/*
	 * Deliver T_UDERROR_IND when the application has asked for it.
	 * The socket layer enables this automatically when connected.
	 */
	if (!udp->udp_dgram_errind) {
		freemsg(mp);
		return;
	}

	sin6 = sin6_null;
	sin6.sin6_family = AF_INET6;
	sin6.sin6_addr = ip6h->ip6_dst;
	sin6.sin6_port = udpha->uha_dst_port;
	sin6.sin6_flowinfo = ip6h->ip6_vcf & ~IPV6_VERS_AND_FLOW_MASK;

	mp1 = mi_tpi_uderror_ind((char *)&sin6, sizeof (sin6_t), NULL, 0,
	    error);
	if (mp1)
		putnext(q, mp1);
	freemsg(mp);
}

/*
 * This routine responds to T_ADDR_REQ messages.  It is called by udp_wput.
 * The local address is filled in if endpoint is bound. The remote address
 * is filled in if remote address has been precified ("connected endpoint")
 * (The concept of connected CLTS sockets is alien to published TPI
 *  but we support it anyway).
 */
static void
udp_addr_req(queue_t *q, mblk_t *mp)
{
	sin_t	*sin;
	sin6_t	*sin6;
	mblk_t	*ackmp;
	struct T_addr_ack *taa;
	udp_t	*udp = Q_TO_UDP(q);

	/* Make it large enough for worst case */
	ackmp = reallocb(mp, sizeof (struct T_addr_ack) +
	    2 * sizeof (sin6_t), 1);
	if (ackmp == NULL) {
		udp_err_ack(q, mp, TSYSERR, ENOMEM);
		return;
	}
	taa = (struct T_addr_ack *)ackmp->b_rptr;

	bzero(taa, sizeof (struct T_addr_ack));
	ackmp->b_wptr = (uchar_t *)&taa[1];

	taa->PRIM_type = T_ADDR_ACK;
	ackmp->b_datap->db_type = M_PCPROTO;
	rw_enter(&udp->udp_rwlock, RW_READER);
	/*
	 * Note: Following code assumes 32 bit alignment of basic
	 * data structures like sin_t and struct T_addr_ack.
	 */
	if (udp->udp_state != TS_UNBND) {
		/*
		 * Fill in local address first
		 */
		taa->LOCADDR_offset = sizeof (*taa);
		if (udp->udp_family == AF_INET) {
			taa->LOCADDR_length = sizeof (sin_t);
			sin = (sin_t *)&taa[1];
			/* Fill zeroes and then initialize non-zero fields */
			*sin = sin_null;
			sin->sin_family = AF_INET;
			if (!IN6_IS_ADDR_V4MAPPED_ANY(&udp->udp_v6src) &&
			    !IN6_IS_ADDR_UNSPECIFIED(&udp->udp_v6src)) {
				IN6_V4MAPPED_TO_IPADDR(&udp->udp_v6src,
				    sin->sin_addr.s_addr);
			} else {
				/*
				 * INADDR_ANY
				 * udp_v6src is not set, we might be bound to
				 * broadcast/multicast. Use udp_bound_v6src as
				 * local address instead (that could
				 * also still be INADDR_ANY)
				 */
				IN6_V4MAPPED_TO_IPADDR(&udp->udp_bound_v6src,
				    sin->sin_addr.s_addr);
			}
			sin->sin_port = udp->udp_port;
			ackmp->b_wptr = (uchar_t *)&sin[1];
			if (udp->udp_state == TS_DATA_XFER) {
				/*
				 * connected, fill remote address too
				 */
				taa->REMADDR_length = sizeof (sin_t);
				/* assumed 32-bit alignment */
				taa->REMADDR_offset = taa->LOCADDR_offset +
				    taa->LOCADDR_length;

				sin = (sin_t *)(ackmp->b_rptr +
				    taa->REMADDR_offset);
				/* initialize */
				*sin = sin_null;
				sin->sin_family = AF_INET;
				sin->sin_addr.s_addr =
				    V4_PART_OF_V6(udp->udp_v6dst);
				sin->sin_port = udp->udp_dstport;
				ackmp->b_wptr = (uchar_t *)&sin[1];
			}
		} else {
			taa->LOCADDR_length = sizeof (sin6_t);
			sin6 = (sin6_t *)&taa[1];
			/* Fill zeroes and then initialize non-zero fields */
			*sin6 = sin6_null;
			sin6->sin6_family = AF_INET6;
			if (!IN6_IS_ADDR_UNSPECIFIED(&udp->udp_v6src)) {
				sin6->sin6_addr = udp->udp_v6src;
			} else {
				/*
				 * UNSPECIFIED
				 * udp_v6src is not set, we might be bound to
				 * broadcast/multicast. Use udp_bound_v6src as
				 * local address instead (that could
				 * also still be UNSPECIFIED)
				 */
				sin6->sin6_addr =
				    udp->udp_bound_v6src;
			}
			sin6->sin6_port = udp->udp_port;
			ackmp->b_wptr = (uchar_t *)&sin6[1];
			if (udp->udp_state == TS_DATA_XFER) {
				/*
				 * connected, fill remote address too
				 */
				taa->REMADDR_length = sizeof (sin6_t);
				/* assumed 32-bit alignment */
				taa->REMADDR_offset = taa->LOCADDR_offset +
				    taa->LOCADDR_length;

				sin6 = (sin6_t *)(ackmp->b_rptr +
				    taa->REMADDR_offset);
				/* initialize */
				*sin6 = sin6_null;
				sin6->sin6_family = AF_INET6;
				sin6->sin6_addr = udp->udp_v6dst;
				sin6->sin6_port =  udp->udp_dstport;
				ackmp->b_wptr = (uchar_t *)&sin6[1];
			}
			ackmp->b_wptr = (uchar_t *)&sin6[1];
		}
	}
	rw_exit(&udp->udp_rwlock);
	ASSERT(ackmp->b_wptr <= ackmp->b_datap->db_lim);
	qreply(q, ackmp);
}

static void
udp_copy_info(struct T_info_ack *tap, udp_t *udp)
{
	if (udp->udp_family == AF_INET) {
		*tap = udp_g_t_info_ack_ipv4;
	} else {
		*tap = udp_g_t_info_ack_ipv6;
	}
	tap->CURRENT_state = udp->udp_state;
	tap->OPT_size = udp_max_optsize;
}

/*
 * This routine responds to T_CAPABILITY_REQ messages.  It is called by
 * udp_wput.  Much of the T_CAPABILITY_ACK information is copied from
 * udp_g_t_info_ack.  The current state of the stream is copied from
 * udp_state.
 */
static void
udp_capability_req(queue_t *q, mblk_t *mp)
{
	t_uscalar_t		cap_bits1;
	struct T_capability_ack	*tcap;
	udp_t	*udp = Q_TO_UDP(q);

	cap_bits1 = ((struct T_capability_req *)mp->b_rptr)->CAP_bits1;

	mp = tpi_ack_alloc(mp, sizeof (struct T_capability_ack),
	    mp->b_datap->db_type, T_CAPABILITY_ACK);
	if (!mp)
		return;

	tcap = (struct T_capability_ack *)mp->b_rptr;
	tcap->CAP_bits1 = 0;

	if (cap_bits1 & TC1_INFO) {
		udp_copy_info(&tcap->INFO_ack, udp);
		tcap->CAP_bits1 |= TC1_INFO;
	}

	qreply(q, mp);
}

/*
 * This routine responds to T_INFO_REQ messages.  It is called by udp_wput.
 * Most of the T_INFO_ACK information is copied from udp_g_t_info_ack.
 * The current state of the stream is copied from udp_state.
 */
static void
udp_info_req(queue_t *q, mblk_t *mp)
{
	udp_t *udp = Q_TO_UDP(q);

	/* Create a T_INFO_ACK message. */
	mp = tpi_ack_alloc(mp, sizeof (struct T_info_ack), M_PCPROTO,
	    T_INFO_ACK);
	if (!mp)
		return;
	udp_copy_info((struct T_info_ack *)mp->b_rptr, udp);
	qreply(q, mp);
}

/*
 * IP recognizes seven kinds of bind requests:
 *
 * - A zero-length address binds only to the protocol number.
 *
 * - A 4-byte address is treated as a request to
 * validate that the address is a valid local IPv4
 * address, appropriate for an application to bind to.
 * IP does the verification, but does not make any note
 * of the address at this time.
 *
 * - A 16-byte address contains is treated as a request
 * to validate a local IPv6 address, as the 4-byte
 * address case above.
 *
 * - A 16-byte sockaddr_in to validate the local IPv4 address and also
 * use it for the inbound fanout of packets.
 *
 * - A 24-byte sockaddr_in6 to validate the local IPv6 address and also
 * use it for the inbound fanout of packets.
 *
 * - A 12-byte address (ipa_conn_t) containing complete IPv4 fanout
 * information consisting of local and remote addresses
 * and ports.  In this case, the addresses are both
 * validated as appropriate for this operation, and, if
 * so, the information is retained for use in the
 * inbound fanout.
 *
 * - A 36-byte address address (ipa6_conn_t) containing complete IPv6
 * fanout information, like the 12-byte case above.
 *
 * IP will also fill in the IRE request mblk with information
 * regarding our peer.  In all cases, we notify IP of our protocol
 * type by appending a single protocol byte to the bind request.
 */
static mblk_t *
udp_ip_bind_mp(udp_t *udp, t_scalar_t bind_prim, t_scalar_t addr_length)
{
	char	*cp;
	mblk_t	*mp;
	struct T_bind_req *tbr;
	ipa_conn_t	*ac;
	ipa6_conn_t	*ac6;
	sin_t		*sin;
	sin6_t		*sin6;

	ASSERT(bind_prim == O_T_BIND_REQ || bind_prim == T_BIND_REQ);
	ASSERT(RW_LOCK_HELD(&udp->udp_rwlock));
	mp = allocb(sizeof (*tbr) + addr_length + 1, BPRI_HI);
	if (!mp)
		return (mp);
	mp->b_datap->db_type = M_PROTO;
	tbr = (struct T_bind_req *)mp->b_rptr;
	tbr->PRIM_type = bind_prim;
	tbr->ADDR_offset = sizeof (*tbr);
	tbr->CONIND_number = 0;
	tbr->ADDR_length = addr_length;
	cp = (char *)&tbr[1];
	switch (addr_length) {
	case sizeof (ipa_conn_t):
		ASSERT(udp->udp_family == AF_INET);
		/* Append a request for an IRE */
		mp->b_cont = allocb(sizeof (ire_t), BPRI_HI);
		if (!mp->b_cont) {
			freemsg(mp);
			return (NULL);
		}
		mp->b_cont->b_wptr += sizeof (ire_t);
		mp->b_cont->b_datap->db_type = IRE_DB_REQ_TYPE;

		/* cp known to be 32 bit aligned */
		ac = (ipa_conn_t *)cp;
		ac->ac_laddr = V4_PART_OF_V6(udp->udp_v6src);
		ac->ac_faddr = V4_PART_OF_V6(udp->udp_v6dst);
		ac->ac_fport = udp->udp_dstport;
		ac->ac_lport = udp->udp_port;
		break;

	case sizeof (ipa6_conn_t):
		ASSERT(udp->udp_family == AF_INET6);
		/* Append a request for an IRE */
		mp->b_cont = allocb(sizeof (ire_t), BPRI_HI);
		if (!mp->b_cont) {
			freemsg(mp);
			return (NULL);
		}
		mp->b_cont->b_wptr += sizeof (ire_t);
		mp->b_cont->b_datap->db_type = IRE_DB_REQ_TYPE;

		/* cp known to be 32 bit aligned */
		ac6 = (ipa6_conn_t *)cp;
		ac6->ac6_laddr = udp->udp_v6src;
		ac6->ac6_faddr = udp->udp_v6dst;
		ac6->ac6_fport = udp->udp_dstport;
		ac6->ac6_lport = udp->udp_port;
		break;

	case sizeof (sin_t):
		ASSERT(udp->udp_family == AF_INET);
		/* Append a request for an IRE */
		mp->b_cont = allocb(sizeof (ire_t), BPRI_HI);
		if (!mp->b_cont) {
			freemsg(mp);
			return (NULL);
		}
		mp->b_cont->b_wptr += sizeof (ire_t);
		mp->b_cont->b_datap->db_type = IRE_DB_REQ_TYPE;

		sin = (sin_t *)cp;
		*sin = sin_null;
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = V4_PART_OF_V6(udp->udp_bound_v6src);
		sin->sin_port = udp->udp_port;
		break;

	case sizeof (sin6_t):
		ASSERT(udp->udp_family == AF_INET6);
		/* Append a request for an IRE */
		mp->b_cont = allocb(sizeof (ire_t), BPRI_HI);
		if (!mp->b_cont) {
			freemsg(mp);
			return (NULL);
		}
		mp->b_cont->b_wptr += sizeof (ire_t);
		mp->b_cont->b_datap->db_type = IRE_DB_REQ_TYPE;

		sin6 = (sin6_t *)cp;
		*sin6 = sin6_null;
		sin6->sin6_family = AF_INET6;
		sin6->sin6_addr = udp->udp_bound_v6src;
		sin6->sin6_port = udp->udp_port;
		break;
	}
	/* Add protocol number to end */
	cp[addr_length] = (char)IPPROTO_UDP;
	mp->b_wptr = (uchar_t *)&cp[addr_length + 1];
	return (mp);
}

/* For /dev/udp aka AF_INET open */
static int
udp_openv4(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *credp)
{
	return (udp_open(q, devp, flag, sflag, credp, B_FALSE));
}

/* For /dev/udp6 aka AF_INET6 open */
static int
udp_openv6(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *credp)
{
	return (udp_open(q, devp, flag, sflag, credp, B_TRUE));
}

/*
 * This is the open routine for udp.  It allocates a udp_t structure for
 * the stream and, on the first open of the module, creates an ND table.
 */
/*ARGSUSED2*/
static int
udp_open(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *credp,
    boolean_t isv6)
{
	int		err;
	udp_t		*udp;
	conn_t		*connp;
	dev_t		conn_dev;
	zoneid_t	zoneid;
	netstack_t	*ns;
	udp_stack_t	*us;
	vmem_t		*minor_arena;

	TRACE_1(TR_FAC_UDP, TR_UDP_OPEN, "udp_open: q %p", q);

	/* If the stream is already open, return immediately. */
	if (q->q_ptr != NULL)
		return (0);

	if (sflag == MODOPEN)
		return (EINVAL);

	ns = netstack_find_by_cred(credp);
	ASSERT(ns != NULL);
	us = ns->netstack_udp;
	ASSERT(us != NULL);

	/*
	 * For exclusive stacks we set the zoneid to zero
	 * to make UDP operate as if in the global zone.
	 */
	if (ns->netstack_stackid != GLOBAL_NETSTACKID)
		zoneid = GLOBAL_ZONEID;
	else
		zoneid = crgetzoneid(credp);

	if ((ip_minor_arena_la != NULL) && (flag & SO_SOCKSTR) &&
	    ((conn_dev = inet_minor_alloc(ip_minor_arena_la)) != 0)) {
		minor_arena = ip_minor_arena_la;
	} else {
		/*
		 * Either minor numbers in the large arena were exhausted
		 * or a non socket application is doing the open.
		 * Try to allocate from the small arena.
		 */
		if ((conn_dev = inet_minor_alloc(ip_minor_arena_sa)) == 0) {
			netstack_rele(ns);
			return (EBUSY);
		}
		minor_arena = ip_minor_arena_sa;
	}

	*devp = makedevice(getemajor(*devp), (minor_t)conn_dev);

	connp = ipcl_conn_create(IPCL_UDPCONN, KM_SLEEP, ns);
	connp->conn_dev = conn_dev;
	connp->conn_minor_arena = minor_arena;
	udp = connp->conn_udp;

	/*
	 * ipcl_conn_create did a netstack_hold. Undo the hold that was
	 * done by netstack_find_by_cred()
	 */
	netstack_rele(ns);

	/*
	 * Initialize the udp_t structure for this stream.
	 */
	q->q_ptr = connp;
	WR(q)->q_ptr = connp;
	connp->conn_rq = q;
	connp->conn_wq = WR(q);

	rw_enter(&udp->udp_rwlock, RW_WRITER);
	ASSERT(connp->conn_ulp == IPPROTO_UDP);
	ASSERT(connp->conn_udp == udp);
	ASSERT(udp->udp_connp == connp);

	/* Set the initial state of the stream and the privilege status. */
	udp->udp_state = TS_UNBND;
	if (isv6) {
		udp->udp_family = AF_INET6;
		udp->udp_ipversion = IPV6_VERSION;
		udp->udp_max_hdr_len = IPV6_HDR_LEN + UDPH_SIZE;
		udp->udp_ttl = us->us_ipv6_hoplimit;
		connp->conn_af_isv6 = B_TRUE;
		connp->conn_flags |= IPCL_ISV6;
	} else {
		udp->udp_family = AF_INET;
		udp->udp_ipversion = IPV4_VERSION;
		udp->udp_max_hdr_len = IP_SIMPLE_HDR_LENGTH + UDPH_SIZE;
		udp->udp_ttl = us->us_ipv4_ttl;
		connp->conn_af_isv6 = B_FALSE;
		connp->conn_flags &= ~IPCL_ISV6;
	}

	udp->udp_multicast_ttl = IP_DEFAULT_MULTICAST_TTL;
	udp->udp_pending_op = -1;
	connp->conn_multicast_loop = IP_DEFAULT_MULTICAST_LOOP;
	connp->conn_zoneid = zoneid;

	udp->udp_open_time = lbolt64;
	udp->udp_open_pid = curproc->p_pid;

	/*
	 * If the caller has the process-wide flag set, then default to MAC
	 * exempt mode.  This allows read-down to unlabeled hosts.
	 */
	if (getpflags(NET_MAC_AWARE, credp) != 0)
		connp->conn_mac_exempt = B_TRUE;

	if (flag & SO_SOCKSTR) {
		connp->conn_flags |= IPCL_SOCKET;
		udp->udp_issocket = B_TRUE;
		udp->udp_direct_sockfs = B_TRUE;
	}

	connp->conn_ulp_labeled = is_system_labeled();

	udp->udp_us = us;

	q->q_hiwat = us->us_recv_hiwat;
	WR(q)->q_hiwat = us->us_xmit_hiwat;
	WR(q)->q_lowat = us->us_xmit_lowat;

	connp->conn_recv = udp_input;
	crhold(credp);
	connp->conn_cred = credp;

	mutex_enter(&connp->conn_lock);
	connp->conn_state_flags &= ~CONN_INCIPIENT;
	mutex_exit(&connp->conn_lock);

	qprocson(q);

	if (udp->udp_family == AF_INET6) {
		/* Build initial header template for transmit */
		if ((err = udp_build_hdrs(udp)) != 0) {
			rw_exit(&udp->udp_rwlock);
			qprocsoff(q);
			ipcl_conn_destroy(connp);
			return (err);
		}
	}
	rw_exit(&udp->udp_rwlock);

	/* Set the Stream head write offset and high watermark. */
	(void) mi_set_sth_wroff(q,
	    udp->udp_max_hdr_len + us->us_wroff_extra);
	(void) mi_set_sth_hiwat(q, udp_set_rcv_hiwat(udp, q->q_hiwat));

	return (0);
}

/*
 * Which UDP options OK to set through T_UNITDATA_REQ...
 */
/* ARGSUSED */
static boolean_t
udp_opt_allow_udr_set(t_scalar_t level, t_scalar_t name)
{
	return (B_TRUE);
}

/*
 * This routine gets default values of certain options whose default
 * values are maintained by protcol specific code
 */
/* ARGSUSED */
int
udp_opt_default(queue_t *q, t_scalar_t level, t_scalar_t name, uchar_t *ptr)
{
	udp_t		*udp = Q_TO_UDP(q);
	udp_stack_t *us = udp->udp_us;
	int *i1 = (int *)ptr;

	switch (level) {
	case IPPROTO_IP:
		switch (name) {
		case IP_MULTICAST_TTL:
			*ptr = (uchar_t)IP_DEFAULT_MULTICAST_TTL;
			return (sizeof (uchar_t));
		case IP_MULTICAST_LOOP:
			*ptr = (uchar_t)IP_DEFAULT_MULTICAST_LOOP;
			return (sizeof (uchar_t));
		}
		break;
	case IPPROTO_IPV6:
		switch (name) {
		case IPV6_MULTICAST_HOPS:
			*i1 = IP_DEFAULT_MULTICAST_TTL;
			return (sizeof (int));
		case IPV6_MULTICAST_LOOP:
			*i1 = IP_DEFAULT_MULTICAST_LOOP;
			return (sizeof (int));
		case IPV6_UNICAST_HOPS:
			*i1 = us->us_ipv6_hoplimit;
			return (sizeof (int));
		}
		break;
	}
	return (-1);
}

/*
 * This routine retrieves the current status of socket options.
 * It returns the size of the option retrieved.
 */
int
udp_opt_get_locked(queue_t *q, t_scalar_t level, t_scalar_t name, uchar_t *ptr)
{
	int	*i1 = (int *)ptr;
	conn_t	*connp;
	udp_t	*udp;
	ip6_pkt_t *ipp;
	int	len;
	udp_stack_t	*us;

	connp = Q_TO_CONN(q);
	udp = connp->conn_udp;
	ipp = &udp->udp_sticky_ipp;
	us = udp->udp_us;

	switch (level) {
	case SOL_SOCKET:
		switch (name) {
		case SO_DEBUG:
			*i1 = udp->udp_debug;
			break;	/* goto sizeof (int) option return */
		case SO_REUSEADDR:
			*i1 = udp->udp_reuseaddr;
			break;	/* goto sizeof (int) option return */
		case SO_TYPE:
			*i1 = SOCK_DGRAM;
			break;	/* goto sizeof (int) option return */

		/*
		 * The following three items are available here,
		 * but are only meaningful to IP.
		 */
		case SO_DONTROUTE:
			*i1 = udp->udp_dontroute;
			break;	/* goto sizeof (int) option return */
		case SO_USELOOPBACK:
			*i1 = udp->udp_useloopback;
			break;	/* goto sizeof (int) option return */
		case SO_BROADCAST:
			*i1 = udp->udp_broadcast;
			break;	/* goto sizeof (int) option return */

		case SO_SNDBUF:
			*i1 = q->q_hiwat;
			break;	/* goto sizeof (int) option return */
		case SO_RCVBUF:
			*i1 = RD(q)->q_hiwat;
			break;	/* goto sizeof (int) option return */
		case SO_DGRAM_ERRIND:
			*i1 = udp->udp_dgram_errind;
			break;	/* goto sizeof (int) option return */
		case SO_RECVUCRED:
			*i1 = udp->udp_recvucred;
			break;	/* goto sizeof (int) option return */
		case SO_TIMESTAMP:
			*i1 = udp->udp_timestamp;
			break;	/* goto sizeof (int) option return */
		case SO_ANON_MLP:
			*i1 = connp->conn_anon_mlp;
			break;	/* goto sizeof (int) option return */
		case SO_MAC_EXEMPT:
			*i1 = connp->conn_mac_exempt;
			break;	/* goto sizeof (int) option return */
		case SO_ALLZONES:
			*i1 = connp->conn_allzones;
			break;	/* goto sizeof (int) option return */
		case SO_EXCLBIND:
			*i1 = udp->udp_exclbind ? SO_EXCLBIND : 0;
			break;
		case SO_PROTOTYPE:
			*i1 = IPPROTO_UDP;
			break;
		case SO_DOMAIN:
			*i1 = udp->udp_family;
			break;
		default:
			return (-1);
		}
		break;
	case IPPROTO_IP:
		if (udp->udp_family != AF_INET)
			return (-1);
		switch (name) {
		case IP_OPTIONS:
		case T_IP_OPTIONS:
			len = udp->udp_ip_rcv_options_len - udp->udp_label_len;
			if (len > 0) {
				bcopy(udp->udp_ip_rcv_options +
				    udp->udp_label_len, ptr, len);
			}
			return (len);
		case IP_TOS:
		case T_IP_TOS:
			*i1 = (int)udp->udp_type_of_service;
			break;	/* goto sizeof (int) option return */
		case IP_TTL:
			*i1 = (int)udp->udp_ttl;
			break;	/* goto sizeof (int) option return */
		case IP_DHCPINIT_IF:
			return (-EINVAL);
		case IP_NEXTHOP:
		case IP_RECVPKTINFO:
			/*
			 * This also handles IP_PKTINFO.
			 * IP_PKTINFO and IP_RECVPKTINFO have the same value.
			 * Differentiation is based on the size of the argument
			 * passed in.
			 * This option is handled in IP which will return an
			 * error for IP_PKTINFO as it's not supported as a
			 * sticky option.
			 */
			return (-EINVAL);
		case IP_MULTICAST_IF:
			/* 0 address if not set */
			*(ipaddr_t *)ptr = udp->udp_multicast_if_addr;
			return (sizeof (ipaddr_t));
		case IP_MULTICAST_TTL:
			*(uchar_t *)ptr = udp->udp_multicast_ttl;
			return (sizeof (uchar_t));
		case IP_MULTICAST_LOOP:
			*ptr = connp->conn_multicast_loop;
			return (sizeof (uint8_t));
		case IP_RECVOPTS:
			*i1 = udp->udp_recvopts;
			break;	/* goto sizeof (int) option return */
		case IP_RECVDSTADDR:
			*i1 = udp->udp_recvdstaddr;
			break;	/* goto sizeof (int) option return */
		case IP_RECVIF:
			*i1 = udp->udp_recvif;
			break;	/* goto sizeof (int) option return */
		case IP_RECVSLLA:
			*i1 = udp->udp_recvslla;
			break;	/* goto sizeof (int) option return */
		case IP_RECVTTL:
			*i1 = udp->udp_recvttl;
			break;	/* goto sizeof (int) option return */
		case IP_ADD_MEMBERSHIP:
		case IP_DROP_MEMBERSHIP:
		case IP_BLOCK_SOURCE:
		case IP_UNBLOCK_SOURCE:
		case IP_ADD_SOURCE_MEMBERSHIP:
		case IP_DROP_SOURCE_MEMBERSHIP:
		case MCAST_JOIN_GROUP:
		case MCAST_LEAVE_GROUP:
		case MCAST_BLOCK_SOURCE:
		case MCAST_UNBLOCK_SOURCE:
		case MCAST_JOIN_SOURCE_GROUP:
		case MCAST_LEAVE_SOURCE_GROUP:
		case IP_DONTFAILOVER_IF:
			/* cannot "get" the value for these */
			return (-1);
		case IP_BOUND_IF:
			/* Zero if not set */
			*i1 = udp->udp_bound_if;
			break;	/* goto sizeof (int) option return */
		case IP_UNSPEC_SRC:
			*i1 = udp->udp_unspec_source;
			break;	/* goto sizeof (int) option return */
		case IP_BROADCAST_TTL:
			*(uchar_t *)ptr = connp->conn_broadcast_ttl;
			return (sizeof (uchar_t));
		default:
			return (-1);
		}
		break;
	case IPPROTO_IPV6:
		if (udp->udp_family != AF_INET6)
			return (-1);
		switch (name) {
		case IPV6_UNICAST_HOPS:
			*i1 = (unsigned int)udp->udp_ttl;
			break;	/* goto sizeof (int) option return */
		case IPV6_MULTICAST_IF:
			/* 0 index if not set */
			*i1 = udp->udp_multicast_if_index;
			break;	/* goto sizeof (int) option return */
		case IPV6_MULTICAST_HOPS:
			*i1 = udp->udp_multicast_ttl;
			break;	/* goto sizeof (int) option return */
		case IPV6_MULTICAST_LOOP:
			*i1 = connp->conn_multicast_loop;
			break;	/* goto sizeof (int) option return */
		case IPV6_JOIN_GROUP:
		case IPV6_LEAVE_GROUP:
		case MCAST_JOIN_GROUP:
		case MCAST_LEAVE_GROUP:
		case MCAST_BLOCK_SOURCE:
		case MCAST_UNBLOCK_SOURCE:
		case MCAST_JOIN_SOURCE_GROUP:
		case MCAST_LEAVE_SOURCE_GROUP:
			/* cannot "get" the value for these */
			return (-1);
		case IPV6_BOUND_IF:
			/* Zero if not set */
			*i1 = udp->udp_bound_if;
			break;	/* goto sizeof (int) option return */
		case IPV6_UNSPEC_SRC:
			*i1 = udp->udp_unspec_source;
			break;	/* goto sizeof (int) option return */
		case IPV6_RECVPKTINFO:
			*i1 = udp->udp_ip_recvpktinfo;
			break;	/* goto sizeof (int) option return */
		case IPV6_RECVTCLASS:
			*i1 = udp->udp_ipv6_recvtclass;
			break;	/* goto sizeof (int) option return */
		case IPV6_RECVPATHMTU:
			*i1 = udp->udp_ipv6_recvpathmtu;
			break;	/* goto sizeof (int) option return */
		case IPV6_RECVHOPLIMIT:
			*i1 = udp->udp_ipv6_recvhoplimit;
			break;	/* goto sizeof (int) option return */
		case IPV6_RECVHOPOPTS:
			*i1 = udp->udp_ipv6_recvhopopts;
			break;	/* goto sizeof (int) option return */
		case IPV6_RECVDSTOPTS:
			*i1 = udp->udp_ipv6_recvdstopts;
			break;	/* goto sizeof (int) option return */
		case _OLD_IPV6_RECVDSTOPTS:
			*i1 = udp->udp_old_ipv6_recvdstopts;
			break;	/* goto sizeof (int) option return */
		case IPV6_RECVRTHDRDSTOPTS:
			*i1 = udp->udp_ipv6_recvrthdrdstopts;
			break;	/* goto sizeof (int) option return */
		case IPV6_RECVRTHDR:
			*i1 = udp->udp_ipv6_recvrthdr;
			break;	/* goto sizeof (int) option return */
		case IPV6_PKTINFO: {
			/* XXX assumes that caller has room for max size! */
			struct in6_pktinfo *pkti;

			pkti = (struct in6_pktinfo *)ptr;
			if (ipp->ipp_fields & IPPF_IFINDEX)
				pkti->ipi6_ifindex = ipp->ipp_ifindex;
			else
				pkti->ipi6_ifindex = 0;
			if (ipp->ipp_fields & IPPF_ADDR)
				pkti->ipi6_addr = ipp->ipp_addr;
			else
				pkti->ipi6_addr = ipv6_all_zeros;
			return (sizeof (struct in6_pktinfo));
		}
		case IPV6_TCLASS:
			if (ipp->ipp_fields & IPPF_TCLASS)
				*i1 = ipp->ipp_tclass;
			else
				*i1 = IPV6_FLOW_TCLASS(
				    IPV6_DEFAULT_VERS_AND_FLOW);
			break;	/* goto sizeof (int) option return */
		case IPV6_NEXTHOP: {
			sin6_t *sin6 = (sin6_t *)ptr;

			if (!(ipp->ipp_fields & IPPF_NEXTHOP))
				return (0);
			*sin6 = sin6_null;
			sin6->sin6_family = AF_INET6;
			sin6->sin6_addr = ipp->ipp_nexthop;
			return (sizeof (sin6_t));
		}
		case IPV6_HOPOPTS:
			if (!(ipp->ipp_fields & IPPF_HOPOPTS))
				return (0);
			if (ipp->ipp_hopoptslen <= udp->udp_label_len_v6)
				return (0);
			/*
			 * The cipso/label option is added by kernel.
			 * User is not usually aware of this option.
			 * We copy out the hbh opt after the label option.
			 */
			bcopy((char *)ipp->ipp_hopopts + udp->udp_label_len_v6,
			    ptr, ipp->ipp_hopoptslen - udp->udp_label_len_v6);
			if (udp->udp_label_len_v6 > 0) {
				ptr[0] = ((char *)ipp->ipp_hopopts)[0];
				ptr[1] = (ipp->ipp_hopoptslen -
				    udp->udp_label_len_v6 + 7) / 8 - 1;
			}
			return (ipp->ipp_hopoptslen - udp->udp_label_len_v6);
		case IPV6_RTHDRDSTOPTS:
			if (!(ipp->ipp_fields & IPPF_RTDSTOPTS))
				return (0);
			bcopy(ipp->ipp_rtdstopts, ptr, ipp->ipp_rtdstoptslen);
			return (ipp->ipp_rtdstoptslen);
		case IPV6_RTHDR:
			if (!(ipp->ipp_fields & IPPF_RTHDR))
				return (0);
			bcopy(ipp->ipp_rthdr, ptr, ipp->ipp_rthdrlen);
			return (ipp->ipp_rthdrlen);
		case IPV6_DSTOPTS:
			if (!(ipp->ipp_fields & IPPF_DSTOPTS))
				return (0);
			bcopy(ipp->ipp_dstopts, ptr, ipp->ipp_dstoptslen);
			return (ipp->ipp_dstoptslen);
		case IPV6_PATHMTU:
			return (ip_fill_mtuinfo(&udp->udp_v6dst,
			    udp->udp_dstport, (struct ip6_mtuinfo *)ptr,
			    us->us_netstack));
		default:
			return (-1);
		}
		break;
	case IPPROTO_UDP:
		switch (name) {
		case UDP_ANONPRIVBIND:
			*i1 = udp->udp_anon_priv_bind;
			break;
		case UDP_EXCLBIND:
			*i1 = udp->udp_exclbind ? UDP_EXCLBIND : 0;
			break;
		case UDP_RCVHDR:
			*i1 = udp->udp_rcvhdr ? 1 : 0;
			break;
		case UDP_NAT_T_ENDPOINT:
			*i1 = udp->udp_nat_t_endpoint;
			break;
		default:
			return (-1);
		}
		break;
	default:
		return (-1);
	}
	return (sizeof (int));
}

int
udp_opt_get(queue_t *q, t_scalar_t level, t_scalar_t name, uchar_t *ptr)
{
	udp_t	*udp;
	int	err;

	udp = Q_TO_UDP(q);

	rw_enter(&udp->udp_rwlock, RW_READER);
	err = udp_opt_get_locked(q, level, name, ptr);
	rw_exit(&udp->udp_rwlock);
	return (err);
}

/*
 * This routine sets socket options.
 */
/* ARGSUSED */
int
udp_opt_set_locked(queue_t *q, uint_t optset_context, int level,
    int name, uint_t inlen, uchar_t *invalp, uint_t *outlenp,
    uchar_t *outvalp, void *thisdg_attrs, cred_t *cr, mblk_t *mblk)
{
	udpattrs_t *attrs = thisdg_attrs;
	int	*i1 = (int *)invalp;
	boolean_t onoff = (*i1 == 0) ? 0 : 1;
	boolean_t checkonly;
	int	error;
	conn_t	*connp;
	udp_t	*udp;
	uint_t	newlen;
	udp_stack_t *us;
	size_t	sth_wroff;

	connp = Q_TO_CONN(q);
	udp = connp->conn_udp;
	us = udp->udp_us;

	switch (optset_context) {
	case SETFN_OPTCOM_CHECKONLY:
		checkonly = B_TRUE;
		/*
		 * Note: Implies T_CHECK semantics for T_OPTCOM_REQ
		 * inlen != 0 implies value supplied and
		 * 	we have to "pretend" to set it.
		 * inlen == 0 implies that there is no
		 * 	value part in T_CHECK request and just validation
		 * done elsewhere should be enough, we just return here.
		 */
		if (inlen == 0) {
			*outlenp = 0;
			return (0);
		}
		break;
	case SETFN_OPTCOM_NEGOTIATE:
		checkonly = B_FALSE;
		break;
	case SETFN_UD_NEGOTIATE:
	case SETFN_CONN_NEGOTIATE:
		checkonly = B_FALSE;
		/*
		 * Negotiating local and "association-related" options
		 * through T_UNITDATA_REQ.
		 *
		 * Following routine can filter out ones we do not
		 * want to be "set" this way.
		 */
		if (!udp_opt_allow_udr_set(level, name)) {
			*outlenp = 0;
			return (EINVAL);
		}
		break;
	default:
		/*
		 * We should never get here
		 */
		*outlenp = 0;
		return (EINVAL);
	}

	ASSERT((optset_context != SETFN_OPTCOM_CHECKONLY) ||
	    (optset_context == SETFN_OPTCOM_CHECKONLY && inlen != 0));

	/*
	 * For fixed length options, no sanity check
	 * of passed in length is done. It is assumed *_optcom_req()
	 * routines do the right thing.
	 */

	switch (level) {
	case SOL_SOCKET:
		switch (name) {
		case SO_REUSEADDR:
			if (!checkonly)
				udp->udp_reuseaddr = onoff;
			break;
		case SO_DEBUG:
			if (!checkonly)
				udp->udp_debug = onoff;
			break;
		/*
		 * The following three items are available here,
		 * but are only meaningful to IP.
		 */
		case SO_DONTROUTE:
			if (!checkonly)
				udp->udp_dontroute = onoff;
			break;
		case SO_USELOOPBACK:
			if (!checkonly)
				udp->udp_useloopback = onoff;
			break;
		case SO_BROADCAST:
			if (!checkonly)
				udp->udp_broadcast = onoff;
			break;

		case SO_SNDBUF:
			if (*i1 > us->us_max_buf) {
				*outlenp = 0;
				return (ENOBUFS);
			}
			if (!checkonly) {
				q->q_hiwat = *i1;
			}
			break;
		case SO_RCVBUF:
			if (*i1 > us->us_max_buf) {
				*outlenp = 0;
				return (ENOBUFS);
			}
			if (!checkonly) {
				RD(q)->q_hiwat = *i1;
				rw_exit(&udp->udp_rwlock);
				(void) mi_set_sth_hiwat(RD(q),
				    udp_set_rcv_hiwat(udp, *i1));
				rw_enter(&udp->udp_rwlock, RW_WRITER);
			}
			break;
		case SO_DGRAM_ERRIND:
			if (!checkonly)
				udp->udp_dgram_errind = onoff;
			break;
		case SO_RECVUCRED:
			if (!checkonly)
				udp->udp_recvucred = onoff;
			break;
		case SO_ALLZONES:
			/*
			 * "soft" error (negative)
			 * option not handled at this level
			 * Do not modify *outlenp.
			 */
			return (-EINVAL);
		case SO_TIMESTAMP:
			if (!checkonly)
				udp->udp_timestamp = onoff;
			break;
		case SO_ANON_MLP:
			/* Pass option along to IP level for handling */
			return (-EINVAL);
		case SO_MAC_EXEMPT:
			/* Pass option along to IP level for handling */
			return (-EINVAL);
		case SCM_UCRED: {
			struct ucred_s *ucr;
			cred_t *cr, *newcr;
			ts_label_t *tsl;

			/*
			 * Only sockets that have proper privileges and are
			 * bound to MLPs will have any other value here, so
			 * this implicitly tests for privilege to set label.
			 */
			if (connp->conn_mlp_type == mlptSingle)
				break;
			ucr = (struct ucred_s *)invalp;
			if (inlen != ucredsize ||
			    ucr->uc_labeloff < sizeof (*ucr) ||
			    ucr->uc_labeloff + sizeof (bslabel_t) > inlen)
				return (EINVAL);
			if (!checkonly) {
				mblk_t *mb;

				if (attrs == NULL ||
				    (mb = attrs->udpattr_mb) == NULL)
					return (EINVAL);
				if ((cr = DB_CRED(mb)) == NULL)
					cr = udp->udp_connp->conn_cred;
				ASSERT(cr != NULL);
				if ((tsl = crgetlabel(cr)) == NULL)
					return (EINVAL);
				newcr = copycred_from_bslabel(cr, UCLABEL(ucr),
				    tsl->tsl_doi, KM_NOSLEEP);
				if (newcr == NULL)
					return (ENOSR);
				mblk_setcred(mb, newcr);
				attrs->udpattr_credset = B_TRUE;
				crfree(newcr);
			}
			break;
		}
		case SO_EXCLBIND:
			if (!checkonly)
				udp->udp_exclbind = onoff;
			break;
		default:
			*outlenp = 0;
			return (EINVAL);
		}
		break;
	case IPPROTO_IP:
		if (udp->udp_family != AF_INET) {
			*outlenp = 0;
			return (ENOPROTOOPT);
		}
		switch (name) {
		case IP_OPTIONS:
		case T_IP_OPTIONS:
			/* Save options for use by IP. */
			newlen = inlen + udp->udp_label_len;
			if ((inlen & 0x3) || newlen > IP_MAX_OPT_LENGTH) {
				*outlenp = 0;
				return (EINVAL);
			}
			if (checkonly)
				break;

			/*
			 * Update the stored options taking into account
			 * any CIPSO option which we should not overwrite.
			 */
			if (!tsol_option_set(&udp->udp_ip_snd_options,
			    &udp->udp_ip_snd_options_len,
			    udp->udp_label_len, invalp, inlen)) {
				*outlenp = 0;
				return (ENOMEM);
			}

			udp->udp_max_hdr_len = IP_SIMPLE_HDR_LENGTH +
			    UDPH_SIZE + udp->udp_ip_snd_options_len;
			sth_wroff = udp->udp_max_hdr_len + us->us_wroff_extra;
			rw_exit(&udp->udp_rwlock);
			(void) mi_set_sth_wroff(RD(q), sth_wroff);
			rw_enter(&udp->udp_rwlock, RW_WRITER);
			break;

		case IP_TTL:
			if (!checkonly) {
				udp->udp_ttl = (uchar_t)*i1;
			}
			break;
		case IP_TOS:
		case T_IP_TOS:
			if (!checkonly) {
				udp->udp_type_of_service = (uchar_t)*i1;
			}
			break;
		case IP_MULTICAST_IF: {
			/*
			 * TODO should check OPTMGMT reply and undo this if
			 * there is an error.
			 */
			struct in_addr *inap = (struct in_addr *)invalp;
			if (!checkonly) {
				udp->udp_multicast_if_addr =
				    inap->s_addr;
			}
			break;
		}
		case IP_MULTICAST_TTL:
			if (!checkonly)
				udp->udp_multicast_ttl = *invalp;
			break;
		case IP_MULTICAST_LOOP:
			if (!checkonly)
				connp->conn_multicast_loop = *invalp;
			break;
		case IP_RECVOPTS:
			if (!checkonly)
				udp->udp_recvopts = onoff;
			break;
		case IP_RECVDSTADDR:
			if (!checkonly)
				udp->udp_recvdstaddr = onoff;
			break;
		case IP_RECVIF:
			if (!checkonly)
				udp->udp_recvif = onoff;
			break;
		case IP_RECVSLLA:
			if (!checkonly)
				udp->udp_recvslla = onoff;
			break;
		case IP_RECVTTL:
			if (!checkonly)
				udp->udp_recvttl = onoff;
			break;
		case IP_PKTINFO: {
			/*
			 * This also handles IP_RECVPKTINFO.
			 * IP_PKTINFO and IP_RECVPKTINFO have same value.
			 * Differentiation is based on the size of the
			 * argument passed in.
			 */
			struct in_pktinfo *pktinfop;
			ip4_pkt_t *attr_pktinfop;

			if (checkonly)
				break;

			if (inlen == sizeof (int)) {
				/*
				 * This is IP_RECVPKTINFO option.
				 * Keep a local copy of whether this option is
				 * set or not and pass it down to IP for
				 * processing.
				 */

				udp->udp_ip_recvpktinfo = onoff;
				return (-EINVAL);
			}

			if (attrs == NULL ||
			    (attr_pktinfop = attrs->udpattr_ipp4) == NULL) {
				/*
				 * sticky option or no buffer to return
				 * the results.
				 */
				return (EINVAL);
			}

			if (inlen != sizeof (struct in_pktinfo))
				return (EINVAL);

			pktinfop = (struct in_pktinfo *)invalp;

			/*
			 * At least one of the values should be specified
			 */
			if (pktinfop->ipi_ifindex == 0 &&
			    pktinfop->ipi_spec_dst.s_addr == INADDR_ANY) {
				return (EINVAL);
			}

			attr_pktinfop->ip4_addr = pktinfop->ipi_spec_dst.s_addr;
			attr_pktinfop->ip4_ill_index = pktinfop->ipi_ifindex;

			break;
		}
		case IP_ADD_MEMBERSHIP:
		case IP_DROP_MEMBERSHIP:
		case IP_BLOCK_SOURCE:
		case IP_UNBLOCK_SOURCE:
		case IP_ADD_SOURCE_MEMBERSHIP:
		case IP_DROP_SOURCE_MEMBERSHIP:
		case MCAST_JOIN_GROUP:
		case MCAST_LEAVE_GROUP:
		case MCAST_BLOCK_SOURCE:
		case MCAST_UNBLOCK_SOURCE:
		case MCAST_JOIN_SOURCE_GROUP:
		case MCAST_LEAVE_SOURCE_GROUP:
		case IP_SEC_OPT:
		case IP_NEXTHOP:
		case IP_DHCPINIT_IF:
			/*
			 * "soft" error (negative)
			 * option not handled at this level
			 * Do not modify *outlenp.
			 */
			return (-EINVAL);
		case IP_BOUND_IF:
			if (!checkonly)
				udp->udp_bound_if = *i1;
			break;
		case IP_UNSPEC_SRC:
			if (!checkonly)
				udp->udp_unspec_source = onoff;
			break;
		case IP_BROADCAST_TTL:
			if (!checkonly)
				connp->conn_broadcast_ttl = *invalp;
			break;
		default:
			*outlenp = 0;
			return (EINVAL);
		}
		break;
	case IPPROTO_IPV6: {
		ip6_pkt_t		*ipp;
		boolean_t		sticky;

		if (udp->udp_family != AF_INET6) {
			*outlenp = 0;
			return (ENOPROTOOPT);
		}
		/*
		 * Deal with both sticky options and ancillary data
		 */
		sticky = B_FALSE;
		if (attrs == NULL || (ipp = attrs->udpattr_ipp6) ==
		    NULL) {
			/* sticky options, or none */
			ipp = &udp->udp_sticky_ipp;
			sticky = B_TRUE;
		}

		switch (name) {
		case IPV6_MULTICAST_IF:
			if (!checkonly)
				udp->udp_multicast_if_index = *i1;
			break;
		case IPV6_UNICAST_HOPS:
			/* -1 means use default */
			if (*i1 < -1 || *i1 > IPV6_MAX_HOPS) {
				*outlenp = 0;
				return (EINVAL);
			}
			if (!checkonly) {
				if (*i1 == -1) {
					udp->udp_ttl = ipp->ipp_unicast_hops =
					    us->us_ipv6_hoplimit;
					ipp->ipp_fields &= ~IPPF_UNICAST_HOPS;
					/* Pass modified value to IP. */
					*i1 = udp->udp_ttl;
				} else {
					udp->udp_ttl = ipp->ipp_unicast_hops =
					    (uint8_t)*i1;
					ipp->ipp_fields |= IPPF_UNICAST_HOPS;
				}
				/* Rebuild the header template */
				error = udp_build_hdrs(udp);
				if (error != 0) {
					*outlenp = 0;
					return (error);
				}
			}
			break;
		case IPV6_MULTICAST_HOPS:
			/* -1 means use default */
			if (*i1 < -1 || *i1 > IPV6_MAX_HOPS) {
				*outlenp = 0;
				return (EINVAL);
			}
			if (!checkonly) {
				if (*i1 == -1) {
					udp->udp_multicast_ttl =
					    ipp->ipp_multicast_hops =
					    IP_DEFAULT_MULTICAST_TTL;
					ipp->ipp_fields &= ~IPPF_MULTICAST_HOPS;
					/* Pass modified value to IP. */
					*i1 = udp->udp_multicast_ttl;
				} else {
					udp->udp_multicast_ttl =
					    ipp->ipp_multicast_hops =
					    (uint8_t)*i1;
					ipp->ipp_fields |= IPPF_MULTICAST_HOPS;
				}
			}
			break;
		case IPV6_MULTICAST_LOOP:
			if (*i1 != 0 && *i1 != 1) {
				*outlenp = 0;
				return (EINVAL);
			}
			if (!checkonly)
				connp->conn_multicast_loop = *i1;
			break;
		case IPV6_JOIN_GROUP:
		case IPV6_LEAVE_GROUP:
		case MCAST_JOIN_GROUP:
		case MCAST_LEAVE_GROUP:
		case MCAST_BLOCK_SOURCE:
		case MCAST_UNBLOCK_SOURCE:
		case MCAST_JOIN_SOURCE_GROUP:
		case MCAST_LEAVE_SOURCE_GROUP:
			/*
			 * "soft" error (negative)
			 * option not handled at this level
			 * Note: Do not modify *outlenp
			 */
			return (-EINVAL);
		case IPV6_BOUND_IF:
			if (!checkonly)
				udp->udp_bound_if = *i1;
			break;
		case IPV6_UNSPEC_SRC:
			if (!checkonly)
				udp->udp_unspec_source = onoff;
			break;
		/*
		 * Set boolean switches for ancillary data delivery
		 */
		case IPV6_RECVPKTINFO:
			if (!checkonly)
				udp->udp_ip_recvpktinfo = onoff;
			break;
		case IPV6_RECVTCLASS:
			if (!checkonly) {
				udp->udp_ipv6_recvtclass = onoff;
			}
			break;
		case IPV6_RECVPATHMTU:
			if (!checkonly) {
				udp->udp_ipv6_recvpathmtu = onoff;
			}
			break;
		case IPV6_RECVHOPLIMIT:
			if (!checkonly)
				udp->udp_ipv6_recvhoplimit = onoff;
			break;
		case IPV6_RECVHOPOPTS:
			if (!checkonly)
				udp->udp_ipv6_recvhopopts = onoff;
			break;
		case IPV6_RECVDSTOPTS:
			if (!checkonly)
				udp->udp_ipv6_recvdstopts = onoff;
			break;
		case _OLD_IPV6_RECVDSTOPTS:
			if (!checkonly)
				udp->udp_old_ipv6_recvdstopts = onoff;
			break;
		case IPV6_RECVRTHDRDSTOPTS:
			if (!checkonly)
				udp->udp_ipv6_recvrthdrdstopts = onoff;
			break;
		case IPV6_RECVRTHDR:
			if (!checkonly)
				udp->udp_ipv6_recvrthdr = onoff;
			break;
		/*
		 * Set sticky options or ancillary data.
		 * If sticky options, (re)build any extension headers
		 * that might be needed as a result.
		 */
		case IPV6_PKTINFO:
			/*
			 * The source address and ifindex are verified
			 * in ip_opt_set(). For ancillary data the
			 * source address is checked in ip_wput_v6.
			 */
			if (inlen != 0 && inlen != sizeof (struct in6_pktinfo))
				return (EINVAL);
			if (checkonly)
				break;

			if (inlen == 0) {
				ipp->ipp_fields &= ~(IPPF_IFINDEX|IPPF_ADDR);
				ipp->ipp_sticky_ignored |=
				    (IPPF_IFINDEX|IPPF_ADDR);
			} else {
				struct in6_pktinfo *pkti;

				pkti = (struct in6_pktinfo *)invalp;
				ipp->ipp_ifindex = pkti->ipi6_ifindex;
				ipp->ipp_addr = pkti->ipi6_addr;
				if (ipp->ipp_ifindex != 0)
					ipp->ipp_fields |= IPPF_IFINDEX;
				else
					ipp->ipp_fields &= ~IPPF_IFINDEX;
				if (!IN6_IS_ADDR_UNSPECIFIED(
				    &ipp->ipp_addr))
					ipp->ipp_fields |= IPPF_ADDR;
				else
					ipp->ipp_fields &= ~IPPF_ADDR;
			}
			if (sticky) {
				error = udp_build_hdrs(udp);
				if (error != 0)
					return (error);
			}
			break;
		case IPV6_HOPLIMIT:
			if (sticky)
				return (EINVAL);
			if (inlen != 0 && inlen != sizeof (int))
				return (EINVAL);
			if (checkonly)
				break;

			if (inlen == 0) {
				ipp->ipp_fields &= ~IPPF_HOPLIMIT;
				ipp->ipp_sticky_ignored |= IPPF_HOPLIMIT;
			} else {
				if (*i1 > 255 || *i1 < -1)
					return (EINVAL);
				if (*i1 == -1)
					ipp->ipp_hoplimit =
					    us->us_ipv6_hoplimit;
				else
					ipp->ipp_hoplimit = *i1;
				ipp->ipp_fields |= IPPF_HOPLIMIT;
			}
			break;
		case IPV6_TCLASS:
			if (inlen != 0 && inlen != sizeof (int))
				return (EINVAL);
			if (checkonly)
				break;

			if (inlen == 0) {
				ipp->ipp_fields &= ~IPPF_TCLASS;
				ipp->ipp_sticky_ignored |= IPPF_TCLASS;
			} else {
				if (*i1 > 255 || *i1 < -1)
					return (EINVAL);
				if (*i1 == -1)
					ipp->ipp_tclass = 0;
				else
					ipp->ipp_tclass = *i1;
				ipp->ipp_fields |= IPPF_TCLASS;
			}
			if (sticky) {
				error = udp_build_hdrs(udp);
				if (error != 0)
					return (error);
			}
			break;
		case IPV6_NEXTHOP:
			/*
			 * IP will verify that the nexthop is reachable
			 * and fail for sticky options.
			 */
			if (inlen != 0 && inlen != sizeof (sin6_t))
				return (EINVAL);
			if (checkonly)
				break;

			if (inlen == 0) {
				ipp->ipp_fields &= ~IPPF_NEXTHOP;
				ipp->ipp_sticky_ignored |= IPPF_NEXTHOP;
			} else {
				sin6_t *sin6 = (sin6_t *)invalp;

				if (sin6->sin6_family != AF_INET6)
					return (EAFNOSUPPORT);
				if (IN6_IS_ADDR_V4MAPPED(
				    &sin6->sin6_addr))
					return (EADDRNOTAVAIL);
				ipp->ipp_nexthop = sin6->sin6_addr;
				if (!IN6_IS_ADDR_UNSPECIFIED(
				    &ipp->ipp_nexthop))
					ipp->ipp_fields |= IPPF_NEXTHOP;
				else
					ipp->ipp_fields &= ~IPPF_NEXTHOP;
			}
			if (sticky) {
				error = udp_build_hdrs(udp);
				if (error != 0)
					return (error);
			}
			break;
		case IPV6_HOPOPTS: {
			ip6_hbh_t *hopts = (ip6_hbh_t *)invalp;
			/*
			 * Sanity checks - minimum size, size a multiple of
			 * eight bytes, and matching size passed in.
			 */
			if (inlen != 0 &&
			    inlen != (8 * (hopts->ip6h_len + 1)))
				return (EINVAL);

			if (checkonly)
				break;

			error = optcom_pkt_set(invalp, inlen, sticky,
			    (uchar_t **)&ipp->ipp_hopopts,
			    &ipp->ipp_hopoptslen,
			    sticky ? udp->udp_label_len_v6 : 0);
			if (error != 0)
				return (error);
			if (ipp->ipp_hopoptslen == 0) {
				ipp->ipp_fields &= ~IPPF_HOPOPTS;
				ipp->ipp_sticky_ignored |= IPPF_HOPOPTS;
			} else {
				ipp->ipp_fields |= IPPF_HOPOPTS;
			}
			if (sticky) {
				error = udp_build_hdrs(udp);
				if (error != 0)
					return (error);
			}
			break;
		}
		case IPV6_RTHDRDSTOPTS: {
			ip6_dest_t *dopts = (ip6_dest_t *)invalp;

			/*
			 * Sanity checks - minimum size, size a multiple of
			 * eight bytes, and matching size passed in.
			 */
			if (inlen != 0 &&
			    inlen != (8 * (dopts->ip6d_len + 1)))
				return (EINVAL);

			if (checkonly)
				break;

			if (inlen == 0) {
				if (sticky &&
				    (ipp->ipp_fields & IPPF_RTDSTOPTS) != 0) {
					kmem_free(ipp->ipp_rtdstopts,
					    ipp->ipp_rtdstoptslen);
					ipp->ipp_rtdstopts = NULL;
					ipp->ipp_rtdstoptslen = 0;
				}

				ipp->ipp_fields &= ~IPPF_RTDSTOPTS;
				ipp->ipp_sticky_ignored |= IPPF_RTDSTOPTS;
			} else {
				error = optcom_pkt_set(invalp, inlen, sticky,
				    (uchar_t **)&ipp->ipp_rtdstopts,
				    &ipp->ipp_rtdstoptslen, 0);
				if (error != 0)
					return (error);
				ipp->ipp_fields |= IPPF_RTDSTOPTS;
			}
			if (sticky) {
				error = udp_build_hdrs(udp);
				if (error != 0)
					return (error);
			}
			break;
		}
		case IPV6_DSTOPTS: {
			ip6_dest_t *dopts = (ip6_dest_t *)invalp;

			/*
			 * Sanity checks - minimum size, size a multiple of
			 * eight bytes, and matching size passed in.
			 */
			if (inlen != 0 &&
			    inlen != (8 * (dopts->ip6d_len + 1)))
				return (EINVAL);

			if (checkonly)
				break;

			if (inlen == 0) {
				if (sticky &&
				    (ipp->ipp_fields & IPPF_DSTOPTS) != 0) {
					kmem_free(ipp->ipp_dstopts,
					    ipp->ipp_dstoptslen);
					ipp->ipp_dstopts = NULL;
					ipp->ipp_dstoptslen = 0;
				}
				ipp->ipp_fields &= ~IPPF_DSTOPTS;
				ipp->ipp_sticky_ignored |= IPPF_DSTOPTS;
			} else {
				error = optcom_pkt_set(invalp, inlen, sticky,
				    (uchar_t **)&ipp->ipp_dstopts,
				    &ipp->ipp_dstoptslen, 0);
				if (error != 0)
					return (error);
				ipp->ipp_fields |= IPPF_DSTOPTS;
			}
			if (sticky) {
				error = udp_build_hdrs(udp);
				if (error != 0)
					return (error);
			}
			break;
		}
		case IPV6_RTHDR: {
			ip6_rthdr_t *rt = (ip6_rthdr_t *)invalp;

			/*
			 * Sanity checks - minimum size, size a multiple of
			 * eight bytes, and matching size passed in.
			 */
			if (inlen != 0 &&
			    inlen != (8 * (rt->ip6r_len + 1)))
				return (EINVAL);

			if (checkonly)
				break;

			if (inlen == 0) {
				if (sticky &&
				    (ipp->ipp_fields & IPPF_RTHDR) != 0) {
					kmem_free(ipp->ipp_rthdr,
					    ipp->ipp_rthdrlen);
					ipp->ipp_rthdr = NULL;
					ipp->ipp_rthdrlen = 0;
				}
				ipp->ipp_fields &= ~IPPF_RTHDR;
				ipp->ipp_sticky_ignored |= IPPF_RTHDR;
			} else {
				error = optcom_pkt_set(invalp, inlen, sticky,
				    (uchar_t **)&ipp->ipp_rthdr,
				    &ipp->ipp_rthdrlen, 0);
				if (error != 0)
					return (error);
				ipp->ipp_fields |= IPPF_RTHDR;
			}
			if (sticky) {
				error = udp_build_hdrs(udp);
				if (error != 0)
					return (error);
			}
			break;
		}

		case IPV6_DONTFRAG:
			if (checkonly)
				break;

			if (onoff) {
				ipp->ipp_fields |= IPPF_DONTFRAG;
			} else {
				ipp->ipp_fields &= ~IPPF_DONTFRAG;
			}
			break;

		case IPV6_USE_MIN_MTU:
			if (inlen != sizeof (int))
				return (EINVAL);

			if (*i1 < -1 || *i1 > 1)
				return (EINVAL);

			if (checkonly)
				break;

			ipp->ipp_fields |= IPPF_USE_MIN_MTU;
			ipp->ipp_use_min_mtu = *i1;
			break;

		case IPV6_BOUND_PIF:
		case IPV6_SEC_OPT:
		case IPV6_DONTFAILOVER_IF:
		case IPV6_SRC_PREFERENCES:
		case IPV6_V6ONLY:
			/* Handled at the IP level */
			return (-EINVAL);
		default:
			*outlenp = 0;
			return (EINVAL);
		}
		break;
		}		/* end IPPROTO_IPV6 */
	case IPPROTO_UDP:
		switch (name) {
		case UDP_ANONPRIVBIND:
			if ((error = secpolicy_net_privaddr(cr, 0,
			    IPPROTO_UDP)) != 0) {
				*outlenp = 0;
				return (error);
			}
			if (!checkonly) {
				udp->udp_anon_priv_bind = onoff;
			}
			break;
		case UDP_EXCLBIND:
			if (!checkonly)
				udp->udp_exclbind = onoff;
			break;
		case UDP_RCVHDR:
			if (!checkonly)
				udp->udp_rcvhdr = onoff;
			break;
		case UDP_NAT_T_ENDPOINT:
			if ((error = secpolicy_ip_config(cr, B_FALSE)) != 0) {
				*outlenp = 0;
				return (error);
			}

			/*
			 * Use udp_family instead so we can avoid ambiguitites
			 * with AF_INET6 sockets that may switch from IPv4
			 * to IPv6.
			 */
			if (udp->udp_family != AF_INET) {
				*outlenp = 0;
				return (EAFNOSUPPORT);
			}

			if (!checkonly) {
				udp->udp_nat_t_endpoint = onoff;

				udp->udp_max_hdr_len = IP_SIMPLE_HDR_LENGTH +
				    UDPH_SIZE + udp->udp_ip_snd_options_len;

				/* Also, adjust wroff */
				if (onoff) {
					udp->udp_max_hdr_len +=
					    sizeof (uint32_t);
				}
				(void) mi_set_sth_wroff(RD(q),
				    udp->udp_max_hdr_len + us->us_wroff_extra);
			}
			break;
		default:
			*outlenp = 0;
			return (EINVAL);
		}
		break;
	default:
		*outlenp = 0;
		return (EINVAL);
	}
	/*
	 * Common case of OK return with outval same as inval.
	 */
	if (invalp != outvalp) {
		/* don't trust bcopy for identical src/dst */
		(void) bcopy(invalp, outvalp, inlen);
	}
	*outlenp = inlen;
	return (0);
}

int
udp_opt_set(queue_t *q, uint_t optset_context, int level,
    int name, uint_t inlen, uchar_t *invalp, uint_t *outlenp,
    uchar_t *outvalp, void *thisdg_attrs, cred_t *cr, mblk_t *mblk)
{
	udp_t	*udp;
	int	err;

	udp = Q_TO_UDP(q);

	rw_enter(&udp->udp_rwlock, RW_WRITER);
	err = udp_opt_set_locked(q, optset_context, level, name, inlen, invalp,
	    outlenp, outvalp, thisdg_attrs, cr, mblk);
	rw_exit(&udp->udp_rwlock);
	return (err);
}

/*
 * Update udp_sticky_hdrs based on udp_sticky_ipp, udp_v6src, and udp_ttl.
 * The headers include ip6i_t (if needed), ip6_t, any sticky extension
 * headers, and the udp header.
 * Returns failure if can't allocate memory.
 */
static int
udp_build_hdrs(udp_t *udp)
{
	udp_stack_t *us = udp->udp_us;
	uchar_t	*hdrs;
	uint_t	hdrs_len;
	ip6_t	*ip6h;
	ip6i_t	*ip6i;
	udpha_t	*udpha;
	ip6_pkt_t *ipp = &udp->udp_sticky_ipp;
	size_t	sth_wroff;

	ASSERT(RW_WRITE_HELD(&udp->udp_rwlock));
	hdrs_len = ip_total_hdrs_len_v6(ipp) + UDPH_SIZE;
	ASSERT(hdrs_len != 0);
	if (hdrs_len != udp->udp_sticky_hdrs_len) {
		/* Need to reallocate */
		hdrs = kmem_alloc(hdrs_len, KM_NOSLEEP);
		if (hdrs == NULL)
			return (ENOMEM);

		if (udp->udp_sticky_hdrs_len != 0) {
			kmem_free(udp->udp_sticky_hdrs,
			    udp->udp_sticky_hdrs_len);
		}
		udp->udp_sticky_hdrs = hdrs;
		udp->udp_sticky_hdrs_len = hdrs_len;
	}
	ip_build_hdrs_v6(udp->udp_sticky_hdrs,
	    udp->udp_sticky_hdrs_len - UDPH_SIZE, ipp, IPPROTO_UDP);

	/* Set header fields not in ipp */
	if (ipp->ipp_fields & IPPF_HAS_IP6I) {
		ip6i = (ip6i_t *)udp->udp_sticky_hdrs;
		ip6h = (ip6_t *)&ip6i[1];
	} else {
		ip6h = (ip6_t *)udp->udp_sticky_hdrs;
	}

	if (!(ipp->ipp_fields & IPPF_ADDR))
		ip6h->ip6_src = udp->udp_v6src;

	udpha = (udpha_t *)(udp->udp_sticky_hdrs + hdrs_len - UDPH_SIZE);
	udpha->uha_src_port = udp->udp_port;

	/* Try to get everything in a single mblk */
	if (hdrs_len > udp->udp_max_hdr_len) {
		udp->udp_max_hdr_len = hdrs_len;
		sth_wroff = udp->udp_max_hdr_len + us->us_wroff_extra;
		rw_exit(&udp->udp_rwlock);
		(void) mi_set_sth_wroff(udp->udp_connp->conn_rq, sth_wroff);
		rw_enter(&udp->udp_rwlock, RW_WRITER);
	}
	return (0);
}

/*
 * This routine retrieves the value of an ND variable in a udpparam_t
 * structure.  It is called through nd_getset when a user reads the
 * variable.
 */
/* ARGSUSED */
static int
udp_param_get(queue_t *q, mblk_t *mp, caddr_t cp, cred_t *cr)
{
	udpparam_t *udppa = (udpparam_t *)cp;

	(void) mi_mpprintf(mp, "%d", udppa->udp_param_value);
	return (0);
}

/*
 * Walk through the param array specified registering each element with the
 * named dispatch (ND) handler.
 */
static boolean_t
udp_param_register(IDP *ndp, udpparam_t *udppa, int cnt)
{
	for (; cnt-- > 0; udppa++) {
		if (udppa->udp_param_name && udppa->udp_param_name[0]) {
			if (!nd_load(ndp, udppa->udp_param_name,
			    udp_param_get, udp_param_set,
			    (caddr_t)udppa)) {
				nd_free(ndp);
				return (B_FALSE);
			}
		}
	}
	if (!nd_load(ndp, "udp_extra_priv_ports",
	    udp_extra_priv_ports_get, NULL, NULL)) {
		nd_free(ndp);
		return (B_FALSE);
	}
	if (!nd_load(ndp, "udp_extra_priv_ports_add",
	    NULL, udp_extra_priv_ports_add, NULL)) {
		nd_free(ndp);
		return (B_FALSE);
	}
	if (!nd_load(ndp, "udp_extra_priv_ports_del",
	    NULL, udp_extra_priv_ports_del, NULL)) {
		nd_free(ndp);
		return (B_FALSE);
	}
	if (!nd_load(ndp, "udp_status", udp_status_report, NULL,
	    NULL)) {
		nd_free(ndp);
		return (B_FALSE);
	}
	if (!nd_load(ndp, "udp_bind_hash", udp_bind_hash_report, NULL,
	    NULL)) {
		nd_free(ndp);
		return (B_FALSE);
	}
	return (B_TRUE);
}

/* This routine sets an ND variable in a udpparam_t structure. */
/* ARGSUSED */
static int
udp_param_set(queue_t *q, mblk_t *mp, char *value, caddr_t cp, cred_t *cr)
{
	long		new_value;
	udpparam_t	*udppa = (udpparam_t *)cp;

	/*
	 * Fail the request if the new value does not lie within the
	 * required bounds.
	 */
	if (ddi_strtol(value, NULL, 10, &new_value) != 0 ||
	    new_value < udppa->udp_param_min ||
	    new_value > udppa->udp_param_max) {
		return (EINVAL);
	}

	/* Set the new value */
	udppa->udp_param_value = new_value;
	return (0);
}

/*
 * Copy hop-by-hop option from ipp->ipp_hopopts to the buffer provided (with
 * T_opthdr) and return the number of bytes copied.  'dbuf' may be NULL to
 * just count the length needed for allocation.  If 'dbuf' is non-NULL,
 * then it's assumed to be allocated to be large enough.
 *
 * Returns zero if trimming of the security option causes all options to go
 * away.
 */
static size_t
copy_hop_opts(const ip6_pkt_t *ipp, uchar_t *dbuf)
{
	struct T_opthdr *toh;
	size_t hol = ipp->ipp_hopoptslen;
	ip6_hbh_t *dstopt = NULL;
	const ip6_hbh_t *srcopt = ipp->ipp_hopopts;
	size_t tlen, olen, plen;
	boolean_t deleting;
	const struct ip6_opt *sopt, *lastpad;
	struct ip6_opt *dopt;

	if ((toh = (struct T_opthdr *)dbuf) != NULL) {
		toh->level = IPPROTO_IPV6;
		toh->name = IPV6_HOPOPTS;
		toh->status = 0;
		dstopt = (ip6_hbh_t *)(toh + 1);
	}

	/*
	 * If labeling is enabled, then skip the label option
	 * but get other options if there are any.
	 */
	if (is_system_labeled()) {
		dopt = NULL;
		if (dstopt != NULL) {
			/* will fill in ip6h_len later */
			dstopt->ip6h_nxt = srcopt->ip6h_nxt;
			dopt = (struct ip6_opt *)(dstopt + 1);
		}
		sopt = (const struct ip6_opt *)(srcopt + 1);
		hol -= sizeof (*srcopt);
		tlen = sizeof (*dstopt);
		lastpad = NULL;
		deleting = B_FALSE;
		/*
		 * This loop finds the first (lastpad pointer) of any number of
		 * pads that preceeds the security option, then treats the
		 * security option as though it were a pad, and then finds the
		 * next non-pad option (or end of list).
		 *
		 * It then treats the entire block as one big pad.  To preserve
		 * alignment of any options that follow, or just the end of the
		 * list, it computes a minimal new padding size that keeps the
		 * same alignment for the next option.
		 *
		 * If it encounters just a sequence of pads with no security
		 * option, those are copied as-is rather than collapsed.
		 *
		 * Note that to handle the end of list case, the code makes one
		 * loop with 'hol' set to zero.
		 */
		for (;;) {
			if (hol > 0) {
				if (sopt->ip6o_type == IP6OPT_PAD1) {
					if (lastpad == NULL)
						lastpad = sopt;
					sopt = (const struct ip6_opt *)
					    &sopt->ip6o_len;
					hol--;
					continue;
				}
				olen = sopt->ip6o_len + sizeof (*sopt);
				if (olen > hol)
					olen = hol;
				if (sopt->ip6o_type == IP6OPT_PADN ||
				    sopt->ip6o_type == ip6opt_ls) {
					if (sopt->ip6o_type == ip6opt_ls)
						deleting = B_TRUE;
					if (lastpad == NULL)
						lastpad = sopt;
					sopt = (const struct ip6_opt *)
					    ((const char *)sopt + olen);
					hol -= olen;
					continue;
				}
			} else {
				/* if nothing was copied at all, then delete */
				if (tlen == sizeof (*dstopt))
					return (0);
				/* last pass; pick up any trailing padding */
				olen = 0;
			}
			if (deleting) {
				/*
				 * compute aligning effect of deleted material
				 * to reproduce with pad.
				 */
				plen = ((const char *)sopt -
				    (const char *)lastpad) & 7;
				tlen += plen;
				if (dopt != NULL) {
					if (plen == 1) {
						dopt->ip6o_type = IP6OPT_PAD1;
					} else if (plen > 1) {
						plen -= sizeof (*dopt);
						dopt->ip6o_type = IP6OPT_PADN;
						dopt->ip6o_len = plen;
						if (plen > 0)
							bzero(dopt + 1, plen);
					}
					dopt = (struct ip6_opt *)
					    ((char *)dopt + plen);
				}
				deleting = B_FALSE;
				lastpad = NULL;
			}
			/* if there's uncopied padding, then copy that now */
			if (lastpad != NULL) {
				olen += (const char *)sopt -
				    (const char *)lastpad;
				sopt = lastpad;
				lastpad = NULL;
			}
			if (dopt != NULL && olen > 0) {
				bcopy(sopt, dopt, olen);
				dopt = (struct ip6_opt *)((char *)dopt + olen);
			}
			if (hol == 0)
				break;
			tlen += olen;
			sopt = (const struct ip6_opt *)
			    ((const char *)sopt + olen);
			hol -= olen;
		}
		/* go back and patch up the length value, rounded upward */
		if (dstopt != NULL)
			dstopt->ip6h_len = (tlen - 1) >> 3;
	} else {
		tlen = hol;
		if (dstopt != NULL)
			bcopy(srcopt, dstopt, hol);
	}

	tlen += sizeof (*toh);
	if (toh != NULL)
		toh->len = tlen;

	return (tlen);
}

/*
 * Update udp_rcv_opt_len from the packet.
 * Called when options received, and when no options received but
 * udp_ip_recv_opt_len has previously recorded options.
 */
static void
udp_save_ip_rcv_opt(udp_t *udp, void *opt, int opt_len)
{
	/* Save the options if any */
	if (opt_len > 0) {
		if (opt_len > udp->udp_ip_rcv_options_len) {
			/* Need to allocate larger buffer */
			if (udp->udp_ip_rcv_options_len != 0)
				mi_free((char *)udp->udp_ip_rcv_options);
			udp->udp_ip_rcv_options_len = 0;
			udp->udp_ip_rcv_options =
			    (uchar_t *)mi_alloc(opt_len, BPRI_HI);
			if (udp->udp_ip_rcv_options != NULL)
				udp->udp_ip_rcv_options_len = opt_len;
		}
		if (udp->udp_ip_rcv_options_len != 0) {
			bcopy(opt, udp->udp_ip_rcv_options, opt_len);
			/* Adjust length if we are resusing the space */
			udp->udp_ip_rcv_options_len = opt_len;
		}
	} else if (udp->udp_ip_rcv_options_len != 0) {
		/* Clear out previously recorded options */
		mi_free((char *)udp->udp_ip_rcv_options);
		udp->udp_ip_rcv_options = NULL;
		udp->udp_ip_rcv_options_len = 0;
	}
}

/* ARGSUSED2 */
static void
udp_input(void *arg1, mblk_t *mp, void *arg2)
{
	conn_t *connp = (conn_t *)arg1;
	struct T_unitdata_ind	*tudi;
	uchar_t			*rptr;		/* Pointer to IP header */
	int			hdr_length;	/* Length of IP+UDP headers */
	int			opt_len;
	int			udi_size;	/* Size of T_unitdata_ind */
	int			mp_len;
	udp_t			*udp;
	udpha_t			*udpha;
	int			ipversion;
	ip6_pkt_t		ipp;
	ip6_t			*ip6h;
	ip6i_t			*ip6i;
	mblk_t			*mp1;
	mblk_t			*options_mp = NULL;
	ip_pktinfo_t		*pinfo = NULL;
	cred_t			*cr = NULL;
	pid_t			cpid;
	uint32_t		udp_ip_rcv_options_len;
	udp_bits_t		udp_bits;
	cred_t			*rcr = connp->conn_cred;
	udp_stack_t *us;

	ASSERT(connp->conn_flags & IPCL_UDPCONN);

	udp = connp->conn_udp;
	us = udp->udp_us;
	rptr = mp->b_rptr;
	ASSERT(DB_TYPE(mp) == M_DATA || DB_TYPE(mp) == M_CTL);
	ASSERT(OK_32PTR(rptr));

	/*
	 * IP should have prepended the options data in an M_CTL
	 * Check M_CTL "type" to make sure are not here bcos of
	 * a valid ICMP message
	 */
	if (DB_TYPE(mp) == M_CTL) {
		if (MBLKL(mp) == sizeof (ip_pktinfo_t) &&
		    ((ip_pktinfo_t *)mp->b_rptr)->ip_pkt_ulp_type ==
		    IN_PKTINFO) {
			/*
			 * IP_RECVIF or IP_RECVSLLA or IPF_RECVADDR information
			 * has been prepended to the packet by IP. We need to
			 * extract the mblk and adjust the rptr
			 */
			pinfo = (ip_pktinfo_t *)mp->b_rptr;
			options_mp = mp;
			mp = mp->b_cont;
			rptr = mp->b_rptr;
			UDP_STAT(us, udp_in_pktinfo);
		} else {
			/*
			 * ICMP messages.
			 */
			udp_icmp_error(connp->conn_rq, mp);
			return;
		}
	}

	mp_len = msgdsize(mp);
	/*
	 * This is the inbound data path.
	 * First, we check to make sure the IP version number is correct,
	 * and then pull the IP and UDP headers into the first mblk.
	 */

	/* Initialize regardless if ipversion is IPv4 or IPv6 */
	ipp.ipp_fields = 0;

	ipversion = IPH_HDR_VERSION(rptr);

	rw_enter(&udp->udp_rwlock, RW_READER);
	udp_ip_rcv_options_len = udp->udp_ip_rcv_options_len;
	udp_bits = udp->udp_bits;
	rw_exit(&udp->udp_rwlock);

	switch (ipversion) {
	case IPV4_VERSION:
		ASSERT(MBLKL(mp) >= sizeof (ipha_t));
		ASSERT(((ipha_t *)rptr)->ipha_protocol == IPPROTO_UDP);
		hdr_length = IPH_HDR_LENGTH(rptr) + UDPH_SIZE;
		opt_len = hdr_length - (IP_SIMPLE_HDR_LENGTH + UDPH_SIZE);
		if ((opt_len > 0 || udp_ip_rcv_options_len > 0) &&
		    udp->udp_family == AF_INET) {
			/*
			 * Record/update udp_ip_rcv_options with the lock
			 * held. Not needed for AF_INET6 sockets
			 * since they don't support a getsockopt of IP_OPTIONS.
			 */
			rw_enter(&udp->udp_rwlock, RW_WRITER);
			udp_save_ip_rcv_opt(udp, rptr + IP_SIMPLE_HDR_LENGTH,
			    opt_len);
			rw_exit(&udp->udp_rwlock);
		}
		/* Handle IPV6_RECVPKTINFO even for IPv4 packet. */
		if ((udp->udp_family == AF_INET6) && (pinfo != NULL) &&
		    udp->udp_ip_recvpktinfo) {
			if (pinfo->ip_pkt_flags & IPF_RECVIF) {
				ipp.ipp_fields |= IPPF_IFINDEX;
				ipp.ipp_ifindex = pinfo->ip_pkt_ifindex;
			}
		}
		break;
	case IPV6_VERSION:
		/*
		 * IPv6 packets can only be received by applications
		 * that are prepared to receive IPv6 addresses.
		 * The IP fanout must ensure this.
		 */
		ASSERT(udp->udp_family == AF_INET6);

		ip6h = (ip6_t *)rptr;
		ASSERT((uchar_t *)&ip6h[1] <= mp->b_wptr);

		if (ip6h->ip6_nxt != IPPROTO_UDP) {
			uint8_t nexthdrp;
			/* Look for ifindex information */
			if (ip6h->ip6_nxt == IPPROTO_RAW) {
				ip6i = (ip6i_t *)ip6h;
				if ((uchar_t *)&ip6i[1] > mp->b_wptr)
					goto tossit;

				if (ip6i->ip6i_flags & IP6I_IFINDEX) {
					ASSERT(ip6i->ip6i_ifindex != 0);
					ipp.ipp_fields |= IPPF_IFINDEX;
					ipp.ipp_ifindex = ip6i->ip6i_ifindex;
				}
				rptr = (uchar_t *)&ip6i[1];
				mp->b_rptr = rptr;
				if (rptr == mp->b_wptr) {
					mp1 = mp->b_cont;
					freeb(mp);
					mp = mp1;
					rptr = mp->b_rptr;
				}
				if (MBLKL(mp) < (IPV6_HDR_LEN + UDPH_SIZE))
					goto tossit;
				ip6h = (ip6_t *)rptr;
				mp_len = msgdsize(mp);
			}
			/*
			 * Find any potentially interesting extension headers
			 * as well as the length of the IPv6 + extension
			 * headers.
			 */
			hdr_length = ip_find_hdr_v6(mp, ip6h, &ipp, &nexthdrp) +
			    UDPH_SIZE;
			ASSERT(nexthdrp == IPPROTO_UDP);
		} else {
			hdr_length = IPV6_HDR_LEN + UDPH_SIZE;
			ip6i = NULL;
		}
		break;
	default:
		ASSERT(0);
	}

	/*
	 * IP inspected the UDP header thus all of it must be in the mblk.
	 * UDP length check is performed for IPv6 packets and IPv4 packets
	 * to check if the size of the packet as specified
	 * by the header is the same as the physical size of the packet.
	 * FIXME? Didn't IP already check this?
	 */
	udpha = (udpha_t *)(rptr + (hdr_length - UDPH_SIZE));
	if ((MBLKL(mp) < hdr_length) ||
	    (mp_len != (ntohs(udpha->uha_length) + hdr_length - UDPH_SIZE))) {
		goto tossit;
	}


	/* Walk past the headers unless IP_RECVHDR was set. */
	if (!udp_bits.udpb_rcvhdr) {
		mp->b_rptr = rptr + hdr_length;
		mp_len -= hdr_length;
	}

	/*
	 * This is the inbound data path.  Packets are passed upstream as
	 * T_UNITDATA_IND messages with full IP headers still attached.
	 */
	if (udp->udp_family == AF_INET) {
		sin_t *sin;

		ASSERT(IPH_HDR_VERSION((ipha_t *)rptr) == IPV4_VERSION);

		/*
		 * Normally only send up the source address.
		 * If IP_RECVDSTADDR is set we include the destination IP
		 * address as an option. With IP_RECVOPTS we include all
		 * the IP options.
		 */
		udi_size = sizeof (struct T_unitdata_ind) + sizeof (sin_t);
		if (udp_bits.udpb_recvdstaddr) {
			udi_size += sizeof (struct T_opthdr) +
			    sizeof (struct in_addr);
			UDP_STAT(us, udp_in_recvdstaddr);
		}

		if (udp_bits.udpb_ip_recvpktinfo && (pinfo != NULL) &&
		    (pinfo->ip_pkt_flags & IPF_RECVADDR)) {
			udi_size += sizeof (struct T_opthdr) +
			    sizeof (struct in_pktinfo);
			UDP_STAT(us, udp_ip_rcvpktinfo);
		}

		if ((udp_bits.udpb_recvopts) && opt_len > 0) {
			udi_size += sizeof (struct T_opthdr) + opt_len;
			UDP_STAT(us, udp_in_recvopts);
		}

		/*
		 * If the IP_RECVSLLA or the IP_RECVIF is set then allocate
		 * space accordingly
		 */
		if ((udp_bits.udpb_recvif) && (pinfo != NULL) &&
		    (pinfo->ip_pkt_flags & IPF_RECVIF)) {
			udi_size += sizeof (struct T_opthdr) + sizeof (uint_t);
			UDP_STAT(us, udp_in_recvif);
		}

		if ((udp_bits.udpb_recvslla) && (pinfo != NULL) &&
		    (pinfo->ip_pkt_flags & IPF_RECVSLLA)) {
			udi_size += sizeof (struct T_opthdr) +
			    sizeof (struct sockaddr_dl);
			UDP_STAT(us, udp_in_recvslla);
		}

		if ((udp_bits.udpb_recvucred) &&
		    (cr = DB_CRED(mp)) != NULL) {
			udi_size += sizeof (struct T_opthdr) + ucredsize;
			cpid = DB_CPID(mp);
			UDP_STAT(us, udp_in_recvucred);
		}

		/*
		 * If SO_TIMESTAMP is set allocate the appropriate sized
		 * buffer. Since gethrestime() expects a pointer aligned
		 * argument, we allocate space necessary for extra
		 * alignment (even though it might not be used).
		 */
		if (udp_bits.udpb_timestamp) {
			udi_size += sizeof (struct T_opthdr) +
			    sizeof (timestruc_t) + _POINTER_ALIGNMENT;
			UDP_STAT(us, udp_in_timestamp);
		}

		/*
		 * If IP_RECVTTL is set allocate the appropriate sized buffer
		 */
		if (udp_bits.udpb_recvttl) {
			udi_size += sizeof (struct T_opthdr) + sizeof (uint8_t);
			UDP_STAT(us, udp_in_recvttl);
		}

		/* Allocate a message block for the T_UNITDATA_IND structure. */
		mp1 = allocb(udi_size, BPRI_MED);
		if (mp1 == NULL) {
			freemsg(mp);
			if (options_mp != NULL)
				freeb(options_mp);
			BUMP_MIB(&us->us_udp_mib, udpInErrors);
			return;
		}
		mp1->b_cont = mp;
		mp = mp1;
		mp->b_datap->db_type = M_PROTO;
		tudi = (struct T_unitdata_ind *)mp->b_rptr;
		mp->b_wptr = (uchar_t *)tudi + udi_size;
		tudi->PRIM_type = T_UNITDATA_IND;
		tudi->SRC_length = sizeof (sin_t);
		tudi->SRC_offset = sizeof (struct T_unitdata_ind);
		tudi->OPT_offset = sizeof (struct T_unitdata_ind) +
		    sizeof (sin_t);
		udi_size -= (sizeof (struct T_unitdata_ind) + sizeof (sin_t));
		tudi->OPT_length = udi_size;
		sin = (sin_t *)&tudi[1];
		sin->sin_addr.s_addr = ((ipha_t *)rptr)->ipha_src;
		sin->sin_port =	udpha->uha_src_port;
		sin->sin_family = udp->udp_family;
		*(uint32_t *)&sin->sin_zero[0] = 0;
		*(uint32_t *)&sin->sin_zero[4] = 0;

		/*
		 * Add options if IP_RECVDSTADDR, IP_RECVIF, IP_RECVSLLA or
		 * IP_RECVTTL has been set.
		 */
		if (udi_size != 0) {
			/*
			 * Copy in destination address before options to avoid
			 * any padding issues.
			 */
			char *dstopt;

			dstopt = (char *)&sin[1];
			if (udp_bits.udpb_recvdstaddr) {
				struct T_opthdr *toh;
				ipaddr_t *dstptr;

				toh = (struct T_opthdr *)dstopt;
				toh->level = IPPROTO_IP;
				toh->name = IP_RECVDSTADDR;
				toh->len = sizeof (struct T_opthdr) +
				    sizeof (ipaddr_t);
				toh->status = 0;
				dstopt += sizeof (struct T_opthdr);
				dstptr = (ipaddr_t *)dstopt;
				*dstptr = ((ipha_t *)rptr)->ipha_dst;
				dstopt += sizeof (ipaddr_t);
				udi_size -= toh->len;
			}

			if (udp_bits.udpb_recvopts && opt_len > 0) {
				struct T_opthdr *toh;

				toh = (struct T_opthdr *)dstopt;
				toh->level = IPPROTO_IP;
				toh->name = IP_RECVOPTS;
				toh->len = sizeof (struct T_opthdr) + opt_len;
				toh->status = 0;
				dstopt += sizeof (struct T_opthdr);
				bcopy(rptr + IP_SIMPLE_HDR_LENGTH, dstopt,
				    opt_len);
				dstopt += opt_len;
				udi_size -= toh->len;
			}

			if ((udp_bits.udpb_ip_recvpktinfo) && (pinfo != NULL) &&
			    (pinfo->ip_pkt_flags & IPF_RECVADDR)) {
				struct T_opthdr *toh;
				struct in_pktinfo *pktinfop;

				toh = (struct T_opthdr *)dstopt;
				toh->level = IPPROTO_IP;
				toh->name = IP_PKTINFO;
				toh->len = sizeof (struct T_opthdr) +
				    sizeof (*pktinfop);
				toh->status = 0;
				dstopt += sizeof (struct T_opthdr);
				pktinfop = (struct in_pktinfo *)dstopt;
				pktinfop->ipi_ifindex = pinfo->ip_pkt_ifindex;
				pktinfop->ipi_spec_dst =
				    pinfo->ip_pkt_match_addr;
				pktinfop->ipi_addr.s_addr =
				    ((ipha_t *)rptr)->ipha_dst;

				dstopt += sizeof (struct in_pktinfo);
				udi_size -= toh->len;
			}

			if ((udp_bits.udpb_recvslla) && (pinfo != NULL) &&
			    (pinfo->ip_pkt_flags & IPF_RECVSLLA)) {

				struct T_opthdr *toh;
				struct sockaddr_dl	*dstptr;

				toh = (struct T_opthdr *)dstopt;
				toh->level = IPPROTO_IP;
				toh->name = IP_RECVSLLA;
				toh->len = sizeof (struct T_opthdr) +
				    sizeof (struct sockaddr_dl);
				toh->status = 0;
				dstopt += sizeof (struct T_opthdr);
				dstptr = (struct sockaddr_dl *)dstopt;
				bcopy(&pinfo->ip_pkt_slla, dstptr,
				    sizeof (struct sockaddr_dl));
				dstopt += sizeof (struct sockaddr_dl);
				udi_size -= toh->len;
			}

			if ((udp_bits.udpb_recvif) && (pinfo != NULL) &&
			    (pinfo->ip_pkt_flags & IPF_RECVIF)) {

				struct T_opthdr *toh;
				uint_t		*dstptr;

				toh = (struct T_opthdr *)dstopt;
				toh->level = IPPROTO_IP;
				toh->name = IP_RECVIF;
				toh->len = sizeof (struct T_opthdr) +
				    sizeof (uint_t);
				toh->status = 0;
				dstopt += sizeof (struct T_opthdr);
				dstptr = (uint_t *)dstopt;
				*dstptr = pinfo->ip_pkt_ifindex;
				dstopt += sizeof (uint_t);
				udi_size -= toh->len;
			}

			if (cr != NULL) {
				struct T_opthdr *toh;

				toh = (struct T_opthdr *)dstopt;
				toh->level = SOL_SOCKET;
				toh->name = SCM_UCRED;
				toh->len = sizeof (struct T_opthdr) + ucredsize;
				toh->status = 0;
				dstopt += sizeof (struct T_opthdr);
				(void) cred2ucred(cr, cpid, dstopt, rcr);
				dstopt += ucredsize;
				udi_size -= toh->len;
			}

			if (udp_bits.udpb_timestamp) {
				struct	T_opthdr *toh;

				toh = (struct T_opthdr *)dstopt;
				toh->level = SOL_SOCKET;
				toh->name = SCM_TIMESTAMP;
				toh->len = sizeof (struct T_opthdr) +
				    sizeof (timestruc_t) + _POINTER_ALIGNMENT;
				toh->status = 0;
				dstopt += sizeof (struct T_opthdr);
				/* Align for gethrestime() */
				dstopt = (char *)P2ROUNDUP((intptr_t)dstopt,
				    sizeof (intptr_t));
				gethrestime((timestruc_t *)dstopt);
				dstopt = (char *)toh + toh->len;
				udi_size -= toh->len;
			}

			/*
			 * CAUTION:
			 * Due to aligment issues
			 * Processing of IP_RECVTTL option
			 * should always be the last. Adding
			 * any option processing after this will
			 * cause alignment panic.
			 */
			if (udp_bits.udpb_recvttl) {
				struct	T_opthdr *toh;
				uint8_t	*dstptr;

				toh = (struct T_opthdr *)dstopt;
				toh->level = IPPROTO_IP;
				toh->name = IP_RECVTTL;
				toh->len = sizeof (struct T_opthdr) +
				    sizeof (uint8_t);
				toh->status = 0;
				dstopt += sizeof (struct T_opthdr);
				dstptr = (uint8_t *)dstopt;
				*dstptr = ((ipha_t *)rptr)->ipha_ttl;
				dstopt += sizeof (uint8_t);
				udi_size -= toh->len;
			}

			/* Consumed all of allocated space */
			ASSERT(udi_size == 0);
		}
	} else {
		sin6_t *sin6;

		/*
		 * Handle both IPv4 and IPv6 packets for IPv6 sockets.
		 *
		 * Normally we only send up the address. If receiving of any
		 * optional receive side information is enabled, we also send
		 * that up as options.
		 */
		udi_size = sizeof (struct T_unitdata_ind) + sizeof (sin6_t);

		if (ipp.ipp_fields & (IPPF_HOPOPTS|IPPF_DSTOPTS|IPPF_RTDSTOPTS|
		    IPPF_RTHDR|IPPF_IFINDEX)) {
			if ((udp_bits.udpb_ipv6_recvhopopts) &&
			    (ipp.ipp_fields & IPPF_HOPOPTS)) {
				size_t hlen;

				UDP_STAT(us, udp_in_recvhopopts);
				hlen = copy_hop_opts(&ipp, NULL);
				if (hlen == 0)
					ipp.ipp_fields &= ~IPPF_HOPOPTS;
				udi_size += hlen;
			}
			if (((udp_bits.udpb_ipv6_recvdstopts) ||
			    udp_bits.udpb_old_ipv6_recvdstopts) &&
			    (ipp.ipp_fields & IPPF_DSTOPTS)) {
				udi_size += sizeof (struct T_opthdr) +
				    ipp.ipp_dstoptslen;
				UDP_STAT(us, udp_in_recvdstopts);
			}
			if ((((udp_bits.udpb_ipv6_recvdstopts) &&
			    udp_bits.udpb_ipv6_recvrthdr &&
			    (ipp.ipp_fields & IPPF_RTHDR)) ||
			    (udp_bits.udpb_ipv6_recvrthdrdstopts)) &&
			    (ipp.ipp_fields & IPPF_RTDSTOPTS)) {
				udi_size += sizeof (struct T_opthdr) +
				    ipp.ipp_rtdstoptslen;
				UDP_STAT(us, udp_in_recvrtdstopts);
			}
			if ((udp_bits.udpb_ipv6_recvrthdr) &&
			    (ipp.ipp_fields & IPPF_RTHDR)) {
				udi_size += sizeof (struct T_opthdr) +
				    ipp.ipp_rthdrlen;
				UDP_STAT(us, udp_in_recvrthdr);
			}
			if ((udp_bits.udpb_ip_recvpktinfo) &&
			    (ipp.ipp_fields & IPPF_IFINDEX)) {
				udi_size += sizeof (struct T_opthdr) +
				    sizeof (struct in6_pktinfo);
				UDP_STAT(us, udp_in_recvpktinfo);
			}

		}
		if ((udp_bits.udpb_recvucred) &&
		    (cr = DB_CRED(mp)) != NULL) {
			udi_size += sizeof (struct T_opthdr) + ucredsize;
			cpid = DB_CPID(mp);
			UDP_STAT(us, udp_in_recvucred);
		}

		/*
		 * If SO_TIMESTAMP is set allocate the appropriate sized
		 * buffer. Since gethrestime() expects a pointer aligned
		 * argument, we allocate space necessary for extra
		 * alignment (even though it might not be used).
		 */
		if (udp_bits.udpb_timestamp) {
			udi_size += sizeof (struct T_opthdr) +
			    sizeof (timestruc_t) + _POINTER_ALIGNMENT;
			UDP_STAT(us, udp_in_timestamp);
		}

		if (udp_bits.udpb_ipv6_recvhoplimit) {
			udi_size += sizeof (struct T_opthdr) + sizeof (int);
			UDP_STAT(us, udp_in_recvhoplimit);
		}

		if (udp_bits.udpb_ipv6_recvtclass) {
			udi_size += sizeof (struct T_opthdr) + sizeof (int);
			UDP_STAT(us, udp_in_recvtclass);
		}

		mp1 = allocb(udi_size, BPRI_MED);
		if (mp1 == NULL) {
			freemsg(mp);
			if (options_mp != NULL)
				freeb(options_mp);
			BUMP_MIB(&us->us_udp_mib, udpInErrors);
			return;
		}
		mp1->b_cont = mp;
		mp = mp1;
		mp->b_datap->db_type = M_PROTO;
		tudi = (struct T_unitdata_ind *)mp->b_rptr;
		mp->b_wptr = (uchar_t *)tudi + udi_size;
		tudi->PRIM_type = T_UNITDATA_IND;
		tudi->SRC_length = sizeof (sin6_t);
		tudi->SRC_offset = sizeof (struct T_unitdata_ind);
		tudi->OPT_offset = sizeof (struct T_unitdata_ind) +
		    sizeof (sin6_t);
		udi_size -= (sizeof (struct T_unitdata_ind) + sizeof (sin6_t));
		tudi->OPT_length = udi_size;
		sin6 = (sin6_t *)&tudi[1];
		if (ipversion == IPV4_VERSION) {
			in6_addr_t v6dst;

			IN6_IPADDR_TO_V4MAPPED(((ipha_t *)rptr)->ipha_src,
			    &sin6->sin6_addr);
			IN6_IPADDR_TO_V4MAPPED(((ipha_t *)rptr)->ipha_dst,
			    &v6dst);
			sin6->sin6_flowinfo = 0;
			sin6->sin6_scope_id = 0;
			sin6->__sin6_src_id = ip_srcid_find_addr(&v6dst,
			    connp->conn_zoneid, us->us_netstack);
		} else {
			sin6->sin6_addr = ip6h->ip6_src;
			/* No sin6_flowinfo per API */
			sin6->sin6_flowinfo = 0;
			/* For link-scope source pass up scope id */
			if ((ipp.ipp_fields & IPPF_IFINDEX) &&
			    IN6_IS_ADDR_LINKSCOPE(&ip6h->ip6_src))
				sin6->sin6_scope_id = ipp.ipp_ifindex;
			else
				sin6->sin6_scope_id = 0;
			sin6->__sin6_src_id = ip_srcid_find_addr(
			    &ip6h->ip6_dst, connp->conn_zoneid,
			    us->us_netstack);
		}
		sin6->sin6_port = udpha->uha_src_port;
		sin6->sin6_family = udp->udp_family;

		if (udi_size != 0) {
			uchar_t *dstopt;

			dstopt = (uchar_t *)&sin6[1];
			if ((udp_bits.udpb_ip_recvpktinfo) &&
			    (ipp.ipp_fields & IPPF_IFINDEX)) {
				struct T_opthdr *toh;
				struct in6_pktinfo *pkti;

				toh = (struct T_opthdr *)dstopt;
				toh->level = IPPROTO_IPV6;
				toh->name = IPV6_PKTINFO;
				toh->len = sizeof (struct T_opthdr) +
				    sizeof (*pkti);
				toh->status = 0;
				dstopt += sizeof (struct T_opthdr);
				pkti = (struct in6_pktinfo *)dstopt;
				if (ipversion == IPV6_VERSION)
					pkti->ipi6_addr = ip6h->ip6_dst;
				else
					IN6_IPADDR_TO_V4MAPPED(
					    ((ipha_t *)rptr)->ipha_dst,
					    &pkti->ipi6_addr);
				pkti->ipi6_ifindex = ipp.ipp_ifindex;
				dstopt += sizeof (*pkti);
				udi_size -= toh->len;
			}
			if (udp_bits.udpb_ipv6_recvhoplimit) {
				struct T_opthdr *toh;

				toh = (struct T_opthdr *)dstopt;
				toh->level = IPPROTO_IPV6;
				toh->name = IPV6_HOPLIMIT;
				toh->len = sizeof (struct T_opthdr) +
				    sizeof (uint_t);
				toh->status = 0;
				dstopt += sizeof (struct T_opthdr);
				if (ipversion == IPV6_VERSION)
					*(uint_t *)dstopt = ip6h->ip6_hops;
				else
					*(uint_t *)dstopt =
					    ((ipha_t *)rptr)->ipha_ttl;
				dstopt += sizeof (uint_t);
				udi_size -= toh->len;
			}
			if (udp_bits.udpb_ipv6_recvtclass) {
				struct T_opthdr *toh;

				toh = (struct T_opthdr *)dstopt;
				toh->level = IPPROTO_IPV6;
				toh->name = IPV6_TCLASS;
				toh->len = sizeof (struct T_opthdr) +
				    sizeof (uint_t);
				toh->status = 0;
				dstopt += sizeof (struct T_opthdr);
				if (ipversion == IPV6_VERSION) {
					*(uint_t *)dstopt =
					    IPV6_FLOW_TCLASS(ip6h->ip6_flow);
				} else {
					ipha_t *ipha = (ipha_t *)rptr;
					*(uint_t *)dstopt =
					    ipha->ipha_type_of_service;
				}
				dstopt += sizeof (uint_t);
				udi_size -= toh->len;
			}
			if ((udp_bits.udpb_ipv6_recvhopopts) &&
			    (ipp.ipp_fields & IPPF_HOPOPTS)) {
				size_t hlen;

				hlen = copy_hop_opts(&ipp, dstopt);
				dstopt += hlen;
				udi_size -= hlen;
			}
			if ((udp_bits.udpb_ipv6_recvdstopts) &&
			    (udp_bits.udpb_ipv6_recvrthdr) &&
			    (ipp.ipp_fields & IPPF_RTHDR) &&
			    (ipp.ipp_fields & IPPF_RTDSTOPTS)) {
				struct T_opthdr *toh;

				toh = (struct T_opthdr *)dstopt;
				toh->level = IPPROTO_IPV6;
				toh->name = IPV6_DSTOPTS;
				toh->len = sizeof (struct T_opthdr) +
				    ipp.ipp_rtdstoptslen;
				toh->status = 0;
				dstopt += sizeof (struct T_opthdr);
				bcopy(ipp.ipp_rtdstopts, dstopt,
				    ipp.ipp_rtdstoptslen);
				dstopt += ipp.ipp_rtdstoptslen;
				udi_size -= toh->len;
			}
			if ((udp_bits.udpb_ipv6_recvrthdr) &&
			    (ipp.ipp_fields & IPPF_RTHDR)) {
				struct T_opthdr *toh;

				toh = (struct T_opthdr *)dstopt;
				toh->level = IPPROTO_IPV6;
				toh->name = IPV6_RTHDR;
				toh->len = sizeof (struct T_opthdr) +
				    ipp.ipp_rthdrlen;
				toh->status = 0;
				dstopt += sizeof (struct T_opthdr);
				bcopy(ipp.ipp_rthdr, dstopt, ipp.ipp_rthdrlen);
				dstopt += ipp.ipp_rthdrlen;
				udi_size -= toh->len;
			}
			if ((udp_bits.udpb_ipv6_recvdstopts) &&
			    (ipp.ipp_fields & IPPF_DSTOPTS)) {
				struct T_opthdr *toh;

				toh = (struct T_opthdr *)dstopt;
				toh->level = IPPROTO_IPV6;
				toh->name = IPV6_DSTOPTS;
				toh->len = sizeof (struct T_opthdr) +
				    ipp.ipp_dstoptslen;
				toh->status = 0;
				dstopt += sizeof (struct T_opthdr);
				bcopy(ipp.ipp_dstopts, dstopt,
				    ipp.ipp_dstoptslen);
				dstopt += ipp.ipp_dstoptslen;
				udi_size -= toh->len;
			}

			if (cr != NULL) {
				struct T_opthdr *toh;

				toh = (struct T_opthdr *)dstopt;
				toh->level = SOL_SOCKET;
				toh->name = SCM_UCRED;
				toh->len = sizeof (struct T_opthdr) + ucredsize;
				toh->status = 0;
				(void) cred2ucred(cr, cpid, &toh[1], rcr);
				dstopt += toh->len;
				udi_size -= toh->len;
			}
			if (udp_bits.udpb_timestamp) {
				struct	T_opthdr *toh;

				toh = (struct T_opthdr *)dstopt;
				toh->level = SOL_SOCKET;
				toh->name = SCM_TIMESTAMP;
				toh->len = sizeof (struct T_opthdr) +
				    sizeof (timestruc_t) + _POINTER_ALIGNMENT;
				toh->status = 0;
				dstopt += sizeof (struct T_opthdr);
				/* Align for gethrestime() */
				dstopt = (uchar_t *)P2ROUNDUP((intptr_t)dstopt,
				    sizeof (intptr_t));
				gethrestime((timestruc_t *)dstopt);
				dstopt = (uchar_t *)toh + toh->len;
				udi_size -= toh->len;
			}

			/* Consumed all of allocated space */
			ASSERT(udi_size == 0);
		}
#undef	sin6
		/* No IP_RECVDSTADDR for IPv6. */
	}

	BUMP_MIB(&us->us_udp_mib, udpHCInDatagrams);
	if (options_mp != NULL)
		freeb(options_mp);

	if (udp_bits.udpb_direct_sockfs) {
		/*
		 * There is nothing above us except for the stream head;
		 * use the read-side synchronous stream interface in
		 * order to reduce the time spent in interrupt thread.
		 */
		ASSERT(udp->udp_issocket);
		udp_rcv_enqueue(connp->conn_rq, udp, mp, mp_len);
	} else {
		/*
		 * Use regular STREAMS interface to pass data upstream
		 * if this is not a socket endpoint, or if we have
		 * switched over to the slow mode due to sockmod being
		 * popped or a module being pushed on top of us.
		 */
		putnext(connp->conn_rq, mp);
	}
	return;

tossit:
	freemsg(mp);
	if (options_mp != NULL)
		freeb(options_mp);
	BUMP_MIB(&us->us_udp_mib, udpInErrors);
}

/*
 * Handle the results of a T_BIND_REQ whether deferred by IP or handled
 * immediately.
 */
static void
udp_bind_result(conn_t *connp, mblk_t *mp)
{
	struct T_error_ack	*tea;

	switch (mp->b_datap->db_type) {
	case M_PROTO:
	case M_PCPROTO:
		/* M_PROTO messages contain some type of TPI message. */
		ASSERT((uintptr_t)(mp->b_wptr - mp->b_rptr) <=
		    (uintptr_t)INT_MAX);
		if (mp->b_wptr - mp->b_rptr < sizeof (t_scalar_t)) {
			freemsg(mp);
			return;
		}
		tea = (struct T_error_ack *)mp->b_rptr;

		switch (tea->PRIM_type) {
		case T_ERROR_ACK:
			switch (tea->ERROR_prim) {
			case O_T_BIND_REQ:
			case T_BIND_REQ:
				udp_bind_error(connp, mp);
				return;
			default:
				break;
			}
			ASSERT(0);
			freemsg(mp);
			return;

		case T_BIND_ACK:
			udp_bind_ack(connp, mp);
			return;

		default:
			break;
		}
		freemsg(mp);
		return;
	default:
		/* FIXME: other cases? */
		ASSERT(0);
		freemsg(mp);
		return;
	}
}

/*
 * Process a T_BIND_ACK
 */
static void
udp_bind_ack(conn_t *connp, mblk_t *mp)
{
	udp_t	*udp = connp->conn_udp;
	mblk_t	*mp1;
	ire_t	*ire;
	struct T_bind_ack *tba;
	uchar_t *addrp;
	ipa_conn_t	*ac;
	ipa6_conn_t	*ac6;
	udp_fanout_t	*udpf;
	udp_stack_t	*us = udp->udp_us;

	ASSERT(udp->udp_pending_op != -1);
	rw_enter(&udp->udp_rwlock, RW_WRITER);
	/*
	 * If a broadcast/multicast address was bound set
	 * the source address to 0.
	 * This ensures no datagrams with broadcast address
	 * as source address are emitted (which would violate
	 * RFC1122 - Hosts requirements)
	 *
	 * Note that when connecting the returned IRE is
	 * for the destination address and we only perform
	 * the broadcast check for the source address (it
	 * is OK to connect to a broadcast/multicast address.)
	 */
	mp1 = mp->b_cont;
	if (mp1 != NULL && mp1->b_datap->db_type == IRE_DB_TYPE) {
		ire = (ire_t *)mp1->b_rptr;

		/*
		 * Note: we get IRE_BROADCAST for IPv6 to "mark" a multicast
		 * local address.
		 */
		udpf = &us->us_bind_fanout[UDP_BIND_HASH(udp->udp_port,
		    us->us_bind_fanout_size)];
		if (ire->ire_type == IRE_BROADCAST &&
		    udp->udp_state != TS_DATA_XFER) {
			ASSERT(udp->udp_pending_op == T_BIND_REQ ||
			    udp->udp_pending_op == O_T_BIND_REQ);
			/* This was just a local bind to a broadcast addr */
			mutex_enter(&udpf->uf_lock);
			V6_SET_ZERO(udp->udp_v6src);
			mutex_exit(&udpf->uf_lock);
			if (udp->udp_family == AF_INET6)
				(void) udp_build_hdrs(udp);
		} else if (V6_OR_V4_INADDR_ANY(udp->udp_v6src)) {
			/*
			 * Local address not yet set - pick it from the
			 * T_bind_ack
			 */
			tba = (struct T_bind_ack *)mp->b_rptr;
			addrp = &mp->b_rptr[tba->ADDR_offset];
			switch (udp->udp_family) {
			case AF_INET:
				if (tba->ADDR_length == sizeof (ipa_conn_t)) {
					ac = (ipa_conn_t *)addrp;
				} else {
					ASSERT(tba->ADDR_length ==
					    sizeof (ipa_conn_x_t));
					ac = &((ipa_conn_x_t *)addrp)->acx_conn;
				}
				mutex_enter(&udpf->uf_lock);
				IN6_IPADDR_TO_V4MAPPED(ac->ac_laddr,
				    &udp->udp_v6src);
				mutex_exit(&udpf->uf_lock);
				break;
			case AF_INET6:
				if (tba->ADDR_length == sizeof (ipa6_conn_t)) {
					ac6 = (ipa6_conn_t *)addrp;
				} else {
					ASSERT(tba->ADDR_length ==
					    sizeof (ipa6_conn_x_t));
					ac6 = &((ipa6_conn_x_t *)
					    addrp)->ac6x_conn;
				}
				mutex_enter(&udpf->uf_lock);
				udp->udp_v6src = ac6->ac6_laddr;
				mutex_exit(&udpf->uf_lock);
				(void) udp_build_hdrs(udp);
				break;
			}
		}
		mp1 = mp1->b_cont;
	}
	udp->udp_pending_op = -1;
	rw_exit(&udp->udp_rwlock);
	/*
	 * Look for one or more appended ACK message added by
	 * udp_connect or udp_disconnect.
	 * If none found just send up the T_BIND_ACK.
	 * udp_connect has appended a T_OK_ACK and a T_CONN_CON.
	 * udp_disconnect has appended a T_OK_ACK.
	 */
	if (mp1 != NULL) {
		if (mp->b_cont == mp1)
			mp->b_cont = NULL;
		else {
			ASSERT(mp->b_cont->b_cont == mp1);
			mp->b_cont->b_cont = NULL;
		}
		freemsg(mp);
		mp = mp1;
		while (mp != NULL) {
			mp1 = mp->b_cont;
			mp->b_cont = NULL;
			putnext(connp->conn_rq, mp);
			mp = mp1;
		}
		return;
	}
	freemsg(mp->b_cont);
	mp->b_cont = NULL;
	putnext(connp->conn_rq, mp);
}

static void
udp_bind_error(conn_t *connp, mblk_t *mp)
{
	udp_t	*udp = connp->conn_udp;
	struct T_error_ack *tea;
	udp_fanout_t	*udpf;
	udp_stack_t	*us = udp->udp_us;

	tea = (struct T_error_ack *)mp->b_rptr;

	/*
	 * If our O_T_BIND_REQ/T_BIND_REQ fails,
	 * clear out the associated port and source
	 * address before passing the message
	 * upstream. If this was caused by a T_CONN_REQ
	 * revert back to bound state.
	 */

	rw_enter(&udp->udp_rwlock, RW_WRITER);
	ASSERT(udp->udp_pending_op != -1);
	tea->ERROR_prim = udp->udp_pending_op;
	udp->udp_pending_op = -1;
	udpf = &us->us_bind_fanout[
	    UDP_BIND_HASH(udp->udp_port,
	    us->us_bind_fanout_size)];
	mutex_enter(&udpf->uf_lock);

	switch (tea->ERROR_prim) {
	case T_CONN_REQ:
		ASSERT(udp->udp_state == TS_DATA_XFER);
		/* Connect failed */
		/* Revert back to the bound source */
		udp->udp_v6src = udp->udp_bound_v6src;
		udp->udp_state = TS_IDLE;
		mutex_exit(&udpf->uf_lock);
		if (udp->udp_family == AF_INET6)
			(void) udp_build_hdrs(udp);
		rw_exit(&udp->udp_rwlock);
		break;

	case T_DISCON_REQ:
	case T_BIND_REQ:
	case O_T_BIND_REQ:
		V6_SET_ZERO(udp->udp_v6src);
		V6_SET_ZERO(udp->udp_bound_v6src);
		udp->udp_state = TS_UNBND;
		udp_bind_hash_remove(udp, B_TRUE);
		udp->udp_port = 0;
		mutex_exit(&udpf->uf_lock);
		if (udp->udp_family == AF_INET6)
			(void) udp_build_hdrs(udp);
		rw_exit(&udp->udp_rwlock);
		break;

	default:
		mutex_exit(&udpf->uf_lock);
		rw_exit(&udp->udp_rwlock);
		(void) mi_strlog(connp->conn_rq, 1,
		    SL_ERROR|SL_TRACE,
		    "udp_input_other: bad ERROR_prim, "
		    "len %d", tea->ERROR_prim);
	}
	putnext(connp->conn_rq, mp);
}

/*
 * return SNMP stuff in buffer in mpdata. We don't hold any lock and report
 * information that can be changing beneath us.
 */
mblk_t *
udp_snmp_get(queue_t *q, mblk_t *mpctl)
{
	mblk_t			*mpdata;
	mblk_t			*mp_conn_ctl;
	mblk_t			*mp_attr_ctl;
	mblk_t			*mp6_conn_ctl;
	mblk_t			*mp6_attr_ctl;
	mblk_t			*mp_conn_tail;
	mblk_t			*mp_attr_tail;
	mblk_t			*mp6_conn_tail;
	mblk_t			*mp6_attr_tail;
	struct opthdr		*optp;
	mib2_udpEntry_t		ude;
	mib2_udp6Entry_t	ude6;
	mib2_transportMLPEntry_t mlp;
	int			state;
	zoneid_t		zoneid;
	int			i;
	connf_t			*connfp;
	conn_t			*connp = Q_TO_CONN(q);
	int			v4_conn_idx;
	int			v6_conn_idx;
	boolean_t		needattr;
	udp_t			*udp;
	ip_stack_t		*ipst = connp->conn_netstack->netstack_ip;
	udp_stack_t		*us = connp->conn_netstack->netstack_udp;
	mblk_t			*mp2ctl;

	/*
	 * make a copy of the original message
	 */
	mp2ctl = copymsg(mpctl);

	mp_conn_ctl = mp_attr_ctl = mp6_conn_ctl = NULL;
	if (mpctl == NULL ||
	    (mpdata = mpctl->b_cont) == NULL ||
	    (mp_conn_ctl = copymsg(mpctl)) == NULL ||
	    (mp_attr_ctl = copymsg(mpctl)) == NULL ||
	    (mp6_conn_ctl = copymsg(mpctl)) == NULL ||
	    (mp6_attr_ctl = copymsg(mpctl)) == NULL) {
		freemsg(mp_conn_ctl);
		freemsg(mp_attr_ctl);
		freemsg(mp6_conn_ctl);
		freemsg(mpctl);
		freemsg(mp2ctl);
		return (0);
	}

	zoneid = connp->conn_zoneid;

	/* fixed length structure for IPv4 and IPv6 counters */
	SET_MIB(us->us_udp_mib.udpEntrySize, sizeof (mib2_udpEntry_t));
	SET_MIB(us->us_udp_mib.udp6EntrySize, sizeof (mib2_udp6Entry_t));
	/* synchronize 64- and 32-bit counters */
	SYNC32_MIB(&us->us_udp_mib, udpInDatagrams, udpHCInDatagrams);
	SYNC32_MIB(&us->us_udp_mib, udpOutDatagrams, udpHCOutDatagrams);

	optp = (struct opthdr *)&mpctl->b_rptr[sizeof (struct T_optmgmt_ack)];
	optp->level = MIB2_UDP;
	optp->name = 0;
	(void) snmp_append_data(mpdata, (char *)&us->us_udp_mib,
	    sizeof (us->us_udp_mib));
	optp->len = msgdsize(mpdata);
	qreply(q, mpctl);

	mp_conn_tail = mp_attr_tail = mp6_conn_tail = mp6_attr_tail = NULL;
	v4_conn_idx = v6_conn_idx = 0;

	for (i = 0; i < CONN_G_HASH_SIZE; i++) {
		connfp = &ipst->ips_ipcl_globalhash_fanout[i];
		connp = NULL;

		while ((connp = ipcl_get_next_conn(connfp, connp,
		    IPCL_UDPCONN))) {
			udp = connp->conn_udp;
			if (zoneid != connp->conn_zoneid)
				continue;

			/*
			 * Note that the port numbers are sent in
			 * host byte order
			 */

			if (udp->udp_state == TS_UNBND)
				state = MIB2_UDP_unbound;
			else if (udp->udp_state == TS_IDLE)
				state = MIB2_UDP_idle;
			else if (udp->udp_state == TS_DATA_XFER)
				state = MIB2_UDP_connected;
			else
				state = MIB2_UDP_unknown;

			needattr = B_FALSE;
			bzero(&mlp, sizeof (mlp));
			if (connp->conn_mlp_type != mlptSingle) {
				if (connp->conn_mlp_type == mlptShared ||
				    connp->conn_mlp_type == mlptBoth)
					mlp.tme_flags |= MIB2_TMEF_SHARED;
				if (connp->conn_mlp_type == mlptPrivate ||
				    connp->conn_mlp_type == mlptBoth)
					mlp.tme_flags |= MIB2_TMEF_PRIVATE;
				needattr = B_TRUE;
			}

			/*
			 * Create an IPv4 table entry for IPv4 entries and also
			 * any IPv6 entries which are bound to in6addr_any
			 * (i.e. anything a IPv4 peer could connect/send to).
			 */
			if (udp->udp_ipversion == IPV4_VERSION ||
			    (udp->udp_state <= TS_IDLE &&
			    IN6_IS_ADDR_UNSPECIFIED(&udp->udp_v6src))) {
				ude.udpEntryInfo.ue_state = state;
				/*
				 * If in6addr_any this will set it to
				 * INADDR_ANY
				 */
				ude.udpLocalAddress =
				    V4_PART_OF_V6(udp->udp_v6src);
				ude.udpLocalPort = ntohs(udp->udp_port);
				if (udp->udp_state == TS_DATA_XFER) {
					/*
					 * Can potentially get here for
					 * v6 socket if another process
					 * (say, ping) has just done a
					 * sendto(), changing the state
					 * from the TS_IDLE above to
					 * TS_DATA_XFER by the time we hit
					 * this part of the code.
					 */
					ude.udpEntryInfo.ue_RemoteAddress =
					    V4_PART_OF_V6(udp->udp_v6dst);
					ude.udpEntryInfo.ue_RemotePort =
					    ntohs(udp->udp_dstport);
				} else {
					ude.udpEntryInfo.ue_RemoteAddress = 0;
					ude.udpEntryInfo.ue_RemotePort = 0;
				}

				/*
				 * We make the assumption that all udp_t
				 * structs will be created within an address
				 * region no larger than 32-bits.
				 */
				ude.udpInstance = (uint32_t)(uintptr_t)udp;
				ude.udpCreationProcess =
				    (udp->udp_open_pid < 0) ?
				    MIB2_UNKNOWN_PROCESS :
				    udp->udp_open_pid;
				ude.udpCreationTime = udp->udp_open_time;

				(void) snmp_append_data2(mp_conn_ctl->b_cont,
				    &mp_conn_tail, (char *)&ude, sizeof (ude));
				mlp.tme_connidx = v4_conn_idx++;
				if (needattr)
					(void) snmp_append_data2(
					    mp_attr_ctl->b_cont, &mp_attr_tail,
					    (char *)&mlp, sizeof (mlp));
			}
			if (udp->udp_ipversion == IPV6_VERSION) {
				ude6.udp6EntryInfo.ue_state  = state;
				ude6.udp6LocalAddress = udp->udp_v6src;
				ude6.udp6LocalPort = ntohs(udp->udp_port);
				ude6.udp6IfIndex = udp->udp_bound_if;
				if (udp->udp_state == TS_DATA_XFER) {
					ude6.udp6EntryInfo.ue_RemoteAddress =
					    udp->udp_v6dst;
					ude6.udp6EntryInfo.ue_RemotePort =
					    ntohs(udp->udp_dstport);
				} else {
					ude6.udp6EntryInfo.ue_RemoteAddress =
					    sin6_null.sin6_addr;
					ude6.udp6EntryInfo.ue_RemotePort = 0;
				}
				/*
				 * We make the assumption that all udp_t
				 * structs will be created within an address
				 * region no larger than 32-bits.
				 */
				ude6.udp6Instance = (uint32_t)(uintptr_t)udp;
				ude6.udp6CreationProcess =
				    (udp->udp_open_pid < 0) ?
				    MIB2_UNKNOWN_PROCESS :
				    udp->udp_open_pid;
				ude6.udp6CreationTime = udp->udp_open_time;

				(void) snmp_append_data2(mp6_conn_ctl->b_cont,
				    &mp6_conn_tail, (char *)&ude6,
				    sizeof (ude6));
				mlp.tme_connidx = v6_conn_idx++;
				if (needattr)
					(void) snmp_append_data2(
					    mp6_attr_ctl->b_cont,
					    &mp6_attr_tail, (char *)&mlp,
					    sizeof (mlp));
			}
		}
	}

	/* IPv4 UDP endpoints */
	optp = (struct opthdr *)&mp_conn_ctl->b_rptr[
	    sizeof (struct T_optmgmt_ack)];
	optp->level = MIB2_UDP;
	optp->name = MIB2_UDP_ENTRY;
	optp->len = msgdsize(mp_conn_ctl->b_cont);
	qreply(q, mp_conn_ctl);

	/* table of MLP attributes... */
	optp = (struct opthdr *)&mp_attr_ctl->b_rptr[
	    sizeof (struct T_optmgmt_ack)];
	optp->level = MIB2_UDP;
	optp->name = EXPER_XPORT_MLP;
	optp->len = msgdsize(mp_attr_ctl->b_cont);
	if (optp->len == 0)
		freemsg(mp_attr_ctl);
	else
		qreply(q, mp_attr_ctl);

	/* IPv6 UDP endpoints */
	optp = (struct opthdr *)&mp6_conn_ctl->b_rptr[
	    sizeof (struct T_optmgmt_ack)];
	optp->level = MIB2_UDP6;
	optp->name = MIB2_UDP6_ENTRY;
	optp->len = msgdsize(mp6_conn_ctl->b_cont);
	qreply(q, mp6_conn_ctl);

	/* table of MLP attributes... */
	optp = (struct opthdr *)&mp6_attr_ctl->b_rptr[
	    sizeof (struct T_optmgmt_ack)];
	optp->level = MIB2_UDP6;
	optp->name = EXPER_XPORT_MLP;
	optp->len = msgdsize(mp6_attr_ctl->b_cont);
	if (optp->len == 0)
		freemsg(mp6_attr_ctl);
	else
		qreply(q, mp6_attr_ctl);

	return (mp2ctl);
}

/*
 * Return 0 if invalid set request, 1 otherwise, including non-udp requests.
 * NOTE: Per MIB-II, UDP has no writable data.
 * TODO:  If this ever actually tries to set anything, it needs to be
 * to do the appropriate locking.
 */
/* ARGSUSED */
int
udp_snmp_set(queue_t *q, t_scalar_t level, t_scalar_t name,
    uchar_t *ptr, int len)
{
	switch (level) {
	case MIB2_UDP:
		return (0);
	default:
		return (1);
	}
}

static void
udp_report_item(mblk_t *mp, udp_t *udp)
{
	char *state;
	char addrbuf1[INET6_ADDRSTRLEN];
	char addrbuf2[INET6_ADDRSTRLEN];
	uint_t print_len, buf_len;

	buf_len = mp->b_datap->db_lim - mp->b_wptr;
	ASSERT(buf_len >= 0);
	if (buf_len == 0)
		return;

	if (udp->udp_state == TS_UNBND)
		state = "UNBOUND";
	else if (udp->udp_state == TS_IDLE)
		state = "IDLE";
	else if (udp->udp_state == TS_DATA_XFER)
		state = "CONNECTED";
	else
		state = "UnkState";
	print_len = snprintf((char *)mp->b_wptr, buf_len,
	    MI_COL_PTRFMT_STR "%4d %5u %s %s %5u %s\n",
	    (void *)udp, udp->udp_connp->conn_zoneid, ntohs(udp->udp_port),
	    inet_ntop(AF_INET6, &udp->udp_v6src, addrbuf1, sizeof (addrbuf1)),
	    inet_ntop(AF_INET6, &udp->udp_v6dst, addrbuf2, sizeof (addrbuf2)),
	    ntohs(udp->udp_dstport), state);
	if (print_len < buf_len) {
		mp->b_wptr += print_len;
	} else {
		mp->b_wptr += buf_len;
	}
}

/* Report for ndd "udp_status" */
/* ARGSUSED */
static int
udp_status_report(queue_t *q, mblk_t *mp, caddr_t cp, cred_t *cr)
{
	zoneid_t zoneid;
	connf_t	*connfp;
	conn_t	*connp = Q_TO_CONN(q);
	udp_t	*udp = connp->conn_udp;
	int	i;
	udp_stack_t *us = udp->udp_us;
	ip_stack_t *ipst = connp->conn_netstack->netstack_ip;

	/*
	 * Because of the ndd constraint, at most we can have 64K buffer
	 * to put in all UDP info.  So to be more efficient, just
	 * allocate a 64K buffer here, assuming we need that large buffer.
	 * This may be a problem as any user can read udp_status.  Therefore
	 * we limit the rate of doing this using us_ndd_get_info_interval.
	 * This should be OK as normal users should not do this too often.
	 */
	if (cr == NULL || secpolicy_ip_config(cr, B_TRUE) != 0) {
		if (ddi_get_lbolt() - us->us_last_ndd_get_info_time <
		    drv_usectohz(us->us_ndd_get_info_interval * 1000)) {
			(void) mi_mpprintf(mp, NDD_TOO_QUICK_MSG);
			return (0);
		}
	}
	if ((mp->b_cont = allocb(ND_MAX_BUF_LEN, BPRI_HI)) == NULL) {
		/* The following may work even if we cannot get a large buf. */
		(void) mi_mpprintf(mp, NDD_OUT_OF_BUF_MSG);
		return (0);
	}
	(void) mi_mpprintf(mp,
	    "UDP     " MI_COL_HDRPAD_STR
	/*   12345678[89ABCDEF] */
	    " zone lport src addr        dest addr       port  state");
	/*    1234 12345 xxx.xxx.xxx.xxx xxx.xxx.xxx.xxx 12345 UNBOUND */

	zoneid = connp->conn_zoneid;

	for (i = 0; i < CONN_G_HASH_SIZE; i++) {
		connfp = &ipst->ips_ipcl_globalhash_fanout[i];
		connp = NULL;

		while ((connp = ipcl_get_next_conn(connfp, connp,
		    IPCL_UDPCONN))) {
			udp = connp->conn_udp;
			if (zoneid != GLOBAL_ZONEID &&
			    zoneid != connp->conn_zoneid)
				continue;

			udp_report_item(mp->b_cont, udp);
		}
	}
	us->us_last_ndd_get_info_time = ddi_get_lbolt();
	return (0);
}

/*
 * This routine creates a T_UDERROR_IND message and passes it upstream.
 * The address and options are copied from the T_UNITDATA_REQ message
 * passed in mp.  This message is freed.
 */
static void
udp_ud_err(queue_t *q, mblk_t *mp, uchar_t *destaddr, t_scalar_t destlen,
    t_scalar_t err)
{
	struct T_unitdata_req *tudr;
	mblk_t	*mp1;
	uchar_t	*optaddr;
	t_scalar_t optlen;

	if (DB_TYPE(mp) == M_DATA) {
		ASSERT(destaddr != NULL && destlen != 0);
		optaddr = NULL;
		optlen = 0;
	} else {
		if ((mp->b_wptr < mp->b_rptr) ||
		    (MBLKL(mp)) < sizeof (struct T_unitdata_req)) {
			goto done;
		}
		tudr = (struct T_unitdata_req *)mp->b_rptr;
		destaddr = mp->b_rptr + tudr->DEST_offset;
		if (destaddr < mp->b_rptr || destaddr >= mp->b_wptr ||
		    destaddr + tudr->DEST_length < mp->b_rptr ||
		    destaddr + tudr->DEST_length > mp->b_wptr) {
			goto done;
		}
		optaddr = mp->b_rptr + tudr->OPT_offset;
		if (optaddr < mp->b_rptr || optaddr >= mp->b_wptr ||
		    optaddr + tudr->OPT_length < mp->b_rptr ||
		    optaddr + tudr->OPT_length > mp->b_wptr) {
			goto done;
		}
		destlen = tudr->DEST_length;
		optlen = tudr->OPT_length;
	}

	mp1 = mi_tpi_uderror_ind((char *)destaddr, destlen,
	    (char *)optaddr, optlen, err);
	if (mp1 != NULL)
		qreply(q, mp1);

done:
	freemsg(mp);
}

/*
 * This routine removes a port number association from a stream.  It
 * is called by udp_wput to handle T_UNBIND_REQ messages.
 */
static void
udp_unbind(queue_t *q, mblk_t *mp)
{
	udp_t *udp = Q_TO_UDP(q);
	udp_fanout_t	*udpf;
	udp_stack_t	*us = udp->udp_us;

	if (cl_inet_unbind != NULL) {
		/*
		 * Running in cluster mode - register unbind information
		 */
		if (udp->udp_ipversion == IPV4_VERSION) {
			(*cl_inet_unbind)(IPPROTO_UDP, AF_INET,
			    (uint8_t *)(&V4_PART_OF_V6(udp->udp_v6src)),
			    (in_port_t)udp->udp_port);
		} else {
			(*cl_inet_unbind)(IPPROTO_UDP, AF_INET6,
			    (uint8_t *)&(udp->udp_v6src),
			    (in_port_t)udp->udp_port);
		}
	}

	rw_enter(&udp->udp_rwlock, RW_WRITER);
	if (udp->udp_state == TS_UNBND || udp->udp_pending_op != -1) {
		rw_exit(&udp->udp_rwlock);
		udp_err_ack(q, mp, TOUTSTATE, 0);
		return;
	}
	udp->udp_pending_op = T_UNBIND_REQ;
	rw_exit(&udp->udp_rwlock);

	/*
	 * Pass the unbind to IP; T_UNBIND_REQ is larger than T_OK_ACK
	 * and therefore ip_unbind must never return NULL.
	 */
	mp = ip_unbind(q, mp);
	ASSERT(mp != NULL);
	ASSERT(((struct T_ok_ack *)mp->b_rptr)->PRIM_type == T_OK_ACK);

	/*
	 * Once we're unbound from IP, the pending operation may be cleared
	 * here.
	 */
	rw_enter(&udp->udp_rwlock, RW_WRITER);
	udpf = &us->us_bind_fanout[UDP_BIND_HASH(udp->udp_port,
	    us->us_bind_fanout_size)];
	mutex_enter(&udpf->uf_lock);
	udp_bind_hash_remove(udp, B_TRUE);
	V6_SET_ZERO(udp->udp_v6src);
	V6_SET_ZERO(udp->udp_bound_v6src);
	udp->udp_port = 0;
	mutex_exit(&udpf->uf_lock);

	udp->udp_pending_op = -1;
	udp->udp_state = TS_UNBND;
	if (udp->udp_family == AF_INET6)
		(void) udp_build_hdrs(udp);
	rw_exit(&udp->udp_rwlock);

	qreply(q, mp);
}

/*
 * Don't let port fall into the privileged range.
 * Since the extra privileged ports can be arbitrary we also
 * ensure that we exclude those from consideration.
 * us->us_epriv_ports is not sorted thus we loop over it until
 * there are no changes.
 */
static in_port_t
udp_update_next_port(udp_t *udp, in_port_t port, boolean_t random)
{
	int i;
	in_port_t nextport;
	boolean_t restart = B_FALSE;
	udp_stack_t *us = udp->udp_us;

	if (random && udp_random_anon_port != 0) {
		(void) random_get_pseudo_bytes((uint8_t *)&port,
		    sizeof (in_port_t));
		/*
		 * Unless changed by a sys admin, the smallest anon port
		 * is 32768 and the largest anon port is 65535.  It is
		 * very likely (50%) for the random port to be smaller
		 * than the smallest anon port.  When that happens,
		 * add port % (anon port range) to the smallest anon
		 * port to get the random port.  It should fall into the
		 * valid anon port range.
		 */
		if (port < us->us_smallest_anon_port) {
			port = us->us_smallest_anon_port +
			    port % (us->us_largest_anon_port -
			    us->us_smallest_anon_port);
		}
	}

retry:
	if (port < us->us_smallest_anon_port)
		port = us->us_smallest_anon_port;

	if (port > us->us_largest_anon_port) {
		port = us->us_smallest_anon_port;
		if (restart)
			return (0);
		restart = B_TRUE;
	}

	if (port < us->us_smallest_nonpriv_port)
		port = us->us_smallest_nonpriv_port;

	for (i = 0; i < us->us_num_epriv_ports; i++) {
		if (port == us->us_epriv_ports[i]) {
			port++;
			/*
			 * Make sure that the port is in the
			 * valid range.
			 */
			goto retry;
		}
	}

	if (is_system_labeled() &&
	    (nextport = tsol_next_port(crgetzone(udp->udp_connp->conn_cred),
	    port, IPPROTO_UDP, B_TRUE)) != 0) {
		port = nextport;
		goto retry;
	}

	return (port);
}

static int
udp_update_label(queue_t *wq, mblk_t *mp, ipaddr_t dst)
{
	int err;
	uchar_t opt_storage[IP_MAX_OPT_LENGTH];
	udp_t *udp = Q_TO_UDP(wq);
	udp_stack_t	*us = udp->udp_us;

	err = tsol_compute_label(DB_CREDDEF(mp, udp->udp_connp->conn_cred), dst,
	    opt_storage, udp->udp_connp->conn_mac_exempt,
	    us->us_netstack->netstack_ip);
	if (err == 0) {
		err = tsol_update_options(&udp->udp_ip_snd_options,
		    &udp->udp_ip_snd_options_len, &udp->udp_label_len,
		    opt_storage);
	}
	if (err != 0) {
		DTRACE_PROBE4(
		    tx__ip__log__info__updatelabel__udp,
		    char *, "queue(1) failed to update options(2) on mp(3)",
		    queue_t *, wq, char *, opt_storage, mblk_t *, mp);
	} else {
		IN6_IPADDR_TO_V4MAPPED(dst, &udp->udp_v6lastdst);
	}
	return (err);
}

static mblk_t *
udp_output_v4(conn_t *connp, mblk_t *mp, ipaddr_t v4dst, uint16_t port,
    uint_t srcid, int *error, boolean_t insert_spi)
{
	udp_t	*udp = connp->conn_udp;
	queue_t	*q = connp->conn_wq;
	mblk_t	*mp1 = mp;
	mblk_t	*mp2;
	ipha_t	*ipha;
	int	ip_hdr_length;
	uint32_t ip_len;
	udpha_t	*udpha;
	boolean_t lock_held = B_FALSE;
	in_port_t	uha_src_port;
	udpattrs_t	attrs;
	uchar_t	ip_snd_opt[IP_MAX_OPT_LENGTH];
	uint32_t	ip_snd_opt_len = 0;
	ip4_pkt_t  pktinfo;
	ip4_pkt_t  *pktinfop = &pktinfo;
	ip_opt_info_t optinfo;
	ip_stack_t	*ipst = connp->conn_netstack->netstack_ip;
	udp_stack_t	*us = udp->udp_us;
	ipsec_stack_t	*ipss = ipst->ips_netstack->netstack_ipsec;


	*error = 0;
	pktinfop->ip4_ill_index = 0;
	pktinfop->ip4_addr = INADDR_ANY;
	optinfo.ip_opt_flags = 0;
	optinfo.ip_opt_ill_index = 0;

	if (v4dst == INADDR_ANY)
		v4dst = htonl(INADDR_LOOPBACK);

	/*
	 * If options passed in, feed it for verification and handling
	 */
	attrs.udpattr_credset = B_FALSE;
	if (DB_TYPE(mp) != M_DATA) {
		mp1 = mp->b_cont;
		if (((struct T_unitdata_req *)mp->b_rptr)->OPT_length != 0) {
			attrs.udpattr_ipp4 = pktinfop;
			attrs.udpattr_mb = mp;
			if (udp_unitdata_opt_process(q, mp, error, &attrs) < 0)
				goto done;
			/*
			 * Note: success in processing options.
			 * mp option buffer represented by
			 * OPT_length/offset now potentially modified
			 * and contain option setting results
			 */
			ASSERT(*error == 0);
		}
	}

	/* mp1 points to the M_DATA mblk carrying the packet */
	ASSERT(mp1 != NULL && DB_TYPE(mp1) == M_DATA);

	rw_enter(&udp->udp_rwlock, RW_READER);
	lock_held = B_TRUE;
	/*
	 * Check if our saved options are valid; update if not.
	 * TSOL Note: Since we are not in WRITER mode, UDP packets
	 * to different destination may require different labels,
	 * or worse, UDP packets to same IP address may require
	 * different labels due to use of shared all-zones address.
	 * We use conn_lock to ensure that lastdst, ip_snd_options,
	 * and ip_snd_options_len are consistent for the current
	 * destination and are updated atomically.
	 */
	mutex_enter(&connp->conn_lock);
	if (is_system_labeled()) {
		/* Using UDP MLP requires SCM_UCRED from user */
		if (connp->conn_mlp_type != mlptSingle &&
		    !attrs.udpattr_credset) {
			mutex_exit(&connp->conn_lock);
			DTRACE_PROBE4(
			    tx__ip__log__info__output__udp,
			    char *, "MLP mp(1) lacks SCM_UCRED attr(2) on q(3)",
			    mblk_t *, mp1, udpattrs_t *, &attrs, queue_t *, q);
			*error = ECONNREFUSED;
			goto done;
		}
		/*
		 * update label option for this UDP socket if
		 * - the destination has changed, or
		 * - the UDP socket is MLP
		 */
		if ((!IN6_IS_ADDR_V4MAPPED(&udp->udp_v6lastdst) ||
		    V4_PART_OF_V6(udp->udp_v6lastdst) != v4dst ||
		    connp->conn_mlp_type != mlptSingle) &&
		    (*error = udp_update_label(q, mp, v4dst)) != 0) {
			mutex_exit(&connp->conn_lock);
			goto done;
		}
	}
	if (udp->udp_ip_snd_options_len > 0) {
		ip_snd_opt_len = udp->udp_ip_snd_options_len;
		bcopy(udp->udp_ip_snd_options, ip_snd_opt, ip_snd_opt_len);
	}
	mutex_exit(&connp->conn_lock);

	/* Add an IP header */
	ip_hdr_length = IP_SIMPLE_HDR_LENGTH + UDPH_SIZE + ip_snd_opt_len +
	    (insert_spi ? sizeof (uint32_t) : 0);
	ipha = (ipha_t *)&mp1->b_rptr[-ip_hdr_length];
	if (DB_REF(mp1) != 1 || (uchar_t *)ipha < DB_BASE(mp1) ||
	    !OK_32PTR(ipha)) {
		mp2 = allocb(ip_hdr_length + us->us_wroff_extra, BPRI_LO);
		if (mp2 == NULL) {
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_END,
			    "udp_wput_end: q %p (%S)", q, "allocbfail2");
			*error = ENOMEM;
			goto done;
		}
		mp2->b_wptr = DB_LIM(mp2);
		mp2->b_cont = mp1;
		mp1 = mp2;
		if (DB_TYPE(mp) != M_DATA)
			mp->b_cont = mp1;
		else
			mp = mp1;

		ipha = (ipha_t *)(mp1->b_wptr - ip_hdr_length);
	}
	ip_hdr_length -= (UDPH_SIZE + (insert_spi ? sizeof (uint32_t) : 0));
#ifdef	_BIG_ENDIAN
	/* Set version, header length, and tos */
	*(uint16_t *)&ipha->ipha_version_and_hdr_length =
	    ((((IP_VERSION << 4) | (ip_hdr_length>>2)) << 8) |
	    udp->udp_type_of_service);
	/* Set ttl and protocol */
	*(uint16_t *)&ipha->ipha_ttl = (udp->udp_ttl << 8) | IPPROTO_UDP;
#else
	/* Set version, header length, and tos */
	*(uint16_t *)&ipha->ipha_version_and_hdr_length =
	    ((udp->udp_type_of_service << 8) |
	    ((IP_VERSION << 4) | (ip_hdr_length>>2)));
	/* Set ttl and protocol */
	*(uint16_t *)&ipha->ipha_ttl = (IPPROTO_UDP << 8) | udp->udp_ttl;
#endif
	if (pktinfop->ip4_addr != INADDR_ANY) {
		ipha->ipha_src = pktinfop->ip4_addr;
		optinfo.ip_opt_flags = IP_VERIFY_SRC;
	} else {
		/*
		 * Copy our address into the packet.  If this is zero,
		 * first look at __sin6_src_id for a hint. If we leave the
		 * source as INADDR_ANY then ip will fill in the real source
		 * address.
		 */
		IN6_V4MAPPED_TO_IPADDR(&udp->udp_v6src, ipha->ipha_src);
		if (srcid != 0 && ipha->ipha_src == INADDR_ANY) {
			in6_addr_t v6src;

			ip_srcid_find_id(srcid, &v6src, connp->conn_zoneid,
			    us->us_netstack);
			IN6_V4MAPPED_TO_IPADDR(&v6src, ipha->ipha_src);
		}
	}
	uha_src_port = udp->udp_port;
	if (ip_hdr_length == IP_SIMPLE_HDR_LENGTH) {
		rw_exit(&udp->udp_rwlock);
		lock_held = B_FALSE;
	}

	if (pktinfop->ip4_ill_index != 0) {
		optinfo.ip_opt_ill_index = pktinfop->ip4_ill_index;
	}

	ipha->ipha_fragment_offset_and_flags = 0;
	ipha->ipha_ident = 0;

	mp1->b_rptr = (uchar_t *)ipha;

	ASSERT((uintptr_t)(mp1->b_wptr - (uchar_t *)ipha) <=
	    (uintptr_t)UINT_MAX);

	/* Determine length of packet */
	ip_len = (uint32_t)(mp1->b_wptr - (uchar_t *)ipha);
	if ((mp2 = mp1->b_cont) != NULL) {
		do {
			ASSERT((uintptr_t)MBLKL(mp2) <= (uintptr_t)UINT_MAX);
			ip_len += (uint32_t)MBLKL(mp2);
		} while ((mp2 = mp2->b_cont) != NULL);
	}
	/*
	 * If the size of the packet is greater than the maximum allowed by
	 * ip, return an error. Passing this down could cause panics because
	 * the size will have wrapped and be inconsistent with the msg size.
	 */
	if (ip_len > IP_MAXPACKET) {
		TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_END,
		    "udp_wput_end: q %p (%S)", q, "IP length exceeded");
		*error = EMSGSIZE;
		goto done;
	}
	ipha->ipha_length = htons((uint16_t)ip_len);
	ip_len -= ip_hdr_length;
	ip_len = htons((uint16_t)ip_len);
	udpha = (udpha_t *)(((uchar_t *)ipha) + ip_hdr_length);

	/* Insert all-0s SPI now. */
	if (insert_spi)
		*((uint32_t *)(udpha + 1)) = 0;

	/*
	 * Copy in the destination address
	 */
	ipha->ipha_dst = v4dst;

	/*
	 * Set ttl based on IP_MULTICAST_TTL to match IPv6 logic.
	 */
	if (CLASSD(v4dst))
		ipha->ipha_ttl = udp->udp_multicast_ttl;

	udpha->uha_dst_port = port;
	udpha->uha_src_port = uha_src_port;

	if (ip_snd_opt_len > 0) {
		uint32_t	cksum;

		bcopy(ip_snd_opt, &ipha[1], ip_snd_opt_len);
		lock_held = B_FALSE;
		rw_exit(&udp->udp_rwlock);
		/*
		 * Massage source route putting first source route in ipha_dst.
		 * Ignore the destination in T_unitdata_req.
		 * Create a checksum adjustment for a source route, if any.
		 */
		cksum = ip_massage_options(ipha, us->us_netstack);
		cksum = (cksum & 0xFFFF) + (cksum >> 16);
		cksum -= ((ipha->ipha_dst >> 16) & 0xFFFF) +
		    (ipha->ipha_dst & 0xFFFF);
		if ((int)cksum < 0)
			cksum--;
		cksum = (cksum & 0xFFFF) + (cksum >> 16);
		/*
		 * IP does the checksum if uha_checksum is non-zero,
		 * We make it easy for IP to include our pseudo header
		 * by putting our length in uha_checksum.
		 */
		cksum += ip_len;
		cksum = (cksum & 0xFFFF) + (cksum >> 16);
		/* There might be a carry. */
		cksum = (cksum & 0xFFFF) + (cksum >> 16);
#ifdef _LITTLE_ENDIAN
		if (us->us_do_checksum)
			ip_len = (cksum << 16) | ip_len;
#else
		if (us->us_do_checksum)
			ip_len = (ip_len << 16) | cksum;
		else
			ip_len <<= 16;
#endif
	} else {
		/*
		 * IP does the checksum if uha_checksum is non-zero,
		 * We make it easy for IP to include our pseudo header
		 * by putting our length in uha_checksum.
		 */
		if (us->us_do_checksum)
			ip_len |= (ip_len << 16);
#ifndef _LITTLE_ENDIAN
		else
			ip_len <<= 16;
#endif
	}
	ASSERT(!lock_held);
	/* Set UDP length and checksum */
	*((uint32_t *)&udpha->uha_length) = ip_len;
	if (DB_CRED(mp) != NULL)
		mblk_setcred(mp1, DB_CRED(mp));

	if (DB_TYPE(mp) != M_DATA) {
		ASSERT(mp != mp1);
		freeb(mp);
	}

	/* mp has been consumed and we'll return success */
	ASSERT(*error == 0);
	mp = NULL;

	/* We're done.  Pass the packet to ip. */
	BUMP_MIB(&us->us_udp_mib, udpHCOutDatagrams);
	TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_END,
	    "udp_wput_end: q %p (%S)", q, "end");

	if ((connp->conn_flags & IPCL_CHECK_POLICY) != 0 ||
	    CONN_OUTBOUND_POLICY_PRESENT(connp, ipss) ||
	    connp->conn_dontroute ||
	    connp->conn_nofailover_ill != NULL ||
	    connp->conn_outgoing_ill != NULL || optinfo.ip_opt_flags != 0 ||
	    optinfo.ip_opt_ill_index != 0 ||
	    ipha->ipha_version_and_hdr_length != IP_SIMPLE_HDR_VERSION ||
	    IPP_ENABLED(IPP_LOCAL_OUT, ipst) ||
	    ipst->ips_ip_g_mrouter != NULL) {
		UDP_STAT(us, udp_ip_send);
		ip_output_options(connp, mp1, connp->conn_wq, IP_WPUT,
		    &optinfo);
	} else {
		udp_send_data(udp, connp->conn_wq, mp1, ipha);
	}

done:
	if (lock_held)
		rw_exit(&udp->udp_rwlock);
	if (*error != 0) {
		ASSERT(mp != NULL);
		BUMP_MIB(&us->us_udp_mib, udpOutErrors);
	}
	return (mp);
}

static void
udp_send_data(udp_t *udp, queue_t *q, mblk_t *mp, ipha_t *ipha)
{
	conn_t	*connp = udp->udp_connp;
	ipaddr_t src, dst;
	ire_t	*ire;
	ipif_t	*ipif = NULL;
	mblk_t	*ire_fp_mp;
	boolean_t retry_caching;
	udp_stack_t *us = udp->udp_us;
	ip_stack_t	*ipst = connp->conn_netstack->netstack_ip;

	dst = ipha->ipha_dst;
	src = ipha->ipha_src;
	ASSERT(ipha->ipha_ident == 0);

	if (CLASSD(dst)) {
		int err;

		ipif = conn_get_held_ipif(connp,
		    &connp->conn_multicast_ipif, &err);

		if (ipif == NULL || ipif->ipif_isv6 ||
		    (ipif->ipif_ill->ill_phyint->phyint_flags &
		    PHYI_LOOPBACK)) {
			if (ipif != NULL)
				ipif_refrele(ipif);
			UDP_STAT(us, udp_ip_send);
			ip_output(connp, mp, q, IP_WPUT);
			return;
		}
	}

	retry_caching = B_FALSE;
	mutex_enter(&connp->conn_lock);
	ire = connp->conn_ire_cache;
	ASSERT(!(connp->conn_state_flags & CONN_INCIPIENT));

	if (ire == NULL || ire->ire_addr != dst ||
	    (ire->ire_marks & IRE_MARK_CONDEMNED)) {
		retry_caching = B_TRUE;
	} else if (CLASSD(dst) && (ire->ire_type & IRE_CACHE)) {
		ill_t *stq_ill = (ill_t *)ire->ire_stq->q_ptr;

		ASSERT(ipif != NULL);
		if (stq_ill != ipif->ipif_ill && (stq_ill->ill_group == NULL ||
		    stq_ill->ill_group != ipif->ipif_ill->ill_group))
			retry_caching = B_TRUE;
	}

	if (!retry_caching) {
		ASSERT(ire != NULL);
		IRE_REFHOLD(ire);
		mutex_exit(&connp->conn_lock);
	} else {
		boolean_t cached = B_FALSE;

		connp->conn_ire_cache = NULL;
		mutex_exit(&connp->conn_lock);

		/* Release the old ire */
		if (ire != NULL) {
			IRE_REFRELE_NOTR(ire);
			ire = NULL;
		}

		if (CLASSD(dst)) {
			ASSERT(ipif != NULL);
			ire = ire_ctable_lookup(dst, 0, 0, ipif,
			    connp->conn_zoneid, MBLK_GETLABEL(mp),
			    MATCH_IRE_ILL_GROUP, ipst);
		} else {
			ASSERT(ipif == NULL);
			ire = ire_cache_lookup(dst, connp->conn_zoneid,
			    MBLK_GETLABEL(mp), ipst);
		}

		if (ire == NULL) {
			if (ipif != NULL)
				ipif_refrele(ipif);
			UDP_STAT(us, udp_ire_null);
			ip_output(connp, mp, q, IP_WPUT);
			return;
		}
		IRE_REFHOLD_NOTR(ire);

		mutex_enter(&connp->conn_lock);
		if (CONN_CACHE_IRE(connp) && connp->conn_ire_cache == NULL &&
		    !(ire->ire_marks & IRE_MARK_CONDEMNED)) {
			irb_t		*irb = ire->ire_bucket;

			/*
			 * IRE's created for non-connection oriented transports
			 * are normally initialized with IRE_MARK_TEMPORARY set
			 * in the ire_marks. These IRE's are preferentially
			 * reaped when the hash chain length in the cache
			 * bucket exceeds the maximum value specified in
			 * ip[6]_ire_max_bucket_cnt. This can severely affect
			 * UDP performance if IRE cache entries that we need
			 * to reuse are continually removed. To remedy this,
			 * when we cache the IRE in the conn_t, we remove the
			 * IRE_MARK_TEMPORARY bit from the ire_marks if it was
			 * set.
			 */
			if (ire->ire_marks & IRE_MARK_TEMPORARY) {
				rw_enter(&irb->irb_lock, RW_WRITER);
				if (ire->ire_marks & IRE_MARK_TEMPORARY) {
					ire->ire_marks &= ~IRE_MARK_TEMPORARY;
					irb->irb_tmp_ire_cnt--;
				}
				rw_exit(&irb->irb_lock);
			}
			connp->conn_ire_cache = ire;
			cached = B_TRUE;
		}
		mutex_exit(&connp->conn_lock);

		/*
		 * We can continue to use the ire but since it was not
		 * cached, we should drop the extra reference.
		 */
		if (!cached)
			IRE_REFRELE_NOTR(ire);
	}
	ASSERT(ire != NULL && ire->ire_ipversion == IPV4_VERSION);
	ASSERT(!CLASSD(dst) || ipif != NULL);

	/*
	 * Check if we can take the fast-path.
	 * Note that "incomplete" ire's (where the link-layer for next hop
	 * is not resolved, or where the fast-path header in nce_fp_mp is not
	 * available yet) are sent down the legacy (slow) path
	 */
	if ((ire->ire_type & (IRE_BROADCAST|IRE_LOCAL|IRE_LOOPBACK)) ||
	    (ire->ire_flags & RTF_MULTIRT) || (ire->ire_stq == NULL) ||
	    (ire->ire_max_frag < ntohs(ipha->ipha_length)) ||
	    ((ire->ire_nce == NULL) ||
	    ((ire_fp_mp = ire->ire_nce->nce_fp_mp) == NULL)) ||
	    connp->conn_nexthop_set || (MBLKL(ire_fp_mp) > MBLKHEAD(mp))) {
		if (ipif != NULL)
			ipif_refrele(ipif);
		UDP_STAT(us, udp_ip_ire_send);
		IRE_REFRELE(ire);
		ip_output(connp, mp, q, IP_WPUT);
		return;
	}

	if (src == INADDR_ANY && !connp->conn_unspec_src) {
		if (CLASSD(dst) && !(ire->ire_flags & RTF_SETSRC))
			ipha->ipha_src = ipif->ipif_src_addr;
		else
			ipha->ipha_src = ire->ire_src_addr;
	}

	if (ipif != NULL)
		ipif_refrele(ipif);

	udp_xmit(connp->conn_wq, mp, ire, connp, connp->conn_zoneid);
}

static void
udp_xmit(queue_t *q, mblk_t *mp, ire_t *ire, conn_t *connp, zoneid_t zoneid)
{
	ipaddr_t src, dst;
	ill_t	*ill;
	mblk_t	*ire_fp_mp;
	uint_t	ire_fp_mp_len;
	uint16_t *up;
	uint32_t cksum, hcksum_txflags;
	queue_t	*dev_q;
	udp_t	*udp = connp->conn_udp;
	ipha_t	*ipha = (ipha_t *)mp->b_rptr;
	udp_stack_t	*us = udp->udp_us;
	ip_stack_t	*ipst = connp->conn_netstack->netstack_ip;
	boolean_t ll_multicast = B_FALSE;

	dev_q = ire->ire_stq->q_next;
	ASSERT(dev_q != NULL);


	if (DEV_Q_IS_FLOW_CTLED(dev_q)) {
		BUMP_MIB(&ipst->ips_ip_mib, ipIfStatsHCOutRequests);
		BUMP_MIB(&ipst->ips_ip_mib, ipIfStatsOutDiscards);
		if (ipst->ips_ip_output_queue)
			(void) putq(connp->conn_wq, mp);
		else
			freemsg(mp);
		ire_refrele(ire);
		return;
	}

	ire_fp_mp = ire->ire_nce->nce_fp_mp;
	ire_fp_mp_len = MBLKL(ire_fp_mp);
	ASSERT(MBLKHEAD(mp) >= ire_fp_mp_len);

	dst = ipha->ipha_dst;
	src = ipha->ipha_src;

	ill = ire_to_ill(ire);
	ASSERT(ill != NULL);

	BUMP_MIB(ill->ill_ip_mib, ipIfStatsHCOutRequests);

	ipha->ipha_ident = (uint16_t)atomic_add_32_nv(&ire->ire_ident, 1);
#ifndef _BIG_ENDIAN
	ipha->ipha_ident = (ipha->ipha_ident << 8) | (ipha->ipha_ident >> 8);
#endif

	if (ILL_HCKSUM_CAPABLE(ill) && dohwcksum) {
		ASSERT(ill->ill_hcksum_capab != NULL);
		hcksum_txflags = ill->ill_hcksum_capab->ill_hcksum_txflags;
	} else {
		hcksum_txflags = 0;
	}

	/* pseudo-header checksum (do it in parts for IP header checksum) */
	cksum = (dst >> 16) + (dst & 0xFFFF) + (src >> 16) + (src & 0xFFFF);

	ASSERT(ipha->ipha_version_and_hdr_length == IP_SIMPLE_HDR_VERSION);
	up = IPH_UDPH_CHECKSUMP(ipha, IP_SIMPLE_HDR_LENGTH);
	if (*up != 0) {
		IP_CKSUM_XMIT_FAST(ire->ire_ipversion, hcksum_txflags,
		    mp, ipha, up, IPPROTO_UDP, IP_SIMPLE_HDR_LENGTH,
		    ntohs(ipha->ipha_length), cksum);

		/* Software checksum? */
		if (DB_CKSUMFLAGS(mp) == 0) {
			UDP_STAT(us, udp_out_sw_cksum);
			UDP_STAT_UPDATE(us, udp_out_sw_cksum_bytes,
			    ntohs(ipha->ipha_length) - IP_SIMPLE_HDR_LENGTH);
		}
	}

	if (!CLASSD(dst)) {
		ipha->ipha_fragment_offset_and_flags |=
		    (uint32_t)htons(ire->ire_frag_flag);
	}

	/* Calculate IP header checksum if hardware isn't capable */
	if (!(DB_CKSUMFLAGS(mp) & HCK_IPV4_HDRCKSUM)) {
		IP_HDR_CKSUM(ipha, cksum, ((uint32_t *)ipha)[0],
		    ((uint16_t *)ipha)[4]);
	}

	if (CLASSD(dst)) {
		boolean_t ilm_exists;

		ILM_WALKER_HOLD(ill);
		ilm_exists = (ilm_lookup_ill(ill, dst, ALL_ZONES) != NULL);
		ILM_WALKER_RELE(ill);
		if (ilm_exists) {
			ip_multicast_loopback(q, ill, mp,
			    connp->conn_multicast_loop ? 0 :
			    IP_FF_NO_MCAST_LOOP, zoneid);
		}

		/* If multicast TTL is 0 then we are done */
		if (ipha->ipha_ttl == 0) {
			freemsg(mp);
			ire_refrele(ire);
			return;
		}
		ll_multicast = B_TRUE;
	}

	ASSERT(DB_TYPE(ire_fp_mp) == M_DATA);
	mp->b_rptr = (uchar_t *)ipha - ire_fp_mp_len;
	bcopy(ire_fp_mp->b_rptr, mp->b_rptr, ire_fp_mp_len);

	UPDATE_OB_PKT_COUNT(ire);
	ire->ire_last_used_time = lbolt;

	BUMP_MIB(ill->ill_ip_mib, ipIfStatsHCOutTransmits);
	UPDATE_MIB(ill->ill_ip_mib, ipIfStatsHCOutOctets,
	    ntohs(ipha->ipha_length));

	if (ILL_DLS_CAPABLE(ill)) {
		/*
		 * Send the packet directly to DLD, where it may be queued
		 * depending on the availability of transmit resources at
		 * the media layer.
		 */
		IP_DLS_ILL_TX(ill, ipha, mp, ipst);
	} else {
		DTRACE_PROBE4(ip4__physical__out__start,
		    ill_t *, NULL, ill_t *, ill,
		    ipha_t *, ipha, mblk_t *, mp);
		FW_HOOKS(ipst->ips_ip4_physical_out_event,
		    ipst->ips_ipv4firewall_physical_out,
		    NULL, ill, ipha, mp, mp, ll_multicast, ipst);
		DTRACE_PROBE1(ip4__physical__out__end, mblk_t *, mp);
		if (mp != NULL) {
			DTRACE_IP7(send, mblk_t *, mp, conn_t *, NULL,
			    void_ip_t *, ipha, __dtrace_ipsr_ill_t *, ill,
			    ipha_t *, ipha, ip6_t *, NULL, int, 0);
			putnext(ire->ire_stq, mp);
		}
	}

	IRE_REFRELE(ire);
}

static boolean_t
udp_update_label_v6(queue_t *wq, mblk_t *mp, in6_addr_t *dst)
{
	udp_t *udp = Q_TO_UDP(wq);
	int err;
	uchar_t opt_storage[TSOL_MAX_IPV6_OPTION];
	udp_stack_t		*us = udp->udp_us;

	err = tsol_compute_label_v6(DB_CREDDEF(mp, udp->udp_connp->conn_cred),
	    dst, opt_storage, udp->udp_connp->conn_mac_exempt,
	    us->us_netstack->netstack_ip);
	if (err == 0) {
		err = tsol_update_sticky(&udp->udp_sticky_ipp,
		    &udp->udp_label_len_v6, opt_storage);
	}
	if (err != 0) {
		DTRACE_PROBE4(
		    tx__ip__log__drop__updatelabel__udp6,
		    char *, "queue(1) failed to update options(2) on mp(3)",
		    queue_t *, wq, char *, opt_storage, mblk_t *, mp);
	} else {
		udp->udp_v6lastdst = *dst;
	}
	return (err);
}

void
udp_output_connected(void *arg, mblk_t *mp)
{
	conn_t	*connp = (conn_t *)arg;
	udp_t	*udp = connp->conn_udp;
	udp_stack_t	*us = udp->udp_us;
	ipaddr_t	v4dst;
	in_port_t	dstport;
	boolean_t	mapped_addr;
	struct sockaddr_storage ss;
	sin_t		*sin;
	sin6_t		*sin6;
	struct sockaddr	*addr;
	socklen_t	addrlen;
	int		error;
	boolean_t	insert_spi = udp->udp_nat_t_endpoint;

	/* M_DATA for connected socket */

	ASSERT(udp->udp_issocket);
	UDP_DBGSTAT(us, udp_data_conn);

	mutex_enter(&connp->conn_lock);
	if (udp->udp_state != TS_DATA_XFER) {
		mutex_exit(&connp->conn_lock);
		BUMP_MIB(&us->us_udp_mib, udpOutErrors);
		UDP_STAT(us, udp_out_err_notconn);
		freemsg(mp);
		TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_END,
		    "udp_wput_end: connp %p (%S)", connp,
		    "not-connected; address required");
		return;
	}

	mapped_addr = IN6_IS_ADDR_V4MAPPED(&udp->udp_v6dst);
	if (mapped_addr)
		IN6_V4MAPPED_TO_IPADDR(&udp->udp_v6dst, v4dst);

	/* Initialize addr and addrlen as if they're passed in */
	if (udp->udp_family == AF_INET) {
		sin = (sin_t *)&ss;
		sin->sin_family = AF_INET;
		dstport = sin->sin_port = udp->udp_dstport;
		ASSERT(mapped_addr);
		sin->sin_addr.s_addr = v4dst;
		addr = (struct sockaddr *)sin;
		addrlen = sizeof (*sin);
	} else {
		sin6 = (sin6_t *)&ss;
		sin6->sin6_family = AF_INET6;
		dstport = sin6->sin6_port = udp->udp_dstport;
		sin6->sin6_flowinfo = udp->udp_flowinfo;
		sin6->sin6_addr = udp->udp_v6dst;
		sin6->sin6_scope_id = 0;
		sin6->__sin6_src_id = 0;
		addr = (struct sockaddr *)sin6;
		addrlen = sizeof (*sin6);
	}
	mutex_exit(&connp->conn_lock);

	if (mapped_addr) {
		/*
		 * Handle both AF_INET and AF_INET6; the latter
		 * for IPV4 mapped destination addresses.  Note
		 * here that both addr and addrlen point to the
		 * corresponding struct depending on the address
		 * family of the socket.
		 */
		mp = udp_output_v4(connp, mp, v4dst, dstport, 0, &error,
		    insert_spi);
	} else {
		mp = udp_output_v6(connp, mp, sin6, &error);
	}
	if (error == 0) {
		ASSERT(mp == NULL);
		return;
	}

	UDP_STAT(us, udp_out_err_output);
	ASSERT(mp != NULL);
	/* mp is freed by the following routine */
	udp_ud_err(connp->conn_wq, mp, (uchar_t *)addr, (t_scalar_t)addrlen,
	    (t_scalar_t)error);
}

/*
 * This routine handles all messages passed downstream.  It either
 * consumes the message or passes it downstream; it never queues a
 * a message.
 *
 * Also entry point for sockfs when udp is in "direct sockfs" mode.  This mode
 * is valid when we are directly beneath the stream head, and thus sockfs
 * is able to bypass STREAMS and directly call us, passing along the sockaddr
 * structure without the cumbersome T_UNITDATA_REQ interface for the case of
 * connected endpoints.
 */
void
udp_wput(queue_t *q, mblk_t *mp)
{
	sin6_t		*sin6;
	sin_t		*sin;
	ipaddr_t	v4dst;
	uint16_t	port;
	uint_t		srcid;
	conn_t		*connp = Q_TO_CONN(q);
	udp_t		*udp = connp->conn_udp;
	int		error = 0;
	struct sockaddr	*addr;
	socklen_t	addrlen;
	udp_stack_t *us = udp->udp_us;
	boolean_t	insert_spi = udp->udp_nat_t_endpoint;

	TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_START,
	    "udp_wput_start: queue %p mp %p", q, mp);

	/*
	 * We directly handle several cases here: T_UNITDATA_REQ message
	 * coming down as M_PROTO/M_PCPROTO and M_DATA messages for connected
	 * socket.
	 */
	switch (DB_TYPE(mp)) {
	case M_DATA:
		/*
		 * Quick check for error cases. Checks will be done again
		 * under the lock later on
		 */
		if (!udp->udp_direct_sockfs || udp->udp_state != TS_DATA_XFER) {
			/* Not connected; address is required */
			BUMP_MIB(&us->us_udp_mib, udpOutErrors);
			UDP_STAT(us, udp_out_err_notconn);
			freemsg(mp);
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_END,
			    "udp_wput_end: connp %p (%S)", connp,
			    "not-connected; address required");
			return;
		}
		udp_output_connected(connp, mp);
		return;

	case M_PROTO:
	case M_PCPROTO: {
		struct T_unitdata_req *tudr;

		ASSERT((uintptr_t)MBLKL(mp) <= (uintptr_t)INT_MAX);
		tudr = (struct T_unitdata_req *)mp->b_rptr;

		/* Handle valid T_UNITDATA_REQ here */
		if (MBLKL(mp) >= sizeof (*tudr) &&
		    ((t_primp_t)mp->b_rptr)->type == T_UNITDATA_REQ) {
			if (mp->b_cont == NULL) {
				TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_END,
				    "udp_wput_end: q %p (%S)", q, "badaddr");
				error = EPROTO;
				goto ud_error;
			}

			if (!MBLKIN(mp, 0, tudr->DEST_offset +
			    tudr->DEST_length)) {
				TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_END,
				    "udp_wput_end: q %p (%S)", q, "badaddr");
				error = EADDRNOTAVAIL;
				goto ud_error;
			}
			/*
			 * If a port has not been bound to the stream, fail.
			 * This is not a problem when sockfs is directly
			 * above us, because it will ensure that the socket
			 * is first bound before allowing data to be sent.
			 */
			if (udp->udp_state == TS_UNBND) {
				TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_END,
				    "udp_wput_end: q %p (%S)", q, "outstate");
				error = EPROTO;
				goto ud_error;
			}
			addr = (struct sockaddr *)
			    &mp->b_rptr[tudr->DEST_offset];
			addrlen = tudr->DEST_length;
			if (tudr->OPT_length != 0)
				UDP_STAT(us, udp_out_opt);
			break;
		}
		/* FALLTHRU */
	}
	default:
		udp_wput_other(q, mp);
		return;
	}
	ASSERT(addr != NULL);

	switch (udp->udp_family) {
	case AF_INET6:
		sin6 = (sin6_t *)addr;
		if (!OK_32PTR((char *)sin6) || (addrlen != sizeof (sin6_t)) ||
		    (sin6->sin6_family != AF_INET6)) {
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_END,
			    "udp_wput_end: q %p (%S)", q, "badaddr");
			error = EADDRNOTAVAIL;
			goto ud_error;
		}

		if (!IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
			/*
			 * Destination is a non-IPv4-compatible IPv6 address.
			 * Send out an IPv6 format packet.
			 */
			mp = udp_output_v6(connp, mp, sin6, &error);
			if (error != 0)
				goto ud_error;

			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_END,
			    "udp_wput_end: q %p (%S)", q, "udp_output_v6");
			return;
		}
		/*
		 * If the local address is not zero or a mapped address
		 * return an error.  It would be possible to send an IPv4
		 * packet but the response would never make it back to the
		 * application since it is bound to a non-mapped address.
		 */
		if (!IN6_IS_ADDR_V4MAPPED(&udp->udp_v6src) &&
		    !IN6_IS_ADDR_UNSPECIFIED(&udp->udp_v6src)) {
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_END,
			    "udp_wput_end: q %p (%S)", q, "badaddr");
			error = EADDRNOTAVAIL;
			goto ud_error;
		}
		/* Send IPv4 packet without modifying udp_ipversion */
		/* Extract port and ipaddr */
		port = sin6->sin6_port;
		IN6_V4MAPPED_TO_IPADDR(&sin6->sin6_addr, v4dst);
		srcid = sin6->__sin6_src_id;
		break;

	case AF_INET:
		sin = (sin_t *)addr;
		if ((!OK_32PTR((char *)sin) || addrlen != sizeof (sin_t)) ||
		    (sin->sin_family != AF_INET)) {
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_END,
			    "udp_wput_end: q %p (%S)", q, "badaddr");
			error = EADDRNOTAVAIL;
			goto ud_error;
		}
		/* Extract port and ipaddr */
		port = sin->sin_port;
		v4dst = sin->sin_addr.s_addr;
		srcid = 0;
		break;
	}

	mp = udp_output_v4(connp, mp, v4dst, port, srcid, &error, insert_spi);
	if (error != 0) {
ud_error:
		UDP_STAT(us, udp_out_err_output);
		ASSERT(mp != NULL);
		/* mp is freed by the following routine */
		udp_ud_err(q, mp, (uchar_t *)addr, (t_scalar_t)addrlen,
		    (t_scalar_t)error);
	}
}

/*
 * udp_output_v6():
 * Assumes that udp_wput did some sanity checking on the destination
 * address.
 */
static mblk_t *
udp_output_v6(conn_t *connp, mblk_t *mp, sin6_t *sin6, int *error)
{
	ip6_t		*ip6h;
	ip6i_t		*ip6i;	/* mp1->b_rptr even if no ip6i_t */
	mblk_t		*mp1 = mp;
	mblk_t		*mp2;
	int		udp_ip_hdr_len = IPV6_HDR_LEN + UDPH_SIZE;
	size_t		ip_len;
	udpha_t		*udph;
	udp_t		*udp = connp->conn_udp;
	queue_t		*q = connp->conn_wq;
	ip6_pkt_t	ipp_s;	/* For ancillary data options */
	ip6_pkt_t	*ipp = &ipp_s;
	ip6_pkt_t	*tipp;	/* temporary ipp */
	uint32_t	csum = 0;
	uint_t		ignore = 0;
	uint_t		option_exists = 0, is_sticky = 0;
	uint8_t		*cp;
	uint8_t		*nxthdr_ptr;
	in6_addr_t	ip6_dst;
	udpattrs_t	attrs;
	boolean_t	opt_present;
	ip6_hbh_t	*hopoptsptr = NULL;
	uint_t		hopoptslen = 0;
	boolean_t	is_ancillary = B_FALSE;
	udp_stack_t	*us = udp->udp_us;
	size_t		sth_wroff = 0;

	*error = 0;

	/*
	 * If the local address is a mapped address return
	 * an error.
	 * It would be possible to send an IPv6 packet but the
	 * response would never make it back to the application
	 * since it is bound to a mapped address.
	 */
	if (IN6_IS_ADDR_V4MAPPED(&udp->udp_v6src)) {
		*error = EADDRNOTAVAIL;
		goto done;
	}

	ipp->ipp_fields = 0;
	ipp->ipp_sticky_ignored = 0;

	/*
	 * If TPI options passed in, feed it for verification and handling
	 */
	attrs.udpattr_credset = B_FALSE;
	opt_present = B_FALSE;
	if (DB_TYPE(mp) != M_DATA) {
		mp1 = mp->b_cont;
		if (((struct T_unitdata_req *)mp->b_rptr)->OPT_length != 0) {
			attrs.udpattr_ipp6 = ipp;
			attrs.udpattr_mb = mp;
			if (udp_unitdata_opt_process(q, mp, error,
			    &attrs) < 0) {
				goto done;
			}
			ASSERT(*error == 0);
			opt_present = B_TRUE;
		}
	}
	rw_enter(&udp->udp_rwlock, RW_READER);
	ignore = ipp->ipp_sticky_ignored;

	/* mp1 points to the M_DATA mblk carrying the packet */
	ASSERT(mp1 != NULL && DB_TYPE(mp1) == M_DATA);

	if (sin6->sin6_scope_id != 0 &&
	    IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
		/*
		 * IPPF_SCOPE_ID is special.  It's neither a sticky
		 * option nor ancillary data.  It needs to be
		 * explicitly set in options_exists.
		 */
		option_exists |= IPPF_SCOPE_ID;
	}

	/*
	 * Compute the destination address
	 */
	ip6_dst = sin6->sin6_addr;
	if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr))
		ip6_dst = ipv6_loopback;

	/*
	 * If we're not going to the same destination as last time, then
	 * recompute the label required.  This is done in a separate routine to
	 * avoid blowing up our stack here.
	 *
	 * TSOL Note: Since we are not in WRITER mode, UDP packets
	 * to different destination may require different labels,
	 * or worse, UDP packets to same IP address may require
	 * different labels due to use of shared all-zones address.
	 * We use conn_lock to ensure that lastdst, sticky ipp_hopopts,
	 * and sticky ipp_hopoptslen are consistent for the current
	 * destination and are updated atomically.
	 */
	mutex_enter(&connp->conn_lock);
	if (is_system_labeled()) {
		/* Using UDP MLP requires SCM_UCRED from user */
		if (connp->conn_mlp_type != mlptSingle &&
		    !attrs.udpattr_credset) {
			DTRACE_PROBE4(
			    tx__ip__log__info__output__udp6,
			    char *, "MLP mp(1) lacks SCM_UCRED attr(2) on q(3)",
			    mblk_t *, mp1, udpattrs_t *, &attrs, queue_t *, q);
			*error = ECONNREFUSED;
			rw_exit(&udp->udp_rwlock);
			mutex_exit(&connp->conn_lock);
			goto done;
		}
		/*
		 * update label option for this UDP socket if
		 * - the destination has changed, or
		 * - the UDP socket is MLP
		 */
		if ((opt_present ||
		    !IN6_ARE_ADDR_EQUAL(&udp->udp_v6lastdst, &ip6_dst) ||
		    connp->conn_mlp_type != mlptSingle) &&
		    (*error = udp_update_label_v6(q, mp, &ip6_dst)) != 0) {
			rw_exit(&udp->udp_rwlock);
			mutex_exit(&connp->conn_lock);
			goto done;
		}
	}

	/*
	 * If there's a security label here, then we ignore any options the
	 * user may try to set.  We keep the peer's label as a hidden sticky
	 * option. We make a private copy of this label before releasing the
	 * lock so that label is kept consistent with the destination addr.
	 */
	if (udp->udp_label_len_v6 > 0) {
		ignore &= ~IPPF_HOPOPTS;
		ipp->ipp_fields &= ~IPPF_HOPOPTS;
	}

	if ((udp->udp_sticky_ipp.ipp_fields == 0) && (ipp->ipp_fields == 0)) {
		/* No sticky options nor ancillary data. */
		mutex_exit(&connp->conn_lock);
		goto no_options;
	}

	/*
	 * Go through the options figuring out where each is going to
	 * come from and build two masks.  The first mask indicates if
	 * the option exists at all.  The second mask indicates if the
	 * option is sticky or ancillary.
	 */
	if (!(ignore & IPPF_HOPOPTS)) {
		if (ipp->ipp_fields & IPPF_HOPOPTS) {
			option_exists |= IPPF_HOPOPTS;
			udp_ip_hdr_len += ipp->ipp_hopoptslen;
		} else if (udp->udp_sticky_ipp.ipp_fields & IPPF_HOPOPTS) {
			option_exists |= IPPF_HOPOPTS;
			is_sticky |= IPPF_HOPOPTS;
			ASSERT(udp->udp_sticky_ipp.ipp_hopoptslen != 0);
			hopoptsptr = kmem_alloc(
			    udp->udp_sticky_ipp.ipp_hopoptslen, KM_NOSLEEP);
			if (hopoptsptr == NULL) {
				*error = ENOMEM;
				mutex_exit(&connp->conn_lock);
				goto done;
			}
			hopoptslen = udp->udp_sticky_ipp.ipp_hopoptslen;
			bcopy(udp->udp_sticky_ipp.ipp_hopopts, hopoptsptr,
			    hopoptslen);
			udp_ip_hdr_len += hopoptslen;
		}
	}
	mutex_exit(&connp->conn_lock);

	if (!(ignore & IPPF_RTHDR)) {
		if (ipp->ipp_fields & IPPF_RTHDR) {
			option_exists |= IPPF_RTHDR;
			udp_ip_hdr_len += ipp->ipp_rthdrlen;
		} else if (udp->udp_sticky_ipp.ipp_fields & IPPF_RTHDR) {
			option_exists |= IPPF_RTHDR;
			is_sticky |= IPPF_RTHDR;
			udp_ip_hdr_len += udp->udp_sticky_ipp.ipp_rthdrlen;
		}
	}

	if (!(ignore & IPPF_RTDSTOPTS) && (option_exists & IPPF_RTHDR)) {
		if (ipp->ipp_fields & IPPF_RTDSTOPTS) {
			option_exists |= IPPF_RTDSTOPTS;
			udp_ip_hdr_len += ipp->ipp_rtdstoptslen;
		} else if (udp->udp_sticky_ipp.ipp_fields & IPPF_RTDSTOPTS) {
			option_exists |= IPPF_RTDSTOPTS;
			is_sticky |= IPPF_RTDSTOPTS;
			udp_ip_hdr_len += udp->udp_sticky_ipp.ipp_rtdstoptslen;
		}
	}

	if (!(ignore & IPPF_DSTOPTS)) {
		if (ipp->ipp_fields & IPPF_DSTOPTS) {
			option_exists |= IPPF_DSTOPTS;
			udp_ip_hdr_len += ipp->ipp_dstoptslen;
		} else if (udp->udp_sticky_ipp.ipp_fields & IPPF_DSTOPTS) {
			option_exists |= IPPF_DSTOPTS;
			is_sticky |= IPPF_DSTOPTS;
			udp_ip_hdr_len += udp->udp_sticky_ipp.ipp_dstoptslen;
		}
	}

	if (!(ignore & IPPF_IFINDEX)) {
		if (ipp->ipp_fields & IPPF_IFINDEX) {
			option_exists |= IPPF_IFINDEX;
		} else if (udp->udp_sticky_ipp.ipp_fields & IPPF_IFINDEX) {
			option_exists |= IPPF_IFINDEX;
			is_sticky |= IPPF_IFINDEX;
		}
	}

	if (!(ignore & IPPF_ADDR)) {
		if (ipp->ipp_fields & IPPF_ADDR) {
			option_exists |= IPPF_ADDR;
		} else if (udp->udp_sticky_ipp.ipp_fields & IPPF_ADDR) {
			option_exists |= IPPF_ADDR;
			is_sticky |= IPPF_ADDR;
		}
	}

	if (!(ignore & IPPF_DONTFRAG)) {
		if (ipp->ipp_fields & IPPF_DONTFRAG) {
			option_exists |= IPPF_DONTFRAG;
		} else if (udp->udp_sticky_ipp.ipp_fields & IPPF_DONTFRAG) {
			option_exists |= IPPF_DONTFRAG;
			is_sticky |= IPPF_DONTFRAG;
		}
	}

	if (!(ignore & IPPF_USE_MIN_MTU)) {
		if (ipp->ipp_fields & IPPF_USE_MIN_MTU) {
			option_exists |= IPPF_USE_MIN_MTU;
		} else if (udp->udp_sticky_ipp.ipp_fields &
		    IPPF_USE_MIN_MTU) {
			option_exists |= IPPF_USE_MIN_MTU;
			is_sticky |= IPPF_USE_MIN_MTU;
		}
	}

	if (!(ignore & IPPF_HOPLIMIT) && (ipp->ipp_fields & IPPF_HOPLIMIT))
		option_exists |= IPPF_HOPLIMIT;
	/* IPV6_HOPLIMIT can never be sticky */
	ASSERT(!(udp->udp_sticky_ipp.ipp_fields & IPPF_HOPLIMIT));

	if (!(ignore & IPPF_UNICAST_HOPS) &&
	    (udp->udp_sticky_ipp.ipp_fields & IPPF_UNICAST_HOPS)) {
		option_exists |= IPPF_UNICAST_HOPS;
		is_sticky |= IPPF_UNICAST_HOPS;
	}

	if (!(ignore & IPPF_MULTICAST_HOPS) &&
	    (udp->udp_sticky_ipp.ipp_fields & IPPF_MULTICAST_HOPS)) {
		option_exists |= IPPF_MULTICAST_HOPS;
		is_sticky |= IPPF_MULTICAST_HOPS;
	}

	if (!(ignore & IPPF_TCLASS)) {
		if (ipp->ipp_fields & IPPF_TCLASS) {
			option_exists |= IPPF_TCLASS;
		} else if (udp->udp_sticky_ipp.ipp_fields & IPPF_TCLASS) {
			option_exists |= IPPF_TCLASS;
			is_sticky |= IPPF_TCLASS;
		}
	}

	if (!(ignore & IPPF_NEXTHOP) &&
	    (udp->udp_sticky_ipp.ipp_fields & IPPF_NEXTHOP)) {
		option_exists |= IPPF_NEXTHOP;
		is_sticky |= IPPF_NEXTHOP;
	}

no_options:

	/*
	 * If any options carried in the ip6i_t were specified, we
	 * need to account for the ip6i_t in the data we'll be sending
	 * down.
	 */
	if (option_exists & IPPF_HAS_IP6I)
		udp_ip_hdr_len += sizeof (ip6i_t);

	/* check/fix buffer config, setup pointers into it */
	ip6h = (ip6_t *)&mp1->b_rptr[-udp_ip_hdr_len];
	if (DB_REF(mp1) != 1 || ((unsigned char *)ip6h < DB_BASE(mp1)) ||
	    !OK_32PTR(ip6h)) {

		/* Try to get everything in a single mblk next time */
		if (udp_ip_hdr_len > udp->udp_max_hdr_len) {
			udp->udp_max_hdr_len = udp_ip_hdr_len;
			sth_wroff = udp->udp_max_hdr_len + us->us_wroff_extra;
		}

		mp2 = allocb(udp_ip_hdr_len + us->us_wroff_extra, BPRI_LO);
		if (mp2 == NULL) {
			*error = ENOMEM;
			rw_exit(&udp->udp_rwlock);
			goto done;
		}
		mp2->b_wptr = DB_LIM(mp2);
		mp2->b_cont = mp1;
		mp1 = mp2;
		if (DB_TYPE(mp) != M_DATA)
			mp->b_cont = mp1;
		else
			mp = mp1;

		ip6h = (ip6_t *)(mp1->b_wptr - udp_ip_hdr_len);
	}
	mp1->b_rptr = (unsigned char *)ip6h;
	ip6i = (ip6i_t *)ip6h;

#define	ANCIL_OR_STICKY_PTR(f) ((is_sticky & f) ? &udp->udp_sticky_ipp : ipp)
	if (option_exists & IPPF_HAS_IP6I) {
		ip6h = (ip6_t *)&ip6i[1];
		ip6i->ip6i_flags = 0;
		ip6i->ip6i_vcf = IPV6_DEFAULT_VERS_AND_FLOW;

		/* sin6_scope_id takes precendence over IPPF_IFINDEX */
		if (option_exists & IPPF_SCOPE_ID) {
			ip6i->ip6i_flags |= IP6I_IFINDEX;
			ip6i->ip6i_ifindex = sin6->sin6_scope_id;
		} else if (option_exists & IPPF_IFINDEX) {
			tipp = ANCIL_OR_STICKY_PTR(IPPF_IFINDEX);
			ASSERT(tipp->ipp_ifindex != 0);
			ip6i->ip6i_flags |= IP6I_IFINDEX;
			ip6i->ip6i_ifindex = tipp->ipp_ifindex;
		}

		if (option_exists & IPPF_ADDR) {
			/*
			 * Enable per-packet source address verification if
			 * IPV6_PKTINFO specified the source address.
			 * ip6_src is set in the transport's _wput function.
			 */
			ip6i->ip6i_flags |= IP6I_VERIFY_SRC;
		}

		if (option_exists & IPPF_DONTFRAG) {
			ip6i->ip6i_flags |= IP6I_DONTFRAG;
		}

		if (option_exists & IPPF_USE_MIN_MTU) {
			ip6i->ip6i_flags = IP6I_API_USE_MIN_MTU(
			    ip6i->ip6i_flags, ipp->ipp_use_min_mtu);
		}

		if (option_exists & IPPF_NEXTHOP) {
			tipp = ANCIL_OR_STICKY_PTR(IPPF_NEXTHOP);
			ASSERT(!IN6_IS_ADDR_UNSPECIFIED(&tipp->ipp_nexthop));
			ip6i->ip6i_flags |= IP6I_NEXTHOP;
			ip6i->ip6i_nexthop = tipp->ipp_nexthop;
		}

		/*
		 * tell IP this is an ip6i_t private header
		 */
		ip6i->ip6i_nxt = IPPROTO_RAW;
	}

	/* Initialize IPv6 header */
	ip6h->ip6_vcf = IPV6_DEFAULT_VERS_AND_FLOW;
	bzero(&ip6h->ip6_src, sizeof (ip6h->ip6_src));

	/* Set the hoplimit of the outgoing packet. */
	if (option_exists & IPPF_HOPLIMIT) {
		/* IPV6_HOPLIMIT ancillary data overrides all other settings. */
		ip6h->ip6_hops = ipp->ipp_hoplimit;
		ip6i->ip6i_flags |= IP6I_HOPLIMIT;
	} else if (IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr)) {
		ip6h->ip6_hops = udp->udp_multicast_ttl;
		if (option_exists & IPPF_MULTICAST_HOPS)
			ip6i->ip6i_flags |= IP6I_HOPLIMIT;
	} else {
		ip6h->ip6_hops = udp->udp_ttl;
		if (option_exists & IPPF_UNICAST_HOPS)
			ip6i->ip6i_flags |= IP6I_HOPLIMIT;
	}

	if (option_exists & IPPF_ADDR) {
		tipp = ANCIL_OR_STICKY_PTR(IPPF_ADDR);
		ASSERT(!IN6_IS_ADDR_UNSPECIFIED(&tipp->ipp_addr));
		ip6h->ip6_src = tipp->ipp_addr;
	} else {
		/*
		 * The source address was not set using IPV6_PKTINFO.
		 * First look at the bound source.
		 * If unspecified fallback to __sin6_src_id.
		 */
		ip6h->ip6_src = udp->udp_v6src;
		if (sin6->__sin6_src_id != 0 &&
		    IN6_IS_ADDR_UNSPECIFIED(&ip6h->ip6_src)) {
			ip_srcid_find_id(sin6->__sin6_src_id,
			    &ip6h->ip6_src, connp->conn_zoneid,
			    us->us_netstack);
		}
	}

	nxthdr_ptr = (uint8_t *)&ip6h->ip6_nxt;
	cp = (uint8_t *)&ip6h[1];

	/*
	 * Here's where we have to start stringing together
	 * any extension headers in the right order:
	 * Hop-by-hop, destination, routing, and final destination opts.
	 */
	if (option_exists & IPPF_HOPOPTS) {
		/* Hop-by-hop options */
		ip6_hbh_t *hbh = (ip6_hbh_t *)cp;
		tipp = ANCIL_OR_STICKY_PTR(IPPF_HOPOPTS);
		if (hopoptslen == 0) {
			hopoptsptr = tipp->ipp_hopopts;
			hopoptslen = tipp->ipp_hopoptslen;
			is_ancillary = B_TRUE;
		}

		*nxthdr_ptr = IPPROTO_HOPOPTS;
		nxthdr_ptr = &hbh->ip6h_nxt;

		bcopy(hopoptsptr, cp, hopoptslen);
		cp += hopoptslen;

		if (hopoptsptr != NULL && !is_ancillary) {
			kmem_free(hopoptsptr, hopoptslen);
			hopoptsptr = NULL;
			hopoptslen = 0;
		}
	}
	/*
	 * En-route destination options
	 * Only do them if there's a routing header as well
	 */
	if (option_exists & IPPF_RTDSTOPTS) {
		ip6_dest_t *dst = (ip6_dest_t *)cp;
		tipp = ANCIL_OR_STICKY_PTR(IPPF_RTDSTOPTS);

		*nxthdr_ptr = IPPROTO_DSTOPTS;
		nxthdr_ptr = &dst->ip6d_nxt;

		bcopy(tipp->ipp_rtdstopts, cp, tipp->ipp_rtdstoptslen);
		cp += tipp->ipp_rtdstoptslen;
	}
	/*
	 * Routing header next
	 */
	if (option_exists & IPPF_RTHDR) {
		ip6_rthdr_t *rt = (ip6_rthdr_t *)cp;
		tipp = ANCIL_OR_STICKY_PTR(IPPF_RTHDR);

		*nxthdr_ptr = IPPROTO_ROUTING;
		nxthdr_ptr = &rt->ip6r_nxt;

		bcopy(tipp->ipp_rthdr, cp, tipp->ipp_rthdrlen);
		cp += tipp->ipp_rthdrlen;
	}
	/*
	 * Do ultimate destination options
	 */
	if (option_exists & IPPF_DSTOPTS) {
		ip6_dest_t *dest = (ip6_dest_t *)cp;
		tipp = ANCIL_OR_STICKY_PTR(IPPF_DSTOPTS);

		*nxthdr_ptr = IPPROTO_DSTOPTS;
		nxthdr_ptr = &dest->ip6d_nxt;

		bcopy(tipp->ipp_dstopts, cp, tipp->ipp_dstoptslen);
		cp += tipp->ipp_dstoptslen;
	}
	/*
	 * Now set the last header pointer to the proto passed in
	 */
	ASSERT((int)(cp - (uint8_t *)ip6i) == (udp_ip_hdr_len - UDPH_SIZE));
	*nxthdr_ptr = IPPROTO_UDP;

	/* Update UDP header */
	udph = (udpha_t *)((uchar_t *)ip6i + udp_ip_hdr_len - UDPH_SIZE);
	udph->uha_dst_port = sin6->sin6_port;
	udph->uha_src_port = udp->udp_port;

	/*
	 * Copy in the destination address
	 */
	ip6h->ip6_dst = ip6_dst;

	ip6h->ip6_vcf =
	    (IPV6_DEFAULT_VERS_AND_FLOW & IPV6_VERS_AND_FLOW_MASK) |
	    (sin6->sin6_flowinfo & ~IPV6_VERS_AND_FLOW_MASK);

	if (option_exists & IPPF_TCLASS) {
		tipp = ANCIL_OR_STICKY_PTR(IPPF_TCLASS);
		ip6h->ip6_vcf = IPV6_TCLASS_FLOW(ip6h->ip6_vcf,
		    tipp->ipp_tclass);
	}
	rw_exit(&udp->udp_rwlock);

	if (option_exists & IPPF_RTHDR) {
		ip6_rthdr_t	*rth;

		/*
		 * Perform any processing needed for source routing.
		 * We know that all extension headers will be in the same mblk
		 * as the IPv6 header.
		 */
		rth = ip_find_rthdr_v6(ip6h, mp1->b_wptr);
		if (rth != NULL && rth->ip6r_segleft != 0) {
			if (rth->ip6r_type != IPV6_RTHDR_TYPE_0) {
				/*
				 * Drop packet - only support Type 0 routing.
				 * Notify the application as well.
				 */
				*error = EPROTO;
				goto done;
			}

			/*
			 * rth->ip6r_len is twice the number of
			 * addresses in the header. Thus it must be even.
			 */
			if (rth->ip6r_len & 0x1) {
				*error = EPROTO;
				goto done;
			}
			/*
			 * Shuffle the routing header and ip6_dst
			 * addresses, and get the checksum difference
			 * between the first hop (in ip6_dst) and
			 * the destination (in the last routing hdr entry).
			 */
			csum = ip_massage_options_v6(ip6h, rth,
			    us->us_netstack);
			/*
			 * Verify that the first hop isn't a mapped address.
			 * Routers along the path need to do this verification
			 * for subsequent hops.
			 */
			if (IN6_IS_ADDR_V4MAPPED(&ip6h->ip6_dst)) {
				*error = EADDRNOTAVAIL;
				goto done;
			}

			cp += (rth->ip6r_len + 1)*8;
		}
	}

	/* count up length of UDP packet */
	ip_len = (mp1->b_wptr - (unsigned char *)ip6h) - IPV6_HDR_LEN;
	if ((mp2 = mp1->b_cont) != NULL) {
		do {
			ASSERT((uintptr_t)MBLKL(mp2) <= (uintptr_t)UINT_MAX);
			ip_len += (uint32_t)MBLKL(mp2);
		} while ((mp2 = mp2->b_cont) != NULL);
	}

	/*
	 * If the size of the packet is greater than the maximum allowed by
	 * ip, return an error. Passing this down could cause panics because
	 * the size will have wrapped and be inconsistent with the msg size.
	 */
	if (ip_len > IP_MAXPACKET) {
		*error = EMSGSIZE;
		goto done;
	}

	/* Store the UDP length. Subtract length of extension hdrs */
	udph->uha_length = htons(ip_len + IPV6_HDR_LEN -
	    (int)((uchar_t *)udph - (uchar_t *)ip6h));

	/*
	 * We make it easy for IP to include our pseudo header
	 * by putting our length in uh_checksum, modified (if
	 * we have a routing header) by the checksum difference
	 * between the ultimate destination and first hop addresses.
	 * Note: UDP over IPv6 must always checksum the packet.
	 */
	csum += udph->uha_length;
	csum = (csum & 0xFFFF) + (csum >> 16);
	udph->uha_checksum = (uint16_t)csum;

#ifdef _LITTLE_ENDIAN
	ip_len = htons(ip_len);
#endif
	ip6h->ip6_plen = ip_len;
	if (DB_CRED(mp) != NULL)
		mblk_setcred(mp1, DB_CRED(mp));

	if (DB_TYPE(mp) != M_DATA) {
		ASSERT(mp != mp1);
		freeb(mp);
	}

	/* mp has been consumed and we'll return success */
	ASSERT(*error == 0);
	mp = NULL;

	/* We're done. Pass the packet to IP */
	BUMP_MIB(&us->us_udp_mib, udpHCOutDatagrams);
	ip_output_v6(connp, mp1, q, IP_WPUT);

done:
	if (sth_wroff != 0) {
		(void) mi_set_sth_wroff(RD(q),
		    udp->udp_max_hdr_len + us->us_wroff_extra);
	}
	if (hopoptsptr != NULL && !is_ancillary) {
		kmem_free(hopoptsptr, hopoptslen);
		hopoptsptr = NULL;
	}
	if (*error != 0) {
		ASSERT(mp != NULL);
		BUMP_MIB(&us->us_udp_mib, udpOutErrors);
	}
	return (mp);
}


static int
udp_getpeername(udp_t *udp, struct sockaddr *sa, uint_t *salenp)
{
	sin_t *sin = (sin_t *)sa;
	sin6_t *sin6 = (sin6_t *)sa;

	ASSERT(RW_LOCK_HELD(&udp->udp_rwlock));

	if (udp->udp_state != TS_DATA_XFER)
		return (ENOTCONN);

	switch (udp->udp_family) {
	case AF_INET:
		ASSERT(udp->udp_ipversion == IPV4_VERSION);

		if (*salenp < sizeof (sin_t))
			return (EINVAL);

		*salenp = sizeof (sin_t);
		*sin = sin_null;
		sin->sin_family = AF_INET;
		sin->sin_port = udp->udp_dstport;
		sin->sin_addr.s_addr = V4_PART_OF_V6(udp->udp_v6dst);
		break;

	case AF_INET6:
		if (*salenp < sizeof (sin6_t))
			return (EINVAL);

		*salenp = sizeof (sin6_t);
		*sin6 = sin6_null;
		sin6->sin6_family = AF_INET6;
		sin6->sin6_port = udp->udp_dstport;
		sin6->sin6_addr = udp->udp_v6dst;
		sin6->sin6_flowinfo = udp->udp_flowinfo;
		break;
	}

	return (0);
}

static int
udp_getmyname(udp_t *udp, struct sockaddr *sa, uint_t *salenp)
{
	sin_t *sin = (sin_t *)sa;
	sin6_t *sin6 = (sin6_t *)sa;

	ASSERT(RW_LOCK_HELD(&udp->udp_rwlock));

	switch (udp->udp_family) {
	case AF_INET:
		ASSERT(udp->udp_ipversion == IPV4_VERSION);

		if (*salenp < sizeof (sin_t))
			return (EINVAL);

		*salenp = sizeof (sin_t);
		*sin = sin_null;
		sin->sin_family = AF_INET;
		sin->sin_port = udp->udp_port;

		/*
		 * If udp_v6src is unspecified, we might be bound to broadcast
		 * / multicast.  Use udp_bound_v6src as local address instead
		 * (that could also still be unspecified).
		 */
		if (!IN6_IS_ADDR_V4MAPPED_ANY(&udp->udp_v6src) &&
		    !IN6_IS_ADDR_UNSPECIFIED(&udp->udp_v6src)) {
			sin->sin_addr.s_addr = V4_PART_OF_V6(udp->udp_v6src);
		} else {
			sin->sin_addr.s_addr =
			    V4_PART_OF_V6(udp->udp_bound_v6src);
		}
		break;

	case AF_INET6:
		if (*salenp < sizeof (sin6_t))
			return (EINVAL);

		*salenp = sizeof (sin6_t);
		*sin6 = sin6_null;
		sin6->sin6_family = AF_INET6;
		sin6->sin6_port = udp->udp_port;
		sin6->sin6_flowinfo = udp->udp_flowinfo;

		/*
		 * If udp_v6src is unspecified, we might be bound to broadcast
		 * / multicast.  Use udp_bound_v6src as local address instead
		 * (that could also still be unspecified).
		 */
		if (!IN6_IS_ADDR_UNSPECIFIED(&udp->udp_v6src))
			sin6->sin6_addr = udp->udp_v6src;
		else
			sin6->sin6_addr = udp->udp_bound_v6src;
		break;
	}

	return (0);
}

/*
 * Handle special out-of-band ioctl requests (see PSARC/2008/265).
 */
static void
udp_wput_cmdblk(queue_t *q, mblk_t *mp)
{
	void	*data;
	mblk_t	*datamp = mp->b_cont;
	udp_t	*udp = Q_TO_UDP(q);
	cmdblk_t *cmdp = (cmdblk_t *)mp->b_rptr;

	if (datamp == NULL || MBLKL(datamp) < cmdp->cb_len) {
		cmdp->cb_error = EPROTO;
		qreply(q, mp);
		return;
	}
	data = datamp->b_rptr;

	rw_enter(&udp->udp_rwlock, RW_READER);
	switch (cmdp->cb_cmd) {
	case TI_GETPEERNAME:
		cmdp->cb_error = udp_getpeername(udp, data, &cmdp->cb_len);
		break;
	case TI_GETMYNAME:
		cmdp->cb_error = udp_getmyname(udp, data, &cmdp->cb_len);
		break;
	default:
		cmdp->cb_error = EINVAL;
		break;
	}
	rw_exit(&udp->udp_rwlock);

	qreply(q, mp);
}

static void
udp_wput_other(queue_t *q, mblk_t *mp)
{
	uchar_t	*rptr = mp->b_rptr;
	struct datab *db;
	struct iocblk *iocp;
	cred_t	*cr;
	conn_t	*connp = Q_TO_CONN(q);
	udp_t	*udp = connp->conn_udp;
	udp_stack_t *us;

	TRACE_1(TR_FAC_UDP, TR_UDP_WPUT_OTHER_START,
	    "udp_wput_other_start: q %p", q);

	us = udp->udp_us;
	db = mp->b_datap;

	cr = DB_CREDDEF(mp, connp->conn_cred);

	switch (db->db_type) {
	case M_CMD:
		udp_wput_cmdblk(q, mp);
		return;

	case M_PROTO:
	case M_PCPROTO:
		if (mp->b_wptr - rptr < sizeof (t_scalar_t)) {
			freemsg(mp);
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
			    "udp_wput_other_end: q %p (%S)", q, "protoshort");
			return;
		}
		switch (((t_primp_t)rptr)->type) {
		case T_ADDR_REQ:
			udp_addr_req(q, mp);
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
			    "udp_wput_other_end: q %p (%S)", q, "addrreq");
			return;
		case O_T_BIND_REQ:
		case T_BIND_REQ:
			udp_bind(q, mp);
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
			    "udp_wput_other_end: q %p (%S)", q, "bindreq");
			return;
		case T_CONN_REQ:
			udp_connect(q, mp);
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
			    "udp_wput_other_end: q %p (%S)", q, "connreq");
			return;
		case T_CAPABILITY_REQ:
			udp_capability_req(q, mp);
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
			    "udp_wput_other_end: q %p (%S)", q, "capabreq");
			return;
		case T_INFO_REQ:
			udp_info_req(q, mp);
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
			    "udp_wput_other_end: q %p (%S)", q, "inforeq");
			return;
		case T_UNITDATA_REQ:
			/*
			 * If a T_UNITDATA_REQ gets here, the address must
			 * be bad.  Valid T_UNITDATA_REQs are handled
			 * in udp_wput.
			 */
			udp_ud_err(q, mp, NULL, 0, EADDRNOTAVAIL);
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
			    "udp_wput_other_end: q %p (%S)", q, "unitdatareq");
			return;
		case T_UNBIND_REQ:
			udp_unbind(q, mp);
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
			    "udp_wput_other_end: q %p (%S)", q, "unbindreq");
			return;
		case T_SVR4_OPTMGMT_REQ:
			if (!snmpcom_req(q, mp, udp_snmp_set, ip_snmp_get,
			    cr)) {
				(void) svr4_optcom_req(q,
				    mp, cr, &udp_opt_obj, B_TRUE);
			}
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
			    "udp_wput_other_end: q %p (%S)", q, "optmgmtreq");
			return;

		case T_OPTMGMT_REQ:
			(void) tpi_optcom_req(q, mp, cr, &udp_opt_obj, B_TRUE);
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
			    "udp_wput_other_end: q %p (%S)", q, "optmgmtreq");
			return;

		case T_DISCON_REQ:
			udp_disconnect(q, mp);
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
			    "udp_wput_other_end: q %p (%S)", q, "disconreq");
			return;

		/* The following TPI message is not supported by udp. */
		case O_T_CONN_RES:
		case T_CONN_RES:
			udp_err_ack(q, mp, TNOTSUPPORT, 0);
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
			    "udp_wput_other_end: q %p (%S)", q,
			    "connres/disconreq");
			return;

		/* The following 3 TPI messages are illegal for udp. */
		case T_DATA_REQ:
		case T_EXDATA_REQ:
		case T_ORDREL_REQ:
			udp_err_ack(q, mp, TNOTSUPPORT, 0);
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
			    "udp_wput_other_end: q %p (%S)", q,
			    "data/exdata/ordrel");
			return;
		default:
			break;
		}
		break;
	case M_FLUSH:
		if (*rptr & FLUSHW)
			flushq(q, FLUSHDATA);
		break;
	case M_IOCTL:
		iocp = (struct iocblk *)mp->b_rptr;
		switch (iocp->ioc_cmd) {
		case TI_GETPEERNAME:
			if (udp->udp_state != TS_DATA_XFER) {
				/*
				 * If a default destination address has not
				 * been associated with the stream, then we
				 * don't know the peer's name.
				 */
				iocp->ioc_error = ENOTCONN;
				iocp->ioc_count = 0;
				mp->b_datap->db_type = M_IOCACK;
				qreply(q, mp);
				TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
				    "udp_wput_other_end: q %p (%S)", q,
				    "getpeername");
				return;
			}
			/* FALLTHRU */
		case TI_GETMYNAME: {
			/*
			 * For TI_GETPEERNAME and TI_GETMYNAME, we first
			 * need to copyin the user's strbuf structure.
			 * Processing will continue in the M_IOCDATA case
			 * below.
			 */
			mi_copyin(q, mp, NULL,
			    SIZEOF_STRUCT(strbuf, iocp->ioc_flag));
			TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
			    "udp_wput_other_end: q %p (%S)", q, "getmyname");
			return;
			}
		case ND_SET:
			/* nd_getset performs the necessary checking */
		case ND_GET:
			if (nd_getset(q, us->us_nd, mp)) {
				qreply(q, mp);
				TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
				    "udp_wput_other_end: q %p (%S)", q, "get");
				return;
			}
			break;
		case _SIOCSOCKFALLBACK:
			/*
			 * Either sockmod is about to be popped and the
			 * socket would now be treated as a plain stream,
			 * or a module is about to be pushed so we could
			 * no longer use read-side synchronous stream.
			 * Drain any queued data and disable direct sockfs
			 * interface from now on.
			 */
			if (!udp->udp_issocket) {
				DB_TYPE(mp) = M_IOCNAK;
				iocp->ioc_error = EINVAL;
			} else {
				udp->udp_issocket = B_FALSE;
				if (udp->udp_direct_sockfs) {
					/*
					 * Disable read-side synchronous
					 * stream interface and drain any
					 * queued data.
					 */
					udp_rcv_drain(RD(q), udp,
					    B_FALSE);
					ASSERT(!udp->udp_direct_sockfs);
					UDP_STAT(us, udp_sock_fallback);
				}
				DB_TYPE(mp) = M_IOCACK;
				iocp->ioc_error = 0;
			}
			iocp->ioc_count = 0;
			iocp->ioc_rval = 0;
			qreply(q, mp);
			return;
		default:
			break;
		}
		break;
	case M_IOCDATA:
		udp_wput_iocdata(q, mp);
		TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
		    "udp_wput_other_end: q %p (%S)", q, "iocdata");
		return;
	default:
		/* Unrecognized messages are passed through without change. */
		break;
	}
	TRACE_2(TR_FAC_UDP, TR_UDP_WPUT_OTHER_END,
	    "udp_wput_other_end: q %p (%S)", q, "end");
	ip_output(connp, mp, q, IP_WPUT);
}

/*
 * udp_wput_iocdata is called by udp_wput_other to handle all M_IOCDATA
 * messages.
 */
static void
udp_wput_iocdata(queue_t *q, mblk_t *mp)
{
	mblk_t	*mp1;
	struct iocblk *iocp = (struct iocblk *)mp->b_rptr;
	STRUCT_HANDLE(strbuf, sb);
	udp_t	*udp = Q_TO_UDP(q);
	int	error;
	uint_t	addrlen;

	/* Make sure it is one of ours. */
	switch (iocp->ioc_cmd) {
	case TI_GETMYNAME:
	case TI_GETPEERNAME:
		break;
	default:
		ip_output(udp->udp_connp, mp, q, IP_WPUT);
		return;
	}

	switch (mi_copy_state(q, mp, &mp1)) {
	case -1:
		return;
	case MI_COPY_CASE(MI_COPY_IN, 1):
		break;
	case MI_COPY_CASE(MI_COPY_OUT, 1):
		/*
		 * The address has been copied out, so now
		 * copyout the strbuf.
		 */
		mi_copyout(q, mp);
		return;
	case MI_COPY_CASE(MI_COPY_OUT, 2):
		/*
		 * The address and strbuf have been copied out.
		 * We're done, so just acknowledge the original
		 * M_IOCTL.
		 */
		mi_copy_done(q, mp, 0);
		return;
	default:
		/*
		 * Something strange has happened, so acknowledge
		 * the original M_IOCTL with an EPROTO error.
		 */
		mi_copy_done(q, mp, EPROTO);
		return;
	}

	/*
	 * Now we have the strbuf structure for TI_GETMYNAME
	 * and TI_GETPEERNAME.  Next we copyout the requested
	 * address and then we'll copyout the strbuf.
	 */
	STRUCT_SET_HANDLE(sb, iocp->ioc_flag, (void *)mp1->b_rptr);
	addrlen = udp->udp_family == AF_INET ? sizeof (sin_t) : sizeof (sin6_t);
	if (STRUCT_FGET(sb, maxlen) < addrlen) {
		mi_copy_done(q, mp, EINVAL);
		return;
	}

	mp1 = mi_copyout_alloc(q, mp, STRUCT_FGETP(sb, buf), addrlen, B_TRUE);
	if (mp1 == NULL)
		return;

	rw_enter(&udp->udp_rwlock, RW_READER);
	switch (iocp->ioc_cmd) {
	case TI_GETMYNAME:
		error = udp_getmyname(udp, (void *)mp1->b_rptr, &addrlen);
		break;
	case TI_GETPEERNAME:
		error = udp_getpeername(udp, (void *)mp1->b_rptr, &addrlen);
		break;
	}
	rw_exit(&udp->udp_rwlock);

	if (error != 0) {
		mi_copy_done(q, mp, error);
	} else {
		mp1->b_wptr += addrlen;
		STRUCT_FSET(sb, len, addrlen);

		/* Copy out the address */
		mi_copyout(q, mp);
	}
}

static int
udp_unitdata_opt_process(queue_t *q, mblk_t *mp, int *errorp,
    udpattrs_t *udpattrs)
{
	struct T_unitdata_req *udreqp;
	int is_absreq_failure;
	cred_t *cr;
	conn_t	*connp = Q_TO_CONN(q);

	ASSERT(((t_primp_t)mp->b_rptr)->type);

	cr = DB_CREDDEF(mp, connp->conn_cred);

	udreqp = (struct T_unitdata_req *)mp->b_rptr;

	*errorp = tpi_optcom_buf(q, mp, &udreqp->OPT_length,
	    udreqp->OPT_offset, cr, &udp_opt_obj,
	    udpattrs, &is_absreq_failure);

	if (*errorp != 0) {
		/*
		 * Note: No special action needed in this
		 * module for "is_absreq_failure"
		 */
		return (-1);		/* failure */
	}
	ASSERT(is_absreq_failure == 0);
	return (0);	/* success */
}

void
udp_ddi_init(void)
{
	udp_max_optsize = optcom_max_optsize(udp_opt_obj.odb_opt_des_arr,
	    udp_opt_obj.odb_opt_arr_cnt);

	/*
	 * We want to be informed each time a stack is created or
	 * destroyed in the kernel, so we can maintain the
	 * set of udp_stack_t's.
	 */
	netstack_register(NS_UDP, udp_stack_init, NULL, udp_stack_fini);
}

void
udp_ddi_destroy(void)
{
	netstack_unregister(NS_UDP);
}

/*
 * Initialize the UDP stack instance.
 */
static void *
udp_stack_init(netstackid_t stackid, netstack_t *ns)
{
	udp_stack_t	*us;
	udpparam_t	*pa;
	int		i;

	us = (udp_stack_t *)kmem_zalloc(sizeof (*us), KM_SLEEP);
	us->us_netstack = ns;

	us->us_num_epriv_ports = UDP_NUM_EPRIV_PORTS;
	us->us_epriv_ports[0] = 2049;
	us->us_epriv_ports[1] = 4045;

	/*
	 * The smallest anonymous port in the priviledged port range which UDP
	 * looks for free port.  Use in the option UDP_ANONPRIVBIND.
	 */
	us->us_min_anonpriv_port = 512;

	us->us_bind_fanout_size = udp_bind_fanout_size;

	/* Roundup variable that might have been modified in /etc/system */
	if (us->us_bind_fanout_size & (us->us_bind_fanout_size - 1)) {
		/* Not a power of two. Round up to nearest power of two */
		for (i = 0; i < 31; i++) {
			if (us->us_bind_fanout_size < (1 << i))
				break;
		}
		us->us_bind_fanout_size = 1 << i;
	}
	us->us_bind_fanout = kmem_zalloc(us->us_bind_fanout_size *
	    sizeof (udp_fanout_t), KM_SLEEP);
	for (i = 0; i < us->us_bind_fanout_size; i++) {
		mutex_init(&us->us_bind_fanout[i].uf_lock, NULL, MUTEX_DEFAULT,
		    NULL);
	}

	pa = (udpparam_t *)kmem_alloc(sizeof (udp_param_arr), KM_SLEEP);

	us->us_param_arr = pa;
	bcopy(udp_param_arr, us->us_param_arr, sizeof (udp_param_arr));

	(void) udp_param_register(&us->us_nd,
	    us->us_param_arr, A_CNT(udp_param_arr));

	us->us_kstat = udp_kstat2_init(stackid, &us->us_statistics);
	us->us_mibkp = udp_kstat_init(stackid);
	return (us);
}

/*
 * Free the UDP stack instance.
 */
static void
udp_stack_fini(netstackid_t stackid, void *arg)
{
	udp_stack_t *us = (udp_stack_t *)arg;
	int i;

	for (i = 0; i < us->us_bind_fanout_size; i++) {
		mutex_destroy(&us->us_bind_fanout[i].uf_lock);
	}

	kmem_free(us->us_bind_fanout, us->us_bind_fanout_size *
	    sizeof (udp_fanout_t));

	us->us_bind_fanout = NULL;

	nd_free(&us->us_nd);
	kmem_free(us->us_param_arr, sizeof (udp_param_arr));
	us->us_param_arr = NULL;

	udp_kstat_fini(stackid, us->us_mibkp);
	us->us_mibkp = NULL;

	udp_kstat2_fini(stackid, us->us_kstat);
	us->us_kstat = NULL;
	bzero(&us->us_statistics, sizeof (us->us_statistics));
	kmem_free(us, sizeof (*us));
}

static void *
udp_kstat2_init(netstackid_t stackid, udp_stat_t *us_statisticsp)
{
	kstat_t *ksp;

	udp_stat_t template = {
		{ "udp_ip_send",		KSTAT_DATA_UINT64 },
		{ "udp_ip_ire_send",		KSTAT_DATA_UINT64 },
		{ "udp_ire_null",		KSTAT_DATA_UINT64 },
		{ "udp_drain",			KSTAT_DATA_UINT64 },
		{ "udp_sock_fallback",		KSTAT_DATA_UINT64 },
		{ "udp_rrw_busy",		KSTAT_DATA_UINT64 },
		{ "udp_rrw_msgcnt",		KSTAT_DATA_UINT64 },
		{ "udp_out_sw_cksum",		KSTAT_DATA_UINT64 },
		{ "udp_out_sw_cksum_bytes",	KSTAT_DATA_UINT64 },
		{ "udp_out_opt",		KSTAT_DATA_UINT64 },
		{ "udp_out_err_notconn",	KSTAT_DATA_UINT64 },
		{ "udp_out_err_output",		KSTAT_DATA_UINT64 },
		{ "udp_out_err_tudr",		KSTAT_DATA_UINT64 },
		{ "udp_in_pktinfo",		KSTAT_DATA_UINT64 },
		{ "udp_in_recvdstaddr",		KSTAT_DATA_UINT64 },
		{ "udp_in_recvopts",		KSTAT_DATA_UINT64 },
		{ "udp_in_recvif",		KSTAT_DATA_UINT64 },
		{ "udp_in_recvslla",		KSTAT_DATA_UINT64 },
		{ "udp_in_recvucred",		KSTAT_DATA_UINT64 },
		{ "udp_in_recvttl",		KSTAT_DATA_UINT64 },
		{ "udp_in_recvhopopts",		KSTAT_DATA_UINT64 },
		{ "udp_in_recvhoplimit",	KSTAT_DATA_UINT64 },
		{ "udp_in_recvdstopts",		KSTAT_DATA_UINT64 },
		{ "udp_in_recvrtdstopts",	KSTAT_DATA_UINT64 },
		{ "udp_in_recvrthdr",		KSTAT_DATA_UINT64 },
		{ "udp_in_recvpktinfo",		KSTAT_DATA_UINT64 },
		{ "udp_in_recvtclass",		KSTAT_DATA_UINT64 },
		{ "udp_in_timestamp",		KSTAT_DATA_UINT64 },
#ifdef DEBUG
		{ "udp_data_conn",		KSTAT_DATA_UINT64 },
		{ "udp_data_notconn",		KSTAT_DATA_UINT64 },
#endif
	};

	ksp = kstat_create_netstack(UDP_MOD_NAME, 0, "udpstat", "net",
	    KSTAT_TYPE_NAMED, sizeof (template) / sizeof (kstat_named_t),
	    KSTAT_FLAG_VIRTUAL, stackid);

	if (ksp == NULL)
		return (NULL);

	bcopy(&template, us_statisticsp, sizeof (template));
	ksp->ks_data = (void *)us_statisticsp;
	ksp->ks_private = (void *)(uintptr_t)stackid;

	kstat_install(ksp);
	return (ksp);
}

static void
udp_kstat2_fini(netstackid_t stackid, kstat_t *ksp)
{
	if (ksp != NULL) {
		ASSERT(stackid == (netstackid_t)(uintptr_t)ksp->ks_private);
		kstat_delete_netstack(ksp, stackid);
	}
}

static void *
udp_kstat_init(netstackid_t stackid)
{
	kstat_t	*ksp;

	udp_named_kstat_t template = {
		{ "inDatagrams",	KSTAT_DATA_UINT64, 0 },
		{ "inErrors",		KSTAT_DATA_UINT32, 0 },
		{ "outDatagrams",	KSTAT_DATA_UINT64, 0 },
		{ "entrySize",		KSTAT_DATA_INT32, 0 },
		{ "entry6Size",		KSTAT_DATA_INT32, 0 },
		{ "outErrors",		KSTAT_DATA_UINT32, 0 },
	};

	ksp = kstat_create_netstack(UDP_MOD_NAME, 0, UDP_MOD_NAME, "mib2",
	    KSTAT_TYPE_NAMED,
	    NUM_OF_FIELDS(udp_named_kstat_t), 0, stackid);

	if (ksp == NULL || ksp->ks_data == NULL)
		return (NULL);

	template.entrySize.value.ui32 = sizeof (mib2_udpEntry_t);
	template.entry6Size.value.ui32 = sizeof (mib2_udp6Entry_t);

	bcopy(&template, ksp->ks_data, sizeof (template));
	ksp->ks_update = udp_kstat_update;
	ksp->ks_private = (void *)(uintptr_t)stackid;

	kstat_install(ksp);
	return (ksp);
}

static void
udp_kstat_fini(netstackid_t stackid, kstat_t *ksp)
{
	if (ksp != NULL) {
		ASSERT(stackid == (netstackid_t)(uintptr_t)ksp->ks_private);
		kstat_delete_netstack(ksp, stackid);
	}
}

static int
udp_kstat_update(kstat_t *kp, int rw)
{
	udp_named_kstat_t *udpkp;
	netstackid_t	stackid = (netstackid_t)(uintptr_t)kp->ks_private;
	netstack_t	*ns;
	udp_stack_t	*us;

	if ((kp == NULL) || (kp->ks_data == NULL))
		return (EIO);

	if (rw == KSTAT_WRITE)
		return (EACCES);

	ns = netstack_find_by_stackid(stackid);
	if (ns == NULL)
		return (-1);
	us = ns->netstack_udp;
	if (us == NULL) {
		netstack_rele(ns);
		return (-1);
	}
	udpkp = (udp_named_kstat_t *)kp->ks_data;

	udpkp->inDatagrams.value.ui64 =	us->us_udp_mib.udpHCInDatagrams;
	udpkp->inErrors.value.ui32 =	us->us_udp_mib.udpInErrors;
	udpkp->outDatagrams.value.ui64 = us->us_udp_mib.udpHCOutDatagrams;
	udpkp->outErrors.value.ui32 =	us->us_udp_mib.udpOutErrors;
	netstack_rele(ns);
	return (0);
}

/*
 * Read-side synchronous stream info entry point, called as a
 * result of handling certain STREAMS ioctl operations.
 */
static int
udp_rinfop(queue_t *q, infod_t *dp)
{
	mblk_t	*mp;
	uint_t	cmd = dp->d_cmd;
	int	res = 0;
	int	error = 0;
	udp_t	*udp = Q_TO_UDP(q);
	struct stdata *stp = STREAM(q);

	mutex_enter(&udp->udp_drain_lock);
	/* If shutdown on read has happened, return nothing */
	mutex_enter(&stp->sd_lock);
	if (stp->sd_flag & STREOF) {
		mutex_exit(&stp->sd_lock);
		goto done;
	}
	mutex_exit(&stp->sd_lock);

	if ((mp = udp->udp_rcv_list_head) == NULL)
		goto done;

	ASSERT(DB_TYPE(mp) != M_DATA && mp->b_cont != NULL);

	if (cmd & INFOD_COUNT) {
		/*
		 * Return the number of messages.
		 */
		dp->d_count += udp->udp_rcv_msgcnt;
		res |= INFOD_COUNT;
	}
	if (cmd & INFOD_BYTES) {
		/*
		 * Return size of all data messages.
		 */
		dp->d_bytes += udp->udp_rcv_cnt;
		res |= INFOD_BYTES;
	}
	if (cmd & INFOD_FIRSTBYTES) {
		/*
		 * Return size of first data message.
		 */
		dp->d_bytes = msgdsize(mp);
		res |= INFOD_FIRSTBYTES;
		dp->d_cmd &= ~INFOD_FIRSTBYTES;
	}
	if (cmd & INFOD_COPYOUT) {
		mblk_t *mp1 = mp->b_cont;
		int n;
		/*
		 * Return data contents of first message.
		 */
		ASSERT(DB_TYPE(mp1) == M_DATA);
		while (mp1 != NULL && dp->d_uiop->uio_resid > 0) {
			n = MIN(dp->d_uiop->uio_resid, MBLKL(mp1));
			if (n != 0 && (error = uiomove((char *)mp1->b_rptr, n,
			    UIO_READ, dp->d_uiop)) != 0) {
				goto done;
			}
			mp1 = mp1->b_cont;
		}
		res |= INFOD_COPYOUT;
		dp->d_cmd &= ~INFOD_COPYOUT;
	}
done:
	mutex_exit(&udp->udp_drain_lock);

	dp->d_res |= res;

	return (error);
}

/*
 * Read-side synchronous stream entry point.  This is called as a result
 * of recv/read operation done at sockfs, and is guaranteed to execute
 * outside of the interrupt thread context.  It returns a single datagram
 * (b_cont chain of T_UNITDATA_IND plus data) to the upper layer.
 */
static int
udp_rrw(queue_t *q, struiod_t *dp)
{
	mblk_t	*mp;
	udp_t	*udp = Q_TO_UDP(q);
	udp_stack_t *us = udp->udp_us;

	/*
	 * Dequeue datagram from the head of the list and return
	 * it to caller; also ensure that RSLEEP sd_wakeq flag is
	 * set/cleared depending on whether or not there's data
	 * remaining in the list.
	 */
	mutex_enter(&udp->udp_drain_lock);
	if (!udp->udp_direct_sockfs) {
		mutex_exit(&udp->udp_drain_lock);
		UDP_STAT(us, udp_rrw_busy);
		return (EBUSY);
	}
	if ((mp = udp->udp_rcv_list_head) != NULL) {
		uint_t size = msgdsize(mp);

		/* Last datagram in the list? */
		if ((udp->udp_rcv_list_head = mp->b_next) == NULL)
			udp->udp_rcv_list_tail = NULL;
		mp->b_next = NULL;

		udp->udp_rcv_cnt -= size;
		udp->udp_rcv_msgcnt--;
		UDP_STAT(us, udp_rrw_msgcnt);

		/* No longer flow-controlling? */
		if (udp->udp_rcv_cnt < udp->udp_rcv_hiwat &&
		    udp->udp_rcv_msgcnt < udp->udp_rcv_hiwat)
			udp->udp_drain_qfull = B_FALSE;
	}
	if (udp->udp_rcv_list_head == NULL) {
		/*
		 * Either we just dequeued the last datagram or
		 * we get here from sockfs and have nothing to
		 * return; in this case clear RSLEEP.
		 */
		ASSERT(udp->udp_rcv_cnt == 0);
		ASSERT(udp->udp_rcv_msgcnt == 0);
		ASSERT(udp->udp_rcv_list_tail == NULL);
		STR_WAKEUP_CLEAR(STREAM(q));
	} else {
		/*
		 * More data follows; we need udp_rrw() to be
		 * called in future to pick up the rest.
		 */
		STR_WAKEUP_SET(STREAM(q));
	}
	mutex_exit(&udp->udp_drain_lock);
	dp->d_mp = mp;
	return (0);
}

/*
 * Enqueue a completely-built T_UNITDATA_IND message into the receive
 * list; this is typically executed within the interrupt thread context
 * and so we do things as quickly as possible.
 */
static void
udp_rcv_enqueue(queue_t *q, udp_t *udp, mblk_t *mp, uint_t pkt_len)
{
	ASSERT(q == RD(q));
	ASSERT(pkt_len == msgdsize(mp));
	ASSERT(mp->b_next == NULL && mp->b_cont != NULL);
	ASSERT(DB_TYPE(mp) == M_PROTO && DB_TYPE(mp->b_cont) == M_DATA);
	ASSERT(MBLKL(mp) >= sizeof (struct T_unitdata_ind));

	mutex_enter(&udp->udp_drain_lock);
	/*
	 * Wake up and signal the receiving app; it is okay to do this
	 * before enqueueing the mp because we are holding the drain lock.
	 * One of the advantages of synchronous stream is the ability for
	 * us to find out when the application performs a read on the
	 * socket by way of udp_rrw() entry point being called.  We need
	 * to generate SIGPOLL/SIGIO for each received data in the case
	 * of asynchronous socket just as in the strrput() case.  However,
	 * we only wake the application up when necessary, i.e. during the
	 * first enqueue.  When udp_rrw() is called, we send up a single
	 * datagram upstream and call STR_WAKEUP_SET() again when there
	 * are still data remaining in our receive queue.
	 */
	STR_WAKEUP_SENDSIG(STREAM(q), udp->udp_rcv_list_head);
	if (udp->udp_rcv_list_head == NULL)
		udp->udp_rcv_list_head = mp;
	else
		udp->udp_rcv_list_tail->b_next = mp;
	udp->udp_rcv_list_tail = mp;
	udp->udp_rcv_cnt += pkt_len;
	udp->udp_rcv_msgcnt++;

	/* Need to flow-control? */
	if (udp->udp_rcv_cnt >= udp->udp_rcv_hiwat ||
	    udp->udp_rcv_msgcnt >= udp->udp_rcv_hiwat)
		udp->udp_drain_qfull = B_TRUE;

	mutex_exit(&udp->udp_drain_lock);
}

/*
 * Drain the contents of receive list to the module upstream; we do
 * this during close or when we fallback to the slow mode due to
 * sockmod being popped or a module being pushed on top of us.
 */
static void
udp_rcv_drain(queue_t *q, udp_t *udp, boolean_t closing)
{
	mblk_t *mp;
	udp_stack_t *us = udp->udp_us;

	ASSERT(q == RD(q));

	mutex_enter(&udp->udp_drain_lock);
	/*
	 * There is no race with a concurrent udp_input() sending
	 * up packets using putnext() after we have cleared the
	 * udp_direct_sockfs flag but before we have completed
	 * sending up the packets in udp_rcv_list, since we are
	 * either a writer or we have quiesced the conn.
	 */
	udp->udp_direct_sockfs = B_FALSE;
	mutex_exit(&udp->udp_drain_lock);

	if (udp->udp_rcv_list_head != NULL)
		UDP_STAT(us, udp_drain);

	/*
	 * Send up everything via putnext(); note here that we
	 * don't need the udp_drain_lock to protect us since
	 * nothing can enter udp_rrw() and that we currently
	 * have exclusive access to this udp.
	 */
	while ((mp = udp->udp_rcv_list_head) != NULL) {
		udp->udp_rcv_list_head = mp->b_next;
		mp->b_next = NULL;
		udp->udp_rcv_cnt -= msgdsize(mp);
		udp->udp_rcv_msgcnt--;
		if (closing) {
			freemsg(mp);
		} else {
			putnext(q, mp);
		}
	}
	ASSERT(udp->udp_rcv_cnt == 0);
	ASSERT(udp->udp_rcv_msgcnt == 0);
	ASSERT(udp->udp_rcv_list_head == NULL);
	udp->udp_rcv_list_tail = NULL;
	udp->udp_drain_qfull = B_FALSE;
}

static size_t
udp_set_rcv_hiwat(udp_t *udp, size_t size)
{
	udp_stack_t *us = udp->udp_us;

	/* We add a bit of extra buffering */
	size += size >> 1;
	if (size > us->us_max_buf)
		size = us->us_max_buf;

	udp->udp_rcv_hiwat = size;
	return (size);
}

/*
 * For the lower queue so that UDP can be a dummy mux.
 * Nobody should be sending
 * packets up this stream
 */
static void
udp_lrput(queue_t *q, mblk_t *mp)
{
	mblk_t *mp1;

	switch (mp->b_datap->db_type) {
	case M_FLUSH:
		/* Turn around */
		if (*mp->b_rptr & FLUSHW) {
			*mp->b_rptr &= ~FLUSHR;
			qreply(q, mp);
			return;
		}
		break;
	}
	/* Could receive messages that passed through ar_rput */
	for (mp1 = mp; mp1; mp1 = mp1->b_cont)
		mp1->b_prev = mp1->b_next = NULL;
	freemsg(mp);
}

/*
 * For the lower queue so that UDP can be a dummy mux.
 * Nobody should be sending packets down this stream.
 */
/* ARGSUSED */
void
udp_lwput(queue_t *q, mblk_t *mp)
{
	freemsg(mp);
}
