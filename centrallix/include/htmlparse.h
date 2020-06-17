#ifndef _HTMLPARSE_H
#define _HTMLPARSE_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module: 	htmlparse.c,htmlparse.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	April 23, 1999   					*/
/* Description:	Provides HTML parsing facilities for converting to FX	*/
/*		and PCL style, and also possibly later for parsing HTML	*/
/*		documents to re-point embedded URL's that are converted	*/
/*		by the proxy agent... hmmm...				*/
/************************************************************************/


#include "cxlib/mtask.h"
#include "obj.h"
#include "stparse_ne.h"


#define HTS_READ_SIZE	1024
#define HTS_BUF_SIZE	(2*HTS_READ_SIZE)


/** HTML parser session structure **/
typedef struct _HTS
    {
    int			(*ReadFn)();
    void*		ReadArg;
    int			Type;		/* one of HTS_T_xxx, see below */
    char		Buffer[HTS_BUF_SIZE+1];
    char*		BufPtr;
    int			BufCnt;
    char*		SetPtr;
    char		ConvBuf[HTS_BUF_SIZE+1];
    char*		ConvSetPtr;
    char*		ConvBufPtr;
    int			ConvBufCnt;
    int			PutBackCnt;
    short		TagParts[16];	/* offset from ConvSetPtr */
    short		TagValues[16];	/* offset from ConvSetPtr */
    int			nTagParts;
    int			Flags;		/* bitmask HTS_F_xxx, see below */
    }
    HtmlSession, *pHtmlSession;

/** Types **/
#define HTS_T_BEGIN		0
#define HTS_T_COMMENT		1
#define HTS_T_TAG		2
#define HTS_T_STRING		3
#define HTS_T_EOF		4
#define HTS_T_ERROR		5


/** Flags **/
#define HTS_F_INSTRING		1


/** Special characters **/
#define HTS_CH_EOF		(-1)
#define HTS_CH_ERROR		(-2)
#define HTS_CH_WHITESPACE	(-3)
#define HTS_CH_TAGSTART		(-4)
#define HTS_CH_TAGEND		(-5)


/** Functions **/
pHtmlSession htsOpenSession(int (*read_fn)(), void* read_arg, int flags);
int htsCloseSession(pHtmlSession this);
int htsNextToken(pHtmlSession this);
int htsStringVal(pHtmlSession this, char** string_ptr, int* string_len);
char* htsTagPart(pHtmlSession this, int part_num);
char* htsTagValue(pHtmlSession this, int part_num);
pStruct htsParseURL(char* url);
char htsConvertChar(char** ptr);

#endif /* not defined _HTMLPARSE_H */
