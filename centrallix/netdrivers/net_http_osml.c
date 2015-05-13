#include "net_http.h"

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
/* Module: 	net_http.h, net_http.c, net_http_conn.c, net_http_sess.c, net_http_osml.c, net_http_app.c			*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 8, 1998  					*/
/* Description:	Network handler providing an HTTP interface to the 	*/
/*		Centrallix and the ObjectSystem.			*/
/************************************************************************/

 
 
/*** nht_internal_ConstructPathname - constructs the proper OSML pathname
 *** for the open-object operation, given the apparent pathname and url
 *** parameters.  This primarily involves recovering the 'ls__type' setting
 *** and putting it back in the path.
 ***/
int
nht_internal_ConstructPathname(pStruct url_inf)
    {
    pStruct param_inf;
    char* oldpath;
    char* newpath;
    char* old_param;
    char* new_param;
    int len;
    int i;
    int first_param = 1;

    	/** Does it have ls__type? **/
	for(i=0;i<url_inf->nSubInf;i++)
	    {
	    param_inf = url_inf->SubInf[i];
	    if (param_inf->Type != ST_T_ATTRIB) continue;
	    if ((!strncmp(param_inf->Name, "ls__", 4) || !strncmp(param_inf->Name, "cx__",4)) && strcmp(param_inf->Name, "ls__type"))
		continue;
	    oldpath = url_inf->StrVal;
	    stAttrValue_ne(param_inf,&old_param);

	    /** Get an encoded param **/
	    len = strlen(old_param)*3+1;
	    new_param = (char*)nmSysMalloc(len);
	    qpfPrintf(NULL,new_param,len,"%STR&URL",old_param);

	    /** Build the new pathname **/
	    newpath = (char*)nmSysMalloc(strlen(oldpath) + strlen(param_inf->Name) + 
		    strlen(new_param) + 4);
	    sprintf(newpath, "%s%c%s=%s", oldpath, first_param?'?':'&', 
		    param_inf->Name, new_param);

	    /** set the new path and remove the old one **/
	    if (url_inf->StrAlloc) nmSysFree(url_inf->StrVal);
	    url_inf->StrVal = newpath;
	    url_inf->StrAlloc = 1;
	    first_param=0;
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
		if (syGetSem(trg->TriggerSem, 1, 0) < 0)
		    {
		    xaRemoveItem(&(sess->Triggers), i);
		    nmFree(trg, sizeof(NhtConnTrigger));
		    break;
		    }
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


/*** nht_internal_WriteOneAttr - put one attribute's information into the
 *** outbound data connection stream.
 ***/
int
nht_internal_WriteOneAttr(pObject obj, pNhtConn conn, handle_t tgt, char* attrname)
    {
    ObjData od;
    char* dptr;
    int type,rval;
    XString xs, hints;
    pObjPresentationHints ph;
    static char* coltypenames[] = {"unknown","integer","string","double","datetime","intvec","stringvec","money",""};

	/** Get type **/
	type = objGetAttrType(obj,attrname);
	if (type < 0 || type > 7) return -1;

	/** Presentation Hints **/
	xsInit(&hints);
	ph = objPresentationHints(obj, attrname);
	hntEncodeHints(ph, &hints);
	objFreeHints(ph);

	/** Get value **/
	rval = objGetAttrValue(obj,attrname,type,&od);
	if (rval != 0) 
	    dptr = "";
	else if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE) 
	    dptr = objDataToStringTmp(type, (void*)&od, 0);
	else
	    dptr = objDataToStringTmp(type, (void*)(od.String), 0);
	if (!dptr) dptr = "";

	/** Write the HTML output. **/
	xsInit(&xs);
	if (tgt != XHN_INVALID_HANDLE && tgt == conn->LastHandle)
	    xsPrintf(&xs, "<A TARGET=R HREF='http://");
	else
	    {
	    conn->LastHandle = tgt;
	    xsPrintf(&xs, "<A TARGET=X" XHN_HANDLE_PRT " HREF='http://", tgt);
	    }
	xsConcatQPrintf(&xs, "X%STR&HEX/?%STR#%STR'>%STR:", 
		attrname, hints.String, coltypenames[type], (rval==0)?"V":((rval==1)?"N":"E"));

	xsQPrintf(&xs,"%STR%STR&URL",xs.String,dptr);

	xsConcatenate(&xs,"</A><br>\n",9);
	fdWrite(conn->ConnFD,xs.String,strlen(xs.String),0,0);
	xsDeInit(&xs);
	xsDeInit(&hints);

    return 0;
    }


/*** nht_internal_WriteAttrs - write an HTML-encoded attribute list for the
 *** object to the connection, given an object and a connection.
 ***/
int
nht_internal_WriteAttrs(pObject obj, pNhtConn conn, handle_t tgt, int put_meta)
    {
    char* attr;

	conn->LastHandle = XHN_INVALID_HANDLE;

	/** Loop throught the attributes. **/
	if (put_meta)
	    {
	    nht_internal_WriteOneAttr(obj, conn, tgt, "name");
	    nht_internal_WriteOneAttr(obj, conn, tgt, "inner_type");
	    nht_internal_WriteOneAttr(obj, conn, tgt, "outer_type");
	    nht_internal_WriteOneAttr(obj, conn, tgt, "annotation");
	    }
	for(attr = objGetFirstAttr(obj); attr; attr = objGetNextAttr(obj))
	    {
	    nht_internal_WriteOneAttr(obj, conn, tgt, attr);
	    }

    return 0;
    }


/*** nht_internal_UpdateNotify() - this routine is called if the UI requested
 *** notifications on an object modification, and such a modification has
 *** indeed occurred.
 ***/
int
nht_internal_UpdateNotify(void* v)
    {
    pObjNotification n = (pObjNotification)v;
    pNhtSessionData sess = (pNhtSessionData)(n->Context);
    handle_t obj_handle = xhnHandle(&(sess->Hctx), n->Obj);
    pNhtControlMsg cm;
    pNhtControlMsgParam cmp;
    XString xs;
    char* dptr;
    int type;

	/** Allocate a control message and set it up **/
	cm = (pNhtControlMsg)nmMalloc(sizeof(NhtControlMsg));
	if (!cm) return -ENOMEM;
	cm->MsgType = NHT_CONTROL_T_REPMSG;
	xaInit(&(cm->Params), 1);
	cm->ResponseSem = NULL;
	cm->ResponseFn = NULL;
	cm->Status = NHT_CONTROL_S_QUEUED;
	cm->Response = NULL;
	cm->Context = NULL;

	/** Parameter indicates what changed and how **/
	cmp = (pNhtControlMsgParam)nmMalloc(sizeof(NhtControlMsgParam));
	if (!cmp)
	    {
	    nmFree(cm, sizeof(NhtControlMsg));
	    return -ENOMEM;
	    }
	memset(cmp, 0, sizeof(NhtControlMsgParam));
	xsInit(&xs);
	xsPrintf(&xs, "X" XHN_HANDLE_PRT, obj_handle);
	cmp->P1 = nmSysStrdup(xs.String);
	xsPrintf(&xs, "http://%s/", n->Name);
	cmp->P3 = nmSysStrdup(xs.String);
	type = n->NewAttrValue.DataType;
	if (n->NewAttrValue.Flags & DATA_TF_NULL) 
	    dptr = "";
	else if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE) 
	    dptr = objDataToStringTmp(type, (void*)&n->NewAttrValue.Data, 0);
	else
	    dptr = objDataToStringTmp(type, (void*)(n->NewAttrValue.Data.String), 0);
	if (!dptr) dptr = "";
	xsCopy(&xs, "", 0);
	xsQPrintf(&xs,"%STR%STR&JSSTR",xs.String,dptr);
	printf("TEST from net_http: %s\n",xs.String);
	cmp->P2 = nmSysStrdup(xs.String);
	xaAddItem(&cm->Params, (void*)cmp);

	/** Enqueue the thing. **/
	xaAddItem(&sess->ControlMsgsList, (void*)cm);
	syPostSem(sess->ControlMsgs, 1, 0);

    return 0;
    }


