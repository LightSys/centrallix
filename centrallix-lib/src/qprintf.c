#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include "qprintf.h"
#include "mtask.h"
#include "newmalloc.h"
#include "cxsec.h"

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

    $Id: qprintf.c,v 1.1 2006/06/21 21:22:44 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/src/qprintf.c,v $

    $Log: qprintf.c,v $
    Revision 1.1  2006/06/21 21:22:44  gbeeley
    - Preliminary versions of strtcpy() and qpfPrintf() calls, which can be
      used for better safety in handling string data.

 **END-CVSDATA***********************************************************/

#ifndef __builtin_expect
#define __builtin_expect(e,c) (e)
#endif

/*** maximum # of externally-defined specifiers ***/
#define QPF_MAX_EXTS		(64)

/*** maximum # of specifiers in a series in the format string ***/
#define QPF_MAX_SPECS		(8)

/*** translation matrix size ***/
#define QPF_MATRIX_SIZE		(256)

/*** builtin source specifiers ***/
#define QPF_SPEC_T_STARTSRC	(1)
#define	QPF_SPEC_T_INT		(1)
#define QPF_SPEC_T_STR		(2)
#define QPF_SPEC_T_POS		(3)
#define QPF_SPEC_T_DBL		(4)
#define QPF_SPEC_T_NSTR		(5)
#define QPF_SPEC_T_CHR		(6)
#define QPF_SPEC_T_ENDSRC	(6)

/*** builtin filtering specifiers ***/
#define QPF_SPEC_T_STARTFILT	(7)
#define QPF_SPEC_T_QUOT		(7)
#define QPF_SPEC_T_DQUOT	(8)
#define QPF_SPEC_T_SYM		(9)
#define QPF_SPEC_T_ESCQ		(10)
#define QPF_SPEC_T_NLEN		(11)
#define QPF_SPEC_T_WS		(12)
#define QPF_SPEC_T_ESCWS	(13)
#define QPF_SPEC_T_ESCSP	(14)
#define QPF_SPEC_T_UNESC	(15)
#define QPF_SPEC_T_SSYB		(16)
#define QPF_SPEC_T_DSYB		(17)
#define QPF_SPEC_T_FILE		(18)
#define QPF_SPEC_T_PATH		(19)
#define QPF_SPEC_T_HEX		(20)
#define QPF_SPEC_T_DHEX		(21)
#define QPF_SPEC_T_B64		(22)
#define QPF_SPEC_T_DB64		(23)
#define QPF_SPEC_T_RF		(24)
#define QPF_SPEC_T_RR		(25)
#define QPF_SPEC_T_HTE		(26)
#define QPF_SPEC_T_DHTE		(27)
#define QPF_SPEC_T_URL		(28)
#define QPF_SPEC_T_DURL		(29)
#define QPF_SPEC_T_NLSET	(30)
#define QPF_SPEC_T_NRSET	(31)
#define QPF_SPEC_T_NZRSET	(32)
#define QPF_SPEC_T_SQLARG	(33)
#define QPF_SPEC_T_SQLSYM	(34)
#define QPF_SPEC_T_HTDATA	(35)
#define QPF_SPEC_T_ENDFILT	(35)
#define QPF_SPEC_T_MAXSPEC	(35)

/** Names for specifiers as used in format string - must match the above. **/
const char*
qpf_spec_names[] = 
    {
    NULL,	/* 0 */
    "INT",	/* 1 */
    "STR",
    "POS",
    "DBL",
    "nSTR",
    "CHR",
    "QUOT",
    "DQUOT",
    "SYM",
    "ESCQ",
    "nLEN",
    "WS",
    "ESCWS",
    "ESCSP",
    "UNESC",
    "SSYB",
    "DSYB",
    "FILE",
    "PATH",
    "HEX",
    "DHEX",
    "B64",
    "DB64",
    "RF",
    "RR",
    "HTE",
    "DHTE",
    "URL",
    "DURL",
    "nLSET",
    "nRSET",
    "nZRSET",
    "SQLARG",
    "SQLSYM",
    "HTDATA",	/* 35 */
    NULL
    };

int qpf_spec_len[QPF_SPEC_T_MAXSPEC+1];

