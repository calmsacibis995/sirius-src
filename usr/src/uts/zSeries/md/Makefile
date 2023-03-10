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
# uts/s390x/md/Makefile
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#	This makefile drives the production of the SLVM's md driver module.
#
#	s390x implementation architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		= md
OBJECTS		= $(MD_OBJS:%=$(OBJS_DIR)/%)
OBJECTS		+= $(MD_COMMON_OBJS:%=$(OBJS_DIR)/%)
OBJECTS		+= $(MD_DERIVED_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(MD_OBJS:%.o=$(LINTS_DIR)/%.ln)
LINTS		+= $(MD_COMMON_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)
CONF_SRCDIR     = $(UTSBASE)/common/io/lvm/md

#
#	Include common rules.
#
include $(UTSBASE)/zSeries/Makefile.zSeries

#
#	Define targets
#
ALL_TARGET	= $(BINARY)
LINT_TARGET	= $(MODULE).lint
INSTALL_TARGET	= $(BINARY) $(ROOTMODULE) $(ROOT_CONFFILE)

#
#	Overrides.
#
CFLAGS		+= $(CCVERBOSE)

#
#	Default build targets.
#
.KEEP_STATE:

def:		derived_h .WAIT $(DEF_DEPS)

all:		derived_h .WAIT $(ALL_DEPS)

clean:		derived_h .WAIT $(CLEAN_DEPS)

clobber:	derived_h .WAIT $(CLOBBER_DEPS)

lint:		$(LINT_DEPS)

modlintlib:	derived_h .WAIT $(MODLINTLIB_DEPS)

clean.lint:	derived_h .WAIT $(CLEAN_LINT_DEPS)

install:	derived_h .WAIT $(INSTALL_DEPS)

derived_h:
	cd $(UTSBASE)/common/sys/lvm; pwd; $(MAKE) $(TARGET)
#
#	Include common targets.
#
include $(UTSBASE)/zSeries/Makefile.targ
