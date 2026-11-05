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
/* 		- snprintBytes() for formatting a byte count.		*/
/* 		- snprintCommasLlu() for formatting large numbers.	*/
/* 		- fprintMem() for printing memory stats.		*/
/************************************************************************/

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "expect.h"
#include "mtsession.h"
#include "newmalloc.h"
#include "range.h"
#include "warn.h"

#include "util.h"

/**
 * Converts a string to the integer value represented
 *  by the string, based on strtol.
 * @param nptr   the string
 * @param endptr if not NULL will be assigned the address of the first invalid character
 * @param base   the base to convert with
 * @return the converted int or INT_MAX/MIN on error
 */
int strtoi(const char *nptr, char **endptr, int base){
    long tmp;
    //try to convert
    tmp = strtol(nptr,endptr,base);
    //check for errors in conversion to long
    if(tmp == LONG_MAX){
        return INT_MAX;
    }
    if(tmp==LONG_MIN){
        return INT_MIN;
    }
    //now check for error in conversion to int
    if(tmp>INT_MAX){
        errno = ERANGE;
        return INT_MAX;   
    }else if(tmp<INT_MIN){
        errno = ERANGE;
        return INT_MIN;
    }
    //return as tmp;
    return (int)tmp;
}

/**
 * Converts a string to the unsigned integer value represented
 *  by the string, based on strtoul
 * @param nptr   the string
 * @param endptr if not NULL will be assigned the address of the first invalid character
 * @param base   the base to convert with
 * @return the converted int or UINT_MAX on error
 */
unsigned int strtoui(const char *nptr, char **endptr, int base){
    long long tmp;
    //try to convert
    tmp = strtoll(nptr,endptr,base);
    
    //check for errors in conversion to long
    if(tmp == ULONG_MAX){
        return UINT_MAX;
    }
    //now check for error in conversion to int
    if(tmp>UINT_MAX){
        errno = ERANGE;
        return UINT_MAX;   
    }  
    //return as tmp;
    return (unsigned int)tmp;
}

