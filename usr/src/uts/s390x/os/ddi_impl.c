/*------------------------------------------------------------------*/
/* 								    */
/* Name        - ddi_impl.c					    */
/* 								    */
/* Function    - Architecture specific DDI implementation support   */
/**		 routines.					    */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - July, 2006  					    */
/* 								    */
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
/*                   L I C E N S E                                  */
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

/*------------------------------------------------------------------*/
/*                 D e f i n e s                                    */
/*------------------------------------------------------------------*/


/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/asm_linkage.h>

/*
 * s390x specific DDI implementation
 */
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/cpuvar.h>
#include <sys/machsystm.h>
#include <sys/sunndi.h>
#include <sys/sysmacros.h>
#include <sys/ontrap.h>
#include <vm/seg_kmem.h>
#include <sys/dditypes.h>
#include <sys/ndifm.h>
#include <sys/fm/io/ddi.h>
#include <sys/bootconf.h>
#include <sys/conf.h>
#include <sys/ethernet.h>
#include <sys/promif.h>
#include <sys/systeminfo.h>
#include <sys/vm.h>
#include <sys/fs/dv_node.h>
#include <sys/fs/snode.h>
#include <sys/ddi_isa.h>
#include <sys/mach_intr.h>
#include <sys/ramdisk.h>
#include <sys/ios390x.h>
#include <sys/devinit.h>
#include <vm/vm_dep.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

struct prop_ispec {
	uint_t	pri, vec;
};

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern void i_ddi_init_root();

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static void impl_bus_probe(dev_info_t *, int);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

vmem_t *dma31_arena;
vmem_t *dma64_arena;

static uintptr_t impl_acc_hdl_id = 0;

static struct bus_probe {
	struct bus_probe *next;
	void (*probe)(dev_info_t *, int);
} *bus_probes;

dev_info_t	*ccw_dip;

/*====================== End of Global Variables ===================*/

