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

    $Id: mtsession.h,v 1.4 2005/02/26 04:32:02 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/mtsession.h,v $

    $Log: mtsession.h,v $
    Revision 1.4  2005/02/26 04:32:02  gbeeley
    - moving include file install directory to include a "cxlib/" prefix
      instead of just putting 'em all in /usr/include with everything else.

    Revision 1.3  2002/11/12 00:26:49  gbeeley
    Updated MTASK approach to user/group security when using system auth.
    The module now handles group ID's as well.  Changes should have no
    effect when running as non-root with altpasswd auth.

    Revision 1.2  2002/02/14 00:41:54  gbeeley
    Added configurable logging and authentication to the mtsession module,
    and made sure mtsession cleared MtSession data structures when it is
    through with them since they contain sensitive data.

    Revision 1.1.1.1  2001/08/13 18:04:19  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:03:01  gbeeley
    Initial checkin of centrallix-lib


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


/** Structure for a session. **/
typedef struct
    {
    int		UserID;
    int		GroupID;
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
int mssInitialize(char* authmethod, char* authfile, char* logmethod, int logall, char* log_progname);
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
