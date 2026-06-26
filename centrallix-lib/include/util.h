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
/* Date:	May 26, 2011						*/
/* Description:	Collection of utilities including:			*/
/* 		- Utilities for parsing numbers.			*/
/* 		- The timer utility for benchmarking code.		*/
/* 		- snprint_bytes() for formatting a byte count.		*/
/* 		- snprint_commas_llu() for formatting large numbers.	*/
/* 		- fprint_mem() for printing memory stats.		*/
/* 		- min() and max() for handling numbers.			*/
/* 		- The check functions for reliably printing debug data.	*/
/************************************************************************/

#include <stdio.h>

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

#ifndef __cplusplus

/*** @brief Returns the smaller of two values.
 *** 
 *** @param a The first value.
 *** @param b The second value.
 *** @return The smaller of the two values.
 *** 
 *** @note This macro uses GCC extensions to ensure type safety.
 ***/
#define min(a, b) \
    ({ \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    (_a < _b) ? _a : _b; \
    })

/*** @brief Returns the larger of two values.
 *** 
 *** @param a The first value.
 *** @param b The second value.
 *** @return The larger of the two values.
 *** 
 *** @note This macro uses GCC extensions to ensure type safety.
 ***/
#define max(a, b) \
    ({ \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    (_a > _b) ? _a : _b; \
    })

#endif  /* __cplusplus */

#endif	/* UTILITY_H */
