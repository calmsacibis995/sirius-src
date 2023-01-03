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
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# Definitions common to command source.
#
# include global definitions; SRC should be defined in the shell.
# SRC is needed until RFE 1026993 is implemented.

include $(SRC)/Makefile.master

LN=		ln
CP=		cp
SH=		sh
ECHO=		echo
MKDIR=		mkdir
TOUCH=		touch

FILEMODE=	0555
LIBFILEMODE=	0444
XPG4=		$(XPG4PROG:%=%.xpg4)
XPG6=		$(XPG6PROG:%=%.xpg6)

KRB5DIR=	$(ROOT)/usr
KRB5BIN=	$(KRB5DIR)/bin
KRB5SBIN=	$(KRB5DIR)/sbin
KRB5LIB=	$(KRB5DIR)/lib/krb5
KRB5RUNPATH=	/usr/lib/krb5
GSSRUNPATH=	/usr/lib/gss


ROOTBIN=		$(ROOT)/usr/bin
ROOTLIB=		$(ROOT)/usr/lib
ROOTLIBSVCBIN=		$(ROOT)/lib/svc/bin
ROOTLIBSVCMETHOD=	$(ROOT)/lib/svc/method
ROOTLIBZONES=		$(ROOT)/lib/zones

ROOTSHLIB=	$(ROOT)/usr/share/lib
ROOTSHLIBCCS=	$(ROOTSHLIB)/ccs
ROOTSBIN=	$(ROOT)/sbin
ROOTUSRSBIN=	$(ROOT)/usr/sbin
ROOTETC=	$(ROOT)/etc

ROOTETCSECURITY=	$(ROOTETC)/security
ROOTETCTSOL=	$(ROOTETCSECURITY)/tsol
ROOTETCSECLIB=	$(ROOTETCSECURITY)/lib
ROOTETCZONES=	$(ROOTETC)/zones

ROOTCCSBIN=	$(ROOT)/usr/ccs/bin
ROOTCCSBIN64=	$(ROOTCCSBIN)/$(MACH64)
ROOTCCSBINLINKDIR=	$(ROOT)/../../bin
ROOTCCSBINLINKDIR64=	$(ROOT)../../../bin/$(MACH)
ROOTUSRKVM=	$(ROOT)/usr/kvm
ROOTXPG4=	$(ROOT)/usr/xpg4
ROOTXPG4BIN=	$(ROOT)/usr/xpg4/bin
ROOTXPG4BIN32=	$(ROOTXPG4BIN)/$(MACH32)
ROOTXPG4BIN64=	$(ROOTXPG4BIN)/$(MACH64)
ROOTXPG6=	$(ROOT)/usr/xpg6
ROOTXPG6BIN=	$(ROOT)/usr/xpg6/bin
ROOTLOCALEDEF=	$(ROOT)/usr/lib/localedef
ROOTCHARMAP=	$(ROOTLOCALEDEF)/charmap
ROOTI18NEXT=	$(ROOTLOCALEDEF)/extensions
ROOTI18NEXT64=	$(ROOTLOCALEDEF)/extensions/$(MACH64)
ROOTBIN32=	$(ROOTBIN)/$(MACH32)
ROOTBIN64=	$(ROOTBIN)/$(MACH64)
ROOTCMDDIR64=	$(ROOTCMDDIR)/$(MACH64)
ROOTLIB64=	$(ROOTLIB)/$(MACH64)
ROOTUSRSBIN32=	$(ROOTUSRSBIN)/$(MACH32)
ROOTUSRSBIN64=	$(ROOTUSRSBIN)/$(MACH64)
ROOTMAN1=	$(ROOT)/usr/share/man/man1
ROOTMAN1M=	$(ROOT)/usr/share/man/man1m
ROOTMAN3=	$(ROOT)/usr/share/man/man3
ROOTVARSMB=	$(ROOT)/var/smb

#

# Like ROOTLIBDIR in $(SRC)/Makefile.lib, any lower-level Makefiles that
# put their binaries in a non-standard location should reset this and use
# $(ROOTCMD) in their `install' target. By default we set this to a bogus
# value so that it will not conflict with any of the other values already
# defined in this Makefile.
#
ROOTCMDDIR=	$(ROOT)/__nonexistent_directory__

ROOTSHAUDIO=	$(ROOT)/usr/share/audio
ROOTAUDIOSAMP=	$(ROOTSHAUDIO)/samples
ROOTAUDIOSAMPAU=$(ROOTAUDIOSAMP)/au

