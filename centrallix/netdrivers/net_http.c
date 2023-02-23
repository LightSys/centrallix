#include "net_http.h"
#include "cxss/cxss.h"
#include "cxlib/memstr.h"
#include "cxlib/strtcpy.h"
#include "json/json.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2004 LightSys Technology Services, Inc.		*/
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

/*** HTTP server globals instantiation ***/
NHT_t NHT;

/*** Response codes ***/
int nht_codes[] = 
    {
    100,101,
    200,201,202,203,204,205,206,
    300,301,302,303,304,305,307,
    400,401,402,403,404,405,406,407,408,409,410,411,412,413,414,415,416,417,
    500,501,502,503,504,505,
    -1
    };

char* nht_texts[] = 
    {
    "Continue",
    "Switching Protocols",

    "OK",
    "Created",
    "Accepted",
    "Non-Authoritative Information",
    "No Content",
    "Reset Content",
    "Partial Content",

    "Multiple Choices",
    "Moved Permanently",
    "Found",
    "See Other",
    "Not Modified",
    "Use Proxy",
    "Temporary Redirect",
    
    "Bad Request",
    "Unauthorized",
    "Payment Required",
    "Forbidden",
    "Not Found",
    "Method Not Allowed",
    "Not Acceptable",
    "Proxy Authentication Required",
    "Request Timeout",
    "Conflict",
    "Gone",
    "Length Required",
    "Precondition Failed",
    "Request Entity Too Large",
    "Request-URI Too Long",
    "Unsupported Media Type",
    "Requested Range Not Satisfiable",
    "Expectation Failed",

    "Internal Server Error",
    "Not Implemented",
    "Bad Gateway",
    "Service Unavailable",
    "Gateway Timeout",
    "HTTP Version Not Supported",
    NULL
    };


/*** Functions for enumerating users for the cx.sysinfo directory ***/
pXArray
nht_i_UsersAttrList(void* ctx, char* objname)
    {
    pXArray xa;

	if (!objname) return NULL;
	xa = (pXArray)nmMalloc(sizeof(XArray));
	xaInit(xa, 8);
	xaAddItem(xa, "session_cnt");
	xaAddItem(xa, "last_activity");
	xaAddItem(xa, "first_activity");
	xaAddItem(xa, "group_cnt");
	xaAddItem(xa, "app_cnt");

    return xa;
    }
pXArray
nht_i_UsersObjList(void* ctx)
    {
    pXArray xa;
    int i;
    pNhtUser usr;

	xa = (pXArray)nmMalloc(sizeof(XArray));
	xaInit(xa, 64);
	for(i=0;i<xaCount(&NHT.UsersList);i++) 
	    {
	    usr = (pNhtUser)xaGetItem(&NHT.UsersList, i);
	    if (usr->SessionCnt > 0)
		xaAddItem(xa, usr->Username);
	    }

    return xa;
    }
int
nht_i_UsersAttrType(void *ctx, char* objname, char* attrname)
    {

	if (!objname || !attrname) return -1;
	if (!strcmp(attrname, "session_cnt")) return DATA_T_INTEGER;
	else if (!strcmp(attrname, "name")) return DATA_T_STRING;
	else if (!strcmp(attrname, "last_activity")) return DATA_T_DATETIME;
	else if (!strcmp(attrname, "first_activity")) return DATA_T_DATETIME;
	else if (!strcmp(attrname, "group_cnt")) return DATA_T_INTEGER;
	else if (!strcmp(attrname, "app_cnt")) return DATA_T_INTEGER;

    return -1;
    }
int
nht_i_UsersAttrValue(void* ctx, char* objname, char* attrname, void* val_v)
    {
    pObjData val = (pObjData)val_v;
    pNhtUser usr;
    pNhtSessionData s;
    pNhtAppGroup g;
    int cnt;
    int i,j;

	if (!objname || !attrname) return -1;
	usr = (pNhtUser)xhLookup(&(NHT.UsersByName), objname);
	if (!usr || usr->SessionCnt == 0) return -1;
	if (!strcmp(attrname, "session_cnt"))
	    val->Integer = usr->SessionCnt;
	else if (!strcmp(attrname, "name"))
	    val->String = usr->Username;
	else if (!strcmp(attrname, "last_activity"))
	    val->DateTime = &(usr->LastActivity);
	else if (!strcmp(attrname, "first_activity"))
	    val->DateTime = &(usr->FirstActivity);
	else if (!strcmp(attrname, "group_cnt"))
	    {
	    cnt=0;
	    for(i=0;i<usr->Sessions.nItems;i++)
		{
		s = (pNhtSessionData)(usr->Sessions.Items[i]);
		cnt += s->AppGroups.nItems;
		}
	    val->Integer = cnt;
	    }
	else if (!strcmp(attrname, "app_cnt"))
	    {
	    cnt=0;
	    for(i=0;i<usr->Sessions.nItems;i++)
		{
		s = (pNhtSessionData)(usr->Sessions.Items[i]);
		for(j=0;j<s->AppGroups.nItems;j++)
		    {
		    g = (pNhtAppGroup)(s->AppGroups.Items[j]);
		    cnt += g->Apps.nItems;
		    }
		}
	    val->Integer = cnt;
	    }
	else
	    return -1;

    return 0;
    }


/*** Functions for enumerating sessions for the cx.sysinfo directory ***/
pXArray
nht_i_SessionsAttrList(void* ctx, char* objname)
    {
    pXArray xa;

	if (!objname) return NULL;
	xa = (pXArray)nmMalloc(sizeof(XArray));
	xaInit(xa, 8);
	xaAddItem(xa, "username");
	xaAddItem(xa, "last_activity");
	xaAddItem(xa, "first_activity");
	xaAddItem(xa, "group_cnt");
	xaAddItem(xa, "app_cnt");
	xaAddItem(xa, "last_ip");

    return xa;
    }

pXArray
nht_i_SessionsObjList(void* ctx)
    {
    pXArray xa;
    int i;
    pNhtSessionData s;

	xa = (pXArray)nmMalloc(sizeof(XArray));
	xaInit(xa, 64);
	for(i=0;i<xaCount(&NHT.Sessions);i++) 
	    {
	    s = (pNhtSessionData)xaGetItem(&NHT.Sessions, i);
	    if (s)
		xaAddItem(xa, s->S_ID_Text);
	    }

    return xa;
    }

int
nht_i_SessionsAttrType(void *ctx, char* objname, char* attrname)
    {

	if (!objname || !attrname) return -1;
	if (!strcmp(attrname, "username")) return DATA_T_STRING;
	else if (!strcmp(attrname, "name")) return DATA_T_STRING;
	else if (!strcmp(attrname, "last_activity")) return DATA_T_DATETIME;
	else if (!strcmp(attrname, "first_activity")) return DATA_T_DATETIME;
	else if (!strcmp(attrname, "group_cnt")) return DATA_T_INTEGER;
	else if (!strcmp(attrname, "app_cnt")) return DATA_T_INTEGER;
	else if (!strcmp(attrname, "last_ip")) return DATA_T_STRING;

    return -1;
    }

int
nht_i_SessionsAttrValue(void* ctx, char* objname, char* attrname, void* val_v)
    {
    pObjData val = (pObjData)val_v;
    pNhtSessionData s;
    pNhtAppGroup g;
    int cnt;
    int i;

	if (!objname || !attrname) return -1;
	s = (pNhtSessionData)xhLookup(&(NHT.SessionsByID), objname);
	if (!s) return -1;
	if (!strcmp(attrname, "username"))
	    val->String = s->User->Username;
	else if (!strcmp(attrname, "name"))
	    val->String = s->S_ID_Text;
	else if (!strcmp(attrname, "last_activity"))
	    val->DateTime = &(s->LastActivity);
	else if (!strcmp(attrname, "first_activity"))
	    val->DateTime = &(s->FirstActivity);
	else if (!strcmp(attrname, "group_cnt"))
	    val->Integer = s->AppGroups.nItems;
	else if (!strcmp(attrname, "app_cnt"))
	    {
	    cnt=0;
	    for(i=0;i<s->AppGroups.nItems;i++)
		{
		g = (pNhtAppGroup)(s->AppGroups.Items[i]);
		cnt += g->Apps.nItems;
		}
	    val->Integer = cnt;
	    }
	else if (!strcmp(attrname, "last_ip"))
	    val->String = s->LastIPAddr;
	else
	    return -1;

    return 0;
    }


/*** Functions for enumerating application groups for the cx.sysinfo directory ***/
pXArray
nht_i_GroupsAttrList(void* ctx, char* objname)
    {
    pXArray xa;

	if (!objname) return NULL;
	xa = (pXArray)nmMalloc(sizeof(XArray));
	xaInit(xa, 8);
	xaAddItem(xa, "username");
	xaAddItem(xa, "session_id");
	xaAddItem(xa, "last_activity");
	xaAddItem(xa, "first_activity");
	xaAddItem(xa, "start_app_path");
	xaAddItem(xa, "app_cnt");

    return xa;
    }

pXArray
nht_i_GroupsObjList(void* ctx)
    {
    pXArray xa;
    int i,j;
    pNhtSessionData s;
    pNhtAppGroup g;

	xa = (pXArray)nmMalloc(sizeof(XArray));
	xaInit(xa, 64);
	for(i=0;i<xaCount(&NHT.Sessions);i++) 
	    {
	    s = (pNhtSessionData)xaGetItem(&NHT.Sessions, i);
	    if (s)
		{
		for(j=0;j<xaCount(&s->AppGroups);j++)
		    {
		    g = (pNhtAppGroup)xaGetItem(&s->AppGroups, j);
		    if (g)
			xaAddItem(xa, g->G_ID_Text);
		    }
		}
	    }

    return xa;
    }

int
nht_i_GroupsAttrType(void *ctx, char* objname, char* attrname)
    {

	if (!objname || !attrname) return -1;
	if (!strcmp(attrname, "username")) return DATA_T_STRING;
	else if (!strcmp(attrname, "name")) return DATA_T_STRING;
	else if (!strcmp(attrname, "last_activity")) return DATA_T_DATETIME;
	else if (!strcmp(attrname, "first_activity")) return DATA_T_DATETIME;
	else if (!strcmp(attrname, "start_app_path")) return DATA_T_STRING;
	else if (!strcmp(attrname, "app_cnt")) return DATA_T_INTEGER;
	else if (!strcmp(attrname, "session_id")) return DATA_T_STRING;

    return -1;
    }

int
nht_i_GroupsAttrValue(void* ctx, char* objname, char* attrname, void* val_v)
    {
    pObjData val = (pObjData)val_v;
    pNhtSessionData s;
    pNhtAppGroup g;
    int i;
    char sessionid[24];

	if (!objname || !attrname) return -1;
	strtcpy(sessionid, objname, sizeof(sessionid));
	if (!strchr(sessionid,'|')) return -1;
	*(strchr(sessionid,'|')) = '\0';
	s = (pNhtSessionData)xhLookup(&(NHT.SessionsByID), sessionid);
	if (!s) return -1;
	for(i=0;i<s->AppGroups.nItems;i++)
	    {
	    g = (pNhtAppGroup)(s->AppGroups.Items[i]);
	    if (g && !strcmp(objname, g->G_ID_Text))
		break;
	    g = NULL;
	    }
	if (!g) return -1;
	if (!strcmp(attrname, "username"))
	    val->String = g->Session->User->Username;
	else if (!strcmp(attrname, "start_app_path"))
	    val->String = g->StartURL;
	else if (!strcmp(attrname, "name"))
	    val->String = g->G_ID_Text;
	else if (!strcmp(attrname, "session_id"))
	    val->String = g->Session->S_ID_Text;
	else if (!strcmp(attrname, "last_activity"))
	    val->DateTime = &(g->LastActivity);
	else if (!strcmp(attrname, "first_activity"))
	    val->DateTime = &(g->FirstActivity);
	else if (!strcmp(attrname, "app_cnt"))
	    {
	    val->Integer = g->Apps.nItems;
	    }
	else
	    return -1;

    return 0;
    }


/*** Functions for enumerating applications for the cx.sysinfo directory ***/
pXArray
nht_i_AppsAttrList(void* ctx, char* objname)
    {
    pXArray xa;

	if (!objname) return NULL;
	xa = (pXArray)nmMalloc(sizeof(XArray));
	xaInit(xa, 8);
	xaAddItem(xa, "username");
	xaAddItem(xa, "session_id");
	xaAddItem(xa, "group_id");
	xaAddItem(xa, "last_activity");
	xaAddItem(xa, "first_activity");
	xaAddItem(xa, "app_path");

    return xa;
    }

pXArray
nht_i_AppsObjList(void* ctx)
    {
    pXArray xa;
    int i,j,k;
    pNhtSessionData s;
    pNhtAppGroup g;
    pNhtApp a;

	xa = (pXArray)nmMalloc(sizeof(XArray));
	xaInit(xa, 64);
	for(i=0;i<xaCount(&NHT.Sessions);i++) 
	    {
	    s = (pNhtSessionData)xaGetItem(&NHT.Sessions, i);
	    if (s)
		{
		for(j=0;j<xaCount(&s->AppGroups);j++)
		    {
		    g = (pNhtAppGroup)xaGetItem(&s->AppGroups, j);
		    if (g)
			{
			for(k=0;k<xaCount(&g->Apps);k++)
			    {
			    a = (pNhtApp)xaGetItem(&g->Apps, k);
			    if (a)
				xaAddItem(xa, a->A_ID_Text);
			    }
			}
		    }
		}
	    }

    return xa;
    }

int
nht_i_AppsAttrType(void *ctx, char* objname, char* attrname)
    {

	if (!objname || !attrname) return -1;
	if (!strcmp(attrname, "username")) return DATA_T_STRING;
	else if (!strcmp(attrname, "name")) return DATA_T_STRING;
	else if (!strcmp(attrname, "last_activity")) return DATA_T_DATETIME;
	else if (!strcmp(attrname, "first_activity")) return DATA_T_DATETIME;
	else if (!strcmp(attrname, "app_path")) return DATA_T_STRING;
	else if (!strcmp(attrname, "group_id")) return DATA_T_STRING;
	else if (!strcmp(attrname, "session_id")) return DATA_T_STRING;

    return -1;
    }

