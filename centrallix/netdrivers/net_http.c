#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include "centrallix.h"
#include "mtask.h"
#include "mtsession.h"
#include "xarray.h"
#include "xhash.h"
#include "mtlexer.h"
#include "exception.h"
#include "obj.h"
#include "stparse_ne.h"
#include "stparse.h"
#include "htmlparse.h"
#include "xhandle.h"
#include "magic.h"

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
/* Module: 	net_http.c              				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 8, 1998  					*/
/* Description:	Network handler providing an HTTP interface to the 	*/
/*		Centrallix and the ObjectSystem.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: net_http.c,v 1.16 2002/05/01 02:20:31 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/netdrivers/net_http.c,v $

    $Log: net_http.c,v $
    Revision 1.16  2002/05/01 02:20:31  gbeeley
    Modification in net_http: ls__req=close now allows multiple object
    ids to be strung together in the ls__oid parameter.

    Revision 1.15  2002/04/25 22:50:00  gbeeley
    Added ability to reference default session in ls__mode=osml operations
    instead of having to create a new one.  Note that the session is the
    transaction context, so be careful just using the default (which
    applies to all connections from the user's browser).  Set the ls__sid
    to "XDEFAULT" to use the default.

    Revision 1.14  2002/04/25 19:29:30  gbeeley
    Added handle support to object ids and query ids in the OSML over HTTP
    communication mechanism.

    Revision 1.13  2002/04/25 18:01:15  gbeeley
    Started adding Handle abstraction in net_http.c.  Testing first with
    just handlized ObjSession structures.

    Revision 1.12  2002/03/23 05:34:26  gbeeley
    Added "pragma: no-cache" headers to the "osml" mode responses to help
    avoid browser caching of that dynamic data.

    Revision 1.11  2002/03/23 05:09:16  gbeeley
    Fixed a logic error in net_http's ls__startat osml feature.  Improved
    OSML error text.

    Revision 1.10  2002/03/23 03:52:54  gbeeley
    Fixed a potential security blooper when the cookie was copied to a tmp
    buffer.

    Revision 1.9  2002/03/23 03:41:02  gbeeley
    Fixed the ages-old problem in net_http.c where cookies weren't anchored
    at the / directory, so zillions of sessions might be created...

    Revision 1.8  2002/03/23 01:30:44  gbeeley
    Added ls__startat option to the osml "queryfetch" mechanism, in the
    net_http.c driver.  Set ls__startat to the number of the first object
    you want returned, where 1 is the first object (in other words,
    ls__startat-1 objects will be skipped over).  Started to add a LIMIT
    clause to the multiquery module, but thought this would be easier and
    just as effective for now.

    Revision 1.7  2002/03/16 06:50:20  gbeeley
    Changed some sprintfs to snprintfs, just for safety's sake.

    Revision 1.6  2002/03/16 04:26:25  gbeeley
    Added functionality in net_http's object access routines so that it,
    when appropriate, sends the metadata attributes also, including the
    following:  "name", "inner_type", "outer_type", and "annotation".

    Revision 1.5  2002/02/14 00:55:20  gbeeley
    Added configuration file centrallix.conf capability.  You now MUST have
    this file installed, default is /usr/local/etc/centrallix.conf, in order
    to use Centrallix.  A sample centrallix.conf is found in the centrallix-os
    package in the "doc/install" directory.  Conf file allows specification of
    file locations, TCP port, server string, auth realm, auth method, and log
    method.  rootnode.type is now an attribute in the conf file instead of
    being a separate file, and thus is no longer used.

    Revision 1.4  2001/11/12 20:43:44  gbeeley
    Added execmethod nonvisual widget and the audio /dev/dsp device obj
    driver.  Added "execmethod" ls__mode in the HTTP network driver.

    Revision 1.3  2001/10/16 23:53:01  gbeeley
    Added expressions-in-structure-files support, aka version 2 structure
    files.  Moved the stparse module into the core because it now depends
    on the expression subsystem.  Almost all osdrivers had to be modified
    because the structure file api changed a little bit.  Also fixed some
    bugs in the structure file generator when such an object is modified.
    The stparse module now includes two separate tree-structured data
    structures: StructInf and Struct.  The former is the new expression-
    enabled one, and the latter is a much simplified version.  The latter
    is used in the url_inf in net_http and in the OpenCtl for objects.
    The former is used for all structure files and attribute "override"
    entries.  The methods for the latter have an "_ne" addition on the
    function name.  See the stparse.h and stparse_ne.h files for more
    details.  ALMOST ALL MODULES THAT DIRECTLY ACCESSED THE STRUCTINF
    STRUCTURE WILL NEED TO BE MODIFIED.

    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:00:57  gbeeley
    Centrallix Core initial import

    Revision 1.3  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.2  2001/08/07 02:49:25  gbeeley
    Changed cookie from =LS-xxxx to LSID=LS-xxxx

    Revision 1.1.1.1  2001/08/07 02:31:22  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** This structure is used for wait-for-conn-to-finish ***/
typedef struct
    {
    int		TriggerID;
    pSemaphore	TriggerSem;
    int		LinkCnt;
    }
    NhtConnTrigger, *pNhtConnTrigger;

/*** This is used to keep track of user/password/cookie information ***/
typedef struct
    {
    char	Username[32];
    char	Password[32];
    char	Cookie[32];
    void*	Session;
    int		IsNewCookie;
    pObjSession	ObjSess;
    pSemaphore	Errors;
    XArray	ErrorList;	/* xarray of xstring */
    XArray	Triggers;	/* xarray of pNhtConnTrigger */
    HandleContext Hctx;
    }
    NhtSessionData, *pNhtSessionData;


/*** GLOBALS ***/
struct 
    {
    XHashTable	CookieSessions;
    XArray	Sessions;
    pFile	StdOut;
    char	ServerString[80];
    char	Realm[80];
    }
    NHT;

int nht_internal_UnConvertChar(int ch, char** bufptr, int maxlen);
extern int htrRender(pFile, pObject);

/*** nht_internal_ConstructPathname - constructs the proper OSML pathname
 *** for the open-object operation, given the apparent pathname and url
 *** parameters.  This primarily involves recovering the 'ls__type' setting
 *** and putting it back in the path.
 ***/
int
nht_internal_ConstructPathname(pStruct url_inf)
    {
    pStruct lstype_inf;
    char* oldpath;
    char* newpath;
    char* old_lstype;
    char* new_lstype;
    char* ptr;
    int len;

    	/** Does it have ls__type? **/
	if ((lstype_inf = stLookup_ne(url_inf,"ls__type")) != NULL)
	    {
	    oldpath = url_inf->StrVal;
	    stAttrValue_ne(lstype_inf,&old_lstype);

	    /** Get an encoded lstype **/
	    len = strlen(old_lstype)*3+1;
	    new_lstype = (char*)nmSysMalloc(len);
	    ptr = new_lstype;
	    while(*old_lstype) nht_internal_UnConvertChar(*(old_lstype++), &ptr, len - (ptr - new_lstype));
	    *ptr = '\0';

	    /** Build the new pathname **/
	    newpath = (char*)nmSysMalloc(strlen(oldpath) + 10 + strlen(new_lstype) + 1);
	    sprintf(newpath,"%s?ls__type=%s",oldpath,new_lstype);

	    /** set the new path and remove the old one **/
	    if (url_inf->StrAlloc) nmSysFree(url_inf->StrVal);
	    url_inf->StrVal = newpath;
	    url_inf->StrAlloc = 1;
	    }

    return 0;
    }


/*** nht_internal_StartTrigger - starts a page that has trigger information
 *** on it
 ***/
int
nht_internal_StartTrigger(pNhtSessionData sess, int t_id)
    {
    pNhtConnTrigger trg;

    	/** Make a new conn completion trigger struct **/
	trg = (pNhtConnTrigger)nmMalloc(sizeof(NhtConnTrigger));
	trg->LinkCnt = 1;
	trg->TriggerSem = syCreateSem(0, 0);
	trg->TriggerID = t_id;
	xaAddItem(&(sess->Triggers), (void*)trg);

    return 0;
    }


/*** nht_internal_EndTrigger - releases a wait on a page completion.
 ***/
int
nht_internal_EndTrigger(pNhtSessionData sess, int t_id)
    {
    pNhtConnTrigger trg;
    int i;

    	/** Search for the trigger **/
	for(i=0;i<sess->Triggers.nItems;i++)
	    {
	    trg = (pNhtConnTrigger)(sess->Triggers.Items[i]);
	    if (trg->TriggerID == t_id)
	        {
		syPostSem(trg->TriggerSem,1,0);
		trg->LinkCnt--;
		if (trg->LinkCnt == 0)
		    {
		    syDestroySem(trg->TriggerSem,0);
		    xaRemoveItem(&(sess->Triggers), i);
		    nmFree(trg, sizeof(NhtConnTrigger));
		    }
		break;
		}
	    }

    return 0;
    }


