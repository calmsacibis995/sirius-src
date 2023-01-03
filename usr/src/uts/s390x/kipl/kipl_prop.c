/*------------------------------------------------------------------*/
/* 								    */
/* Name        - kipl_prop.c					    */
/* 								    */
/* Function    - Routines to handle propositions.                   */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - January, 2007					    */
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

#define BOOT_SUCCESS	0
#define BOOT_FAILURE	-1

#define NPROP		50
#define LNAME 		31
#define LVALUE		240

#define MAX(a,b)	((a) > (b) ? (a) : (b))
#define MIN(a,b)	((a) < (b) ? (a) : (b))

#define DBG(a)		if (kbm_debug) prom_printf("%l\n",(a))

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/vmparam.h>
#include <sys/bootconf.h>
#include <sys/archsystm.h>
#include <sys/bootvfs.h>
#include <sys/ctype.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

typedef struct __bootProp {
	char    name[LNAME+1];		// Name of proposition
	size_t	size;			// Size of proposition
	char	value[LVALUE+1];	// Value of proposition
} bootProp;

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

extern pgcnt_t physmem;

extern struct bootops *bop;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static int parse_value(char *, uint64_t *);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

static const bootProp proto[] = {{"bios-boot-device", 4, 0},
		 {"bootprog",  4, "hmc"},
		 {"bootargs", 0, 0},
		 {"impl-arch-name", 0, 0},
		 {"mfg-id", 0, 0},
		 {"ramdisk_start", 0, 0},
		 {"ramdisk_end", 0, 0},
		 {"whoami", 34, "/platform/s390x/kernel/s390x/unix"},
		 {0, 0, 0}};

static bootProp bp[NPROP];

