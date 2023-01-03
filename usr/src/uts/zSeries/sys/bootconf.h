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

#ifndef	_SYS_BOOTCONF_H
#define	_SYS_BOOTCONF_H

/*
 * Boot time configuration information objects
 */

#include <sys/types.h>
#include <sys/varargs.h>
#include <sys/memlist.h>
#include <sys/memlist_plat.h>
#include <sys/bootstat.h>
#include <net/if.h>			/* for IFNAMSIZ */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * masks to hand to bsys_alloc memory allocator
 * XXX	These names shouldn't really be srmmu derived.
 */
#define	BO_NO_ALIGN	0x00001000
#define	BO_ALIGN_L3	0x00001000
#define	BO_ALIGN_L2	0x00040000
#define	BO_ALIGN_L1	0x01000000

#define MAGIC_PHYS	0

/*
 *  We pass a ptr to the space that boot has been using
 *  for its memory lists.
 */
struct bsys_mem {
	void	       *scratchLimit;	/* End of scratch memory area */
	uint_t		extent; 	/* number of bytes in the space */
};

typedef struct bootops {
	/*
	 * the ubiquitous version number
	 */
	uint_t	bsys_version;

	/*
	 * Region/Segment/Page table descriptors
	 */
	void   	*bootRSP;

	/*
	 * Memory descriptors
	 */
	memoryChunk *bootSysMem;		// Physical Memory layout
	size_t bootAvailMem;			// Available memory
	int    bootNChunks;			// Number of chunks

	/*
	 * the area containing boot's memlists
	 */
	struct 	bsys_mem *boot_mem;

	/*
	 * have boot allocate size bytes at virthint
	 */
	caddr_t	(*bsys_alloc)(caddr_t, size_t, int);

	/*
	 * have boot allocate size bytes of real memory
	 */
	caddr_t	(*bsys_allreal)(size_t, int);

	/*
	 * free size bytes allocated at virt - put the
	 * address range back onto the avail lists.
	 */
	void	(*bsys_free)(caddr_t, size_t);

	/*
	 * to find the size of the buffer to allocate
	 */
	int	(*bsys_getproplen)(const char *);

	/*
	 * get the value associated with this name
	 */
	int	(*bsys_getprop)(const char *, void *);

	/*
	 * get the value associated with this name
	 */
	int	(*bsys_setprop)(const char *, int, void *);

	/*
	 * print formatted output
	 */
	void	(*bsys_printf)(char *, ...);

	/*
	 * print formatted output variable parm list
	 */
	void	(*bsys_vprintf)(char *, va_list);

	/*
	 * print formatted output - kobj_printf compatible
	 */
	void	(*bsys_kprintf)(struct bootops *, char *, ...);

	/* end of bootops which exist if (bootops-extensions >= 1) */
} bootops_t;

extern caddr_t	bop_alloc(struct bootops *, caddr_t, size_t, int);
extern caddr_t	bop_allreal(struct bootops *, size_t, int);
extern caddr_t	boot_alloc_virt(caddr_t virt, size_t size);
extern void	bop_free(caddr_t virt, size_t size);
extern void	kipl_kprintf(struct bootops *, char *, ...);

#define	BO_VERSION	11		/* bootops interface revision # */

#define	BOP_GETVERSION(bop)		((bop)->bsys_version)
#define	BOP_ALLOC(bop, virthint, size, align)	\
				bop_alloc(bop, virthint, size, align)
#define	BOP_ALLREAL(bop, size, align)	\
				bop_allreal(bop, size, align)
#define	BOP_FREE(bop, virt, size)	((bop)->bsys_free)(virt, size)
#define	BOP_GETPROPLEN(bop, name)	((bop)->bsys_getproplen)(name)
#define	BOP_GETPROP(bop, name, buf)	((bop)->bsys_getprop)(name, buf)
#define	BOP_SETPROP(bop, name, ln, buf)	((bop)->bsys_setprop)(name, ln, buf)
#define	BOP_NEXTPROP(bop, prev)		((bop)->bsys_nextprop)(prev)
#define	BOP_PUTSARG(bop, msg, arg)	((bop)->bsys_printf)(msg, arg)
#define	BOP_ENTER_MON(bop)		

#define	BOOT_SVC_FAIL	(int)(-1)
#define	BOOT_SVC_OK	(int)(1)

#if defined(_KERNEL) && !defined(_BOOT)

/*
 * Boot configuration information
 */

#define	BO_MAXFSNAME	16
#define	BO_MAXOBJNAME	256

struct bootobj {
	char	bo_fstype[BO_MAXFSNAME];	/* vfs type name (e.g. nfs) */
	char	bo_name[BO_MAXOBJNAME];		/* name of object */
	int	bo_flags;			/* flags, see below */
	int	bo_size;			/* number of blocks */
	struct vnode *bo_vp;			/* vnode of object */
	char	bo_devname[BO_MAXOBJNAME];
	char	bo_ifname[BO_MAXOBJNAME];
	int	bo_ppa;
};

/*
 * flags
 */
#define	BO_VALID	0x01	/* all information in object is valid */
#define	BO_BUSY		0x02	/* object is busy */

extern struct bootobj rootfs;
extern struct bootobj swapfile;

extern char obp_bootpath[BO_MAXOBJNAME];
extern char svm_bootpath[BO_MAXOBJNAME];

extern dev_t getrootdev(void);
extern void getfsname(char *, char *, size_t);
extern int loadrootmodules(void);

extern int strplumb(void);
extern int strplumb_load(void);

extern void consconfig(void);

extern int dhcpinit(void);

/* XXX	Doesn't belong here */
extern int zsgetspeed(dev_t);

extern void param_check(void);

extern struct bootops *bootops;
extern int netboot;
extern int swaploaded;
extern int modrootloaded;
extern char kern_bootargs[];
extern char *default_path;
extern char *dhcack;
extern int dhcacklen;
extern char dhcifname[IFNAMSIZ];
extern char *netdev_path;

#endif /* _KERNEL && !_BOOT */

#ifdef __cplusplus
}
#endif

#endif	/* _SYS_BOOTCONF_H */
