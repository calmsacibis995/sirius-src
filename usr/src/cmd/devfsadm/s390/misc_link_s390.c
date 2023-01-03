/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License                  
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
/*                                                                  */
/* Copyright 2008 Sine Nomine Associates.                           */
/* All rights reserved.                                             */
/* Use is subject to license terms.                                 */
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <regex.h>
#include <devfsadm.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/mkdev.h>

static int node_name(di_minor_t minor, di_node_t node);


static int ddi_other(di_minor_t minor, di_node_t node);

static devfsadm_create_t misc_cbt[] = {
	{ "other", "ddi_other", NULL,
	    TYPE_EXACT, ILEVEL_0, ddi_other
	},
};

DEVFSADM_CREATE_INIT_V0(misc_cbt);

/*
 * Handles minor node type "ddi_other"
 * type=ddi_other;name=SUNW,pmc    pmc
 * type=ddi_other;name=SUNW,mic    mic\M0
 */
static int
ddi_other(di_minor_t minor, di_node_t node)
{
	char path[PATH_MAX + 1];
	char *nn = di_node_name(node);
	char *mn = di_minor_name(minor);

	if (strcmp(nn, "SUNW,pmc") == 0) {
		(void) devfsadm_mklink("pcm", node, minor, 0);
	} else if (strcmp(nn, "SUNW,mic") == 0) {
		(void) strcpy(path, "mic");
		(void) strcat(path, mn);
		(void) devfsadm_mklink(path, node, minor, 0);
	}

	return (DEVFSADM_CONTINUE);
}
