#include "net_ldap.h"
#include "cxss/cxss.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2017 LightSys Technology Services, Inc.		*/
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
/* Module: 	net_ldap.h, net_ldap.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	April 28, 2017    					*/
/* Description:	Network handler providing an LDAP interface to the 	*/
/*		Centrallix and selected parts of the ObjectSystem.	*/
/************************************************************************/

#define CXLBER_SB_OPT_SET_CONN		(255)


/*** nldap_i_LberRead - function allowing the OpenLDAP LBER library to read
 *** data from an MTask network connection.
 ***/
ber_slen_t
nldap_i_LberRead(Sockbuf_IO_Desc* sbiod, void* buf, ber_len_t len)
    {
    pNldapConn conn = (pNldapConn)(sbiod->sbiod_pvt);
    return fdRead(conn->ConnFD, (char*)buf, len, 0, 0);
    }


/*** nldap_i_LberWrite - function allowing the OpenLDAP LBER library to
 *** write data to an MTask network connection.
 ***/
ber_slen_t
nldap_i_LberWrite(Sockbuf_IO_Desc* sbiod, void* buf, ber_len_t len)
    {
    pNldapConn conn = (pNldapConn)(sbiod->sbiod_pvt);
    return fdWrite(conn->ConnFD, (char*)buf, len, 0, 0);
    }


/*** nldap_i_LberCtrl - set control options.  The main thing we need to
 *** do here is to allow passing down the NldapConn structure so the
 *** read and write functions can access the network connection.
 ***/
int
nldap_i_LberCtrl(Sockbuf_IO_Desc* sbiod, int opt, void* arg)
    {
    pNldapConn conn = (pNldapConn)arg;

	/** Pass on down the NldapConn information **/
	if (opt == CXLBER_SB_OPT_SET_CONN)
	    {
	    sbiod->sbiod_pvt = (void*)conn;
	    return 1;
	    }

    return 0;
    }


/*** nldap_i_Log - generate an access logfile entry
 ***/
int
nldap_i_Log(pNldapConn conn)
    {
    struct tm* thetime;
    time_t tval;

	/** logging not available? **/
	if (!NLDAP.AccessLogFD)
	    return 0;

	/** Record the current time **/
	tval = time(NULL);
	thetime = localtime(&tval);
	strftime(conn->ResponseTime, sizeof(conn->ResponseTime), "%d/%a/%Y:%T %z", thetime);

	/** Print the log message **/
	fdQPrintf(NLDAP.AccessLogFD, 
		"%STR %STR&DQUOT [%STR] %INT %INT\n",
		conn->IPAddr,
		conn->RequestorDN,
		conn->ResponseTime,
		conn->RequestID,
		conn->AppID
		);


    return 0;
    }


/*** nldap_i_HandleRequest() - read one asn1 request from the network
 *** and respond to it.
 ***/
int
nldap_i_HandleRequest(pNldapConn conn)
    {
    ber_tag_t tag;
    ber_len_t len;
    BerElement* ber;
    char* auth;

	/** Read the protocol message from the network **/
	ber = ber_alloc();
	if (!ber)
	    goto error;
	tag = ber_get_next(conn->LberSockbuf, &len, ber);
	if (tag != LBER_SEQUENCE)
	    goto error;

	/** Request ID **/
	tag = ber_get_int(ber, &conn->RequestID);
	if (tag != LBER_INTEGER)
	    goto error;

	/** Application ID (request type) **/
	tag = ber_scanf(ber, "{}");
	if ((tag & LBER_CLASS_MASK) != LBER_CLASS_APPLICATION || (tag & LBER_ENCODING_MASK) != LBER_CONSTRUCTED)
	    goto error;
	tag &= ~(LBER_CLASS_MASK | LBER_ENCODING_MASK);
	conn->AppID = tag;

	/** Handle various application packet types **/
	switch(conn->AppID)
	    {
	    case 0:	/* Bind Request */
		tag = ber_get_int(ber, &conn->Version);
		if (tag != LBER_INTEGER)
		    goto error;
		if (conn->Version != 3)
		    {
		    mssError(1, "NLDAP", "Only LDAP protocol version 3 supported (version %d requested)", conn->Version);
		    goto error;
		    }
		tag = ber_scanf(ber, "a", &conn->RequestorDN);
		if (tag != LBER_OCTETSTRING)
		    goto error;
		tag = ber_peek_tag(ber, &len);
		switch(tag)
		    {
		    case LBER_OCTETSTRING:
			/** Simple auth **/
			tag = ber_scanf(ber, "a", &auth);
			if (tag != LBER_OCTETSTRING)
			    goto error;
			strtcpy(conn->Password, auth, sizeof(conn->Password));
			break;

		    case LBER_CLASS_CONTEXT:
			/** SASL object **/
			if (len == 0)
			    {
			    /** no auth given **/
			    break;
			    }
			mssError(1, "NLDAP", "SASL authentication is not supported");
			goto error;

		    default:
			mssError(1, "NLDAP", "Unsupported authentication method requested");
			goto error;
		    }
		break;

	    default:
		mssError(1, "NLDAP", "Unsupported LDAP application message ID %d", conn->AppID);
		goto error;
	    }

	/** Log the request **/
	nldap_i_Log(conn);

	return 0;

    error:
	return -1;
    }


