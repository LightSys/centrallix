#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "bdqs.h"
#include "mtask.h"
#include "mtsession.h"
#include "newmalloc.h"
#include "xarray.h"
#include "xstring.h"
#include "xhash.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 2001-2002 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	bdqs.c, bdqs.h          				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 18, 2002    					*/
/* Description:	This module implements the Batchable Data Query Service,*/
/*		which is Centrallix's raw communication protocol.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: bdqs_transport.c,v 1.1 2002/03/23 06:25:09 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/src/bdqs_transport.c,v $

    $Log: bdqs_transport.c,v $
    Revision 1.1  2002/03/23 06:25:09  gbeeley
    Updated MSS to have a larger error string buffer, as a lot of errors
    were getting chopped off.  Added BDQS protocol files with some very
    simple initial implementation.

 **END-CVSDATA***********************************************************/


/*** bdqs_internal_ReadConnectString() - reads the connect string from the
 *** other end, which should be two lines (the second one blank), and parses
 *** the options into the appropriate flags in the connection structure.  Works
 *** for client or server.  Returns -1 on error, 0 on success.
 ***/
int
bdqs_internal_ReadConnectString(pBDQSTransportConn connection, char* strbuf, int buflen)
    {
    int cnt, rcnt;
    char ch;
    char* ptr;
    char* ext;
    int flags = 0;

	/** Buffer not large enough? **/
	if (buflen < 64) return -1;

	/** Read the first line. **/
	cnt = 0;
	while(1)
	    {
	    /** Read a single character **/
	    rcnt = connection->TransportReadFn(connection->TransportReadArg, strbuf+cnt, 1, 0, FD_U_PACKET);
	    if (rcnt != 1) 
		{
		mssError(1,"BDQS","Error in connection handshake (as %s)", connection->IsServer?"server":"client");
		return -1;
		}
	    if (strbuf[cnt] == '\n') break;
	    cnt++;
	    if (cnt >= buflen)
		{
		mssError(1,"BDQS","Bark!  Remote %s exceeded connect string buffer.", connection->IsServer?"client":"server");
		strbuf[buflen-1] = '\0';
		return -1;
		}
	    }
	strbuf[cnt] = '\0';
	if (cnt > 0 && strbuf[cnt-1] == '\r') strbuf[cnt-1] = '\0';

	/** Next chars should be \r\n or just \n... **/
	rcnt = connection->TransportReadFn(connection->TransportReadArg, &ch, 1, 0, FD_U_PACKET);
	if (rcnt != 1 || (ch != '\r' && ch != '\n'))
	    {
	    mssError(1,"BDQS","Error in connection handshake (as %s)", connection->IsServer?"server":"client");
	    return -1;
	    }
	if (ch == '\r')
	    {
	    rcnt = connection->TransportReadFn(connection->TransportReadArg, &ch, 1, 0, FD_U_PACKET);
	    if (rcnt != 1 || ch != '\n')
		{
		mssError(1,"BDQS","Error in connection handshake (as %s)", connection->IsServer?"server":"client");
		return -1;
		}
	    }

	/** Ok, apparently valid sequence so far.  Now parse it. **/
	if (strncmp(strbuf,"STARTBDQS /1.0/", 15))
	    {
	    mssError(1,"BDQS","Error in connection handshake (as %s)", connection->IsServer?"server":"client");
	    return -1;
	    }
	ptr = strbuf+15;
	ext = strtok(ptr, "/");
	while(ext)
	    {
	    if (!strcmp(ext,"LE")) flags |= BDQS_PROTO_F_LSB;
	    else if (!strcmp(ext,"MULTIQUERY")) flags |= BDQS_PROTO_F_MULTIQUERY;
	    else if (!strcmp(ext,"SELECT")) flags |= BDQS_PROTO_F_SELECT;
	    else if (!strcmp(ext,"COLLATE")) flags |= BDQS_PROTO_F_COLLATE;
	    else if (!strcmp(ext,"EVAL")) flags |= BDQS_PROTO_F_EVAL;
	    else if (!strcmp(ext,"EVENTS")) flags |= BDQS_PROTO_F_EVENTS;
	    else if (!strncmp(ext,"MTU=",4)) connection->RemoteMTU = strtoul(ext+4, NULL, 10);
	    ext = strtok(NULL, "/");
	    }
	connection->RemoteFlags = flags;

    return 0;
    }


