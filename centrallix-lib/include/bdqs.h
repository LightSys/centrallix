#ifndef _BDQS_H
#define _BDQS_H

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
/* Creation:	March 13, 2002    					*/
/* Description:	This module implements the Batchable Data Query Service,*/
/*		which is Centrallix's raw communication protocol.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: bdqs.h,v 1.2 2005/02/26 04:32:02 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/bdqs.h,v $

    $Log: bdqs.h,v $
    Revision 1.2  2005/02/26 04:32:02  gbeeley
    - moving include file install directory to include a "cxlib/" prefix
      instead of just putting 'em all in /usr/include with everything else.

    Revision 1.1  2002/03/23 06:25:09  gbeeley
    Updated MSS to have a larger error string buffer, as a lot of errors
    were getting chopped off.  Added BDQS protocol files with some very
    simple initial implementation.

 **END-CVSDATA***********************************************************/

#ifdef CXLIB_INTERNAL
#include "mtask.h"
#include "xarray.h"
#include "xstring.h"
#include "xhash.h"
#else
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "cxlib/xhash.h"
#endif


/*** Compatibility data type specifications, the below are for 
 *** Linux/gcc/x86.
 ***/
typedef long long BDQS_INT64;
typedef int BDQS_INT32;
typedef short BDQS_INT16;
typedef char BDQS_INT8;

/*** Byte order for local host ***/
#define BDQS_LSBFIRST		(1)	/* set to 0 for big-endian, 1 for little */


/*** BDQS commands ***/
#define BDQS_CMD_NOOP		(0)	/* no operation */
#define BDQS_CMD_OPENAUTH	(1)	/* open authenticated session */
#define BDQS_CMD_CLOSEAUTH	(2)	/* close authenticated session */
#define BDQS_CMD_OPENCHANNEL	(3)	/* open a channel */
#define BDQS_CMD_CLOSECHANNEL	(4)	/* close an open channel */
#define BDQS_CMD_OPENOBJECT	(5)	/* open an object */
#define BDQS_CMD_CLOSEOBJECT	(6)	/* close an open object */
#define BDQS_CMD_OPENQUERY	(7)	/* open a query */
#define BDQS_CMD_CLOSEQUERY	(8)	/* close an open query */
#define BDQS_CMD_FETCH		(9)	/* fetch objects from a query */
#define BDQS_CMD_MULTIQUERY	(10)	/* open a multiquery (sql query) */
#define BDQS_CMD_READ		(11)	/* read data (content) from an object */
#define BDQS_CMD_WRITE		(12)	/* write data to an object */
#define BDQS_CMD_CHANGEDIR	(13)	/* change directory (affects channel */
#define BDQS_CMD_ITERATTR	(14)	/* begin obj attribute iterator loop */
#define BDQS_CMD_ENDATTR	(15)	/* end obj attribute iterator loop */
#define BDQS_CMD_GETATTRTYPE	(16)	/* get the type of an attribute */
#define BDQS_CMD_GETATTRVALUE	(17)	/* get the value of an object's attribute */
#define BDQS_CMD_SETATTRVALUE	(18)	/* set the value of an object's attribute */
#define BDQS_CMD_EXECMETHOD	(19)	/* execute a method on an object */
#define BDQS_CMD_METHODITER	(20)	/* begin obj method iterator loop */
#define BDQS_CMD_ENDMETHOD	(21)	/* end obj method iterator loop */
#define BDQS_CMD_EVAL		(22)	/* evaluate an expression */
#define BDQS_CMD_COMMIT		(23)	/* commits changes made, without closing any objects */
#define BDQS_CMD_BEGINLOOP	(24)	/* begin a loop */
#define BDQS_CMD_ENDLOOP	(25)	/* end a loop */
#define BDQS_CMD_EXITLOOPIF	(26)	/* breaks out of a loop if condition true */
#define BDQS_CMD_CONTINUELOOPIF	(27)	/* continues a loop at top if condition true */
#define BDQS_CMD_IF		(28)	/* begins a conditional execution block */
#define BDQS_CMD_ELSE		(29)	/* marks an alternate part of a cond block */
#define BDQS_CMD_ENDIF		(30)	/* ends a conditional execution block */
#define BDQS_CMD_FOR		(31)	/* begins a for loop */
#define BDQS_CMD_NEXT		(32)	/* ends a for loop */
#define BDQS_CMD_DELETE		(33)	/* deletes an object */
#define BDQS_CMD_CREATE		(34)	/* creates an object */
#define BDQS_CMD_MONITOR	(35)	/* remotely monitor for changes, etc */
#define BDQS_CMD_PAUSEMONITOR	(36)	/* pause all monitors in the session */
#define BDQS_CMD_CONTMONITOR	(37)	/* continue all monitors in this session */
#define BDQS_CMD_ENDMONITOR	(38)	/* terminates a monitor */
#define BDQS_CMD_RESUMEMONITOR	(39)	/* resume a 'global' event monitor */
#define BDQS_CMD_END		(40)	/* ends batch execution */


