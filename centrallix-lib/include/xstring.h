#ifndef _XSTRING_H
#define _XSTRING_H

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
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	xstring.c, xstring.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 17, 1998					*/
/* Description:	Extensible string support module.  Implements an auto-	*/
/*		realloc'ing string data structure.			*/
/************************************************************************/

#include <stdlib.h>
#include <stdarg.h>

#define XS_BLK_SIZ	256
#define XS_PRINTF_BUFSIZ 1024

typedef struct _XS
    {
    Magic_t	Magic;
    CXSEC_DS_BEGIN;
    char	InitBuf[XS_BLK_SIZ];
    char*	String;
    int		Length;
    int		AllocLen;
    CXSEC_DS_END;
    }
    XString, *pXString;


/** XString methods **/
int xsInit(pXString this);
int xsDeInit(pXString this);
int xsCheckAlloc(pXString this, int addl_needed);
int xsConcatenate(pXString this, char* text, int len);
int xsCopy(pXString this, char* text, int len);
char* xsString(pXString this);
char* xsStringEnd(pXString this);
int xsLength(pXString this);
int xsPrintf(pXString this, char* fmt, ...);
int xsConcatPrintf(pXString this, char* fmt, ...);
int xsWrite(pXString this, char* buf, int len, int offset, int flags);
int xsRTrim(pXString this);
int xsLTrim(pXString this);
int xsTrim(pXString this);
int xsFind(pXString this,char* find,int findlen, int offset);
int xsFindRev(pXString this,char* find,int findlen, int offset);
int xsSubst(pXString this, int offset, int len, char* rep, int replen);
int xsReplace(pXString this, char* find, int findlen, int offset, char* rep, int replen);
int xsInsertAfter(pXString this, char* ins, int inslen, int offset);
int xsGenPrintf(int (*write_fn)(), void* write_arg, char** buf, int* buf_size, const char* fmt, ...);
int xsGenPrintf_va(int (*write_fn)(), void* write_arg, char** buf, int* buf_size, const char* fmt, va_list va);
int xsQPrintf(pXString this, char* fmt, ...);
int xsQPrintf_va(pXString this, char* fmt, va_list va);
int xsConcatQPrintf(pXString this, char* fmt, ...);
pXString xsNew();
void xsFree(pXString this);

/** Needed utiliy function **/
size_t chrCharLength(char* string);

#define XS_U_SEEK	2

#endif /* _XSTRING_H */