int
nht_i_AppsAttrValue(void* ctx, char* objname, char* attrname, void* val_v)
    {
    pObjData val = (pObjData)val_v;
    pNhtSessionData s;
    pNhtAppGroup g;
    pNhtApp a;
    int i;
    char sessionid[24];
    char groupid[24];

	if (!objname || !attrname) return -1;
	strtcpy(sessionid, objname, sizeof(sessionid));
	if (!strchr(sessionid,'|')) return -1;
	*(strchr(sessionid,'|')) = '\0';
	s = (pNhtSessionData)xhLookup(&(NHT.SessionsByID), sessionid);
	if (!s) return -1;
	strtcpy(groupid, objname, sizeof(groupid));
	*(strrchr(groupid,'|')) = '\0';
	for(i=0;i<s->AppGroups.nItems;i++)
	    {
	    g = (pNhtAppGroup)(s->AppGroups.Items[i]);
	    if (g && !strcmp(groupid, g->G_ID_Text))
		break;
	    g = NULL;
	    }
	if (!g) return -1;
	for(i=0;i<g->Apps.nItems;i++)
	    {
	    a = (pNhtApp)(g->Apps.Items[i]);
	    if (a && !strcmp(objname, a->A_ID_Text))
		break;
	    a = NULL;
	    }
	if (!a) return -1;
	if (!strcmp(attrname, "username"))
	    val->String = a->Group->Session->User->Username;
	else if (!strcmp(attrname, "app_path"))
	    val->String = a->AppPathname;
	else if (!strcmp(attrname, "name"))
	    val->String = a->A_ID_Text;
	else if (!strcmp(attrname, "session_id"))
	    val->String = a->Group->Session->S_ID_Text;
	else if (!strcmp(attrname, "group_id"))
	    val->String = a->Group->G_ID_Text;
	else if (!strcmp(attrname, "last_activity"))
	    val->DateTime = &(a->LastActivity);
	else if (!strcmp(attrname, "first_activity"))
	    val->DateTime = &(a->FirstActivity);
	else
	    return -1;

    return 0;
    }


/*** nht_i_RegisterSessionInfo() - register a handler for listing
 *** users logged into the server.
 ***/
int
nht_i_RegisterSessionInfo()
    {
    pSysInfoData si;

	/** users list **/
	si = sysAllocData("/session/users", nht_i_UsersAttrList, nht_i_UsersObjList, NULL, nht_i_UsersAttrType, nht_i_UsersAttrValue, NULL, 0);
	sysRegister(si, NULL);

	/** sessions list **/
	si = sysAllocData("/session/sessions", nht_i_SessionsAttrList, nht_i_SessionsObjList, NULL, nht_i_SessionsAttrType, nht_i_SessionsAttrValue, NULL, 0);
	sysRegister(si, NULL);

	/** groups list **/
	si = sysAllocData("/session/appgroups", nht_i_GroupsAttrList, nht_i_GroupsObjList, NULL, nht_i_GroupsAttrType, nht_i_GroupsAttrValue, NULL, 0);
	sysRegister(si, NULL);

	/** apps list **/
	si = sysAllocData("/session/apps", nht_i_AppsAttrList, nht_i_AppsObjList, NULL, nht_i_AppsAttrType, nht_i_AppsAttrValue, NULL, 0);
	sysRegister(si, NULL);

    return 0;
    }


/*** nht_i_AddRequestHeader - add a header to the HTTP request
 ***/
int
nht_i_AddHeader(pXArray hdrlist, char* hdrname, char* hdrvalue, int hdralloc)
    {
    pHttpHeader hdr;
    //int i;

#if 00
	/** Already present? **/
	for(i=0;i<hdrlist->nItems;i++)
	    {
	    hdr = (pHttpHeader)hdrlist->Items[i];
	    if (!strcasecmp(hdr->Name, hdrname))
		{
		if (hdr->ValueAlloc)
		    nmSysFree(hdr->Value);

		/** If null new header value - delete the existing one **/
		if (!hdrvalue)
		    {
		    xaRemoveItem(hdrlist, i);
		    nmFree(hdr, sizeof(HttpHeader));
		    return 0;
		    }

		/** Replace existing header value **/
		hdr->Value = hdrvalue;
		hdr->ValueAlloc = hdralloc;
		return 0;
		}
	    }
#endif

	/** Don't add a NULL header **/
	if (!hdrvalue) return 0;
	
	/** Allocate the header **/
	hdr = (pHttpHeader)nmMalloc(sizeof(HttpHeader));
	if (!hdr)
	    return -1;

	/** Set it up and add it **/
	strtcpy(hdr->Name, hdrname, sizeof(hdr->Name));
	hdr->Value = hdrvalue;
	hdr->ValueAlloc = hdralloc;
	xaAddItem(hdrlist, (void*)hdr);

    return 0;
    }


/*** nht_i_AddResponseHeader - add a HTTP response header
 ***/
int
nht_i_AddResponseHeader(pNhtConn conn, char* hdrname, char* hdrvalue, int hdralloc)
    {
    return nht_i_AddHeader(&conn->ResponseHeaders, hdrname, hdrvalue, hdralloc);
    }


/*** nht_i_AddResponseHeaderQPrintf - add a HTTP response header
 ***/
int
nht_i_AddResponseHeaderQPrintf(pNhtConn conn, char* hdrname, char* hdrfmt, ...)
    {
    va_list va;
    char* hdrval;
    pXString xs;

	xs = xsNew();
	if (!xs) return -1;

	va_start(va, hdrfmt);
	xsQPrintf_va(xs, hdrfmt, va);
	va_end(va);

	hdrval = nmSysStrdup(xsString(xs));
	xsFree(xs);

	if (!hdrval)
	    return -1;

    return nht_i_AddResponseHeader(conn, hdrname, hdrval, 1);
    }


/*** nht_i_FreeHeaders - release HTTP headers from an xarray
 ***/
int
nht_i_FreeHeaders(pXArray xa)
    {
    pHttpHeader hdr;

	/** Loop through, removing and freeing the headers **/
	while (xa->nItems)
	    {
	    hdr = (pHttpHeader)xaGetItem(xa, xa->nItems - 1);
	    if (hdr)
		{
		if (hdr->ValueAlloc)
		    nmSysFree(hdr->Value);
		nmFree(hdr, sizeof(HttpHeader));
		}
	    xaRemoveItem(xa, xa->nItems - 1);
	    }

    return 0;
    }


/*** nht_i_GetHeader - find a header with the given name
 ***/
char*
nht_i_GetHeader(pXArray xa, char* name)
    {
    int i;
    pHttpHeader hdr;

	/** Loop through array **/
	for(i=0;i<xa->nItems;i++)
	    {
	    hdr = (pHttpHeader)xa->Items[i];
	    if (!strcasecmp(hdr->Name, name))
		return hdr->Value;
	    }

    return NULL;
    }


/*** nht_i_WriteConn() - write data to a network connection
 ***/
int
nht_i_WriteConn(pNhtConn conn, char* buf, int len, int is_hdr)
    {
    int wcnt, rval;

	if (len == -1) len = strlen(buf);

	/** Catch a type of error in our header handling **/
	if (is_hdr && conn->InBody)
	    {
	    mssError(1, "NHT", "Attempt to add header information after headers have been emitted");
	    return -1;
	    }

	/** Emit the header/body separator **/
	if (!is_hdr && !conn->InBody)
	    {
	    conn->InBody = 1;
	    fdWrite(conn->ConnFD, "\r\n", 2, 0, FD_U_PACKET);
	    }

	if (len == 0)
	    return 0;

	/** Write the data, direct or as a chunk, and count the bytes we used **/
	wcnt = 0;
	if (conn->UsingChunkedEncoding)
	    {
	    rval = fdPrintf(conn->ConnFD, "%x\r\n", len);
	    if (rval > 0)
		wcnt += rval;
	    }
	rval = fdWrite(conn->ConnFD, buf, len, 0, FD_U_PACKET);
	if (rval > 0)
	    wcnt += rval;
	if (conn->UsingChunkedEncoding)
	    {
	    rval = fdPrintf(conn->ConnFD, "\r\n");
	    if (rval > 0)
		wcnt += rval;
	    }
	if (wcnt > 0 && !is_hdr)
	    conn->BytesWritten += wcnt;

    return wcnt;
    }


/*** nht_i_QPrintfConn() - write data to the network connection,
 *** formatted.
 ***/
int
nht_i_QPrintfConn(pNhtConn conn, int is_hdr, char* fmt, ...)
    {
    va_list va;
    int wcnt;

	if (!is_hdr && !conn->InBody)
	    {
	    conn->InBody = 1;
	    fdWrite(conn->ConnFD, "\r\n", 2, 0, FD_U_PACKET);
	    }

	va_start(va, fmt);
	wcnt = fdQPrintf_va(conn->ConnFD, fmt, va);
	va_end(va);
	if (wcnt > 0 && !is_hdr)
	    conn->BytesWritten += wcnt;

    return wcnt;
    }


/*** error exit handler -- does not return
 ***/
void
nht_i_ErrorExit(pNhtConn conn, int code, char* text)
    {
    pXString err_message;

	/** Write the standard HTTP response **/
	nht_i_WriteResponse(conn, code, text, NULL);

	/** Display error info **/
	err_message = xsNew();
	if (err_message)
	    {
	    mssStringError(err_message);
	    fdQPrintf(conn->ConnFD, "<!doctype html>\r\n<html>\r\n<head><title>Error</title></head>\r\n<body>\r\n<h1>%POS %STR&HTE</h1>\r\n<hr>\r\n<pre>%STR&HTE</pre>\r\n</body>\r\n</html>\r\n",
		    code,
		    text,
		    xsString(err_message)
		    );
	    xsFree(err_message);
	    }

	/** Shutdown the connection and free memory **/
	nht_i_FreeConn(conn);

    thExit(); /* no return */
    }


/*** nht_i_WriteResponse() - write the HTTP response header,
 *** not including content.
 ***/
int
nht_i_WriteResponse(pNhtConn conn, int code, char* text, char* resptxt)
    {
    int wcnt, rval;
    struct tm* thetime;
    time_t tval;
    char tbuf[40];
    int i;
    pHttpHeader hdr;

	/** Get the current date/time **/
	tval = time(NULL);
	thetime = gmtime(&tval);
	strftime(tbuf, sizeof(tbuf), "%a, %d %b %Y %T", thetime);

	/** Record the time and response code **/
	thetime = localtime(&tval);
	strftime(conn->ResponseTime, sizeof(conn->ResponseTime), "%d/%b/%Y:%T %z", thetime);
	conn->ResponseCode = code;
	strtcpy(conn->ResponseText, text, sizeof(conn->ResponseText));

	/** Send the main headers **/
	wcnt = nht_i_QPrintfConn(conn, 1,
		"HTTP/1.0 %INT %STR\r\n"
		"Server: %STR\r\n"
		"Date: %STR\r\n"
		"%[Set-Cookie: %STR; path=/; HttpOnly%]%[; Secure%]%[; SameSite=strict%]%[\r\n%]"
		"%[Content-Length: %INT\r\n%]"
		"%[Content-Type: %STR\r\n%]"
		"%[Pragma: %STR\r\n%]"
		"%[Transfer-Encoding: chunked\r\n%]"
		"Referrer-Policy: same-origin\r\n"
		"Connection: %STR\r\n",
		code,
		text,
		NHT.ServerString,
		tbuf,
		(conn->NhtSession && conn->NhtSession->IsNewCookie && conn->NhtSession->Cookie),
		(conn->NhtSession)?conn->NhtSession->Cookie:NULL,
		(conn->NhtSession && conn->NhtSession->IsNewCookie && conn->NhtSession->Cookie) && conn->UsingTLS,
		(conn->NhtSession && conn->NhtSession->IsNewCookie && conn->NhtSession->Cookie) && conn->StrictSameSite,
		(conn->NhtSession && conn->NhtSession->IsNewCookie && conn->NhtSession->Cookie),
		conn->ResponseContentLength > 0, conn->ResponseContentLength,
		conn->ResponseContentType[0] && !strpbrk(conn->ResponseContentType, "\r\n"), conn->ResponseContentType,
		conn->NoCache, "no-cache",
		conn->UsingChunkedEncoding,
		(conn->Keepalive?"keep-alive":"close")
		);
	if (wcnt < 0) return wcnt;

	/** Send any auxiliary headers **/
	for(i=0;i<conn->ResponseHeaders.nItems;i++)
	    {
	    hdr = (pHttpHeader)conn->ResponseHeaders.Items[i];
	    if (strpbrk(hdr->Name, ": \r\n\t") || strpbrk(hdr->Value, "\r\n"))
		continue;
	    rval = nht_i_QPrintfConn(conn, 1, "%STR: %STR\r\n", hdr->Name, hdr->Value);
	    if (rval < 0) return rval;
	    wcnt += rval;
	    }

	/** End of headers **/
	/*rval = fdPrintf(conn->ConnFD, "\r\n");
	if (rval < 0) return rval;
	wcnt += rval;*/

	/** Send any response text **/
	if (resptxt && *resptxt)
	    {
	    rval = nht_i_WriteConn(conn, resptxt, strlen(resptxt), 0);
	    if (rval < 0) return rval;
	    wcnt += rval;
	    }

	/** Mark the cookie as having been sent **/
	if (conn->NhtSession && conn->NhtSession->IsNewCookie && conn->NhtSession->Cookie)
	    conn->NhtSession->IsNewCookie = 0;

    return wcnt;
    }


/*** nht_i_WriteErrResponse() - write an HTTP error response
 *** header without any real content.  Use the normal WriteResponse
 *** routine if you want more flexible content.
 ***/
int
nht_i_WriteErrResponse(pNhtConn conn, int code, char* text, char* bodytext)
    {
    int wcnt;

	conn->ResponseContentLength = bodytext?strlen(bodytext):0;
	wcnt = nht_i_WriteResponse(conn, code, text, bodytext);

    return wcnt;
    }


/*** nht_i_FreeControlMsg() - release memory used by a control
 *** message, its parameters, etc.
 ***/
int
nht_i_FreeControlMsg(pNhtControlMsg cm)
    {
    int i;
    pNhtControlMsgParam cmp;

	/** Destroy semaphore? **/
	if (cm->ResponseSem) syDestroySem(cm->ResponseSem, SEM_U_HARDCLOSE);

	/** Release params? **/
	for(i=0;i<cm->Params.nItems;i++)
	    {
	    cmp = (pNhtControlMsgParam)cm->Params.Items[i];
	    if (cmp->P1) nmSysFree(cmp->P1);
	    if (cmp->P2) nmSysFree(cmp->P2);
	    if (cmp->P3) nmSysFree(cmp->P3);
	    if (cmp->P3a) nmSysFree(cmp->P3a);
	    if (cmp->P3b) nmSysFree(cmp->P3b);
	    if (cmp->P3c) nmSysFree(cmp->P3c);
	    if (cmp->P3d) nmSysFree(cmp->P3d);
	    nmFree(cmp, sizeof(NhtControlMsgParam));
	    }

	/** Free the control msg structure itself **/
	nmFree(cm, sizeof(NhtControlMsg));

    return 0;
    }

#if 0
int
nht_i_CacheHandler(pNhtConn conn)
    {
#if 0
    pWgtrNode tree = xmGet(conn->NhtSession->CachedApps2, "0");

#if 1
    /** cache the app **/
    char buf[1<<10];
    sprintf(buf, "%i", NHT.numbCachedApps);
    NHT.numbCachedApps++;
    xmAdd(&nsess->CachedApps, buf, tree);
#else
    xaAddItem(&nsess->CachedApps2, NHT.numbCachedApps)
    NHT.numbCachedApps++;
#endif

    if(! (wgtrVerify(tree, client_info) >= 0))
	{
	if(tree) wgtrFree(tree);
	return -1;
	}

    rval = wgtrRender(output, s, tree, app_params, client_info, method);

    if(tree) wgtrFree(tree);
    return rval;
    //fdQPrintf(conn->ConnFD, "Hello World!");    
#endif
    }
