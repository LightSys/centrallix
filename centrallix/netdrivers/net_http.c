#include "net_http.h"
#include "cxss/cxss.h"

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


/*** Functions for enumerating users for the cx.sysinfo directory ***/
pXArray
nht_internal_UsersAttrList(void* ctx, char* objname)
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
nht_internal_UsersObjList(void* ctx)
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
nht_internal_UsersAttrType(void *ctx, char* objname, char* attrname)
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
nht_internal_UsersAttrValue(void* ctx, char* objname, char* attrname, void* val_v)
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
nht_internal_SessionsAttrList(void* ctx, char* objname)
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
nht_internal_SessionsObjList(void* ctx)
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
nht_internal_SessionsAttrType(void *ctx, char* objname, char* attrname)
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
nht_internal_SessionsAttrValue(void* ctx, char* objname, char* attrname, void* val_v)
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
nht_internal_GroupsAttrList(void* ctx, char* objname)
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
nht_internal_GroupsObjList(void* ctx)
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
nht_internal_GroupsAttrType(void *ctx, char* objname, char* attrname)
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
nht_internal_GroupsAttrValue(void* ctx, char* objname, char* attrname, void* val_v)
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
nht_internal_AppsAttrList(void* ctx, char* objname)
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
nht_internal_AppsObjList(void* ctx)
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
nht_internal_AppsAttrType(void *ctx, char* objname, char* attrname)
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
nht_internal_AppsAttrValue(void* ctx, char* objname, char* attrname, void* val_v)
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


/*** nht_internal_RegisterSessionInfo() - register a handler for listing
 *** users logged into the server.
 ***/
int
nht_internal_RegisterSessionInfo()
    {
    pSysInfoData si;

	/** users list **/
	si = sysAllocData("/session/users", nht_internal_UsersAttrList, nht_internal_UsersObjList, NULL, nht_internal_UsersAttrType, nht_internal_UsersAttrValue, NULL, 0);
	sysRegister(si, NULL);

	/** sessions list **/
	si = sysAllocData("/session/sessions", nht_internal_SessionsAttrList, nht_internal_SessionsObjList, NULL, nht_internal_SessionsAttrType, nht_internal_SessionsAttrValue, NULL, 0);
	sysRegister(si, NULL);

	/** groups list **/
	si = sysAllocData("/session/appgroups", nht_internal_GroupsAttrList, nht_internal_GroupsObjList, NULL, nht_internal_GroupsAttrType, nht_internal_GroupsAttrValue, NULL, 0);
	sysRegister(si, NULL);

	/** apps list **/
	si = sysAllocData("/session/apps", nht_internal_AppsAttrList, nht_internal_AppsObjList, NULL, nht_internal_AppsAttrType, nht_internal_AppsAttrValue, NULL, 0);
	sysRegister(si, NULL);

    return 0;
    }


/*** nht_internal_AddRequestHeader - add a header to the HTTP request
 ***/
