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

#ifndef	_SES_H
#define	_SES_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <assert.h>

#include <scsi/libses.h>

#include <fm/topo_mod.h>
#include <fm/topo_list.h>
#include <fm/topo_method.h>

#ifdef	__cplusplus
extern "C" {
#endif

extern ses_node_t *ses_node_get(topo_mod_t *, tnode_t *);

extern int ses_node_enum_facility(topo_mod_t *, tnode_t *, topo_version_t,
    nvlist_t *, nvlist_t **);
extern int ses_enc_enum_facility(topo_mod_t *, tnode_t *, topo_version_t,
    nvlist_t *, nvlist_t **);

#define	TOPO_PGROUP_SES		"ses"
#define	TOPO_PROP_NODE_ID	"node-id"
#define	TOPO_PROP_TARGET_PATH	"target-path"

#ifndef	NDEBUG
#define	verify(x)	assert(x)
#else
#define	verify(x)	((void)(x))
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SES_H */
