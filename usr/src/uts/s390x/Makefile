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
# Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#	This makefile drives the production of all implementation architecture
#	dependent modules for the zSeries architecture.
#

UTSBASE = ..

include Makefile.s390x

#
#	The following are s390x specific (rather than zSeries) specific modules
#	which are required for the zSeries kernel to completely lint. They are
#	not involved in the build in any other way. In order to minimize
#	build time, it is assumed that they are up to date.
#
S390_LIB_DIR	 = $(UTSBASE)/s390x/lint-libs/$(OBJS_DIR)

S390_LINTS	 = genunix

LINT_LIBS	 = $(LINT_LIB) \
		   $(KMODS:%=$(LINT_LIB_DIR)/llib-l%.ln) \
		   $(CLOSED_KMODS:%=$(LINT_LIB_DIR)/llib-l%.ln) \
		   $(INTEL_LINTS:%=$(INTEL_LIB_DIR)/llib-l%.ln)

#
#
#
def		:=	TARGET= def
def.prereq	:=	TARGET= def
all		:=	TARGET= all
all.prereq	:=	TARGET= all
install		:=	TARGET= install
install.prereq	:=	TARGET= all
install_h	:=	TARGET= install_h
install_h.prere	:=	TARGET= install_h
clean		:=	TARGET= clean
clobber		:=	TARGET= clobber
lint		:=	TARGET= lint
lint.prereq	:=	TARGET= lint
lintlib		:=	TARGET= lintlib
machmodlintlib	:=	TARGET= modlintlib
modlist		:=	TARGET= modlist
modlist	modlist.s390x :=	NO_STATE= -K $$MODSTATE$$$$
clean.lint	:=	TARGET= clean.lint
check		:=	TARGET= check

.KEEP_STATE:

.PARALLEL:	$(KMODS) $(CLOSED_KMODS) $(XMODS) $(CLOSED_XMODS) \
		modlist modlist.s390x

INITIAL_TARGETS = \
	genassym \
	unix 

def all clean clobber clean.lint: genassym unix .WAIT \
	$(KMODS) $(CLOSED_KMODS) $(XMODS) $(CLOSED_XMODS)

install: install_platforms genassym unix .WAIT \
	$(KMODS) $(CLOSED_KMODS) $(XMODS) $(CLOSED_XMODS)

# list the modules under zSeries.
modlist: unix $(KMODS) $(CLOSED_KMODS) $(XMODS) $(CLOSED_XMODS)

# list the modules for Install -k zSeries.
modlist.karch: modlist modlist.s390x

modlist.s390x:
	@cd $(SRC)/uts/s390x; pwd; $(MAKE) $(NO_STATE) modlist

lintlib:	unix

modlintlib:	$(KMODS) $(CLOSED_KMODS)

genassym unix $(KMODS):	FRC
	@cd $@; pwd; $(MAKE) $(NO_STATE) $(TARGET)

#
# Privilege constants
#
# NOTE: The rules for generating priv_const.c file are shared between all
# processor architectures and should be kept in sync. If they are changed in
# this file make sure that x86 rules are updated as well.
#
PRIVS_C = $(UTSBASE)/common/os/priv_const.c

$(PRIVS_C): $(PRIVS_AWK) $(PRIVS_DEF)
	$(NAWK) -f $(PRIVS_AWK) < $(PRIVS_DEF) cfile=$@

#
# Prerequisites
#
# The uts/Makefile defines build parallelism for sun4 platforms such that s390x
# and zSeries are all built in parallel. Also this Makefile specifies that
# all IMPLEMENTATIONS s390x sub-platforms are built in parallel. This requires
# building certain parts before the parallel build can start. The uts/Makefile
# appends the '.prereq' string to the original target and executes this Makefile
# to build any prerequisites needed before the full parallel build can start.
# After that make continues with normal targets.
#
# Any build prerequisites for s390x and IMPLEMENTATIONS builds should be
# described here.
#
# genassym is used to build dtrace and genunix, so it should be built first.
#
# priv_const.c is required to build genunix.
#
# genunix is used by everyone to ctfmerge with. Genunix is merged with zSeries/ip
#   so as a side effect this dependency builds zSeries/ip as part of the
#   prerequisites.
#
# unix is not required by itself but several s390x platforms require
#   s390x/platmod to be present. The easiest way to achieve this is to build
#   s390x/unix first since s390x/unix Makefile builds s390x/platform correctly.
#   This causes full s390x/unix to be built before all s390x platforms and
#   before uts/zSeries, but it acceptable since it is not spending
#   too much time building s390x/unix.
#
all.prereq def.prereq install.prereq: genassym genunix unix

