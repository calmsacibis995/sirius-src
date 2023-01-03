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

#ifndef _LIBDLVLAN_H
#define	_LIBDLVLAN_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

/*
 * This file includes structures, macros and routines used by VLAN link
 * administration.
 */

#include <libdladm.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct dladm_vlan_attr {
	uint16_t	dv_vid;
	datalink_id_t	dv_linkid;
	boolean_t	dv_force;
	boolean_t	dv_implicit;
} dladm_vlan_attr_t;

extern dladm_status_t	dladm_vlan_info(datalink_id_t, dladm_vlan_attr_t *,
			    uint32_t);
extern dladm_status_t	dladm_vlan_create(const char *, datalink_id_t,
			    uint16_t, uint32_t);
extern dladm_status_t	dladm_vlan_delete(datalink_id_t, uint32_t);
extern dladm_status_t	dladm_vlan_up(datalink_id_t);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBDLVLAN_H */
