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
#	This makefile drives the production of the ibmf kernel module.
#
#	s390x architecture dependent
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		= ibmf
OBJECTS		= $(IBMF_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(IBMF_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_MISC_DIR)/$(MODULE)
MDFLAGS		+= -Nmisc/ibtl
LDFLAGS		+= -dy
WARLOCK_OUT     = $(IBMF_OBJS:%.o=%.ll)
WARLOCK_OK      = $(MODULE).ok

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
#	Overrides
#

#DEBUG_DEFS_DBG64 += -DTNF_DEBUG
ALL_BUILDS	= $(ALL_BUILDSONLY64)
DEF_BUILDS	= $(DEF_BUILDSONLY64)

# 
# lint pass one enforcement 
# 
CFLAGS += $(CCVERBOSE)

#
#	Default build targets.
#
.KEEP_STATE:

def:		$(DEF_DEPS)

all:		$(ALL_DEPS)

clean:		$(CLEAN_DEPS); \
		$(RM) $(WARLOCK_OUT) $(WARLOCK_OK)

clobber:	$(CLOBBER_DEPS); \
		$(RM) $(WARLOCK_OUT) $(WARLOCK_OK)

lint:		$(LINT_DEPS)

modlintlib:	$(MODLINTLIB_DEPS) lint32

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
SCS		= sccs
TEST		= test

#
#	Warlock targets
#
WARLOCK_OBJECTS	= $(IBMF_OBJS:%.o=%.ll)
IBMF_FILES = $(IBMF_OBJS:%.o= -l ../ibmf/%.ll)

warlock: $(MODULE).ok

%.wlcmd:
	$(TEST) -f $@ || @$(SCCS) get $@

$(MODULE).ok: $(WARLOCK_OBJECTS) warlock_ddi.files ibmf.wlcmd
	$(WARLOCK) -c ibmf.wlcmd $(WARLOCK_OBJECTS) $(IBMF_FILES) \
		-l ../warlock/ddi_dki_impl.ll
	$(TOUCH) $@

%.ll: $(UTSBASE)/common/io/ib/mgt/ibmf/%.c
	$(WLCC) $(CPPFLAGS) -DNPROBE -DDEBUG -o $@ $<

warlock_ddi.files:
	@cd ../warlock; pwd; $(MAKE) warlock
