#include "net_http.h"
#include "cxss/cxss.h"
#include "application.h"
#include <openssl/sha.h>
#include <cxlib/qprintf.h>

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


/*** nht_i_AddLoginHashCookie - send a response header setting a time-limited
 *** cookie used for allowing a login.  If the browser does not send a valid
 *** login hash cookie, authentication will be rejected even if the username
 *** and password are valid.  This is used to help ensure a browser's cached
 *** credentials cannot be used to log in.
 ***/
int
nht_i_AddLoginHashCookie(pNhtConn conn)
    {
    SHA256_CTX hashctx;
    long long timestamp;
    unsigned char nonce[8];
    unsigned char hash[SHA256_DIGEST_LENGTH];

	/** Get timestamp and nonce **/
	timestamp = mtLastTick() * 1000LL / NHT.ClkTck;
	cxssKeystreamGenerate(NHT.NonceData, nonce, sizeof(nonce));

	/** Generate our secure hash **/
	SHA256_Init(&hashctx);
	SHA256_Update(&hashctx, nonce, sizeof(nonce));
	SHA256_Update(&hashctx, NHT.LoginKey, sizeof(NHT.LoginKey));
	SHA256_Update(&hashctx, (unsigned char*)&timestamp, sizeof(timestamp));
	SHA256_Update(&hashctx, nonce, sizeof(nonce));
	SHA256_Update(&hashctx, NHT.LoginKey, sizeof(NHT.LoginKey));
	SHA256_Update(&hashctx, (unsigned char*)&timestamp, sizeof(timestamp));
	SHA256_Final(hash, &hashctx);

	/** Send the header **/
	nht_i_AddResponseHeaderQPrintf(conn, "Set-Cookie", "CXLH=%8STR&HEX%8STR&HEX%*STR&HEX; Max-Age=60; HttpOnly; SameSite=Strict; Path=/",
		nonce,
		(unsigned char*)&timestamp,
		SHA256_DIGEST_LENGTH,
		hash
		);

    return 0;
    }


/*** nht_i_CheckLoginHashCookie - check whether a valid login hash cookie was
 *** provided with the request.
 ***/
int
nht_i_CheckLoginHashCookie(pNhtConn conn)
    {
    char hex[] = "0123456789abcdef";
    SHA256_CTX hashctx;
    long long timestamp, cur_timestamp;
    unsigned char nonce[8];
    unsigned char decode[256];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char* cookieptr;
    int found = 0;

	cur_timestamp = mtLastTick() * 1000LL / NHT.ClkTck;
	cookieptr = conn->AllCookies;
	while((cookieptr = strstr(cookieptr, "CXLH=")) != NULL)
	    {
	    found = 1;
	    if (strspn(cookieptr + 5, hex) == (SHA256_DIGEST_LENGTH + sizeof(nonce) + sizeof(timestamp)) * 2)
		{
		/** Extract the pieces **/
		qpfPrintf(NULL, (char*)decode, sizeof(decode), "%16STR&DHEX", cookieptr + 5);
		memcpy(nonce, decode, sizeof(nonce));
		qpfPrintf(NULL, (char*)decode, sizeof(decode), "%16STR&DHEX", cookieptr + 5 + (sizeof(nonce) * 2));
		memcpy((char*)&timestamp, decode, sizeof(timestamp));
		qpfPrintf(NULL, (char*)decode, sizeof(decode), "%64STR&DHEX", cookieptr + 5 + (sizeof(nonce) * 2) + (sizeof(timestamp) * 2));

		/** Recompute the hash **/
		SHA256_Init(&hashctx);
		SHA256_Update(&hashctx, nonce, sizeof(nonce));
		SHA256_Update(&hashctx, NHT.LoginKey, sizeof(NHT.LoginKey));
		SHA256_Update(&hashctx, (unsigned char*)&timestamp, sizeof(timestamp));
		SHA256_Update(&hashctx, nonce, sizeof(nonce));
		SHA256_Update(&hashctx, NHT.LoginKey, sizeof(NHT.LoginKey));
		SHA256_Update(&hashctx, (unsigned char*)&timestamp, sizeof(timestamp));
		SHA256_Final(hash, &hashctx);

		/** Does it match, and is it within the last minute? **/
		if (memcmp(hash, decode, sizeof(hash)) == 0 && cur_timestamp >= timestamp && (cur_timestamp - timestamp) <= 60000)
		    return 0;
		}
	    cookieptr++;
	    }

    return found?(-2):(-1);
    }


