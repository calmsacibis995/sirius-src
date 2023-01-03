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
#ident	"%Z%%M%	%I%	%E% SMI"
#
# Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# cmd/modload/Makefile.com
#
# makefile for loadable module utilities

DRVPROG = add_drv rem_drv update_drv
MODPROG = modinfo modunload modload
PROG = $(MODPROG) $(DRVPROG)

include ../../Makefile.cmd

MODCOMMONOBJ = modsubr.o
MODCOMMONSRC = $(MODCOMMONOBJ:%.o=../%.c)

PLCYOBJ = plcysubr.o
PLCYSRC = $(PLCYOBJ:%.o=../%.c)

$(PLCYOBJ) := CPPFLAGS += -D_REENTRANT

ROOTDRVPROG = $(DRVPROG:%=$(ROOTUSRSBIN)/%)

DRVCOMMONOBJ = drvsubr.o $(PLCYOBJ)
DRVCOMMONSRC = $(DRVCOMMONOBJ:%.o=../%.c)

OBJECTS = $(MODCOMMONOBJ) $(DRVCOMMONOBJ) $(PROG:%=%.o)
SRCS = $(OBJECTS:%.o=../%.c)

CLOBBERFILES = $(PROG)

# install specifics

$(ROOTDRVPROG) := FILEMODE = 0555

add_drv		:= LDLIBS += -ldevinfo -lelf
rem_drv		:= LDLIBS += -ldevinfo
update_drv	:= LDLIBS += -ldevinfo

.KEEP_STATE:

%.o:	../%.c
	$(COMPILE.c) $<

all: $(PROG)

add_drv:	add_drv.o $(DRVCOMMONOBJ)
	$(LINK.c)  -o $@ add_drv.o $(DRVCOMMONOBJ) $(LDLIBS)
	$(POST_PROCESS)

rem_drv:	rem_drv.o $(DRVCOMMONOBJ)
	$(LINK.c)  -o $@ rem_drv.o $(DRVCOMMONOBJ) $(LDLIBS)
	$(POST_PROCESS)

update_drv:	update_drv.o $(DRVCOMMONOBJ)
	$(LINK.c)  -o $@ update_drv.o $(DRVCOMMONOBJ) $(LDLIBS)
	$(POST_PROCESS)

modload:	modload.o $(MODCOMMONOBJ)
	 $(LINK.c)  -o $@ modload.o $(MODCOMMONOBJ) $(LDLIBS)
	$(POST_PROCESS)

modunload:	modunload.o $(MODCOMMONOBJ)
	 $(LINK.c)  -o $@ modunload.o $(MODCOMMONOBJ) $(LDLIBS)
	$(POST_PROCESS)

modinfo:	modinfo.o $(MODCOMMONOBJ)
	 $(LINK.c)  -o $@ modinfo.o $(MODCOMMONOBJ) $(LDLIBS)
	$(POST_PROCESS)

clean:
	$(RM) $(OBJECTS) core

lint: lint_SRCS

include ../../Makefile.targ
