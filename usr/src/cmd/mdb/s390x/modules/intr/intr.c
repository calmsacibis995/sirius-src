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
/*                                                                  */
/* Copyright 2008 Sine Nomine Associates.                           */
/* All rights reserved.                                             */
/* Use is subject to license terms.                                 */
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/mdb_modapi.h>
#include <mdb/mdb_ks.h>
#include <sys/async.h>		/* ecc_flt for pci_ecc.h */
#include <sys/ddi_subrdefs.h>
#include <sys/pci/pci_obj.h>
#include "px_obj.h"

static int intr_pci_walk_step(mdb_walk_state_t *);
static int intr_px_walk_step(mdb_walk_state_t *);
static void intr_pci_print_items(mdb_walk_state_t *);
static void intr_px_print_items(mdb_walk_state_t *);
static char *intr_get_intr_type(msiq_rec_type_t);
static void intr_print_banner(void);

typedef struct intr_info {
	uint32_t	cpuid;
	uint32_t	inum;
	uint32_t	num;
	uint32_t	pil;
	uint16_t	mondo;
	uint8_t		ino_ino;
	uint_t		intr_state;
	int		instance;
	int		shared;
	msiq_rec_type_t intr_type;
	char		driver_name[12];
	char		pathname[MAXNAMELEN];
}
intr_info_t;

static void intr_print_elements(intr_info_t);
static int detailed = 0; /* Print detailed view */


static int
intr_walk_init(mdb_walk_state_t *wsp)
{
	wsp->walk_addr = NULL;

	return (WALK_NEXT);
}

static int
intr_walk_step(mdb_walk_state_t *wsp)
{
	pci_t		*pci_per_p;
	px_t		*px_state_p;

	/* read globally declared structures in the pci driver */
	if (mdb_readvar(&pci_per_p, "per_pci_state") != -1) {
		wsp->walk_addr = (uintptr_t)pci_per_p;
		intr_pci_walk_step(wsp);
	}

	/* read globally declared structures in the px driver */
	if (mdb_readvar(&px_state_p, "px_state_p") != -1) {
		wsp->walk_addr = (uintptr_t)px_state_p;
		intr_px_walk_step(wsp);
	}

	return (WALK_DONE);
}

static int
intr_pci_walk_step(mdb_walk_state_t *wsp)
{
	pci_t		*pci_per_p;
	pci_t		pci_per;
	uintptr_t	start_addr;

	/* Read start of state structure array */
	if (mdb_vread(&pci_per_p, sizeof (uintptr_t),
	    (uintptr_t)wsp->walk_addr) == -1) {
		mdb_warn("intr: failed to read the initial pci_per_p "
		    "structure\n");
		return (WALK_ERR);
	}

	/* Figure out how many items are here */
	start_addr = (uintptr_t)pci_per_p;

	intr_print_banner();

	while (mdb_vread(&pci_per_p, sizeof (uintptr_t),
	    (uintptr_t)start_addr) != -1) {
		/* Read until nothing is left */
		if (mdb_vread(&pci_per, sizeof (pci_t),
		    (uintptr_t)pci_per_p) == -1) {
			return (WALK_DONE);
		}

		wsp->walk_addr = (uintptr_t)pci_per.pci_ib_p;
		intr_pci_print_items(wsp);

		start_addr += sizeof (uintptr_t);
	}

	return (WALK_DONE);
}

static int
intr_px_walk_step(mdb_walk_state_t *wsp)
{
	px_t		*px_state_p;
	px_t		px_state;
	uintptr_t	start_addr;

	/* Read start of state structure array */
	if (mdb_vread(&px_state_p, sizeof (uintptr_t),
	    (uintptr_t)wsp->walk_addr) == -1) {
		mdb_warn("intr: failed to read the initial px_per_p "
		    "structure\n");
		return (WALK_ERR);
	}

	/* Figure out how many items are here */
	start_addr = (uintptr_t)px_state_p;

	intr_print_banner();

	while (mdb_vread(&px_state_p, sizeof (uintptr_t),
	    (uintptr_t)start_addr) != -1) {
		/* Read until nothing is left */
		if (mdb_vread(&px_state, sizeof (px_t),
		    (uintptr_t)px_state_p) == -1) {
			return (WALK_DONE);
		}

		wsp->walk_addr = (uintptr_t)px_state.px_ib_p;
		intr_px_print_items(wsp);

		start_addr += sizeof (uintptr_t);
	}

	return (WALK_DONE);
}

