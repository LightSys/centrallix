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

char* EncodingStrings[] =
    {
    "7bit",
    "8bit",
    "base64",
    "quoted-printable",
    "binary"
    };

/*  libmime_ParseHeader
**
**  Parses a message (located at obj->Prev) starting at the "start" byte, and ending
**  at the "end" byte.  This creates the MimeHeader data structure and recursively
**  calls itself to fill it in.  Note that no data is actually stored.  This is just
**  the shell of the message and contains seek points denoting where to start
**  and end reading.
*/
int
libmime_ParseHeader(pObject obj, pMimeHeader msg, long start, long end, pLxSession lex)
    {
    int flag, toktype, alloc, err, size, len;
    XString xsbuf;
    char *hdrnme, *hdrbdy;

    /** Initialize the message structure **/
    msg->ContentLength = 0;
    msg->ContentDisposition[0] = 0;
    msg->Filename[0] = 0;
    msg->ContentMainType = MIME_TYPE_APPLICATION;
    msg->ContentSubType[0] = 0;
    msg->Boundary[0] = 0;
    msg->Subject[0] = 0;
    msg->Charset[0] = 0;
    msg->TransferEncoding = MIME_ENC_7BIT;
    msg->MIMEVersion[0] = 0;
    msg->Mailer[0] = 0;
    msg->MsgSeekStart = 0;
    msg->MsgSeekEnd = 0;

    if (!lex)
	{
	return -1;
	}

    mlxSetOffset(lex, start);
    if (MIME_DEBUG) fprintf(stderr, "\nStarting Header Parsing... (s:%ld)\n", start);
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
	len = strlen(xsbuf.String);
	xsRTrim(&xsbuf);
	//if (MIME_DEBUG) fprintf(stderr, "MIME: Got Token (%s)\n", xsbuf.String);
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
		return -1;
		}

	    hdrnme = (char*)nmMalloc(64);
	    hdrbdy = (char*)nmMalloc(strlen(xsbuf.String)+1);
	    strncpy(hdrbdy, xsbuf.String, strlen(xsbuf.String));
	    if (libmime_ParseHeaderElement(hdrbdy, hdrnme) == 0)
		{
		if      (!strcasecmp(hdrnme, "Content-Type")) err = libmime_SetContentType(msg, hdrbdy);
		else if (!strcasecmp(hdrnme, "Content-Disposition")) err = libmime_SetContentDisp(msg, hdrbdy);
		else if (!strcasecmp(hdrnme, "Content-Transfer-Encoding")) err = libmime_SetTransferEncoding(msg, hdrbdy);
		else if (!strcasecmp(hdrnme, "Content-Length")) err = libmime_SetContentLength(msg, hdrbdy);
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
	xsDeInit(&xsbuf);
	}

    /** Set the start and end offsets for the message **/
    msg->MsgSeekStart = mlxGetOffset(lex) + len;
    /** If an end offset was passed in, use it!  If not (end==0), then find the end **/
    if (end)
	{
	msg->MsgSeekEnd = end;
	}
    else
	{
	flag = 1;
	start = 0;
	mlxSetOffset(lex, msg->MsgSeekStart);
	while (flag)
	    {
	    toktype = mlxNextToken(lex);
	    if (toktype == MLX_TOK_ERROR)
		{
		flag = 0;
		}
	    else
		{
		xsInit(&xsbuf);
		xsCopy(&xsbuf, mlxStringVal(lex, &alloc), -1);
		xsDeInit(&xsbuf);
		start += strlen(xsbuf.String);
		}
	    }
	msg->MsgSeekEnd = size;
	}

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
libmime_LoadExtendedHeader(pMimeHeader msg, pXString xsbuf, pLxSession lex)
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
**  Parses the "X-Mailer" header element and fills in the MimeHeader data structure
**  with the data accordingly.
*/
int
libmime_SetMailer(pMimeHeader msg, char *buf)
    {
    strncpy(msg->Mailer, buf, 79);
    msg->MIMEVersion[79] = 0;

    if (MIME_DEBUG)
	{
	printf("  X-MAILER    : \"%s\"\n", msg->Mailer);
	}
    return 0;
    }

/*  libmime_SetMIMEVersion
**
**  Parses the "MIME-Version" header element and fills in the MimeHeader data structure
**  with the data accordingly.
*/
int
libmime_SetMIMEVersion(pMimeHeader msg, char *buf)
    {
    strncpy(msg->MIMEVersion, buf, 15);
    msg->MIMEVersion[15] = 0;

    if (MIME_DEBUG)
	{
	printf("  MIME-VERSION: \"%s\"\n", msg->MIMEVersion);
	}
    return 0;
    }