#
# Begin /usr/sfw freeware rules for freeware that has "External"
# taxonomy stability level.
#
ROOTSFW=	$(ROOT)/usr/sfw
ROOTSFWBIN=	$(ROOT)/usr/sfw/bin
ROOTSFWSHARE=	$(ROOT)/usr/sfw/share
ROOTSFWSHSRC=	$(ROOT)/usr/sfw/share/src
ROOTSFWINFO=	$(ROOT)/usr/sfw/share/info
ROOTSFWINCLUDE=	$(ROOT)/usr/sfw/include
ROOTSFWLIB=	$(ROOT)/usr/sfw/lib
ROOTLIBEXEC=	$(ROOT)/usr/sfw/libexec
ROOTSFWMAN=	$(ROOT)/usr/sfw/share/man
ROOTSFWMANN=	$(ROOT)/usr/sfw/share/mann
ROOTSFWMAN1=	$(ROOT)/usr/sfw/share/man/man1
ROOTSFWMAN1M=	$(ROOT)/usr/sfw/share/man/man1m
ROOTSFWMAN3=	$(ROOT)/usr/sfw/share/man/man3
ROOTSFWMAN5=	$(ROOT)/usr/sfw/share/man/man5
ROOTSFWMAN7=	$(ROOT)/usr/sfw/share/man/man7
#
# End /usr/sfw freeware rules
#

ISAEXEC=	$(ROOT)/usr/lib/isaexec
PLATEXEC=	$(ROOT)/usr/lib/platexec

s390_XLDLIBS=	-ldl -lgcc_s
XLDLIBS=	$($(MACH)_XLDLIBS)
LDLIBS =	$(LDLIBS.cmd:-L%=-L% -Wl,--rpath-link %) $(XLDLIBS)

sparc_MAPFILES=	$(MAPFILE.NES:%=-M%) $(MAPFILE.PGA:%=-M%) $(MAPFILE.NED:%=-M%)
i386_MAPFILES= 	$(MAPFILE.NES:%=-M%) $(MAPFILE.PGA:%=-M%) $(MAPFILE.NED:%=-M%)
s390_MAPFILES= 	

LDFLAGS.cmd = \
	$(BDIRECT) $(ENVLDFLAGS1) $(ENVLDFLAGS2) $(ENVLDFLAGS3) \
	$($(MACH)_MAPFILES)

LDFLAGS =	$(LDFLAGS.cmd)

LINTFLAGS=	-axsm
LINTFLAGS64=	-axsm -Xarch=$(MACH64:sparcv9=v9)
LINTOUT=	lint.out

KRB5PROG=	$(PROG:%=$(KRB5BIN)/%)
KRB5SBINPROG=	$(PROG:%=$(KRB5SBIN)/%)
KRB5LIBPROG=	$(PROG:%=$(KRB5LIB)/%)

