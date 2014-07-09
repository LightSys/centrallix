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
#include <ctype.h>
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "obj.h"
#include "mime.h"

/*** libmime_AllocateHeader
 ***
 *** Allocates all memory used for the mime header
 ***/
pMimeHeader
libmime_AllocateHeader()
    {
    pMimeHeader msg;

	msg = nmMalloc(sizeof(MimeHeader));
	if (!msg) return NULL;
	memset(msg, 0, sizeof(MimeHeader));

	xaInit(&msg->Parts, 8);
	xhInit(&msg->Attrs, 17, 0);

    return msg;
    }

/*** libmime_Cleanup_Header
 ***
 *** Deallocates all memory used for the mime header
 ***/
void
libmime_DeallocateHeader(pMimeHeader msg)
    {
    int i;

	for (i = 0; i < msg->Parts.nItems; i++)
	    {
	    libmime_DeallocateHeader(msg->Parts.Items[i]);
	    }
	xaDeInit(&msg->Parts);

	if (msg->Sender) nmFree(msg->Sender, sizeof(EmailAddr));

	/** Clear the attributes. **/
	xhClear(&msg->Attrs, &libmime_ClearAttr, NULL);
	libmime_xhDeInit(&msg->Attrs);

	nmFree(msg, sizeof(MimeHeader));

    return;
    }

/***  libmime_StringLTrim
 ***
 ***  Trims whitespace off the left side of a string.
 ***/
int
libmime_StringLTrim(char *str)
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

/***  libmime_StringRTrim
 ***
 ***  Trims whitespace off the right side of a string.
 ***/
int
libmime_StringRTrim(char *str)
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

/***  libmime_StringTrim
 ***
 ***  Trims whitespace off both sides of a string.
 ***/
int
libmime_StringTrim(char *str)
    {
    libmime_StringLTrim(str);
    libmime_StringRTrim(str);

    return 0;
    }

/***  libmime_StringFirstCaseCmp
 ***
 ***  Checks if the first part of the given string matches the second string.
 ***  This function is case insensitive.
 ***/
int
libmime_StringFirstCaseCmp(char *s1, char *s2)
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

/***
 ***  libmime_PrintAddrList
 ***/
int
libmime_PrintAddressList(pXArray xary, int level)
    {
    int i,j;
    pEmailAddr addr;

    for (i=0; i < xaCount(xary); i++)
	{
	addr = xaGetItem(xary, i);
	if (i || level)
	    {
	    printf("                ");
	    for (j=0; j < level; j++)
		printf("    ");
	    }
	if (addr->Group)
	    {
	    printf("[GROUP: %s]\n", addr->Display);
	    libmime_PrintAddressList(addr->Group, level+1);
	    }
	else
	    {
	    if (strlen(addr->Display))
		printf("\"%s\" ", addr->Display);
	    printf("%s@%s\n", addr->Mailbox, addr->Host);
	    }
	}
    return 0;
    }

/***  libmime_StringUnquote
 ***
 ***  Internal function used to unquote strings if they are quoted.
 ***/
char*
libmime_StringUnquote(char *str)
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

/***  libmime_B64Purify
 ***
 ***  Removes all characters from a Base64 string that are not part
 ***  of the Base64 alphabet.
 ***/
