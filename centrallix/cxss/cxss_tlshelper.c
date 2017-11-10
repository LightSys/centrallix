#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "config.h"
#include "centrallix.h"
#include "cxlib/xstring.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "cxss/cxss.h"
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2013 LightSys Technology Services, Inc.		*/
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
/* Module:	cxss (Centrallix Security Subsystem)                    */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	October 26, 2013                                        */
/*									*/
/* Description:	This file provides TLS helper functionality, by adding	*/
/*		TLS support to any connected network file descriptor.	*/
/************************************************************************/


/*** cxss_internal_DoTLS - perform the actual TLS negotiation and cryptography.
 ***/
int
cxss_internal_DoTLS(SSL_CTX* context, pFile encrypted_fd, pFile decrypted_fd, pFile report_fd, int as_server, char* remotename)
    {
    int pid, tm, ret, err;
    SSL* encrypted_conn;
    EventReq event0, event1;
    pEventReq ev[2] = { &event0, &event1 };
    enum { Idle=0, Data=1, Done=2 } locstate, netstate;
    enum { Read=0, Write=1, Try=2 } locsslstate, netsslstate;
    char SSLBuf[256];
    int SSLBytes;
    char LocBuf[256];
    int LocBytes;
    int cnt;

	/** Add to the PRNG **/
	pid = getpid();
	RAND_add(&pid, 4, (double)0.25);
	tm = time(NULL);
	RAND_add(&tm, 4, (double)0.125);

	/** get an SSL context and connection **/
	encrypted_conn = SSL_new(context);
	if (!encrypted_conn) return -1;
	SSL_set_fd(encrypted_conn, fdFD(encrypted_fd));

	/** SNI? **/
	if (remotename && remotename[0] && !as_server)
	    SSL_set_tlsext_host_name(encrypted_conn, remotename);

	/** start the handshake **/
	while (1)
	    {
	    if (as_server)
		ret = SSL_accept(encrypted_conn);
	    else
		ret = SSL_connect(encrypted_conn);
	    if (ret > 0) break;
	    err = SSL_get_error(encrypted_conn, ret);
	    if (err == SSL_ERROR_WANT_READ)
		{
		thWait(PMTOBJECT(encrypted_fd), OBJ_T_FD, EV_T_FD_READ, 1);
		continue;
		}
	    else if (err == SSL_ERROR_WANT_WRITE)
		{
		thWait(PMTOBJECT(encrypted_fd), OBJ_T_FD, EV_T_FD_WRITE, 1);
		continue;
		}
	    else
		{
		err = ERR_get_error();
		fdPrintf(report_fd, "!Abnormal SSL termination from network during handshake: %s\n", ERR_error_string(err, NULL));
		return -1;
		}
	    }

	/** Notify parent process of security type negotiated **/
	fdPrintf(report_fd, ".%s %s %dbit\n",
		SSL_get_cipher_version(encrypted_conn),
		SSL_get_cipher_name(encrypted_conn),
		SSL_get_cipher_bits(encrypted_conn, NULL));

	/** Ok, handshake done.  Now transfer the data. **/
	locstate = Idle;
	netstate = Idle;
	locsslstate = Try;
	netsslstate = Try;
	while(locstate != Done && netstate != Done)
	    {
	    /** Set up the two event objects **/
	    event0.ObjType = OBJ_T_FD;
	    event0.ReqLen = 1;
	    event1.ObjType = OBJ_T_FD;
	    event1.ReqLen = 1;

	    /** Data transfer is bidirectional.  States per direction: idle, data, and done. **/
	    switch(locstate)
		{
		case Idle:
		    event0.Object = decrypted_fd;
		    event0.EventType = EV_T_FD_READ;
		    break;
		case Data:
		    if (locsslstate == Try)
			{
			event0.Object = NULL;
			event0.EventType = EV_T_MT_TIMER;
			event0.ReqLen = 0;
			event0.ObjType = OBJ_T_MTASK;
			}
		    else
			{
			event0.Object = encrypted_fd;
			event0.EventType = (locsslstate == Read)?EV_T_FD_READ:EV_T_FD_WRITE;
			}
		    break;
		case Done:
		    /* unreachable */
		    break;
		}
	    switch(netstate)
		{
		case Idle:
		    if (netsslstate == Try)
			{
			event1.Object = NULL;
			event1.EventType = EV_T_MT_TIMER;
			event1.ReqLen = 0;
			event1.ObjType = OBJ_T_MTASK;
			}
		    else
			{
			event1.Object = encrypted_fd;
			event1.EventType = (netsslstate == Read)?EV_T_FD_READ:EV_T_FD_WRITE;
			}
		    break;
		case Data:
		    event1.Object = decrypted_fd;
		    event1.EventType = EV_T_FD_WRITE;
		    break;
		case Done:
		    /* unreachable */
		    break;
		}

	    /** Wait on events **/
	    thMultiWait(2, ev);

	    /** What completed? **/
	    if (event0.Status == EV_S_ERROR)
		{
		locstate = Done;
		}
	    else if (event0.Status == EV_S_COMPLETE)
		{
		switch(locstate)
		    {
		    case Idle:
			LocBytes = fdRead(decrypted_fd, LocBuf, sizeof(LocBuf), 0, 0);
			if (LocBytes > 0)
			    {
			    locstate = Data;
			    locsslstate = Try;
			    }
			else
			    {
			    locstate = Done;
			    }
			break;
		    case Data:
			ret = SSL_write(encrypted_conn, LocBuf, LocBytes);
			if (ret <= 0)
			    {
			    err = SSL_get_error(encrypted_conn, ret);
			    if (err == SSL_ERROR_WANT_WRITE)
				locsslstate = Write;
			    else if (err == SSL_ERROR_WANT_READ)
				locsslstate = Read;
			    else if (err == SSL_ERROR_ZERO_RETURN)
				locstate = Done;
			    else
				{
				err = ERR_get_error();
				locstate = Done;
				fdPrintf(report_fd, "!Abnormal SSL termination from network during SSL_write: %s\n", ERR_error_string(err, NULL));
				}
			    }
			else /** openssl docs: SSL_write writes entire buffer **/
			    {
			    locstate = Idle;
			    locsslstate = Try;
			    }
			break;
		    case Done:
			/* unreachable */
			break;
		    }
		}
	    if (event1.Status == EV_S_ERROR)
		{
		netstate = Done;
		}
	    else if (event1.Status == EV_S_COMPLETE)
		{
		switch(netstate)
		    {
		    case Idle:
			SSLBytes = SSL_read(encrypted_conn, SSLBuf, sizeof(SSLBuf));
			if (SSLBytes > 0)
			    {
			    netstate = Data;
			    netsslstate = Try;
			    }
			else
			    {
			    err = SSL_get_error(encrypted_conn, SSLBytes);
			    if (err == SSL_ERROR_WANT_WRITE)
				netsslstate = Write;
			    else if (err == SSL_ERROR_WANT_READ)
				netsslstate = Read;
			    else if (err == SSL_ERROR_ZERO_RETURN)
				netstate = Done;
			    else
				{
				err = ERR_get_error();
				netstate = Done;
				if (err != 0)
				    fdPrintf(report_fd, "!Abnormal SSL termination from network during SSL_read: %s\n", ERR_error_string(err, NULL));
				}
			    }
			break;
		    case Data:
			ret = fdWrite(decrypted_fd, SSLBuf, SSLBytes, 0, FD_U_PACKET);
			if (ret <= 0)
			    {
			    netstate = Done;
			    }
			else
			    {
			    netstate = Idle;
			    netsslstate = Try;
			    }
			break;
		    case Done:
			/* unreachable */
			break;
		    }
		}
	    }

	/** Now shut down the connection. **/
	fdClose(decrypted_fd, 0);
	cnt = 0;
	while ((ret = SSL_shutdown(encrypted_conn)) <= 0)
	    {
	    err = SSL_get_error(encrypted_conn, ret);
	    if (err == SSL_ERROR_WANT_READ)
		{
		thWait(PMTOBJECT(encrypted_fd), OBJ_T_FD, EV_T_FD_READ, 1);
		continue;
		}
	    else if (err == SSL_ERROR_WANT_WRITE)
		{
		thWait(PMTOBJECT(encrypted_fd), OBJ_T_FD, EV_T_FD_WRITE, 1);
		continue;
		}
	    else if (err == SSL_ERROR_ZERO_RETURN)
		{
		/** ok! **/
		break;
		}
	    else
		{
		cnt++;
		if (ret == 0 && cnt == 1) continue; /* shutdown both ways per openssl docs */
		if (ret == 0 && cnt == 2)
		    {
		    /* try to recover */
		    thSleep(1000);
		    continue;
		    }
		err = ERR_get_error();
		if (err == 0) return 0; /* no error */
		fdPrintf(report_fd, "!Abnormal SSL termination from network during connection shutdown: %s\n", ERR_error_string(err, NULL));
		return -1;
		}
	    }

    return 0;
    }


