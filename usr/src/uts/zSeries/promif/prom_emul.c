/*------------------------------------------------------------------*/
/* 								    */
/* Name        - promif_emul.					    */
/* 								    */
/* Function    - PROM emulation routines.                           */
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

#include <sys/promif.h>
#include <sys/promimpl.h>
#include <sys/prom_emul.h>
#include <sys/obpdefs.h>
#include <sys/sunddi.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/


/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/


/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

static prom_node_t *promif_find_node(pnode_t nodeid);
static int getproplen(prom_node_t *pnp, char *name);
static void *getprop(prom_node_t *pnp, char *name);
static void promif_create_children(prom_node_t *, dev_info_t *);



/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

static prom_node_t *promif_top;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- promif_create_prop.                               */
/*                                                                  */
/* Function	- Create a proposition on a given node.             */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
promif_create_prop(prom_node_t *pnp, char *name, void *val, int len, int flags)
{
	struct prom_prop *p, *q;

	q = kmem_zalloc(sizeof (*q), KM_SLEEP);
	q->pp_name = kmem_zalloc(strlen(name) + 1, KM_SLEEP);
	(void) strcpy(q->pp_name, name);
	q->pp_val = kmem_alloc(len, KM_SLEEP);
	q->pp_len = len;
	switch (flags) {
	case DDI_PROP_TYPE_INT:
	case DDI_PROP_TYPE_INT64:
		/*
		 * Technically, we need byte-swapping to conform to 1275.
		 * However, the old x86 prom simulator used little endian
		 * representation, so we don't swap here either.
		 *
		 * NOTE: this is inconsistent with ddi_prop_lookup_*()
		 * which does byte-swapping when looking up prom properties.
		 * Since all kernel nodes are SID nodes, drivers no longer
		 * access PROM properties on x86.
		 */
	default:	/* no byte swapping */
		(void) bcopy(val, q->pp_val, len);
		break;
	}

	if (pnp->pn_propp == NULL) {
		pnp->pn_propp = q;
		return;
	}

	for (p = pnp->pn_propp; p->pp_next != NULL; p = p->pp_next)
		/* empty */;

	p->pp_next = q;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- promif_create_node.                               */
/*                                                                  */
/* Function	- Create a node for propositions.                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static prom_node_t *
promif_create_node(dev_info_t *dip)
{
	prom_node_t *pnp;
	ddi_prop_t *hwprop;
	char *nodename;

	pnp = kmem_zalloc(sizeof (prom_node_t), KM_SLEEP);
	pnp->pn_nodeid = DEVI(dip)->devi_nodeid;

	hwprop = DEVI(dip)->devi_hw_prop_ptr;
	while (hwprop != NULL) {
		/* need to encode to proper endianness */
		promif_create_prop(pnp, hwprop->prop_name, hwprop->prop_val,
		    hwprop->prop_len, hwprop->prop_flags & DDI_PROP_TYPE_MASK);
		hwprop = hwprop->prop_next;
	}
	nodename = ddi_node_name(dip);
	promif_create_prop(pnp, "name", nodename, strlen(nodename) + 1,
	    DDI_PROP_TYPE_STRING);

	return (pnp);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- promif_create_peers.                              */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
