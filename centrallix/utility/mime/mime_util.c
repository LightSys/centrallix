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

/*  libmime_Cleanup
**
**  Deallocates all memory used for the mime message
*/
void
libmime_Cleanup(pMimeMsg msg)
    {
    // FIXME FIXME
    // Do cleanup stuff.. memory leaking now!
    return;
    }

/*  libmime_StringLTrim
**
**  Trims whitespace off the left side of a string.
*/
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

/*  libmime_StringRTrim
**
**  Trims whitespace off the right side of a string.
*/
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

/*  libmime_StringTrim
**
**  Trims whitespace off both sides of a string.
*/
int
libmime_StringTrim(char *str)
    {
    libmime_StringLTrim(str);
    libmime_StringRTrim(str);

    return 0;
    }

/*  libmime_StringFirstCaseCmp
**
**  Checks if the first part of the given string matches the second string.
**  This function is case insensitive.
*/
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

/*
**  libmime_PrintAddrList
*/
int
libmime_PrintAddressList(pXArray ary, int level)
    {
    int i,j;
    pEmailAddr itm;

    if (level > 8) return 0;
    if (level==0) printf("-----------------------------------------------------\n");
    for (i=0; i < ary->nItems; i++)
	{
	itm = (pEmailAddr)xaGetItem(ary, i);
	for (j=0; j < level; j++)
	    {
	    printf("   ");
	    }
	if (itm->Group == NULL)
	    {
	    printf("%s@%s", itm->Mailbox, itm->Host);
	    if (strlen(itm->Display))
		printf(" (%s)", itm->Display);
	    printf("\n");
	    }
	else
	    {
	    printf("GROUP: \"%s\"\n", itm->Display);
	    libmime_PrintAddressList(itm->Group, level+1);
	    }
	}
    if (level==0) printf("-----------------------------------------------------\n");
    return 0;
    }

/*  libmime_StringUnquote
**
**  Internal function used to unquote strings if they are quoted.
*/
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
