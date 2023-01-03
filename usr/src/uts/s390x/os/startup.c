/*------------------------------------------------------------------*/
/* 								    */
/* Name        - startup.c  					    */
/* 								    */
/* Function    - Commence the startup of the Solaris OS on the      */
/* 		 System z hardware				    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - July, 2006  					    */
/* 								    */
/*------------------------------------------------------------------*/

/*==================================================================*/
/* 								    */
/* CDDL HEADER START						    */
/* 								    */
/* The contents of this file are subject to the terms of the	    */
/* Common Development and Distribution License                      */
/* (the "License").  You may not use this file except in compliance */
/* with the License.						    */
/* 								    */
/* You can obtain a copy of the license at: 			    */
/* - usr/src/OPENSOLARIS.LICENSE, or,				    */
/* - http://www.opensolaris.org/os/licensing.			    */
/* See the License for the specific language governing permissions  */
/* and limitations under the License.				    */
/* 								    */
/* When distributing Covered Code, include this CDDL HEADER in each */
/* file and include the License file at usr/src/OPENSOLARIS.LICENSE.*/
/* If applicable, add the following below this CDDL HEADER, with    */
/* the fields enclosed by brackets "[]" replaced with your own      */
/* identifying information: 					    */
/* Portions Copyright [yyyy] [name of copyright owner]		    */
/* 								    */
/* CDDL HEADER END						    */
/*                                                                  */
/* Copyright 2008 Sine Nomine Associates.                           */
/* All rights reserved.                                             */
/* Use is subject to license terms.                                 */
/* 								    */
/*==================================================================*/

//                        Physical memory layout
//                     (not necessarily contiguous)
//                       (THIS IS SOMEWHAT WRONG)
//                       /-----------------------\ top
//                       |       monitor pages   |
//             availmem -|-----------------------|
//                       |                       |
//                       |       page pool       |
//                       |                       |
//                       |-----------------------|
//                       |   configured tables   |
//                       |       buffers         |
//            firstaddr -|-----------------------|
//                       |   hat data structures |
//                       |-----------------------|
//                       |    kernel data, bss   |
//                       |-----------------------|
//                       |    interrupt stack    |
//                       |-----------------------|
//                       |    kernel text (RO)   |
//                       |-----------------------|
//                       |    trace table (48k)  |
//                       |-----------------------|
//               page 3  |      panicbuf         |
//                       |-----------------------|
//               page 0  |      lowcore          |
//                       |_______________________|
//
//
//
//                    Kernel's Virtual Memory Layout.
//                       /-----------------------\ top
// 0xFFFFFFFF.FFFFFFFF  -|                       |-
//                       |                       |
//                       :                       :
//                       :                       :
// 0xFFFFFE00.00000000  -|-----------------------|-
//                       |                       |  
//                       |    segkpm segment     | 
//                       | (64-bit kernel ONLY)  |
//                       |                       |
// 0xFFFFFA00.00000000  -|-----------------------|- 2TB segkpm alignment
//                       :                       :
//                       :                       :
//                       :                       :		   ^
//                       :                       :		   |
// 0x00000XXX.XXXXXXXX  -|-----------------------|- kmem64_end	   |
//                       |                       |		   |
//                       |   64-bit kernel ONLY  |		   |
//                       |                       |		   |
//                       |    kmem64 segment     |		   |
//                       |                       |		   |
//                       | (Relocated extra HME  |	     Approximately
//                       |   block allocations,  |	    1 TB of virtual
//                       |   memnode freelists,  |	     address space
//                       |    HME hash buckets,  |		   |
//                       | mml_table, kpmp_table,|		   |
//                       |  page_t array and     |		   |
//                       |  hashblock pool to    |		   |
//                       |   avoid hard-coded    |		   |
//                       |     32-bit vaddr      |		   |
//                       |     limitations)      |		   |
//                       |                       |		   v
// 0x00000700.00000000  -|-----------------------|- SYSLIMIT (kmem64_base)
//                       |                       |
//                       |  segkmem segment      | (SYSLIMIT - SYSBASE = 4TB)
//                       |                       |
// 0x00000300.00000000  -|-----------------------|- SYSBASE
//                       :                       :
//                       :                       :
//                      -|-----------------------|-
//                       |                       |
//                       |  segmap segment       |   SEGMAPSIZE (1/8th physmem,
//                       |                       |               256G MAX)
// 0x000002a7.50000000  -|-----------------------|- SEGMAPBASE
//                       :                       :
//                       :                       :
//                      -|-----------------------|-
//                       |                       |
//                       |       segkp           |    SEGKPSIZE (2GB)
//                       |                       |
//                       |                       |
// 0x000002a1.00000000  -|-----------------------|- SEGKPBASE
//                       |                       |
// 0x000002a0.00000000  -|-----------------------|- MEMSCRUBBASE
//                       |                       |       (SEGKPBASE - 0x400000)
// 0x0000029F.FFE00000  -|-----------------------|- ARGSBASE
//                       |                       |       (MEMSCRUBBASE - NCARGS)
// 0x0000029F.FFD80000  -|-----------------------|- PPMAPBASE
//                       |                       |       (ARGSBASE - PPMAPSIZE)
// 0x0000029F.FFD00000  -|-----------------------|- PPMAP_FAST_BASE
//                       |                       |
// 0x0000029F.FF980000  -|-----------------------|- PIOMAPBASE
//                       |                       |
// 0x0000029F.FF580000  -|-----------------------|- NARG_BASE
//                       :                       :
//                       :                       :
// 0x00000000.FFFFFFFF  -|-----------------------|- OFW_END_ADDR
//                       |                       |
//                       |         OBP           |
//                       |                       |
// 0x00000000.F0000000  -|-----------------------|- OFW_START_ADDR
//                       |         kmdb          |
// 0x00000000.EDD00000  -|-----------------------|- SEGDEBUGBASE
//                       :                       :
//                       :                       :
// 0x00000000.80002000  -|-----------------------|
//                       |     panicbuf          |
// 0x00000000.80000000  -|-----------------------|- core_end
//                       |     core heap         |
// 0x00000000.40000000  -|-----------------------|- core_base
//                       :                       :
//                       :                       :
//                       |                       |
//                       |-----------------------|- 
//                       |    vm structures      |
// 0x00000000.01C00000   |-----------------------|- nalloc_end
//                       |         TSBs          |
//                       |-----------------------|- end/nalloc_base
//                       |   kernel data & bss   |
// 0x00000000.01800000  -|-----------------------|
//                       :   nucleus text hole   :
// 0x00000000.01400000  -|-----------------------|
//                       :                       :
//                       |-----------------------|
//                       |      module text      |
//                       |-----------------------|- e_text/modtext
//                       |      kernel text      |
//                       |-----------------------|
//                       |    trace table (48k)  |
// 0x00000000.01000000  -|-----------------------|- KERNELBASE
//                       | reserved for trapstat |} TSTAT_TOTAL_SIZE
//                       |-----------------------|
//                       |                       |
//                       |        invalid        |
//                       |                       |
// 0x00000000.00000000  _|_______________________|
//
//
//
//                   64-bit User Virtual Memory Layout.
//                       /-----------------------\ top
//                       |                       |
//                       |        invalid        |
//                       |                       |
//  0xFFFFFFFF.80000000 -|-----------------------|- USERLIMIT
//                       |       user stack      |
//                       :                       :
//                       :                       :
//                       :                       :
//                       |       user data       |
//                      -|-----------------------|-
//                       |       user text       |
//  0x00000000.00100000 -|-----------------------|-
//                       |       invalid         |
//  0x00000000.00000000 _|_______________________|
//
//                   32-bit User Virtual Memory Layout.
//                       /-----------------------\ top
//                       |                       |
//                       |        invalid        |
//                       |                       |
//  0x00000000.7FFF0000 -|-----------------------|- USERLIMIT32
//                       |       user stack      |
//                       :                       :
//                       :                       :
//                       :                       :
//                       |       user data       |
//                      -|-----------------------|-
//                       |       user text       |
//  0x00000000.00100000 -|-----------------------|-
//                       |       invalid         |
//  0x00000000.00000000 _|_______________________|

