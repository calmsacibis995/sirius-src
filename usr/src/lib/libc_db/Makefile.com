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

LIBRARY = libc_db.a
VERS = .1

CRTI=	crti.o
CRTN=	crtn.o

CMNOBJS = thread_db.o
OBJECTS = $(CRTI) $(CMNOBJS) $(CRTN)

include	../../Makefile.lib
include ../../Makefile.rootfs

LIBS = $(DYNLIB) $(LINTLIB)

SRCDIR =	../common
SRCS = $(CMNOBJS:%.o=$(SRCDIR)/%.c)
$(LINTLIB) := SRCS = $(SRCDIR)/$(LINTSRC)

sparc_ASFLAGS += -P -D__STDC__ -D_ASM -DPIC
i386_ASFLAGS  += -P -D__STDC__ -D_ASM -DPIC
s390_ASFLAGS  += -D__STDC__ -D_ASM -DPIC

ASFLAGS       += $($(MACH)_ASFLAGS)
CPPFLAGS +=	-I../../libc/inc -D_REENTRANT
CFLAGS +=	$(CCVERBOSE)
CFLAGS +=	$(CCVERBOSE)
LDLIBS +=	-lc

.KEEP_STATE:

all: $(LIBS)

lint: lintcheck

include	../../Makefile.targ

pics/%.o: $(CRTSRCS)/%.s
	$(COMPILE.s) -o $@ $<
	$(POST_PROCESS_O)
