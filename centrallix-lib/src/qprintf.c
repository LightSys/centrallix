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
#include <stdint.h>

#include "qprintf.h"
#include "mtask.h"
#include "newmalloc.h"
#include "cxsec.h"
#include "util.h"
#include "expect.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
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
/* See centrallix-sysdoc/QPrintf.md for more information. */
/************************************************************************/


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
#define QPF_SPEC_T_LL		(7)
#define QPF_SPEC_T_ENDSRC	(7)

/*** builtin filtering specifiers ***/
#define QPF_SPEC_T_STARTFILT	(8)
#define QPF_SPEC_T_QUOT		(8)
#define QPF_SPEC_T_DQUOT	(9)
#define QPF_SPEC_T_SYM		(10)
#define QPF_SPEC_T_JSSTR	(11)
#define QPF_SPEC_T_NLEN		(12)
#define QPF_SPEC_T_WS		(13)
#define QPF_SPEC_T_ESCWS	(14)
#define QPF_SPEC_T_ESCSP	(15)
#define QPF_SPEC_T_UNESC	(16)
#define QPF_SPEC_T_SSYB		(17)
#define QPF_SPEC_T_DSYB		(18)
#define QPF_SPEC_T_FILE		(19)
#define QPF_SPEC_T_PATH		(20)
#define QPF_SPEC_T_HEX		(21)
#define QPF_SPEC_T_DHEX		(22)
#define QPF_SPEC_T_B64		(23)
#define QPF_SPEC_T_DB64		(24)
#define QPF_SPEC_T_RF		(25)
#define QPF_SPEC_T_RR		(26)
#define	QPF_SPEC_T_HTENLBR	(27)
#define QPF_SPEC_T_DHTE		(28)
#define QPF_SPEC_T_URL		(29)
#define QPF_SPEC_T_DURL		(30)
#define QPF_SPEC_T_NLSET	(31)
#define QPF_SPEC_T_NRSET	(32)
#define QPF_SPEC_T_NZRSET	(33)
#define QPF_SPEC_T_SQLARG	(34)
#define QPF_SPEC_T_SQLSYM	(35)
#define QPF_SPEC_T_HTDATA	(36)
#define QPF_SPEC_T_HTE		(37)
#define QPF_SPEC_T_ESCQWS	(38)
#define QPF_SPEC_T_ESCQ		(39)
#define QPF_SPEC_T_CSSVAL	(40)
#define QPF_SPEC_T_CSSURL	(41)
#define QPF_SPEC_T_JSONSTR	(42)
#define QPF_SPEC_T_ENDFILT	(42)
#define QPF_SPEC_T_MAXSPEC	(42)

/** Names for specifiers as used in format string - must match the above. **/
const char*
qpf_spec_names[] = 
    {
    NULL,	/* 0 */
    "INT",	/* 1 */
    "STR",	/* 2 */
    "POS",	/* 3 */
    "DBL",	/* 4 */
    "nSTR",	/* 5 */
    "CHR",	/* 6 */
    "LL",	/* 7 */
    "QUOT",	/* 8 */
    "DQUOT",	/* 9 */
    "SYM",	/* 10 */
    "JSSTR",	/* 11 */
    "nLEN",	/* 12 */
    "WS",	/* 13 */
    "ESCWS",	/* 14 */
    "ESCSP",	/* 15 */
    "UNESC",	/* 16 */
    "SSYB",	/* 17 */
    "DSYB",	/* 18 */
    "FILE",	/* 19 */
    "PATH",	/* 20 */
    "HEX",	/* 21 */
    "DHEX",	/* 22 */
    "B64",	/* 23 */
    "DB64",	/* 24 */
    "RF",	/* 25 */
    "RR",	/* 26 */
    "HTENLBR",	/* 27 */
    "DHTE",	/* 28 */
    "URL",	/* 29 */
    "DURL",	/* 30 */
    "nLSET",	/* 31 */
    "nRSET",	/* 32 */
    "nZRSET",	/* 33 */
    "SQLARG",	/* 34 */
    "SQLSYM",	/* 35 */
    "HTDATA",	/* 36 */
    "HTE",	/* 37 */
    "ESCQWS",	/* 38 */
    "ESCQ",	/* 39 */
    "CSSVAL",	/* 40 */
    "CSSURL",	/* 41 */
    "JSONSTR",	/* 42 */
    NULL
    };

int qpf_spec_len[QPF_SPEC_T_MAXSPEC+1];

typedef struct
    {
    char*	Matrix[QPF_MATRIX_SIZE];
    unsigned char MatrixLen[QPF_MATRIX_SIZE];
    int		MaxExpand;
    }
    QPConvTable, *pQPConvTable;

typedef struct
    {
    int		n_ext;
    int		is_init;
    char*	ext_specs[QPF_MAX_EXTS];
    int		(*ext_fns[QPF_MAX_EXTS])();
    char	is_source[QPF_MAX_EXTS];
    
    /** Conversion matrices for various tasks. **/
    QPConvTable	quote_matrix;    /* Escapes quotes. */
    QPConvTable	quote_ws_matrix; /* Escapes quotes & whitespace. */
    QPConvTable	ws_matrix;       /* Escapes whitespace. */
    QPConvTable	hte_matrix;      /* Escapes characters for HTML. */
    QPConvTable	htenlbr_matrix;  /* Escapes characters and line breaks for HTML. */
    QPConvTable	cssval_matrix;   /* Escapes CSS values. */
    QPConvTable	cssurl_matrix;   /* Escapes CSS URLs. */
    QPConvTable	jsstr_matrix;    /* Escapes JavaScript strings. */
    QPConvTable	jsonstr_matrix;  /* Escapes JSON strings. */
    QPConvTable	hex_matrix;      /* Encodes strings into HEX. */
    QPConvTable	url_matrix;      /* Encodes URLs. */
    QPConvTable	dsyb_matrix;     /* Escapes double quotes sybase-style: " -> "" */
    QPConvTable	ssyb_matrix;     /* Escapes single quotes sybase-style: ' -> '' */
    }
    QPF_t;
    
static QPF_t QPF = { n_ext:0, is_init:0 };

/*** Searches for a substring within a buffer.
 *** Uses an optimized two-step approach: first locate the first character
 *** using `memchr()`, then verify the full match with `memcmp()`.
 ***
 *** @param haystack    The buffer to search.
 *** @param haystacklen The length of the haystack buffer (in bytes).
 *** @param needle      The substring for which to search.
 *** @param needlelen   The length of the needle substring (in bytes).
 *** @return The byte offset to the first instance of `needle` in `haystack`,
 ***         or -1 if not found (including when needlelen exceeds haystacklen).
 ***         Returns 0 if needlelen is 0 (empty needle matches at haystack).
 ***/
