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
# Copyright (c) 1997-1999 by Sun Microsystems, Inc.
# All rights reserved.
#
# ucblib/libucb/s390x/Makefile

SYSOBJS=	 	\
	setjmp.o	\
	signal.o

MAPDIR=	../spec/s390x

include ../Makefile.com
include $(SRC)/lib/Makefile.lib.64

DYNFLAGS += $(DYNFLAGS64)
BUILD.SO= $(CC) $(CFLAGS) -o $@ -G $(DYNFLAGS) $(PICS) -L $(ROOT)/usr/lib/$(MACH64) $(LDLIBS)
BUILD.s=	$(AS) -D_ASM -I$(ROOT)/usr/ucbinclude -I$(ROOT)/usr/include $< -o $@


# s390x-specific rule
objs/%.o pics/%.o: sys/%.c
	$(COMPILE.c) -o $@ $<
	$(POST_PROCESS_O)

objs/%.o pics/%.o: sys/%.s
	$(BUILD.s)
	$(POST_PROCESS_O)

.KEEP_STATE:

install: all $(ROOTLIBS64) $(ROOTLINKS64) $(ROOTLINT64)
