#include "net_http.h"
#include "cxss/cxss.h"
#include "application.h"

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

 
 /*** nht_i_AllocConn() - allocates a connection structure and
 *** initializes it given a network connection.
 ***/
pNhtConn
nht_i_AllocConn(pFile net_conn)
    {
    pNhtConn conn;
    char* remoteip;

	/** Allocate and zero-out the structure **/
	conn = (pNhtConn)nmMalloc(sizeof(NhtConn));
	if (!conn) return NULL;
	memset(conn, 0, sizeof(NhtConn));
	conn->ConnFD = net_conn;
	conn->Size = -1;
	conn->LastHandle = XHN_INVALID_HANDLE;
	conn->StrictSameSite = 1;
	xaInit(&conn->RequestHeaders, 16);
	xaInit(&conn->ResponseHeaders, 16);
	strtcpy(conn->ResponseContentType, "text/html", sizeof(conn->ResponseContentType));

	/** Get the remote IP and port **/
	remoteip = netGetRemoteIP(net_conn, NET_U_NOBLOCK);
	if (remoteip) strtcpy(conn->IPAddr, remoteip, sizeof(conn->IPAddr));
	conn->Port = netGetRemotePort(net_conn);

    return conn;
    }


/*** nht_i_FreeConn() - releases a connection structure and 
 *** closes the associated network connection.
 ***/
int
nht_i_FreeConn(pNhtConn conn)
    {

	/** Issue final chunk in chunked encoding **/
	if (conn->UsingChunkedEncoding)
	    fdPrintf(conn->ConnFD, "0\r\n\r\n");

	/** Log the connection **/
	nht_i_Log(conn);

	/** Close the connection **/
	if (conn->SSLpid > 0 && conn->UsingTLS)
	    cxssFinishTLS(conn->SSLpid, conn->ConnFD, conn->ReportingFD);
	else
	    netCloseTCP(conn->ConnFD, 1000, 0);

	/** Deallocate the structure **/
	if (conn->Referrer) nmSysFree(conn->Referrer);
	if (conn->URL) nmSysFree(conn->URL);
	if (conn->ReqURL) stFreeInf_ne(conn->ReqURL);

	/** Unlink from the app/group/session. **/
	if (conn->App) nht_i_UnlinkApp(conn->App);
	if (conn->AppGroup) nht_i_UnlinkAppGroup(conn->AppGroup);
	if (conn->NhtSession) nht_i_UnlinkSess(conn->NhtSession);

	/** Release the connection structure **/
	nht_i_FreeHeaders(&conn->RequestHeaders);
	nht_i_FreeHeaders(&conn->ResponseHeaders);
	xaDeInit(&conn->RequestHeaders);
	xaDeInit(&conn->ResponseHeaders);
	nmFree(conn, sizeof(NhtConn));

    return 0;
    }


/*** nht_i_Log - generate an access logfile entry
 ***/
int
nht_i_Log(pNhtConn conn)
    {
    char method[32];
    int i;

	/** logging not available? **/
	if (!NHT.AccessLogFD)
	    return 0;

	/** Upcase the method **/
	strtcpy(method, conn->Method, sizeof(method));
	for(i=0;i<strlen(method);i++)
	    method[i] = toupper(method[i]);

	/** Print the log message, using the typical "combined" log format **/
	fdQPrintf(NHT.AccessLogFD, 
		"%STR %STR %STR [%STR] \"%STR&ESCQ %STR&ESCQ %STR&ESCQ\" %INT %INT \"%STR&ESCQ\" \"%STR&ESCQ\"\n",
		conn->IPAddr,
		"-",
		(conn->NhtSession && conn->NhtSession->Username[0])?conn->NhtSession->Username:"-",
		conn->ResponseTime,
		method,
		conn->ReqURL?(conn->ReqURL->StrVal):"",
		conn->HTTPVer,
		conn->ResponseCode,
		conn->BytesWritten,
		conn->Referrer?conn->Referrer:"-",
		conn->UserAgent[0]?conn->UserAgent:"-");


    return 0;
    }


/*** nht_i_ConnHandler - manages a single incoming HTTP connection
 *** and processes the connection's request.
 ***/