int
nht_internal_AddHeader(pXArray hdrlist, char* hdrname, char* hdrvalue, int hdralloc)
    {
    pHttpHeader hdr;
    int i;

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


/*** nht_internal_FreeHeaders - release HTTP headers from an xarray
 ***/
int
nht_internal_FreeHeaders(pXArray xa)
    {
    pHttpHeader hdr;
    int i;

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


/*** nht_internal_GetHeader - find a header with the given name
 ***/
char*
nht_internal_GetHeader(pXArray xa, char* name)
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


/*** error exit handler -- does not return
 ***/
void
nht_internal_ErrorExit(pNhtConn conn, int code, char* text)
    {

	/** Write the standard HTTP response **/
	nht_internal_WriteResponse(conn, code, text, -1, "text/html", NULL, NULL);
	fdQPrintf(conn->ConnFD, "<H1>%POS %STR</H1>\r\n<hr><pre>\r\n", code, text);

	/** Display error info **/
	mssPrintError(conn->ConnFD);

	/** Shutdown the connection and free memory **/
	nht_internal_FreeConn(conn);

    thExit();
    }


/*** nht_internal_WriteResponse() - write the HTTP response header,
 *** not including content.
 ***/
int
nht_internal_WriteResponse(pNhtConn conn, int code, char* text, int contentlen, char* contenttype, char* pragma, char* resptxt)
    {
    int wcnt, rval;
    struct tm* thetime;
    time_t tval;
    char tbuf[40];

	/** Get the current date/time **/
	tval = time(NULL);
	thetime = gmtime(&tval);
	strftime(tbuf, sizeof(tbuf), "%a, %d %b %Y %T", thetime);

	wcnt = fdQPrintf(conn->ConnFD, 
		"HTTP/1.0 %INT %STR\r\n"
		"Server: %STR\r\n"
		"Date: %STR\r\n"
		"%[Set-Cookie: %STR\r\n%]"
		"%[Content-Length: %INT\r\n%]"
		"%[Content-Type: %STR\r\n%]"
		"%[Pragma: %STR\r\n%]"
		"\r\n",
		code,
		text,
		NHT.ServerString,
		tbuf,
		(conn->NhtSession && conn->NhtSession->IsNewCookie && conn->NhtSession->Cookie), (conn->NhtSession)?conn->NhtSession->Cookie:NULL,
		contentlen > 0, contentlen,
		contenttype != NULL, contenttype,
		pragma != NULL, pragma);
	if (wcnt < 0) return wcnt;

	if (resptxt)
	    {
	    rval = nht_internal_WriteConn(conn, resptxt, strlen(resptxt), 0);
	    if (rval < 0) return rval;
	    wcnt += rval;
	    }

	if (conn->NhtSession && conn->NhtSession->IsNewCookie && conn->NhtSession->Cookie)
	    conn->NhtSession->IsNewCookie = 0;

    return wcnt;
    }


/*** nht_internal_WriteErrResponse() - write an HTTP error response
 *** header without any real content.  Use the normal WriteResponse
 *** routine if you want more flexible content.
 ***/
int
nht_internal_WriteErrResponse(pNhtConn conn, int code, char* text, char* bodytext)
    {
    int wcnt, rval;

	wcnt = nht_internal_WriteResponse(conn, code, text, strlen(bodytext), "text/html", NULL, NULL);
	if (wcnt < 0) return wcnt;
	if (bodytext && *bodytext)
	    {
	    rval = nht_internal_WriteConn(conn, bodytext, strlen(bodytext), 0);
	    if (rval < 0) return rval;
	    wcnt += rval;
	    }

    return wcnt;
    }


/*** nht_internal_FreeControlMsg() - release memory used by a control
 *** message, its parameters, etc.
 ***/
int
nht_internal_FreeControlMsg(pNhtControlMsg cm)
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
nht_internal_CacheHandler(pNhtConn conn)
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



/*** nht_internal_ControlMsgHandler() - the main handler for all connections
 *** which access /INTERNAL/control, thus requesting to receive control
 *** messages from the system.
 ***/
int
nht_internal_ControlMsgHandler(pNhtConn conn, pStruct url_inf)
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
		nht_internal_WriteResponse(conn, 200, "OK", -1, "text/html", "no-cache", "<A HREF=0.0 TARGET=0>NO SUCH MESSAGE</A>\r\n");
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
		nht_internal_FreeControlMsg(cm);

	    /** Tell syGetSem to return immediately, below **/
	    wait_for_sem = 0;
	    }

	/** Send header **/
	nht_internal_WriteResponse(conn, 200, "OK", -1, "text/html", "no-cache", NULL);

    	/** Wait on the control msgs semaphore **/
	while (1)
	    {
	    if (syGetSem(sess->ControlMsgs, 1, wait_for_sem?0:SEM_U_NOBLOCK) < 0)
		{
		nht_internal_WriteConn(conn, "<A HREF=0.0 TARGET=0>END OF CONTROL MESSAGES</A>\r\n", -1, 0);
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
	    nht_internal_QPrintfConn(conn, 0, "<A HREF=%INT.%INT TARGET=%POS>CONTROL MESSAGE</A>\r\n",
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
		nht_internal_FreeControlMsg(cm);
		}

	    wait_for_sem = 0;
	    }

    return 0;
    }



/*** nht_internal_ErrorHandler - handle the printing of notice and error
 *** messages to the error stream for the client, if the client has such
 *** an error stream (which is how this is called).
 ***/
int
nht_internal_ErrorHandler(pNhtConn net_conn)
    {
    pNhtSessionData nsess = net_conn->NhtSession;
    pXString errmsg;

    	/** Wait on the errors semaphore **/
	if (syGetSem(nsess->Errors, 1, 0) < 0)
	    {
	    fdPrintf(net_conn->ConnFD,"HTTP/1.0 200 OK\r\n"
			 "Server: %s\r\n"
			 "Pragma: no-cache\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<A HREF=/ TARGET=ERR></A>\r\n",NHT.ServerString);
	    return -1;
	    }

	/** Grab one error **/
	errmsg = (pXString)(nsess->ErrorList.Items[0]);
	xaRemoveItem(&nsess->ErrorList, 0);

	/** Format the error and print it as HTML. **/
	fdPrintf(net_conn->ConnFD,"HTTP/1.0 200 OK\r\n"
		     "Server: %s\r\n"
		     "Pragma: no-cache\r\n"
		     "Content-Type: text/html\r\n"
		     "\r\n"
		     "<HTML><BODY><PRE><A NAME=\"Message\">",NHT.ServerString);
	fdWrite(net_conn->ConnFD,errmsg->String,strlen(errmsg->String),0,0);
	fdPrintf(net_conn->ConnFD,"</A></PRE></BODY></HTML>\r\n");

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


/*** nht_internal_Hex16ToInt - convert a 16-bit hex value, in four chars, to
 *** an unsigned integer.
 ***/
unsigned int
nht_internal_Hex16ToInt(char* hex)
    {
    char hex2[5];
    memcpy(hex2, hex, 4);
    hex2[4] = '\0';
    return strtoul(hex2, NULL, 16);
    }


/*** nht_internal_POST - handle the HTTP POST method.
 ***/
int
nht_internal_POST(pNhtConn conn, pStruct url_inf, int size)
    {
    pNhtSessionData nsess = conn->NhtSession;
    pStruct find_inf;

	/** app key must be specified for all POST operations. **/
	find_inf = stLookup_ne(url_inf,"cx__akey");
	if (!find_inf || strcmp(find_inf->StrVal, nsess->SKey) != 0)
	    return -1;

    return 0;
    }


/*** nht_internal_GET - handle the HTTP GET method, reading a document or
 *** attribute list, etc.
 ***/
int
nht_internal_GET(pNhtConn conn, pStruct url_inf, char* if_modified_since)
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
    int tid = -1;
    int convert_text = 0;
    pDateTime dt = NULL;
    DateTime dtval;
    struct tm systime;
    struct tm* thetime;
    time_t tval;
    char tbuf[32];
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

	acceptencoding=(char*)mssGetParam("Accept-Encoding");

//START INTERNAL handler -------------------------------------------------------------------
//TODO (from Seth): should this be moved out of nht_internal_GET and back into nht_internal_ConnHandler?

    	/*printf("GET called, stack ptr = %8.8X\n",&cnt);*/
        /** If we're opening the "errorstream", pass of processing to err handler **/
	if (!strncmp(url_inf->StrVal,"/INTERNAL/errorstream",21))
	    {
		return nht_internal_ErrorHandler(conn);
	    }
	else if (!strncmp(url_inf->StrVal, "/INTERNAL/control", 17))
	    {
		return nht_internal_ControlMsgHandler(conn, url_inf);
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
	    if (nht_internal_VerifyAKey(find_inf->StrVal, nsess, &group, &app) == 0)
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

	/** Indicate activity... **/
	if (!conn->NotActivity)
	    {
	    if (group) objCurrentDate(&(group->LastActivity));
	    if (app) objCurrentDate(&(app->LastActivity));
	    /*if (group) nht_internal_ResetWatchdog(group->InactivityTimer);
	    if (app) nht_internal_ResetWatchdog(app->InactivityTimer);*/
	    }
	if (group) nht_internal_ResetWatchdog(group->WatchdogTimer);
	if (app) nht_internal_ResetWatchdog(app->WatchdogTimer);

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
		nht_internal_GenerateError(nsess);
		fdPrintf(conn->ConnFD,"HTTP/1.0 404 Not Found\r\n"
			     "Server: %s\r\n"
			     "Content-Type: text/html\r\n"
			     "\r\n"
			     "<H1>404 Not Found</H1><HR><PRE>\r\n",NHT.ServerString);
		mssPrintError(conn->ConnFD);
		netCloseTCP(conn->ConnFD,1000,0);
		nht_internal_UnlinkSess(nsess);
		thExit();
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
		nht_internal_CkParams(url_inf, target_obj);
	    }
	else
	    {
	    target_obj = NULL;
	    }

	/** WAIT TRIGGER mode. **/
	if (find_inf && !strcmp(find_inf->StrVal,"triggerwait"))
	    {
	    find_inf = stLookup_ne(url_inf,"ls__waitid");
	    if (find_inf)
	        {
		tid = strtoi(find_inf->StrVal,NULL,0);
		nht_internal_WaitTrigger(nsess,tid);
		}
	    }

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

	/** Ok, issue the HTTP header for this one. **/
	fdSetOptions(conn->ConnFD, FD_UF_WRBUF);
	if (nsess->IsNewCookie)
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
	    }

	/** Exit now if wait trigger. **/
	if (tid != -1)
	    {
	    fdWrite(conn->ConnFD,"OK\r\n",4,0,0);
	    objClose(target_obj);
	    return 0;
	    }

	/** Add anti-clickjacking X-Frame-Options header?
	 ** see: https://developer.mozilla.org/en/The_X-FRAME-OPTIONS_response_header
	 **/
	if (target_obj && (!strcmp(ptr,"widget/page") || !strcmp(ptr,"widget/frameset") || !strcmp(ptr,"widget/component-decl")))
	    {
	    xfo_ptr = NULL;
	    if (objGetAttrValue(target_obj, "http_frame_options", DATA_T_STRING, POD(&xfo_ptr)) == 0 || NHT.XFrameOptions != NHT_XFO_T_NONE)
		{
		if ((xfo_ptr && !strcmp(xfo_ptr,"deny")) || (!xfo_ptr && NHT.XFrameOptions == NHT_XFO_T_DENY))
		    fdPrintf(conn->ConnFD, "X-Frame-Options: DENY\r\n");
		else if ((xfo_ptr && !strcmp(xfo_ptr,"sameorigin")) || (!xfo_ptr && NHT.XFrameOptions == NHT_XFO_T_SAMEORIGIN))
		    fdPrintf(conn->ConnFD, "X-Frame-Options: SAMEORIGIN\r\n");
		}
	    }

	/** Add content-disposition header with filename **/
	if (target_obj && objGetAttrValue(target_obj, "cx__download_as", DATA_T_STRING, POD(&name)) == 0)
	    {
	    find_inf2 = stLookup_ne(url_inf,"cx__forcedownload");
	    if (!strpbrk(name, "\r\n\t"))
		fdQPrintf(conn->ConnFD, "Content-Disposition: %STR&SYM; filename=%STR&DQUOT\r\n",
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
	    fdPrintf(conn->ConnFD, "Last-Modified: %s GMT\r\n", tbuf);
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
		    fdPrintf(conn->ConnFD,"Content-Type: text/html\r\n\r\n");
		    nht_internal_GetGeom(target_obj, conn->ConnFD);
		    objClose(target_obj);
		    return 0;
		    }
		if (!strcmp(gptr,"design"))
		    {
		    client_h=0;
		    client_w=0;
		    objGetAttrValue(target_obj, "width", DATA_T_INTEGER, POD(&client_w));
		    objGetAttrValue(target_obj, "height", DATA_T_INTEGER, POD(&client_h));
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
		    client_w = nht_internal_Hex16ToInt(gptr);
		    client_h = nht_internal_Hex16ToInt(gptr+4);
		    wgtr_params.CharWidth = nht_internal_Hex16ToInt(gptr+8);
		    wgtr_params.CharHeight = nht_internal_Hex16ToInt(gptr+12);
		    wgtr_params.ParagraphHeight = nht_internal_Hex16ToInt(gptr+16);
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
		    fdPrintf(conn->ConnFD,"Content-Encoding: gzip\r\n");
		    }
		fdPrintf(conn->ConnFD,"Content-Type: text/html\r\nPragma: no-cache\r\n\r\n");
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
			group = nht_internal_AllocAppGroup(url_inf->StrVal, nsess);
		    if (group && !app)
			app = nht_internal_AllocApp(url_inf->StrVal, group);
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
		    //|| nht_internal_Parse_and_Cache_an_App(target_obj, url_inf, &wgtr_params, nsess, &tree) < 0
		    //|| nht_internal_Verify_and_Position_and_Render_an_App(conn->ConnFD, nsess, &wgtr_params, "DHTML", tree) < 0)
		if (nhtRenderApp(conn->ConnFD, target_obj->Session, target_obj, url_inf, &wgtr_params, "DHTML", nsess) < 0)
		    {
		    mssError(0, "HTTP", "Unable to render application %s of type %s", url_inf->StrVal, ptr);
		    fdPrintf(conn->ConnFD,"<h1>An error occurred while constructing the application:</h1><pre>");
		    mssPrintError(conn->ConnFD);
		    objClose(target_obj);
		    if (tptr) nmSysFree(tptr);
		    if (lptr) nmSysFree(lptr);
		    return -1;
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
		fdPrintf(conn->ConnFD,"Content-Type: text/html\r\n\r\n");

		/** call the html-related interface translating function **/
		if (ifcToHtml(conn->ConnFD, nsess->ObjSess, url_inf->StrVal) < 0)
		    {
		    mssError(0, "NHT", "Error sending Interface info for '%s' to client", url_inf->StrVal);
		    fdQPrintf(conn->ConnFD, "<A TARGET=\"ERR\" HREF=\"%STR&HTE\"></A>", url_inf->StrVal);
		    }
		else
		    {
		    fdQPrintf(conn->ConnFD, "<A NAME=\"%s\" TARGET=\"OK\" HREF=\"%STR&HTE\"></A>", ptr, url_inf->StrVal);
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
			obj_internal_IsA(ptr,"text/plain")>0 /* a subtype of text/plain */
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
		    fdPrintf(conn->ConnFD,"Content-Encoding: gzip\r\n");
		    }
		fdPrintf(conn->ConnFD,"Content-Type: %s\r\n\r\n", ptr);
		if(gzip==1)
		    {
		    fdSetOptions(conn->ConnFD, FD_UF_GZIP);
		    }
		if (convert_text) fdWrite(conn->ConnFD,"<HTML><PRE>",11,0,FD_U_PACKET);
		bufptr = (char*)nmMalloc(4096);
	        while((cnt=objRead(target_obj,bufptr,4096,0,0)) > 0)
	            {
		    fdWrite(conn->ConnFD,bufptr,cnt,0,FD_U_PACKET);
		    }
		if (convert_text) fdWrite(conn->ConnFD,"</HTML></PRE>",13,0,FD_U_PACKET);
		if (cnt < 0) 
		    {
		    mssError(0,"NHT","Incomplete read of object's content");
		    nht_internal_GenerateError(nsess);
		    }
		nmFree(bufptr, 4096);
	        }
	    }

	/** REST mode? **/
	else if (!strcmp(find_inf->StrVal,"rest"))
	    {
	    rval = nht_internal_RestGet(conn, url_inf, target_obj);
	    }

	/** GET DIRECTORY LISTING mode. **/
	else if (!strcmp(find_inf->StrVal,"list"))
	    {
	    if (stAttrValue_ne(stLookup_ne(url_inf,"ls__info"),&ptr) >= 0 && !strcmp(ptr,"1"))
		send_info = 1;
	    if (stAttrValue_ne(stLookup_ne(url_inf,"ls__orderdesc"),&ptr) >= 0 && !strcmp(ptr,"1"))
		order_desc = 1;
	    if (order_desc)
		query = objOpenQuery(target_obj,"",":name desc",NULL,NULL);
	    else
		query = objOpenQuery(target_obj,"",NULL,NULL,NULL);
	    if (query)
	        {
		fdPrintf(conn->ConnFD,"Content-Type: text/html\r\n\r\n");
		fdQPrintf(conn->ConnFD,"<HTML><HEAD><META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\"></HEAD><BODY><TT><A HREF=\"%STR&HTE/..\">..</A><BR>\n",url_inf->StrVal);
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
			fdQPrintf(conn->ConnFD,"<A HREF=\"%STR&HTE%[/%]%STR&HTE\" TARGET='%STR&HTE'>%INT:%INT:%STR&HTE</A><BR>\n",dptr,
			    (dptr[0]!='/' || dptr[1]!='\0'),ptr,ptr,objinfo->Flags,objinfo->nSubobjects,aptr);
			}
		    else if (send_info && !objinfo)
			{
			fdQPrintf(conn->ConnFD,"<A HREF=\"%STR&HTE%[/%]%STR&HTE\" TARGET='%STR&HTE'>0:0:%STR&HTE</A><BR>\n",dptr,
			    (dptr[0]!='/' || dptr[1]!='\0'),ptr,ptr,aptr);
			}
		    else
			{
			fdQPrintf(conn->ConnFD,"<A HREF=\"%STR&HTE%[/%]%STR&HTE\" TARGET='%STR&HTE'>%STR&HTE</A><BR>\n",dptr,
			    (dptr[0]!='/' || dptr[1]!='\0'),ptr,ptr,aptr);
			}
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
	else if (!strcmp(find_inf->StrVal,"query") && akey_match)
	    {
	    /** Change directory to appropriate query root **/
	    fdPrintf(conn->ConnFD,"Content-Type: text/html\r\n\r\n");
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
			nht_internal_WriteAttrs(sub_obj,conn,(handle_t)rowid,1);
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
	    find_inf = stLookup_ne(url_inf,"ls__req");
	    nht_internal_OSML(conn,target_obj, find_inf->StrVal, url_inf);
	    }

	/** Exec method mode **/
	else if (!strcmp(find_inf->StrVal,"execmethod") && akey_match)
	    {
	    find_inf = stLookup_ne(url_inf,"ls__methodname");
	    find_inf2 = stLookup_ne(url_inf,"ls__methodparam");
	    fdPrintf(conn->ConnFD, "Content-Type: text/html\r\nPragma: no-cache\r\n\r\n");
	    if (!find_inf || !find_inf2)
	        {
		mssError(1,"NHT","Invalid call to execmethod - requires name and param");
		nht_internal_GenerateError(nsess);
		}
	    else
	        {
	    	ptr = find_inf2->StrVal;
	    	objExecuteMethod(target_obj, find_inf->StrVal, POD(&ptr));
		fdWrite(conn->ConnFD,"OK",2,0,0);
		}
	    }

	/** Close the objectsystem entry. **/
	if (target_obj) objClose(target_obj);

    return 0;
    }


/*** nht_internal_PUT - implements the PUT HTTP method.  Set content_buf to
 *** data to write, otherwise it will be read from the connection if content_buf
 *** is NULL.
 ***/
int
nht_internal_PUT(pNhtConn conn, pStruct url_inf, int size, char* content_buf)
    {
    pNhtSessionData nsess = conn->NhtSession;
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
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>404 Not Found</H1><HR><PRE>\r\n",NHT.ServerString);
	    fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
	    mssPrintError(conn->ConnFD);
	    netCloseTCP(conn->ConnFD,1000,0);
	    nht_internal_UnlinkSess(nsess);
	    thExit();
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
	if (nsess->IsNewCookie)
	    {
	    if (already_exist)
	        {
	        snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
		     "Server: %s\r\n"
		     "Set-Cookie: %s; path=/\r\n"
		     "Content-Type: text/html\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,nsess->Cookie, url_inf->StrVal);
		}
	    else
	        {
	        snprintf(sbuf,160,"HTTP/1.0 201 Created\r\n"
		     "Server: %s\r\n"
		     "Set-Cookie: %s; path=/\r\n"
		     "Content-Type: text/html\r\n"
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
		     "Content-Type: text/html\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,url_inf->StrVal);
		}
	    else
	        {
	        snprintf(sbuf,160,"HTTP/1.0 201 Created\r\n"
		     "Server: %s\r\n"
		     "Content-Type: text/html\r\n"
		     "\r\n"
		     "%s\r\n", NHT.ServerString,url_inf->StrVal);
		}
	    }
	fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);

    return 0;
    }


/*** nht_internal_COPY - implements the COPY centrallix-http method.
 ***/
int
nht_internal_COPY(pNhtConn conn, pStruct url_inf, char* dest)
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
		nht_internal_ErrorExit(conn, 404, "Source Not Found");
		}

	    /** If using srctype copy mode, get the type now **/
	    if (!copytype)
		objGetAttrValue(source_obj, "inner_type", DATA_T_STRING, POD(&copytype));
	    }

	/** Do we need to set params as a part of the open? **/
	nht_internal_CkParams(url_inf, source_obj);

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
		nht_internal_ErrorExit(conn, 404, "Target Not Found");
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
		nht_internal_ErrorExit(conn, 404, "Source Not Found");
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
	nht_internal_WriteResponse(conn, already_exists?200:201, already_exists?"OK":"Created", -1, "text/html", NULL, NULL);
	fdQPrintf(conn->ConnFD, "%STR\r\n", dest);

    return 0;
    }


