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
# Copyright 2008 Sine Nomine Associates. All rights reserved.
# Use is subject to license terms.
#
#
# Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

# Fetch minor version from RELEASE, e.g. "10" for "5.10", "11" for "5.11" etc.
GETRELEASEMINOR = /usr/bin/ksh -c 'x=$(RELEASE) ; print "$${x$(POUND_SIGN)$(POUND_SIGN)*.}"'

# Platform-specific config
# Note that "HOSTTYPE" matches the value used by AT&T upstream.
ASTPLATFORMCPPFLAGS = '-DHOSTTYPE="sol$(GETRELEASEMINOR:sh).s390x"' 

# platform-specific sources
OBJECTS = \
	s390x/src/lib/libast/conftab.o
OBJDIRS = \
	s390x/src/lib/libast

include ../Makefile.com
include ../../Makefile.lib.64

# Use -KPIC since libast is too big for -Kpic on 64bit
# (and on 32bit it is close to the barrier)
s390x_C_PICFLAGS  = $(C_BIGPICFLAGS)
CPPFLAGS += -I$(ROOT)/usr/include 

install: all $(ROOTLIBS64) $(ROOTLINKS64)