#endif



/*** nht_i_ControlMsgHandler() - the main handler for all connections
 *** which access /INTERNAL/control, thus requesting to receive control
 *** messages from the system.
 ***/
int
nht_i_ControlMsgHandler(pNhtConn conn, pStruct url_inf)
    {
    pNhtControlMsg cm, usr_cm;
    pNhtControlMsgParam cmp;
    pNhtSessionData sess = conn->NhtSession;
    int i;
    char* response = NULL;
    char* cm_ptr = "0";
    char* err_ptr = NULL;
    int wait_for_sem = 1;
    char* ptr;

	/** No delay? **/
	if (stAttrValue_ne(stLookup_ne(url_inf, "cx_cm_nowait"), &ptr) == 0 && !strcmp(ptr,"1"))
	    wait_for_sem = 0;

	/** Control message response? **/
	stAttrValue_ne(stLookup_ne(url_inf, "cx_cm_response"), &response);
	if (response)
	    {
	    /** Get control message id **/
	    stAttrValue_ne(stLookup_ne(url_inf, "cx_cm_id"), &cm_ptr);
	    usr_cm = (pNhtControlMsg)strtoul(cm_ptr, NULL, 16);
	    cm = NULL;
	    for(i=0;i<sess->ControlMsgsList.nItems;i++)
		{
		if ((pNhtControlMsg)(sess->ControlMsgsList.Items[i]) == usr_cm)
		    {
		    cm = usr_cm;
		    break;
		    }
		}

	    /** No such id? **/
	    if (!cm)
		{
		conn->NoCache = 1;
		nht_i_WriteResponse(conn, 200, "OK", "<A HREF=0.0 TARGET=0>NO SUCH MESSAGE</A>\r\n");
		return 0;
		}

	    /** Is this an error? **/
	    stAttrValue_ne(stLookup_ne(url_inf, "cx_cm_error"), &err_ptr);
	    if (err_ptr)
		{
		cm->Status = NHT_CONTROL_S_ERROR;
		cm->Response = err_ptr;
		}
	    else
		{
		cm->Status = NHT_CONTROL_S_RESPONSE;
		cm->Response = response;
		}

	    /** Remove from queue and do response action (sem or fn call) **/
	    xaRemoveItem(&(sess->ControlMsgsList), xaFindItem(&(sess->ControlMsgsList), cm));
	    if (cm->ResponseSem)
		syPostSem(cm->ResponseSem, 1, 0);
	    else if (cm->ResponseFn)
		cm->ResponseFn(cm);
	    else
		nht_i_FreeControlMsg(cm);

	    /** Tell syGetSem to return immediately, below **/
	    wait_for_sem = 0;
	    }

	/** Send header **/
	conn->NoCache = 1;
	nht_i_WriteResponse(conn, 200, "OK", NULL);

    	/** Wait on the control msgs semaphore **/
	while (1)
	    {
	    if (syGetSem(sess->ControlMsgs, 1, wait_for_sem?0:SEM_U_NOBLOCK) < 0)
		{
		nht_i_WriteConn(conn, "<A HREF=0.0 TARGET=0>END OF CONTROL MESSAGES</A>\r\n", -1, 0);
		return 0;
		}

	    /** Grab one control message **/
	    for(i=0;i<sess->ControlMsgsList.nItems;i++)
		{
		cm = (pNhtControlMsg)(sess->ControlMsgsList.Items[i]);
		if (cm->Status == NHT_CONTROL_S_QUEUED)
		    {
		    cm->Status = NHT_CONTROL_S_SENT;
		    break;
		    }
		}

	    /** Send ctl message header **/
            // TODO Once LONG is implemented in qprintf, use it
	    nht_i_QPrintfConn(conn, 0, "<A HREF=%INT.%INT TARGET=%POS>CONTROL MESSAGE</A>\r\n",
			    cm->MsgType,
			    cm->Params.nItems,
			    (unsigned int)(unsigned long)cm);

	    /** Send parameters **/
	    for(i=0;i<cm->Params.nItems;i++)
		{
		cmp = (pNhtControlMsgParam)(cm->Params.Items[i]);
		if (cmp->P3a)
		    {
		    /** split-up HREF **/
		    fdPrintf(conn->ConnFD, "<A HREF=\"http://%s/%s?%s#%s\" TARGET=\"%s\">%s</A>\r\n",
			    cmp->P3a,
			    cmp->P3b?cmp->P3b:"",
			    cmp->P3c?cmp->P3c:"",
			    cmp->P3d?cmp->P3d:"",
			    cmp->P1?cmp->P1:"",
			    cmp->P2?cmp->P2:"");
		    }
		else
		    {
		    /** unified HREF **/
		    fdPrintf(conn->ConnFD, "<A HREF=\"%s\" TARGET=\"%s\">%s</A>\r\n",
			    cmp->P3?cmp->P3:"",
			    cmp->P1?cmp->P1:"",
			    cmp->P2?cmp->P2:"");
		    }
		}
	    printf("NHT: sending message\n");

	    /** Dequeue message if no response needed? **/
	    if (!cm->ResponseSem && !cm->ResponseFn)
		{
		xaRemoveItem(&(sess->ControlMsgsList), xaFindItem(&(sess->ControlMsgsList), cm));
		nht_i_FreeControlMsg(cm);
		}

	    wait_for_sem = 0;
	    }

    return 0;
    }



/*** nht_i_ErrorHandler - handle the printing of notice and error
 *** messages to the error stream for the client, if the client has such
 *** an error stream (which is how this is called).
 ***/
int
nht_i_ErrorHandler(pNhtConn net_conn)
    {
    pNhtSessionData nsess = net_conn->NhtSession;
    pXString errmsg;

    	/** Wait on the errors semaphore **/
	if (syGetSem(nsess->Errors, 1, 0) < 0)
	    {
	    net_conn->NoCache = 1;
	    nht_i_WriteResponse(net_conn, 200, "OK", "<A HREF=/ TARGET=ERR></A>\r\n");
	    return -1;
	    }

	/** Grab one error **/
	errmsg = (pXString)(nsess->ErrorList.Items[0]);
	xaRemoveItem(&nsess->ErrorList, 0);

	/** Format the error and print it as HTML. **/
	net_conn->NoCache = 1;
	nht_i_WriteResponse(net_conn, 200, "OK", NULL);
	nht_i_QPrintfConn(net_conn, 0, "<HTML><BODY><PRE><A NAME=\"Message\">%STR&HTE</A></PRE></BODY></HTML>\r\n", errmsg->String);

	/** Discard the string **/
	xsDeInit(errmsg);
	nmFree(errmsg,sizeof(XString));

    return 0;
    }


/*** nht_i_GenerateError - grab up the current error text listing
 *** and queue it on the outbound error queue for this session, so that
 *** the error stream reader in the DHTML client can pick it up and show
 *** it to the user.
 ***/
int
nht_i_GenerateError(pNhtSessionData nsess)
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


/*** nht_i_Hex16ToInt - convert a 16-bit hex value, in four chars, to
 *** an unsigned integer.
 ***/
unsigned int
nht_i_Hex16ToInt(char* hex)
    {
    char hex2[5];
    memcpy(hex2, hex, 4);
    hex2[4] = '\0';
    return strtoul(hex2, NULL, 16);
    }


/*** nht_i_Logout - log the current session out.
 ***/
int
nht_i_Logout(pNhtConn conn, pNhtAppGroup group, pNhtApp app, int do_all)
    {
    pNhtSessionData nsess = conn->NhtSession;
    int do_logout = (app != NULL);

	/** Send the answer to the user **/
	conn->NoCache = 1;
	strtcpy(conn->ResponseContentType, "application/json", sizeof(conn->ResponseContentType));
	nht_i_WriteResponse(conn, 200, "OK", NULL);
	nht_i_QPrintfConn(conn, 0,
		"{\"logout\":%STR, \"all\":%STR}\r\n",
		do_logout?"true":"false",
		do_all?"true":"false"
		);

	/** Do the logout by unlinking the session **/
	if (do_logout)
	    {
	    if (do_all)
		nht_i_LogoutUser(nsess->User->Username);
	    else
		nht_i_UnlinkSess(nsess);
	    }

    return 0;
    }


/*** nht_i_ParsePostPayload - parses the payload of a post request.
 *** The request includes one or more files.  This function needs to be
 *** called once for each file recieved and -not- per request.  Payload
 *** contains relevant info about the given file and a pointer to the file on disk.
 *** Returns the payload struct.
 ***/
pNhtPostPayload
nht_i_ParsePostPayload(pNhtConn conn)
    {
    char hex_chars[] = "0123456789abcdef";
    char token[512];
    char name[48];
    char *ptr, *half;
    int length, start;
    int fd, i, extStart;
    pFile file;
    char buffer[2048];
    int offset = 1024;
    int rcnt;
    int allowed;
    int rval;
    pNhtPostPayload payload = (pNhtPostPayload) nmMalloc(sizeof *payload);
    pContentType ct;
    char* apparent_type;

    memset(payload, 0, sizeof(*payload));
    
    /* Remove the boundary line and test to see if the stream is empty */
    length = nht_i_NextLine(token, conn, sizeof token);
    if(length <= 0)
	{
	payload->status = 1;
	return payload; /* Signals end of stream */
	}
    if(strncmp(token, "--", 2) != 0 || strncmp(token + 2, conn->RequestBoundary, strlen(conn->RequestBoundary)) != 0) //Verifying data is formatted the way I hope it is :p
	{
	payload->status = -1;
	mssError(1,"NHT","Malformed file upload POST request received - MIME boundary not detected");
	return payload; //Error
	}
	
    length = nht_i_NextLine(token, conn, sizeof token);
    if(length <= 0)
	{
	payload->status = -1;
	mssError(0,"NHT","Malformed file upload POST request received - no content following boundary");
	return payload; //Error
	}
	
    /* find the filename property and copy it to payload */
    ptr = strstr(token, "filename");
    if(ptr == NULL)
	{
	mssError(1,"NHT","Malformed file upload POST request received - no filename specified");
	payload->status = -1;
	return payload; //Error
	}
    ptr = strchr(ptr, '\"');
    if(ptr == NULL)
	{
	mssError(1,"NHT","Malformed file upload POST request received - start of filename not found");
	payload->status = -1;
	return payload; //Error
	}
    ptr++;
    
    extStart = -1;
    for(i=0; ptr[i] != '\"' && ptr[i] != '\0'; i++)
	{
	if (i >= sizeof(payload->filename) - 1)
	    {
	    mssError(1,"NHT","Invalid filename in file upload POST request");
	    payload->status = -1;
	    return payload; //Error
	    }
	if(ptr[i] == '.') extStart = i; //Also find the file extension
	payload->filename[i] = ptr[i];
	}
    payload->filename[i] = '\0';
    
    /* Copy the extension to payload */
    if (extStart >= 0)
	{
	ptr = payload->filename + extStart;
	for(i=0; ptr[i] != '\0'; i++)
	    {
	    if (i >= sizeof(payload->extension) - 1)
		{
		mssError(1,"NHT","Invalid filename in file upload POST request");
		payload->status = -1;
		return payload; //Error
		}
	    payload->extension[i] = ptr[i];
	    }
	payload->extension[i] = '\0';

	/** Validate that this extension is allowed **/
	allowed = 0;
	for(i=0; i<NHT.AllowedUploadExts.nItems; i++)
	    {
	    if (!strcasecmp(payload->extension+1, NHT.AllowedUploadExts.Items[i]))
		{
		allowed = 1;
		break;
		}
	    }
	if (!allowed)
	    {
	    mssError(1,"NHT","Uploaded file extension not allowed on this server");
	    payload->status = -1;
	    return payload;
	    }
	}
    
    /* find the "ContentType" which seems to be the MIME type.  This may possibly be sorta useful sometime in the future.  Maybe... */
    length = nht_i_NextLine(token, conn, sizeof token);
    if(length <= 0)
	{
	payload->status = -1;
	mssError(0,"NHT","Malformed file upload POST request received - no content-type line");
	return payload; //Error
	}
    ptr = strstr(token, "Content-Type:");
    if(!ptr)
	{
	payload->status = -1;
	mssError(1,"NHT","Malformed file upload POST request received - no content-type");
	return payload; //Error
	}
    ptr = strchr(ptr, ' ');
    if(!ptr)
	{
	payload->status = -1;
	mssError(1,"NHT","Malformed file upload POST request received - bad content-type");
	return payload; //Error
	}
    ptr++;
    
    for(i=0; ptr[i] != '\r' && ptr[i] != '\n' && ptr[i] != '\0'; i++)
	{
	if (i >= sizeof(payload->mime_type) - 1)
	    {
	    mssError(1,"NHT","Invalid MIME type in file upload POST request");
	    payload->status = -1;
	    return payload; //Error
	    }
	payload->mime_type[i] = ptr[i];
	}
    payload->mime_type[i] = '\0';

    length = nht_i_NextLine(token, conn, sizeof token);
    if (strncasecmp(token, "Content-Length", 14) == 0)
	{
	length = 0;
	start = 4;
	}
    else
	{
	start = 0;
	}

    /** some user agents use "binary/octet-stream" for some obtuse reason **/
    if (!strcasecmp(payload->mime_type, "binary/octet-stream"))
	strtcpy(payload->mime_type, "application/octet-stream", sizeof(payload->mime_type));

    /** Verify the mime type is valid -- check is that the type is or is
     ** a subtype of application/octet-stream (this basically makes sure we know
     ** about the file type).
     **/
    rval = objIsA(payload->mime_type, "application/octet-stream");
    if (rval == OBJSYS_NOT_ISA)
	{
	mssError(1,"NHT","Disallowed file MIME type for upload POST request");
	payload->status = -1;
	return payload;
	}
    if (rval < 0)
	strcpy(payload->mime_type,"application/octet-stream");

    /* Generate a random name for the file. */
    while(1)
	{
	strtcpy(name, payload->filename, 33);
	if (strrchr(name, '.'))
	    *(strrchr(name, '.')) = '\0';
	for(i=0;i<strlen(name);i++)
	    {
	    if ((name[i] >= '0' && name[i] <= '9') || (name[i] >= 'a' && name[i] <= 'z') || (name[i] >= 'A' && name[i] <= 'Z') || name[i] == '_' || name[i] == '+' || name[i] == '-')
		continue;
	    name[i] = '_';
	    }
	strtcat(name, "-", 34);
	ptr = strchr(name, '\0');
	cxssGenerateKey((unsigned char*)ptr, 12);
	for(i=0; i<12; i++)
	    {
	    ptr[i] = hex_chars[ptr[i] & 0x0f];
	    }
	ptr[i] = '\0';
	
	/** Append the extension to the unique name and store it in payload **/
	if (strlen(name) + strlen(payload->extension) >= sizeof(payload->newname) - 1 || strlen(name) + strlen(payload->extension) + 9 >= sizeof(payload->full_new_path) - 1)
	    {
	    mssError(1,"NHT","Invalid filename in file upload POST request - could not generate temp file name");
	    payload->status = -1;
	    return payload; //Error
	    }
	snprintf(payload->newname, sizeof (payload->newname), "%s%s", name, payload->extension);
	snprintf(payload->path, sizeof (payload->path), NHT.UploadTmpDir);
	snprintf(payload->full_new_path, sizeof (payload->full_new_path), "%s/%s", payload->path, payload->newname);

	/** Validate that the new file name matches the mime type **/
	ct = objTypeFromName(payload->full_new_path);
	if (ct)
	    apparent_type = ct->Name;
	else
	    apparent_type = "application/octet-stream";
	if (objIsA(apparent_type, payload->mime_type) == OBJSYS_NOT_ISA)
	    {
	    mssError(1,"NHT","Unknown or mismatched file type and file extension for POST upload request");
	    payload->status = -1;
	    return payload;
	    }

	/* Try to create the file.  It will fail if the file already exists. */
	/** Actually creating the file prevents race conditions that can occur
	 ** if we only checked for the file here. i.e. the file could otherwise
	 ** be created between the time we check and the time we actually open
	 ** the file.
	 **/
	fd = open(payload->full_new_path, O_CREAT | O_RDWR | O_EXCL, 0660);
	if (fd>=0)
	    {
	    close(fd);
	    //printf("Created new file named %s.\n", payload->full_new_path);
	    break;
	    }
	else
	    {
	    //printf("File %s already exists.  Creating new name.\n", payload->full_new_path);
	    }
	}
	
    /* Read in the data and store into a file. */
    file = fdOpen(payload->full_new_path, O_RDWR | O_CREAT | O_TRUNC, 0660);
    if(!file)
	{
	mssErrorErrno(1,"CX","Unable to open output file '%s'",payload->full_new_path);
	payload->status = -1;
	return payload; //Error
	}

    /** Data transfer loop **/
    if (length)
	fdWrite(file, token, length, 0, 0);
    length = fdRead(conn->ConnFD, buffer, sizeof buffer, 0, 0);
    half = buffer + offset;
    while(length > 0)
	{
	ptr = memstr(buffer, conn->RequestBoundary, length);
	if(!ptr)
	    {
	    if(length < sizeof buffer)
		{
		rcnt = fdRead(conn->ConnFD, buffer + length, sizeof buffer - length, 0, 0);
		if (rcnt < 0)
		    {
		    mssErrorErrno(1,"NHT","Interrupted upload to file '%s'",payload->full_new_path);
		    payload->status = -1;
		    return payload; //Error
		    }
		else if (rcnt == 0)
		    {
		    mssError(1,"NHT","Short upload to file '%s'",payload->full_new_path);
		    payload->status = -1;
		    return payload; //Error
		    }
		length += rcnt;
		}
	    else
		{
		if (fdWrite(file, buffer+start, offset-start, 0, 0) < 0)
		    {
		    mssErrorErrno(1,"NHT","Error writing to uploaded file '%s'",payload->full_new_path);
		    payload->status = -1;
		    return payload; //Error
		    }
		memmove(buffer, half, offset);
		memset(half, 0, offset);
		rcnt = fdRead(conn->ConnFD, half, offset, 0, 0);
		if (rcnt < 0)
		    {
		    mssErrorErrno(1,"NHT","Interrupted upload to file '%s'",payload->full_new_path);
		    payload->status = -1;
		    return payload; //Error
		    }
		else if (rcnt == 0)
		    {
		    mssError(1,"NHT","Short upload to file '%s'",payload->full_new_path);
		    payload->status = -1;
		    return payload; //Error
		    }
		length = offset + rcnt;
		start = 0;
		}
	    }
	else
	    {
	    //The 4 chars between boundary and EOF should be "\r\n--"
	    if (fdWrite(file, buffer+start, (ptr - buffer - 4) - start, 0, 0) < 0)
		{
		mssErrorErrno(1,"NHT","Error writing to uploaded file '%s'",payload->full_new_path);
		payload->status = -1;
		return payload; //Error
		}
	    fdUnRead(conn->ConnFD, ptr - 4, length - (ptr - buffer - 4), 0, 0); //Unread remainder of data.
	    start = 0;
	    break;
	    }
	}
    fdClose(file, 0);
    //printf("\n");
    
    payload->status = 0;
    return payload;
    }
   

