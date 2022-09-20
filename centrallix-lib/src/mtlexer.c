#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include "newmalloc.h"
#include "mtask.h"
#include "mtlexer.h"
#include "mtsession.h"
#include "magic.h"
#include "util.h"

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
/* Module:	MTLEXER, a Lexical Analyzer (tokenizer) (mtlexer.c,.h)  */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	September 23, 1998                                      */
/* Description:	Lexical analyzer (tokenizing preprocessor) for the base	*/
/*		Centrallix library.  This module handles the pre-   	*/
/*		processing of most data files and data streams.		*/
/************************************************************************/


#ifndef __builtin_expect
#define __builtin_expect(e,c) (e)
#endif

#define MLX_EOF		(-1)
#define MLX_ERROR	(-2)

int mlxReadLine(pLxSession s, char* buf, int maxlen);
int mlxSkipChars(pLxSession s, int n_chars);
int mlxPeekChar(pLxSession s, int offset);
int mlx_internal_WillSplitUTF8(char c, int ind, int max);


/*** mlxOpenSession - start a new lexer session on a given file 
 *** descriptor returned from MTASK.  
 ***/
pLxSession
mlxOpenSession(pFile fd, int flags)
    {
    pLxSession this;

	/** Allocate the session **/
	this = (pLxSession)nmMalloc(sizeof(LxSession));
	if (!this) return NULL;
	memset(this, 0, sizeof(LxSession));

	/** Set up the session structure **/
	this->TokType = MLX_TOK_BEGIN;
	this->BufCnt = 0;
	this->BufPtr = this->Buffer;
	this->InpCnt = 0;
	this->InpPtr = this->InpBuf;
	this->ReservedWords = NULL;
	this->Flags = flags & MLX_F_PUBLIC;
	/* determine which validate to set */
	/** FIXME: change this to depend on a flag... **/
	char * locale = setlocale(LC_CTYPE, NULL);
	if(strstr(locale, "UTF-8") || strstr(locale, "UTF8") || strstr(locale, "utf-8") || strstr(locale, "utf8"))
	    {
	    this->ValidateFn = verifyUTF8; 
	    this->IsCharSplit = mlx_internal_WillSplitUTF8;
	    }
	else 
	    {
	    this->ValidateFn = NULL;
	    this->IsCharSplit = NULL;
	    }

	/** Preload the buffer with the first line **/
	this->Buffer[0] = 0;
	this->BufPtr = this->Buffer;
	this->LineNumber = 1;
	this->ReadArg = (void*)fd;
	this->ReadFn = fdRead;
	this->BytesRead = 0;
	this->TokStartOffset = 0;
	this->Magic = MGK_LXSESSION;

    return this;
    }


/*** mlxStringSession - open a session, but read the input from
 *** an immediate string instead of from a file descriptor.
 ***/
pLxSession
mlxStringSession(char* str, int flags)
    {
    pLxSession this;

	/** Allocate the session **/
	this = (pLxSession)nmMalloc(sizeof(LxSession));
	if (!this) return NULL;
	memset(this, 0, sizeof(LxSession));

	/** Set up the session structure **/
	this->TokType = MLX_TOK_BEGIN;
	this->BufCnt = 0;
	this->BufPtr = this->Buffer;
	this->InpCnt = strlen(str);
	this->InpPtr = str;
	this->InpStartPtr = str;
	this->ReservedWords = NULL;
	this->Flags = (flags & MLX_F_PUBLIC) & ~MLX_F_NODISCARD;
	this->Flags |= MLX_F_NOFILE;
	this->Magic = MGK_LXSESSION;
	/* determine which validate to set */
	char * locale = setlocale(LC_CTYPE, NULL);
	if(locale != NULL && (strstr(locale, "utf8") || strstr(locale, "UTF8") || strstr(locale, "utf-8") || strstr(locale, "UTF-8")))
	    {
	    this->ValidateFn = verifyUTF8; 
	    this->IsCharSplit = mlx_internal_WillSplitUTF8;
	    }
	else 
	    {
	    this->ValidateFn = NULL;
	    this->IsCharSplit = NULL;
	    }

	/** Read the first line. **/
	this->BufPtr = this->Buffer;
	this->LineNumber = 1;
	this->ReadArg = NULL;
	this->ReadFn = NULL;
	this->TokStartOffset = 0;
	this->BytesRead = 0;

    return this;
    }

/*** mlxGenericSession - open a generic session, which can read data
 *** from any compliant fdRead/objRead style function.
 ***/
pLxSession
mlxGenericSession(void* src, int (*read_fn)(), int flags)
    {
    pLxSession this;

	/** Allocate the session **/
	this = (pLxSession)nmMalloc(sizeof(LxSession));
	if (!this) return NULL;
	memset(this, 0, sizeof(LxSession));

	/** Set up the session structure **/
	this->TokType = MLX_TOK_BEGIN;
	this->BufCnt = 0;
	this->BufPtr = this->Buffer;
	this->InpCnt = 0;
	this->InpPtr = this->InpBuf;
	this->ReservedWords = NULL;
	this->Flags = (flags & MLX_F_PUBLIC) & ~MLX_F_NODISCARD;
	/* determine which validate to set */
	char * locale = setlocale(LC_CTYPE, NULL);
	if(strstr(locale, "utf8") || strstr(locale, "UTF8") || strstr(locale, "utf-8") || strstr(locale, "UTF-8"))
	    {
	    this->ValidateFn = verifyUTF8; 
	    this->IsCharSplit = mlx_internal_WillSplitUTF8;
	    }
	    	else 
	    {
	    this->ValidateFn = NULL;
	    this->IsCharSplit = NULL;
	    }

	/** Preload the buffer with the first line **/
	this->Buffer[0] = 0;
	this->BufPtr = this->Buffer;
	this->LineNumber = 1;
	this->ReadArg = src;
	this->ReadFn = read_fn;
	this->BytesRead = 0;
	this->TokStartOffset = 0;
	this->Magic = MGK_LXSESSION;

    return this;
    }


