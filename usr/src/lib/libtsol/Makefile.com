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

LIBRARY =	libtsol.a
VERS =		.2

OBJECTS = \
	blabel.o btohex.o btos.o call_labeld.o \
	getlabel.o getplabel.o hextob.o \
	ltos.o misc.o getpathbylabel.o private.o privlib.o \
	setflabel.o stob.o stol.o zone.o

include ../../Makefile.lib

# install this library in the root filesystem
include ../../Makefile.rootfs

LIBS =		$(DYNLIB) $(LINTLIB)
LDLIBS +=	-lsecdb -lc

SRCDIR =	../common
$(LINTLIB):=	SRCS = $(SRCDIR)/$(LINTSRC)

NONCOMMON =	$(OBJECTS:blabel.o=)
lint:=		SRCS = $(NONCOMMON:%.o=$(SRCDIR)/%.c) $(COMMONDIR)/blabel.c

COMMONDIR=	$(SRC)/common/tsol

CFLAGS +=	$(CCVERBOSE)
CPPFLAGS +=	-D_REENTRANT -I$(SRCDIR) -I$(COMMONDIR)

LINTFLAGS64 +=	-Xarch=v9

.KEEP_STATE:

all: $(LIBS)

lint: lintcheck

objs/%.o pic_profs/%.o pics/%.o:	$(COMMONDIR)/%.c
	$(COMPILE.c) -o $@ $<
	$(POST_PROCESS_O)

include ../../Makefile.targ
