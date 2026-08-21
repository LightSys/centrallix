#ifndef _MTSESSION_H
#define _MTSESSION_H

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
/* Module: 	mtsession.c, mtsession.h				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 4, 1998  					*/
/* Description:	Session management module to complement the MTASK	*/
/*		module.  Maintains user/password authentication.	*/
/************************************************************************/



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


/** optimum salt size for mssGenCred() **/
#define	MSS_SALT_SIZE	4


/** Password and Username size **/
#define	MSS_PASSWORD_SIZE	64
#define MSS_USERNAME_SIZE	32


/** Structure for a session. **/
typedef struct
    {
    int		UserID;
    int		GroupID;
    char	UserName[MSS_USERNAME_SIZE];
    char	Password[MSS_PASSWORD_SIZE];
    XArray	ErrList;
    XHashTable	Params;
    int		LinkCnt;
    }
    MtSession, *pMtSession;


/** Parameter data **/
typedef struct
    {
    char	Name[32];
    char*	Value;
    char	ValueBuf[64];
    }
    MtParam, *pMtParam;


/** User auth Functions **/
int mssInitialize(char* authmethod, char* authfile, char* logmethod, int logall, char* log_progname);
char* mssUserName();
char* mssPassword();
int mssAuthenticate(char* username, char* password, int bypass_crypt);
int mssGenCred(char* salt, int salt_len, char* password, char* credential, int cred_maxlen);
int mssEndSession(pMtSession s);
int mssLinkSession(pMtSession s);
int mssUnlinkSession(pMtSession s);
int mssSetParam(char* paramname, void* param);
int mssSetParamPtr(char* paramname, void* ptr);
void* mssGetParam(char* paramname);

/** Error handling functions **/
int mssLog(int level, char* msg);
int mssError(int clr, char* module, char* message, ...);
int mssErrorErrno(int clr, char* module, char* message, ...);
int mssClearError();
int mssPrintError(pFile fd);
int mssStringError(pXString str);
int mssUserError(pXString str);


#endif
