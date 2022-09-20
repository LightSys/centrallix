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
#define QPF_SPEC_T_LL     (7)
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
    "STR",
    "POS",
    "DBL",
    "nSTR",
    "CHR",
    "LL",
    "QUOT",
    "DQUOT",
    "SYM",
    "JSSTR",
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
    "HTENLBR",
    "DHTE",
    "URL",
    "DURL",
    "nLSET",
    "nRSET",
    "nZRSET",
    "SQLARG",
    "SQLSYM",
    "HTDATA",	/* 36 */
    "HTE",
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
    QPConvTable	quote_matrix;
    QPConvTable	quote_ws_matrix;
    QPConvTable	ws_matrix;
    QPConvTable	hte_matrix;
    QPConvTable	htenlbr_matrix;
    QPConvTable	hex_matrix;
    QPConvTable	url_matrix;
    QPConvTable	jsstr_matrix;
    QPConvTable	jsonstr_matrix;
    QPConvTable	cssval_matrix;
    QPConvTable	cssurl_matrix;
    QPConvTable	dsyb_matrix;
    QPConvTable	ssyb_matrix;
    }
    QPF_t;
    
static QPF_t QPF = { n_ext:0, is_init:0 };

/*** qprInitialize() - used to initialize the global flags like a 
 *** driver would. However, qpf is NOT an os driver.
 ***/
void
qprInitialize(int isUTF8)
    {
	if(isUTF8) QPF_FLAGS.Flags = QPF_F_ENFORCE_UTF8;
	else QPF_FLAGS.Flags = 0;
	return;
	}

int
qpf_internal_FindStr(const char* haystack, size_t haystacklen, const char* needle, size_t needlelen)
    {
    int pos;
    char* ptr;
    if (needlelen > haystacklen) return -1;
    if (needlelen == 0) return 0;
    pos = 0;
    while(pos <= haystacklen - needlelen)
	{
	ptr = memchr(haystack+pos, needle[0], haystacklen - pos);
	if (!ptr) return -1;
	if (memcmp(ptr, needle, needlelen) == 0)
	    return (ptr - haystack);
	pos = (ptr - haystack) + 1;
	}
    return -1;
    }


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


/*** qpfInitialize() - inits the QPF suite.
 ***/
int
qpfInitialize()
    {
    int i;
    char buf[4];
    char hex[] = "0123456789abcdef";

	memset(&QPF.quote_matrix, 0, sizeof(QPF.quote_matrix));
	QPF.quote_matrix.Matrix['\''] = "\\'";
	QPF.quote_matrix.Matrix['"'] = "\\\"";
	QPF.quote_matrix.Matrix['\\'] = "\\\\";
	qpf_internal_SetupTable(&QPF.quote_matrix);

	memset(&QPF.quote_ws_matrix, 0, sizeof(QPF.quote_ws_matrix));
	QPF.quote_ws_matrix.Matrix['\''] = "\\'";
	QPF.quote_ws_matrix.Matrix['"'] = "\\\"";
	QPF.quote_ws_matrix.Matrix['\\'] = "\\\\";
	QPF.quote_ws_matrix.Matrix['\n'] = "\\n";
	QPF.quote_ws_matrix.Matrix['\t'] = "\\t";
	QPF.quote_ws_matrix.Matrix['\r'] = "\\r";
	qpf_internal_SetupTable(&QPF.quote_ws_matrix);

	memset(&QPF.jsstr_matrix, 0, sizeof(QPF.jsstr_matrix)); /* javascript */
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

	memset(&QPF.ws_matrix, 0, sizeof(QPF.ws_matrix));
	QPF.ws_matrix.Matrix['\n'] = "\\n";
	QPF.ws_matrix.Matrix['\t'] = "\\t";
	QPF.ws_matrix.Matrix['\r'] = "\\r";
	qpf_internal_SetupTable(&QPF.ws_matrix);

	memset(&QPF.dsyb_matrix, 0, sizeof(QPF.dsyb_matrix));
	QPF.dsyb_matrix.Matrix['"'] = "\"\"";
	qpf_internal_SetupTable(&QPF.dsyb_matrix);

	memset(&QPF.ssyb_matrix, 0, sizeof(QPF.ssyb_matrix));
	QPF.ssyb_matrix.Matrix['\''] = "''";
	qpf_internal_SetupTable(&QPF.ssyb_matrix);

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

	for(i=0;i<QPF_MATRIX_SIZE;i++)
	    {
	    buf[0] = hex[(i>>4)&0x0F];
	    buf[1] = hex[i&0x0F];
	    buf[2] = '\0';
	    QPF.hex_matrix.Matrix[i] = nmSysStrdup(buf);
	    }
	qpf_internal_SetupTable(&QPF.hex_matrix);

	/* set up table for url encoding everything except 0-9, A-Z, and a-z */
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

	
	for(i=0;i<=QPF_SPEC_T_MAXSPEC;i++)
	    {
	    if (qpf_spec_names[i]) 
		qpf_spec_len[i] = strlen(qpf_spec_names[i]);
	    }

	QPF.is_init = 1;

    return 0;
    }


