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
# Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

include		$(SRC)/Makefile.master

ROOTDIR=	$(ROOT)/usr/lib/ld
ROOTDIR64=	$(ROOT)/usr/lib/ld/$(MACH64)

DIRS=		$(ROOTDIR) $(ROOTDIR64)
FILES=		$(ROOTDIR64)/map.above4G $(ROOTDIR64)/map.below4G

install: $(DIRS) $(FILES) map.link

$(FILES):=	FILEMODE=444
$(FILES):=	GROUP=bin

$(DIRS):=	DIRMODE=755
$(DIRS):=	GROUP=bin

map.link:
	$(RM) $(ROOTDIR64)/map.default
	$(SYMLINK) ./map.below4G $(ROOTDIR64)/map.default

$(DIRS):
	$(INS.dir)

$(ROOTDIR)/%: %
	$(INS.file)

$(ROOTDIR64)/%: %
	$(INS.file)