static void
intr_pci_print_items(mdb_walk_state_t *wsp)
{
	ib_t			pci_ib;
	ib_ino_info_t		*ib_ino_lst;
	ib_ino_info_t		list;
	ih_t			ih;
	int			count;
	char			name[MODMAXNAMELEN + 1];
	struct dev_info		devinfo;
	intr_info_t		info;

	if (mdb_vread(&pci_ib, sizeof (ib_t),
	    (uintptr_t)wsp->walk_addr) == -1) {
		mdb_warn("intr: failed to read pci interrupt block "
		    "structure\n");
		return;
	}

	/* Read in ib_ino_info_t structure at address */
	ib_ino_lst = pci_ib.ib_ino_lst;
	if (mdb_vread(&list, sizeof (ib_ino_info_t),
	    (uintptr_t)ib_ino_lst) == -1) {
		/* Nothing here to read from */
		return;
	}

	do {
		if (mdb_vread(&ih, sizeof (ih_t),
		    (uintptr_t)list.ino_ih_start) == -1) {
			mdb_warn("intr: failed to read pci interrupt entry "
			    "structure\n");
			return;
		}

		count = 0;

		do {
			bzero((void *)&info, sizeof (intr_info_t));

			if (list.ino_ih_size > 1) {
				info.shared = 1;
			}

			(void) mdb_devinfo2driver((uintptr_t)ih.ih_dip,
			    name, sizeof (name));

			(void) mdb_ddi_pathname((uintptr_t)ih.ih_dip,
			    info.pathname, sizeof (info.pathname));

			/* Get instance */
			if (mdb_vread(&devinfo, sizeof (struct dev_info),
			    (uintptr_t)ih.ih_dip) == -1) {
				mdb_warn("intr: failed to read DIP "
				    "structure\n");
				return;
			}

			/* Make sure the name doesn't over run */
			(void) mdb_snprintf(info.driver_name,
			    sizeof (info.driver_name), "%s", name);

			info.instance = devinfo.devi_instance;
			info.inum = ih.ih_inum;
			info.intr_type = INTX_REC;
			info.num = 0;
			info.intr_state = ih.ih_intr_state;
			info.ino_ino = list.ino_ino;
			info.mondo = list.ino_mondo;
			info.pil = list.ino_pil;
			info.cpuid = list.ino_cpuid;

			intr_print_elements(info);
			count++;

			(void) mdb_vread(&ih, sizeof (ih_t),
			    (uintptr_t)ih.ih_next);

		} while (count < list.ino_ih_size);

	} while (mdb_vread(&list, sizeof (ib_ino_info_t),
	    (uintptr_t)list.ino_next) != -1);
}

static void
intr_px_print_items(mdb_walk_state_t *wsp)
{
	px_ib_t			px_ib;
	px_ib_ino_info_t	*px_ib_ino_lst;
	px_ib_ino_info_t	px_list;
	px_ih_t			px_ih;
	int			count;
	char			name[MODMAXNAMELEN + 1];
	struct dev_info		devinfo;
	intr_info_t		info;

	if (mdb_vread(&px_ib, sizeof (px_ib_t), wsp->walk_addr) == -1) {
		mdb_warn("intr: failed to read px interrupt block "
		    "structure\n");
		return;
	}

	/* Read in px_ib_ino_info_t structure at address */
	px_ib_ino_lst = px_ib.ib_ino_lst;
	if (mdb_vread(&px_list, sizeof (px_ib_ino_info_t),
	    (uintptr_t)px_ib_ino_lst) == -1) {
		/* Nothing here to read from */
		return;
	}

	do {
		if (mdb_vread(&px_ih, sizeof (px_ih_t),
		    (uintptr_t)px_list.ino_ih_start) == -1) {
			mdb_warn("intr: failed to read px interrupt entry "
			    "structure\n");
			return;
		}

		count = 0;

		do {
			bzero((void *)&info, sizeof (intr_info_t));

			if (px_list.ino_ih_size > 1) {
				info.shared = 1;
			}

			(void) mdb_devinfo2driver((uintptr_t)px_ih.ih_dip,
			    name, sizeof (name));

			(void) mdb_ddi_pathname((uintptr_t)px_ih.ih_dip,
			    info.pathname, sizeof (info.pathname));

			/* Get instance */
			if (mdb_vread(&devinfo, sizeof (struct dev_info),
			    (uintptr_t)px_ih.ih_dip) == -1) {
				mdb_warn("intr: failed to read DIP "
				    "structure\n");
				return;
			}

			/* Make sure the name doesn't over run */
			(void) mdb_snprintf(info.driver_name,
			    sizeof (info.driver_name), "%s", name);

			info.instance = devinfo.devi_instance;
			info.inum = px_ih.ih_inum;
			info.intr_type = px_ih.ih_rec_type;
			info.num = px_ih.ih_msg_code;
			info.intr_state = px_ih.ih_intr_state;
			info.ino_ino = px_list.ino_ino;
			info.mondo = px_list.ino_sysino;
			info.pil = px_list.ino_pil;
			info.cpuid = px_list.ino_cpuid;

			intr_print_elements(info);
			count++;

			(void) mdb_vread(&px_ih, sizeof (ih_t),
			    (uintptr_t)px_ih.ih_next);

		} while (count < px_list.ino_ih_size);

	} while (mdb_vread(&px_list, sizeof (px_ib_ino_info_t),
	    (uintptr_t)px_list.ino_next) != -1);
}