/*** Error codes ***/
#define BDQS_ERR_UNKNOWN	(0)	/* an unknown or unspecified error occurred */
#define BDQS_ERR_IMBALANCE	(1)	/* imbalanced for/next, begin/end, etc. */
#define BDQS_ERR_NOREFERENCE	(2)	/* an invalid reference to a previous value */
#define BDQS_ERR_LIMITED	(3)	/* a limit was reached in open sessions/etc */
#define BDQS_ERR_DENIED		(4)	/* access was not permitted */
#define BDQS_ERR_UNIMPLEMENTED	(5)	/* command or feature not implemented */
#define BDQS_ERR_PARAMTYPE	(6)	/* a parameter-type mismatch occurred */
#define BDQS_ERR_PARAMCOUNT	(7)	/* a parameter count was invalid for a command */
#define BDQS_ERR_NOSESSION	(8)	/* an invalid session was given in a batch */
#define BDQS_ERR_NOCHANNEL	(9)	/* an invalid channel was given for a command */
#define BDQS_ERR_PROTOCOL	(10)	/* a protocol error occurred */
#define BDQS_ERR_TERMINATED	(11)	/* transport connection is being terminated */
#define BDQS_ERR_NOTEXIST	(12)	/* the referenced pathname or object does not exist */
#define BDQS_ERR_TERMSESSION	(13)	/* an auth session is being terminated */
#define BDQS_ERR_TERMCHANNEL	(14)	/* a data channel is being terminated */
#define BDQS_ERR_INTERNAL	(15)	/* internal error occurred */


/*** Monitor Event types ***/
#define BDQS_MON_TERM		(0)	/* the event monitor is being terminated */
#define BDQS_MON_CREATE		(1)	/* an object was created */
#define BDQS_MON_DELETE		(2)	/* an object was deleted */
#define BDQS_MON_OPEN		(3)	/* an object was opened */
#define BDQS_MON_CLOSE		(4)	/* an object was closed */
#define BDQS_MON_READ		(5)	/* an object's content was read */
#define BDQS_MON_WRITE		(6)	/* an object's content was updated */
#define BDQS_MON_SETATTR	(7)	/* an object's attribute was modified */
#define BDQS_MON_EXECMETHOD	(8)	/* a method was run on an object */
#define BDQS_MON_GROUP		(9)	/* beginning of a group of related events */
#define BDQS_MON_ENDGROUP	(10)	/* end of a group of related events */


/*** Message Types ***/
#define BDQS_MSG_T_ENDFRAME	(0)	/* end of batch frame */
#define BDQS_MSG_T_COMMAND	(1)	/* command message */
#define BDQS_MSG_T_ACK		(2)	/* acknowledge - command results */
#define BDQS_MSG_T_ERROR	(3)	/* error - command results */
#define BDQS_MSG_T_EVENT	(4)	/* event occurred for a monitor session */

/*** BDQS Protocol Options ***/
#define BDQS_PROTO_F_LSB	(1)	/* little endian data */
#define BDQS_PROTO_F_MULTIQUERY	(2)	/* multiquery (sql) support */
#define BDQS_PROTO_F_SELECT	(4)	/* where clause (selection) support */
#define BDQS_PROTO_F_COLLATE	(8)	/* order by (sorting/collation) support */
#define BDQS_PROTO_F_EVAL	(16)	/* "eval" command support */
#define BDQS_PROTO_F_EVENTS	(32)	/* event monitoring support */


/*** Macros for converting LSB/MSB ***/
#define BDQS_CONV8(x)	(x)
#define BDQS_CONV16(x)	((BDQS_CONV8((x)&0xff)<<8)|BDQS_CONV8(((x)>>8)&0xff))
#define BDQS_CONV32(x)	((BDQS_CONV16((x)&0xffff)<<16)|BDQS_CONV16(((x)>>16)&0xffff))
#define BDQS_CONV64(x)	((BDQS_CONV32((x)&0xffffffffLL)<<32)|BDQS_CONV32(((x)>>32)&0xffffffffLL))