ROOTPROG=	$(PROG:%=$(ROOTBIN)/%)
ROOTCMD=	$(PROG:%=$(ROOTCMDDIR)/%)
ROOTSHFILES=	$(SHFILES:%=$(ROOTBIN)/%)
ROOTLIBPROG=	$(PROG:%=$(ROOTLIB)/%)
ROOTLIBSHFILES= $(SHFILES:%=$(ROOTLIB)/%)
ROOTSHLIBPROG=	$(PROG:%=$(ROOTSHLIB)/%)
ROOTSBINPROG=	$(PROG:%=$(ROOTSBIN)/%)
ROOTUSRSBINPROG=$(PROG:%=$(ROOTUSRSBIN)/%)
ROOTUSRSBINSCRIPT=$(SCRIPT:%=$(ROOTUSRSBIN)/%)
ROOTETCPROG=	$(PROG:%=$(ROOTETC)/%)
ROOTCCSBINPROG=	$(PROG:%=$(ROOTCCSBIN)/%)
ROOTCCSBINPROG64=	  $(PROG:%=$(ROOTCCSBIN64)/%)
ROOTCCSBINLINK=$(PROG:%=  $(ROOTCCSBINLINKDIR)/%)
ROOTCCSBINLINK64=$(PROG:%=$(ROOTCCSBINLINKDIR64)/%)
ROOTUSRKVMPROG=	$(PROG:%=$(ROOTUSRKVM)/%)
ROOTXPG4PROG=	$(XPG4PROG:%=$(ROOTXPG4BIN)/%)
ROOTXPG4PROG32=	$(XPG4PROG:%=$(ROOTXPG4BIN32)/%)
ROOTXPG4PROG64=	$(XPG4PROG:%=$(ROOTXPG4BIN64)/%)
ROOTXPG6PROG=	$(XPG6PROG:%=$(ROOTXPG6BIN)/%)
ROOTLOCALEPROG=	$(PROG:%=$(ROOTLOCALEDEF)/%)
ROOTPROG64=	$(PROG:%=$(ROOTBIN64)/%)
ROOTPROG32=	$(PROG:%=$(ROOTBIN32)/%)
ROOTCMD64=	$(PROG:%=$(ROOTCMDDIR64)/%)
ROOTUSRSBINPROG32=	$(PROG:%=$(ROOTUSRSBIN32)/%)
ROOTUSRSBINPROG64=	$(PROG:%=$(ROOTUSRSBIN64)/%)
ROOTMAN1FILES=	$(MAN1FILES:%=$(ROOTMAN1)/%)
$(ROOTMAN1FILES) := FILEMODE= 444
$(ROOTMAN1FILES) := OWNER= root
$(ROOTMAN1FILES) := GROUP= bin
ROOTMAN1MFILES=	$(MAN1MFILES:%=$(ROOTMAN1M)/%)
$(ROOTMAN1MFILES) := FILEMODE= 444
$(ROOTMAN1MFILES) := OWNER= root
$(ROOTMAN1MFILES) := GROUP= bin
ROOTMAN3FILES=	$(MAN3FILES:%=$(ROOTMAN3)/%)
$(ROOTMAN3FILES) := FILEMODE= 444
$(ROOTMAN3FILES) := OWNER= root
$(ROOTMAN3FILES) := GROUP= bin

# Symlink rules for /usr/ccs/bin commands. Note, those commands under
# the rule of the linker area, are controlled by a different set of 
# rules defined in $(SRC)/cmd/sgs/Makefile.var.

INS.ccsbinlink= \
	$(RM) $(ROOTCCSBINPROG); \
	$(SYMLINK) ../../bin/$(PROG) $(ROOTCCSBINPROG)

INS.ccsbinlink64= \
	$(RM) $(ROOTCCSBINPROG64); \
	$(SYMLINK) ../../../bin/$(MACH64)/$(PROG) $(ROOTCCSBINPROG64)

ROOTETCDEFAULT=	$(ROOTETC)/default
ROOTETCDEFAULTFILES=	$(DEFAULTFILES:%.dfl=$(ROOTETCDEFAULT)/%)
$(ROOTETCDEFAULTFILES) :=	OWNER = root
$(ROOTETCDEFAULTFILES) :=	GROUP = sys
$(ROOTETCDEFAULTFILES) :=	FILEMODE = 0644

ROOTETCSECFILES=	$(ETCSECFILES:%=$(ROOTETCSECURITY)/%)
$(ROOTETCSECFILES) :=	FILEMODE = 0644

ROOTETCTSOLFILES=	$(ETCTSOLFILES:%=$(ROOTETCTSOL)/%)
$(ROOTETCTSOLFILES) :=	FILEMODE = 0644

ROOTETCSECLIBFILES=	$(ETCSECLIBFILES:%=$(ROOTETCSECLIB)/%)

ROOTETCZONESFILES=	$(ETCZONESFILES:%=$(ROOTETCZONES)/%)
$(ROOTETCZONESFILES) :=	FILEMODE = 0444

ROOTLIBZONESFILES=	$(LIBZONESFILES:%=$(ROOTLIBZONES)/%)
$(ROOTLIBZONESFILES) :=	FILEMODE = 0555


#
# Directories for smf(5) service manifests and profiles.
#
ROOTVARSVC=			$(ROOT)/var/svc

ROOTVARSVCMANIFEST=		$(ROOTVARSVC)/manifest
ROOTVARSVCPROFILE=		$(ROOTVARSVC)/profile

