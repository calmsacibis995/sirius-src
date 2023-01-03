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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/cmn_err.h>
#include <sys/stream.h>
#include <sys/sunddi.h>
#include <sys/hook.h>
#include <sys/hook_impl.h>
#include <sys/netstack.h>
#include <net/if.h>

#include <sys/neti.h>
#include <sys/hook_event.h>
#include <inet/arp_impl.h>

/*
 * ARP netinfo entry point declarations.
 */
static int 	arp_getifname(phy_if_t, char *, const size_t, netstack_t *);
static int 	arp_getmtu(phy_if_t, lif_if_t, netstack_t *);
static int 	arp_getpmtuenabled(netstack_t *);
static int 	arp_getlifaddr(phy_if_t, lif_if_t, size_t,
		    net_ifaddr_t [], void *, netstack_t *);
static phy_if_t arp_phygetnext(phy_if_t, netstack_t *);
static phy_if_t arp_phylookup(const char *, netstack_t *);
static lif_if_t arp_lifgetnext(phy_if_t, lif_if_t, netstack_t *);
static int 	arp_inject(inject_t, net_inject_t *, netstack_t *);
static phy_if_t arp_routeto(struct sockaddr *, netstack_t *);
static int 	arp_ispartialchecksum(mblk_t *);
static int 	arp_isvalidchecksum(mblk_t *);

static net_info_t arp_netinfo = {
	NETINFO_VERSION,
	NHF_ARP,
	arp_getifname,
	arp_getmtu,
	arp_getpmtuenabled,
	arp_getlifaddr,
	arp_phygetnext,
	arp_phylookup,
	arp_lifgetnext,
	arp_inject,
	arp_routeto,
	arp_ispartialchecksum,
	arp_isvalidchecksum
};

/*
 * Register ARP netinfo functions.
 */
void
arp_net_init(arp_stack_t *as, netstack_t *ns)
{
	as->as_net_data = net_register_impl(&arp_netinfo, ns);
	ASSERT(as->as_net_data != NULL);
}

/*
 * Unregister ARP netinfo functions.
 */
void
arp_net_destroy(arp_stack_t *as)
{
	(void) net_unregister(as->as_net_data);
}

/*
 * Initialize ARP hook family and events
 */
void
arp_hook_init(arp_stack_t *as)
{
	HOOK_FAMILY_INIT(&as->as_arproot, Hn_ARP);
	if (net_register_family(as->as_net_data, &as->as_arproot) != 0) {
		cmn_err(CE_NOTE, "arp_hook_init: "
		    "net_register_family failed for arp");
	}

	HOOK_EVENT_INIT(&as->as_arp_physical_in_event, NH_PHYSICAL_IN);
	as->as_arp_physical_in = net_register_event(as->as_net_data,
	    &as->as_arp_physical_in_event);
	if (as->as_arp_physical_in == NULL) {
		cmn_err(CE_NOTE, "arp_hook_init: "
		    "net_register_event failed for arp/physical_in");
	}

	HOOK_EVENT_INIT(&as->as_arp_physical_out_event, NH_PHYSICAL_OUT);
	as->as_arp_physical_out = net_register_event(as->as_net_data,
	    &as->as_arp_physical_out_event);
	if (as->as_arp_physical_out == NULL) {
		cmn_err(CE_NOTE, "arp_hook_init: "
		    "net_register_event failed for arp/physical_out");
	}

	HOOK_EVENT_INIT(&as->as_arp_nic_events, NH_NIC_EVENTS);
	as->as_arpnicevents = net_register_event(as->as_net_data,
	    &as->as_arp_nic_events);
	if (as->as_arpnicevents == NULL) {
		cmn_err(CE_NOTE, "arp_hook_init: "
		    "net_register_event failed for arp/nic_events");
	}
}

void
arp_hook_destroy(arp_stack_t *as)
{
	if (as->as_arpnicevents != NULL) {
		if (net_unregister_event(as->as_net_data,
		    &as->as_arp_nic_events) == 0)
			as->as_arpnicevents = NULL;
	}

	if (as->as_arp_physical_out != NULL) {
		if (net_unregister_event(as->as_net_data,
		    &as->as_arp_physical_out_event) == 0)
			as->as_arp_physical_out = NULL;
	}

	if (as->as_arp_physical_in != NULL) {
		if (net_unregister_event(as->as_net_data,
		    &as->as_arp_physical_in_event) == 0)
			as->as_arp_physical_in = NULL;
	}

	(void) net_unregister_family(as->as_net_data, &as->as_arproot);
}

