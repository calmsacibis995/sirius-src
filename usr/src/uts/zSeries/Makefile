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
# uts/zSeries/Makefile
#
#	This makefile drives the production of all implementation architecture
#	independent modules for the System z processor.

UTSBASE = ..

include Makefile.zSeries

LINT_LIBS	 = $(LINT_LIB) $(GEN_LINT_LIB) \
		   $(LINT_KMODS:%=$(LINT_LIB_DIR)/llib-l%.ln)

# EXPORT DELETE START
DRV_KMODS	+= dprov
#
# CRYPTO_EK_KMODS modules go in the encryption pack (SUNWcry*)
# They need to be listed separately since they duplicate global symbols
# causing the 2nd pass of lint on the kernel to complain. CRYPTO_EK_KMODS
# should not be listed in the lint target.
#
CRYPTO_EK_KMODS	+= aes256
CRYPTO_EK_KMODS	+= arcfour2048
CRYPTO_EK_KMODS	+= blowfish448
# EXPORT DELETE END

#
#
#
def		:=	TARGET= def
all		:=	TARGET= all
install		:=	TARGET= install
clean		:=	TARGET= clean
clobber		:=	TARGET= clobber
lint		:=	TARGET= lint
modlintlib	:=	TARGET= modlintlib
modlist		:=	TARGET= modlist
modlist		:=	NO_STATE= -K $$MODSTATE$$$$
clean.lint	:=	TARGET= clean.lint
check		:=	TARGET= check
install_h	:=	TARGET= install_h

.KEEP_STATE:

.PARALLEL:	$(KMODS) $(CLOSED_KMODS) $(SVVS) $(XMODS) $(CLOSED_XMODS) \
		config $(LINT_DEPS)

def all install clean clobber modlist: $(KMODS) $(CLOSED_KMODS) \
	$(SVVS) $(XMODS) $(CLOSED_XMODS) config

modlintlib clean.lint: $(LINT_KMODS) $(CLOSED_LINT_KMODS) $(SVVS) \
	$(XMODS) $(CLOSED_XMODS)

$(KMODS) config:	FRC
	@cd $@; pwd; $(MAKE) $(NO_STATE) $(TARGET)

$(CLOSED_KMODS):	FRC
	cd $(CLOSED)/uts/zSeries/$@; pwd; $(MAKE) $(NO_STATE) $(TARGET)

$(XMODS):	FRC
	@if [ -f $@/Makefile  ]; then \
		cd $@; pwd; $(MAKE) $(NO_STATE) $(TARGET); \
	else \
		true; \
	fi

$(SVVS) $(CLOSED_XMODS):	FRC
	@if [ -f $(CLOSED)/uts/zSeries/$@/Makefile  ]; then \
		cd $(CLOSED)/uts/zSeries/$@; pwd; \
		    $(MAKE) $(NO_STATE) $(TARGET); \
	else \
		true; \
	fi

install_h check:	FRC
	@cd sys; pwd; $(MAKE) $(TARGET)
	@cd asm; pwd; $(MAKE) $(TARGET)

#
#	Full kernel lint target.
#
LINT_TARGET	= globallint

globallint:
	@-$(ECHO) "\nFULL KERNEL: global crosschecks:"
	@-$(LINT) $(LINTFLAGS) $(LINT_LIBS) 2>&1 | $(LGREP.2)

lint:	modlintlib .WAIT $(LINT_DEPS)

# EXPORT DELETE START

EXPORT_SRC:
	$(RM) Makefile+
	sed -e "/^# EXPORT DELETE START/,/^# EXPORT DELETE END/d" \
	    < Makefile > Makefile+
	$(MV) Makefile+ Makefile
	$(CHMOD) 444 Makefile

# EXPORT DELETE END

include ../Makefile.targ