/*** nht_internal_OSML_GetAttrType - get an attribute type from parameters
 *** supplied to the query
 ***/
int
nht_internal_OSML_GetAttrType(void* nhtqy_v, char* attrname)
    {
    pNhtQuery nhtqy = (pNhtQuery)nhtqy_v;
    pStruct find_inf;

	/** search through the list of params **/
	find_inf = stLookup_ne(nhtqy->ParamData, attrname);
	if (!find_inf) return -1;

	/** get data type **/
	if (!strncmp(find_inf->StrVal, "integer:", 8)) return DATA_T_INTEGER;
	if (!strncmp(find_inf->StrVal, "string:", 7)) return DATA_T_STRING;
	if (!strncmp(find_inf->StrVal, "datetime:", 9)) return DATA_T_DATETIME;
	if (!strncmp(find_inf->StrVal, "money:", 6)) return DATA_T_MONEY;
	if (!strncmp(find_inf->StrVal, "double:", 7)) return DATA_T_DOUBLE;

    return -1;
    }


/*** nht_internal_OSML_GetAttrValue - get the value of an attribute from the
 *** query parameters.
 ***/
int
nht_internal_OSML_GetAttrValue(void* nhtqy_v, char* attrname, int datatype, pObjData val)
    {
    pNhtQuery nhtqy = (pNhtQuery)nhtqy_v;
    pStruct find_inf;
    char* ptr;
    static DateTime dt;
    static MoneyType m;

	/** search through the list of params **/
	find_inf = stLookup_ne(nhtqy->ParamData, attrname);
	if (!find_inf) return -1;

	/** skip to after the colon, which separates data type from value **/
	ptr = strchr(find_inf->StrVal, ':');
	if (!ptr) return -1;
	ptr++;
	if (*ptr == 'N') // N = null, V = valid value
	    return 1;
	ptr = strchr(ptr,':');
	if (!ptr) return -1;
	ptr++;

	/** Memory for date and money types **/
	if (datatype == DATA_T_DATETIME)
	    val->DateTime = &dt;
	else if (datatype == DATA_T_MONEY)
	    val->Money = &m;

    return objDataFromString(val, datatype, ptr);
    }