int
qpf_internal_FindStr(const char* haystack, size_t haystacklen, const char* needle, size_t needlelen)
    {
    int pos;
    char* ptr;
    
    /** Edge cases. **/
    if (UNLIKELY(needlelen > haystacklen)) return -1;
    if (UNLIKELY(needlelen == 0)) return 0;
    
    /** Search for the needle. **/
    pos = 0;
    while(pos <= haystacklen - needlelen)
	{
	ptr = memchr(haystack+pos, needle[0], haystacklen - pos);
	if (!ptr) return -1;
	if (memcmp(ptr, needle, needlelen) == 0)
	    return (ptr - haystack);
	pos = (ptr - haystack) + 1;
	}
    
    /** Not found. **/
    return -1;
    }

/*** Set up data in a conversion matrix (aka. table) data structure, called
 *** after the matrix values have been set.
 *** 
 *** @param table The conversion matrix table data structure to set up.
 *** @returns 0 if successful, or -1 if an error occurs.
 ***/
int
qpf_internal_SetupTable(pQPConvTable table)
    {
    int i;
    int mx = 1;
    int n;

	for(i=0;i<QPF_MATRIX_SIZE;i++)
	    {
	    if (table->Matrix[i])
		{
		n = strlen(table->Matrix[i]);
		if (n > mx) mx = n;
		table->MatrixLen[i] = n;
		}
	    }
	table->MaxExpand = mx;

    return 0;
    }


/*** Initialize internal data structures for the QPF module.
 *** 
 *** @returns 0 if successful, or -1 if an error occurs.
 ***/