static char *
intr_get_intr_type(msiq_rec_type_t rec_type)
{
	switch (rec_type) {
		case	MSG_REC:
			return ("PCIe");
		case	MSI32_REC:
		case	MSI64_REC:
			return ("MSI");
		case	INTX_REC:
		default:
			return ("Fixed");
	}
}

static void
intr_print_banner(void)
{
	if (!detailed) {
		mdb_printf("\n%<u>\tDevice\t"
		    " Shared\t"
		    " Type\t"
		    " MSG #\t"
		    " State\t"
		    " INO\t"
		    " Mondo\t"
		    "  Pil\t"
		    " CPU   %</u>"
		    "\n");
	}
}

static void
intr_print_elements(intr_info_t info)
{
	if (!detailed) {
		mdb_printf(" %11s#%d\t", info.driver_name, info.instance);
		mdb_printf(" %5s\t",
		    info.shared ? "yes" : "no");
		mdb_printf(" %s\t", intr_get_intr_type(info.intr_type));
		if (strcmp("Fixed", intr_get_intr_type(info.intr_type)) == 0) {
			mdb_printf("  --- \t");
		} else {
			mdb_printf(" %4d\t", info.num);
		}

		mdb_printf(" %2s\t",
		    info.intr_state ? "enbl" : "disbl");
		mdb_printf(" 0x%x\t", info.ino_ino);
		mdb_printf(" 0x%x\t", info.mondo);
		mdb_printf(" %4d\t", info.pil);
		mdb_printf(" %3d \n", info.cpuid);
	} else {
		mdb_printf("\n-------------------------------------------\n");
		mdb_printf("Device:\t\t%s\n", info.driver_name);
		mdb_printf("Instance:\t%d\n", info.instance);
		mdb_printf("Path:\t\t%s\n", info.pathname);
		mdb_printf("Inum:\t\t%d\n", info.inum);
		mdb_printf("Interrupt Type:\t%s\n",
		    intr_get_intr_type(info.intr_type));
		if (strcmp("MSI", intr_get_intr_type(info.intr_type)) == 0)
			mdb_printf("MSI/X Number:\t%s\n", info.num);

		mdb_printf("Shared Intr:\t%s\n",
		    info.shared ? "yes" : "no");
		mdb_printf("State:\t\t%d (%s)\n", info.intr_state,
		    info.intr_state ? "Enabled" : "Disabled");
		mdb_printf("INO:\t\t0x%x\n", info.ino_ino);
		mdb_printf("Mondo:\t\t0x%x\n", info.mondo);
		mdb_printf("Pil:\t\t%d\n", info.pil);
		mdb_printf("CPU:\t\t%d\n", info.cpuid);
	}
}

/*ARGSUSED*/
static void
intr_walk_fini(mdb_walk_state_t *wsp)
{
	/* Nothing to do here */
}

/*ARGSUSED*/
static int
intr_intr(uintptr_t addr, uint_t flags, int argc, const mdb_arg_t *argv)
{
	detailed = 0;

	if (mdb_getopts(argc, argv, 'd', MDB_OPT_SETBITS, TRUE, &detailed,
	    NULL) != argc)
		return (DCMD_USAGE);

	if (!(flags & DCMD_ADDRSPEC)) {
		if (mdb_walk_dcmd("interrupts", "interrupts", argc, argv)
		    == -1) {
			mdb_warn("can't walk pci/px buffer entries\n");
			return (DCMD_ERR);
		}
		return (DCMD_OK);
	}

	return (DCMD_OK);
}

/*
 * MDB module linkage information:
 */

static const mdb_dcmd_t dcmds[] = {
	{ "interrupts", "[-d]", "display the interrupt info registered with "
	    "the PCI/PX nexus drivers", intr_intr },
	{ NULL }
};

static const mdb_walker_t walkers[] = {
	{ "interrupts", "walk PCI/PX interrupt structures",
		intr_walk_init, intr_walk_step, intr_walk_fini },
	{ NULL }
};

static const mdb_modinfo_t modinfo = {
	MDB_API_VERSION, dcmds, walkers
};

const mdb_modinfo_t *
_mdb_init(void)
{
	return (&modinfo);
}
