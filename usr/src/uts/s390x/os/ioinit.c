/*------------------------------------------------------------------*/
/* 								    */
/* Name        - ioinit.c   					    */
/* 								    */
/* Function    - Scan the system for I/O devices and create a       */
/* 		 double linked list of structures representing	    */
/* 		 these devices. Get device characteristics of these */
/* 		 devices.					    */
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

/*------------------------------------------------------------------*/
/*                 D e f i n e s                                    */
/*------------------------------------------------------------------*/

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/asm_linkage.h>
#include <sys/memlist_plat.h>
#include <sys/memlist_impl.h>
#include <sys/machparam.h>
#include <sys/intr.h>
#include <sys/machsystm.h>
#include <sys/ios390x.h>
#include <sys/devinit.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static devList * getIOdevs(struct memlist *, uint16_t);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

devList *sysDevs;		// Linked list of all devices	
ioDev	*conDev;		// System console device

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getIOdevs.                                        */
/*                                                                  */
/* Function	- Find all the I/O devices connected to this system */
/*		  and get their details. Return the head of this    */
/*		  list to the caller.				    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static devList *
getIOdevs(struct memlist *ndata, uint16_t cons)
{
	int sch,
	    lSch,
	    nSch = 0,
	    cc = 0;

	devList *dev;
	ioDev	*curDev,
		*prev = 0;
	struct schib subch;
	long	schMask = 0x00010000;

	//
	// First just count the number of devices we have
	//
	for (sch = 0, cc = 0; cc == 0; sch++) {
		__asm__ ("	lghi	%0,0\n"
		         "	lgr	1,%3\n"
			 "	ogr	1,%4\n"
			 "	stsch	%2\n"
			 "	jnz	0f\n"
			 "	aghi	%1,1\n"
			 "	j	1f\n"
			 "0:	lghi	%0,1\n"
			 "1:"
			 : "+r" (cc), "+r" (nSch), "=m" (subch)
			 : "r" (sch), "r" (schMask)
			 : "1", "memory", "cc");
	}

	prom_printf("Highest subchannel address encountered: %04x\n",nSch);
	
	// 
	// Rather than an alloc for each device grab a slab of 
	// memory now to keep the devices together
	//
	lSch = sizeof(devList) + (++nSch * sizeof(ioDev));
	dev  = (devList *) ndata_alloc(ndata, lSch, MMU_PAGESIZE);
	if (dev) {
		bzero(dev, lSch);
		prom_printf("I/O Device List starts at %p for %d bytes\n",dev,lSch);
		dev->devices  = (ioDev *) (dev + sizeof(devList));
		dev->devCount = nSch;
		curDev	      = dev->devices;
		
		for (sch = 0, cc = 0; cc == 0; sch++) {

			__asm__ ("	lghi	%0,0\n"
			         "	lgr  	1,%2\n"
				 "	ogr	1,%3\n"
				 "	stsch	%1\n"
				 "	jz	1f\n"
				 "	lghi	%0,1\n"
				 "1:"
				 : "+r" (cc), "=m" (curDev->sch)
				 : "r" (sch), "r" (schMask)
				 : "1", "memory", "cc");

			if (cc == 0) {
				ioDev *nextDev;
		
				//
				// Build the ioDev for this device
				// and chain it to the rest
				//
				curDev->prev  = prev;
				prev	      = curDev;
				nextDev	      = curDev + 1;
				curDev->next  = nextDev;
				curDev->schid = sch | 0x00010000;

				//
				// If this is a valid device no. ask z/VM for
				// the device characteristics via DIAG 0x210
				//
				if (curDev->sch.pmcw.dnv) {
					curDev->dev.vrdcdvno = curDev->sch.pmcw.dev; 
					curDev->dev.vrdclen  = sizeof(curDev->dev);
					__asm__ ("	sam31\n"
						 "	la	1,%0\n"
						 "	diag	1,0,0x210\n"
						 "	sam64"
						: : "m" (curDev->dev)
						: "1", "cc");
					if (curDev->dev.vrdcdvno == cons)
						conDev = curDev;
				}	
				curDev = nextDev;
			}
		}
		curDev--;
		curDev->next = 0;
	}
	return (dev);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- io_init.                                          */
/*                                                                  */
/* Function	- Determine which device is the system console and  */
/*		  then go obtain a list of system I/O devices.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
init_io_struct(struct memlist *ndata)
{
	uint16_t cons;

	prom_printf("Initializing I/O structures\n");

	__asm__ ("	sam31\n"
		 "	lhi	%0,-1\n"
		 "	diag	%0,0,0x24\n"
		 "	sam64"
		 : "+r" (cons) : : "0", "1", "cc");

	prom_printf("Console address = %04x\n",cons);

	sysDevs = getIOdevs(ndata, cons);
	
	return (sysDevs != 0);
}

/*========================= End of Function ========================*/
