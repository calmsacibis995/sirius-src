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
# uts/s390x/cmlb/Makefile
#
#	This makefile drives the production of the cmlb "misc"
#	kernel module.
#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		= cmlb
OBJECTS		= $(CMLB_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(CMLB_OBJS:%.o=$(LINTS_DIR)/%.ln)
WARLOCK_OUT	= $(CMLB_OBJS:%.o=%.ll)
WARLOCK_OK	= $(MODULE).ok
ROOTMODULE	= $(ROOT_MISC_DIR)/$(MODULE)

#
#	Include common rules.
#
include $(UTSBASE)/zSeries/Makefile.zSeries

#
#	Define targets
#
ALL_TARGET	= $(BINARY)
LINT_TARGET	= $(MODULE).lint
INSTALL_TARGET	= $(BINARY) $(ROOTMODULE)

#
#	Overrides.
#
DEBUG_FLGS	=
DEBUG_DEFS	+= $(DEBUG_FLGS)

#
#	Default build targets.
#
.KEEP_STATE:

def:		$(DEF_DEPS)

all:		$(ALL_DEPS)

clean:		$(CLEAN_DEPS)
		$(RM) $(WARLOCK_OUT) $(WARLOCK_OK)

clobber:	$(CLOBBER_DEPS)
		$(RM) $(WARLOCK_OUT) $(WARLOCK_OK)

lint:		$(LINT_DEPS)

modlintlib:	$(MODLINTLIB_DEPS)

clean.lint:	$(CLEAN_LINT_DEPS)

install:	$(INSTALL_DEPS)

#
#	Include common targets.
#
include $(UTSBASE)/zSeries/Makefile.targ

#
#	Defines for local commands.
#
WLCC		= wlcc
TOUCH		= touch
WARLOCK		= warlock

#
#	Warlock targets
#

CMLB_FILES = $(CMLB_OBJS:%.o=%.ll)

warlock: $(MODULE).ok 

%.ok:	$(CMLB_FILES)
	$(TOUCH) $@

%.ll: $(UTSBASE)/common/io/%.c
	$(WLCC) $(CPPFLAGS) -DDEBUG -o $@ $<
