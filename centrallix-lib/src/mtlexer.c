#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "newmalloc.h"
#include "mtask.h"
#include "mtlexer.h"
#include "mtsession.h"
#include "magic.h"

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

/**CVSDATA***************************************************************

    $Id: mtlexer.c,v 1.11 2009/06/19 21:29:44 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/src/mtlexer.c,v $

    $Log: mtlexer.c,v $
    Revision 1.11  2009/06/19 21:29:44  gbeeley
    - (bugfix) Permit lines returned from mlxNextToken() to span blocks of
      data that were read in using the ReadFn().
    - BUGBUG - The mtlexer needs an overhaul ASAP.  There are parts of this
      code that have problems.

    Revision 1.10  2007/12/05 22:10:15  gbeeley
    - (bugfix) fixing problem regarding quoting of chars in a string under
      some circumstances

    Revision 1.9  2007/10/29 20:42:43  gbeeley
    - (bugfix) new changes to mtlexer had some problems dealing with special
      and/or escaped characters.

    Revision 1.8  2007/09/21 23:13:03  gbeeley
    - (bugfix) handle NL's inside a quoted string when not using a user-
      managed buffer.

    Revision 1.7  2004/07/22 00:20:52  mmcgill
    Added a magic number define for WgtrNode, and added xaInsertBefore and
    xaInsertAfter functions to the XArray module.

    Revision 1.6  2003/04/03 04:32:39  gbeeley
    Added new cxsec module which implements some optional-use security
    hardening measures designed to protect data structures and stack
    return addresses.  Updated build process to have hardening and
    optimization options.  Fixed some build-related dependency checking
    problems.  Updated mtask to put some variables in registers even
    when not optimizing with -O.  Added some security hardening features
    to xstring as an example.

    Revision 1.5  2002/08/05 20:54:29  gbeeley
    This fix should allow multiple mlxCopyToken()s if the string size is
    larger than the buffer, to allow incremental retrievals of parts of the
    string.

    Revision 1.4  2002/08/05 19:51:23  gbeeley
    Adding only "mildly tested" support for getting/setting the seek offset
    while in a lexer session.  The lexer does blocked/buffered I/O, so it
    is otherwise difficult to know 'where' in the document one is at.  Note
    that the offsets returned from mlxGetOffset and mlxGetCurOffset are
    relative to the *start* of the lexer processing.  If data was read from
    the file/object *before* processing with the lexer, that data is *not*
    included in the seek counts/offsets.

    Revision 1.3  2002/06/20 15:57:05  gbeeley
    Fixed some compiler warnings.  Repaired improper buffer handling in
    mtlexer's mlxReadLine() function.

    Revision 1.2  2001/09/28 20:00:21  gbeeley
    Modified magic number system syntax slightly to eliminate semicolon
    from within the macro expansions of the ASSERT macros.

    Revision 1.1.1.1  2001/08/13 18:04:20  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:02:52  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


int mlxReadLine(pLxSession s, char* buf, int maxlen);


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
	this->Flags = flags & (MLX_F_ICASE | MLX_F_POUNDCOMM | 
		MLX_F_SEMICOMM | MLX_F_CPPCOMM | MLX_F_DASHCOMM |
		MLX_F_EOL | MLX_F_EOF | MLX_F_IFSONLY | MLX_F_DASHKW | 
		MLX_F_NODISCARD | MLX_F_FILENAMES | MLX_F_DBLBRACE | 
		MLX_F_LINEONLY | MLX_F_SYMBOLMODE | MLX_F_CCOMM |
		MLX_F_TOKCOMM | MLX_F_NOUNESC | MLX_F_SSTRING);

	/** Preload the buffer with the first line **/
        /*this->BufCnt = mlxReadLine(this, this->Buffer, 2047);
	if (this->BufCnt <= 0)
	    {
	    this->TokType = MLX_TOK_ERROR;
	    }*/
	this->Flags |= MLX_F_FOUNDEOL;
	this->Buffer[0] = 0;
	this->BufPtr = this->Buffer;
	this->LineNumber = 0;
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
	this->Flags = flags & (MLX_F_ICASE | MLX_F_POUNDCOMM | 
		MLX_F_SEMICOMM | MLX_F_CPPCOMM | MLX_F_DASHCOMM |
		MLX_F_EOL | MLX_F_EOF | MLX_F_IFSONLY | MLX_F_DASHKW |
		MLX_F_FILENAMES | MLX_F_DBLBRACE | MLX_F_SYMBOLMODE | 
		MLX_F_CCOMM | MLX_F_TOKCOMM | MLX_F_NOUNESC |
		MLX_F_SSTRING);
	this->Flags |= MLX_F_NOFILE;
	this->Magic = MGK_LXSESSION;

	/** Read the first line. **/
        this->BufCnt = mlxReadLine(this, this->Buffer, 2047);
	if (this->BufCnt == 0)
	    {
	    if (!(this->Flags & MLX_F_EOF))
		{
		this->TokType = MLX_TOK_EOF;
		}
	    }
	else if (this->BufCnt < 0)
	    {
	    this->TokType = MLX_TOK_ERROR;
	    }
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
	this->Flags = flags & (MLX_F_ICASE | MLX_F_POUNDCOMM | 
		MLX_F_SEMICOMM | MLX_F_CPPCOMM | MLX_F_DASHCOMM |
		MLX_F_EOL | MLX_F_EOF | MLX_F_IFSONLY | MLX_F_DASHKW | 
		MLX_F_FILENAMES | MLX_F_DBLBRACE | MLX_F_LINEONLY |
		MLX_F_SYMBOLMODE | MLX_F_CCOMM | MLX_F_TOKCOMM |
		MLX_F_NOUNESC | MLX_F_SSTRING);

	/** Preload the buffer with the first line **/
	this->Flags |= MLX_F_FOUNDEOL;
	this->Buffer[0] = 0;
	this->BufPtr = this->Buffer;
	this->LineNumber = 0;
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

    	/** Need to do an 'unread' on the buffer? **/
	if (this->Flags & MLX_F_NODISCARD && this->InpCnt > 0)
	    {
	    fdUnRead((pFile)(this->ReadArg), this->InpPtr, this->InpCnt, 0,0);
	    }

	/** Free the structure **/
	nmFree(this, sizeof(LxSession));

    return 0;
    }


