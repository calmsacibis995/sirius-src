#
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only
# (the "License").  You may not use this file except in compliance
# with the License.
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
#pragma ident	"%Z%%M%	%I%	%E% SMI"

@ _START_

# Message file for cmd/sgs/rtld (ld.so.1) - 31bit z-Series specific.

# General error diagnostics (a.out)

@ MSG_REL_OVERFLOW      "relocation error: file %s: symbol %s: \
			 value 0x%x overflows %d bits at offset 0x%x"

@ MSG_GEN_OLDREV	"%s: has older revision than expected %s"

@ _END_

@ MSG_STR_AOUT		"AOUT"

@ MSG_FMT_4XPATH	"./%s"
@ MSG_FMT_4XLIB		"lib%s.so.%d.%d"

@ MSG_FIL_DOTSODOT	".so."
@ MSG_FIL_LIB		"lib"

@ MSG_PTH_USRLCLIB	"/usr/local/lib"
@ MSG_PTH_USR4LIB	"/usr/4lib"
@ MSG_PTH_USRUCBLIB	"/usr/ucblib"

@ MSG_SYM_LA390PLTENTER	"la_s390_pltenter"

@ MSG_SYM_CTYPE		"__ctype"
@ MSG_SYM_IOB		"__iob"
@ MSG_PTH_LIBC		"/usr/lib/libc.so.1"
