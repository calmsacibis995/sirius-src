/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
 *
 * Copyright 2008 Sine Nomine Associates.
 * All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/asm_linkage.h>
#include <sn1_brand.h>

#define	RVAL2_FLAG	0x100

#ifdef __s390x
#define	PIC_SETUP(r)						\
	bras	0,8f;						\
8:;								\
	agr	r,0;						
#else
#define	PIC_SETUP(r)						\
	bras	0,8f;						\
8:;								\
	ar	r,0;						
#endif

/*
 * Translate a global symbol into an address.  The resulting address
 * is returned in the first register parameter.  The second register
 * is just for scratch space.
 */
#ifdef __s390x
#define	GET_SYM_ADDR(r1, r2, name)		\
	PIC_SETUP(r1)				;\
	larl	r2,name				;\
	lg	r1,0(r1,r2)			;
#else
#define	GET_SYM_ADDR(r1, r2, name)		\
	PIC_SETUP(r1)				;\
	l	r1,name(r1)			;
#endif

#if defined(lint)

void
sn1_handler(void)
{
}

#else	/* lint */

	.section	".text"

#ifdef __s390x
	/*
	 * When we get here, %r0 should contain the system call and
	 * %r1 should contain the address immediately after the trap
	 * instruction.
	 */
	ENTRY_NP(sn1_handler)
	stmg	%r6,%r14,48(%r15)
	lgr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	stg	%r14,0(%r15)
	

	/*
	 * Find the base address of the jump table, index into it based
	 * on the system call number, and extract the address of the proper
	 * emulation routine.
	 */
	sllg	%r7,%r0,(1+CLONGSHIFT)		/* Each entry has 2 longs */

	GET_SYM_ADDR(%r8, %r9, sn1_sysent_table)
	
	lgr	%r10,%r7
	agr	%r10,%r2			/* Index to proper entry  */
	lg	%r11,CPTRSIZE(%r10)		/* Save NARGS             */
	lg	%r10,0(%r10)			/* Emulation address	  */
	basr	%r14,%r10			/* Go call		  */

	/*
	 * Check for two-return syscall. 
	 */
	nill	%r11,RVAL2_FLAG			
	jz	1f

	/*
	 * In 64-bit code, the syscall emulation routine returns the values
	 * in a single 64-bit register.  We split it into two 32-bit values.
	 */
	lgr	%r3,%r2
	nilf	%r3,0xffffffff
	srlg	%r2,%r2,32

1:
	/*
	 * If %r2 >= 0, it means the call completed successfully and %r2 is
	 * the proper return value.  Otherwise, %r2 contains -errno.  In
	 * the event of an error, we need to set %r0 to -1 (which is
	 * the kernel's indication of failure to libc) and set %r2 to the
	 * positive errno.
	 */
	lghi 	%r0,0			/* Assume all was well      */
	ltgr	%r2,%r2			/* %r2 >= 0?		    */
	jnm	2f			/* No... Skip		    */

	lghi	%r0,-1			/* Set error indication     */
	lpgr	%r2,%r2			/* Set +ve errno	    */
2:
	aghi	%r15,SA(MINFRAME)
	lmg	%r6,%r14,48(%r15)
	br	%r14
	SET_SIZE(sn1_handler)
#else
	/*
	 * When we get here, %r0 should contain the system call and
	 * %r1 should contain the address immediately after the trap
	 * instruction.
	 */
	ENTRY_NP(sn1_handler)
	stm	%r6,%r14,24(%r15)
	lr	%r14,%r15
	aghi	%r15,-SA(MINFRAME)
	st	%r14,0(%r15)
	

	/*
	 * Find the base address of the jump table, index into it based
	 * on the system call number, and extract the address of the proper
	 * emulation routine.
	 */
	lr	%r7,%r0
	sll	%r7,(1+CLONGSHIFT)		/* Each entry has 2 longs */

	GET_SYM_ADDR(%r8, %r9, sn1_sysent_table)
	
	lr	%r10,%r7
	ar	%r10,%r2			/* Index to proper entry  */
	l	%r11,CPTRSIZE(%r10)		/* Save NARGS             */
	l	%r10,0(%r10)			/* Emulation address	  */
	basr	%r14,%r10			/* Go call		  */

	/*
	 * Check for two-return syscall. 
	 */
	nill	%r11,RVAL2_FLAG			
	jz	1f

1:
	/*
	 * If %r2 >= 0, it means the call completed successfully and %r2 is
	 * the proper return value.  Otherwise, %r2 contains -errno.  In
	 * the event of an error, we need to set %r0 to -1 (which is
	 * the kernel's indication of failure to libc) and set %r2 to the
	 * positive errno.
	 */
	lhi 	%r0,0			/* Assume all was well      */
	ltr	%r2,%r2			/* %r2 >= 0?		    */
	jnm	2f			/* No... Skip		    */

	lhi	%r0,-1			/* Set error indication     */
	lpr	%r2,%r2			/* Set +ve errno	    */
