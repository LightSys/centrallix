#ifndef BASE64_H
#define BASE64_H

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

#include <stddef.h>
#include <sys/types.h>

/** Flags. **/
#define B64_F_NONE		(0)
#define B64_F_CRLF		(1<<0)	/* Wrap lines with "\r\n", not "\n". */
#define B64_F_SKIP_INVALID	(1<<1)	/* Decoding ignores non-alphabet characters. */

size_t b64EncodedSize(size_t src_len, size_t line_len, int flags);
size_t b64DecodedSizeMax(size_t src_len);
size_t b64WrappedSize(size_t src_len, size_t line_len, int flags);
ssize_t b64Encode(char* dst, size_t dst_size, const void* src, ssize_t src_len, size_t line_len, int flags);
ssize_t b64Decode(void* dst, size_t dst_size, const char* src, ssize_t src_len, int flags);
ssize_t b64Wrap(char* dst, size_t dst_size, const char* src, ssize_t src_len, size_t line_len, int flags);
ssize_t b64Purify(char* str, ssize_t len);

#endif /* BASE64_H */
