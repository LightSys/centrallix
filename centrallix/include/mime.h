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

#define MIME_DEBUG            1
#define MIME_DEBUG_ADDR       1

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

#define MIME_ENC_7BIT         1
#define MIME_ENC_8BIT         2
#define MIME_ENC_BASE64       3
#define MIME_ENC_QP           4
#define MIME_ENC_BINARY       5

#define MIME_BUF_SIZE         768   // These must be in a 3/4 ratio (or higher) of
#define MIME_ENCBUF_SIZE      1024  // each other because of how b64 encoding works

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
    char	MIMEVersion[16];
    char	Mailer[80];
    int		TransferEncoding;
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

/** libmime internal structure **/
typedef struct
    {
    void*	Parent;
    int		(*ReadFn)();
    int		(*WriteFn)();
    long	ExternalChunkSeek;
    int		ExternalChunkSize;
    long	InternalSeek;
    long	InternalChunkSeek;
    int		InternalChunkSize;
    char	Buffer[MIME_BUF_SIZE];
    char	EncBuffer[MIME_ENCBUF_SIZE+1];
    }
    MimeData, *pMimeData;

/*** Possible Main Content Types ***/
extern char* TypeStrings[];
extern char* EncodingStrings[];

/** mime_parse.c **/
int libmime_ParseHeader(pLxSession lex, pMimeHeader msg, long start, long end);
int libmime_ParseHeaderElement(char *buf, char *element);
int libmime_ParseMultipartBody(pLxSession lex, pMimeHeader msg, int start, int end);
int libmime_LoadExtendedHeader(pLxSession lex, pMimeHeader msg, pXString xsbuf);
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
int libmime_ReadPart(pMimeData mdat, pMimeHeader msg, char* buffer, int maxcnt, int offset, int flags);

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
int libmime_B64Purify(char *str);

/** mime_encode.c **/
int libmime_EncodeQP();
int libmime_DecodeQP();
int libmime_EncodeBase64();
int libmime_DecodeBase64();

#endif /** _MIME_H **/
