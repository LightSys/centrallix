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
/* 		- The timer utility for benchmarking code.		*/
/* 		- snprint_bytes() for formatting a byte count.		*/
/* 		- snprint_commas_llu() for formatting large numbers.	*/
/* 		- fprint_mem() for printing memory stats.		*/
/* 		- min() and max() for handling numbers.			*/
/* 		- The check functions for reliably printing debug data.	*/
/************************************************************************/

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "newmalloc.h"
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

/*** snprint_bytes() allows one to pick between CS units, where the kibibyte
 *** (KiB) is 1024 bytes, and metric units where the kilobyte (KB) is 1000 bytes.
 *** Fun Fact: Windows uses kibibytes, but displays them as KB.
 ***/
#define USE_METRIC false
static char* units_cs[] = {"bytes", "KiB", "MiB", "GiB"};
static char* units_metric[] = {"bytes", "KB", "MB", "GB"};
#define N_UNITS ((unsigned int)(sizeof(units_cs) / sizeof(units_cs[0])))

/*** Displays a size in bytes using the largest unit where the result would be
 *** at least 1.0. Note that units larger than GB and GiB are not supported
 *** because the largest possible unsigned int is 4,294,967,295, which is
 *** exactly 4 GiB (or approximately 4.29 GB).
 *** 
 *** @param buf The buffer to which new text will be written, using snprintf().
 *** @param buf_size The amount of space in the buffer, passed to snprintf().
 *** 	It is recommended to have at least 12 characters available.
 *** @param bytes The number of bytes, which will be formatted and written
 *** 	to the buffer..
 *** @returns buf, for chaining.
 ***/
char*
snprint_bytes(char* buf, const size_t buf_size, unsigned int bytes)
    {
	char** units = (USE_METRIC) ? units_metric : units_cs;
	const double unit_size = (USE_METRIC) ? 1000.0 : 1024.0;
	
	/** Search for the largest unit where the value would be at least 1. **/
	const double size = (double)bytes;
	for (unsigned char i = N_UNITS - 1; i >= 1u; i--)
	    {
	    const double denominator = pow(unit_size, i);
	    if (size >= denominator)
		{
		const double converted_size = size / denominator;
		if (converted_size >= 100.0)
		    snprintf(buf, buf_size, "%.5g %s", converted_size, units[i]);
		else if (converted_size >= 10.0)
		    snprintf(buf, buf_size, "%.4g %s", converted_size, units[i]);
		else /* if (converted_size >= 1.0) - Always true. */
		    snprintf(buf, buf_size, "%.3g %s", converted_size, units[i]);
		return buf;
		}
	    }
	
	/** None of the larger units work, so we just use bytes. **/
	snprintf(buf, buf_size, "%u %s", bytes, units[0]);
    
    return buf;
    }
#undef N_UNITS

/*** Print a large number formatted with comas to a buffer.
 *** 
 *** @param buf The buffer to print the number into.
 *** @param buf_size The maximum number of characters to add to the buffer.
 *** @param value The value to write into the buffer.
 *** @returns `buf`, or NULL if `buf_size` is 0.
 */
char*
snprint_commas_llu(char* buf, size_t buf_size, unsigned long long value)
    {
	if (buf_size == 0) return NULL;
	if (value == 0)
	    {
	    if (buf_size > 1) { buf[0] = '0'; buf[1] = '\0'; }
	    else buf[0] = '\0';
	    return buf;
	    }
	
	/*** Write the number to the string in reverse order, adding commas as
	 *** they are needed.
	 ***/
	char tmp[32];
	unsigned int ti = 0;
	while (value > 0 && ti < sizeof(tmp) - 1)
	    {
	    if (ti % 4 == 3) tmp[ti++] = ',';
	    tmp[ti++] = '0' + (value % 10);
	    value /= 10;
	    }
	tmp[ti] = '\0';
	
	unsigned int outlen = min(ti, buf_size - 1u);
	for (unsigned int i = 0u; i < outlen; i++) buf[i] = tmp[ti - i - 1];
	buf[outlen] = '\0';
    
    return buf;
    }