/*** nht_i_AddHeaderNonce - attaches a X-Nonce: header to the response,
 *** which perturbs the order of things in the header which could make
 *** some types of cryptographic attacks harder to carry out.
 ***/
int
nht_i_AddHeaderNonce(pNhtConn conn)
    {
    char* nonce = NULL;
    int cnt;
    unsigned char noncelen;

	/** Buffer for the nonce **/
	nonce = nmSysMalloc(256+16+1);
	if (!nonce)
	    goto error;

	/** This gives us the length of the nonce, between 16 and 271 chars **/
	cxssKeystreamGenerate(NHT.NonceData, &noncelen, 1);
	cnt = ((int)noncelen) + 16;

	/** This is the actual nonce data **/
	if (cxssKeystreamGenerateHex(NHT.NonceData, nonce, cnt) < 0)
	    goto error;

	/** Add the header **/
	nht_i_AddResponseHeader(conn, "X-Nonce", nonce, 1);
	nonce = NULL;

	return 0;

    error:
	if (nonce) nmSysFree(nonce);
	return -1;
    }


/*** nht_i_ZapSessionCookie - see if there is a session cookie in the request,
 *** and if so, cause it to expire.
 ***/
int
nht_i_ZapSessionCookie(pNhtConn conn)
    {
    char* cookie_prefix = (conn->UsingTLS)?NHT.TlsSessionCookie:NHT.SessionCookie;

	/** If session is not valid and session cookie is set, zap it **/
	if (!conn->NhtSession && strncmp(conn->Cookie, cookie_prefix, strlen(cookie_prefix)) == 0 && conn->Cookie[strlen(cookie_prefix)] == '=' && strspn(strchr(conn->Cookie, '=') + 1, "abcdef0123456789") == 32)
	    {
	    nht_i_AddResponseHeaderQPrintf(conn, "Set-Cookie", "%STR=%STR&32LEN; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT", cookie_prefix, strchr(conn->Cookie, '=') + 1);
	    return 1;
	    }

    return 0;
    }


/*** nht_i_SendRefreshDocument - send an HTML document to refresh the current
 *** request in the current site context (same-site).  Used after verifying
 *** a signed url request is trusted.
 ***/
