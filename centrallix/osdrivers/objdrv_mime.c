#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#endif
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"
#include "stparse.h"
#include "st_node.h"
#include "centrallix.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_mime.c						*/
/* Author:	Luke Ehresman <LME>					*/
/* Creation:	August 2, 2002						*/
/* Description:	MIME objectsystem driver.				*/
/*              Much of this drivers structure is based off of the      */
/*              MIME parser that Greg Beeley wrote as an extension to   */
/*              Elm in 1996.                                            */
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_mime.c,v 1.2 2002/08/09 02:38:49 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_mime.c,v $

    $Log: objdrv_mime.c,v $
    Revision 1.2  2002/08/09 02:38:49  jorupp
     * added basic attribute support to mime driver
     * set obj->SubCnt=1 in mimeOpen() <-- let the OSML the mime driver is processing one level of the path

    Revision 1.1  2002/08/09 01:55:05  lkehresman
    Removing the old "mailmsg" driver which wasn't working anyway, and added the
    shell for the mime driver.  It still needs a lot of work, but initial headers
    are being parsed.


 **END-CVSDATA***********************************************************/


/* ***********************************************************************
** DEFINITONS                                                           **
** **********************************************************************/

/*** GLOBALS ***/

/** information structure for MIME msg **/
typedef struct _MM
    {
    int		ContentMainType;
    char	ContentSubType[80];
    char	ContentDisp[80];
    char	ContentDispFilename[80];
    char	PartBoundary[80];
    char	PartName[80];
    char	PartSubject[80];
    char	Subject[80];
    char	Charset[32];
    char	TransferEncoding[32];
    char	MIMEVersion[16];
    DateTime	PartDate;
    unsigned long	HdrSeekStart;
    unsigned long	MsgSeekStart;
    unsigned long	MsgSeekEnd;
    XArray	Parts;
    }
    MimeMsg, *pMimeMsg;

/*** Structure used by this driver internally. ***/
typedef struct 
    {
    pObject	Obj;
    int		Mask;
    char	Pathname[256];
    char*	AttrValue; /* GetAttrValue has to return a refence to memory that won't be free()ed */
    pMimeMsg	Message;
    int		NextAttr;
    }
    MimeData, *pMimeData;

/*** Possible Main Content Types ***/
char *TypeStrings[7] =
    {
    "text",
    "multipart",
    "application",
    "message",
    "image",
    "audio",
    "video",
    };

#define MIME(x) ((pMimeData)(x))
#define MIME_DEBUG            (MIME_DBG_HDRPARSE | MIME_DBG_PARSER)
//#define MIME_DEBUG            (MIME_DBG_HDRPARSE | MIME_DBG_PARSER | MIME_DBG_FUNC1 | MIME_DBG_FUNC2 | MIME_DBG_FUNCEND)

//#define MIME_DEBUG 0x00000000

#define MIME_DBG_HDRPARSE    1
#define MIME_DBG_HDRREAD     2
#define MIME_DBG_PARSER      4
#define MIME_DBG_FUNC1       8
#define MIME_DBG_FUNC2      16
#define MIME_DBG_FUNC3      32
#define MIME_DBG_FUNCEND    64

/* ***********************************************************************
** INTERNAL FUNCTIONS                                                   **
** **********************************************************************/

