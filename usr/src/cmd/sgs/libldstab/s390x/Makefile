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
# Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

DUPOBJS=	stab64.o

include ../Makefile.com

.KEEP_STATE:

ROOTLIBDIR=	$(ROOT)/usr/lib/$(MACH64)
ROOTDYNLIB64=	$(DYNLIB:%=$(ROOTLIBDIR)/%)

CONVLIBDIR=	$(CONVLIBDIR64)
ELFLIBDIR=	$(ELFLIBDIR64)

MAPFILES += ../common/mapfile-64

LINTFLAGS64 += $(VAR_LINTFLAGS64)

all:		$(DYNLIB)
install \
package:	all $(ROOTDYNLIB64)
lint:		$(LINTOUT64)

include	../Makefile.targ
include	../../Makefile.sub.64

