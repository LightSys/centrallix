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
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "obj.h"
#include "mime.h"

/*
**  int
**  libmime_ParseAddressList(char* buf, XArray *ary);
**     Parameters:
**         (char*) buf     A string that has been cleaned up by the HdrParse
**                         function.  It is assumed that this string is clean and
**                         that it conforms to the standards in RFC822.
**         (XArray*) ary   A pointer to an XArray data structure.  This XArray will
**                         be populated with pEmailAddr structures that represent
**                         individual email addresses or groups of addresses.
**     Returns:
**         This function returns -1 on failure, and 0 on success.  It will split
**         addresses out into the desired data structure and will place it in the
**         second parameter.
**
**     NOTES:
**         This functionality is described in great detail in sections 3.4 and
**         3.4.1 of RFC2822.  I have attempted to implement that functionality.  If
**         you find inconsistencies, please align with the RFC unless otherwise
**         noted by a comment.
*/

int
libmime_ParseAddressList(char *buf, pXArray xary)
    {
    int count=0, state=MIME_ST_NORM, prev_state=MIME_ST_NORM, flag=0, cnest=0, tmp;
    char *s_ptr, *e_ptr, *t_str;
    char ch;
    pEmailAddr p_addr;

    libmime_StringTrim(buf);
    s_ptr = buf;
    while (flag == 0)
	{
	ch = buf[count];
	switch (state)
	    {
	    /** If no special states are set, then check for comma delimiters and such **/
	    case MIME_ST_NORM:
		if (ch == '"')
		    {
		    prev_state = state;
		    state = MIME_ST_QUOTE;
		    }
		else if (ch == '(')
		    {
		    cnest++;
		    prev_state = state;
		    state = MIME_ST_COMMENT;
		    }
		else if (ch == ':' && buf[count-1] != '\\')
		    {
		    prev_state = state;
		    state = MIME_ST_GROUP;
		    }
		else if (ch == ',' || count == strlen(buf))
		    {
		    e_ptr = &buf[count];
		    t_str = (char*)nmMalloc(e_ptr - s_ptr + 1);
		    strncpy(t_str, s_ptr, e_ptr-s_ptr);
		    t_str[e_ptr-s_ptr] = 0;
		    /**  If we have a valid email address, parse it and add it to the list **/
		    if (strlen(t_str))
			{
			p_addr = (pEmailAddr)nmMalloc(sizeof(EmailAddr));
			if (!libmime_ParseAddress(t_str, p_addr))
			    xaAddItem(xary, p_addr);
			else
			    nmFree(p_addr, sizeof(EmailAddr));
			}
		    nmFree(t_str, e_ptr - s_ptr + 1);
		    s_ptr = e_ptr+1;
		    }
		break;
	    case MIME_ST_GROUP:
		if (ch == '"')
		    {
		    prev_state = state;
		    state = MIME_ST_QUOTE;
		    }
		else if (ch == '(')
		    {
		    cnest++;
		    prev_state = state;
		    state = MIME_ST_COMMENT;
		    }
		else if (ch == ';' || count == strlen(buf))
		    {
		    e_ptr = &buf[count];
		    t_str = (char*)nmMalloc(e_ptr - s_ptr + 1);
		    strncpy(t_str, s_ptr, e_ptr-s_ptr);
		    t_str[e_ptr-s_ptr] = 0;
		    /**  If we have a valid group, parse it and add it to the list **/
		    if (strlen(t_str))
			{
			p_addr = (pEmailAddr)nmMalloc(sizeof(EmailAddr));
			if (!libmime_HdrParseGroup(t_str, p_addr))
			    xaAddItem(xary, p_addr);
			else
			    nmFree(p_addr, sizeof(EmailAddr));
			}
		    nmFree(t_str, e_ptr - s_ptr + 1);
		    s_ptr = e_ptr+1;
		    prev_state = state;
		    state = MIME_ST_NORM;
		    }
	    /** If we are in a quote, ignore everything until the end quote is found **/
	    case MIME_ST_QUOTE:
		if (ch == '"' && buf[count-1] != '\\')
		    {
		    tmp = prev_state;
		    prev_state = state;
		    state = tmp;
		    }
		break;
	    /** If we are in a comment, ignore everything until the end of the comment is found **/
	    case MIME_ST_COMMENT:
		if (ch == ')' && buf[count-1] != '\\')
		    {
		    cnest--;
		    if (!cnest)
			{
			tmp = prev_state;
			prev_state = state;
			state = tmp;
			}
		    }
		break;
	    }
	if (count > strlen(buf))
	    flag = 1;
	count++;
	}
    return 0;
    }

