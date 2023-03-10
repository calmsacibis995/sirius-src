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
# Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# ident	"%Z%%M%	%I%	%E% SMI"
#

LIBRARY = libfmd_snmp.a
VERS = .1

LIBSRCS = \
	debug_subr.c	\
	init.c		\
	module.c	\
	problem.c	\
	resource.c	\
	scheme.c

OBJECTS = $(LIBSRCS:%.c=%.o)

include ../../../Makefile.lib
include ../../Makefile.lib

SRCS = $(LIBSRCS:%.c=../common/%.c)
LIBS = $(DYNLIB) $(LINTLIB)

SRCDIR =	../common

CPPFLAGS += -I../common -I. -I/usr/sfw/include
$(NOT_RELEASE_BUILD)CPPFLAGS += -DDEBUG
CFLAGS += $(CCVERBOSE) $(C_BIGPICFLAGS)
CFLAGS64 += $(CCVERBOSE) $(C_BIGPICFLAGS)

# No lint libraries are delivered for Net-SNMP yet
sparc_SNMPDIR = -L$(SFWLIBDIR)
i38l_SNMPDIR =	-L$(SFWLIBDIR)
s390_SNMPDIR = 
SNMPLIBS = $($(MACH)_SNMPDIR) -lnetsnmp -lnetsnmphelpers -lnetsnmpagent
lint := SNMPLIBS=

LDLIBS += $(MACH_LDLIBS)
LDLIBS += -lfmd_adm -luutil -lnvpair -ltopo
LDLIBS += $(SNMPLIBS)
LDLIBS += -lc
DYNFLAGS += -R$(SFWLIBDIR)

LINTFLAGS = -msux
LINTFLAGS64 = -msux -Xarch=$(MACH64:sparcv9=v9)

# Net-SNMP's headers use do {} while (0) a lot
LINTCHECKFLAGS += -erroff=E_CONSTANT_CONDITION

$(LINTLIB) := SRCS = $(SRCDIR)/$(LINTSRC)
$(LINTLIB) := LINTFLAGS = -nsvx
$(LINTLIB) := LINTFLAGS64 = -nsvx -Xarch=$(MACH64:sparcv9=v9)

.KEEP_STATE:

all: $(LIBS)

lint: $(LINTLIB) lintcheck

pics/%.o: ../$(MACH)/%.c
	$(COMPILE.c) -o $@ $<
	$(POST_PROCESS_O)

%.o: ../common/%.c
	$(COMPILE.c) -o $@ $<
	$(POST_PROCESS_O)

include ../../../Makefile.targ
include ../../Makefile.targ