/*  mime_internal_Cleanup
**
**  Deallocates all memory used for the mime message
*/
void*
mime_internal_Cleanup(pMimeData inf, char *str)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC2) fprintf(stderr, "MIME (3): mime_internal_Cleanup() called.\n");
    if (inf->Message)
	{
	nmFree(inf->Message, sizeof(MimeMsg));
	nmFree(inf, sizeof(MimeData));
	}
    if (str)
	{
	if (MIME_DEBUG) fprintf(stderr, "MIME: ERROR!! DANGER WILL ROBINSON!!  \"%s\"\n", str);
	mssError(0, "MIME", str);
	}
    if (MIME_DEBUG & (MIME_DBG_FUNC2 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (3): mime_internal_Cleanup() closing.\n");
    return NULL;
    }

/*  mime_internal_StrLTrim
**
**  Trims whitespace off the left side of a string.
*/
int
mime_internal_StrLTrim(char *str)
    {
    int i;

    for (i=0; i < strlen(str) &&
		(str[i] == '\r' ||
		 str[i] == '\n' ||
		 str[i] == '\t' ||
		 str[i] == ' '); i++);
    memmove(str, str+i, strlen(str)-i);
    str[strlen(str)-i] = '\0';

    return 0;
    }

/*  mime_internal_StrRTrim
**
**  Trims whitespace off the right side of a string.
*/
int
mime_internal_StrRTrim(char *str)
    {
    int i;

    for (i=strlen(str)-1; i >= 0 &&
		(str[i] == '\r' ||
		 str[i] == '\n' ||
		 str[i] == '\t' ||
		 str[i] == ' '); i--);
    str[i+1] = '\0';

    return 0;
    }

/*  mime_internal_StrTrim
**
**  Trims whitespace off both sides of a string.
*/
int
mime_internal_StrTrim(char *str)
    {
    mime_internal_StrLTrim(str);
    mime_internal_StrRTrim(str);

    return 0;
    }

/*  mime_internal_StrFirstCaseCmp
**
**  Checks if the first part of the given string matches the second string.
**  This function is case insensitive.
*/

int
mime_internal_StrFirstCaseCmp(char *s1, char *s2)
    {
    while (*s2)
	{
	if ((*s1 & 0xDF) >= 'A' && (*s1 & 0xDF) <= 'Z' &&
	    (*s2 & 0xDF) >= 'A' && (*s2 & 0xDF) <= 'Z')
	    {
	    if ((*s1 & 0xDF) != (*s2 & 0xDF))
		return ((*s1 & 0xDF) > (*s2 & 0xDF))*2-1;
	    }
	else
	    {
	    if (*s1 != *s2) return (*s1 > *s2)*2-1;
	    }
	s1++;
	s2++;
	}
    return 0;
    }

/*  mime_internal_Unquote
**
**  Internal function used to unquote strings if they are quoted.
*/

char*
mime_internal_Unquote(char *str)
    {
    /** Strip trailing spaces... **/
    while(strlen(str) && str[strlen(str)-1] == ' ')
	{
	str[strlen(str)-1] = 0;
	}

    /** If quoted, drop the quotes! **/
    if (*str == '"' && str[strlen(str)-1] == '"')
	{
	str[strlen(str)-1] = 0;
	return str+1;
	}
    if (*str == '\'' && str[strlen(str)-1] == '\'')
	{
	str[strlen(str)-1] = 0;
	return str+1;
	}
    return str;
    }

/*  mime_internal_LoadExtendedHeader
**
**  Header elements can span multiple lines.  We know that this occurs when there
**  is any white space at the beginning of the line.  This function will check if
**  there are any more lines that belong to the current header element.  If so, 
**  they will be read into xsbuf replacing all white spaces with just normal spaces.
*/

int
mime_internal_LoadExtendedHeader(pMimeData inf, pXString xsbuf, pLxSession lex)
    {
    int offset, alloc=1, toktype, i;
    char *ptr;

    if (MIME_DEBUG & MIME_DBG_FUNC2) fprintf(stderr, "MIME (2): mime_internal_LoadExtendedHeader() called.\n");
    while (1)
	{
	offset = mlxGetCurOffset(lex);
	toktype = mlxNextToken(lex);
	if (toktype == MLX_TOK_ERROR)
	    {
	    mlxCloseSession(lex);
	    return -1;
	    }
	ptr = mlxStringVal(lex, NULL);
	if (!strchr(" \t", ptr[0])) break;
	xsConcatPrintf(xsbuf, "%s", ptr);
	}
    /** Be kind, rewind! (resetting the offset because we don't use the last string it fetched) **/
    mlxSetOffset(lex, offset);
    /** Set all tabs, NL's, CR's to spaces **/
    for(i=0;i<strlen(xsbuf->String);i++) if (strchr("\t\r\n",xsbuf->String[i])) xsbuf->String[i]=' ';

    if (MIME_DEBUG & (MIME_DBG_FUNC2 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (2): mime_internal_LoadExtendedHeader() closing.\n");
    return 0;
    }

/*  mime_internal_SetMIMEVersion
**
**  Parses the "MIME-Version" header element and fills in the MimeMsg data structure
**  with the data accordingly.
*/
int
mime_internal_SetMIMEVersion(pMimeData inf, XString *xsbuf, pLxSession lex)
    {
    char *ptr;

    if (MIME_DEBUG & MIME_DBG_FUNC2) fprintf(stderr, "MIME (2): mime_internal_SetMIMEVersion() called.\n");
    if (!(ptr=strpbrk(xsbuf->String, ":"))) return 0;
    ptr++;
    mime_internal_StrTrim(ptr);
    strncpy(inf->Message->MIMEVersion, ptr, 15);
    inf->Message->MIMEVersion[15] = 0;

    if (MIME_DEBUG & MIME_DBG_HDRPARSE)
	{
	printf("MIME Parser (MIME-Version)\n");
	printf("  MIME-VERSION: \"%s\"\n", ptr);
	}
    if (MIME_DEBUG & (MIME_DBG_FUNC2 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (2): mime_internal_SetMIMEVersion() closing.\n");
    return 0;
    }

/*  mime_internal_SetDate
**
**  Parses the "Date" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
mime_internal_SetDate(pMimeData inf, XString *xsbuf, pLxSession lex)
    {
    XString xsptr;
    char *ptr, *cptr;
    int i;

    if (MIME_DEBUG & MIME_DBG_FUNC2) fprintf(stderr, "MIME (2): mime_internal_SetDate() called.\n");
    /** Get the date **/
    if (!(ptr=strpbrk(xsbuf->String, ":"))) return 0;
    ptr++;
    mime_internal_StrTrim(ptr);

    /** FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME:
     **   objDataToDateTime does not currently behave properly.  When that function
     **   gets fixed, fix this!!
     **/
    // objDataToDateTime(DATA_T_STRING, xsptr->String, &msg->PartDate, NULL);

    if (MIME_DEBUG & (MIME_DBG_FUNC2 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (2): mime_internal_SetDate() closing.\n");
    return 0;
    }

/*  mime_internal_SetSubject
**
**  Parses the "Subject" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
mime_internal_SetSubject(pMimeData inf, XString *xsbuf, pLxSession lex)
    {
    char *ptr;

    if (MIME_DEBUG & MIME_DBG_FUNC2) fprintf(stderr, "MIME (2): mime_internal_SetSubject() called.\n");
    /** Get the date **/
    if (!(ptr=strpbrk(xsbuf->String, ":"))) return 0;
    ptr++;
    mime_internal_StrTrim(ptr);
    strncpy(inf->Message->Subject, ptr, 79);
    inf->Message->Subject[79] = 0;

    if (MIME_DEBUG & MIME_DBG_HDRPARSE)
	{
	printf("MIME Parser (Subject)\n");
	printf("  SUBJECT     : \"%s\"\n", inf->Message->Subject);
	}

    if (MIME_DEBUG & (MIME_DBG_FUNC2 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (2): mime_internal_SetSubject() closing.\n");
    return 0;
    }

/*  mime_internal_SetFrom
**
**  Parses the "From" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
mime_internal_SetFrom(pMimeData inf, XString *xsbuf, pLxSession lex)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC2) fprintf(stderr, "MIME (2): mime_internal_SetFrom() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC2 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (2): mime_internal_SetFrom() closing.\n");
    return 0;
    }

/*  mime_internal_SetCc
**
**  Parses the "Cc" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
mime_internal_SetCc(pMimeData inf, XString *xsbuf, pLxSession lex)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC2) fprintf(stderr, "MIME (2): mime_internal_SetCc() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC2 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (2): mime_internal_SetCc() closing.\n");
    return 0;
    }

/*  mime_internal_SetTo
**
**  Parses the "To" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
mime_internal_SetTo(pMimeData inf, XString *xsbuf, pLxSession lex)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC2) fprintf(stderr, "MIME (2): mime_internal_SetTo() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC2 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (2): mime_internal_SetTo() closing.\n");
    return 0;
    }

/*  mime_internal_SetTransferEncoding
**
**  Parses the "Content-Transfer-Encoding" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
mime_internal_SetTransferEncoding(pMimeData inf, XString *xsbuf, pLxSession lex)
    {
    char *ptr, *cptr, *buf;

    if (MIME_DEBUG & MIME_DBG_FUNC2) fprintf(stderr, "MIME (2): mime_internal_SetTransferEncoding() called.\n");
    /** Make sure we have the entire content-transfer-encoding line (why would it span **/
    /** multiple lines??  I don't know but there are some crazy mailers out there) **/
    if (mime_internal_LoadExtendedHeader(inf, xsbuf, lex) < 0) return -1;
    buf = xsbuf->String; /** FIXME FIXME FIXME FIXME FIXME FIXME : i can't copy the string, so i pointed it **/

    if (!(ptr=strtok_r(buf, ": ", &buf))) return 0;
    if (!(ptr=strtok_r(buf, "; ", &buf))) return 0;
    strncpy(inf->Message->TransferEncoding, ptr, 31);
    inf->Message->TransferEncoding[31] = 0;

    if (MIME_DEBUG & MIME_DBG_HDRPARSE)
	{
	printf("MIME Parser (Content-Transfer-Encoding)\n");
	printf("  TRANSFER ENCODING: \"%s\"\n", inf->Message->TransferEncoding);
	}

    if (MIME_DEBUG & (MIME_DBG_FUNC2 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (2): mime_internal_SetTransferEncoding() closing.\n");
    return 0;
    }

/*  mime_internal_SetContentDisp
**
**  Parses the "Content-Disposition" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
mime_internal_SetContentDisp(pMimeData inf, XString *xsbuf, pLxSession lex)
    {
    char *ptr, *cptr, *buf;

    if (MIME_DEBUG & MIME_DBG_FUNC2) fprintf(stderr, "MIME (2): mime_internal_SetContentDisp() called.\n");
    /** Make sure we have the entire content-display line **/
    if (mime_internal_LoadExtendedHeader(inf, xsbuf, lex) < 0) return -1;
    buf = xsbuf->String; /** FIXME FIXME FIXME FIXME FIXME FIXME : i can't copy the string, so i pointed it **/

    /** get the display main type **/
    if (!(ptr=strtok_r(buf, ": ", &buf))) return 0;
    if (!(ptr=strtok_r(buf, "; ", &buf))) return 0;

    strncpy(inf->Message->ContentDisp, ptr, 79);
    inf->Message->ContentDisp[79] = 0;

    /** Check for the "filename=" content-disp token **/
    while ((ptr = strtok_r(buf, "= ", &buf)))
	{
	if (!(cptr = strtok_r(buf, ";", &buf))) break;
	while (*ptr == ' ') ptr++;
	if (!mime_internal_StrFirstCaseCmp(ptr, "filename"))
	    {
	    strncpy(inf->Message->ContentDispFilename, mime_internal_Unquote(cptr), 79);
	    inf->Message->ContentDispFilename[79] = 0;
	    }
	}

    if (MIME_DEBUG & MIME_DBG_HDRPARSE)
	{
	printf("MIME Parser (Content-Disposition)\n");
	printf("  CONTENT DISP: \"%s\"\n", inf->Message->ContentDisp);
	printf("  FILENAME    : \"%s\"\n", inf->Message->ContentDispFilename);
	}
    if (MIME_DEBUG & (MIME_DBG_FUNC2 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (2): mime_internal_SetContentDisp() closing.\n");

    return 0;
    }

/*  mime_internal_SetContentType
**
**  Parses the "Content-Type" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
mime_internal_SetContentType(pMimeData inf, XString *xsbuf, pLxSession lex)
    {
    char *ptr, *cptr, *buf;
    char maintype[32], tmpname[128];
    int len,i;

    if (MIME_DEBUG & MIME_DBG_FUNC2) fprintf(stderr, "MIME (2): mime_internal_SetContentType() called.\n");
    if (mime_internal_LoadExtendedHeader(inf, xsbuf, lex) < 0) return -1;
    buf = xsbuf->String; /** FIXME FIXME FIXME FIXME FIXME FIXME : i can't copy the string, so i pointed it **/

    /** Get the disp main type and subtype **/
    if (!(ptr=strtok_r(buf, ": ", &buf))) return 0;
    if (!(ptr=strtok_r(buf, "; ", &buf))) return 0;
    if ((cptr=strchr(ptr,'/')))
	{
	len = (int)cptr - (int)ptr;
	if (len>31) len=31;
	strncpy(maintype, ptr, len);
	maintype[len] = 0;
	strncpy(inf->Message->ContentSubType, cptr+1, 79);
	inf->Message->ContentSubType[79] = 0;
	}
    else
	{
	strncpy(maintype, ptr, 31);
	maintype[31] = 0;
	}
    for (i=0; i<7; i++)
	{
	if (!mime_internal_StrFirstCaseCmp(maintype, TypeStrings[i]))
	    {
	    inf->Message->ContentMainType = i;
	    }
	}
    
    /** Look at any possible parameters **/
    while ((ptr = strtok_r(buf, "= ", &buf)))
	{
	if (!(cptr=strtok_r(buf, ";", &buf))) break;
	while (*ptr == ' ') ptr++;
	if (!mime_internal_StrFirstCaseCmp(ptr, "boundary"))
	    {
	    strncpy(inf->Message->PartBoundary, mime_internal_Unquote(cptr), 79);
	    inf->Message->PartBoundary[79] = 0;
	    }
	else if (!mime_internal_StrFirstCaseCmp(ptr, "name"))
	    {
	    strncpy(tmpname, mime_internal_Unquote(cptr), 127);
	    tmpname[127] = 0;
	    if (strchr(tmpname,'/'))
		strncpy(inf->Message->PartName, strrchr(tmpname,'/')+1,79);
	    else 
		strncpy(inf->Message->PartName, tmpname, 79);
	    inf->Message->PartName[79] = 0;
	    if (strchr(inf->Message->PartName,'\\'))
		strncpy(inf->Message->PartName,strrchr(tmpname,'\\')+1,79);
	    inf->Message->PartName[79] = 0;
	    }
	else if (!mime_internal_StrFirstCaseCmp(ptr, "subject"))
	    {
	    strncpy(inf->Message->PartSubject, mime_internal_Unquote(cptr), 79);
	    inf->Message->PartSubject[79] = 0;
	    }
	else if (!mime_internal_StrFirstCaseCmp(ptr, "charset"))
	    {
	    strncpy(inf->Message->Charset, mime_internal_Unquote(cptr), 31);
	    inf->Message->Charset[31] = 0;
	    }
	}

    if (MIME_DEBUG & MIME_DBG_HDRPARSE)
	{
	printf("MIME Parser (Content-Type)\n");
	printf("  TYPE:       : \"%s\"\n", TypeStrings[inf->Message->ContentMainType]);
	printf("  SUBTYPE     : \"%s\"\n", inf->Message->ContentSubType);
	printf("  BOUNDARY    : \"%s\"\n", inf->Message->PartBoundary);
	printf("  NAME        : \"%s\"\n", inf->Message->PartName);
	printf("  SUBJECT     : \"%s\"\n", inf->Message->PartSubject);
	printf("  CHARSET     : \"%s\"\n", inf->Message->Charset);
	}

    if (MIME_DEBUG & (MIME_DBG_FUNC2 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (2): mime_internal_SetContentType() closing.\n");
    return 0;
    }

/*  mime_internal_ParseMessage
**
**  Parses a message (located at obj->Prev) starting at the "start" byte, and ending
**  at the "end" byte.  This creates the MimeMsg data structure and recursively
**  calls itself to fill it in.  Note that no data is actually stored.  This is just
**  the shell of the message and contains seek points denoting where to start
**  and end reading.
*/
int
mime_internal_ParseMessage(pObject obj, pMimeData inf, int start, int end, int nohdr)
    {
    pLxSession lex;
    int flag, toktype, alloc, err;
    XString xsbuf;

    if (MIME_DEBUG & MIME_DBG_FUNC2) fprintf(stderr, "MIME (2): mime_internal_ParseMessage() called.\n");
    /** Initialize the message structure **/
    inf->Message->ContentDisp[0] = 0;
    inf->Message->ContentDispFilename[0] = 0;
    inf->Message->ContentMainType = 0;
    inf->Message->ContentSubType[0] = 0;
    inf->Message->PartBoundary[0] = 0;
    inf->Message->PartName[0] = 0;
    inf->Message->PartSubject[0] = 0;
    inf->Message->Charset[0] = 0;
    inf->Message->TransferEncoding[0] = 0;
    inf->Message->MIMEVersion[0] = 0;
    inf->Message->HdrSeekStart = 0;
    inf->Message->MsgSeekStart = 0;
    inf->Message->MsgSeekEnd = 0;

    lex = mlxGenericSession(obj->Prev, objRead, MLX_F_LINEONLY|MLX_F_NODISCARD);
    if (!lex)
	{
	mime_internal_Cleanup(inf, "Unable to open lexer session");
	return -1;
	}

    flag = 1;
    while (flag)
	{
	mlxSetOptions(lex, MLX_F_LINEONLY|MLX_F_NODISCARD);
	toktype = mlxNextToken(lex);
	if (toktype == MLX_TOK_ERROR)
	    {
	    mlxCloseSession(lex);
	    mime_internal_Cleanup(inf, "Error in the Lexer");
	    return -1;
	    }
	/* get the next line */
	alloc = 0;
	xsInit(&xsbuf);
	xsCopy(&xsbuf, mlxStringVal(lex, &alloc), -1);
	xsRTrim(&xsbuf);
	if (MIME_DEBUG & MIME_DBG_HDRREAD) printf("MIME: Got Token (%s)\n", xsbuf.String);
	/* check if this is the end of the headers, if so, exit the loop (flag=0), */
	/* otherwise parse the header elements */
	if (!strlen(xsbuf.String))
	    {
	    flag = 0;
	    }
	else
	    {
	    if (!mime_internal_StrFirstCaseCmp(xsbuf.String, "Content-Disposition"))
		{
		err = mime_internal_SetContentDisp(inf, &xsbuf, lex);
		}
	    else if (!mime_internal_StrFirstCaseCmp(xsbuf.String, "Content-Type"))
		{
		err = mime_internal_SetContentType(inf, &xsbuf, lex);
		}
	    else if (!mime_internal_StrFirstCaseCmp(xsbuf.String, "Content-Transfer-Encoding"))
		{
		err = mime_internal_SetTransferEncoding(inf, &xsbuf, lex);
		}
	    else if (!mime_internal_StrFirstCaseCmp(xsbuf.String, "MIME-Version"))
		{
		err = mime_internal_SetMIMEVersion(inf, &xsbuf, lex);
		}
	    else if (!mime_internal_StrFirstCaseCmp(xsbuf.String, "To"))
		{
		err = mime_internal_SetTo(inf, &xsbuf, lex);
		}
	    else if (!mime_internal_StrFirstCaseCmp(xsbuf.String, "Cc"))
		{
		err = mime_internal_SetCc(inf, &xsbuf, lex);
		}
	    else if (!mime_internal_StrFirstCaseCmp(xsbuf.String, "From"))
		{
		err = mime_internal_SetFrom(inf, &xsbuf, lex);
		}
	    else if (!mime_internal_StrFirstCaseCmp(xsbuf.String, "Subject"))
		{
		err = mime_internal_SetSubject(inf, &xsbuf, lex);
		}
	    else if (!mime_internal_StrFirstCaseCmp(xsbuf.String, "Date"))
		{
		err = mime_internal_SetDate(inf, &xsbuf, lex);
		}

	    if (err < 0)
		{
		mime_internal_Cleanup(inf, "Error in the Lexer");
		return -1;
		}
	    }
	}
    xsDeInit(&xsbuf);
    mlxCloseSession(lex);
    inf->Message->HdrSeekStart = start;
//    msg->MsgSeekStart = mlxGetOffset(lex);

    if (MIME_DEBUG & (MIME_DBG_FUNC2 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (2): mime_internal_ParseMessage() closing.\n");
    return 0;
    }


/* ***********************************************************************
** API FUNCTIONS                                                        **
** **********************************************************************/

/*
**  mimeOpen
*/
void*
mimeOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pMimeData inf;
    pMimeMsg msg;

    if (MIME_DEBUG) fprintf(stderr, "\n");
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeOpen() called.\n");
    if (MIME_DEBUG & MIME_DBG_PARSER) fprintf(stderr, "MIME: mimeOpen called with \"%s\" content type.  Parsing as such.\n", systype->Name);

    if(MIME_DEBUG) printf("objdrv_mime.c was offered: (%i,%i,%i) %s\n",obj->SubPtr,
	    obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));
    /*
    **  Handle the content-type: message/rfc822
    */
    if (!strcasecmp(systype->Name, "message/rfc822"))
	{

	/** Allocate and initialize the MIME structure **/
	inf = (pMimeData)nmMalloc(sizeof(MimeData));
	msg = (pMimeMsg)nmMalloc(sizeof(MimeMsg));
	if (!inf) return NULL;
	memset(inf,0,sizeof(MimeData));

	/** Set object parameters **/
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));
	inf->Message = msg;
	inf->Obj = obj;
	inf->Mask = mask;
	if (mime_internal_ParseMessage(obj, inf, 0, 0, 0) < 0)
	    {
	    if (MIME_DEBUG) fprintf(stderr, "MIME: There was an error somewhere so I'm returning NULL from mimeOpen().\n");
	    return NULL;
	    }

	if (strstr(inf->Message->MIMEVersion, "1.0"))
	    {
	    if (MIME_DEBUG & (MIME_DBG_PARSER)) fprintf(stderr, "MIME: We have a MIME Message, version 1.0\n");
	    }
	}

    /*
    **  Handle MIME parsing of the content
    */
    else if (!strcasecmp(systype->Name, "multipart/mixed") ||
             !strcasecmp(systype->Name, "multipart/alternative"))
	{
	}

    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeOpen() closing.\n");
    if (MIME_DEBUG) fprintf(stderr, "inf = %08x\n",(int)inf);

    /** assume we're only going to handle one level **/
    obj->SubCnt=1;

    if(MIME_DEBUG) printf("objdrv_mime.c is taking: (%i,%i,%i) %s\n",obj->SubPtr,
	    obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));

    return (void*)inf;
    }


/*
**  mimeClose
*/
int
mimeClose(void* inf_v, pObjTrxTree* oxt)
    {
    pMimeData inf = MIME(inf_v);

    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeClose(%08x) called.\n",inf);
    /** free any memory used to return an attribute **/
    if(inf->AttrValue)
	{
	free(inf->AttrValue);
	inf->AttrValue=NULL;
	}
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeClose() closing.\n");
    return 0;
    }


/*
**  mimeCreate
*/
int
mimeCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeCreate() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeCreate() closing.\n");
    return 0;
    }


/*
**  mimeDelete
*/
int
mimeDelete(pObject obj, pObjTrxTree* oxt)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeDelete() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeDelete() closing.\n");
    return 0;
    }


/*
**  mimeRead
*/
int
mimeRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeRead() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeRead() closing.\n");
    return 0;
    }


/*
**  mimeWrite
*/
int
mimeWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeWrite() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeWrite() closing.\n");
    return 0;
    }


/*
**  mimeOpenQuery
*/
void*
mimeOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeOpenQuery() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeOpenQuery() closing.\n");
    return 0;
    }


/*
**  mimeQueryFetch
*/
void*
mimeQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeQueryFetch() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeQueryFetch() closing.\n");
    return 0;
    }


/*
**  mimeQueryClose
*/
int
mimeQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeQueryClose() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeQueryClose() closing.\n");
    return 0;
    }


