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
/* Module:      hex.c, hex.h                                            */
/* Author:      Israel Fuller                                           */
/* Date:        November 17, 2026                                       */
/* Description: Hexadecimal (base16) encoding and decoding.             */
/************************************************************************/

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "hex.h"
#include "mtsession.h"
#include "range.h"

/** The hex digits, in value order. **/
static const char hex_i_digits[] = "0123456789abcdef";

/** The value of each hex digit, or -1 for other characters. **/
static const signed char hex_i_values[256] =
    {
    [0 ... 255] = -1,
    ['0'] = 0,
    ['1'] = 1,
    ['2'] = 2,
    ['3'] = 3,
    ['4'] = 4,
    ['5'] = 5,
    ['6'] = 6,
    ['7'] = 7,
    ['8'] = 8,
    ['9'] = 9,
    ['a'] = 10, ['A'] = 10,
    ['b'] = 11, ['B'] = 11,
    ['c'] = 12, ['C'] = 12,
    ['d'] = 13, ['D'] = 13,
    ['e'] = 14, ['E'] = 14,
    ['f'] = 15, ['F'] = 15,
    };


/*** Resolve a length argument, where -1 means the string's strlen().
 ***
 *** @param str The string the length applies to.
 *** @param len The length argument.
 *** @param action What the caller is doing, for the error message.
 *** @param out Set to the resolved length.
 *** @returns 0 on success, or -1 with mssError() if len is invalid.
 ***/
static int
hex_i_Length(const char* str, ssize_t len, const char* action, size_t* out)
    {
	if (len == -1)
	    {
	    *out = strlen(str);
	    return 0;
	    }
	if (len < 0)
	    {
	    mssError(1, "HEX", "Failed to %s: invalid length %zd.", action, len);
	    return -1;
	    }
	*out = (size_t)len;

    return 0;
    }


/*** Get the exact size needed to hold hex encoded data.
 ***
 *** @param src_len The number of bytes to encode.
 *** @returns The exact size hexEncode() needs for its output, including the
 ***	null, or 0 if the size does not fit in a ssize_t.
 ***/
size_t
hexEncodedSize(size_t src_len)
    {
	if (src_len > ((size_t)SSIZE_MAX - 1) / 2) return 0;

    return src_len * 2 + 1;
    }


/*** Get the exact size needed to hold decoded hex data.
 ***
 *** @param src_len The length of the hex text, excluding a null terminator.
 *** @returns The size hexDecode() needs for its output, including the null.
 ***	It is exact for valid hex text.
 ***/
size_t
hexDecodedSize(size_t src_len)
    {
    return src_len / 2 + 1;
    }


/*** Encode data as lowercase hex, two digits per byte, high digit first.
 ***
 *** @param dst The buffer for writing the null-terminated encoding.  It may
 ***	equal src to encode in place, but must not otherwise overlap it.
 *** @param dst_size The size of dst.  hexEncodedSize() gives the size needed
 ***	to avoid truncation.
 *** @param src The data to encode.
 *** @param src_len The number of bytes to encode, or -1 to use strlen(src).
 *** @param flags HEX_F_TRUNCATE to write as many digits as fit in dst (which
 ***	may be an odd number) when the encoding is too large, otherwise
 ***	HEX_F_NONE.
 *** @returns The length of the encoding, excluding the null, or -1 with
 ***	mssError() on failure.  On failure, dst is not modified.
 ***/
