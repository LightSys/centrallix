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
    int		ContentMainType;
    char	ContentSubType[80];
    char	ContentDisp[80];
    char	ContentDispFilename[80];
    char	Boundary[80];
    char	PartName[80];
    char	Subject[80];
    char	Charset[32];
    char	TransferEncoding[32];
    char	MIMEVersion[16];
    char	Mailer[80];
    DateTime	Date;
    long	HdrSeekStart;
    long	MsgSeekStart;
    long	MsgSeekEnd;
    pXArray	ToList;
    pXArray	FromList;
    pXArray	CcList;
    pEmailAddr	Sender;
    XArray	Parts;
    }
    MimeMsg, *pMimeMsg;

/*** Possible Main Content Types ***/
extern char* TypeStrings[];

#define MIME_DEBUG            0
#define MIME_DEBUG_ADDR       1

#define MIME_ST_NORM          0
#define MIME_ST_QUOTE         1
#define MIME_ST_COMMENT       2
#define MIME_ST_GROUP         3

#define MIME_ST_ADR_HOST      0
#define MIME_ST_ADR_MAILBOX   1
#define MIME_ST_ADR_DISPLAY   2

/** mime_parse.c **/
int libmime_ParseMessage(pObject obj, pMimeMsg msg, int start, int end);
int libmime_ParseHeaderElement(char *buf, char *element);
int libmime_LoadExtendedHeader(pMimeMsg msg, pXString xsbuf, pLxSession lex);
int libmime_SetMIMEVersion(pMimeMsg msg, char *buf);
int libmime_SetDate(pMimeMsg msg, char *buf);
int libmime_SetSubject(pMimeMsg msg, char *buf);
int libmime_SetFrom(pMimeMsg msg, char *buf);
int libmime_SetCc(pMimeMsg msg, char *buf);
int libmime_SetTo(pMimeMsg msg, char *buf);
int libmime_SetTransferEncoding(pMimeMsg msg, char *buf);
int libmime_SetContentDisp(pMimeMsg msg, char *buf);
int libmime_SetContentType(pMimeMsg msg, char *buf);

/** mime_address.c **/
int libmime_ParseAddressList(char *buf, pXArray xary);
int libmime_ParseAddressGroup(char *buf, pEmailAddr addr);
int libmime_ParseAddress(char *buf, pEmailAddr addr);
int libmime_ParseAddressElements(char *buf, pEmailAddr addr);

/** mime_util.c **/
void libmime_Cleanup(pMimeMsg msg);
int libmime_StringLTrim(char *str);
int libmime_StringRTrim(char *str);
int libmime_StringTrim(char *str);
int libmime_StringFirstCaseCmp(char *c1, char *c2);
int libmime_PrintAddressList(pXArray ary, int level);
char* libmime_StringUnquote(char *str);
#endif /** _MIME_H **/