/*** nht_internal_WaitTrigger - waits on a trigger on a page.
 ***/
int
nht_internal_WaitTrigger(pNhtSessionData sess, int t_id)
    {
    pNhtConnTrigger trg;
    int i;

    	/** Search for the trigger **/
	for(i=0;i<sess->Triggers.nItems;i++)
	    {
	    trg = (pNhtConnTrigger)(sess->Triggers.Items[i]);
	    if (trg->TriggerID == t_id)
	        {
		trg->LinkCnt++;
		syGetSem(trg->TriggerSem, 1, 0);
		trg->LinkCnt--;
		if (trg->LinkCnt == 0)
		    {
		    syDestroySem(trg->TriggerSem,0);
		    xaRemoveItem(&(sess->Triggers), i);
		    nmFree(trg, sizeof(NhtConnTrigger));
		    }
		break;
		}
	    }

    return 0;
    }


/*** nht_internal_ErrorHandler - handle the printing of notice and error
 *** messages to the error stream for the client, if the client has such
 *** an error stream (which is how this is called).
 ***/
int
nht_internal_ErrorHandler(pNhtSessionData nsess, pFile net_conn)
    {
    pXString errmsg;
    char sbuf[256];

    	/** Wait on the errors semaphore **/
	syGetSem(nsess->Errors, 1, 0);

	/** Grab one error **/
	errmsg = (pXString)(nsess->ErrorList.Items[0]);
	xaRemoveItem(&nsess->ErrorList, 0);

	/** Format the error and print it as HTML. **/
	snprintf(sbuf,256,"HTTP/1.0 200 OK\r\n"
		     "Server: %s\r\n"
		     "\r\n"
		     "<HTML><BODY><PRE><A NAME=\"Message\">",NHT.ServerString);
	fdWrite(net_conn,sbuf,strlen(sbuf),0,0);
	fdWrite(net_conn,errmsg->String,strlen(errmsg->String),0,0);
	snprintf(sbuf,256,"</A></PRE></BODY></HTML>\r\n");
	fdWrite(net_conn,sbuf,strlen(sbuf),0,0);

	/** Discard the string **/
	xsDeInit(errmsg);
	nmFree(errmsg,sizeof(XString));

    return 0;
    }


/*** nht_internal_GenerateError - grab up the current error text listing
 *** and queue it on the outbound error queue for this session, so that
 *** the error stream reader in the DHTML client can pick it up and show
 *** it to the user.
 ***/
int
nht_internal_GenerateError(pNhtSessionData nsess)
    {
    pXString errmsg;

    	/** Pick up the error msg text **/
	errmsg = (pXString)nmMalloc(sizeof(XString));
	xsInit(errmsg);
	mssStringError(errmsg);

	/** Queue it and post to the semaphore **/
	xaAddItem(&nsess->ErrorList, (void*)errmsg);
	syPostSem(nsess->Errors, 1, 0);

    return 0;
    }


/*** nht_internal_Encode64 - encode a string to MIME base64 encoding.
 ***/
int
nht_internal_Encode64(unsigned char* dst, unsigned char* src, int maxdst)
    {
    static char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        /** Step through src 3 bytes at a time, generating 4 dst bytes for each 3 src **/
        while(*src)
            {
            /** First 6 bits of source[0] --> first byte dst. **/
            if (maxdst < 5) return -1;
            dst[0] = b64[src[0]>>2];

            /** Second dst byte from last 2 bits of src[0] and first 4 of src[1] **/
            if (src[1] == '\0')
                {
                dst[1] = b64[(src[0]&0x03)<<4];
                dst[2] = '=';
                dst[3] = '=';
                dst += 4;
                break;
                }
            dst[1] = b64[((src[0]&0x03)<<4) | (src[1]>>4)];

            /** Third dst byte from second 4 bits of src[1] and first 2 of src[2] **/
            if (src[2] == '\0')
                {
                dst[2] = b64[(src[1]&0x0F)<<2];
                dst[3] = '=';
                dst += 4;
                break;
                }
            dst[2] = b64[((src[1]&0x0F)<<2) | (src[2]>>6)];

            /** Last dst byte from last 6 bits of src[2] **/
            dst[3] = b64[(src[2]&0x3F)];

            /** Increment ctrs **/
            maxdst -= 4;
            dst += 4;
            src += 3;
            }

        /** Null-terminate the thing **/
        *dst = '\0';

    return 0;
    }


/*** nht_internal_Decode64 - decodes a MIME base64 encoded string.
 ***/
int
nht_internal_Decode64(char* dst, char* src, int maxdst)
    {
    static char b64[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char* ptr;
    int ix;

    	/** Step through src 4 bytes at a time. **/
	while(*src)
	    {
	    /** First 6 bits. **/
	    if (maxdst < 4) 
	        {
		mssError(1,"NHT","Could not decode HTTP data field - internal resources exceeded");
		return -1;
		}
	    ptr = strchr(b64,src[0]);
	    if (!ptr) 
	        {
		mssError(1,"NHT","Illegal protocol character in HTTP encoded data field");
		return -1;
		}
	    ix = ptr-b64;
	    dst[0] = ix<<2;

	    /** Second six bits are split between dst[0] and dst[1] **/
	    ptr = strchr(b64,src[1]);
	    if (!ptr)
	        {
		mssError(1,"NHT","Illegal protocol character in HTTP encoded data field");
		return -1;
		}
	    ix = ptr-b64;
	    dst[0] |= ix>>4;
	    dst[1] = (ix<<4)&0xF0;

	    /** Third six bits are nonmandatory and split between dst[1] and [2] **/
	    if (src[2] == '=' && src[3] == '=')
	        {
		maxdst -= 1;
		dst += 1;
		src += 4;
		break;
		}
	    ptr = strchr(b64,src[2]);
	    if (!ptr)
	        {
		mssError(1,"NHT","Illegal protocol character in HTTP encoded data field");
		return -1;
		}
	    ix = ptr-b64;
	    dst[1] |= ix>>2;
	    dst[2] = (ix<<6)&0xC0;

	    /** Fourth six bits are nonmandatory and a part of dst[2]. **/
	    if (src[3] == '=')
	        {
		maxdst -= 2;
		dst += 2;
		src += 4;
		break;
		}
	    ptr = strchr(b64,src[3]);
	    if (!ptr)
	        {
		mssError(1,"NHT","Illegal protocol character in HTTP encoded data field");
		return -1;
		}
	    ix = ptr-b64;
	    dst[2] |= ix;
	    maxdst -= 3;
	    src += 4;
	    dst += 3;
	    }

	/** Null terminate the destination **/
	dst[0] = 0;

    return 0;
    }


/*** nht_internal_CreateCookie - generate a random string value that can
 *** be used as an HTTP cookie.
 ***/
int
nht_internal_CreateCookie(char* ck)
    {
    /*int i;
    	printf("CreateCk called, stack ptr = %8.8X\n",&i);*/
    sprintf(ck,"LSID=LS-%6.6X%4.4X", (((int)(time(NULL)))&0xFFFFFF), (((int)(lrand48()))&0xFFFF));
    return 0;
    }



/*** nht_internal_UnConvertChar - convert a character back to its escaped
 *** notation such as %xx or +.
 ***/
int
nht_internal_UnConvertChar(int ch, char** bufptr, int maxlen)
    {
    static char hex[] = "0123456789abcdef";

    	/** Room for at least one char? **/
	if (maxlen == 0) return -1;

	/** If space, convert to plus **/
	if (ch == ' ') *((*bufptr)++) = '+';

	/** Else if special char, convert to %xx **/
	else if (ch == '/' || ch == '.' || ch == '?' || ch == '%' || ch == '&' || ch == '=')
	    {
	    if (maxlen < 3) return -1;
	    *((*bufptr)++) = '%';
	    *((*bufptr)++) = hex[(ch & 0xF0)>>4];
	    *((*bufptr)++) = hex[(ch & 0x0F)];
	    }

	/** Else convert directly **/
	else *((*bufptr)++) = (ch&0xFF);

    return 0;
    }


/*** nht_internal_EncodeHTML - encode a string such that it can cleanly
 *** be shown in HTML format (change ' ', '<', '>', '&' to &xxx; representation).
 ***/
int
nht_internal_EncodeHTML(int ch, char** bufptr, int maxlen)
    {

    	/** Room for at least one char? **/
	if (maxlen == 0) return -1;

	/** If special char, convert it. **/
	if (ch == ' ')
	    {
	    if (maxlen < 6) return -1;
	    strcpy(*bufptr, "&nbsp;");
	    (*bufptr) += 6;
	    }
	else if (ch == '<')
	    {
	    if (maxlen < 4) return -1;
	    strcpy(*bufptr, "&lt;");
	    (*bufptr) += 4;
	    }
	else if (ch == '>')
	    {
	    if (maxlen < 4) return -1;
	    strcpy(*bufptr, "&gt;");
	    (*bufptr) += 4;
	    }
	else if (ch == '&')
	    {
	    if (maxlen < 5) return -1;
	    strcpy(*bufptr, "&amp;");
	    (*bufptr) += 5;
	    }

	/** Else convert char directly **/
	else *((*bufptr)++) = (ch & 0xFF);

    return 0;
    }


/*** nht_internal_WriteOneAttr - put one attribute's information into the
 *** outbound data connection stream.
 ***/
int
nht_internal_WriteOneAttr(pNhtSessionData sess, pObject obj, pFile conn, handle_t tgt, char* attrname)
    {
    ObjData od;
    char* dptr;
    int type,rval;
    XString xs;
    char sbuf[100];
    static char* coltypenames[] = {"unknown","integer","string","double","datetime","intvec","stringvec","money",""};

	/** Get type and value **/
	xsInit(&xs);
	type = objGetAttrType(obj,attrname);
	if (type < 0) return -1;
	rval = objGetAttrValue(obj,attrname,&od);
	if (rval != 0) 
	    dptr = "";
	else if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE) 
	    dptr = objDataToStringTmp(type, (void*)&od, 0);
	else
	    dptr = objDataToStringTmp(type, (void*)(od.String), 0);
	if (!dptr) dptr = "";

	/** Write the HTML output. **/
	snprintf(sbuf,100,"<A TARGET=X" XHN_HANDLE_PRT " HREF='http://%.40s/#%s'>",tgt,attrname,coltypenames[type]);
	xsCopy(&xs,sbuf,-1);
	xsConcatenate(&xs,dptr,-1);
	xsConcatenate(&xs,"</A>\n",5);
	fdWrite(conn,xs.String,strlen(xs.String),0,0);
	xsDeInit(&xs);

    return 0;
    }


