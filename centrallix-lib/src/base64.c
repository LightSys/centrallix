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
/* Module:      base64.c, base64.h                                      */
/* Author:      Israel Fuller                                           */
/* Date:        November 10, 2026                                       */
/* Description: Standard base64 (RFC 4648) encoding and decoding, with  */
/*              optional line wrapping.                                 */
/************************************************************************/

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "base64.h"
#include "expect.h"
#include "mtsession.h"
#include "range.h"

/** The base64 alphabet, in value order. **/
static const char b64_i_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


/*** Get the value of a base64 alphabet character.
 ***
 *** @param ch The character.
 *** @returns The character's value (0-63), or -1 if it is not in the alphabet.
 ***/
static int
b64_i_Value(unsigned char ch)
    {
	if (ch >= 'A' && ch <= 'Z') return ch - 'A';
	if (ch >= 'a' && ch <= 'z') return ch - 'a' + 26;
	if (ch >= '0' && ch <= '9') return ch - '0' + 52;
	if (ch == '+') return 62;
	if (ch == '/') return 63;

    return -1;
    }


/*** Check whether a character is whitespace.
 ***
 *** @param ch The character.
 *** @returns Whether ch is a space, tab, line ending, vertical tab, or form
 ***	feed.
 ***/
static bool
b64_i_IsSpace(unsigned char ch)
    {
    return (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\v' || ch == '\f');
    }


/*** Resolve a length argument, where -1 means the string's strlen().
 ***
 *** @param str The string the length applies to.
 *** @param len The length argument.
 *** @param action What the caller is doing, for the error message.
 *** @param out Set to the resolved length.
 *** @returns 0 on success, or -1 with mssError() if len is invalid.
 ***/
static int
b64_i_Length(const char* str, ssize_t len, const char* action, size_t* out)
    {
	if (len == -1)
	    {
	    *out = strlen(str);
	    return 0;
	    }
	if (UNLIKELY(len < 0))
	    {
	    mssError(1, "B64", "Failed to %s: invalid length %zd.", action, len);
	    return -1;
	    }
	*out = (size_t)len;

    return 0;
    }


/*** Get the exact size needed to hold text with line endings added.
 ***
 *** @param src_len The length of the text, excluding any null.
 *** @param line_len The maximum characters per line.
 *** @param flags B64_F_CRLF for "\r\n" line endings, otherwise B64_F_NONE.
 *** @returns The exact size b64Wrap() needs for its output, including the
 ***	null, or 0 if b64Wrap() would reject these arguments or the size does
 ***	not fit in a ssize_t.
 ***/
size_t
b64WrappedSize(size_t src_len, size_t line_len, int flags)
    {
	if (line_len == 0 || (flags & ~B64_F_CRLF) != 0) return 0;

	/** Every line, including a short final line, ends in eol_len bytes. **/
	const size_t eol_len = (flags & B64_F_CRLF) ? 2 : 1;
	const size_t n_lines = src_len / line_len + ((src_len % line_len != 0) ? 1 : 0);

	/** Handle outputs that are too long for ssize_t. **/
	const size_t max_len = (size_t)SSIZE_MAX - 1;
	if (src_len > max_len) return 0;
	const size_t max_eols = (max_len - src_len) / eol_len;
	if (n_lines > max_eols) return 0;

    return src_len + (n_lines * eol_len) + 1;
    }


/*** Get the exact size needed to hold base64 encoded data.
 ***
 *** @param src_len The number of bytes to encode.
 *** @param line_len The maximum characters per line, a multiple of 4, or 0
 ***	for no wrapping.
 *** @param flags B64_F_CRLF for "\r\n" line endings, otherwise B64_F_NONE.
 *** @returns The exact size b64Encode() needs for its output, including the
 ***	null, or 0 if b64Encode() would reject these arguments or the size
 ***	does not fit in a ssize_t.
 ***/
size_t
b64EncodedSize(size_t src_len, size_t line_len, int flags)
    {
	if (line_len % 4 != 0 || (flags & ~B64_F_CRLF) != 0) return 0;

	/** Every 3 bytes, or fewer at the end, become 4 characters. **/
	const size_t n_groups = src_len / 3 + ((src_len % 3 != 0) ? 1 : 0);
	if (n_groups > (size_t)SSIZE_MAX / 4) return 0;
	const size_t enc_len = n_groups * 4;

	if (line_len > 0) return b64WrappedSize(enc_len, line_len, flags);

    return enc_len + 1;
    }


/*** Get a size that can hold any decoding of base64 text.
 ***
 *** @param src_len The length of the base64 text, excluding any null.
 *** @returns A size b64Decode() never exceeds for its output, including the
 ***	null.  It is exact for text without padding or line endings.
 ***/
size_t
b64DecodedSizeMax(size_t src_len)
    {
    return (src_len / 4 * 3) + 1;
    }


/*** Add line endings to text, such as base64 encoded data.  Every line,
 *** including a short final line, ends in a line ending, so empty text
 *** stays empty.
 ***
 *** @param dst The buffer for writing the null-terminated result.  It may
 ***	equal src to wrap in place, but must not otherwise overlap it.
 *** @param dst_size The size of dst.  b64WrappedSize() gives the size needed
 ***	to avoid truncation.
 *** @param src The text to wrap.
 *** @param src_len The length of src, or -1 to use strlen(src).
 *** @param line_len The maximum characters per line (must be at least 1).
 *** @param flags B64_F_CRLF for "\r\n" line endings, otherwise B64_F_NONE.
 *** @returns The length of the result, excluding the null, or -1 with
 ***	mssError() on failure.  On failure, dst is not modified.
 ***/
ssize_t
b64Wrap(char* dst, size_t dst_size, const char* src, ssize_t src_len, size_t line_len, int flags)
    {
    size_t len;

	/** Check the arguments. **/
	if (UNLIKELY((flags & ~B64_F_CRLF) != 0))
	    {
	    mssError(1, "B64", "Failed to wrap: invalid flags 0x%x.", (unsigned int)flags);
	    return -1;
	    }
	if (UNLIKELY(line_len == 0))
	    {
	    mssError(1, "B64", "Failed to wrap: invalid line length 0.");
	    return -1;
	    }
	if (UNLIKELY(b64_i_Length(src, src_len, "wrap", &len) < 0)) return -1;

	/** Check that the result fits. **/
	const size_t req_size = b64WrappedSize(len, line_len, flags);
	if (UNLIKELY(req_size == 0))
	    {
	    mssError(1, "B64", "Failed to wrap %zu characters: the result is too large.", len);
	    return -1;
	    }
	if (UNLIKELY(req_size > dst_size))
	    {
	    mssError(1, "B64",
		"Failed to wrap %zu characters: dst can only hold %zu/%zu needed bytes.",
		len, dst_size, req_size
	    );
	    return -1;
	    }

	/** Move each line into place from last to first, so dst may equal src. **/
	const size_t eol_len = (flags & B64_F_CRLF) ? 2 : 1;
	const size_t n_lines = len / line_len + ((len % line_len != 0) ? 1 : 0);
	dst[req_size - 1] = '\0';
	for (size_t line = n_lines; line > 0; line--)
	    {
	    const size_t start = (line - 1) * line_len;
	    const size_t n = min(line_len, len - start);
	    char* out = dst + start + (line - 1) * eol_len;
	    memmove(out, src + start, n);
	    if (eol_len == 2) out[n] = '\r';
	    out[n + eol_len - 1] = '\n';
	    }

    return (ssize_t)(req_size - 1);
    }


/*** Encode data as standard, padded base64, optionally wrapped into lines.
 *** Encoding data in chunks gives the same text as encoding it in one call,
 *** as long as every chunk but the last is a multiple of 3 bytes, or (when
 *** wrapping) line_len / 4 * 3 bytes.
 ***
 *** @param dst The buffer for writing the null-terminated encoding, which
 ***	must not overlap src.
 *** @param dst_size The size of dst.  b64EncodedSize() gives the size needed.
 *** @param src The data to encode.
 *** @param src_len The number of bytes to encode, or -1 to use strlen(src).
 *** @param line_len The maximum characters per line, a multiple of 4, or 0
 ***	for no wrapping.  Every line ends in a line ending.
 *** @param flags B64_F_CRLF for "\r\n" line endings, otherwise B64_F_NONE.
 *** @returns The length of the encoding, excluding the null, or -1 with
 ***	mssError() on failure.  On failure, dst is not modified.
 ***/
ssize_t
b64Encode(char* dst, size_t dst_size, const void* src, ssize_t src_len, size_t line_len, int flags)
    {
    const unsigned char* in = (const unsigned char*)src;
    size_t len;
    size_t i, o;

	/** Check the arguments. **/
	if (UNLIKELY((flags & ~B64_F_CRLF) != 0))
	    {
	    mssError(1, "B64", "Failed to encode: invalid flags 0x%x.", (unsigned int)flags);
	    return -1;
	    }
	if (UNLIKELY(line_len % 4 != 0))
	    {
	    mssError(1, "B64", "Failed to encode: line length %zu is not a multiple of 4.", line_len);
	    return -1;
	    }
	if (UNLIKELY(b64_i_Length((const char*)src, src_len, "encode", &len) < 0)) return -1;

	/** Check that the encoding fits. **/
	const size_t req_size = b64EncodedSize(len, line_len, flags);
	if (UNLIKELY(req_size == 0))
	    {
	    mssError(1, "B64", "Failed to encode %zu bytes: the encoding is too large.", len);
	    return -1;
	    }
	if (UNLIKELY(req_size > dst_size))
	    {
	    mssError(1, "B64", "Failed to encode %zu bytes: needs %zu bytes, but dst holds %zu.", len, req_size, dst_size);
	    return -1;
	    }

	/** Encode each group of 3 bytes as 4 characters. **/
	for (i = 0, o = 0; len - i >= 3; i += 3, o += 4)
	    {
	    const uint32_t group = ((uint32_t)in[i] << 16) | ((uint32_t)in[i + 1] << 8) | in[i + 2];
	    dst[o + 0] = b64_i_alphabet[(group >> 18) & 0x3F];
	    dst[o + 1] = b64_i_alphabet[(group >> 12) & 0x3F];
	    dst[o + 2] = b64_i_alphabet[(group >> 6) & 0x3F];
	    dst[o + 3] = b64_i_alphabet[group & 0x3F];
	    }

	/** Encode a final 1 or 2 bytes as a padded group. **/
	if (i < len)
	    {
	    const bool has_second = (len - i == 2);
	    const uint32_t group = ((uint32_t)in[i] << 16) | ((has_second) ? (uint32_t)in[i + 1] << 8 : 0);
	    dst[o + 0] = b64_i_alphabet[(group >> 18) & 0x3F];
	    dst[o + 1] = b64_i_alphabet[(group >> 12) & 0x3F];
	    dst[o + 2] = (has_second) ? b64_i_alphabet[(group >> 6) & 0x3F] : '=';
	    dst[o + 3] = '=';
	    o += 4;
	    }
	dst[o] = '\0';

	/** Wrap the encoding into lines in place. **/
	if (line_len > 0) return b64Wrap(dst, dst_size, dst, (ssize_t)o, line_len, flags);

    return (ssize_t)o;
    }


/*** Decode standard, padded base64.  A final group of 2 or 3 characters must
 *** be padded to 4.  Whitespace is ignored unless B64_F_REJECT_SPACE is
 *** passed.  Decoding text in chunks gives the same data as decoding it in
 *** one call if every chunk but the last holds a multiple of 4 alphabet
 *** characters.
 ***
 *** @param dst The buffer to write the null-terminated decoded data into.  It
 ***	may equal src to decode in place, but must not otherwise overlap it.
 *** @param dst_size The size of dst.  b64DecodedSizeMax() gives a size that
 ***	is always enough.
 *** @param src The base64 text to decode.
 *** @param src_len The length of src, or -1 to use strlen(src).
 *** @param flags B64_F_SKIP_INVALID to ignore all characters outside the
 ***	base64 alphabet, B64_F_REJECT_SPACE to reject whitespace, otherwise
 ***	B64_F_NONE.  These two flags cannot be combined.
 *** @returns The number of decoded bytes, excluding the null, or -1 with
 ***	mssError() on failure.  On failure, dst may be partly written.
 ***/
ssize_t
b64Decode(void* dst, size_t dst_size, const char* src, ssize_t src_len, int flags)
    {
    unsigned char* out = (unsigned char*)dst;
    const bool skip_invalid = ((flags & B64_F_SKIP_INVALID) != 0);
    const bool skip_space = ((flags & B64_F_REJECT_SPACE) == 0);
    uint32_t group = 0;
    int n_chars = 0, n_pads = 0;
    size_t len;
    size_t o = 0;

	/** Check the arguments. **/
	if (UNLIKELY((flags & ~(B64_F_SKIP_INVALID | B64_F_REJECT_SPACE)) != 0
	    || (skip_invalid && !skip_space)))
	    {
	    mssError(1, "B64", "Failed to decode: invalid flags 0x%x.", (unsigned int)flags);
	    return -1;
	    }
	if (UNLIKELY(b64_i_Length(src, src_len, "decode", &len) < 0)) return -1;

	for (size_t i = 0; i < len; i++)
	    {
	    const unsigned char ch = (unsigned char)src[i];

	    /** Padding completes a final group of 2 or 3 characters. **/
	    if (ch == '=')
		{
		if (UNLIKELY(n_chars < 2 || n_chars + n_pads >= 4))
		    {
		    mssError(1, "B64", "Failed to decode: unexpected '=' at offset %zu.", i);
		    return -1;
		    }
		n_pads++;
		continue;
		}

	    /** Skip or reject characters outside the alphabet. **/
	    const int value = b64_i_Value(ch);
	    if (value < 0)
		{
		if (skip_invalid || (skip_space && b64_i_IsSpace(ch))) continue;
		mssError(1, "B64", "Failed to decode: invalid character 0x%02x at offset %zu.", ch, i);
		return -1;
		}
	    if (UNLIKELY(n_pads > 0))
		{
		mssError(1, "B64", "Failed to decode: data after padding at offset %zu.", i);
		return -1;
		}

	    /** Decode each group of 4 characters as 3 bytes. **/
	    group = (group << 6) | (uint32_t)value;
	    if (++n_chars == 4)
		{
		if (UNLIKELY(dst_size - o <= 3))
		    {
		    mssError(1, "B64", "Failed to decode: %zu-byte dst is too small.", dst_size);
		    return -1;
		    }
		out[o++] = (unsigned char)(group >> 16);
		out[o++] = (unsigned char)(group >> 8);
		out[o++] = (unsigned char)group;
		group = 0;
		n_chars = 0;
		}
	    }

	/** The final group must be padded to 4 characters. **/
	if (UNLIKELY(n_chars > 0 && n_chars + n_pads != 4))
	    {
	    mssError(1, "B64", "Failed to decode: incomplete group after %zu characters.", len);
	    return -1;
	    }

	/** Decode a final 2 or 3 characters as 1 or 2 bytes (respectively). **/
	const size_t n_bytes = (n_chars == 0) ? 0 : (size_t)n_chars - 1;
	if (UNLIKELY(dst_size - o <= n_bytes))
	    {
	    mssError(1, "B64", "Failed to decode: %zu-byte dst is too small.", dst_size);
	    return -1;
	    }
	if (n_chars == 2)
	    {
	    out[o++] = (unsigned char)(group >> 4);
	    }
	else if (n_chars == 3)
	    {
	    out[o++] = (unsigned char)(group >> 10);
	    out[o++] = (unsigned char)(group >> 2);
	    }
	out[o] = '\0';

    return (ssize_t)o;
    }


/*** Remove every character outside the base64 alphabet and '=' from a
 *** string, in place.
 ***
 *** @param str The string to purify.  str[len] may be overwritten by the
 ***	null at the end of the result.
 *** @param len The length of str, or -1 to use strlen(str).
 *** @returns The length of the result, excluding the null, or -1 with
 ***	mssError() if len is invalid.
 ***/
ssize_t
b64Purify(char* str, ssize_t len)
    {
    size_t n;
    size_t o = 0;

	if (UNLIKELY(b64_i_Length(str, len, "purify", &n) < 0)) return -1;

	/** Keep the alphabet and padding. **/
	for (size_t i = 0; i < n; i++)
	    if (str[i] == '=' || b64_i_Value((unsigned char)str[i]) >= 0)
		str[o++] = str[i];
	str[o] = '\0';

    return (ssize_t)o;
    }
