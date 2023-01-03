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
# lib/pkcs11/Makefile.softtoken.s390x
#
# specific to s390x, common to pkcs11_softtoken and pkcs11_softtoken_extra

AES_PSR_OBJECTS =
ARCFOUR_PSR_OBJECTS =
DES_PSR_OBJECTS =
RSA_PSR_OBJECTS =
SHA1_PSR_OBJECTS =
BIGNUM_PSR_OBJECTS = bignumimpl.o mont_mulf.o
BIGNUM_CFG = 
BIGNUM_PSR_SRCS = \
	$(BIGNUMDIR)/bignumimpl.c \
	$(BIGNUMDIR)/mont_mulf.c

include ../Makefile.com
include ../../../Makefile.lib.64

LDLIBS		+=	-lm

install: all $(ROOTLIBS64) $(ROOTLINKS64)

pics/%.o:	$(BIGNUMDIR)/$(MACH)/%.c
	$(COMPILE.c) -o $@ $(BIGNUM_CFG) $<
	$(POST_PROCESS_O)

pics/%.o:	$(BIGNUMDIR)/$(MACH)/%.s
	$(COMPILE.s) -o $@ $(BIGNUM_CFG) $<
	$(POST_PROCESS_O)