void
nht_i_ConnHandler(void* conn_v)
    {
    char sbuf[160];
    char* msg = "";
    char* ptr;
    char* nextptr;
    char* usrname;
    char* passwd = NULL;
    pStruct url_inf = NULL;
    pStruct find_inf,akey_inf;
    handle_t w_timer = XHN_INVALID_HANDLE, i_timer = XHN_INVALID_HANDLE;
    pNhtConn conn = (pNhtConn)conn_v;
    unsigned char t_lsb;
    int err;
    time_t t;
    struct tm* timeptr;
    char timestr[80];
    pNhtApp app;
    pNhtAppGroup group;
    int context_started = 0;
    pApplication tmp_app = NULL;
    unsigned char* keydata;
    char* nonce;
    unsigned char noncelen;
    int cnt, i;
    char hexval[17] = "0123456789abcdef";

	/** Set the thread's name **/
	thSetName(NULL,"HTTP Connection Handler");

	/** Ignore SIGPIPE events from end-user **/
	thSetFlags(NULL, THR_F_IGNPIPE);

	/** Restrict access to connections from localhost only? **/
	if (NHT.RestrictToLocalhost && strcmp(conn->IPAddr, "127.0.0.1") != 0)
	    {
	    msg = "Connections currently restricted via accept_localhost_only in centrallix.conf";
	    goto error;
	    }

	/** Parse the HTTP Headers... **/
	if (nht_i_ParseHeaders(conn) < 0)
	    {
	    if (conn->ReportingFD != NULL && cxssStatTLS(conn->ReportingFD, sbuf, sizeof(sbuf)) >= 0)
		msg = sbuf;
	    else
		msg = "Error parsing headers";
	    goto error;
	    }

	/** Compute header nonce.  This is used for functionally nothing, but
	 ** it causes the content and offsets to values in the header to change
	 ** with each response; this can help frustrate certain types of 
	 ** cryptographic attacks.
	 **/
	if (conn->UsingTLS && NHT.NonceData)
	    {
	    keydata = nmSysMalloc(128+8+1);
	    nonce = nmSysMalloc(256+16+1);
	    cxssKeystreamGenerate(NHT.NonceData, &noncelen, 1);
	    cnt = noncelen;
	    cnt += 16;
	    cxssKeystreamGenerate(NHT.NonceData, keydata, cnt / 2 + 1);
	    for(i=0;i<cnt;i++)
		{
		if (i & 1)
		    nonce[i] = hexval[(keydata[i/2] & 0xf0) >> 4];
		else
		    nonce[i] = hexval[keydata[i/2] & 0x0f];
		}
	    nonce[cnt] = '\0';
	    nht_i_AddResponseHeader(conn, "X-Nonce", nonce, 1);
	    nmSysFree(keydata);
	    }

	/** Add some entropy to the pool - just the LSB of the time **/
	t_lsb = mtRealTicks() & 0xFF;
	cxssAddEntropy(&t_lsb, 1, 4);

	/** Did client send authentication? **/
	if (!*(conn->Auth))
	    {
	    nht_i_AddResponseHeaderQPrintf(conn, "WWW-Authenticate", "Basic realm=%STR&DQUOT", NHT.Realm);
	    nht_i_WriteErrResponse(conn, 401, "Unauthorized", "<h1>Unauthorized</h1>\r\n");
	    goto out;
	    }

	/** Got authentication.  Parse the auth string. **/
	usrname = strtok(conn->Auth,":");
	if (usrname) passwd = strtok(NULL,"\r\n");
	if (usrname && !passwd) passwd = "";
	if (!usrname || !passwd) 
	    {
	    nht_i_WriteErrResponse(conn, 401, "Unauthorized", "<h1>401 Unauthorized</h1>\r\n");
	    goto out;
	    }

	/** Check for a cookie -- if one, try to resume a session. **/
	if (*(conn->Cookie))
	    {
	    if (conn->Cookie[strlen(conn->Cookie)-1] == ';') conn->Cookie[strlen(conn->Cookie)-1] = '\0';
	    conn->NhtSession = (pNhtSessionData)xhLookup(&(NHT.CookieSessions), conn->Cookie);
	    if (conn->NhtSession)
	        {
		nht_i_LinkSess(conn->NhtSession);
		if (strcmp(conn->NhtSession->Username,usrname) || strcmp(passwd,conn->NhtSession->Password))
		    {
		    nht_i_AddResponseHeaderQPrintf(conn, "WWW-Authenticate", "Basic realm=%STR&DQUOT", NHT.Realm);
		    nht_i_WriteErrResponse(conn, 401, "Unauthorized", "<h1>Unauthorized</h1>\r\n");
		    mssError(1,"NHT","Bark! User supplied valid cookie %s but cred mismatch (sesslink %d, provided %s, stored %s)", conn->Cookie, conn->NhtSession->LinkCnt, usrname, conn->NhtSession->Username);
		    goto out;
		    }
		if (conn->NhtSession->Session)
		    {
		    thSetParam(NULL,"mss",conn->NhtSession->Session);
		    thSetParamFunctions(NULL,mssLinkSession,mssUnlinkSession);
		    mssLinkSession(conn->NhtSession->Session);
		    }
		thSetSecContext(NULL, &(conn->NhtSession->SecurityContext));
		w_timer = conn->NhtSession->WatchdogTimer;
		i_timer = conn->NhtSession->InactivityTimer;
		}
	    else
		{
		if (strncmp(conn->Cookie, (conn->UsingTLS)?NHT.TlsSessionCookie:NHT.SessionCookie, strlen((conn->UsingTLS)?NHT.TlsSessionCookie:NHT.SessionCookie)) == 0 && conn->Cookie[strlen((conn->UsingTLS)?NHT.TlsSessionCookie:NHT.SessionCookie)] == '=' && strspn(strchr(conn->Cookie, '=') + 1, "abcdef0123456789") == 32)
		    {
		    /** Valid Authorization header but cookie expired.  Force re-login. **/
		    nht_i_AddResponseHeaderQPrintf(conn, "WWW-Authenticate", "Basic realm=%STR&DQUOT", NHT.Realm);
		    nht_i_AddResponseHeaderQPrintf(conn, "Set-Cookie", "%STR=%STR&32LEN; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT", (conn->UsingTLS)?NHT.TlsSessionCookie:NHT.SessionCookie, strchr(conn->Cookie, '=') + 1);
		    nht_i_WriteErrResponse(conn, 401, "Unauthorized", "<h1>Unauthorized</h1>\r\n");
		    goto out;
		    }
		}
	    }
	else
	    {
	    conn->NhtSession = NULL;
	    }

	/** Parse out the requested url **/
	/*printf("debug: %s\n",urlptr);*/
	url_inf = conn->ReqURL = htsParseURL(conn->URL);
	if (!url_inf)
	    {
	    nht_i_WriteErrResponse(conn, 500, "Internal Server Error", "<h1>500 Internal Server Error</h1>\r\n");
	    mssError(1,"NHT","Failed to handle HTTP request, exiting thread (could not parse URL).");
	    goto out;
	    }

	/** If there is a date code, strip the code off **/
	ptr = strstr(url_inf->StrVal, "/CXDC:");
	if (ptr)
	    {
	    nextptr = strchr(ptr+6, '/');
	    if (nextptr && strspn(ptr+6, "0123456789") == (nextptr - (ptr+6)))
		{
		memmove(ptr, nextptr, strlen(nextptr)+1);
		}
	    }

	/** Fixup the path by re-adding the needed path parameters **/
	nht_i_ConstructPathname(url_inf);

	/** Watchdog ping? **/
	conn->NotActivity = 0;
	if ((find_inf=stLookup_ne(url_inf,"cx__noact")))
	    {
	    if (strtol(find_inf->StrVal,NULL,0) != 0)
		conn->NotActivity = 1;
	    }
	if (!strcmp(url_inf->StrVal,"/INTERNAL/ping"))
	    {
	    if (conn->NhtSession)
		{
		/** Reset only the watchdog timer on a ping. **/
		if (nht_i_ResetWatchdog(w_timer))
		    {
		    conn->NoCache = 1;
		    nht_i_WriteResponse(conn, 200, "OK", "<A HREF=/ TARGET=ERR></A>\r\n");
		    goto out;
		    }
		else
		    {
		    /** Update watchdogs on app and group, if specified **/
		    find_inf = stLookup_ne(url_inf,"cx__akey");
		    if (find_inf)
			{
			if (nht_i_VerifyAKey(find_inf->StrVal, conn->NhtSession, &group, &app) == 0)
			    {
			    /** session key matched... now update app and group **/
			    if (app)
				{
				conn->App = nht_i_LinkApp(app);
				nht_i_ResetWatchdog(app->WatchdogTimer);
				}
			    if (group)
				{
				conn->AppGroup = nht_i_LinkAppGroup(group);
				nht_i_ResetWatchdog(group->WatchdogTimer);
				}
			    }
			}

		    /** Report current time to client as a part of ping response **/
		    t = time(NULL);
		    timeptr = localtime(&t);
		    if (timeptr)
			{
			/** This isn't 100% ideal -- it causes a couple seconds of clock drift **/
			if (strftime(timestr, sizeof(timestr), "%Y %m %d %T", timeptr) <= 0)
			    {
			    strcpy(timestr, "OK");
			    }
			}
		    nht_i_WriteResponse(conn, 200, "OK", NULL);
		    nht_i_QPrintfConn(conn, 0, "<A HREF=/ TARGET='%STR&HTE'></A>\r\n", timestr);
		    goto out;
		    }
		}
	    else
		{
		/** No session and this is a watchdog ping?  If so, we don't
		 ** want to automatically re-login the user since that defeats the purpose
		 ** of session timeouts.
		 **/
		conn->NoCache = 1;
		nht_i_WriteResponse(conn, 200, "OK", "<A HREF=/ TARGET=ERR></A>\r\n");
		goto out;
		}
	    }
	else if (conn->NhtSession)
	    {
	    /** Reset the idle and watchdog (disconnect) timers on a normal request
	     ** however if cx__noact is set, only reset the watchdog timer.
	     **/
	    err = 0;
	    if (!conn->NotActivity && err == 0)
		err = nht_i_ResetWatchdog(i_timer);
	    if (err == 0)
		err = nht_i_ResetWatchdog(w_timer);
	    if (err < 0)
		{
		conn->NoCache = 1;
		nht_i_WriteResponse(conn, 200, "OK", "<A HREF=/ TARGET=ERR></A>\r\n");
		goto out;
		}
	    }

	/** No cookie or no session for the given cookie? **/
	if (!conn->NhtSession)
	    {
	    /** No session, and the connection is a 'non-activity' request? **/
	    if (conn->NotActivity)
		{
		conn->NoCache = 1;
		nht_i_WriteResponse(conn, 200, "OK", "<A HREF=/ TARGET=ERR></A>\r\n");
		goto out;
		}

	    /** Attempt authentication **/
	    if (mssAuthenticate(usrname, passwd) < 0)
	        {
		nht_i_AddResponseHeaderQPrintf(conn, "WWW-Authenticate", "Basic realm=%STR&DQUOT", NHT.Realm);
		nht_i_WriteErrResponse(conn, 401, "Unauthorized", "<h1>Unauthorized</h1>\r\n");
		goto out;
		}

	    /** Authentication succeeded - start a new session **/
	    conn->NhtSession = nht_i_AllocSession(usrname, conn->UsingTLS);
	    printf("NHT: new session for username [%s], cookie [%s]\n", conn->NhtSession->Username, conn->NhtSession->Cookie);
	    nht_i_LinkSess(conn->NhtSession);
	    }

	/** Start the application security context **/
	akey_inf = stLookup_ne(url_inf,"cx__akey");
	if (conn->NhtSession && conn->NhtSession->Session)
	    {
	    cxssPushContext();
	    context_started = 1;

	    /** If a valid akey was specified, resume the application **/
	    if (akey_inf && nht_i_VerifyAKey(akey_inf->StrVal, conn->NhtSession, &group, &app) == 0 && group && app)
		{
		appResume(app->Application);
		if (!conn->App)
		    conn->App = nht_i_LinkApp(app);
		if (!conn->AppGroup)
		    conn->AppGroup = nht_i_LinkAppGroup(group);
		}
	    else
		{
		tmp_app = appCreate(NULL);
		}
	    }
	else
	    {
	    tmp_app = appCreate(NULL);
	    }

	/** Bump last activity dates. **/
	if (!conn->NotActivity)
	    {
	    objCurrentDate(&(conn->NhtSession->User->LastActivity));
	    objCurrentDate(&(conn->NhtSession->LastActivity));
	    }

	/** Update most-recent-ipaddr in session **/
	strtcpy(conn->NhtSession->LastIPAddr, conn->IPAddr, sizeof(conn->NhtSession->LastIPAddr));

	/** Set nht session http ver **/
	strtcpy(conn->NhtSession->HTTPVer, conn->HTTPVer, sizeof(conn->NhtSession->HTTPVer));

	/** Version compatibility **/
	if (!strcmp(conn->NhtSession->HTTPVer, "HTTP/1.0"))
	    {
	    conn->NhtSession->ver_10 = 1;
	    conn->NhtSession->ver_11 = 0;
	    }
	else if (!strcmp(conn->NhtSession->HTTPVer, "HTTP/1.1"))
	    {
	    conn->NhtSession->ver_10 = 1;
	    conn->NhtSession->ver_11 = 1;
	    }
	else
	    {
	    conn->NhtSession->ver_10 = 0;
	    conn->NhtSession->ver_11 = 0;
	    }

	/** Set the session's UserAgent if one was found in the headers. **/
	if (conn->UserAgent[0])
	    {
	    mssSetParam("User-Agent", conn->UserAgent);
	    }

	/** Set the session's AcceptEncoding if one was found in the headers. **/
	/*if (acceptencoding)
	    {
	    if (*acceptencoding) mssSetParam("Accept-Encoding", acceptencoding);
	    nmFree(acceptencoding, 160);
	    }*/

	/** Need to start an available connection completion trigger on this? **/
	/*if ((find_inf=stLookup_ne(url_inf,"ls__triggerid")))
	    {
	    tid = strtoi(find_inf->StrVal,NULL,0);
	    nht_i_StartTrigger(conn->NhtSession, tid);
	    }*/

	/** If the method was GET and an ls__method was specified, use that method **/
	if (!strcmp(conn->Method,"get") && (find_inf=stLookup_ne(url_inf,"ls__method")))
	    {
	    if (!akey_inf || strncmp(akey_inf->StrVal, conn->NhtSession->SKey, strlen(conn->NhtSession->SKey)))
		{
		nht_i_WriteResponse(conn, 200, "OK", "<A HREF=/ TARGET=ERR></A>\r\n");
		goto out;
		}
	    if (!strcasecmp(find_inf->StrVal,"get"))
	        {
	        nht_i_GET(conn,url_inf,conn->IfModifiedSince);
		}
	    else if (!strcasecmp(find_inf->StrVal,"copy"))
	        {
		find_inf = stLookup_ne(url_inf,"ls__destination");
		if (!find_inf || !(find_inf->StrVal))
		    {
		    nht_i_WriteErrResponse(conn, 400, "Method Error", "<H1>400 Method Error - include ls__destination for copy</H1>\r\n");
		    }
		else
		    {
		    ptr = find_inf->StrVal;
		    nht_i_COPY(conn,url_inf, ptr);
		    }
		}
	    else if (!strcasecmp(find_inf->StrVal,"put"))
	        {
		find_inf = stLookup_ne(url_inf,"ls__content");
		if (!find_inf || !(find_inf->StrVal))
		    {
		    nht_i_WriteErrResponse(conn, 400, "Method Error", "<H1>400 Method Error - include ls__content for put</H1>\r\n");
		    }
		else
		    {
		    ptr = find_inf->StrVal;
		    conn->Size = strlen(ptr);
	            nht_i_PUT(conn,url_inf,conn->Size,ptr);
		    }
		}
	    else if (!strcasecmp(find_inf->StrVal,"post"))
	        {
		find_inf = stLookup_ne(url_inf,"cx__content");
		if (!find_inf || !(find_inf->StrVal))
		    {
		    nht_i_WriteErrResponse(conn, 400, "Method Error", "<H1>400 Method Error - include cx__content for POST</H1>\r\n");
		    }
		else
		    {
		    ptr = find_inf->StrVal;
		    conn->Size = strlen(ptr);
	            nht_i_POST(conn, url_inf, conn->Size, ptr);
		    }
		}
	    else if (!strcasecmp(find_inf->StrVal,"patch"))
	        {
		find_inf = stLookup_ne(url_inf,"cx__content");
		if (!find_inf || !(find_inf->StrVal))
		    {
		    nht_i_WriteErrResponse(conn, 400, "Method Error", "<H1>400 Method Error - include cx__content for PATCH</H1>\r\n");
		    }
		else
		    {
		    ptr = find_inf->StrVal;
		    conn->Size = strlen(ptr);
	            nht_i_PATCH(conn, url_inf, ptr);
		    }
		}
	    else if (!strcasecmp(find_inf->StrVal,"delete"))
		{
		nht_i_DELETE(conn, url_inf);
		}
	    }
	else
	    {
	    /** Which method was used? **/
	    if (!strcmp(conn->Method,"get"))
	        {
	        nht_i_GET(conn,url_inf,conn->IfModifiedSince);
	        }
	    else if (!strcmp(conn->Method,"put"))
	        {
	        nht_i_PUT(conn,url_inf,conn->Size,NULL);
	        }
	    else if (!strcmp(conn->Method,"post"))
		{
		nht_i_POST(conn, url_inf, conn->Size, NULL);
		}
	    else if (!strcmp(conn->Method,"patch"))
		{
		nht_i_PATCH(conn, url_inf, NULL);
		}
	    else if (!strcmp(conn->Method,"copy"))
	        {
	        nht_i_COPY(conn,url_inf, conn->Destination);
	        }
	    else if (!strcmp(conn->Method,"delete"))
		{
		nht_i_DELETE(conn,url_inf);
		}
	    else
	        {
		nht_i_WriteErrResponse(conn, 501, "Not Implemented", "<h1>501 Method Not Implemented</h1>\r\n");
		}
	    }

	/** Catch-all, since we relocated the default response calls **/
	if (!conn->InBody)
	    {
	    nht_i_WriteErrResponse(conn, 500, "Internal Server Error", "<h1>500 Internal Server Error</h1>\r\n");
	    }

	/** End a trigger? **/
	/*if (tid != -1) nht_i_EndTrigger(conn->NhtSession,tid);*/

	/** Close and exit. **/
	/*if (url_inf) stFreeInf_ne(url_inf);*/
	nht_i_FreeConn(conn);
	conn = NULL;
	if (context_started) cxssPopContext();
	thExit();

    error:
	mssError(1,"NHT","Failed to handle HTTP request, exiting thread (%s).",msg);
	nht_i_WriteErrResponse(conn, 400, "Request Error", NULL);
	nht_i_QPrintfConn(conn, 0, "%STR\r\n", msg);

    out:
	if (url_inf && !conn) stFreeInf_ne(url_inf);
	if (conn) nht_i_FreeConn(conn);
	if (tmp_app) appDestroy(tmp_app);
	if (context_started) cxssPopContext();
	thExit();
    }


