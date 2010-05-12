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

    $Id: mtlexer.h,v 1.4 2010/05/12 18:21:21 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/mtlexer.h,v $

    $Log: mtlexer.h,v $
    Revision 1.4  2010/05/12 18:21:21  gbeeley
    - (rewrite) This is a mostly-rewrite of the mtlexer module for correctness
      and for security.  Adding many test suite items for mtlexer, a good
      fraction of which fail on the old mtlexer module.  The new module is
      currently mildly slower than the old one, but is more correct.

    Revision 1.3  2005/02/26 04:32:02  gbeeley
    - moving include file install directory to include a "cxlib/" prefix
      instead of just putting 'em all in /usr/include with everything else.

    Revision 1.2  2002/08/05 19:51:22  gbeeley
    Adding only "mildly tested" support for getting/setting the seek offset
    while in a lexer session.  The lexer does blocked/buffered I/O, so it
    is otherwise difficult to know 'where' in the document one is at.  Note
    that the offsets returned from mlxGetOffset and mlxGetCurOffset are
    relative to the *start* of the lexer processing.  If data was read from
    the file/object *before* processing with the lexer, that data is *not*
    included in the seek counts/offsets.

    Revision 1.1.1.1  2001/08/13 18:04:19  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:03:01  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/

#ifdef CXLIB_INTERNAL
#include "mtask.h"
#else
#include "cxlib/mtask.h"
#endif

#define	MLX_BUFSIZ	2048	/* amount of data to be read at a time from input */
#define MLX_STRVAL	256	/* max size of non-copied string value */

/** lexer session structure **/
typedef struct _LX
    {
    int			Magic;
    char*		BufPtr;		/* ptr to next data */
    int			BufCnt;		/* length of buffer data */
    char		Buffer[MLX_BUFSIZ];	/* line buffer */
    char		Delimiter;	/* string delim. */
    int			Flags;
    char		InpBuf[MLX_BUFSIZ + 1];	/* input buffer */
    char*		InpStartPtr;	/* start of data, for strings */
    char*		InpPtr;
    int			InpCnt;
    unsigned long	BytesRead;	/* for offset tracking */
    unsigned long	TokStartOffset;	/* for offset tracking */
    char		TokString[MLX_STRVAL];	/* string val */
    int			TokStrCnt;	/* counter for string value */
    int			TokInteger;	/* int val */
    double		TokDouble;	/* double val */
    int			TokType;	/* current token type */
    int			Hold;		/* hold current token. */
    int			EndOfFile;	/* already hit end of file */
    int			StreamErr;	/* already hit a read error */
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
unsigned long mlxGetOffset(pLxSession this);
int mlxSetOffset(pLxSession this, unsigned long new_offset);

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
#define MLX_TOK_POUND		29	/* pound sign # */
#define MLX_TOK_MAX		29	/* highest possible token value */

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
#define MLX_F_PROCLINE		8388608	/* (int) Line has been processed. */

#define MLX_F_PUBLIC		(MLX_F_ICASE | MLX_F_POUNDCOMM | MLX_F_CCOMM | MLX_F_CPPCOMM | MLX_F_SEMICOMM | MLX_F_DASHCOMM | MLX_F_EOL | MLX_F_EOF | MLX_F_IFSONLY | MLX_F_DASHKW | MLX_F_NODISCARD | MLX_F_FILENAMES | MLX_F_DBLBRACE | MLX_F_LINEONLY | MLX_F_SYMBOLMODE | MLX_F_TOKCOMM | MLX_F_NOUNESC | MLX_F_SSTRING)
#define	MLX_F_PRIVATE		(MLX_F_INSTRING | MLX_F_NOFILE | MLX_F_FOUNDEOL | MLX_F_INCOMM | MLX_F_PROCLINE)

/** Other defines **/
#define MLX_BLK_SIZ		1024

#endif /* _MTLEXER_H */