/*** cxssStartTLS - start TLS on the current connection.  This creates
 *** a subprocess to do the TLS stuff (via OpenSSL), and generates a
 *** new ext_conn so that the subprocess talks to the network, and
 *** this process reads/writes unencrypted data to/from the subprocess
 *** instead.
 ***
 *** On return, sets ext_conn to the new connection that the process
 *** should comunicate through, and reporting_stream to a file descriptor
 *** that provides status/error reporting from the SSL helper.  Returns
 *** the PID of the SSL helper process.
 ***/
int
cxssStartTLS(SSL_CTX* context, pFile* ext_conn, pFile* reporting_stream, int as_server, char* remotename)
    {
    int pid;
    int fds[2];
    pFile mainprocess_fd, subprocess_fd;
    pFile mainprocess_report_fd, subprocess_report_fd;

	/** Create two pipes **/
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
	    return -1;
	subprocess_fd = fdOpenFD(fds[0], O_RDWR);
	mainprocess_fd = fdOpenFD(fds[1], O_RDWR);
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
	    return -1;
	subprocess_report_fd = fdOpenFD(fds[0], O_RDWR);
	mainprocess_report_fd = fdOpenFD(fds[1], O_RDWR);

	/** fork a subprocess **/
	pid = fork();
	if (pid < 0) return -1;
	if (pid == 0)
	    {
	    /** subprocess **/
	    thLock();
	    fdClose(mainprocess_fd, 0);
	    fdClose(mainprocess_report_fd, 0);
	    cxss_internal_DoTLS(context, *ext_conn, subprocess_fd, subprocess_report_fd, as_server, remotename);
	    _exit(0);
	    }
	else
	    {
	    /** main process **/
	    fdClose(subprocess_fd, 0);
	    fdClose(subprocess_report_fd, 0);
	    fdClose(*ext_conn, 0);
	    *ext_conn = mainprocess_fd;
	    *reporting_stream = mainprocess_report_fd;
	    }

    return pid;
    }