2:
	ahi	%r15,SA(MINFRAME)
	lm	%r6,%r14,24(%r15)
	br	%r14
	SET_SIZE(sn1_handler)
#endif
	
	.section        ".data"
	.global sn1_sysent_table
	.align	CLONGSIZE

	.global sn1_unimpl
#ifdef	__s390x
#define	WORD	.quad 
#else
#define	WORD	.word
#endif

#define NOSYS	\
	WORD	sn1_unimpl	;\
	WORD	0

#define	EMULATE(name, args)	\
	.global	name		;\
	WORD	name		;\
	WORD	args

sn1_sysent_table:
	.type   sn1_sysent_table, @object
	.size   sn1_sysent_table, (2 * 256 * CLONGSIZE)
	.align	CLONGSIZE
	NOSYS					/*  0 */
	NOSYS					/*  1 */
	NOSYS					/*  2 */
	NOSYS					/*  3 */
	NOSYS					/*  4 */
	NOSYS					/*  5 */
	NOSYS					/*  6 */
	NOSYS					/*  7 */
	NOSYS					/*  8 */
	NOSYS					/*  9 */
	NOSYS					/* 10 */
	NOSYS					/* 11 */
	NOSYS					/* 12 */
	NOSYS					/* 13 */
	NOSYS					/* 14 */
	NOSYS					/* 15 */
	NOSYS					/* 16 */
	NOSYS					/* 17 */
	NOSYS					/* 18 */
	NOSYS					/* 19 */
	NOSYS					/* 20 */
	NOSYS					/* 21 */
	NOSYS					/* 22 */
	NOSYS					/* 23 */
	NOSYS					/* 24 */
	NOSYS					/* 25 */
	NOSYS					/* 26 */
	NOSYS					/* 27 */
	NOSYS					/* 28 */
	NOSYS					/* 29 */
	NOSYS					/* 30 */
	NOSYS					/* 31 */
	NOSYS					/* 32 */
	NOSYS					/* 33 */
	NOSYS					/* 34 */
	NOSYS					/* 35 */
	NOSYS					/* 36 */
	NOSYS					/* 37 */
	NOSYS					/* 38 */
	NOSYS					/* 39 */
	NOSYS					/* 40 */
	NOSYS					/* 41 */
	NOSYS					/* 42 */
	NOSYS					/* 43 */
	NOSYS					/* 44 */
	NOSYS					/* 45 */
	NOSYS					/* 46 */
	NOSYS					/* 47 */
	NOSYS					/* 48 */
	NOSYS					/* 49 */
	NOSYS					/* 50 */
	NOSYS					/* 51 */
	NOSYS					/* 52 */
	NOSYS					/* 53 */
	NOSYS					/* 54 */
	NOSYS					/* 55 */
	NOSYS					/* 56 */
	NOSYS					/* 57 */
	NOSYS					/* 58 */
	NOSYS					/* 59 */
	NOSYS					/* 60 */
	NOSYS					/* 61 */
	NOSYS					/* 62 */
	NOSYS					/* 63 */
	NOSYS					/* 64 */
	NOSYS					/* 65 */
	NOSYS					/* 66 */
	NOSYS					/* 67 */
	NOSYS					/* 68 */
	NOSYS					/* 69 */
	NOSYS					/* 70 */
	NOSYS					/* 71 */
	NOSYS					/* 72 */
	NOSYS					/* 73 */
	NOSYS					/* 74 */
	NOSYS					/* 75 */
	NOSYS					/* 76 */
	NOSYS					/* 77 */
	NOSYS					/* 78 */
	NOSYS					/* 79 */
	NOSYS					/* 80 */
	NOSYS					/* 81 */
	NOSYS					/* 82 */
	NOSYS					/* 83 */
	NOSYS					/* 84 */
	NOSYS					/* 85 */
	NOSYS					/* 86 */
	NOSYS					/* 87 */
	NOSYS					/* 88 */
	NOSYS					/* 89 */
	NOSYS					/* 90 */
	NOSYS					/* 91 */
	NOSYS					/* 92 */
	NOSYS					/* 93 */
	NOSYS					/* 94 */
	NOSYS					/* 95 */
	NOSYS					/* 96 */
	NOSYS					/* 97 */
	NOSYS					/* 98 */
	NOSYS					/* 99 */
	NOSYS					/* 100 */
	NOSYS					/* 101 */
	NOSYS					/* 102 */
	NOSYS					/* 103 */
	NOSYS					/* 104 */
	NOSYS					/* 105 */
	NOSYS					/* 106 */
	NOSYS					/* 107 */
	NOSYS					/* 108 */
	NOSYS					/* 109 */
	NOSYS					/* 110 */
	NOSYS					/* 111 */
	NOSYS					/* 112 */
	NOSYS					/* 113 */
	NOSYS					/* 114 */
	NOSYS					/* 115 */
	NOSYS					/* 116 */
	NOSYS					/* 117 */
	NOSYS					/* 118 */
	NOSYS					/* 119 */
	NOSYS					/* 120 */
	NOSYS					/* 121 */
	NOSYS					/* 122 */
	NOSYS					/* 123 */
	NOSYS					/* 124 */
	NOSYS					/* 125 */
	NOSYS					/* 126 */
	NOSYS					/* 127 */
	NOSYS					/* 128 */
	NOSYS					/* 129 */
	NOSYS					/* 130 */
	NOSYS					/* 131 */
	NOSYS					/* 132 */
	NOSYS					/* 133 */
	NOSYS					/* 134 */
	EMULATE(sn1_uname, 1)			/* 135 */
	NOSYS					/* 136 */
	NOSYS					/* 137 */
	NOSYS					/* 138 */
	NOSYS					/* 139 */
	NOSYS					/* 140 */
	NOSYS					/* 141 */
	NOSYS					/* 142 */
	NOSYS					/* 143 */
	NOSYS					/* 144 */
	NOSYS					/* 145 */
	NOSYS					/* 146 */
	NOSYS					/* 147 */
	NOSYS					/* 148 */
	NOSYS					/* 149 */
	NOSYS					/* 150 */
	NOSYS					/* 151 */
	NOSYS					/* 152 */
	NOSYS					/* 153 */
	NOSYS					/* 154 */
	NOSYS					/* 155 */
	NOSYS					/* 156 */
	NOSYS					/* 157 */
	NOSYS					/* 158 */
	NOSYS					/* 159 */
	NOSYS					/* 160 */
	NOSYS					/* 161 */
	NOSYS					/* 162 */
	NOSYS					/* 163 */
	NOSYS					/* 164 */
	NOSYS					/* 165 */
	NOSYS					/* 166 */
	NOSYS					/* 167 */
	NOSYS					/* 168 */
	NOSYS					/* 169 */
	NOSYS					/* 170 */
	NOSYS					/* 171 */
	NOSYS					/* 172 */
	NOSYS					/* 173 */
	NOSYS					/* 174 */
	NOSYS					/* 175 */
	NOSYS					/* 176 */
	NOSYS					/* 177 */
	NOSYS					/* 178 */
	NOSYS					/* 179 */
	NOSYS					/* 180 */
	NOSYS					/* 181 */
	NOSYS					/* 182 */
	NOSYS					/* 183 */
	NOSYS					/* 184 */
	NOSYS					/* 185 */
	NOSYS					/* 186 */
	NOSYS					/* 187 */
	NOSYS					/* 188 */
	NOSYS					/* 189 */
	NOSYS					/* 190 */
	NOSYS					/* 191 */
	NOSYS					/* 192 */
	NOSYS					/* 193 */
	NOSYS					/* 194 */
	NOSYS					/* 195 */
	NOSYS					/* 196 */
	NOSYS					/* 197 */
	NOSYS					/* 198 */
	NOSYS					/* 199 */
	NOSYS					/* 200 */
	NOSYS					/* 201 */
	NOSYS					/* 202 */
	NOSYS					/* 203 */
	NOSYS					/* 204 */
	NOSYS					/* 205 */
	NOSYS					/* 206 */
	NOSYS					/* 207 */
	NOSYS					/* 208 */
	NOSYS					/* 209 */
	NOSYS					/* 210 */
	NOSYS					/* 211 */
	NOSYS					/* 212 */
	NOSYS					/* 213 */
	NOSYS					/* 214 */
	NOSYS					/* 215 */
	NOSYS					/* 216 */
	NOSYS					/* 217 */
	NOSYS					/* 218 */
	NOSYS					/* 219 */
	NOSYS					/* 220 */
	NOSYS					/* 221 */
	NOSYS					/* 222 */
	NOSYS					/* 223 */
	NOSYS					/* 224 */
	NOSYS					/* 225 */
	NOSYS					/* 226 */
	NOSYS					/* 227 */
	NOSYS					/* 228 */
	NOSYS					/* 229 */
	NOSYS					/* 230 */
	NOSYS					/* 231 */
	NOSYS					/* 232 */
	NOSYS					/* 233 */
	NOSYS					/* 234 */
	NOSYS					/* 235 */
	NOSYS					/* 236 */
	NOSYS					/* 237 */
	NOSYS					/* 238 */
	NOSYS					/* 239 */
	NOSYS					/* 240 */
	NOSYS					/* 241 */
	NOSYS					/* 242 */
	NOSYS					/* 243 */
	NOSYS					/* 244 */
	NOSYS					/* 245 */
	NOSYS					/* 246 */
	NOSYS					/* 247 */
	NOSYS					/* 248 */
	NOSYS					/* 249 */
	NOSYS					/* 250 */
	NOSYS					/* 251 */
	NOSYS					/* 252 */
	NOSYS					/* 253 */
	NOSYS					/* 254 */
	NOSYS					/* 255 */

#endif	/* lint */