/*  libmime_SetDate
**
**  Parses the "Date" header element and fills in the MimeHeader data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetDate(pMimeHeader msg, char *buf)
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
**  Parses the "Subject" header element and fills in the MimeHeader data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetSubject(pMimeHeader msg, char *buf)
    {
    /** Get the date **/
    strncpy(msg->Subject, buf, 79);
    msg->Subject[79] = 0;

    if (MIME_DEBUG)
	{
	printf("  SUBJECT     : \"%s\"\n", msg->Subject);
	}

    return 0;
    }

/*  libmime_SetFrom
**
**  Parses the "From" header element and fills in the MimeHeader data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetFrom(pMimeHeader msg, char *buf)
    {
    msg->FromList = (pXArray)nmMalloc(sizeof(XArray));
    xaInit(msg->FromList, sizeof(EmailAddr));
    libmime_ParseAddressList(buf, msg->FromList);
    if (MIME_DEBUG)
	{
	printf("  FROM        : ");
	libmime_PrintAddressList(msg->FromList, 0);
	}

    return 0;
    }

/*  libmime_SetCc
**
**  Parses the "Cc" header element and fills in the MimeHeader data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetCc(pMimeHeader msg, char *buf)
    {
    msg->CcList = (pXArray)nmMalloc(sizeof(XArray));
    xaInit(msg->CcList, sizeof(EmailAddr));
    libmime_ParseAddressList(buf, msg->CcList);
    if (MIME_DEBUG)
	{
	printf("  CC          : ");
	libmime_PrintAddressList(msg->CcList, 0);
	}
    return 0;
    }

/*  libmime_SetTo
**
**  Parses the "To" header element and fills in the MimeHeader data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetTo(pMimeHeader msg, char *buf)
    {
    msg->ToList = (pXArray)nmMalloc(sizeof(XArray));
    xaInit(msg->ToList, sizeof(EmailAddr));
    libmime_ParseAddressList(buf, msg->ToList);
    if (MIME_DEBUG)
	{
	printf("  TO          : ");
	libmime_PrintAddressList(msg->ToList, 0);
	}
    return 0;
    }

/*  libmime_SetContentLength
**
**  Parses the "Content-Length" header element and fills in the MimeHeader data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetContentLength(pMimeHeader msg, char *buf)
    {
    msg->ContentLength = atoi(buf);

    if (MIME_DEBUG)
	{
	printf("  CONTENT-LEN : %d\n", msg->ContentLength);
	}
    return 0;
    }

/*  libmime_SetTransferEncoding
**
**  Parses the "Content-Transfer-Encoding" header element and fills in the MimeHeader data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetTransferEncoding(pMimeHeader msg, char *buf)
    {
    if (!strlen(buf) || !strcasecmp(buf, "7bit"))
	msg->TransferEncoding = MIME_ENC_7BIT;
    else if (!strcasecmp(buf, "8bit"))
	msg->TransferEncoding = MIME_ENC_8BIT;
    else if (!strcasecmp(buf, "base64"))
	msg->TransferEncoding = MIME_ENC_BASE64;
    else if (!strcasecmp(buf, "quoted-printable"))
	msg->TransferEncoding = MIME_ENC_QP;
    else if (!strcasecmp(buf, "binary"))
	msg->TransferEncoding = MIME_ENC_BINARY;
    else
	msg->TransferEncoding = MIME_ENC_7BIT;

    if (MIME_DEBUG)
	{
	printf("  TRANS-ENC   : %d\n", msg->TransferEncoding);
	}
    return 0;
    }

/*  libmime_SetContentDisp
**
**  Parses the "Content-Disposition" header element and fills in the MimeHeader data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetContentDisp(pMimeHeader msg, char *buf)
    {
    char *ptr, *cptr;

    /** get the display main type **/
    if (!(ptr=strtok_r(buf, "; ", &buf))) return 0;

    strncpy(msg->ContentDisposition, ptr, 79);
    msg->ContentDisposition[79] = 0;

    /** Check for the "filename=" content-disp token **/
    while ((ptr = strtok_r(buf, "= ", &buf)))
	{
	if (!(cptr = strtok_r(buf, ";", &buf))) break;
	while (*ptr == ' ') ptr++;
	if (!libmime_StringFirstCaseCmp(ptr, "filename"))
	    {
	    strncpy(msg->Filename, libmime_StringUnquote(cptr), 79);
	    msg->Filename[79] = 0;
	    }
	}

    if (MIME_DEBUG)
	{
	printf("  CONTENT DISP: \"%s\"\n", msg->ContentDisposition);
	printf("  FILENAME    : \"%s\"\n", msg->Filename);
	}

    return 0;
    }

