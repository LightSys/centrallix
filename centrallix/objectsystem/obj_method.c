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
/*		--> obj_method.c: implements the functions to access	*/
/*		methods within objects.					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: obj_method.c,v 1.2 2003/05/30 17:39:52 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/objectsystem/obj_method.c,v $

    $Log: obj_method.c,v $
    Revision 1.2  2003/05/30 17:39:52  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.1.1.1  2001/08/13 18:00:58  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:31:00  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** objGetFirstMethod - get the name of the first method associated with
 *** the object.  If the object has no methods, this function returns NULL.
 ***/
char* 
objGetFirstMethod(pObject this)
    {
    return this->Driver->GetFirstMethod(this->Data, &(this->Session->Trx));
    }


/*** objGetNextMethod - gets the name of the next method associated with
 *** an object.  Call this after a successful call to GetFirstMethod to
 *** repeatedly enumerate methods.  Returns NULL if the last method was
 *** already returned earlier or if the object has no methods.
 ***/
char* 
objGetNextMethod(pObject this)
    {
    return this->Driver->GetNextMethod(this->Data, &(this->Session->Trx));
    }


/*** objExecuteMethod - executes a given method with a method- and object-
 *** dependent parameter.
 ***/
int 
objExecuteMethod(pObject this, char* methodname, void* param)
    {
    return this->Driver->ExecuteMethod(this->Data, methodname, param, &(this->Session->Trx));
    }


