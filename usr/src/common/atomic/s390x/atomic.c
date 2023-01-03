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
 *                                            
 * Copyright 2008 Sine Nomine Associates.    
 * All rights reserved.                     
 * Use is subject to license terms.        
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */


#if defined(_KERNEL)
	/*
	 * Legacy kernel interfaces; they will go away (eventually).
	 */
	#pragma weak cas8 = atomic_cas_8
	#pragma weak cas32 = atomic_cas_32
	#pragma weak cas64 = atomic_cas_64
	#pragma weak caslong = atomic_cas_ulong
	#pragma weak casptr = atomic_cas_ptr
	#pragma weak atomic_and_long = atomic_and_ulong
	#pragma weak atomic_or_long = atomic_or_ulong
	#pragma weak swapl = atomic_swap_32

#endif

#include <sys/atomic.h>

inline
void
atomic_inc_8(volatile uint8_t *target)
{ __sync_fetch_and_add(target, 1); }

inline
void
atomic_inc_uchar(volatile uchar_t *target)
{ __sync_fetch_and_add(target, 1); }

inline
void
atomic_inc_16(volatile uint16_t *target)
{ __sync_fetch_and_add(target, 1); }

inline
void
atomic_inc_ushort(volatile ushort_t *target)
{ __sync_fetch_and_add(target, 1); }

void
atomic_inc_32(volatile uint32_t *target)
{ __sync_fetch_and_add(target, 1); }

inline
void
atomic_inc_uint(volatile uint_t *target)
{ __sync_fetch_and_add(target, 1); }

inline
void
atomic_inc_ulong(volatile ulong_t *target)
{ __sync_fetch_and_add(target, 1); }

inline
void
atomic_inc_64(volatile uint64_t *target)
{ __sync_fetch_and_add(target, 1); }

inline
void
atomic_dec_8(volatile uint8_t *target)
{ __sync_fetch_and_sub(target, 1); }

inline
void
atomic_dec_uchar(volatile uchar_t *target)
{ __sync_fetch_and_sub(target, 1); }

inline
void
atomic_dec_16(volatile uint16_t *target)
{ __sync_fetch_and_sub(target, 1); }

inline
void
atomic_dec_ushort(volatile ushort_t *target)
{ __sync_fetch_and_sub(target, 1); }

inline
void
atomic_dec_32(volatile uint32_t *target)
{ __sync_fetch_and_sub(target, 1); }

inline
void
atomic_dec_uint(volatile uint_t *target)
{ __sync_fetch_and_sub(target, 1); }

inline
void
atomic_dec_ulong(volatile ulong_t *target)
{ __sync_fetch_and_sub(target, 1); }

inline
void
atomic_dec_64(volatile uint64_t *target)
{ __sync_fetch_and_sub(target, 1); }

inline
void
atomic_add_8(volatile uint8_t *target, int8_t value)
{ __sync_fetch_and_add(target, value); }

inline
void
atomic_add_char(volatile uchar_t *target, signed char value)
{ __sync_fetch_and_add(target, value); }

inline
void
atomic_add_16(volatile uint16_t *target, int16_t delta)
{ __sync_fetch_and_add(target, delta); }

inline
void
atomic_add_ushort(volatile ushort_t *target, short value)
{ __sync_fetch_and_add(target, value); }

inline
void
atomic_add_32(volatile uint32_t *target, int32_t delta)
{ __sync_fetch_and_add(target, delta); }

inline
void
atomic_add_ptr(volatile void *target, ssize_t value)
{ *(caddr_t *)target += value; }

inline
void
atomic_add_long(volatile ulong_t *target, long delta)
{ __sync_fetch_and_add(target, delta); }

inline
void
atomic_add_64(volatile uint64_t *target, int64_t delta)
{ __sync_fetch_and_add(target, delta); }

inline
void
atomic_or_8(volatile uint8_t *target, uint8_t bits)
{ __sync_fetch_and_or(target, bits); }

inline
void
atomic_or_uchar(volatile uchar_t *target, uchar_t bits)
{ __sync_fetch_and_or(target, bits); }

inline
void
atomic_or_16(volatile uint16_t *target, uint16_t bits)
{ __sync_fetch_and_or(target, bits); }

inline
void
atomic_or_ushort(volatile ushort_t *target, ushort_t bits)
{ __sync_fetch_and_or(target, bits); }

inline
void
atomic_or_32(volatile uint32_t *target, uint32_t bits)
{ __sync_fetch_and_or(target, bits); }

