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

/**CVSDATA***************************************************************

    $Id: net_http_conn.c,v 1.2 2008/08/16 00:31:38 thr4wn Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/netdrivers/net_http_conn.c,v $

    $Log: net_http_conn.c,v $
    Revision 1.2  2008/08/16 00:31:38  thr4wn
    I made some more modification of documentation and begun logic for
    caching generated WgtrNode instances (see centrallix-sysdoc/misc.txt)

    Revision 1.1  2008/06/25 22:48:12  jncraton
    - (change) split net_http into separate files
    - (change) replaced nht_internal_UnConvertChar with qprintf filter
    - (change) replaced nht_internal_escape with qprintf filter
    - (change) replaced nht_internal_decode64 with qprintf filter
    - (change) removed nht_internal_Encode64
    - (change) removed nht_internal_EncodeHTML


 **END-CVSDATA***********************************************************/
 
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
nht_internal_ConnHandler(void* connfd_v)
    {
    pFile connfd = (pFile)connfd_v;
    char sbuf[160];
    char* msg = "";
    char* ptr;
    char* usrname;
    char* passwd = NULL;
    pStruct url_inf,find_inf;
    int tid = -1;
    handle_t w_timer = XHN_INVALID_HANDLE, i_timer = XHN_INVALID_HANDLE;
    pNhtUser usr;
    pNhtConn conn = NULL;
    pNhtSessionData nsess;
    int akey[2];
    unsigned char t_lsb;
    int noact = 0;
    int err;

    	/*printf("ConnHandler called, stack ptr = %8.8X\n",&s);*/

	/** Set the thread's name **/
	thSetName(NULL,"HTTP Connection Handler");

	/** Create the connection structure **/
	conn = nht_internal_AllocConn(connfd);
	if (!conn)
	    {
	    netCloseTCP(connfd, 1000, 0);
	    thExit();
	    }

	/** Restrict access to connections from localhost only? **/
	if (NHT.RestrictToLocalhost && strcmp(conn->IPAddr, "127.0.0.1") != 0)
	    {
	    msg = "Connections currently restricted via accept_localhost_only in centrallix.conf";
	    goto error;
	    }

	/** Parse the HTTP Headers... **/
	if (nht_internal_ParseHeaders(conn) < 0)
	    {
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
	    nht_internal_FreeConn(conn);
	    thExit();
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
	    nht_internal_FreeConn(conn);
	    thExit();
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
		    nht_internal_FreeConn(conn);
	            thExit();
		    }
		thSetParam(NULL,"mss",conn->NhtSession->Session);
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
	    fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
	    nht_internal_FreeConn(conn);
	    conn = NULL;
	    }
	nht_internal_ConstructPathname(url_inf);

	/** Watchdog ping? **/
	if ((find_inf=stLookup_ne(url_inf,"cx__noact")))
	    {
	    if (strtol(find_inf->StrVal,NULL,0) != 0)
		noact = 1;
	    }
	if (!strcmp(conn->URL,"/INTERNAL/ping"))
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
		    nht_internal_FreeConn(conn);
		    thExit();
		    }
		else
		    {
		    snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
				 "Server: %s\r\n"
				 "Pragma: no-cache\r\n"
				 "Content-Type: text/html\r\n"
				 "\r\n"
				 "<A HREF=/ TARGET=OK></A>\r\n",NHT.ServerString);
		    fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		    nht_internal_FreeConn(conn);
		    thExit();
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
		nht_internal_FreeConn(conn);
		thExit();
		}
	    }
	else if (conn->NhtSession)
	    {
	    /** Reset the idle and watchdog (disconnect) timers on a normal request
	     ** however if cx__noact is set, only reset the watchdog timer.
	     **/
	    err = 0;
	    if (!noact && err == 0)
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
		nht_internal_FreeConn(conn);
		thExit();
		}
	    }

	/** No cookie or no session for the given cookie? **/
	if (!conn->NhtSession)
	    {
	    if (mssAuthenticate(usrname, passwd) < 0)
	        {
	        snprintf(sbuf,160,"HTTP/1.0 401 Unauthorized\r\n"
			     "Server: %s\r\n"
			     "Content-Type: text/html\r\n"
			     "WWW-Authenticate: Basic realm=\"%s\"\r\n"
			     "\r\n"
			     "<H1>Unauthorized</H1>\r\n",NHT.ServerString,NHT.Realm);
	        fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		nht_internal_FreeConn(conn);
	        thExit();
		}
	    usr = (pNhtUser)xhLookup(&(NHT.UsersByName), usrname);
	    if (!usr)
		{
		usr = (pNhtUser)nmMalloc(sizeof(NhtUser));
		usr->SessionCnt = 0;
		strtcpy(usr->Username, usrname, sizeof(usr->Username));
		xhAdd(&(NHT.UsersByName), usr->Username, (void*)(usr));
		xaAddItem(&NHT.UsersList, (void*)usr);
		}
	    if (usr->SessionCnt >= NHT.UserSessionLimit)
		{
		nht_internal_DiscardASession(usr);
		}
	    nsess = (pNhtSessionData)nmMalloc(sizeof(NhtSessionData));
	    strtcpy(nsess->Username, mssUserName(), sizeof(nsess->Username));
	    strtcpy(nsess->Password, mssPassword(), sizeof(nsess->Password));
	    thGetSecContext(NULL, &(nsess->SecurityContext));
	    nsess->User = usr;
	    nsess->Session = thGetParam(NULL,"mss");
	    nsess->IsNewCookie = 1;
	    nsess->ObjSess = objOpenSession("/");
	    nsess->Errors = syCreateSem(0,0);
	    nsess->ControlMsgs = syCreateSem(0,0);
	    nsess->WatchdogTimer = nht_internal_AddWatchdog(NHT.WatchdogTime*1000, nht_internal_WTimeout, (void*)nsess);
	    nsess->InactivityTimer = nht_internal_AddWatchdog(NHT.InactivityTime*1000, nht_internal_ITimeout, (void*)nsess);
	    nsess->LinkCnt = 1;
	    xaInit(&nsess->Triggers,16);
	    xaInit(&nsess->ErrorList,16);
	    xaInit(&nsess->ControlMsgsList,16);
	    nht_internal_CreateCookie(nsess->Cookie);
	    cxssGenerateKey((unsigned char*)akey, sizeof(akey));
	    sprintf(nsess->AKey, "%8.8x%8.8x", akey[0], akey[1]);
	    xhnInitContext(&(nsess->Hctx));
	    xhAdd(&(NHT.CookieSessions), nsess->Cookie, (void*)nsess);
	    usr->SessionCnt++;
	    xaAddItem(&(NHT.Sessions), (void*)nsess);
	    nsess->CachedApps = (pXHashTable)nmSysMalloc(sizeof(XHashTable));
	    xhInit(nsess->CachedApps, 127, 4);
	    conn->NhtSession = nsess;
	    printf("NHT: new session for username [%s], cookie [%s]\n", nsess->Username, nsess->Cookie);
	    nht_internal_LinkSess(conn->NhtSession);
	    }

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
	    tid = strtol(find_inf->StrVal,NULL,0);
	    nht_internal_StartTrigger(conn->NhtSession, tid);
	    }

	/** If the method was GET and an ls__method was specified, use that method **/
	if (!strcmp(conn->Method,"get") && (find_inf=stLookup_ne(url_inf,"ls__method")))
	    {
	    find_inf = stLookup_ne(url_inf,"cx__akey");
	    if (!find_inf || strcmp(find_inf->StrVal, nsess->AKey))
		{
		snprintf(sbuf,160,"HTTP/1.0 200 OK\r\n"
			     "Server: %s\r\n"
			     "Pragma: no-cache\r\n"
			     "Content-Type: text/html\r\n"
			     "\r\n"
			     "<A HREF=/ TARGET=ERR></A>\r\n",NHT.ServerString);
		fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
		nht_internal_FreeConn(conn);
		thExit();
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
		nht_internal_POST(conn, url_inf, conn->Size);
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

    thExit();

    error:
	mssError(1,"NHT","Failed to handle HTTP request, exiting thread (%s).",msg);
	snprintf(sbuf,160,"HTTP/1.0 400 Request Error\r\n\r\n%s\r\n",msg);
	fdWrite(conn->ConnFD,sbuf,strlen(sbuf),0,0);
	if (conn) nht_internal_FreeConn(conn);
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
    int i;

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
		    strtcpy(listen_port, strval, sizeof(listen_port));
		    }
		else
		    {
		    snprintf(listen_port,32,"%d",intval);
		    }
		}

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

	    /** Get the timer settings **/
	    stAttrValue(stLookup(my_config, "session_watchdog_timer"), &(NHT.WatchdogTime), NULL, 0);
	    stAttrValue(stLookup(my_config, "session_inactivity_timer"), &(NHT.InactivityTime), NULL, 0);

	    /** Session limits **/
	    stAttrValue(stLookup(my_config, "user_session_limit"), &(NHT.UserSessionLimit), NULL, 0);

	    /** Access log file **/
	    if (stAttrValue(stLookup(my_config, "access_log"), NULL, &strval, 0) >= 0)
		{
		strtcpy(NHT.AccessLogFile, strval, sizeof(NHT.Realm));
		NHT.AccessLogFD = fdOpen(NHT.AccessLogFile, O_WRONLY | O_APPEND, 0600);
		if (!NHT.AccessLogFD)
		    {
		    mssErrorErrno(1,"NHT","Could not open access_log file '%s'", NHT.AccessLogFile);
		    }
		}

	    if (stAttrValue(stLookup(my_config, "session_cookie"), NULL, &strval, 0) < 0)
		{
		strval = "CXID";
		}
	    strtcpy(NHT.SessionCookie, strval, sizeof(NHT.SessionCookie));
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
	    if (!connection_socket)
		{
		thSleep(10);
		continue;
		}
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