/*  libmime_SetContentType
**
**  Parses the "Content-Type" header element and fills in the MimeHeader data structure
**  with the data accordingly.  If certain elements are not there, defaults are used.
*/
int
libmime_SetContentType(pMimeHeader msg, char *buf)
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
	    msg->ContentMainType = i+1;
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
	else if (!libmime_StringFirstCaseCmp(ptr, "name") && !strlen(msg->Filename))
	    {
	    strncpy(tmpname, libmime_StringUnquote(cptr), 127);
	    tmpname[127] = 0;
	    if (strchr(tmpname,'/'))
		strncpy(msg->Filename, strrchr(tmpname,'/')+1,79);
	    else 
		strncpy(msg->Filename, tmpname, 79);
	    msg->Filename[79] = 0;
	    if (strchr(msg->Filename,'\\'))
		strncpy(msg->Filename,strrchr(tmpname,'\\')+1,79);
	    msg->Filename[79] = 0;
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
	printf("  TYPE        : \"%s\"\n", TypeStrings[msg->ContentMainType-1]);
	printf("  SUBTYPE     : \"%s\"\n", msg->ContentSubType);
	printf("  BOUNDARY    : \"%s\"\n", msg->Boundary);
	printf("  FILENAME    : \"%s\"\n", msg->Filename);
	printf("  SUBJECT     : \"%s\"\n", msg->Subject);
	printf("  CHARSET     : \"%s\"\n", msg->Charset);
	}

    return 0;
    }

/*
**  int
**  libmime_ParseHeaderElement(char* buf, char* hdr);
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
libmime_ParseHeaderElement(char *buf, char* hdr)
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

/*
**  int
**  libmime_ParseMultipartBody(pObject obj, pMimeHeader msg, int start, int end, pLxSession lex)
**
**  Parses the body of a multipart message.  This fills in the Parts section of the
**  pMimeHeader data structure.  It will start parsing at the "start" location, and
**  will keep parsing until all the boundaries have been found or until the byte "end"
**  has been reached.
*/
int
libmime_ParseMultipartBody(pObject obj, pMimeHeader msg, int start, int end, pLxSession lex)
    {
    XString xsbuf;
    pMimeHeader l_msg;
    int flag=1, alloc, toktype, p_count=0, count=0, s=0;
    int l_pos=0;
    char bound[80], bound_end[82];

    if (!lex)
	{
	return -1;
	}
    mlxSetOffset(lex, msg->MsgSeekStart);
    count = msg->MsgSeekStart;

    snprintf(bound, 79, "--%s", msg->Boundary);
    snprintf(bound_end, 81, "--%s--", msg->Boundary);
    bound[79] = 0;
    bound_end[81] = 0;

    while (flag)
	{
	mlxSetOptions(lex, MLX_F_LINEONLY|MLX_F_NODISCARD);
	toktype = mlxNextToken(lex);
	if (toktype == MLX_TOK_ERROR || end <= count)
	    {
	    flag = 0;
	    }
	else
	    {
	    alloc = 0;
	    xsInit(&xsbuf);
	    xsCopy(&xsbuf, mlxStringVal(lex, &alloc), -1);
	    count = mlxGetOffset(lex);
	    /** Check if this is the start of a boundary **/
	    if (!strncmp(xsbuf.String, bound, strlen(bound)))
		{
		if (l_pos != 0)
		    {
		    l_msg = (pMimeHeader)nmMalloc(sizeof(MimeHeader));
		    libmime_ParseHeader(obj, l_msg, l_pos+s, p_count, lex);
		    xaAddItem(&msg->Parts, l_msg);
		    }
		s=strlen(xsbuf.String);
		l_pos = count;
		/** Check if it is the boundary end **/
		if (!strncmp(xsbuf.String, bound_end, strlen(bound_end)))
		    {
		    flag = 0;
		    }
		mlxSetOffset(lex, l_pos+s);
		}
	    p_count = count + strlen(xsbuf.String);
	    xsDeInit(&xsbuf);
	    }
	}

    return 0;
    }

