#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "cxlib/util.h"
#include "cxlib/exception.h"
#include "obj.h"
#include "stparse.h"

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
/* Module: 	net_cgi.c              					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 8, 1998  					*/
/* Description:	Network handler providing a CGI interface via stdin/	*/
/*		stdout for use with existing webservers.		*/
/************************************************************************/



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
    }
    NhtSessionData, *pNhtSessionData;


/*** GLOBALS ***/
struct 
    {
    XHashTable	CookieSessions;
    XArray	Sessions;
    pFile	StdOut;
    }
    NHT;


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
	sprintf(sbuf,"HTTP/1.0 200 OK\r\n"
		     "Server: Centrallix/1.0\r\n"
		     "\r\n"
		     "<HTML><BODY><PRE>");
	fdWrite(net_conn,sbuf,strlen(sbuf),0,0);
	fdWrite(net_conn,errmsg->String,strlen(errmsg->String),0,0);
	sprintf(sbuf,"</PRE></BODY></HTML>\r\n");
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
    int i;
    	/*printf("CreateCk called, stack ptr = %8.8X\n",&i);*/
    sprintf(ck,"LS-%6.6X%4.4X", (((int)(time(NULL)))&0xFFFFFF), (((int)(lrand48()))&0xFFFF));
    return 0;
    }


/*** nht_internal_ConvertChar - converts a %xx type entry to the correct
 *** representation of it, as well as '+' to ' '.
 ***/
char
nht_internal_ConvertChar(char** ptr)
    {
    char x[3];
    char ch;
       
	/** + converts to a space. **/
	if (**ptr == '+')
            {
	    (*ptr)++;
	    ch = ' ';
	    }
	else if (**ptr == '%')
	    {
            x[0] = (*ptr)[1];
            x[1] = (*ptr)[2];
            x[2] = 0;
	    (*ptr)+=3;
    	    ch = strtol(x,NULL,16);
	    }
	else
	    {
	    ch = **ptr;
	    (*ptr)++;
	    }

    return ch;
    }


/*** nht_internal_ParseURL - parses the url passed to the http server into
 *** the pathname and the named parameters.
 ***/
