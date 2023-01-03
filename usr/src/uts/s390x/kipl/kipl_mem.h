/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License                  
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
 *                                              
 * Copyright 2008 Sine Nomine Associates.        
 * All rights reserved.                           
 * Use is subject to license terms.                
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	_SYS_STANDALLOC_H
#define	_SYS_STANDALLOC_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/param.h>

/*
 * Types of resources that can be allocated by resalloc().
 */
enum RESOURCES {
	RES_MAINMEM,		/* Main memory, accessible to CPU */
	RES_RAWVIRT,		/* Raw addr space that can be mapped */
	RES_DMAMEM,		/* Memory acc. by CPU and by all DMA I/O */
	RES_DMAVIRT,		/* Raw addr space accessible by DMA I/O */
	RES_PHYSICAL,		/* Physical address */
	RES_VIRTALLOC,		/* Virtual addresses used */
	RES_BOOTSCRATCH,	/* Memory <4MB used only by boot. */
	RES_CHILDVIRT		/* Virt anywhere, phys > 4MB */
};

#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))
#define	rounddown(x, y)	(((x)/(y))*(y))

/* backing resources for memory allocation */
caddr_t resalloc(enum RESOURCES type, size_t, caddr_t, int);
void resfree(caddr_t, size_t);
void reset_alloc(void);

/* memory allocation */
void *vmx_zalloc_identity(size_t);

caddr_t phys_alloc_mem(size_t, int);

void	*kipl_alloc(size_t);
void	kipl_free(void *ptr, size_t nbytes);

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_STANDALLOC_H */
