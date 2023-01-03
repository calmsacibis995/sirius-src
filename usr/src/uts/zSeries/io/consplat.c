/*------------------------------------------------------------------*/
/* 								    */
/* Name        - consplat.c 					    */
/* 								    */
/* Function    - Console configuration routines.                    */
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
#include <sys/param.h>
#include <sys/cmn_err.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/debug.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/esunddi.h>
#include <sys/ddi_impldefs.h>
#include <sys/promif.h>
#include <sys/modctl.h>
#include <sys/termios.h>
#include <sys/archsystm.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern char *get_alias(char *alias, char *buf);

extern int polled_debug;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/


/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

static char consolepath[] = "/ccw/cnsl@0x0000";

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_use_polled_debug.                            */
/*                                                                  */
/* Function	- Return the debug state.                           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
plat_use_polled_debug()
{
	return (polled_debug);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_support_serial_kbd_and_ms.                   */
/*                                                                  */
/* Function	- Tell caller we don't support a serial keyboard or */
/*		  mouse.                       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
plat_support_serial_kbd_and_ms()
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_kbdpath.                                     */
/*                                                                  */
/* Function	- Return generic path to keyboard device from the   */
/*		  aliases.                     		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

char *
plat_kbdpath(void)
{
	static char *kbdpath = NULL;
	static char buf[MAXPATHLEN];
	char *path;

	if (kbdpath != NULL)
		return (kbdpath);

	/*
	 * look for the keyboard property in /aliases
	 * The keyboard alias is required on 1275 systems
	 */
	path = get_alias("keyboard", buf);
	if (path != NULL) {
		kbdpath = path;
		return (path);
	}

	return (NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_fbpath.                                      */
/*                                                                  */
/* Function	- Return generic path to display device from the    */
/*		  alias.                       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

char *
plat_fbpath(void)
{
	static char *fbpath = NULL;
	static char buf[MAXPATHLEN];
	char *path;

	if (fbpath != NULL)
		return (fbpath);

	/* look for the screen property in /aliases */
	path = get_alias("screen", buf);
	if (path != NULL) {
		fbpath = path;
		return (path);
	}

	return (NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_mousepath.                                   */
/*                                                                  */
/* Function	- Return generic path to mouse device from the      */
/*		  alias.                       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

char *
plat_mousepath(void)
{
	return (NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_stdoutpath.                                  */
/*                                                                  */
/* Function	- Return the pathname of the generic stdout device. */
/*		                               		 	    */
/*------------------------------------------------------------------*/

char *
plat_stdoutpath(void)
{
	int cons;

	cons = diag_24(-1, NULL, NULL);

	snprintf(consolepath,
		 sizeof(consolepath),
		 "/ccw/cnsl@0x%04x",
		 cons);

	return (consolepath);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_stdinpath.                                   */
/*                                                                  */
/* Function	- Return the pathname of the generic stdin device.  */
/*		                               		 	    */
/*------------------------------------------------------------------*/

char *
plat_stdinpath(void)
{
	return (plat_stdoutpath());
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_stdin_is_keyboard.                           */
/*                                                                  */
/* Function	- Return whether stdin is the keyboard device.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
plat_stdin_is_keyboard(void)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_stdout_is_framebuffer.                       */
/*                                                                  */
/* Function	- Return whether stdout is a framebuffer device.    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
plat_stdout_is_framebuffer(void)
{
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kadb_uses_kernel.                                 */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
kadb_uses_kernel()
{
	/* only used on intel */
}

/*========================= End of Function ========================*/

/********************************************************************/
/* If VIS_PIXEL mode will be implemented on s390x, these following  */
/* functions should be re-considered. Now these functions are	    */
/* unused on s390x.						    */
/********************************************************************/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_tem_get_inverses.                            */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
plat_tem_get_inverses(int *inverse, int *inverse_screen)
{
	*inverse = 0;
	*inverse_screen = 0;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_tem_get_prom_font_size.                      */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
plat_tem_get_prom_font_size(int *charheight, int *windowtop)
{
	*charheight = 0;
	*windowtop = 0;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_tem_get_prom_size.                      	    */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
plat_tem_get_prom_size(size_t *height, size_t *width)
{
	*height = 24;
	*width  = 80;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_tem_hide_prom_cursor.                   	    */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
plat_tem_hide_prom_cursor(void)
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- plat_tem_get_prom_pos.                       	    */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
plat_tem_get_prom_pos(uint32_t *row, uint32_t *col)
{
	*row = 0;
	*col = 0;
}

/*========================= End of Function ========================*/
