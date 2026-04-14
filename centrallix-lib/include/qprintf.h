#ifndef _QPRINTF_H
#define _QPRINTF_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2006 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	qprintf.c, qprintf.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	January 31, 2006 					*/
/* Description:	Quoting Printf routine, used to make sure that		*/
/*		injection type attacks don't occur when building	*/
/*		strings.  These functions do not support some of the	*/
/*		more advanced (and dangerous) features of the normal	*/
/*		printf() library calls.					*/
/************************************************************************/

#include <stdarg.h>

#ifdef CXLIB_INTERNAL
#include "cxsec.h"
#include "magic.h"
#else
#include "cxlib/cxsec.h"
#include "cxlib/magic.h"
#endif

#define QPF_ERR_T_NO_ERRORS	(0)	/* a default error buffer value with no errors */
#define QPF_ERR_T_NOTIMPL	(1<<0)	/* unimplemented feature */
#define QPF_ERR_T_BUFOVERFLOW	(1<<1)	/* dest buffer too small */
#define QPF_ERR_T_INSOVERFLOW	(1<<2)	/* NLEN or *LEN restriction occurred */
#define QPF_ERR_T_NOTPOSITIVE	(1<<3)	/* %POS conversion but number was neg */
#define QPF_ERR_T_BADSYMBOL	(1<<4)	/* &SYM filter did not match the data. */
#define QPF_ERR_T_MEMORY	(1<<5)	/* Memory allocation failed (internal). */
#define QPF_ERR_T_BADLENGTH	(1<<6)	/* Length for NLEN or *LEN was invalid */
#define QPF_ERR_T_BADFORMAT	(1<<7)	/* Format string was invalid */
#define QPF_ERR_T_RESOURCE	(1<<8)	/* Internal resource limit hit */
#define QPF_ERR_T_NULL		(1<<9)	/* NULL pointer passed (e.g. as a string) */
#define QPF_ERR_T_INTERNAL	(1<<10)	/* Unrecoverable internal error. */
#define QPF_ERR_T_BADFILE	(1<<11)	/* Bad filename for &FILE filter */
#define QPF_ERR_T_BADPATH	(1<<12)	/* Bad pathname for &PATH filter */
#define QPF_ERR_T_BADCHAR	(1<<13)	/* Bad character for filter (e.g. an octothorpe for &DB64) */
#define QPF_ERR_COUNT           (14) /* The number of errors listed above. */

/*** A function to grow a string buffer.
 *** 
 *** @param str    The string buffer being grown.
 *** @param size   A pointer to the current size of the string buffer.
 *** @param offset An offset up to which data must be preserved.
 *** @param args   Arguments for growing the buffer.
 *** @param req    The requested size.
 *** @returns True (1) if successful, and false (0) if an error occurs.
 ***/
// typedef int (*qpf_grow_fn_t)(char** str, size_t* size, size_t offset, void* args, size_t req);
typedef int (*qpf_grow_fn_t)(char**, size_t*, size_t, void*, size_t);

/*** Stores information about a qprint session, including details about errors
 *** such as parsing and formatting issues that have occurred.
 *** 
 *** @param Errors An error mask indicating any errors that have occurred, or
 *** 	0 (aka. `QPF_ERR_T_NO_ERRORS`) if no errors have occurred.
 *** @param ErrorLines An array where each index represents a type of error in
 *** 	order (e.g. 0 represents `QPF_ERR_T_NOTIMPL`) and each value represents
 *** 	the line number where the error occurred, or 0 if this error type has
 *** 	not occurred during the session.
 ***/
typedef struct _QPS
    {
    unsigned int	Errors;		/* QPF_ERR_T_xxx */
    unsigned short	ErrorLines[QPF_ERR_COUNT];
    }
    QPSession, *pQPSession;

/*** QPrintf methods ***/
pQPSession qpfOpenSession(void);
int qpfCloseSession(pQPSession s);
int qpfClearErrors(pQPSession s);
unsigned int qpfErrors(pQPSession s);
void qpfLogErrors(pQPSession s);
int qpfPrintf(pQPSession s, char* str, size_t size, const char* format, ...);
int qpfPrintf_va(pQPSession s, char* str, size_t size, const char* format, va_list ap);
void qpfRegisterExt(char* ext_spec, int (*ext_fn)(), int is_source);

/*** Raw interface - should only be used internally by cxlib **/
int qpfPrintf_va_internal(pQPSession s, char** str, size_t* size, qpf_grow_fn_t grow_fn, void* grow_arg, const char* format, va_list ap);

#endif /* _QPRINTF_H */
