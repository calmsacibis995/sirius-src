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
# uts/s390x/ehci/Makefile
#
# Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#	This makefile drives the production of the ehci driver kernel module.
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
MODULE		= ehci 
OBJECTS		= $(EHCI_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(EHCI_OBJS:%.o=$(LINTS_DIR)/%.ln)
WARLOCK_OUT	= $(EHCI_OBJS:%.o=%.ll)
WARLOCK_OK	= $(MODULE).ok
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)
CONF_SRCDIR	= $(UTSBASE)/common/io/usb/hcd/ehci
	

#
#	Include common rules.
#
include $(UTSBASE)/zSeries/Makefile.zSeries

#
# lint pass one enforcement
#
CFLAGS += $(CCVERBOSE)   

#
# depends on misc/usba
#
MDFLAGS         += -Nmisc/usba
LDFLAGS         += -dy

#
#	Define targets
#
ALL_TARGET	= $(BINARY) $(SRC_CONFFILE)
LINT_TARGET	= $(MODULE).lint
INSTALL_TARGET	= $(BINARY) $(ROOTMODULE) $(ROOT_CONFFILE)

.KEEP_STATE:

all:		$(ALL_DEPS)

def:		$(DEF_DEPS)

clean:		$(CLEAN_DEPS); \
                $(RM) $(WARLOCK_OUT) $(WARLOCK_OK)

clobber:	$(CLOBBER_DEPS); \
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
WARLOCK		= warlock
WLCC		= wlcc
TOUCH		= touch
SCCS		= sccs
TEST		= test

#
#	lock_lint rules
#
WARLOCK_OBJECTS = $(EHCI_OBJS:%.o=%.ll)
USBA_FILES = $(USBA_OBJS:%.o= -l ../usba/%.ll)

warlock: $(MODULE).ok $(WARLOCK_OBJECTS)

%.wlcmd:
	$(TEST) -f $@ || $(SCCS) get $@

ehci.ok: $(WARLOCK_OBJECTS) ehci.wlcmd warlock_ddi.files usba.files
	$(WARLOCK) -c ehci.wlcmd  $(WARLOCK_OBJECTS) \
	$(USBA_FILES) \
	-l ../warlock/ddi_dki_impl.ll
	$(TOUCH) $@

%.ll: $(UTSBASE)/common/io/usb/hcd/ehci/%.c
	$(WLCC) $(CPPFLAGS) -DDEBUG -o $@ $<

warlock_ddi.files:
	@cd ../warlock; pwd; $(MAKE) warlock

usba.files:
	@cd ../usba; pwd; $(MAKE) warlock