/*** mlxCloseSession - end a session 
 ***/
int
mlxCloseSession(pLxSession this)
    {

    	ASSERTMAGIC(this,MGK_LXSESSION);

	/** Left the last \n in the buffer for line numbering purposes? **/
	if ((this->Flags & MLX_F_FOUNDEOL) && mlxPeekChar(this,0) == '\n')
	    {
	    this->Flags &= ~MLX_F_FOUNDEOL;
	    mlxSkipChars(this,1);
	    }

    	/** Need to do an 'unread' on the buffer? **/
	if (this->Flags & MLX_F_NODISCARD && this->InpCnt > 0)
	    {
	    /** Incomplete processing on a line that needs to be wrapped up? **/
	    if ((this->Flags & MLX_F_PROCLINE) && !(this->Flags & MLX_F_EOL) && mlxPeekChar(this,0) == '\n')
		mlxSkipChars(this,1);
	    fdUnRead((pFile)(this->ReadArg), this->InpPtr, this->InpCnt, 0,0);
	    }

	/** Free the structure **/
	nmFree(this, sizeof(LxSession));

    return 0;
    }


/*** mlx_internal_CheckBuffer() - load the input buffer if possible.
 *** returns n chars available, or -1 on error.  'offset' is the offset
 *** from the current position that we're interested in.
 ***/
int
mlx_internal_CheckBuffer(pLxSession s, int offset)
    {
    int newcnt;

	/** no data in input? **/
	if (__builtin_expect(s->InpCnt <= offset, 0))
	    {
	    if (!(s->Flags & MLX_F_NOFILE))
		{
		if (s->InpCnt)
		    {
		    /** shift existing bytes **/
		    memmove(s->InpBuf, s->InpPtr, s->InpCnt);
		    }

		/** read in new bytes **/
		s->InpPtr = s->InpBuf;
		if (!s->EndOfFile && !s->StreamErr)
		    {
		    newcnt = s->ReadFn(s->ReadArg, s->InpBuf + s->InpCnt, sizeof(s->InpBuf) - s->InpCnt, 0,0);
		    if (newcnt < 0)
			{
			s->StreamErr = 1;
			mssErrorErrno(1,"MLX","Could not read line %d from token stream", s->LineNumber);
			return -1; /* error */
			}
		    if (newcnt == 0)
			s->EndOfFile = 1;
		    else
			s->InpCnt += newcnt;
		    }
		else if (s->StreamErr)
		    return -1;
		}
	    }

    return s->InpCnt;
    }


/*** mlxUseOneChar() - 'use up' one char in the input buffer
 ***/
int
mlxUseOneChar(pLxSession s)
    {
	/** Keep track of what line we're on **/
	if (s->InpPtr[0] == '\n')
	    s->LineNumber++;

	/** Advance the counters **/
	s->InpCnt--;
	s->InpPtr++;
	s->BytesRead++;

    return 0;
    }


/*** mlxNextChar() - fetch the next character from the input data.
 *** Returns 0 - 255 on success (valid 8-bit char), -1 on EOF, and
 *** -2 on a read error from the source.
 ***/
int
mlxNextChar(pLxSession s)
    {
    int ch, v;

	v = mlx_internal_CheckBuffer(s, 0);
	if (v < 0) return MLX_ERROR;
	if (v == 0) return MLX_EOF;

	if (!(s->Flags & MLX_F_ALLOWNUL) && ((int)((unsigned char)(s->InpPtr[0]))) == 0)
	    {
	    mssError(1,"MLX","Invalid NUL character in input data stream");
	    return MLX_ERROR;
	    }

	/** input buffer has a char? **/
	ch = (int)((unsigned char)(s->InpPtr[0]));
	mlxUseOneChar(s);

    return ch;
    }


/*** mlxPeekChar() - return the current (or a future) character, without
 *** advancing the char ptr.  Returns -1 on eof, or -2 on error.  'offset'
 *** can be 0 for the current char, 1 for the one after that, etc.
 ***/
int
mlxPeekChar(pLxSession s, int offset)
    {
    int v, ch;
	v = mlx_internal_CheckBuffer(s, offset);
	if (__builtin_expect(v < 0, 0)) return MLX_ERROR;
	if (__builtin_expect(v <= offset, 0)) return MLX_EOF;

	ch = (int)((unsigned char)(s->InpPtr[offset]));

	if (__builtin_expect(!(s->Flags & MLX_F_ALLOWNUL) && ch == 0, 0))
	    {
	    mssError(1,"MLX","Invalid NUL character in input data stream");
	    return MLX_ERROR;
	    }

    return ch;
    }


/*** mlxSkipChars() - moves the char ptr forward by n characters, without
 *** actually returning a char.  Does *not* fill the buffer.  It is an error
 *** to call this when it is unknown whether the chars to be skipped exist
 *** in the buffer (i.e., with PeekChar)
 ***/
int
mlxSkipChars(pLxSession s, int n_chars)
    {
    int i;

	/** Clamp n_chars to what is in the buffer **/
	if (n_chars > s->InpCnt) n_chars = s->InpCnt;

	/** Update our counters **/
	for(i=0;i<n_chars;i++)
	    mlxUseOneChar(s);

    return n_chars;
    }


/*** mlxSkipWhitespace() - skip over any whitespace characters \r \t \n ' '
 ***/
int
mlxSkipWhitespace(pLxSession s)
    {
    int ch;

	while(1)
	    {
	    ch = mlxPeekChar(s, 0);
	    if (ch == ' ' || ch == '\t' || ch == '\r')
		mlxSkipChars(s, 1);
	    else
		break;
	    }

    return ch;
    }


/*** mlxSkipToEOL() - skip past the end of line, then return the current char.
 ***/
