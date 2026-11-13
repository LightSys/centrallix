/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Base Library                                              */
/*                                                                      */
/* Copyright (C) 2026 LightSys Technology Services, Inc.                */
/*                                                                      */
/* You may use these files and this library under the terms of the      */
/* GNU Lesser General Public License, Version 2.1, contained in the     */
/* included file "COPYING".                                             */
/*                                                                      */
/* Module:      ascii85.c, ascii85.h                                    */
/* Author:      Israel Fuller                                           */
/* Date:        November 13, 2026                                       */
/* Description: ASCII85 encoding, as used by PostScript and PDF.        */
/************************************************************************/

#include <limits.h>
#include <stdint.h>

#include "ascii85.h"
#include "expect.h"
#include "mtsession.h"


/*** Get a size that can hold the ASCII85 encoding of any data.
 ***
 *** @param src_len The number of bytes to encode.
 *** @returns A size a85Encode() never exceeds for its output, including the
 ***	null, or 0 if the size does not fit in a ssize_t.  It is exact for
 ***	data without 4-byte groups of zeros.
 ***/
size_t
a85EncodedSizeMax(size_t src_len)
    {
	/** Every 4 bytes become 5 characters, and a final 1-3 bytes become 2-4. **/
	const size_t n_groups = src_len / 4;
	const size_t n_extra = src_len % 4;
	if (n_groups > (size_t)SSIZE_MAX / 5) return 0;
	const size_t enc_len = (n_groups * 5) + ((n_extra != 0) ? n_extra + 1 : 0);
	if (enc_len > (size_t)SSIZE_MAX) return 0;

    return enc_len + 1;
    }


/*** Encode data as ASCII85, writing "z" for each 4-byte group of zeros.
 *** Encoding data in pieces gives the same text as encoding it in one call
 *** if every piece but the last holds a multiple of 4 bytes.  Callers add
 *** any delimiters and line endings, such as PostScript's "~>" end marker.
 ***
 *** @param dst The buffer to write the null-terminated encoding into.  It
 ***	must not overlap src.
 *** @param dst_size The size of dst, which must be at least
 ***	a85EncodedSizeMax(src_len).
 *** @param src The data to encode.
 *** @param src_len The number of bytes to encode.
 *** @returns The length of the encoding, excluding the null, or -1 with
 ***	mssError() on failure.  On failure, dst is not modified.
 ***/
ssize_t
a85Encode(char* dst, size_t dst_size, const void* src, size_t src_len)
    {
    const unsigned char* in = (const unsigned char*)src;
    size_t i, o;
    int d;

	/** Check that any encoding fits. **/
	const size_t req_size = a85EncodedSizeMax(src_len);
	if (UNLIKELY(req_size == 0))
	    {
	    mssError(1, "A85", "Failed to encode %zu bytes: the encoding is too large.", src_len);
	    return -1;
	    }
	if (UNLIKELY(req_size > dst_size))
	    {
	    mssError(1, "A85", "Failed to encode %zu bytes: needs %zu bytes, but dst holds %zu.", src_len, req_size, dst_size);
	    return -1;
	    }

	/** Encode each group of 4 bytes as 5 base-85 digits, or "z" for zeros. **/
	for (i = 0, o = 0; src_len - i >= 4; i += 4)
	    {
	    uint32_t group = ((uint32_t)in[i] << 24) | ((uint32_t)in[i + 1] << 16) | ((uint32_t)in[i + 2] << 8) | in[i + 3];
	    if (group == 0)
		{
		dst[o++] = 'z';
		continue;
		}
	    for (d = 4; d >= 0; d--)
		{
		dst[o + d] = (char)('!' + group % 85);
		group /= 85;
		}
	    o += 5;
	    }

	/** Encode a final 1-3 bytes as a zero-padded group, keeping one more digit than bytes. **/
	const size_t n_extra = src_len - i;
	if (n_extra > 0)
	    {
	    uint32_t group = 0;
	    char digits[5];
	    for (d = 0; d < 4; d++)
		group = (group << 8) | (((size_t)d < n_extra) ? in[i + d] : 0);
	    for (d = 4; d >= 0; d--)
		{
		digits[d] = (char)('!' + group % 85);
		group /= 85;
		}
	    for (d = 0; (size_t)d <= n_extra; d++)
		dst[o++] = digits[d];
	    }
	dst[o] = '\0';

    return (ssize_t)o;
    }