int
libmime_B64Purify(char *string)
    {
    char *wrk, *tmp;
    static char allowset[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
    int rem=0;

    for (wrk = string; *wrk;)
	{
	if (strchr(allowset, *wrk) != NULL)
	    wrk++;
	else
	    {
	    char ch = *wrk;
	    for (tmp = wrk; tmp;)
		{
		memmove(tmp, tmp+1, strlen(tmp+1)+1);
		tmp = strchr(tmp, ch);
		rem++;
		}
	    }
	}
    return rem;
    }

/***  libmime_ContentExtension
 ***
 ***  Modifies the first parameter to contain the three leter extension
 ***  that is associated with the given content type and subtype.
 ***/
int
libmime_ContentExtension(char *str, int type, char *subtype)
    {
    switch (type)
	{
	case MIME_TYPE_TEXT:
	    if (!strlen(subtype) || !strcasecmp(subtype, "plain"))
		strcpy(str, "txt");
	    else if (!strcasecmp(subtype, "html"))
		strcpy(str, "html");
	    else if (!strcasecmp(subtype, "richtext"))
		strcpy(str, "rtf");
	    else
		return 0;
	    break;
	case MIME_TYPE_MULTIPART:
	    return 0;
	    break;
	case MIME_TYPE_APPLICATION:
	    return 0;
	    break;
	case MIME_TYPE_MESSAGE:
	    if (!strlen(subtype) || !strcasecmp(subtype, "rfc822"))
		strcpy(str, "msg");
	    else
		return 0;
	    break;
	case MIME_TYPE_IMAGE:
	    if (!strcasecmp(subtype, "jpg") || !strcasecmp(subtype, "jpeg"))
		strcpy(str, "jpg");
	    else if (!strcasecmp(subtype, "png"))
		strcpy(str, "png");
	    else if (!strcasecmp(subtype, "gif"))
		strcpy(str, "gif");
	    else if (!strcasecmp(subtype, "pbm"))
		strcpy(str, "pbm");
	    else
		return 0;
	    break;
	case MIME_TYPE_AUDIO:
	    if (!strcasecmp(subtype, "wav"))
		strcpy(str, "wav");
	    else if (!strcasecmp(subtype, "basic"))
		strcpy(str, "au");
	    else
		return 0;
	    break;
	case MIME_TYPE_VIDEO:
	    return 0;
	    break;
	}
    return 1;
    }

/***
 ***  libmime_StringToLower
 ***
 ***  Converts a string to lower case
 ***/
int
libmime_StringToLower(char *str)
    {
    char *ptr = str;

    while (*ptr)
	{
	*ptr = tolower(*ptr);
	ptr++;
	}

    return 0;
    }

/*** libmime_xhLookup - Wraps the xhLookup function in order to preserve
 *** consistency since MIME is case insensitive.
 ***
 *** NOTE: Any other string sanitization for hash keys may be performed here.
 ***/
void*
libmime_xhLookup(pXHashTable this, char* key)
    {
    void* rval;
    char* buf;

	buf = nmSysStrdup(key);

	libmime_StringToLower(buf);

	rval = xhLookup(this, buf);

    return rval;
    }

/*** libmime_xhAdd - Wraps the xhAdd function in order to preserve
 *** consistency since MIME is case insensitive.
 ***
 *** NOTE: Any other string sanitization for hash keys may be performed here.
 ***/
int
libmime_xhAdd(pXHashTable this, char* key, char* data)
    {
    int rval;
    char* buf;

	buf = nmSysStrdup(key);

	libmime_StringToLower(buf);

	rval = xhAdd(this, buf, data);

    return rval;
    }

/*** libmime_xhDeInit - Deallocate anything necessary to save memory.
 ***/
int
libmime_xhDeInit(pXHashTable this)
    {
    pXHashEntry entry = NULL;

	/** While we have elements, do the deallocation stuff. **/
	entry = xhGetNextElement(this, NULL);
	while (entry)
	    {
	    nmSysFree(entry->Key);
	    entry = xhGetNextElement(this, entry);
	    }

	xhDeInit(this);
    return 0;
    }

/*** libmime_SaveTemporaryFile - Write the temp file.
 ***/
int
libmime_SaveTemporaryFile(pFile fd, pObject obj, int truncSeek)
    {
    pPathname path = NULL;
    char buf[MIME_BUFSIZE+1];
    pObjSession session = NULL;

	/** Allocate our local path. **/
	path = (pPathname)nmMalloc(sizeof(Pathname));
	if (!path)
	    {
	    mssError(1, "MIME", "Could not allocate the pathname.");
	    goto error;
	    }
	memset(path, 0, sizeof(Pathname));

	/** Copy the object's path into our local path. **/
	obj_internal_CopyPath(path, obj->Pathname);

	/** Get the session. **/
	session = obj->Session;

	/** Get the path. **/
	path = obj_internal_PathPart(path, 0, obj->SubPtr);
	if (!path)
	    {
	    mssError(1, "MIME", "Could not copy pathname.");
	    goto error;
	    }

	/** Copy the file. **/
	memset(buf, 0, MIME_BUFSIZE + 1);
	fdRead(fd, NULL, 0, truncSeek, FD_U_SEEK) > 0;
	objWrite(obj->Prev, 0, 0, truncSeek, OBJ_U_SEEK | OBJ_U_TRUNCATE);
	while (fdRead(fd, buf, MIME_BUFSIZE, 0, 0) > 0)
	    {
	    objWrite(obj->Prev, buf, strlen(buf), 0, OBJ_U_TRUNCATE);
	    memset(buf, 0, MIME_BUFSIZE + 1);
	    }

	/** Deallocate the pathname. **/
	nmFree(path, sizeof(Pathname));

    return 0;

    error:
	if (path) nmFree(path, sizeof(Pathname));

	return -1;
    }

/***
 *** libmime_internal_MakeARandomFilename - Append on random chars to name.
 ***/
int
libmime_internal_MakeARandomFilename(char* name, int len)
    {
    int i;
    int nameLen = strlen(name);

	for (i = nameLen; i < nameLen + len; i++)
	    {
	    name[i] = 'a' + (random() % 26);
	    }
	name[i] = '\0';

    return 0;
    }