/*** nht_internal_CreateQuery() - create a NhtQuery object, filled in from the
 *** request data.
 ***/
pNhtQuery
nht_internal_CreateQuery(pStruct req_inf)
    {
    pNhtQuery nht_query;
    char* ptr;

	nht_query = (pNhtQuery)nmMalloc(sizeof(NhtQuery));
	if (nht_query)
	    {
	    memset(nht_query, 0, sizeof(NhtQuery));
	    if (stAttrValue_ne(stLookup_ne(req_inf,"ls__sqlparam"),&ptr) == 0)
		{
		nht_query->ParamData = htsParseURL(ptr);
		if (nht_query->ParamData)
		    {
		    nht_query->ParamList = expCreateParamList();
		    expAddParamToList(nht_query->ParamList, "parameters", (void*)nht_query, 0);
		    expSetParamFunctions(nht_query->ParamList, "parameters", nht_internal_OSML_GetAttrType, nht_internal_OSML_GetAttrValue, NULL);
		    }
		}
	    }
    
    return nht_query;
    }


/*** nht_internal_FreeQuery() - deinit and release an NhtQuery object
 ***/
int
nht_internal_FreeQuery(pNhtQuery nht_query)
    {

	if (nht_query->ParamData) stFreeInf_ne(nht_query->ParamData);
	if (nht_query->ParamList) expFreeParamList(nht_query->ParamList);
	nmFree(nht_query, sizeof(NhtQuery));

    return 0;
    }


/*** nht_internal_OSML - direct OSML access from the client.  This will take
 *** the form of a number of different OSML operations available seemingly
 *** seamlessly (hopefully) from within the JavaScript functionality in an
 *** DHTML document.
 ***/