/*** nldap_i_ConnHandler - manages a single incoming LDAP connection
 *** and processes the connection's request.
 ***/
void
nldap_i_ConnHandler(void* conn_v)
    {
    pNldapConn conn = (pNldapConn)conn_v;
    int rval;

	/** Set up the LBER socket buffer **/
	conn->LberSockbuf = ber_sockbuf_alloc();
	if (!conn->LberSockbuf)
	    goto error;
	conn->LberIo.sbi_read = nldap_i_LberRead;
	conn->LberIo.sbi_write = nldap_i_LberWrite;
	conn->LberIo.sbi_ctrl = nldap_i_LberCtrl;
	ber_sockbuf_add_io(conn->LberSockbuf, &conn->LberIo, LBER_SBIOD_LEVEL_PROVIDER, (void*)conn);
	ber_sockbuf_ctrl(conn->LberSockbuf, CXLBER_SB_OPT_SET_CONN, (void*)conn);

	/** Loop, reading messages **/
	while ((rval = nldap_i_HandleRequest(conn)))
	    {
	    if (rval < 0)
		goto error;
	    if (rval == 1)
		break;
	    }

    error:
	nldap_i_FreeConn(conn);
	thExit(); /* no return */
    }

 
/*** nldap_i_CheckAccessLog - check to see if the access log needs to be
 *** (re)opened.
 ***/
int
nldap_i_CheckAccessLog()
    {
    pFile test_fd;

	/** Is it open yet? **/
	if (!NLDAP.AccessLogFD)
	    {
	    NLDAP.AccessLogFD = fdOpen(NLDAP.AccessLogFile, O_CREAT | O_WRONLY | O_APPEND, 0600);
	    if (!NLDAP.AccessLogFD)
		{
		mssErrorErrno(1,"NLDAP","Could not open access_log file '%s'", NLDAP.AccessLogFile);
		}
	    }
	else
	    {
	    /** Has file been renamed (e.g., by logrotate) **/
	    test_fd = fdOpen(NLDAP.AccessLogFile, O_RDONLY, 0600);
	    if (!test_fd)
		{
		test_fd = fdOpen(NLDAP.AccessLogFile, O_CREAT | O_WRONLY | O_APPEND, 0600);
		if (!test_fd)
		    {
		    mssErrorErrno(1,"NLDAP","Could not reopen access log file '%s'", NLDAP.AccessLogFile);
		    }
		else
		    {
		    fdClose(NLDAP.AccessLogFD, 0);
		    NLDAP.AccessLogFD = test_fd;
		    }
		}
	    else
		{
		fdClose(test_fd, 0);
		}
	    }

    return 0;
    }


/*** nldap_i_AllocConn() - allocates a connection structure and
 *** initializes it given a network connection.
 ***/
pNldapConn
nldap_i_AllocConn(pFile net_conn)
    {
    pNldapConn conn;
    char* remoteip;

	/** Allocate and zero-out the structure **/
	conn = (pNldapConn)nmMalloc(sizeof(NldapConn));
	if (!conn) return NULL;
	memset(conn, 0, sizeof(NldapConn));
	conn->ConnFD = net_conn;

	/** Get the remote IP and port **/
	remoteip = netGetRemoteIP(net_conn, NET_U_NOBLOCK);
	if (remoteip) strtcpy(conn->IPAddr, remoteip, sizeof(conn->IPAddr));
	conn->Port = netGetRemotePort(net_conn);

    return conn;
    }


/*** nldap_i_FreeConn() - releases a connection structure and 
 *** closes the associated network connection.
 ***/