/*
**  int
**  libmime_HdrParseGroup(char *buf, EmailAddr* addr);
**     Parameters:
**         (char*) buf     A string that represents a single email address.  This 
**                         is usually one of the XArray elements that was returned
**                         from the ParseAddressSplit function.
**         (EmailAddr*) addr
**                         An email address data structure that will be filled in 
**                         by this function.  Since this is defining a group of 
**                         addresses, this will mainly be a pointer structure to 
**                         other addresses and groups.
**
**     Returns:
**         This function returns -1 on failure and 0 on success.  It will split a
**         group into its different parts as per RFC2822 and fill in the EmailAddr
**         structure.
*/

int
libmime_HdrParseGroup(char *buf, pEmailAddr addr)
    {
    int count=0, flag=0, ncount=0, len, err;
    char *s_ptr, *e_ptr, *t_str;
    char ch;
    pXArray p_xary;

    libmime_StringTrim(buf);
    p_xary = (pXArray)nmMalloc(sizeof(XArray));
    xaInit(p_xary, sizeof(EmailAddr));

    addr->Host[0] = 0;
    addr->Mailbox[0] = 0;
    addr->Display[0] = 0;
    addr->Group = p_xary;
    strncpy(addr->AddressLine, buf, 255);
    addr->AddressLine[255] = 0;

    /**  Parse out the displayable name of this group  
     **  A group should be in the following format or similar, according to
     **  RFC2822, section 3.4 (Address Specification):
     **    "Group Name": addr1@myhost.com, addr2@myhost.com;
     **/
    s_ptr = buf;
    e_ptr = strchr(buf, ':');
    if (e_ptr - s_ptr > 0)
	{
	len = (e_ptr-s_ptr<127?e_ptr-s_ptr:127);
	strncpy(addr->Display, s_ptr, len);
	addr->Display[len] = 0;
	libmime_StringTrim(addr->Display);
	}
    else
	{
	return 0;
	}
    if (addr->Display[0] == '"')
	{
	t_str = (char*)nmMalloc(strlen(addr->Display)+1);
	count = 1;
	ncount = 0;
	while (flag == 0 && count < strlen(addr->Display))
	    {
	    ch = addr->Display[count];
	    if (ch == '\\' && count+1<strlen(addr->Display))
		{
		t_str[ncount++] = addr->Display[count+1];
		count++;
		}
	    else if (ch == '"')
		{
		flag = 1;
		}
	    else
		{
		t_str[ncount++] = addr->Display[count];
		}
	    count++;
	    }
	strncpy(addr->Display, t_str, 127);
	addr->Display[127] = 0;
	nmFree(t_str, strlen(addr->Display)+1);
	}

    /**  Parse out the individual addresses  **/
    t_str = (char*)nmMalloc(strlen(e_ptr+1)+1);
    strncpy(t_str, e_ptr+1, strlen(e_ptr+1));
    t_str[strlen(e_ptr+1)+1] = 0;
    err = libmime_ParseAddressList(t_str, addr->Group);
    nmFree(t_str, strlen(e_ptr+1)+1);
    return err;
    }

/*
**  int
**  libmime_ParseAddress(char *buf, EmailAddr* addr);
**     Parameters:
**         (char*) buf     A string that represents a single email address.  This 
**                         is usually one of the XArray elements that was returned
**                         from the ParseAddressSplit function.
**         (EmailAddr*) addr
**                         An email address data structure that will be filled in 
**                         by this function.  These properties will be stripped of
**                         all their quotes and extranious whitespace.--
**     Returns:
**         This function returns -1 on failure and 0 on success.  It will split an
**         individual email address or a group of addresses up into their various
**         parts.  and return an EmailAddr structure.
*/

