#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License                  
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
# Copyright 2008 Sine Nomine Associates. All rights reserved.
# Use is subject to license terms.
#
#
# Copyright 2003 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# cmd/sgs/libdl/s390x/Makefile

include		../Makefile.com

CONVLIBDIR=	$(CONVLIBDIR64)
DYNFLAGS +=	-F /usr/lib/$(MACH64)/ld.so.1 $(MAPOPTS)
LINTFLAGS64 +=	$(VAR_LINTFLAGS64)

.KEEP_STATE:

include		../Makefile.targ
include		../../Makefile.sub.64

all:		$(DYNLIB) $(LINTLIB)

install:	all $(ROOTFS_DYNLIB64) $(ROOTFS_LINKS64) $(ROOTFS_LINTLIB64)

$(DYNLIB):	$(MAPFILES)

BUILD.SO=	$(LD) -o $@ -G $(DYNFLAGS) $(PICS)

# Add build for generated mapfile
include ../../../../Makefile.mapfilegnu