int
qpfInitialize(void)
    {
    int i;
    char buf[4];
    char hex[] = "0123456789abcdef";

	/** Initialize matrix that: Escapes quotes. **/
	memset(&QPF.quote_matrix, 0, sizeof(QPF.quote_matrix));
	QPF.quote_matrix.Matrix['\''] = "\\'";
	QPF.quote_matrix.Matrix['"'] = "\\\"";
	QPF.quote_matrix.Matrix['\\'] = "\\\\";
	qpf_internal_SetupTable(&QPF.quote_matrix);

	/** Initialize matrix that: Escapes quotes & whitespace. **/
	memset(&QPF.quote_ws_matrix, 0, sizeof(QPF.quote_ws_matrix));
	QPF.quote_ws_matrix.Matrix['\''] = "\\'";
	QPF.quote_ws_matrix.Matrix['"'] = "\\\"";
	QPF.quote_ws_matrix.Matrix['\\'] = "\\\\";
	QPF.quote_ws_matrix.Matrix['\n'] = "\\n";
	QPF.quote_ws_matrix.Matrix['\t'] = "\\t";
	QPF.quote_ws_matrix.Matrix['\r'] = "\\r";
	qpf_internal_SetupTable(&QPF.quote_ws_matrix);

	/** Initialize matrix that: Escapes whitespace. **/
	memset(&QPF.ws_matrix, 0, sizeof(QPF.ws_matrix));
	QPF.ws_matrix.Matrix['\n'] = "\\n";
	QPF.ws_matrix.Matrix['\t'] = "\\t";
	QPF.ws_matrix.Matrix['\r'] = "\\r";
	qpf_internal_SetupTable(&QPF.ws_matrix);

	/** Initialize matrix that: Escapes characters for HTML. **/
	memset(&QPF.hte_matrix, 0, sizeof(QPF.hte_matrix));
	QPF.hte_matrix.Matrix['<'] = "&lt;";
	QPF.hte_matrix.Matrix['>'] = "&gt;";
	QPF.hte_matrix.Matrix['&'] = "&amp;";
	QPF.hte_matrix.Matrix['"'] = "&quot;";
	QPF.hte_matrix.Matrix['\''] = "&#39;";
	QPF.hte_matrix.Matrix[';'] = "&#59;";
	QPF.hte_matrix.Matrix['}'] = "&#125;";
	QPF.hte_matrix.Matrix['\0'] = "&#0;";
	qpf_internal_SetupTable(&QPF.hte_matrix);

	/** Initialize matrix that: Escapes characters and line breaks for HTML. **/
	memset(&QPF.htenlbr_matrix, 0, sizeof(QPF.htenlbr_matrix));
	QPF.htenlbr_matrix.Matrix['<'] = "&lt;";
	QPF.htenlbr_matrix.Matrix['>'] = "&gt;";
	QPF.htenlbr_matrix.Matrix['&'] = "&amp;";
	QPF.htenlbr_matrix.Matrix['"'] = "&quot;";
	QPF.htenlbr_matrix.Matrix['\''] = "&#39;";
	QPF.htenlbr_matrix.Matrix[';'] = "&#59;";
	QPF.htenlbr_matrix.Matrix['}'] = "&#125;";
	QPF.htenlbr_matrix.Matrix['\0'] = "&#0;";
	QPF.htenlbr_matrix.Matrix['\n'] = "<br>";
	qpf_internal_SetupTable(&QPF.htenlbr_matrix);

	/** Initialize matrix that: Escapes CSS values. **/
	memset(&QPF.cssval_matrix, 0, sizeof(QPF.cssval_matrix));
	QPF.cssval_matrix.Matrix[';'] = "\\;";
	QPF.cssval_matrix.Matrix['}'] = "\\}";
	QPF.cssval_matrix.Matrix['{'] = "\\{";
	QPF.cssval_matrix.Matrix['<'] = "\\<";
	QPF.cssval_matrix.Matrix['>'] = "\\>";
	QPF.cssval_matrix.Matrix['/'] = "\\/";
	QPF.cssval_matrix.Matrix['\\'] = "\\\\";
	QPF.cssval_matrix.Matrix['"'] = "\\\"";
	QPF.cssval_matrix.Matrix['\''] = "\\'";
	qpf_internal_SetupTable(&QPF.cssval_matrix);

	/** Initialize matrix that: Escapes CSS URLs. **/
	memset(&QPF.cssurl_matrix, 0, sizeof(QPF.cssurl_matrix));
	QPF.cssurl_matrix.Matrix[';'] = "\\;";
	QPF.cssurl_matrix.Matrix['}'] = "\\}";
	QPF.cssurl_matrix.Matrix['{'] = "\\{";
	QPF.cssurl_matrix.Matrix['<'] = "\\<";
	QPF.cssurl_matrix.Matrix['>'] = "\\>";
	QPF.cssurl_matrix.Matrix['/'] = "\\/";
	QPF.cssurl_matrix.Matrix['\\'] = "\\\\";
	QPF.cssurl_matrix.Matrix['"'] = "\\\"";
	QPF.cssurl_matrix.Matrix['\''] = "\\'";
	QPF.cssurl_matrix.Matrix['('] = "\\(";
	QPF.cssurl_matrix.Matrix[')'] = "\\)";
	QPF.cssurl_matrix.Matrix[','] = "\\,";
	QPF.cssurl_matrix.Matrix[' '] = "\\ ";
	QPF.cssurl_matrix.Matrix['\t'] = "\\\t";
	QPF.cssurl_matrix.Matrix['\n'] = "\\\n";
	QPF.cssurl_matrix.Matrix['\r'] = "\\\r";
	qpf_internal_SetupTable(&QPF.cssurl_matrix);

	/** Initialize matrix that: Escapes JavaScript strings. **/
	memset(&QPF.jsstr_matrix, 0, sizeof(QPF.jsstr_matrix));
	QPF.jsstr_matrix.Matrix['\''] = "\\'";
	QPF.jsstr_matrix.Matrix['"'] = "\\\"";
	QPF.jsstr_matrix.Matrix['\\'] = "\\\\";
	QPF.jsstr_matrix.Matrix['/'] = "\\/";
	QPF.jsstr_matrix.Matrix['\n'] = "\\n";
	QPF.jsstr_matrix.Matrix['\t'] = "\\t";
	QPF.jsstr_matrix.Matrix['\r'] = "\\r";
	QPF.jsstr_matrix.Matrix['\b'] = "\\b";
	QPF.jsstr_matrix.Matrix['\f'] = "\\f";
	for(i=0;i<=31;i++)
	    {
	    if (!QPF.jsstr_matrix.Matrix[i])
		{
		QPF.jsstr_matrix.Matrix[i] = nmSysMalloc(7);
		snprintf(QPF.jsstr_matrix.Matrix[i], 7, "\\u%4.4X", i);
		}
	    }
	qpf_internal_SetupTable(&QPF.jsstr_matrix);

	/** Initialize matrix that: Escapes JSON strings. **/
	memset(&QPF.jsonstr_matrix, 0, sizeof(QPF.jsonstr_matrix));
	QPF.jsonstr_matrix.Matrix['"'] = "\\\"";
	QPF.jsonstr_matrix.Matrix['\\'] = "\\\\";
	QPF.jsonstr_matrix.Matrix['\n'] = "\\n";
	QPF.jsonstr_matrix.Matrix['\t'] = "\\t";
	QPF.jsonstr_matrix.Matrix['\r'] = "\\r";
	QPF.jsonstr_matrix.Matrix['\b'] = "\\b";
	QPF.jsonstr_matrix.Matrix['\f'] = "\\f";
	for(i=0;i<=31;i++)
	    {
	    if (!QPF.jsonstr_matrix.Matrix[i])
		{
		QPF.jsonstr_matrix.Matrix[i] = nmSysMalloc(7);
		snprintf(QPF.jsonstr_matrix.Matrix[i], 7, "\\u%4.4X", i);
		}
	    }
	qpf_internal_SetupTable(&QPF.jsonstr_matrix);

	/** Initialize matrix that: Encodes strings into HEX. **/
	for(i=0;i<QPF_MATRIX_SIZE;i++)
	    {
	    buf[0] = hex[(i>>4)&0x0F];
	    buf[1] = hex[i&0x0F];
	    buf[2] = '\0';
	    QPF.hex_matrix.Matrix[i] = nmSysStrdup(buf);
	    }
	qpf_internal_SetupTable(&QPF.hex_matrix);

	/** Initialize matrix that: Encodes URLs (everything except 0-9, A-Z, and a-z). **/
	memset(&QPF.url_matrix, 0, sizeof(QPF.url_matrix));
	for(i=0;i<48;i++) /* escape until 0-9 */
	    {
	    buf[0] = '%';
	    buf[1] = hex[(i>>4)&0x0F];
	    buf[2] = hex[i&0x0F];
	    buf[3] = '\0';
	    QPF.url_matrix.Matrix[i] = nmSysStrdup(buf);
	    }
	for(i=58;i<65;i++) /* skip 0-9 and continue until A-Z */
	    {
	    buf[0] = '%';
	    buf[1] = hex[(i>>4)&0x0F];
	    buf[2] = hex[i&0x0F];
	    buf[3] = '\0';
	    QPF.url_matrix.Matrix[i] = nmSysStrdup(buf);
	    }
	for(i=91;i<97;i++) /* skip A-Z and continue until a-z */
	    {
	    buf[0] = '%';
	    buf[1] = hex[(i>>4)&0x0F];
	    buf[2] = hex[i&0x0F];
	    buf[3] = '\0';
	    QPF.url_matrix.Matrix[i] = nmSysStrdup(buf);
	    }
	for(i=123;i<QPF_MATRIX_SIZE;i++) /* encode everything higher */
	    {
	    buf[0] = '%';
	    buf[1] = hex[(i>>4)&0x0F];
	    buf[2] = hex[i&0x0F];
	    buf[3] = '\0';
	    QPF.url_matrix.Matrix[i] = nmSysStrdup(buf);
	    }
	qpf_internal_SetupTable(&QPF.url_matrix);

	/** Initialize matrix that: Escapes double quotes sybase-style: " -> "". **/
	memset(&QPF.dsyb_matrix, 0, sizeof(QPF.dsyb_matrix));
	QPF.dsyb_matrix.Matrix['"'] = "\"\"";
	qpf_internal_SetupTable(&QPF.dsyb_matrix);

	/** Initialize matrix that: Escapes single quotes sybase-style: ' -> ''. **/
	memset(&QPF.ssyb_matrix, 0, sizeof(QPF.ssyb_matrix));
	QPF.ssyb_matrix.Matrix['\''] = "''";
	qpf_internal_SetupTable(&QPF.ssyb_matrix);

	/** Initialize the qpf_spec_len array. **/
	for(i=0;i<=QPF_SPEC_T_MAXSPEC;i++)
	    {
	    if (qpf_spec_names[i]) 
		qpf_spec_len[i] = strlen(qpf_spec_names[i]);
	    }

	/** Done. **/
	QPF.is_init = 1;

    return 0;
    }


/*** Open a new qprintf session.
 *** The session is used for storing cumulative error information which makes
 *** error handling much cleaner for callers that use this feature.
 *** 
 *** @returns A new, initialized pQPSession object, or NULL if an error occurs.
 ***/
pQPSession
qpfOpenSession(void)
    {
    pQPSession s = NULL;

	if (UNLIKELY(!QPF.is_init)) 
	    {
	    if (UNLIKELY(qpfInitialize() < 0)) return NULL;
	    }

	s = (pQPSession)nmMalloc(sizeof(QPSession));
	if (UNLIKELY(!s)) return NULL;
	s->Errors = 0;

    return s;
    }