int
nldap_i_FreeConn(pNldapConn conn)
    {

	/** Free up connection metadata **/
	if (conn->RequestorDN)
	    ber_memfree(conn->RequestorDN);

	/** Clean up the LBER interface **/
	if (conn->LberSockbuf)
	    {
	    ber_sockbuf_remove_io(conn->LberSockbuf, &conn->LberIo, LBER_SBIOD_LEVEL_PROVIDER);
	    ber_sockbuf_free(conn->LberSockbuf);
	    }

	/** Close the connection **/
	if (conn->SSLpid)
	    cxssFinishTLS(conn->SSLpid, conn->ConnFD, conn->ReportingFD);
	else
	    netCloseTCP(conn->ConnFD, 1000, 0);

	/** Release the connection structure **/
	nmFree(conn, sizeof(NldapConn));

    return 0;
    }

/*** nldap_i_TLSHandler - manages incoming TLS-encrypted HTTP
 *** connections.
 ***/
void
nldap_i_TLSHandler(void* v)
    {
    pFile listen_socket;
    pFile connection_socket;
    pStructInf my_config;
    char listen_port[32];
    char* strval;
    int intval;
    char* ptr;
    pNldapConn conn;
    FILE* fp;
    X509* cert;
    int sslflags;

	/** Set the thread's name **/
	thSetName(NULL,"LDAP Network Listener");

	/** Get our configuration **/
	strcpy(listen_port, "636");
	my_config = stLookup(CxGlobals.ParsedConfig, "net_ldap");
	if (my_config)
	    {
	    /** Got the config.  Now lookup what the TCP port is that we listen on **/
	    strval=NULL;
	    if (stAttrValue(stLookup(my_config, "ssl_listen_port"), &intval, &strval, 0) >= 0)
		{
		if (strval)
		    strtcpy(listen_port, strval, sizeof(listen_port));
		else
		    snprintf(listen_port, sizeof(listen_port), "%d", intval);
		}
	    }

	/** Set up OpenSSL **/
	NLDAP.SSL_ctx = SSL_CTX_new(SSLv23_server_method());
	if (!NLDAP.SSL_ctx)
	    {
	    mssError(1,"NLDAP","Could not initialize SSL library");
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
	SSL_CTX_set_options(NLDAP.SSL_ctx, sslflags);

	/** Cipher list -- check at both top level and at net_ldap module level **/
	if (stAttrValue(stLookup(CxGlobals.ParsedConfig, "ssl_cipherlist"),NULL,&ptr,0) < 0)
	    ptr="DEFAULT";
	SSL_CTX_set_cipher_list(NLDAP.SSL_ctx, ptr);
	if (stAttrValue(stLookup(my_config, "ssl_cipherlist"),NULL,&ptr,0) == 0)
	    SSL_CTX_set_cipher_list(NLDAP.SSL_ctx, ptr);

	/** set the server certificate and key **/
	if (stAttrValue(stLookup(my_config,"ssl_cert"), NULL, &strval, 0) != 0)
	    strval = "/usr/local/etc/centrallix/snakeoil.crt";
	if (SSL_CTX_use_certificate_file(NLDAP.SSL_ctx, strval, SSL_FILETYPE_PEM) != 1)
	    {
	    mssError(1,"HTTP","Could not load certificate %s.", strval);
	    thExit();
	    }
	if (stAttrValue(stLookup(my_config,"ssl_key"), NULL, &strval, 0) != 0)
	    strval = "/usr/local/etc/centrallix/snakeoil.key";
	if (SSL_CTX_use_PrivateKey_file(NLDAP.SSL_ctx, strval, SSL_FILETYPE_PEM) != 1)
	    {
	    mssError(1,"HTTP","Could not load key %s.", strval);
	    thExit();
	    }
	if (SSL_CTX_check_private_key(NLDAP.SSL_ctx) != 1)
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
		    if (!SSL_CTX_add_extra_chain_cert(NLDAP.SSL_ctx, cert))
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
	    mssErrorErrno(1,"NLDAP","Could not open TLS network listener");
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

	    /** Check reopen **/
	    nldap_i_CheckAccessLog();

	    /** Set up the connection structure **/
	    conn = nldap_i_AllocConn(connection_socket);
	    if (!conn)
		{
		netCloseTCP(connection_socket, 1000, 0);
		thSleep(1);
		continue;
		}

	    /** Start TLS on the connection.  This replaces conn->ConnFD with
	     ** a pipe to the TLS encryption/decryption process.
	     **/
	    conn->SSLpid = cxssStartTLS(NLDAP.SSL_ctx, &conn->ConnFD, &conn->ReportingFD, 1, NULL);
	    if (conn->SSLpid <= 0)
		{
		nldap_i_FreeConn(conn);
		mssError(1,"NLDAP","Could not start TLS on the connection!");
		}

	    /** Start the request handler thread **/
	    if (!thCreate(nldap_i_ConnHandler, 0, conn))
	        {
		nldap_i_FreeConn(conn);
		mssError(1,"NLDAP","Could not create thread to handle TLS connection!");
		}
	    }

	/** Exit. **/
	mssError(1,"NLDAP","Could not continue to accept TLS requests.");
	netCloseTCP(listen_socket,0,0);

    thExit();
    }


