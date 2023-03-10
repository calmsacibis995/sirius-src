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

#
# Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# ident	"%Z%%M%	%I%	%E% SMI"
#

#
# cmd/lp/Makefile.lp
# Common makefile definitions (should be) used by all lp makefiles
#

include		$(SRC)/cmd/Makefile.cmd

LPROOT=		$(SRC)/cmd/lp
ROOTVAR=	$(ROOT)/var
ROOTVARSP=	$(ROOT)/var/spool

ROOTETCLP=	$(ROOTETC)/lp
ROOTLIBLP=	$(ROOTLIB)/lp
ROOTBINLP=	$(ROOTBIN)/lp
ROOTLIBLPPOST =	$(ROOTLIBLP)/postscript
ROOTLIBLPLOCL =	$(ROOTLIBLP)/local

ROOTUSRUCB=	$(ROOT)/usr/ucb

#
# Typical owner and group for LP things. These can be overridden
# in the individual makefiles.
#
OWNER	=	root
GROUP	=	lp
SUPER	=	root

#
# $(EMODES): Modes for executables
# $(SMODES): Modes for setuid executables
# $(DMODES): Modes for directories
#
EMODES	=	0555
SMODES	=	04555
DMODES	=	0775


INC	=	$(ROOT)/usr/include
INCSYS  =       $(INC)/sys

LPINC	=	$(SRC)/cmd/lp/include
LPLIB	=	$(SRC)/cmd/lp/lib

LIBACC	=	$(LPLIB)/access/liblpacc.a
LIBCLS	=	$(LPLIB)/class/liblpcls.a
LIBFLT	=	$(LPLIB)/filters/liblpflt.a
LIBFRM	=	$(LPLIB)/forms/liblpfrm.a
LIBLP	=	$(LPLIB)/lp/liblp.a
LIBMSG	=	$(LPLIB)/msgs/liblpmsg.a
LIBOAM	=	$(LPLIB)/oam/liblpoam.a
LIBPRT	=	$(LPLIB)/printers/liblpprt.a
LIBREQ	=	$(LPLIB)/requests/liblpreq.a
LIBSEC	=	$(LPLIB)/secure/liblpsec.a
LIBUSR	=	$(LPLIB)/users/liblpusr.a

LINTACC	=	$(LPLIB)/access/llib-llpacc.ln
LINTCLS	=	$(LPLIB)/class/llib-llpcls.ln
LINTFLT	=	$(LPLIB)/filters/llib-llpflt.ln
LINTFRM	=	$(LPLIB)/forms/llib-llpfrm.ln
LINTLP	=	$(LPLIB)/lp/llib-llp.ln
LINTMSG	=	$(LPLIB)/msgs/llib-llpmsg.ln
LINTOAM	=	$(LPLIB)/oam/llib-llpoam.ln
LINTPRT	=	$(LPLIB)/printers/llib-llpprt.ln
LINTREQ	=	$(LPLIB)/requests/llib-llpreq.ln
LINTSEC	=	$(LPLIB)/secure/llib-llpsec.ln
LINTUSR	=	$(LPLIB)/users/llib-llpusr.ln

sparc_CFLAGS += -_gcc=-fwritable-strings -_gcc=-Wno-sequence-points
i386_CFLAGS += 	-_gcc=-fwritable-strings -_gcc=-Wno-sequence-points
s390_CFLAGS +=  
CFLAGS =	$($(MACH)_CFLAGS)
sparc_CFLAGS64 += 	-_gcc=-fwritable-strings -_gcc=-Wno-sequence-points
i386_CFLAGS64 += 	-_gcc=-fwritable-strings -_gcc=-Wno-sequence-points
s390_CFLAGS64 += 
CFLAGS64 =	$($(MACH)_CFLAGS64)

all:=           TARGET= all
install:=       TARGET= install
clean:=         TARGET= clean
clobber:=       TARGET= clobber
lint:=          TARGET= lint
strip:=          TARGET= strip
catalog:=	TARGET= catalog
_msg:=		TARGET= catalog

ROOTLIBLPPROG=	$(PROG:%=$(ROOTLIBLP)/%)
ROOTBINLPPROG=	$(PROG:%=$(ROOTBINLP)/%)
ROOTETCLPPROG=	$(PROG:%=$(ROOTETCLP)/%)
ROOTUSRUCBPROG=	$(PROG:%=$(ROOTUSRUCB)/%)
ROOTLIBLPPOSTPROG=	$(PROG:%=$(ROOTLIBLPPOST)/%)
ROOTLIBLPLOCLPROG=	$(PROG:%=$(ROOTLIBLPLOCL)/%)

$(ROOTLIBLP)/%	\
$(ROOTBINLP)/%	\
$(ROOTETCLP)/%	\
$(ROOTUSRUCB)/%	\
$(ROOTLIBLPPOST)/% $(ROOTLIBLPLOCL)/%:	%
		$(INS.file)