static const char* const UNITS_CS[] = {"bytes", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB"};
static const char* const UNITS_METRIC[] = {"bytes", "KB", "MB", "GB", "TB", "PB", "EB"};
#define N_UNITS ((unsigned int)(sizeof(UNITS_CS) / sizeof(UNITS_CS[0])))

/*** Displays a size in bytes using the largest unit where the printed result
 *** would be at least 1.0.  Units up to the exbibyte (EiB) and exabyte (EB) are
 *** supported, which is enough for any unsigned long: the largest possible
 *** value is 18,446,744,073,709,551,615, which is just under 16 EiB (or
 *** approximately 18.45 EB).
 *** 
 *** @param buf The buffer to which new text will be written, using snprintf().
 *** @param buf_size The amount of space in the buffer, passed to snprintf().
 *** 	A `SNPRINT_BYTES_BUF_SIZE` buffer holds any result without truncating.
 *** @param bytes The number of bytes, which will be formatted and written
 *** 	to the buffer.
 *** @returns The length the result would have had if buf_size were unlimited,
 *** 	not counting the null terminator, as snprintf() does.  A value of
 *** 	buf_size or more means the result was truncated.  Negative on error.
 ***/
int
snprintBytes(char* buf, const size_t buf_size, unsigned long bytes)
    {
	const char* const* units = (USE_METRIC) ? UNITS_METRIC : UNITS_CS;
	const double unit_size = (USE_METRIC) ? 1000.0 : 1024.0;
	
	/** Search for the largest unit where the value would be at least 1. **/
	const double size = (double)bytes;
	for (unsigned char i = N_UNITS - 1; i >= 1u; i--)
	    {
	    const double denominator = pow(unit_size, i);
	    if (size >= denominator)
		{
		double converted_size = size / denominator;

		/** Move up a unit if rounding would print a size equal to one of the next unit. **/
		const int decimals = (converted_size >= 1000.0) ? 1 : 2;
		if (roundTo(converted_size, decimals) >= unit_size && i + 1u < N_UNITS)
		    {
		    converted_size /= unit_size;
		    i++;
		    }

		if (converted_size >= 100.0)
		    return snprintf(buf, buf_size, "%.5g %s", converted_size, units[i]);
		else if (converted_size >= 10.0)
		    return snprintf(buf, buf_size, "%.4g %s", converted_size, units[i]);
		else /* if (converted_size >= 1.0) - Always true. */
		    return snprintf(buf, buf_size, "%.3g %s", converted_size, units[i]);
		}
	    }
	
	/** None of the larger units work, so we just use bytes. **/
    return snprintf(buf, buf_size, "%lu %s", bytes, units[0]);
    }
#undef N_UNITS

/*** Print a large number formatted with commas to a buffer.
 *** 
 *** @param buf The buffer to print the number into.  Only written if
 *** 	`buf_size` is nonzero.
 *** @param buf_size The size of the buffer, including room for the null
 *** 	terminator.  A `SNPRINT_COMMAS_LLU_BUF_SIZE` buffer holds any
 *** 	unsigned long long without truncating.
 *** @param value The value to write into the buffer.
 *** @returns The length the result would have had if `buf_size` were
 *** 	unlimited, not counting the null terminator, as snprintf() does.  A
 *** 	value of `buf_size` or more means the result was truncated.
 ***/
int
snprintCommasLlu(char* buf, size_t buf_size, unsigned long long value)
    {
	/*** Write the number to a scratch buffer in reverse order, adding
	 *** commas as they are needed.  The largest unsigned long long is 20
	 *** digits and 6 commas, so tmp is never the limiting factor.
	 ***/
	char tmp[32];
	unsigned int ti = 0;
	do  {
	    if (ti % 4 == 3) tmp[ti++] = ',';
	    tmp[ti++] = '0' + (value % 10);
	    value /= 10;
	    }
	    while (value > 0 && ti < sizeof(tmp) - 1);
	
	/** Copy it back out in the right order, truncating as snprintf() does. **/
	if (buf_size > 0)
	    {
	    const unsigned int outlen = min(ti, buf_size - 1u);
	    for (unsigned int i = 0u; i < outlen; i++) buf[i] = tmp[ti - i - 1];
	    buf[outlen] = '\0';
	    }
    
    return (int)ti;
    }

/*** Print a summary of the current memory in use to the file pointer.
 ***
 *** @param out The file pointer for printing.  Defaults to stdout when NULL.
 *** @returns 0 if successful, or -1 if an error occurs.
 ***/
int
fprintMem(FILE* out)
    {
    FILE* fp = NULL;
    int rval = -1;

	/** Handle edge cases. **/
	if (out == NULL)
	    out = stdout;

	/** Open the OS stats file to read memory. **/
	fp = fopen("/proc/self/statm", "r");
	if (UNLIKELY(fp == NULL))
	    {
	    mssError(1, "UTIL",
		"fopen(\"/proc/self/statm\", \"r\") failed: %s.",
		strerror(errno)
	    );
	    goto end;
	    }
	
	/** Get page counts. **/
	long size, resident, share, text, lib, data, dt;
	if (UNLIKELY(fscanf(fp, "%ld %ld %ld %ld %ld %ld %ld",
	    &size, &resident, &share, &text, &lib, &data, &dt) != 7))
	    {
	    mssError(1, "UTIL", "Failed to read memory info.");
	    goto end;
	    }
	
	/** Get page size. **/
	const long page_size = sysconf(_SC_PAGESIZE); /* in bytes */
	if (UNLIKELY(page_size < 0))
	    {
	    mssError(1, "UTIL", "Failed to get page size (error code: %ld).", page_size);
	    goto end;
	    }
	
	/** Get the number of resident bytes used. **/
	const unsigned long resident_bytes = (unsigned long)resident * (unsigned long)page_size;
	char buf[SNPRINT_BYTES_BUF_SIZE];
	if (snprintBytes(buf, sizeof(buf), resident_bytes) < 0)
	    {
	    mssError(1, "UTIL", "Failed to format memory info.");
	    goto end;
	    }
	
	/** fprintf() out data. **/
	fprintf(out, "Memory used: %lu bytes (%s)\n", resident_bytes, buf);
	fprintf(out,
	    "Share %ldb, Text %ldb, Lib %ldb, Data %ldb\n",
	    share * page_size, text * page_size, lib * page_size, data * page_size
	);
    
	/** Success. **/
	rval = 0;

    end:
	if (rval != 0)
	    mssError(0, "UTIL", "Failed to print memory.");

	/** Clean up. **/
	if (LIKELY(fp != NULL)) warnFail(fclose(fp));

	return rval;
    }
