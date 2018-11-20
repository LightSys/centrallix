#ifndef _APPLICATION_H
#define _APPLICATION_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2018 LightSys Technology Services, Inc.		*/
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
/* Module:	application management layer                            */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	November 17, 2018                                       */
/*									*/
/* Description:	This module provides the application context layer for 	*/
/*		the Centrallix application platform.			*/
/************************************************************************/


/*** Globals for the AML ***/
typedef struct
    {
    XArray	Applications;	/* running applications */
    XHashTable	AppsByKey;
    }
    AML_t;

extern AML_t AML;


/*** One-page app data.  Each time the user launches an .app, a new app
 *** structure is created with a new key.
 ***/
typedef struct
    {
    char	Key[256];	/* full "akey" value, including sess/group/app keys */
    XHashTable	AppData;	/* of pAppData, lookup by Key */
    }
    Application, *pApplication;


/*** App data information - appRegisterAppData()
 ***/
typedef struct
    {
    char*	Key;		/* lookup key for app data */
    void*	Data;		/* the data itself */
    int		(*Finalize)();	/* a function to call when we're getting rid of the data */
    }
    AppData, *pAppData;


/*** AML Initialization ***/
int appInitialize();

/*** Application management functions ***/
pApplication appCreate(char* akey);
int appResume(pApplication app);
int appDestroy(pApplication app);

/*** Application context data management functions ***/
int appRegisterAppData(char* datakey, void* data, int (*finalize_fn)());
void* appLookupAppData(char* datakey);


#endif /* not defined _APPLICATION_H */

