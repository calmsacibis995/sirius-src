#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
#
# Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# ident	"%Z%%M%	%I%	%E% SMI"
#
{
    local:
	__imax_lldiv;
	_ti_thr_self ;
	rw_read_is_held;
	rw_write_is_held;
	__cerror;
	__cerror64;
	_seekdir64;
	_telldir64;
	find_stack;
	set_cancel_pending_flag;
	do_exit_critical;
	_thrp_suspend;
	unsleep_self;
	set_parking_flag;
	sigon;
	__assfail;
};