/*------------------------------------------------------------------*/
/*                 D e f i n e s                                    */
/*------------------------------------------------------------------*/

#define DEBUGGING_MEM 1
#ifdef DEBUGGING_MEM
# define	debug_pause(str)	halt((str))
# define	MPRINTF(str)		if (debugging_mem) prom_printf((str))
# define	MPRINTF1(str, a)	if (debugging_mem) prom_printf((str), (a))
# define	MPRINTF2(str, a, b)	if (debugging_mem) prom_printf((str), (a), (b))
# define	MPRINTF3(str, a, b, c) 	if (debugging_mem) prom_printf((str), (a), (b), (c))
#else	/* DEBUGGING_MEM */
# define	MPRINTF(str)
# define	MPRINTF1(str, a)
# define	MPRINTF2(str, a, b)
# define	MPRINTF3(str, a, b, c)
#endif	/* DEBUGGING_MEM */

#define	TBUFSIZE	1024

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/avintr.h>
#include <sys/intreg.h>
#include <sys/machsystm.h>
#include <sys/archsystm.h>
#include <sys/bootconf.h>
#include <sys/bootvfs.h>
#include <sys/vm.h>
#include <sys/cpu.h>
#include <sys/cpuvar.h>
#include <sys/atomic.h>
#include <sys/reboot.h>
#include <sys/kdi.h>
#include <sys/memlist_plat.h>
#include <sys/memlist_impl.h>
#include <sys/modctl.h>
#include <sys/autoconf.h>
#include <sys/kobj_impl.h>
#include <sys/sunddi.h>
#include <vm/vm_dep.h>
#include <vm/seg_dev.h>
#include <vm/seg_kmem.h>
#include <vm/seg_kpm.h>
#include <vm/seg_map.h>
#include <vm/seg_kp.h>
#include <vm/mm_s390x.h>
#include <vm/vm_dep.h>
#include <sys/sysconf.h>
#include <sys/machs390x.h>
#include <sys/smp.h>
#include <vm/hat_s390x.h>
#include <sys/kobj.h>
#include <sys/clconf.h>
#include <sys/panic.h>
#include <sys/clock.h>
#include <sys/cmn_err.h>
#include <sys/promif.h>
#include <sys/prom_debug.h>
#include <sys/memnode.h>
#include <sys/mem_cage.h>
#include <sys/devinit.h>
#include <sys/machs390x.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*          E x t e r n a l   R e f e r e n c e s    		    */
/*------------------------------------------------------------------*/

extern int size_pse_array(pgcnt_t, int);
extern void page_coloring_init(void);
extern uint_t dumpSys(caddr_t, caddr_t);

extern int vac_size;    		// cache size in bytes 
extern uint_t vac_mask; 		// VAC alignment consistency mask 
extern uint_t vac_colors;

extern struct cpu cpu0;

extern ssize_t RSPSize;			// Size of Region/Segment/Page tables

extern int	nMemChunk;		// Number of chunks in sysMemory
extern long  	availmem;		// Amount of memory available
extern long  	highmem;		// Highet memory address

//
// Memory lists created by boot processing
//
extern	struct memlist	*pinstalledp, 
			*pfreelistp, 
			*pavailistp, 
			*vfreelistp, 
			*pbooterp,
			*pramdiskp;

extern struct boot_fs_ops bfs_ufs, bhsfs_ops;

extern int use_brk_lpg;			// Large page support
extern int use_stk_lpg;

extern int moddebug;

extern cpuset_t mp_cpus;

extern void kobj_boot_unmountroot(void);

extern void 	*_ramdk,
		*_eramd;

//
// Tracking of early memory allocations - before page hash is built
//
extern earlyAlloc *primalPages;

extern char *isa_list;

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

/*
 * Static Routines:
 */
static void memlist_add(uint64_t, uint64_t, struct memlist **,
			struct memlist **);
static void memseg_list_add(struct memseg *);
static void kphysm_init(page_t *, struct memseg *, pgcnt_t, uintptr_t,
			pgcnt_t);
static void kvm_init(void);
static void startup_modules(void);
static void startup_vm(void);
static void startup_end(void);

#if 0
static void startup_init(void);
static void startup_bop_gone(void);
static void setup_cage_params(void);
static void startup_create_io_node(void);
#endif
static void count_cpus(void);
static void mem_init(void);
static void kpm_init(void);
static void kpm_npages_setup(int);
static struct memlist * memlist_init(struct memlist **, void *, size_t);
static void setup_trap_table(void);
static void claimBootPages(void);
static void sets390isalist(void);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*
 * Declare these as initialized data so we can patch them.
 */
pgcnt_t physmem = 0;		// Memory size in pages
pgcnt_t segkpsize =
    btop(SEGKPDEFSIZE);		// Size of segkp segment in pages 
uint_t segmap_percent = 12; 	// Size of segmap segment 

int vac_copyback = 1;
char *cache_mode = NULL;
int use_mix = 1;

caddr_t s_text;			// Start of kernel text segment 
caddr_t e_text;			// End of kernel text segment 
caddr_t s_data;			// Start of kernel data segment 
caddr_t e_data;			// End of kernel data segment 

caddr_t modtext;		// Beginning of module text 
size_t	modtext_sz;		// Size of module text 
caddr_t moddata;		// Beginning of module data reserve 
caddr_t e_moddata;		// End of module data reserve 

caddr_t	ncbase;			// Beginning of non-cached segment 
caddr_t	ncend;			// End of non-cached segment 
caddr_t	sdata;			// Beginning of data segment 

size_t	valloc_sz;		// Virtual address space allocated

caddr_t	extra_etva;		// Beginning of unused nucleus text 
pgcnt_t	extra_etpg;		// Number of pages of unused nucleus text 

size_t	ndata_remain_sz;	// bytes from end of data to 4MB boundary 
caddr_t	nalloc_base;		// beginning of nucleus allocation 
caddr_t nalloc_end;		// end of nucleus allocatable memory 
caddr_t valloc_base;		// beginning of kvalloc segment	

caddr_t kmem64_base;		// base of kernel mem segment in 64-bit space 
caddr_t kmem64_end;		// end of kernel mem segment in 64-bit space 

// core_base: start of the kernel's "core" heap area.
// This area is intended to be used for global data as well as for module
// text/data that does not fit into the nucleus pages.  The core heap is
// restricted to being within 2GB of kernel text, allowing every address within
// it to be accessed using long relative addressing
uintptr_t core_base;		
uintptr_t core_end;
size_t	core_size;		// size of "core" heap

caddr_t econtig;		// end of first block of contiguous kernel 

uintptr_t shm_alignment = 0;	// VAC address consistency modulus 
struct memlist *phys_install;	// Total installed physical memory 
struct memlist *phys_avail;	// Available physical memory 
struct memlist *phys_free; 	// Unreserved available physical memory 
struct memlist *virt_avail;	// Available (unmapped?) virtual memory 
struct memlist ndata;		// memlist of nucleus allocatable memory 

//
// VM data structures
//
int pse_shift;			/* log2(pse_table_size) */
pad_mutex_t *pse_mutex;		/* Locks protecting pp->p_selock */
size_t pse_table_size;		/* Number of mutexes in pse_mutex[] */
long page_hashsz;		// Size of page hash table (power of two) 
struct page *pp_base;		// Base of system page struct array 
size_t pp_sz;			// Size in bytes of page struct array 
struct page **page_hash;	// Page hash table 
struct seg ktextseg;		// Segment used for kernel executable image 
struct seg kvalloc;		// Segment used for "valloc" mapping 
struct seg kpseg;		// Segment used for pageable kernel virt mem 
struct seg ktexthole;		// Segment used for nucleus text hole 
struct seg kmapseg;		// Segment used for generic kernel mappings 
struct seg kpmseg;		// Segment used for physical mapping 
struct seg kdebugseg;		// Segment used for the kernel debugger 
struct seg kvseg_core;		// Segment used for the core heap 