/*** qpfOpenSession() - open a new qprintf session.  The session is used for
 *** storing cumulative error information, to make error handling much cleaner
 *** for any caller that wants to do so.
 ***
 *** Assumes default values for all flags based on global context (currently just the one flag)
 ***/
pQPSession
qpfOpenSession()
    {
    pQPSession s = NULL;

	if (!QPF.is_init) 
	    {
	    if (qpfInitialize() < 0) return NULL;
	    }

	s = (pQPSession)nmMalloc(sizeof(QPSession));
	if (!s) return NULL;
	s->Errors = 0;
	s->Flags = QPF_FLAGS.Flags;
    return s;
    }

/*** qpfOpenSessionFlags() - Same as qpfOpenSession, but enables session flags to be set directly, 
 *** trumping global flags
 ***/
pQPSession
qpfOpenSessionFlags(unsigned int flags)
    {
    pQPSession s = NULL;

	if (!QPF.is_init) 
	    {
	    if (qpfInitialize() < 0) return NULL;
	    }

	s = (pQPSession)nmMalloc(sizeof(QPSession));
	if (!s) return NULL;
	s->Errors = 0;
	s->Flags = flags;

    return s;
    }


/*** qpfErrors() - return the current error mask.
 ***/
unsigned int
qpfErrors(pQPSession s)
    {
    return s->Errors;
    }


/*** qpfClearErrors() - reset the errors mask.
 ***/
int
qpfClearErrors(pQPSession s)
    {
    s->Errors = 0;
    return 0;
    }


/*** qpfCloseSession() - close a qprintf session.
 ***/