int
mlxSkipToEOL(pLxSession s)
    {
    int ch;

	while(1)
	    {
	    ch = mlxNextChar(s);
	    if (ch == '\n' || ch < 0)
		break;
	    }

    return mlxPeekChar(s, 0);
    }


/*** mlx_internal_Copy() - copy data from the input stream into a buffer,
 *** given proper semantics for the type of copy that is occurring based on
 *** the lexer flags and the token type.  The logic is slightly different
 *** for each type; at least we can consolidate this here and use it in
 *** mlxNextToken, mlxStringVal, and mlxCopyToken.
 ***/
int
mlx_internal_Copy(pLxSession this, char* buf, int bufsize, int* found_end)
    {
    int ch;
    int len = 0;

	/** Get the current character **/
	ch = mlxPeekChar(this, 0);

	/** LINEONLY mode 
	 ** A \n goes in the TokString, but we leave it in the input
	 ** as well.
	 **/
	if (this->Flags & MLX_F_LINEONLY)
	    {
	    *found_end = 0;
	    while(ch >= 0 && len < bufsize-1)
		{
		buf[len++] = ch;
		if (ch == '\n')
		    {
		    *found_end = 1;
		    break;
		    }
		else if(this->IsCharSplit && this->IsCharSplit(ch, len, bufsize-1))
		    {
		    *found_end = 0;
		    break;
		    }
		mlxSkipChars(this,1);
		ch = mlxPeekChar(this,0);
		}
	    }

	/** IFSONLY mode **/
	else if (this->Flags & MLX_F_IFSONLY)
	    {
	    *found_end = 1;
	    while(ch >= 0 && ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n')
		{
		if (len >= bufsize-1 || (this->IsCharSplit && this->IsCharSplit(ch, len, bufsize-1)))
		    {
		    *found_end = 0;
		    break;
		    }
		buf[len++] = ch;
		mlxSkipChars(this,1);
		ch = mlxPeekChar(this,0);
		}
	    }

	/** STRING mode **/
	else if (this->TokType == MLX_TOK_STRING)
	    {
	    *found_end = 1;
	    while((ch = mlxPeekChar(this,0)) >= 0 && ch != this->Delimiter)
		{
		if (len >= bufsize-1 || (this->IsCharSplit && this->IsCharSplit(ch, len, bufsize-1)))
		    {
		    *found_end = 0;
		    break;
		    }
		if (ch == '\\')
		    {
		    if (this->Flags & MLX_F_NOUNESC)
			{
			/** Not unescaping the string - keep the \ chars **/
			if (len >= bufsize-2)
			    {
			    /** copying 2 chars as an atomic unit - require 2 spaces in the buffer **/
			    *found_end = 0;
			    break;
			    }
			buf[len++] = ch;
			mlxSkipChars(this,1);
			ch = mlxPeekChar(this,0);
			}
		    else
			{
			/** Removing escapes - transform to control chars or strip the \ **/
			mlxSkipChars(this,1);
			ch = mlxPeekChar(this,0);
			if (ch == 'n') ch='\n'; 
			else if (ch == 't') ch='\t';
			else if (ch == 'r') ch='\r';
			}
		    }
		buf[len++] = ch;
		mlxSkipChars(this,1);
		}

	    /** skip delimiter if found ending of string **/
	    if (*found_end && ch == this->Delimiter)
		mlxSkipChars(this,1);
	    }

	/** FILENAME mode **/
	else if (this->TokType == MLX_TOK_FILENAME)
	    {
	    *found_end = 1;
	    while(ch >= 0 && ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n' && ch != ':')
		{
		if (len >= bufsize-1 || (this->IsCharSplit && this->IsCharSplit(ch, len, bufsize-1)))
		    {
		    *found_end = 0;
		    break;
		    }
		buf[len++] = ch;
		mlxSkipChars(this,1);
		ch = mlxPeekChar(this,0);
		}
	    }

	/** Nul-terminate the buffer **/
	buf[len] = '\0';

    return len;
    }


/*** Read a line, up to maxlen characters.
 ***/
int
mlxReadLine(pLxSession s, char* buf, int maxlen)
    {
    int bufpos = 0;
    int ch;

    	ASSERTMAGIC(s,MGK_LXSESSION);

	/** Keep piling data into buffer until we get a NL or buf full **/
	while(bufpos < maxlen-1)
	    {
	    ch = mlxNextChar(s);
	    if (ch == MLX_EOF)
		break;
	    if (ch == MLX_ERROR)
		return -1;
	    buf[bufpos++] = ch;
	    if (ch == '\n') /* eol */
		break;
	    }

	buf[bufpos] = 0;

    return bufpos;
    }


/*** mlxNextToken -- move to the next token in the input data
 *** stream.  The caller can get the value of the token by calling
 *** mlxStringVal or mlxIntVal.  This function returns the token type.
 ***/
int
mlxNextToken(pLxSession this)
    {
    int ch;
    int invert;
    int i,got_dot;
    int found_end;
    char* ptr;
    char* s_tokens = ":()/$*,#;";
    int s_tokid[] = {MLX_TOK_COLON, MLX_TOK_OPENPAREN, MLX_TOK_CLOSEPAREN, MLX_TOK_SLASH, MLX_TOK_DOLLAR, MLX_TOK_ASTERISK, MLX_TOK_COMMA, MLX_TOK_POUND, MLX_TOK_SEMICOLON };

    	ASSERTMAGIC(this,MGK_LXSESSION);

	/** Token on hold?  If so, return current one. **/
	if (this->Hold)
	    {
	    this->Hold = 0;
	    return this->TokType;
	    }

	/** Already in error condition?  Still in error condition. **/
	if (this->TokType == MLX_TOK_ERROR) 
	    {
	    mssError(0,"MLX","Attempt to keep reading past error");
	    return MLX_TOK_ERROR;
	    }

	/** Trying to read past EOF token?  Dumb user. **/
	if (this->TokType == MLX_TOK_EOF)
	    {
	    this->TokType = MLX_TOK_ERROR;
	    mssError(1,"MLX","Attempt to read past end of data");
	    return MLX_TOK_ERROR;
	    }

	/** Still in string and user didn't care?  Discard til delimiter. **/
	if (this->Flags & MLX_F_INSTRING && (this->Flags & MLX_F_LINEONLY))
	    {
	    while((ch = mlxNextChar(this)) >= 0 && ch != '\n') ;
	    this->Flags &= ~MLX_F_INSTRING;
	    }
	else if ((this->Flags & MLX_F_INSTRING) && ((this->Flags & MLX_F_IFSONLY) || this->Delimiter == ' '))
	    {
	    while((ch = mlxNextChar(this)) >= 0 && ch != '\n' && ch != '\t' && ch != '\r' && ch != ' ') ;
	    this->Flags &= ~MLX_F_INSTRING;
	    }
	else if (this->Flags & MLX_F_INSTRING)
	    {
	    while((ch = mlxNextChar(this)) != this->Delimiter)
		{
		if (ch == MLX_EOF || ch == MLX_ERROR)
		    {
		    mssError((ch == MLX_EOF)?1:0,"MLX","Unterminated string value");
		    this->TokType = MLX_TOK_ERROR;
		    return this->TokType;
		    }
		if (ch == '\\') mlxNextChar(this);
		}
	    this->Flags &= ~MLX_F_INSTRING;
	    }

	/** Loop until we get a token **/
	while(1)
	    {
	    this->TokString[0] = 0;
	    this->TokString[1] = 0;

	    /** Handled the EOL char?  If so, skip over it, thus letting the LineNumber advance. **/
	    if ((this->Flags & MLX_F_FOUNDEOL) && mlxPeekChar(this,0) == '\n')
		{
		this->Flags &= ~MLX_F_FOUNDEOL;
		mlxSkipChars(this,1);
		}

	    /** First, eliminate whitespace and comments. **/
	    if (!(this->Flags & MLX_F_LINEONLY))
	        {
		ch = mlxSkipWhitespace(this);

	        /** Single-line comment?  Skip EOL if so... **/
	        if ((ch == '#' && (this->Flags & MLX_F_POUNDCOMM)) ||
		    (ch == ';' && (this->Flags & MLX_F_SEMICOMM)) ||
		    (ch == '/' && mlxPeekChar(this,1) == '/' && (this->Flags & MLX_F_CPPCOMM)) ||
		    (ch == '-' && mlxPeekChar(this,1) == '-' && (this->Flags & MLX_F_DASHCOMM)))
		    {
		    mlxSkipToEOL(this);
		    continue;
		    }

		/** Beginning of a C-style comment?  Skip to comment end if so. **/
		if (ch == '/' && mlxPeekChar(this,1) == '*' && (this->Flags & MLX_F_CCOMM))
		    {
		    mlxSkipChars(this,2);
		    while(mlxPeekChar(this,0) != '*' && mlxPeekChar(this,1) != '/')
			mlxSkipChars(this,1);
		    mlxSkipChars(this,2);
		    continue;
		    }
		}
	    else
		{
		ch = mlxPeekChar(this,0);
		}

	    /** Mark the beginning of the token **/
	    this->TokStartOffset = this->BytesRead;

	    /** Error? **/
	    if (ch == MLX_ERROR)
		{
		mssError(0,"MLX","Unexpected end of data");
		this->TokType = MLX_TOK_ERROR;
		break;
		}

	    /** Handle end-of-line, either EOL or an EOF with no immediately preceding EOL. **/
	    if ((ch == '\n' || (ch == MLX_EOF && this->TokType != MLX_TOK_EOL)) && (this->Flags & MLX_F_PROCLINE))
		{
		if (ch == '\n')
		    this->Flags &= ~MLX_F_PROCLINE;

		/** Return the EOL to the caller? **/
		this->TokType = MLX_TOK_EOL;
		this->Flags |= MLX_F_FOUNDEOL;
		if (this->Flags & MLX_F_EOL)
		    break; /* return it to caller */
		else
		    continue; /* continue processing */
		}

	    /** End of file?  If so, return EOF or ERROR.  But, if there was no immediately
	     ** preceding EOL, give the line a chance to be processed first in LINEONLY mode
	     **/
	    if (ch == MLX_EOF && ((this->Flags & MLX_F_PROCLINE) || this->TokType == MLX_TOK_EOL))
		{
		if (this->Flags & MLX_F_EOF)
		    this->TokType = MLX_TOK_EOF;
		else
		    {
		    mssError(1,"MLX","Unexpected end of data");
		    this->TokType = MLX_TOK_ERROR; /* read past end of file */
		    }
		break; /* return to caller */
		}

	    /** Mark the line as having been processed, if not already.  That
	     ** way we make sure we return the line token (with LINEONLY) before
	     ** we return the EOL token.
	     **/
	    if (!(this->Flags & MLX_F_PROCLINE))
		{
		if ((this->Flags & MLX_F_LINEONLY))
		    {
		    this->TokType = MLX_TOK_STRING;
		    this->Delimiter = '\0';

		    /** Copy data up until \n or EOF or ERROR. **/
		    this->TokStrCnt = mlx_internal_Copy(this, this->TokString, sizeof(this->TokString), &found_end);

		    /** If we stopped because of a full strbuf, make a note of it **/
		    if (!found_end && mlxPeekChar(this,0) >= 0)
			this->Flags |= MLX_F_INSTRING;
		    this->Flags |= MLX_F_PROCLINE;
		    break;
		    }
		else
		    {
		    this->Flags |= MLX_F_PROCLINE;
		    continue;
		    }
		}

	    /** Ok, we got a token here.  What is it? **/
	    if (this->Flags & MLX_F_IFSONLY)
	        {
		this->TokType = MLX_TOK_STRING;

		/** Copy until eof/error, whitespace, or buffer full **/
		this->TokStrCnt = mlx_internal_Copy(this, this->TokString, sizeof(this->TokString), &found_end);

		/** If we stopped because of a full strbuf, make a note of it **/
		if (!found_end)
		    this->Flags |= MLX_F_INSTRING;
		break;
		}

	    /** Not LINEONLY, and not IFSONLY.  Check the actual data **/
	    if (ch == '\'' || ch == '"')
		{
		/** A quoted string **/
		this->TokType = MLX_TOK_STRING;
		this->Delimiter = ch;
		mlxSkipChars(this,1);

		/** Keep scanning until delimiter, unless delimiter preceeded by an escape **/
		this->TokStrCnt = mlx_internal_Copy(this, this->TokString, sizeof(this->TokString), &found_end);

		/** Hit EOF or ERROR?  Oops. **/
		if (!found_end && mlxPeekChar(this,0) < 0)
		    {
		    mssError(1,"MLX","Unterminated string value");
		    this->TokType = MLX_TOK_ERROR;
		    break;
		    }

		/** Found delimiter? **/
		if (!found_end)
		    this->Flags |= MLX_F_INSTRING;
		break;
		}
	    else if ((ch == '/' || (ch == '.' && mlxPeekChar(this,1) == '/')) && 
	    	     this->Flags & MLX_F_FILENAMES)
	        {
		/** Got a filename **/
		this->Delimiter = ' ';
		this->TokType = MLX_TOK_FILENAME;
		this->TokStrCnt = mlx_internal_Copy(this, this->TokString, sizeof(this->TokString), &found_end);
		if (!found_end)
		    this->Flags |= MLX_F_INSTRING;
		break;
		}
	    else if (ch == '{')
		{
		this->TokString[0] = '{';
		if (mlxPeekChar(this,1) == '{' && (this->Flags & MLX_F_DBLBRACE))
		    {
		    this->TokString[1] = '{';
		    this->TokStrCnt = 2;
		    this->TokType = MLX_TOK_DBLOPENBRACE;
		    }
		else
		    {
		    this->TokStrCnt = 1;
		    this->TokType = MLX_TOK_OPENBRACE;
		    }
		mlxSkipChars(this,this->TokStrCnt);
		this->TokString[this->TokStrCnt] = '\0';
		break;
		}
	    else if (ch == '}')
		{
		this->TokString[0] = '}';
		if (mlxPeekChar(this,1) == '}' && (this->Flags & MLX_F_DBLBRACE))
		    {
		    this->TokString[1] = '}';
		    this->TokStrCnt = 2;
		    this->TokType = MLX_TOK_DBLCLOSEBRACE;
		    }
		else
		    {
		    this->TokStrCnt = 1;
		    this->TokType = MLX_TOK_CLOSEBRACE;
		    }
		mlxSkipChars(this,this->TokStrCnt);
		this->TokString[this->TokStrCnt] = '\0';
		break;
		}
	    else if (ch >= 0 && ch <= 255 && (ptr = strchr(s_tokens, (char)((unsigned char)ch))) != NULL)
		{
		/** single-char token that isn't dependent on what the next character is **/
		this->TokString[0] = ch;
		this->TokStrCnt = 1;
		this->TokType = s_tokid[ptr - s_tokens];
		mlxSkipChars(this,1);
		break;
		}
	    else if (ch == '.' && (mlxPeekChar(this,1) < '0' || mlxPeekChar(this,1) > '9'))
	        {
		/** only a MLX_TOK_PERIOD if the next char is not a digit (that would make it part of a number) **/
		this->TokString[0] = ch;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_PERIOD;
		mlxSkipChars(this,1);
		break;
		}
	    else if (ch == '+' && (mlxPeekChar(this,1) < '0' || mlxPeekChar(this,1) > '9'))
	        {
		/** only a MLX_TOK_PLUS if the next char is not a digit (that would make it part of a number) **/
		this->TokString[0] = ch;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_PLUS;
		mlxSkipChars(this,1);
		break;
		}
	    else if (ch == '=' && mlxPeekChar(this,1) != '=')
		{
		/** only a MLX_TOK_EQUALS if the next char is not also an equals (that would be MLX_TOK_COMPARE) **/
		this->TokString[0] = ch;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_EQUALS;
		mlxSkipChars(this,1);
		break;
		}
	    else if (ch == '=' || ch == '<' || ch == '>' || 
		(ch == '!' && (mlxPeekChar(this,1) == '=')))
		{
		this->TokStrCnt = 0;
		this->TokType = MLX_TOK_COMPARE;
		this->TokInteger = 0;
		invert = (ch == '!');
		if (ch == '!')
		    {
		    this->TokString[this->TokStrCnt++] = ch;
		    mlxSkipChars(this,1);
		    ch = mlxPeekChar(this,0);
		    }
		while ((ch == '=' || ch == '<' || ch == '>') && this->TokStrCnt < 2)
		    {
		    this->TokString[this->TokStrCnt++] = ch;
		    switch(ch)
		        {
		        case '=': this->TokInteger |= MLX_CMP_EQUALS; break;
		        case '<': this->TokInteger |= MLX_CMP_LESS; break;
		        case '>': this->TokInteger |= MLX_CMP_GREATER; break;
			}
		    mlxSkipChars(this,1);
		    ch = mlxPeekChar(this,0);
		    }
		if (invert) this->TokInteger = 7 - this->TokInteger;
		this->TokString[this->TokStrCnt] = '\0';
		break;
		}
	    else if (ch == '-' && !strchr("0123456789.",mlxPeekChar(this,1))) 
	        {
		/** only a MLX_TOK_PLUS if the next char is not a digit or period (that would make it part of a number) **/
		this->TokString[0] = ch;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_MINUS;
		mlxSkipChars(this,1);
		break;
		}
	    else if (strchr("0123456789+-.",ch)) 
		{
		got_dot = (ch == '.')?1:0;
		this->TokType = MLX_TOK_INTEGER;
		this->TokStrCnt = 1;
		this->TokString[0] = ch;
		mlxSkipChars(this,1);
		found_end=1;
		while((ch = mlxPeekChar(this,0)) >= 0 && (strchr("0123456789xabcdefABCDEF",ch) && ch != '\0' || (!got_dot && ch == '.')))
		    {
		    if (this->TokStrCnt >= 255)
			{
			found_end = 0;
			break;
			}
		    if (ch == '.') got_dot = 1;
		    this->TokString[this->TokStrCnt++] = ch;
		    mlxSkipChars(this,1);
		    }
		if (got_dot) this->TokType = MLX_TOK_DOUBLE;
		if (!found_end) 
		    {
		    mssError(1,"MLX","Bark!  Number is way way too long -- more than 255 digits!");
		    this->TokType = MLX_TOK_ERROR;
		    }
		this->TokString[this->TokStrCnt] = '\0';
		errno = 0;
		this->TokInteger = strtoi(this->TokString, NULL, 0);
		if (errno == ERANGE && this->TokType == MLX_TOK_INTEGER)
		    {
		    mssError(1,"MLX","Number exceeds 32-bit integer range");
		    this->TokType = MLX_TOK_ERROR;
		    break;
		    }
		this->TokDouble = strtod(this->TokString, NULL);
		break;
		}
	    else if ((ch>='A' && ch<='Z') || (ch>='a' && ch<='z') || ch=='_')
		{
		this->TokType = MLX_TOK_KEYWORD;
		this->TokStrCnt = 0;
		found_end = 1;
		while(ch >= 0 && ((ch>='0' && ch<='9') || (ch>='A' && ch<='Z') ||
		    (ch>='a' && ch<='z') || ch=='_' || ch=='.' || (ch=='-' && (this->Flags & MLX_F_DASHKW))))
		    {
		    if (this->TokStrCnt >= 255)
			{
			found_end = 0;
			break;
			}
		    this->TokString[this->TokStrCnt++] = ch;
		    mlxSkipChars(this,1);
		    ch = mlxPeekChar(this,0);
		    }
		if (!found_end)
		    {
		    mssError(1,"MLX","Keyword/reserved word is too long - max length is 255 characters");
		    this->TokType = MLX_TOK_ERROR;
		    }
		this->TokString[this->TokStrCnt] = 0;
		break;
		}
	    else
		{
		mssError(1,"MLX","Unexpected character encountered");
		this->TokType = MLX_TOK_ERROR;
		break;
		}
	    }

	/** Differentiating string types? **/
	if (this->Flags & MLX_F_SSTRING && this->Delimiter == '\'' && this->TokType == MLX_TOK_STRING)
	    {
	    this->TokType = MLX_TOK_SSTRING;
	    }

	/** Need to check for a reserved word? **/
	if (this->TokType == MLX_TOK_KEYWORD && this->ReservedWords)
	    {
	    for(i=0;this->ReservedWords[i];i++)
	        {
		if (((this->Flags & MLX_F_ICASER) && !strcasecmp(this->TokString,this->ReservedWords[i])) ||
		    (!(this->Flags & MLX_F_ICASER) && !strcmp(this->TokString,this->ReservedWords[i])))
		    {
		    this->TokType = MLX_TOK_RESERVEDWD;
		    break;
		    }
		}
	    }

	/** Need to lowercase the string? **/
	if (((this->Flags & MLX_F_ICASEK) && this->TokType == MLX_TOK_KEYWORD) ||
	    ((this->Flags & MLX_F_ICASER) && this->TokType == MLX_TOK_RESERVEDWD))
	    {
	    ptr = this->TokString;
	    while(ptr < (this->TokString + this->TokStrCnt))
		{
		if (*ptr >= 'A' && *ptr <= 'Z')
		    (*ptr) += ('a' - 'A');
		ptr++;
		}
	    }

	/** check string is valid, if applicable **/
	if(this->ValidateFn != NULL && (this->TokType == MLX_TOK_SSTRING || 
	    this->TokType == MLX_TOK_STRING || this->TokType == MLX_TOK_FILENAME))
	    {
	    if(this->ValidateFn(this->TokString) != 0)
	        {
	        mssError(1,"MLX","String token contained invalid characters");
	        this->TokType = MLX_TOK_ERROR;
	        }
	    }

    return this->TokType;
    }


/*** mlxStringVal -- return the current string value of the token,
 *** up to 255 chars max.  If token is longer than this, this routine
 *** allocates memory for the token and returns alloc'd memory, in which
 *** case it sets the int* alloc flag to 1.  IF alloc is null, this routine
 *** will not alloc the memory, returning NULL instead.
 ***/
char*
mlxStringVal(pLxSession this, int* alloc)
    {
    char* ptr;
    int len,cnt,ccnt;
    char* nptr;

    	ASSERTMAGIC(this,MGK_LXSESSION);

	/** Error or non-string / non-keyword / non-int? **/
	if (this->TokType == MLX_TOK_ERROR) 
	    {
	    if (alloc) *alloc = 0;
	    mssError(0,"MLX","Attempt to obtain string value after error");
	    return NULL;
	    }

	/** String too long and no alloc? **/
	if (this->Flags & MLX_F_INSTRING && !alloc) 
	    {
	    mssError(1,"MLX","String too long for application's request");
	    return NULL;
	    }

	/** String didn't fit? **/
	if (this->Flags & MLX_F_INSTRING || (alloc && *alloc == 1))
	    {
	    if (!(this->Flags & MLX_F_INSTRING))
		len = this->TokStrCnt+1;
	    else
	        len = MLX_BLK_SIZ;
	    *alloc = 1;
	    ptr = (char*)nmSysMalloc(len);
	    if (!ptr)
		{
		this->TokType = MLX_TOK_ERROR;
		*alloc = 0;
		mssError(1,"MLX","Insufficient memory for string token");
		return NULL;
		}

	    /** Copy what we have, then try to copy more from the source **/
	    cnt = 0;
	    while (1)
		{
		/** Try copying a block of the data **/
		ccnt = mlxCopyToken(this, ptr+cnt, len-cnt);
		cnt += ccnt;
		if (ccnt == 0 || !(this->Flags & MLX_F_INSTRING)) break;

		/** Need more space.  Allocate it. **/
		len += MLX_BLK_SIZ;
		nptr = (char*)nmSysRealloc(ptr,len);
		if (!nptr)
		    {
		    this->TokType = MLX_TOK_ERROR;
		    *alloc = 0;
		    nmSysFree(ptr);
		    mssError(1,"MLX","Insufficient memory for string token");
		    return NULL;
		    }
		ptr = nptr;
		}
	    }
	else
	    {
	    /** No alloc requested/needed.  Just point to TokString **/
	    if (alloc) *alloc = 0;
	    ptr = this->TokString;
	    }

		/** validate again; could have more copied from buffer **/
		if(this->ValidateFn != NULL && this->ValidateFn(ptr) != 0)
		    {
		    mssError(1,"MLX","String token contained invalid characters");
		    this->TokType = MLX_TOK_ERROR;
		    *alloc = 0;
		    nmSysFree(ptr);
		    return NULL;
		    }

    return ptr;
    }


/*** mlxIntVal -- return the current integer value of the token, in a 32
 *** bit signed format.  If non-integer, returns 0.
 ***/
int
mlxIntVal(pLxSession this)
    {
	
    	ASSERTMAGIC(this,MGK_LXSESSION);

	/** Error or non-int? **/
	if (this->TokType != MLX_TOK_INTEGER && 
	    this->TokType != MLX_TOK_DOUBLE &&
	    this->TokType != MLX_TOK_COMPARE) return 0;

    return (this->TokType==MLX_TOK_DOUBLE)?((int)this->TokDouble):(this->TokInteger);
    }


/*** mlxDoubleVal - return the current double value of the token, in
 *** a double-precision floating point number.
 ***/
double
mlxDoubleVal(pLxSession this)
    {

    	ASSERTMAGIC(this,MGK_LXSESSION);

	/** Error or non-int? **/
	if (this->TokType != MLX_TOK_INTEGER && 
	    this->TokType != MLX_TOK_DOUBLE) return 0;

    return (this->TokType==MLX_TOK_DOUBLE)?(this->TokDouble):((double)(this->TokInteger));
    }


/*** mlxCopyToken -- instead of using internal storage or allocating
 *** memory like mlxStringVal does, this routine copies the current
 *** token to buffer with maxlen length.  Actual string length is
 *** up to maxlen-1; this routine puts the null terminator.
 ***/
int
mlxCopyToken(pLxSession this, char* buffer, int maxlen)
    {
    int cnt;
    int len;
    int found_end;

    	ASSERTMAGIC(this,MGK_LXSESSION);

	/** Copy from TokString first **/
	len = this->TokStrCnt;
	if (maxlen-1 < len)
	    len = maxlen-1;
	memcpy(buffer, this->TokString, len);
	if (len < this->TokStrCnt)
	    memmove(this->TokString, this->TokString + len, this->TokStrCnt - len + 1);
	this->TokStrCnt -= len;
	this->TokString[this->TokStrCnt] = '\0';
	buffer[len] = '\0';
	found_end = 1;

	/** If more data available, and there is room for it, copy it too **/
	if (this->Flags & MLX_F_INSTRING)
	    {
	    found_end = 0;
	    if (len < maxlen-1)
		{
		cnt = mlx_internal_Copy(this, buffer+len, maxlen - len, &found_end);
		len += cnt;
		buffer[len]=0;
		if (found_end)
		    this->Flags &= ~MLX_F_INSTRING;
		}
	    }

	/** Unterminated string value? **/
	if (!found_end && !(this->Flags & (MLX_F_IFSONLY | MLX_F_LINEONLY)) && this->TokType == MLX_TOK_STRING && mlxPeekChar(this,0) < 0)
	    {
	    mssError(1,"MLX","Unterminated string value");
	    this->TokType = MLX_TOK_ERROR;
	    }
	
	/** validate buffer; could have copied more from input, thereby avoiding next token's check **/
	if(this->ValidateFn && this->ValidateFn(buffer) != 0)
	    {
	    mssError(1,"MLX","Invalid characters in string");
	    this->TokType = MLX_TOK_ERROR;
	    }

    return len;
    }


/*** mlxHoldToken - place current token on hold so that next mlxNextToken
 *** call keeps the current token instead of fetching the next one.  This
 *** is useful if a subroutine finds it got one too many tokens from the
 *** input stream.   NOTE: this can't be called if user has already fetched
 *** the value using mlxStringVal().
 ***/
int
mlxHoldToken(pLxSession this)
    {

    	ASSERTMAGIC(this,MGK_LXSESSION);

	/** Note that token is held. **/
	this->Hold = 1;

    return 0;
    }


/*** mlxSetOptions - changes parsing options during the parsing of a stream
 *** or string.  Not all options necessarily support this, and behavior when
 *** changing MLX_F_EOF and MLX_F_EOL is undefined at present.
 ***/
int
mlxSetOptions(pLxSession this, int options)
    {

    	ASSERTMAGIC(this,MGK_LXSESSION);

	if ((options & MLX_F_LINEONLY) && !(this->Flags & MLX_F_LINEONLY))
	    this->Flags &= ~MLX_F_PROCLINE;

    	this->Flags |= (options & (MLX_F_ICASE | MLX_F_IFSONLY | MLX_F_LINEONLY | MLX_F_SYMBOLMODE));

    return 0;
    }


/*** mlxUnsetOptions - the complement to the above function.
 ***/
int
mlxUnsetOptions(pLxSession this, int options)
    {

    	ASSERTMAGIC(this,MGK_LXSESSION);

    	this->Flags &= ~(options & (MLX_F_ICASE | MLX_F_IFSONLY | MLX_F_LINEONLY | MLX_F_SYMBOLMODE));

    return 0;
    }


/*** mlxSetReservedWords - create a list of reserved words that should
 *** trigger a different token type than the keyword token type.
 ***/
int
mlxSetReservedWords(pLxSession this, char** res_words)
    {

    	ASSERTMAGIC(this,MGK_LXSESSION);
	
    	/** Set the words **/
	this->ReservedWords = res_words;

    return 0;
    }


/*** mlxNoteError - do an mssError error message to note an error in
 *** the output stream indicating approximately where the error occurred
 *** in processing.
 ***/
int
mlxNoteError(pLxSession this)
    {
    ASSERTMAGIC(this,MGK_LXSESSION);
    mssError(0,"MLX","Error near '%s'", this->TokString);
    return 0;
    }


/*** mlxNotePosition - do an mssError message to note the position in
 *** the input stream where the error occurred.
 ***/
int
mlxNotePosition(pLxSession this)
    {
    ASSERTMAGIC(this,MGK_LXSESSION);
    mssError(0,"MLX","Error at line %d", this->LineNumber);
    return 0;
    }


/*** mlxGetOffset() - figure out how many bytes into the file or object we have
 *** read so far (as of the *beginning* of the _current_token_ that was just
 *** returned by mlxNextToken).  The idea is, you can do an mlxSetOffset() to
 *** restart the parsing at exactly the most recent token returned.
 ***/
unsigned long
mlxGetOffset(pLxSession this)
    {

	ASSERTMAGIC(this,MGK_LXSESSION);

	/** Some error conditions that don't allow us to properly determine
	 ** the seek offset.
	 **/
	if (this->BufCnt < 0) return this->BytesRead;

    return this->TokStartOffset;
    }


/*** mlxGetCurOffset() - like the above, but returns the current offset, not
 *** the start offset of the current token.
 ***/
unsigned long
mlxGetCurOffset(pLxSession this)
    {

	ASSERTMAGIC(this,MGK_LXSESSION);

	/** Some error conditions that don't allow us to properly determine
	 ** the seek offset.
	 **/
	if (this->BufCnt <= 0) return this->BytesRead;

    return this->BytesRead;
    }


/*** mlxSetOffset() - allows us to set the offset to continue the tokenizing
 *** at.  This only works on pFiles and pObjects that support FD_U_SEEK (or,
 *** OBJ_U_SEEK which is the same flag for pObject's).  It also works on
 *** StringSession's (opened by mlxStringSession).
 ***
 *** WARNING: line numbering is RESET when an mlxSetOffset() is done.
 ***/
int
mlxSetOffset(pLxSession this, unsigned long new_offset)
    {
    char nullbuf[1];

	ASSERTMAGIC(this,MGK_LXSESSION);

	/** Ok, if this is a string session, just reset the bufptr. **/
	if (!this->ReadFn)
	    {
	    if (new_offset > strlen(this->InpStartPtr)) return -1;

	    /** Set up the session structure **/
	    this->TokType = MLX_TOK_BEGIN;
	    this->BufCnt = 0;
	    this->BufPtr = this->Buffer;
	    this->InpCnt = strlen(this->InpStartPtr);
	    this->InpPtr = this->InpStartPtr + new_offset;
	    this->Flags = this->Flags & MLX_F_PUBLIC;
	    this->Flags |= MLX_F_NOFILE;

	    /** Read the first line. **/
	    this->BufPtr = this->Buffer;
	    this->LineNumber = 1;
	    this->TokStartOffset = new_offset;
	    this->BytesRead = new_offset;
	    }
	else
	    {
	    /** Ok, either fd or generic session.  Seek and you shall find :)
	     ** Do an empty read to force the seek offset to what we need. 
	     ** We use FD_U_SEEK here, but OBJ_U_SEEK is the same thing.
	     **/
	    if (this->ReadFn(this->ReadArg, nullbuf, 0, new_offset, FD_U_SEEK) < 0) return -1;

	    /** Set up the session structure **/
	    this->TokType = MLX_TOK_BEGIN;
	    this->BufCnt = 0;
	    this->BufPtr = this->Buffer;
	    this->InpCnt = 0;
	    this->InpPtr = this->InpBuf;
	    this->Flags = this->Flags & MLX_F_PUBLIC;
	    this->Buffer[0] = 0;
	    this->LineNumber = 1;
	    this->BytesRead = new_offset;
	    this->TokStartOffset = new_offset;
	    }

    return 0;
    }


/*** mlx_internal_WillSplitChar() - Given the current character, index, and maximum
 *** length, returns 1 if a utf-8 character would be split, and 0 if the character fits,
 *** of -1 if the chracter does not fit at all (i.e. buffer is full)
 ***
 *** NOTE: in cases where there is room on the buffer, this returns T/F as the name indicates
 *** If the buffer may be fll, it behaves more like it should be named "isRoomOnBuff" (-1 --> true)
 ***/
int 
mlx_internal_WillSplitUTF8(char c, int ind, int max)
    {
    if((unsigned char) c >= (unsigned char) 0xF0 && ind > max-4) return 1; /* cuts 4 byte */
    else if ((unsigned char) c >= (unsigned char) 0xE0 && ind > max-3) return 1; /* cuts 3 byte */
    else if ((unsigned char) c >= (unsigned char) 0xC0 && ind > max-2) return 1; /* cuts 2 byte */
    else if (ind < max ) return 0; /* fits */
    else return -1; /* buffer is full */
    }