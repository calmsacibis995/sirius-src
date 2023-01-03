/*------------------------------------------------------------------*/
/* 								    */
/* Name        - kipl_kmem.c					    */
/* 								    */
/* Function    - Free storage handler for boot.                     */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - January, 2007				    	    */
/* 								    */
/*------------------------------------------------------------------*/

/*
 * Conditions on use:
 * kipl_alloc and kipl_free must not be called from interrupt level,
 * except from software interrupt level.  This is because they are
 * not reentrant, and only block out software interrupts.  They take
 * too long to block any real devices.  There is a routine
 * kipl_free_intr that can be used to free blocks at interrupt level,
 * but only up to splimp, not higher.  This is because kipl_free_intr
 * only spl's to splimp.
 *
 * Also, these routines are not that fast, so they should not be used
 * in very frequent operations (e.g. operations that happen more often
 * than, say, once every few seconds).
 */

/*
 * description:
 *	Yet another memory allocator, this one based on a method
 *	described in C.J. Stephenson, "Fast Fits", IBM Sys. Journal
 *
 *	The basic data structure is a "Cartesian" binary tree, in which
 *	nodes are ordered by ascending addresses (thus minimizing free
 *	list insertion time) and block sizes decrease with depth in the
 *	tree (thus minimizing search time for a block of a given size).
 *
 *	In other words, for any node s, letting D(s) denote
 *	the set of descendents of s, we have:
 *
 *	a. addr(D(left(s))) <  addr(s) <  addr(D(right(s)))
 *	b. len(D(left(s)))  <= len(s)  >= len(D(right(s)))
 */

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

#define	NIL		((Freehdr) 0)
#define	WORDSIZE	sizeof (int)
#define	SMALLEST_BLK	1		/* Size of smallest block */

/*
 * weight(x) is the size of a block, in bytes; or 0 if and only if x
 *	is a null pointer. It is the responsibility of kipl_alloc() and
 *	kipl_free() to keep zero-length blocks out of the arena.
 */

#define	weight(x)	((x) == NIL? 0: (x->size))
#define	nextblk(p, size) ((Dblk) ((char *)(p) + (size)))
#define	max(a, b)	((a) < (b)? (b): (a))

#undef DEBUG

#if DEBUG
# define dprintf(a,b...)	prom_printf((a), ## b)
#else
# define dprintf(a,b...)
#endif

/*
 * We need to return blocks that are on word boundaries so that callers
 * that are putting int's into the area will work.  Since we allow
 * arbitrary free'ing, we need a weight function that considers
 * free blocks starting on an odd boundary special.  Allocation is
 * aligned to 8 byte boundaries (ALIGN).
 */
#define	ALIGN		8		/* doubleword aligned .. */
#define	ALIGNMASK	(ALIGN-1)
#define	ALIGNMORE(addr)	(ALIGN - ((uintptr_t)(addr) & ALIGNMASK))

/*
 * If it is empty then weight == 0
 * If it is aligned then weight == size
 * If it is unaligned
 *	if not enough room to align then weight == 0
 *	else weight == aligned size
 */
#define	mweight(x) ((x) == NIL ? 0 : \
	((((uintptr_t)(x)->block) & ALIGNMASK) == 0 ? (x)->size : \
		(((x)->size <= ALIGNMORE((x)->block)) ? 0 : \
			(x)->size - ALIGNMORE((x)->block))))

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/bootconf.h>
#include "kipl_mem.h"

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

/*
 * The node header structure.
 *
 * To reduce storage consumption, a header block is associated with
 * free blocks only, not allocated blocks.
 * When a free block is allocated, its header block is put on
 * a free header block list.
 *
 * This creates a header space and a free block space.
 * The left pointer of a header blocks is used to chain free header
 * blocks together.
 */

typedef enum {false, true} bool;
typedef struct	freehdr	*Freehdr;
typedef struct	dblk	*Dblk;

