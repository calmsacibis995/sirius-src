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
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#ifndef _KERNEL
#include <strings.h>
#include <limits.h>
#include <assert.h>
#include <security/cryptoki.h>
#endif

#include <sys/types.h>
#include <modes/modes.h>
#include <sys/crypto/common.h>
#include <sys/crypto/impl.h>

/*
 * Encrypt and decrypt multiple blocks of data in counter mode.
 */
int
ctr_mode_contiguous_blocks(ctr_ctx_t *ctx, char *data, size_t length,
    crypto_data_t *out, size_t block_size,
    int (*cipher)(const void *ks, const uint8_t *pt, uint8_t *ct),
    void (*xor_block)(uint8_t *, uint8_t *))
{
	size_t remainder = length;
	size_t need;
	uint8_t *datap = (uint8_t *)data;
	uint8_t *blockp;
	uint8_t *lastp;
	void *iov_or_mp;
	offset_t offset;
	uint8_t *out_data_1;
	uint8_t *out_data_2;
	size_t out_data_1_len;
	uint64_t counter;
#ifdef _LITTLE_ENDIAN
	uint8_t *p;
#endif

	if (length + ctx->ctr_remainder_len < block_size) {
		/* accumulate bytes here and return */
		bcopy(datap,
		    (uint8_t *)ctx->ctr_remainder + ctx->ctr_remainder_len,
		    length);
		ctx->ctr_remainder_len += length;
		ctx->ctr_copy_to = datap;
		return (CRYPTO_SUCCESS);
	}

	lastp = (uint8_t *)ctx->ctr_cb;
	if (out != NULL)
		crypto_init_ptrs(out, &iov_or_mp, &offset);

	do {
		/* Unprocessed data from last call. */
		if (ctx->ctr_remainder_len > 0) {
			need = block_size - ctx->ctr_remainder_len;

			if (need > remainder)
				return (CRYPTO_DATA_LEN_RANGE);

			bcopy(datap, &((uint8_t *)ctx->ctr_remainder)
			    [ctx->ctr_remainder_len], need);

			blockp = (uint8_t *)ctx->ctr_remainder;
		} else {
			blockp = datap;
		}

		/* ctr_cb is the counter block */
		cipher(ctx->ctr_keysched, (uint8_t *)ctx->ctr_cb,
		    (uint8_t *)ctx->ctr_tmp);

		lastp = (uint8_t *)ctx->ctr_tmp;

		/*
		 * Increment counter. Counter bits are confined
		 * to the bottom 64 bits of the counter block.
		 */
		counter = ctx->ctr_cb[1] & ctx->ctr_counter_mask;
#ifdef _LITTLE_ENDIAN
		p = (uint8_t *)&counter;
		counter = (((uint64_t)p[0] << 56) |
		    ((uint64_t)p[1] << 48) |
		    ((uint64_t)p[2] << 40) |
		    ((uint64_t)p[3] << 32) |
		    ((uint64_t)p[4] << 24) |
		    ((uint64_t)p[5] << 16) |
		    ((uint64_t)p[6] << 8) |
		    (uint64_t)p[7]);
#endif
		counter++;
#ifdef _LITTLE_ENDIAN
		counter = (((uint64_t)p[0] << 56) |
		    ((uint64_t)p[1] << 48) |
		    ((uint64_t)p[2] << 40) |
		    ((uint64_t)p[3] << 32) |
		    ((uint64_t)p[4] << 24) |
		    ((uint64_t)p[5] << 16) |
		    ((uint64_t)p[6] << 8) |
		    (uint64_t)p[7]);
#endif
		counter &= ctx->ctr_counter_mask;
		ctx->ctr_cb[1] =
		    (ctx->ctr_cb[1] & ~(ctx->ctr_counter_mask)) | counter;

		/*
		 * XOR the previous cipher block or IV with the
		 * current clear block.
		 */
		xor_block(blockp, lastp);

		if (out == NULL) {
			if (ctx->ctr_remainder_len > 0) {
				bcopy(lastp, ctx->ctr_copy_to,
				    ctx->ctr_remainder_len);
				bcopy(lastp + ctx->ctr_remainder_len, datap,
				    need);
			}
		} else {
			crypto_get_ptrs(out, &iov_or_mp, &offset, &out_data_1,
			    &out_data_1_len, &out_data_2, block_size);

			/* copy block to where it belongs */
			bcopy(lastp, out_data_1, out_data_1_len);
			if (out_data_2 != NULL) {
				bcopy(lastp + out_data_1_len, out_data_2,
				    block_size - out_data_1_len);
			}
			/* update offset */
			out->cd_offset += block_size;
		}

		/* Update pointer to next block of data to be processed. */
		if (ctx->ctr_remainder_len != 0) {
			datap += need;
			ctx->ctr_remainder_len = 0;
		} else {
			datap += block_size;
		}

		remainder = (size_t)&data[length] - (size_t)datap;

		/* Incomplete last block. */
		if (remainder > 0 && remainder < block_size) {
			bcopy(datap, ctx->ctr_remainder, remainder);
			ctx->ctr_remainder_len = remainder;
			ctx->ctr_copy_to = datap;
			goto out;
		}
		ctx->ctr_copy_to = NULL;

	} while (remainder > 0);

out:
	return (CRYPTO_SUCCESS);
}

