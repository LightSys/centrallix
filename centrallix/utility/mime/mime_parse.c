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
/* Module: 	libmime							*/
/* Author:	Luke Ehresman (LME)					*/
/* Creation:	August 12, 2002						*/
/* Description:	Provides MIME parsing facilities used mainly in the	*/
/*		MIME object system driver (objdrv_mime.c)		*/
/************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "mtask.h"
#include "mtsession.h"
#include "obj.h"
#include "mime.h"

char* TypeStrings[] =
    {
    "text",
    "multipart",
    "application",
    "message",
    "image",
    "audio",
    "video"
    };

/*  libmime_ParseMessage
**
**  Parses a message (located at obj->Prev) starting at the "start" byte, and ending
**  at the "end" byte.  This creates the MimeMsg data structure and recursively
**  calls itself to fill it in.  Note that no data is actually stored.  This is just
**  the shell of the message and contains seek points denoting where to start
**  and end reading.
*/
int
libmime_ParseMessage(pObject obj, pMimeMsg msg, int start, int end)
    {
    pLxSession lex;
    int flag, toktype, alloc, err;
    XString xsbuf;
    char *hdrnme, *hdrbdy;

    /** Initialize the message structure **/
    msg->ContentDisp[0] = 0;
    msg->ContentDispFilename[0] = 0;
    msg->ContentMainType = 0;
    msg->ContentSubType[0] = 0;
    msg->Boundary[0] = 0;
    msg->PartName[0] = 0;
    msg->Subject[0] = 0;
    msg->Charset[0] = 0;
    msg->TransferEncoding[0] = 0;
    msg->MIMEVersion[0] = 0;
    msg->Mailer[0] = 0;
    msg->HdrSeekStart = 0;
    msg->MsgSeekStart = 0;
    msg->MsgSeekEnd = 0;

    lex = mlxGenericSession(obj->Prev, objRead, MLX_F_LINEONLY|MLX_F_NODISCARD);
    if (!lex)
	{
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
	    return -1;
	    }
	/* get the next line */
	alloc = 0;
	xsInit(&xsbuf);
	xsCopy(&xsbuf, mlxStringVal(lex, &alloc), -1);
	xsRTrim(&xsbuf);
	//if (MIME_DEBUG) printf("MIME: Got Token (%s)\n", xsbuf.String);
	/* check if this is the end of the headers, if so, exit the loop (flag=0), */
	/* otherwise parse the header elements */
	if (!strlen(xsbuf.String))
	    {
	    flag = 0;
	    }
	else
	    {
	    if (libmime_LoadExtendedHeader(msg, &xsbuf, lex) < 0)
		{
		mlxCloseSession(lex);
		return -1;
		}

	    hdrnme = (char*)nmMalloc(64);
	    hdrbdy = (char*)nmMalloc(strlen(xsbuf.String)+1);
	    strncpy(hdrbdy, xsbuf.String, strlen(xsbuf.String));
	    if (libmime_ParseHeader(hdrbdy, hdrnme) == 0)
		{
		if      (!strcasecmp(hdrnme, "Content-Type")) err = libmime_SetContentType(msg, hdrbdy);
		else if (!strcasecmp(hdrnme, "Content-Disposition")) err = libmime_SetContentDisp(msg, hdrbdy);
		else if (!strcasecmp(hdrnme, "Content-Transfer-Encoding")) err = libmime_SetTransferEncoding(msg, hdrbdy);
		else if (!strcasecmp(hdrnme, "To")) err = libmime_SetTo(msg, hdrbdy);
		else if (!strcasecmp(hdrnme, "Cc")) err = libmime_SetCc(msg, hdrbdy);
		else if (!strcasecmp(hdrnme, "From")) err = libmime_SetFrom(msg, hdrbdy);
		else if (!strcasecmp(hdrnme, "Subject")) err = libmime_SetSubject(msg, hdrbdy);
		else if (!strcasecmp(hdrnme, "Date")) err = libmime_SetDate(msg, hdrbdy);
		else if (!strcasecmp(hdrnme, "MIME-Version")) err = libmime_SetMIMEVersion(msg, hdrbdy);
		else if (!strcasecmp(hdrnme, "X-Mailer")) err = libmime_SetMailer(msg, hdrbdy);

		if (err < 0)
		    {
		    if (MIME_DEBUG) fprintf(stderr, "ERROR PARSING \"%s\": \"%s\"\n", hdrnme, hdrbdy);
		    }
		}
	    else
		{
		if (MIME_DEBUG) fprintf(stderr, "ERROR PARSING: %s\n", xsbuf.String);
		}
	    }
	}
    xsDeInit(&xsbuf);
    mlxCloseSession(lex);
    msg->HdrSeekStart = start;