/*** cxssFinishTLS - wraps up the TLS connection.
 ***/
int
cxssFinishTLS(int childpid, pFile ext_conn, pFile reporting_stream)
    {
    int child_status;
    int rval;
    int interval;

	/** Close up the streams.  We linger for 5 seconds on the
	 ** ext_conn to make sure things are flushed out.
	 **/
	fdClose(ext_conn, 5000);
	fdClose(reporting_stream, 0);

	/** Shutdown and/or wait for the TLS helper process **/
	interval = 1;
	while(interval < 128)
	    {
	    rval = waitpid(childpid, &child_status, WNOHANG);
	    if (rval > 0 && WIFEXITED(child_status))
		return 0;
	    thSleep(interval);
	    interval *= 2;
	    }
	kill(childpid, SIGTERM);
	thSleep(500);
	rval = waitpid(childpid, &child_status, WNOHANG);
	if (rval > 0 && WIFEXITED(child_status))
	    return 0;
	kill(childpid, SIGKILL);
	thSleep(500);
	waitpid(childpid, &child_status, 0);

    return 0;
    }


/*** cxssStatTLS - read a status message from the status report
 *** pipe.
 ***/
int
cxssStatTLS(pFile reporting_stream, char* status, int maxlen)
    {
    char ch;
    int rval;
    int pos = 0;

	/** Read until a newline **/
	while(1)
	    {
	    rval = fdRead(reporting_stream, &ch, 1, 0, 0);
	    if (rval <= 0)
		return -1;
	    if (ch == '\n')
		break;
	    if (pos < maxlen-1)
		status[pos++] = ch;
	    }
	status[pos] = '\0';

    return 0;
    }

