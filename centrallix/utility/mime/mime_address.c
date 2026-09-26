/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2015 LightSys Technology Services, Inc.		*/
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
#include "cxlib/strtcpy.h"
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
    int count=0, state=MIME_ST_NORM, prev_state=MIME_ST_NORM, new_state, done=0, cnest=0;
    char *s_ptr, *e_ptr;
    char *t_str = NULL;
    char ch;
    char tmp_ch;
    pEmailAddr p_addr = NULL;

	libmime_StringTrim(buf);
	s_ptr = buf;
	while (!done)
	    {
	    ch = buf[count];
	    new_state = state;
	    switch (state)
		{
		/** If no special states are set, then check for comma delimiters and such **/
		case MIME_ST_NORM:
		    if (ch == '"')
			{
			new_state = MIME_ST_QUOTE;
			}
		    else if (ch == '(')
			{
			cnest++;
			new_state = MIME_ST_COMMENT;
			}
		    else if (ch == ':' && buf[count-1] != '\\')
			{
			new_state = MIME_ST_GROUP;
			}
		    else if (ch == ',' || count == strlen(buf))
			{
			e_ptr = buf+count;
			tmp_ch = *e_ptr;
			*e_ptr = '\0';
			t_str = (char*)nmSysStrdup(s_ptr);
			*e_ptr = tmp_ch;
			if (!t_str)
			    goto error;

			/** If we have a valid email address, parse it and add it to the list **/
			if (*t_str)
			    {
			    p_addr = (pEmailAddr)nmMalloc(sizeof(EmailAddr));
			    if (!p_addr)
				goto error;

			    /** If this fails, we ignore the failure and continue **/
			    if (libmime_ParseAddress(t_str, p_addr) < 0)
				nmFree(p_addr, sizeof(EmailAddr));
			    else
				xaAddItem(xary, p_addr);
			    }
			nmSysFree(t_str);
			t_str = NULL;
			s_ptr = e_ptr+1;
			}
		    break;

		case MIME_ST_GROUP:
		    if (ch == '"')
			{
			new_state = MIME_ST_QUOTE;
			}
		    else if (ch == '(')
			{
			cnest++;
			new_state = MIME_ST_COMMENT;
			}
		    else if (ch == ';' || count == strlen(buf))
			{
			e_ptr = buf+count;
			tmp_ch = *e_ptr;
			*e_ptr = '\0';
			t_str = (char*)nmSysStrdup(s_ptr);
			*e_ptr = tmp_ch;
			if (!t_str)
			    goto error;

			/** If we have a valid group, parse it and add it to the list **/
			if (*t_str)
			    {
			    p_addr = (pEmailAddr)nmMalloc(sizeof(EmailAddr));
			    if (!p_addr)
				goto error;

			    /** If this fails, we ignore the failure and continue **/
			    if (libmime_HdrParseGroup(t_str, p_addr) < 0)
				nmFree(p_addr, sizeof(EmailAddr));
			    else
				xaAddItem(xary, p_addr);
			    }
			nmSysFree(t_str);
			t_str = NULL;
			s_ptr = e_ptr+1;
			new_state = MIME_ST_NORM;
			}
		    break;

		/** If we are in a quote, ignore everything until the end quote is found **/
		case MIME_ST_QUOTE:
		    if (ch == '"' && buf[count-1] != '\\')
			{
			new_state = prev_state;
			}
		    break;

		/** If we are in a comment, ignore everything until the end of the comment is found **/
		case MIME_ST_COMMENT:
		    if (ch == ')' && buf[count-1] != '\\')
			{
			cnest--;
			if (!cnest)
			    {
			    new_state = prev_state;
			    }
			}
		    break;
		}
	    prev_state = state;
	    state = new_state;
	    if (count > strlen(buf))
		done = 1;
	    count++;
	    }

	return 0;

    error:
	if (t_str)
	    nmSysFree(t_str);
	return -1;
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
    int count=0, done=0, ncount=0;
    char *s_ptr, *e_ptr;
    char *t_str = NULL;
    char ch;
    pXArray p_xary = NULL;

	libmime_StringTrim(buf);
	p_xary = (pXArray)nmMalloc(sizeof(XArray));
	if (!p_xary)
	    goto error;
	xaInit(p_xary, 8);

	addr->Host[0] = 0;
	addr->Mailbox[0] = 0;
	addr->Display[0] = 0;
	addr->Group = p_xary;
	strtcpy(addr->AddressLine, buf, sizeof(addr->AddressLine));

	/**  Parse out the displayable name of this group  
	 **  A group should be in the following format or similar, according to
	 **  RFC2822, section 3.4 (Address Specification):
	 **    "Group Name": addr1@myhost.com, addr2@myhost.com;
	 **/
	s_ptr = buf;
	e_ptr = strchr(buf, ':');
	if (e_ptr - s_ptr > 0)
	    {
	    *e_ptr = '\0';
	    strtcpy(addr->Display, s_ptr, sizeof(addr->Display));
	    *e_ptr = ':';
	    libmime_StringTrim(addr->Display);
	    }
	else
	    {
	    return 0;
	    }
	if (addr->Display[0] == '"')
	    {
	    t_str = (char*)nmSysMalloc(strlen(addr->Display)+1);
	    if (!t_str)
		goto error;
	    count = 1;
	    ncount = 0;
	    while (!done && count < strlen(addr->Display))
		{
		ch = addr->Display[count];
		if (ch == '\\' && count+1<strlen(addr->Display))
		    {
		    t_str[ncount++] = addr->Display[count+1];
		    count++;
		    }
		else if (ch == '"')
		    {
		    done = 1;
		    }
		else
		    {
		    t_str[ncount++] = addr->Display[count];
		    }
		count++;
		}
	    t_str[ncount++] = '\0';
	    strtcpy(addr->Display, t_str, sizeof(addr->Display));
	    nmSysFree(t_str);
	    t_str = NULL;
	    }

	/**  Parse out the individual addresses  **/
	t_str = (char*)nmSysStrdup(e_ptr+1);
	if (!t_str)
	    goto error;
	if (libmime_ParseAddressList(t_str, addr->Group) < 0)
	    goto error;
	nmSysFree(t_str);
	t_str = NULL;

	return 0;

    error:
	if (p_xary)
	    {
	    addr->Group = NULL;
	    xaDeInit(p_xary);
	    nmFree(p_xary, sizeof(XArray));
	    }
	if (t_str)
	    nmSysFree(t_str);
	return -1;
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
    char *s_ptr, *e_ptr, *t_str;
    int rval;

    libmime_StringTrim(buf);
    addr->Host[0] = 0;
    addr->Mailbox[0] = 0;
    addr->Display[0] = 0;
    addr->Group = NULL;
    strtcpy(addr->AddressLine, buf, sizeof(addr->AddressLine));

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
	*e_ptr = '\0';
	rval = libmime_ParseAddressElements(s_ptr, addr);
	*e_ptr = '>';
	if (rval < 0)
	    return -1;

	/** Now, lets go through and see if we can find a displayable name **/
	if ((s_ptr = strchr(buf, '"')))
	    {
	    s_ptr++;
	    if ((e_ptr = strchr(s_ptr, '"')))
		{
		*e_ptr = '\0';
		strtcpy(addr->Display, s_ptr, sizeof(addr->Display));
		*e_ptr = '"';
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
		*e_ptr = '\0';
		strtcpy(addr->Display, s_ptr, sizeof(addr->Display));
		*e_ptr = ')';
		}
	    else
		{
		return -1;
		}
	    }
	else
	    {
	    /** Display text is whatever is outside the <> **/
	    t_str = (char*)nmSysMalloc(strlen(buf)+1);
	    if (!t_str)
		return -1;
	    s_ptr = strchr(buf, '<');
	    e_ptr = strchr(s_ptr, '>');

	    /** entire string is <user@host> **/
	    if (s_ptr == buf && e_ptr[1] == '\0') return 0;

	    /** Copy whatever is outside the < > **/
	    if (s_ptr > buf && s_ptr[-1] == ' ')
		s_ptr--;
	    if (e_ptr[1] == ' ')
		e_ptr++;
	    memcpy(t_str, buf, s_ptr-buf);
	    strcpy(t_str+(s_ptr-buf), e_ptr+1);
	    strtcpy(addr->Display, t_str, sizeof(addr->Display));
	    nmSysFree(t_str);
	    }
	}

    return 0;
    }

/*
**  int
**  libmime_ParseAddressElements(char *buf, EmailAddr *addr)
**     Parameters:
**         (char*) buf    A string that contains a single mailbox@host address.
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

    libmime_StringTrim(buf);
    if (!(ptr = strchr(buf, '@')))
	{
	strtcpy(addr->Mailbox, buf, sizeof(addr->Mailbox));
	addr->Host[0] = '\0';
	}
    else
	{
	*ptr = '\0';
	strtcpy(addr->Mailbox, buf, sizeof(addr->Mailbox));
	strtcpy(addr->Host, ptr+1, sizeof(addr->Host));
	*ptr = '@';
	}
    return 0;
    }