typedef struct
    {
    int		n_ext;
    int		is_init;
    char*	ext_specs[QPF_MAX_EXTS];
    int		(*ext_fns[QPF_MAX_EXTS])();
    char	is_source[QPF_MAX_EXTS];
    char*	quote_matrix[QPF_MATRIX_SIZE];
    char*	hte_matrix[QPF_MATRIX_SIZE];
    }
    QPF_t;
    
static QPF_t QPF = { n_ext:0, is_init:0 };


/*** qpfInitialize() - inits the QPF suite.
 ***/
int
qpfInitialize()
    {
    int i;

	memset(QPF.quote_matrix, 0, sizeof(QPF.quote_matrix));
	QPF.quote_matrix['\''] = "\\'";
	QPF.quote_matrix['"'] = "\\\"";

	memset(QPF.hte_matrix, 0, sizeof(QPF.hte_matrix));
	QPF.hte_matrix['<'] = "&lt;";
	QPF.hte_matrix['>'] = "&gt;";
	QPF.hte_matrix['&'] = "&amp;";
	QPF.hte_matrix['"'] = "&quot;";
	QPF.hte_matrix['\''] = "&#39;";
	QPF.hte_matrix['\0'] = "&#0;";

	for(i=0;i<=QPF_SPEC_T_MAXSPEC;i++)
	    {
	    if (qpf_spec_names[i]) 
		qpf_spec_len[i] = strlen(qpf_spec_names[i]);
	    }

	QPF.is_init = 1;

    return 0;
    }


/*** qpf_internal_itoa() - convert an integer into a string representation.
 *** this seems to perform better than snprintf(%d), even without 
 *** optimization enabled.
 ***/
static inline int
qpf_internal_itoa(char* dst, size_t dstlen, int i)
    {
    char ibuf[sizeof(int)*8*3/10+4];
    char* iptr = ibuf;
    int r;
    int i2 = i;
    int rval;
    if (i2 == 0)
	{
	*(iptr++) = '0';
	}
    else
	{
	while(i2)
	    {
	    r = abs(i2%10);
	    i2 /= 10;
	    *(iptr++) = '0' + r;
	    }
	if (i < 0) *(iptr++) = '-';
	}
    rval = iptr - ibuf;
    while(iptr > ibuf && dstlen > 1)
	{
	*(dst++) = *(--iptr);
	dstlen--;
	}
    *dst = '\0';
    return rval;
    }


/*** qpfPrintf() - do the quoting printf operation, given a standard vararg
 *** function call.
 ***/
int 
qpfPrintf(char* str, size_t size, const char* format, ...)
    {
    va_list va;
    int rval;

	/** Grab the va ptr and call the _va version **/
	va_start(va, format);
	rval = qpfPrintf_va(str, size, format, va);
	va_end(va);

    return rval;
    }


/*** qpfPrintf_grow() - returns whether the additional size will fit.
 ***/
int
qpfPrintf_grow(char** str, size_t* size, void* arg, int req_size)
    {
    return (*size) >= req_size;
    }


/*** qpfPrintf_va() - same as qpfPrintf(), but takes a va_list instead of
 *** a list of arguments.
 ***/
int 
qpfPrintf_va(char* str, size_t size, const char* format, va_list ap)
    {
    return qpfPrintf_va_internal(&str, &size, qpfPrintf_grow, NULL, format, ap);
    }


/*** qpf_internal_Translate() - do a translation copy from one buffer to
 *** another, respecting a soft and hard limit, and using a given char
 *** translation table.  Returns the number of chars that would have been
 *** placed in dstbuf had there been enough room (or, if there was enough
 *** room, the actual # chars placed in dstbuf); NOTE - does NOT return
 *** the number of chars pulled from the srcbuf!!!
 ***/
