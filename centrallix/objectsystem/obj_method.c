#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
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



/*** Gets the name of an object's first method.  Use objGetNextMethod() to get
 *** the rest.
 ***
 *** @param this The object.
 *** @returns The method's name, or NULL if the object has no methods.
 ***/
char* 
objGetFirstMethod(pObject this)
    {
    return this->Driver->GetFirstMethod(this->Data, &(this->Session->Trx));
    }


/*** Gets the name of an object's next method, after calling
 *** objGetFirstMethod().
 ***
 *** @param this The object.
 *** @returns The method's name, or NULL after the last method.
 ***/
char* 
objGetNextMethod(pObject this)
    {
    return this->Driver->GetNextMethod(this->Data, &(this->Session->Trx));
    }


/*** Runs a method on an object.  Methods are driver-specific actions, such as
 *** clearing a cache.
 ***
 *** @param this The object.
 *** @param methodname The method's name.
 *** @param param The method's string parameter, e.g. POD(&str).
 *** @returns 0 if successful, or -1 if an error occurs.
 ***/
int 
objExecuteMethod(pObject this, char* methodname, pObjData param)
    {
    return this->Driver->ExecuteMethod(this->Data, methodname, param, &(this->Session->Trx));
    }