/*** nht_internal_WriteAttrs - write an HTML-encoded attribute list for the
 *** object to the connection, given an object and a connection.
 ***/
int
nht_internal_WriteAttrs(pNhtSessionData sess, pObject obj, pFile conn, handle_t tgt, int put_meta)
    {
    char* attr;

	/** Loop throught the attributes. **/
	if (put_meta)
	    {
	    nht_internal_WriteOneAttr(sess, obj, conn, tgt, "name");
	    nht_internal_WriteOneAttr(sess, obj, conn, tgt, "inner_type");
	    nht_internal_WriteOneAttr(sess, obj, conn, tgt, "outer_type");
	    nht_internal_WriteOneAttr(sess, obj, conn, tgt, "annotation");
	    }
	for(attr = objGetFirstAttr(obj); attr; attr = objGetNextAttr(obj))
	    {
	    nht_internal_WriteOneAttr(sess, obj, conn, tgt, attr);
	    }

    return 0;
    }



/*** nht_internal_OSML - direct OSML access from the client.  This will take
 *** the form of a number of different OSML operations available seemingly
 *** seamlessly (hopefully) from within the JavaScript functionality in an
 *** DHTML document.
 ***/
int
nht_internal_OSML(pNhtSessionData sess, pFile conn, pObject target_obj, char* request, pStruct req_inf)
    {
    char* ptr;
    char* newptr;
    pObjSession objsess;
    pObject obj;
    pObjQuery qy;
    char* sid = NULL;
    char sbuf[256];
    char hexbuf[3];
    int mode,mask;
    char* usrtype;
    int i,t,n,o,cnt,start;
    pStruct subinf;
    MoneyType m;
    DateTime dt;
    double dbl;
    char* where;
    char* orderby;
    handle_t session_handle;
    handle_t query_handle;
    handle_t obj_handle;

    	/** Choose the request to perform **/
	if (!strcmp(request,"opensession"))
	    {
	    objsess = objOpenSession(req_inf->StrVal);
	    if (!objsess) 
		session_handle = XHN_INVALID_HANDLE;
	    else
		session_handle = xhnAllocHandle(&(sess->Hctx), objsess);
	    snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X" XHN_HANDLE_PRT ">&nbsp;</A>\r\n",
		    session_handle);
	    fdWrite(conn, sbuf, strlen(sbuf), 0,0);
	    }
	else 
	    {
	    /** Get the session data **/
	    stAttrValue_ne(stLookup_ne(req_inf,"ls__sid"),&sid);
	    if (!sid) 
		{
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
	        fdWrite(conn, sbuf, strlen(sbuf), 0,0);
		mssError(1,"NHT","Session ID required for OSML request '%s'",request);
		return -1;
		}
	    if (!strcmp(sid,"XDEFAULT"))
		{
		session_handle = XHN_INVALID_HANDLE;
		objsess = sess->ObjSess;
		}
	    else
		{
		session_handle = xhnStringToHandle(sid+1,NULL,16);
		objsess = (pObjSession)xhnHandlePtr(&(sess->Hctx), session_handle);
		}
	    if (!objsess || !ISMAGIC(objsess, MGK_OBJSESSION))
		{
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
	        fdWrite(conn, sbuf, strlen(sbuf), 0,0);
		mssError(1,"NHT","Invalid Session ID in OSML request");
		return -1;
		}

	    /** Get object handle, as needed.  If the client specified an
	     ** oid, it had better be a valid one.
	     **/
	    if (stAttrValue_ne(stLookup_ne(req_inf,"ls__oid"),&ptr) < 0)
		{
		obj_handle = XHN_INVALID_HANDLE;
		}
	    else
		{
		obj_handle = xhnStringToHandle(ptr+1, NULL, 16);
		obj = (pObject)xhnHandlePtr(&(sess->Hctx), obj_handle);
		if (!obj || !ISMAGIC(obj, MGK_OBJECT))
		    {
		    snprintf(sbuf,256,"Content-Type: text/html\r\n"
			     "Pragma: no-cache\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
		    fdWrite(conn, sbuf, strlen(sbuf), 0,0);
		    mssError(1,"NHT","Invalid Object ID in OSML request");
		    return -1;
		    }
		}

	    /** Get the query handle, as needed.  If the client specified a
	     ** query handle, as with the object handle, it had better be a
	     ** valid one.
	     **/
	    if (stAttrValue_ne(stLookup_ne(req_inf,"ls__qid"),&ptr) < 0)
		{
		query_handle = XHN_INVALID_HANDLE;
		}
	    else
		{
		query_handle = xhnStringToHandle(ptr+1, NULL, 16);
		qy = (pObjQuery)xhnHandlePtr(&(sess->Hctx), query_handle);
		if (!qy || !ISMAGIC(qy, MGK_OBJQUERY))
		    {
		    snprintf(sbuf,256,"Content-Type: text/html\r\n"
			     "Pragma: no-cache\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
		    fdWrite(conn, sbuf, strlen(sbuf), 0,0);
		    mssError(1,"NHT","Invalid Query ID in OSML request");
		    return -1;
		    }
		}

	    /** Does this request require an object handle? **/
	    if (obj_handle == XHN_INVALID_HANDLE && (!strcmp(request,"close") || !strcmp(request,"objquery") ||
		!strcmp(request,"read") || !strcmp(request,"write") || !strcmp(request,"attrs") || 
		!strcmp(request, "setattrs")))
		{
		snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
			 "\r\n"
			 "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
		fdWrite(conn, sbuf, strlen(sbuf), 0,0);
		mssError(1,"NHT","Object ID required for OSML '%s' request", request);
		return -1;
		}

	    /** Does this request require a query handle? **/
	    if (query_handle == XHN_INVALID_HANDLE && (!strcmp(request,"queryfetch") || !strcmp(request,"queryclose")))
		{
		snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
			 "\r\n"
			 "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
		fdWrite(conn, sbuf, strlen(sbuf), 0,0);
		mssError(1,"NHT","Query ID required for OSML '%s' request", request);
		return -1;
		}

	    /** Again check the request... **/
	    if (!strcmp(request,"closesession"))
	        {
		if (session_handle == XHN_INVALID_HANDLE)
		    {
		    snprintf(sbuf,256,"Content-Type: text/html\r\n"
			     "Pragma: no-cache\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
		    fdWrite(conn, sbuf, strlen(sbuf), 0,0);
		    mssError(1,"NHT","Illegal attempt to close the default OSML session.");
		    return -1;
		    }
		xhnFreeHandle(&(sess->Hctx), session_handle);
	        objCloseSession(objsess);
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X%8.8X>&nbsp;</A>\r\n",
		    0);
	        fdWrite(conn, sbuf, strlen(sbuf), 0,0);
	        }
	    else if (!strcmp(request,"open"))
	        {
		/** Get the info and open the object **/
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__usrtype"),&usrtype) < 0) return -1;
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__objmode"),&ptr) < 0) return -1;
		mode = strtol(ptr,NULL,0);
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__objmask"),&ptr) < 0) return -1;
		mask = strtol(ptr,NULL,0);
		obj = objOpen(objsess, req_inf->StrVal, mode, mask, usrtype);
		if (!obj)
		    obj_handle = XHN_INVALID_HANDLE;
		else
		    obj_handle = xhnAllocHandle(&(sess->Hctx), obj);
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X" XHN_HANDLE_PRT ">&nbsp;</A>\r\n",
		    obj_handle);
	        fdWrite(conn, sbuf, strlen(sbuf), 0,0);

		/** Include an attribute listing **/
		nht_internal_WriteAttrs(sess,obj,conn,obj_handle,1);
	        }
	    else if (!strcmp(request,"close"))
	        {
		/** For this, we loop through a comma-separated list of object
		 ** ids to close.
		 **/
		ptr = NULL;
		stAttrValue_ne(stLookup_ne(req_inf,"ls__oid"),&ptr);
		while(ptr && *ptr)
		    {
		    obj_handle = xhnStringToHandle(ptr+1, &newptr, 16);
		    if (newptr <= ptr+1) break;
		    ptr = newptr;
		    obj = (pObject)xhnHandlePtr(&(sess->Hctx), obj_handle);
		    if (!obj || !ISMAGIC(obj, MGK_OBJECT)) 
			{
			mssError(1,"NHT","Invalid object id(s) in OSML 'close' request");
			continue;
			}
		    xhnFreeHandle(&(sess->Hctx), obj_handle);
		    objClose(obj);
		    }
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X%8.8X>&nbsp;</A>\r\n",
		    0);
	        fdWrite(conn, sbuf, strlen(sbuf), 0,0);
	        }
	    else if (!strcmp(request,"multiquery"))
	        {
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__sql"),&ptr) < 0) return -1;
		qy = objMultiQuery(objsess, ptr);
		if (!qy)
		    query_handle = XHN_INVALID_HANDLE;
		else
		    query_handle = xhnAllocHandle(&(sess->Hctx), qy);
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X" XHN_HANDLE_PRT ">&nbsp;</A>\r\n",
		    query_handle);
	        fdWrite(conn, sbuf, strlen(sbuf), 0,0);
		}
	    else if (!strcmp(request,"objquery"))
	        {
		where=NULL;
		orderby=NULL;
		stAttrValue_ne(stLookup_ne(req_inf,"ls__where"),&where);
		stAttrValue_ne(stLookup_ne(req_inf,"ls__orderby"),&orderby);
		qy = objOpenQuery(obj,where,orderby,NULL,NULL);
		if (!qy)
		    query_handle = XHN_INVALID_HANDLE;
		else
		    query_handle = xhnAllocHandle(&(sess->Hctx), qy);
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X" XHN_HANDLE_PRT ">&nbsp;</A>\r\n",
		    query_handle);
	        fdWrite(conn, sbuf, strlen(sbuf), 0,0);
		}
	    else if (!strcmp(request,"queryfetch"))
	        {
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__objmode"),&ptr) < 0) return -1;
		mode = strtol(ptr,NULL,0);
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__rowcount"),&ptr) < 0)
		    n = 0x7FFFFFFF;
		else
		    n = strtol(ptr,NULL,0);
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__startat"),&ptr) < 0)
		    start = 0;
		else
		    start = strtol(ptr,NULL,0) - 1;
		if (start < 0) start = 0;
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X%8.8X>&nbsp;</A>\r\n",
		         0);
	        fdWrite(conn, sbuf, strlen(sbuf), 0,0);
		while(start > 0 && (obj = objQueryFetch(qy,mode)))
		    {
		    objClose(obj);
		    start--;
		    }
		while(n > 0 && (obj = objQueryFetch(qy,mode)))
		    {
		    obj_handle = xhnAllocHandle(&(sess->Hctx), obj);
		    nht_internal_WriteAttrs(sess,obj,conn,obj_handle,1);
		    n--;
		    }
		}
	    else if (!strcmp(request,"queryclose"))
	        {
		xhnFreeHandle(&(sess->Hctx), query_handle);
		objQueryClose(qy);
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X%8.8X>&nbsp;</A>\r\n",
		    0);
	        fdWrite(conn, sbuf, strlen(sbuf), 0,0);
		}
	    else if (!strcmp(request,"read"))
	        {
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__bytecount"),&ptr) < 0)
		    n = 0x7FFFFFFF;
		else
		    n = strtol(ptr,NULL,0);
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__offset"),&ptr) < 0)
		    o = -1;
		else
		    o = strtol(ptr,NULL,0);
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X%8.8X>",
		    0);
	        fdWrite(conn, sbuf, strlen(sbuf), 0,0);
		while(n > 0 && (cnt=objRead(obj,sbuf,(256>n)?n:256,(o != -1)?o:0,(o != -1)?OBJ_U_SEEK:0)) > 0)
		    {
		    for(i=0;i<cnt;i++)
		        {
		        sprintf(hexbuf,"%2.2X",((unsigned char*)sbuf)[i]);
			fdWrite(conn,hexbuf,2,0,0);
			}
		    n -= cnt;
		    o = -1;
		    }
		fdWrite(conn, "</A>\r\n", 6,0,0);
		}
	    else if (!strcmp(request,"write"))
	        {
		}
	    else if (!strcmp(request,"attrs"))
	        {
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X%8.8X>&nbsp;</A>\r\n",
		         0);
	        fdWrite(conn, sbuf, strlen(sbuf), 0,0);
		nht_internal_WriteAttrs(sess,obj,conn,obj_handle,1);
		}
	    else if (!strcmp(request,"setattrs"))
	        {
		/** Find all GET params that are NOT like ls__thingy **/
		for(i=0;i<req_inf->nSubInf;i++)
		    {
		    subinf = req_inf->SubInf[i];
		    if (strncmp(subinf->Name,"ls__",4) != 0)
		        {
			t = objGetAttrType(obj, subinf->Name);
			if (t < 0) return -1;
			switch(t)
			    {
			    case DATA_T_INTEGER:
			        n = objDataToInteger(DATA_T_STRING, subinf->StrVal, NULL);
				objSetAttrValue(obj,subinf->Name,POD(&n));
				break;

			    case DATA_T_DOUBLE:
			        dbl = objDataToDouble(DATA_T_STRING, subinf->StrVal);
				objSetAttrValue(obj,subinf->Name,POD(&dbl));
				break;

			    case DATA_T_STRING:
			        objSetAttrValue(obj,subinf->Name,POD(&(subinf->StrVal)));
				break;

			    case DATA_T_DATETIME:
			        objDataToDateTime(DATA_T_STRING, subinf->StrVal, &dt, NULL);
				objSetAttrValue(obj,subinf->Name,POD(&dt));
				break;

			    case DATA_T_MONEY:
			        objDataToMoney(DATA_T_STRING, subinf->StrVal, &m);
				objSetAttrValue(obj,subinf->Name,POD(&m));
				break;

			    case DATA_T_STRINGVEC:
			    case DATA_T_INTVEC:
			    case DATA_T_UNAVAILABLE: 
			        return -1;
			    }
			}
		    }
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X%8.8X>&nbsp;</A>\r\n",
		         0);
	        fdWrite(conn, sbuf, strlen(sbuf), 0,0);
		}
	    }
	
    return 0;
    }



