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

#ifndef	_INET_IP_IMPL_H
#define	_INET_IP_IMPL_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

/*
 * IP implementation private declarations.  These interfaces are
 * used to build the IP module and are not meant to be accessed
 * by any modules except IP itself.  They are undocumented and are
 * subject to change without notice.
 */

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef _KERNEL

#include <sys/sdt.h>

#define	IP_MOD_ID		5701

#ifdef	_BIG_ENDIAN
#define	IP_HDR_CSUM_TTL_ADJUST	256
#define	IP_TCP_CSUM_COMP	IPPROTO_TCP
#define	IP_UDP_CSUM_COMP	IPPROTO_UDP
#else
#define	IP_HDR_CSUM_TTL_ADJUST	1
#define	IP_TCP_CSUM_COMP	(IPPROTO_TCP << 8)
#define	IP_UDP_CSUM_COMP	(IPPROTO_UDP << 8)
#endif

#define	TCP_CHECKSUM_OFFSET	16
#define	TCP_CHECKSUM_SIZE	2

#define	UDP_CHECKSUM_OFFSET	6
#define	UDP_CHECKSUM_SIZE	2

#define	IPH_TCPH_CHECKSUMP(ipha, hlen)	\
	((uint16_t *)(((uchar_t *)(ipha)) + ((hlen) + TCP_CHECKSUM_OFFSET)))

#define	IPH_UDPH_CHECKSUMP(ipha, hlen)	\
	((uint16_t *)(((uchar_t *)(ipha)) + ((hlen) + UDP_CHECKSUM_OFFSET)))

#define	ILL_HCKSUM_CAPABLE(ill)		\
	(((ill)->ill_capabilities & ILL_CAPAB_HCKSUM) != 0)
/*
 * Macro that performs software checksum calculation on the IP header.
 */
#define	IP_HDR_CKSUM(ipha, sum, v_hlen_tos_len, ttl_protocol) {		\
	(sum) += (ttl_protocol) + (ipha)->ipha_ident +			\
	    ((v_hlen_tos_len) >> 16) +					\
	    ((v_hlen_tos_len) & 0xFFFF) +				\
	    (ipha)->ipha_fragment_offset_and_flags;			\
	(sum) = (((sum) & 0xFFFF) + ((sum) >> 16));			\
	(sum) = ~((sum) + ((sum) >> 16));				\
	(ipha)->ipha_hdr_checksum = (uint16_t)(sum);			\
}

#define	IS_IP_HDR_HWCKSUM(ipsec, mp, ill)				\
	((!ipsec) && (DB_CKSUMFLAGS(mp) & HCK_IPV4_HDRCKSUM) &&		\
	ILL_HCKSUM_CAPABLE(ill) && dohwcksum)

/*
 * This macro acts as a wrapper around IP_CKSUM_XMIT_FAST, and it performs
 * several checks on the IRE and ILL (among other things) in order to see
 * whether or not hardware checksum offload is allowed for the outgoing
 * packet.  It assumes that the caller has held a reference to the IRE.
 */
