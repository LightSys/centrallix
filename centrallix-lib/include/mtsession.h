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

/**CVSDATA***************************************************************

    $Id: mtsession.h,v 1.1 2001/08/13 18:04:19 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/mtsession.h,v $

    $Log: mtsession.h,v $
    Revision 1.1  2001/08/13 18:04:19  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/07/03 01:03:01  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


#include "mtask.h"
#include "xarray.h"
#include "xstring.h"
#include "xhash.h"


/** Structure for a session. **/
typedef struct
    {
    int		UserID;
    char	UserName[32];
    char	Password[32];
    XArray	ErrList;
    XHashTable	Params;
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
int mssInitialize();
char* mssUserName();
char* mssPassword();
int mssAuthenticate(char* username, char* password);
int mssEndSession();
int mssSetParam(char* paramname, void* param);
void* mssGetParam(char* paramname);

/** Error handling functions **/
int mssError(int clr, char* module, char* message, ...);
int mssErrorErrno(int clr, char* module, char* message, ...);
int mssClearError();
int mssPrintError(pFile fd);
int mssStringError(pXString str);


#endif