/*** Batch Frame Structure ***/
typedef struct _BQB
    {
    BDQS_INT64		BatchID;
    BDQS_INT32		FrameID;
    BDQS_INT32		FrameLength;
    BDQS_INT32		nCommands;
    BDQS_INT32		Flags;
    BDQS_INT32		SessionID;
    BDQS_INT64		BatchRefID;
    }
    BDQSBatchHeader, *pBDQSBatchHeader;


/*** Message Structure (command, error, event, etc) ***/
typedef struct _BQM
    {
    BDQS_INT16		MsgType;
    BDQS_INT16		Flags;
    BDQS_INT32		MessageID;
    BDQS_INT32		nParams;
    }
    BDQSMessageHeader, *pBDQSMessageHeader;


/*** Parameter Structure ***/
typedef struct _BQP
    {
    BDQS_INT16		ParamType;
    BDQS_INT16		ParamFlags;
    }
    BDQSParamHeader, *pBDQSParamHeader;


/*** Data Context Channel structure ***/
typedef struct _BQC
    {
    struct _BQS*	Next;
    struct _BQS*	Prev;
    void*		DataContext;
    }
    BDQSChannel, *pBDQSChannel;


/*** Authenticated Session structure ***/
typedef struct _BQS
    {
    struct _BQS*	Next;
    struct _BQS*	Prev;
    void*		AuthSession;
    pBDQSChannel	ChannelList;
    BDQS_INT64		SessionID;
    }
    BDQSAuthSession, *pBDQSAuthSession;


/*** Transport connection structure ***/
typedef struct _BQT
    {
    int			(*TransportReadFn)();
    int			(*TransportWriteFn)();
    void*		TransportReadArg;
    void*		TransportWriteArg;
    void*		(*SessionAuthFn)();
    int			(*EndSessionFn)();
    void*		(*ChannelAllocFn)();
    int			(*EndChannelFn)();
    int			(*EndConnectionFn)();
    int			IsServer;
    pBDQSAuthSession	AuthSessionList;
    void*		Thread;
    int			RemoteFlags;
    int			RemoteMTU;
    int			LocalFlags;
    int			LocalMTU;
    pBDQSAuthSession	CurSessionContext;
    pBDQSChannel	CurChannelContext;
    BDQS_INT32		RecvMsgSeqID;
    BDQS_INT64		RecvBatchSeqID;
    BDQS_INT32		RecvFrameSeqID;
    }
    BDQSTransportConn, *pBDQSTransportConn;


/*** Message management structure ***/
typedef struct _BDM
    {
    int			HeaderOffset;
    XArray		ParamIndex;
    }
    BDQSMessage, *pBDQSMessage;


/*** Batch management structure ***/
typedef struct _BDB
    {
    XString		RawData;
    int			HeaderOffset;
    XArray		MessageIndex;
    }
    BDQSBatch, *pBDQSBatch;

/*** x = pBDQSBatch pointer, result = pBDQSBatchHeader ***/
#define BDQS_BATCHHEADER(x) ((pBDQSBatchHeader)((x)->RawData.String + (x)->HeaderOffset))

/*** x = pBDQSBatch pointer, y = message id, result = pBDQSMessage ***/
#define BDQS_MESSAGE(x,y) ((pBDQSMessage)((x)->RawData.String + ((int)((x)->MessageIndex[(y)]))))

/*** x = pBDQSBatch pointer, y = pBDQSMessage, result = pBDQSMessageHeader ***/
#define BDQS_MESSAGEHEADER(x,y) ((pBDQSMessageHeader)((x)->RawData.String + (y)->HeaderOffset))

/*** x = pBDQSBatch pointer, y = pBDQSMessage pointer, z = param id, result = pBDQSParamHeader ***/
#define BDQS_PARAM(x,y,z) ((pBDQSParamHeader)((x)->RawData.String + ((int)((y)->ParamIndex[(z)]))

/*** x = pBDQSParamHeader pointer, result = raw data pointer ***/
#define BDQS_PARAMDATA(x) (((BDQS_INT8*)(x)) + sizeof(BDQSParamHeader))


/*** Transport level functions ***/
int bdqsStartServer(int (*read_fn)(), int (*write_fn)(), void* read_arg, void* write_arg, void* (*auth_fn)(), int (*endauth_fn)(), void* (*chan_fn)(), int (*endchan_fn)(), int (*endconn_fn)(), int mtu, int flags);
pBDQSTransportConn bdqsStartClient(int (*read_fn)(), int (*write_fn)(), void* read_arg, void* write_arg);

/*** Batch management functions ***/
pBDQSBatch bdqsCreateBatch(pBDQSAuthSession ssn, BDQS_INT64 ref_batchid);
int bdqsSendBatch(pBDQSTransportConn tconn, pBDQSBatch batch);

#endif /* not defined _BDQS_H */