/*** nldap_i_Handler - manages incoming HTTP connections and sends
 *** the appropriate documents/etc to the requesting client.
 ***/
void
nldap_i_Handler(void* v)
    {
    pFile listen_socket;
    pFile connection_socket;
    pStructInf my_config;
    char listen_port[32];
    char* strval;
    int intval;
    pNldapConn conn;

	/** Set the thread's name **/
	thSetName(NULL,"HTTP Network Listener");

	/** Get our configuration **/
	strcpy(listen_port,"389");
	my_config = stLookup(CxGlobals.ParsedConfig, "net_ldap");
	if (my_config)
	    {
	    /** Got the config.  Now lookup what the TCP port is that we listen on **/
	    strval=NULL;
	    if (stAttrValue(stLookup(my_config, "listen_port"), &intval, &strval, 0) >= 0)
		{
		if (strval)
		    strtcpy(listen_port, strval, sizeof(listen_port));
		else
		    snprintf(listen_port, sizeof(listen_port),"%d",intval);
		}
	    }

    	/** Open the server listener socket. **/
	listen_socket = netListenTCP(listen_port, 32, 0);
	if (!listen_socket) 
	    {
	    mssErrorErrno(1,"NLDAP","Could not open network listener");
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

	    /** Check reopen **/
	    nldap_i_CheckAccessLog();

	    /** Set up the connection structure **/
	    conn = nldap_i_AllocConn(connection_socket);
	    if (!conn)
		{
		netCloseTCP(connection_socket, 1000, 0);
		thSleep(1);
		continue;
		}

	    /** Start the request handler thread **/
	    if (!thCreate(nldap_i_ConnHandler, 0, conn))
	        {
		nldap_i_FreeConn(conn);
		mssError(1,"NLDAP","Could not create thread to handle connection!");
		}
	    }

	/** Exit. **/
	mssError(1,"NLDAP","Could not continue to accept requests.");
	netCloseTCP(listen_socket,0,0);

    thExit();
    }


/*** nldapInitialize - initialize the LDAP network handler and start the 
 *** listener thread.
 ***/
int
nldapInitialize()
    {
    pStructInf my_config;
    char* strval;

	/** Initialize globals **/
	memset(&NLDAP, 0, sizeof(NLDAP));
	NLDAP.InactivityTime = 300;
	NLDAP.RestrictToLocalhost = 0;

#ifdef _SC_CLK_TCK
        NLDAP.ClkTck = sysconf(_SC_CLK_TCK);
#else
        NLDAP.ClkTck = CLK_TCK;
#endif

	/** Read configuration data **/
	my_config = stLookup(CxGlobals.ParsedConfig, "net_ldap");
	if (my_config)
	    {
	    stAttrValue(stLookup(my_config, "accept_localhost_only"), &(NLDAP.RestrictToLocalhost), NULL, 0);

	    /** Get the timer settings **/
	    stAttrValue(stLookup(my_config, "inactivity_timer"), &(NLDAP.InactivityTime), NULL, 0);

	    /** Access log file **/
	    if (stAttrValue(stLookup(my_config, "access_log"), NULL, &strval, 0) >= 0)
		{
		strtcpy(NLDAP.AccessLogFile, strval, sizeof(NLDAP.AccessLogFile));
		nldap_i_CheckAccessLog();
		}
	    }

	/** Start the watchdog timer thread **/
	//thCreate(nldap_i_Watchdog, 0, NULL);

	/** Start the network listeners. **/
	thCreate(nldap_i_Handler, 0, NULL);
	thCreate(nldap_i_TLSHandler, 0, NULL);

    return 0;
    }

    
MODULE_INIT(nldapInitialize);
MODULE_PREFIX("nldap");
MODULE_DESC("LDAP Network Driver");
MODULE_VERSION(0,1,0);
MODULE_IFACE(CX_CURRENT_IFACE);