int
nht_i_SendRefreshDocument(pNhtConn conn, char* url)
    {

	/** This is a simple HTML document that loads the url we give it. **/
	nht_i_QPrintfConn(conn, 0,
		"<!doctype html>\r\n"
		"<html>\r\n"
		"    <head>\r\n"
		"         <script language=\"JavaScript\">\r\n"
		"             var url = '%STR&JSSTR';\r\n"
		"         </script>\r\n"
		"    </head>\r\n"
		"    <body onload=\"document.location = url;\">\r\n"
		"    </body>\r\n"
		"</html>\r\n"
		"\r\n",
		url
		);

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
    char* parsemsg;
    char errbuf[256];
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
    pNhtApp app = NULL;
    pNhtAppGroup group = NULL;
    int context_started = 0;
    pApplication tmp_app = NULL;
    long long n;
    int cred_valid = 0;
    int akey_valid = 0;
    int is_signed_url = 0;
    int rval;
    int i;

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
	if (nht_i_ParseHeaders(conn, &parsemsg) < 0)
	    {
	    if (conn->ReportingFD != NULL && cxssStatTLS(conn->ReportingFD, sbuf, sizeof(sbuf)) >= 0 && sbuf[0] == '!')
		snprintf(errbuf, sizeof(errbuf), "Error parsing headers: %s: %s", parsemsg, sbuf+1);
	    else
		snprintf(errbuf, sizeof(errbuf), "Error parsing headers: %s", parsemsg);
	    msg = errbuf;
	    goto error;
	    }

	/** Sanity check **/
	if (strspn(conn->Method, "abcdefghijklmnopqrstuvwxyz") < strlen(conn->Method))
	    {
	    msg = "Malformed method";
	    goto error;
	    }

	/** Signed url? **/
	is_signed_url = (cxssLinkVerify(conn->URL) == 0);
	ptr = strstr(conn->URL, "cx__lkey=");
	if (!is_signed_url && ptr && ptr != conn->URL && (ptr[-1] == '&' || ptr[-1] == '?'))
	    {
	    cxDebugLog("NHT: %s: URL contains an invalid signature.", conn->IPAddr);
	    }

	/** Compute header nonce.  This is used for functionally nothing, but
	 ** it causes the content and offsets to values in the header to change
	 ** with each response; this can help frustrate certain types of 
	 ** cryptographic attacks.
	 **/
	if (conn->UsingTLS && NHT.NonceData)
	    {
	    nht_i_AddHeaderNonce(conn);
	    }

	/** Add some entropy to the pool - just the LSB of the time.  Our
	 ** estimate is that this byte contains just 2 bits of entropy.
	 **/
	t_lsb = mtRealTicks() & 0xFF;
	cxssAddEntropy(&t_lsb, 1, 2);

	/** Did client send authentication? **/
	if (!*(conn->Auth))
	    {
	    /** It helps in some cases to zap the session cookie when a request
	     ** without auth happens, but due to a browser bug we can't reliably
	     ** do this (tested: Chrome 122).  Without this zap, occasionally a
	     ** user will get an extra login prompt.
	     **/
	    /*nht_i_ZapSessionCookie(conn);*/
	    nht_i_AddResponseHeaderQPrintf(conn, "WWW-Authenticate", "Basic realm=%STR&DQUOT", NHT.Realm);
	    if (NHT.AuthMethods & NHT_AUTH_HTTPSTRICT)
		{
		nht_i_AddLoginHashCookie(conn);
		cxDebugLog("NHT: %s: No Authentication provided, sending 401 and login cookie.", conn->IPAddr);
		}
	    else
		{
		cxDebugLog("NHT: %s: No Authentication provided, sending 401.", conn->IPAddr);
		}
	    nht_i_WriteErrResponse(conn, 401, "Unauthorized", "<h1>Unauthorized</h1>\r\n");
	    goto out;
	    }

	/** Got authentication.  Parse the auth string. **/
	usrname = strtok(conn->Auth,":");
	if (usrname) passwd = strtok(NULL,"\r\n");
	if (usrname && !passwd) passwd = "";
	if (!usrname || !passwd || !usrname[0] || strspn(usrname, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.") < strlen(usrname))
	    {
	    cxDebugLog("NHT: %s: Malformed authentication provided, sending 401.", conn->IPAddr);
	    nht_i_WriteErrResponse(conn, 401, "Unauthorized", "<h1>401 Unauthorized</h1>\r\n");
	    goto out;
	    }

	/** Check for a cookie -- if one, try to resume a session. **/
	if (*(conn->Cookie))
	    {
	    /** Lookup the cookie in our session list **/
	    if (conn->Cookie[strlen(conn->Cookie)-1] == ';') conn->Cookie[strlen(conn->Cookie)-1] = '\0';
	    conn->NhtSession = (pNhtSessionData)xhLookup(&(NHT.CookieSessions), conn->Cookie);
	    if (conn->NhtSession && !conn->NhtSession->Closed)
	        {
		nht_i_LinkSess(conn->NhtSession);

		/** Active session - compare credentials with session data **/
		if (strcmp(conn->NhtSession->Username,usrname) || strcmp(passwd,conn->NhtSession->Password))
		    {
		    nht_i_AddResponseHeaderQPrintf(conn, "WWW-Authenticate", "Basic realm=%STR&DQUOT", NHT.Realm);
		    mssError(1,"NHT","Bark! User supplied valid cookie %s but cred mismatch (sesslink %d, provided %s, stored %s)", conn->Cookie, conn->NhtSession->LinkCnt, usrname, conn->NhtSession->Username);

		    /** Insert a synthetic delay to deter brute forcing in a
		     ** cookie theft situation.  Yield occasionally, to mitigate
		     ** against DoS.  Alternative:  destroy the session right
		     ** away, but that has the disadvantage of affecting the
		     ** real user.  We may switch our tactic on this in the
		     ** future.
		     **/
		    for(n=i=0; i<25000000; i++)
			{
			if (i % 2500000 == 0)
			    thYield();
			n += i;
			}

		    cxDebugLog("NHT: %s: Valid session cookie but credential mismatch, sending 401.", conn->IPAddr);
		    nht_i_WriteErrResponse(conn, 401, "Unauthorized", "<h1>Unauthorized</h1>\r\n");

		    goto out;
		    }
		else
		    {
		    /** Remember that we already checked the credentials
		     ** via comparing with the session data.  This is redundant,
		     ** as it is invariant with a valid NhtSession, but we do
		     ** it this way for maximum clarity.
		     **/
		    cred_valid = 1;
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
		conn->NhtSession = NULL;
		}
	    }
	else
	    {
	    /** No cookie supplied, thus no session. **/
	    conn->NhtSession = NULL;
	    }

	/** Authenticated but using a signed URL and no session cookie?  This
	 ** indicates the browser may be withholding cookies due to same-origin
	 ** constraints.  So we verify we made the link via the HMAC signature
	 ** and cause the browser to re-load the URL with us as the origin,
	 ** which causes the request to be re-sent with cookies.
	 **/
	if (!conn->NhtSession && is_signed_url && mssAuthenticate(usrname, passwd, cred_valid) == 0)
	    {
	    ptr = strstr(conn->URL, "cx__lkey=");
	    if (ptr && ptr != conn->URL && (ptr[-1] == '&' || ptr[-1] == '?'))
		{
		ptr[-1] = '\0';
		nht_i_WriteResponse(conn, 200, "OK", NULL);
		nht_i_SendRefreshDocument(conn, conn->URL);
		cxDebugLog("NHT: %s: Processing correctly signed URL.", conn->IPAddr);
		goto out;
		}
	    }

	/** If the user is logging in initially, ensure they are doing so
	 ** with a valid login hash cookie (CXLH).
	 **/
	if ((NHT.AuthMethods & NHT_AUTH_HTTPSTRICT) && !cred_valid && (rval = nht_i_CheckLoginHashCookie(conn)) < 0)
	    {
	    nht_i_AddResponseHeaderQPrintf(conn, "WWW-Authenticate", "Basic realm=%STR&DQUOT", NHT.Realm);

	    /** Also deal with expired session cookie **/
	    nht_i_ZapSessionCookie(conn);
	    nht_i_AddLoginHashCookie(conn);
	    nht_i_WriteErrResponse(conn, 401, "Unauthorized", "<h1>Unauthorized</h1>\r\n");
	    if (rval == -2)
		cxDebugLog("NHT: %s: No valid session, and login cookie was invalid; sending login cookie.", conn->IPAddr);
	    else
		cxDebugLog("NHT: %s: No valid session; sending login cookie.", conn->IPAddr);
	    goto out;
	    }

	/** Attempt authentication.  If we already checked the credentials,
	 ** then we just set up a session here without checking again, since
	 ** checking against the auth database is inherently a slow process.
	 **/
	if (mssAuthenticate(usrname, passwd, cred_valid) < 0)
	    {
	    nht_i_AddResponseHeaderQPrintf(conn, "WWW-Authenticate", "Basic realm=%STR&DQUOT", NHT.Realm);
	    if (NHT.AuthMethods & NHT_AUTH_HTTPSTRICT)
		{
		nht_i_AddLoginHashCookie(conn);
		cxDebugLog("NHT: %s(%s): Failed login attempt, sending 401 and login cookie.", conn->IPAddr, usrname);
		}
	    else
		{
		cxDebugLog("NHT: %s(%s): Failed login attempt, sending 401.", conn->IPAddr, usrname);
		}
	    nht_i_WriteErrResponse(conn, 401, "Unauthorized", "<h1>Unauthorized</h1>\r\n");
	    mssError(1, "NHT", "Failed login attempt for user '%s'", usrname);
	    goto out;
	    }
	else
	    {
	    cred_valid = 1;
	    }

	/** Parse out the requested url **/
	url_inf = conn->ReqURL = htsParseURL(conn->URL);
	if (!url_inf)
	    {
	    cxDebugLog("NHT: %s(%s): Malformed URL.", conn->IPAddr, usrname);
	    nht_i_WriteErrResponse(conn, 500, "Internal Server Error", "<h1>500 Internal Server Error</h1>\r\n");
	    mssError(1,"NHT","Failed to handle HTTP request, exiting thread (could not parse URL).");
	    goto out;
	    }

	/** Validate akey (CSRF token / app linking token), if supplied **/
	if (conn->NhtSession)
	    {
	    akey_inf = stLookup_ne(url_inf, "cx__akey");
	    if (akey_inf)
		{
		if (nht_i_VerifyAKey(akey_inf->StrVal, conn->NhtSession, &group, &app) == 0)
		    {
		    akey_valid = 1;
		    if (app)
			conn->App = nht_i_LinkApp(app);
		    if (group)
			conn->AppGroup = nht_i_LinkAppGroup(group);
		    }
		}
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
		    if (akey_valid)
			{
			/** session key matched... now update app and group **/
			if (conn->App)
			    nht_i_ResetWatchdog(conn->App->WatchdogTimer);
			if (conn->AppGroup)
			    nht_i_ResetWatchdog(conn->AppGroup->WatchdogTimer);
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
		    i = nht_i_WatchdogTime(i_timer);
		    nht_i_QPrintfConn(conn, 0, "<A HREF=\"/\" TARGET='%STR&HTE'>%INT</A>\r\n", timestr, i);
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
	    /** Expired cookie?  Force logout. **/
	    if ((NHT.AuthMethods & NHT_AUTH_HTTPSTRICT) && nht_i_ZapSessionCookie(conn))
		{
		nht_i_AddResponseHeaderQPrintf(conn, "WWW-Authenticate", "Basic realm=%STR&DQUOT", NHT.Realm);
		nht_i_AddLoginHashCookie(conn);
		cxDebugLog("NHT: %s(%s): Expired session, forcing logout and sending login cookie.", conn->IPAddr, usrname);
		nht_i_WriteErrResponse(conn, 401, "Unauthorized", "<h1>Unauthorized</h1>\r\n");
		goto out;
		}

	    /** No session, and the connection is a "non-activity" request? This
	     ** check prevents creating a new session for "non-activity" after a
	     ** prior session has ended, instead we just treat the request as a
	     ** NOP.
	     **/
	    if (conn->NotActivity)
		{
		conn->NoCache = 1;
		nht_i_WriteResponse(conn, 200, "OK", "<A HREF=/ TARGET=ERR></A>\r\n");
		goto out;
		}

	    /** Authentication succeeded - start a new session **/
	    conn->NhtSession = nht_i_AllocSession(usrname, conn->UsingTLS);
	    printf("NHT: new session for username [%s], cookie [%s]\n", conn->NhtSession->Username, conn->NhtSession->Cookie);
	    }

	/** Start the application security context **/
	if (conn->NhtSession && conn->NhtSession->Session)
	    {
	    cxssPushContext();
	    context_started = 1;

	    /** If a valid akey was specified, resume the application **/
	    if (akey_valid && conn->AppGroup && conn->App)
		{
		appResume(conn->App->Application);
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
	    /** We require a valid CSRF token for use of these other-methods-
	     ** via-get mechanisms.
	     **/
	    if (!akey_valid)
		{
		nht_i_WriteResponse(conn, 200, "OK", "<A HREF=/ TARGET=ERR></A>\r\n");
		mssError(1, "NHT", "Invalid use of ls__method without akey");
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
		cxDebugLog("NHT: %s(%s): Method '%s' not implemented, sending 501", conn->IPAddr, usrname, conn->Method);
		nht_i_WriteErrResponse(conn, 501, "Not Implemented", "<h1>501 Method Not Implemented</h1>\r\n");
		}
	    }

	/** Catch-all, since we relocated the default response calls **/
	if (!conn->InBody)
	    {
	    cxDebugLog("NHT: %s(%s): No response available for request, sending 500", conn->IPAddr, usrname);
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
	cxDebugLog("NHT: %s: %s, sending 400.", conn->IPAddr, msg);
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