//    msg->MsgSeekStart = mlxGetOffset(lex);

    return 0;
    }


/*  libmime_LoadExtendedHeader
**
**  Header elements can span multiple lines.  We know that this occurs when there
**  is any white space at the beginning of the line.  This function will check if
**  there are any more lines that belong to the current header element.  If so, 
**  they will be read into xsbuf replacing all white spaces with just normal spaces.
*/

int
libmime_LoadExtendedHeader(pMimeMsg msg, pXString xsbuf, pLxSession lex)
    {
    int toktype, i;
    unsigned long offset;
    char *ptr;

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
	libmime_StringTrim(ptr);
	xsConcatPrintf(xsbuf, " %s", ptr);
	}
    /** Be kind, rewind! (resetting the offset because we don't use the last string it fetched) **/
    mlxSetOffset(lex, offset);
    /** Set all tabs, NL's, CR's to spaces **/
    for(i=0;i<strlen(xsbuf->String);i++) if (strchr("\t\r\n",xsbuf->String[i])) xsbuf->String[i]=' ';

    return 0;
    }

/*  libmime_SetMailer
**
**  Parses the "X-Mailer" header element and fills in the MimeMsg data structure
**  with the data accordingly.
*/
int
libmime_SetMailer(pMimeMsg msg, char *buf)
    {
    strncpy(msg->Mailer, buf, 79);
    msg->MIMEVersion[79] = 0;

    if (MIME_DEBUG)
	{
	printf("MIME Parser (X-Mailer)\n");
	printf("  X-MAILER    : \"%s\"\n", msg->Mailer);
	}
    return 0;
    }

/*  libmime_SetMIMEVersion
**
**  Parses the "MIME-Version" header element and fills in the MimeMsg data structure
**  with the data accordingly.
*/
int
libmime_SetMIMEVersion(pMimeMsg msg, char *buf)
    {
    strncpy(msg->MIMEVersion, buf, 15);
    msg->MIMEVersion[15] = 0;

    if (MIME_DEBUG)
	{
	printf("MIME Parser (MIME-Version)\n");
	printf("  MIME-VERSION: \"%s\"\n", msg->MIMEVersion);
	}
    return 0;
    }

/*  libmime_SetDate
**
**  Parses the "Date" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetDate(pMimeMsg msg, char *buf)
    {
    /** Get the date **/

    /** FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME:
     **   objDataToDateTime does not currently behave properly.  When that function
     **   gets fixed, fix this!!
     **/
    // objDataToDateTime(DATA_T_STRING, xsptr->String, &msg->PartDate, NULL);

    return 0;
    }

/*  libmime_SetSubject
**
**  Parses the "Subject" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetSubject(pMimeMsg msg, char *buf)
    {
    /** Get the date **/
    strncpy(msg->Subject, buf, 79);
    msg->Subject[79] = 0;

    if (MIME_DEBUG)
	{
	printf("MIME Parser (Subject)\n");
	printf("  SUBJECT     : \"%s\"\n", msg->Subject);
	}

    return 0;
    }