/*
 * Description of a header for a free block
 * Only free blocks have such headers.
 */
struct 	freehdr	{
	Freehdr	left;			/* Left tree pointer */
	Freehdr	right;			/* Right tree pointer */
	Dblk	block;			/* Ptr to the data block */
	size_t	size;			/* Size of the data block */
};

/*
 * Description of a data block.
 */
struct	dblk	{
	char	data[1];		/* Addr returned to the caller */
};

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                E x t e r n a l   R e f e r e n c e s             */
/*------------------------------------------------------------------*/

void		*kipl_alloc(size_t);
void		kipl_free(void *ptr, size_t nbytes);
Freehdr		getfreehdr(void);
static bool	morecore(size_t);
void		insert(Dblk p, size_t len, Freehdr *tree);
void		freehdr(Freehdr p);
void		delete(Freehdr *p);
static void	check_need_to_free(void);
extern caddr_t	resalloc(enum RESOURCES, size_t, caddr_t, int);
extern struct bootops *bop;

/*=================== End of External References ===================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

#ifdef DEBUG
static void prtree(Freehdr, char *);
#else
# define prtree(a, b)
#endif

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

/*
 * Structure containing various info about allocated memory.
 */
#define	NEED_TO_FREE_SIZE	5
struct kmem_info {
	Freehdr	free_root;
	Freehdr	free_hdr_list;
	struct map *map;
	struct pte *pte;
	caddr_t	vaddr;
	struct need_to_free {
		caddr_t addr;
		size_t	nbytes;
	} need_to_free_list, need_to_free[NEED_TO_FREE_SIZE];
} kmem_info;

struct map *kernelmap;

