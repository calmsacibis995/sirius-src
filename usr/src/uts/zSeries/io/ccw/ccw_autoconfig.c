/*------------------------------------------------------------------*/
/* 								    */
/* Name        - ccw_autoconfig.c				    */
/* 								    */
/* Function    - Discover I/O configuration and create device tree. */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - September, 2007  				    */
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

#define DC_ENDT	0xff
#define DT_ENDT	0xff
#define PIL_0	0
#define PIL_1	2
#define PIL_2	5
#define PIL_3	6
#define PIL_4	7

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/sunddi.h>
#include <sys/ddi_subrdefs.h>
#include <sys/bootconf.h>
#include <sys/modctl.h>
#include <sys/errno.h>
#include <sys/ios390x.h>
#include <sys/devinit.h>
#include <sys/mutex.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

typedef struct _devTable {
	uint8_t	 class;		/* Device class			    */
	uint8_t  type;		/* Device type			    */
	short	 count;		/* Count of compat entries	    */
	short	 isc;		/* Interrupt subclass	 	    */
	int32_t  *instance;	/* Instance number		    */
	char	 *name;		/* Device name			    */
	char	 *compat[2];	/* Driver compatability list	    */
} devTable;

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern devList *sysDevs;		// Linked list of all devices	
extern ioDev	*conDev;		// System console device

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

void ccw_enumerate(dev_info_t *, int);
void create_device(dev_info_t *, ioDev *);
static int locateDev(uint8_t, uint8_t);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

static struct modlmisc modlmisc = {
	&mod_miscops, "CCW interface %I%"
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modlmisc, NULL
};


/*------------------------------------------------------------------*/
/* Instance counters for various device classes			    */
/*------------------------------------------------------------------*/
static int32_t	consinst = 0,
		grafinst = 0,
		tapeinst = 0,
		osadinst = 0,
		dasdinst = 0;

