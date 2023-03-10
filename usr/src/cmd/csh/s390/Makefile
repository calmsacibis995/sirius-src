# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.

#	Copyright (c) 1983, 1984, 1985, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley Software License Agreement
# specifies the terms and conditions for redistribution.

# ident	"@(#)Makefile	1.21	05/08/04 SMI"

#
# C Shell with process control; VM/UNIX VAX Makefile
# Bill Joy UC Berkeley; Jim Kulp IIASA, Austria
#

CSH_PROG =	csh
PROG =		$(CSH_PROG)
PFCSH_PROG=	pfcsh
ROOTPFCSH=	$(ROOTBIN)/$(PFCSH_PROG)

include ../../Makefile.cmd

MBCHAR	= -DMBCHAR	# Define this line to include multibyte input support
DEFS	=  -DVFORK -DFILEC -DBSD_COMP -DFIVE	# No TELL when MBCHAR
CPPFLAGS= -I. $(DEFS) $(MBCHAR) $(CPPFLAGS.master)
CPPFLAGS +=	-I../../sh
CPPFLAGS += -D_FILE_OFFSET_BITS=64
LAZYLIBS = $(ZLAZYLOAD) -lsecdb $(ZNOLAZYLOAD)
lint := LAZYLIBS = -lsecdb
LDLIBS += -lcurses $(LAZYLIBS)

PFOBJS = sh_policy.o

HDDEP = sh.o sh.dir.o sh.dol.o sh.err.o sh.exec.o sh.exp.o sh.file.o \
	sh.func.o sh.glob.o sh.hist.o sh.init.o sh.lex.o sh.misc.o \
	sh.parse.o sh.proc.o sh.sem.o sh.set.o sh.time.o

COMMONOBJS=	printf.o sh.char.o sh.dir.o sh.dol.o sh.err.o \
	sh.exec.o sh.exp.o sh.file.o sh.func.o sh.glob.o sh.hist.o sh.init.o \
	sh.lex.o sh.misc.o sh.parse.o sh.print.o sh.proc.o sh.sem.o sh.set.o \
	sh.time.o sh.tchar.o sh.tconst.o sh.o \
        wait3.o 

LOCALOBJS= signal.o

COMMONSRCS=	$(COMMONOBJS:%.o=../%.c)
PFSRCS=		../../sh/sh_policy.c

.KEEP_STATE:

.PARALLEL: $(COMMONOBJS) $(LOCALOBJS)

all: $(PROG)

# build rule for common source above
%.o:	../%.c
	$(COMPILE.c) $<
	$(POST_PROCESS_O)

%.o:	../../sh/%.c
	$(COMPILE.c) $<
	$(POST_PROCESS_O)

$(CSH_PROG): sh.tconst.h .WAIT $(COMMONOBJS) $(PFOBJS) $(LOCALOBJS)
	$(LINK.c) $(COMMONOBJS) $(PFOBJS) $(LOCALOBJS) -o $@ $(LDLIBS)
	$(POST_PROCESS)

$(ROOTPFCSH): $(ROOTPROG)
	$(RM) $@
	$(LN) $(ROOTPROG) $@

sh.tconst.h: ../sh.tconst.c ../make.sh.tconst.h.ed 	
	$(RM) $@
	ed ../sh.tconst.c < ../make.sh.tconst.h.ed 

$(HDDEP): sh.tconst.h

install: all $(ROOTBINPROG)  $(ROOTPROG) $(ROOTPFCSH)

lint:	sh.tconst.h
	$(LINT.c) $(COMMONSRCS) $(PFSRCS) signal.c $(LDLIBS)

clean:
	$(RM) $(LOCALOBJS) $(COMMONOBJS) $(PFOBJS) sh.tconst.h

clobber:	clean
	$(RM)  $(PROG)
