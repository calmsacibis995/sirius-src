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
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# ident	"%Z%%M%	%I%	%E% SMI"
#

PROG= init 
ROOTFS_PROG= $(PROG)

DEFAULTFILES= init.dfl

s390x_CROSS=

include ../Makefile.cmd

OWNER = root
GROUP = sys

LDLIBS += -lpam -lbsm -lcontract -lscf
LINTFLAGS += -erroff=E_NAME_USED_NOT_DEF2
CLOBBERFILES= $(STATIC)

.KEEP_STATE:

all: $(ROOTFS_PROG)

install: all $(ROOTETCDEFAULTFILES) $(ROOTSBINPROG)
	$(RM) $(ROOTETCPROG)
	$(RM) $(ROOTUSRSBINPROG)
	$(RM) $(ROOTETC)/telinit
	$(SYMLINK) ../sbin/$(PROG) $(ROOTETCPROG)
	$(SYMLINK) ../../sbin/$(PROG) $(ROOTUSRSBINPROG)
	$(SYMLINK) ../sbin/$(PROG) $(ROOTETC)/telinit
	$(RM) $(ROOTETC)/TIMEZONE
	cd $(ROOTETC); $(SYMLINK) default/init TIMEZONE

test:
	rtest $(PROG)

clean:

lint:	lint_PROG

include ../Makefile.targ