/*** nht_i_NextLine - fills token with the next line of data.
 *** Ignores leading newlines.  Returns the length of data in the buffer.
 *** Size is the size of token.  If the buffer is too small, returns -1
 *** and leaves token empty.  Token is always null terminated.
 ***/
int
nht_i_NextLine(char * token, pNhtConn conn, int size)
    {
    char buffer[2048];
    int length;
    int start, end;
    int i;
    
    start = -1;
    end = -1;
    length = fdRead(conn->ConnFD, buffer, (sizeof buffer) - 1, 0, 0);
    if (length < 0)
	{
	mssErrorErrno(1,"NHT","Interrupted network connection during upload");
	return -1;
	}
    
    for(i = 0; i < length; i++)
	{
	if(start == -1 && !(buffer[i] == '\r' || buffer[i] == '\n')) start = i;
	if(start != -1 &&  (buffer[i] == '\r' || buffer[i] == '\n'))
	    {
	    end = i;
	    break;
	    }
	}
	
    if(start < 0) 
	{
	token[0] = '\0';
	return -1; /* No relevant data in the buffer. */
	}
    if(start >= 0 && end < 0)
	{
	if(length < (sizeof buffer) - 1)
	    {
	    /* Max size wasn't read; means end of stream. */
	    /** GRB: FIXME: network connections can read partial buffers
	     ** without indicating end of stream.  End of stream is indicated
	     ** by a zero read.  We should read further into the buffer at
	     ** least once if we hit this here.
	     **/
	    end = length;
	    }
	else
	    {
	    /* No newline was read; unread buffer. */
	    fdUnRead(conn->ConnFD, buffer, length, 0, 0);
	    token[0] = '\0';
	    return -1;
	    }
	}
   
    /** end-start+1 --> means the length of the token plus the length of
     ** the terminating \0 character.
     **/
    if(size > (end - start + 1)) size = (end-start + 1);
    if(size < (end - start + 1)) end -= ((end-start + 1) - size);
    memcpy(token, buffer + start , size-1); /* Copy relevant data into token. */
    token[size-1] = '\0';
    fdUnRead(conn->ConnFD, buffer+end, length - end, 0, 0); //Unread extra data.
    return size-1;
    }


/*** nht_i_POST - handle the HTTP POST method.
 ***/
int
nht_i_POST(pNhtConn conn, pStruct url_inf, int size, char* content)
    {
    pNhtSessionData nsess = conn->NhtSession;
    pStruct find_inf;
    pFile file = NULL;
    pObject obj;
    pNhtPostPayload payload;
    pXString json = NULL;
    char buffer[2048];
    int length, wcnt, bytes_written;
    pNhtApp app = NULL;
    pNhtAppGroup group = NULL;
    int n_uploaded_files = 0;
    int allowed, i;
   
	/** Validate akey and make sure app and group id's match as well.  AKey
	 ** must be supplied with all POST requests.
	 **/
	find_inf = stLookup_ne(url_inf,"cx__akey");
	if (!find_inf || nht_i_VerifyAKey(find_inf->StrVal, nsess, &group, &app) != 0 || !group || !app)
	    {
	    nht_i_WriteErrResponse(conn, 403, "Forbidden", NULL);
	    goto error;
	    }

	/** REST-type request vs. standard file upload POST? **/
	find_inf = stLookup_ne(url_inf,"cx__mode");
	if (find_inf && !strcmp(find_inf->StrVal, "rest"))
	    {
	    conn->StrictSameSite = 0;
	    if (nht_i_RestPost(conn, url_inf, size, content) < 0)
		goto error;
	    else
		return 0;
	    }
	   
	/** Standard file upload POST request **/
	find_inf = NULL;
	find_inf = stLookup_ne(url_inf,"target");
	if(!find_inf)
	    {
	    nht_i_WriteErrResponse(conn, 400, "Bad Request", NULL);
	    goto error;
	    }

	/** Validate the target location **/
	allowed = 0;
	for(i=0; i<NHT.AllowedUploadDirs.nItems; i++)
	    {
	    if (!strcmp(NHT.AllowedUploadDirs.Items[i], find_inf->StrVal))
		{
		allowed = 1;
		break;
		}
	    }
	if (!allowed)
	    {
	    nht_i_WriteErrResponse(conn, 400, "Bad Request", NULL);
	    goto error;
	    }
	
	/** Keep parsing files until stream is empty. **/
	json = xsNew();
	while(1)
	    {
	    payload = nht_i_ParsePostPayload(conn);
	    
	    if(payload->status == 0)
		{
		/** Copy file into object system **/
		file = fdOpen(payload->full_new_path, O_RDONLY, 0660);
		if (!file)
		    {
		    mssErrorErrno(1, "NHT", "POST request: could not open file %s", payload->full_new_path);
		    nht_i_WriteErrResponse(conn, 500, "Internal Server Error", NULL);
		    goto error;
		    }
		snprintf(buffer, sizeof buffer, "%s/%s", find_inf->StrVal, payload->newname);
		xsConcatQPrintf(json, ",{\"fn\":\"%STR&JSONSTR\",\"up\":\"%STR&JSONSTR\"}", payload->filename, buffer);
		obj = objOpen(nsess->ObjSess, buffer, O_CREAT | O_RDWR | O_EXCL, 0660, "application/file");
		if (!obj)
		    {
		    mssError(0, "NHT", "POST request: could not create object %s", buffer);
		    nht_i_WriteErrResponse(conn, 403, "Forbidden", NULL);
		    goto error;
		    }
		while(1)
		    {
		    length = fdRead(file, buffer, sizeof buffer, 0, 0);
		    if (length <= 0)
			break;
		    bytes_written = 0;
		    while (bytes_written < length)
			{
			wcnt = objWrite(obj, buffer+bytes_written, length-bytes_written, 0, 0);
			if (wcnt <= 0)
			    break;
			bytes_written += wcnt;
			}
		    }
		objClose(obj);
		fdClose(file, 0);
		file = NULL;
		unlink(payload->full_new_path);
		n_uploaded_files++;
		}
	    else
		{
		break;
		}
	    }
	xsRTrim(json);

	/** Error if POST with no files **/
	if (n_uploaded_files == 0)
	    {
	    nht_i_WriteErrResponse(conn, 400, "Bad Request", NULL);
	    goto error;
	    }

	/** Send out the response **/
	nht_i_WriteResponse(conn, 202, "Accepted", NULL);
	nht_i_QPrintfConn(conn, 0, "[%STR]", json->String+1);

	/** Clean up **/
	xsFree(json);

	return 0;

    error:
	if (json)
	    xsFree(json);
	if (file)
	    fdClose(file, 0);
	return -1;
    }


/*** nht_i_GET - handle the HTTP GET method, reading a document or
 *** attribute list, etc.
 ***/
