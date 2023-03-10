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
# uts/s390x/ecpp/Makefile
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#	This Makefile drives the production of ecpp
#	kernel module.
#	
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		= ecpp
OBJECTS		= $(ECPP_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(ECPP_OBJS:%.o=$(LINTS_DIR)/%.ln)
WARLOCK_OUT	= $(ECPP_OBJS:%.o=%.ll)
WARLOCK_OK	= $(MODULE).ok
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)
CONF_SRCDIR	= $(UTSBASE)/common/io

#
#	Include common rules.
#
include $(UTSBASE)/zSeries/Makefile.zSeries

#
#	Define targets
#
ALL_TARGET	= $(BINARY) $(SRC_CONFILE)
LINT_TARGET	= $(MODULE).lint
INSTALL_TARGET	= $(BINARY) $(ROOTMODULE) $(ROOT_CONFFILE)

#
# lint pass one enforcement
#
CFLAGS += $(CCVERBOSE)

# Turn this on once compiler understands v9 in it's backend
#INLINES		+= $(UTSBASE)/sun/io/ecpp.il

#	Default build targets.
#
.KEEP_STATE:

def:		$(DEF_DEPS)

all:		$(ALL_DEPS)

clean:		$(CLEAN_DEPS)

clean:		$(CLEAN_DEPS); \
		$(RM) $(WARLOCK_OUT) $(WARLOCK_OK)

clobber:	$(CLOBBER_DEPS); \
		$(RM) $(WARLOCK_OUT) $(WARLOCK_OK)

modlintlib:	$(MODLINTLIB_DEPS) lint64

clean.lint:	$(CLEAN_LINT_DEPS)

install:	$(INSTALL_DEPS)

#
#	Include common targets.
#
include $(UTSBASE)/zSeries/Makefile.targ

#
#	Defines for local commands.
#
WARLOCK		= warlock
WLCC		= wlcc
TOUCH		= touch
SCCS		= sccs
TEST		= test

#
#	lock_lint rules
#
ECPP_FILES	= $(MODULE).ll

warlock: $(MODULE).ok

%.wlcmd:
	$(TEST) -f $@ || $(SCCS) get $@

ecpp.ok: ecpp.ll ecpp.wlcmd warlock_ddi.files
	$(WARLOCK) -c ecpp.wlcmd $(ECPP_FILES) \
		-l ../warlock/ddi_dki_impl.ll
	$(TOUCH) $@


%.ll: $(UTSBASE)/common/io/%.c $(UTSBASE)/common/sys/ecppvar.h \
		$(UTSBASE)/common/sys/ecppio.h
	$(WLCC) $(CPPFLAGS) -DDEBUG -o $@ $<

warlock_ddi.files:
	cd ../warlock; pwd; $(MAKE) warlock
