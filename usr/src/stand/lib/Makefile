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
#
# Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# ident	"%Z%%M%	%I%	%E% SMI"
#

include Makefile.com

i386_SUBDIRS = fs/pcfs fs/compfs
sparc_SUBDIRS = md5 nvpair scrypt wanboot crypto ssl
s390_SUBDIRS = md5 nvpair scrypt ssl
SUBDIRS = fakeboot fs/hsfs fs/nfs fs/ufs inet sa sock tcpstubs xdr \
	tcp $($(MACH)_SUBDIRS)

#
# It would be groovy if everything was lint-clean, but the crypto and ssl
# code is third-party swill out of our jurisdiction.  Also, don't lint
# fakeboot since it doesn't have any source files.
#
# If someone can come up with a more elegant way to strip these
# directories out, please make it so.
#
LINTSUBDIRS1 = $(SUBDIRS:ssl=)
LINTSUBDIRS2 = $(LINTSUBDIRS1:crypto=)
LINTSUBDIRS  = $(LINTSUBDIRS2:fakeboot=)

#
# We can get away with this since we're only building .a's, and by the
# time we run lint all of the lint libraries have already been built.
#
.PARALLEL: $(SUBDIRS)

.KEEP_STATE:

all	:=	TARGET = all
clean	:=	TARGET = clean
clobber	:=	TARGET = clobber
install	:=	TARGET = install
lint	:=	TARGET = lint

all clean clobber install: $(SUBDIRS)

lint: $(LINTSUBDIRS)

#
# The reason this rule checks for the existence of the
# Makefile is that some of the directories do not exist
# in our exportable source builds.
#
$(SUBDIRS): FRC
	@if [ -f $@/Makefile  ]; then \
		cd $@; pwd; $(MAKE) $(TARGET); \
	else \
		true; \
	fi

FRC:

#
# Cross-reference customization: set the include path to be reasonable.
#
XRINCDIRS = $(CPPINCS:-YI,%=-I%)
