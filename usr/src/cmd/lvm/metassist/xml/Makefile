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

METASSIST_TOPLEVEL = ..

SRCS = xml_convert.c
OBJS = $(SRCS:%.c=%.o)
HDRS = $(SRCS:%.c=%.h)
MSGFILES = $(SRCS:%.c=%.i)

include $(METASSIST_TOPLEVEL)/../../Makefile.cmd
include $(METASSIST_TOPLEVEL)/Makefile.env

INCLUDES += -I$(ROOT)/usr/include/libxml2 -I/usr/include/libxml2 -I../common
CFLAGS   += $(INCLUDES)

POFILE = xmlp.po

include $(METASSIST_TOPLEVEL)/Makefile.targ

# Build .po file from message files
$(POFILE): $(MSGFILES)
	$(BUILDPO.msgfiles)

cstyle:
	$(CSTYLE) $(CSTYLE_FLAGS) $(SRCS) $(HDRS)

hdrchk:
	$(HDRCHK) $(HDRCHK_FLAGS) $(HDRS)