/*
 * Determine the name of the lower level interface
 */
static int
arp_getifname(phy_if_t phy_ifdata, char *buffer, const size_t buflen,
    netstack_t *ns)
{
	arl_t	*arl;
	arp_stack_t *as;

	ASSERT(buffer != NULL);
	ASSERT(ns != NULL);

	as = ns->netstack_arp;
	rw_enter(&as->as_arl_lock, RW_READER);
	for (arl = as->as_arl_head; arl != NULL; arl = arl->arl_next) {
		if (arl->arl_index == phy_ifdata) {
			(void) strlcpy(buffer, arl->arl_name, buflen);
			rw_exit(&as->as_arl_lock);
			return (0);
		}
	}
	rw_exit(&as->as_arl_lock);

	return (1);
}

/*
 * Unsupported with ARP.
 */
/*ARGSUSED*/
static int
arp_getmtu(phy_if_t phy_ifdata, lif_if_t ifdata, netstack_t *ns)
{
	return (-1);
}

/*
 * Unsupported with ARP.
 */
/*ARGSUSED*/
static int
arp_getpmtuenabled(netstack_t *ns)
{
	return (-1);
}

/*
 * Unsupported with ARP.
 */
/*ARGSUSED*/
static int
arp_getlifaddr(phy_if_t phy_ifdata, lif_if_t ifdata, size_t nelem,
	net_ifaddr_t type[], void *storage, netstack_t *ns)
{
	return (-1);
}

/*
 * Determine the instance number of the next lower level interface
 */
static phy_if_t
arp_phygetnext(phy_if_t phy_ifdata, netstack_t *ns)
{
	arl_t *arl;
	int index;
	arp_stack_t *as;

	ASSERT(ns != NULL);

	as = ns->netstack_arp;
	rw_enter(&as->as_arl_lock, RW_READER);
	if (phy_ifdata == 0) {
		arl = as->as_arl_head;
	} else {
		for (arl = as->as_arl_head; arl != NULL;
		    arl = arl->arl_next) {
			if (arl->arl_index == phy_ifdata) {
				arl = arl->arl_next;
				break;
			}
		}
	}

	index = (arl != NULL) ? arl->arl_index : 0;

	rw_exit(&as->as_arl_lock);

	return (index);
}

/*
 * Given a network interface name, find its ARP layer instance number.
 */
static phy_if_t
arp_phylookup(const char *name, netstack_t *ns)
{
	arl_t *arl;
	int index;
	arp_stack_t *as;

	ASSERT(name != NULL);
	ASSERT(ns != NULL);

	index = 0;
	as = ns->netstack_arp;
	rw_enter(&as->as_arl_lock, RW_READER);
	for (arl = as->as_arl_head; arl != NULL; arl = arl->arl_next) {
		if (strcmp(name, arl->arl_name) == 0) {
			index = arl->arl_index;
			break;
		}
	}
	rw_exit(&as->as_arl_lock);

	return (index);

}

/*
 * Unsupported with ARP.
 */
/*ARGSUSED*/
static lif_if_t
arp_lifgetnext(phy_if_t ifp, lif_if_t lif, netstack_t *ns)
{
	return ((lif_if_t)-1);
}

/*
 * Unsupported with ARP.
 */
/*ARGSUSED*/
static int
arp_inject(inject_t injection, net_inject_t *neti, netstack_t *ns)
{
	return (-1);
}

/*
 * Unsupported with ARP.
 */
/*ARGSUSED*/
static phy_if_t
arp_routeto(struct sockaddr *addr, netstack_t *ns)
{
	return ((phy_if_t)-1);
}

/*
 * Unsupported with ARP.
 */
/*ARGSUSED*/
int
arp_ispartialchecksum(mblk_t *mb)
{
	return (-1);
}

/*
 * Unsupported with ARP.
 */
/*ARGSUSED*/
static int
arp_isvalidchecksum(mblk_t *mb)
{
	return (-1);
}
