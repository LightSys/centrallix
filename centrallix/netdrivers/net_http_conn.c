#include "net_http.h"
#include "cxss/cxss.h"

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

 
 /*** nht_internal_AllocConn() - allocates a connection structure and
 *** initializes it given a network connection.
 ***/
pNhtConn
nht_internal_AllocConn(pFile net_conn)
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

	/** Get the remote IP and port **/
	remoteip = netGetRemoteIP(net_conn, NET_U_NOBLOCK);
	if (remoteip) strtcpy(conn->IPAddr, remoteip, sizeof(conn->IPAddr));
	conn->Port = netGetRemotePort(net_conn);

    return conn;
    }


/*** nht_internal_FreeConn() - releases a connection structure and 
 *** closes the associated network connection.
 ***/
int
nht_internal_FreeConn(pNhtConn conn)
    {

	/** Close the connection **/
	if (conn->SSLpid)
	    cxssFinishTLS(conn->SSLpid, conn->ConnFD, conn->ReportingFD);
	else
	    netCloseTCP(conn->ConnFD, 1000, 0);

	/** Deallocate the structure **/
	if (conn->Referrer) nmSysFree(conn->Referrer);
	if (conn->URL) nmSysFree(conn->URL);

	/** Unlink from the session. **/
	if (conn->NhtSession) nht_internal_UnlinkSess(conn->NhtSession);

	/** Release the connection structure **/
	nmFree(conn, sizeof(NhtConn));

    return 0;
    }


/*** nht_internal_WriteConn() - write data to a network connection
 ***/
int
nht_internal_WriteConn(pNhtConn conn, char* buf, int len, int is_hdr)
    {
    int wcnt;

	if (len == -1) len = strlen(buf);

	if (!is_hdr && !conn->InBody)
	    {
	    conn->InBody = 1;
	    fdWrite(conn->ConnFD, "\r\n", 2, 0, FD_U_PACKET);
	    }

	wcnt = fdWrite(conn->ConnFD, buf, len, 0, FD_U_PACKET);
	if (wcnt > 0 && !is_hdr)
	    conn->BytesWritten += wcnt;

    return wcnt;
    }


/*** nht_internal_PrintfConn() - write data to the network connection,
 *** formatted.
 ***/
int
nht_internal_QPrintfConn(pNhtConn conn, int is_hdr, char* fmt, ...)
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

/*** nht_internal_Log - generate an access logfile entry
 ***/
int
nht_internal_Log(pNhtConn conn)
    {
    /*pStruct req_inf = conn->ReqURL;*/

	/** logging not available? **/
	if (!NHT.AccessLogFD)
	    return 0;

	/** Print the log message **/
	fdQPrintf(NHT.AccessLogFD, 
		"%STR %STR %STR [%STR] \"%STR&ESCQ\" %INT %INT \"%STR&ESCQ\" \"%STR&ESCQ\"\n",
		conn->IPAddr,
		"-",
		conn->Username,
		"",
		conn->ReqURL?(conn->ReqURL->StrVal):"",
		conn->ResultCode,
		conn->BytesWritten,
		conn->Referrer?conn->Referrer:"",
		conn->UserAgent?conn->UserAgent:"");


    return 0;
    }


/*** nht_internal_ConnHandler - manages a single incoming HTTP connection
 *** and processes the connection's request.
 ***/