/*** nht_internal_ParseHeaders - read from the connection and parse the
 *** headers into the NhtConn structure
 ***/
int
nht_internal_ParseHeaders(pNhtConn conn)
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
	    nht_internal_FreeConn(conn);
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
		mlxSetOptions(s,MLX_F_IFSONLY);
		if (mlxNextToken(s) != MLX_TOK_STRING) { msg="Expected str after Cookie:"; goto error; }
		mlxCopyToken(s,conn->Cookie,sizeof(conn->Cookie));
		while((toktype = mlxNextToken(s)))
		    {
		    if (toktype == MLX_TOK_EOL || toktype == MLX_TOK_ERROR) break;
		    /** if the token is a string, and the current cookie doesn't look like ours, try the next one **/
		    if (toktype == MLX_TOK_STRING && (strncmp(conn->Cookie,NHT.SessionCookie,strlen(NHT.SessionCookie)) || conn->Cookie[strlen(NHT.SessionCookie)] != '='))
			{
			mlxCopyToken(s,conn->Cookie,sizeof(conn->Cookie));
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

#ifdef _SC_CLK_TCK
        NHT.ClkTck = sysconf(_SC_CLK_TCK);
#else
        NHT.ClkTck = CLK_TCK;
#endif

	NHT.A_ID_Count = 0;
	NHT.G_ID_Count = 0;
	NHT.S_ID_Count = 0;

	nht_internal_RegisterSessionInfo();

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
	    stAttrValue(stLookup(my_config, "session_inactivity_timer"), &(NHT.InactivityTime), NULL, 0);

	    /** Session limits **/
	    stAttrValue(stLookup(my_config, "user_session_limit"), &(NHT.UserSessionLimit), NULL, 0);

	    /** Cookie name **/
	    if (stAttrValue(stLookup(my_config, "session_cookie"), NULL, &strval, 0) < 0)
		{
		strval = "CXID";
		}
	    strtcpy(NHT.SessionCookie, strval, sizeof(NHT.SessionCookie));

	    /** Access log file **/
	    if (stAttrValue(stLookup(my_config, "access_log"), NULL, &strval, 0) >= 0)
		{
		strtcpy(NHT.AccessLogFile, strval, sizeof(NHT.AccessLogFile));
		NHT.AccessLogFD = fdOpen(NHT.AccessLogFile, O_WRONLY | O_APPEND, 0600);
		if (!NHT.AccessLogFD)
		    {
		    mssErrorErrno(1,"NHT","Could not open access_log file '%s'", NHT.AccessLogFile);
		    }
		}
	    }

	/** Start the watchdog timer thread **/
	thCreate(nht_internal_Watchdog, 0, NULL);

	/** Start the network listeners. **/
	thCreate(nht_internal_Handler, 0, NULL);
	thCreate(nht_internal_TLSHandler, 0, NULL);

    return 0;
    }


int CachedAppDeconstructor(pCachedApp this)
    {
    nmSysFree(&this->Key);
    wgtrFree(this->Node);
    return 0;
    }