#if DEBUG
static int depth = 0;
#endif

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kipl_init.                                        */
/*                                                                  */
/* Function	- Initialize kernel memory allocator.               */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
kipl_init(void)
{
	int i;
	struct need_to_free *ntf;

	dprintf("kipl_init entered\n");

	kmem_info.free_root 		 = NIL;
	kmem_info.free_hdr_list 	 = NULL;
	kmem_info.map			 = kernelmap;
	kmem_info.need_to_free_list.addr = 0;
	ntf				 = kmem_info.need_to_free;

	for (i = 0; i < NEED_TO_FREE_SIZE; i++) {
		ntf[i].addr = 0;
	}

	dprintf("kipl_init returning\n");
	prtree(kmem_info.free_root, "kipl_init");
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- insert.                                           */
/*                                                                  */
/* Function	- Insert a new node in a cartesian tree or subtree, */
/*		  placing it in the correct position with respect   */
/*		  to the existing nodes.       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
insert(Dblk p,		/* Ptr to the block to insert */
    size_t len,		/* Length of new node */
    Freehdr *tree)	/* Address of ptr to root */
{
	Freehdr x;
	Freehdr *left_hook;	/* Temp for insertion */
	Freehdr *right_hook;	/* Temp for insertion */
	Freehdr newhdr;

	/*
	 * algorithm:
	 *	Starting from the root, a binary search is made for the new
	 *	node. If this search were allowed to continue, it would
	 *	eventually fail (since there cannot already be a node at the
	 *	given address); but in fact it stops when it reaches a node in
	 *	the tree which has a length less than that of the new node (or
	 *	when it reaches a null tree pointer).  The new node is then
	 *	inserted at the root of the subtree for which the shorter node
	 *	forms the old root (or in place of the null pointer).
	 */

	x = *tree;
	/*
	 * Search for the first node which has a weight less
	 *	than that of the new node; this will be the
	 *	point at which we insert the new node.
	 */

	while (weight(x) >= len) {
		if (p < x->block)
			tree = &x->left;
		else
			tree = &x->right;
		x = *tree;
	}

	/*
	 * Perform root insertion. The variable x traces a path through
	 *	the tree, and with the help of left_hook and right_hook,
	 *	rewrites all links that cross the territory occupied
	 *	by p.  Note that this improves performance under
	 *	paging.
	 */

	newhdr = getfreehdr();
	*tree = newhdr;
	left_hook = &newhdr->left;
	right_hook = &newhdr->right;

	newhdr->left = NIL;
	newhdr->right = NIL;
	newhdr->block = p;
	newhdr->size = len;

	while (x != NIL) {
		/*
		 * Remark:
		 *	The name 'left_hook' is somewhat confusing, since
		 *	it is always set to the address of a .right link
		 *	field.  However, its value is always an address
		 *	below (i.e., to the left of) p. Similarly
		 *	for right_hook. The values of left_hook and
		 *	right_hook converge toward the value of p,
		 *	as in a classical binary search.
		 */
		if (x->block < p) {
			/*
			 * rewrite link crossing from the left
			 */
			*left_hook = x;
			left_hook = &x->right;
			x = x->right;
		} else {
			/*
			 * rewrite link crossing from the right
			 */
			*right_hook = x;
			right_hook = &x->left;
			x = x->left;
		} /* else */
	} /* while */

	*left_hook = *right_hook = NIL;		/* clear remaining hooks */

} 

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- delete.                                           */
/*                                                                  */
/* Function	- Delete a node from a cartesian tree. p is the     */
/*		  address of a pointer to the node which is to be   */
/*		  deleted.                     		 	    */
/*		                               		 	    */
/* On Entry	- *p is not null.              		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
delete(Freehdr *p)
{
	Freehdr x;
	Freehdr left_branch;	/* left subtree of deleted node */
	Freehdr right_branch;	/* right subtree of deleted node */

	/*
	 * algorithm:
	 *	The left and right sons of the node to be deleted define two
	 *	subtrees which are to be merged and attached in place of the
	 *	deleted node.  Each node on the inside edges of these two
	 *	subtrees is examined and longer nodes are placed above the
	 *	shorter ones.
	 *
	 */

	x = *p;
	left_branch = x->left;
	right_branch = x->right;

	while (left_branch != right_branch) {
		/*
		 * iterate until left branch and right branch are
		 * both NIL.
		 */
		if (weight(left_branch) >= weight(right_branch)) {
			/*
			 * promote the left branch
			 */
			*p = left_branch;
			p = &left_branch->right;
			left_branch = left_branch->right;
		} else {
			/*
			 * promote the right branch
			 */
			*p = right_branch;
			p = &right_branch->left;
			right_branch = right_branch->left;
		} /* else */
	} /* while */
	*p = NIL;
	freehdr(x);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- demote.                                           */
/*                                                                  */
/* Function	- Demote a node in a cartesian tree, if necessary,  */
/*		  to establish the required vertical ordering.	    */
/*		                               		 	    */
/* On Entry	- a. All the nodes in the tree, including the one   */
/*		     to be demoted must be correctly ordered horiz- */
/*		     ontally.                  		 	    */
/*		  b. All the nodes except the one to be demoted     */
/*		     must be correctly positioned vertically. The   */
/*		     node to be demoted may be already correctly    */
/*		     positioned vertically, or it may have a length */
/*		     which is less than that of one or both of its  */
/*		     progeny.                  		 	    */
/*		  c. *p is non-null.           		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
demote(Freehdr *p)
{
	Freehdr x;		/* addr of node to be demoted */
	Freehdr left_branch;
	Freehdr right_branch;
	size_t    wx;

	/*
	 * algorithm:
	 *	The left and right subtrees of the node to be demoted are to
	 *	be partially merged and attached in place of the demoted node.
	 *	The nodes on the inside edges of these two subtrees are
	 *	examined and the longer nodes are placed above the shorter
	 *	ones, until a node is reached which has a length no greater
	 *	than that of the node being demoted (or until a null pointer
	 *	is reached).  The node is then attached at this point, and
	 *	the remaining subtrees (if any) become its descendants.
	 *
	 */

	x = *p;
	left_branch = x->left;
	right_branch = x->right;
	wx = weight(x);

	while (weight(left_branch) > wx || weight(right_branch) > wx) {
		/*
		 * select a descendant branch for promotion
		 */
		if (weight(left_branch) >= weight(right_branch)) {
			/*
			 * promote the left branch
			 */
			*p = left_branch;
			p = &left_branch->right;
			left_branch = *p;
		} else {
			/*
			 * promote the right branch
			 */
			*p = right_branch;
			p = &right_branch->left;
			right_branch = *p;
		} /* else */
	} /* while */

	*p = x;				/* attach demoted node here */
	x->left = left_branch;
	x->right = right_branch;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kipl_alloc.                                       */