/*** bdqs_internal_ReadBatchHeader - This function reads a batch header from
 *** the given transport connection.  It allocates one if one is not provided.
 ***/
int
bdqs_internal_ReadBatchHeader(pBDQSTransportConn conn, pBDQSBatchHeader* hdr)
    {
    int rcnt;
    pBDQSBatchHeader my_hdr;
    pBDQSAuthSession s;
    
	/** Alloc a new header? **/
	if (!*hdr)
	    {
	    my_hdr = (pBDQSBatchHeader)nmMalloc(sizeof(BDQSBatchHeader));
	    if (!my_hdr) return BDQS_ERR_INTERNAL;
	    }
	else
	    {
	    my_hdr = *hdr;
	    }

	/** Read it from the connection. **/
	rcnt = conn->TransportReadFn(conn->TransportReadArg, my_hdr, sizeof(BDQSBatchHeader), 0, FD_U_PACKET);
	if (rcnt != sizeof(BDQSBatchHeader))
	    {
	    if (!*hdr) nmFree(my_hdr, sizeof(BDQSBatchHeader));
	    return BDQS_ERR_INTERNAL;
	    }

	/** Convert it to local byte order if needed **/
	if ((conn->RemoteFlags & BDQS_PROTO_F_LSB) != BDQS_LSBFIRST)
	    {
	    my_hdr->BatchID = BDQS_CONV64(my_hdr->BatchID);
	    my_hdr->FrameID = BDQS_CONV32(my_hdr->FrameID);
	    my_hdr->FrameLength = BDQS_CONV32(my_hdr->FrameLength);
	    my_hdr->nCommands = BDQS_CONV32(my_hdr->nCommands);
	    my_hdr->Flags = BDQS_CONV32(my_hdr->Flags);
	    my_hdr->SessionID = BDQS_CONV32(my_hdr->SessionID);
	    my_hdr->BatchRefID = BDQS_CONV64(my_hdr->BatchRefID);
	    }

	/** Lookup the session **/
	if (my_hdr->SessionID == -1)
	    {
	    conn->CurSessionContext = NULL;
	    }
	else
	    {
	    s = conn->AuthSessionList;
	    while(s && s->SessionID != my_hdr->SessionID)
		{
		s=s->Next;
		}
	    if (!s)
		{
		if (!*hdr) nmFree(my_hdr, sizeof(BDQSBatchHeader));
		return BDQS_ERR_NOSESSION;
		}
	    conn->CurSessionContext = s;
	    }

	/** Verify sequence ID **/
	if (my_hdr->BatchID == conn->RecvBatchSeqID)
	    {
	    if (my_hdr->FrameID != conn->RecvFrameSeqID + 1)
		{
		if (!*hdr) nmFree(my_hdr, sizeof(BDQSBatchHeader));
		return BDQS_ERR_PROTOCOL;
		}
	    }
	else if (my_hdr->BatchID != conn->RecvBatchSeqID + 1)
	    {
	    if (!*hdr) nmFree(my_hdr, sizeof(BDQSBatchHeader));
	    return BDQS_ERR_PROTOCOL;
	    }
	else if (my_hdr->FrameID != 1)
	    {
	    if (!*hdr) nmFree(my_hdr, sizeof(BDQSBatchHeader));
	    return BDQS_ERR_PROTOCOL;
	    }
	conn->RecvBatchSeqID = my_hdr->BatchID;
	conn->RecvFrameSeqID = my_hdr->FrameID;
	conn->RecvMsgSeqID = 0;

	*hdr = my_hdr;

    return 0;
    }


/*** bdqs_internal_ReadMessageHeader - This function reads a message header
 *** from the given transport connection, allocating one if space is not 
 *** already provided for one.
 ***/