int
nht_i_GET(pNhtConn conn, pStruct url_inf, char* if_modified_since)
    {
    pNhtSessionData nsess = conn->NhtSession;
    int cnt;
    pStruct find_inf,find_inf2;
    pObjQuery query;
    char* dptr;
    char* ptr;
    char* aptr;
    char* acceptencoding;
    pObject target_obj, sub_obj, tmp_obj;
    char* bufptr;
    char path[256];
    int rowid;
    int convert_text = 0;
    pDateTime dt = NULL;
    DateTime dtval;
    struct tm systime;
    struct tm* thetime;
    time_t tval;
    char tbuf[32];
    char tbuf2[32];
    int send_info = 0;
    pObjectInfo objinfo;
    char* gptr;
    int client_h, client_w;
    int gzip;
    int i;
    WgtrClientInfo wgtr_params;
    int akey_match = 0;
    char* tptr;
    char* tptr2;
    char* lptr;
    char* lptr2;
    char* pptr;
    char* xfo_ptr;
    int rowlimit;
    int order_desc = 0;
    char* name;
    char* slashptr;
    pNhtApp app = NULL;
    pNhtAppGroup group = NULL;
    int rval;
    char* kname;
    pXString err_xs;

	acceptencoding=(char*)mssGetParam("Accept-Encoding");

//START INTERNAL handler -------------------------------------------------------------------
//TODO (from Seth): should this be moved out of nht_i_GET and back into nht_i_ConnHandler?

    	/*printf("GET called, stack ptr = %8.8X\n",&cnt);*/
        /** If we're opening the "errorstream", pass of processing to err handler **/
	if (!strncmp(url_inf->StrVal,"/INTERNAL/errorstream",21))
	    {
		return nht_i_ErrorHandler(conn);
	    }
	else if (!strncmp(url_inf->StrVal, "/INTERNAL/control", 17))
	    {
		return nht_i_ControlMsgHandler(conn, url_inf);
	    }
#if 0 //TODO: finish the caching ability. (this section could very well belong somewhere else)
	else if (!strncmp(url_inf->StrVal, "/INTERNAL/cache", 15))
	    {
		return htrRenderObject(conn->ConnFD, target_obj->Session, target_obj, url_inf, &wgtr_params, "DHTML", nsess);
	    }
#endif

//END INTERNAL handler ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	/** app key specified? **/ //the app key feature is to prevent CSRFing attacks
	find_inf = stLookup_ne(url_inf,"cx__akey");
	if (find_inf)
	    {
	    /** VerifyAKey returns success if the session token matches.  We
	     ** need the group and app tokens to match too for full security
	     ** authorization.
	     **/
	    if (nht_i_VerifyAKey(find_inf->StrVal, nsess, &group, &app) == 0)
		{
		if (group && app) 
		    {
		    cxssAddEndorsement("system:from_application", "*");
		    akey_match = 1;
		    }
		if (group)
		    {
		    cxssAddEndorsement("system:from_appgroup", "*");
		    }
		}
	    }

	/** Logout? **/
	if (!strncmp(url_inf->StrVal, "/INTERNAL/logoutall", 19))
	    {
	    return nht_i_Logout(conn, group, app, 1);
	    }
	if (!strncmp(url_inf->StrVal, "/INTERNAL/logout", 16))
	    {
	    return nht_i_Logout(conn, group, app, 0);
	    }

	/** Indicate activity... **/
	if (!conn->NotActivity)
	    {
	    if (group) objCurrentDate(&(group->LastActivity));
	    if (app) objCurrentDate(&(app->LastActivity));
	    /*if (group) nht_i_ResetWatchdog(group->InactivityTimer);
	    if (app) nht_i_ResetWatchdog(app->InactivityTimer);*/
	    }
	if (group) nht_i_ResetWatchdog(group->WatchdogTimer);
	if (app) nht_i_ResetWatchdog(app->WatchdogTimer);

	/** Check GET mode. **/
	find_inf = stLookup_ne(url_inf,"cx__mode");
	if (!find_inf)
	    find_inf = stLookup_ne(url_inf,"ls__mode"); /* compatibility */

	/** Ok, open the object here, if not using OSML mode. **/
	if (!find_inf || strcmp(find_inf->StrVal,"osml") != 0)
	    {
	    target_obj = objOpen(nsess->ObjSess, url_inf->StrVal, O_RDONLY | OBJ_O_AUTONAME, 0600, "text/html");
	    if (!target_obj)
		{
		nht_i_GenerateError(nsess);
		/*if (url_inf) stFreeInf_ne(url_inf);*/
		nht_i_ErrorExit(conn, 404, "Not Found"); /* does not return */
		}

	    /** Directory indexing? **/
	    objGetAttrValue(target_obj, "inner_type", DATA_T_STRING, POD(&ptr));
	    if (!strcmp(ptr,"system/void") && NHT.DirIndex[0] && (!find_inf || !strcmp(find_inf->StrVal,"content")))
		{
		/** Check the object type. **/
		objGetAttrValue(target_obj, "outer_type", DATA_T_STRING,POD(&ptr));

		/** no dirindex on .app files! **/
		if (strcmp(ptr,"widget/page") && strcmp(ptr,"widget/frameset") &&
			strcmp(ptr,"widget/component-decl"))
		    {
		    tmp_obj = NULL;
		    for(i=0;i<sizeof(NHT.DirIndex)/sizeof(char*);i++)
			{
			if (NHT.DirIndex[i])
			    {
			    snprintf(path,sizeof(path),"%s/%s",url_inf->StrVal,NHT.DirIndex[i]);
			    tmp_obj = objOpen(nsess->ObjSess, path, O_RDONLY | OBJ_O_AUTONAME, 0600, "text/html");
			    if (tmp_obj) break;
			    }
			}
		    if (tmp_obj)
			{
			objClose(target_obj);
			target_obj = tmp_obj;
			tmp_obj = NULL;
			}
		    }
		}

	    /** Do we need to set params as a part of the open? **/
	    if (akey_match && (!find_inf || strcmp(find_inf->StrVal, "rest") != 0))
		nht_i_CkParams(url_inf, target_obj);
	    }
	else
	    {
	    target_obj = NULL;
	    }

	/** WAIT TRIGGER mode. **/
	/*if (find_inf && !strcmp(find_inf->StrVal,"triggerwait"))
	    {
	    find_inf = stLookup_ne(url_inf,"ls__waitid");
	    if (find_inf)
	        {
		tid = strtoi(find_inf->StrVal,NULL,0);
		nht_i_WaitTrigger(nsess,tid);
		}
	    }*/

	/** Check object's modification time **/
	if (target_obj && objGetAttrValue(target_obj, "last_modification", DATA_T_DATETIME, POD(&dt)) == 0)
	    {
	    memcpy(&dtval, dt, sizeof(DateTime));
	    dt = &dtval;
	    }
	else
	    {
	    dt = NULL;
	    }

	/** Should we bother comparing if-modified-since? **/
	/** FIXME - GRB this is not working yet **/
#if 0
	if (dt && *if_modified_since)
	    {
	    /** ims is in GMT; convert it **/
	    strptime(if_modified_since, "%a, %d %b %Y %T", &systime);

	    if (objDataToDateTime(DATA_T_STRING, if_modified_since, &ims_dtval, NULL) == 0)
		{
		printf("comparing %lld to %lld\n", dtval.Value, ims_dtval.Value);
		}
	    }
#endif

	/** Get the current date/time **/
	tval = time(NULL);
	thetime = gmtime(&tval);
	strftime(tbuf, sizeof(tbuf), "%a, %d %b %Y %T", thetime);

	/** Ok, set options on the connection; header will be issued later **/
	fdSetOptions(conn->ConnFD, FD_UF_WRBUF);

	/*if (nsess->IsNewCookie)
	    {
	    fdPrintf(conn->ConnFD,"HTTP/1.0 200 OK\r\n"
		     "Date: %s GMT\r\n"
		     "Server: %s\r\n"
		     "Set-Cookie: %s; path=/\r\n", 
		     tbuf, NHT.ServerString, nsess->Cookie);
	    nsess->IsNewCookie = 0;
	    }
	else
	    {
	    fdPrintf(conn->ConnFD,"HTTP/1.0 200 OK\r\n"
		     "Date: %s GMT\r\n"
		     "Server: %s\r\n",
		     tbuf, NHT.ServerString);
	    }*/

	/** Exit now if wait trigger. **/
	/*if (tid != -1)
	    {
	    fdWrite(conn->ConnFD,"OK\r\n",4,0,0);
	    objClose(target_obj);
	    return 0;
	    }*/

	/** Add anti-clickjacking X-Frame-Options header?
	 ** see: https://developer.mozilla.org/en/The_X-FRAME-OPTIONS_response_header
	 **/
	if (target_obj && (!strcmp(ptr,"widget/page") || !strcmp(ptr,"widget/frameset") || !strcmp(ptr,"widget/component-decl")))
	    {
	    xfo_ptr = NULL;
	    if (objGetAttrValue(target_obj, "http_frame_options", DATA_T_STRING, POD(&xfo_ptr)) == 0 || NHT.XFrameOptions != NHT_XFO_T_NONE)
		{
		if ((xfo_ptr && !strcmp(xfo_ptr,"deny")) || (!xfo_ptr && NHT.XFrameOptions == NHT_XFO_T_DENY))
		    nht_i_AddResponseHeader(conn, "X-Frame-Options", "DENY", 0);
		else if ((xfo_ptr && !strcmp(xfo_ptr,"sameorigin")) || (!xfo_ptr && NHT.XFrameOptions == NHT_XFO_T_SAMEORIGIN))
		    nht_i_AddResponseHeader(conn, "X-Frame-Options", "SAMEORIGIN", 0);
		}
	    }

	/** Add content-disposition header with filename **/
	if (target_obj && objGetAttrValue(target_obj, "cx__download_as", DATA_T_STRING, POD(&name)) == 0)
	    {
	    find_inf2 = stLookup_ne(url_inf,"cx__forcedownload");
	    if (!strpbrk(name, "\r\n\t"))
		nht_i_AddResponseHeaderQPrintf(conn, "Content-Disposition", "%STR&SYM; filename=%STR&DQUOT", 
			(find_inf2 && find_inf2->StrVal && *(find_inf2->StrVal))?"attachment":"inline", name);
	    }

	/** Add last modified information if we can. **/
	if (dt)
	    {
	    systime.tm_sec = dt->Part.Second;
	    systime.tm_min = dt->Part.Minute;
	    systime.tm_hour = dt->Part.Hour;
	    systime.tm_mday = dt->Part.Day + 1;
	    systime.tm_mon = dt->Part.Month;
	    systime.tm_year = dt->Part.Year;
	    systime.tm_isdst = -1;
	    tval = mktime(&systime);
	    thetime = gmtime(&tval);
	    strftime(tbuf, sizeof(tbuf), "%a, %d %b %Y %T", thetime);
	    nht_i_AddResponseHeaderQPrintf(conn, "Last-Modified", "%STR GMT", tbuf);
	    }

	/** GET CONTENT mode. **/
	if (!find_inf || !strcmp(find_inf->StrVal, "content"))
	    {
	    /** Check the object type. **/
	    objGetAttrValue(target_obj, "outer_type", DATA_T_STRING,POD(&ptr));

	    /** request for application content of some kind **/
	    if (!strcmp(ptr,"widget/page") || !strcmp(ptr,"widget/frameset") ||
		    !strcmp(ptr,"widget/component-decl"))
	        {
		/** Width and Height of user agent specified? **/
		memset(&wgtr_params, 0, sizeof(wgtr_params));
		gptr = NULL;
		stAttrValue_ne(stLookup_ne(url_inf,"cx__geom"),&gptr);
		if (!gptr || (strlen(gptr) != 20 && strcmp(gptr,"design") != 0))
		    {
		    /** Deploy snippet to get geom from browser **/
		    nht_i_WriteResponse(conn, 200, "OK", NULL);
		    nht_i_GetGeom(target_obj, conn);
		    objClose(target_obj);
		    return 0;
		    }
		if (!strcmp(gptr,"design"))
		    {
		    client_h=0;
		    client_w=0;
		    objGetAttrValue(target_obj, "width", DATA_T_INTEGER, POD(&client_w));
		    objGetAttrValue(target_obj, "height", DATA_T_INTEGER, POD(&client_h));

		    /** These are some default values **/
		    wgtr_params.CharWidth = 7;
		    wgtr_params.CharHeight = 16;
		    wgtr_params.ParagraphHeight = 16;
		    }
		else
		    {
		    client_h = client_w = 0;
		    /*client_w = strtoi(gptr,&gptr,10);
		    if (client_w < 0) client_w = 0;
		    if (client_w > 10000) client_w = 10000;
		    if (*gptr == 'x')
			{
			client_h = strtoi(gptr+1,NULL,10);
			if (client_h < 0) client_h = 0;
			if (client_h > 10000) client_h = 10000;
			}*/
		    client_w = nht_i_Hex16ToInt(gptr);
		    client_h = nht_i_Hex16ToInt(gptr+4);
		    wgtr_params.CharWidth = nht_i_Hex16ToInt(gptr+8);
		    wgtr_params.CharHeight = nht_i_Hex16ToInt(gptr+12);
		    wgtr_params.ParagraphHeight = nht_i_Hex16ToInt(gptr+16);
		    }
		wgtr_params.MaxHeight = client_h;
		wgtr_params.MinHeight = client_h;
		wgtr_params.MaxWidth = client_w;
		wgtr_params.MinWidth = client_w;
		wgtr_params.AppMaxHeight = client_h;
		wgtr_params.AppMinHeight = client_h;
		wgtr_params.AppMaxWidth = client_w;
		wgtr_params.AppMinWidth = client_w;

		/** Check for gzip encoding **/
		gzip=0;
#ifdef HAVE_LIBZ
		if(NHT.EnableGzip && acceptencoding && strstr(acceptencoding,"gzip"))
		    gzip=1; /* enable gzip for this request */
#endif
		if(gzip==1)
		    {
		    nht_i_AddResponseHeader(conn, "Content-Encoding", "gzip", 0);
		    }

		/** Send the response **/
		conn->NoCache = 1;
		nht_i_WriteResponse(conn, 200, "OK", NULL);

		if(gzip==1)
		    fdSetOptions(conn->ConnFD, FD_UF_GZIP);

		/** Check for template specs **/
		tptr = NULL;
		if (akey_match && stAttrValue_ne(stLookup_ne(url_inf,"cx__templates"),&tptr) == 0 && tptr)
		    {
		    cnt = 0;
		    tptr = nmSysStrdup(tptr);
		    tptr2 = strtok(tptr,"|");
		    while(tptr2)
			{
			if (cnt < WGTR_MAX_TEMPLATE)
			    {
			    wgtr_params.Templates[cnt] = tptr2;
			    cnt++;
			    }
			else
			    break;
			tptr2 = strtok(NULL, "|");
			}
		    }

		/** Check for application layering **/
		lptr = NULL;
		if (akey_match && stAttrValue_ne(stLookup_ne(url_inf,"cx__overlays"),&lptr) == 0 && lptr)
		    {
		    cnt = 0;
		    lptr = nmSysStrdup(lptr);
		    lptr2 = strtok(lptr,"|");
		    while(lptr2)
			{
			if (cnt < WGTR_MAX_OVERLAY)
			    {
			    wgtr_params.Overlays[cnt] = lptr2;
			    cnt++;
			    }
			else
			    break;
			lptr2 = strtok(NULL, "|");
			}
		    }
		pptr = NULL;
		if (akey_match && stAttrValue_ne(stLookup_ne(url_inf,"cx__app_path"),&pptr) == 0 && pptr)
		    {
		    wgtr_params.AppPath = pptr;
		    }

		/** Set the base directory pathname for the app -- for relative path references **/
		wgtr_params.BaseDir = nmSysStrdup(objGetPathname(target_obj));
		slashptr = strrchr(wgtr_params.BaseDir, '/');
		if (slashptr && slashptr != wgtr_params.BaseDir)
		    {
		    *slashptr = '\0';
		    }

		/** Create a new group and app if necessary **/
		if (!strcmp(ptr, "widget/page"))
		    {
		    if (!group)
			group = nht_i_AllocAppGroup(url_inf->StrVal, nsess);
		    if (group && !app)
			app = nht_i_AllocApp(url_inf->StrVal, group);
		    }

		/** Build the akey - CSRF prevention **/
		if (group && app)
		    snprintf(wgtr_params.AKey, sizeof(wgtr_params.AKey), "%s-%s-%s", nsess->SKey, group->GKey, app->AKey);
		else
		    strtcpy(wgtr_params.AKey, nsess->SKey, sizeof(wgtr_params.AKey));

		/** Read the app spec, verify it, and generate it to DHTML **/
		//pWgtrNode tree;
		//if ((tree = wgtrParseOpenObject(target_obj, url_inf, wgtr_params.Templates)) < 0
		    //|| (objpath = objGetPathname(obj)) && wgtrMergeOverlays(tree, objpath, client_info->AppPath, client_info->Overlays, client_info->Templates) < 0 )
		    //|| nht_i_Parse_and_Cache_an_App(target_obj, url_inf, &wgtr_params, nsess, &tree) < 0
		    //|| nht_i_Verify_and_Position_and_Render_an_App(conn->ConnFD, nsess, &wgtr_params, "DHTML", tree) < 0)
		nht_i_WriteConn(conn, "", 0, 0);
		if (nhtRenderApp(conn, target_obj->Session, target_obj, url_inf, &wgtr_params, "DHTML", nsess) < 0)
		    {
		    mssError(0, "HTTP", "Unable to render application %s of type %s", url_inf->StrVal, ptr);
		    err_xs = xsNew();
		    if (err_xs)
			{
			mssStringError(err_xs);
			nht_i_QPrintfConn(conn, 0, "<h1>An error occurred while constructing the application:</h1><pre>%STR&HTE\r\n</pre>", xsString(err_xs));
			xsFree(err_xs);
			}
		    objClose(target_obj);
		    if (tptr) nmSysFree(tptr);
		    if (lptr) nmSysFree(lptr);
		    return -1;
		    }

		/** copy security endorsement data to app info structure **/
		if (app)
		    {
		    for(i=0;i<app->Endorsements.nItems;i++)
			{
			nmSysFree(app->Endorsements.Items[i]);
			nmSysFree(app->Contexts.Items[i]);
			}
		    xaClear(&app->Endorsements, NULL, NULL);
		    xaClear(&app->Contexts, NULL, NULL);
		    cxssGetEndorsementList(&app->Endorsements, &app->Contexts);
		    }

		if (tptr) nmSysFree(tptr);
		if (lptr) nmSysFree(lptr);
	        }

	    /** a client app requested an interface definition **/ 
	    else if (!strcmp(ptr, "iface/definition"))
		{
		/** get the type **/
		objGetAttrValue(target_obj, "type", DATA_T_STRING, POD(&ptr));

		/** end the headers **/
		nht_i_WriteResponse(conn, 200, "OK", NULL);

		/** call the html-related interface translating function **/
		if (ifcToHtml(conn->ConnFD, nsess->ObjSess, url_inf->StrVal) < 0)
		    {
		    mssError(0, "NHT", "Error sending Interface info for '%s' to client", url_inf->StrVal);
		    nht_i_QPrintfConn(conn, 0, "<A TARGET=\"ERR\" HREF=\"%STR&HTE\"></A>", url_inf->StrVal);
		    }
		else
		    {
		    nht_i_QPrintfConn(conn, 0, "<A NAME=\"%s\" TARGET=\"OK\" HREF=\"%STR&HTE\"></A>", ptr, url_inf->StrVal);
		    }
		}
	    /** some other sort of request **/
	    else
	        {
		int gzip=0;
		char *browser;

		browser=(char*)mssGetParam("User-Agent");

		objGetAttrValue(target_obj,"inner_type", DATA_T_STRING,POD(&ptr));
		if (!strcmp(ptr,"text/plain")) 
		    {
		    ptr = "text/html";
		    convert_text = 1;
		    }

#ifdef HAVE_LIBZ
		if(	NHT.EnableGzip && /* global enable flag */
			objIsA(ptr,"text/plain")>0 /* a subtype of text/plain */
			&& acceptencoding && strstr(acceptencoding,"gzip") /* browser wants it gzipped */
			&& (!strcmp(ptr,"text/html") || (browser && regexec(NHT.reNet47,browser,(size_t)0,NULL,0) != 0 ) )
			/* only gzip text/html for Netscape 4.7, which doesn't like it if we gzip .js files */
		  )
		    {
		    gzip=1; /* enable gzip for this request */
		    }
#endif
		if(gzip==1)
		    {
		    nht_i_AddResponseHeader(conn, "Content-Encoding", "gzip", 0);
		    }
		strtcpy(conn->ResponseContentType, ptr, sizeof(conn->ResponseContentType));
		nht_i_WriteResponse(conn, 200, "OK", NULL);
		if(gzip==1)
		    {
		    fdSetOptions(conn->ConnFD, FD_UF_GZIP);
		    }
		bufptr = (char*)nmMalloc(4096);
		if (bufptr)
		    {
		    if (convert_text) nht_i_WriteConn(conn, "<HTML><PRE>",11,0);
		    while((cnt=objRead(target_obj,bufptr,4096,0,0)) > 0)
			{
			nht_i_WriteConn(conn,bufptr,cnt,0);
			}
		    if (convert_text) nht_i_WriteConn(conn,"</HTML></PRE>",13,0);
		    if (cnt < 0) 
			{
			mssError(0,"NHT","Incomplete read of object's content");
			nht_i_GenerateError(nsess);
			}
		    nmFree(bufptr, 4096);
		    }
	        }
	    }

	/** REST mode? **/
	else if (!strcmp(find_inf->StrVal,"rest"))
	    {
	    conn->StrictSameSite = 0;
	    rval = nht_i_RestGet(conn, url_inf, target_obj);
	    }

	/** Retrieve a new session/group/app key? **/
	else if (!strcmp(find_inf->StrVal, "appinit"))
	    {
	    find_inf = stLookup_ne(url_inf, "cx__groupname");
	    if (!find_inf || find_inf->StrVal[0] == '/')
		kname = "API Client";
	    else
		kname = find_inf->StrVal;

	    /** Try to join an existing app group, if specified **/
	    if (!group)
		group = nht_i_AllocAppGroup(kname, nsess);
	    if (group)
		{
		find_inf = stLookup_ne(url_inf, "cx__appname");
		if (!find_inf || find_inf->StrVal[0] == '/')
		    kname = "API Client";
		else
		    kname = find_inf->StrVal;

		/** Set up a new active app **/
		app = nht_i_AllocApp(kname, group);
		if (app)
		    {
		    /** We want to give the api client information about our
		     ** watchdog timeout (so the client can know how often to
		     ** ping us), as well as information about our current time
		     ** and timezone (for proper date/time data display
		     ** synchronization).
		     **/
		    tval = time(NULL);
		    thetime = localtime(&tval);
		    tbuf[0] = '\0';
		    tbuf2[0] = '\0';
		    if (thetime)
			{
			if (strftime(tbuf, sizeof(tbuf), "%Y %m %d %T %Z", thetime) <= 0)
			    tbuf[0] = '\0';
			if (strftime(tbuf2, sizeof(tbuf2), "%Y %m %d %T", thetime) <= 0)
			    tbuf2[0] = '\0';
			}
		    conn->NoCache = 1;
		    strtcpy(conn->ResponseContentType, "application/json", sizeof(conn->ResponseContentType));
		    nht_i_WriteResponse(conn, 200, "OK", NULL);
		    nht_i_QPrintfConn(conn, 0,
			    "{\"akey\":\"%STR&JSONSTR-%STR&JSONSTR-%STR&JSONSTR\", \"watchdogtimer\":%INT, \"servertime\":\"%STR&JSONSTR\", \"servertime_notz\":\"%STR&JSONSTR\"}\r\n",
			    nsess->SKey, group->GKey, app->AKey, NHT.WatchdogTime, tbuf, tbuf2
			    );
		    }
		}
	    }

	/** GET DIRECTORY LISTING mode. **/
	else if (!strcmp(find_inf->StrVal,"list"))
	    {
	    if (stAttrValue_ne(stLookup_ne(url_inf,"ls__info"),&ptr) >= 0 && !strcmp(ptr,"1"))
		send_info = 1;
	    if (stAttrValue_ne(stLookup_ne(url_inf,"ls__orderdesc"),&ptr) >= 0 && !strcmp(ptr,"1"))
		order_desc = 1;
	    if (order_desc)
		query = objOpenQuery(target_obj,"",":name desc",NULL,NULL,0);
	    else
		query = objOpenQuery(target_obj,"",NULL,NULL,NULL,0);
	    if (query)
	        {
		nht_i_WriteResponse(conn, 200, "OK", NULL);
		nht_i_QPrintfConn(conn, 0, "<HTML><HEAD><META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\"></HEAD><BODY><TT><A HREF=\"%STR&HTE/..\">..</A><BR>\n",url_inf->StrVal);
		dptr = url_inf->StrVal;
		while(*dptr && *dptr == '/' && dptr[1] == '/') dptr++;
		while((sub_obj = objQueryFetch(query,O_RDONLY)))
		    {
		    if (send_info)
			{
			objinfo = objInfo(sub_obj);
			}
		    objGetAttrValue(sub_obj, "name", DATA_T_STRING,POD(&ptr));
		    objGetAttrValue(sub_obj, "annotation", DATA_T_STRING,POD(&aptr));
		    if (send_info && objinfo)
			{
			nht_i_QPrintfConn(conn, 0, "<A HREF=\"%STR&HTE%[/%]%STR&HTE\" TARGET='%STR&HTE'>%INT:%INT:%STR&HTE</A><BR>\n",dptr,
			    (dptr[0]!='/' || dptr[1]!='\0'),ptr,ptr,objinfo->Flags,objinfo->nSubobjects,aptr);
			}
		    else if (send_info && !objinfo)
			{
			nht_i_QPrintfConn(conn, 0, "<A HREF=\"%STR&HTE%[/%]%STR&HTE\" TARGET='%STR&HTE'>0:0:%STR&HTE</A><BR>\n",dptr,
			    (dptr[0]!='/' || dptr[1]!='\0'),ptr,ptr,aptr);
			}
		    else
			{
			nht_i_QPrintfConn(conn, 0, "<A HREF=\"%STR&HTE%[/%]%STR&HTE\" TARGET='%STR&HTE'>%STR&HTE</A><BR>\n",dptr,
			    (dptr[0]!='/' || dptr[1]!='\0'),ptr,ptr,aptr);
			}
		    objClose(sub_obj);
		    }
		objQueryClose(query);
		}
	    else
	        {
		nht_i_GenerateError(nsess);
		}
	    }

	/** SQL QUERY mode **/
	else if (!strcmp(find_inf->StrVal,"query") && akey_match)
	    {
	    /** Change directory to appropriate query root **/
	    conn->NoCache = 1;
	    nht_i_WriteResponse(conn, 200, "OK", NULL);
	    strtcpy(path, objGetWD(nsess->ObjSess), sizeof(path));
	    objSetWD(nsess->ObjSess, target_obj);

	    /** row limit? **/
	    rowlimit = 0;
	    if (stAttrValue_ne(stLookup_ne(url_inf,"ls__rowcount"),&ptr) >= 0)
		rowlimit = strtoi(ptr, NULL, 10);

	    /** Get the SQL **/
	    if (stAttrValue_ne(stLookup_ne(url_inf,"ls__sql"),&ptr) >= 0)
	        {
		query = objMultiQuery(nsess->ObjSess, ptr, NULL, 0);
		if (query)
		    {
		    rowid = 0;
		    while((sub_obj = objQueryFetch(query,O_RDONLY)))
		        {
			nht_i_WriteAttrs(sub_obj,conn,(handle_t)rowid,1);
			objClose(sub_obj);
			rowid++;
			if (rowid == rowlimit) break;
			}
		    objQueryClose(query);
		    }
		}

	    /** Switch the current directory back to what it used to be. **/
	    tmp_obj = objOpen(nsess->ObjSess, path, O_RDONLY, 0600, "text/html");
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
	else if (!strcmp(find_inf->StrVal,"osml") && akey_match)
	    {
	    /** Load in security endorsements **/
	    for(i=0;i<app->Endorsements.nItems;i++)
		{
		cxssAddEndorsement(app->Endorsements.Items[i], app->Contexts.Items[i]);
		}

	    find_inf = stLookup_ne(url_inf,"ls__req");
	    nht_i_OSML(conn,target_obj, find_inf->StrVal, url_inf, app);
	    }

	/** Exec method mode **/
	else if (!strcmp(find_inf->StrVal,"execmethod") && akey_match)
	    {
	    find_inf = stLookup_ne(url_inf,"ls__methodname");
	    find_inf2 = stLookup_ne(url_inf,"ls__methodparam");
	    conn->NoCache = 1;
	    nht_i_WriteResponse(conn, 200, "OK", NULL);
	    if (!find_inf || !find_inf2)
	        {
		mssError(1,"NHT","Invalid call to execmethod - requires name and param");
		nht_i_GenerateError(nsess);
		}
	    else
	        {
	    	ptr = find_inf2->StrVal;
	    	objExecuteMethod(target_obj, find_inf->StrVal, POD(&ptr));
		nht_i_WriteConn(conn, "OK", 2, 0);
		}
	    }

	/** Close the objectsystem entry. **/
	if (target_obj) objClose(target_obj);

    return 0;
    }


/*** nht_i_DELETE - implements the HTTP DELETE method, which after
 *** verifying credentials, deletes the referenced path.
 ***/
int
nht_i_DELETE(pNhtConn conn, pStruct url_inf)
    {
    char* msg;
    int code;
    pStruct find_inf;
    pObject target_obj = NULL;
    pNhtApp app = NULL;
    pNhtAppGroup group = NULL;
    int rval;

	/** Check akey **/
	find_inf = stLookup_ne(url_inf,"cx__akey");
	if (!find_inf || nht_i_VerifyAKey(find_inf->StrVal, conn->NhtSession, &group, &app) != 0 || !group || !app)
	    {
	    mssError(1,"NHT","DELETE request requires akey for validation");
	    msg = "Unauthorized";
	    code = 401;
	    goto error;
	    }

	/** Open the target object **/
	target_obj = objOpen(conn->NhtSession->ObjSess, url_inf->StrVal, OBJ_O_RDWR, 0600, "application/octet-stream");
	if (!target_obj)
	    {
	    mssError(0,"NHT","Could not open requested object for DELETE request");
	    msg = "Not Found";
	    code = 404;
	    goto error;
	    }

	/** Determine mode **/
	find_inf = stLookup_ne(url_inf,"cx__mode");
	if (!find_inf || strcmp(find_inf->StrVal, "rest") != 0)
	    {
	    mssError(1,"NHT","DELETE: only REST mode supported");
	    msg = "Bad Request";
	    code = 400;
	    goto error;
	    }

	/** Call out to REST functionality **/
	conn->StrictSameSite = 0;
	rval = nht_i_RestDelete(conn, url_inf, target_obj);
	if (rval < 0)
	    {
	    msg = "Bad Request";
	    code = 400;
	    goto error;
	    }
	target_obj = NULL; /* deleted, handle no longer valid */

	nht_i_WriteResponse(conn, 204, "No Content", NULL);

	/** Cleanup and return **/
	return 0;

    error:
	nht_i_WriteErrResponse(conn, code, msg, NULL);

	if (target_obj)
	    objClose(target_obj);

	return -1;
    }


/*** nht_i_PATCH - implements the HTTP PATCH method, which is used for
 *** modifying existing data without replacing the entire resource (which is
 *** what PUT would do).
 ***/
int
nht_i_PATCH(pNhtConn conn, pStruct url_inf, char* content)
    {
    pStruct find_inf;
    struct json_tokener* jtok = NULL;
    enum json_tokener_error jerr;
    char rbuf[256];
    int rcnt, rval, total_rcnt;
    struct json_object*	j_obj = NULL;
    char* msg;
    int code;
    pObject target_obj = NULL;
    pNhtApp app = NULL;
    pNhtAppGroup group = NULL;

	/** Check akey **/
	find_inf = stLookup_ne(url_inf,"cx__akey");
	if (!find_inf || nht_i_VerifyAKey(find_inf->StrVal, conn->NhtSession, &group, &app) != 0 || !group || !app)
	    {
	    mssError(1,"NHT","PATCH request requires akey for validation");
	    msg = "Unauthorized";
	    code = 401;
	    goto error;
	    }

	/** Open the target object **/
	target_obj = objOpen(conn->NhtSession->ObjSess, url_inf->StrVal, OBJ_O_RDWR, 0600, "application/octet-stream");
	if (!target_obj)
	    {
	    mssError(0,"NHT","Could not open requested object for PATCH request");
	    msg = "Not Found";
	    code = 404;
	    goto error;
	    }

	/** Initialize the JSON tokenizer **/
	jtok = json_tokener_new();
	if (!jtok)
	    {
	    mssError(1,"NHT","Could not initialize JSON parser");
	    msg = "Internal Server Error";
	    code = 500;
	    goto error;
	    }

	/** Content supplied on URL line, or do we need to read it? **/
	if (content)
	    {
	    /** Supplied on command line **/
	    j_obj = json_tokener_parse_ex(jtok, content, strlen(content));
	    jerr = json_tokener_get_error(jtok);
	    }
	else
	    {
	    /** Read it in via the network connection **/
	    total_rcnt = 0;
	    do  {
		rcnt = fdRead(conn->ConnFD, rbuf, sizeof(rbuf), 0, 0);
		if (rcnt <= 0)
		    {
		    mssError(1,"NHT","Could not read JSON object from HTTP connection");
		    msg = "Bad Request";
		    code = 400;
		    goto error;
		    }
		total_rcnt += rcnt;
		if (total_rcnt > NHT_PAYLOAD_MAX)
		    {
		    mssError(1,"NHT","JSON object too large");
		    msg = "Bad Request";
		    code = 400;
		    goto error;
		    }
		j_obj = json_tokener_parse_ex(jtok, rbuf, rcnt);
		} while((jerr = json_tokener_get_error(jtok)) == json_tokener_continue);
	    }

	/** Success? **/
	if (!j_obj || jerr != json_tokener_success)
	    {
	    mssError(1,"NHT","Invalid JSON object in PATCH request");
	    msg = "Bad Request";
	    code = 400;
	    goto error;
	    }
	json_tokener_free(jtok);
	jtok = NULL;

	/** Determine mode **/
	find_inf = stLookup_ne(url_inf,"cx__mode");
	if (!find_inf || strcmp(find_inf->StrVal, "rest") != 0)
	    {
	    mssError(1,"NHT","PATCH: only REST mode supported");
	    msg = "Bad Request";
	    code = 400;
	    goto error;
	    }

	/** Call out to REST functionality **/
	conn->StrictSameSite = 0;
	rval = nht_i_RestPatch(conn, url_inf, target_obj, j_obj);
	if (rval < 0)
	    {
	    msg = "Bad Request";
	    code = 400;
	    goto error;
	    }

	/** Cleanup and return **/
	json_object_put(j_obj);
	objClose(target_obj);
	return 0;

    error:
	nht_i_WriteErrResponse(conn, code, msg, NULL);

	if (jtok)
	    json_tokener_free(jtok);
	if (j_obj)
	    json_object_put(j_obj);
	if (target_obj)
	    objClose(target_obj);

	return -1;
    }


/*** nht_i_PUT - implements the PUT HTTP method.  Set content_buf to
 *** data to write, otherwise it will be read from the connection if content_buf
 *** is NULL.
 ***/
int
nht_i_PUT(pNhtConn conn, pStruct url_inf, int size, char* content_buf)
    {
    pNhtSessionData nsess = conn->NhtSession;
    pObject target_obj;
    char sbuf[160];
    int rcnt;
    int type,i,v;
    pStruct sub_inf;
    pStruct find_inf;
    pNhtApp app = NULL;
    pNhtAppGroup group = NULL;
    int already_exist=0;

	/** Validate akey and make sure app and group id's match as well.  AKey
	 ** must be supplied with all PUT requests.
	 **/
	find_inf = stLookup_ne(url_inf,"cx__akey");
	if (!find_inf || nht_i_VerifyAKey(find_inf->StrVal, nsess, &group, &app) != 0 || !group || !app)
	    {
	    nht_i_WriteErrResponse(conn, 403, "Forbidden", NULL);
	    return -1;
	    }

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
	    if (url_inf) stFreeInf_ne(url_inf);
	    nht_i_ErrorExit(conn, 404, "Not Found"); /* does not return */
	    }

	/** OK, we're ready.  Send the 100 Continue message. **/
	/*sprintf(sbuf,"HTTP/1.1 100 Continue\r\n"
		     "Server: %s\r\n"
		     "\r\n",NHT.ServerString);
	fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);*/

	/** If size specified, set the size. **/
	if (size >= 0) objSetAttrValue(target_obj, "size", DATA_T_INTEGER,POD(&size));

	/** Set any attributes specified in the url inf **/
	for(i=0;i<url_inf->nSubInf;i++)
	    {
	    sub_inf = url_inf->SubInf[i];
	    type = objGetAttrType(target_obj, sub_inf->Name);
	    if (type == DATA_T_INTEGER)
	        {
		v = strtoi(sub_inf->StrVal,NULL,10);
		objSetAttrValue(target_obj, sub_inf->Name, DATA_T_INTEGER,POD(&v));
		}
	    else if (type == DATA_T_STRING)
	        {
		objSetAttrValue(target_obj, sub_inf->Name, DATA_T_STRING, POD(&(sub_inf->StrVal)));
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
	    while(size != 0 && (rcnt=fdRead(conn->ConnFD,sbuf,160,0,0)) > 0)
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
	if (already_exist)
	    nht_i_WriteResponse(conn, 200, "OK", NULL);
	else
	    nht_i_WriteResponse(conn, 201, "Created", NULL);
	nht_i_QPrintfConn(conn, 0, "%STR\r\n", url_inf->StrVal);

    return 0;
    }


/*** nht_i_COPY - implements the COPY centrallix-http method.
 ***/
int
nht_i_COPY(pNhtConn conn, pStruct url_inf, char* dest)
    {
    pNhtSessionData nsess = conn->NhtSession;
    pObject source_obj = NULL;
    pObject target_obj = NULL;
    int size;
    int already_exists = 0;
    char sbuf[256];
    int rcnt,wcnt;
    pStruct copymode_inf;
    char* copymode = NULL;
    char* copytype = NULL;

	/** What copy mode are we using? **/
	copymode_inf = stLookup_ne(url_inf, "cx__copymode");
	if (copymode_inf)
	    stAttrValue_ne(copymode_inf, &copymode);

	/** copy mode is srctype, dsttype, or an explicitly provided type **/
	if (copymode && strcmp(copymode, "srctype") != 0 && strcmp(copymode, "dsttype") != 0)
	    copytype = copymode;

	/** If copymode is not dsttype, open the source object here. **/
	if (!copymode || strcmp(copymode, "dsttype") != 0)
	    {
	    source_obj = objOpen(nsess->ObjSess, url_inf->StrVal, O_RDONLY, 0600, copytype?copytype:"application/octet-stream");
	    if (!source_obj)
		{
		nht_i_ErrorExit(conn, 404, "Source Not Found");
		}

	    /** If using srctype copy mode, get the type now **/
	    if (!copytype)
		objGetAttrValue(source_obj, "inner_type", DATA_T_STRING, POD(&copytype));
	    }

	/** Do we need to set params as a part of the open? **/
	nht_i_CkParams(url_inf, source_obj);

	/** Try to open the new object without O_CREAT to see if it exists... **/
	target_obj = objOpen(nsess->ObjSess, dest, O_WRONLY | O_TRUNC, 0600, "text/html");
	if (target_obj)
	    {
	    already_exists = 1;
	    }
	else
	    {
	    already_exists = 0;
	    target_obj = objOpen(nsess->ObjSess, dest, O_WRONLY | O_TRUNC | O_CREAT, 0600, "text/html");
	    if (!target_obj)
		{
		nht_i_ErrorExit(conn, 404, "Target Not Found");
		}
	    }

	/** If using dsttype, get the type from the target object **/
	if (!copytype)
	    objGetAttrValue(target_obj, "inner_type", DATA_T_STRING, POD(&copytype));

	/** If using dsttype, now we open the source **/
	if (!source_obj)
	    {
	    source_obj = objOpen(nsess->ObjSess, url_inf->StrVal, O_RDONLY, 0600, copytype?copytype:"application/octet-stream");
	    if (!source_obj)
		{
		nht_i_ErrorExit(conn, 404, "Source Not Found");
		}
	    }

	/** Get the size of the original object, if possible **/
	if (objGetAttrValue(source_obj,"size",DATA_T_INTEGER,POD(&size)) != 0) size = -1;

	/** Set the size of the new document... **/
	if (size >= 0) objSetAttrValue(target_obj, "size", DATA_T_INTEGER,POD(&size));

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
	nht_i_WriteResponse(conn, already_exists?200:201, already_exists?"OK":"Created", NULL);
	nht_i_QPrintfConn(conn, 0, "%STR\r\n", dest);

    return 0;
    }


/*** nht_i_ParseHeaders - read from the connection and parse the
 *** headers into the NhtConn structure
 ***/
int
nht_i_ParseHeaders(pNhtConn conn)
    {
    char* msg;
    pLxSession s = NULL;
    int toktype;
    char hdr[64];
    int did_alloc = 1;
    char* ptr;

    	/** Initialize a lexical analyzer session... **/
	s = mlxOpenSession(conn->ConnFD, MLX_F_NODISCARD | MLX_F_DASHKW | MLX_F_ICASE |
		MLX_F_EOL | MLX_F_EOF);

	/** Read in the main request header.  Note - error handler is at function
	 ** tail, as in standard goto-based error handling.
	 **/
	toktype = mlxNextToken(s);
	if (toktype == MLX_TOK_EOF || toktype == MLX_TOK_EOL)
	    {
	    /** Browsers like to open connections and then close them without
	     ** sending a request; don't print errors on this condition.
	     **/
	    mlxCloseSession(s);
	    nht_i_FreeConn(conn);
	    thExit();
	    }

	/** Expecting request method **/
	if (toktype != MLX_TOK_KEYWORD) { msg="Invalid method syntax"; goto error; }
	mlxCopyToken(s,conn->Method,sizeof(conn->Method));
	mlxSetOptions(s,MLX_F_IFSONLY);

	/** Expecting request URL and version **/
	if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Invalid url syntax"; goto error; }
	did_alloc = 1;
	conn->URL = mlxStringVal(s, &did_alloc);
	if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected HTTP version after url"; goto error; }
	mlxCopyToken(s,conn->HTTPVer,sizeof(conn->HTTPVer));
	if (mlxNextToken(s) != MLX_TOK_EOL) { msg="Expected EOL after version"; goto error; }
	mlxUnsetOptions(s,MLX_F_IFSONLY);

	/** Read in the various header parameters. **/
	while((toktype = mlxNextToken(s)) != MLX_TOK_EOL)
	    {
	    if (toktype == MLX_TOK_EOF) break;
	    if (toktype != MLX_TOK_KEYWORD) { msg="Expected HTTP header item"; goto error; }
	    /*ptr = mlxStringVal(s,NULL);*/
	    mlxCopyToken(s,hdr,sizeof(hdr));
	    if (mlxNextToken(s) != MLX_TOK_COLON) { msg="Expected : after HTTP header"; goto error; }

	    /** Got a header item.  Pick an appropriate type. **/
	    if (!strcmp(hdr,"destination"))
	        {
		/** Copy next IFS-only token to destination value **/
		mlxSetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected filename after dest."; goto error; }
		mlxCopyToken(s,conn->Destination,sizeof(conn->Destination));
		mlxUnsetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_EOF) { msg="Expected EOL after filename"; goto error; }
		}
	    else if (!strcmp(hdr,"authorization"))
	        {
		/** Get 'Basic' then the auth string in base64 **/
		mlxSetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected auth type"; goto error; }
		ptr = mlxStringVal(s,NULL);
		if (strcasecmp(ptr,"basic")) { msg="Can only handle BASIC auth"; goto error; }
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected auth after Basic"; goto error; }
		qpfPrintf(NULL,conn->Auth,sizeof(conn->Auth),"%STR&DB64",mlxStringVal(s,NULL));
		mlxUnsetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_EOL) { msg="Expected EOL after auth"; goto error; }
		}
	    else if (!strcmp(hdr,"cookie"))
	        {
		/** Copy whole thing. **/
		conn->AllCookies[0] = '\0';
		mlxSetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected str after Cookie:"; goto error; }
		mlxCopyToken(s,conn->Cookie,sizeof(conn->Cookie));
		strtcpy(conn->AllCookies + strlen(conn->AllCookies), conn->Cookie, sizeof(conn->AllCookies) - strlen(conn->AllCookies));
		while((toktype = mlxNextToken(s)))
		    {
		    if (toktype == MLX_TOK_EOL || toktype == MLX_TOK_ERROR) break;
		    /** if the token is a string, and the current cookie doesn't look like ours, try the next one **/
		    if (!conn->UsingTLS && toktype == MLX_TOK_STRING && (strncmp(conn->Cookie,NHT.SessionCookie,strlen(NHT.SessionCookie)) || conn->Cookie[strlen(NHT.SessionCookie)] != '='))
			{
			mlxCopyToken(s,conn->Cookie,sizeof(conn->Cookie));
			strtcpy(conn->AllCookies + strlen(conn->AllCookies), conn->Cookie, sizeof(conn->AllCookies) - strlen(conn->AllCookies));
			}
		    else if (conn->UsingTLS && toktype == MLX_TOK_STRING && (strncmp(conn->Cookie,NHT.TlsSessionCookie,strlen(NHT.TlsSessionCookie)) || conn->Cookie[strlen(NHT.TlsSessionCookie)] != '='))
			{
			mlxCopyToken(s,conn->Cookie,sizeof(conn->Cookie));
			strtcpy(conn->AllCookies + strlen(conn->AllCookies), conn->Cookie, sizeof(conn->AllCookies) - strlen(conn->AllCookies));
			}
		    else
			{
			mlxCopyToken(s, conn->AllCookies + strlen(conn->AllCookies), sizeof(conn->AllCookies) - strlen(conn->AllCookies));
			}
		    }
		mlxUnsetOptions(s,MLX_F_IFSONLY);
		}
	    else if (!strcmp(hdr,"content-length"))
	        {
		/** Get the integer. **/
		if (mlxNextToken(s) != MLX_TOK_INTEGER) { msg="Expected content-length"; goto error; }
		conn->Size = mlxIntVal(s);
		if (mlxNextToken(s) != MLX_TOK_EOL) { msg="Expected EOL after length"; goto error; }
		}
	    else if (!strcmp(hdr,"referer"))
		{
		mlxSetOptions(s, MLX_F_LINEONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected str after Referer:"; goto error; }
		did_alloc = 1;
		conn->Referrer = mlxStringVal(s, &did_alloc);
		if (conn->Referrer[0] == ' ')
		    memmove(conn->Referrer, conn->Referrer+1, strlen(conn->Referrer));
		if (strpbrk(conn->Referrer, "\r\n"))
		    *(strpbrk(conn->Referrer, "\r\n")) = '\0';
		mlxUnsetOptions(s,MLX_F_LINEONLY);
		if (mlxNextToken(s) != MLX_TOK_EOL) { msg="Expected EOL after Referer: header"; goto error; }
		}
	    else if (!strcmp(hdr,"user-agent"))
	        {
		/** Copy whole User-Agent. **/
		mlxSetOptions(s, MLX_F_LINEONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected str after User-Agent:"; goto error; }
		/** NOTE: This needs to be freed up at the end of the session.  Is that taken
		          care of by mssEndSession?  I don't think it is, since xhClear is passed
			  a NULL function for free_fn.  This will be a 160 byte memory leak for
			  each session otherwise. 
		    January 6, 2002   NRE
		 **/
		mlxCopyToken(s,conn->UserAgent,sizeof(conn->UserAgent));
		if (conn->UserAgent[0] == ' ')
		    memmove(conn->UserAgent, conn->UserAgent+1, strlen(conn->UserAgent));
		if (strpbrk(conn->UserAgent, "\r\n"))
		    *(strpbrk(conn->UserAgent, "\r\n")) = '\0';
		/*while((toktype=mlxNextToken(s)))
		    {
		    if(toktype == MLX_TOK_STRING && strlen(conn->UserAgent)<158)
			{
			strcat(useragent," ");
			mlxCopyToken(s,useragent+strlen(useragent),160-strlen(useragent));
			}
		    if (toktype == MLX_TOK_EOL || toktype == MLX_TOK_ERROR) break;
		    }
		mlxUnsetOptions(s,MLX_F_IFSONLY);*/
		mlxUnsetOptions(s,MLX_F_LINEONLY);
		if (mlxNextToken(s) != MLX_TOK_EOL) { msg="Expected EOL after User-Agent: header"; goto error; }
		}
	    else if (!strcmp(hdr,"content-type"))
		{
		mlxSetOptions(s, MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected type after Content-Type:"; goto error; }
		mlxCopyToken(s, conn->RequestContentType, sizeof(conn->RequestContentType));
		if ((ptr = strchr(conn->RequestContentType, ';')) != NULL) *ptr = '\0';
		while(1)
		    {
		    toktype = mlxNextToken(s);
		    if (toktype == MLX_TOK_EOL || toktype == MLX_TOK_EOF || toktype == MLX_TOK_ERROR) break;
		    if (toktype == MLX_TOK_STRING && !conn->RequestBoundary[0])
			{
			mlxCopyToken(s, conn->RequestBoundary, sizeof(conn->RequestBoundary));
			if (!strncmp(conn->RequestBoundary, "boundary=", 9))
			    {
			    ptr = conn->RequestBoundary + 9;
			    if (*ptr == '"') ptr++;
			    memmove(conn->RequestBoundary, ptr, strlen(ptr)+1);
			    if ((ptr = strrchr(conn->RequestBoundary, '"')) != NULL) *ptr = '\0';
			    }
			}
		    }
		mlxUnsetOptions(s, MLX_F_IFSONLY);
		}
	    else if (!strcmp(hdr,"accept-encoding"))
	        {
		mlxSetOptions(s,MLX_F_LINEONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected str after Accept-encoding:"; goto error; }
		mlxCopyToken(s, conn->AcceptEncoding, sizeof(conn->AcceptEncoding));
		/*conn->AcceptEncoding = mlxStringVal(s, &did_alloc);
		acceptencoding = (char*)nmMalloc(160);
		mlxCopyToken(s,acceptencoding,160);
		while((toktype=mlxNextToken(s)))
		    {
		    if(toktype == MLX_TOK_STRING && strlen(acceptencoding)<158)
			{
			strcat(acceptencoding+strlen(acceptencoding)," ");
			mlxCopyToken(s,acceptencoding+strlen(acceptencoding),160-strlen(acceptencoding));
			}
		    if (toktype == MLX_TOK_EOL || toktype == MLX_TOK_ERROR) break;
		    }
		mlxUnsetOptions(s,MLX_F_IFSONLY);*/
		/* printf("accept-encoding: %s\n",acceptencoding); */
		mlxUnsetOptions(s,MLX_F_LINEONLY);
		if (mlxNextToken(s) != MLX_TOK_EOL) { msg="Expected EOL after Accept-Encoding: header"; goto error; }
		}
	    else if (!strcmp(hdr,"if-modified-since"))
		{
		mlxSetOptions(s,MLX_F_LINEONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected date after If-Modified-Since:"; goto error; }
		mlxCopyToken(s, conn->IfModifiedSince, sizeof(conn->IfModifiedSince));
		mlxUnsetOptions(s,MLX_F_LINEONLY);
		if (mlxNextToken(s) != MLX_TOK_EOL) { msg="Expected EOL after If-Modified-Since: header"; goto error; }
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
	s = NULL;

    return 0;

    error:
	if (s) mlxCloseSession(s);
    return -1;
    }


/*** nht_i_CheckAccessLog - check to see if the access log needs to be
 *** (re)opened.
 ***/
int
nht_i_CheckAccessLog()
    {
    pFile test_fd;

	/** Is it open yet? **/
	if (!NHT.AccessLogFD)
	    {
	    NHT.AccessLogFD = fdOpen(NHT.AccessLogFile, O_CREAT | O_WRONLY | O_APPEND, 0600);
	    if (!NHT.AccessLogFD)
		{
		mssErrorErrno(1,"NHT","Could not open access_log file '%s'", NHT.AccessLogFile);
		}
	    }
	else
	    {
	    /** Has file been renamed (e.g., by logrotate) **/
	    test_fd = fdOpen(NHT.AccessLogFile, O_RDONLY, 0600);
	    if (!test_fd)
		{
		test_fd = fdOpen(NHT.AccessLogFile, O_CREAT | O_WRONLY | O_APPEND, 0600);
		if (!test_fd)
		    {
		    mssErrorErrno(1,"NHT","Could not reopen access_log file '%s'", NHT.AccessLogFile);
		    }
		else
		    {
		    fdClose(NHT.AccessLogFD, 0);
		    NHT.AccessLogFD = test_fd;
		    }
		}
	    else
		{
		fdClose(test_fd, 0);
		}
	    }

    return 0;
    }


/*** nhtInitialize - initialize the HTTP network handler and start the 
 *** listener thread.
 ***/
int
nhtInitialize()
    {
    pStructInf my_config;
    char* strval;
    int i;

    	/** Initialize the random number generator. **/
	srand48(time(NULL));

	/** Initialize globals **/
	NHT.numbCachedApps = 0;
	memset(&NHT, 0, sizeof(NHT));
	xhInit(&(NHT.CookieSessions),255,0);
	xhInit(&(NHT.SessionsByID),255,0);
	xaInit(&(NHT.Sessions),256);
	NHT.TimerUpdateSem = syCreateSem(0, 0);
	NHT.TimerDataMutex = syCreateSem(1, 0);
	xhnInitContext(&(NHT.TimerHctx));
	xaInit(&(NHT.Timers),512);
	NHT.WatchdogTime = 180;
	NHT.InactivityTime = 1800;
	NHT.CondenseJS = 1; /* not yet implemented */
	NHT.UserSessionLimit = 100;
	xhInit(&(NHT.UsersByName), 255, 0);
	xaInit(&NHT.UsersList, 64);
	NHT.AccCnt = 0;
	NHT.RestrictToLocalhost = 0;
	NHT.XFrameOptions = NHT_XFO_T_SAMEORIGIN;
	strcpy(NHT.UploadTmpDir, "/var/tmp");
	xaInit(&NHT.AllowedUploadDirs, 16);
	xaInit(&NHT.AllowedUploadExts, 16);
	NHT.CollectedConns = syCreateSem(0, 0);
	NHT.CollectedTLSConns = syCreateSem(0, 0);
	NHT.AuthMethods = NHT_AUTH_HTTPSTRICT;

#ifdef _SC_CLK_TCK
        NHT.ClkTck = sysconf(_SC_CLK_TCK);
#else
        NHT.ClkTck = CLK_TCK;
#endif

	NHT.A_ID_Count = 0;
	NHT.G_ID_Count = 0;
	NHT.S_ID_Count = 0;

	nht_i_RegisterSessionInfo();

	/** Set up header nonce **/
	NHT.NonceData = cxssKeystreamNew(NULL, 0);
	if (!NHT.NonceData)
	    mssError(1, "NHT", "Warning: X-Nonce headers will not be emitted");

	/** Set up secret for login cookie hash **/
	cxssGenerateKey(NHT.LoginKey, 32);

	/* intialize the regex for netscape 4.7 -- it has a broken gzip implimentation */
	NHT.reNet47=(regex_t *)nmMalloc(sizeof(regex_t));
	if(!NHT.reNet47 || regcomp(NHT.reNet47, "Mozilla\\/4\\.(7[5-9]|8)",REG_EXTENDED|REG_NOSUB|REG_ICASE))
	    {
	    printf("unable to build Netscape 4.7 regex\n"); /* shouldn't this be mssError? -- but there's no session yet.. */
	    return -1;
	    }

	/** Read configuration data **/
	my_config = stLookup(CxGlobals.ParsedConfig, "net_http");
	if (my_config)
	    {
	    /** Find out what server string we should use **/
	    if (stAttrValue(stLookup(my_config, "server_string"), NULL, &strval, 0) >= 0)
		{
		strtcpy(NHT.ServerString, strval, sizeof(NHT.ServerString));
		}
	    else
		{
		snprintf(NHT.ServerString, 80, "Centrallix/%.16s", cx__version);
		}

	    /** Get the realm name **/
	    if (stAttrValue(stLookup(my_config, "auth_realm"), NULL, &strval, 0) >= 0)
		{
		strtcpy(NHT.Realm, strval, sizeof(NHT.Realm));
		}
	    else
		{
		snprintf(NHT.Realm, 80, "Centrallix");
		}

	    /** Auth methods **/
	    for(i=0; i<4; i++)
		{
		if (stAttrValue(stLookup(my_config, "auth_methods"), NULL, &strval, i) >= 0)
		    {
		    if (i == 0)
			NHT.AuthMethods = 0;
		    if (!strcasecmp(strval, "http"))
			NHT.AuthMethods |= NHT_AUTH_HTTP;
		    else if (!strcasecmp(strval, "http-strict"))
			NHT.AuthMethods |= NHT_AUTH_HTTPSTRICT;
		    else if (!strcasecmp(strval, "http-bearer"))
			NHT.AuthMethods |= NHT_AUTH_HTTPBEARER;
		    else if (!strcasecmp(strval, "web-form"))
			NHT.AuthMethods |= NHT_AUTH_WEBFORM;
		    else
			mssError(1, "NHT", "Warning: invalid auth method '%s'", strval);
		    }
		else
		    {
		    break;
		    }
		}

	    /** Directory indexing? **/
	    for(i=0;i<sizeof(NHT.DirIndex)/sizeof(char*);i++)
		{
		stAttrValue(stLookup(my_config,"dir_index"), NULL, &(NHT.DirIndex[i]), i);
		}

	    /** Should we enable gzip? **/
#ifdef HAVE_LIBZ
	    stAttrValue(stLookup(my_config, "enable_gzip"), &(NHT.EnableGzip), NULL, 0);
#endif
	    stAttrValue(stLookup(my_config, "condense_js"), &(NHT.CondenseJS), NULL, 0);

	    stAttrValue(stLookup(my_config, "accept_localhost_only"), &(NHT.RestrictToLocalhost), NULL, 0);

	    /** X-Frame-Options anti-clickjacking header? **/
	    if (stAttrValue(stLookup(my_config, "x_frame_options"), NULL, &strval, 0) >= 0)
		{
		if (!strcasecmp(strval, "none"))
		    NHT.XFrameOptions = NHT_XFO_T_NONE;
		else if (!strcasecmp(strval, "deny"))
		    NHT.XFrameOptions = NHT_XFO_T_DENY;
		else if (!strcasecmp(strval, "sameorigin")) /* default - see net_http.c */
		    NHT.XFrameOptions = NHT_XFO_T_SAMEORIGIN;
		}

	    /** Get the timer settings **/
	    stAttrValue(stLookup(my_config, "session_watchdog_timer"), &(NHT.WatchdogTime), NULL, 0);
	    if (NHT.WatchdogTime <= 0 || NHT.WatchdogTime > 200000)
		{
		mssError(1, "NHT", "Warning: session_watchdog_timer (%d) must be > 0 and <= 200000, reverting to default (180).", NHT.WatchdogTime);
		NHT.WatchdogTime = 180;
		}
	    stAttrValue(stLookup(my_config, "session_inactivity_timer"), &(NHT.InactivityTime), NULL, 0);
	    if (NHT.InactivityTime <= NHT.WatchdogTime || NHT.InactivityTime > 200000)
		{
		i = (NHT.WatchdogTime >= 1800)?(NHT.WatchdogTime + 1):1800;
		if (i > 200000)
		    {
		    i = 200000;
		    NHT.WatchdogTime = 199999;
		    }
		mssError(1, "NHT", "Warning: session_inactivity_timer (%d) must be > watchdog (%d) and <= 200000, reverting to %d.", NHT.InactivityTime, NHT.WatchdogTime, i);
		NHT.InactivityTime = i;
		}

	    /** Session limits **/
	    stAttrValue(stLookup(my_config, "user_session_limit"), &(NHT.UserSessionLimit), NULL, 0);
	    if (NHT.UserSessionLimit > 1000 || NHT.UserSessionLimit < 1)
		{
		mssError(1, "NHT", "Warning: user_session_limit (%d) must be > 0 and <= 1000, reverting to default (100).", NHT.UserSessionLimit);
		NHT.UserSessionLimit = 100;
		}

	    /** Cookie name **/
	    if (stAttrValue(stLookup(my_config, "session_cookie"), NULL, &strval, 0) < 0)
		{
		strval = "CXID";
		}
	    strtcpy(NHT.SessionCookie, strval, sizeof(NHT.SessionCookie));
	    if (stAttrValue(stLookup(my_config, "ssl_session_cookie"), NULL, &strval, 0) < 0)
		{
		strval = NHT.SessionCookie;
		}
	    strtcpy(NHT.TlsSessionCookie, strval, sizeof(NHT.TlsSessionCookie));

	    /** Access log file **/
	    if (stAttrValue(stLookup(my_config, "access_log"), NULL, &strval, 0) >= 0)
		{
		strtcpy(NHT.AccessLogFile, strval, sizeof(NHT.AccessLogFile));
		nht_i_CheckAccessLog();
		}

	    /** Upload temporary directory **/
	    if (stAttrValue(stLookup(my_config, "upload_tmpdir"), NULL, &strval, 0) >= 0)
		strtcpy(NHT.UploadTmpDir, strval, sizeof(NHT.UploadTmpDir));

	    /** Allowed file upload directories **/
	    for(i=0; stAttrValue(stLookup(my_config, "upload_dirs"), NULL, &strval, i) >= 0; i++)
		xaAddItem(&NHT.AllowedUploadDirs, nmSysStrdup(strval));

	    /** Allowed file upload extensions **/
	    for(i=0; stAttrValue(stLookup(my_config, "upload_extensions"), NULL, &strval, i) >= 0; i++)
		xaAddItem(&NHT.AllowedUploadExts, nmSysStrdup(strval));
	    }

	/** Start the watchdog timer thread **/
	thCreate(nht_i_Watchdog, 0, NULL);

	/** Start the network listeners. **/
	thCreate(nht_i_TLSHandler, 0, NULL);
	thCreate(nht_i_Handler, 0, NULL);

    return 0;
    }


int CachedAppDeconstructor(pCachedApp this)
    {
    nmSysFree(&this->Key);
    wgtrFree(this->Node);
    return 0;
    }