static uint_t kbm_debug = 1;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- propInit.                                         */
/*                                                                  */
/* Function	- Initialize the bp[] array.                        */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
propInit()
{
	int iProp;

	bzero(bp, sizeof(bp));
	for (iProp = 0; proto[iProp].name[0] != 0; iProp++) {
		strncpy(&bp[iProp].name, &proto[iProp].name, LNAME);
		if (proto[iProp].size > 0) {
			bp[iProp].size = proto[iProp].size;
			bcopy(&proto[iProp].value, &bp[iProp].value, bp[iProp].size);
		}
	}

	return;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getproplen.                                       */
/*                                                                  */
/* Function	- Return the size of the named proposition.         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
getproplen(const char *name)
{
	int iProp;

	for (iProp = 0; bp[iProp].name[0] != 0; iProp++) {
		if (strcmp(&bp[iProp].name, name) == 0)
			return (bp[iProp].size);
	}
	return (BOOT_FAILURE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getprop.                                          */
/*                                                                  */
/* Function	- Return the address of the named proposition.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
getprop(const char *name, void *value)
{
	int iProp;

	for (iProp = 0; bp[iProp].name[0] != 0; iProp++) {
		if (strcmp(&bp[iProp].name, name) == 0) {
			bcopy(&bp[iProp].value, value, bp[iProp].size);
			return (BOOT_SUCCESS);
		}
	}
	return (BOOT_FAILURE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- setprop.                                          */
/*                                                                  */
/* Function	- Set the value of a named proposition.             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
setprop(const char *name, int size, void *value)
{
	int iProp;

	for (iProp = 0; bp[iProp].name[0] != 0; iProp++) {
		if (strcmp(&bp[iProp].name, name) == 0) {
			if (size > 0) {
				bp[iProp].size = size;
				bcopy(value, &bp[iProp].value, MIN(size, LVALUE));
			} else {
				bp[iProp].size = 0;
				_memset(&bp[iProp].value, 0, LVALUE);
			}
			return (BOOT_SUCCESS);
		}
	}

	if (iProp < NPROP) {
		strncpy(&bp[iProp].name, name, LNAME);
		bp[iProp].size = size;
		if (size > 0) 
			bcopy(value, &bp[iProp].value, MIN(size, LVALUE));
		else
			_memset(&bp[iProp].value, 0, LVALUE);
		return (BOOT_SUCCESS);
	}
	return (BOOT_FAILURE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- nextprop.                                         */
/*                                                                  */
/* Function	- Get the name of the next property in succession   */
/*		  from the standalone.                              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED*/
static char *
nextprop(char *name)
{
	int i;
	char *next;

	/*
	 * A null name is a special signal for the 1st boot property
	 */
	if (name == NULL || strlen(name) == 0) {
		return ((char *) bp[0].name);
	}

	for (i = 0; bp[i].name[0] != 0; i++) {
		if (name == bp[i].name) {
			next = bp[++i].name;
			break;
		}
	}

	if (next[0] != 0)
		return (next);
	else 
		return (NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- parse_value.                                      */
/*                                                                  */
/* Function	- Parse numeric value from a string. Understands    */
/*		  decimal, hex, octal, - and ~.                     */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
parse_value(char *p, uint64_t *retval)
{
	int adjust = 0;
	uint64_t tmp = 0;
	int digit;
	int radix = 10;

	*retval = 0;
	if (*p == '-' || *p == '~')
		adjust = *p++;

	if (*p == '0') {
		++p;
		if (*p == 0)
			return (0);
		if (*p == 'x' || *p == 'X') {
			radix = 16;
			++p;
		} else {
			radix = 8;
			++p;
		}
	}
	while (*p) {
		if ('0' <= *p && *p <= '9')
			digit = *p - '0';
		else if ('a' <= *p && *p <= 'f')
			digit = 10 + *p - 'a';
		else if ('A' <= *p && *p <= 'F')
			digit = 10 + *p - 'A';
		else
			return (-1);
		if (digit >= radix)
			return (-1);
		tmp = tmp * radix + digit;
		++p;
	}
	if (adjust == '-')
		tmp = -tmp;
	else if (adjust == '~')
		tmp = ~tmp;
	*retval = tmp;
	return (0);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- boot_prop_finish.                                 */
/*                                                                  */
/* Function	- Second part of building the table of boot prop-   */
/*		  erties. This includes values from /boot/solaris/  */
/*		  bootenv.rc (i.e. eeprom(1m) values).		    */
/*		                               		 	    */
/*		  lines look like one of:      		 	    */
/*		  ^$                           		 	    */
/*		  ^# comment until end of line 		 	    */
/*		  setprop name 'value'         		 	    */
/*		  setprop name value         		 	    */
/*		  setprop name "value"         		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
boot_prop_finish(void)
{
	int fd;
	char *line;
	int c;
	int bytes_read;
	char *name;
	int n_len;
	char *value;
	int v_len;
	char *inputdev;	/* these override the comand line if serial ports */
	char *outputdev;
	char *consoledev;
	uint64_t lvalue;

	if (kbm_debug)
		prom_printf("Opening /boot/solaris/bootenv.rc\n");
	fd = BRD_OPEN(bfs_ops, "/boot/solaris/bootenv.rc", 0);
	DBG(fd);

	line = bop_allreal(bop, MMU_PAGESIZE, MMU_PAGESIZE);
	while (fd >= 0) {

		/*
		 * get a line
		 */
		for (c = 0; ; ++c) {
			bytes_read = BRD_READ(bfs_ops, fd, line + c, 1);
			if (bytes_read == 0) {
				if (c == 0)
					goto done;
				break;
			}
			if (line[c] == '\n')
				break;
		}
		line[c] = 0;

		/*
		 * ignore comment lines
		 */
		c = 0;
		while (ISSPACE(line[c]))
			++c;
		if (line[c] == '#' || line[c] == 0)
			continue;

		/*
		 * must have "setprop " or "setprop\t"
		 */
		if (strncmp(line + c, "setprop ", 8) != 0 &&
		    strncmp(line + c, "setprop\t", 8) != 0)
			continue;
		c += 8;
		while (ISSPACE(line[c]))
			++c;
		if (line[c] == 0)
			continue;

		/*
		 * gather up the property name
		 */
		name = line + c;
		n_len = 0;
		while (line[c] && !ISSPACE(line[c]))
			++n_len, ++c;

		/*
		 * gather up the value, if any
		 */
		value = "";
		v_len = 0;
		while (ISSPACE(line[c]))
			++c;
		if (line[c] != 0) {
			value = line + c;
			while (line[c] && !ISSPACE(line[c]))
				++v_len, ++c;
		}

		if (v_len >= 2 && value[0] == value[v_len - 1] &&
		    (value[0] == '\'' || value[0] == '"')) {
			++value;
			v_len -= 2;
		}
		name[n_len] = 0;
		if (v_len > 0)
			value[v_len] = 0;
		else
			continue;

		/*
		 * ignore "boot-file" property, it's now meaningless
		 */
		if (strcmp(name, "boot-file") == 0)
			continue;

		/*
		 * If a property was explicitly set on the command line
		 * it will override a setting in bootenv.rc
		 */
		if (getproplen(name) > 0)
			continue;

		setprop(name, v_len + 1, value);
	}
done:
	if (fd >= 0)
		BRD_CLOSE(bfs_ops, fd);

	/*
	 * Check if we have to limit the boot time allocator
	 */
	if (getproplen("physmem") != -1 &&
	    getprop("physmem", line) >= 0 &&
	    parse_value(line, &lvalue) != -1) {
		if (0 < lvalue && (lvalue < physmem || physmem == 0)) {
			physmem = (pgcnt_t)lvalue;
			DBG(physmem);
		}
	}

	/*
	 * check to see if we have to override the default value of the console
	 */
	inputdev = line;
	v_len    = getproplen("input-device");
	if (v_len > 0)
		(void) getprop("input-device", inputdev);
	else
		v_len = 0;
	inputdev[v_len] = 0;

	outputdev = inputdev + v_len + 1;
	v_len     = getproplen("output-device");
	if (v_len > 0)
		(void) getprop("output-device", outputdev);
	else
		v_len = 0;
	outputdev[v_len] = 0;

	consoledev = outputdev + v_len + 1;
	v_len      = getproplen("console");
	if (v_len > 0)
		(void) getprop("console", consoledev);
	else
		v_len = 0;
	consoledev[v_len] = 0;

	if (kbm_debug) {
		value = line;
		prom_printf("\nBoot properties:\n");
		name = "";
		while ((name = nextprop(name)) != NULL) {
			prom_printf("\t0x%p %s = ", (void *)name, name);
			(void) getprop(name, value);
			v_len = getproplen(name);
			prom_printf("len=%d ", v_len);
			value[v_len] = 0;
			prom_printf("%s\n", value);
		}
	}
}

/*========================= End of Function ========================*/