static inline int
qpf_internal_Translate(const char* srcbuf, size_t srcsize, char** dstbuf, int* dstoffs, int* dstsize, int limit, char* table[], int(*grow_fn)(), void* grow_arg)
    {
    int rval = 0;
    int tlen, i;
    char* trans;
    int nogrow = (grow_fn == NULL);

	if (srcsize)
	    {
	    rval += srcsize;
	    for(i=0;i<srcsize;i++)
		{
		if (__builtin_expect(((trans = table[(unsigned char)(srcbuf[i])]) != NULL), 0))
		    {
		    tlen = strlen(trans);
		    if (__builtin_expect(limit >= tlen, 1))
			{
			rval += (tlen-1);
			if (__builtin_expect(!nogrow,1) && (__builtin_expect((*dstoffs)+tlen+1 <= (*dstsize), 1) || 
			      (grow_fn(dstbuf, dstsize, grow_arg, (*dstoffs)+tlen+1))))
			    {
			    while(*trans) (*dstbuf)[(*dstoffs)++] = *(trans++);
			    limit -= tlen;
			    }
			else
			    {
			    nogrow = 1;
			    }
			}
		    else
			{
			rval--;
			}
		    }
		else
		    {
		    if (__builtin_expect(limit > 0, 1))
			{
			if (__builtin_expect(!nogrow,1) && (__builtin_expect((*dstoffs)+2 <= (*dstsize), 1) || 
			      (grow_fn(dstbuf, dstsize, grow_arg, (*dstoffs)+2))))
			    {
			    (*dstbuf)[(*dstoffs)++] = srcbuf[i];
			    limit--;
			    }
			else
			    {
			    nogrow = 1;
			    }
			}
		    else
			{
			rval--;
			}
		    }
		}
	    }

    return rval;
    }


/*** qpfPrintf_va_internal() - does all of the guts work of the qpfPrintf
 *** family of functions
 ***/