/*
**  mimeGetAttrType
*/
int
mimeGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeGetAttrType(\"%s\") called.\n",attrname);

    if (!strcmp(attrname, "name")) return DATA_T_STRING;
    if (!strcmp(attrname, "content_type")) return DATA_T_STRING;
    if (!strcmp(attrname, "annotation")) return DATA_T_STRING;
    if (!strcmp(attrname, "inner_type")) return DATA_T_STRING;
    if (!strcmp(attrname, "outer_type")) return DATA_T_STRING;
    if (!strcmp(attrname, "subject")) return DATA_T_STRING;
    if (!strcmp(attrname, "charset")) return DATA_T_STRING;
    if (!strcmp(attrname, "transfer_encoding")) return DATA_T_STRING;
    if (!strcmp(attrname, "mime_version")) return DATA_T_STRING;

    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeGetAttrType() closing.\n");
    return -1;
    }


/*
**  mimeGetAttrValue
*/
int
mimeGetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree* oxt)
    {
    pMimeData inf = MIME(inf_v);
    char tmp[32];

    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeGetAttrValue(\"%s\") called.\n",attrname);
    if (inf->AttrValue)
	{
	free(inf->AttrValue);
	inf->AttrValue = NULL;
	}
    if (!strcmp(attrname, "inner_type"))
	{
	return mimeGetAttrValue(inf_v, "content_type", val, oxt);
	}
    if (!strcmp(attrname, "outer_type"))
	{
	*((char**)val) = "text/plain";
	return 0;
	}
    if (!strcmp(attrname, "content_type"))
	{
	/** malloc an arbitrary value -- we won't know the real value until the snprintf **/
	inf->AttrValue = (char*)malloc(128);
	snprintf(inf->AttrValue, 128, "%s/%s", TypeStrings[inf->Message->ContentMainType], inf->Message->ContentSubType);
	*((char**)val) = inf->AttrValue;
	return 0;
	}
    if (!strcmp(attrname, "subject"))
	{
	*((char**)val) = inf->Message->Subject;
	return 0;
	}
    if (!strcmp(attrname, "charset"))
	{
	*((char**)val) = inf->Message->Charset;
	return 0;
	}
    if (!strcmp(attrname, "transfer_encoding"))
	{
	*((char**)val) = inf->Message->TransferEncoding;
	return 0;
	}
    if (!strcmp(attrname, "mime_version"))
	{
	*((char**)val) = inf->Message->MIMEVersion;
	return 0;
	}
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeGetAttrValue() closing.\n");

    return -1;
    }