#
# Various s390x platforms expect proto/root_s390/platform/s390x/include to be
# present. This is handled by running make install_h in s390x/unix directory
# first.
#
install_h.prereq: FRC
	@cd sys; pwd; $(MAKE) $(TARGET)

#
# s390x/unix and s390x/genunix should be linted first since sparc does global
# cross-check with these lint libraries. The s390x/unix and s390x/genunix can be
# linted in parallel.
#
LINT_PREREQ = unix.lint genunix.lint
lint.prereq: $(LINT_PREREQ)

.PARALLEL:  $(LINT_PREREQ)

$(LINT_PREREQ):
	@cd $(@:%.lint=%); pwd; $(MAKE) $(TARGET)

#
# Nothing to do with any other prerequisites
#
%.prereq:

#
# Platform inter-dependencies
#


#
# The genunix requires priv_const.c file to be generated first.
#
genunix: $(PRIVS_C)

#
# Rules
#

$(IMPLEMENTATIONS):	FRC
	@cd $@; pwd; THISIMPL=$@ $(MAKE) $(NO_STATE) $(TARGET)

$(CLOSED_IMPLEMENTATIONS):	FRC
	cd $(CLOSED)/uts/sun4u/$@; pwd; \
	    THISIMPL=$@ $(MAKE) $(NO_STATE) $(TARGET); \

$(XMODS):	FRC
	@if [ -f $@/Makefile  ]; then \
		cd $@; pwd; $(MAKE) $(NO_STATE) $(TARGET); \
	else \
		true; \
	fi

$(CLOSED_KMODS):	FRC
	cd $(CLOSED)/uts/zSeries/$@; pwd; $(MAKE) $(NO_STATE) $(TARGET)

$(CLOSED_XMODS):	FRC
	@if [ -f $(CLOSED)/uts/zSeries/$@/Makefile  ]; then \
		cd $(CLOSED)/uts/zSeries/$@; pwd; \
		    $(MAKE) $(NO_STATE) $(TARGET); \
	else \
		true; \
	fi

install_h check: install_platforms $(IMPLEMENTATIONS) \
	$(CLOSED_IMPLEMENTATIONS) FRC
	@cd sys; pwd; $(MAKE) $(TARGET)
	@cd vm; pwd; $(MAKE) $(TARGET)

#
# For some s390x OEMs that deliver their own platform kernel modules but
# will not deliver anything in usr/platform, they should add their platforms 
# here so a symlink will be created to the standard usr/platform/zSeries.
# 
OEM_LINKS	= 

OEM_USR_PLAT_LINKS	= $(OEM_LINKS:%=$(USR_PLAT_DIR)/%)

$(OEM_USR_PLAT_LINKS):
	$(INS.slink1)

#
# Definitions for the /platform directory aliases.
# Currently none for zSeries.
#
PLAT_LINKS	=

#
# Make the /platform directories.  This is hardwired here because
# the first stage of the project (KBI) only implements the userland
# changes, but the only reasonable place to record the aliases is
# here in kernel land.
#
install_platforms:	$(ROOT_PSM_DIR) $(USR_PSM_DIR) \
			$(ROOT_PLAT_LINKS) $(USR_PLAT_LINKS) \
			$(OEM_USR_PLAT_LINKS)

#
# Work-around to disable acpica global crosscheck lint warnings
#
LGREP.zSeries =	grep -v 'zSeries/io/acpica'

#
#	Full kernel lint target.
#
LINT_TARGET	= globallint

# workaround for multiply defined errors
globallint := LINTFLAGS += -erroff=E_NAME_MULTIPLY_DEF2

globallint:
	@-$(ECHO) "\nFULL KERNEL: global crosschecks:"
	@-$(LINT) $(LINTFLAGS) $(LINT_LIBS) 2>&1 | $(LGREP.zSeries) | $(LGREP.2)

lint:	lintlib .WAIT modlintlib .WAIT $(INTEL_LINTS) $(LINT_DEPS)

$(INTEL_LINTS):	FRC
	@cd $(UTSBASE)/s390x/$@; pwd; $(MAKE) modlintlib

FRC:

include ../Makefile.targ

#
# Cross-reference customization: build a cross-reference over all of the
# zSeries-related directories.
#
SHARED_XRDIRS	= ../zSeries ../s390x ../common
XRDIRS		= $(SHARED_XRDIRS)
$(CLOSED_BUILD)XRDIRS	= $(SHARED_XRDIRS:../%=../% ../../../closed/uts/%)
XRPRUNE	= sun4u sun4 i86pc

cscope.out tags: FRC
	$(XREF) -x $@
