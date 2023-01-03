/*
 * This file is provided under a CDDLv1 license.  When using or
 * redistributing this file, you may do so under this license.
 * In redistributing this file this license must be included
 * and no other modification of this header file is permitted.
 *
 * CDDL LICENSE SUMMARY
 *
 * Copyright(c) 1999 - 2008 Intel Corporation. All rights reserved.
 *
 * The contents of this file are subject to the terms of Version
 * 1.0 of the Common Development and Distribution License (the "License").
 *
 * You should have received a copy of the License with this software.
 * You can obtain a copy of the License at
 *	http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 */

/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms of the CDDLv1.
 */

#ifndef _E1000_OSDEP_H
#define	_E1000_OSDEP_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/conf.h>
#include <sys/debug.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/strlog.h>
#include <sys/kmem.h>
#include <sys/stat.h>
#include <sys/kstat.h>
#include <sys/modctl.h>
#include <sys/errno.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/pci.h>
#include <sys/atomic.h>
#include "e1000g_debug.h"

/*
 * === BEGIN CONTENT FORMERLY IN FXHW.H ===
 */
#define	usec_delay(x)		drv_usecwait(x)
#define	msec_delay(x)		drv_usecwait(x * 1000)

#ifdef E1000G_DEBUG
#define	DEBUGOUT(S)		\
	E1000G_DEBUGLOG_0(NULL, E1000G_INFO_LEVEL, S)
#define	DEBUGOUT1(S, A)		\
	E1000G_DEBUGLOG_1(NULL, E1000G_INFO_LEVEL, S, A)
#define	DEBUGOUT2(S, A, B)	\
	E1000G_DEBUGLOG_2(NULL, E1000G_INFO_LEVEL, S, A, B)
#define	DEBUGOUT3(S, A, B, C)	\
	E1000G_DEBUGLOG_3(NULL, E1000G_INFO_LEVEL, S, A, B, C)
#define	DEBUGFUNC(F)		\
	E1000G_DEBUGLOG_0(NULL, E1000G_TRACE_LEVEL, F)
#else
#define	DEBUGOUT(S)
#define	DEBUGOUT1(S, A)
#define	DEBUGOUT2(S, A, B)
#define	DEBUGOUT3(S, A, B, C)
#define	DEBUGFUNC(F)
#endif

#define	OS_DEP(hw)		((struct e1000g_osdep *)((hw)->back))

#define	FALSE		0
#define	TRUE		1
#define	CMD_MEM_WRT_INVALIDATE	0x0010	/* BIT_4 */
#define	PCI_COMMAND_REGISTER	0x04
#define	PCI_EX_CONF_CAP		0xE0
#define	ICH_FLASH_REG_SET	2	/* solaris mapping of flash memory */

#define	RECEIVE_BUFFER_ALIGN_SIZE	256
#define	E1000_MDALIGN			4096
#define	E1000_ERT_2048			0x100

#define	E1000_DEV_ID_ICH10D_BM_LM	0x10DE

/* PHY Extended Status Register */
#define	IEEE_ESR_1000T_HD_CAPS	0x1000	/* 1000T HD capable */
#define	IEEE_ESR_1000T_FD_CAPS	0x2000	/* 1000T FD capable */
#define	IEEE_ESR_1000X_HD_CAPS	0x4000	/* 1000X HD capable */
#define	IEEE_ESR_1000X_FD_CAPS	0x8000	/* 1000X FD capable */

/*
 * required by shared code
 */
#define	E1000_WRITE_FLUSH(a)	E1000_READ_REG(a, E1000_STATUS)

#define	E1000_WRITE_REG(hw, reg, value)	\
{\
	if ((hw)->mac.type != e1000_82542) \
		ddi_put32((OS_DEP(hw))->reg_handle, \
		    (uint32_t *)((hw)->hw_addr + reg), \
		    value); \
	else \
		ddi_put32((OS_DEP(hw))->reg_handle, \
		    (uint32_t *)((hw)->hw_addr + \
		    e1000_translate_register_82542(reg)), \
		    value); \
}

#define	E1000_READ_REG(hw, reg) (\
	((hw)->mac.type != e1000_82542) ? \
	    ddi_get32((OS_DEP(hw))->reg_handle, \
		(uint32_t *)((hw)->hw_addr + reg)) : \
	    ddi_get32((OS_DEP(hw))->reg_handle, \
		(uint32_t *)((hw)->hw_addr + \
		e1000_translate_register_82542(reg))))

