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
# Copyright 2006 Cyril Plisko.  All rights reserved.
# Use is subject to license terms.
#
#	This makefile drives the production of the generic
#	unix kernel module.
#
#	s390x implementation architecture dependent
#
#	Based on i86pc counterpart

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		= genunix
GENUNIX		= $(OBJS_DIR)/$(MODULE)

OBJECTS		= $(GENUNIX_OBJS:%=$(OBJS_DIR)/%) \
		  $(NOT_YET_KMODS:%=$(OBJS_DIR)/%)

LINTS		= $(GENUNIX_OBJS:%.o=$(LINTS_DIR)/%.ln) \
		  $(NOT_YET_KMODS:%.o=$(LINTS_DIR)/%.ln)

ROOTMODULE	= $(ROOT_KERN_DIR)/$(MODULE)

PLATFORM	= chrp
LIBGEN		= $(OBJS_DIR)/libgenunix.so
LIBSTUBS	= $(GENSTUBS_OBJS:%=$(OBJS_DIR)/%)

#
#	Include common rules.
#
include $(UTSBASE)/zSeries/Makefile.zSeries

#
#	Define targets
#
ALL_TARGET	= $(LIBGEN)
LINT_TARGET	= $(MODULE).lint
INSTALL_TARGET	= $(GENUNIX) $(ROOTMODULE)

#
#	Overrides
#
CLEANFILES	+= $(LIBSTUBS) $(LIBGEN)
BINARY		=

#
# Non-patch genunix builds merge a version of the ip module called ipctf.  This
# is to ensure that the common network-related types are included in genunix and
# can thus be uniquified out of other modules.  We don't want to do this for
# patch builds, since we can't guarantee that ip and genunix will be in the same
# patch.
#
IPCTF_TARGET	= $(IPCTF)
$(PATCH_BUILD)IPCTF_TARGET =

CPPFLAGS	+= -I$(SRC)/common

#
#	Default build targets.
#
.KEEP_STATE:

.PARALLEL:	$(LIBSTUBS)

def:		$(DEF_DEPS)

all:		$(ALL_DEPS)

clean:		$(CLEAN_DEPS)

clobber:	$(CLOBBER_DEPS)

lint:		$(LINT_DEPS)

modlintlib:	$(MODLINTLIB_DEPS)

clean.lint:	$(CLEAN_LINT_DEPS)

install:	$(INSTALL_DEPS)

$(LIBGEN):	$(LIBSTUBS) $(GENUNIX)
#	$(BUILD.SO) $(GENUNIX) 
	$(BUILD.SO) $(GENUNIX) $(LIBSTUBS)

$(IPCTF_TARGET) ipctf_target: FRC
	@cd $(IPDRV_DIR); pwd; $(MAKE) ipctf.$(OBJS_DIR)
	@pwd

$(GENUNIX): $(IPCTF_TARGET) $(OBJECTS)
	$(LD) -r $(LDFLAGS) -o $@ $(OBJECTS) 
	$(CTFMERGE_GENUNIX_MERGE)
	$(POST_PROCESS)

#
#	Include common targets.
#
include $(UTSBASE)/zSeries/Makefile.targ

#
# Override.
#
$(MODULE).lint := GEN_LINT_LIB =
