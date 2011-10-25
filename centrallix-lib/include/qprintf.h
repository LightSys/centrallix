#ifndef _QPRINTF_H
#define _QPRINTF_H

#ifdef CXLIB_INTERNAL
#include "cxsec.h"
#include "magic.h"
#else
#include "cxlib/cxsec.h"
#include "cxlib/magic.h"
#endif

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

typedef int (*qpf_grow_fn_t)(char**, size_t*, size_t, void*, size_t);

typedef struct _QPS
    {
    unsigned int	Errors;		/* QPF_ERR_T_xxx */
    }
    QPSession, *pQPSession;

#define QPF_ERR_T_NOTIMPL	1	/* unimplemented feature */
#define QPF_ERR_T_BUFOVERFLOW	2	/* dest buffer too small */
#define QPF_ERR_T_INSOVERFLOW	4	/* NLEN or *LEN restriction occurred */
#define QPF_ERR_T_NOTPOSITIVE	8	/* %POS conversion but number was neg */
#define QPF_ERR_T_BADSYMBOL	16	/* &SYM filter did not match the data. */
#define QPF_ERR_T_MEMORY	32	/* Memory allocation failed (internal). */
#define QPF_ERR_T_BADLENGTH	64	/* Length for NLEN or *LEN was invalid */
#define QPF_ERR_T_BADFORMAT	128	/* Format string was invalid */
#define QPF_ERR_T_RESOURCE	256	/* Internal resource limit hit */
#define QPF_ERR_T_NULL		512	/* NULL pointer passed (e.g. as a string) */
#define QPF_ERR_T_INTERNAL	1024	/* Uncorrectable internal error. */
#define QPF_ERR_T_BADFILE	2048	/* Bad filename for &FILE filter */
#define QPF_ERR_T_BADPATH	4096	/* Bad pathname for &PATH filter */
#define QPF_ERR_T_BADCHAR	8192	/* Bad character for filter (e.g. an octothorpe for &DB64) */

#define QPERR(x) (s->Errors |= (x))

/*** QPrintf methods ***/
pQPSession qpfOpenSession();
int qpfCloseSession(pQPSession s);
int qpfClearErrors(pQPSession s);
unsigned int qpfErrors(pQPSession s);
int qpfPrintf(pQPSession s, char* str, size_t size, const char* format, ...);
int qpfPrintf_va(pQPSession s, char* str, size_t size, const char* format, va_list ap);
void qpfRegisterExt(char* ext_spec, int (*ext_fn)(), int is_source);

/*** Raw interface - should only be used internally by cxlib **/
int qpfPrintf_va_internal(pQPSession s, char** str, size_t* size, qpf_grow_fn_t grow_fn, void* grow_arg, const char* format, va_list ap);

#endif /* _QPRINTF_H */