ROOTSVCMILESTONE=		$(ROOTVARSVCMANIFEST)/milestone
ROOTSVCDEVICE=			$(ROOTVARSVCMANIFEST)/device
ROOTSVCSYSTEM=			$(ROOTVARSVCMANIFEST)/system
ROOTSVCSYSTEMDEVICE=		$(ROOTSVCSYSTEM)/device
ROOTSVCSYSTEMFILESYSTEM=	$(ROOTSVCSYSTEM)/filesystem
ROOTSVCSYSTEMSECURITY=		$(ROOTSVCSYSTEM)/security
ROOTSVCNETWORK=			$(ROOTVARSVCMANIFEST)/network
ROOTSVCNETWORKDNS=		$(ROOTSVCNETWORK)/dns
ROOTSVCNETWORKLDAP=		$(ROOTSVCNETWORK)/ldap
ROOTSVCNETWORKNFS=		$(ROOTSVCNETWORK)/nfs
ROOTSVCNETWORKNIS=		$(ROOTSVCNETWORK)/nis
ROOTSVCNETWORKROUTING=		$(ROOTSVCNETWORK)/routing
ROOTSVCNETWORKRPC=		$(ROOTSVCNETWORK)/rpc
ROOTSVCNETWORKSMB=		$(ROOTSVCNETWORK)/smb
ROOTSVCNETWORKSECURITY=		$(ROOTSVCNETWORK)/security
ROOTSVCNETWORKSSL=		$(ROOTSVCNETWORK)/ssl
ROOTSVCNETWORKIPSEC=		$(ROOTSVCNETWORK)/ipsec
ROOTSVCNETWORKSHARES=		$(ROOTSVCNETWORK)/shares
ROOTSVCSMB=			$(ROOTSVCNETWORK)/smb
ROOTSVCPLATFORM=		$(ROOTVARSVCMANIFEST)/platform
ROOTSVCPLATFORMSUN4U=		$(ROOTSVCPLATFORM)/sun4u
ROOTSVCPLATFORMSUN4V=		$(ROOTSVCPLATFORM)/sun4v
ROOTSVCAPPLICATION=		$(ROOTVARSVCMANIFEST)/application
ROOTSVCAPPLICATIONMANAGEMENT=	$(ROOTSVCAPPLICATION)/management
ROOTSVCAPPLICATIONSECURITY=	$(ROOTSVCAPPLICATION)/security
ROOTSVCAPPLICATIONPRINT=	$(ROOTSVCAPPLICATION)/print

#
# Commands Makefiles delivering a manifest are expected to define MANIFEST.
#
# Like ROOTCMDDIR, any lower-level Makefiles that put their manifests in a
# subdirectory of the manifest directories listed above should reset
# ROOTMANIFESTDIR and use it in their `install' target. By default we set this
# to a bogus value so that it will not conflict with any of the other values
# already  defined in this Makefile.
#
# The manifest validation of the $SRC/cmd check target is also derived from a
# valid MANIFEST setting.
#
ROOTMANIFESTDIR=	$(ROOTVARSVCMANIFEST)/__nonexistent_directory__
ROOTMANIFEST=		$(MANIFEST:%=$(ROOTMANIFESTDIR)/%)
CHKMANIFEST=		$(MANIFEST:%.xml=%.xmlchk)

#
# For installing "starter scripts" of services
#

ROOTSVCMETHOD=		$(SVCMETHOD:%=$(ROOTLIBSVCMETHOD)/%)

ROOTSVCBINDIR=		$(ROOTLIBSVCBIN)/__nonexistent_directory__
ROOTSVCBIN= 		$(SVCBIN:%=$(ROOTSVCBINDIR)/%)

#

# For programs that are installed in the root filesystem,
# build $(ROOTFS_PROG) rather than $(PROG)
$(ROOTFS_PROG) := LDFLAGS += -Wl,-I/lib/ld.so.1

$(KRB5BIN)/%: %
	$(INS.file)

$(KRB5SBIN)/%: %
	$(INS.file)

$(KRB5LIB)/%: %
	$(INS.file)

$(ROOTBIN)/%: %
	$(INS.file)

$(ROOTLIB)/%: %
	$(INS.file)

$(ROOTBIN64)/%: %
	$(INS.file)

$(ROOTLIB64)/%: %
	$(INS.file)

$(ROOTBIN32)/%: %
	$(INS.file)

$(ROOTSHLIB)/%: %
	$(INS.file)

$(ROOTSBIN)/%: %
	$(INS.file)

$(ROOTUSRSBIN)/%: %
	$(INS.file)

$(ROOTUSRSBIN32)/%: %
	$(INS.file)

$(ROOTUSRSBIN64)/%: %
	$(INS.file)

$(ROOTETC)/%: %
	$(INS.file)

$(ROOTETCDEFAULT)/%:	%.dfl
	$(INS.rename)

$(ROOTETCTSOL)/%: %
	$(INS.file)

$(ROOTETCSECLIB)/%: %
	$(INS.file)

$(ROOTETCZONES)/%: %
	$(INS.file)

$(ROOTLIBZONES)/%: %
	$(INS.file)

