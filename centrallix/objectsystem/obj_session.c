#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	obj.h, obj_*.c    					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 26, 1998					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.    */
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/*		--> obj_session.c: implements the session-management of	*/
/*		the objectsystem.					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: obj_session.c,v 1.1 2001/08/13 18:00:59 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/objectsystem/obj_session.c,v $

    $Log: obj_session.c,v $
    Revision 1.1  2001/08/13 18:00:59  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/08/07 02:31:01  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** objOpenSession - start a new session with a specific current
 *** directory.  If current_dir is NULL, then the new session is started
 *** in the home directory of the calling user.
 ***/
pObjSession 
objOpenSession(char* current_dir)
    {
    pObjSession this;

	/** Allocate the session **/
	this = (pObjSession)nmMalloc(sizeof(ObjSession));
	if (!this) return NULL;

	/** Setup the session structure **/
	xaInit(&(this->OpenObjects),16);
	xaInit(&(this->OpenQueries),16);
	strncpy(this->CurrentDirectory,current_dir,255);
	this->CurrentDirectory[255]=0;
	this->Trx = NULL;

	/** Add to the sessions list **/
	xaAddItem(&(OSYS.OpenSessions),(void*)this);

    return this;
    }


/*** objCloseSession - closes an open session and closes any objects,
 *** queries, and attributes that were open via this session.
 ***/
int 
objCloseSession(pObjSession this)
    {
    int i;

	/** Close any open queries **/
	for(i=0;i<this->OpenQueries.nItems;i++)
	    {
	    objQueryClose((pObjQuery)(this->OpenQueries.Items[i]));
	    }

	/** Close any open objects **/
	for(i=0;i<this->OpenObjects.nItems;i++)
	    {
	    objClose((pObject)(this->OpenObjects.Items[i]));
	    }

	/** Remove from the session list **/
	xaRemoveItem(&(OSYS.OpenSessions),xaFindItem(&(OSYS.OpenSessions),(void*)this));

	/** Free the memory **/
	if (this->Trx) obj_internal_FreeTree(this->Trx);
	xaDeInit(&(this->OpenObjects));
	xaDeInit(&(this->OpenQueries));
	nmFree(this,sizeof(ObjSession));

    return 0;
    }


/*** objSetWD - sets the current working directory to that of an open
 *** object.
 ***/
int
objSetWD(pObjSession this, pObject wd)
    {
    if (!strcmp(wd->Pathname->Pathbuf,".")) strcpy(this->CurrentDirectory,"/");
    else strcpy(this->CurrentDirectory, wd->Pathname->Pathbuf+1);
    return 0;
    }


/*** objGetWD - gets the current working directory.
 ***/
char*
objGetWD(pObjSession this)
    {
    return this->CurrentDirectory;
    }

