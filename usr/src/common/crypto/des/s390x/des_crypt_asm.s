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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * Unified version for both position independent and non position independent
 * for both v8plus and v9
 * compile with:
 *
 * cw -c des_crypt_asm.s     or
 * for kernel use (no -KPIC)
 *
 * and with
 *
 * cw -c -KPIC -DPIC des_crypt_asm.s     or
 * cw -c -KPIC -DPIC des_crypt_asm.s
 * for  .so  use
 *
 * EXPORT DELETE START
 *
 * The tables were generated by a C program, compiled into the C version 
 * of this function, from which a .s was generated by the C compiler and
 * that .s was used as a starting point for this one, in particular for
 * the data definitions. It is important, though that the tables and
 * the code both remain in the text section and in this order, otherwise,
 * at least on UltraSparc-II processors, collisions in the E-cache are
 * highly probable between the code and the data it is using which can
 * result in up to 40% performance loss
 *
 * For a description of the DES algithm, see NIST publication FIPS PUB 46-3
 *
 * In this implementation, the 16 rounds of DES are carried out by unrolling
 * a loop that computes two rounds. For those 2 rounds, the two parts of
 * the intermediate variable (L and R in the FIPS pub) are kept in their
 * extended forms (i.e. in the one after applying the transformation E),
 * with the appropriate bits repeated so that bits needed for the S-box 
 * lookups are in consecutive positions. So the bits of the L (or R)
 * variable appear in the following order (X represents a bit that is not
 * from L (R), these bits are always 0):
 * 32  1  2  3  4  5  X  X   X  X  X  X  X  X  4  5
 *  6  7  8  9  8  9 10 11  12 13 12 13 14 15 16 17
 * 16 17 18 19 20 21  X  X   X  X  X 20 21 22 23 24
 * 25 24 25 26 27 28 29 28  29 30 31 32  1  X  X  X
 * This arrangement makes it possible that 3 of the 8 S-box indices
 * can be extracted by a single instruction: srlx by 55 for the S1 index,
 * srl by 23 for the S5 index and and by 0x1f80 for the S8 index. The rest
 * of the indices requires two operations, a shift and an and.
 * The tables for the S-boxes are computed in such a way that when or-ed
 * together, they give the result of the S-box, P and E computations.
 * Also, the key schedule bits are computed to follow this bit-scheme.
 * The initial permutation tables are also computed to produce this
 * bit distribution and the final permutation works from these, too.
 *
 * The end of each round is overlapped with the beginning of the next
 * one since after the first 6 S-box lookups all the bits necessary
 * for one S-box lookup in the next round can be computed (by xor-ing
 * the next key schedule item to the partially computed next R).
 *
 * EXPORT DELETE END
 */

#if defined(lint) || defined(__lint)
	/* LINTED */
	/* Nothing to be linted in this file, its pure assembly source */
#else	/* lint || __lint */ 

	.file	"encrypt_asm.S"

	.section	".text",@alloc
	.align	32

/* EXPORT DELETE START */

//
// CONSTANT POOL
//

	.section	".text",@alloc,@execinstr
/* 000000	   0 */		.align	32
/* 000000	     */		.skip	32
//
// SUBROUTINE des_crypt_impl
//
// OFFSET    SOURCE LINE	LABEL	INSTRUCTION

.global des_crypt_impl

// uint64_t des_crypt_impl(uint64_t *ks, uint64_t block, int one_or_three); 
//
// ks is the key schedule, en/decryption is differentiated by computing
//    an encryption key schedule for encryption and the reverse of it
//    for decryption (for DES, 16 entries, for triple-DES, 48 entries)
// block is the 64-bit block to en/decrypt
// one_or_three is 1 for DES and 3 for triple-DES
	
	des_crypt_impl:

/* S390X FIXME - use the KMAC instruction to accomplish this */
	lghi	%r2,0
	br	%r14
	.type	des_crypt_impl,2
	.size	des_crypt_impl,(.-des_crypt_impl)
/* EXPORT DELETE END */

#endif	/* lint || __lint */