/*** Queries the current error mask, indicating errors that have occurred since
 *** when the session was initialized or the last time that `qpfClearErrors()`
 *** was called.
 *** 
 *** @param s The session to be queried.
 *** @returns The current error mask (does not fail).
 ***/
unsigned int
qpfErrors(pQPSession s)
    {
    return s->Errors;
    }


/*** Reset the errors mask, clearing all errors that have occurred.
 *** 
 *** @param s The session holding the error mask to be cleared.
 *** @returns 0 if successful, or -1 if an error occurs.
 ***/
int
qpfClearErrors(pQPSession s)
    {
    s->Errors = 0;
    return 0;
    }


/*** Closes a qprintf session and deallocates associated resources.
 *** 
 *** @param s The session to close.
 *** @returns 0 if successful, or -1 if an error occurs.
 ***/
int
qpfCloseSession(pQPSession s)
    {
	
	nmFree(s, sizeof(QPSession));

    return 0;
    }


/*** Convert an integer into a string representation.  Seems to perform better
 *** than `snprintf("%d")`, even without optimization enabled.
 *** 
 *** @param dst The destination string buffer.
 *** @param dstlen The allocated length of the string buffer, used to avoid
 *** 	buffer overflows.
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


/*** Convert string to its base 64 representation.
 *** This function is the reverse of `qpf_internal_base64decode()`.
 *** 
 *** @param s The qprintf session in use.
 *** @param src The source string to convert.
 *** @param src_size The length of `src` in bytes.
 *** @param dst A pointer to a location where the resulting string pointer
 *** 	should be saved.
 *** @param dst_size The size of the currently allocated string at `dst`.
 *** @param dst_offset The number of bytes to skip at the start of `dst`
 *** 	before writing the result.
 *** @param grow_fn An optional grow function, used to grow the dst string if
 *** 	more space is needed.
 *** @param grow_arg The argument to be passed to the grow function.
 *** @returns The number of bytes written.
 ***/
int
qpf_internal_base64encode(pQPSession s, const char* src, size_t src_size, char** dst, size_t* dst_size, size_t* dst_offset, qpf_grow_fn_t grow_fn, void* grow_arg)
    {
    static char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const unsigned char* srcptr = (const unsigned char*)src;
    const unsigned char* origsrc = (const unsigned char*)src;
    char* dstptr;
    int req_size = ((src_size+2) / 3) * 4 + *dst_offset;

	/** Grow dstbuf if necessary and possible, otherwise return error **/
	if (req_size > *dst_size)
	    {
	    if(grow_fn == NULL || !grow_fn(dst, dst_size, 0, grow_arg, req_size))
		{
		QPERR(QPF_ERR_T_MEMORY);
		return -1;
		}
	    }

	dstptr = *dst + *dst_offset;
	
	/** Step through src 3 bytes at a time, generating 4 dst bytes for each 3 src **/
	while(srcptr < origsrc + src_size)
	    {
	    /** First 6 bits of source[0] --> first byte dst. **/
	    dstptr[0] = b64[srcptr[0]>>2];

	    /** Second dst byte from last 2 bits of src[0] and first 4 of src[1] **/
	    if (srcptr+1 < origsrc + src_size)
		dstptr[1] = b64[((srcptr[0]&0x03)<<4) | (srcptr[1]>>4)];
	    else
		{
		dstptr[1] = b64[(srcptr[0]&0x03)<<4];
		dstptr[2] = '=';
		dstptr[3] = '=';
		dstptr += 4;
		break;
		}

	    /** Third dst byte from second 4 bits of src[1] and first 2 of src[2] **/
	    if (srcptr+2 < origsrc + src_size)
		dstptr[2] = b64[((srcptr[1]&0x0F)<<2) | (srcptr[2]>>6)];
	    else
		{
		dstptr[2] = b64[(srcptr[1]&0x0F)<<2];
		dstptr[3] = '=';
		dstptr += 4;
		break;
		}

	    /** Last dst byte from last 6 bits of src[2] **/
	    dstptr[3] = b64[(srcptr[2]&0x3F)];

	    /** Increment pointers **/
	    dstptr += 4;
	    srcptr += 3;
	    }

	*dst_offset = *dst_offset + (dstptr - *dst);

    return dstptr - *dst;
    }


/*** Convert the base 64 representation of a string back to the original
 *** string.
 *** This function is the reverse of `qpf_internal_base64encode()`.
 *** 
 *** @param s The qprintf session in use.
 *** @param src The source string representation to convert.
 *** @param src_size The length of `src` in bytes.
 *** @param dst A pointer to a location where the resulting string pointer
 *** 	should be saved.
 *** @param dst_size The size of the currently allocated string at `dst`.
 *** @param dst_offset The number of bytes to skip at the start of `dst`
 *** 	before writing the result.
 *** @param grow_fn An optional grow function, used to grow the dst string if
 *** 	more space is needed.
 *** @param grow_arg The argument to be passed to the grow function.
 *** @returns The number of bytes written.
 ***/