/*
 * SECTION: DDI Node Configuration
 */

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- get_prop_int_array.                               */
/*                                                                  */
/* Function	- Retrieve an integer array from a named proposit-  */
/*		  ion.						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
get_prop_int_array(dev_info_t *di, char *pname, int **pval, uint_t *plen)
{
	int ret;

	if ((ret = ddi_prop_lookup_int_array(DDI_DEV_T_ANY, di,
					     DDI_PROP_DONTPASS, 
					     pname, pval, plen))
			== DDI_PROP_SUCCESS) {
		*plen = (*plen) * (sizeof (int));
	}
	return (ret);
}

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- make_ddi_ppd.                                     */
/*                                                                  */
/* Function	- Create a ddi_parent_private_data structure from   */
/*		  the ddi properties of the dev_info node.	    */
/*		                               		 	    */
/* 		  The "reg" is required if the driver wishes to     */
/*		  create mappings on behalf of the device. The 	    */
/*		  "reg" property is assumed to be a list of at 	    */
/*		  least one triplet:				    */
/*		                               		 	    */
/*		  <bustype, address, size> * 1			    */
/*		                               		 	    */
/* 		  The "ranges" property describes the mapping of    */
/*		  child addresses to parent addresses.		    */
/*		                               		 	    */
/* 		  N.B. struct rangespec is defined for the 	    */
/*		  following default values:			    */
/*					parent  child		    */
/*		  #address-cells	2	2		    */
/*		  #size-cells		1	1		    */
/*		                               		 	    */
/* 		  This function doesn't deal with non-default cells */
/*		  and will not create ranges in such cases.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
make_ddi_ppd(dev_info_t *child, struct ddi_parent_private_data **ppd)
{
	struct ddi_parent_private_data *pdptr;
	int n;
	int *reg_prop, *rng_prop;
	uint_t reg_len, rng_len;

	*ppd = pdptr = kmem_zalloc(sizeof (*pdptr), KM_SLEEP);

	/*
	 * Handle the 'reg' property.
	 */
	if ((get_prop_int_array(child, "reg", &reg_prop, &reg_len) ==
	    DDI_PROP_SUCCESS) && (reg_len != 0)) {
		pdptr->par_nreg = reg_len / (int)sizeof (struct regspec);
		pdptr->par_reg = (struct regspec *)reg_prop;
	}

	/*
	 * See if I have a range
	 */
	if (get_prop_int_array(child, "ranges", &rng_prop, &rng_len)
	    == DDI_PROP_SUCCESS) {
		pdptr->par_nrng = rng_len / (int)(sizeof (struct rangespec));
		pdptr->par_rng = (struct rangespec *)rng_prop;
	}

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_setup_ddi.                                   */
/*                                                                  */
/* Function	- Probe the I/O environment of the machine.         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
impl_setup_ddi(void)
{
	dev_info_t *xdip;
	rd_existing_t rd_mem_prop;
	void 	*ramdisk_start,
		*ramdisk_end;
	int err;

	ndi_devi_alloc_sleep(ddi_root_node(), "ramdisk",
	    (pnode_t)DEVI_SID_NODEID, &xdip);

	(void) BOP_GETPROP(bootops, "ramdisk_start", &ramdisk_start);
	(void) BOP_GETPROP(bootops, "ramdisk_end", &ramdisk_end);

	rd_mem_prop.phys = (uintptr_t) ramdisk_start;
	rd_mem_prop.size = (uintptr_t) (ramdisk_end - ramdisk_start + 1);

	(void) ndi_prop_update_byte_array(DDI_DEV_T_NONE, xdip,
	    RD_EXISTING_PROP_NAME, (uchar_t *)&rd_mem_prop,
	    sizeof (rd_mem_prop));
	err = ndi_devi_bind_driver(xdip, 0);
	ASSERT(err == 0);

	ndi_devi_alloc(ddi_root_node(), "ccw",
	    	       (pnode_t) DEVI_SID_NODEID, &ccw_dip);
	ndi_prop_update_string(DDI_DEV_T_NONE, ccw_dip, "device_type", "ccw");
	ndi_prop_update_string(DDI_DEV_T_NONE, ccw_dip, "bus-type", "ccw");

	/* do bus dependent probes. */
	impl_bus_probe(ccw_dip, 0);

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_free_ddi_ppd.                                */
/*                                                                  */
/* Function	- Free ddi_parent_private_structure.                */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
impl_free_ddi_ppd(dev_info_t *dip)
{
	struct ddi_parent_private_data *pdptr = ddi_get_parent_data(dip);

	if (pdptr == NULL)
		return;

	if (pdptr->par_nrng != 0)
		ddi_prop_free((void *)pdptr->par_rng);

	if (pdptr->par_nreg != 0)
		ddi_prop_free((void *)pdptr->par_reg);

	kmem_free(pdptr, sizeof (*pdptr));
	ddi_set_parent_data(dip, NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_sunbus_name_child.                           */
/*                                                                  */
/* Function	- Name a child of sun busses based on the reg spec  */
/*		                               		 	    */
/*		  Handles the following properties:		    */
/*		                               		 	    */
/*		  Property   Value Type        		 	    */
/*		  Name                         		 	    */
/*		  --------   ----- ----        		 	    */
/*		  reg        register spec     		 	    */
/*		  interrupts new (bus-oriented) interrupt spec	    */
/*		  ranges     range spec        		 	    */
/*		                               		 	    */
/*		  This may be called multiple times, independent of */
/*		  initchild calls.             		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
impl_sunbus_name_child(dev_info_t *child, char *name, int namelen)
{
	struct ddi_parent_private_data *pdptr;
	struct regspec *rp;

	/*
	 * Fill in parent-private data and this function returns to us
	 * an indication if it used "registers" to fill in the data.
	 */
	if (ddi_get_parent_data(child) == NULL) {
		make_ddi_ppd(child, &pdptr);
		ddi_set_parent_data(child, pdptr);
	}

	/*
	 * No reg property, return null string as address
	 * (e.g. root node)
	 */
	name[0] = '\0';
	if (sparc_pd_getnreg(child) == 0) {
		return (DDI_SUCCESS);
	}

	rp = sparc_pd_getreg(child, 0);
	(void) snprintf(name, namelen, "%x,%x",
	    rp->regspec_bustype, rp->regspec_addr);
	return (DDI_SUCCESS);
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_ddi_sunbus_initchild.                        */
/*                                                                  */
/* Function	- Called from the bus_ctl op of some drivers to     */
/*		  implement the DDI_CTLOPS_INITCHILD operation.     */
/*		                               		 	    */
/*		  NEW drivers should not use this function, but     */
/*		  should declare their own initchild/uninitchild    */
/*		  handlers. (This function assumes the layout of    */
/*		  of the parent private data and that #address-     */
/*		  cells and #size-cells of the parent bus are       */
/*		  defined to be default values.)	 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
impl_ddi_sunbus_initchild(dev_info_t *child)
{
	char name[MAXNAMELEN];

	(void) impl_sunbus_name_child(child, name, MAXNAMELEN);
	ddi_set_name_addr(child, name);

	/*
	 * Try to merge .conf node. If successful, return failure to
	 * remove this child.
	 */
	if ((ndi_dev_is_persistent_node(child) == 0) &&
	    (ndi_merge_node(child, impl_sunbus_name_child) == DDI_SUCCESS)) {
		impl_ddi_sunbus_removechild(child);
		return (DDI_FAILURE);
	}
	return (DDI_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_ddi_sunbus_removechild.                      */
/*                                                                  */
/* Function	- A better name for this function would be          */
/*		  impl_ddi_sunbus_uninitchild(). 	 	    */
/*		                               		 	    */
/*		  It does not remove the child, it uninitializes it,*/
/*		  reclaiming the resources taken by initchild.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
impl_ddi_sunbus_removechild(dev_info_t *dip)
{
	impl_free_ddi_ppd(dip);
	ddi_set_name_addr(dip, NULL);
	/*
	 * Strip the node to properly convert it back to prototype form
	 */
	impl_rem_dev_props(dip);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_bus_add_probe.                               */
/*                                                                  */
/* Function	- Add a device configuration probe to the list.     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
impl_bus_add_probe(void (*func)(dev_info_t *, int))
{
	struct bus_probe *probe;

	probe	     = kmem_alloc(sizeof (*probe), KM_SLEEP);
	probe->next  = bus_probes;
	probe->probe = func;
	bus_probes   = probe;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_bus_delete_probe.                            */
/*                                                                  */
/* Function	- Delete a device configuration probe from the list.*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
impl_bus_delete_probe(void (*func)(dev_info_t *, int))
{
	struct bus_probe *prev = NULL;
	struct bus_probe *probe = bus_probes;

	while (probe) {
		if (probe->probe == func)
			break;
		prev = probe;
		probe = probe->next;
	}

	if (probe == NULL)
		return;

	if (prev)
		prev->next = probe->next;
	else
		bus_probes = probe->next;

	kmem_free(probe, sizeof (struct bus_probe));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_bus_probe.                            	    */
/*                                                                  */
/* Function	- Modload the device autoconfigurator(s) and get    */
/*		  it(them) to set up the device tree.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
impl_bus_probe(dev_info_t *rdip, int type)
{
	struct bus_probe *probe;

	if (type == 0) {
		/* load modules to install bus probes */
		}

	probe = bus_probes;
	while (probe) {
		/* run the probe function */
		(*probe->probe)(rdip, type);
		probe = probe->next;
	}
}

/*========================= End of Function ========================*/

/*
 * SECTION: DDI Interrupt
 */

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- softlevel1.                                       */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
uint_t
softlevel1(caddr_t arg1, caddr_t arg2)
{
	softint();
	return (1);
}

/*========================= End of Function ========================*/

/*
 * Wrapper functions used by New DDI interrupt framework.
 */

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_intr_ops.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
i_ddi_intr_ops(dev_info_t *dip, dev_info_t *rdip, ddi_intr_op_t op,
    ddi_intr_handle_impl_t *hdlp, void *result)
{
	dev_info_t	*pdip = ddi_get_parent(dip);
	int		ret = DDI_FAILURE;

	/*
	 * The following check is required to address
	 * one of the test case of ADDI test suite.
	 */
	if (pdip == NULL)
		return (DDI_FAILURE);
	
	if (NEXUS_HAS_INTR_OP(pdip)) {
		ret = (*(DEVI(pdip)->devi_ops->devo_bus_ops->
		    bus_intr_op)) (pdip, rdip, op, hdlp, result);
	} else {
		cmn_err(CE_WARN, "Failed to process interrupt "
		    "for %s%d due to down-rev nexus driver %s%d",
		    ddi_get_name(rdip), ddi_get_instance(rdip),
		    ddi_get_name(pdip), ddi_get_instance(pdip));
	}

	return (ret);
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_add_softint.                                */
/*                                                                  */
/* Function	- Allocate and add a soft interrupt to the system.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
i_ddi_add_softint(ddi_softint_hdl_impl_t *hdlp)
{
	int ret;

	/* add soft interrupt handler */
	ret = add_avsoftintr((void *)hdlp, hdlp->ih_pri, hdlp->ih_cb_func,
			     DEVI(hdlp->ih_dip)->devi_name, 
			     hdlp->ih_cb_arg1, hdlp->ih_cb_arg2);
	return (ret ? DDI_SUCCESS : DDI_FAILURE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_remove_softint.                             */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
i_ddi_remove_softint(ddi_softint_hdl_impl_t *hdlp)
{
	(void) rem_avsoftintr((void *)hdlp, hdlp->ih_pri, hdlp->ih_cb_func);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_trigger_softint.                            */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
i_ddi_trigger_softint(ddi_softint_hdl_impl_t *hdlp, void *arg2)
{
	uint_t		intr_id;
	int		ret;

	ASSERT(hdlp != NULL);
	ASSERT(hdlp->ih_private != NULL);

	if (av_check_softint_pending(hdlp->ih_pending, B_FALSE))
		return (DDI_EPENDING);

	update_avsoftintr_args((void *)hdlp, hdlp->ih_pri, arg2);

	av_set_softint_pending(hdlp->ih_pri, hdlp->ih_pending);
	ret = 0;

	return (ret);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_set_softint_pri.                            */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
int
i_ddi_set_softint_pri(ddi_softint_hdl_impl_t *hdlp, uint_t old_pri)
{
	int ret;

	/*
	 * If a softint is pending at the old priority then fail the request.
	 */
	if (av_check_softint_pending(hdlp->ih_pending, B_TRUE))
		return (DDI_FAILURE);

	ret = av_softint_movepri((void *)hdlp, old_pri);
	return (ret ? DDI_SUCCESS : DDI_FAILURE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_alloc_intr_phdl.                            */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
i_ddi_alloc_intr_phdl(ddi_intr_handle_impl_t *hdlp)
{
	hdlp->ih_private = (void *)kmem_zalloc(sizeof (ihdl_plat_t), KM_SLEEP);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_free_intr_phdl.                             */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
i_ddi_free_intr_phdl(ddi_intr_handle_impl_t *hdlp)
{
	kmem_free(hdlp->ih_private, sizeof (ihdl_plat_t));
	hdlp->ih_private = NULL;
}

/*========================= End of Function ========================*/

/*
 * SECTION: DDI Memory/DMA
 */

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- page_create_dma31.                                */
/*                                                                  */
/* Function	- Page allocator that uses page_create_contig() to  */
/*		  guarantee we get below the line real memory.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static page_t *
page_create_dma31(void *addr, size_t size, int vmflag, void *arg)
{
	static struct seg kseg;
	int pgflags;
	struct vnode *vp = arg;
	page_t *p;

	if (vp == NULL) {
		vp = &kvp;
	}

	kseg.s_as = &kas;
	pgflags = PG_EXCL;

	if (segkmem_reloc == 0 || (vmflag & VM_NORELOC)) {
		pgflags |= PG_NORELOC;
	}

	if (!(vmflag & VM_NOSLEEP)) {
		pgflags |= PG_WAIT;
	}

	if (vmflag & VM_PANIC) {
		pgflags |= PG_PANIC;
	}

	if (vmflag & VM_PUSHPAGE) {
		pgflags |= PG_PUSHPAGE;
	}

	return page_create_contig(vp,
				  (u_offset_t)(uintptr_t)addr,
				  size,
				  pgflags,
				  &kas,
				  0,
				  0x80000000 / MMU_PAGESIZE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- segkmem_alloc_dma31.                              */
/*                                                                  */
/* Function	- Allocate from the DMA arena.		            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void *
segkmem_alloc_dma31(vmem_t *vmp, size_t size, int flag)
{
	return segkmem_xalloc(vmp,
			      NULL,
			      size,
			      flag | VM_NORELOC,
			      0,
			      page_create_dma31,
			      &kvp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- page_create_dma64.                                */
/*                                                                  */
/* Function	- Page allocator that uses page_create_contig() to  */
/*		  guarantee we get contiguous real memory.          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static page_t *
page_create_dma64(void *addr, size_t size, int vmflag, void *arg)
{
	static struct seg kseg;
	int pgflags;
	struct vnode *vp = arg;
	page_t *p;

	if (vp == NULL) {
		vp = &kvp;
	}

	kseg.s_as = &kas;
	pgflags = PG_EXCL;

	if (segkmem_reloc == 0 || (vmflag & VM_NORELOC)) {
		pgflags |= PG_NORELOC;
	}

	if (!(vmflag & VM_NOSLEEP)) {
		pgflags |= PG_WAIT;
	}

	if (vmflag & VM_PANIC) {
		pgflags |= PG_PANIC;
	}

	if (vmflag & VM_PUSHPAGE) {
		pgflags |= PG_PUSHPAGE;
	}

	return page_create_contig(vp,
				  (u_offset_t)(uintptr_t)addr,
				  size,
				  pgflags,
				  &kas,
				  0,
				  0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- segkmem_alloc_dma64.                              */
/*                                                                  */
/* Function	- Allocate from the DMA arena.	                    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void *
segkmem_alloc_dma64(vmem_t *vmp, size_t size, int flag)
{
	return segkmem_xalloc(vmp,
			      NULL,
			      size,
			      flag | VM_NORELOC,
			      0,
			      page_create_dma64,
			      &kvp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ka_init.                                          */
/*                                                                  */
/* Function	- Create arenas needed by DDI infrastructure.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
ka_init(void)
{
	dma31_arena = vmem_create("dma31 arena", NULL, 0, 8,
	    segkmem_alloc_dma31, segkmem_free, heap_arena, 0, VM_SLEEP);
	dma64_arena = vmem_create("dma64 arena", NULL, 0, 8,
	    segkmem_alloc_dma64, segkmem_free, heap_arena, 0, VM_SLEEP);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kalloca.                                          */
/*                                                                  */
/* Function	- Allocate from the system, aligned on a specific   */
/*		  boundary. The alignment, if non-zero, must be a   */
/*		  power of 2.                  		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void *
kalloca(size_t size, size_t align, int cansleep)
{
	size_t *addr, *raddr, rsize;
	size_t hdrsize = 2 * sizeof (size_t);	/* must be power of 2 */

	align = MAX(align, hdrsize);
	ASSERT((align & (align - 1)) == 0);

	/*
	 * We need to allocate
	 *    rsize = size + hdrsize + align - MIN(hdrsize, buffer_alignment)
	 * bytes to be sure we have enough freedom to satisfy the request.
	 * Since the buffer alignment depends on the request size, this is
	 * not straightforward to use directly.
	 *
	 * kmem guarantees that any allocation of a 64-byte multiple will be
	 * 64-byte aligned.  Since rounding up the request could add more
	 * than we save, we compute the size with and without alignment, and
	 * use the smaller of the two.
	 */
	rsize = size + hdrsize + align;

	raddr = vmem_alloc(dma64_arena, rsize,
		    cansleep ? VM_SLEEP : VM_NOSLEEP);

	if (raddr == NULL)
		return (NULL);

	addr = (size_t *)P2ROUNDUP((uintptr_t)raddr + hdrsize, align);
	ASSERT((uintptr_t)addr + size - (uintptr_t)raddr <= rsize);

	addr[-2] = (size_t)raddr;
	addr[-1] = rsize;

	return (addr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kfreea.                                           */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
kfreea(void *addr)
{
	size_t *saddr = addr;

	vmem_free(dma64_arena, (void *)saddr[-2], saddr[-1]);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		-                                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
i_ddi_mem_alloc(dev_info_t *dip, ddi_dma_attr_t *attr,
    size_t length, int cansleep, int streaming,
    ddi_device_acc_attr_t *accattrp,
    caddr_t *kaddrp, size_t *real_length, ddi_acc_hdl_t *handlep)
{
	caddr_t a;
	int iomin, align;

#if defined(lint)
	*handlep = *handlep;
#endif

	/*
	 * Check legality of arguments
	 */
	if (length == 0 || kaddrp == NULL || attr == NULL) {
		return (DDI_FAILURE);
	}
	if (attr->dma_attr_minxfer == 0 || attr->dma_attr_align == 0 ||
	    (attr->dma_attr_align & (attr->dma_attr_align - 1)) ||
	    (attr->dma_attr_minxfer & (attr->dma_attr_minxfer - 1))) {
		return (DDI_FAILURE);
	}

	/*
	 * Drivers for 64-bit capable SBus devices will encode
	 * the burtsizes for 64-bit xfers in the upper 16-bits.
	 * For DMA alignment, we use the most restrictive
	 * alignment of 32-bit and 64-bit xfers.
	 */
	iomin = (attr->dma_attr_burstsizes & 0xffff) |
	    ((attr->dma_attr_burstsizes >> 16) & 0xffff);
	/*
	 * If a driver set burtsizes to 0, we give him byte alignment.
	 * Otherwise align at the burtsizes boundary.
	 */
	if (iomin == 0)
		iomin = 1;
	else
		iomin = 1 << (ddi_fls(iomin) - 1);
	iomin = maxbit(iomin, attr->dma_attr_minxfer);
	iomin = maxbit(iomin, attr->dma_attr_align);
	iomin = ddi_iomin(dip, iomin, streaming);
	if (iomin == 0)
		return (DDI_FAILURE);

	ASSERT((iomin & (iomin - 1)) == 0);
	ASSERT(iomin >= attr->dma_attr_minxfer);
	ASSERT(iomin >= attr->dma_attr_align);

	length = P2ROUNDUP(length, iomin);
	align = iomin;

	a = kalloca(length, align, cansleep);
	if ((*kaddrp = a) == 0) {
		return (DDI_FAILURE);
	} else {
		if (real_length) {
			*real_length = length;
		}
		if (handlep) {
			/*
			 * assign handle information
			 */
			impl_acc_hdl_init(handlep);
		}
		return (DDI_SUCCESS);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_mem_alloc_lim.                              */
/*                                                                  */
/* Function	- Convert old DMA limits structure to DMA attribute */
/*		  structure and continue.      		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
i_ddi_mem_alloc_lim(dev_info_t *dip, ddi_dma_lim_t *limits,
    size_t length, int cansleep, int streaming,
    ddi_device_acc_attr_t *accattrp, caddr_t *kaddrp,
    uint_t *real_length, ddi_acc_hdl_t *ap)
{
	ddi_dma_attr_t dma_attr, *attrp;
	size_t rlen;
	int ret;

	ASSERT(limits);
	attrp = &dma_attr;
	attrp->dma_attr_version = DMA_ATTR_V0;
	attrp->dma_attr_addr_lo = (uint64_t)limits->dlim_addr_lo;
	attrp->dma_attr_addr_hi = (uint64_t)limits->dlim_addr_hi;
	attrp->dma_attr_count_max = (uint64_t)-1;
	attrp->dma_attr_align = 1;
	attrp->dma_attr_burstsizes = (uint_t)limits->dlim_burstsizes;
	attrp->dma_attr_minxfer = (uint32_t)limits->dlim_minxfer;
	attrp->dma_attr_maxxfer = (uint64_t)-1;
	attrp->dma_attr_seg = (uint64_t)limits->dlim_cntr_max;
	attrp->dma_attr_sgllen = 1;
	attrp->dma_attr_granular = 1;
	attrp->dma_attr_flags = 0;

	ret = i_ddi_mem_alloc(dip, attrp, length, cansleep, streaming,
	    accattrp, kaddrp, &rlen, ap);
	if (ret == DDI_SUCCESS) {
		if (real_length)
			*real_length = (uint_t)rlen;
	}
	return (ret);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_mem_free.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
void
i_ddi_mem_free(caddr_t kaddr, ddi_acc_hdl_t *ap)
{
	kfreea(kaddr);
}

/*========================= End of Function ========================*/

/*
 * SECTION: DDI Data Access
 */

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_acc_hdL_t.                                   */
/*                                                                  */
/* Function	- Access handle allocator.                          */
/*		                               		 	    */
/*------------------------------------------------------------------*/

ddi_acc_hdl_t *
impl_acc_hdl_get(ddi_acc_handle_t hdl)
{
	/*
	 * Extract the access handle address from the DDI implemented
	 * access handle
	 */
	return (&((ddi_acc_impl_t *)hdl)->ahi_common);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_acc_hdl_alloc.                               */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

ddi_acc_handle_t
impl_acc_hdl_alloc(int (*waitfp)(caddr_t), caddr_t arg)
{
	ddi_acc_impl_t *hp;
	on_trap_data_t *otp;
	int sleepflag;

	sleepflag = ((waitfp == (int (*)())KM_SLEEP) ? KM_SLEEP : KM_NOSLEEP);

	/*
	 * Allocate and initialize the data access handle and error status.
	 */
	if ((hp = kmem_zalloc(sizeof (ddi_acc_impl_t), sleepflag)) == NULL)
		goto fail;
	if ((hp->ahi_err = (ndi_err_t *)kmem_zalloc(
	    sizeof (ndi_err_t), sleepflag)) == NULL) {
		kmem_free(hp, sizeof (ddi_acc_impl_t));
		goto fail;
	}
	if ((otp = (on_trap_data_t *)kmem_zalloc(
	    sizeof (on_trap_data_t), sleepflag)) == NULL) {
		kmem_free(hp->ahi_err, sizeof (ndi_err_t));
		kmem_free(hp, sizeof (ddi_acc_impl_t));
		goto fail;
	}
	hp->ahi_err->err_ontrap = otp;
	hp->ahi_common.ah_platform_private = (void *)hp;

	return ((ddi_acc_handle_t)hp);
fail:
	if ((waitfp != (int (*)())KM_SLEEP) &&
	    (waitfp != (int (*)())KM_NOSLEEP))
		ddi_set_callback(waitfp, arg, &impl_acc_hdl_id);
	return (NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_acc_hdl_free.                                */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
impl_acc_hdl_free(ddi_acc_handle_t handle)
{
	ddi_acc_impl_t *hp;

	/*
	 * The supplied (ddi_acc_handle_t) is actually a (ddi_acc_impl_t *),
	 * because that's what we allocated in impl_acc_hdl_alloc() above.
	 */
	hp = (ddi_acc_impl_t *)handle;
	if (hp) {
		kmem_free(hp->ahi_err->err_ontrap, sizeof (on_trap_data_t));
		kmem_free(hp->ahi_err, sizeof (ndi_err_t));
		kmem_free(hp, sizeof (ddi_acc_impl_t));
		if (impl_acc_hdl_id)
			ddi_run_callback(&impl_acc_hdl_id);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_acc_err_init.                                */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
impl_acc_err_init(ddi_acc_hdl_t *handlep)
{
	/* Error handling not supported */
	handlep->ah_acc.devacc_attr_access = DDI_DEFAULT_ACC;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_acc_hdl_init.                                */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
impl_acc_hdl_init(ddi_acc_hdl_t *handlep)
{
	ddi_acc_impl_t *hp;

	ASSERT(handlep);

	hp = (ddi_acc_impl_t *)handlep;

	/*
	 * check for SW byte-swapping
	 */
	hp->ahi_get8 = i_ddi_get8;
	hp->ahi_put8 = i_ddi_put8;
	hp->ahi_rep_get8 = i_ddi_rep_get8;
	hp->ahi_rep_put8 = i_ddi_rep_put8;
	if (handlep->ah_acc.devacc_attr_endian_flags & DDI_STRUCTURE_LE_ACC) {
		hp->ahi_get16 = i_ddi_swap_get16;
		hp->ahi_get32 = i_ddi_swap_get32;
		hp->ahi_get64 = i_ddi_swap_get64;
		hp->ahi_put16 = i_ddi_swap_put16;
		hp->ahi_put32 = i_ddi_swap_put32;
		hp->ahi_put64 = i_ddi_swap_put64;
		hp->ahi_rep_get16 = i_ddi_swap_rep_get16;
		hp->ahi_rep_get32 = i_ddi_swap_rep_get32;
		hp->ahi_rep_get64 = i_ddi_swap_rep_get64;
		hp->ahi_rep_put16 = i_ddi_swap_rep_put16;
		hp->ahi_rep_put32 = i_ddi_swap_rep_put32;
		hp->ahi_rep_put64 = i_ddi_swap_rep_put64;
	} else {
		hp->ahi_get16 = i_ddi_get16;
		hp->ahi_get32 = i_ddi_get32;
		hp->ahi_get64 = i_ddi_get64;
		hp->ahi_put16 = i_ddi_put16;
		hp->ahi_put32 = i_ddi_put32;
		hp->ahi_put64 = i_ddi_put64;
		hp->ahi_rep_get16 = i_ddi_rep_get16;
		hp->ahi_rep_get32 = i_ddi_rep_get32;
		hp->ahi_rep_get64 = i_ddi_rep_get64;
		hp->ahi_rep_put16 = i_ddi_rep_put16;
		hp->ahi_rep_put32 = i_ddi_rep_put32;
		hp->ahi_rep_put64 = i_ddi_rep_put64;
	}

	/* Legacy fault flags and support */
	hp->ahi_fault_check = i_ddi_acc_fault_check;
	hp->ahi_fault_notify = i_ddi_acc_fault_notify;
	hp->ahi_fault = 0;
	impl_acc_err_init(handlep);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_acc_set_fault.                              */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
i_ddi_acc_set_fault(ddi_acc_handle_t handle)
{
	ddi_acc_impl_t *hp = (ddi_acc_impl_t *)handle;

	if (!hp->ahi_fault) {
		hp->ahi_fault = 1;
			(*hp->ahi_fault_notify)(hp);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_acc_clr_fault.                              */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
i_ddi_acc_clr_fault(ddi_acc_handle_t handle)
{
	ddi_acc_impl_t *hp = (ddi_acc_impl_t *)handle;

	if (hp->ahi_fault) {
		hp->ahi_fault = 0;
			(*hp->ahi_fault_notify)(hp);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_acc_fault_notify.                           */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/* ARGSUSED */
void
i_ddi_acc_fault_notify(ddi_acc_impl_t *hp)
{
	/* Default version, does nothing */
}

/*========================= End of Function ========================*/

/*
 * SECTION: Misc functions
 */

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_assign_instance.                             */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
uint_t
impl_assign_instance(dev_info_t *dip)
{
	return ((uint_t)-1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_keep_instance.                               */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
impl_keep_instance(dev_info_t *dip)
{
	return (DDI_FAILURE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_free_instance.                               */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
impl_free_instance(dev_info_t *dip)
{
	return (DDI_FAILURE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_check_cpu.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
impl_check_cpu(dev_info_t *devi)
{
	return (DDI_SUCCESS);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- e_ddi_copyfromdev.                                */
/*                                                                  */
/* Function	- Perform a copy from a memory mapped device        */
/*		  (whose devinfo pointer is devi) separately mapped */
/*		  at devaddr in the kernel to a kernel buffer at    */
/*		  kaddr.                       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
e_ddi_copyfromdev(dev_info_t *devi,
    off_t off, const void *devaddr, void *kaddr, size_t len)
{
	const char **argv;

	bcopy(devaddr, kaddr, len);
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- e_ddi_copytodev.                                  */
/*                                                                  */
/* Function	- Perform a copy to a memory mapped device (whose   */
/*		  devinfo pointer is devi) separately mapped at     */
/*		  devaddr in the kernel from a kernel buffer at     */
/*		  kaddr.                       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
e_ddi_copytodev(dev_info_t *devi,
    off_t off, const void *kaddr, void *devaddr, size_t len)
{
	const char **argv;

	bcopy(kaddr, devaddr, len);
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- configure.                                        */
/*                                                                  */
/* Function	- Configure the hardware on the system. Called      */
/*		  before the rootfs is mounted.		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
configure(void)
{
	/*
	 * Initialize devices on the machine.
	 * Uses configuration tree built by the PROMs to determine what
	 * is present, and builds a tree of prototype dev_info nodes
	 * corresponding to the hardware which identified itself.
	 */
	i_ddi_init_root();

	impl_bus_probe(ccw_dip, 1);

	ndi_devi_online(ccw_dip, 0);

#ifdef	DDI_PROP_DEBUG
	(void) ddi_prop_debug(1);	/* Enable property debugging */
#endif	/* DDI_PROP_DEBUG */
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- status_okay.                                      */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
int
status_okay(int id, char *buf, int buflen)
{
	return (1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- impl_fix_props.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
void
impl_fix_props(dev_info_t *dip, dev_info_t *ch_dip, char *name, int len,
    caddr_t buffer)
{
	/*
	 * There are no adjustments needed in this implementation.
	 */
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- poke_mem.                                         */
/*                                                                  */
/* Function	- 						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
poke_mem(peekpoke_ctlops_t *in_args)
{
	int err = DDI_SUCCESS;
	on_trap_data_t otd;

	/* Set up protected environment. */
	if (!on_trap(&otd, OT_DATA_ACCESS)) {
		switch (in_args->size) {
		case sizeof (uint8_t):
			*(uint8_t *)(in_args->dev_addr) =
			    *(uint8_t *)in_args->host_addr;
			break;

		case sizeof (uint16_t):
			*(uint16_t *)(in_args->dev_addr) =
			    *(uint16_t *)in_args->host_addr;
			break;

		case sizeof (uint32_t):
			*(uint32_t *)(in_args->dev_addr) =
			    *(uint32_t *)in_args->host_addr;
			break;

		case sizeof (uint64_t):
			*(uint64_t *)(in_args->dev_addr) =
			    *(uint64_t *)in_args->host_addr;
			break;

		default:
			err = DDI_FAILURE;
			break;
		}
	} else
		err = DDI_FAILURE;

	/* Take down protected environment. */
	no_trap();

	return (err);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- peek_mem.                                         */
/*                                                                  */
/* Function	- 						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
peek_mem(peekpoke_ctlops_t *in_args)
{
	int err = DDI_SUCCESS;
	on_trap_data_t otd;

	if (!on_trap(&otd, OT_DATA_ACCESS)) {
		switch (in_args->size) {
		case sizeof (uint8_t):
			*(uint8_t *)in_args->host_addr =
			    *(uint8_t *)in_args->dev_addr;
			break;

		case sizeof (uint16_t):
			*(uint16_t *)in_args->host_addr =
			    *(uint16_t *)in_args->dev_addr;
			break;

		case sizeof (uint32_t):
			*(uint32_t *)in_args->host_addr =
			    *(uint32_t *)in_args->dev_addr;
			break;

		case sizeof (uint64_t):
			*(uint64_t *)in_args->host_addr =
			    *(uint64_t *)in_args->dev_addr;
			break;

		default:
			err = DDI_FAILURE;
			break;
		}
	} else
		err = DDI_FAILURE;

	no_trap();
	return (err);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- peekpoke_mem.                                     */
/*                                                                  */
/* Function	- This is only called to process peek/poke when the */
/*		  DIP is NULL. Assume that this is for memory as    */
/*		  nexi takes care of device safe accesses.	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
peekpoke_mem(ddi_ctl_enum_t cmd, peekpoke_ctlops_t *in_args)
{
	return (cmd == DDI_CTLOPS_PEEK ? peek_mem(in_args) 
				       : poke_mem(in_args));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_devacc_to_hatacc.                           */
/*                                                                  */
/* Function	- Set HAT endianess attributes from ddi_device_acc_ */
/*		  attr.                        		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
i_ddi_devacc_to_hatacc(ddi_device_acc_attr_t *devaccp, uint_t *hataccp)
{
	if (devaccp != NULL) {
		if (devaccp->devacc_attr_endian_flags == DDI_STRUCTURE_LE_ACC) {
			*hataccp &= ~HAT_ENDIAN_MASK;
			*hataccp |= HAT_STRUCTURE_LE;
		}
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_check_cache_attr.                           */
/*                                                                  */
/* Function	- Check if the specified cache attribute is support-*/
/*		  ed on the platform. This function must be called  */
/*		  before i_ddi_cacheattr_to_hatacc.		    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

boolean_t
i_ddi_check_cache_attr(uint_t flags)
{
	/*
	 * The cache attributes are mutually exclusive. Any combination of
	 * the attributes leads to a failure.
	 */
	uint_t cache_attr = IOMEM_CACHE_ATTR(flags);
	if ((cache_attr != 0) && ((cache_attr & (cache_attr - 1)) != 0))
		return (B_FALSE);

	/*
	 * On the sparc architecture, only IOMEM_DATA_CACHED is meaningful,
	 * but others lead to a failure.
	 */
	if (cache_attr & IOMEM_DATA_CACHED)
		return (B_TRUE);
	else
		return (B_FALSE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_cacheattr_to_hatacc.                        */
/*                                                                  */
/* Function	- Set HAT cache attributes from the cache attr-     */
/*		  ibutes.					    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
i_ddi_cacheattr_to_hatacc(uint_t flags, uint_t *hataccp)
{
	uint_t cache_attr = IOMEM_CACHE_ATTR(flags);
	static char *fname = "i_ddi_cacheattr_to_hatacc";
#if defined(lint)
	*hataccp = *hataccp;
#endif
	/*
	 * set HAT attrs according to the cache attrs.
	 */
	switch (cache_attr) {
	/*
	 * The cache coherency is always maintained on s390x, and
	 * nothing is required.
	 */
	case IOMEM_DATA_CACHED:
		break;
	/*
	 * Both IOMEM_DATA_UC_WRITE_COMBINED and IOMEM_DATA_UNCACHED are
	 * not supported on s390x -- this case must not occur because the
	 * cache attribute is scrutinized before this function is called.
	 */
	case IOMEM_DATA_UNCACHED:
	case IOMEM_DATA_UC_WR_COMBINE:
	default:
		cmn_err(CE_WARN, "%s: cache_attr=0x%x is ignored.",
		    fname, cache_attr);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- i_ddi_get_intx_nintrs.                            */
/*                                                                  */
/* Function	- Return the number of interrupts.                  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
i_ddi_get_intx_nintrs(dev_info_t *dip)
{
	struct ddi_parent_private_data *pdp;

	if ((pdp = ddi_get_parent_data(dip)) == NULL)
		return (0);

	pdp->par_nintr = 6;

	return (pdp->par_nintr);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getrootdev.                                       */
/*                                                                  */
/* Function	- Return the dev_t of the rootdev.                  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

dev_t
getrootdev(void)
{
	/*
	 * Precedence given to rootdev if set in /etc/system
	 */
	if (root_is_svm) {
		return (ddi_pathname_to_dev_t(svm_bootpath));
	}

	/*
	 * Usually rootfs.bo_name is initialized by the
	 * the bootpath property from bootenv.rc, but
	 * defaults to "/ramdisk:a" otherwise.
	 */
	return (ddi_pathname_to_dev_t(rootfs.bo_name));
}

/*========================= End of Function ========================*/