/*** nht_internal_CkParams - check to see if we need to set parameters as
 *** a part of the object open process.  This is for opening objects for
 *** read access.
 ***/
int
nht_internal_CkParams(pStruct url_inf, pObject obj)
    {
    pStruct find_inf, search_inf;
    int i,t,n;
    char* ptr = NULL;
    DateTime dt;
    pDateTime dtptr;
    MoneyType m;
    pMoneyType mptr;
    double dbl;

	/** Check for the ls__params=yes tag **/
	stAttrValue_ne(find_inf = stLookup_ne(url_inf,"ls__params"), &ptr);
	if (!ptr || strcmp(ptr,"yes")) return 0;

	/** Ok, look for any params not beginning with ls__ **/
	for(i=0;i<url_inf->nSubInf;i++)
	    {
	    search_inf = url_inf->SubInf[i];
	    if (strncmp(search_inf->Name,"ls__",4))
	        {
		/** Get the value and call objSetAttrValue with it **/
		t = objGetAttrType(obj, search_inf->Name);
		switch(t)
		    {
		    case DATA_T_INTEGER:
		        /*if (search_inf->StrVal == NULL)
		            n = search_inf->IntVal[0];
		        else*/
		            n = strtol(search_inf->StrVal, NULL, 10);
		        objSetAttrValue(obj, search_inf->Name, POD(&n));
			break;

		    case DATA_T_STRING:
		        if (search_inf->StrVal != NULL)
		    	    {
		    	    ptr = search_inf->StrVal;
		    	    objSetAttrValue(obj, search_inf->Name, POD(&ptr));
			    }
			break;
		    
		    case DATA_T_DOUBLE:
		        /*if (search_inf->StrVal == NULL)
		            dbl = search_inf->IntVal[0];
		        else*/
		            dbl = strtod(search_inf->StrVal, NULL);
			objSetAttrValue(obj, search_inf->Name, POD(&dbl));
			break;

		    case DATA_T_MONEY:
		        /*if (search_inf->StrVal == NULL)
			    {
			    m.WholePart = search_inf->IntVal[0];
			    m.FractionPart = 0;
			    }
			else
			    {*/
			    objDataToMoney(DATA_T_STRING, search_inf->StrVal, &m);
			    /*}*/
			mptr = &m;
			objSetAttrValue(obj, search_inf->Name, POD(&mptr));
			break;
		
		    case DATA_T_DATETIME:
		        if (search_inf->StrVal != NULL)
			    {
			    objDataToDateTime(DATA_T_STRING, search_inf->StrVal, &dt,NULL);
			    dtptr = &dt;
			    objSetAttrValue(obj, search_inf->Name, POD(&dtptr));
			    }
			break;
		    }
		}
	    }

    return 0;
    }


