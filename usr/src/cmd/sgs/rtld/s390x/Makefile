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
# Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

# Object lists are organized into primary (most frequently used code) and
# secondary lists (less frequently used code, ie. a.out support).

P_COMOBJS=	debugdata.o \
		analyze.o	elf.o		external.o	globals.o \
		malloc.o	paths.o		setup.o		util.o \
		dlfcns.o	config_elf.o	locale.o	tsort.o \
		getcwd.o	remove.o	move.o		tls.o \
		cap.o

S_COMOBJS=	debug.o		audit.o		object.o

G_MACHOBJS=	doreloc.o

P_MACHOBJS=	s390x_elf.o	_setup.o

CP_MACHOBJS=	dtrace_data.o	\
		liblddbg.a

S_MACHOBJS=	

P_ASOBJS=	boot.o		boot_elf.o	caller.o	zero.o

S_ASOBJS=	

CRTSRCS=	../../../../lib/common/s390x
CRTI=		pics/crti.o
CRTN=		pics/crtn.o
CRTS=		$(CRTI)		$(CRTN)

CHKSRCS=	$(SRCBASE)/uts/common/krtld/reloc.h

BASEPLAT=	s390

# __GNUC is overridden in the environment when building with gcc
__GNUC:sh=		echo \\043

# We build s390x objects with cc by default
MAPFILE-ORDER =		../common/mapfile-order-devpro
$(__GNUC)MAPFILE-ORDER =	../common/mapfile-order-gcc

include		$(SRC)/cmd/sgs/rtld/Makefile.com
include		$(SRC)/lib/Makefile.lib.64

# Add any machine specific flags.

CPPFLAGS +=	-I../../../../uts/zSeries/krtld 
CFLAGS +=
ASFLAGS +=	-s -fPIC -D__s390x -D_ELF64
ADBGENFLAGS +=	-mlp64
ADBGENCFLAGS +=	-erroff=%all
ADBSUB=		$(ADBSUB64)
CPPFLAGS +=	-D_ELF64
LINTFLAGS64 +=	$(VAR_LINTFLAGS64)
SONAME=		$(VAR_RTLD_S390X_SONAME)
DYNFLAGS += 	-melf64_s390 -zlazyload -Bsymbolic 

SGSMSGTARG +=	$(SGSMSGZSERIES) $(SGSMSGZSERIES64) $(SGSMSG64)

LDLIB =		-L ../../libld/$(MACH64)
RTLDLIB =	-L ../../librtld/$(MACH64)

CPICLIB =	$(CPICLIB64)
LDDBGLIBDIR =	$(LDDBGLIBDIR64)
CONVLIBDIR =	$(CONVLIBDIR64)


.KEEP_STATE:

all:		pics/liblddbg.a $(RTLD)

pics/liblddbg.a:	
		@rm -f $@
		@ar -rc $@ ../../liblddbg/s390x/pics/*.o

install:	all $(ROOTDYNLIB64)

lint:		$(LINTOUT64)

adbmacros:	adb .WAIT $(ADBSCRIPTS)

adbinstall:	adbmacros .WAIT $(ROOTADB64)

pics/%.o:	../s390/%.c
		$(COMPILE.c) -o $@ $< $(WARNFLAGS)
		$(POST_PROCESS_O)

pics/%.o:	$(SRCBASE)/uts/zSeries/krtld/%.c
		$(COMPILE.c) -o $@ $<  $(WARNFLAGS)
		$(POST_PROCESS_O)


include		$(SRC)/cmd/sgs/rtld/Makefile.targ
include		../../Makefile.sub.64