int
ctr_mode_final(ctr_ctx_t *ctx, crypto_data_t *out,
    int (*encrypt_block)(const void *, const uint8_t *, uint8_t *))
{
	uint8_t *lastp;
	void *iov_or_mp;
	offset_t offset;
	uint8_t *out_data_1;
	uint8_t *out_data_2;
	size_t out_data_1_len;
	uint8_t *p;
	int i;

	if (out->cd_length < ctx->ctr_remainder_len)
		return (CRYPTO_DATA_LEN_RANGE);

	encrypt_block(ctx->ctr_keysched, (uint8_t *)ctx->ctr_cb,
	    (uint8_t *)ctx->ctr_tmp);

	lastp = (uint8_t *)ctx->ctr_tmp;
	p = (uint8_t *)ctx->ctr_remainder;
	for (i = 0; i < ctx->ctr_remainder_len; i++) {
		p[i] ^= lastp[i];
	}

	crypto_init_ptrs(out, &iov_or_mp, &offset);
	crypto_get_ptrs(out, &iov_or_mp, &offset, &out_data_1,
	    &out_data_1_len, &out_data_2, ctx->ctr_remainder_len);

	bcopy(p, out_data_1, out_data_1_len);
	if (out_data_2 != NULL) {
		bcopy((uint8_t *)p + out_data_1_len,
		    out_data_2, ctx->ctr_remainder_len - out_data_1_len);
	}
	out->cd_offset += ctx->ctr_remainder_len;
	ctx->ctr_remainder_len = 0;
	return (CRYPTO_SUCCESS);
}

int
ctr_init_ctx(ctr_ctx_t *ctr_ctx, ulong_t count, uint8_t *cb,
void (*copy_block)(uint8_t *, uint8_t *))
{
	uint64_t mask = 0;
#ifdef _LITTLE_ENDIAN
	uint8_t *p8;
#endif

	if (count == 0 || count > 64) {
		return (CRYPTO_MECHANISM_PARAM_INVALID);
	}
	while (count-- > 0)
		mask |= (1ULL << count);
#ifdef _LITTLE_ENDIAN
	p8 = (uint8_t *)&mask;
	mask = (((uint64_t)p8[0] << 56) |
	    ((uint64_t)p8[1] << 48) |
	    ((uint64_t)p8[2] << 40) |
	    ((uint64_t)p8[3] << 32) |
	    ((uint64_t)p8[4] << 24) |
	    ((uint64_t)p8[5] << 16) |
	    ((uint64_t)p8[6] << 8) |
	    (uint64_t)p8[7]);
#endif
	ctr_ctx->ctr_counter_mask = mask;
	copy_block(cb, (uchar_t *)ctr_ctx->ctr_cb);
	ctr_ctx->ctr_lastp = (uint8_t *)&ctr_ctx->ctr_cb[0];
	ctr_ctx->ctr_flags |= CTR_MODE;
	return (CRYPTO_SUCCESS);
}

/* ARGSUSED */
void *
ctr_alloc_ctx(int kmflag)
{
	ctr_ctx_t *ctr_ctx;

#ifdef _KERNEL
	if ((ctr_ctx = kmem_zalloc(sizeof (ctr_ctx_t), kmflag)) == NULL)
#else
	if ((ctr_ctx = calloc(1, sizeof (ctr_ctx_t))) == NULL)
#endif
		return (NULL);

	ctr_ctx->ctr_flags = CTR_MODE;
	return (ctr_ctx);
}
