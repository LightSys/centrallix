#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/magic.h"

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
	this->Magic = MGK_OBJSESSION;
	xhqInit(&(this->DirectoryCache), 256, 0, 509, obj_internal_DiscardDC, 0);
	this->Handle = xhnAllocHandle(&(OSYS.SessionHandleCtx), this);

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

	ASSERTMAGIC(this, MGK_OBJSESSION);

	xhnFreeHandle(&(OSYS.SessionHandleCtx), this->Handle);

	/** Uncache any cached node objects **/
	xhqDeInit(&(this->DirectoryCache));

	/** Close any open queries **/
	while(this->OpenQueries.nItems)
	    {
	    objQueryClose((pObjQuery)(this->OpenQueries.Items[0]));
	    }

	/** Close any open objects **/
	while(this->OpenObjects.nItems)
	    {
	    objClose((pObject)(this->OpenObjects.Items[0]));
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
    ASSERTMAGIC(this, MGK_OBJSESSION);
    if (!strcmp(wd->Pathname->Pathbuf,".")) strcpy(this->CurrentDirectory,"/");
    else strcpy(this->CurrentDirectory, wd->Pathname->Pathbuf+1);
    return 0;
    }


/*** objGetWD - gets the current working directory.
 ***/
char*
objGetWD(pObjSession this)
    {
    ASSERTMAGIC(this, MGK_OBJSESSION);
    return this->CurrentDirectory;
    }


/*** objUnmanageObject - removes an object from the list of open objects
 *** associated with this session.  This basically means that the object
 *** won't be auto-closed when the session is closed.  This should ONLY
 *** be used when a module can guarantee that the object will otherwise
 *** be closed when the session is closed, such as with open objects
 *** and queries used internally by the multiquery module.
 ***/
int
objUnmanageObject(pObjSession this, pObject obj)
    {
    xaRemoveItem(&(this->OpenObjects), xaFindItem(&(this->OpenObjects), (void*)obj));
    return 0;
    }


/*** objUnmanageQuery - removes a query from the list of open queries
 *** associated with this session.  This basically means that the query
 *** won't be auto-closed when the session is closed.  This should ONLY
 *** be used when a module can guarantee that the query will otherwise
 *** be closed when the session is closed, such as with open objects
 *** and queries used internally by the multiquery module.
 ***/
int
objUnmanageQuery(pObjSession this, pObjQuery qy)
    {
    xaRemoveItem(&(this->OpenQueries), xaFindItem(&(this->OpenQueries), (void*)qy));
    return 0;
    }


/*** objCommit - commit changes made during a transaction.  Only partly
 *** implemented at present, and only if supported by the underlying
 *** driver.
 ***/
int
objCommit(pObjSession this)
    {
    int rval = 0;
    pObject open_obj;

	/** For each open object in the session's transaction tree, do
	 ** a commit operation.  Just top-level object for now (simple case).
	 **/
	if (!this->Trx) return 0;
	open_obj = this->Trx->Object;
	if (open_obj && open_obj->Driver->Commit)
	    rval = open_obj->Driver->Commit(open_obj->Data, &(this->Trx));

    return rval;
    }