int
bdqs_internal_ReadMessageHeader(pBDQSTransportConn conn, pBDQSMessageHeader* hdr)
    {
    int rcnt;
    pBDQSMessageHeader my_hdr;
    
	/** Alloc a new header? **/
	if (!*hdr)
	    {
	    my_hdr = (pBDQSMessageHeader)nmMalloc(sizeof(BDQSMessageHeader));
	    if (!my_hdr) return BDQS_ERR_INTERNAL;
	    }
	else
	    {
	    my_hdr = *hdr;
	    }

	/** Read it from the connection. **/
	rcnt = conn->TransportReadFn(conn->TransportReadArg, my_hdr, sizeof(BDQSMessageHeader), 0, FD_U_PACKET);
	if (rcnt != sizeof(BDQSMessageHeader))
	    {
	    if (!*hdr) nmFree(my_hdr, sizeof(BDQSMessageHeader));
	    return BDQS_ERR_INTERNAL;
	    }
	
	/** Convert it to local byte order if needed **/
	if ((conn->RemoteFlags & BDQS_PROTO_F_LSB) != BDQS_LSBFIRST)
	    {
	    my_hdr->MsgType = BDQS_CONV16(my_hdr->MsgType);
	    my_hdr->Flags = BDQS_CONV16(my_hdr->Flags);
	    my_hdr->MessageID = BDQS_CONV32(my_hdr->MessageID);
	    my_hdr->nParams = BDQS_CONV32(my_hdr->nParams);
	    }

	/** Verify the message sequence id **/
	if (my_hdr->MessageID != conn->RecvMsgSeqID + 1)
	    {
	    if (!*hdr) nmFree(my_hdr, sizeof(BDQSMessageHeader));
	    return BDQS_ERR_PROTOCOL;
	    }

	/** Verify message type is correct **/
	if (my_hdr->MsgType < 0 || my_hdr->MsgType > 4)
	    {
	    if (!*hdr) nmFree(my_hdr, sizeof(BDQSMessageHeader));
	    return BDQS_ERR_PROTOCOL;
	    }

	*hdr = my_hdr;

    return 0;
    }


/*** bdqs_internal_ReadParamHeader - This function reads a message header
 *** from the given transport connection, allocating one if space is not 
 *** already provided for one.
 ***/
int
bdqs_internal_ReadParamHeader(pBDQSTransportConn conn, pBDQSParamHeader* hdr)
    {
    int rcnt;
    pBDQSParamHeader my_hdr;
    
	/** Alloc a new header? **/
	if (!*hdr)
	    {
	    my_hdr = (pBDQSParamHeader)nmMalloc(sizeof(BDQSParamHeader));
	    if (!my_hdr) return BDQS_ERR_INTERNAL;
	    }
	else
	    {
	    my_hdr = *hdr;
	    }

	/** Read it from the connection. **/
	rcnt = conn->TransportReadFn(conn->TransportReadArg, my_hdr, sizeof(BDQSParamHeader), 0, FD_U_PACKET);
	if (rcnt != sizeof(BDQSParamHeader))
	    {
	    if (!*hdr) nmFree(my_hdr, sizeof(BDQSParamHeader));
	    return BDQS_ERR_INTERNAL;
	    }

	/** Convert it to local byte order if needed **/
	if ((conn->RemoteFlags & BDQS_PROTO_F_LSB) != BDQS_LSBFIRST)
	    {
	    my_hdr->ParamType = BDQS_CONV16(my_hdr->ParamType);
	    my_hdr->ParamFlags = BDQS_CONV16(my_hdr->ParamFlags);
	    }

	/** Verify param type is in appropriate range **/
	if (my_hdr->ParamType < -1 || my_hdr->ParamType > 5)
	    {
	    if (!*hdr) nmFree(my_hdr, sizeof(BDQSParamHeader));
	    return BDQS_ERR_PROTOCOL;
	    }

	*hdr = my_hdr;

    return 0;
    }


/*** bdqs_internal_Server - This is the function that a BDQS server thread
 *** runs, doing the connection handshake and then waiting for sessions to
 *** be created and used.
 ***/