inline
void
atomic_or_uint(volatile uint_t *target, uint_t bits)
{ __sync_fetch_and_or(target, bits); }

inline
void
atomic_or_ulong(volatile ulong_t *target, ulong_t bits)
{ __sync_fetch_and_or(target, bits); }

inline
void
atomic_or_64(volatile uint64_t *target, uint64_t bits)
{ __sync_fetch_and_or(target, bits); }

inline
void
atomic_and_8(volatile uint8_t *target, uint8_t bits)
{ __sync_fetch_and_and(target, bits); }

inline
void
atomic_and_uchar(volatile uchar_t *target, uchar_t bits)
{ __sync_fetch_and_and(target, bits); }

inline
void
atomic_and_16(volatile uint16_t *target, uint16_t bits)
{ __sync_fetch_and_and(target, bits); }

inline
void
atomic_and_ushort(volatile ushort_t *target, ushort_t bits)
{ __sync_fetch_and_and(target, bits); }

inline
void
atomic_and_32(volatile uint32_t *target, uint32_t bits)
{ __sync_fetch_and_and(target, bits); }

inline
void
atomic_and_uint(volatile uint_t *target, uint_t bits)
{ __sync_fetch_and_and(target, bits); }

inline
void
atomic_and_ulong(volatile ulong_t *target, ulong_t bits)
{ __sync_fetch_and_and(target, bits); }

inline
void
atomic_and_64(volatile uint64_t *target, uint64_t bits)
{ __sync_fetch_and_and(target, bits); }

inline
uint8_t
atomic_inc_8_nv(volatile uint8_t *target)
{ return __sync_add_and_fetch(target, 1); }

inline
uchar_t
atomic_inc_uchar_nv(volatile uchar_t *target)
{ return __sync_add_and_fetch(target, 1); }

inline
uint16_t
atomic_inc_16_nv(volatile uint16_t *target)
{ return __sync_add_and_fetch(target, 1); }

inline
ushort_t
atomic_inc_ushort_nv(volatile ushort_t *target)
{ return __sync_add_and_fetch(target, 1); }

inline
uint32_t
atomic_inc_32_nv(volatile uint32_t *target)
{ return __sync_add_and_fetch(target, 1); }

inline
uint_t
atomic_inc_uint_nv(volatile uint_t *target)
{ return __sync_add_and_fetch(target, 1); }

inline
ulong_t
atomic_inc_ulong_nv(volatile ulong_t *target)
{ return __sync_add_and_fetch(target, 1); }

inline
uint64_t
atomic_inc_64_nv(volatile uint64_t *target)
{ return __sync_add_and_fetch(target, 1); }

inline
uint8_t
atomic_dec_8_nv(volatile uint8_t *target)
{ return __sync_sub_and_fetch(target, 1); }

inline
uchar_t
atomic_dec_uchar_nv(volatile uchar_t *target)
{ return __sync_sub_and_fetch(target, 1); }

inline
uint16_t
atomic_dec_16_nv(volatile uint16_t *target)
{ return __sync_sub_and_fetch(target, 1); }

inline
ushort_t
atomic_dec_ushort_nv(volatile ushort_t *target)
{ return __sync_sub_and_fetch(target, 1); }

inline
uint32_t
atomic_dec_32_nv(volatile uint32_t *target)
{ return __sync_sub_and_fetch(target, 1); }

inline
uint_t
atomic_dec_uint_nv(volatile uint_t *target)
{ return __sync_sub_and_fetch(target, 1); }

inline
ulong_t
atomic_dec_ulong_nv(volatile ulong_t *target)
{ return __sync_sub_and_fetch(target, 1); }

inline
uint64_t
atomic_dec_64_nv(volatile uint64_t *target)
{ return __sync_sub_and_fetch(target, 1); }

inline
uint8_t
atomic_add_8_nv(volatile uint8_t *target, int8_t value)
{ return __sync_add_and_fetch(target, value); }

inline
uchar_t
atomic_add_char_nv(volatile uchar_t *target, signed char value)
{ return __sync_add_and_fetch(target, value); }

inline
uint16_t
atomic_add_16_nv(volatile uint16_t *target, int16_t delta)
{ return __sync_add_and_fetch(target, delta); }

inline
ushort_t
atomic_add_short_nv(volatile ushort_t *target, short value)
{ return __sync_add_and_fetch(target, value); }

