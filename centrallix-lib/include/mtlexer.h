#ifndef _MTLEXER_H
#define _MTLEXER_H

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
/* Module: 	mtlexer.c, mtlexer.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	September 23, 1998					*/
/* Description:	Lexical analyzer (tokenizing preprocessor) for the 	*/
/*		Centrallix proj.  This module handles the processing	*/
/*		of the various files and http headers, etc.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: mtlexer.h,v 1.1 2001/08/13 18:04:19 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/mtlexer.h,v $

    $Log: mtlexer.h,v $
    Revision 1.1  2001/08/13 18:04:19  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/07/03 01:03:01  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


#include "mtask.h"

/** lexer session structure **/
typedef struct _LX
    {
    int			Magic;
    char*		BufPtr;		/* ptr to next data */
    int			BufCnt;		/* length of buffer data */
    char		Buffer[2048];	/* line buffer */
    char		Delimiter;	/* string delim. */
    int			Flags;
    char		InpBuf[2049];	/* input buffer */
    char*		InpPtr;
    int			InpCnt;
    char		TokString[256];	/* string val */
    int			TokStrCnt;	/* counter for string value */
    int			TokInteger;	/* int val */
    double		TokDouble;	/* double val */
    int			TokType;	/* current token type */
    int			Hold;		/* hold current token. */
    char**		ReservedWords;	/* reserved words */
    int			LineNumber;
    int			(*ReadFn)();
    void*		ReadArg;
    }
    LxSession, *pLxSession;

/** lexer functions **/
pLxSession mlxOpenSession(pFile fd, int flags);
pLxSession mlxStringSession(char* str, int flags);
pLxSession mlxGenericSession(void* src, int (*read_fn)(), int flags);
int mlxCloseSession(pLxSession this);
int mlxNextToken(pLxSession this);
char* mlxStringVal(pLxSession this, int* alloc);
int mlxIntVal(pLxSession this);
double mlxDoubleVal(pLxSession this);
int mlxCopyToken(pLxSession this, char* buffer, int maxlen);
int mlxHoldToken(pLxSession this);
int mlxSetOptions(pLxSession this, int options);
int mlxUnsetOptions(pLxSession this, int options);
int mlxSetReservedWords(pLxSession this, char** res_words);
int mlxNoteError(pLxSession this);
int mlxNotePosition(pLxSession this);

/** Token types **/
#define	MLX_TOK_BEGIN		0	/* beginning of input stream */
#define MLX_TOK_STRING		1	/* string token */
#define MLX_TOK_INTEGER		2	/* numeric token */
#define MLX_TOK_EQUALS		3	/* equals sign */
#define MLX_TOK_OPENBRACE	4	/* Open curly brace '{' */
#define MLX_TOK_CLOSEBRACE	5	/* close curly brace '}' */
#define MLX_TOK_ERROR		6	/* error occurred in input */
#define MLX_TOK_KEYWORD		7	/* unquoted ascii sequence */
#define MLX_TOK_COMMA		8	/* comma separator */
#define MLX_TOK_EOL		9	/* end of line */
#define MLX_TOK_EOF		10	/* end of file */
#define MLX_TOK_COMPARE		11	/* eq, less, gtr, etc */
#define MLX_TOK_COLON		12	/* colon ':' */
#define MLX_TOK_OPENPAREN	13	/* open paren '(' */
#define MLX_TOK_CLOSEPAREN	14	/* close paren ')' */
#define MLX_TOK_SLASH		15	/* forward slash '/' */
#define MLX_TOK_PERIOD		16	/* period '.' */
#define MLX_TOK_PLUS		17	/* plus '+' */
#define MLX_TOK_ASTERISK	18	/* asterisk '*' */
#define MLX_TOK_RESERVEDWD	19	/* reserved word */
#define MLX_TOK_FILENAME	20	/* unquoted filename */
#define MLX_TOK_DOUBLE		21	/* fractional/decimal number */
#define MLX_TOK_DOLLAR		22	/* dollar sign '$' */
#define MLX_TOK_MINUS		23	/* minus sign '-' */
#define MLX_TOK_DBLOPENBRACE	24	/* double brace {{ */
#define MLX_TOK_DBLCLOSEBRACE	25	/* double cls brace }} */
#define MLX_TOK_SYMBOL		26	/* symbol mode +-=.,<> etc */
#define MLX_TOK_SEMICOLON	27	/* semicolon ';' */
#define MLX_TOK_SSTRING		28	/* string using single quotes */

/** Compare types -- get with mlxIntVal() **/
#define MLX_CMP_EQUALS		1
#define MLX_CMP_GREATER		2
#define MLX_CMP_LESS		4

/** Flags **/
#define MLX_F_INSTRING		1	/* in _long_ string (internal) */
#define MLX_F_ICASEK		2	/* lowercase all keywords. */
#define MLX_F_POUNDCOMM		4	/* Pound sign comments */
#define MLX_F_CCOMM		8	/* C comments */
#define MLX_F_CPPCOMM		16	/* C++ // comments */
#define MLX_F_SEMICOMM		32	/* Semicolon comments */
#define MLX_F_DASHCOMM		64	/* Double-dash -- comments */
#define MLX_F_EOL		128	/* Return EOL as a token */
#define MLX_F_EOF		256	/* Return EOF as a token */
#define MLX_F_NOFILE		512	/* Source is a string, not file (int) */
#define MLX_F_IFSONLY		1024	/* Only return ifs-sep strings. */
#define MLX_F_DASHKW		2048	/* Keyword can include '-' */
#define MLX_F_FOUNDEOL		4096	/* found an eol (internal) */
#define MLX_F_NODISCARD		8192	/* don't discard buffer at close */
#define MLX_F_FILENAMES		16384	/* treat as filename if /xx or ./xx */
#define MLX_F_ICASER		32768	/* lowercase all reserved wds */
#define MLX_F_ICASE		(MLX_F_ICASER | MLX_F_ICASEK)
#define MLX_F_DBLBRACE		65536	/* {{ is double brace, not 2 singles */
#define MLX_F_LINEONLY		131072	/* only parse into lines. */
#define MLX_F_SYMBOLMODE	262144	/* return all symbols as same token */
#define MLX_F_TOKCOMM		524288	/* return comments as a token */
#define MLX_F_INCOMM		1048576	/* Inside a C-style comment (internal) */
#define MLX_F_NOUNESC		2097152	/* Don't remove escapes in strings */
#define MLX_F_SSTRING		4194304	/* Differentiate "" and '' strings */

/** Other defines **/
#define MLX_BLK_SIZ		1024

#endif /* _MTLEXER_H */