/*
**  mimeGetNextAttr
*/
char*
mimeGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pMimeData inf = MIME(inf_v);
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeGetNextAttr() called.\n");
    switch (inf->NextAttr++)
	{
	case 0: return "content_type";
	case 1: return "subject";
	case 2: return "charset";
	case 3: return "transfer_encoding";
	case 4: return "mime_version";
	}
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeGetNextAttr() closing.\n");
    return NULL;
    }


/*
**  mimeGetFirstAttr
*/
char*
mimeGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pMimeData inf = MIME(inf_v);
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeGetFirstAttr() called.\n");
    inf->NextAttr=0;
    return mimeGetNextAttr(inf,oxt);
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeGetFirstAttr() closing.\n");
    return NULL;
    }


/*
**  mimeSetAttrValue
*/
int
mimeSetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree oxt)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeSetAttrValue() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeSetAttrValue() closing.\n");
    return 0;
    }


/*
**  mimeAddAttr
*/
int
mimeAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeAddAttr() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeAddAttr() closing.\n");
    return -1;
    }


/*
**  mimeOpenAttr
*/
void*
mimeOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeOpenAttr() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeOpenAttr() closing.\n");
    return NULL;
    }


/*
**  mimeGetFirstMethod
*/
char*
mimeGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeGetFirstMethod() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeGetFirstMethod() closing.\n");
    return NULL;
    }