int
qpfCloseSession(pQPSession s)
    {
	
	nmFree(s, sizeof(QPSession));

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


/*** numBytesInChar_internal() - computes the number of bytes in a utf-8 char based on 
 *** the first byte of the character. Returns -1 for a byte which does not indicate the 
 *** legth of the byte (i.e., anything of the form 10XX XXXX or 1111 10XX would be invalid)
 *** 
 *** NOTE: this should not be used to validate characters; overlong forms, UTF-16 surrogate
 *** halves, and overly large characters starting with 0xF4-0xF7 are not handled
 ***/
int
numBytesInChar_internal(char byte)
    {
    if      (!(byte&0x80)) return  1; /* of the form 0XXX XXXX */ 
    else if (!(byte&0x40)) return -1; /* of the form 10XX XXXX */
    else if (!(byte&0x20)) return  2; /* of the form 110X XXXX */
    else if (!(byte&0x10)) return  3; /* of the form 1110 XXXX */
    else if (!(byte&0x08)) return  4; /* of the form 1111 0XXX */
    else                   return -1; /* of the form 1111 1XXX */
    }


/*** qpf_internal_base64encode() - convert string to base 64 representation
 *** returns the number of characters added to destination, and modifies dst_offset
 ***/
int
qpf_internal_base64encode(pQPSession s, const char* src, size_t src_size, char** dst, size_t* dst_size, size_t* dst_offset, qpf_grow_fn_t grow_fn, void* grow_arg)
    {
    static char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const unsigned char* srcptr = (const unsigned char*)src;
    const unsigned char* origsrc = (const unsigned char*)src;
    char* dstptr;
    int req_size = ((src_size+2) / 3) * 4 + *dst_offset + 1; /* make sure to leave room for null */ 
	int oldOff = *dst_offset;

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

	*dst_offset = (dstptr - *dst); 

    return dstptr - *dst - oldOff; 
    }


/*** qpf_internal_base64decode() - convert base 64 to a string representation
 ***/
static inline int
qpf_internal_base64decode(pQPSession s, const char* src, size_t src_size, char** dst, size_t* dst_size, size_t* dst_offset, qpf_grow_fn_t grow_fn, void* grow_arg)
    {
    char b64[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char* ptr;
    char* cursor;
    int ix;
    int bytes_left;
    int req_size = (.75 * src_size) + *dst_offset + 1; /** +1 for null. Valid B64 must be a multiple of 4, so no truncation **/
    int oldOffset = *dst_offset;
    char * oldSrc = src;

        /* adjust esitmate; only accurate to nearest 3 bytes otherwise */
	if(src[src_size - 2] == '=' && src[src_size - 1] == '=') req_size -= 2;
	else if(src[src_size - 1] == '=') req_size -= 1;

	if(src_size % 4 != 0) /* confirm assumption above is correct. */
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
	while(*src && src - oldSrc < src_size)
	    {
	    /** First 6 bits. **/
	    ptr = strchr(b64,src[0]);
	    if (!ptr || !*ptr) 
	        {
		QPERR(QPF_ERR_T_BADCHAR);
		return -1;
		}
	    ix = ptr-b64;
	    cursor[0] = ix<<2;

	    /** Second six bits are split between cursor[0] and cursor[1] **/
	    ptr = strchr(b64,src[1]);
	    if (!ptr || !*ptr)
	        {
		QPERR(QPF_ERR_T_BADCHAR);
		return -1;
		}
	    ix = ptr-b64;
	    cursor[0] |= ix>>4;
	    cursor[1] = (ix<<4)&0xF0;

	    /** make sure have enough room for the full character just started. **/
	    bytes_left = src_size - (int) (src + 4 - oldSrc); /* source bytes after this iteration */
	    /** only 4 byte chars could be a problem here **/
	    if(s->Flags & QPF_F_ENFORCE_UTF8 && numBytesInChar_internal((char)(cursor[0])) >= 4 && bytes_left < 4 ) break;

	    /** Third six bits are nonmandatory and split between cursor[1] and [2] **/
	    if (src[2] == '=' && src[3] == '=')
	        {
		cursor += 1;
		break;
		}
	    ptr = strchr(b64,src[2]);
	    if (!ptr || !*ptr)
	        {
		QPERR(QPF_ERR_T_BADCHAR);
		return -1;
		}
	    ix = ptr-b64;
	    cursor[1] |= ix>>2;
	    cursor[2] = (ix<<6)&0xC0;

	    /** a 3 or 4 byte char could be a problem **/
	    if(s->Flags & QPF_F_ENFORCE_UTF8 && numBytesInChar_internal((char)(cursor[1])) >= 3 && bytes_left < 4 )
		{
		cursor += 1;
		break;
		}

	    /** Fourth six bits are nonmandatory and a part of cursor[2]. **/
	    if (src[3] == '=')
	        {
		cursor += 2;
		break;
		}
	    ptr = strchr(b64,src[3]);
	    if (!ptr || !*ptr)
	        {
		QPERR(QPF_ERR_T_BADCHAR);
		return -1;
		}
	    ix = ptr-b64;
	    cursor[2] |= ix;
	    
	    /** a 2, 3, or 4 byte char could be a problem **/
	    if(s->Flags & QPF_F_ENFORCE_UTF8 && numBytesInChar_internal((char)(cursor[2])) >= 2 && bytes_left < 4 )
	        {
		cursor += 2;
		break;
		}

	    src += 4;
	    cursor += 3;
	    }

    /* make sure data decoded is valid, if applicable */
    *cursor = 0; /* only check what just added */
    if(s->Flags & QPF_F_ENFORCE_UTF8 && verifyUTF8((*dst + oldOffset)) != 0)
	{
	QPERR(QPF_ERR_T_BADCHAR);
	return -1;
	}

    *dst_offset = cursor - *dst;
    return *dst_offset - oldOffset;
    }


/*** qpf_internal_hexdecode() - convert base 16 to a string representation
 ***/
static inline int
qpf_internal_hexdecode(pQPSession s, const char* src, size_t src_size, char** dst, size_t* dst_size, size_t* dst_offset, qpf_grow_fn_t grow_fn, void* grow_arg)
    {
    char hex[23] = "0123456789abcdefABCDEF";
    int conv[22] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 10, 11, 12, 13, 14, 15 };
    char* ptr;
    char* cursor;
    int ix;
    int req_size, bytes_left;
    int oldOffset = *dst_offset;
    char* oldSrc = src;

	/** Required size **/
	if (src_size%2 == 1)
	    {
	    QPERR(QPF_ERR_T_BADLENGTH);
	    return -1;
	    }
	req_size = src_size/2 + *dst_offset + 1; /* need to fit decoded data and null */

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
	while(*src && src - oldSrc < src_size)
	    {
	    /** First 4 bits. **/
	    ptr = strchr(hex, src[0]);
	    if (!ptr || !*ptr) /* make sure null's are not counted */
	        {
		QPERR(QPF_ERR_T_BADCHAR);
		return -1;
		}
	    ix = conv[ptr-hex];
	    cursor[0] = ix<<4;

	    /** Second four bits  **/
	    ptr = strchr(hex, src[1]);
	    if (!ptr || !*ptr)
	        {
		QPERR(QPF_ERR_T_BADCHAR);
		return -1;
		}
	    ix = conv[ptr-hex];
	    cursor[0] |= ix;

            /** make sure have enough room for the full character just started. **/
	    bytes_left = src_size - (int) (src + 2 - oldSrc); /* source bytes after this iteration */
	    if(s->Flags & QPF_F_ENFORCE_UTF8 && (numBytesInChar_internal((char)*(cursor)) - 1)*2 > bytes_left ) break;

	    src += 2;
	    cursor += 1;
	    }

	    *cursor = 0; /* only check what just added */
	    if(s->Flags & QPF_F_ENFORCE_UTF8 && verifyUTF8((*dst + oldOffset)) != 0)
		{
		QPERR(QPF_ERR_T_BADCHAR);
		return -1;
		}
    
        *dst_offset = cursor - *dst; 

    return *dst_offset - oldOffset;
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


/*** qpfPrintf_grow() - returns whether the additional size will fit.
 ***/
int
qpfPrintf_grow(char** str, size_t* size, size_t offs, void* arg, size_t req_size)
    {
    return (*size) >= req_size;
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
    int i, j;
    char* trans;
    int nogrow = (grow_fn == NULL);
    int charBytes, totalBytes;

	if (srcsize >= SIZE_MAX/2/table->MaxExpand)
	    return -1;

	if (srcsize)
	    {
	    rval += srcsize;
	    if ((srcsize*table->MaxExpand) <= limit && (srcsize*table->MaxExpand + min_room) <= (*dstsize - *dstoffs))
		{
		/** Easy route - definitely enough space! **/
		for(i=0;i<srcsize;i++)
		    {
		    if (__builtin_expect(((trans = table->Matrix[(unsigned char)(srcbuf[i])]) != NULL), 0))
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
		    if (__builtin_expect(((trans = table->Matrix[(unsigned char)(srcbuf[i])]) != NULL), 0))
			{
			tlen = table->MatrixLen[(unsigned char)(srcbuf[i])];
			/** if enforcing utf-8, then must make sure full byte will fit. **/
			/* Determine if full char will fit. If is 1 byte, stays tlen. If middle byte (i.e. 10XX XXXX) was checked before */
			charBytes = numBytesInChar_internal(srcbuf[i]);
			totalBytes = tlen; 
			if(s->Flags & QPF_F_ENFORCE_UTF8 && charBytes >= 2) 
			    {
			    for(j = 1 ; j < charBytes ; j++ )
			        {
				/** if will sub, need to count on larger size. If not, just needs 1 byte.  **/
				totalBytes += ((table->Matrix[(unsigned char)(srcbuf[i+j])]) != NULL)?  
				    table->MatrixLen[(unsigned char)(srcbuf[i+j])] : 1;
				}
			    }

			if (__builtin_expect(limit >= totalBytes, 1))
			    {
			    rval += (tlen-1);
			    if (__builtin_expect(!nogrow,1) && (__builtin_expect((*dstoffs)+totalBytes+min_room <= (*dstsize), 1) || 
				  (grow_fn(dstbuf, dstsize, *dstoffs, grow_arg, (*dstoffs)+totalBytes+min_room))))
				{
				while(*trans) (*dstbuf)[(*dstoffs)++] = *(trans++);
				limit -= tlen; /* although room is ensured for totalBytes, only used up tlen so far */
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
			    limit = 0;
			    }
			}
		    else
			{
			if (__builtin_expect(limit > 0, 1)) 
			    {
				/** check for UTF-8 **/
				if(s->Flags & QPF_F_ENFORCE_UTF8)
				    {
				    charBytes = numBytesInChar_internal(srcbuf[i]);
				    if(charBytes == -1)
				        { 
				        QPERR(QPF_ERR_T_BADCHAR); /* let caller handle */
				        charBytes = 1; 
				        }
				    }
				else charBytes = 1;

			    if ((__builtin_expect(!nogrow,1) && (__builtin_expect((*dstoffs)+charBytes+min_room <= (*dstsize), 1)) || 
				  (grow_fn(dstbuf, dstsize, *dstoffs, grow_arg, (*dstoffs)+charBytes+min_room))))
				{
				/** make sure the whole chracter fits **/
				if(charBytes > limit)
				    {
				    QPERR(QPF_ERR_T_TRUNC);
				    limit = 0; /* prevent any other chars from filling in the space */
				    rval--;
				    continue;
				    }

				/* copy over all bytes of char at once */ 
				for(j = 0 ; j < charBytes ; j++)
				    {
			 	    (*dstbuf)[(*dstoffs)++] = srcbuf[i+j];
				    }
				i+=j-1;
				limit -= charBytes;
				}
			    else
				{
				QPERR(QPF_ERR_T_BUFOVERFLOW);
				break;
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


/*** qpfPrintf_va_internal() - does all of the guts work of the qpfPrintf
 *** family of functions.
 ***
 *** A warning to those who would modify this:  the 'str' parameter may
 *** change out from under this function to a new buffer if a realloc is
 *** done by the grow_fn function.  Do not store pointers to 'str'.  Go
 *** solely by offsets.
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

	if (!QPF.is_init) 
	    {
	    if (qpfInitialize() < 0) return -ENOMEM;
	    }

	if (!s)
	    {
	    null_session.Errors = 0;
	    s=&null_session;
	    }

	/** this all falls apart if there isn't at least room for the
	 ** null terminator!
	 **/
	if ((!*str || *size < 1) && !grow_fn(str, size, cpoffset, grow_arg, 1)) 
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
		if (__builtin_expect(ptrdiff_cplen < 0 || ptrdiff_cplen >= SIZE_MAX/4, 0))
		    {
		    QPERR(QPF_ERR_T_BUFOVERFLOW);
		    rval = -EINVAL;
		    goto error;
		    }
		cplen = ptrdiff_cplen;
		if (__builtin_expect(nogrow, 0)) cplen = 0;
		if (__builtin_expect(cpoffset+cplen+1 > SIZE_MAX/2, 0))
		    {
		    QPERR(QPF_ERR_T_BUFOVERFLOW);
		    rval = -EINVAL;
		    goto error;
		    }
		if (__builtin_expect(cpoffset+cplen+1 > *size, 0) && (nogrow || !grow_fn(str, size, cpoffset, grow_arg, cpoffset+cplen+1)))
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
		if (__builtin_expect(format[0] == '%', 0))
		    {
		    if (__builtin_expect(!nogrow, 1) && (__builtin_expect(cpoffset+2 <= *size, 1) || (grow_fn(str, size, cpoffset, grow_arg, cpoffset+2))))
			(*str)[cpoffset++] = '%';
		    else
			{
			QPERR(QPF_ERR_T_BUFOVERFLOW);
			nogrow = 1;
			}
		    copied++;
		    format++;
		    }
		else if (__builtin_expect(format[0] == '&',0))
		    {
		    if (__builtin_expect(!nogrow, 1) && (__builtin_expect(cpoffset+2 <= *size, 1) || (grow_fn(str, size, cpoffset, grow_arg, cpoffset+2))))
			(*str)[cpoffset++] = '&';
		    else
			{
			QPERR(QPF_ERR_T_BUFOVERFLOW);
			nogrow = 1;
			}
		    copied++;
		    format++;
		    }
		else if (__builtin_expect(format[0] == ']',0))
		    {
		    format++;
		    ignore = 0;
		    }
		else if (__builtin_expect(format[0] == '[',0))
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
			if (__builtin_expect(!found, 0))
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
			    if (__builtin_expect(n_specs == QPF_MAX_SPECS,0))
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
			    if (__builtin_expect(strval == NULL && !ignore, 0))
				{ rval = -EINVAL; QPERR(QPF_ERR_T_NULL); goto error; }
			    cplen = strval?strlen(strval):0;
			    break;

			case QPF_SPEC_T_POS:
			    intval = va_arg(ap, int);
			    if (__builtin_expect(intval < 0 && !ignore, 0))
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
			    if (__builtin_expect(strval == NULL && !ignore, 0))
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
			if (__builtin_expect(cplen < 0, 0))
			    { rval = -EINVAL; QPERR(QPF_ERR_T_BADLENGTH); goto error; }

			/** Filters? **/
			for (i=1;i<n_specs;i++)
			    {
			    switch(specchain[i])
				{
				case QPF_SPEC_T_NLEN:
				    if (cplen > specchain_n[i])
					{
					QPERR(QPF_ERR_T_INSOVERFLOW);
					cplen = specchain_n[i];
					}
				    break;

				case QPF_SPEC_T_SYM:
				    if (n_specs-i == 2 && specchain[i+1] == QPF_SPEC_T_NLEN && cplen > specchain_n[i+1])
					cplen = specchain_n[i+1];
				    if (cxsecVerifySymbol_n(strval, cplen) < 0)
					{ rval = -EINVAL; QPERR(QPF_ERR_T_BADSYMBOL); goto error; }
				    break;

				case QPF_SPEC_T_FILE:
				    if (n_specs-i == 2 && specchain[i+1] == QPF_SPEC_T_NLEN && cplen > specchain_n[i+1])
					cplen = specchain_n[i+1];
				    if ((cplen == 1 && strval[0] == '.') || 
					    (cplen == 2 && strval[0] == '.' && strval[1] == '.') ||
					    memchr(strval, '/', cplen) || 
					    memchr(strval, '\0', cplen) ||
					    cplen == 0 ||
					    (s->Flags & QPF_F_ENFORCE_UTF8 && verifyUTF8(strval) != 0))
					{ rval = -EINVAL; QPERR(QPF_ERR_T_BADFILE); goto error; }
				    break;

				case QPF_SPEC_T_PATH:
				    if (n_specs-i == 2 && specchain[i+1] == QPF_SPEC_T_NLEN && cplen > specchain_n[i+1])
					cplen = specchain_n[i+1];
				    if ((cplen == 2 && strval[0] == '.' && strval[1] == '.') ||
					    (cplen > 2 && strval[0] == '.' && strval[1] == '.' && strval[2] == '/') ||
					    memchr(strval, '\0', cplen) ||
					    cplen == 0 ||
					    (cplen > 2 && strval[cplen-1] == '.' && strval[cplen-2] == '.' && strval[cplen-3] == '/') ||
					    qpf_internal_FindStr(strval, cplen, "/../", 4) >= 0 || 
					    (s->Flags & QPF_F_ENFORCE_UTF8 && verifyUTF8(strval) != 0))
					{ rval = -EINVAL; QPERR(QPF_ERR_T_BADPATH); goto error; }
				    break;

				case QPF_SPEC_T_B64:
				    if((n=qpf_internal_base64encode(s, strval, cplen, str, size, &cpoffset, grow_fn, grow_arg))<0) 
					{ rval = -EINVAL; goto error; } 
				    else 
					{
					copied+=n;
					cplen=0; 
					}
				    break;
				
				case QPF_SPEC_T_DB64:
				    if((n=qpf_internal_base64decode(s, strval, cplen, str, size, &cpoffset, grow_fn, grow_arg))<0) 
					{ rval = -EINVAL; goto error; } 
				    else 
					{
					copied+=n;
					cplen=0; 
					}
				    break;
				
				case QPF_SPEC_T_DHEX:
				    if((n=qpf_internal_hexdecode(s, strval, cplen, str, size, &cpoffset, grow_fn, grow_arg))<0) 
					{ rval = -EINVAL; goto error; } 
				    else 
					{
					copied+=n;
					cplen=0; 
					}
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
				    if (n_specs-i == 1 || (n_specs-i == 2 && specchain[i+1] == QPF_SPEC_T_NLEN))
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
					    if (cplen > maxdst)
						{
						QPERR(QPF_ERR_T_INSOVERFLOW);
						cplen = maxdst;
						}
					    break;
					    }
					if (quote)
					    {
					    if (maxdst < 2)
						{
						QPERR(QPF_ERR_T_BADFORMAT);
						rval = -EINVAL;
						goto error;
						}
					    maxdst -= 2;
					    }
					if (quote)
					    {
					    if (__builtin_expect(!nogrow, 1) && (__builtin_expect(cpoffset+1+1 <= *size, 1) || (grow_fn(str, size, cpoffset, grow_arg, cpoffset+1+1))))
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
					if (n < 0) 
					    {
					    QPERR(QPF_ERR_T_INTERNAL);
					    rval = n;
					    goto error;
					    }
					if (n != cpoffset - oldcpoffset) nogrow = 1;
					if (quote)
					    {
					    if ((__builtin_expect(cpoffset+1+1 <= *size, 1) || (grow_fn(str, size, cpoffset, grow_arg, cpoffset+1+1))))
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
			if (__builtin_expect(cplen != 0,1))
			    {
			    copied += cplen;
			    if (__builtin_expect(cpoffset+cplen+1 > SIZE_MAX/2, 0))
				{
				QPERR(QPF_ERR_T_BUFOVERFLOW);
				rval = -EINVAL;
				goto error;
				}
			    if (__builtin_expect(nogrow, 0)) cplen = 0;
			    if (__builtin_expect(cpoffset+cplen+1 > *size, 0) && (!grow_fn(str, size, cpoffset, grow_arg, cpoffset+cplen+1)))
				{
				QPERR(QPF_ERR_T_BUFOVERFLOW);
				cplen = (*size) - cpoffset - 1;
				nogrow = 1;
				}
				/** adjust bounds to ensure no utf-8 characters are chopped **/
				if(s->Flags & QPF_F_ENFORCE_UTF8)
				    {
				    if((unsigned char)strval[cplen-1] >= (unsigned char) 0xC0 && cplen >= 1) /* check for any header */
				        {
				    	cplen -= 1;
				    	}
				    else if((unsigned char)strval[cplen-2] >= (unsigned char) 0xE0 && cplen >= 2) /* check for cut off 3/4 byte */
				    	{
				    	cplen -= 2;
				    	}
				    else if((unsigned char)strval[cplen-3] >= (unsigned char) 0xF0 && cplen >= 3) /* check for cut off 4 byte */
				    	{
				    	cplen -= 3;
				    	}
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

    
	/** check for possible utf-8 characters split at end. Only a possible problem if in last 3 chracters **/
	if(s->Flags & QPF_F_ENFORCE_UTF8)
	    {
	    if((unsigned char)(*str)[cpoffset-1] >= (unsigned char) 0xC0 && cpoffset >= 1) /* check for any header */
	        {
	        cpoffset -= 1;
	        }
	    else if((unsigned char)(*str)[cpoffset-2] >= (unsigned char) 0xE0 && cpoffset >= 2) /* check for cut off 3/4 byte */
	    	{
	    	cpoffset -= 2;
	    	}
	    else if((unsigned char)(*str)[cpoffset-3] >= (unsigned char) 0xF0 && cpoffset >= 3) /* check for cut off 4 byte */
	    	{
	    	cpoffset -= 3;
	    	}
	    }
	
	/* if 3rd to last character is header for 4 bytes (>= 0xF0) then is truncated or invalid byte */

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