/*** nht_i_TLSHandler - manages incoming TLS-encrypted HTTP
 *** connections.
 ***/
void
nht_i_TLSHandler(void* v)
    {
    pFile listen_socket;
    pFile connection_socket;
    pStructInf my_config;
    char listen_port[32];
    char* strval;
    int intval;
    char* ptr;
    pNhtConn conn;
    FILE* fp;
    X509* cert;
    int sslflags;

	/** Set the thread's name **/
	thSetName(NULL,"HTTPS Network Listener");

	/** Get our configuration **/
	strcpy(listen_port,"843");
	my_config = stLookup(CxGlobals.ParsedConfig, "net_http");
	if (my_config)
	    {
	    /** Got the config.  Now lookup what the TCP port is that we listen on **/
	    strval=NULL;
	    if (stAttrValue(stLookup(my_config, "ssl_listen_port"), &intval, &strval, 0) >= 0)
		{
		if (strval)
		    strtcpy(listen_port, strval, sizeof(listen_port));
		else
		    snprintf(listen_port,32,"%d",intval);
		}
	    }

	/** Set up OpenSSL **/
	NHT.SSL_ctx = SSL_CTX_new(SSLv23_server_method());
	if (!NHT.SSL_ctx)
	    {
	    mssError(1,"NHT","Could not initialize SSL library");
	    thExit();
	    }

	/** Determine flags to use with OpenSSL.  We opt for a more secure
	 ** conservative configuration first, but the admin can override
	 ** our choices if they insist.
	 **/
	sslflags = SSL_OP_NO_SSLv2 | SSL_OP_SINGLE_DH_USE | SSL_OP_ALL;
#ifdef SSL_OP_CIPHER_SERVER_PREFERENCE
	sslflags |= SSL_OP_CIPHER_SERVER_PREFERENCE;
	if (stAttrValue(stLookup(my_config, "ssl_enable_client_cipherpref"), &intval, NULL, 0) == 0 && intval != 0)
	    sslflags &= ~SSL_OP_CIPHER_SERVER_PREFERENCE;
#else
	printf("CX: Warning: SSL server cipher preference option not available.\n");
#endif
#ifdef SSL_OP_NO_COMPRESSION
	sslflags |= SSL_OP_NO_COMPRESSION;
	if (stAttrValue(stLookup(my_config, "ssl_enable_compression"), &intval, NULL, 0) == 0 && intval != 0)
	    sslflags &= ~SSL_OP_NO_COMPRESSION;
#else
	printf("CX: Warning: SSL compression disable option not available.\n");
#endif
#ifdef SSL_OP_NO_SSLv3
	sslflags |= SSL_OP_NO_SSLv3;
	if (stAttrValue(stLookup(my_config, "ssl_enable_sslv3"), &intval, NULL, 0) == 0 && intval != 0)
	    sslflags &= ~SSL_OP_NO_SSLv3;
#else
	printf("CX: Warning: SSLv3 disable option not available.\n");
#endif
	if (stAttrValue(stLookup(my_config, "ssl_enable_sslv2"), &intval, NULL, 0) == 0 && intval != 0)
	    sslflags &= ~SSL_OP_NO_SSLv2;
	SSL_CTX_set_options(NHT.SSL_ctx, sslflags);

	/** Cipher list -- check at both top level and at net_http module level **/
	if (stAttrValue(stLookup(CxGlobals.ParsedConfig, "ssl_cipherlist"),NULL,&ptr,0) < 0)
	    ptr="DEFAULT";
	SSL_CTX_set_cipher_list(NHT.SSL_ctx, ptr);
	if (stAttrValue(stLookup(my_config, "ssl_cipherlist"),NULL,&ptr,0) == 0)
	    SSL_CTX_set_cipher_list(NHT.SSL_ctx, ptr);

	/** set the server certificate and key **/
	if (stAttrValue(stLookup(my_config,"ssl_cert"), NULL, &strval, 0) != 0)
	    strval = "/usr/local/etc/centrallix/snakeoil.crt";
	if (SSL_CTX_use_certificate_file(NHT.SSL_ctx, strval, SSL_FILETYPE_PEM) != 1)
	    {
	    mssError(1,"HTTP","Could not load certificate %s.", strval);
	    thExit();
	    }
	if (stAttrValue(stLookup(my_config,"ssl_key"), NULL, &strval, 0) != 0)
	    strval = "/usr/local/etc/centrallix/snakeoil.key";
	if (SSL_CTX_use_PrivateKey_file(NHT.SSL_ctx, strval, SSL_FILETYPE_PEM) != 1)
	    {
	    mssError(1,"HTTP","Could not load key %s.", strval);
	    thExit();
	    }
	if (SSL_CTX_check_private_key(NHT.SSL_ctx) != 1)
	    {
	    mssError(1,"HTTP", "Integrity check failed for key; connection handshake might not succeed.");
	    }
	if (stAttrValue(stLookup(my_config,"ssl_cert_chain"), NULL, &strval, 0) == 0)
	    {
	    /** Load certificate chain also **/
	    fp = fopen(strval, "r");
	    if (fp)
		{
		while ((cert = PEM_read_X509(fp, NULL, NULL, NULL)) != NULL)
		    {
		    /** Got one; let's add it **/
		    if (!SSL_CTX_add_extra_chain_cert(NHT.SSL_ctx, cert))
			X509_free(cert);
		    }
		fclose(fp);
		}
	    else
		{
		mssErrorErrno(1, "HTTP", "Warning: could not open certificate chain file.");
		}
	    }

    	/** Open the server listener socket. **/
	listen_socket = netListenTCP(listen_port, 32, 0);
	if (!listen_socket) 
	    {
	    mssErrorErrno(1,"NHT","Could not open TLS network listener");
	    thExit();
	    }
	
	/** Loop, accepting requests **/
	while((connection_socket = netAcceptTCP(listen_socket,0)))
	    {
	    if (!connection_socket)
		{
		thSleep(10);
		continue;
		}

	    //cxDebugLog("new TLS connection");

	    /** Check reopen **/
	    nht_i_CheckAccessLog();

	    /** Set up the connection structure **/
	    conn = nht_i_AllocConn(connection_socket);
	    if (!conn)
		{
		netCloseTCP(connection_socket, 1000, 0);
		thSleep(1);
		continue;
		}
	    conn->UsingTLS = 1;

	    /** Start TLS on the connection.  This replaces conn->ConnFD with
	     ** a pipe to the TLS encryption/decryption process.
	     **/
	    conn->SSLpid = cxssStartTLS(NHT.SSL_ctx, &conn->ConnFD, &conn->ReportingFD, 1, NULL);
	    if (conn->SSLpid <= 0)
		{
		nht_i_FreeConn(conn);
		mssError(1,"NHT","Could not start TLS on the connection!");
		continue;
		}

	    /** Start the request handler thread **/
	    if (!thCreate(nht_i_ConnHandler, 0, conn))
	        {
		nht_i_FreeConn(conn);
		mssError(1,"NHT","Could not create thread to handle TLS connection!");
		}
	    }

	/** Exit. **/
	mssError(1,"NHT","Could not continue to accept TLS requests.");
	netCloseTCP(listen_socket,0,0);

    thExit();
    }