promif_create_peers(prom_node_t *pnp, dev_info_t *dip)
{
	dev_info_t *ndip = ddi_get_next_sibling(dip);

	while (ndip) {
		pnp->pn_sibling = promif_create_node(ndip);
		promif_create_children(pnp->pn_sibling, ndip);
		pnp = pnp->pn_sibling;
		ndip = ddi_get_next_sibling(ndip);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- promif_create_children.                           */
/*                                                                  */
/* Function	- Creat a child node.                               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
promif_create_children(prom_node_t *pnp, dev_info_t *dip)
{
	dev_info_t *cdip = ddi_get_child(dip);

	while (cdip) {
		pnp->pn_child = promif_create_node(cdip);
		promif_create_peers(pnp->pn_child, cdip);
		pnp = pnp->pn_child;
		cdip = ddi_get_child(cdip);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- promif_create_device_tree.                        */
/*                                                                  */
/* Function	- Create a device tree and its children.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
promif_create_device_tree(void)
{
	promif_top = promif_create_node(ddi_root_node());
	promif_create_children(promif_top, ddi_root_node());
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- find_node_work.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static prom_node_t *
find_node_work(prom_node_t *pnp, pnode_t n)
{
	prom_node_t *qnp;

	if (pnp->pn_nodeid == n)
		return (pnp);

	if (pnp->pn_child)
		if ((qnp = find_node_work(pnp->pn_child, n)) != NULL)
			return (qnp);

	if (pnp->pn_sibling)
		if ((qnp = find_node_work(pnp->pn_sibling, n)) != NULL)
			return (qnp);

	return (NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- promif_find_node.                                 */
/*                                                                  */
/* Function	- Find a node with a specified nodeid.              */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static prom_node_t *
promif_find_node(pnode_t nodeid)
{
	if (nodeid == OBP_NONODE)
		return (promif_top);

	if (promif_top == NULL)
		return (NULL);

	return (find_node_work(promif_top, nodeid));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- promif_nextnode.                                  */
/*                                                                  */
/* Function	- Return the node after the specified nodeid.       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

pnode_t
promif_nextnode(pnode_t nodeid)
{
	prom_node_t *pnp;

	/*
	 * Note: next(0) returns the root node
	 */
	pnp = promif_find_node(nodeid);
	if (pnp && (nodeid == OBP_NONODE))
		return (pnp->pn_nodeid);
	if (pnp && pnp->pn_sibling)
		return (pnp->pn_sibling->pn_nodeid);

	return (OBP_NONODE);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- promif_childnode.                                 */
/*                                                                  */
/* Function	- Return the child node of a specified parent.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

pnode_t
promif_childnode(pnode_t nodeid)
{
	prom_node_t *pnp;

	pnp = promif_find_node(nodeid);
	if (pnp && pnp->pn_child)
		return (pnp->pn_child->pn_nodeid);

	return (OBP_NONODE);
}


/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getproplen.                                       */
/*                                                                  */
/* Function	- Retrieve a PROM property (len and value).         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static int
getproplen(prom_node_t *pnp, char *name)
{
	struct prom_prop *propp;

	for (propp = pnp->pn_propp; propp != NULL; propp = propp->pp_next)
		if (strcmp(propp->pp_name, name) == 0)
			return (propp->pp_len);

	return (-1);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- promif_getproplen.                                */
/*                                                                  */
/* Function	- Return the length of a specified property on a    */
/*		  given node.                  		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
promif_getproplen(pnode_t nodeid, char *name)
{
	prom_node_t *pnp;

	pnp = promif_find_node(nodeid);
	if (pnp == NULL)
		return (-1);

	return (getproplen(pnp, name));
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getprop.                                          */
/*                                                                  */
/* Function	- Return the value of a property of a specific node.*/
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void *
getprop(prom_node_t *pnp, char *name)
{
	struct prom_prop *propp;

	for (propp = pnp->pn_propp; propp != NULL; propp = propp->pp_next)
		if (strcmp(propp->pp_name, name) == 0)
			return (propp->pp_val);

	return (NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- promif_getprop.                                   */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

int
promif_getprop(pnode_t nodeid, char *name, void *value)
{
	prom_node_t *pnp;
	void *v;
	int len;

	pnp = promif_find_node(nodeid);
	if (pnp == NULL)
		return (-1);

	len = getproplen(pnp, name);
	if (len > 0) {
		v = getprop(pnp, name);
		bcopy(v, value, len);
	}
	return (len);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- nextprop.                                         */
/*                                                                  */
/* Function	- Return the value of the next proposition.         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static char *
nextprop(prom_node_t *pnp, char *name)
{
	struct prom_prop *propp;

	/*
	 * getting next of NULL or a null string returns the first prop name
	 */
	if (name == NULL || *name == '\0')
		if (pnp->pn_propp)
			return (pnp->pn_propp->pp_name);

	for (propp = pnp->pn_propp; propp != NULL; propp = propp->pp_next)
		if (strcmp(propp->pp_name, name) == 0)
			if (propp->pp_next)
				return (propp->pp_next->pp_name);

	return (NULL);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- promif_nextprop.                                  */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

char *
promif_nextprop(pnode_t nodeid, char *name, char *next)
{
	prom_node_t *pnp;
	char *s;

	next[0] = '\0';

	pnp = promif_find_node(nodeid);
	if (pnp == NULL)
		return (NULL);

	s = nextprop(pnp, name);
	if (s == NULL)
		return (next);

	(void) strcpy(next, s);
	return (next);
}

/*========================= End of Function ========================*/
