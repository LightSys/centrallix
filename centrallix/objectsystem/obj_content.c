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
/*		--> obj_content.c: contains implementation of the 	*/
/*		content-access (read/write) object methods.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: obj_content.c,v 1.1 2001/08/13 18:00:57 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/objectsystem/obj_content.c,v $

    $Log: obj_content.c,v $
    Revision 1.1  2001/08/13 18:00:57  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/08/07 02:30:59  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** objRead - read content from an object at a particular optional seek
 *** offset.  Very similar to MTask's fdRead().
 ***/
int 
objRead(pObject this, char* buffer, int maxcnt, int flags, int offset)
    {
    return this->Driver->Read(this->Data, buffer, maxcnt, flags, offset, &(this->Session->Trx));
    }


/*** objWrite - write content to an object at a particular optional seek
 *** offset.  Also very similar to MTask's fdWrite().
 ***/
int 
objWrite(pObject this, char* buffer, int cnt, int flags, int offset)
    {
    return this->Driver->Write(this->Data, buffer, cnt, flags, offset, &(this->Session->Trx));
    }