ssize_t
hexEncode(char* dst, size_t dst_size, const void* src, ssize_t src_len, int flags)
    {
    const unsigned char* in = (const unsigned char*)src;
    size_t len;
    size_t n_bytes, out_len;

	/** Check the arguments. **/
	if ((flags & ~HEX_F_TRUNCATE) != 0)
	    {
	    mssError(1, "HEX", "Failed to encode: invalid flags 0x%x.", (unsigned int)flags);
	    return -1;
	    }
	if (hex_i_Length((const char*)src, src_len, "encode", &len) < 0) return -1;

	/** Check that dst does not partly overlap src. **/
	const uintptr_t d = (uintptr_t)dst, s = (uintptr_t)src;
	if (d != s && d < s + len && s < d + dst_size)
	    {
	    mssError(1, "HEX", "Failed to encode: dst %p (%zu bytes) overlaps src %p (%zu bytes).", dst, dst_size, src, len);
	    return -1;
	    }

	/** Truncate to fit dst, or check that the whole encoding fits. **/
	if ((flags & HEX_F_TRUNCATE) != 0)
	    {
	    if (dst_size == 0)
		{
		mssError(1, "HEX", "Failed to encode %zu bytes: dst has no room for the null.", len);
		return -1;
		}
	    n_bytes = min(len, dst_size / 2);
	    out_len = (len <= (dst_size - 1) / 2) ? len * 2 : dst_size - 1;
	    }
	else
	    {
	    const size_t req_size = hexEncodedSize(len);
	    if (req_size == 0)
		{
		mssError(1, "HEX", "Failed to encode %zu bytes: the encoding is too large.", len);
		return -1;
		}
	    if (req_size > dst_size)
		{
		mssError(1, "HEX", "Failed to encode %zu bytes: needs %zu bytes, but dst holds %zu.", len, req_size, dst_size);
		return -1;
		}
	    n_bytes = len;
	    out_len = len * 2;
	    }

	/** Encode from last byte to first, so dst may equal src. **/
	for (size_t i = n_bytes; i > 0; i--)
	    {
	    const unsigned char byte = in[i - 1];
	    dst[i * 2 - 2] = hex_i_digits[byte >> 4];
	    dst[i * 2 - 1] = hex_i_digits[byte & 0x0F];
	    }

	/** Note: The null overwrites the extra digit of an odd truncation. **/
	dst[out_len] = '\0';

    return (ssize_t)out_len;
    }


/*** Decode hex text (case insensitive).
 ***
 *** @param dst The buffer to write the null-terminated decoded data into.  It
 ***	may equal src to decode in place, but must not otherwise overlap it.
 *** @param dst_size The size of dst.  hexDecodedSize() gives the size needed.
 *** @param src The hex text to decode, which must have an even length.
 *** @param src_len The length of src, or -1 to use strlen(src).
 *** @param flags HEX_F_NONE.
 *** @returns The number of decoded bytes, excluding the null, or -1 with
 ***	mssError() on failure.  On failure, dst may be partly written.
 ***/
ssize_t
hexDecode(void* dst, size_t dst_size, const char* src, ssize_t src_len, int flags)
    {
    unsigned char* out = (unsigned char*)dst;
    size_t len;

	/** Check the arguments. **/
	if (flags != HEX_F_NONE)
	    {
	    mssError(1, "HEX", "Failed to decode: invalid flags 0x%x.", (unsigned int)flags);
	    return -1;
	    }
	if (hex_i_Length(src, src_len, "decode", &len) < 0) return -1;

	/** Check that dst does not partly overlap src. **/
	const uintptr_t d = (uintptr_t)dst, s = (uintptr_t)src;
	if (d != s && d < s + len && s < d + dst_size)
	    {
	    mssError(1, "HEX", "Failed to decode: dst %p (%zu bytes) overlaps src %p (%zu bytes).", dst, dst_size, src, len);
	    return -1;
	    }

	if (len % 2 != 0)
	    {
	    mssError(1, "HEX", "Failed to decode: odd length %zu.", len);
	    return -1;
	    }

	/** Check that the decoded data fits. **/
	const size_t n_bytes = len / 2;
	if (n_bytes >= dst_size)
	    {
	    mssError(1, "HEX", "Failed to decode %zu characters: needs %zu bytes, but dst holds %zu.", len, n_bytes + 1, dst_size);
	    return -1;
	    }

	/** Decode from first byte to last, so dst may equal src. **/
	for (size_t i = 0; i < n_bytes; i++)
	    {
	    const int high = hex_i_values[(unsigned char)src[i * 2]];
	    const int low = hex_i_values[(unsigned char)src[i * 2 + 1]];
	    if (high < 0 || low < 0)
		{
		const size_t bad = (high < 0) ? i * 2 : i * 2 + 1;
		mssError(1, "HEX", "Failed to decode: invalid character 0x%02x at offset %zu.", (unsigned char)src[bad], bad);
		return -1;
		}
	    out[i] = (unsigned char)((high << 4) | low);
	    }
	out[n_bytes] = '\0';

    return (ssize_t)n_bytes;
    }