void
nht_internal_ConnHandler(void* conn_v)
    {
    char sbuf[160];
    char* msg = "";
    char* ptr;
    char* nextptr;
    char* usrname;
    char* passwd = NULL;
    pStruct url_inf = NULL;
    pStruct find_inf,akey_inf;
    int tid = -1;
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
	if (nht_internal_ParseHeaders(conn) < 0)
	    {
	    if (conn->ReportingFD != NULL && cxssStatTLS(conn->ReportingFD, sbuf, sizeof(sbuf)) >= 0)
		msg = sbuf;
	    else
		msg = "Error parsing headers";
	    goto error;
	    }

	/** Add some entropy to the pool - just the LSB of the time **/
	t_lsb = mtRealTicks() & 0xFF;
	cxssAddEntropy(&t_lsb, 1, 4);

	/** Did client send authentication? **/
	if (!*(conn->Auth))
	    {
	    snprintf(sbuf,160,"HTTP/1.0 401 Unauthorized\r\n"
	    		 "Server: %s\r\n"
			 "WWW-Authenticate: Basic realm=\"%s\"\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>Unauthorized</H1>\r\n",NHT.ServerString,NHT.Realm);
	    fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
	    /*if (*(conn->Cookie))
		printf("Warning: session %s did not provide an Auth header.\n", conn->Cookie);*/
	    goto out;
	    }

	/** Got authentication.  Parse the auth string. **/
	usrname = strtok(conn->Auth,":");
	if (usrname) passwd = strtok(NULL,"\r\n");
	if (usrname && !passwd) passwd = "";
	if (!usrname || !passwd) 
	    {
	    snprintf(sbuf,160,"HTTP/1.0 400 Bad Request\r\n"
	    		 "Server: %s\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>400 Bad Request</H1>\r\n",NHT.ServerString);
	    fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
	    goto out;
	    }

	/** Check for a cookie -- if one, try to resume a session. **/
	if (*(conn->Cookie))
	    {
	    if (conn->Cookie[strlen(conn->Cookie)-1] == ';') conn->Cookie[strlen(conn->Cookie)-1] = '\0';
	    conn->NhtSession = (pNhtSessionData)xhLookup(&(NHT.CookieSessions), conn->Cookie);
	    if (conn->NhtSession)
	        {
		nht_internal_LinkSess(conn->NhtSession);
		if (strcmp(conn->NhtSession->Username,usrname) || strcmp(passwd,conn->NhtSession->Password))
		    {
	    	    snprintf(sbuf,160,"HTTP/1.0 401 Unauthorized\r\n"
		    		 "Server: %s\r\n"
				 "WWW-Authenticate: Basic realm=\"%s\"\r\n"
				 "Content-Type: text/html\r\n"
				 "\r\n"
				 "<H1>Unauthorized</H1>\r\n",NHT.ServerString,NHT.Realm);
	            fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		    mssError(1,"NHT","Bark! User supplied valid cookie %s but cred mismatch (sesslink %d, provided %s, stored %s)", conn->Cookie, conn->NhtSession->LinkCnt, usrname, conn->NhtSession->Username);
		    /*printf("Valid cookie but cred mismatch:  session data structure for %s %s", usrname, conn->Cookie);
		    for(i=0;i<sizeof(NhtSessionData);i++)
			{
			printf("%c%2.2x", (((i%16) == 0)?'\n':' '), ((unsigned char*)conn->NhtSession)[i]);
			}
		    printf("\nAuth Data: ");
		    for(i=0;i<16;i++) printf("%2.2x %2.2x ", usrname[i], passwd[i]);
		    printf("\n");*/
		    goto out;
		    }
		if (conn->NhtSession->Session)
		    {
		    thSetParam(NULL,"mss",conn->NhtSession->Session);
		    thSetParamFunctions(NULL,mssLinkSession,mssUnlinkSession);
		    mssLinkSession(conn->NhtSession->Session);
		    }
		/*thSetUserID(NULL,((pMtSession)(conn->NhtSession->Session))->UserID);*/
		thSetSecContext(NULL, &(conn->NhtSession->SecurityContext));
		w_timer = conn->NhtSession->WatchdogTimer;
		i_timer = conn->NhtSession->InactivityTimer;
		}
	    }
	else
	    {
	    conn->NhtSession = NULL;
	    }

	/** Parse out the requested url **/
	/*printf("debug: %s\n",urlptr);*/
	url_inf = htsParseURL(conn->URL);
	if (!url_inf)
	    {
	    snprintf(sbuf,160,"HTTP/1.0 500 Internal Server Error\r\n"
	    		 "Server: %s\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>500 Internal Server Error</H1>\r\n",NHT.ServerString);
	    mssError(1,"NHT","Failed to handle HTTP request, exiting thread (could not parse URL).");
	    fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
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
	nht_internal_ConstructPathname(url_inf);

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
		if (nht_internal_ResetWatchdog(w_timer))
		    {
		    snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
				 "Server: %s\r\n"
				 "Pragma: no-cache\r\n"
				 "Content-Type: text/html\r\n"
				 "\r\n"
				 "<A HREF=/ TARGET=ERR></A>\r\n",NHT.ServerString);
		    fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		    goto out;
		    }
		else
		    {
		    /** Update watchdogs on app and group, if specified **/
		    find_inf = stLookup_ne(url_inf,"cx__akey");
		    if (find_inf)
			{
			if (nht_internal_VerifyAKey(find_inf->StrVal, conn->NhtSession, &group, &app) == 0)
			    {
			    /** session key matched... now update app and group **/
			    if (app) nht_internal_ResetWatchdog(app->WatchdogTimer);
			    if (group) nht_internal_ResetWatchdog(group->WatchdogTimer);
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
		    fdQPrintf(conn->ConnFD,"HTTP/1.0 200 OK\r\n"
				 "Server: %STR\r\n"
				 "Pragma: no-cache\r\n"
				 "Content-Type: text/html\r\n"
				 "\r\n"
				 "<A HREF=/ TARGET='%STR&HTE'></A>\r\n",
				 NHT.ServerString, timestr);
		    goto out;
		    }
		}
	    else
		{
		/** No session and this is a watchdog ping?  If so, we don't
		 ** want to automatically re-login the user since that defeats the purpose
		 ** of session timeouts.
		 **/
		snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
			     "Server: %s\r\n"
			     "Pragma: no-cache\r\n"
			     "Content-Type: text/html\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR></A>\r\n",NHT.ServerString);
		fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
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
		err = nht_internal_ResetWatchdog(i_timer);
	    if (err == 0)
		err = nht_internal_ResetWatchdog(w_timer);
	    if (err < 0)
		{
		snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
			     "Server: %s\r\n"
			     "Pragma: no-cache\r\n"
			     "Content-Type: text/html\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR></A>\r\n",NHT.ServerString);
		fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		goto out;
		}
	    }

	/** No cookie or no session for the given cookie? **/
	if (!conn->NhtSession)
	    {
	    /** No session, and the connection is a 'non-activity' request? **/
	    if (conn->NotActivity)
		{
		snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
			     "Server: %s\r\n"
			     "Pragma: no-cache\r\n"
			     "Content-Type: text/html\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR></A>\r\n",NHT.ServerString);
		fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		goto out;
		}

	    /** Attempt authentication **/
	    if (mssAuthenticate(usrname, passwd) < 0)
	        {
	        snprintf(sbuf,160,"HTTP/1.0 401 Unauthorized\r\n"
			     "Server: %s\r\n"
			     "Content-Type: text/html\r\n"
			     "WWW-Authenticate: Basic realm=\"%s\"\r\n"
			     "\r\n"
			     "<H1>Unauthorized</H1>\r\n",NHT.ServerString,NHT.Realm);
	        fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		/*printf("\nNew session requested, but user supplied invalid auth data: ");
		for(i=0;i<16;i++) printf("%2.2x %2.2x ", usrname[i], passwd[i]);
		printf("\n");*/
		goto out;
		}

	    /** Authentication succeeded - start a new session **/
	    conn->NhtSession = nht_internal_AllocSession(usrname);
	    printf("NHT: new session for username [%s], cookie [%s]\n", conn->NhtSession->Username, conn->NhtSession->Cookie);
	    nht_internal_LinkSess(conn->NhtSession);
	    }

	/** Start the application security context **/
	if (conn->NhtSession && conn->NhtSession->Session)
	    {
	    cxssPushContext();
	    context_started = 1;
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
	if ((find_inf=stLookup_ne(url_inf,"ls__triggerid")))
	    {
	    tid = strtoi(find_inf->StrVal,NULL,0);
	    nht_internal_StartTrigger(conn->NhtSession, tid);
	    }

	/** If the method was GET and an ls__method was specified, use that method **/
	if (!strcmp(conn->Method,"get") && (find_inf=stLookup_ne(url_inf,"ls__method")))
	    {
	    akey_inf = stLookup_ne(url_inf,"cx__akey");
	    if (!akey_inf || strncmp(akey_inf->StrVal, conn->NhtSession->SKey, strlen(conn->NhtSession->SKey)))
		{
		snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
			     "Server: %s\r\n"
			     "Pragma: no-cache\r\n"
			     "Content-Type: text/html\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR></A>\r\n",NHT.ServerString);
		fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		goto out;
		}
	    if (!strcasecmp(find_inf->StrVal,"get"))
	        {
	        nht_internal_GET(conn,url_inf,conn->IfModifiedSince);
		}
	    else if (!strcasecmp(find_inf->StrVal,"copy"))
	        {
		find_inf = stLookup_ne(url_inf,"ls__destination");
		if (!find_inf || !(find_inf->StrVal))
		    {
	            snprintf(sbuf,160,"HTTP/1.0 400 Method Error\r\n"
	    		 "Server: %s\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>400 Method Error - include ls__destination for copy</H1>\r\n",NHT.ServerString);
	            fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		    }
		else
		    {
		    ptr = find_inf->StrVal;
		    nht_internal_COPY(conn,url_inf, ptr);
		    }
		}
	    else if (!strcasecmp(find_inf->StrVal,"put"))
	        {
		find_inf = stLookup_ne(url_inf,"ls__content");
		if (!find_inf || !(find_inf->StrVal))
		    {
	            snprintf(sbuf,160,"HTTP/1.0 400 Method Error\r\n"
	    		 "Server: %s\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>400 Method Error - include ls__content for put</H1>\r\n",NHT.ServerString);
	            fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		    }
		else
		    {
		    ptr = find_inf->StrVal;
		    conn->Size = strlen(ptr);
	            nht_internal_PUT(conn,url_inf,conn->Size,ptr);
		    }
		}
	    else if (!strcasecmp(find_inf->StrVal,"post"))
	        {
		find_inf = stLookup_ne(url_inf,"cx__content");
		if (!find_inf || !(find_inf->StrVal))
		    {
	            fdPrintf(conn->ConnFD,
			"HTTP/1.0 400 Method Error\r\n"
	    		"Server: %s\r\n"
			"Content-Type: text/html\r\n"
			"\r\n"
			"<H1>400 Method Error - include cx__content for POST</H1>\r\n",NHT.ServerString);
		    }
		else
		    {
		    ptr = find_inf->StrVal;
		    conn->Size = strlen(ptr);
	            nht_internal_POST(conn, url_inf, conn->Size, ptr);
		    }
		}
	    else if (!strcasecmp(find_inf->StrVal,"patch"))
	        {
		find_inf = stLookup_ne(url_inf,"cx__content");
		if (!find_inf || !(find_inf->StrVal))
		    {
	            fdPrintf(conn->ConnFD,
			"HTTP/1.0 400 Method Error\r\n"
	    		"Server: %s\r\n"
			"Content-Type: text/html\r\n"
			"\r\n"
			"<H1>400 Method Error - include cx__content for PATCH</H1>\r\n",NHT.ServerString);
		    }
		else
		    {
		    ptr = find_inf->StrVal;
		    conn->Size = strlen(ptr);
	            nht_internal_PATCH(conn, url_inf, ptr);
		    }
		}
	    }
	else
	    {
	    /** Which method was used? **/
	    if (!strcmp(conn->Method,"get"))
	        {
	        nht_internal_GET(conn,url_inf,conn->IfModifiedSince);
	        }
	    else if (!strcmp(conn->Method,"put"))
	        {
	        nht_internal_PUT(conn,url_inf,conn->Size,NULL);
	        }
	    else if (!strcmp(conn->Method,"post"))
		{
		nht_internal_POST(conn, url_inf, conn->Size, NULL);
		}
	    else if (!strcmp(conn->Method,"patch"))
		{
		nht_internal_PATCH(conn, url_inf, NULL);
		}
	    else if (!strcmp(conn->Method,"copy"))
	        {
	        nht_internal_COPY(conn,url_inf, conn->Destination);
	        }
	    else
	        {
	        snprintf(sbuf,160,"HTTP/1.0 501 Not Implemented\r\n"
	    		 "Server: %s\r\n"
			 "Content-Type: text/html\r\n"
			 "\r\n"
			 "<H1>501 Method Not Implemented</H1>\r\n",NHT.ServerString);
	        fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
	        /*netCloseTCP(conn,1000,0);*/
		}
	    }

	/** End a trigger? **/
	if (tid != -1) nht_internal_EndTrigger(conn->NhtSession,tid);

	/*nht_internal_UnlinkSess(conn->NhtSession);*/

	/** Close and exit. **/
	if (url_inf) stFreeInf_ne(url_inf);
	nht_internal_FreeConn(conn);
	conn = NULL;
	if (context_started) cxssPopContext();
	thExit();

    error:
	mssError(1,"NHT","Failed to handle HTTP request, exiting thread (%s).",msg);
	snprintf(sbuf,160,"HTTP/1.0 400 Request Error\r\n\r\n%s\r\n",msg);
	fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);

    out:
	if (url_inf) stFreeInf_ne(url_inf);
	if (conn) nht_internal_FreeConn(conn);
	if (context_started) cxssPopContext();
	thExit();
    }