uintptr_t kpm_pp_base;		// Base of system kpm_page array 
size_t	kpm_pp_sz;		// Size of system kpm_page array 
pgcnt_t	kpm_npages;		// How many kpm pages are managed 

struct seg *segkp = &kpseg;	// Pageable kernel virtual memory segment 
struct seg *segkmap = &kmapseg;	// Kernel generic mapping segment 
struct seg *segkpm = &kpmseg;	// 64bit kernel physical mapping segment 

//
// Size of nalloc area
//
size_t nalloc_sz;

//
// debugger pages (if allocated)
//
struct vnode kdebugvp;

//
// Segment for relocated kernel structures in 64-bit large RAM kernels
//
struct seg kmem64;

struct memseg *memseg_base;
size_t memseg_sz;		// Used to translate a va to page 
struct vnode unused_pages_vp;

//
// VM data structures allocated early during boot.
//
size_t pagehash_sz;
uint64_t memlist_sz;

char tbr_wr_addr_inited = 0;

static pgcnt_t npages;
static struct memlist *memlist;
void *memlist_end;

/*
 * By default the Dynamic Reconfiguration (DR) Cage is enabled for 
 * maximum OS MPSS performance.  Users needing to disable the cage 
 * mechanism can set this variable to zero via /etc/system.
 * Disabling the cage on systems supporting (DR) * will result in 
 * loss of DR functionality.  Platforms wishing to disable kernel 
 * Cage by default should do so in their set_platform_defaults() 
 * routine.
 */
int	kernel_cage_enable = 1,
	prom_debug = 1,
	physMemInit = 0;	// page_t has been initialized

int nCPU;			// Number of CPUs on this system

pgcnt_t obp_pages;		// Pages used by PROM text and data

struct regs sync_reg_buf;
uint64_t sync_tt;

memoryChunk *sysMemory;		// Physical memory layout

extern struct bootops *bop;
struct bootops *bootops;

char kern_bootargs[OBP_MAXPATHLEN];

#ifdef  DEBUGGING_MEM
static int debugging_mem = 1;
#endif

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- main.                                             */
/*                                                                  */
/* Function	- Bring up Solaris on s390x.                        */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
startup()
{
	int i;
	struct modctl *mcp = &modules;

	bootops	      = bop;
	sysMemory     = (memoryChunk *) bop->bootSysMem;
	phys_install  = pinstalledp;
	phys_avail    = pavailistp;
	phys_free     = pfreelistp;
	virt_avail    = vfreelistp;

	prom_printf("OpenSolaris on System z - Startup commenced\n");
	prom_printf("Memory size: %ldMB Chunks: %d\n",
		    (availmem+1)/(1024*1024), nMemChunk);
	for (i = 0; i < nMemChunk; i++) 
		prom_printf("%d. %016lx %08lx %ld\n",
			    i,(uint64_t) sysMemory[i].start, sysMemory[i].len, 
			    sysMemory[i].type);

	prom_printf("CPU %d trace table starts at %p\n",
		    CPU->cpu_id,CPU->cpu_m.traceTbl);
	count_cpus();
	mem_init();
	startup_modules();
	startup_vm();
	startup_end();
	(void) add_avintr(NULL, 1, dumpSys, "dump_producer",
			  S390_INTR_EXT, 0, 0, NULL, NULL);

	prom_printf("System z specific initialization complete\n");
}

/*========================= End of Function ========================*/