#define	IP_CKSUM_XMIT(ill, ire, mp, ihp, up, proto, start, end,		\
	    max_frag, ipsec_len, pseudo) {				\
	uint32_t _hck_flags;						\
	/*								\
	 * We offload checksum calculation to hardware when IPsec isn't	\
	 * present and if fragmentation isn't required.  We also check	\
	 * if M_DATA fastpath is safe to be used on the	corresponding	\
	 * IRE; this check is performed without grabbing ire_lock but	\
	 * instead by holding a reference to it.  This is sufficient	\
	 * for IRE_CACHE; for IRE_BROADCAST on non-Ethernet links, the	\
	 * DL_NOTE_FASTPATH_FLUSH indication could come up from the	\
	 * driver and trigger the IRE (hence fp_mp) deletion.  This is	\
	 * why only IRE_CACHE type is eligible for offload.		\
	 *								\
	 * The presense of IP options also forces the network stack to	\
	 * calculate the checksum in software.  This is because:	\
	 *								\
	 * Wrap around: certain partial-checksum NICs (eri, ce) limit	\
	 * the size of "start offset" width to 6-bit.  This effectively	\
	 * sets the largest value of the offset to 64-bytes, starting	\
	 * from the MAC header.  When the cumulative MAC and IP headers	\
	 * exceed such limit, the offset will wrap around.  This causes	\
	 * the checksum to be calculated at the wrong place.		\
	 *								\
	 * IPv4 source routing: none of the full-checksum capable NICs	\
	 * is capable of correctly handling the	IPv4 source-routing	\
	 * option for purposes of calculating the pseudo-header; the	\
	 * actual destination is different from the destination in the	\
	 * header which is that of the next-hop.  (This case may not be	\
	 * true for NICs which can parse IPv6 extension headers, but	\
	 * we choose to simplify the implementation by not offloading	\
	 * checksum when they are present.)				\
	 *								\
	 */								\
	if ((ill) != NULL && ILL_HCKSUM_CAPABLE(ill) &&			\
	    !((ire)->ire_flags & RTF_MULTIRT) &&			\
	    (!((ire)->ire_type & IRE_BROADCAST) ||			\
	    (ill)->ill_type == IFT_ETHER) &&			\
	    (ipsec_len) == 0 &&						\
	    (((ire)->ire_ipversion == IPV4_VERSION &&			\
	    (start) == IP_SIMPLE_HDR_LENGTH &&				\
	    ((ire)->ire_nce != NULL &&					\
	    (ire)->ire_nce->nce_fp_mp != NULL &&	\
	    MBLKHEAD(mp) >= MBLKL((ire)->ire_nce->nce_fp_mp))) ||	\
	    ((ire)->ire_ipversion == IPV6_VERSION &&			\
	    (start) == IPV6_HDR_LEN &&					\
	    (ire)->ire_nce->nce_fp_mp != NULL &&			\
	    MBLKHEAD(mp) >= MBLKL((ire)->ire_nce->nce_fp_mp))) &&	\
	    (max_frag) >= (uint_t)((end) + (ipsec_len)) &&		\
	    dohwcksum) {						\
		_hck_flags = (ill)->ill_hcksum_capab->ill_hcksum_txflags; \
	} else {							\
		_hck_flags = 0;						\
	}								\
	IP_CKSUM_XMIT_FAST((ire)->ire_ipversion, _hck_flags, mp, ihp,	\
	    up, proto, start, end, pseudo);				\
}

/*
 * Based on the device capabilities, this macro either marks an outgoing
 * packet with hardware checksum offload information or calculate the
 * checksum in software.  If the latter is performed, the checksum field
 * of the dblk is cleared; otherwise it will be non-zero and contain the
 * necessary flag(s) for the driver.
 */