void
bdqs_internal_Server(void* tconn_v)
    {
    pBDQSTransportConn tconn = (pBDQSTransportConn)tconn_v;
    BDQS_INT8 connect_string[160];
    BDQS_INT8 reply_string[160];
    pBDQSBatchHeader batch_hdr = NULL;
    int e;

	/** Get thread identifier **/
	tconn->Thread = (void*)thCurrent();

	/** Read the connect string from the client **/
	if (bdqs_internal_ReadConnectString(tconn, connect_string, 160) < 0)
	    {
	    tconn->EndConnectionFn(tconn->TransportReadArg, tconn->TransportWriteArg);
	    nmFree(tconn, sizeof(BDQSTransportConn));
	    thExit();
	    }

	/** Send our reply back to the client **/
	snprintf(reply_string, 160, "BDQS/1.0/MTU=%d%s%s%s%s%s%s/ 200 OK\n\n",
	    tconn->LocalMTU,
	    (tconn->LocalFlags & BDQS_PROTO_F_LSB)?"/LE":"",
	    (tconn->LocalFlags & BDQS_PROTO_F_MULTIQUERY)?"/MULTIQUERY":"",
	    (tconn->LocalFlags & BDQS_PROTO_F_SELECT)?"/SELECT":"",
	    (tconn->LocalFlags & BDQS_PROTO_F_COLLATE)?"/COLLATE":"",
	    (tconn->LocalFlags & BDQS_PROTO_F_EVAL)?"/EVAL":"",
	    (tconn->LocalFlags & BDQS_PROTO_F_EVENTS)?"/EVENTS":"");
	if (tconn->TransportWriteFn(tconn->TransportWriteArg, reply_string, strlen(reply_string), 0, FD_U_PACKET) <= 0)
	    {
	    /** Error sending the reply?  Can't do much else... sigh... **/
	    tconn->EndConnectionFn(tconn->TransportReadArg, tconn->TransportWriteArg);
	    nmFree(tconn, sizeof(BDQSTransportConn));
	    thExit();
	    }

	/** Ok, handshake complete.  Now wait for batch frames. **/
	e = 0;
	while(1)
	    {
	    /** Get the batch frame header. **/
	    e = bdqs_internal_ReadBatchHeader(tconn, &batch_hdr);
	    if (e < 0) break;
	    }

	/** Close the connection and exit. **/
	tconn->EndConnectionFn(tconn->TransportReadArg, tconn->TransportWriteArg);
	nmFree(tconn, sizeof(BDQSTransportConn));

    thExit();
    }


/*** bdqsStartServer - starts a BDQS server thread running on the given
 *** transport connection which should be open for reading and writing of
 *** protocol data.  This routine is *not* TCP aware, and does not do any
 *** listen()ing or anything of that sort.
 ***/
int
bdqsStartServer(int (*read_fn)(), int (*write_fn)(), void* read_arg, 
    void* write_arg, void* (*auth_fn)(), int (*endauth_fn)(), 
    void* (*chan_fn)(), int (*endchan_fn)(), int (*endconn_fn)(),
    int mtu, int flags)
    {
    pThread t;
    pBDQSTransportConn tc;

	/** Create the connection context structure **/
	tc = (pBDQSTransportConn)nmMalloc(sizeof(BDQSTransportConn));
	if (!tc) return -1;
	tc->TransportReadFn = read_fn;
	tc->TransportWriteFn = write_fn;
	tc->TransportReadArg = read_arg;
	tc->TransportWriteArg = write_arg;
	tc->SessionAuthFn = auth_fn;
	tc->EndSessionFn = endauth_fn;
	tc->ChannelAllocFn = chan_fn;
	tc->EndChannelFn = endchan_fn;
	tc->EndConnectionFn = endconn_fn;
	tc->IsServer = 1;
	tc->AuthSessionList = NULL;
	tc->LocalFlags = flags;
	tc->LocalMTU = mtu;
	tc->CurSessionContext = NULL;
	tc->CurChannelContext = NULL;
	tc->RecvBatchSeqID = 0;

	/** Start the server thread on the connection **/
	t = thCreate(bdqs_internal_Server, 0, (void*)tc);
	if (!t) return -1;
	tc->Thread = (void*)t;

    return 0;
    }