/*** nht_internal_GET - handle the HTTP GET method, reading a document or
 *** attribute list, etc.
 ***/
int
nht_internal_GET(pNhtSessionData nsess, pFile conn, pStruct url_inf)
    {
    char sbuf[256];
    int cnt;
    pStruct find_inf,find_inf2;
    pObjQuery query;
    char* dptr;
    char* ptr;
    char* aptr;
    pObject target_obj, sub_obj, tmp_obj;
    char* bufptr;
    char cur_wd[256];
    int rowid;
    int tid = -1;
    int convert_text = 0;

    	/*printf("GET called, stack ptr = %8.8X\n",&cnt);*/
        /** If we're opening the "errorstream", pass of processing to err handler **/
	if (!strncmp(url_inf->StrVal,"/errorstream",12))
	    {
	    return nht_internal_ErrorHandler(nsess, conn);
	    }

	/** Ok, open the object here. **/
	target_obj = objOpen(nsess->ObjSess, url_inf->StrVal, O_RDONLY, 0600, "text/html");
	if (!target_obj)
	    {
	    nht_internal_GenerateError(nsess);
	    snprintf(sbuf,256,"HTTP/1.0 404 Not Found\r\n"
	    		 "Server: %s\r\n"
			 "\r\n"
			 "<H1>404 Not Found</H1><HR><PRE>\r\n",NHT.ServerString);
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    mssPrintError(conn);
	    netCloseTCP(conn,1000,0);
	    thExit();
	    }

	/** Do we need to set params as a part of the open? **/
	nht_internal_CkParams(url_inf, target_obj);

	/** Check GET mode. **/
	find_inf = stLookup_ne(url_inf,"ls__mode");

	/** WAIT TRIGGER mode. **/
	if (find_inf && !strcmp(find_inf->StrVal,"triggerwait"))
	    {
	    find_inf = stLookup_ne(url_inf,"ls__waitid");
	    if (find_inf)
	        {
		tid = strtol(find_inf->StrVal,NULL,0);
		nht_internal_WaitTrigger(nsess,tid);
		}
	    }

	/** Ok, issue the HTTP header for this one. **/
	if (nsess->IsNewCookie)
	    {
	    snprintf(sbuf,256,"HTTP/1.0 200 OK\r\n"
		     "Server: %s\r\n"
		     "Set-Cookie: %s; path=/\r\n", 
		     NHT.ServerString,nsess->Cookie);
	    nsess->IsNewCookie = 0;
	    }
	else
	    {
	    snprintf(sbuf,256,"HTTP/1.0 200 OK\r\n"
		     "Server: %s\r\n",NHT.ServerString);
	    }
	fdWrite(conn,sbuf,strlen(sbuf),0,0);

	/** Exit now if wait trigger. **/
	if (tid != -1)
	    {
	    fdWrite(conn,"OK\r\n",4,0,0);
	    objClose(target_obj);
	    return 0;
	    }

	/** GET CONTENT mode. **/
	if (!find_inf || !strcmp(find_inf->StrVal, "content"))
	    {
	    /** Check the object type. **/
	    objGetAttrValue(target_obj, "outer_type", POD(&ptr));
	    if (!strcmp(ptr,"widget/page") || !strcmp(ptr,"widget/frameset"))
	        {
		/*fdSetOptions(conn, FD_UF_WRCACHE);*/
		snprintf(sbuf,256,"Content-Type: text/html\r\n\r\n");
		fdWrite(conn,sbuf,strlen(sbuf),0,0);
	        htrRender(conn, target_obj);
	        }
	    else
	        {
		objGetAttrValue(target_obj,"inner_type", POD(&ptr));
		if (!strcmp(ptr,"text/plain")) 
		    {
		    ptr = "text/html";
		    convert_text = 1;
		    }
		snprintf(sbuf,256,"Content-Type: %s\r\n\r\n", ptr);
		fdWrite(conn,sbuf,strlen(sbuf),0,0);
		if (convert_text) fdWrite(conn,"<HTML><PRE>",11,0,FD_U_PACKET);
		bufptr = (char*)nmMalloc(4096);
	        while((cnt=objRead(target_obj,bufptr,4096,0,0)) > 0)
	            {
		    fdWrite(conn,bufptr,cnt,0,FD_U_PACKET);
		    }
		if (convert_text) fdWrite(conn,"</HTML></PRE>",13,0,FD_U_PACKET);
		if (cnt < 0) 
		    {
		    mssError(0,"NHT","Incomplete read of object's content");
		    nht_internal_GenerateError(nsess);
		    }
		nmFree(bufptr, 4096);
	        }
	    }

	/** GET DIRECTORY LISTING mode. **/
	else if (!strcmp(find_inf->StrVal,"list"))
	    {
	    query = objOpenQuery(target_obj,"",NULL,NULL,NULL);
	    if (query)
	        {
		snprintf(sbuf,256,"Content-Type: text/html\r\n\r\n");
		fdWrite(conn,sbuf,strlen(sbuf),0,0);
		snprintf(sbuf,256,"<HTML><HEAD><META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\"></HEAD><BODY><TT><A HREF=%s/..>..</A><BR>\n",url_inf->StrVal);
		fdWrite(conn,sbuf,strlen(sbuf),0,0);
		dptr = url_inf->StrVal;
		while(*dptr && *dptr == '/' && dptr[1] == '/') dptr++;
		while((sub_obj = objQueryFetch(query,O_RDONLY)))
		    {
		    objGetAttrValue(sub_obj, "name", POD(&ptr));
		    objGetAttrValue(sub_obj, "annotation", POD(&aptr));
		    snprintf(sbuf,256,"<A HREF=%s%s%s TARGET='%s'>%s</A><BR>\n",dptr,
		    	(dptr[0]=='/' && dptr[1]=='\0')?"":"/",ptr,ptr,aptr);
		    fdWrite(conn,sbuf,strlen(sbuf),0,0);
		    objClose(sub_obj);
		    }
		objQueryClose(query);
		}
	    else
	        {
		nht_internal_GenerateError(nsess);
		}
	    }

	/** SQL QUERY mode **/
	else if (!strcmp(find_inf->StrVal,"query"))
	    {
	    /** Change directory to appropriate query root **/
	    snprintf(sbuf,256,"Content-Type: text/html\r\n\r\n");
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    strcpy(cur_wd, objGetWD(nsess->ObjSess));
	    objSetWD(nsess->ObjSess, target_obj);

	    /** Get the SQL **/
	    if (stAttrValue_ne(stLookup_ne(url_inf,"ls__sql"),&ptr) >= 0)
	        {
		query = objMultiQuery(nsess->ObjSess, ptr);
		if (query)
		    {
		    rowid = 0;
		    while((sub_obj = objQueryFetch(query,O_RDONLY)))
		        {
			nht_internal_WriteAttrs(nsess,sub_obj,conn,(handle_t)rowid,1);
			objClose(sub_obj);
			rowid++;
			}
		    objQueryClose(query);
		    }
		}

	    /** Switch the current directory back to what it used to be. **/
	    tmp_obj = objOpen(nsess->ObjSess, cur_wd, O_RDONLY, 0600, "text/html");
	    objSetWD(nsess->ObjSess, tmp_obj);
	    objClose(tmp_obj);
	    }

	/** GET METHOD LIST mode. **/
	else if (!strcmp(find_inf->StrVal,"methods"))
	    {
	    }

	/** GET ATTRIBUTE-VALUE LIST mode. **/
	else if (!strcmp(find_inf->StrVal,"attr"))
	    {
	    }

	/** Direct OSML Access mode... **/
	else if (!strcmp(find_inf->StrVal,"osml"))
	    {
	    find_inf = stLookup_ne(url_inf,"ls__req");
	    nht_internal_OSML(nsess,conn,target_obj, find_inf->StrVal, url_inf);
	    }

	/** Exec method mode **/
	else if (!strcmp(find_inf->StrVal,"execmethod"))
	    {
	    find_inf = stLookup_ne(url_inf,"ls__methodname");
	    find_inf2 = stLookup_ne(url_inf,"ls__methodparam");
	    if (!find_inf || !find_inf2)
	        {
		mssError(1,"NHT","Invalid call to execmethod - requires name and param");
		nht_internal_GenerateError(nsess);
		}
	    else
	        {
	    	ptr = find_inf2->StrVal;
	    	objExecuteMethod(target_obj, find_inf->StrVal, POD(&ptr));
		fdWrite(conn,"OK",2,0,0);
		}
	    }

	/** Close the objectsystem entry. **/
	objClose(target_obj);

    return 0;
    }