#define	IP_CKSUM_XMIT_FAST(ipver, hck_flags, mp, ihp, up, proto, start,	\
	    end, pseudo) {						\
	uint32_t _sum;							\
	/*								\
	 * Underlying interface supports hardware checksum offload for	\
	 * the payload; leave the payload checksum for the hardware to	\
	 * calculate.  N.B: We only need to set up checksum info on the	\
	 * first mblk.							\
	 */								\
	DB_CKSUMFLAGS(mp) = 0;						\
	if (((ipver) == IPV4_VERSION &&					\
	    ((hck_flags) & HCKSUM_INET_FULL_V4)) ||			\
	    ((ipver) == IPV6_VERSION &&					\
	    ((hck_flags) & HCKSUM_INET_FULL_V6))) {			\
		/*							\
		 * Hardware calculates pseudo-header, header and the	\
		 * payload checksums, so clear the checksum field in	\
		 * the protocol header.					\
		 */							\
		*(up) = 0;						\
		DB_CKSUMFLAGS(mp) |= HCK_FULLCKSUM;			\
	} else if ((hck_flags) & HCKSUM_INET_PARTIAL)  {		\
		/*							\
		 * Partial checksum offload has been enabled.  Fill	\
		 * the checksum field in the protocl header with the	\
		 * pseudo-header checksum value.			\
		 */							\
		_sum = ((proto) == IPPROTO_UDP) ?			\
		    IP_UDP_CSUM_COMP : IP_TCP_CSUM_COMP;		\
		_sum += *(up) + (pseudo);				\
		_sum = (_sum & 0xFFFF) + (_sum >> 16);			\
		*(up) = (_sum & 0xFFFF) + (_sum >> 16);			\
		/*							\
		 * Offsets are relative to beginning of IP header.	\
		 */							\
		DB_CKSUMSTART(mp) = (start);				\
		DB_CKSUMSTUFF(mp) = ((proto) == IPPROTO_UDP) ?		\
		    (start) + UDP_CHECKSUM_OFFSET :			\
		    (start) + TCP_CHECKSUM_OFFSET;			\
		DB_CKSUMEND(mp) = (end);				\
		DB_CKSUMFLAGS(mp) |= HCK_PARTIALCKSUM;			\
	} else {							\
		/*							\
		 * Software checksumming.				\
		 */							\
		_sum = ((proto) == IPPROTO_UDP) ?			\
		    IP_UDP_CSUM_COMP : IP_TCP_CSUM_COMP;		\
		_sum += (pseudo);					\
		_sum = IP_CSUM(mp, start, _sum);			\
		*(up) = (uint16_t)(((proto) == IPPROTO_UDP) ?		\
		    (_sum ? _sum : ~_sum) : _sum);			\
	}								\
	/*								\
	 * Hardware supports IP header checksum offload; clear the	\
	 * contents of IP header checksum field as expected by NIC.	\
	 * Do this only if we offloaded either full or partial sum.	\
	 */								\
	if ((ipver) == IPV4_VERSION && DB_CKSUMFLAGS(mp) != 0 &&	\
	    ((hck_flags) & HCKSUM_IPHDRCKSUM)) {			\
		DB_CKSUMFLAGS(mp) |= HCK_IPV4_HDRCKSUM;			\
		((ipha_t *)(ihp))->ipha_hdr_checksum = 0;		\
	}								\
}

/*
 * Macro to inspect the checksum of a fully-reassembled incoming datagram.
 */
#define	IP_CKSUM_RECV_REASS(hck_flags, off, pseudo, sum, err) {		\
	(err) = B_FALSE;						\
	if ((hck_flags) & HCK_FULLCKSUM) {				\
		/*							\
		 * The sum of all fragment checksums should		\
		 * result in -0 (0xFFFF) or otherwise invalid.		\
		 */							\
		if ((sum) != 0xFFFF)					\
			(err) = B_TRUE;					\
	} else if ((hck_flags) & HCK_PARTIALCKSUM) {			\
		(sum) += (pseudo);					\
		(sum) = ((sum) & 0xFFFF) + ((sum) >> 16);		\
		(sum) = ((sum) & 0xFFFF) + ((sum) >> 16);		\
		if (~(sum) & 0xFFFF)					\
			(err) = B_TRUE;					\
	} else if (((sum) = IP_CSUM(mp, off, pseudo)) != 0) {		\
		(err) = B_TRUE;						\
	}								\
}

/*
 * This macro inspects an incoming packet to see if the checksum value
 * contained in it is valid; if the hardware has provided the information,
 * the value is verified, otherwise it performs software checksumming.
 * The checksum value is returned to caller.
 */