static inline int
qpf_internal_base64decode(pQPSession s, const char* src, size_t src_size, char** dst, size_t* dst_size, size_t* dst_offset, qpf_grow_fn_t grow_fn, void* grow_arg)
    {
    char b64[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char* ptr;
    char* cursor;
    int ix;
    int req_size = (.75 * src_size) + *dst_offset + 1; /** fmul could truncate when cast to int hence +1 **/

	/** Verify source data is correct length for base 64 **/
	if (UNLIKELY(src_size % 4 != 0))
	    {
	    QPERR(QPF_ERR_T_BADCHAR);
	    return -1;
	    }

	/** Grow dstbuf if necessary and possible, otherwise return error **/
	if (req_size > *dst_size)
	    {
	    if(grow_fn == NULL || !grow_fn(dst, dst_size, 0, grow_arg, req_size))
		{
		QPERR(QPF_ERR_T_MEMORY);
		return -1;
		}
	    }

	cursor = *dst + *dst_offset;
	
	/** Step through src 4 bytes at a time. **/
	while(*src)
	    {
	    /** First 6 bits. **/
	    ptr = strchr(b64,src[0]);
	    if (UNLIKELY(!ptr || !*ptr))
	        {
		QPERR(QPF_ERR_T_BADCHAR);
		return -1;
		}
	    ix = ptr-b64;
	    cursor[0] = ix<<2;

	    /** Second six bits are split between cursor[0] and cursor[1] **/
	    ptr = strchr(b64,src[1]);
	    if (UNLIKELY(!ptr || !*ptr))
	        {
		QPERR(QPF_ERR_T_BADCHAR);
		return -1;
		}
	    ix = ptr-b64;
	    cursor[0] |= ix>>4;
	    cursor[1] = (ix<<4)&0xF0;

	    /** Third six bits are nonmandatory and split between cursor[1] and [2] **/
	    if (src[2] == '=' && src[3] == '=')
	        {
		cursor += 1;
		break;
		}
	    ptr = strchr(b64,src[2]);
	    if (UNLIKELY(!ptr || !*ptr))
	        {
		QPERR(QPF_ERR_T_BADCHAR);
		return -1;
		}
	    ix = ptr-b64;
	    cursor[1] |= ix>>2;
	    cursor[2] = (ix<<6)&0xC0;

	    /** Fourth six bits are nonmandatory and a part of cursor[2]. **/
	    if (src[3] == '=')
	        {
		cursor += 2;
		break;
		}
	    ptr = strchr(b64,src[3]);
	    if (UNLIKELY(!ptr || !*ptr))
	        {
		QPERR(QPF_ERR_T_BADCHAR);
		return -1;
		}
	    ix = ptr-b64;
	    cursor[2] |= ix;
	    src += 4;
	    cursor += 3;
	    }

	*dst_offset = *dst_offset + cursor - *dst;

    return cursor - *dst;
    }


/*** Convert the base 16 (hex) representation of a string back to the
 *** original string.
 *** 
 *** @param s The qprintf session in use.
 *** @param src The source string representation to convert.
 *** @param src_size The length of `src` in bytes.
 *** @param dst A pointer to a location where the resulting string pointer
 *** 	should be saved.
 *** @param dst_size The size of the currently allocated string at `dst`.
 *** @param dst_offset The number of bytes to skip at the start of `dst`
 *** 	before writing the result.
 *** @param grow_fn An optional grow function, used to grow the dst string if
 *** 	more space is needed.
 *** @param grow_arg The argument to be passed to the grow function.
 *** @returns The number of bytes written.
 ***/
static inline int
qpf_internal_hexdecode(pQPSession s, const char* src, size_t src_size, char** dst, size_t* dst_size, size_t* dst_offset, qpf_grow_fn_t grow_fn, void* grow_arg)
    {
    char hex[23] = "0123456789abcdefABCDEF";
    int conv[22] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 10, 11, 12, 13, 14, 15 };
    char* ptr;
    char* cursor;
    int ix;
    int req_size;
    char* orig_src = src;

	/** Required size **/
	if (UNLIKELY(src_size%2 == 1))
	    {
	    QPERR(QPF_ERR_T_BADLENGTH);
	    return -1;
	    }
	req_size = src_size/2;

	/** Grow dstbuf if necessary and possible, otherwise return error **/
	if (req_size > *dst_size)
	    {
	    if(grow_fn == NULL || !grow_fn(dst, dst_size, 0, grow_arg, req_size))
		{
		QPERR(QPF_ERR_T_MEMORY);
		return -1;
		}
	    }

	cursor = *dst + *dst_offset;
	
	/** Step through src 2 bytes at a time. **/
	while(*src && (src - orig_src) < src_size)
	    {
	    /** First 4 bits. **/
	    ptr = strchr(hex, src[0]);
	    if (UNLIKELY(!ptr))
	        {
		QPERR(QPF_ERR_T_BADCHAR);
		return -1;
		}
	    ix = conv[ptr-hex];
	    cursor[0] = ix<<4;

	    /** Second four bits  **/
	    ptr = strchr(hex, src[1]);
	    if (UNLIKELY(!ptr))
	        {
		QPERR(QPF_ERR_T_BADCHAR);
		return -1;
		}
	    ix = conv[ptr-hex];
	    cursor[0] |= ix;

	    src += 2;
	    cursor += 1;
	    }

	*dst_offset = *dst_offset + cursor - *dst;

    return cursor - *dst;
    }


/*** Returns the amount of additional size that will fit, but does not
 *** reallocate the string to grow it to a larger size.
 ***/
int
qpfPrintf_grow(char** str, size_t* size, size_t offs, void* arg, size_t req_size)
    {
    return (*size) >= req_size;
    }


/*** qpfPrintf() - do the quoting printf operation, given a standard vararg
 *** function call.
 ***/
int 
qpfPrintf(pQPSession s, char* str, size_t size, const char* format, ...)
    {
    va_list va;
    int rval;

	/** Grab the va ptr and call the _va version **/
	va_start(va, format);
	rval = qpfPrintf_va(s, str, size, format, va);
	va_end(va);

    return rval;
    }


/*** qpfPrintf_va() - same as qpfPrintf(), but takes a va_list instead of
 *** a list of arguments.
 ***/
int 
qpfPrintf_va(pQPSession s, char* str, size_t size, const char* format, va_list ap)
    {
    return qpfPrintf_va_internal(s, &str, &size, qpfPrintf_grow, NULL, format, ap);
    }


/*** qpf_internal_Translate() - do a translation copy from one buffer to
 *** another, respecting a soft and hard limit, and using a given char
 *** translation table.  Returns the number of chars that would have been
 *** placed in dstbuf had there been enough room (or, if there was enough
 *** room, the actual # chars placed in dstbuf); NOTE - does NOT return
 *** the number of chars pulled from the srcbuf!!!
 ***
 *** min_room applies to 'dstsize' only -- we must leave at least that much
 *** room in dstbuf once we are done.  Often this is "1", to leave room for
 *** a null terminator, or "2" to leave room for a closing quote mark and a
 *** null terminator, for instance.
 ***/
