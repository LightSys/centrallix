#ifndef ASCII85_H
#define ASCII85_H

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

#include <stddef.h>
#include <sys/types.h>

size_t a85EncodedSizeMax(size_t src_len);
ssize_t a85Encode(char* dst, size_t dst_size, const void* src, size_t src_len);

#endif /* ASCII85_H */