/*
**  int
**  libmime_PartRead(pObject obj, pMimeHeader msg, char* buffer, int maxcnt, int offset)
**
**  Using nearly the same interface as objRead (except for the first
**  parameter), this function will read an arbitrary number of bytes from a
**  MIME part, doing all the decoding of that part behind the scenes (as
**  specified by the Content-Transfer-Encoding header element).
*/
int
libmime_PartRead(pObject obj, pMimeHeader msg, char* buffer, int maxcnt, int offset)
    {
    static char b64[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
    int size;
    char *a_buf, *b_buf, a_ch, *ptr;
    int a_count=0, a_bufcnt=0, a_pos=0;
    int b_count=0, i, buf_cnt=0;

    a_pos = msg->MsgSeekStart;
    switch (msg->TransferEncoding)
	{
	/** 7BIT AND 8BIT ENCODING **/
	/** QUOTED-PRINTABLE ENCODING **/
	case MIME_ENC_7BIT:
	case MIME_ENC_8BIT:
	case MIME_ENC_QP:  /**  Not currently supported, just print the text **/
	    if (msg->MsgSeekStart+offset > msg->MsgSeekEnd)
		return 0;
	    if (msg->MsgSeekStart+offset+maxcnt > msg->MsgSeekEnd)
		maxcnt = msg->MsgSeekEnd - (msg->MsgSeekStart + offset);
	    buf_cnt = objRead(obj->Prev, buffer, maxcnt, msg->MsgSeekStart+offset, FD_U_SEEK);
	    break;
	/** BASE64 ENCODING **/
	case MIME_ENC_BASE64:
	    /**
	     **  NOTE:  This is not optomized.  It should be possible to 
	     **  not have to read every character in, but to seek to the
	     **  specific point in the stream using the fact that 4-b64
	     **  characters decodes to 3 bytes.  Do this at some point.
	     */
	    a_buf = (char*)nmMalloc(5);
	    b_buf = (char*)nmMalloc(4);
	    size = objRead(obj->Prev, &a_ch, 1, msg->MsgSeekStart, FD_U_SEEK);
	    a_pos += size;
	    a_count += size;
	    while (size && a_pos <= msg->MsgSeekEnd+2)
		{
		/** If the character isn't in the b64 alphabet, ignore it **/
		ptr = strchr(b64, a_ch);
		if (ptr)
		    {
		    a_buf[a_bufcnt++] = a_ch;
		    /** Collect 4 characters before sending it through the decoder **/
		    if (a_bufcnt >= 4)
			{
			a_bufcnt = 0;
			a_buf[4] = 0;
			b_count += 3;
			/** If we are somewhere within the read range (offset by max 3 on each side) **/
			if (b_count >= offset && b_count <= offset+maxcnt+2)
			    {
			    libmime_DecodeBase64(b_buf, a_buf, 4);
			    b_buf[3] = 0;
			    /** within three on the left side of the string **/
			    if ((b_count-offset) < 3)
				{
				for (i=3-(b_count-offset); i < 3; i++)
				    {
				    buffer[buf_cnt++] = b_buf[i];
				    }
				}
			    /** within three on the right side of the string **/
			    else if (b_count >= (offset+maxcnt))
				{
				for (i=0; i <= 2-(b_count-(offset+maxcnt)); i++)
				    {
				    buffer[buf_cnt++] = b_buf[i];
				    }
				}
			    /** the full three characters **/
			    else
				{
				buffer[buf_cnt++] = b_buf[0];
				buffer[buf_cnt++] = b_buf[1];
				buffer[buf_cnt++] = b_buf[2];
				}
			    }
			}
		    }
		size = objRead(obj->Prev, &a_ch, 1, msg->MsgSeekStart+a_count, FD_U_SEEK);
		a_pos += size;
		a_count += size;
		}
	    nmFree(a_buf, 5);
	    nmFree(b_buf, 4);
	    break;
	/** BINARY ENCODING **/
	case MIME_ENC_BINARY:
	    if (msg->MsgSeekStart+offset > msg->MsgSeekEnd)
		return 0;
	    if (msg->MsgSeekStart+offset+maxcnt > msg->MsgSeekEnd)
		maxcnt = msg->MsgSeekEnd - (msg->MsgSeekStart + offset);
	    buf_cnt = objRead(obj->Prev, buffer, maxcnt, msg->MsgSeekStart+offset, FD_U_SEEK);
	    break;
	}

    return buf_cnt;
    }