/*
**  mimeGetNextMethod
*/
char*
mimeGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeGetNextMethod() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeGetNextMethod() closing.\n");
    return NULL;
    }


/*
**  mimeExecuteMethod
*/
int
mimeExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree oxt)
    {
    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeExecuteMethod() called.\n");
    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeExecuteMethod() closing.\n");
    return -1;
    }


/*
**  mimeInitialize
*/
int
mimeInitialize()
    {
    pObjDriver drv;

    if (MIME_DEBUG & MIME_DBG_FUNC1) fprintf(stderr, "MIME (1): mimeInitialize() called.\n");
    drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
    if (!drv) return -1;
    memset(drv, 0, sizeof(ObjDriver));

    /** Setup the function references. **/
    drv->Open = mimeOpen;
    drv->Close = mimeClose;
    drv->Create = mimeCreate;
    drv->Delete = mimeDelete;
    drv->OpenQuery = mimeOpenQuery;
    drv->QueryDelete = NULL;
    drv->QueryFetch = mimeQueryFetch;
    drv->QueryClose = mimeQueryClose;
    drv->Read = mimeRead;
    drv->Write = mimeWrite;
    drv->GetAttrType = mimeGetAttrType;
    drv->GetAttrValue = mimeGetAttrValue;
    drv->GetFirstAttr = mimeGetFirstAttr;
    drv->GetNextAttr = mimeGetNextAttr;
    drv->SetAttrValue = mimeSetAttrValue;
    drv->AddAttr = mimeAddAttr;
    drv->OpenAttr = mimeOpenAttr;
    drv->GetFirstMethod = mimeGetFirstMethod;
    drv->GetNextMethod = mimeGetNextMethod;
    drv->ExecuteMethod = mimeExecuteMethod;

    strcpy(drv->Name, "MIME - MIME Parsing Driver");
    drv->Capabilities = 0;
    xaInit(&(drv->RootContentTypes), 16);
    xaAddItem(&(drv->RootContentTypes), "message/rfc822");

    if (objRegisterDriver(drv) < 0) return -1;

    if (MIME_DEBUG & (MIME_DBG_FUNC1 | MIME_DBG_FUNCEND)) fprintf(stderr, "MIME (1): mimeInitialize() closing.\n");
    return 0;
    }

MODULE_INIT(mimeInitialize);
MODULE_PREFIX("mime");
MODULE_DESC("MIME ObjectSystem Driver");
MODULE_VERSION(0,1,0);
MODULE_IFACE(CX_CURRENT_IFACE);