#define	E1000_WRITE_REG_ARRAY(hw, reg, offset, value) \
{\
	if ((hw)->mac.type != e1000_82542) \
		ddi_put32((OS_DEP(hw))->reg_handle, \
		    (uint32_t *)((hw)->hw_addr + reg + ((offset) << 2)),\
		    value); \
	else \
		ddi_put32((OS_DEP(hw))->reg_handle, \
		    (uint32_t *)((hw)->hw_addr + \
		    e1000_translate_register_82542(reg) + \
		    ((offset) << 2)), value); \
}

#define	E1000_READ_REG_ARRAY(hw, reg, offset) (\
	((hw)->mac.type != e1000_82542) ? \
	    ddi_get32((OS_DEP(hw))->reg_handle, \
		(uint32_t *)((hw)->hw_addr + reg + ((offset) << 2))) : \
	    ddi_get32((OS_DEP(hw))->reg_handle, \
		(uint32_t *)((hw)->hw_addr + \
		e1000_translate_register_82542(reg) + \
		((offset) << 2))))


#define	E1000_WRITE_REG_ARRAY_BYTE(a, reg, offset, value)	NULL
#define	E1000_WRITE_REG_ARRAY_WORD(a, reg, offset, value)	NULL
#define	E1000_WRITE_REG_ARRAY_DWORD(a, reg, offset, value)	NULL
#define	E1000_READ_REG_ARRAY_BYTE(a, reg, offset)		NULL
#define	E1000_READ_REG_ARRAY_WORD(a, reg, offset)		NULL
#define	E1000_READ_REG_ARRAY_DWORD(a, reg, offset)		NULL


#define	E1000_READ_FLASH_REG(hw, reg)	\
	ddi_get32((OS_DEP(hw))->ich_flash_handle, \
		(uint32_t *)((hw)->flash_address + (reg)))

#define	E1000_READ_FLASH_REG16(hw, reg)	\
	ddi_get16((OS_DEP(hw))->ich_flash_handle, \
		(uint16_t *)((hw)->flash_address + (reg)))

#define	E1000_WRITE_FLASH_REG(hw, reg, value)	\
	ddi_put32((OS_DEP(hw))->ich_flash_handle, \
		(uint32_t *)((hw)->flash_address + (reg)), (value))

#define	E1000_WRITE_FLASH_REG16(hw, reg, value)	\
	ddi_put16((OS_DEP(hw))->ich_flash_handle, \
		(uint16_t *)((hw)->flash_address + (reg)), (value))

/*
 * === END CONTENT FORMERLY IN FXHW.H ===
 */

#define	msec_delay_irq	msec_delay

typedef	int8_t		s8;
typedef	int16_t		s16;
typedef	int32_t		s32;
typedef	int64_t		s64;
typedef	uint8_t		u8;
typedef	uint16_t	u16;
typedef	uint32_t	u32;
typedef	uint64_t	u64;

typedef uint8_t		UCHAR;	/* 8-bit unsigned */
typedef UCHAR		UINT8;	/* 8-bit unsigned */
typedef uint16_t	USHORT;	/* 16-bit unsigned */
typedef uint16_t	UINT16;	/* 16-bit unsigned */
typedef uint32_t	ULONG;	/* 32-bit unsigned */
typedef uint32_t	UINT32;
typedef uint32_t	UINT;	/* 32-bit unsigned */
typedef UCHAR		BOOLEAN;
typedef	BOOLEAN		bool;
typedef UCHAR		*PUCHAR;
typedef UINT		*PUINT;
typedef ULONG		*PLONG;
typedef ULONG		NDIS_STATUS;
typedef USHORT		*PUSHORT;
typedef PUSHORT		PUINT16; /* 16-bit unsigned pointer */
typedef ULONG		E1000_32_BIT_PHYSICAL_ADDRESS,
	*PFX_32_BIT_PHYSICAL_ADDRESS;
typedef uint64_t	E1000_64_BIT_PHYSICAL_ADDRESS,
	*PFX_64_BIT_PHYSICAL_ADDRESS;

struct e1000g_osdep {
	ddi_acc_handle_t reg_handle;
	ddi_acc_handle_t cfg_handle;
	ddi_acc_handle_t ich_flash_handle;
	struct e1000g *adapter;
};

#ifdef __sparc	/* on SPARC, use only memory-mapped routines */
#define	E1000_WRITE_REG_IO	E1000_WRITE_REG
#else	/* on x86, use port io routines */
#define	E1000_WRITE_REG_IO(a, reg, val)	{ \
	outl(((a)->io_base), reg); \
	outl(((a)->io_base + 4), val); }
#endif	/* __sparc */

#ifdef __cplusplus
}
#endif

#endif	/* _E1000_OSDEP_H */
