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
 *
 * Copyright 2008 Sine Nomine Associates.
 * All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * Routines to capture processor-dependencies in event specification.
 */

#include <sys/types.h>
#include <string.h>
#include <strings.h>
#include <alloca.h>
#include <stdlib.h>
#include <stdio.h>
#include <libintl.h>
#include <assert.h>

#include "libcpc.h"
#include "libcpc_impl.h"

/*
 * By default, user event counting is enabled, system event counting
 * is disabled.
 *
 * The routine counts the number of errors encountered while parsing
 * the string, if no errors are encountered, the event handle is
 * returned.
 */

const char *
cpc_getusage(int cpuver)
{
	return (NULL);
}

int
cpc_strtoevent(int cpuver, const char *spec, cpc_event_t *event)
{
	int errcnt = 0;

	return (errcnt = 1);
}

char *
cpc_eventtostr(cpc_event_t *event)
{
	return (NULL);
}

/*
 * Utility operations on events
 */
void
cpc_event_accum(cpc_event_t *accum, cpc_event_t *event)
{
	return;
}

void
cpc_event_diff(cpc_event_t *diff, cpc_event_t *left, cpc_event_t *right)
{
	return;
}

/*
 * Given a cpc_event_t and cpc_bind_event() flags, translate the event into the
 * cpc_set_t format.
 *
 * Returns NULL on failure.
 */
cpc_set_t *
__cpc_eventtoset(cpc_t *cpc, cpc_event_t *event, int iflags)
{
	return (NULL);
}
