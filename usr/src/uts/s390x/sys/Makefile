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
# Copyright 2008 Sine Nomine Associates. All rights reserved.
# Use is subject to license terms.
#
#
#pragma ident	"%Z%%M%	%I%	%E% SMI"
#
# Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# uts/s390x/sys/Makefile
#
UTSBASE = ../..

#
# include global definitions
#
include ../Makefile.s390x

#
# Override defaults.
#
FILEMODE = 644
OWNER = root
GROUP = bin

HDRS=  			\
	clock.h		\
	ddi_subrdefs.h	\
	intr.h		\
	intreg.h	\
	mach_intr.h	\
	machbrand.h	\
	machclock.h	\
	machcpuvar.h	\
	machparam.h	\
	machpcb.h	\
	machsystm.h	\
	machthread.h	\
	nexusdebug.h	\
	rootnex.h	\
	smp_impldefs.h	\
	traptrace.h	\
	x_call.h

ROOTHDRS=	$(HDRS:%=$(USR_PSM_ISYS_DIR)/%)

ROOTDIR=	$(ROOT)/usr/share/src
ROOTDIRS=	$(ROOTDIR)/uts $(ROOTDIR)/uts/$(PLATFORM)

ROOTLINK=	$(ROOTDIR)/uts/$(PLATFORM)/sys
LINKDEST=	../../../../platform/$(PLATFORM)/include/sys

CHECKHDRS=	$(HDRS:%.h=%.check)

.KEEP_STATE:

.PARALLEL:	$(CHECKHDRS) $(ROOTHDRS)

install_h:	$(ROOTDIRS) .WAIT $(ROOTHDRS) $(ROOTLINK)
		@echo $(ROOTHDRS)
		@echo
		@echo $(ROOTDIRS)
		@echo
		@echo $(ROOTLINK)

check:		$(CHECKHDRS)

$(ROOTDIRS):
	$(INS.dir.root.bin)

$(ROOTLINK):	$(ROOTDIRS)
	-$(RM) -r $@; $(SYMLINK) $(LINKDEST) $@ $(CHOWNLINK) $(CHGRPLINK)

FRC:

include ../../Makefile.targ
