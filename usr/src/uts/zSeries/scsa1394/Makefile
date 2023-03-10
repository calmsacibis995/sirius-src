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

#
#	Path to the base of the uts directory tree.
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		= scsa1394
OBJECTS		= $(SCSA1394_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(SCSA1394_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)
WARLOCK_OUT     = $(SCSA1394_OBJS:%.o=%.ll)
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
#       Overrides
#
ALL_BUILDS	= $(ALL_BUILDSONLY64)
DEF_BUILDS	= $(DEF_BUILDSONLY64)
CLEANLINTFILES	+= $(LINT32_FILES)

#
# lint pass one enforcement
#
CFLAGS += -v

#
# depends on
#
MDFLAGS		+= -Nmisc/scsi -Nmisc/s1394 -Nmisc/sbp2
LDFLAGS		+= -dy

#	Default build targets.
#
.KEEP_STATE:

def:		$(DEF_DEPS)

all:		$(ALL_DEPS)

clean:		$(CLEAN_DEPS); \
		$(RM) $(WARLOCK_OUT) $(WARLOCK_OK)

clobber:	$(CLOBBER_DEPS); \
		$(RM) $(WARLOCK_OUT) $(WARLOCK_OK)

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
WARLOCK		= warlock
WLCC		= wlcc
TOUCH		= touch
SCCS		= sccs
TEST		= test

#
#	Warlock targets
#
WARLOCK_OBJECTS	= $(SCSA1394_OBJS:%.o=%.ll)
SBP2_FILES = $(SBP2_OBJS:%.o= -l ../sbp2/%.ll)
SD_FILES   = $(SD_OBJS:%.o=../sd/%.ll)
SCSI_FILES = $(SCSI_OBJS:%.o= -l ../scsi/%.ll)

warlock: $(MODULE).ok

%.wlcmd:
	$(TEST) -f $@ || $(SCCS) get $@

$(MODULE).ok: $(WARLOCK_OBJECTS) sbp2.files warlock_ddi.files scsa1394.wlcmd \
	sd.files scsi.files
	$(WARLOCK) -c scsa1394.wlcmd $(WARLOCK_OBJECTS) $(SD_FILES) \
		$(SCSI_FILES) \
		$(SBP2_FILES) \
		-l ../warlock/ddi_dki_impl.ll
	$(TOUCH) $@

%.ll: $(UTSBASE)/common/io/1394/targets/scsa1394/%.c
	$(WLCC) $(CPPFLAGS) -DNPROBE -DDEBUG -o $@ $<

sbp2.files:
	@cd ../sbp2; pwd; $(MAKE) warlock

warlock_ddi.files:
	@cd ../warlock; pwd; $(MAKE) warlock

sd.files:
	@cd ../sd; pwd; $(MAKE) warlock_alone

scsi.files:
	@cd ../scsi; pwd; $(MAKE) warlock