/*** nht_internal_PUT - implements the PUT HTTP method.  Set content_buf to
 *** data to write, otherwise it will be read from the connection if content_buf
 *** is NULL.
 ***/
int
nht_internal_PUT(pNhtSessionData nsess, pFile conn, pStruct url_inf, int size, char* content_buf)
    {
    pObject target_obj;
    char sbuf[160];
    int rcnt;
    int type,i,v;
    pStruct sub_inf;
    int already_exist=0;

    	/** See if the object already exists. **/
	target_obj = objOpen(nsess->ObjSess, url_inf->StrVal, O_RDONLY, 0600, "text/html");
	if (target_obj)
	    {
	    objClose(target_obj);
	    already_exist = 1;
	    }

	/** Ok, open the object here. **/
	target_obj = objOpen(nsess->ObjSess, url_inf->StrVal, O_WRONLY | O_CREAT | O_TRUNC, 0600, "text/html");
	if (!target_obj)
	    {
	    snprintf(sbuf,160,"HTTP/1.0 404 Not Found\r\n"
	    		 "Server: %s\r\n"
			 "\r\n"
			 "<H1>404 Not Found</H1><HR><PRE>\r\n",NHT.ServerString);
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    mssPrintError(conn);
	    netCloseTCP(conn,1000,0);
	    thExit();
	    }

	/** OK, we're ready.  Send the 100 Continue message. **/
	/*sprintf(sbuf,"HTTP/1.1 100 Continue\r\n"
		     "Server: %s\r\n"
		     "\r\n",NHT.ServerString);
	fdWrite(conn,sbuf,strlen(sbuf),0,0);*/

	/** If size specified, set the size. **/
	if (size != -1) objSetAttrValue(target_obj, "size", POD(&size));

	/** Set any attributes specified in the url inf **/
	for(i=0;i<url_inf->nSubInf;i++)
	    {
	    sub_inf = url_inf->SubInf[i];
	    type = objGetAttrType(target_obj, sub_inf->Name);
	    if (type == DATA_T_INTEGER)
	        {
		v = strtol(sub_inf->StrVal,NULL,10);
		objSetAttrValue(target_obj, sub_inf->Name, POD(&v));
		}
	    else if (type == DATA_T_STRING)
	        {
		objSetAttrValue(target_obj, sub_inf->Name, POD(&(sub_inf->StrVal)));
		}
	    }

	/** If content_buf, write that else write from the connection. **/
	if (content_buf)
	    {
	    while(size != 0)
	        {
		rcnt = (size>1024)?1024:size;
		objWrite(target_obj, content_buf, rcnt, 0,0);
		size -= rcnt;
		content_buf += rcnt;
		}
	    }
	else
	    {
	    /** Ok, read from the connection, either until size bytes or until EOF. **/
	    while(size != 0 && (rcnt=fdRead(conn,sbuf,160,0,0)) > 0)
	        {
	        if (size > 0)
	            {
		    size -= rcnt;
		    if (size < 0) 
		        {
		        rcnt += size;
		        size = 0;
			}
		    }
	        if (objWrite(target_obj, sbuf, rcnt, 0,0) < 0) break;
		}
	    }

	/** Close the object. **/
	objClose(target_obj);

	/** Ok, issue the HTTP header for this one. **/
	if (nsess->IsNewCookie)
	    {
	    if (already_exist)
	        {
	        snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
		     "Server: %s\r\n"
		     "Set-Cookie: %s; path=/\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,nsess->Cookie, url_inf->StrVal);
		}
	    else
	        {
	        snprintf(sbuf,160,"HTTP/1.0 201 Created\r\n"
		     "Server: %s\r\n"
		     "Set-Cookie: %s; path=/\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,nsess->Cookie, url_inf->StrVal);
		}
	    nsess->IsNewCookie = 0;
	    }
	else
	    {
	    if (already_exist)
	        {
	        snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
		     "Server: %s\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,url_inf->StrVal);
		}
	    else
	        {
	        snprintf(sbuf,160,"HTTP/1.0 201 Created\r\n"
		     "Server: %s\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,url_inf->StrVal);
		}
	    }
	fdWrite(conn,sbuf,strlen(sbuf),0,0);

    return 0;
    }


/*** nht_internal_COPY - implements the COPY centrallix-http method.
 ***/
int
nht_internal_COPY(pNhtSessionData nsess, pFile conn, pStruct url_inf, char* dest)
    {
    pObject source_obj,target_obj;
    int size;
    int already_exist = 0;
    char sbuf[256];
    int rcnt,wcnt;

	/** Ok, open the source object here. **/
	source_obj = objOpen(nsess->ObjSess, url_inf->StrVal, O_RDONLY, 0600, "text/html");
	if (!source_obj)
	    {
	    snprintf(sbuf,256,"HTTP/1.0 404 Not Found\r\n"
	    		 "Server: %s\r\n"
			 "\r\n"
			 "<H1>404 Source Not Found</H1><HR><PRE>\r\n",NHT.ServerString);
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    mssPrintError(conn);
	    netCloseTCP(conn,1000,0);
	    thExit();
	    }

	/** Do we need to set params as a part of the open? **/
	nht_internal_CkParams(url_inf, source_obj);

	/** Get the size of the original object, if possible **/
	if (objGetAttrValue(source_obj,"size",POD(&size)) != 0) size = -1;

	/** Try to open the new object read-only to see if it exists... **/
	target_obj = objOpen(nsess->ObjSess, dest, O_RDONLY, 0600, "text/html");
	if (target_obj)
	    {
	    objClose(target_obj);
	    already_exist = 1;
	    }

	/** Ok, open the target object for keeps now. **/
	target_obj = objOpen(nsess->ObjSess, dest, O_WRONLY | O_TRUNC | O_CREAT, 0600, "text/html");
	if (!target_obj)
	    {
	    snprintf(sbuf,256,"HTTP/1.0 404 Not Found\r\n"
	    		 "Server: %s\r\n"
			 "\r\n"
			 "<H1>404 Target Not Found</H1>\r\n",NHT.ServerString);
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    netCloseTCP(conn,1000,0);
	    thExit();
	    }

	/** Set the size of the new document... **/
	if (size >= 0) objSetAttrValue(target_obj, "size", POD(&size));

	/** Do the copy operation. **/
	while((rcnt = objRead(source_obj, sbuf, 256, 0,0)) > 0)
	    {
	    while(rcnt > 0)
	        {
		wcnt = objWrite(target_obj, sbuf, rcnt, 0,0);
		if (wcnt <= 0) break;
		rcnt -= wcnt;
		}
	    }

	/** Close the objects **/
	objClose(source_obj);
	objClose(target_obj);

	/** Ok, issue the HTTP header for this one. **/
	if (nsess->IsNewCookie)
	    {
	    if (already_exist)
	        {
	        snprintf(sbuf,256,"HTTP/1.0 200 OK\r\n"
		     "Server: %s\r\n"
		     "Set-Cookie: %s; path=/\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,nsess->Cookie, dest);
		}
	    else
	        {
	        snprintf(sbuf,256,"HTTP/1.0 201 Created\r\n"
		     "Server: %s\r\n"
		     "Set-Cookie: %s; path=/\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,nsess->Cookie, dest);
		}
	    nsess->IsNewCookie = 0;
	    }
	else
	    {
	    if (already_exist)
	        {
	        snprintf(sbuf,256,"HTTP/1.0 200 OK\r\n"
		     "Server: %s\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,dest);
		}
	    else
	        {
	        snprintf(sbuf,256,"HTTP/1.0 201 Created\r\n"
		     "Server: %s\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,dest);
		}
	    }
	fdWrite(conn,sbuf,strlen(sbuf),0,0);

    return 0;
    }


/*** nht_internal_ConnHandler - manages a single incoming HTTP connection
 *** and processes the connection's request.
 ***/
void
nht_internal_ConnHandler(void* conn_v)
    {
    pFile conn = (pFile)conn_v;
    pLxSession s;
    int toktype;
    char method[16];
    char* urlptr;
    char sbuf[160];
    char auth[160] = "";
    char cookie[160] = "";
    char dest[256] = "";
    char hdr[64];
    char* msg;
    char* ptr;
    Exception e;
    char* usrname;
    char* passwd;
    pNhtSessionData nsess = NULL;
    pStruct url_inf,find_inf;
    int size=-1;
    int did_alloc = 1;
    int tid = -1;

    	/*printf("ConnHandler called, stack ptr = %8.8X\n",&s);*/

	/** Set the thread's name **/
	thSetName(NULL,"HTTP Connection Handler");

        /** Handle parsing exceptions... **/
	Catch(e)
	    {
	    mlxCloseSession(s);
	    mssError(1,"NHT","Failed to parse HTTP request, exiting thread.");
	    snprintf(sbuf,160,"HTTP/1.0 400 Request Error\n\n%s\n",msg);
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    netCloseTCP(conn,1000,0);
	    thExit();
	    }

    	/** Initialize a lexical analyzer session... **/
	s = mlxOpenSession(conn, MLX_F_NODISCARD | MLX_F_DASHKW | MLX_F_ICASE |
		MLX_F_EOL | MLX_F_EOF);

	/** Read in the main request header. **/
	if (mlxNextToken(s) != MLX_TOK_KEYWORD) msg="Invalid method syntax",Throw(e);
	mlxCopyToken(s,method,16);
	mlxSetOptions(s,MLX_F_IFSONLY);
	if (mlxNextToken(s) != MLX_TOK_STRING) msg="Invalid url syntax",Throw(e);
	/*strcpy(url,mlxStringVal(s,NULL));*/
	did_alloc = 1;
	urlptr = mlxStringVal(s, &did_alloc);
	mlxNextToken(s);
	if (mlxNextToken(s) != MLX_TOK_EOL) msg="Expected EOL after version",Throw(e);
	mlxUnsetOptions(s,MLX_F_IFSONLY);

	/** Read in the various header parameters. **/
	while((toktype = mlxNextToken(s)) != MLX_TOK_EOL)
	    {
	    if (toktype == MLX_TOK_EOF) break;
	    if (toktype != MLX_TOK_KEYWORD) msg="Expected HTTP header item",Throw(e);
	    /*ptr = mlxStringVal(s,NULL);*/
	    mlxCopyToken(s,hdr,64);
	    if (mlxNextToken(s) != MLX_TOK_COLON) msg="Expected : after HTTP header",Throw(e);

	    /** Got a header item.  Pick an appropriate type. **/
	    if (!strcmp(hdr,"destination"))
	        {
		/** Copy next IFS-only token to destination value **/
		mlxSetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) msg="Expected filename after dest.",Throw(e);
		mlxCopyToken(s,dest,256);
		mlxUnsetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_EOF) msg="Expected EOL after filename",Throw(e);
		}
	    else if (!strcmp(hdr,"authorization"))
	        {
		/** Get 'Basic' then the auth string in base64 **/
		mlxSetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) msg="Expected auth type",Throw(e);
		ptr = mlxStringVal(s,NULL);
		if (strcasecmp(ptr,"basic")) msg="Can only handle BASIC auth",Throw(e);
		if (mlxNextToken(s) != MLX_TOK_STRING) msg="Expected auth after Basic",Throw(e);
		nht_internal_Decode64(auth,mlxStringVal(s,NULL),160);
		mlxUnsetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_EOL) msg="Expected EOL after auth",Throw(e);
		}
	    else if (!strcmp(hdr,"cookie"))
	        {
		/** Copy whole thing. **/
		mlxSetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) msg="Expected str after Cookie:",Throw(e);
		mlxCopyToken(s,cookie,160);
		while((toktype = mlxNextToken(s)))
		    {
		    if (toktype == MLX_TOK_EOL || toktype == MLX_TOK_ERROR) break;
		    }
		mlxUnsetOptions(s,MLX_F_IFSONLY);
		}
	    else if (!strcmp(hdr,"content-length"))
	        {
		/** Get the integer. **/
		if (mlxNextToken(s) != MLX_TOK_INTEGER) msg="Expected content-length",Throw(e);
		size = mlxIntVal(s);
		if (mlxNextToken(s) != MLX_TOK_EOL) msg="Expected EOL after length",Throw(e);
		}
	    else
	        {
		/** Don't know what it is.  Just skip to end-of-line. **/
		mlxSetOptions(s,MLX_F_IFSONLY);
		while((toktype = mlxNextToken(s)))
		    {
		    if (toktype == MLX_TOK_EOL || toktype == MLX_TOK_ERROR) break;
		    }
		mlxUnsetOptions(s,MLX_F_IFSONLY);
		}
	    }

	/** Ok, done parsing.  Close the lexer session. **/
	mlxCloseSession(s);

	/** Did client send authentication? **/
	if (!*auth)
	    {
	    snprintf(sbuf,160,"HTTP/1.0 401 Unauthorized\r\n"
	    		 "Server: %s\r\n"
			 "WWW-Authenticate: Basic realm=\"%s\"\r\n"
			 "\r\n"
			 "<H1>Unauthorized</H1>\r\n",NHT.ServerString,NHT.Realm);
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    netCloseTCP(conn,1000,0);
	    thExit();
	    }

	/** Got authentication.  Parse the auth string. **/
	usrname = strtok(auth,":");
	if (usrname) passwd = strtok(NULL,"\r\n");
	if (!usrname || !passwd) 
	    {
	    snprintf(sbuf,160,"HTTP/1.0 400 Bad Request\r\n"
	    		 "Server: %s\r\n"
			 "\r\n"
			 "<H1>400 Bad Request</H1>\r\n",NHT.ServerString);
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    netCloseTCP(conn,1000,0);
	    thExit();
	    }

	/** Check for a cookie -- if one, try to resume a session. **/
	if (*cookie)
	    {
	    if (cookie[strlen(cookie)-1] == ';') cookie[strlen(cookie)-1] = '\0';
	    nsess = (pNhtSessionData)xhLookup(&(NHT.CookieSessions), cookie);
	    if (nsess)
	        {
		if (strcmp(nsess->Username,usrname) || strcmp(passwd,nsess->Password))
		    {
	    	    snprintf(sbuf,160,"HTTP/1.0 401 Unauthorized\r\n"
		    		 "Server: %s\r\n"
				 "WWW-Authenticate: Basic realm=\"%s\"\r\n"
				 "\r\n"
				 "<H1>Unauthorized</H1>\r\n",NHT.ServerString,NHT.Realm);
	            fdWrite(conn,sbuf,strlen(sbuf),0,0);
	            netCloseTCP(conn,1000,0);
	            thExit();
		    }
		thSetParam(NULL,"mss",nsess->Session);
		thSetUserID(NULL,((pMtSession)(nsess->Session))->UserID);
		}
	    }

	/** No cookie or no session for the given cookie? **/
	if (!nsess)
	    {
	    if (mssAuthenticate(usrname, passwd) < 0)
	        {
	        snprintf(sbuf,160,"HTTP/1.0 401 Unauthorized\r\n"
			     "Server: %s\r\n"
			     "WWW-Authenticate: Basic realm=\"%s\"\r\n"
			     "\r\n"
			     "<H1>Unauthorized</H1>\r\n",NHT.ServerString,NHT.Realm);
	        fdWrite(conn,sbuf,strlen(sbuf),0,0);
	        netCloseTCP(conn,1000,0);
	        thExit();
		}
	    nsess = (pNhtSessionData)nmSysMalloc(sizeof(NhtSessionData));
	    strcpy(nsess->Username, mssUserName());
	    strcpy(nsess->Password, mssPassword());
	    nsess->Session = thGetParam(NULL,"mss");
	    nsess->IsNewCookie = 1;
	    nsess->ObjSess = objOpenSession("/");
	    nsess->Errors = syCreateSem(0,0);
	    xaInit(&nsess->Triggers,16);
	    xaInit(&nsess->ErrorList,16);
	    nht_internal_CreateCookie(nsess->Cookie);
	    xhnInitContext(&(nsess->Hctx));
	    xhAdd(&(NHT.CookieSessions), nsess->Cookie, (void*)nsess);
	    xaAddItem(&(NHT.Sessions), (void*)nsess);
	    }

	/** Parse out the requested url **/
	url_inf = htsParseURL(urlptr);
	if (!url_inf)
	    {
	    snprintf(sbuf,160,"HTTP/1.0 500 Internal Server Error\r\n"
	    		 "Server: %s\r\n"
			 "\r\n"
			 "<H1>500 Internal Server Error</H1>\r\n",NHT.ServerString);
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    netCloseTCP(conn,1000,0);
	    }
	nht_internal_ConstructPathname(url_inf);

	/** Need to start an available connection completion trigger on this? **/
	if ((find_inf=stLookup_ne(url_inf,"ls__triggerid")))
	    {
	    tid = strtol(find_inf->StrVal,NULL,0);
	    nht_internal_StartTrigger(nsess, tid);
	    }

	/** If the method was GET and an ls__method was specified, use that method **/
	if (!strcmp(method,"get") && (find_inf=stLookup_ne(url_inf,"ls__method")))
	    {
	    if (!strcasecmp(find_inf->StrVal,"get"))
	        {
	        nht_internal_GET(nsess,conn,url_inf);
		}
	    else if (!strcasecmp(find_inf->StrVal,"copy"))
	        {
		find_inf = stLookup_ne(url_inf,"ls__destination");
		if (!find_inf || !(find_inf->StrVal))
		    {
	            snprintf(sbuf,160,"HTTP/1.0 400 Method Error\r\n"
	    		 "Server: %s\r\n"
			 "\r\n"
			 "<H1>400 Method Error - include ls__destination for copy</H1>\r\n",NHT.ServerString);
	            fdWrite(conn,sbuf,strlen(sbuf),0,0);
		    }
		else
		    {
		    ptr = find_inf->StrVal;
		    nht_internal_COPY(nsess,conn,url_inf, ptr);
		    }
		}
	    else if (!strcasecmp(find_inf->StrVal,"put"))
	        {
		find_inf = stLookup_ne(url_inf,"ls__content");
		if (!find_inf || !(find_inf->StrVal))
		    {
	            snprintf(sbuf,160,"HTTP/1.0 400 Method Error\r\n"
	    		 "Server: %s\r\n"
			 "\r\n"
			 "<H1>400 Method Error - include ls__content for put</H1>\r\n",NHT.ServerString);
	            fdWrite(conn,sbuf,strlen(sbuf),0,0);
		    }
		else
		    {
		    ptr = find_inf->StrVal;
		    size = strlen(ptr);
	            nht_internal_PUT(nsess,conn,url_inf,size,ptr);
		    }
		}
	    }
	else
	    {
	    /** Which method was used? **/
	    if (!strcmp(method,"get"))
	        {
	        nht_internal_GET(nsess,conn,url_inf);
	        }
	    else if (!strcmp(method,"put"))
	        {
	        nht_internal_PUT(nsess,conn,url_inf,size,NULL);
	        }
	    else if (!strcmp(method,"copy"))
	        {
	        nht_internal_COPY(nsess,conn,url_inf,dest);
	        }
	    else
	        {
	        snprintf(sbuf,160,"HTTP/1.0 501 Not Implemented\r\n"
	    		 "Server: %s\r\n"
			 "\r\n"
			 "<H1>501 Method Not Implemented</H1>\r\n",NHT.ServerString);
	        fdWrite(conn,sbuf,strlen(sbuf),0,0);
	        /*netCloseTCP(conn,1000,0);*/
		}
	    }

	/** Close and exit. **/
	if (url_inf) stFreeInf_ne(url_inf);
	if (did_alloc) nmSysFree(urlptr);
	netCloseTCP(conn,1000,0);

	/** End a trigger? **/
	if (tid != -1) nht_internal_EndTrigger(nsess,tid);

    thExit();
    }


/*** nht_internal_Handler - manages incoming HTTP connections and sends
 *** the appropriate documents/etc to the requesting client.
 ***/
void
nht_internal_Handler(void* v)
    {
    pFile listen_socket;
    pFile connection_socket;
    pStructInf my_config;
    char listen_port[32];
    char* strval;
    int intval;

    	/*printf("Handler called, stack ptr = %8.8X\n",&listen_socket);*/

	/** Set the thread's name **/
	thSetName(NULL,"HTTP Network Listener");

	/** Get our configuration **/
	strcpy(listen_port,"800");
	my_config = stLookup(CxGlobals.ParsedConfig, "net_http");
	if (my_config)
	    {
	    /** Got the config.  Now lookup what the TCP port is that we listen on **/
	    strval=NULL;
	    if (stAttrValue(stLookup(my_config, "listen_port"), &intval, &strval, 0) >= 0)
		{
		if (strval)
		    {
		    memccpy(listen_port, strval, 0, 31);
		    listen_port[31] = '\0';
		    }
		else
		    {
		    snprintf(listen_port,32,"%d",intval);
		    }
		}

	    /** Find out what server string we should use **/
	    if (stAttrValue(stLookup(my_config, "server_string"), NULL, &strval, 0) >= 0)
		{
		memccpy(NHT.ServerString, strval, 0, 79);
		NHT.ServerString[79] = '\0';
		}
	    else
		{
		snprintf(NHT.ServerString, 80, "Centrallix/%.16s", cx__version);
		}

	    /** Get the realm name **/
	    if (stAttrValue(stLookup(my_config, "auth_realm"), NULL, &strval, 0) >= 0)
		{
		memccpy(NHT.Realm, strval, 0, 79);
		NHT.Realm[79] = '\0';
		}
	    else
		{
		snprintf(NHT.Realm, 80, "Centrallix");
		}
	    }

    	/** Open the server listener socket. **/
	listen_socket = netListenTCP(listen_port, 32, 0);
	if (!listen_socket) 
	    {
	    mssErrorErrno(1,"NHT","Could not open network listener");
	    thExit();
	    }
	
    	/*printf("Handler return from netListenTCP, stack ptr = %8.8X\n",&listen_socket);*/

	/** Loop, accepting requests **/
	while((connection_socket = netAcceptTCP(listen_socket,0)))
	    {
	    if (!thCreate(nht_internal_ConnHandler, 0, connection_socket))
	        {
		mssError(1,"NHT","Could not create thread to handle connection!");
		netCloseTCP(connection_socket,0,0);
		}
	    }

	/** Exit. **/
	mssError(1,"NHT","Could not continue to accept requests.");
	netCloseTCP(listen_socket,0,0);

    thExit();
    }


/*** nhtInitialize - initialize the HTTP network handler and start the 
 *** listener thread.
 ***/
int
nhtInitialize()
    {
    /*int i;

	printf("nhtInit called, stack ptr = %8.8X\n",&i);*/

    	/** Initialize the random number generator. **/
	srand48(time(NULL));

	/** Initialize globals **/
	memset(&NHT, 0, sizeof(NHT));
	xhInit(&(NHT.CookieSessions),255,0);
	xaInit(&(NHT.Sessions),256);
	NHT.StdOut = fdOpenFD(1,O_RDWR);

	/** Start the network listener. **/
	thCreate(nht_internal_Handler, 0, NULL);

	/*printf("nhtInit return from thCreate, stack ptr = %8.8X\n",&i);*/

    return 0;
    }