inline
uint32_t
atomic_add_32_nv(volatile uint32_t *target, int32_t delta)
{ return __sync_add_and_fetch(target, delta); }

inline
uint_t
atomic_add_int_nv(volatile uint_t *target, int delta)
{ return __sync_add_and_fetch(target, delta); }
inline

void *
atomic_add_ptr_nv(volatile void *target, ssize_t value)
{ return __sync_add_and_fetch((caddr_t *)target, value); }
inline

ulong_t
atomic_add_long_nv(volatile ulong_t *target, long delta)
{ return __sync_add_and_fetch(target, delta); }

inline
uint64_t
atomic_add_64_nv(volatile uint64_t *target, int64_t delta)
{ return __sync_add_and_fetch(target, delta); }

inline
uint8_t
atomic_or_8_nv(volatile uint8_t *target, uint8_t value)
{ return __sync_or_and_fetch(target, value); }

inline
uchar_t
atomic_or_uchar_nv(volatile uchar_t *target, uchar_t value)
{ return __sync_or_and_fetch(target, value); }

inline
uint16_t
atomic_or_16_nv(volatile uint16_t *target, uint16_t value)
{ return __sync_or_and_fetch(target, value); }

inline
ushort_t
atomic_or_ushort_nv(volatile ushort_t *target, ushort_t value)
{ return __sync_or_and_fetch(target, value); }

inline
uint32_t
atomic_or_32_nv(volatile uint32_t *target, uint32_t value)
{ return __sync_or_and_fetch(target, value); }

inline
uint_t
atomic_or_uint_nv(volatile uint_t *target, uint_t value)
{ return __sync_or_and_fetch(target, value); }

inline
ulong_t
atomic_or_ulong_nv(volatile ulong_t *target, ulong_t value)
{ return __sync_or_and_fetch(target, value); }

inline
uint64_t
atomic_or_64_nv(volatile uint64_t *target, uint64_t value)
{ return __sync_or_and_fetch(target, value); }

inline
uint8_t
atomic_and_8_nv(volatile uint8_t *target, uint8_t value)
{ return __sync_and_and_fetch(target, value); }

inline
uchar_t
atomic_and_uchar_nv(volatile uchar_t *target, uchar_t value)
{ return __sync_and_and_fetch(target, value); }

inline
uint16_t
atomic_and_16_nv(volatile uint16_t *target, uint16_t value)
{ return __sync_and_and_fetch(target, value); }

inline
ushort_t
atomic_and_ushort_nv(volatile ushort_t *target, ushort_t value)
{ return __sync_and_and_fetch(target, value); }

inline
uint32_t
atomic_and_32_nv(volatile uint32_t *target, uint32_t value)
{ return __sync_and_and_fetch(target, value); }

inline
uint_t
atomic_and_uint_nv(volatile uint_t *target, uint_t value)
{ return __sync_and_and_fetch(target, value); }

inline
ulong_t
atomic_and_ulong_nv(volatile ulong_t *target, ulong_t value)
{ return __sync_and_and_fetch(target, value); }

inline
uint64_t
atomic_and_64_nv(volatile uint64_t *target, uint64_t value)
{ return __sync_and_and_fetch(target, value); }

inline
uint8_t
atomic_cas_8(volatile uint8_t *target, uint8_t cmp, uint8_t new)
{
	return __sync_val_compare_and_swap(target, cmp, new);
}

inline
uchar_t
atomic_cas_uchar(volatile uchar_t *target, uchar_t cmp, uchar_t new)
{
	return __sync_val_compare_and_swap(target, cmp, new);
}

inline
uint16_t
atomic_cas_16(volatile uint16_t *target, uint16_t cmp, uint16_t new)
{
	return __sync_val_compare_and_swap(target, cmp, new);
}

inline
ushort_t
atomic_cas_ushort(volatile ushort_t *target, ushort_t cmp, ushort_t new)
{
	return __sync_val_compare_and_swap(target, cmp, new);
}

inline
uint32_t
atomic_cas_32(volatile uint32_t *target, uint32_t cmp, uint32_t new)
{
	return __sync_val_compare_and_swap(target, cmp, new);
}

inline
uint64_t
atomic_cas_64(volatile uint64_t *target, uint64_t cmp, uint64_t new)
{
	return __sync_val_compare_and_swap(target, cmp, new);
}

inline
uint_t
atomic_cas_uint(volatile uint_t *target, uint_t cmp, uint_t new)
{
	return __sync_val_compare_and_swap(target, cmp, new);
}

