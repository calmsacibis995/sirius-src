#
# Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# ident	"%Z%%M%	%I%	%E% SMI"
#
# lib/libldap5/s390/Makefile
#

PR_MDUSOBJS = os_SunOS.o

include ../Makefile.com

LDLIBS +=	-L$(NSS_LDPATH) $(NSS_LIBS)
DYNFLAGS +=	-R$(NSS_LDPATH)


all: $(LIBS)

install: all $(ROOTLIBS) $(ROOTLINKS) $(ROOTLINT)