/*  libmime_SetFrom
**
**  Parses the "From" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetFrom(pMimeMsg msg, char *buf)
    {
    msg->FromList = (pXArray)nmMalloc(sizeof(XArray));
    xaInit(msg->FromList, sizeof(EmailAddr));
    libmime_ParseAddressList(buf, msg->FromList);
    if (MIME_DEBUG) libmime_PrintAddressList(msg->FromList, 0);
    return 0;
    }

/*  libmime_SetCc
**
**  Parses the "Cc" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetCc(pMimeMsg msg, char *buf)
    {
    msg->CcList = (pXArray)nmMalloc(sizeof(XArray));
    xaInit(msg->CcList, sizeof(EmailAddr));
    libmime_ParseAddressList(buf, msg->CcList);
    if (MIME_DEBUG) libmime_PrintAddressList(msg->CcList, 0);
    return 0;
    }

/*  libmime_SetTo
**
**  Parses the "To" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetTo(pMimeMsg msg, char *buf)
    {
    msg->ToList = (pXArray)nmMalloc(sizeof(XArray));
    xaInit(msg->ToList, sizeof(EmailAddr));
    libmime_ParseAddressList(buf, msg->ToList);
    if (MIME_DEBUG) libmime_PrintAddressList(msg->ToList, 0);
    return 0;
    }

/*  libmime_SetTransferEncoding
**
**  Parses the "Content-Transfer-Encoding" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetTransferEncoding(pMimeMsg msg, char *buf)
    {
    strncpy(msg->TransferEncoding, buf, 31);
    msg->TransferEncoding[31] = 0;

    if (MIME_DEBUG)
	{
	printf("MIME Parser (Content-Transfer-Encoding)\n");
	printf("  TRANSFER ENCODING: \"%s\"\n", msg->TransferEncoding);
	}
    return 0;
    }

/*  libmime_SetContentDisp
**
**  Parses the "Content-Disposition" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetContentDisp(pMimeMsg msg, char *buf)
    {
    char *ptr, *cptr;

    /** get the display main type **/
    if (!(ptr=strtok_r(buf, ": ", &buf))) return 0;
    if (!(ptr=strtok_r(buf, "; ", &buf))) return 0;

    strncpy(msg->ContentDisp, ptr, 79);
    msg->ContentDisp[79] = 0;

    /** Check for the "filename=" content-disp token **/
    while ((ptr = strtok_r(buf, "= ", &buf)))
	{
	if (!(cptr = strtok_r(buf, ";", &buf))) break;
	while (*ptr == ' ') ptr++;
	if (!libmime_StringFirstCaseCmp(ptr, "filename"))
	    {
	    strncpy(msg->ContentDispFilename, libmime_StringUnquote(cptr), 79);
	    msg->ContentDispFilename[79] = 0;
	    }
	}

    if (MIME_DEBUG)
	{
	printf("MIME Parser (Content-Disposition)\n");
	printf("  CONTENT DISP: \"%s\"\n", msg->ContentDisp);
	printf("  FILENAME    : \"%s\"\n", msg->ContentDispFilename);
	}

    return 0;
    }

