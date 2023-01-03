#
# Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# ident	"@(#)Makefile	1.7	04/09/28 SMI"
#

MAPDIR= ../spec/s390x

include ../Makefile.com

# library dependency
DYNFLAGS += $(KRUNPATH) $(KMECHLIB)
DYNFLAGS += $(KRB5LIB)/libkdb.so $(KRB5LIB)/libdyn.so
LDLIBS += -L $(ROOTLIBDIR) -lgss -lnsl -lc -lsocket

install: all $(ROOTLIBS) $(ROOTLINKS)
