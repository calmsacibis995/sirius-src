/*
 * Copyright (c) 1999 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Cylink Corporation ? 1998
 * 
 * This software is licensed by Cylink to the Internet Software Consortium to
 * promote implementation of royalty free public key cryptography within IETF
 * standards.  Cylink wishes to expressly thank the contributions of Dr.
 * Martin Hellman, Whitfield Diffie, Ralph Merkle and Stanford University for
 * their contributions to Internet Security.  In accordance with the terms of
 * this license, ISC is authorized to distribute and sublicense this software
 * for the practice of IETF standards.  
 *
 * The software includes BigNum, written by Colin Plumb and licensed by Philip
 * R. Zimmermann for royalty free use and distribution with Cylink's
 * software.  Use of BigNum as a stand alone product or component is
 * specifically prohibited.
 *
 * Disclaimer of All Warranties. THIS SOFTWARE IS BEING PROVIDED "AS IS",
 * WITHOUT ANY EXPRESSED OR IMPLIED WARRANTY OF ANY KIND WHATSOEVER. IN
 * PARTICULAR, WITHOUT LIMITATION ON THE GENERALITY OF THE FOREGOING, CYLINK
 * MAKES NO REPRESENTATION OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
 *
 * Cylink or its representatives shall not be liable for tort, indirect,
 * special or consequential damages such as loss of profits or loss of
 * goodwill from the use or inability to use the software for any purpose or
 * for any reason whatsoever.
 *
 * EXPORT LAW: Export of the Foundations Suite may be subject to compliance
 * with the rules and regulations promulgated from time to time by the Bureau
 * of Export Administration, United States Department of Commerce, which
 * restrict the export and re-export of certain products and technical data.
 * If the export of the Foundations Suite is controlled under such rules and
 * regulations, then the Foundations Suite shall not be exported or
 * re-exported, directly or indirectly, (a) without all export or re-export
 * licenses and governmental approvals required by any applicable laws, or (b)
 * in violation of any applicable prohibition against the export or re-export
 * of any part of the Foundations Suite. All export licenses for software
 * containing the Foundations Suite are the sole responsibility of the licensee.
 */
 
/*
 * lbn68020.h - 32-bit bignum primitives for the 68020 (or 683xx) processors.
 *
 * These primitives use little-endian word order.
 * (The order of bytes within words is irrelevant.)
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#define BN_LITTLE_ENDIAN 1

typedef unsigned long bnword32
#define BNWORD32 bnword32

bnword32 lbnSub1_32(bnword32 *num, unsigned len, bnword32 borrow);
bnword32 lbnAdd1_32(bnword32 *num, unsigned len, bnword32 carry);
void lbnMulN1_32(bnword32 *out, bnword32 const *in, unsigned len, bnword32 k);
bnword32
lbnMulAdd1_32(bnword32 *out, bnword32 const *in, unsigned len, bnword32 k);
bnword32
lbnMulSub1_32(bnword32 *out, bnword32 const *in, unsigned len, bnword32 k);
bnword32 lbnDiv21_32(bnword32 *q, bnword32 nh, bnword32 nl, bnword32 d);
unsigned lbnModQ_32(bnword32 const *n, unsigned len, unsigned d);

/* #define the values to exclude the C versions */
#define lbnSub1_32 lbnSub1_32
#define lbnAdd1_32 lbnAdd1_32
#define lbnMulN1_32 lbnMulN1_32
#define lbnMulAdd1_32 lbnMulAdd1_32
#define lbnMulSub1_32 lbnMulSub1_32
#define lbnDiv21_32 lbnDiv21_32
#define lbnModQ_32 lbnModQ_32