inline
ulong_t
atomic_cas_ulong(volatile ulong_t *target, ulong_t cmp, ulong_t new)
{
	return __sync_val_compare_and_swap(target, cmp, new);
}

inline
uint64_t
atomic_cas_uint64(volatile uint64_t *target, ulong_t cmp, uint64_t new)
{
	return __sync_val_compare_and_swap(target, cmp, new);
}

inline
void *
atomic_cas_ptr(volatile void *target, void *cmp, void *new)
{
	return __sync_val_compare_and_swap((caddr_t*)target, (caddr_t*)cmp, (caddr_t*)new);
}

inline
uint8_t
atomic_swap_8(volatile uint8_t *target, uint8_t new)
{
	uint8_t old;
	do {
		old = *target;
	} while(! __sync_bool_compare_and_swap(target, old, new));
	return (old);
}

inline
uchar_t
atomic_swap_char(volatile uchar_t *target, uchar_t new)
{
	uchar_t old;
	do {
		old = *target;
	} while(! __sync_bool_compare_and_swap(target, old, new));
	return (old);
}

inline
uchar_t
atomic_swap_uchar(volatile uchar_t *target, uchar_t new)
{
	uchar_t old;
	do {
		old = *target;
	} while(! __sync_bool_compare_and_swap(target, old, new));
	return (old);
}

inline
uint16_t
atomic_swap_16(volatile uint16_t *target, uint16_t new)
{
	uint16_t old;
	do {
		old = *target;
	} while(! __sync_bool_compare_and_swap(target, old, new));
	return (old);
}

inline
ushort_t
atomic_swap_ushort(volatile ushort_t *target, ushort_t new)
{
	ushort_t old;
	do {
		old = *target;
	} while(! __sync_bool_compare_and_swap(target, old, new));
	return (old);
}

inline
uint32_t
atomic_swap_32(volatile uint32_t *target, uint32_t new)
{
	uint32_t old;
	do {
		old = *target;
	} while(! __sync_bool_compare_and_swap(target, old, new));
	return (old);
}

inline
uint_t
atomic_swap_uint(volatile uint_t *target, uint_t new)
{
	ulong_t old;
	do {
		old = *target;
	} while(! __sync_bool_compare_and_swap(target, old, new));
	return (old);
}

inline
uint64_t
atomic_swap_64(volatile uint64_t *target, uint64_t new)
{
	uint64_t old;
	do {
		old = *target;
	} while(! __sync_bool_compare_and_swap(target, old, new));
	return (old);
}

inline
void *
atomic_swap_ptr(volatile void *target, void *new)
{
	void *old;
	do {
		old = *(void **)target;
	} while(! __sync_bool_compare_and_swap((caddr_t *)target, (caddr_t *)old, new));
	return (old);
}

inline
ulong_t
atomic_swap_ulong(volatile ulong_t *target, ulong_t new)
{
	ulong_t old;
	do {
		old = *target;
	} while(! __sync_bool_compare_and_swap(target, old, new));
	return (old);
}

inline
int
atomic_set_long_excl(volatile ulong_t *target, uint_t value)
{
/*
	ulong_t bit = (1UL << value);
	if ((*target & bit) != 0)
		return (-1);
	*target |= bit;
	return (0);
*/
	ulong_t bit = (1UL << value);
	ulong_t old;
	do {
		if (((old = *target) & bit) != 0)
			return (-1);
	} while(! __sync_bool_compare_and_swap(target, old, (*target | bit)));
	return(0);
}

inline
int
atomic_clear_long_excl(volatile ulong_t *target, uint_t value)
{
/*
	ulong_t bit = (1UL << value);
	if ((*target & bit) == 0)
		return (-1);
	*target &= ~bit;
	return (0);
*/
	ulong_t bit = (1UL << value);
	ulong_t old;
	do {
		if (((old = *target) & bit) == 0)
			return (-1);
	} while(! __sync_bool_compare_and_swap(target, old, (*target & bit)));
	return(0);
}

#if !defined(_KERNEL)

inline
void
membar_enter(void)
{
	__sync_synchronize();
}

inline
void
membar_exit(void)
{
	__sync_synchronize();
}

inline
void
membar_producer(void)
{
	__sync_synchronize();
}

inline
void
membar_consumer(void)
{
	__sync_synchronize();
}

#endif	/* _KERNEL */

