#ifndef _MIME_H
#define _MIME_H

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
/* Module: 	libmime							*/
/* Author:	Luke Ehresman(LME)					*/
/* Creation:	August 12, 2002						*/
/* Description:	Provides declarations for the MIME parser		*/
/************************************************************************/

/** Structure used to represent an email address **/
typedef struct
    {
    char	Host[128];
    char	Mailbox[128];
    char	Display[128];
    char	AddressLine[256];
    pXArray	Group;
    }
    EmailAddr, *pEmailAddr;

/** information structure for MIME msg **/
typedef struct _MM
    {
    int		ContentLength;
    int		ContentMainType;
    char	ContentSubType[80];
    char	ContentDisposition[80];
    char	Filename[80];
    char	Boundary[80];
    char	Subject[80];
    char	Charset[32];
    char	TransferEncoding[32];
    char	MIMEVersion[16];
    char	Mailer[80];
    DateTime	Date;
    long	MsgSeekStart;
    long	MsgSeekEnd;
    pXArray	ToList;
    pXArray	FromList;
    pXArray	CcList;
    pEmailAddr	Sender;
    XArray	Parts;
    }
    MimeHeader, *pMimeHeader;

/*** Possible Main Content Types ***/
extern char* TypeStrings[];

#define MIME_DEBUG            1
#define MIME_DEBUG_ADDR       0

#define MIME_ST_NORM          0
#define MIME_ST_QUOTE         1
#define MIME_ST_COMMENT       2
#define MIME_ST_GROUP         3

#define MIME_ST_ADR_HOST      0
#define MIME_ST_ADR_MAILBOX   1
#define MIME_ST_ADR_DISPLAY   2

#define MIME_BUFSIZE          64

#define MIME_TYPE_TEXT        1
#define MIME_TYPE_MULTIPART   2
#define MIME_TYPE_APPLICATION 3
#define MIME_TYPE_MESSAGE     4
#define MIME_TYPE_IMAGE       5
#define MIME_TYPE_AUDIO       6
#define MIME_TYPE_VIDEO       7


/** mime_parse.c **/
int libmime_ParseHeader(pObject obj, pMimeHeader msg, long start, long end, pLxSession lex);
int libmime_ParseHeaderElement(char *buf, char *element);
int libmime_ParseMultipartBody(pObject obj, pMimeHeader msg, int start, int end, pLxSession lex);
int libmime_LoadExtendedHeader(pMimeHeader msg, pXString xsbuf, pLxSession lex);
int libmime_SetMIMEVersion(pMimeHeader msg, char *buf);
int libmime_SetDate(pMimeHeader msg, char *buf);
int libmime_SetSubject(pMimeHeader msg, char *buf);
int libmime_SetFrom(pMimeHeader msg, char *buf);
int libmime_SetCc(pMimeHeader msg, char *buf);
int libmime_SetTo(pMimeHeader msg, char *buf);
int libmime_SetTransferEncoding(pMimeHeader msg, char *buf);
int libmime_SetContentDisp(pMimeHeader msg, char *buf);
int libmime_SetContentType(pMimeHeader msg, char *buf);
void libmime_PrintEntityContent(pMimeHeader msg, pLxSession lex);
int libmime_GetEntityContent(long start, long end, pLxSession lex);

/** mime_address.c **/
int libmime_ParseAddressList(char *buf, pXArray xary);
int libmime_ParseAddressGroup(char *buf, pEmailAddr addr);
int libmime_ParseAddress(char *buf, pEmailAddr addr);
int libmime_ParseAddressElements(char *buf, pEmailAddr addr);

/** mime_util.c **/
void libmime_Cleanup(pMimeHeader msg);
int libmime_StringLTrim(char *str);
int libmime_StringRTrim(char *str);
int libmime_StringTrim(char *str);
int libmime_StringFirstCaseCmp(char *c1, char *c2);
int libmime_PrintAddressList(pXArray ary, int level);
char* libmime_StringUnquote(char *str);

/** mime_encode.c **/
int libmime_EncodeQP();
int libmime_DecodeQP();
int libmime_EncodeBase64();
int libmime_DecodeBase64();

#endif /** _MIME_H **/