$(ROOTUSRKVM)/%: %
	$(INS.file)

$(ROOTXPG4BIN)/%: %.xpg4
	$(INS.rename)

$(ROOTXPG4BIN32)/%: %.xpg4
	$(INS.rename)

$(ROOTXPG4BIN64)/%: %.xpg4
	$(INS.rename)

$(ROOTXPG6BIN)/%: %.xpg6
	$(INS.rename)

$(ROOTLOCALEDEF)/%: %
	$(INS.file)

$(ROOTCHARMAP)/%: %
	$(INS.file)

$(ROOTI18NEXT)/%: %
	$(INS.file)

$(ROOTI18NEXT64)/%: %
	$(INS.file)

$(ROOTLIBSVCMETHOD)/%: %
	$(INS.file)

$(ROOTLIBSVCBIN)/%: %
	$(INS.file)

$(ROOTSVCMILESTONE)/%: %
	$(INS.file)

$(ROOTSVCDEVICE)/%: %
	$(INS.file)

$(ROOTSVCSYSTEM)/%: %
	$(INS.file)

$(ROOTSVCSYSTEMDEVICE)/%: %
	$(INS.file)

$(ROOTSVCSYSTEMFILESYSTEM)/%: %
	$(INS.file)

$(ROOTSVCSYSTEMSECURITY)/%: %
	$(INS.file)

$(ROOTSVCNETWORK)/%: %
	$(INS.file)

$(ROOTSVCNETWORKLDAP)/%: %
	$(INS.file)

$(ROOTSVCNETWORKNFS)/%: %
	$(INS.file)

$(ROOTSVCNETWORKNIS)/%: %
	$(INS.file)

$(ROOTSVCNETWORKRPC)/%: %
	$(INS.file)

$(ROOTSVCNETWORKSECURITY)/%: %
	$(INS.file)

$(ROOTSVCNETWORKSSL)/%: %
	$(INS.file)

$(ROOTSVCNETWORKIPSEC)/%: %
	$(INS.file)

$(ROOTSVCNETWORKSHARES)/%: %
	$(INS.file)

$(ROOTSVCNETWORKSMB)/%: %
	$(INS.file)

$(ROOTSVCAPPLICATION)/%: %
	$(INS.file)

$(ROOTSVCAPPLICATIONMANAGEMENT)/%: %
	$(INS.file)

$(ROOTSVCAPPLICATIONSECURITY)/%: %
	$(INS.file)

$(ROOTSVCAPPLICATIONPRINT)/%: %
	$(INS.file)

$(ROOTSVCPLATFORM)/%: %
	$(INS.file)

$(ROOTSVCPLATFORMSUN4U)/%: %
	$(INS.file)

$(ROOTSVCPLATFORMSUN4V)/%: %
	$(INS.file)

# Install rule for gprof, yacc, and lex dependency files
$(ROOTSHLIBCCS)/%: ../common/%
	$(INS.file)

$(ROOTCCSBINLINKDIR)/%: %
	$(INS.ccsbinlink)

$(ROOTCCSBINLINKDIR64)/%: %
	$(INS.ccsbinlink64)

$(ROOTMAN1)/%: %.sunman
	$(INS.rename)

$(ROOTMAN1M)/%: %.sunman
	$(INS.rename)

$(ROOTMAN3)/%: %.sunman
	$(INS.rename)

$(ROOTVARSMB)/%: %
	$(INS.file)

# build rule for statically linked programs with single source file.
%.static: %.c
	$(LINK.c) -o $@ $< $(LDLIBS)
	$(POST_PROCESS)

%.xpg4: %.c
	$(LINK.c) -o $@ $< $(LDLIBS)
	$(POST_PROCESS)

%.xpg6: %.c
	$(LINK.c) -o $@ $< $(LDLIBS)
	$(POST_PROCESS)

# Define the majority text domain in this directory.
TEXT_DOMAIN= SUNW_OST_OSCMD	

CLOBBERFILES += $(XPG4) $(XPG6) $(DCFILE)

# This flag is being added only for SCO (x86) compatibility
i386_SPFLAG=    -D_iBCS2
sparc_SPFLAG=
s390x_SPFLAG=

iBCS2FLAG = $($(MACH)_SPFLAG)

# This flag is for programs which should not build a 32-bit binary
sparc_64ONLY= $(POUND_SIGN)
s390x_64ONLY= $(POUND_SIGN)
64ONLY=	 $($(MACH)_64ONLY)