#define	IP_CKSUM_RECV(hck_flags, sum, cksum_start, ulph_off, mp, mp1, err) { \
	int32_t _len;							\
									\
	(err) = B_FALSE;						\
	if ((hck_flags) & HCK_FULLCKSUM) {				\
		/*							\
		 * Full checksum has been computed by the hardware	\
		 * and has been attached.  If the driver wants us to	\
		 * verify the correctness of the attached value, in	\
		 * order to protect against faulty hardware, compare	\
		 * it against -0 (0xFFFF) to see if it's valid.		\
		 */							\
		(sum) = DB_CKSUM16(mp);					\
		if (!((hck_flags) & HCK_FULLCKSUM_OK) && (sum) != 0xFFFF) \
			(err) = B_TRUE;					\
	} else if (((hck_flags) & HCK_PARTIALCKSUM) &&			\
	    ((mp1) == NULL || (mp1)->b_cont == NULL) &&			\
	    (ulph_off) >= DB_CKSUMSTART(mp) &&				\
	    ((_len = (ulph_off) - DB_CKSUMSTART(mp)) & 1) == 0) {	\
		uint32_t _adj;						\
		/*							\
		 * Partial checksum has been calculated by hardware	\
		 * and attached to the packet; in addition, any		\
		 * prepended extraneous data is even byte aligned,	\
		 * and there are at most two mblks associated with	\
		 * the packet.  If any such data exists, we adjust	\
		 * the checksum; also take care any postpended data.	\
		 */							\
		IP_ADJCKSUM_PARTIAL(cksum_start, mp, mp1, _len, _adj);	\
		/*							\
		 * One's complement subtract extraneous checksum	\
		 */							\
		(sum) += DB_CKSUM16(mp);				\
		if (_adj >= (sum))					\
			(sum) = ~(_adj - (sum)) & 0xFFFF;		\
		else							\
			(sum) -= _adj;					\
		(sum) = ((sum) & 0xFFFF) + ((int)(sum) >> 16);		\
		(sum) = ((sum) & 0xFFFF) + ((int)(sum) >> 16);		\
		if (~(sum) & 0xFFFF)					\
			(err) = B_TRUE;					\
	} else if (((sum) = IP_CSUM(mp, ulph_off, sum)) != 0) {		\
		(err) = B_TRUE;						\
	}								\
}

/*
 * Macro to adjust a given checksum value depending on any prepended
 * or postpended data on the packet.  It expects the start offset to
 * begin at an even boundary and that the packet consists of at most
 * two mblks.
 */
#define	IP_ADJCKSUM_PARTIAL(cksum_start, mp, mp1, len, adj) {		\
	/*								\
	 * Prepended extraneous data; adjust checksum.			\
	 */								\
	if ((len) > 0)							\
		(adj) = IP_BCSUM_PARTIAL(cksum_start, len, 0);		\
	else								\
		(adj) = 0;						\
	/*								\
	 * len is now the total length of mblk(s)			\
	 */								\
	(len) = MBLKL(mp);						\
	if ((mp1) == NULL)						\
		(mp1) = (mp);						\
	else								\
		(len) += MBLKL(mp1);					\
	/*								\
	 * Postpended extraneous data; adjust checksum.			\
	 */								\
	if (((len) = (DB_CKSUMEND(mp) - len)) > 0) {			\
		uint32_t _pad;						\
									\
		_pad = IP_BCSUM_PARTIAL((mp1)->b_wptr, len, 0);		\
		/*							\
		 * If the postpended extraneous data was odd		\
		 * byte aligned, swap resulting checksum bytes.		\
		 */							\
		if ((uintptr_t)(mp1)->b_wptr & 1)			\
			(adj) += ((_pad << 8) & 0xFFFF) | (_pad >> 8);	\
		else							\
			(adj) += _pad;					\
		(adj) = ((adj) & 0xFFFF) + ((int)(adj) >> 16);		\
	}								\
}

#define	ILL_MDT_CAPABLE(ill)		\
	(((ill)->ill_capabilities & ILL_CAPAB_MDT) != 0)

/*
 * ioctl identifier and structure for Multidata Transmit update
 * private M_CTL communication from IP to ULP.
 */
#define	MDT_IOC_INFO_UPDATE	(('M' << 8) + 1020)

typedef struct ip_mdt_info_s {
	uint_t	mdt_info_id;	/* MDT_IOC_INFO_UPDATE */
	ill_mdt_capab_t	mdt_capab; /* ILL MDT capabilities */
} ip_mdt_info_t;

/*
 * Macro that determines whether or not a given ILL is allowed for MDT.
 */
#define	ILL_MDT_USABLE(ill)						\
	(ILL_MDT_CAPABLE(ill) &&					\
	ill->ill_mdt_capab != NULL &&					\
	ill->ill_mdt_capab->ill_mdt_version == MDT_VERSION_2 &&		\
	ill->ill_mdt_capab->ill_mdt_on != 0)

#define	ILL_LSO_CAPABLE(ill)		\
	(((ill)->ill_capabilities & ILL_CAPAB_LSO) != 0)

/*
 * ioctl identifier and structure for Large Segment Offload
 * private M_CTL communication from IP to ULP.
 */
