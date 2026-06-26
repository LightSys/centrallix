#ifndef UTILITY_H
#define	UTILITY_H

/************************************************************************/
/* Centrallix Application Server System					*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module:	util.c, util.h						*/
/* Author:	Micah Shennum and Israel Fuller				*/
/* Date:	May 26, 2011 and October 13, 2025 (respectively)	*/
/* Description:	Collection of utilities including:			*/
/* 		- Utilities for parsing numbers.			*/
/* 		- snprint_bytes() for formatting a byte count.		*/
/* 		- snprint_commas_llu() for formatting large numbers.	*/
/* 		- fprint_mem() for printing memory stats.		*/
/************************************************************************/


#include <stdio.h>

/** Temporary stub to include refactored libraries until dependent code is updated. **/
#include "timer.h"
#include "check.h"
#include "range.h"

#ifdef	__cplusplus
extern "C" {
#endif

    int strtoi(const char *nptr, char **endptr, int base);
    unsigned int strtoui(const char *nptr, char **endptr, int base);

    char* snprint_bytes(char* buf, const size_t buf_size, unsigned int bytes);
    char* snprint_commas_llu(char* buf, size_t buf_size, unsigned long long value);
    void fprint_mem(FILE* out);

#ifdef	__cplusplus
}
#endif

#endif	/* UTILITY_H */