pStruct
nht_internal_ParseURL(char* url)
    {
    pStruct main_inf;
    pStruct attr_inf;
    char* ptr;
    char* dst;
    int len;

    	/** Allocate the main inf for the filename. **/
	main_inf = stAllocInf();
	if (!main_inf) return NULL;

	/** Find the length of the undecoded pathname **/
	ptr = strchr(url,'?');
	if (!ptr) len = strlen(url);
	else len = ptr-url;

	/** Alloc some space for the decoded pathname, and copy. **/
	ptr = (char*)nmSysMalloc(len*3+1);
	if (!ptr) return NULL;
	dst = ptr;
	while(*url && *url != '?') *(dst++) = nht_internal_ConvertChar(&url);
	*dst=0;
	main_inf->StrAlloc[0] = 1;
	main_inf->StrVal = ptr;

	/** Step through any parameters, and alloc inf structures for them. **/
	if (*url == '?') url++;
	while(*url)
	    {
	    /** Alloc a structure for this param. **/
	    attr_inf = stAllocInf();
	    if (!attr_inf)
	        {
		stFreeInf(main_inf);
		return NULL;
		}
	    stAddInf(main_inf, attr_inf);

	    /** Copy the param name. **/
	    dst = attr_inf->Name;
	    while (*url && *url != '=' && (dst-(attr_inf->Name)) < 31) 
	        {
		if (*url == '/') 
		    {
		    url++;
		    continue;
		    }
	        *(dst++) = nht_internal_ConvertChar(&url);
		}
	    *dst=0;
	    if (*url == '=') url++;

	    /** Copy the param value, alloc'ing a string for it **/
	    ptr = strchr(url,'&');
	    if (!ptr) len = strlen(url);
	    else len = ptr-url;
	    ptr = (char*)nmSysMalloc(len*3+1);
	    if (!ptr)
	        {
		stFreeInf(main_inf);
		return NULL;
		}
	    dst=ptr;
	    while(*url && *url != '&') 
	        {
		if (*url == '/')
		    {
		    url++;
		    continue;
		    }
		*(dst++) = nht_internal_ConvertChar(&url);
		}
	    *dst=0;
	    attr_inf->StrAlloc[0] = 1;
	    attr_inf->StrVal = ptr;
	    attr_inf->Type = ST_T_ATTRIB;
	    attr_inf->nVal = 1;
	    if (*url == '&') url++;
	    }

    return main_inf;
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
		            n = strtoi(search_inf->StrVal, NULL, 10);
		        objSetAttrValue(obj, search_inf->Name, &n);
			break;

		    case DATA_T_STRING:
		        if (search_inf->StrVal != NULL)
		    	    {
		    	    ptr = search_inf->StrVal;
		    	    objSetAttrValue(obj, search_inf->Name, &ptr);
			    }
			break;
		    
		    case DATA_T_DOUBLE:
		        /*if (search_inf->StrVal == NULL)
		            dbl = search_inf->IntVal[0];
		        else*/
		            dbl = strtod(search_inf->StrVal, NULL);
			objSetAttrValue(obj, search_inf->Name, &dbl);
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
			objSetAttrValue(obj, search_inf->Name, &mptr);
			break;
		
		    case DATA_T_DATETIME:
		        if (search_inf->StrVal != NULL)
			    {
			    objDataToDateTime(DATA_T_STRING, search_inf->StrVal, &dt);
			    dtptr = &dt;
			    objSetAttrValue(obj, search_inf->Name, &dtptr);
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
    pStruct find_inf;
    pObjQuery query;
    char* dptr;
    char* ptr;
    char* aptr;
    pObject target_obj, sub_obj;
    char* bufptr;

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
	    sprintf(sbuf,"HTTP/1.0 404 Not Found\r\n"
	    		 "Server: Centrallix/1.0\r\n"
			 "\r\n"
			 "<H1>404 Not Found</H1><HR><PRE>\r\n");
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    mssPrintError(conn);
	    netCloseTCP(conn,1000,0);
	    thExit();
	    }

	/** Do we need to set params as a part of the open? **/
	nht_internal_CkParams(url_inf, target_obj);

	/** Ok, issue the HTTP header for this one. **/
	if (nsess->IsNewCookie)
	    {
	    sprintf(sbuf,"HTTP/1.0 200 OK\r\n"
		     "Server: Centrallix/1.0\r\n"
		     "Set-Cookie: %s\r\n"
		     "\r\n", nsess->Cookie);
	    nsess->IsNewCookie = 0;
	    }
	else
	    {
	    sprintf(sbuf,"HTTP/1.0 200 OK\r\n"
		     "Server: Centrallix/1.0\r\n"
		     "\r\n");
	    }
	fdWrite(conn,sbuf,strlen(sbuf),0,0);

	/** Check GET mode. **/
	find_inf = stLookup_ne(url_inf,"ls__mode");

	/** GET CONTENT mode. **/
	if (!find_inf || !strcmp(find_inf->StrVal, "content"))
	    {
	    /** Check the object type. **/
	    objGetAttrValue(target_obj, "content_type", &ptr);
	    if (!strcmp(ptr,"widget/page") || !strcmp(ptr,"widget/frameset"))
	        {
		/*fdSetOptions(conn, FD_UF_WRBUF);*/
	        htrRender(conn, target_obj);
	        }
	    else
	        {
		bufptr = (char*)nmMalloc(4096);
	        while((cnt=objRead(target_obj,bufptr,4096,0,0)) > 0)
	            {
		    fdWrite(conn,bufptr,cnt,0,FD_U_PACKET);
		    }
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
	    query = objOpenQuery(target_obj,"",NULL,NULL,NULL,0);
	    if (query)
	        {
		sprintf(sbuf,"<HTML><HEAD><META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\"></HEAD><BODY><TT><A HREF=%s/..>..</A><BR>\n",url_inf->StrVal);
		fdWrite(conn,sbuf,strlen(sbuf),0,0);
		dptr = url_inf->StrVal;
		while(*dptr && *dptr == '/' && dptr[1] == '/') dptr++;
		while(sub_obj = objQueryFetch(query,O_RDONLY))
		    {
		    objGetAttrValue(sub_obj, "name", &ptr);
		    objGetAttrValue(sub_obj, "annotation", &aptr);
		    sprintf(sbuf,"<A HREF=%s%s%s TARGET='%s'>%s</A><BR>\n",dptr,
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

	/** GET METHOD LIST mode. **/
	else if (!strcmp(find_inf->StrVal,"methods"))
	    {
	    }

	/** GET ATTRIBUTE-VALUE LIST mode. **/
	else if (!strcmp(find_inf->StrVal,"attr"))
	    {
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
    int rcnt,wcnt;
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
	    sprintf(sbuf,"HTTP/1.0 404 Not Found\r\n"
	    		 "Server: Centrallix/1.0\r\n"
			 "\r\n"
			 "<H1>404 Not Found</H1><HR><PRE>\r\n");
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    mssPrintError(conn);
	    netCloseTCP(conn,1000,0);
	    thExit();
	    }

	/** OK, we're ready.  Send the 100 Continue message. **/
	/*sprintf(sbuf,"HTTP/1.1 100 Continue\r\n"
		     "Server: Centrallix/1.0\r\n"
		     "\r\n");
	fdWrite(conn,sbuf,strlen(sbuf),0,0);*/

	/** If size specified, set the size. **/
	if (size != -1) objSetAttrValue(target_obj, "size", &size);

	/** Set any attributes specified in the url inf **/
	for(i=0;i<url_inf->nSubInf;i++)
	    {
	    sub_inf = url_inf->SubInf[i];
	    type = objGetAttrType(target_obj, sub_inf->Name);
	    if (type == DATA_T_INTEGER)
	        {
		v = strtoi(sub_inf->StrVal,NULL,10);
		objSetAttrValue(target_obj, sub_inf->Name, &v);
		}
	    else if (type == DATA_T_STRING)
	        {
		objSetAttrValue(target_obj, sub_inf->Name, &(sub_inf->StrVal));
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
	        sprintf(sbuf,"HTTP/1.0 200 OK\r\n"
		     "Server: Centrallix/1.0\r\n"
		     "Set-Cookie: %s\r\n"
		     "\r\n"
		     "%s\r\n", nsess->Cookie, url_inf->StrVal);
		}
	    else
	        {
	        sprintf(sbuf,"HTTP/1.0 201 Created\r\n"
		     "Server: Centrallix/1.0\r\n"
		     "Set-Cookie: %s\r\n"
		     "\r\n"
		     "%s\r\n", nsess->Cookie, url_inf->StrVal);
		}
	    nsess->IsNewCookie = 0;
	    }
	else
	    {
	    if (already_exist)
	        {
	        sprintf(sbuf,"HTTP/1.0 200 OK\r\n"
		     "Server: Centrallix/1.0\r\n"
		     "\r\n"
		     "%s\r\n", url_inf->StrVal);
		}
	    else
	        {
	        sprintf(sbuf,"HTTP/1.0 201 Created\r\n"
		     "Server: Centrallix/1.0\r\n"
		     "\r\n"
		     "%s\r\n", url_inf->StrVal);
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
    int cnt,rcnt,wcnt;

	/** Ok, open the source object here. **/
	source_obj = objOpen(nsess->ObjSess, url_inf->StrVal, O_RDONLY, 0600, "text/html");
	if (!source_obj)
	    {
	    sprintf(sbuf,"HTTP/1.0 404 Not Found\r\n"
	    		 "Server: Centrallix/1.0\r\n"
			 "\r\n"
			 "<H1>404 Source Not Found</H1><HR><PRE>\r\n");
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    mssPrintError(conn);
	    netCloseTCP(conn,1000,0);
	    thExit();
	    }

	/** Do we need to set params as a part of the open? **/
	nht_internal_CkParams(url_inf, target_obj);

	/** Get the size of the original object, if possible **/
	if (objGetAttrValue(source_obj,"size",&size) != 0) size = -1;

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
	    sprintf(sbuf,"HTTP/1.0 404 Not Found\r\n"
	    		 "Server: Centrallix/1.0\r\n"
			 "\r\n"
			 "<H1>404 Target Not Found</H1>\r\n");
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    netCloseTCP(conn,1000,0);
	    thExit();
	    }

	/** Set the size of the new document... **/
	if (size >= 0) objSetAttrValue(target_obj, "size", &size);

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
	        sprintf(sbuf,"HTTP/1.0 200 OK\r\n"
		     "Server: Centrallix/1.0\r\n"
		     "Set-Cookie: %s\r\n"
		     "\r\n"
		     "%s\r\n", nsess->Cookie, dest);
		}
	    else
	        {
	        sprintf(sbuf,"HTTP/1.0 201 Created\r\n"
		     "Server: Centrallix/1.0\r\n"
		     "Set-Cookie: %s\r\n"
		     "\r\n"
		     "%s\r\n", nsess->Cookie, dest);
		}
	    nsess->IsNewCookie = 0;
	    }
	else
	    {
	    if (already_exist)
	        {
	        sprintf(sbuf,"HTTP/1.0 200 OK\r\n"
		     "Server: Centrallix/1.0\r\n"
		     "\r\n"
		     "%s\r\n", dest);
		}
	    else
	        {
	        sprintf(sbuf,"HTTP/1.0 201 Created\r\n"
		     "Server: Centrallix/1.0\r\n"
		     "\r\n"
		     "%s\r\n", dest);
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
    char url[256];
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
    void* mts;
    int cnt;
    pStruct url_inf,find_inf;
    int size=-1;

    	/*printf("ConnHandler called, stack ptr = %8.8X\n",&s);*/

	/** Set the thread's name **/
	thSetName(NULL,"HTTP Connection Handler");

        /** Handle parsing exceptions... **/
	Catch(e)
	    {
	    mlxCloseSession(s);
	    mssError(1,"NHT","Failed to parse HTTP request, exiting thread.");
	    sprintf(sbuf,"HTTP/1.0 400 Request Error\n\n%s\n",msg);
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
	strcpy(url,mlxStringVal(s,NULL));
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
		strcpy(cookie, mlxStringVal(s,NULL));
		while(toktype = mlxNextToken(s))
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
		while(toktype = mlxNextToken(s))
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
	    sprintf(sbuf,"HTTP/1.0 401 Unauthorized\r\n"
	    		 "Server: Centrallix/1.0\r\n"
			 "WWW-Authenticate: Basic realm=\"Centrallix\"\r\n"
			 "\r\n"
			 "<H1>Unauthorized</H1>\r\n");
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    netCloseTCP(conn,1000,0);
	    thExit();
	    }

	/** Got authentication.  Parse the auth string. **/
	usrname = strtok(auth,":");
	if (usrname) passwd = strtok(NULL,"\r\n");
	if (!usrname || !passwd) 
	    {
	    sprintf(sbuf,"HTTP/1.0 400 Bad Request\r\n"
	    		 "Server: Centrallix/1.0\r\n"
			 "\r\n"
			 "<H1>400 Bad Request</H1>\r\n");
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    netCloseTCP(conn,1000,0);
	    thExit();
	    }

	/** Check for a cookie -- if one, try to resume a session. **/
	if (*cookie)
	    {
	    nsess = (pNhtSessionData)xhLookup(&(NHT.CookieSessions), cookie);
	    if (nsess)
	        {
		if (strcmp(nsess->Username,usrname) || strcmp(passwd,nsess->Password))
		    {
	    	    sprintf(sbuf,"HTTP/1.0 401 Unauthorized\r\n"
		    		 "Server: Centrallix/1.0\r\n"
				 "WWW-Authenticate: Basic realm=\"Centrallix\"\r\n"
				 "\r\n"
				 "<H1>Unauthorized</H1>\r\n");
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
	        sprintf(sbuf,"HTTP/1.0 401 Unauthorized\r\n"
			     "Server: Centrallix/1.0\r\n"
			     "WWW-Authenticate: Basic realm=\"Centrallix\"\r\n"
			     "\r\n"
			     "<H1>Unauthorized</H1>\r\n");
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
	    xaInit(&nsess->ErrorList,16);
	    nht_internal_CreateCookie(nsess->Cookie);
	    xhAdd(&(NHT.CookieSessions), nsess->Cookie, (void*)nsess);
	    xaAddItem(&(NHT.Sessions), (void*)nsess);
	    }

	/** Parse out the requested url **/
	url_inf = nht_internal_ParseURL(url);
	if (!url_inf)
	    {
	    sprintf(sbuf,"HTTP/1.0 500 Internal Server Error\r\n"
	    		 "Server: Centrallix/1.0\r\n"
			 "\r\n"
			 "<H1>500 Internal Server Error</H1>\r\n");
	    fdWrite(conn,sbuf,strlen(sbuf),0,0);
	    netCloseTCP(conn,1000,0);
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
	            sprintf(sbuf,"HTTP/1.0 400 Method Error\r\n"
	    		 "Server: Centrallix/1.0\r\n"
			 "\r\n"
			 "<H1>400 Method Error - include ls__destination for copy</H1>\r\n");
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
	            sprintf(sbuf,"HTTP/1.0 400 Method Error\r\n"
	    		 "Server: Centrallix/1.0\r\n"
			 "\r\n"
			 "<H1>400 Method Error - include ls__content for put</H1>\r\n");
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
	        sprintf(sbuf,"HTTP/1.0 501 Not Implemented\r\n"
	    		 "Server: Centrallix/1.0\r\n"
			 "\r\n"
			 "<H1>501 Method Not Implemented</H1>\r\n");
	        fdWrite(conn,sbuf,strlen(sbuf),0,0);
	        /*netCloseTCP(conn,1000,0);*/
		}
	    }

	/** Close and exit. **/
	if (url_inf) stFreeInf(url_inf);
	netCloseTCP(conn,1000,0);
	thExit();

    return;
    }



/*** nhtInitialize - initialize the HTTP network handler and start the 
 *** listener thread.
 ***/
int
nhtInitialize()
    {
    int i;

	/*printf("nhtInit called, stack ptr = %8.8X\n",&i);*/

    	/** Initialize the random number generator. **/
	srand48(time(NULL));

	/** Initialize globals **/
	memset(&NHT, 0, sizeof(NHT));
	xhInit(&(NHT.CookieSessions),255,0);
	xaInit(&(NHT.Sessions),256);
	NHT.StdOut = fdOpenFD(1,O_RDWR);

	/** Print version header. **/
	printf("X-CGIVersion: Centrallix/%s prerelease build #%4.4d\r\n",VERSION,BUILD);
	fflush(stdout);
	nht_internal_ConnHandler((void*)(NHT.StdOut));

    return 0;
    }