int
qpfPrintf_va_internal(char** str, size_t* size, int (*grow_fn)(), void* grow_arg, const char* format, va_list ap)
    {
    const char* specptr;
    const char* endptr;
    int copied = 0;
    int rval;
    int cplen;
    char specchain[QPF_MAX_SPECS];
    int specchain_n[QPF_MAX_SPECS];
    int n_specs;
    int i;
    int n;
    int found;
    int intval;
    const char* strval;
    double dblval;
    char tmpbuf[64];
    char chrval;
    int cpoffset = 0;
    int oldcpoffset;
    int nogrow = 0;
    int startspec;
    int endspec;
    int maxdst;

	if (!QPF.is_init) 
	    {
	    if (qpfInitialize() < 0) return -ENOMEM;
	    }

	/** this all fall apart if there isn't at least room for the
	 ** null terminator!
	 **/
	if ((!*str || *size < 1) && !grow_fn(str, size, grow_arg, 1)) 
	    { rval = -EINVAL; goto error; }

	/** search for %this-and-that (specifiers), copy everything else **/
	do  {
	    /** Find the end of the non-specifier string segment **/
	    specptr = strchr(format, '%');
	    endptr = specptr?specptr:(format+strlen(format));

	    /** Copy the plain section of string **/
	    cplen = (endptr - format);
	    if (__builtin_expect(nogrow, 0)) cplen = 0;
	    if (__builtin_expect(cpoffset+cplen+1 > *size, 0) && (nogrow || !grow_fn(str, size, grow_arg, cpoffset+cplen+1)))
		{
		cplen = (*size)-cpoffset-1;
		nogrow = 1;
		}
	    if (cplen) memcpy((*str) + cpoffset, format, cplen);
	    cpoffset += cplen;
	    copied += (endptr - format);
	    format = endptr;

	    /** Handle specifiers **/
	    if (format[0] == '%')
		{
		format++;

		/** Simple specifiers **/
		if (__builtin_expect(format[0] == '%', 0))
		    {
		    if (__builtin_expect(!nogrow, 1) && (__builtin_expect(cpoffset+2 <= *size, 1) || (grow_fn(str, size, grow_arg, cpoffset+2))))
			(*str)[cpoffset++] = '%';
		    else
			nogrow = 1;
		    copied++;
		    format++;
		    }
		else if (__builtin_expect(format[0] == '&',0))
		    {
		    if (__builtin_expect(!nogrow, 1) && (__builtin_expect(cpoffset+2 <= *size, 1) || (grow_fn(str, size, grow_arg, cpoffset+2))))
			(*str)[cpoffset++] = '&';
		    else
			nogrow = 1;
		    copied++;
		    format++;
		    }
		else if (__builtin_expect(format[0] == ']',0))
		    {
		    format++;
		    }
		else if (__builtin_expect(format[0] == '[',0))
		    {
		    format++;
		    intval = va_arg(ap, int);
		    if (!intval)
			{
			while(*format && format[0] != '%' && format[1] != ']') 
			    format++;
			if (*format)
			    format += 2;
			}
		    }
		else
		    {
		    /** Source data specifier.  Build the specifier chain **/
		    n_specs = 0;
		    startspec = QPF_SPEC_T_STARTSRC;
		    endspec = QPF_SPEC_T_ENDSRC;
		    while(1)	
			{
			n = -1;
			found = 0;

			/** Is this a numerically-constrained spec? **/
			if (*format >= '0' && *format <= '9')
			    {
			    n = strtol(format, (char**)&endptr, 10); /* cast is needed because endptr is const char* */
			    format = endptr;
			    if (n < 0) n = 0;
			    }
			else if (*format == '*')
			    {
			    format++;
			    n = va_arg(ap, int);
			    if (n < 0) n = 0;
			    }

			/** Look for which spec this is **/
			for(i=startspec; i<=endspec; i++)
			    {
			    if ((n == -1 && 
				    qpf_spec_names[i][0] != 'n' &&
				    !strncmp(format, qpf_spec_names[i], qpf_spec_len[i])) || 
				(n >= 0 && 
				    qpf_spec_names[i][0] == 'n' && 
				    !strncmp(format, qpf_spec_names[i]+1, qpf_spec_len[i]-1)))
				{
				/** Found it. **/
				if (n == -1) format += qpf_spec_len[i];
				else format += (qpf_spec_len[i]-1);
				specchain_n[n_specs] = n;
				specchain[n_specs++] = i;
				found = 1;
				break;
				}
			    }
			startspec = QPF_SPEC_T_STARTFILT;
			endspec = QPF_SPEC_T_ENDFILT;

			/** Did we find it? **/
			if (__builtin_expect(!found, 0))
			    { rval = -EINVAL; goto error; }

			/** More? **/
			if (*format == '&')
			    {
			    if (__builtin_expect(n_specs == QPF_MAX_SPECS,0))
				{ rval = -ENOMEM; goto error; }
			    format++;
			    }
			else
			    {
			    break;
			    }
			}

		    /** Get source **/
		    switch(specchain[0])
			{
			case QPF_SPEC_T_INT:
			    intval = va_arg(ap, int);
			    cplen = qpf_internal_itoa(tmpbuf, sizeof(tmpbuf), intval);
			    strval = tmpbuf;
			    break;

			case QPF_SPEC_T_STR:
			    strval = va_arg(ap, const char*);
			    if (__builtin_expect(strval == NULL, 0))
				{ rval = -EINVAL; goto error; }
			    cplen = strlen(strval);
			    break;

			case QPF_SPEC_T_POS:
			    intval = va_arg(ap, int);
			    if (__builtin_expect(intval < 0, 0))
				{ rval = -EINVAL; goto error; }
			    cplen = qpf_internal_itoa(tmpbuf, sizeof(tmpbuf), intval);
			    strval = tmpbuf;
			    break;

			case QPF_SPEC_T_DBL:
			    dblval = va_arg(ap, double);
			    cplen = snprintf(tmpbuf, sizeof(tmpbuf), "%lf", dblval);
			    strval = tmpbuf;
			    break;

			case QPF_SPEC_T_NSTR:
			    strval = va_arg(ap, const char*);
			    if (__builtin_expect(strval == NULL, 0))
				{ rval = -EINVAL; goto error; }
			    cplen = specchain_n[0];
			    break;

			case QPF_SPEC_T_CHR:
			    chrval = va_arg(ap, int);
			    cplen = 1;
			    strval = &chrval;
			    break;

			default:
			    rval = -EINVAL;
			    goto error;
			}

		    /** Length problem? **/
		    if (__builtin_expect(cplen < 0, 0))
			{ rval = -EINVAL; goto error; }

		    /** Filters? **/
		    for (i=1;i<n_specs;i++)
			{
			switch(specchain[i])
			    {
			    case QPF_SPEC_T_NLEN:
				if (cplen > specchain_n[i]) cplen = specchain_n[i];
				break;

			    case QPF_SPEC_T_SYM:
				if (cxsecVerifySymbol_n(strval, cplen) < 0)
				    { rval = -EINVAL; goto error; }
				break;
			    
			    case QPF_SPEC_T_ESCQ:
				/** We need to special-case this both for performance
				 ** and for security so that \' doesn't get truncated
				 ** with just the \ and no ', for instance.
				 **/
				if (n_specs-i == 1 || (n_specs-i == 2 && specchain[i+1] == QPF_SPEC_T_NLEN))
				    {
				    if (n_specs-i == 2)
					maxdst = specchain_n[i+1];
				    else
					maxdst = INT_MAX;
				    oldcpoffset = cpoffset;
				    n = qpf_internal_Translate(strval, cplen, str, &cpoffset, size, maxdst, QPF.quote_matrix, nogrow?NULL:grow_fn, grow_arg);
				    if (n < 0) 
					{
					rval = n;
					goto error;
					}
				    if (n != cpoffset - oldcpoffset) nogrow = 1;
				    copied += n;
				    cplen = 0;
				    }
				else
				    {
				    rval = -ENOSYS;
				    goto error;
				    }
				break;

			    case QPF_SPEC_T_HTE:
				if (n_specs-i == 1 || (n_specs-i == 2 && specchain[i+1] == QPF_SPEC_T_NLEN))
				    {
				    if (n_specs-i == 2)
					maxdst = specchain_n[i+1];
				    else
					maxdst = INT_MAX;
				    oldcpoffset = cpoffset;
				    n = qpf_internal_Translate(strval, cplen, str, &cpoffset, size, maxdst, QPF.hte_matrix, nogrow?NULL:grow_fn, grow_arg);
				    if (n < 0) 
					{
					rval = n;
					goto error;
					}
				    if (n != cpoffset - oldcpoffset) nogrow = 1;
				    copied += n;
				    cplen = 0;
				    }
				else
				    {
				    rval = -ENOSYS;
				    }
				break;

			    default:
				/** Unimplemented filter **/
				rval = -ENOSYS;
				goto error;
			    }
			}

		    /** Copy it. **/
		    if (__builtin_expect(cplen != 0,1))
			{
			copied += cplen;
			if (__builtin_expect(nogrow, 0)) cplen = 0;
			if (__builtin_expect(cpoffset+cplen+1 > *size, 0) && (!grow_fn(str, size, grow_arg, cpoffset+cplen+1)))
			    {
			    cplen = (*size) - cpoffset - 1;
			    nogrow = 1;
			    }
			memcpy((*str)+cpoffset, strval, cplen);
			}

		    /** Update string counters **/
		    cpoffset += cplen;
		    }
		}
	    }
	    while (specptr);

	rval = copied;

    error:
	/** Null terminate.  Only case where this does not happen is
	 ** if size == 0 on the initial call.  Terminator is not counted
	 ** in the return value.
	 **/
	if ((*size) > cpoffset) (*str)[cpoffset] = '\0';

    return rval;
    }


/*** qpfRegisterExt() - registers an extension with the QPF module, allowing
 *** outside modules to provide extra specifiers for processing data or for
 *** data sources.  If is_source is nonzero, the extension is treated as a
 *** source specifier, otherwise it is a filtering specifier.
 ***/
void
qpfRegisterExt(char* ext_spec, int (*ext_fn)(), int is_source)
    {

	/** Add to list of extensions **/
	if (QPF.n_ext >= QPF_MAX_EXTS)
	    {
	    fprintf(stderr, "warning: qpfRegisterExt: QPF_MAX_EXTS exceeded\n");
	    return;
	    }
	QPF.ext_specs[QPF.n_ext] = nmSysStrdup(ext_spec);
	QPF.ext_fns[QPF.n_ext] = ext_fn;
	QPF.is_source[QPF.n_ext] = is_source?1:0;
	QPF.n_ext++;

    return;
    }