/*  libmime_SetContentType
**
**  Parses the "Content-Type" header element and fills in the MimeMsg data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetContentType(pMimeMsg msg, char *buf)
    {
    char *ptr, *cptr;
    char maintype[32], tmpname[128];
    int len,i;

    /** Get the disp main type and subtype **/
    if (!(ptr=strtok_r(buf, "; ", &buf))) return 0;
    if ((cptr=strchr(ptr,'/')))
	{
	len = (int)cptr - (int)ptr;
	if (len>31) len=31;
	strncpy(maintype, ptr, len);
	maintype[len] = 0;
	strncpy(msg->ContentSubType, cptr+1, 79);
	msg->ContentSubType[79] = 0;
	}
    else
	{
	strncpy(maintype, ptr, 31);
	maintype[31] = 0;
	}
    for (i=0; i<7; i++)
	{
	if (!libmime_StringFirstCaseCmp(maintype, TypeStrings[i]))
	    {
	    msg->ContentMainType = i;
	    }
	}
    
    /** Look at any possible parameters **/
    while ((ptr = strtok_r(buf, "= ", &buf)))
	{
	if (!(cptr=strtok_r(buf, ";", &buf))) break;
	while (*ptr == ' ') ptr++;
	if (!libmime_StringFirstCaseCmp(ptr, "boundary"))
	    {
	    strncpy(msg->Boundary, libmime_StringUnquote(cptr), 79);
	    msg->Boundary[79] = 0;
	    }
	else if (!libmime_StringFirstCaseCmp(ptr, "name"))
	    {
	    strncpy(tmpname, libmime_StringUnquote(cptr), 127);
	    tmpname[127] = 0;
	    if (strchr(tmpname,'/'))
		strncpy(msg->PartName, strrchr(tmpname,'/')+1,79);
	    else 
		strncpy(msg->PartName, tmpname, 79);
	    msg->PartName[79] = 0;
	    if (strchr(msg->PartName,'\\'))
		strncpy(msg->PartName,strrchr(tmpname,'\\')+1,79);
	    msg->PartName[79] = 0;
	    }
	else if (!libmime_StringFirstCaseCmp(ptr, "subject"))
	    {
	    strncpy(msg->Subject, libmime_StringUnquote(cptr), 79);
	    msg->Subject[79] = 0;
	    }
	else if (!libmime_StringFirstCaseCmp(ptr, "charset"))
	    {
	    strncpy(msg->Charset, libmime_StringUnquote(cptr), 31);
	    msg->Charset[31] = 0;
	    }
	}

    if (MIME_DEBUG)
	{
	printf("MIME Parser (Content-Type)\n");
	printf("  TYPE:       : \"%s\"\n", TypeStrings[msg->ContentMainType]);
	printf("  SUBTYPE     : \"%s\"\n", msg->ContentSubType);
	printf("  BOUNDARY    : \"%s\"\n", msg->Boundary);
	printf("  NAME        : \"%s\"\n", msg->PartName);
	printf("  SUBJECT     : \"%s\"\n", msg->Subject);
	printf("  CHARSET     : \"%s\"\n", msg->Charset);
	}

    return 0;
    }

/*
**  int
**  libmime_ParseHeader(char* buf, char* hdr);
**     Parameters:
**         (char*) buf     A string of characters with no CRLF's in it.  This
**                         string should represent the whole header, including any
**                         folded header elements below itself.  This string will
**                         be modified to contain the main part of the header.
**         (char*) hdr     This string will be overwritten with a string that 
**                         is the name of the header element (To, From, Sender...)
**     Returns:
**         This function returns 0 on success, and -1 on failure.  It modifies
**         the "buf" parameter and sends its work back in this way.  This
**         function will return a string of characters that is properly
**         formatted according to RFC822.  The header tag will be stripped away
**         from the beginning ("X-Header"), all extra whitespace will be
**         removed, and all comments will be removed as well.  This will be a
**         clean header line.
**
**     State Definitions:
**         0 == We have only seen non-space, non-tab, and non-colon characters
**              up to this point.  As soon as one of those characters is seen,
**              the state will change.
**         1 == We have seen a whitespace character, thus only a colon or more
**              whitespace should be visible.  If not, return an error.
**         2 == We have seen the colon!  The next character is the beginning of
**              the header content.  Trim and return that string.
*/

int
libmime_ParseHeader(char *buf, char* hdr)
    {
    int count=0, state=0;
    char *ptr;
    char ch;

    while (count < strlen(buf))
	{
	ch = buf[count];
	/**  STATE 0 (no spaces or colons have been seen) **/
	if (state == 0)
	    {
	    if (ch == ':')
		{
		ptr = buf+count+1;
		state = 2;
		}
	    else if (ch==' ' || ch=='\t')
		{
		state = 1;
		}
	    }
	/** STATE 1 (space has been seen, only spaces and a colon should follow) **/
	else if (state == 1)
	    {
	    if (ch == ':')
		{
		ptr = buf+count+1;
		state = 2;
		}
	    else if (ch!=' ' && ch!='\t')
		{
		return -1;
		}
	    }
	/** STATE 2 (the colon has been spotted, left side is header, right is body **/
	else if (state == 2)
	    {
	    memcpy(hdr, buf, (count-1>79?79:count-1));
	    hdr[(count-1>79?79:count-1)] = 0;
	    memcpy(buf, &buf[count+1], strlen(&buf[count+1])+1);
	    libmime_StringTrim(hdr);
	    libmime_StringTrim(buf);
	    return 0;
	    }
	count++;
	}
    return -1;
    }