int
libmime_ParseAddress(char *buf, pEmailAddr addr)
    {
    int len=0;
    char *s_ptr, *e_ptr, *t_str;

    libmime_StringTrim(buf);
    addr->Host[0] = 0;
    addr->Mailbox[0] = 0;
    addr->Display[0] = 0;
    addr->Group = NULL;
    strncpy(addr->AddressLine, buf, 255);
    addr->AddressLine[255] = 0;

    /*
    **  First, get the email address itself which can be displayed in two forms,
    **  either inside angle-brackets (<>), or not.  If it is not inside angle 
    **  brackets, then we are done...the address is all there is.  If it IS inside
    **  the brackets, then we need to check for the other parts as well.
    */
    if (!(s_ptr = strchr(buf, '<')))
	{
	libmime_ParseAddressElements(buf, addr); /* set Host and Mailbox */
	}
    /** If this address DOES have <>'s, then treat it like anormal address **/
    else
	{
	/** First, get the <mailbox@host> part parsed out of there **/
	s_ptr++;
	if (!(e_ptr = strchr(buf, '>')))
	    {
	    return -1;
	    }
	t_str = (char*)nmMalloc((e_ptr - s_ptr)+1);
	strncpy(t_str, s_ptr, e_ptr-s_ptr);
	t_str[e_ptr-s_ptr] = 0;
	libmime_ParseAddressElements(t_str, addr); /* set Host and Mailbox */
	nmFree(t_str, (e_ptr-s_ptr)+1);

	/** Now, lets go through and see if we can find a displayable name **/
	if ((s_ptr = strchr(buf, '"')))
	    {
	    s_ptr++;
	    if ((e_ptr = strchr(s_ptr, '"')))
		{
		len = (e_ptr-s_ptr<127?e_ptr-s_ptr:127);
		memcpy(addr->Display, s_ptr, len);
		addr->Display[len] = 0;
		}
	    else
		{
		return -1;
		}
	    }
	else if ((s_ptr = strchr(buf, '(')))
	    {
	    s_ptr++;
	    if ((e_ptr = strchr(s_ptr, ')')))
		{
		len = (e_ptr-s_ptr<127?e_ptr-s_ptr:127);
		memcpy(addr->Display, s_ptr, len);
		addr->Display[len] = 0;
		}
	    else
		{
		return -1;
		}
	    }
	else
	    {
	    t_str = (char*)nmMalloc(strlen(buf)+1);
	    s_ptr = strchr(buf, '<');
	    e_ptr = strchr(s_ptr, '>');
	    if (s_ptr == buf && e_ptr == buf+strlen(buf)) return 0;
	    if (s_ptr > buf)
		s_ptr--;
	    if (e_ptr < buf+strlen(buf))
		e_ptr++;
	    memcpy(t_str, buf, s_ptr-buf);
	    strcpy(t_str+(s_ptr-buf), e_ptr);
	    strncpy(addr->Display, t_str, strlen(t_str));
	    nmFree(t_str, strlen(buf)+1);
	    }
	}

    return 0;
    }

/*
**  int
**  libmime_ParseAddressElements(char *buf, EmailAddr *addr)
**     Parameters:
**         (char*) buf    A string that reperess a single mailbox@host address.
**         (EmailAddr*) addr
**                        An email address data structure that will be filled in
**                        by this function.  This function will set the Host and
**                        Mailbox properties if they are available in the buffer.
**     Returns:
**         This function parses Host and Mailbox out of the buffer and places
**         them in the addr->Host and addr->Mailbox properties.
*/
int
libmime_ParseAddressElements(char *buf, pEmailAddr addr)
    {
    char *ptr;
    int len;

    libmime_StringTrim(buf);
    if (!(ptr = strchr(buf, '@')))
	{
	strncpy(addr->Mailbox, buf, 127);
	addr->Mailbox[127] = 0;
	}
    else
	{
	len = (ptr-buf<127?ptr-buf:127);
	memcpy(addr->Mailbox, buf, len);
	addr->Mailbox[len] = 0;
	ptr++;
	strncpy(addr->Host, ptr, 127);
	addr->Host[127] = 0;
	}
    return 0;
    }
