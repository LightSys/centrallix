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

#define MIME_ST_NORM          0
#define MIME_ST_QUOTE         1
#define MIME_ST_COMMENT       2
#define MIME_ST_GROUP         3

#define MIME_ST_ADR_HOST      0
#define MIME_ST_ADR_MAILBOX   1
#define MIME_ST_ADR_DISPLAY   2

#define MIME_BUFSIZE          63 /* TODO: Change to 64 and refactor accordingly. */

#define MIME_TYPE_TEXT        0
#define MIME_TYPE_MULTIPART   1
#define MIME_TYPE_APPLICATION 2
#define MIME_TYPE_MESSAGE     3
#define MIME_TYPE_IMAGE       4
#define MIME_TYPE_AUDIO       5
#define MIME_TYPE_VIDEO       6

#define MIME_ENC_7BIT         0
#define MIME_ENC_8BIT         1
#define MIME_ENC_BASE64       2
#define MIME_ENC_QP           3
#define MIME_ENC_BINARY       4

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

/** Structure to store arbitrary Mime parameters **/
typedef struct
    {
    char*	Name;
    pTObjData	Ptod;
    long	ValueSeekStart;
    long	ValueSeekEnd;
    }
    MimeParam, *pMimeParam;

/** Structure to store arbitrary Mime attributes
 ** Stores default param inside itself and points
 ** to an xarray of any extra params.
 **/
typedef struct
    {
    char*	Name;
    pTObjData	Ptod;
    XHashTable	Params;
    long	ValueSeekStart;
    long	ValueSeekEnd;
    }
    MimeAttr, *pMimeAttr;

/** information structure for MIME msg **/
typedef struct _MM
    {
    DateTime	Date;
    long	HdrSeekEnd;
    long	MsgSeekStart;
    long	MsgSeekEnd;
    pEmailAddr	Sender;
    XArray	Parts;
    XHashTable	Attrs;
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
int libmime_ParseHeaderElement(char *buf, char *element, int* attrSeekEnd, int* nameOffset);
int libmime_ParseMultipartBody(pLxSession lex, pMimeHeader msg, int start, int end);
int libmime_LoadExtendedHeader(pLxSession lex, pMimeHeader msg, pXString xsbuf, int* attrSeekStart);
int libmime_SetDate(pMimeHeader msg, char *buf);
int libmime_SetTransferEncoding(pMimeHeader msg, char *buf);
int libmime_SetContentType(pMimeHeader msg, char *buf);
void libmime_PrintEntityContent(pMimeHeader msg, pLxSession lex);
int libmime_GetEntityContent(long start, long end, pLxSession lex);
int libmime_SetFilename(pMimeHeader msg, char *defaultName);
int libmime_ReadPart(pMimeData mdat, pMimeHeader msg, char* buffer, int maxcnt, int offset, int flags);

/** mime_address.c **/
int libmime_ParseAddressList(char *buf, pXArray xary);
int libmime_ParseAddressGroup(char *buf, pEmailAddr addr);
int libmime_ParseAddress(char *buf, pEmailAddr addr);
int libmime_ParseAddressElements(char *buf, pEmailAddr addr);

/** mime_util.c **/
pMimeHeader libmime_AllocateHeader();
void libmime_DeallocateHeader(pMimeHeader msg);
int libmime_StringLTrim(char *str);
int libmime_StringRTrim(char *str);
int libmime_StringTrim(char *str);
int libmime_StringFirstCaseCmp(char *c1, char *c2);
int libmime_PrintAddressList(pXArray ary, int level);
char* libmime_StringUnquote(char *str);
int libmime_B64Purify(char *str);
int libmime_ContentExtension(char *str, int type, char *subtype);
void* libmime_xhLookup(pXHashTable this, char* key);
int libmime_xhAdd(pXHashTable this, char* key, char* data);
int libmime_xhDeInit(pXHashTable this);
int libmime_SaveTemporaryFile(pFile fd, pObject obj, int truncSeek);
int libmime_internal_MakeARandomFilename(char* name, int len);


/** mime_attributes.c **/
int libmime_ParseAttr(pMimeHeader this, char* name, char* data, int attrSeekStart, int attrSeekEnd, int nameOffset);
int libmime_ParseIntAttr(pMimeHeader this, char* name, char* data);
int libmime_ParseStringAttr(pMimeHeader this, char* name, char* data);
int libmime_ParseEmailAttr(pMimeHeader this, char* name, char* data);
int libmime_ParseEmailListAttr(pMimeHeader this, char* name, char* data);
int libmime_ParseCsvAttr(pMimeHeader this, char* name, char* data);
int libmime_ParseParameterListAttr(pMimeAttr attr, char* data);

int libmime_CreateIntAttr(pMimeHeader this, char* attr, char* param, int data);
int libmime_CreateStringAttr(pMimeHeader this, char* attr, char* param, char* data, int flags);
int libmime_CreateStringArrayAttr(pMimeHeader this, char* attr, char* param);
int libmime_CreateAttr(pMimeHeader this, char* attr, char* param, void* data, int datatype);
int libmime_CreateArrayAttr(pMimeHeader this, char* attr, char* param);

pTObjData* libmime_CreateAttrParam(pMimeHeader this, char* attr, char* param);
pTObjData libmime_GetPtodFromHeader(pMimeHeader this, char* attr, char* param);
pTObjData* libmime_GetPtodPointer(pMimeHeader this, char* attr, char* param);
int libmime_GetAttrParamNames(char* raw, char** attr, char** param);
pMimeAttr libmime_GetMimeAttr(pMimeHeader this, char* attr);
pMimeParam libmime_GetMimeParam(pMimeHeader this, char* attr, char* param);

int libmime_GetIntAttr(pMimeHeader this, char* attr, char* param, int* ret);
int libmime_GetStringAttr(pMimeHeader this, char* attr, char* param, char** ret);
int libmime_GetStringArrayAttr(pMimeHeader this, char* attr, char* param, pStringVec* ret);
int libmime_GetAttr(pMimeHeader this, char* attr, char* param, void** ret);
int libmime_GetArrayAttr(pMimeHeader this, char* attr, char* param, pXArray* ret);

int libmime_SetIntAttr(pMimeHeader this, char* attr, char* param, int data);
int libmime_SetStringAttr(pMimeHeader this, char* attr, char* param, char* data, int flags);
int libmime_SetAttr(pMimeHeader this, char* attr, char* param, void* data, int datatype);

int libmime_AppendStringArrayAttr(pMimeHeader this, char* attr, char* param, pXArray dataList);
int libmime_AddStringArrayAttr(pMimeHeader this, char* attr, char* param,  char* data);
int libmime_AppendArrayAttr(pMimeHeader this, char* attr, char* param, pXArray dataList);
int libmime_AddArrayAttr(pMimeHeader this, char* attr, char* param, void* data);

int libmime_ClearAttr(char* attr_c, void* arg);
int libmime_ClearParam(char* param_c, void* arg);
int libmime_ClearSpecials(pTObjData ptod);

int libmime_WriteAttrParam(pFile fd, pMimeHeader msg, char* attrName, char* paramName, int datatype, pObjData val);

/** mime_encode.c **/
int libmime_EncodeQP();
int libmime_DecodeQP();
int libmime_EncodeBase64();
int libmime_DecodeBase64();

#endif /** _MIME_H **/
