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
#
#ident	"%Z%%M%	%I%	%E% SMI"
#
# Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# cmd/w/s390/Makefile
#

PROG=	w

OBJS=	w.o

SRCS=	$(OBJS:%.o=../%.c)

include ../../Makefile.cmd

CFLAGS	+= $(CCVERBOSE)

lint	:= LINTFLAGS = -x

ROOTLINK= $(ROOTBIN32)/uptime

FILEMODE= 4555
OWNER= root		# w needs to read /proc entries
GROUP= bin

.KEEP_STATE:

%.o:	../%.c
	$(COMPILE.c) $<

all: $(PROG)

$(PROG): $(OBJS)
	$(LINK.c) $(OBJS) -o $@ $(LDLIBS)
	$(POST_PROCESS)

install: all $(ROOTLINK)

$(ROOTLINK):	$(ROOTPROG32)
	$(RM) $@
	$(LN) $(ROOTPROG32) $@

clean:
	$(RM) $(OBJS)

lint:
	$(LINT.c) $(SRCS) $(LDLIBS)

include ../../Makefile.targ