/*** nht_internal_TLSHandler - manages incoming TLS-encrypted HTTP
 *** connections.
 ***/
void
nht_internal_TLSHandler(void* v)
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
	SSL_CTX_set_options(NHT.SSL_ctx, SSL_OP_NO_SSLv2 | SSL_OP_SINGLE_DH_USE | SSL_OP_ALL);
	if (stAttrValue(stLookup(CxGlobals.ParsedConfig,"ssl_cipherlist"),NULL,&ptr,0) < 0)
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
	if (stAttrValue(stLookup(my_config,"ssl_cert_chain"), NULL, &strval, 0) != 0)
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

	    /** Set up the connection structure **/
	    conn = nht_internal_AllocConn(connection_socket);
	    if (!conn)
		{
		netCloseTCP(connection_socket, 1000, 0);
		thSleep(1);
		continue;
		}

	    /** Start TLS on the connection.  This replaces conn->ConnFD with
	     ** a pipe to the TLS encryption/decryption process.
	     **/
	    conn->SSLpid = cxssStartTLS(NHT.SSL_ctx, &conn->ConnFD, &conn->ReportingFD, 1);
	    if (conn->SSLpid <= 0)
		{
		nht_internal_FreeConn(conn);
		mssError(1,"NHT","Could not start TLS on the connection!");
		}

	    /** Start the request handler thread **/
	    if (!thCreate(nht_internal_ConnHandler, 0, conn))
	        {
		nht_internal_FreeConn(conn);
		mssError(1,"NHT","Could not create thread to handle TLS connection!");
		}
	    }

	/** Exit. **/
	mssError(1,"NHT","Could not continue to accept TLS requests.");
	netCloseTCP(listen_socket,0,0);

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

	    /** Set up the connection structure **/
	    conn = nht_internal_AllocConn(connection_socket);
	    if (!conn)
		{
		netCloseTCP(connection_socket, 1000, 0);
		thSleep(1);
		continue;
		}

	    /** Start the request handler thread **/
	    if (!thCreate(nht_internal_ConnHandler, 0, conn))
	        {
		nht_internal_FreeConn(conn);
		mssError(1,"NHT","Could not create thread to handle connection!");
		}
	    }

	/** Exit. **/
	mssError(1,"NHT","Could not continue to accept requests.");
	netCloseTCP(listen_socket,0,0);

    thExit();
    }
