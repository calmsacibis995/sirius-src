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

MYPROG = svc.configd
MYOBJS = \
	backend.o \
	configd.o \
	client.o \
	file_object.o \
	maindoor.o \
	object.o \
	rc_node.o \
	snapshot.o

PROG = $(MYPROG)
OBJS = $(MYOBJS)

SRCS = $(MYOBJS:%.o=%.c)

include ../../Makefile.cmd
include ../Makefile.ctf

NATIVE_BUILD=$(POUND_SIGN)
$(NATIVE_BUILD)PROG = $(MYPROG:%=%-native)
$(NATIVE_BUILD)OBJS = $(MYOBJS:%.o=%-native.o)

ROOTCMDDIR=	$(ROOT)/lib/svc/bin

MYCPPFLAGS = -I. -I../common -I../../../common/svc -I$(ROOT)/usr/include/sqlite -D_REENTRANT
CPPFLAGS += $(MYCPPFLAGS)
CFLAGS	+= -v
MYLDLIBS = -lumem -luutil -lbsm
LDLIBS	+= -lsecdb $(MYLDLIBS)
LINTFLAGS += -errtags -erroff=E_BAD_FORMAT_ARG_TYPE2

CLOBBERFILES +=	$(MYPROG:%=%-native)

LIBUUTIL	= $(SRC)/lib/libuutil
LIBSCF		= $(SRC)/lib/libscf

SCRIPTFILE	= restore_repository
ROOTSCRIPTFILE	= $(ROOTCMDDIR)/$(SCRIPTFILE)

$(NATIVE_BUILD)CC =	$(NATIVECC)
$(NATIVE_BUILD)LD =	$(NATIVELD)
$(NATIVE_BUILD)CFLAGS =	$(NATIVE_CFLAGS)
$(NATIVE_BUILD)CPPFLAGS = $(MYCPPFLAGS) -I$(LIBUUTIL)/common -I$(LIBSCF)/inc
$(NATIVE_BUILD)CPPFLAGS += -DNATIVE_BUILD
$(NATIVE_BUILD)LDFLAGS =
$(NATIVE_BUILD)LDLIBS = -L$(LIBUUTIL)/native -R $(LIBUUTIL)/native \
	$(MYLDLIBS) -ldoor

DIRMODE = 0755
FILEMODE = 0555
OWNER = root
GROUP = sys

LIBSQLITE = $(ROOT)/usr/lib/libsqlite.o
$(NATIVE_BUILD)LIBSQLITE = $(ROOT)/usr/lib/libsqlite-native.o
SQLITELINT = $(ROOT)/usr/lib/llib-lsqlite.ln

OBJS += $(LIBSQLITE)

install := TARGET = install
clobber := TARGET = clobber

lint	:= LDLIBS += $(SQLITELINT)

.KEEP_STATE:
.PARALLEL: $(MYOBJS) $(MYOBJS:%.o=%-native.o)

all: $(PROG)

native: FRC

$(PROG): $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LDLIBS) $(CTFMERGE_HOOK)
	$(POST_PROCESS)

%-native.o: %.c
	$(COMPILE.c) -o $@ $< $(CTFCONVERT_HOOK)
	$(POST_PROCESS_O)

$(ROOTCMDDIR)/%: %.sh
	$(INS.rename)

install: all $(ROOTCMD) $(ROOTVARSADMFILE) $(ROOTSCRIPTFILE)

clean: FRC
	$(RM) $(MYOBJS) $(MYOBJS:%.o=%-native.o)

clobber:

lint:   lint_SRCS

lint_SRCS:

include ../../Makefile.targ

FRC:
