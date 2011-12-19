#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "mtask.h"
#include "mtsession.h"
#include "xarray.h"
#include "xhash.h"
#include "mtlexer.h"
#include "exception.h"
#include "datatypes.h"
#include "stparse.h"
#include "net_raw_api.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	net_raw_api.c               				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	May 1st, 2001     					*/
/* Description:	Utility module providing common protocol-handling 	*/
/*		routines for the RAW network protocol			*/
/************************************************************************/



/*** Module controls ***/
#define NET_USERAUTH_LIMIT	16	/* max channels open by one user */


/*** Data on command parameter counts, for sanity checking purposes
 ***/
int cmd_n_params[] =
    {
    -1, -1, -1, 1,  3,  1,  2,  4,	/* ACK through OPENQUERY */
    2,  2,  2,  2,  1,  0,  4,  4,	/* MQ through WC */
    2,  3,  3,  4,  4,  1,  4,  4,	/* CD through CLI */
    1,  2,  2,  3,  1,  1,		/* EL through EB */
    };


/*** raw_internal_ReadBytes - read and discard <n> bytes from the
 *** raw connection.  Useful when a batch error occurs and we need to
 *** ignore the remainder of the batch.
 ***/
int
raw_internal_ReadBytes(pNetManagedConn conn, int n_bytes)
    {
    int rcnt, cnt;

    	/** Trying to read a silly amount of data? **/
	if (n_bytes < 0) return -1;

	/** Read the data... **/
	rcnt = 1;
	while (n_bytes > 0)
	    {
	    cnt = n_bytes;
	    if (cnt > 256) cnt=256;
	    rcnt = fdRead(conn->ConnFD, buf, cnt, 0, 0);
	    if (rcnt <= 0) break;
	    n_bytes -= rcnt;
	    }
	if (rcnt <= 0) return -1;

    return 0;
    }


/*** raw_internal_SkipCmd - skip past the end of a command, given the
 *** number of parameters in the command.  This is normally used in the
 *** event that a command's Length value isn't declared (but is set 0).
 ***/
int
raw_internal_SkipCmd(pNetManagedConn conn, pNetCmdHeader cmd)
    {
    char param_type;
    int len,rcnt;

    	/** Length set?  ReadBytes the whole honkin' thing then **/
	if (cmd->Length)
	    {
	    return raw_internal_ReadBytes(conn, cmd->Length - sizeof(NetCmdHeader));
	    }

    	/** Read all of the command's parameters, discarding them **/
	while(cmd->nParams)
	    {
	    cmd->nParams--;
	    rcnt = fdRead(conn->ConnFD, &param_type, 1, 0, FD_U_PACKET);
	    if (rcnt != 1) return -1;
	    switch(ch)
	        {
		case NET_PARAM_INT:	len = 4; break;
		case NET_PARAM_REF:	len = 4; break;
		case NET_PARAM_DOUBLE:	len = 8; break;
		case NET_PARAM_DATETIME: len= 5; break;
		case NET_PARAM_MONEY:	len = 6; break;
		case NET_PARAM_NULL:	len = 0; break;
		case NET_PARAM_STRING:
		    rcnt = fdRead(conn->ConnFD, &len, 4, 0, FD_U_PACKET);
		    if (rcnt != 4) return -1;
		    break;
		default:
		    return -1;
		}
	    if (len) if (raw_internal_ReadBytes(conn, len) < 0) return -1;
	    }

    return 0;
    }


/*** raw_internal_SkipBatch - skip past the end of a batch in the
 *** event that a Length was not specified in the command header.
 *** This requires reading commands one at a time until we find
 *** an ENDBATCH command.  Assumes that the batch header has been
 *** read, and all that remains are commands of various sorts.
 ***/
int
raw_internal_SkipBatch(pNetManagedConn conn, pNetBatchHeader hdr)
    {
    NetCmdHeader cmd;
    int rcnt,len;
    char param_type;

    	/** Length specified?  No need to do one-cmd-at-a-time then **/
	if (hdr && hdr->Length)
	    {
	    return raw_internal_ReadBytes(conn, hdr->Length - sizeof(NetBatchHeader));
	    }

    	/** Loop, reading cmds **/
	while(1)
	    {
	    /** Get a command header blk **/
	    rcnt = fdRead(conn->ConnFD, &cmd, sizeof(NetCmdHeader), 0, FD_U_PACKET);
	    if (rcnt != sizeof(NetCmdHeader)) return -1;

	    /** Skip to end of blk **/
	    if (raw_internal_SkipCmd(conn, &cmd) < 0) return -1;
	    if (cmd.CommandCode == NET_M_ENDBATCH) break;
	    }

    return 0;
    }


/*** rawSendParam - write a parameter out to the network.
 ***/
int
rawSendParam(pNetManagedConn conn, char param_type, pObjData value, int cmd_id)
    {
    char buf[9];
    int len;
    int v;

    	/** Build the parameter initial buffer **/
	buf[0] = param_type;
	switch(param_type)
	    {
	    case NET_PARAM_INT:		
	        memcpy(buf+1, value, 4);
	    	len = 5;
	    	break;

	    case NET_PARAM_STRING:	
	        v = strlen(value->String);
	    	memcpy(buf+1, &v, 4);
		len = 5;
		break;

	    case NET_PARAM_DOUBLE:
	        memcpy(buf+1, value, 8);
		len = 9;
		break;

	    case NET_PARAM_DATETIME:
	        memcpy(buf+1, value->DateTime, 5);
		len = 6;
		break;

	    case NET_PARAM_MONEY:
	        memcpy(buf+1, value->Money, 6);
		len = 7;
		break;

	    case NET_PARAM_NULL:
	        len = 1;
		break;
	
	    case NET_PARAM_REF:
	        memcpy(buf+1, &cmd_id, 4);
		len = 5;
		break;
	    }

	/** Send it on the wire **/
	fdWrite(conn->ConnFD, buf, len, 0, FD_U_PACKET);

	/** Send string, if string value **/
	if (param_type == NET_PARAM_STRING) fdWrite(conn->ConnFD, value->String, v, 0, FD_U_PACKET);

    return 0;
    }


/*** raw_internal_NextParam - read a parameter from the connection.  If the
 *** param is a REFERENCE, this does not resolve the reference, but stores the
 *** ref cmdid in the value->Integer element of the POD (pObjData).  An
 *** "expected type" of NULL (0) will match any incoming type.
 ***/
int
raw_internal_NextParam(pNetManagedConn conn, char expected_type, char* param_type, pObjData value, int max_string_size)
    {
    char buf[9];
    unsigned int l;
    int sz,sl,extra;

    	/** Read the param type into the buf **/
	if (fdRead(conn->ConnFD, buf, 1, 0, FD_U_PACKET) != 1) return -1;
	if (param_type) *param_type = buf[0];
	if (buf[0] != expected_type && expected_type != 0x00) return -1;

	/** Depending on param type, read the data **/
	switch(buf[0])
	    {
	    case NET_PARAM_INT:
	    case NET_PARAM_REF:
	        fdRead(conn->ConnFD, value, 4, 0, FD_U_PACKET);
		break;
	    
	    case NET_PARAM_STRING:
	        fdRead(conn->ConnFD, &l, 4, 0, FD_U_PACKET);
		sz = 0;
		sl = 0;
		extra = 0;
		if (l > max_string_size)
		    {
		    extra = max_string_size - l;
		    l = max_string_size;
		    }

		/** Careful here - only alloc for the string as it is actually
		 ** read in.  Otherwise, it is trivial to DOS the server by specifying
		 ** a huge string size with no data to follow it.
		 **/
		while(sz < l)
		    {
		    sz = (sz*2 + 1024);
		    if (sz > l) sz = l;
		    value->String = (char*)nmSysMalloc(sz+1);
		    sl = fdRead(conn->ConnFD, buf, sz, 0, FD_U_PACKET);
		    value->String[sz] = '\0';
		    if (sl < sz) break;
		    }
		if (extra) raw_internal_ReadBytes(conn, extra);
		break;

	    case NET_PARAM_DOUBLE:
	        fdRead(conn->ConnFD, value, 8, 0, FD_U_PACKET);
		break;
	       
	    case NET_PARAM_MONEY:
	        value->Money = nmMalloc(sizeof(MoneyType));
		if (!value->Money) 
		    {
		    raw_internal_ReadBytes(conn, 6);
		    return -1;
		    }
		fdRead(conn->ConnFD, value->Money, 6, 0, FD_U_PACKET);
	        break;

	    case NET_PARAM_DATETIME:
	        value->DateTime = nmMalloc(sizeof(DateTime));
		if (!value->DateTime) 
		    {
		    raw_internal_ReadBytes(conn, 5);
		    return -1;
		    }
		fdRead(conn->ConnFD, value->DateTime, 5, 0, FD_U_PACKET);
	        break;
	    }

    return 0;
    }

/*** rawSendResponse - send an ACK or an ERR to the other end of the
 *** connection.
 ***/
int
rawSendResponse(pNetManagedConn conn, int is_err, long long batch_id, int cmd_id, int n_params)
    {
    NetError err;
    NetACK ack;

    	/** error or ack message? **/
	if (is_err)
	    {
	    err->FrameType = NET_FRAME_ERROR;
	    err->Sequence = conn->NextACKseq++;
	    err->BatchID = batch_id;
	    err->CommandID = cmd_id;
	    fdWrite(conn->ConnFD, &err, sizeof(err), 0, FD_U_PACKET);
	    }
	else
	    {
	    ack->FrameType = NET_FRAME_ACK;
	    ack->Sequence = conn->NextACKseq++;
	    ack->BatchID = batch_id;
	    ack->CommandID = cmd_id;
	    ack->nParams = n_params;
	    fdWrite(conn->ConnFD, &ack, sizeof(ack), 0, FD_U_PACKET);
	    }

    return 0;
    }


/*** raw_internal_SendError - a wrapper for SendResponse for simple 
 *** sending of error strings
 ***/
int
raw_internal_SendError(pNetManagedConn conn, long long batch_id, int cmd_id, int errcode, char* errstr)
    {

	/** Send an error message **/
	rawSendResponse(conn, 1, batch_id, 0, 0);
	rawSendParam(conn, NET_PARAM_INT, POD(&errcode), 0);
	rawSendParam(conn, NET_PARAM_STRING, POD(&errstr), 0);

    return 0;
    }


/*** raw_internal_ServerHandler - this routine is run by the connection
 *** handler thread for server-side connections.  It will call the auth
 *** and session handlers as needed, otherwise it runs "unattended"
 ***/
void
raw_internal_ServerHandler(void *conn_v)
    {
    pNetManagedConn conn = (pNetManagedConn)conn_v;
    NetBatchHeader hdr;
    NetCmdHeader cmd;
    pNetAuthUser usr, search;
    char buf[256];
    int cnt,rcnt;
    char* str;
    ObjData od;
    char t;
    int batch_err;
    int n_auth;
    int cmd_id;

    	/** Set thread name **/
	thSetName(NULL, "RAW Protocol Server");

	/** Loop, reading batch headers **/
	while(1)
	    {
	    /** Get the batch header **/
	    if (fdRead(conn->ConnFD, &hdr, sizeof(hdr), 0, FD_U_PACKET) < sizeof(hdr)) break;

	    /** Check frametype just for grins **/
	    if (hdr.FrameType != NET_FRAME_BATCH)
	        {
		/** Invalid frame type.  Read the remainder of the batch and toss it **/
		raw_internal_SkipBatch(conn, &hdr);

		/** Toss an error out to the user **/
		raw_internal_SendError(conn, hdr.BatchID, 0, 0, "Protocol error: expected batch, got invalid frame type");
		rawSendResponse(conn, 0, hdr.BatchID, 0, 0);
		continue;
		}

	    /** Scan for the user channel, if any **/
	    if (hdr.UserChannel == 0)
	        {
		usr = NULL;
		}
	    else
	        {
		usr = conn->Userlist;
		while(usr && usr->ChannelID != hdr.UserChannel) usr = usr->Next;
		if (!usr)
		    {
		    /** Invalid user.  Read the remainder of the batch and toss it **/
		    raw_internal_ReadBytes(conn, hdr.Length - sizeof(hdr));

		    /** Toss an error out to the user **/
		    raw_internal_SendError(conn, hdr.BatchID, 0, 0, "Protocol error: invalid user channel id in batch header");
		    rawSendResponse(conn, 0, hdr.BatchID, 0, 0);
		    continue;
		    }
		}
	    
	    /** Ok, this is a valid batch frame.  Start reading cmds **/
	    batch_err = 0;
	    cmd_id = 0;
	    while((--hdr.nCommands) != 0)
	        {
		/** Read the command header **/
		cmd_id++;
	        if (fdRead(conn->ConnFD, &cmd, sizeof(cmd), 0, FD_U_PACKET) < sizeof(cmd)) break;
		if (batch_err) 
		    {
		    raw_internal_SkipCmd(conn, &cmd);
		    continue;
		    }

		/** Check command sequencing **/
		if (cmd_id != cmd.CommandID)
		    {
		    raw_internal_SendError(conn, hdr.BatchID, cmd.CommandID, 0, "Protocol error: command id out of sequence");
	    	    rawSendResponse(conn, 0, hdr.BatchID, 0, 0);
		    batch_err = 1;
		    continue;
		    }

		/** Check for user authorization **/
		if (!usr)
		    {
		    if (cmd.CommandCode == NET_M_REQAUTH)
		        {
			usr = (pNetAuthUser)nmMalloc(sizeof(NetAuthUser));
			if (!usr) break;
			usr->Next = NULL;
			if (raw_internal_NextParam(conn, NET_PARAM_STRING, NULL, &od, 31) < 0) break;
			strcpy(usr->Username, od.String);
			nmSysFree(od.String);
			if (raw_internal_NextParam(conn, NET_PARAM_STRING, NULL, &od, 31) < 0) break;
			strcpy(usr->Password, od.String);
			nmSysFree(od.String);
			if (raw_internal_NextParam(conn, NET_PARAM_INT, NULL, &od, 0) < 0) break;
			usr->ChannelID = od.Integer;
			if (mssAuthenticate(usr->Username, usr->Password) < 0)
			    {
			    raw_internal_SendError(conn, hdr.BatchID, cmd.CommandID, 0, "Username or password incorrect");
		    	    rawSendResponse(conn, 0, hdr.BatchID, 0, 0);
			    nmFree(usr, sizeof(NetAuthUser));
			    batch_err = 1;
			    continue;
			    }
			mssEndSession();

			/** Before adding this to the list, search to see if this user auth'd already **/
			n_auth = 0;
			search = conn->Userlist;
			while(search)
			    {
			    if (!strcmp(search->Username, usr->Username)) n_auth++;
			    search = search->Next;
			    }
			if (n_auth >= NET_USERAUTH_LIMIT)
			    {
			    raw_internal_SendError(conn, hdr.BatchID, cmd.CommandID, 0, "Too many logins for this user on this connection");
		    	    rawSendResponse(conn, 0, hdr.BatchID, 0, 0);
			    nmFree(usr, sizeof(NetAuthUser));
			    batch_err = 1;
			    continue;
			    }

			/** Add this auth to the list for this connection **/
			usr->Next = conn->Userlist;
			conn->Userlist = usr;
			continue;
			}
		    else
		        {
		    	/** Did not auth.  Read the remainder of the batch and toss it **/
			raw_internal_SkipCmd(conn, &cmd);
			raw_internal_SkipBatch(conn, NULL);

		    	/** Toss an error out to the user **/
		    	raw_internal_SendError(conn, hdr.BatchID, 0, 0, "Protocol error: command attempted without authorization");
		    	rawSendResponse(conn, 0, hdr.BatchID, 0, 0);
		    	break;
			}
		    }

		/** Verify this command's param counts **/
		if (cmd.CommandCode >= NET_M_REQVERSION && cmd.CommandCode <= NET_M_ENDBATCH)
		    {
		    if (cmd.nParams != cmd_n_params[cmd.CommandCode] && cmd_n_params[cmd.CommandCode] != -1)
		        {
			raw_internal_SkipCmd(conn, &cmd);
		        raw_internal_SendError(conn, hdr.BatchID, cmd.CommandID, 0, "Protocol error: invalid command parameter count");
			break;
			}
		    }

		/** Process a command **/
		switch(cmd.CommandCode)
		    {
		    case NET_M_CLOSEAUTH:
		        /** End a userauth session, close it by channel id, not by username **/
			raw_internal_CloseAuth(conn, usr);
			break;

		    default:
		        /** For an unknown command code, try to finish the rest of the batch **/
			raw_internal_SkipCmd(conn, &cmd);
		        raw_internal_SendError(conn, hdr.BatchID, cmd.CommandID, 0, "Protocol error: unknown command code");
			break;
		    }
		}
	    if (hdr.nCommands >= 0) break;
	    }

    thExit();
    }


/*** rawManageConnection - take a MTASK file descriptor and manage it as
 *** if we are the server or client end of a raw protocol connection.
 ***/
pNetManagedConn
rawManageConnection(pFile net_fd, int is_server)
    {
    pNetManagedConn this;

    	/** Allocate the structure **/
	this = (pNetManagedConn)nmMalloc(sizeof(NetManagedConn));
	if (!this) return NULL;

	/** Setup the information components **/
	this->ConnFD = net_fd;
	this->IsServer = is_server?1:0;
	this->InpBufPtr = this->InputBuffer;
	this->OutBufPtr = this->OutputBuffer;
	this->InpBufLen = 0;
	this->OutBufLen = 0;
	this->NextBatchID = 1;
	this->NextACKseq = 1;
	this->SessHandler = NULL;
	this->AuthHandler = NULL;
	this->Userlist = NULL;

    return this;
    }


/*** rawUnmanageConnection - convert a managed connection back into a
 *** normal MTASK file descriptor.
 ***/
pFile
rawUnmanageConnection(pNetManagedConn this)
    {
    pFile original_fd;

	/** Flush the output buffer? **/
    	if (this->OutBufLen)
	    {
	    fdWrite(this->ConnFD, this->OutBufPtr, OBLEN(this), 0, FD_U_PACKET);
	    }

	/** Free the memory **/
	original_fd = this->ConnFD;
	nmFree(this, sizeof(NetManagedConn));

    return original_fd;
    }


/*** rawSetAuthHandler - installs an authentication handler in a server-side
 *** managed connection, so that when authentication requests come through
 *** from the client, we know how to authenticate them.  If no authentication
 *** handler is installed, then the system uses the builtin mssAuthenticate
 *** authentication mechanism.
 ***/
int
rawSetAuthHandler(pNetManagedConn this, int (*callback_fn)())
    {

    	this->AuthHandler = callback_fn;

    return 0;
    }


/*** rawSetSessionHandler - installs a session handler in a server-side
 *** managed connection.  This must be done before any connections can
 *** be properly utilized.  Once authentication has taken place, the remote
 *** end can request that a session be opened for the issuing of further
 *** commands.
 ***/
int
rawSetSessionHandler(pNetManagedConn this, int (*callback_fn)())
    {

    	this->SessHandler = callback_fn;

    return 0;
    }


/*---------------------------------------------------------------------------*/
