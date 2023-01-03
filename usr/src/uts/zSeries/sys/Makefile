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

include ../../../Makefile.master

HDRS=  \
	archsystm.h	\
	asm_linkage.h	\
	blockio.h	\
	bootconf.h	\
	bootvfs.h	\
	ccw.h		\
	cmpregs.h	\
	cpu.h		\
	ddi_isa.h	\
	devinit.h	\
	diag250_ll.h	\
	exts390x.h 	\
	fasttrap_isa.h	\
	frame.h		\
	inline.h	\
	ios390x.h	\
	kdi_machimpl.h	\
	machelf.h	\
	machlock.h	\
	machs390x.h	\
	machsig.h	\
	machtypes.h	\
	memlist_plat.h	\
	memnode.h	\
	mutex_impl.h	\
	obpdefs.h	\
	old_procfs.h	\
	osa.h		\
	pcb.h		\
	polled_io.h	\
	privregs.h	\
	procfs_isa.h	\
	prom_debug.h	\
	prom_emul.h	\
	prom_plat.h	\
	promif.h	\
	promimpl.h	\
	psw.h		\
	reg.h		\
	regset.h	\
	sclp.h		\
	smp.h		\
	spl.h		\
	stack.h		\
	stat_impl.h	\
	synch32.h	\
	sysconfig_impl.h	\
	trap.h		\
	ucontext.h      \
	utrap.h		\
	vm_machparam.h	\
	vmparam.h

ROOTDIR=	$(ROOT)/usr/include/sys
ROOTDIRS=	$(ROOTDIR) 

ROOTHDRS= $(HDRS:%=$(ROOTDIR)/%)

CHECKHDRS= $(HDRS:%.h=%.check) 

# install rules
$(ROOTDIR)/%: %
	$(INS.file)

.KEEP_STATE:

.PARALLEL: $(CHECKHDRS) $(ROOTHDRS)

install_h: $(ROOTDIRS) .WAIT $(ROOTHDRS)

$(ROOTDIRS):
	$(INS.dir)

check:	$(CHECKHDRS)
