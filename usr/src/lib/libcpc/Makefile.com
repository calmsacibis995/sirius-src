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
# Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# ident	"%Z%%M%	%I%	%E% SMI"
#

LIBRARY = libcpc.a
VERS	= .1
COBJS	= libcpc.o subr.o
V1_OBJS = obsoleted.o
OBJECTS = $(ASOBJS) $(MACHCOBJS) $(COBJS) $(V1_OBJS)

include ../../Makefile.lib

SRCS = 	$(ASOBJS:%.o=../$(MACH)/%.s)	\
	$(MACHCOBJS:%.o=../$(MACH)/%.c)	\
	$(V1_OBJS:%.o=../common/%.c)	\
	$(COBJS:%.o=../common/%.c)

LIBS =		$(DYNLIB) $(LINTLIB)
$(LINTLIB) :=	SRCS = ../common/llib-lcpc
LDLIBS +=	-lpctx -lnvpair -lc

SRCDIR =	../common

MAPFILES +=	mapfile-vers

sparc_ASFLAGS += -P -D_ASM -I../common
i386_ASFLAGS  += -P -D_ASM -I../common
s390_ASFLAGS  += -D_ASM -I../common

ASFLAGS += 	$($(MACH)_ASFLAGS)
CPPFLAGS +=	-D_REENTRANT -I../common
CFLAGS +=	$(CCVERBOSE)

.KEEP_STATE:

all: $(LIBS)

lint: lintcheck

include ../../Makefile.targ

pics/%.o: ../$(MACH)/%.c
	$(COMPILE.c) -o $@ $<
	$(POST_PROCESS_O)

pics/%.o: ../$(MACH)/%.s
	$(COMPILE.s) -o $@ $<
	$(POST_PROCESS_O)
