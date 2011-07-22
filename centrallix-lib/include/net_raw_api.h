#ifndef _NET_RAW_API_H
#define _NET_RAW_API_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	net_raw.h               				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	May 1st, 2001     					*/
/* Description:	Basic protocol design header file, for any module that	*/
/*		needs to implement the Centrallix RAW protocol		*/
/************************************************************************/



#ifdef CXLIB_INTERNAL
#include "mtask.h"
#include "mtsession.h"
#include "xarray.h"
#include "xhash.h"
#include "mtlexer.h"
#include "datatypes.h"
#else
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "cxlib/datatypes.h"
#endif

/*** Protocol Messages ***/
#define NET_M_ACK		1
#define NET_M_ERROR		2
#define	NET_M_REQVERSION	3
#define NET_M_REQAUTH		4
#define NET_M_OPENSESSION	5
#define NET_M_OPENOBJ		6
#define NET_M_OPENQUERY		7
#define NET_M_MULTIQUERY	8
#define NET_M_FETCH		9
#define NET_M_CLOSEQUERY	10
#define NET_M_CLOSEOBJ		11
#define NET_M_CLOSESESSION	12
#define NET_M_CLOSEAUTH		13
#define NET_M_READCONTENT	14
#define NET_M_WRITECONTENT	15
#define NET_M_CHANGEDIR		16
#define NET_M_GETATTRS		17
#define NET_M_GETATTRVALUES	18
#define NET_M_SETATTRVALUE	19
#define NET_M_EXECMETHOD	20
#define NET_M_BEGINLOOP		21
#define NET_M_EXITLOOPIF	22
#define NET_M_CONTINUELOOPIF	23
#define NET_M_ENDLOOP		24
#define NET_M_BEGINITERLOOP	25
#define NET_M_DELETEOBJ		26
#define NET_M_CREATEOBJ		27
#define NET_M_EXIT		28
#define NET_M_ENDBATCH		29

#define NET_FRAME_ACK		1
#define NET_FRAME_ERROR		2
#define NET_FRAME_BATCH		3

#define NET_PARAM_INT		0xFF
#define NET_PARAM_STRING	0xFE
#define NET_PARAM_DOUBLE	0xFD
#define NET_PARAM_MONEY		0xFC
#define NET_PARAM_DATETIME	0xFB
#define NET_PARAM_REF		0x01
#define NET_PARAM_NULL		0x00


/*** protocol error response ***/
typedef struct
    {
    int			FrameType;
    long long		Sequence;
    long long		BatchID;
    int			CommandID;
    }
    NetError, *pNetError;

/*** protocol ack response ***/
typdef struct
    {
    int			FrameType;
    long long		Sequence;
    long long		BatchID;
    int			CommandID;
    int			nParams;
    }
    NetACK, *pNetACK;


/*** protocol batch header ***/
typedef struct
    {
    int			FrameType;
    long long		BatchID;
    unsigned long	UserChannel;
    unsigned long	Length;
    int			nCommands;
    }
    NetBatchHeader, *pNetBatchHeader;

/*** protocol command header ***/
typedef struct
    {
    unsigned long	CommandID;
    unsigned long	Length;
    unsigned short	CommandCode;	/* NET_M_xxx */
    unsigned short	nParams;
    unsigned long	Flags;		/* NET_CMD_F_xxx */
    }
    NetCmdHeader, *pNetCmdHeader;

/*** parsed protocol command ***/
typedef struct
    {
    NetCmdHeader	Header;
    unsigned long	SessionID;
    unsigned long	ObjOrQueryID;
    }
    NetCmd, *pNetCmd;

typedef struct _NA
    {
    struct _NA*		Next;
    char		Username[32];
    char		Password[32];
    int			ChannelID;
    }
    NetAuthUser, *pNetAuthUser;

/*** internal connection handling structure ***/
typedef struct
    {
    pFile		ConnFD;
    unsigned char	OutputBuffer[1024];
    int			OutBufLen;
    int			OutBufPtr;
    unsigned char	InputBuffer[1024];
    int			InpBufLen;
    int			InpBufPtr;
    long long		NextBatchID;
    long long		NextACKseq;
    int			IsServer;
    int			(*AuthHandler)();
    int			(*SessHandler)();
    pNetAuthUser	Userlist;
    void*		Sessionlist;	/* pNetSession */
    }
    NetManagedConn, *pNetManagedConn;

#define IBLEN(c) (((c)->InpBufLen) - ((c)->InpBufPtr - (c)->InputBuffer))
#define OBLEN(c) (((c)->OutBufLen) - ((c)->OutBufPtr - (c)->OutputBuffer))

typedef struct _NS
    {
    struct _NS*		Next;
    pNetAuthUser	User;
    int			SessionID;
    pNetManagedConn	Conn;
    pSemaphore		CmdSemaphore;
    int			CmdID;
    int			CmdFlags;
    int			CmdParams;
    int			IsClosed;
    }
    NetSession, *pNetSession;

/*** support functions - client and server ***/
pNetManagedConn rawManageConnection(pFile net_fd, int is_server);
pFile rawUnmanageConnection(pNetManagedConn this);
int rawNextParam(pNetSession this, int req_datatype, pObjData value);

/*** support functions - client only ***/
int rawOpenAuthentication(pNetManagedConn this, char* user, char* pass);
int rawCloseAuthentication(pNetManagedConn this, char* user);
pNetSession rawOpenSession(pNetManagedConn this, char* user);
int rawCloseSession(pNetSession this);
int rawOpenBatch(pNetSession this);
int rawCloseBatch(pNetSession this);
int rawSendCommand(pNetSession this, int command_id, ...);
int rawNextResponse(pNetSession this, int skip_to_id, int* command_id, int* is_err, int* code, int* n_params);

/*** support functions - server only ***/
int rawSetSessionHandler(pNetManagedConn, int ((*callback_fn)()));
int rawSetAuthHandler(pNetManagedConn, int ((*callback_fn)()));
int rawNextBatch(pNetSession this, int* n_commands);
int rawNextCommand(pNetSession this, int* command_id, int* flags, int* n_params);

#endif /* NET_RAW_API_H */