#ifdef  DEBUGGING_MEM
/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- printmemlist.                                     */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
printmemlist(char *title, struct memlist *list)
{
	if (!debugging_mem)
		return;

	prom_printf("%s\n", title);

	while (list) {
		prom_printf("\taddr = %p, size = 0x%016lx\n",
		    (void *) list->address, list->size);
		list = list->next;
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- printmemseg.                                      */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
printmemseg(struct memseg *memseg)
{
	if (!debugging_mem)
		return;

	prom_printf("memseg\n");

	while (memseg) {
		prom_printf("\tpage = 0x%p, epage = 0x%p, "
		    "pfn = 0x%lx, epfn = 0x%lx\n",
		    memseg->pages, memseg->epages,
		    memseg->pages_base, memseg->pages_end);
		memseg = memseg->next;
	}
}

/*========================= End of Function ========================*/

#endif

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setup_cage_params.                                */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

#if 0
static void
setup_cage_params(void)
{
	void (*func)(void);

	func = (void (*)(void))kobj_getsymvalue("set_platform_cage_params", 0);
	if (func != NULL) {
		(*func)();
		return;
	}

	if (kernel_cage_enable == 0) {
		return;
	}
	kcage_range_lock();
	if (kcage_range_init(phys_avail, 1) == 0) {
		kcage_init(total_pages / 256);
	}
	kcage_range_unlock();

	if (kcage_on) {
		cmn_err(CE_NOTE, "!Kernel Cage is ENABLED");
	} else {
		cmn_err(CE_NOTE, "!Kernel Cage is DISABLED");
	}

}
#endif

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- count_cpus.                                       */
/*                                                                  */
/* Function	- Determine how many CPUs we have on the system.    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
count_cpus(void)
{
	uint16_t	thisCPU,
			iCPU,
			cpuAddr;

	_pfxPage	*pfx = NULL;	

	nCPU    = 1;
	cpuAddr = getprocessorid();
	prom_printf("Boot CPU hardware address: %d\n",cpuAddr);

	for (iCPU = 0; iCPU < 65535; iCPU++) {
		if (iCPU != cpuAddr) {
			if (sigp(iCPU, sigp_Sense, NULL, NULL) != sigp_NotOp) {
				CPUSET_ADD(mp_cpus, iCPU);
				nCPU++;		
			} else
				break;
		}
	}
	prom_printf("%d CPUs detected\n",nCPU);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- mem_init.                                         */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
mem_init(void)
{
	size_t alloc_sz;
	size_t ctrs_sz;
	caddr_t alloc_base;
	caddr_t ctrs_base, ctrs_end;
	caddr_t memspace;
	caddr_t va;
	int 	memblocks = 0,
	  	i_Chunk;
	struct 	memlist *cur;
	size_t 	syslimit    = (size_t) SYSLIMIT,
	       	sysbase	    = (size_t) SYSBASE,
		sysSize,
	       	asSize;
	int alloc_alignsize = MMU_PAGESIZE;

	/*
	 * Initialize enough of the system to allow kmem_alloc to work by
	 * calling boot to allocate its memory until the time that
	 * kvm_init is completed.  The page structs are allocated after
	 * rounding up end to the nearest page boundary; the memsegs are
	 * initialized and the space they use comes from the kernel heap.
	 * With appropriate initialization, they can be reallocated later
	 * to a size appropriate for the machine's configuration.
	 *
	 * At this point, memory is allocated for things that will never
	 * need to be freed, this used to be "valloced".  This allows a
	 * savings as the pages don't need page structures to describe
	 * them because them will not be managed by the vm system.
	 */

	/*
	 * We're loaded by boot with the following configuration (as
	 * specified in the s390x/conf/Mapfile):
	 *
	 * 	text:		Chunk aligned on a 1MB boundary
	 * 	data & bss:	Chunk aligned on a 1MB boundary
	 *
	 * The free space in the data-bss chunk is used for nucleus
	 * allocatable data structures and we reserve it using the
	 * nalloc_base and nalloc_end variables.  This space is currently
	 * being used for hat data structures required for tlb miss
	 * handling operations.  We align nalloc_base to a l2 cache
	 * linesize because this is the line size the hardware uses to
	 * maintain cache coherency.
	 *
	 */

	moddata     = e_data;
	econtig	    = e_moddata = moddata + MODDATA;
	valloc_base = (caddr_t) roundup((uintptr_t) e_moddata, MMU_PAGESIZE);

	core_base   = (uintptr_t) COREHEAP_BASE;
	core_end    = (uintptr_t) COREHEAP_END;
	core_size   = core_end - core_base;

	PRM_DEBUG(core_base);
	PRM_DEBUG(core_end);

	/*
	 * Remember what the physically available highest page is
	 * so that dumpsys works properly, and find out how much
	 * memory is installed.
	 */
	physinstalled = physmax	= availmem / MMU_PAGESIZE;
	for (i_Chunk = 0, sysSize = 0; i_Chunk < nMemChunk; i_Chunk++) {
		sysSize += sysMemory[i_Chunk].len;
	}

	npages    = sysSize / MMU_PAGESIZE;
	obp_pages = physinstalled - npages;

	PRM_DEBUG(sysSize);
	PRM_DEBUG(physmax);
	PRM_DEBUG(physinstalled);

	memspace = (caddr_t)BOP_ALLOC(bop, (caddr_t) panicbuf,
				      PANICBUFSIZE, MMU_PAGESIZE);
	if (memspace != (caddr_t) panicbuf)
		prom_panic("Panic buffer allocation failure");

	/*
	 * Check how much virtual storage we'll support based
	 * on the amount of real storage we have. There's no 
	 * point supporting a full 64-bit space if we don't have
	 * enough real storage to house the region/segment/page
	 * tables and try and guesstimate everything else
	 */
	nalloc_sz   = RSPSize;
	nalloc_sz  += (npages * sizeof(page_t));
	nalloc_sz  += (npages / PAGE_HASHAVELEN * sizeof(struct page *));
	nalloc_sz  += (2 * sizeof(struct memlist) * npages);
	nalloc_sz  += (8 * MMU_SEGMSIZE);
	nalloc_sz   = roundup(nalloc_sz, MMU_PAGESIZE);
	nalloc_base = BOP_ALLREAL(bop, nalloc_sz, MMU_PAGESIZE);

	nalloc_end  = (caddr_t)((uintptr_t) nalloc_base + nalloc_sz);

	/*
	 * Calculate the start of the data segment.
	 */
	sdata = s_data;

	PRM_DEBUG(moddata);
	PRM_DEBUG(RSPSize);
	PRM_DEBUG(nalloc_sz);
	PRM_DEBUG(nalloc_base);
	PRM_DEBUG(nalloc_end);
	PRM_DEBUG(sdata);

	/*
	 * Remember any slop after e_text so we can give it to the modules.
	 */
	PRM_DEBUG(e_text);
	modtext    = (caddr_t)roundup((uintptr_t) e_text, MMU_PAGESIZE);
	modtext_sz = (caddr_t)roundup((uintptr_t) modtext, MMU_SEGMSIZE) -
		     modtext;
modtext_sz = 0;
	PRM_DEBUG(modtext);
	PRM_DEBUG(modtext_sz);

	/* Fill out memory nodes config structure */
	startup_build_mem_nodes();

	/*
	 * On small-memory systems (<MODTEXT_SM_SIZE MB, currently 256MB), the
	 * in-nucleus module text is capped to MODTEXT_SM_CAP bytes (currently
	 * 2MB) and any excess pages are put on physavail.  The assumption is
	 * that small-memory systems will need more pages more than they'll
	 * need efficiently-mapped module texts.
	 */
	if ((sysSize < mmu_btop(MODTEXT_SM_SIZE << PAGESHIFT)) &&
	    (modtext_sz > MODTEXT_SM_CAP)) {
		extra_etpg = mmu_btop(modtext_sz - MODTEXT_SM_CAP);
		modtext_sz = MODTEXT_SM_CAP;
	} else
		extra_etpg = 0;

	extra_etva = modtext + modtext_sz; PRM_DEBUG(extra_etpg);
	PRM_DEBUG(modtext_sz);
	PRM_DEBUG(extra_etva);

	/*
	 * initialize the nucleus memory allocator.
	 */
	ndata_alloc_init(&ndata, (uintptr_t) nalloc_base, (uintptr_t) nalloc_end);
	PRM_DEBUG(nalloc_base);
	PRM_DEBUG(nalloc_end);

	mmu_init();
	page_coloring_init();

	/*
	 * Allocate page_freelists bin headers for memnode 0 from the
	 * nucleus data area.
	 */
	if (ndata_alloc_page_freelists(&ndata, 0) != 0)
		cmn_err(CE_PANIC,
		    "no more nucleus memory after page free lists alloc");

	if (kpm_enable) {
		kpm_init();
		/*
		 * kpm page space -- Update kpm_npages and make the
		 * same assumption about fragmenting as it is done
		 * for memseg_sz.
		 */
		kpm_npages_setup(memblocks + 4);
		vpm_enable = 1;
	}

	/*
	 * On current CPUs we'll run out of physical memory address bits before
	 * we need to worry about the allocations running into anything else in
	 * VM.
	 */
	kmem64_base = (caddr_t) syslimit;
	PRM_DEBUG(kmem64_base);

	alloc_base = (caddr_t)roundup((uintptr_t) kmem64_base, alloc_alignsize);

	/*
	 * Allocate the remaining page freelists.  NUMA systems can
	 * have lots of page freelists, one per node, which quickly
	 * outgrow the amount of nucleus memory available.
	 */
	if (max_mem_nodes > 1) {
		int mnode;
		caddr_t alloc_start = alloc_base;

		for (mnode = 1; mnode < max_mem_nodes; mnode++) {
			alloc_base = (caddr_t) alloc_page_freelists(mnode, 
								    alloc_base,
								    CACHE_ALIGNSIZE);
		}

		if (alloc_base > alloc_start) {
			cmn_err(CE_PANIC,
				"Unable to alloc page freelists\n");
		}
		PRM_DEBUG(alloc_base);
	}

	/*
	 * Account for extra memory after e_text.
	 */
	npages += extra_etpg;

	/*
	 * Calculate the largest free memory chunk in the nucleus data area.
	 * We need to figure out if page structs can fit in there or not.
	 * We also make sure enough page structs get created for any physical
	 * memory we might be returning to the system.
	 */
	ndata_remain_sz = ndata_maxsize(&ndata);
	PRM_DEBUG(ndata_remain_sz);

	pp_sz = sizeof (struct page) * npages;

	/*
	 * Here's a nice bit of code based on somewhat recursive logic:
	 *
	 * If the page array would fit within the nucleus, we want to
	 * add npages to cover any extra memory we may be returning back
	 * to the system.
	 *
	 * HOWEVER, the page array is sized by calculating the size of
	 * (struct page * npages), as are the pagehash table, ctrs and
	 * memseg_list, so the very act of performing the calculation below may
	 * in fact make the array large enough that it no longer fits in the
	 * nucleus, meaning there would now be a much larger area of the
	 * nucleus free that should really be added to npages, which would
	 * make the page array that much larger, and so on.
	 *
	 * This also ignores the memory possibly used in the nucleus for the
	 * the page hash, ctrs and memseg list and the fact that whether they
	 * fit there or not varies with the npages calculation below, but we
	 * don't even factor them into the equation at this point; perhaps we
	 * should or perhaps we should just take the approach that the few
	 * extra pages we could add via this calculation REALLY aren't worth
	 * the hassle...
	 */
	if (ndata_remain_sz > pp_sz) {
		size_t spare = ndata_spare(&ndata, pp_sz, CACHE_ALIGNSIZE);

		npages += mmu_btop(spare);

		pp_sz = npages * sizeof (struct page);

		pp_base = ndata_alloc(&ndata, pp_sz, CACHE_ALIGNSIZE);
	} else {
		panic("page alloc failure - remaining: %lx pp_sz: %lx",
		      ndata_remain_sz,pp_sz);
	}

	/*
	 * If physmem is patched to be non-zero, use it instead of
	 * the monitor value unless physmem is larger than the total
	 * amount of memory on hand.
	 */
	if (physmem == 0 || physmem > npages)
		physmem = npages;

	/*
	 * The page structure hash table size is a power of 2
	 * such that the average hash chain length is PAGE_HASHAVELEN.
	 */
	page_hashsz = npages / PAGE_HASHAVELEN;
	page_hashsz = 1 << highbit((ulong_t)page_hashsz);
	pagehash_sz = sizeof (struct page *) * page_hashsz;

	/*
	 * We want to TRY to fit the page structure hash table,
	 * the page size free list counters, the memseg list and
	 * and the kpm page space in the nucleus if possible.
	 *
	 * alloc_sz counts how much memory needs to be allocated
	 */
	page_hash = ndata_alloc(&ndata, pagehash_sz, CACHE_ALIGNSIZE);

	alloc_sz = (page_hash == NULL ? pagehash_sz : 0);

	/*
	 * Size up per page size free list counters.
	 */
	ctrs_sz   = page_ctrs_sz();
	ctrs_base = ndata_alloc(&ndata, ctrs_sz, CACHE_ALIGNSIZE);

	if (ctrs_base == NULL)
		alloc_sz = roundup(alloc_sz, CACHE_ALIGNSIZE) + ctrs_sz;

	/*
	 * The memseg list is for the chunks of physical memory that
	 * will be managed by the vm system.  The number calculated is
	 * a guess as boot may fragment it more when memory allocations
	 * are made before kphysm_init().  Currently, there are two
	 * allocations before then, so we assume each causes fragmen-
	 * tation, and add a couple more for good measure.
	 */
	memseg_sz   = sizeof (struct memseg) * (memblocks + 4);
	memseg_base = ndata_alloc(&ndata, memseg_sz, CACHE_ALIGNSIZE);

	if (memseg_base == NULL)
		alloc_sz = roundup(alloc_sz, CACHE_ALIGNSIZE) + memseg_sz;

	if (kpm_enable) {
		/*
		 * kpm page space -- Update kpm_npages and make the
		 * same assumption about fragmenting as it is done
		 * for memseg_sz above.
		 */
		kpm_npages_setup(memblocks + 4);
		kpm_pp_sz = (kpm_smallpages == 0) ?
				kpm_npages * sizeof (kpm_page_t):
				kpm_npages * sizeof (kpm_spage_t);

		kpm_pp_base = (uintptr_t)ndata_alloc(&ndata, kpm_pp_sz,
		    CACHE_ALIGNSIZE);

		if (kpm_pp_base == NULL)
			alloc_sz = roundup(alloc_sz, CACHE_ALIGNSIZE) +
			    kpm_pp_sz;
	}

	/*
	 * Allocate the array that protects pp->p_selock.
	 */
	pse_shift = size_pse_array(physmem, max_ncpus);
	pse_table_size = 1 << pse_shift;
	pse_mutex = ndata_alloc(&ndata, pse_table_size * sizeof (pad_mutex_t),
				CACHE_ALIGNSIZE);
	if (pse_mutex == NULL)
		alloc_sz = roundup(alloc_sz, CACHE_ALIGNSIZE) +
		    pse_table_size * sizeof (pad_mutex_t);

	if (alloc_sz > 0) {
		uintptr_t tableBase;

		/*
		 * We need extra memory allocated 
		 */
		alloc_base = (caddr_t)roundup((uintptr_t)alloc_base,
		    alloc_alignsize);

		alloc_sz = roundup(alloc_sz, alloc_alignsize);

		tableBase = (uintptr_t) ndata_alloc(&ndata, alloc_sz, alloc_alignsize);
		if (tableBase == NULL)
			panic("system page struct alloc failure");

		alloc_base += alloc_sz;

		if (page_hash == NULL) {
			page_hash = (struct page **)tableBase;
			tableBase = roundup(tableBase + pagehash_sz,
			    CACHE_ALIGNSIZE);
		}

		if (ctrs_base == NULL) {
			ctrs_base = (caddr_t)tableBase;
			tableBase = roundup(tableBase + ctrs_sz,
			    CACHE_ALIGNSIZE);
		}

		if (memseg_base == NULL) {
			memseg_base = (struct memseg *)tableBase;
			tableBase = roundup(tableBase + memseg_sz,
			    CACHE_ALIGNSIZE);
		}

		if (kpm_enable && kpm_pp_base == NULL) {
			kpm_pp_base = (uintptr_t)tableBase;
			tableBase = roundup(tableBase + kpm_pp_sz,
			    CACHE_ALIGNSIZE);
		}

	}

	/*
	 * Initialize per page size free list counters.
	 */
	ctrs_end = page_ctrs_alloc(ctrs_base);
	ASSERT(ctrs_base + ctrs_sz >= ctrs_end);

	PRM_DEBUG(page_hash);
	PRM_DEBUG(memseg_base);
	PRM_DEBUG(kpm_pp_base);
	PRM_DEBUG(kpm_pp_sz);
	PRM_DEBUG(pp_base);
	PRM_DEBUG(pp_sz);
	PRM_DEBUG(alloc_base);

	if (kmem64_base) {
		/*
		 * Set the end of the kmem64 segment
		 */
		kmem64_end = (caddr_t)roundup((uintptr_t)alloc_base, alloc_alignsize);

		PRM_DEBUG(kmem64_base);
		PRM_DEBUG(kmem64_end);
	}

	va 	    = (caddr_t) sysbase;
	memlist_sz  = 2 * sizeof(struct memlist) * npages;
	memlist_sz  = roundup(memlist_sz, MMU_PAGESIZE);
	PRM_DEBUG(memlist_sz);
	memspace    = (caddr_t) ndata_alloc(&ndata, memlist_sz, MMU_PAGESIZE);
	if (memspace == NULL)
		panic("memspace allocation failed - size: %lx",memlist_sz);

	memlist     = (struct memlist *) memspace;
	memlist_end = (char *) memspace + memlist_sz;
	virt_avail  = memlist;

	PRM_DEBUG(memlist);
	PRM_DEBUG(memlist_end);
	PRM_DEBUG(sysbase);
	PRM_DEBUG(syslimit);

	kernelheap_init((void *) sysbase, (void *) syslimit,
	  		  (caddr_t) (sysbase + PAGESIZE), (void *) core_base, (void *) core_end);

	for (cur = virt_avail; cur->next; cur = cur->next) {
		uint64_t range_base, range_size;

		if ((range_base = cur->address + cur->size) < (uint64_t)sysbase)
			continue;
		if (range_base >= (uint64_t)syslimit)
			break;
		/*
		 * Limit the range to end at syslimit.
		 */
		range_size = MIN(cur->next->address,
		    (uint64_t)syslimit) - range_base;
		(void) vmem_xalloc(heap_arena, (size_t)range_size, PAGESIZE,
		    0, 0, (void *)range_base, (void *)(range_base + range_size),
		    VM_NOSLEEP | VM_BESTFIT | VM_PANIC);
	}

	/*
	 * Add any extra memory after e_text to the phys_avail list, as long
	 * as there's at least a page to add.
	 */
	if (extra_etpg)
		memlist_add((uint64_t) extra_etva, mmu_ptob(extra_etpg),
		    	    &memlist, &phys_avail);

	/*
	 * Add any extra memory after e_data to the phys_avail list as long
	 * as there's at least a page to add.  
	 */
	if ((nalloc_base = ndata_extra_base(&ndata, MMU_PAGESIZE)) == NULL)
		nalloc_base = nalloc_end;
	ndata_remain_sz = nalloc_end - nalloc_base;

	if (ndata_remain_sz >= MMU_PAGESIZE) 
		memlist_add((uint64_t) nalloc_base, (uint64_t) ndata_remain_sz, 
			    &memlist, &phys_free);

	PRM_DEBUG(memspace);

	if ((caddr_t)memlist > (memspace + memlist_sz))
		panic("memlist overflow");

	PRM_DEBUG(pp_base);
	PRM_DEBUG(memseg_base);
	PRM_DEBUG(npages);
	PRM_DEBUG(phys_avail);

	/*
	 * Initialize the page structures from the memory lists.
	 */
	kphysm_init(pp_base, memseg_base, npages, kpm_pp_base, kpm_npages);

	availrmem_initial = availrmem = freemem;
	PRM_DEBUG(availrmem);

	/*
	 * Some of the locks depend on page_hashsz being set!
	 * kmem_init() depends on this; so, keep it here.
	 */
	page_lock_init();

	/*
	 * Initialize kernel memory allocator.
	 */
	kmem_init();

	/*
	 * Initialize bp_mapin().
	 */
	bp_init(shm_alignment, HAT_STRICTORDER);

	claimBootPages();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- startup_modules.                                  */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
startup_modules(void)
{
	unsigned int i;

	prom_printf("startup_modules() starting...\n");
	/*
	 * Initialize ten-micro second timer so that drivers will
	 * not get short changed in their init phase. This was
	 * not getting called until clkinit which, on fast cpu's
	 * caused the drv_usecwait to be way too short.
	 */

	/*
	 * Calculate default settings of system parameters based upon
	 * maxusers, yet allow to be overridden via the /etc/system file.
	 */
	param_calc(0);

	mod_setup();

	/*
	 * Initialize system parameters.
	 */
	param_init();

	/*
	 * maxmem is the amount of physical memory we're playing with.
	 */
	maxmem = physmem;

	/*
	 * Initialize the hat layer.
	 */
	hat_init();

	/*
	 * Initialize segment management stuff.
	 */
	seg_init();

	if (modload("fs", "specfs") == -1)
		halt("Can't load specfs");

	if (modload("fs", "devfs") == -1)
		halt("Can't load devfs");

	if (modload("fs", "dev") == -1)
		halt("Can't load dev");

#if 0
	if (modloadonly("misc", "swapgeneric") == -1)
		halt("Can't load swapgeneric");

	(void) modloadonly("sys", "lbl_edition");
#endif

	dispinit();

	/* Read cluster configuration data. */
	clconf_init();

	/*
	 * Create a kernel device tree. First, create rootnex and
	 * then invoke bus specific code to probe devices.
	 */
	setup_ddi();

	prom_printf("startup_modules() done\n");
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- startup_vm.                                       */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
startup_vm(void)
{
	size_t	i;
	struct segmap_crargs a;
	struct segkpm_crargs b;

	uint64_t avmem;
	caddr_t va;
	pgcnt_t	max_phys_segkp;
	int	mnode;

	/*
	 * get prom's mappings, create hments for them and switch
	 * to the kernel context.
	 */
	hat_kern_setup();

	/*
	 * Take over trap table
	 */
	setup_trap_table();

	/*
	 * Initialize VM system, and map kernel address space.
	 */
	kvm_init();

	/*
	 * If the following is true, someone has patched
	 * phsymem to be less than the number of pages that
	 * the system actually has.  Remove pages until system
	 * memory is limited to the requested amount.  Since we
	 * have allocated page structures for all pages, we
	 * correct the amount of memory we want to remove
	 * by the size of the memory used to hold page structures
	 * for the non-used pages.
	 */
	if (physmem < npages) {
		pgcnt_t diff, off;
		struct page *pp;
		struct seg kseg;

		cmn_err(CE_WARN, "limiting physmem to %ld pages", physmem);

		off = 0;
		diff = npages - physmem;
		diff -= mmu_btopr(diff * sizeof (struct page));
		kseg.s_as = &kas;
		while (diff--) {
			pp = page_create_va(&unused_pages_vp, (offset_t)off,
			    MMU_PAGESIZE, PG_WAIT | PG_EXCL,
			    &kseg, (caddr_t)off);
			if (pp == NULL)
				cmn_err(CE_PANIC, "limited physmem too much!");
			page_io_unlock(pp);
			page_downgrade(pp);
			availrmem--;
			off += MMU_PAGESIZE;
		}
	}

	/*
	 * When printing memory, show the total as physmem less
	 * that stolen by a debugger.
	 */
	cmn_err(CE_CONT, "?mem = %ldK (0x%lx000)\n",
	    (ulong_t)(physinstalled) << (PAGESHIFT - 10),
	    (ulong_t)(physinstalled) << (PAGESHIFT - 12));

	avmem = (uint64_t)freemem << PAGESHIFT;
	cmn_err(CE_CONT, "?avail mem = %lld\n", (unsigned long long)avmem);

	if (mmu.max_page_level == 0) {
		use_brk_lpg	 = 0;
		use_stk_lpg 	 = 0;
	}

	/*
	 * Initialize the segkp segment type.  We position it
	 * after the configured tables and buffers (whose end
	 * is given by econtig) and before V_WKBASE_ADDR.
	 * Also in this area is segkmap (size SEGMAPSIZE).
	 */

	/* XXX - cache alignment? */
	va = (caddr_t)SEGKPBASE;
	ASSERT(((uintptr_t)va & PAGEOFFSET) == 0);

	max_phys_segkp = (physmem * 2);

	if (segkpsize < btop(SEGKPMINSIZE) || segkpsize > btop(SEGKPMAXSIZE)) {
		segkpsize = btop(SEGKPDEFSIZE);
		cmn_err(CE_WARN, "Illegal value for segkpsize. "
		    "segkpsize has been reset to %ld pages", segkpsize);
	}

	i = ptob(MIN(segkpsize, max_phys_segkp));

	rw_enter(&kas.a_lock, RW_WRITER);
	if (seg_attach(&kas, va, i, segkp) < 0)
		cmn_err(CE_PANIC, "startup: cannot attach segkp");
	if (segkp_create(segkp) != 0)
		cmn_err(CE_PANIC, "startup: segkp_create failed");
	rw_exit(&kas.a_lock);

	/*
	 * kpm segment
	 */
	segmap_kpm = kpm_enable &&
		segmap_kpm && PAGESIZE == MAXBSIZE;

	if (kpm_enable) {
		rw_enter(&kas.a_lock, RW_WRITER);

		/*
		 * The segkpm virtual range range is larger than the
		 * actual physical memory size and also covers gaps in
		 * the physical address range for the following reasons:
		 * . keep conversion between segkpm and physical addresses
		 *   simple, cheap and unambiguous.
		 * . avoid extension/shrink of the the segkpm in case of DR.
		 * . avoid complexity for handling of virtual addressed
		 *   caches, segkpm and the regular mapping scheme must be
		 *   kept in sync wrt. the virtual color of mapped pages.
		 * Any accesses to virtual segkpm ranges not backed by
		 * physical memory will fall through the memseg pfn hash
		 * and will be handled in segkpm_fault.
		 * Additional kpm_size spaces needed for vac alias prevention.
		 */
		if (seg_attach(&kas, kpm_vbase, kpm_size, segkpm) < 0)
			cmn_err(CE_PANIC, "cannot attach segkpm");

		b.prot	   = PROT_READ | PROT_WRITE;
		b.nvcolors = 1;

		if (segkpm_create(segkpm, (caddr_t)&b) != 0)
			panic("segkpm_create segkpm");

		rw_exit(&kas.a_lock);
	}

	/*
	 * Now create generic mapping segment.  This mapping
	 * goes SEGMAPSIZE beyond SEGMAPBASE.  But if the total
	 * virtual address is greater than the amount of free
	 * memory that is available, then we trim back the
	 * segment size to that amount
	 */
	va = (caddr_t)SEGMAPBASE;

	/*
	 * segkmap base address must be MAXBSIZE aligned
	 */
	ASSERT(((uintptr_t)va & MAXBOFFSET) == 0);

	/*
	 * Set size of segmap to percentage of freemem at boot,
	 * but stay within the allowable range
	 * Note we take percentage  before converting from pages
	 * to bytes to avoid an overflow on 32-bit kernels.
	 */
	i = mmu_ptob((freemem * segmap_percent) / 100);

	if (i < MINMAPSIZE)
		i = MINMAPSIZE;

	if (i > MIN(SEGMAPSIZE, mmu_ptob(freemem)))
		i = MIN(SEGMAPSIZE, mmu_ptob(freemem));

	i &= MAXBMASK;	/* segkmap size must be MAXBSIZE aligned */

	rw_enter(&kas.a_lock, RW_WRITER);
	if (seg_attach(&kas, va, i, segkmap) < 0)
		cmn_err(CE_PANIC, "cannot attach segkmap");

	a.prot	    = PROT_READ | PROT_WRITE;
	a.shmsize   = shm_alignment;
	a.nfreelist = 0;	/* use segmap driver defaults */

	if (segmap_create(segkmap, (caddr_t)&a) != 0)
		panic("segmap_create segkmap");
	rw_exit(&kas.a_lock);

	segdev_init();

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kpm_init.                                         */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
kpm_init()
{
	kpm_pgshft 	= MMU_PAGESHIFT;
	kpm_pgsz 	= MMU_PAGESIZE;
	kpm_pgoff 	= kpm_pgsz - 1;
	kpmp2pshft 	= kpm_pgshft - PAGESHIFT;
	kpmpnpgs 	= 1 << kpmp2pshft;
	ASSERT(((uintptr_t)kpm_vbase & (kpm_pgsz - 1)) == 0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- startup_end.                                      */
/*                                                                  */
/* Function	- Complete the startup process, discard the junk,   */
/*		  and configure the DDI stuff. 		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
startup_end()
{
	/*
	 * Initialize SMP related stuff
	 */
	smp_init();

	kern_setup1();

	/*
	 * Initialize interrupt related stuff
	 */
	cpu_intr_alloc(CPU, NINTR_THREADS);

	sclp_init(); 

	timers_init();

	mch_slih_init();

	add_avsoftintr((void *)&softlevel1_hdl, 1, softlevel1,
			"softlevel1", NULL, NULL); 

	configure();

	sets390isalist();
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- post_startup.                                     */
/*                                                                  */
/* Function	- Perform post startup activities:                  */
/*		  - Register the base CPU in the device tree	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
post_startup()
{
	/*
	 * Set the system wide, processor-specific flags to be passed
	 * to userland via the aux vector for performance hints and
	 * instruction set extensions.
	 */
	bind_hwcap();

	/*
	 * Startup the memory scrubber.
	 */
	memscrub_init();

	/*
	 * Perform forceloading tasks for /etc/system.
	 */
	(void) mod_sysctl(SYS_FORCELOAD, NULL);

	/*
	 * ON4.0: Force /proc module in until clock interrupt handle fixed
	 * ON4.0: This must be fixed or restated in /etc/systems.
	 */
	(void) modload("fs", "procfs");

	maxmem = freemem;

	add_cpunode2devtree(CPU->cpu_id);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kpm_npages_setup.                                 */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
kpm_npages_setup(int memblocks)
{
	/*
	 * npages can be scattered in a maximum of 'memblocks'
	 */
	kpm_npages = ptokpmpr(npages) + memblocks;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kobj_vmem_init.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
kobj_vmem_init(vmem_t **text_arena, vmem_t **data_arena)
{
	*text_arena = vmem_create("module_text", NULL, 0, 1,
				  segkmem_alloc, segkmem_free, heaptext_arena, 
				  0, VM_SLEEP);
	*data_arena = vmem_create("module_data", NULL, 0, 1, 
	                          segkmem_alloc, segkmem_free, heap32_arena, 
				  0, VM_SLEEP);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- memlist_add.                                      */
/*                                                                  */
/* Function	- Add to a memory list.				    */
/*                                                                  */
/*------------------------------------------------------------------*/

static void
memlist_add(uint64_t start, 		// Start of new memory segment
	    uint64_t len, 		// Length of new memory segment
	    struct memlist **memlistp,	// Array of available segment structures
	    struct memlist **curmlstp)  // Memory list which to add segment
{
	struct memlist *new;

	new 	     = *memlistp;
	new->address = start;
	new->size    = len;
	*memlistp    = new + 1;

	memlist_insert(new, curmlstp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kphysm_init.                                      */
/*                                                                  */
/* Function	- Initialize physical memory.                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
kphysm_init(page_t *pp, struct memseg *memsegp, pgcnt_t npages,
	uintptr_t kpm_pp, pgcnt_t kpm_npages)
{
	struct memlist	*pmem;
	struct memseg	*msp;
	pfn_t		 base,
			 lastseg_pages_end = 0;
	pgcnt_t		 nelem_used = 0,
			 num;
	int		 iAlloc;
	page_t		*ppBase = pp;

	ASSERT(page_hash != NULL && page_hashsz != 0);

	msp = memsegp;
	for (pmem = phys_avail; pmem && npages; pmem = pmem->next) {

		/*
		 * Build the memsegs entry
		 */
		num = btop(pmem->size);
		if (num > npages)
			num = npages;
		npages -= num;
		base = btop(pmem->address);

		msp->pages 	= pp;
		msp->epages 	= pp + num;
		msp->pages_base = base;
		msp->pages_end  = base + num;

		if (kpm_enable) {
			pfn_t pbase_a;
			pfn_t pend_a;
			pfn_t prev_pend_a;
			pgcnt_t	nelem;

			msp->pagespa     = (uint64_t) pp;
			msp->epagespa    = (uint64_t) (pp + num);
			pbase_a 	 = kpmptop(ptokpmp(base));
			pend_a 		 = kpmptop(ptokpmp(base + num - 1)) + kpmpnpgs;
			nelem 		 = ptokpmp(pend_a - pbase_a);
			msp->kpm_nkpmpgs = nelem;
			msp->kpm_pbase   = pbase_a;

			if (lastseg_pages_end) {
				/*
				 * Assume phys_avail is in ascending order
				 * of physical addresses.
				 */
				ASSERT(base + num > lastseg_pages_end);
				prev_pend_a = kpmptop(
				    ptokpmp(lastseg_pages_end - 1)) + kpmpnpgs;

				if (prev_pend_a > pbase_a) {
					/*
					 * Overlap, more than one memseg may
					 * point to the same kpm_page range.
					 */
					msp->kpm_pages =
					    (kpm_page_t *)kpm_pp - 1;
					kpm_pp = (uintptr_t)
						((kpm_page_t *)kpm_pp
						+ nelem - 1);
					nelem_used += nelem - 1;

				} else {
					msp->kpm_pages =
					    (kpm_page_t *)kpm_pp;
					kpm_pp = (uintptr_t)
						((kpm_page_t *)kpm_pp
						+ nelem);
					nelem_used += nelem;
				}

			} else {
				msp->kpm_pages = (kpm_page_t *)kpm_pp;
				kpm_pp = (uintptr_t)
					((kpm_page_t *)kpm_pp + nelem);
				nelem_used = nelem;
			}

			if (nelem_used > kpm_npages)
				panic("kphysm_init: kpm_pp overflow\n");

			msp->kpm_pagespa  = (uint64_t) msp->kpm_pages;
			lastseg_pages_end = msp->pages_end;
		}

		memseg_list_add(msp);

		/*
		 * add_physmem() initializes the PSM part of the page
		 * struct by calling the PSM back with add_physmem_cb().
		 * In addition it coalesces pages into larger pages as
		 * it initializes them.
		 */
		add_physmem(pp, num, base);
		pp += num;
		msp++;
	}

	build_pfn_hash();

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- release_bootstrap.                                */
/*                                                                  */
/* Function	- Release the storage used by the boot ramdisk.     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
release_bootstrap(void)
{
	page_t	*pp,
		*lastpp;
	pfn_t	pfn;
	void	*nAddr;
	dev_info_t *dip;

	/* unmount boot ramdisk and release kmem usage */
	kobj_boot_unmountroot();

	/*
	 * We're finished using the boot loader so free its pages.
	 */

	prom_printf("Releasing ramdisk and associated pages\n");
	dip = ddi_find_devinfo("ramdisk", -1, 0);
	ASSERT(dip && ddi_get_parent(dip) == ddi_root_node());
	ndi_rele_devi(dip);	/* held from ddi_find_devinfo */
	(void) ddi_remove_child(dip, 0);

	for (nAddr = _ramdk ; nAddr < _eramd; nAddr += MMU_PAGESIZE) {
		pp = page_find(&kvp, (u_offset_t)(uintptr_t) nAddr);
		if (pp == NULL)
			panic("release_bootstrap: pp is NULL");
		page_free(pp, 1);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- add_physmem_cb.                                   */
/*                                                                  */
/* Function	- Initialize the platform-specific parts of a       */
/*		  page_t structure.                            	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
add_physmem_cb(page_t *pp, pfn_t pnum)
{
	pp->p_pagenum = pnum;
	pp->p_mapping = NULL;
	pp->p_embed   = 0;
	pp->p_share   = 0;
	pp->p_mlentry = 0;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- claimBootPages.                                   */
/*                                                                  */
/* Function	- Claim pages that were allocated by bop_xxxx       */
/*		  and place in the pfn hash.                   	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
claimBootPages()
{
	earlyAlloc *iPage;
	int 	iAlloc;
	page_t 	*pp;
	pfn_t	pfn;
	void 	*nAddr,
		*eAddr;

	/*
	 * Claim all those pages allocated before the pfn hash was created
	 */
	for (iPage = primalPages; iPage != NULL; iPage = iPage[0].addr) {
		for (iAlloc = 1; iAlloc < iPage[0].size; iAlloc++) {
			nAddr = iPage[iAlloc].addr,
			eAddr = nAddr + iPage[iAlloc].size;
			for ( ; nAddr < eAddr; nAddr += MMU_PAGESIZE) {
				pp = page_find(&kvp, (u_offset_t)(uintptr_t) nAddr);
				if (pp == NULL) {
					pfn = va_to_pfn(nAddr);
					pp = page_numtopp(pfn, SE_EXCL);
					if (pp == NULL) 
						panic("claimBootPages: pp is NULL!");
							
					PP_CLRFREE(pp);
					page_hashin(pp, &kvp, (u_offset_t)(uintptr_t) nAddr, NULL);
					page_unlock(pp);
				}
			}
		}
	}

	/*
	 * Remove the RAMDISK pages from the list until
	 * we can reinsert them in release_bootstrap
	 */
	for (nAddr = _ramdk ; nAddr < _eramd; nAddr += MMU_PAGESIZE) {
		pp = page_find(&kvp, (u_offset_t)(uintptr_t) nAddr);
		if (pp == NULL) {
			pfn = va_to_pfn(nAddr);
			pp = page_numtopp(pfn, SE_EXCL);
			if (pp == NULL) 
				panic("claimBootPages: pp is NULL for %lx",pfn);
					
			PP_CLRFREE(pp);
			page_hashin(pp, &kvp, (u_offset_t)(uintptr_t) nAddr, NULL);
		}
	}
	physMemInit = 1;	// page_t has been initialized
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- memseg_list_add.                                  */
/*                                                                  */
/* Function	- In the case of architectures that support dynamic */
/*		  addition of memory at run-time there are two 	    */
/*		  cases where memsegs need to be initialized and    */
/*		  added to the memseg list:			    */
/* 			1. memsegs that are constructed at startup  */
/* 			2. memsegs that are constructed at run-time */
/*			   on hot-plug capable architectures	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
memseg_list_add(struct memseg *memsegp)
{
	struct memseg **prev_memsegp;
	pgcnt_t num;

	/* insert in memseg list, decreasing number of pages order */

	num = MSEG_NPAGES(memsegp);

	for (prev_memsegp = &memsegs; *prev_memsegp;
	    prev_memsegp = &((*prev_memsegp)->next)) {
		if (num > MSEG_NPAGES(*prev_memsegp))
			break;
	}

	memsegp->next = *prev_memsegp;
	*prev_memsegp = memsegp;

	if (kpm_enable) {
		memsegp->nextpa = (uint64_t) 
				  ((memsegp->next) ? memsegp->next 
						   : MSEG_NULLPTR_PA);

		if (prev_memsegp != &memsegs) {
			struct memseg *msp;
			msp = (struct memseg *)((caddr_t)prev_memsegp -
				offsetof(struct memseg, next));
			msp->nextpa = va_to_pa(memsegp);
		} else {
			memsegspa = va_to_pa(memsegs);
		}
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kvm_init.                                         */
/*                                                                  */
/* Function	- Kernel VM initialization.                         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
kvm_init(void)
{
	/*
	 * Put the kernel segments in kernel address space.
	 */
	rw_enter(&kas.a_lock, RW_WRITER);
	as_avlinit(&kas);

	(void) seg_attach(&kas, (caddr_t)KERNELBASE,
	    (size_t)(e_moddata - KERNELBASE), &ktextseg);
	(void) segkmem_create(&ktextseg);

	(void) seg_attach(&kas, (caddr_t)valloc_base,
	    (size_t)(nalloc_end - valloc_base), &kvalloc);
	(void) segkmem_create(&kvalloc);

	if (kmem64_base) {
	    (void) seg_attach(&kas, (caddr_t)kmem64_base,
		(size_t)(kmem64_end - kmem64_base), &kmem64);
	    (void) segkmem_create(&kmem64);
	}

	(void) seg_attach(&kas, kernelheap, ekernelheap - kernelheap, &kvseg);
	(void) segkmem_create(&kvseg);

	seg_attach(&kas, (caddr_t)core_base, core_size, &kvseg_core);
	segkmem_create(&kvseg_core);

	/*
	 * 32-bit kernel segment
	 */
	(void) seg_attach(&kas, (caddr_t)SYSBASE32, SYSLIMIT32 - SYSBASE32,
			  &kvseg32);
	(void) segkmem_create(&kvseg32);

	/*
	 * Create a segment for the debugger.
	 */
	(void) seg_attach(&kas, (caddr_t)SEGDEBUGBASE, (size_t)SEGDEBUGSIZE,
	    &kdebugseg);
	(void) segkmem_create(&kdebugseg);

	rw_exit(&kas.a_lock);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- memlist_init.                                     */
/*                                                                  */
/* Function	- Initialize a memlist with the 1st entry.          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static struct memlist *
memlist_init(struct memlist **list, void *start, size_t len)
{			
	struct memlist *entry;

	entry		= *list;
	entry->address	= (uintptr_t) start;
	entry->size	= len;
	entry->next	= NULL;
	entry->prev	= NULL;
	*list		= entry + 1;
	return (entry);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sync_handler.                                     */
/*                                                                  */
/* Function	- Handle a sync panic.                              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
sync_handler(void)
{
	nopanicdebug = 1; /* do not perform debug_enter() prior to dump */
	panic("sync initiated");
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kobj_texthole_free.                               */
/*                                                                  */
/* Function	- Free storage occupied by addr.                    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

caddr_t
kobj_text_alloc(vmem_t *arena, size_t size)
{
	return (vmem_alloc(arena, size, VM_SLEEP | VM_BESTFIT));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kobj_texthole_free.                               */
/*                                                                  */
/* Function	- Free storage occupied by addr.                    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
kobj_texthole_free(caddr_t addr, size_t size)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setup_trap_table.                                 */
/*                                                                  */
/* Function	- Establish a trap table for the system.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
setup_trap_table()
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sets390isalist.                                   */
/*                                                                  */
/* Function	- Set up/emulate ISALIST for s390. This is called   */
/*		  just after configure() in startup().		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
sets390isalist(void)
{
	char *tp;
	size_t len;

	tp = kmem_alloc(TBUFSIZE, KM_SLEEP);
	*tp = '\0';

	/* If we're here, s390x/64-bit support is assumed */
	(void) strcpy(tp, "s390x ");

/* TODO: Add capability detection, for now assume z9 required/available */
	(void) strcat(tp, "z9 ");

	/* We can also run 32-bit binaries */
	(void) strcat(tp, "s390");
	len = strlen(tp) + 1;   /* account for NULL at end of string */
	isa_list = strcpy(kmem_alloc(len, KM_SLEEP), tp);
	kmem_free(tp, TBUFSIZE);
}

/*========================= End of Function ========================*/
