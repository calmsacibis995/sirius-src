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
MODULE = dtrace.so
MDBTGT = kvm

MODSRCS = dtrace.c dof.c dof_names.c

include ../../../../Makefile.cmd
include ../../../../Makefile.cmd.64
include ../../Makefile.sparcv9
include ../../../Makefile.module

MODULE_BUILD_TYPE = mdb
MODSRCS_DIR = ../../../common/modules/dtrace

CPPFLAGS += -I$(SRC)/uts/sun4u
LDLIBS  += -ldtrace -lm
CLEANFILES += dof_names.c

dof_names.c: $(MODSRCS_DIR)/mkdof.sh $(SRC)/uts/common/sys/dtrace.h
	sh $(MODSRCS_DIR)/mkdof.sh < $(SRC)/uts/common/sys/dtrace.h > $@