#define	LSO_IOC_INFO_UPDATE	(('L' << 24) + ('S' << 16) + ('O' << 8))

typedef struct ip_lso_info_s {
	uint_t	lso_info_id;	/* LSO_IOC_INFO_UPDATE */
	ill_lso_capab_t	lso_capab; /* ILL LSO capabilities */
} ip_lso_info_t;

/*
 * Macro that determines whether or not a given ILL is allowed for LSO.
 */
#define	ILL_LSO_USABLE(ill)						\
	(ILL_LSO_CAPABLE(ill) &&					\
	ill->ill_lso_capab != NULL &&					\
	ill->ill_lso_capab->ill_lso_version == LSO_VERSION_1 &&		\
	ill->ill_lso_capab->ill_lso_on != 0)

#define	ILL_LSO_TCP_USABLE(ill)						\
	(ILL_LSO_USABLE(ill) &&						\
	ill->ill_lso_capab->ill_lso_flags & LSO_TX_BASIC_TCP_IPV4)

/*
 * Macro that determines whether or not a given CONN may be considered
 * for fast path prior to proceeding further with LSO or Multidata.
 */
#define	CONN_IS_LSO_MD_FASTPATH(connp)	\
	((connp)->conn_dontroute == 0 &&	/* SO_DONTROUTE */	\
	!((connp)->conn_nexthop_set) &&		/* IP_NEXTHOP */	\
	(connp)->conn_nofailover_ill == NULL &&	/* IPIF_NOFAILOVER */	\
	(connp)->conn_outgoing_pill == NULL &&	/* IP{V6}_BOUND_PIF */	\
	(connp)->conn_outgoing_ill == NULL)	/* IP{V6}_BOUND_IF */

/* Definitons for fragmenting IP packets using MDT. */

/*
 * Smaller and private version of pdescinfo_t used specifically for IP,
 * which allows for only a single payload span per packet.
 */
typedef struct ip_pdescinfo_s PDESCINFO_STRUCT(2)	ip_pdescinfo_t;

/*
 * Macro version of ip_can_frag_mdt() which avoids the function call if we
 * only examine a single message block.
 */
#define	IP_CAN_FRAG_MDT(mp, hdr_len, len)			\
	(((mp)->b_cont == NULL) ?				\
	(MBLKL(mp) >= ((hdr_len) + ip_wput_frag_mdt_min)) :	\
	ip_can_frag_mdt((mp), (hdr_len), (len)))

/*
 * Macro that determines whether or not a given IPC requires
 * outbound IPSEC processing.
 */
#define	CONN_IPSEC_OUT_ENCAPSULATED(connp)	\
	((connp)->conn_out_enforce_policy ||	\
	((connp)->conn_latch != NULL &&		\
	(connp)->conn_latch->ipl_out_policy != NULL))

/*
 * These are used by the synchronous streams code in tcp and udp.
 * When we set the flags for a wakeup from a synchronous stream we
 * always set RSLEEP in sd_wakeq, even if we have a read thread waiting
 * to do the io. This is in case the read thread gets interrupted
 * before completing the io. The RSLEEP flag in sd_wakeq is used to
 * indicate that there is data available at the synchronous barrier.
 * The assumption is that subsequent functions calls through rwnext()
 * will reset sd_wakeq appropriately.
 */
#define	STR_WAKEUP_CLEAR(stp) {						\
	mutex_enter(&stp->sd_lock);					\
	stp->sd_wakeq &= ~RSLEEP;					\
	mutex_exit(&stp->sd_lock);					\
}

#define	STR_WAKEUP_SET(stp) {						\
	mutex_enter(&stp->sd_lock);					\
	if (stp->sd_flag & RSLEEP) {					\
		stp->sd_flag &= ~RSLEEP;				\
		cv_broadcast(&_RD(stp->sd_wrq)->q_wait);		\
	}								\
	stp->sd_wakeq |= RSLEEP;					\
	mutex_exit(&stp->sd_lock);					\
}

/*
 * Combined wakeup and sendsig to avoid dropping and reacquiring the
 * sd_lock. The list of messages waiting at the synchronous barrier is
 * supplied in order to determine whether a wakeup needs to occur. We
 * only send a wakeup to the application when necessary, i.e. during
 * the first enqueue when the received messages list will be NULL.
 */
