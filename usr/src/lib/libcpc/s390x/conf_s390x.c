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

#include <sys/types.h>
#include <string.h>
#include <alloca.h>
#include <stdlib.h>
#include <stdio.h>
#include <libintl.h>
#include <libdevinfo.h>

#include "libcpc.h"
#include "libcpc_impl.h"

/*
 * Configuration data for System z performance counters.
 *
 */

void
cpc_walk_names(int cpuver, int regno, void *arg,
    void (*action)(void *, int, const char *, uint8_t))
{
	return;
}

const char *
__cpc_reg_to_name(int cpuver, int regno, uint8_t bits)
{
	return (NULL);
}

/*
 * Register names can be specified as strings or even as numbers
 */
int
__cpc_name_to_reg(int cpuver, int regno, const char *name, uint8_t *bits)
{
	return (-1);
}

const char *
cpc_getcciname(int cpuver)
{
	return (NULL);
}

#define	CPU_REF_URL " Documentation for IBM processors can be found at: " \
			"http://www.ibm.com/systems/z/hardware/"

const char *
cpc_getcpuref(int cpuver)
{
	return (NULL);
}

/*
 * This is a functional interface to allow CPUs with fewer %pic registers
 * to share the same data structure as those with more %pic registers
 * within the same instruction family.
 */
uint_t
cpc_getnpic(int cpuver)
{
	return (0);
}

/*
 * Return the version of the current processor.
 *
 * Version -1 is defined as 'not performance counter capable'
 *
 */
int
cpc_getcpuver(void)
{
	static int ver = -1;

	return (ver);
}