int
mlxReadLine(pLxSession s, char* buf, int maxlen)
    {
    int n;
    char* ptr;
    int got_newline=0;
    int bufpos = 0;

    	ASSERTMAGIC(s,MGK_LXSESSION);

	/** Keep piling data into buffer until we get a NL or buf full **/
	while(!got_newline && bufpos < maxlen-1)
	    {
	    /** Have some data already? **/
	    if (s->InpCnt > 0)
		{
	        /** NL already in buffer? **/
	        ptr = memchr(s->InpPtr,'\n',s->InpCnt);
		if (ptr)
		    {
		    got_newline = 1;
	    	    n = ((int)ptr - (int)s->InpPtr)+1;
		    }
		else
		    {
		    n = s->InpCnt;
		    }
	    	if (n>maxlen-1-bufpos) n=maxlen-1-bufpos;
	    	memcpy(buf+bufpos, s->InpPtr, n);

		/** Add data retrieved so far to the BytesRead value for
		 ** offset tracking.
		 **/
		s->BytesRead += n;
		
		bufpos += n;
		s->InpCnt -= n;
		s->InpPtr += n;
		continue;
		}

	    /** Ok, get more data. **/
	    if (s->Flags & MLX_F_NOFILE)
		{
		s->InpCnt = 0;
		s->InpBuf[0] = 0;
		break;
		}
	    s->InpPtr = s->InpBuf;
	    s->InpCnt = s->ReadFn(s->ReadArg, s->InpBuf, 2048, 0,0);
	    if (s->InpCnt <= 0) 
		{
		if (s->InpCnt < 0) mssErrorErrno(1,"MLX","Could not read line %d from token stream", s->LineNumber);
		s->InpCnt = 0;
		s->InpBuf[0] = 0;
		break;
		}
	    s->InpBuf[s->InpCnt] = 0;
	    }

	buf[bufpos] = 0;
	s->LineNumber++;

    return bufpos;
    }


/*** mlxNextToken -- move to the next token in the input data
 *** stream.  The caller can get the value of the token by calling
 *** mlxStringVal or mlxIntVal.  This function returns the token type.
 ***/