#define	STR_WAKEUP_SENDSIG(stp, rcv_list) {				\
	int _events;							\
	mutex_enter(&stp->sd_lock);					\
	if (rcv_list == NULL) {						\
		if (stp->sd_flag & RSLEEP) {				\
			stp->sd_flag &= ~RSLEEP;			\
			cv_broadcast(&_RD(stp->sd_wrq)->q_wait);	\
		}							\
		stp->sd_wakeq |= RSLEEP;				\
	}								\
	if ((_events = stp->sd_sigflags & (S_INPUT | S_RDNORM)) != 0)	\
		strsendsig(stp->sd_siglist, _events, 0, 0);		\
	if (stp->sd_rput_opt & SR_POLLIN) {				\
		stp->sd_rput_opt &= ~SR_POLLIN;				\
		mutex_exit(&stp->sd_lock);				\
		pollwakeup(&stp->sd_pollist, POLLIN | POLLRDNORM);	\
	} else {							\
		mutex_exit(&stp->sd_lock);				\
	}								\
}

#define	CONN_UDP_SYNCSTR(connp)						\
	(IPCL_IS_UDP(connp) && (connp)->conn_udp->udp_direct_sockfs)

/*
 * Macro that checks whether or not a particular UDP conn is
 * flow-controlling on the read-side.  If udp module is directly
 * above ip, check to see if the drain queue is full; note here
 * that we check this without any lock protection because this
 * is a coarse granularity inbound flow-control.  If the module
 * above ip is not udp, then use canputnext to determine the
 * flow-control.
 *
 * Note that these checks are done after the conn is found in
 * the UDP fanout table.
 * FIXME? Might be faster to check both udp_drain_qfull and canputnext.
 */
#define	CONN_UDP_FLOWCTLD(connp)					\
	(CONN_UDP_SYNCSTR(connp) ?					\
	(connp)->conn_udp->udp_drain_qfull :				\
	!canputnext((connp)->conn_rq))

#define	ILL_DLS_CAPABLE(ill)	\
	(((ill)->ill_capabilities &		\
	(ILL_CAPAB_POLL|ILL_CAPAB_SOFT_RING)) != 0)

/*
 * Macro that hands off one or more messages directly to DLD
 * when the interface is marked with ILL_CAPAB_POLL.
 */
#define	IP_DLS_ILL_TX(ill, ipha, mp, ipst) {				\
	ill_dls_capab_t *ill_dls = ill->ill_dls_capab;			\
	ASSERT(ILL_DLS_CAPABLE(ill));					\
	ASSERT(ill_dls != NULL);					\
	ASSERT(ill_dls->ill_tx != NULL);				\
	ASSERT(ill_dls->ill_tx_handle != NULL);				\
	DTRACE_PROBE4(ip4__physical__out__start,			\
	    ill_t *, NULL, ill_t *, ill,				\
	    ipha_t *, ipha, mblk_t *, mp);				\
	FW_HOOKS(ipst->ips_ip4_physical_out_event,			\
	    ipst->ips_ipv4firewall_physical_out,			\
	    NULL, ill, ipha, mp, mp, 0, ipst);				\
	DTRACE_PROBE1(ip4__physical__out__end, mblk_t *, mp);		\
	if (mp != NULL)	{						\
		DTRACE_IP7(send, mblk_t *, mp, conn_t *, NULL,		\
		    void_ip_t *, ipha, __dtrace_ipsr_ill_t *, ill,	\
		    ipha_t *, ipha, ip6_t *, NULL, int,	0);		\
		ill_dls->ill_tx(ill_dls->ill_tx_handle, mp);		\
	}								\
}

extern int	ip_wput_frag_mdt_min;
extern boolean_t ip_can_frag_mdt(mblk_t *, ssize_t, ssize_t);
extern mblk_t   *ip_prepend_zoneid(mblk_t *, zoneid_t, ip_stack_t *);

#endif	/* _KERNEL */

#ifdef	__cplusplus
}
#endif

#endif	/* _INET_IP_IMPL_H */
