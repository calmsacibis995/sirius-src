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
# Copyright 2003 Sun Microsystems, Inc.   All rights reserved.
# Use is subject to license terms.
#
#pragma ident	"@(#)Makefile	1.2	05/06/08 SMI"

MODULE = md.so 
MDBTGT = kvm

MODSRCS = dumphotspare.c \
	dumpnamespace.c \
	findset.c \
	md.c  \
	metaset.c \
	metastat.c \
	set_io_cnt.c \
	snarf.c \
	simple_de_ic.c \
	walk_hsp.c \
	walk_sets.c \
	walk_units.c \
	walk_didnm.c \
	walk_nm.c \
	walk_directory_block.c \
	walk_directory_entry.c


include ../../../../Makefile.cmd
include ../../../../Makefile.cmd.64
include ../../Makefile.sparcv9
include ../../../Makefile.module