int
nht_internal_OSML(pNhtConn conn, pObject target_obj, char* request, pStruct req_inf)
    {
    pNhtSessionData sess = conn->NhtSession;
    char* ptr;
    char* newptr;
    pObjSession objsess;
    pObject obj = NULL;
    pObjQuery qy = NULL;
    char* sid = NULL;
    int auto_session = 0;
    char sbuf[256];
    char sbuf2[256];
    char hexbuf[3];
    int mode,mask;
    char* usrtype;
    int i,t,n,o,cnt,start,flags,len,rval;
    pStruct subinf;
    MoneyType m;
    DateTime dt;
    pDateTime pdt;
    pMoneyType pm;
    double dbl;
    char* where;
    char* orderby;
    int autoclose = 0;
    int autoclose_shortres = 0;
    int retval;		/** FIXME FIXME FIXME FIXME FIXME FIXME **/
    char* reopen_sql;
    int reopen_having;
    pXString reopen_str;
    pObject reopen_obj;
    int reopen_success;
    pNhtQuery nht_query;
    char* strval;
    XArray tail_buffer;
    int n_skipped;
    
    handle_t session_handle;
    handle_t query_handle;
    handle_t obj_handle;

	if (DEBUG_OSML) stPrint_ne(req_inf);

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
	    if (DEBUG_OSML) printf("ls__mode=opensession X" XHN_HANDLE_PRT "\n", session_handle);
	    fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
	    }
	else 
	    {
	    /** Get the session data **/
	    ptr = NULL;
	    stAttrValue_ne(stLookup_ne(req_inf,"ls__newsess"),&ptr);
	    if (ptr && !strcasecmp(ptr,"yes"))
		auto_session = 1;
	    stAttrValue_ne(stLookup_ne(req_inf,"ls__sid"),&sid);
	    if (!sid)
		{
		if (!auto_session) 
		    {
		    snprintf(sbuf,256,"Content-Type: text/html\r\n"
			     "Pragma: no-cache\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
		    fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		    mssError(1,"NHT","Session ID required for OSML request '%s'",request);
		    return -1;
		    }
		else
		    {
		    objsess = objOpenSession(req_inf->StrVal);
		    if (!objsess) 
			{
			session_handle = XHN_INVALID_HANDLE;
			snprintf(sbuf,256,"Content-Type: text/html\r\n"
				     "Pragma: no-cache\r\n"
				     "\r\n"
				     "<A HREF=/ TARGET=X" XHN_HANDLE_PRT ">&nbsp;</A>\r\n",
				session_handle);
			fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
			mssError(1,"NHT","Failed to open new OSML session");
			return -1;
			}
		    else
			{
			session_handle = xhnAllocHandle(&(sess->Hctx), objsess);
			}
		    }
		}
	    else
		{
		auto_session = 0;
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
		}

	    if (!objsess || !ISMAGIC(objsess, MGK_OBJSESSION))
		{
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
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
		    fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
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
		    fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		    mssError(1,"NHT","Invalid Query ID in OSML request");
		    return -1;
		    }
		}

	    /** Does this request require an object handle? **/
	    if (obj_handle == XHN_INVALID_HANDLE && (!strcmp(request,"close") || !strcmp(request,"objquery") ||
		!strcmp(request,"read") || !strcmp(request,"write") || !strcmp(request,"attrs") || 
		!strcmp(request, "setattrs") || !strcmp(request,"delete")))
		{
		snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
			 "\r\n"
			 "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
		fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
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
		fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
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
		    fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
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
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
	        }
	    else if (!strcmp(request,"open"))
	        {
		/** Get the info and open the object **/
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__usrtype"),&usrtype) < 0) return -1;
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__objmode"),&ptr) < 0) return -1;
		mode = strtoi(ptr,NULL,0);
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__objmask"),&ptr) < 0) return -1;
		mask = strtoi(ptr,NULL,0);
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
		if (DEBUG_OSML) printf("ls__mode=open X" XHN_HANDLE_PRT "\n", obj_handle);
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);

		if (obj && stAttrValue_ne(stLookup_ne(req_inf,"ls__notify"),&ptr) >= 0 && !strcmp(ptr,"1"))
		    objRequestNotify(obj, nht_internal_UpdateNotify, sess, OBJ_RN_F_ATTRIB);

		/** Include an attribute listing **/
		nht_internal_WriteAttrs(obj,conn,obj_handle,1);
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
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
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
		if (DEBUG_OSML) printf("ls__mode=objquery X" XHN_HANDLE_PRT "\n", query_handle);
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		}
	    else if (!strcmp(request,"queryfetch") || !strcmp(request,"multiquery"))
	        {
		fdQPrintf(conn->ConnFD,
			"Content-Type: text/html\r\n"
			"Pragma: no-cache\r\n"
			"\r\n");
		nht_query = NULL;
		if (!strcmp(request,"multiquery"))
		    {
		    if (auto_session)
			{
			fdPrintf(conn->ConnFD, "<A HREF=/ TARGET=X" XHN_HANDLE_PRT ">&nbsp;</A>\r\n", session_handle);
			}
		    qy = NULL;

		    /** check for query parameters **/
		    nht_query = nht_internal_CreateQuery(req_inf);
		    if (nht_query)
			{
			if (stAttrValue_ne(stLookup_ne(req_inf,"ls__autoclose"),&ptr) == 0 && strtol(ptr,NULL,0))
			    autoclose = 1;
			if (stAttrValue_ne(stLookup_ne(req_inf,"ls__autoclose_sr"),&ptr) == 0 && strtol(ptr,NULL,0))
			    autoclose_shortres = 1;
			if (stAttrValue_ne(stLookup_ne(req_inf,"ls__sql"),&ptr) < 0) return -1;

			/** open the query with the osml **/
			qy = nht_query->OsmlQuery = objMultiQuery(objsess, ptr, nht_query->ParamList, 0);
			if (!qy)
			    query_handle = XHN_INVALID_HANDLE;
			else
			    query_handle = xhnAllocHandle(&(sess->Hctx), qy);
			nht_query->QueryHandle = query_handle;
			}
		    else
			{
			qy = NULL;
			query_handle = XHN_INVALID_HANDLE;
			}
		    if (autoclose)
			snprintf(sbuf, sizeof(sbuf), "<A HREF=/ TARGET=X" XHN_HANDLE_PRT ">&nbsp;</A>\r\n",
			    XHN_INVALID_HANDLE);
		    else
			snprintf(sbuf, sizeof(sbuf), "<A HREF=/ TARGET=X" XHN_HANDLE_PRT ">&nbsp;</A>\r\n",
			    query_handle);
		    if (DEBUG_OSML) printf("ls__mode=multiquery X" XHN_HANDLE_PRT "\n", query_handle);
		    fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		    if (!qy && nht_query)
			{
			nht_internal_FreeQuery(nht_query);
			}
		    else
			{
			xaAddItem(&sess->OsmlQueryList, nht_query);
			}
		    }
		if (!strcmp(request,"queryfetch") || (qy && stAttrValue_ne(stLookup_ne(req_inf,"ls__autofetch"),&ptr) == 0 && strtol(ptr,NULL,0)))
		    {
		    if (!nht_query)
			{
			for(i=0;i<sess->OsmlQueryList.nItems;i++)
			    {
			    nht_query = (pNhtQuery)(sess->OsmlQueryList.Items[i]);
			    if (nht_query->QueryHandle == query_handle)
				break;
			    nht_query = NULL;
			    }
			}

		    if (stAttrValue_ne(stLookup_ne(req_inf,"ls__objmode"),&ptr) < 0) return -1;
		    mode = strtoi(ptr,NULL,0);
		    if (stAttrValue_ne(stLookup_ne(req_inf,"ls__rowcount"),&ptr) < 0)
			n = 0x7FFFFFFF;
		    else
			n = strtoi(ptr,NULL,0);
		    if (stAttrValue_ne(stLookup_ne(req_inf,"ls__startat"),&ptr) < 0)
			start = 0;
		    else
			start = strtoi(ptr,NULL,0) - 1;
		    if (start < 0) start = 0;
		    if (!strcmp(request,"queryfetch"))
			{
			snprintf(sbuf, sizeof(sbuf), "<A HREF=/ TARGET=X%8.8X>&nbsp;</A>\r\n", 0);
			fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
			}

		    /** Skip over objects at the beginning? **/
		    while(start > 0 && (obj = objQueryFetch(qy,mode)))
			{
			objClose(obj);
			start--;
			}

		    /** Skip-to-tail mode?  If so, we need to go through the results,
		     ** storing N results in a FIFO, so that when we hit the end, we
		     ** have the last N results for the query.
		     **/
		    if (stAttrValue_ne(stLookup_ne(req_inf, "ls__tail"), &ptr) == 0 && strtol(ptr,NULL,0))
			{
			xaInit(&tail_buffer, n);
			n_skipped = 0;

			/** Get the object listing **/
			while((obj = objQueryFetch(qy, mode)) != NULL)
			    {
			    xaAddItem(&tail_buffer, obj);
			    if (tail_buffer.nItems > n)
				{
				objClose((pObject)tail_buffer.Items[0]);
				xaRemoveItem(&tail_buffer, 0);
				n_skipped++;
				}
			    }

			/** Send the data to the client **/
			fdPrintf(conn->ConnFD, "<A HREF=\"/\" TARGET=\"SKIPPED\">%d</A>\r\n", n_skipped);
			for(i=0; i<tail_buffer.nItems; i++)
			    {
			    obj = (pObject)tail_buffer.Items[i];
			    if (!autoclose)
				obj_handle = xhnAllocHandle(&(sess->Hctx), obj);
			    else
				obj_handle = n;
			    if (DEBUG_OSML) printf("ls__mode=queryfetch X" XHN_HANDLE_PRT "\n", obj_handle);
			    if (stAttrValue_ne(stLookup_ne(req_inf,"ls__notify"),&ptr) >= 0 && !strcmp(ptr,"1"))
				objRequestNotify(obj, nht_internal_UpdateNotify, sess, OBJ_RN_F_ATTRIB);
			    nht_internal_WriteAttrs(obj,conn,obj_handle,1);
			    n--;
			    if (autoclose) objClose(obj);
			    }
			xaDeInit(&tail_buffer);

			/** We set n to 1 to indicate to the below code that we
			 ** reached the end of the actual result set.
			 **/
			n = 1;
			}
		    else
			{
			/** Here's the main fetch loop **/
			while(n > 0 && (obj = objQueryFetch(qy,mode)))
			    {
			    if (!autoclose)
				obj_handle = xhnAllocHandle(&(sess->Hctx), obj);
			    else
				obj_handle = n;
			    if (DEBUG_OSML) printf("ls__mode=queryfetch X" XHN_HANDLE_PRT "\n", obj_handle);
			    if (stAttrValue_ne(stLookup_ne(req_inf,"ls__notify"),&ptr) >= 0 && !strcmp(ptr,"1"))
				objRequestNotify(obj, nht_internal_UpdateNotify, sess, OBJ_RN_F_ATTRIB);
			    nht_internal_WriteAttrs(obj,conn,obj_handle,1);
			    n--;
			    if (autoclose) objClose(obj);
			    }
			}

		    /** if end of result set was reached before rowlimit ran out **/
		    if (autoclose_shortres && n > 0)
			{
			xhnFreeHandle(&(sess->Hctx), query_handle);
			for(i=0;i<sess->OsmlQueryList.nItems;i++)
			    {
			    nht_query = (pNhtQuery)(sess->OsmlQueryList.Items[i]);
			    if (nht_query->OsmlQuery == qy)
				{
				xaRemoveItem(&sess->OsmlQueryList, i);
				nht_internal_FreeQuery(nht_query);
				break;
				}
			    }
			objQueryClose(qy);
			qy = NULL;
			fdPrintf(conn->ConnFD, "<A HREF=/ TARGET=QUERYCLOSED>&nbsp;</A>\r\n");
			}
		    else if (autoclose)
			{
			xhnFreeHandle(&(sess->Hctx), query_handle);
			for(i=0;i<sess->OsmlQueryList.nItems;i++)
			    {
			    nht_query = (pNhtQuery)(sess->OsmlQueryList.Items[i]);
			    if (nht_query->OsmlQuery == qy)
				{
				xaRemoveItem(&sess->OsmlQueryList, i);
				nht_internal_FreeQuery(nht_query);
				break;
				}
			    }
			objQueryClose(qy);
			qy = NULL;
			}
		    }
		}
	    else if (!strcmp(request,"queryclose"))
	        {
		xhnFreeHandle(&(sess->Hctx), query_handle);
		for(i=0;i<sess->OsmlQueryList.nItems;i++)
		    {
		    nht_query = (pNhtQuery)(sess->OsmlQueryList.Items[i]);
		    if (nht_query->OsmlQuery == qy)
			{
			xaRemoveItem(&sess->OsmlQueryList, i);
			nht_internal_FreeQuery(nht_query);
			break;
			}
		    }
		objQueryClose(qy);
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=X%8.8X>&nbsp;</A>\r\n",
		    0);
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		}
	    else if (!strcmp(request,"read"))
	        {
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__bytecount"),&ptr) < 0)
		    n = 0x7FFFFFFF;
		else
		    n = strtoi(ptr,NULL,0);
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__offset"),&ptr) < 0)
		    o = -1;
		else
		    o = strtoi(ptr,NULL,0);
		if (stAttrValue_ne(stLookup_ne(req_inf,"ls__flags"),&ptr) < 0)
		    flags = 0;
		else
		    flags = strtoi(ptr,NULL,0);
		start = 1;
		while(n > 0 && (cnt=objRead(obj,sbuf,(256>n)?n:256,(o != -1)?o:0,(o != -1)?flags|OBJ_U_SEEK:flags)) > 0)
		    {
		    if(start)
			{
			snprintf(sbuf2,256,"Content-Type: text/html\r\n"
				 "Pragma: no-cache\r\n"
				 "\r\n"
				 "<A HREF=/ TARGET=X%8.8X>",
			    0);
			fdWrite(conn->ConnFD, sbuf2, strlen(sbuf2), 0,0);
			start = 0;
			}
		    for(i=0;i<cnt;i++)
		        {
		        sprintf(hexbuf,"%2.2X",((unsigned char*)sbuf)[i]);
			fdWrite(conn->ConnFD,hexbuf,2,0,0);
			}
		    n -= cnt;
		    o = -1;
		    }
		if(start)
		    {
		    snprintf(sbuf,256,"Content-Type: text/html\r\n"
			     "Pragma: no-cache\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=X%8.8X>",
			cnt);
		    fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		    start = 0;
		    }
		fdWrite(conn->ConnFD, "</A>\r\n", 6,0,0);
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
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		nht_internal_WriteAttrs(obj,conn,obj_handle,1);
		}
	    else if (!strcmp(request,"setattrs") || !strcmp(request,"create"))
	        {
		nht_query = NULL;

		/** First, if creating, open the new object. **/
		if (!strcmp(request,"create"))
		    {
		    len = strlen(req_inf->StrVal);
		    /* let osml determine whether or not it is autoname - it is in a much better position to do so. */
		    /*if (len < 1 || req_inf->StrVal[len-1] != '*' || (len >= 2 && req_inf->StrVal[len-2] != '/'))
			obj = objOpen(objsess, req_inf->StrVal, O_CREAT | O_RDWR, 0600, "system/object");
		    else*/
			obj = objOpen(objsess, req_inf->StrVal, OBJ_O_AUTONAME | O_CREAT | O_RDWR, 0600, "system/object");
		    if (!obj)
			{
			snprintf(sbuf,256,"Content-Type: text/html\r\n"
				 "Pragma: no-cache\r\n"
				 "\r\n"
				 "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
			fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
			mssError(0,"NHT","Could not create object");
			return -1;
			}
		    else
			{
			obj_handle = xhnAllocHandle(&(sess->Hctx), obj);
			}
		    }
		/*else
		    obj_handle = 0;*/

		/** Find all GET params that are NOT like ls__thingy **/
		for(i=0;i<req_inf->nSubInf;i++)
		    {
		    subinf = req_inf->SubInf[i];
		    if (strncmp(subinf->Name,"ls__",4) != 0 && strncmp(subinf->Name,"cx__",4) != 0)
		        {
			retval = 0;
			t = objGetAttrType(obj, subinf->Name);
			if (t < 0) continue;

			/** Handle newer set-attr-value format that allows for nulls **/
			if ((subinf->StrVal[0] == 'V' || subinf->StrVal[0] == 'N') && subinf->StrVal[1] == ':')
			    strval = subinf->StrVal+2;
			else
			    strval = subinf->StrVal; /* deprecated - transition only */

			/** set to NULL? **/
			if (subinf->StrVal[0] == 'N' && subinf->StrVal[1] == ':')
			    {
			    retval = objSetAttrValue(obj, subinf->Name, t, NULL);
			    }
			else
			    {
			    /** Set to a value **/
			    switch(t)
				{
				case DATA_T_INTEGER:
				    if (*strval)
					{
					n = objDataToInteger(DATA_T_STRING, strval, NULL);
					retval=objSetAttrValue(obj,subinf->Name,DATA_T_INTEGER,POD(&n));
					}
				    else
					retval=objSetAttrValue(obj,subinf->Name,DATA_T_INTEGER,NULL);
				    break;

				case DATA_T_DOUBLE:
				    if (*strval)
					{
					dbl = objDataToDouble(DATA_T_STRING, strval);
					retval=objSetAttrValue(obj,subinf->Name,DATA_T_DOUBLE,POD(&dbl));
					}
				    else
					retval=objSetAttrValue(obj,subinf->Name,DATA_T_DOUBLE,NULL);
				    break;

				case DATA_T_STRING:
				    retval=objSetAttrValue(obj,subinf->Name,DATA_T_STRING,POD(&strval));
				    break;

				case DATA_T_DATETIME:
				    if (*strval)
					{
					objDataToDateTime(DATA_T_STRING, strval, &dt, NULL);
					pdt = &dt;
					retval=objSetAttrValue(obj,subinf->Name,DATA_T_DATETIME,POD(&pdt));
					}
				    else
					retval=objSetAttrValue(obj,subinf->Name,DATA_T_DATETIME,NULL);
				    break;

				case DATA_T_MONEY:
				    if (*strval)
					{
					pm = &m;
					objDataToMoney(DATA_T_STRING, strval, &m);
					retval=objSetAttrValue(obj,subinf->Name,DATA_T_MONEY,POD(&pm));
					}
				    else
					retval=objSetAttrValue(obj,subinf->Name,DATA_T_MONEY,NULL);
				    break;

				case DATA_T_UNAVAILABLE: 
				    /** Attribute probably doesn't exist yet.  Until we have info
				     ** from the client on the attr type, assume it is a string or an
				     ** integer, and try to set it that way.
				     **/
				    retval=objSetAttrValue(obj,subinf->Name,DATA_T_STRING,POD(&strval));
				    if (retval < 0 && *strval && strspn(strval, "0123456789-") == strlen(strval) && strchr(strval+1, '-') == NULL)
					{
					n = objDataToInteger(DATA_T_STRING, strval, NULL);
					retval=objSetAttrValue(obj,subinf->Name,DATA_T_INTEGER,POD(&n));
					}
				    break;

				case DATA_T_STRINGVEC:
				case DATA_T_INTVEC:
				default:
				    retval = -1;
				    break;
				}
			    }
			if (retval < 0)
			    {
			    mssError(0, "NHT", "Failed to set attribute <%s> on object <%s>", subinf->Name, obj->Pathname->Pathbuf + 1);
			    snprintf(sbuf,256,"Content-Type: text/html\r\n"
				     "Pragma: no-cache\r\n"
				     "\r\n"
				     "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
			    fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
			    if (!strcmp(request, "create"))
				{
				xhnFreeHandle(&(sess->Hctx), obj_handle);
				objClose(obj);
				}
			    return -1;
			    }
			}
		    }

		/** Commit the change. **/
		rval = objCommit(objsess);
		if (rval < 0)
		    {
		    snprintf(sbuf,256,"Content-Type: text/html\r\n"
			     "Pragma: no-cache\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
		    fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
		    objClose(obj);
		    }
		else
		    {
		    /** Do the re-open if requested.  This re-runs the SQL to pick up any
		     ** joins and computed fields and such.
		     **/
		    if (stAttrValue_ne(stLookup_ne(req_inf,"ls__reopen_sql"),&reopen_sql) == 0)
			{
			reopen_success = 0;
			reopen_str = (pXString)nmMalloc(sizeof(XString));
			if (reopen_str)
			    {
			    xsInit(reopen_str);
			    nht_query = nht_internal_CreateQuery(req_inf);

			    /** note - it is possible for autoname to fail, thus name == NULL.  IT
			     ** is also possible for a poorly constructed SQL query to result in
			     ** NULL name values (for doing setattr on multiquery result set
			     ** objects).  So we must check.
			     **/
			    if (objGetAttrValue(obj, "name", DATA_T_STRING, POD(&ptr)) == 0)
				{
				reopen_having = stLookup_ne(req_inf,"ls__reopen_having")?1:0;
				xsQPrintf(reopen_str, "%STR %[WHERE%]%[HAVING%] :name = %STR&QUOT FOR UPDATE", reopen_sql, !reopen_having, reopen_having, ptr);
				qy = objMultiQuery(objsess, reopen_str->String, nht_query?nht_query->ParamList:NULL, 0);
				if (qy)
				    {
				    reopen_obj = objQueryFetch(qy, O_RDWR);
				    if (reopen_obj)
					{
					xhnUpdateHandle(&sess->Hctx, obj_handle, reopen_obj);
					objClose(obj);
					obj = reopen_obj;
					reopen_success = 1;
					}
				    else
					{
					mssError(1,"NHT","Could not retrieve updated object in re-open");
					}
				    objQueryClose(qy);
				    }
				else
				    {
				    mssError(0,"NHT","Could not run SQL query to re-open updated object");
				    }
				}
			    else
				{
				mssError(0,"NHT","Could not obtain <name> property of object in order to do the re-open");
				}

			    xsDeInit(reopen_str);
			    nmFree(reopen_str, sizeof(XString));
			    }

			/** Reopen failure during create means create failed.  During
			 ** setattrs (modify), we ignore it and go with our already 
			 ** open (not reopened) object.
			 **/
			if (!reopen_success && !strcmp(request,"create"))
			    {
			    xhnFreeHandle(&(sess->Hctx), obj_handle);
			    obj_handle = XHN_INVALID_HANDLE;
			    objClose(obj);
			    obj = NULL;
			    }
			}

		    /** Output result status **/
		    if (!strcmp(request,"create"))
			{
			if (DEBUG_OSML) printf("ls__mode=create X" XHN_HANDLE_PRT "\n", obj_handle);
			if (obj)
			    {
			    fdPrintf(conn->ConnFD,"Content-Type: text/html\r\n"
				     "Pragma: no-cache\r\n"
				     "\r\n"
				     "<A HREF=/ TARGET=X" XHN_HANDLE_PRT ">&nbsp;</A>\r\n",
				     obj_handle);
			    }
			else
			    {
			    fdPrintf(conn->ConnFD,"Content-Type: text/html\r\n"
				     "Pragma: no-cache\r\n"
				     "\r\n"
				     "<A HREF=/ TARGET=ERR>&nbsp;</A>\r\n");
			    }
			}
		    else
			{
			snprintf(sbuf,256,"Content-Type: text/html\r\n"
				 "Pragma: no-cache\r\n"
				 "\r\n"
				 "<A HREF=/ TARGET=X%8.8X>&nbsp;</A>\r\n",
				 0);
			fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
			}

		    /** Write the (possibly updated) attrs to the connection **/
		    if (obj)
			nht_internal_WriteAttrs(obj,conn,obj_handle,1);
		    }

		if (nht_query) nht_internal_FreeQuery(nht_query);
		}
	    else if (!strcmp(request,"delete"))
		{
		/** Delete an object that is currently open **/
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
			mssError(1,"NHT","Invalid object id(s) in OSML 'delete' request");
			continue;
			}
		    xhnFreeHandle(&(sess->Hctx), obj_handle);

		    /** Delete it **/
		    rval = objDeleteObj(obj);
		    if (rval < 0) break;
		    }
	        snprintf(sbuf,256,"Content-Type: text/html\r\n"
			 "Pragma: no-cache\r\n"
	    		 "\r\n"
			 "<A HREF=/ TARGET=%s>&nbsp;</A>\r\n",
		    (rval==0)?"X00000000":"ERR");
	        fdWrite(conn->ConnFD, sbuf, strlen(sbuf), 0,0);
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
	    if (strncmp(search_inf->Name,"ls__",4) && strncmp(search_inf->Name,"cx__",4))
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
		        objSetAttrValue(obj, search_inf->Name, DATA_T_INTEGER,POD(&n));
			break;

		    case DATA_T_STRING:
		        if (search_inf->StrVal != NULL)
		    	    {
		    	    ptr = search_inf->StrVal;
		    	    objSetAttrValue(obj, search_inf->Name, DATA_T_STRING,POD(&ptr));
			    }
			break;
		    
		    case DATA_T_DOUBLE:
		        /*if (search_inf->StrVal == NULL)
		            dbl = search_inf->IntVal[0];
		        else*/
		            dbl = strtod(search_inf->StrVal, NULL);
			objSetAttrValue(obj, search_inf->Name, DATA_T_DOUBLE,POD(&dbl));
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
			objSetAttrValue(obj, search_inf->Name, DATA_T_MONEY,POD(&mptr));
			break;
		
		    case DATA_T_DATETIME:
		        if (search_inf->StrVal != NULL)
			    {
			    objDataToDateTime(DATA_T_STRING, search_inf->StrVal, &dt,NULL);
			    dtptr = &dt;
			    objSetAttrValue(obj, search_inf->Name, DATA_T_DATETIME,POD(&dtptr));
			    }
			break;
		    }
		}
	    }

    return 0;
    }