int
mlxNextToken(pLxSession this)
    {
    char* ptr;
    char ch, prev_ch;
    int invert;
    int i,got_dot;
    int found_end;

    	ASSERTMAGIC(this,MGK_LXSESSION);

	/** Token on hold?  If so, return current one. **/
	if (this->Hold)
	    {
	    this->Hold = 0;
	    return this->TokType;
	    }

	ptr = this->BufPtr;

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
	    mssError(1,"MLX","Attempt to read past end-of-token-stream");
	    return MLX_TOK_ERROR;
	    }

	/** Still in string and user didn't care?  Discard til delimiter. **/
	if (this->Flags & MLX_F_INSTRING && (this->Flags & MLX_F_LINEONLY))
	    {
	    while(*ptr) ptr++;
	    this->Flags &= ~MLX_F_INSTRING;
	    }
	else if ((this->Flags & MLX_F_INSTRING) && ((this->Flags & MLX_F_IFSONLY) || this->Delimiter == ' '))
	    {
	    while(*ptr && *ptr != '\t' && *ptr != '\r' && *ptr != ' ' &&
	    	  *ptr != '\n') ptr++;
	    this->Flags &= ~MLX_F_INSTRING;
	    }
	else if (this->Flags & MLX_F_INSTRING)
	    {
	    while(*ptr != this->Delimiter)
		{
		if (*ptr == '\0')
		    {
		    this->BufCnt = mlxReadLine(this,this->Buffer, 2047);
		    if (this->BufCnt <= 0)
			{
			mssError(0,"MLX","Unterminated string value");
			this->TokType = MLX_TOK_ERROR;
			return this->TokType;
			}
		    this->BufPtr = this->Buffer;
		    ptr = this->Buffer;
		    continue;
		    }
		if (*ptr == '\\') ptr++;
		ptr++;
		}
	    this->Flags &= ~MLX_F_INSTRING;
	    }


	/** Loop until we get a token **/
	while(1)
	    {
	    /** Inside a C-style comment? **/
	    if (this->Flags & MLX_F_INCOMM)
	        {
		while(*ptr && (*ptr != '*' || *(ptr+1) != '/')) ptr++;
		if (*ptr)
		    {
		    ptr += 2;
		    this->Flags &= ~MLX_F_INCOMM;
		    }
		}

	    /** First, eliminate whitespace. **/
	    this->TokString[0] = 0;
	    this->TokString[1] = 0;
	    if (!(this->Flags & MLX_F_LINEONLY))
	        {
	        while(*ptr == ' ' || *ptr == '\r' || *ptr == '\n' || *ptr == '\t')
	            ptr++;

	        /** Single-line comment?  Skip EOL if so... **/
	        if ((*ptr == '#' && (this->Flags & MLX_F_POUNDCOMM)) ||
		    (*ptr == ';' && (this->Flags & MLX_F_SEMICOMM)) ||
		    (*ptr == '/' && *(ptr+1) == '/' && (this->Flags & MLX_F_CPPCOMM)) ||
		    (*ptr == '-' && *(ptr+1) == '-' && (this->Flags & MLX_F_DASHCOMM)))
		    {
		    ptr += strlen(ptr);
		    }

		/** Beginning of a C-style comment?  Skip to comment end if so. **/
		if (*ptr == '/' && *(ptr+1) == '*' && (this->Flags & MLX_F_CCOMM))
		    {
		    ptr += 2;
		    while(*ptr && (*ptr != '*' || *(ptr+1) != '/')) ptr++;

		    /** End of comment or end of line? **/
		    if (!*ptr)
		        {
			this->Flags |= MLX_F_INCOMM;
			}
		    else
		        {
			ptr += 2;
			}
		    }
		}

	    /** At end of buffer?  Read another line. **/
	    this->TokStartOffset = this->BytesRead - ((this->Buffer + this->BufCnt) - ptr);
	    if (!*ptr)
	        {
		if ((this->Flags & MLX_F_FOUNDEOL) || !(this->Flags & MLX_F_EOL))
		    {
		    this->Flags &= ~MLX_F_FOUNDEOL;
	            this->BufCnt = mlxReadLine(this, this->Buffer, 2047);
		    if (this->BufCnt <= 0)
		        {
		        if (this->Flags & MLX_F_EOF)
			    {
			    this->TokType = MLX_TOK_EOF;
			    }
		        else
			    {
			    mssError(1,"MLX","Unexpected end-of-token-stream");
			    this->TokType = MLX_TOK_ERROR;
			    }
		        break;
		        }
		    this->BufPtr = this->Buffer;
		    ptr = this->Buffer;
		    }
		else
		    {
		    this->TokType = MLX_TOK_EOL;
		    this->Flags |= MLX_F_FOUNDEOL;
		    break;
		    }
		continue;
	        }

	    /** Ok, not an EOL/EOF token; reset tokstart. **/
	    this->TokStartOffset = this->BytesRead - ((this->Buffer + this->BufCnt) - ptr);

	    /** Ok, we're at something.  What is it? **/
	    if (this->Flags & MLX_F_LINEONLY)
	        {
		this->TokStrCnt = 0;
		found_end = 0;
		while(1)
		    {
		    if (!*ptr)
			{
			if (found_end) break;
			this->BufCnt = mlxReadLine(this, this->Buffer, 2047);
			this->BufPtr = this->Buffer;
			ptr = this->Buffer;
			if (this->BufCnt <= 0 || !*ptr)
			    {
			    mssError(1,"MLX","Unterminated line at end of data");
			    this->TokType = MLX_TOK_ERROR;
			    break;
			    }
			}
		    if (*ptr == '\n') found_end = 1;
		    this->TokString[this->TokStrCnt++] = *(ptr++);
		    if (this->TokStrCnt >= 255)
		        {
			this->Flags |= MLX_F_INSTRING;
			break;
			}
		    }
		if (this->TokType == MLX_TOK_ERROR) break;
		this->TokString[this->TokStrCnt] = 0;
		this->TokType = MLX_TOK_STRING;
		this->Delimiter = '\0';
		break;
		}
	    else if (this->Flags & MLX_F_IFSONLY)
	        {
		this->TokStrCnt = 0;
		while(*ptr != ' ' && *ptr != '\t' && *ptr != '\r' &&
		      *ptr != '\n')
		    {
		    if (!*ptr)
			{
			this->BufCnt = mlxReadLine(this, this->Buffer, 2047);
			this->BufPtr = this->Buffer;
			ptr = this->Buffer;
			if (this->BufCnt < 0)
			    {
			    this->TokType = MLX_TOK_ERROR;
			    break;
			    }
			if (this->BufCnt == 0 || !*ptr)
			    break;
			}
		    this->TokString[this->TokStrCnt++] = *(ptr++);
		    if (this->TokStrCnt >= 255)
		        {
			this->Flags |= MLX_F_INSTRING;
			break;
			}
		    }
		if (this->TokType == MLX_TOK_ERROR) break;
		this->TokString[this->TokStrCnt] = 0;
		this->TokType = MLX_TOK_STRING;
		break;
		}
	    else if (*ptr == '\'' || *ptr == '"')
		{
		this->Delimiter = *ptr;
		this->Flags |= MLX_F_INSTRING;
		ptr++;
		this->TokStrCnt = 0;
		this->TokType = MLX_TOK_STRING;

		/** Keep scanning until delimiter, unless delimiter preceeded by an escape **/
		prev_ch = '\0';
		while(prev_ch == '\\' || *ptr != this->Delimiter)
		    {
		    ch=*ptr;

		    /** Load in more data? **/
		    if (ch == '\0')
			{
			this->BufCnt = mlxReadLine(this,this->Buffer, 2047);
			if (this->BufCnt <= 0)
			    {
			    mssError(1,"MLX","Unterminated string value");
			    this->TokType = MLX_TOK_ERROR;
			    break;
			    }
			this->BufPtr = this->Buffer;
			ptr = this->Buffer;
			continue;
			}

		    /** Allow escape of delimiter. **/
		    if (prev_ch == '\\' && ch && !(this->Flags & MLX_F_NOUNESC))
			{
			if (ch == 'n') this->TokString[this->TokStrCnt-1]='\n'; 
			else if (ch == 't') this->TokString[this->TokStrCnt-1]='\t';
			else if (ch == 'r') this->TokString[this->TokStrCnt-1]='\r';
			else this->TokString[this->TokStrCnt-1]=ch;
			ptr++;
			prev_ch = '\0';
			continue;
			}

		    /** Verify no overrun **/
		    if (this->TokStrCnt >= 256) 
			break;

		    /** Copy the char. **/
		    this->TokString[this->TokStrCnt++] = ch;
		    ptr++;
		    prev_ch = ch;
		    }
		
		/** Found delimiter? **/
		if (*ptr == this->Delimiter)
		    {
		    ptr++;
		    this->Flags &= ~MLX_F_INSTRING;
		    this->TokString[this->TokStrCnt] = 0;
		    }
		break;
		}
	    else if ((*ptr == '/' || (ptr[0] == '.' && ptr[1] == '/')) && 
	    	     this->Flags & MLX_F_FILENAMES)
	        {
		this->TokStrCnt = 0;
		this->Delimiter = ' ';
		this->TokType = MLX_TOK_FILENAME;
		while(*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\r' &&
		      *ptr != '\n' && *ptr != ':')
		    {
		    this->TokString[this->TokStrCnt++] = *(ptr++);
		    if (this->TokStrCnt >= 255)
		        {
			this->Flags |= MLX_F_INSTRING;
			break;
			}
		    }
		this->TokString[this->TokStrCnt] = 0;
		break;
		}
	    else if (*ptr == '{')
		{
		if (ptr[1] == '{' && (this->Flags & MLX_F_DBLBRACE))
		    {
		    this->TokString[0] = '{';
		    this->TokString[1] = '{';
		    this->TokStrCnt = 2;
		    this->TokType = MLX_TOK_DBLOPENBRACE;
		    ptr+=2;
		    }
		else
		    {
		    this->TokString[0] = *ptr;
		    this->TokStrCnt = 1;
		    this->TokType = MLX_TOK_OPENBRACE;
		    ptr++;
		    }
		break;
		}
	    else if (*ptr == '}')
		{
		if (ptr[1] == '}' && (this->Flags & MLX_F_DBLBRACE))
		    {
		    this->TokString[0] = '}';
		    this->TokString[1] = '}';
		    this->TokStrCnt = 2;
		    this->TokType = MLX_TOK_DBLCLOSEBRACE;
		    ptr+=2;
		    }
		else
		    {
		    this->TokString[0] = *ptr;
		    this->TokStrCnt = 1;
		    this->TokType = MLX_TOK_CLOSEBRACE;
		    ptr++;
		    }
		break;
		}
	    else if (*ptr == ':')
		{
		this->TokString[0] = *ptr;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_COLON;
		ptr++;
		break;
		}
	    else if (*ptr == '(')
		{
		this->TokString[0] = *ptr;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_OPENPAREN;
		ptr++;
		break;
		}
	    else if (*ptr == ')')
		{
		this->TokString[0] = *ptr;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_CLOSEPAREN;
		ptr++;
		break;
		}
	    else if (*ptr == '/')
	        {
		this->TokString[0] = *ptr;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_SLASH;
		ptr++;
		break;
		}
	    else if (*ptr == '$')
	        {
		this->TokString[0] = *ptr;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_DOLLAR;
		ptr++;
		break;
		}
	    else if (*ptr == '.' && (ptr[1] < '0' || ptr[1] > '9'))
	        {
		this->TokString[0] = *ptr;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_PERIOD;
		ptr++;
		break;
		}
	    else if (*ptr == '+' && (ptr[1] < '0' || ptr[1] > '9'))
	        {
		this->TokString[0] = *ptr;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_PLUS;
		ptr++;
		break;
		}
	    else if (*ptr == '*')
	        {
		this->TokString[0] = *ptr;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_ASTERISK;
		ptr++;
		break;
		}
	    else if (*ptr == '=' && ptr[1] != '=')
		{
		this->TokString[0] = *ptr;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_EQUALS;
		ptr++;
		break;
		}
	    else if (*ptr == ',')
		{
		this->TokString[0] = *ptr;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_COMMA;
		ptr++;
		break;
		}
	    else if (*ptr == ';')
		{
		this->TokString[0] = *ptr;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_SEMICOLON;
		ptr++;
		break;
		}
	    else if (*ptr == '=' || *ptr == '<' || *ptr == '>' || 
		(*ptr == '!' && (ptr[1] == '=')))
		{
		this->TokStrCnt = 0;
		this->TokType = MLX_TOK_COMPARE;
		this->TokInteger = 0;
		invert = (*ptr == '!');
		if (*ptr == '!') this->TokString[this->TokStrCnt++] = *(ptr++);
		while(*ptr == '=' || *ptr == '<' || *ptr == '>')
		    {
		    this->TokString[this->TokStrCnt++] = *ptr;
		    switch(*ptr)
		        {
		        case '=': this->TokInteger |= MLX_CMP_EQUALS; break;
		        case '<': this->TokInteger |= MLX_CMP_LESS; break;
		        case '>': this->TokInteger |= MLX_CMP_GREATER; break;
			}
		    ptr++;
		    }
		if (invert) this->TokInteger = 7 - this->TokInteger;
		this->TokString[this->TokStrCnt] = 0;
		break;
		}
	    else if (*ptr == '-' && !strchr("0123456789.",ptr[1]))
	        {
		this->TokString[0] = *ptr;
		this->TokStrCnt = 1;
		this->TokType = MLX_TOK_MINUS;
		ptr++;
		break;
		}
	    else if (strchr("0123456789+-.",*ptr))
		{
		got_dot = (*ptr == '.')?1:0;
		this->TokType = MLX_TOK_INTEGER;
		this->TokStrCnt = 1;
		this->TokString[0] = *(ptr++);
		while(*ptr && (strchr("0123456789xabcdefABCDEF",*ptr) || (!got_dot && *ptr == '.')) && this->TokStrCnt<254)
		    {
		    if (*ptr == '.') got_dot = 1;
		    this->TokString[this->TokStrCnt++] = *(ptr++);
		    }
		if (got_dot) this->TokType = MLX_TOK_DOUBLE;
		if (this->TokStrCnt >= 254) 
		    {
		    mssError(1,"MLX","Bark!  Number is way way too long -- more than 255 digits!");
		    this->TokType = MLX_TOK_ERROR;
		    }
		this->TokString[this->TokStrCnt] = 0;
		this->TokInteger = strtol(this->TokString, NULL, 0);
		this->TokDouble = strtod(this->TokString, NULL);
		break;
		}
	    else if ((*ptr>='A' && *ptr<='Z') || (*ptr>='a' && *ptr<='z') || *ptr=='_')
		{
		this->TokType = MLX_TOK_KEYWORD;
		this->TokStrCnt = 0;
		while(*ptr && ((*ptr>='0' && *ptr<='9') || (*ptr>='A' && *ptr<='Z') ||
		    (*ptr>='a' && *ptr<='z') || *ptr=='_' || *ptr=='.' || (*ptr=='-' && (this->Flags & MLX_F_DASHKW))) && this->TokStrCnt<254)
		    {
		    this->TokString[this->TokStrCnt++] = *(ptr++);
		    }
		if (this->TokStrCnt >= 254) 
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

	this->BufPtr = ptr;

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
		if (!strcasecmp(this->TokString,this->ReservedWords[i]))
		    {
		    this->TokType = MLX_TOK_RESERVEDWD;
		    break;
		    }
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
    int len,cnt;
    char* bptr;
    char* nptr;

    	ASSERTMAGIC(this,MGK_LXSESSION);

	/** Error or non-string / non-keyword / non-int? **/
	if (this->TokType == MLX_TOK_ERROR) 
	    {
	    if (alloc) *alloc = 0;
	    mssError(0,"MLX","Attempt to obtain string value after error");
	    return NULL;
	    }
	/*if (this->TokType != MLX_TOK_STRING && 
	    this->TokType != MLX_TOK_INTEGER &&
	    this->TokType != MLX_TOK_KEYWORD &&
	    this->TokType != MLX_TOK_RESERVEDWD &&
	    this->TokType != MLX_TOK_FILENAME)
	    {
	    if (alloc) *alloc = 0;
	    return NULL;
	    }*/

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
	    cnt = 0;
	    memcpy(ptr, this->TokString, this->TokStrCnt);
	    cnt += this->TokStrCnt;
	    bptr = this->BufPtr;
	    while(this->Flags & MLX_F_INSTRING)
		{
		if (this->Flags & MLX_F_LINEONLY)
		    {
		    if (*bptr == '\0') break;
		    }
		else if (this->Flags & MLX_F_IFSONLY)
		    {
		    if (*bptr == ' ' || *bptr == '\t' || *bptr == '\r' || *bptr == '\n') break;
		    }
		else
		    {
		    if (*bptr == this->Delimiter) break;
		    }
		if ((*bptr == ' ' || *bptr == ':' || *bptr == '\t' || *bptr == '\r' || *bptr == '\n') &&
		    this->TokType == MLX_TOK_FILENAME) break;
		if (*bptr == '\0')
		    {
		    this->BufCnt = mlxReadLine(this,this->Buffer, 2047);
		    if (this->BufCnt <= 0)
			{
			mssError(1,"MLX","Unterminated string value");
			this->TokType = MLX_TOK_ERROR;
			*alloc = 0;
			nmSysFree(ptr);
			return NULL;
			}
		    this->BufPtr = this->Buffer;
		    bptr = this->Buffer;
		    continue;
		    }
		if (*bptr == '\\') 
		    {
		    bptr++;
		    if (*bptr == 'n' || *bptr == 'r' || *bptr == 't')
			{
			if (*bptr == 'n') ptr[cnt] = '\n';
			if (*bptr == 'r') ptr[cnt] = '\r';
			if (*bptr == 't') ptr[cnt] = '\t';
			bptr++;
			}
		    else ptr[cnt] = *(bptr++);
		    }
		else
		    {
		    ptr[cnt] = *(bptr++);
		    }
		cnt++;
		if (cnt >= len-1)
		    {
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
	    if (this->Flags & MLX_F_INSTRING && this->Delimiter != '\0') bptr++; /* skip delimiter */
	    this->BufPtr = bptr;
	    ptr[cnt] = 0;
	    this->Flags &= ~MLX_F_INSTRING;
	    }
	else
	    {
	    if (alloc) *alloc = 0;
	    ptr = this->TokString;
	    }

	/** Need to lowercase the string? **/
	if ((ptr && (this->Flags & MLX_F_ICASEK) && this->TokType == MLX_TOK_KEYWORD) ||
	    (ptr && (this->Flags & MLX_F_ICASER) && this->TokType == MLX_TOK_RESERVEDWD))
	    {
	    nptr = ptr;
	    while(*nptr)
		{
		if (*nptr >= 'A' && *nptr <= 'Z') (*nptr) += ('a' - 'A');
		nptr++;
		}
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
    char* bptr;
    int cnt;
    int len;
    char ch;
    char* sptr;

    	ASSERTMAGIC(this,MGK_LXSESSION);

	/** Maybe buffer smaller than tok string? **/
	if (maxlen-1 < this->TokStrCnt)
	    {
	    if ((this->TokType == MLX_TOK_RESERVEDWD && (this->Flags & MLX_F_ICASER)) ||
	        (this->TokType == MLX_TOK_KEYWORD && (this->Flags & MLX_F_ICASEK)))
		{
		sptr = this->TokString;
		bptr = buffer;
		while(*sptr)
		    {
		    if (*sptr >= 'A' && *sptr <= 'Z')
		        *(bptr++) = *(sptr++) + ('a' - 'A');
		    else
		        *(bptr++) = *(sptr++);
		    }
		}
	    else
	        {
	        memcpy(buffer, this->TokString, maxlen-1);
		}
	    memmove(this->TokString, this->TokString+maxlen-1, 
		this->TokStrCnt - (maxlen-1) + 1);
	    this->TokStrCnt -= (maxlen-1);
	    buffer[maxlen-1] = 0;
	    return maxlen-1;
	    }

	cnt = 0;
	if ((this->TokType == MLX_TOK_RESERVEDWD && (this->Flags & MLX_F_ICASER)) ||
	    (this->TokType == MLX_TOK_KEYWORD && (this->Flags & MLX_F_ICASEK)))
	    {
	    sptr = this->TokString;
	    bptr = buffer;
	    while(*sptr)
		{
		if (*sptr >= 'A' && *sptr <= 'Z') 
		    *(bptr++) = *(sptr++) + ('a' - 'A');
		else
		    *(bptr++) = *(sptr++);
		}
	    }
	else
	    {
	    memcpy(buffer, this->TokString, this->TokStrCnt);
	    }
	len = this->TokStrCnt;
	if (!(this->Flags & MLX_F_INSTRING)) 
	    {
	    buffer[len] = 0;
	    return len;
	    }

	bptr = this->BufPtr;
	while(*bptr != this->Delimiter && cnt < maxlen-1)
	    {
	    if (*bptr == '\0')
		{
		this->BufCnt = mlxReadLine(this,this->Buffer, 2047);
		if (this->BufCnt <= 0)
		    {
		    mssError(1,"MLX","Unterminated string value");
		    this->TokType = MLX_TOK_ERROR;
		    return -1;
		    }
		this->BufPtr = this->Buffer;
		bptr = this->Buffer;
		continue;
		}
	    ch = *bptr;
	    if (*bptr == '\\') 
		{
		bptr++;
		if (*bptr == 'n') ch='\n';
		if (*bptr == 't') ch='\t';
		if (*bptr == 'r') ch='\r';
		else ch = *bptr;
		}
	    buffer[cnt] = ch;
	    bptr++;
	    cnt++;
	    }
	buffer[cnt]=0;
	this->Flags &= ~MLX_F_INSTRING;
	this->BufPtr = bptr;
    
    return cnt;
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

    return this->BytesRead - ((this->Buffer + this->BufCnt) - this->BufPtr);
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
	    if (new_offset < 0 || new_offset > strlen(this->InpStartPtr)) return -1;

	    /** Set up the session structure **/
	    this->TokType = MLX_TOK_BEGIN;
	    this->BufCnt = 0;
	    this->BufPtr = this->Buffer;
	    this->InpCnt = strlen(this->InpStartPtr);
	    this->InpPtr = this->InpStartPtr + new_offset;
	    this->Flags = this->Flags & (MLX_F_ICASE | MLX_F_POUNDCOMM | 
		    MLX_F_SEMICOMM | MLX_F_CPPCOMM | MLX_F_DASHCOMM |
		    MLX_F_EOL | MLX_F_EOF | MLX_F_IFSONLY | MLX_F_DASHKW |
		    MLX_F_FILENAMES | MLX_F_DBLBRACE | MLX_F_SYMBOLMODE | 
		    MLX_F_CCOMM | MLX_F_TOKCOMM | MLX_F_NOUNESC |
		    MLX_F_SSTRING);
	    this->Flags |= MLX_F_NOFILE;

	    /** Read the first line. **/
	    this->BufCnt = mlxReadLine(this, this->Buffer, 2047);
	    if (this->BufCnt <= 0)
		{
		this->TokType = MLX_TOK_ERROR;
		}
	    this->BufPtr = this->Buffer;
	    this->LineNumber = 1;
	    this->TokStartOffset = new_offset;
	    this->BytesRead = new_offset;
	    }
	else
	    {
	    /** Ok, either fd or generic session.  Seek and you shall find :) **/
	    if (new_offset < 0) return -1;

	    /** Do an empty read to force the seek offset to what we need. 
	     ** We use FD_U_SEEK here, but OBJ_U_SEEK is the same thing.
	     **/
	    if (this->ReadFn(this->ReadArg, nullbuf, 0, new_offset, FD_U_SEEK) < 0) return -1;

	    /** Set up the session structure **/
	    this->TokType = MLX_TOK_BEGIN;
	    this->BufCnt = 0;
	    this->BufPtr = this->Buffer;
	    this->InpCnt = 0;
	    this->InpPtr = this->InpBuf;
	    this->Flags = this->Flags & (MLX_F_ICASE | MLX_F_POUNDCOMM | 
		    MLX_F_SEMICOMM | MLX_F_CPPCOMM | MLX_F_DASHCOMM |
		    MLX_F_EOL | MLX_F_EOF | MLX_F_IFSONLY | MLX_F_DASHKW |
		    MLX_F_FILENAMES | MLX_F_DBLBRACE | MLX_F_SYMBOLMODE | 
		    MLX_F_CCOMM | MLX_F_TOKCOMM | MLX_F_NOUNESC |
		    MLX_F_SSTRING);
	    this->Flags |= MLX_F_FOUNDEOL;
	    this->Buffer[0] = 0;
	    this->LineNumber = 0;
	    this->BytesRead = new_offset;
	    this->TokStartOffset = new_offset;
	    }

    return 0;
    }