static inline int
qpf_internal_Translate(pQPSession s, const char* srcbuf, size_t srcsize, char** dstbuf, size_t* dstoffs, size_t* dstsize, size_t limit, pQPConvTable table, qpf_grow_fn_t grow_fn, void* grow_arg, size_t min_room)
    {
    int rval = 0;
    unsigned int tlen;
    int i;
    char* trans;
    int nogrow = (grow_fn == NULL);

	if (UNLIKELY(srcsize >= SIZE_MAX/2/table->MaxExpand))
	    return -1;

	if (LIKELY(srcsize))
	    {
	    rval += srcsize;
	    if ((srcsize*table->MaxExpand) <= limit && (srcsize*table->MaxExpand + min_room) <= (*dstsize - *dstoffs))
		{
		/** Easy route - definitely enough space! **/
		for(i=0;i<srcsize;i++)
		    {
		    if (UNLIKELY(((trans = table->Matrix[(unsigned char)(srcbuf[i])]) != NULL)))
			{
			tlen = table->MatrixLen[(unsigned char)(srcbuf[i])];
			while(*trans) (*dstbuf)[(*dstoffs)++] = *(trans++);
			rval += (tlen-1);
			}
		    else
			{
			(*dstbuf)[(*dstoffs)++] = srcbuf[i];
			}
		    }
		}
	    else
		{
		/** Hard route - may or may not be enough space! **/
		for(i=0;i<srcsize;i++)
		    {
		    if (UNLIKELY(((trans = table->Matrix[(unsigned char)(srcbuf[i])]) != NULL)))
			{
			tlen = table->MatrixLen[(unsigned char)(srcbuf[i])];
			if (LIKELY(limit >= tlen))
			    {
			    rval += (tlen-1);
			    if (LIKELY(!nogrow) && (LIKELY((*dstoffs)+tlen+min_room <= (*dstsize)) || 
				  (grow_fn(dstbuf, dstsize, *dstoffs, grow_arg, (*dstoffs)+tlen+min_room))))
				{
				while(*trans) (*dstbuf)[(*dstoffs)++] = *(trans++);
				limit -= tlen;
				}
			    else
				{
				QPERR(QPF_ERR_T_BUFOVERFLOW);
				nogrow = 1;
				}
			    }
			else
			    {
			    QPERR(QPF_ERR_T_INSOVERFLOW);
			    rval--;
			    }
			}
		    else
			{
			if (LIKELY(limit > 0))
			    {
			    if (LIKELY(!nogrow) && (LIKELY((*dstoffs)+1+min_room <= (*dstsize)) || 
				  (grow_fn(dstbuf, dstsize, *dstoffs, grow_arg, (*dstoffs)+1+min_room))))
				{
				(*dstbuf)[(*dstoffs)++] = srcbuf[i];
				limit--;
				}
			    else
				{
				QPERR(QPF_ERR_T_BUFOVERFLOW);
				nogrow = 1;
				}
			    }
			else
			    {
			    QPERR(QPF_ERR_T_INSOVERFLOW);
			    rval--;
			    }
			}
		    }
		}
	    }

    return rval;
    }


/*** Parse the provided format to apply all of the qprintf() format specifier
 *** rules while printing the indicated characters.
 *** 
 *** Warning:  The 'str' parameter may change during the execution of this
 *** function if grow_fn reallocates it.  Do not store pointers to 'str'!
 *** Instead, use offsets.
 *** 
 *** NULL, &(s->Tmpbuf), &(s->TmpbufSize), htr_internal_GrowFn, (void*)s, fmt, va
 *** @param s Optional session struct.
 *** @param str Pointer to a string buffer where data will be written.
 *** @param size Pointer to the current size of the string buffer.
 *** @param grow_fn A function to grow the string buffer.
 *** @param format The format of data which should be written.
 *** @param ap The arguments list to fulfill the provided format.
 ***/
