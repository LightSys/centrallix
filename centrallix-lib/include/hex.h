#ifndef HEX_H
#define HEX_H

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

#include <stddef.h>
#include <sys/types.h>

/** Flags. **/
#define HEX_F_NONE		(0)
#define HEX_F_TRUNCATE		(1<<0)	/* Encoding cuts off what does not fit in dst. */

size_t hexEncodedSize(size_t src_len);
size_t hexDecodedSize(size_t src_len);
ssize_t hexEncode(char* dst, size_t dst_size, const void* src, ssize_t src_len, int flags);
ssize_t hexDecode(void* dst, size_t dst_size, const char* src, ssize_t src_len, int flags);

#endif /* HEX_H */
