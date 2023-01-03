/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
 *                                                                
 * Copyright 2008 Sine Nomine Associates.                          
 * All rights reserved.                                          
 * Use is subject to license terms.                             
 */
/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma	ident	"@(#)ddi_asm.s	1.43	06/01/23 SMI"

#ifndef lint
#include "assym.h"
#endif

#include <sys/asm_linkage.h>
#include <sys/machthread.h>
#include <sys/privregs.h>
#include <sys/ontrap.h>
#include <sys/dditypes.h>

#if defined(lint)
#include <sys/isa_defs.h>
#include <sys/types.h>
#include <sys/sunddi.h>
#endif  /* lint */

/*
 * This file implements the following ddi common access 
 * functions:
 *
 *	ddi_get{8,16,32,64}
 *	ddi_put{8,16,32,64}
 *
 * and the underlying "trivial" implementations
 *
 *      i_ddi_{get,put}{8,16,32,64}
 *
 * which assume that there is no need to check the access handle -
 * byte swapping will be done by the mmu and the address is always
 * accessible via ld/st instructions.
 */

#if defined(lint)

/*ARGSUSED*/
uint8_t
ddi_get8(ddi_acc_handle_t handle, uint8_t *addr)
{
	return (0);
}

/*ARGSUSED*/
uint8_t
ddi_mem_get8(ddi_acc_handle_t handle, uint8_t *addr)
{
	return (0);
}

/*ARGSUSED*/
uint8_t
ddi_io_get8(ddi_acc_handle_t handle, uint8_t *dev_addr)
{
	return (0);
}

/*ARGSUSED*/
uint16_t
ddi_get16(ddi_acc_handle_t handle, uint16_t *addr)
{
	return (0);
}

/*ARGSUSED*/
uint16_t
ddi_mem_get16(ddi_acc_handle_t handle, uint16_t *addr)
{
	return (0);
}

/*ARGSUSED*/
uint16_t
ddi_io_get16(ddi_acc_handle_t handle, uint16_t *dev_addr)
{
	return (0);
}

/*ARGSUSED*/
uint32_t
ddi_get32(ddi_acc_handle_t handle, uint32_t *addr)
{
	return (0);
}

/*ARGSUSED*/
uint32_t
ddi_mem_get32(ddi_acc_handle_t handle, uint32_t *addr)
{
	return (0);
}

/*ARGSUSED*/
uint32_t
ddi_io_get32(ddi_acc_handle_t handle, uint32_t *dev_addr)
{
	return (0);
}

/*ARGSUSED*/
uint64_t
ddi_get64(ddi_acc_handle_t handle, uint64_t *addr)
{
	return (0);
}

/*ARGSUSED*/
uint64_t
ddi_mem_get64(ddi_acc_handle_t handle, uint64_t *addr)
{
	return (0);
}

/*ARGSUSED*/
void
ddi_put8(ddi_acc_handle_t handle, uint8_t *addr, uint8_t value) {}

/*ARGSUSED*/
void
ddi_mem_put8(ddi_acc_handle_t handle, uint8_t *dev_addr, uint8_t value) {}

/*ARGSUSED*/
void
ddi_io_put8(ddi_acc_handle_t handle, uint8_t *dev_addr, uint8_t value) {}

/*ARGSUSED*/
void
ddi_put16(ddi_acc_handle_t handle, uint16_t *addr, uint16_t value) {}

/*ARGSUSED*/
void
ddi_mem_put16(ddi_acc_handle_t handle, uint16_t *dev_addr, uint16_t value) {}

/*ARGSUSED*/
void
ddi_io_put16(ddi_acc_handle_t handle, uint16_t *dev_addr, uint16_t value) {}

/*ARGSUSED*/
void
ddi_put32(ddi_acc_handle_t handle, uint32_t *addr, uint32_t value) {}

/*ARGSUSED*/
void
ddi_mem_put32(ddi_acc_handle_t handle, uint32_t *dev_addr, uint32_t value) {}

/*ARGSUSED*/
void
ddi_io_put32(ddi_acc_handle_t handle, uint32_t *dev_addr, uint32_t value) {}

/*ARGSUSED*/
void
ddi_put64(ddi_acc_handle_t handle, uint64_t *addr, uint64_t value) {}

/*ARGSUSED*/
void
ddi_mem_put64(ddi_acc_handle_t handle, uint64_t *dev_addr, uint64_t value) {}

