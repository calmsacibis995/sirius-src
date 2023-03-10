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
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
MODULE = genunix.so
MDBTGT = kvm

COMMONSRCS = \
	avl.c \
	bio.c \
	contract.c \
	cpupart.c \
	ctxop.c \
	cyclic.c \
	devinfo.c \
	findstack.c \
	genunix.c \
	kgrep.c \
	kmem.c \
	ldi.c \
	leaky.c \
	leaky_subr.c \
	lgrp.c \
	list.c \
	log.c \
	memory.c \
	mmd.c \
	modhash.c \
	ndievents.c \
	net.c \
	nvpair.c \
	rctl.c \
	sobj.c \
	streams.c \
	sysevent.c \
	thread.c \
	tsd.c \
	vfs.c \
	zone.c

KMODSRCS = \
	$(COMMONSRCS)

MODSRCS = \
	$(COMMONSRCS) \
	typegraph.c

#
# This signals that $(KMODSRCS) != $(MODSRCS).  Typegraph is not usable under
# kmdb.  As such, we don't bother compiling it.
KMOD_SOURCES_DIFFERENT=$(POUND_SIGN)

include ../../../../Makefile.cmd
include ../../../../Makefile.cmd.64
include ../../Makefile.sparcv9
include ../../../Makefile.module

#
# We are not actually hardwiring some dependency on sun4u, we just need to
# include sys/param.h with _MACHDEP defined, and this forces the inclusion of
# machparam.h, even though we don't use anything there.  This is a temporary
# kludge until we invent -DDONTINCLUDEMACHPARAM or something.
#
CPPFLAGS += -I$(SRC)/uts/sun4u

# Needed to include c2/audit.h (from cred.h)
CPPFLAGS += -I$(SRC)/uts/common
# Needed to find include file mutex_impl.h
CPPFLAGS += -I$(SRC)/uts/sparc/v9