/*                                                                  */
/* Function	- Allocate a block of storage.                      */
/*		                               		 	    */
/* On Exit	- kipl_alloc returns a pointer to the allocated     */
/*		  block; a null pointer if storage could not be     */
/*		  allocated.                   		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

/*ARGSUSED1*/
void *
kipl_alloc(size_t nbytes)
{
	Freehdr a;		/* ptr to node to be allocated */
	Freehdr *p;		/* address of ptr to node */
	size_t	 left_weight;
	size_t	 right_weight;
	Freehdr left_son;
	Freehdr right_son;
	char	 *retblock;	/* Address returned to the user */

	/*
	 * algorithm:
	 *	The freelist is searched by descending the tree from the root
	 *	so that at each decision point the "better fitting" child node
	 *	is chosen (i.e., the shorter one, if it is long enough, or
	 *	the longer one, otherwise).  The descent stops when both
	 *	child nodes are too short.
	 *
	 */

	dprintf("kipl_alloc(nbytes 0x%lx)\n", nbytes);

	if (nbytes == 0) {
		return (NULL);
	}

	if (nbytes < SMALLEST_BLK) {
		prom_printf("illegal kipl_alloc call for %lx bytes\n", nbytes);
		prom_panic("kipl_alloc");
	}
	check_need_to_free();

	/*
	 * ensure that at least one block is big enough to satisfy
	 *	the request.
	 */

	if (mweight(kmem_info.free_root) <= nbytes) {
		/*
		 * the largest block is not enough.
		 */
		if (!morecore(nbytes)) {
			prom_printf("kipl_alloc failed, nbytes %lx\n", nbytes);
			prom_panic("kipl_alloc");
		}
	}

	/*
	 * search down through the tree until a suitable block is
	 *	found.  At each decision point, select the better
	 *	fitting node.
	 */

	p 	     = (Freehdr *) &kmem_info.free_root;
	a	     = *p;
	left_son     = a->left;
	right_son    = a->right;
	left_weight  = mweight(left_son);
	right_weight = mweight(right_son);

	while (left_weight >= nbytes || right_weight >= nbytes) {
		if (left_weight <= right_weight) {
			if (left_weight >= nbytes) {
				p = &a->left;
				a = left_son;
			} else {
				p = &a->right;
				a = right_son;
			}
		} else {
			if (right_weight >= nbytes) {
				p = &a->right;
				a = right_son;
			} else {
				p = &a->left;
				a = left_son;
			}
		}
		left_son = a->left;
		right_son = a->right;
		left_weight = mweight(left_son);
		right_weight = mweight(right_son);
	} /* while */

	/*
	 * allocate storage from the selected node.
	 */

	if (a->size - nbytes < SMALLEST_BLK) {
		/*
		 * not big enough to split; must leave at least
		 * a dblk's worth of space.
		 */
		retblock = a->block->data;
		delete(p);
	} else {

		/*
		 * split the node, allocating nbytes from the top.
		 *	Remember we've already accounted for the
		 *	allocated node's header space.
		 */
		Freehdr x;
		x = getfreehdr();
		if ((uintptr_t)a->block->data & ALIGNMASK) {
			size_t size;
			if (a->size <= ALIGNMORE(a->block->data))
				prom_panic("kipl_alloc: short block allocated");
			size = nbytes + ALIGNMORE(a->block->data);
			x->block = a->block;
			x->size = ALIGNMORE(a->block->data);
			x->left = a->left;
			x->right = a->right;
			/*
			 * the node pointed to by *p has become smaller;
			 *	move it down to its appropriate place in
			 *	the tree.
			 */
			*p = x;
			demote(p);
			retblock = a->block->data + ALIGNMORE(a->block->data);
			if (a->size > size) {
				kipl_free((caddr_t)nextblk(a->block, size),
				    (size_t)(a->size - size));
			}
			freehdr(a);
		} else {
			x->block = nextblk(a->block, nbytes);
			x->size = a->size - nbytes;
			x->left = a->left;
			x->right = a->right;
			/*
			 * the node pointed to by *p has become smaller;
			 *	move it down to its appropriate place in
			 *	the tree.
			 */
			*p = x;
			demote(p);
			retblock = a->block->data;
			freehdr(a);
		}
	}
	prtree(kmem_info.free_root, "kipl_alloc");

	bzero(retblock, nbytes);
	dprintf("kipl_alloc  bzero complete - returning %p\n", retblock);

	return (retblock);

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- kipl_free.                                        */
/*                                                                  */
/* Function	- Return a block to the free space tree.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
kipl_free(void *ptr, size_t nbytes)
{
	Freehdr *np;		/* For deletion from free list */
	Freehdr neighbor;	/* Node to be coalesced */
	char	 *neigh_block;	/* Ptr to potential neighbor */
	size_t	 neigh_size;	/* Size of potential neighbor */

	/*
	 * algorithm:
	 *	Starting at the root, search for and coalesce free blocks
	 *	adjacent to one given.  When the appropriate place in the
	 *	tree is found, insert the given block.
	 *
	 */

	dprintf("kipl_free (ptr %p nbytes %lx)\n", ptr, nbytes);
	prtree(kmem_info.free_root, "kipl_free");

#ifdef	lint
	neigh_block = kipl_alloc(nbytes);
	neigh_block = neigh_block;
#endif
	if (nbytes == 0) {
		return;
	}

	if (ptr == 0) {
		prom_panic("kipl_free of 0");
	}

	/*
	 * Search the tree for the correct insertion point for this
	 *	node, coalescing adjacent free blocks along the way.
	 */
	np = &kmem_info.free_root;
	neighbor = *np;
	while (neighbor != NIL) {
		neigh_block = (char *)neighbor->block;
		neigh_size = neighbor->size;
		if ((char *)ptr < neigh_block) {
			if ((char *)ptr + nbytes == neigh_block) {
				/*
				 * Absorb and delete right neighbor
				 */
				nbytes += neigh_size;
				delete(np);
			} else if ((char *)ptr + nbytes > neigh_block) {
				/*
				 * The block being freed overlaps
				 * another block in the tree.  This
				 * is bad news.
				 */
				prom_printf("kipl_free: free block overlap %p+%lx"
				    " over %p\n", (void *)ptr, nbytes,
				    (void *)neigh_block);
				prom_panic("kipl_free: free block overlap");
			} else {
				/*
				 * Search to the left
				 */
				np = &neighbor->left;
			}
		} else if ((char *)ptr > neigh_block) {
			if (neigh_block + neigh_size == ptr) {
				/*
				 * Absorb and delete left neighbor
				 */
				ptr = neigh_block;
				nbytes += neigh_size;
				delete(np);
			} else if (neigh_block + neigh_size > (char *)ptr) {
				/*
				 * This block has already been freed
				 */
				prom_panic("kipl_free block already free");
			} else {
				/*
				 * search to the right
				 */
				np = &neighbor->right;
			}
		} else {
			/*
			 * This block has already been freed
			 * as "ptr == neigh_block"
			 */
			prom_panic("kipl_free: block already free as neighbor");
			return;
		} /* else */
		neighbor = *np;
	} /* while */

	/*
	 * Insert the new node into the free space tree
	 */
	insert((Dblk) ptr, nbytes, &kmem_info.free_root);
	dprintf("exiting kipl_free\n");
	prtree(kmem_info.free_root, "kipl_free");
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- check_need_to_free.                               */
/*                                                                  */
/* Function	-                                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
check_need_to_free(void)
{
	int i;
	struct need_to_free *ntf;
	caddr_t addr;
	size_t nbytes;

again:
	ntf = &kmem_info.need_to_free_list;
	if (ntf->addr) {
		addr   = ntf->addr;
		nbytes = ntf->nbytes;
		*ntf   = *(struct need_to_free *)ntf->addr;
		kipl_free(addr, nbytes);
		goto again;
	}
	ntf = kmem_info.need_to_free;
	for (i = 0; i < NEED_TO_FREE_SIZE; i++) {
		if (ntf[i].addr) {
			addr        = ntf[i].addr;
			nbytes      = ntf[i].nbytes;
			ntf[i].addr = 0;
			kipl_free(addr, nbytes);
			goto again;
		}
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- morecore.                                         */
/*                                                                  */
/* Function	- Add a block of at least nbytes to the free space  */
/*		  tree. Free space (delimited by the static var-    */
/*		  iable ubound) is extended by an amount determ-    */
/*		  ined by rounding nbytes up to a multiple of the   */
/*		  system page size.            		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static bool
morecore(size_t nbytes)
{
	enum RESOURCES type = RES_BOOTSCRATCH;
	Dblk p;

	dprintf("morecore(nbytes 0x%lx)\n", nbytes);

	nbytes = roundup(nbytes, PAGESIZE);

	p = (Dblk) resalloc(type, nbytes, (caddr_t)0, 0);
	if (p == 0) {
		return (false);
	}
	kipl_free((caddr_t)p, nbytes);

	dprintf("morecore() returing, p = %p\n", p);
	return (true);

}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- getfreehdr.                                       */
/*                                                                  */
/* Function	- Get a free block header. There is a list of avail-*/
/*		  able free block headers. When the list is empty,  */
/*		  allocate another page.       		 	    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

Freehdr
getfreehdr(void)
{
	Freehdr	r;
	int	n = 0;

	if (kmem_info.free_hdr_list != NIL) {
		r = kmem_info.free_hdr_list;
		kmem_info.free_hdr_list = kmem_info.free_hdr_list->left;
	} else {
		r = (Freehdr)resalloc(RES_BOOTSCRATCH, PAGESIZE, (caddr_t)0, 0);
		if (r == 0) {
			prom_panic("getfreehdr");
		}
		for (n = 1; n < PAGESIZE / sizeof (*r); n++) {
			freehdr(&r[n]);
		}
	}

	return (r);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- freehdr.                                          */
/*                                                                  */
/* Function	- Free a free block header.                         */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
freehdr(Freehdr p)
{

	p->left  = kmem_info.free_hdr_list;
	p->right = NIL;
	p->block = NULL;
	kmem_info.free_hdr_list = p;
}

/*========================= End of Function ========================*/

#ifdef DEBUG
/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- prtree.                                           */
/*                                                                  */
/* Function	- Print the tree.                                   */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
prtree(Freehdr p, char *cp)
{
	int n;
	if (depth == 0) {
		prom_printf("prtree(p %p cp %s)\n", p, cp);
	}
	if (p != NIL) {
		depth++;
		prtree(p->left, (char *)NULL);
		depth--;

		for (n = 0; n < depth; n++) {
			prom_printf("   ");
		}
		prom_printf(
		    "(%p): (left = %p, right = %p, block = %p, size = %lx)\n",
			p, p->left, p->right, p->block, p->size);

		depth++;
		prtree(p->right, (char *)NULL);
		depth--;
	}
}

/*========================= End of Function ========================*/
#endif /* DEBUG */