/** Print summary the current memory in use to the file pointer. **/
void
fprint_mem(FILE* out)
    {
	FILE* fp = fopen("/proc/self/statm", "r");
	if (fp == NULL) { perror("fopen()"); return; }
	
	/** Get page counts. **/
	long size, resident, share, text, lib, data, dt;
	if (fscanf(fp, "%ld %ld %ld %ld %ld %ld %ld",
	    &size, &resident, &share, &text, &lib, &data, &dt) != 7)
	    {
	    fprintf(stderr, "Failed to read memory info\n");
	    check(fclose(fp)); /* Failure ignored. */
	    return;
	    }
	check(fclose(fp)); /* Failure ignored. */
	
	/** Get page size. **/
	const long page_size = sysconf(_SC_PAGESIZE); /* in bytes */
	if (page_size == -1)
	    {
	    fprintf(stderr, "Failed to get page size.\n");
	    return;
	    }
	
	/** Get the number of resident bytes used. **/
	const long resident_bytes = resident * page_size;
	char buf[16];
	snprint_bytes(buf, sizeof(buf), (unsigned int)(resident_bytes));
	
	/** fprintf() out data. **/
	fprintf(out, "Memory used: %ld bytes (%s)\n", resident_bytes, buf);
	fprintf(out,
	    "Share %ldb, Text %ldb, Lib %ldb, Data %ldb\n",
	    share * page_size, text * page_size, lib * page_size, data * page_size
	);
    
    return;
    }

/*** Get the current monotonic time in seconds.
 ***
 *** @returns The current monotonic time as a fractional number of seconds.
 ***/
static double
get_time(void)
    {
    struct timespec ts;
    
	clock_gettime(CLOCK_MONOTONIC, &ts);
    
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1.0e9f;
    }

/*** Initialize a timer struct.
 ***
 *** @param timer The timer to initialize.
 *** @returns `timer`, for chaining.
 ***/
pTimer
timer_init(pTimer timer)
    {
	if (timer == NULL) return NULL;
	timer->start = NAN;
	timer->total = 0.0;
    
    return timer;
    }

/*** Allocate and initialize a new timer.
 ***
 *** @returns A newly allocated timer, or NULL if allocation fails.
 ***/
pTimer
timer_new(void)
    {
    return timer_init(check_ptr(nmMalloc(sizeof(Timer))));
    }

/*** Start timing.
 ***
 *** @param timer The timer to start.
 *** @returns `timer`, for chaining.
 ***/
pTimer
timer_start(pTimer timer)
    {
	if (!timer) return timer;
	timer->start = get_time();
    
    return timer;
    }

/*** Stop timing and add the elapsed time to the timer total.
 ***
 *** @param timer The timer to stop.
 *** @returns `timer`, for chaining.
 ***/
pTimer
timer_stop(pTimer timer)
    {
	if (!timer) return timer;
	timer->total += get_time() - timer->start;
    
    return timer;
    }

/*** Get the total accumulated time for a timer.
 ***
 *** @param timer The timer to read.
 *** @returns The total accumulated time in seconds, or NAN if `timer`
 *** 	is NULL.
 ***/
double
timer_get(pTimer timer)
    {
    return (timer) ? timer->total : NAN;
    }

/*** Reset a timer to its initial state so that it can be reused to time
 *** something else.
 ***
 *** @param timer The timer to reset.
 *** @returns `timer`, for chaining.
 ***/
pTimer
timer_reset(pTimer timer)
    {
    return timer_init(timer);
    }

/*** De-initialize a timer before it is freed.
 ***
 *** @param timer The timer to de-initialize.
 ***/
void
timer_de_init(pTimer timer) {}

/*** De-initialize and free a timer allocated by timer_new().
 ***
 *** @param timer The timer to free.
 ***/
void
timer_free(pTimer timer)
    {
	timer_de_init(timer);
	nmFree(timer, sizeof(Timer));
    
    return;
    }

/*** Function for failing on error, assuming the error came from a library or
 *** system function call, so that the error buffer is set to a valid value.
 ***/
void print_err(int code, const char* function_name, const char* file_name, const int line_number)
    {
	/** Create a descriptive error message. **/
	char error_buf[BUFSIZ];
	snprintf(error_buf, sizeof(error_buf), "%s:%d: %s failed", file_name, line_number, function_name);
	
	/** Print it with as much info as we can reasonably find. **/
	if (errno != 0) perror(error_buf);
	else if (code != 0) fprintf(stderr, "%s (error code %d).\n", error_buf, code);
	else fprintf(stderr, "%s.\n", error_buf);
    
    return;
    }