/*** nht_i_Handler - manages incoming HTTP connections and sends
 *** the appropriate documents/etc to the requesting client.
 ***/
void
nht_i_Handler(void* v)
    {
    pFile listen_socket;
    pFile connection_socket;
    pStructInf my_config;
    char listen_port[32];
    char* strval;
    int intval;
    pNhtConn conn;

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
		    strtcpy(listen_port, strval, sizeof(listen_port));
		else
		    snprintf(listen_port,32,"%d",intval);
		}
	    }

    	/** Open the server listener socket. **/
	listen_socket = netListenTCP(listen_port, 32, 0);
	if (!listen_socket) 
	    {
	    mssErrorErrno(1,"NHT","Could not open network listener");
	    thExit();
	    }
	
	/** Loop, accepting requests **/
	while((connection_socket = netAcceptTCP(listen_socket,0)))
	    {
	    if (!connection_socket)
		{
		thSleep(10);
		continue;
		}

	    //cxDebugLog("new HTTP connection");

	    /** Check reopen **/
	    nht_i_CheckAccessLog();

	    /** Set up the connection structure **/
	    conn = nht_i_AllocConn(connection_socket);
	    if (!conn)
		{
		netCloseTCP(connection_socket, 1000, 0);
		thSleep(1);
		continue;
		}

	    /** Start the request handler thread **/
	    if (!thCreate(nht_i_ConnHandler, 0, conn))
	        {
		nht_i_FreeConn(conn);
		mssError(1,"NHT","Could not create thread to handle connection!");
		}
	    }

	/** Exit. **/
	mssError(1,"NHT","Could not continue to accept requests.");
	netCloseTCP(listen_socket,0,0);

    thExit();
    }