int
qpfPrintf_va_internal(pQPSession s, char** str, size_t* size, qpf_grow_fn_t grow_fn, void* grow_arg, const char* format, va_list ap)
    {
    const char* specptr;
    const char* endptr;
    size_t copied = 0;
    int rval;
    size_t cplen;
    ptrdiff_t ptrdiff_cplen;
    char specchain[QPF_MAX_SPECS];
    int specchain_n[QPF_MAX_SPECS];
    int n_specs;
    int i;
    int n;
    int found;
    int intval;
    long long llval;
    const char* strval;
    double dblval;
    char tmpbuf[64];
    char chrval;
    size_t cpoffset = 0;
    size_t oldcpoffset;
    int nogrow = 0;
    int startspec;
    int endspec;
    size_t maxdst;
    int ignore = 0;
    pQPConvTable table;
    QPSession null_session;
    size_t min_room;
    char quote;

	if (UNLIKELY(!QPF.is_init)) 
	    {
	    if (qpfInitialize() < 0) return -ENOMEM;
	    }

	if (UNLIKELY(!s))
	    {
	    null_session.Errors = 0;
	    s=&null_session;
	    }

	/** this all falls apart if there isn't at least room for the
	 ** null terminator!
	 **/
	if (UNLIKELY((!*str || *size < 1) && !grow_fn(str, size, cpoffset, grow_arg, 1))) 
	    { rval = -EINVAL; QPERR(QPF_ERR_T_BUFOVERFLOW); goto error; }

	/** search for %this-and-that (specifiers), copy everything else **/
	do  {
	    /** Find the end of the non-specifier string segment **/
	    specptr = strchr(format, '%');
	    endptr = specptr?specptr:(format+strlen(format));

	    /** Copy the plain section of string **/
	    if (!ignore)
		{
		ptrdiff_cplen = (endptr - format);
		if (UNLIKELY(ptrdiff_cplen < 0 || ptrdiff_cplen >= SIZE_MAX/4))
		    {
		    QPERR(QPF_ERR_T_BUFOVERFLOW);
		    rval = -EINVAL;
		    goto error;
		    }
		cplen = ptrdiff_cplen;
		if (UNLIKELY(nogrow)) cplen = 0;
		if (UNLIKELY(cpoffset+cplen+1 > SIZE_MAX/2))
		    {
		    QPERR(QPF_ERR_T_BUFOVERFLOW);
		    rval = -EINVAL;
		    goto error;
		    }
		if (UNLIKELY(cpoffset+cplen+1 > *size) && (nogrow || !grow_fn(str, size, cpoffset, grow_arg, cpoffset+cplen+1)))
		    {
		    QPERR(QPF_ERR_T_BUFOVERFLOW);
		    cplen = (*size)-cpoffset-1;
		    nogrow = 1;
		    }
		if (cplen) memcpy((*str) + cpoffset, format, cplen);
		cpoffset += cplen;
		copied += ptrdiff_cplen;
		}
	    format = endptr;

	    /** Handle specifiers **/
	    if (format[0] == '%')
		{
		format++;

		/** Simple specifiers **/
		if (UNLIKELY(format[0] == '%'))
		    {
		    if (ignore)
			{
			format++;
			continue;
			}
		    
		    if (LIKELY(!nogrow) && (LIKELY(cpoffset+2 <= *size) || (grow_fn(str, size, cpoffset, grow_arg, cpoffset+2))))
			(*str)[cpoffset++] = '%';
		    else
			{
			QPERR(QPF_ERR_T_BUFOVERFLOW);
			nogrow = 1;
			}
		    copied++;
		    format++;
		    }
		else if (UNLIKELY(format[0] == '&'))
		    {
		    if (ignore)
			{
			format++;
			continue;
			}
		    
		    if (LIKELY(!nogrow) && (LIKELY(cpoffset+2 <= *size) || (grow_fn(str, size, cpoffset, grow_arg, cpoffset+2))))
			(*str)[cpoffset++] = '&';
		    else
			{
			QPERR(QPF_ERR_T_BUFOVERFLOW);
			nogrow = 1;
			}
		    copied++;
		    format++;
		    }
		else if (UNLIKELY(format[0] == ']'))
		    {
		    format++;
		    ignore = 0;
		    }
		else if (UNLIKELY(format[0] == '['))
		    {
		    format++;
		    intval = va_arg(ap, int);
		    if (!intval)
			{
			/*while(*format && format[0] != '%' && format[1] != ']') 
			    format++;
			if (*format)
			    format += 2;*/
			ignore = 1;
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
			    n = strtoi(format, (char**)&endptr, 10); /* cast is needed because endptr is const char* */
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
			if (UNLIKELY(!found))
			    { 
			    if (n_specs == 0)
				{
				/** need at least one spec **/
				rval = -EINVAL;
				QPERR(QPF_ERR_T_BADFORMAT);
				goto error;
				}
			    /** invalid spec, ignore and print **/
			    format--;
			    break;
			    }

			/** More? **/
			if (*format == '&')
			    {
			    if (UNLIKELY(n_specs == QPF_MAX_SPECS))
				{ rval = -ENOMEM; QPERR(QPF_ERR_T_RESOURCE); goto error; }
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
			    if (UNLIKELY(strval == NULL && !ignore))
				{ rval = -EINVAL; QPERR(QPF_ERR_T_NULL); goto error; }
			    cplen = strval?strlen(strval):0;
			    break;

			case QPF_SPEC_T_POS:
			    intval = va_arg(ap, int);
			    if (UNLIKELY(intval < 0 && !ignore))
				{ rval = -EINVAL; QPERR(QPF_ERR_T_NOTPOSITIVE); goto error; }
			    cplen = qpf_internal_itoa(tmpbuf, sizeof(tmpbuf), intval);
			    strval = tmpbuf;
			    break;

            case QPF_SPEC_T_LL:
                llval = va_arg(ap, long long);
                cplen = snprintf(tmpbuf, sizeof(tmpbuf), "%lld", llval);
                strval = tmpbuf;
                break;
                
			case QPF_SPEC_T_DBL:
			    dblval = va_arg(ap, double);
			    cplen = snprintf(tmpbuf, sizeof(tmpbuf), "%lf", dblval);
			    strval = tmpbuf;
			    break;

			case QPF_SPEC_T_NSTR:
			    strval = va_arg(ap, const char*);
			    if (UNLIKELY(strval == NULL && !ignore))
				{ rval = -EINVAL; QPERR(QPF_ERR_T_NULL); goto error; }
			    cplen = specchain_n[0];
			    break;

			case QPF_SPEC_T_CHR:
			    chrval = va_arg(ap, int);
			    cplen = 1;
			    strval = &chrval;
			    break;

			default:
			    rval = -EINVAL;
			    QPERR(QPF_ERR_T_BADFORMAT);
			    goto error;
			}

		    if (!ignore)
			{
			/** Length problem? **/
			if (UNLIKELY(cplen < 0))
			    { rval = -EINVAL; QPERR(QPF_ERR_T_BADLENGTH); goto error; }

			/** Filters? **/
			for (i=1;i<n_specs;i++)
			    {
			    switch(specchain[i])
				{
				case QPF_SPEC_T_NLEN:
				    if (UNLIKELY(cplen > specchain_n[i]))
					{
					QPERR(QPF_ERR_T_INSOVERFLOW);
					cplen = specchain_n[i];
					}
				    break;

				case QPF_SPEC_T_SYM:
				    if (n_specs-i == 2 && specchain[i+1] == QPF_SPEC_T_NLEN && cplen > specchain_n[i+1])
					cplen = specchain_n[i+1];
				    if (UNLIKELY(cxsecVerifySymbol_n(strval, cplen) < 0))
					{ rval = -EINVAL; QPERR(QPF_ERR_T_BADSYMBOL); goto error; }
				    break;

				case QPF_SPEC_T_FILE:
				    if (n_specs-i == 2 && specchain[i+1] == QPF_SPEC_T_NLEN && cplen > specchain_n[i+1])
					cplen = specchain_n[i+1];
				    if (UNLIKELY((cplen == 1 && strval[0] == '.') || 
					    (cplen == 2 && strval[0] == '.' && strval[1] == '.') ||
					    memchr(strval, '/', cplen) || 
					    memchr(strval, '\0', cplen) ||
					    cplen == 0))
					{ rval = -EINVAL; QPERR(QPF_ERR_T_BADFILE); goto error; }
				    break;

				case QPF_SPEC_T_PATH:
				    if (n_specs-i == 2 && specchain[i+1] == QPF_SPEC_T_NLEN && cplen > specchain_n[i+1])
					cplen = specchain_n[i+1];
				    if (UNLIKELY((cplen == 2 && strval[0] == '.' && strval[1] == '.') ||
					    (cplen > 2 && strval[0] == '.' && strval[1] == '.' && strval[2] == '/') ||
					    memchr(strval, '\0', cplen) ||
					    cplen == 0 ||
					    (cplen > 2 && strval[cplen-1] == '.' && strval[cplen-2] == '.' && strval[cplen-3] == '/') ||
					    qpf_internal_FindStr(strval, cplen, "/../", 4) >= 0))
					{ rval = -EINVAL; QPERR(QPF_ERR_T_BADPATH); goto error; }
				    break;

				case QPF_SPEC_T_B64:
				    n = qpf_internal_base64encode(s, strval, cplen, str, size, &cpoffset, grow_fn, grow_arg);
				    if (UNLIKELY(n < 0))
					{
					rval = -EINVAL;
					goto error;
					}
				    copied += n;
				    cplen = 0;
				    break;
				
				case QPF_SPEC_T_DB64:
				    n = qpf_internal_base64decode(s, strval, cplen, str, size, &cpoffset, grow_fn, grow_arg);
				    if (UNLIKELY(n < 0))
					{
					rval = -EINVAL;
					goto error;
					}
				    copied += n;
				    cplen = 0;
				    break;
				
				case QPF_SPEC_T_DHEX:
				    n = qpf_internal_hexdecode(s, strval, cplen, str, size, &cpoffset, grow_fn, grow_arg);
				    if (UNLIKELY(n < 0))
					{
					rval = -EINVAL;
					goto error;
					}
				    copied += n;
				    cplen = 0;
				    break;
				
				case QPF_SPEC_T_ESCQ:
				case QPF_SPEC_T_ESCQWS:
				case QPF_SPEC_T_JSSTR:
				case QPF_SPEC_T_JSONSTR:
				case QPF_SPEC_T_DSYB:
				case QPF_SPEC_T_CSSVAL:
				case QPF_SPEC_T_CSSURL:
				case QPF_SPEC_T_ESCWS:
				case QPF_SPEC_T_QUOT:
				case QPF_SPEC_T_DQUOT:
				case QPF_SPEC_T_HTE:
				case QPF_SPEC_T_HTENLBR:
				case QPF_SPEC_T_HEX:
				case QPF_SPEC_T_URL:
				    if (LIKELY(n_specs-i == 1 || (n_specs-i == 2 && specchain[i+1] == QPF_SPEC_T_NLEN)))
					{
					if (n_specs-i == 2)
					    maxdst = specchain_n[i+1];
					else
					    maxdst = INT_MAX;
					switch(specchain[i])
					    {
					    case QPF_SPEC_T_ESCQ:	table = &QPF.quote_matrix;
									min_room = 1;
									quote = 0;
									break;
					    case QPF_SPEC_T_ESCQWS:	table = &QPF.quote_ws_matrix;
									min_room = 1;
									quote = 0;
									break;
					    case QPF_SPEC_T_JSSTR:	table = &QPF.jsstr_matrix;
									min_room = 1;
									quote = 0;
									break;
					    case QPF_SPEC_T_JSONSTR:	table = &QPF.jsonstr_matrix;
									min_room = 1;
									quote = 0;
									break;
					    case QPF_SPEC_T_DSYB:	table = &QPF.dsyb_matrix;
									min_room = 1;
									quote = 0;
									break;
					    case QPF_SPEC_T_SSYB:	table = &QPF.ssyb_matrix;
									min_room = 1;
									quote = 0;
									break;
					    case QPF_SPEC_T_CSSVAL:	table = &QPF.cssval_matrix;
									min_room = 1;
									quote = 0;
									break;
					    case QPF_SPEC_T_CSSURL:	table = &QPF.cssurl_matrix;
									min_room = 1;
									quote = 0;
									break;
					    case QPF_SPEC_T_ESCWS:	table = &QPF.ws_matrix;
									min_room = 1;
									quote = 0;
									break;
					    case QPF_SPEC_T_QUOT:	table = &QPF.quote_matrix; 
									min_room = 2;
									quote = '\'';
									break;
					    case QPF_SPEC_T_DQUOT:	table = &QPF.quote_matrix;
									min_room = 2;
									quote = '"';
									break;
					    case QPF_SPEC_T_HTE:	table = &QPF.hte_matrix;
									min_room = 1;
									quote = 0;
									break;
					    case QPF_SPEC_T_HTENLBR:	table = &QPF.htenlbr_matrix;
									min_room = 1;
									quote = 0;
									break;
					    case QPF_SPEC_T_HEX:	table = &QPF.hex_matrix;
									min_room = 1;
									quote = 0;
									break;
					    case QPF_SPEC_T_URL:	table = &QPF.url_matrix;
									min_room = 1;
									quote = 0;
									break;
					    default:			table = NULL; 
									quote = 0;
									min_room = 1;
									QPERR(QPF_ERR_T_INTERNAL);
									break;
					    }
					if (quote && specchain[0] != QPF_SPEC_T_STR)
					    {
					    /** don't quote things other than strings **/
					    if (UNLIKELY(cplen > maxdst))
						{
						QPERR(QPF_ERR_T_INSOVERFLOW);
						cplen = maxdst;
						}
					    break;
					    }
					if (quote)
					    {
					    if (UNLIKELY(maxdst < 2))
						{
						QPERR(QPF_ERR_T_BADFORMAT);
						rval = -EINVAL;
						goto error;
						}
					    maxdst -= 2;
					    }
					if (quote)
					    {
					    if (LIKELY(!nogrow && (cpoffset + 2 <= *size || grow_fn(str, size, cpoffset, grow_arg, cpoffset + 2))))
						{
						(*str)[cpoffset++] = quote;
						}
					    else
						{
						QPERR(QPF_ERR_T_BUFOVERFLOW);
						nogrow = 1;
						}
					    copied++;
					    }
					oldcpoffset = cpoffset;
					n = qpf_internal_Translate(s, strval, cplen, str, &cpoffset, size, maxdst, table, nogrow?NULL:grow_fn, grow_arg, min_room);
					if (UNLIKELY(n < 0))
					    {
					    QPERR(QPF_ERR_T_INTERNAL);
					    rval = n;
					    goto error;
					    }
					if (n != cpoffset - oldcpoffset) nogrow = 1;
					if (quote)
					    {
					    if (LIKELY(cpoffset + 2 <= *size || grow_fn(str, size, cpoffset, grow_arg, cpoffset + 2)))
						{
						(*str)[cpoffset++] = quote;
						}
					    else
						{
						QPERR(QPF_ERR_T_BUFOVERFLOW);
						nogrow = 1;
						}
					    copied++;
					    }
					copied += n;
					cplen = 0;
					}
				    else
					{
					QPERR(QPF_ERR_T_NOTIMPL);
					rval = -ENOSYS;
					goto error;
					}
				    break;

				default:
				    /** Unimplemented filter **/
				    QPERR(QPF_ERR_T_NOTIMPL);
				    rval = -ENOSYS;
				    goto error;
				}
			    }

			/** Copy it. **/
			if (LIKELY(cplen != 0))
			    {
			    copied += cplen;
			    if (UNLIKELY(cpoffset+cplen+1 > SIZE_MAX/2))
				{
				QPERR(QPF_ERR_T_BUFOVERFLOW);
				rval = -EINVAL;
				goto error;
				}
			    if (UNLIKELY(nogrow)) cplen = 0;
			    if (UNLIKELY(cpoffset+cplen+1 > *size) && (!grow_fn(str, size, cpoffset, grow_arg, cpoffset+cplen+1)))
				{
				QPERR(QPF_ERR_T_BUFOVERFLOW);
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


/*** Registers a new extension with the QPF module.  This allows outside
 *** modules to provide extra specifiers for processing data or for data
 *** sources.
 *** 
 *** Warning:  It appears that extensions are not currently implemented.
 *** TODO: This comment should document the expected signature for ext_fn()
 *** 	more clearly once it is used somewhere.
 *** 
 *** @param ext_spec The name of the extension (e.g. "STR" for `%str`).
 *** @param ext_fn The function to call on data that meets the extension.
 *** @param is_source If nonzero, the extension is treated as a source
 *** 	specifier (also known as a format specifier, e.g. `%STR`), otherwise
 *** 	it is a treated as a filtering specifier (e.g. `&QUOT`).
 ***/
void
qpfRegisterExt(char* ext_spec, int (*ext_fn)(), int is_source)
    {

	/** Check if extension max has been reached. **/
	if (UNLIKELY(QPF.n_ext >= QPF_MAX_EXTS))
	    {
	    fprintf(stderr, "warning: qpfRegisterExt: QPF_MAX_EXTS exceeded\n");
	    return;
	    }
	
	/** Add to list of extensions **/
	QPF.ext_specs[QPF.n_ext] = nmSysStrdup(ext_spec);
	QPF.ext_fns[QPF.n_ext] = ext_fn;
	QPF.is_source[QPF.n_ext] = is_source?1:0;
	QPF.n_ext++;

    return;
    }