/*------------------------------------------------------------------*/
/* Device driver to device type lookup table			    */
/*------------------------------------------------------------------*/
static devTable devMap[] = {
	{DC_CONS, DT_3215, 1, PIL_4, &consinst, "console", {"con3215", NULL}},
	{DC_CONS, DT_ENDT, 0, PIL_0, NULL, NULL, {NULL, NULL}},
	{DC_GRAF, DT_3277, 1, PIL_0, &grafinst, "graf", {"term3270", NULL}},
	{DC_GRAF, DT_3278, 1, PIL_0, &grafinst, "graf", {"term3270", NULL}},
	{DC_GRAF, DT_ENDT, 0, PIL_0, NULL, NULL, {NULL, NULL}},
	{DC_URIN, DT_ENDT, 0, PIL_0, NULL, NULL, {NULL, NULL}},
	{DC_UROT, DT_ENDT, 0, PIL_0, NULL, NULL, {NULL, NULL}},
	{DC_TAPE, DT_3480, 1, PIL_1, &tapeinst, "mt", {"tape3480", NULL}},
	{DC_TAPE, DT_3590, 1, PIL_1, &tapeinst, "mt", {"tape3590", NULL}},
	{DC_TAPE, DT_NTAP, 1, PIL_1, &tapeinst, "mt", {"tapenew", NULL}},
	{DC_TAPE, DT_ENDT, 0, PIL_0, NULL, NULL, {NULL, NULL}},
	{DC_DASD, DT_3390, 1, PIL_3, &dasdinst, "dasd", {"diag250", NULL}},
	{DC_DASD, DT_ENDT, 0, PIL_0, NULL, NULL, {NULL, NULL}},
	{DC_SPEC, DT_OSAD, 1, PIL_2, &osadinst, "osad", {"osad", NULL}},
	{DC_SPEC, DT_ENDT, 0, PIL_0, NULL, NULL, {NULL, NULL}},
	{DC_FBAD, DT_9336, 1, PIL_3, &dasdinst, "dasd", {"diag250", NULL}},
	{DC_SPEC, DT_ENDT, 0, PIL_0, NULL, NULL, {NULL, NULL}},
	{DC_ENDT, DT_ENDT, 0, PIL_0, NULL, NULL, {NULL, NULL}}
};

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _init.                                            */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
_init(void)
{
	int	err;

	if ((err = mod_install(&modlinkage)) != 0)
		return (err);

	impl_bus_add_probe(ccw_enumerate);
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _fini.                                            */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
_fini(void)
{
	int	err;

	if ((err = mod_remove(&modlinkage)) != 0)
		return (err);

	impl_bus_delete_probe(ccw_enumerate);
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- _info.                                            */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- ccw_enumerate.                                    */
/*                                                                  */
/* Function	- Create the device tree.                           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
ccw_enumerate(dev_info_t *rdip, int probe)
{
	ioDev	*dvc;
	int	ccw_regs[3] = {0, 0, 0};
	int	intr[5]	    = {PIL_0, PIL_1, PIL_2, PIL_3, PIL_4};

	ndi_prop_update_int_array(DDI_DEV_T_NONE, rdip, "reg", 
				  (int *)ccw_regs, 3);
	ndi_prop_update_int_array(DDI_DEV_T_NONE, rdip,
				  "interrupts", (int *)intr, 
				  (int)(sizeof (intr) / sizeof (int)));

	if (probe) {
		for (dvc = sysDevs->devices; dvc != NULL; dvc = dvc->next) {
			create_device(rdip, dvc);
		}
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- create_device.                                    */
/*                                                                  */
/* Function	- Determine if this is a supported device and if so */
/*		  create a node for it and add some propositions.   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
create_device(dev_info_t *rdip, ioDev *dvc)
{
	char 	nodename[32], 
		unitaddr[9];
	int	iMap;
	dev_info_t *dip;

	prom_printf("in ccw autoconfig/create_device\n");
	iMap  = locateDev(dvc->dev.vrdcvcla, dvc->dev.vrdcvtyp);
	if (iMap != -1) {
		snprintf(nodename, sizeof (nodename), "%s,0x%x", 
			 devMap[iMap].name,dvc->dev.vrdcdvno);
		snprintf(unitaddr, sizeof (unitaddr), "0x%x",
			 dvc->dev.vrdcdvno);
		dvc->instance = (*devMap[iMap].instance)++;
		dvc->private  = (void *) ddi_add_child(rdip, nodename, 
						       DEVI_SID_NODEID, 
						       dvc->instance);
				     
		dip = dvc->private;
		prom_printf("ccw_autoconfig/create_dev--dvc: %p; dip: %p\n");
		ddi_set_driver_private(dip, dvc);
		ndi_prop_update_int(DDI_DEV_T_NONE, dip, 
				    "device-type", dvc->dev.vrdcvtyp);
		ndi_prop_update_int(DDI_DEV_T_NONE, dip, 
				    "device-class", dvc->dev.vrdcvcla);
		ndi_prop_update_string(DDI_DEV_T_NONE, dip, 
				       "unit-address", unitaddr);
		if (devMap[iMap].count) {
			ndi_prop_update_string_array(DDI_DEV_T_NONE, dip, 
						     "compatible", 
						     (char **)devMap[iMap].compat,
						     devMap[iMap].count);
		}
		dvc->sch.pmcw.enabled	= 1;
		dvc->sch.pmcw.isc	= devMap[iMap].isc;
		__asm__ ("	lgf 	1,%1\n"
			 "	msch	%0\n"
			 : : "Q" (dvc->sch), "m" (dvc->schid) 
			 : "1", "cc");
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- locateDev.                                        */
/*                                                                  */
/* Function	- Locate an entry in the devMap table that matches  */
/*		  the device class and type.   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int 
locateDev(uint8_t class, uint8_t type)
{
	int iMap;

	for (iMap = 0; devMap[iMap].class != DC_ENDT; iMap++) {
		if (devMap[iMap].class == class) {
			for (; devMap[iMap].type != DT_ENDT; iMap++) {
				if (devMap[iMap].type == type)
					return(iMap);
			}
			return(-1);
		}
	}
	return(-1);
}

/*========================= End of Function ========================*/
