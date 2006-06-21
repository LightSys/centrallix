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

/**CVSDATA***************************************************************

    $Id: qprintf.h,v 1.1 2006/06/21 21:22:44 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/qprintf.h,v $

    $Log: qprintf.h,v $
    Revision 1.1  2006/06/21 21:22:44  gbeeley
    - Preliminary versions of strtcpy() and qpfPrintf() calls, which can be
      used for better safety in handling string data.

 **END-CVSDATA***********************************************************/

#include <stdarg.h>


/*** QPrintf methods ***/
int qpfPrintf(char* str, size_t size, const char* format, ...);
int qpfPrintf_va(char* str, size_t size, const char* format, va_list ap);
void qpfRegisterExt(char* ext_spec, int (*ext_fn)(), int is_source);

/*** Raw interface - should only be used internally by cxlib **/
int qpfPrintf_va_internal(char** str, size_t* size, int (*grow_fn)(), void* grow_arg, const char* format, va_list ap);

#endif /* _QPRINTF_H */