/*ARGSUSED*/
void
ddi_rep_get8(ddi_acc_handle_t handle, uint8_t *host_addr, uint8_t *dev_addr,
        size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_rep_get16(ddi_acc_handle_t handle, uint16_t *host_addr, uint16_t *dev_addr,
        size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_rep_get32(ddi_acc_handle_t handle, uint32_t *host_addr, uint32_t *dev_addr,
        size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_rep_get64(ddi_acc_handle_t handle, uint64_t *host_addr, uint64_t *dev_addr,
        size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_rep_put8(ddi_acc_handle_t handle, uint8_t *host_addr, uint8_t *dev_addr,
        size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_rep_put16(ddi_acc_handle_t handle, uint16_t *host_addr, uint16_t *dev_addr,
        size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_rep_put32(ddi_acc_handle_t handle, uint32_t *host_addr, uint32_t *dev_addr,
        size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_rep_put64(ddi_acc_handle_t handle, uint64_t *host_addr, uint64_t *dev_addr,
        size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_mem_rep_get8(ddi_acc_handle_t handle, uint8_t *host_addr,
        uint8_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_mem_rep_get16(ddi_acc_handle_t handle, uint16_t *host_addr,
        uint16_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_mem_rep_get32(ddi_acc_handle_t handle, uint32_t *host_addr,
        uint32_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_mem_rep_get64(ddi_acc_handle_t handle, uint64_t *host_addr,
        uint64_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_mem_rep_put8(ddi_acc_handle_t handle, uint8_t *host_addr,
        uint8_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_mem_rep_put16(ddi_acc_handle_t handle, uint16_t *host_addr,
        uint16_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_mem_rep_put32(ddi_acc_handle_t handle, uint32_t *host_addr,
        uint32_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_mem_rep_put64(ddi_acc_handle_t handle, uint64_t *host_addr,
        uint64_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
ddi_io_rep_get8(ddi_acc_handle_t handle,
	uint8_t *host_addr, uint8_t *dev_addr, size_t repcount) {}
 
/*ARGSUSED*/
void
ddi_io_rep_get16(ddi_acc_handle_t handle,
	uint16_t *host_addr, uint16_t *dev_addr, size_t repcount) {}
 
/*ARGSUSED*/
void
ddi_io_rep_get32(ddi_acc_handle_t handle,
	uint32_t *host_addr, uint32_t *dev_addr, size_t repcount) {}
 
/*ARGSUSED*/
void
ddi_io_rep_put8(ddi_acc_handle_t handle,
	uint8_t *host_addr, uint8_t *dev_addr, size_t repcount) {}
 
/*ARGSUSED*/
void
ddi_io_rep_put16(ddi_acc_handle_t handle,
	uint16_t *host_addr, uint16_t *dev_addr, size_t repcount) {}
 

/*ARGSUSED*/
void
ddi_io_rep_put32(ddi_acc_handle_t handle,
	uint32_t *host_addr, uint32_t *dev_addr, size_t repcount) {}

/*ARGSUSED*/
uint8_t
i_ddi_get8(ddi_acc_impl_t *hdlp, uint8_t *addr) 
{
	return (0);
}

/*ARGSUSED*/
uint16_t
i_ddi_get16(ddi_acc_impl_t *hdlp, uint16_t *addr)
{
	return (0);
}

/*ARGSUSED*/
uint32_t
i_ddi_get32(ddi_acc_impl_t *hdlp, uint32_t *addr)
{
	return (0);
}

/*ARGSUSED*/
uint64_t
i_ddi_get64(ddi_acc_impl_t *hdlp, uint64_t *addr)
{
	return (0);
}

/*ARGSUSED*/
void
i_ddi_put8(ddi_acc_impl_t *hdlp, uint8_t *addr, uint8_t value) {}

/*ARGSUSED*/
void
i_ddi_put16(ddi_acc_impl_t *hdlp, uint16_t *addr, uint16_t value) {}

/*ARGSUSED*/
void
i_ddi_put32(ddi_acc_impl_t *hdlp, uint32_t *addr, uint32_t value) {}

/*ARGSUSED*/
void
i_ddi_put64(ddi_acc_impl_t *hdlp, uint64_t *addr, uint64_t value) {}

/*ARGSUSED*/
void
i_ddi_rep_get8(ddi_acc_impl_t *hdlp, uint8_t *host_addr, uint8_t *dev_addr,
        size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
i_ddi_rep_get16(ddi_acc_impl_t *hdlp, uint16_t *host_addr, 
	uint16_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
i_ddi_rep_get32(ddi_acc_impl_t *hdlp, uint32_t *host_addr, 
	uint32_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
i_ddi_rep_get64(ddi_acc_impl_t *hdlp, uint64_t *host_addr, 
	uint64_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
i_ddi_rep_put8(ddi_acc_impl_t *hdlp, uint8_t *host_addr, uint8_t *dev_addr,
        size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
i_ddi_rep_put16(ddi_acc_impl_t *hdlp, uint16_t *host_addr, 
	uint16_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
i_ddi_rep_put32(ddi_acc_impl_t *hdlp, uint32_t *host_addr, 
	uint32_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
i_ddi_rep_put64(ddi_acc_impl_t *hdlp, uint64_t *host_addr, 
	uint64_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
uint8_t
i_ddi_prot_get8(ddi_acc_impl_t *hdlp, uint8_t *addr) 
{
	return (0);
}

/*ARGSUSED*/
uint16_t
i_ddi_prot_get16(ddi_acc_impl_t *hdlp, uint16_t *addr)
{
	return (0);
}

/*ARGSUSED*/
uint32_t
i_ddi_prot_get32(ddi_acc_impl_t *hdlp, uint32_t *addr)
{
	return (0);
}

/*ARGSUSED*/
uint64_t
i_ddi_prot_get64(ddi_acc_impl_t *hdlp, uint64_t *addr)
{
	return (0);
}

/*ARGSUSED*/
void
i_ddi_prot_put8(ddi_acc_impl_t *hdlp, uint8_t *addr, uint8_t value) {}

/*ARGSUSED*/
void
i_ddi_prot_put16(ddi_acc_impl_t *hdlp, uint16_t *addr, uint16_t value) {}

/*ARGSUSED*/
void
i_ddi_prot_put32(ddi_acc_impl_t *hdlp, uint32_t *addr, uint32_t value) {}

/*ARGSUSED*/
void
i_ddi_prot_put64(ddi_acc_impl_t *hdlp, uint64_t *addr, uint64_t value) {}

/*ARGSUSED*/
void
i_ddi_prot_rep_get8(ddi_acc_impl_t *hdlp, uint8_t *host_addr, uint8_t *dev_addr,
        size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
i_ddi_prot_rep_get16(ddi_acc_impl_t *hdlp, uint16_t *host_addr, 
	uint16_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
i_ddi_prot_rep_get32(ddi_acc_impl_t *hdlp, uint32_t *host_addr, 
	uint32_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
i_ddi_prot_rep_get64(ddi_acc_impl_t *hdlp, uint64_t *host_addr, 
	uint64_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
i_ddi_prot_rep_put8(ddi_acc_impl_t *hdlp, uint8_t *host_addr, uint8_t *dev_addr,
        size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
i_ddi_prot_rep_put16(ddi_acc_impl_t *hdlp, uint16_t *host_addr, 
	uint16_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
i_ddi_prot_rep_put32(ddi_acc_impl_t *hdlp, uint32_t *host_addr, 
	uint32_t *dev_addr, size_t repcount, uint_t flags)
{
}

/*ARGSUSED*/
void
i_ddi_prot_rep_put64(ddi_acc_impl_t *hdlp, uint64_t *host_addr, 
	uint64_t *dev_addr, size_t repcount, uint_t flags)
{
}

#else

/*
 * The functionality of each of the ddi_get/put routines is performed by
 * the respective indirect function defined in the access handle.  Use of
 * the access handle functions provides compatibility across platforms for
 * drivers.
 * 
 * By default, the indirect access handle functions are initialized to the
 * i_ddi_get/put routines to perform memory mapped IO.  If memory mapped IO
 * is not possible or desired, the access handle must be intialized to another
 * valid routine to perform the sepcified IO operation.
 *
 * The alignment and placement of the following functions have been optimized
 * such that the implementation specific versions, i_ddi*, fall within the 
 * same cache-line of the generic versions, ddi_*.  This insures that an
 * I-cache hit will occur thus minimizing the performance impact of using the
 * access handle.
 */

	.align 32
	ENTRY(ddi_get8)
	ALTENTRY(ddi_getb)
	ALTENTRY(ddi_io_get8)
	ALTENTRY(ddi_io_getb)
	ALTENTRY(ddi_mem_get8)
	ALTENTRY(ddi_mem_getb)
	lg	%r1,AHI_GET8(%r2)
	br	%r1
	SET_SIZE(ddi_get8)
	SET_SIZE(ddi_getb)
	SET_SIZE(ddi_io_get8)
	SET_SIZE(ddi_io_getb)
	SET_SIZE(ddi_mem_get8)
	SET_SIZE(ddi_mem_getb)

	.align 16
	ENTRY(i_ddi_get8)
	llgc	%r2,0(%r3)
	br	%r14
	SET_SIZE(i_ddi_get8)

	.align 32
	ENTRY(ddi_get16)
	ALTENTRY(ddi_getw)
	ALTENTRY(ddi_io_get16)
	ALTENTRY(ddi_io_getw)
	ALTENTRY(ddi_mem_get16)
	ALTENTRY(ddi_mem_getw)
	lg	%r1,AHI_GET16(%r2)
	br	%r1
	SET_SIZE(ddi_get16)
	SET_SIZE(ddi_getw)
	SET_SIZE(ddi_io_get16)
	SET_SIZE(ddi_io_getw)
	SET_SIZE(ddi_mem_get16)
	SET_SIZE(ddi_mem_getw)

	.align 16
	ENTRY(i_ddi_get16)
	ALTENTRY(i_ddi_swap_get16)
	llgh	%r2,0(%r3)
	br	%r14
	SET_SIZE(i_ddi_get16)
	SET_SIZE(i_ddi_swap_get16)

	.align 32
	ENTRY(ddi_get32)
	ALTENTRY(ddi_getl)
	ALTENTRY(ddi_io_get32)
	ALTENTRY(ddi_io_getl)
	ALTENTRY(ddi_mem_get32)
	ALTENTRY(ddi_mem_getl)
	lg	%r1,AHI_GET32(%r2)
	br	%r1
	SET_SIZE(ddi_get32)
	SET_SIZE(ddi_getl)
	SET_SIZE(ddi_io_get32)
	SET_SIZE(ddi_io_getl)
	SET_SIZE(ddi_mem_get32)
	SET_SIZE(ddi_mem_getl)

	.align 16
	ENTRY(i_ddi_get32)
	ALTENTRY(i_ddi_swap_get32)
	llgf 	%r2,0(%r3)
	br	%r14
	SET_SIZE(i_ddi_get32)
	SET_SIZE(i_ddi_swap_get32)

	.align 32
	ENTRY(ddi_get64)
	ALTENTRY(ddi_getll)
	ALTENTRY(ddi_io_get64)
	ALTENTRY(ddi_io_getll)
	ALTENTRY(ddi_mem_get64)
	ALTENTRY(ddi_mem_getll)
	lg	%r1,AHI_GET64(%r2)
	br	%r1
	SET_SIZE(ddi_get64)
	SET_SIZE(ddi_getll)
	SET_SIZE(ddi_io_get64)
	SET_SIZE(ddi_io_getll)
	SET_SIZE(ddi_mem_get64)
	SET_SIZE(ddi_mem_getll)

	.align 16
	ENTRY(i_ddi_get64)
	ALTENTRY(i_ddi_swap_get64)
	lg 	%r2,0(%r3)
	br	%r14
	SET_SIZE(i_ddi_get64)
	SET_SIZE(i_ddi_swap_get64)

	.align 32
	ENTRY(ddi_put8)
	ALTENTRY(ddi_putb)
	ALTENTRY(ddi_io_put8)
	ALTENTRY(ddi_io_putb)
	ALTENTRY(ddi_mem_put8)
	ALTENTRY(ddi_mem_putb)
	lg	%r1,AHI_PUT8(%r2)
	br	%r1
	SET_SIZE(ddi_put8)
	SET_SIZE(ddi_putb)
	SET_SIZE(ddi_io_put8)
	SET_SIZE(ddi_io_putb)
	SET_SIZE(ddi_mem_put8)
	SET_SIZE(ddi_mem_putb)

	.align 16
	ENTRY(i_ddi_put8)
	stc	%r4,0(%r3)
	br	%r14
	SET_SIZE(i_ddi_put8)

	.align 32
	ENTRY(ddi_put16)
	ALTENTRY(ddi_putw)
	ALTENTRY(ddi_io_put16)
	ALTENTRY(ddi_io_putw)
	ALTENTRY(ddi_mem_put16)
	ALTENTRY(ddi_mem_putw)
	lg	%r1,AHI_PUT16(%r2)
	br	%r1
	SET_SIZE(ddi_put16)
	SET_SIZE(ddi_putw)
	SET_SIZE(ddi_io_put16)
	SET_SIZE(ddi_io_putw)
	SET_SIZE(ddi_mem_put16)
	SET_SIZE(ddi_mem_putw)

	.align 16
	ENTRY(i_ddi_put16)
	ALTENTRY(i_ddi_swap_put16)
	sth	%r4,0(%r3)
	br	%r14
	SET_SIZE(i_ddi_put16)
	SET_SIZE(i_ddi_swap_put16)

	.align 32
	ENTRY(ddi_put32)
	ALTENTRY(ddi_putl)
	ALTENTRY(ddi_io_put32)
	ALTENTRY(ddi_io_putl)
	ALTENTRY(ddi_mem_put32)
	ALTENTRY(ddi_mem_putl)
	lg	%r1,AHI_PUT32(%r2)
	br	%r1
	SET_SIZE(ddi_put32)
	SET_SIZE(ddi_putl)
	SET_SIZE(ddi_io_put32)
	SET_SIZE(ddi_io_putl)
	SET_SIZE(ddi_mem_put32)
	SET_SIZE(ddi_mem_putl)

	.align 16
	ENTRY(i_ddi_put32)
	ALTENTRY(i_ddi_swap_put32)
	st	%r4,0(%r3)
	br	%r14
	SET_SIZE(i_ddi_put32)
	SET_SIZE(i_ddi_swap_put32)

	.align 32
	ENTRY(ddi_put64)
	ALTENTRY(ddi_putll)
	ALTENTRY(ddi_io_put64)
	ALTENTRY(ddi_io_putll)
	ALTENTRY(ddi_mem_put64)
	ALTENTRY(ddi_mem_putll)
	lg	%r1,AHI_PUT64(%r2)
	br	%r1
	SET_SIZE(ddi_put64)
	SET_SIZE(ddi_putll)
	SET_SIZE(ddi_io_put64)
	SET_SIZE(ddi_io_putll)
	SET_SIZE(ddi_mem_put64)
	SET_SIZE(ddi_mem_putll)

	.align 16
	ENTRY(i_ddi_put64)
	ALTENTRY(i_ddi_swap_put64)
	stg	%r4,0(%r3)
	br	%r14
	SET_SIZE(i_ddi_put64)
	SET_SIZE(i_ddi_swap_put64)

/*
 * The ddi_io_rep_get/put routines don't take a flag argument like the "plain"
 * and mem versions do.  This flag is used to determine whether or not the 
 * device address or port should be automatically incremented.  For IO space,
 * the device port is never incremented and as such, the flag is always set
 * to DDI_DEV_NO_AUTOINCR.
 *
 * This define processes the repetitive get functionality.  Automatic 
 * incrementing of the device address is determined by the flag field 
 * %r6. 
 */

#define DDI_REP_GET(d,l,s)						\
	cghi	%r6,DDI_DEV_NO_AUTOINCR;	// No auto required	\
	jne	1f;							\
	##l	%r0,0(%r3);			// Get data		\
	##s	%r0,0(%r4);			// Plug into dest	\
1:									\
	ltgr	%r5,%r5;			// Zero rep count? 	\
	jz 	3f;				// Yes.. exit		\
2:									\
	##l	%r0,0(%r3);			// Get data		\
	##s	%r0,0(%r4);			// Plug into dest	\
	aghi	%r3,##d;			// Bump src ptr		\
	aghi	%r4,##d;			// Bump dst ptr		\
	brct	%r5,2b;				// Do until done	\
3:	

	.align 32
	ENTRY(ddi_rep_get8)
	ALTENTRY(ddi_rep_getb)
	ALTENTRY(ddi_mem_rep_get8)
	ALTENTRY(ddi_mem_rep_getb)
	lg      %r1,AHI_REP_GET8(%r2)
	br	%r1
	SET_SIZE(ddi_rep_get8)
	SET_SIZE(ddi_rep_getb)
	SET_SIZE(ddi_mem_rep_get8)
	SET_SIZE(ddi_mem_rep_getb)

	.align 16
	ENTRY(i_ddi_rep_get8)
	DDI_REP_GET(1,ic,tc)
	br	%r14
	SET_SIZE(i_ddi_rep_get8)
	
	.align 32
	ENTRY(ddi_rep_get16)
	ALTENTRY(ddi_rep_getw)
	ALTENTRY(ddi_mem_rep_get16)
	ALTENTRY(ddi_mem_rep_getw)
	lg      %r1,AHI_REP_GET16(%r2)
	br	%r1
	SET_SIZE(ddi_rep_get16)
	SET_SIZE(ddi_rep_getw)
	SET_SIZE(ddi_mem_rep_get16)
	SET_SIZE(ddi_mem_rep_getw)

	.align 16
	ENTRY(i_ddi_rep_get16)
	ALTENTRY(i_ddi_swap_rep_get16)
	DDI_REP_GET(2,lgh,sth)
	br	%r14
	SET_SIZE(i_ddi_rep_get16)
	SET_SIZE(i_ddi_swap_rep_get16)

	.align 32
	ENTRY(ddi_rep_get32)
	ALTENTRY(ddi_rep_getl)
	ALTENTRY(ddi_mem_rep_get32)
	ALTENTRY(ddi_mem_rep_getl)
	lg      %r1,AHI_REP_GET32(%r2)
	br	%r1
	SET_SIZE(ddi_rep_get32)
	SET_SIZE(ddi_rep_getl)
	SET_SIZE(ddi_mem_rep_get32)
	SET_SIZE(ddi_mem_rep_getl)

	.align 16
	ENTRY(i_ddi_rep_get32)
	ALTENTRY(i_ddi_swap_rep_get32)
	DDI_REP_GET(4,lgf,st)
	br	%r14
	SET_SIZE(i_ddi_rep_get32)
	SET_SIZE(i_ddi_swap_rep_get32)

	.align 32
	ENTRY(ddi_rep_get64)
	ALTENTRY(ddi_rep_getll)
	ALTENTRY(ddi_mem_rep_get64)
	ALTENTRY(ddi_mem_rep_getll)
	lg      %r1,AHI_REP_GET64(%r2)
	br	%r1
	SET_SIZE(ddi_rep_get64)
	SET_SIZE(ddi_rep_getll)
	SET_SIZE(ddi_mem_rep_get64)
	SET_SIZE(ddi_mem_rep_getll)

	.align 16
	ENTRY(i_ddi_rep_get64)
	ALTENTRY(i_ddi_swap_rep_get64)
	DDI_REP_GET(8,lg,st)
	br	%r14
	SET_SIZE(i_ddi_rep_get64)
	SET_SIZE(i_ddi_swap_rep_get64)

/* 
 * This define processes the repetitive put functionality.  Automatic 
 * incrementing of the device address is determined by the flag field 
 * %r6.  
 */
#define DDI_REP_PUT(d,l,s)						\
	cghi	%r6,DDI_DEV_NO_AUTOINCR;	// No auto required	\
	jne	1f;							\
	##l	%r0,0(%r4);			// Get data		\
	##s	%r0,0(%r3);			// Plug into dest	\
1:									\
	ltgr	%r5,%r5;			// Zero rep count? 	\
	jz 	3f;				// Yes.. exit		\
2:									\
	##l	%r0,0(%r4);			// Get data		\
	##s	%r0,0(%r3);			// Plug into dest	\
	aghi	%r3,##d;			// Bump src ptr		\
	aghi	%r4,##d;			// Bump dst ptr		\
	brct	%r5,2b;				// Do until done	\
3:	

	.align 32
	ENTRY(ddi_rep_put8)
	ALTENTRY(ddi_rep_putb)
	ALTENTRY(ddi_mem_rep_put8)
	ALTENTRY(ddi_mem_rep_putb)
	lg	%r1,AHI_REP_PUT8(%r2)
	br	%r1
	SET_SIZE(ddi_rep_put8)
	SET_SIZE(ddi_rep_putb)
	SET_SIZE(ddi_mem_rep_put8)
	SET_SIZE(ddi_mem_rep_putb)

	.align 16
	ENTRY(i_ddi_rep_put8)
	DDI_REP_PUT(1,ic,stc)
	br	%r14
	SET_SIZE(i_ddi_rep_put8)

	.align 32
	ENTRY(ddi_rep_put16)
	ALTENTRY(ddi_rep_putw)
	ALTENTRY(ddi_mem_rep_put16)
	ALTENTRY(ddi_mem_rep_putw)
	lg	%r1,AHI_REP_PUT16(%r2)
	br	%r1
	SET_SIZE(ddi_rep_put16)
	SET_SIZE(ddi_rep_putw)
	SET_SIZE(ddi_mem_rep_put16)
	SET_SIZE(ddi_mem_rep_putw)

	.align 16
	ENTRY(i_ddi_rep_put16)
	ALTENTRY(i_ddi_swap_rep_put16)
	DDI_REP_PUT(2,lgh,sth)
	br	%r14
	SET_SIZE(i_ddi_rep_put16)
	SET_SIZE(i_ddi_swap_rep_put16)

	.align 32
	ENTRY(ddi_rep_put32)
	ALTENTRY(ddi_rep_putl)
	ALTENTRY(ddi_mem_rep_put32)
	ALTENTRY(ddi_mem_rep_putl)
	lg	%r1,AHI_REP_PUT32(%r2)
	br	%r1
	SET_SIZE(ddi_rep_put32)
	SET_SIZE(ddi_rep_putl)
	SET_SIZE(ddi_mem_rep_put32)
	SET_SIZE(ddi_mem_rep_putl)

	.align 16
	ENTRY(i_ddi_rep_put32)
	ALTENTRY(i_ddi_swap_rep_put32)
	DDI_REP_PUT(4,lgf,st)
	br	%r14
	SET_SIZE(i_ddi_rep_put32)
	SET_SIZE(i_ddi_swap_rep_put32)

	.align 32
	ENTRY(ddi_rep_put64)
	ALTENTRY(ddi_rep_putll)
	ALTENTRY(ddi_mem_rep_put64)
	ALTENTRY(ddi_mem_rep_putll)
	lg	%r1,AHI_REP_PUT64(%r2)
	br	%r1
	SET_SIZE(ddi_rep_put64)
	SET_SIZE(ddi_rep_putll)
	SET_SIZE(ddi_mem_rep_put64)
	SET_SIZE(ddi_mem_rep_putll)

	.align 16
	ENTRY(i_ddi_rep_put64)
	ALTENTRY(i_ddi_swap_rep_put64)
	DDI_REP_PUT(8,lg,stg)
	br	%r14
	SET_SIZE(i_ddi_rep_put64)
	SET_SIZE(i_ddi_swap_rep_put64)

	.align 16
	ENTRY(ddi_io_rep_get8)
	ALTENTRY(ddi_io_rep_getb)
	lghi	%r6,DDI_DEV_NO_AUTOINCR 	// Set flag to DDI_DEV_NO_AUTOINCR
	lg	%r1,AHI_REP_GET8(%r2) 
	br	%r1
	SET_SIZE(ddi_io_rep_get8)
	SET_SIZE(ddi_io_rep_getb)

	.align 16
	ENTRY(ddi_io_rep_get16)
	ALTENTRY(ddi_io_rep_getw)
	lghi	%r6,DDI_DEV_NO_AUTOINCR 	// Set flag to DDI_DEV_NO_AUTOINCR
	lg	%r1,AHI_REP_GET16(%r2) 
	br	%r1
	SET_SIZE(ddi_io_rep_get16)
	SET_SIZE(ddi_io_rep_getw)

	.align 16
	ENTRY(ddi_io_rep_get32)
	ALTENTRY(ddi_io_rep_getl)
	lghi	%r6,DDI_DEV_NO_AUTOINCR 	// Set flag to DDI_DEV_NO_AUTOINCR
	lg	%r1,AHI_REP_GET32(%r2) 
	br	%r1
	SET_SIZE(ddi_io_rep_get32)
	SET_SIZE(ddi_io_rep_getl)

	.align 16
	ENTRY(ddi_io_rep_get64)
	ALTENTRY(ddi_io_rep_getll)
	lghi	%r6,DDI_DEV_NO_AUTOINCR 	// Set flag to DDI_DEV_NO_AUTOINCR
	lg	%r1,AHI_REP_GET64(%r2) 
	br	%r1
	SET_SIZE(ddi_io_rep_get64)
	SET_SIZE(ddi_io_rep_getll)

        .align 64
	ENTRY(ddi_check_acc_handle)
	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)
	lg	%r1,AHI_FAULT_CHECK(%r2)
	basr	%r14,%r1
	ltgr	%r2,%r2				// if Rc = 0 then return(DDI_SUCCESS)
	jz	0f	

	lghi	%r2,-1				// else return (DDI_FAILURE)
0:
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	br	%r14	
	SET_SIZE(ddi_check_acc_handle)

        .align 16
        ENTRY(i_ddi_acc_fault_check)
	lg	%r2,AHI_FAULT(%r2)
	br	%r14
        SET_SIZE(i_ddi_acc_fault_check)

	.align 16
	ENTRY(ddi_io_rep_put8)
	ALTENTRY(ddi_io_rep_putb)
	lghi	%r6,DDI_DEV_NO_AUTOINCR 	// Set flag to DDI_DEV_NO_AUTOINCR
	lg	%r1,AHI_REP_PUT8(%r2) 
	br	%r1
	SET_SIZE(ddi_io_rep_put8)
	SET_SIZE(ddi_io_rep_putb)

	.align 16
	ENTRY(ddi_io_rep_put16)
	ALTENTRY(ddi_io_rep_putw)
	lghi	%r6,DDI_DEV_NO_AUTOINCR 	// Set flag to DDI_DEV_NO_AUTOINCR
	lg	%r1,AHI_REP_PUT16(%r2) 
	br	%r1
	SET_SIZE(ddi_io_rep_put16)
	SET_SIZE(ddi_io_rep_putw)

	.align 16
	ENTRY(ddi_io_rep_put32)
	ALTENTRY(ddi_io_rep_putl)
	lghi	%r6,DDI_DEV_NO_AUTOINCR 	// Set flag to DDI_DEV_NO_AUTOINCR
	lg	%r1,AHI_REP_PUT32(%r2) 
	br	%r1
	SET_SIZE(ddi_io_rep_put32)
	SET_SIZE(ddi_io_rep_putl)

	.align 16
	ENTRY(ddi_io_rep_put64)
	ALTENTRY(ddi_io_rep_putll)
	lghi	%r6,DDI_DEV_NO_AUTOINCR 	// Set flag to DDI_DEV_NO_AUTOINCR
	lg	%r1,AHI_REP_PUT64(%r2) 
	br	%r1
	SET_SIZE(ddi_io_rep_put64)
	SET_SIZE(ddi_io_rep_putll)

	.align 16
	ENTRY(do_peek)
	stosm	48(%r15),0x00			// Save the system mask
	tm	48(%r15),0x03			// Interrupts enabled?
	jz	.peekErr			// No... That's bad

	larl	%r5,.peekIt			// Address the move
	aghi	%r2,-1				// Adjust length for move
	ex	%r2,0(%r5)			// Peek the data
	lghi	%r2,0				// Set the all clear
	br	%r14				// Return

.peekErr:
	lghi	%r2,-1				// Indicate error has occurred
	br	%r14				// Return
.peekIt:
	mvc	0(1,%r4),0(%r3)			// Peek the data
	SET_SIZE(do_peek)

	.align 16
	ENTRY(do_poke)
	stosm	48(%r15),0x00			// Save the system mask
	tm	48(%r15),0x03			// Interrupts enabled?
	jz	.pokeErr			// No... That's bad

	larl	%r5,.pokeIt			// Address the move
	aghi	%r2,-1				// Adjust length for move
	ex	%r2,0(%r5)			// Peek the data
	lghi	%r2,0				// Set the all clear
	br	%r14				// Return

.pokeErr:
	lghi	%r2,-1				// Indicate error has occurred
	br	%r14				// Return
.pokeIt:
	mvc	0(1,%r3),0(%r4)			// Peek the data
	SET_SIZE(do_peek)

/*
 * The peek_fault() and poke_fault() routines below are used as on_trap()
 * trampoline routines.  i_ddi_peek and i_ddi_poke execute do_peek and do_poke
 * under on_trap protection (see <sys/ontrap.h>), but modify ot_trampoline to
 * refer to the corresponding routine below.  If a trap occurs, the trap code
 * will bounce back to the trampoline code, which will effectively cause
 * do_peek or do_poke to return DDI_FAILURE, instead of longjmp'ing back to
 * on_trap.  In the case of a peek, we may also need to re-enable interrupts.
 */
	.section ".data"
.peek_panic:
	.asciz	"peek_fault: missing or invalid on_trap_data"
.poke_panic:
	.asciz	"poke_fault: missing or invalid on_trap_data"

	.align 16
	ENTRY(peek_fault)
	
	GET_THR (4)

	lg	%r2,T_ONTRAP(%r4)		// Get Trap pointer
	ltgr	%r2,%r2				// Is it set?
	jz	.peekFail			// No... Get out

	llgh	%r3,OT_PROT(%r2)		// Get protection
	nill	%r3,OT_DATA_ACCESS		// OT_DATA_ACCESS set?
	jz	.peekFail			// No... Panic time

	stosm	48(%r15),0x03			// Enable interrupts
	lghi	%r2,-1				// Set DDI_FAILURE
	br	%r14				// Return

.peekFail:
	larl	%r2,.peek_panic			// Get A(Panic Message)
	brasl	%r14,panic			// Go panic
	SET_SIZE(peek_fault)

	.align 16
	ENTRY(poke_fault)
	
	GET_THR (4)

	lg	%r2,T_ONTRAP(%r4)		// Get Trap pointer
	ltgr	%r2,%r2				// Is it set?
	jz	.pokeFail			// No... Get out

	llgh	%r3,OT_PROT(%r2)		// Get protection
	nill	%r3,OT_DATA_ACCESS		// OT_DATA_ACCESS set?
	jz	.pokeFail			// No... Panic time

	lghi	%r2,-1				// Set DDI_FAILURE
	br	%r14				// Return

.pokeFail:
	larl	%r2,.poke_panic			// Get A(Panic Message)
	brasl	%r14,panic			// Go panic
	SET_SIZE(poke_fault)

#endif	/* lint	*/
