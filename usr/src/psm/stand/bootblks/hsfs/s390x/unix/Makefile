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
# Copyright (c) 1994, by Sun Microsystems, Inc.
# All rights reserved.
#
# psm/stand/bootblks/hsfs/sparc/unix/Makefile
#

#
# Build the "Unix" version of the boot block.
# Useful solely for testing.  DO NOT INSTALL IT!
#
BASEDIR		= ../../..
PLATFORM	= unix
PLATFORM_INCS	= ./
PLATLINKS	=

include $(BASEDIR)/hsfs/Makefile.hsfs
include $(BASEDIR)/obp-c/Makefile.rules

CPPINCS		+= -I$(SRC)/head
CPPINCS		+= -I$(SRC)/uts/common
CPPINCS		+= -I$(SRC)/uts/s390x
CPPINCS		+= -I$(PLATFORM_INCS)

UNIX_OBJS	= hsfs_small.o unix_devio.o

$(UNIX_OBJS) :=	CFLAGS += -g

.KEEP_STATE:

all:	$(PROG)

$(PROG): $(UNIX_OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -g -o $@ $(UNIX_OBJS)

lint: FRC

clean:
	$(RM) $(UNIX_OBJS)

clobber: clean
	$(RM) $(PROG)

install: FRC

FRC:

