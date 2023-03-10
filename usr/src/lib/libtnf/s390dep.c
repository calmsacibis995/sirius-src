/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
/*                                                                  */
/* Copyright 2008 Sine Nomine Associates.                           */
/* All rights reserved.                                             */
/* Use is subject to license terms.                                 */
 */
/*
 *	Copyright (c) 1994, by Sun Microsytems, Inc.
 */

#pragma	ident	"@(#)s390dep.c	1.4	05/06/08 SMI"

#include "libtnf.h"

tnf_uint32_t
_tnf_swap32(tnf_uint32_t x)
{
	return (((x<<24) | (((x>>8)<<24)>>8) | (((x<<8)>>24)<<8) | (x>>24)));
}

tnf_uint16_t
_tnf_swap16(tnf_uint16_t x)
{
	return (((x<<8) | (x>>8)));
}
