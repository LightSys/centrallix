#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"
#include "stparse.h"
#include "expression.h"
#include "magic.h"

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
/*		--> obj_attr.c: implements the object attribute access	*/
/*		functions for the objectsystem.				*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: obj_attr.c,v 1.4 2002/08/10 02:09:45 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/objectsystem/obj_attr.c,v $

    $Log: obj_attr.c,v $
    Revision 1.4  2002/08/10 02:09:45  gbeeley
    Yowzers!  Implemented the first half of the conversion to the new
    specification for the obj[GS]etAttrValue OSML API functions, which
    causes the data type of the pObjData argument to be passed as well.
    This should improve robustness and add some flexibilty.  The changes
    made here include:

        * loosening of the definitions of those two function calls on a
          temporary basis,
        * modifying all current objectsystem drivers to reflect the new
          lower-level OSML API, including the builtin drivers obj_trx,
          obj_rootnode, and multiquery.
        * modification of these two functions in obj_attr.c to allow them
          to auto-sense the use of the old or new API,
        * Changing some dependencies on these functions, including the
          expSetParamFunctions() calls in various modules,
        * Adding type checking code to most objectsystem drivers.
        * Modifying *some* upper-level OSML API calls to the two functions
          in question.  Not all have been updated however (esp. htdrivers)!

    Revision 1.3  2002/04/25 17:59:59  gbeeley
    Added better magic number support in the OSML API.  ObjQuery and
    ObjSession structures are now protected with magic numbers, and
    support for magic numbers in Object structures has been improved
    a bit.

    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:00:57  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:59  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/*** objGetAttrType -- returns the data type of a particular attribute.
 *** Data types are DATA_T_xxx, as defined in obj.h
 ***/
int
objGetAttrType(pObject this, char* attrname)
    {

	ASSERTMAGIC(this, MGK_OBJECT);

    	/** Builtin attribute 'objcontent' is always a string **/
	if (!strcmp(attrname, "objcontent")) return DATA_T_STRING;

    return this->Driver->GetAttrType(this->Data,attrname,&(this->Session->Trx));
    }


/*** objGetAttrValue -- returns the integer, string, or other value of a
 *** particular attribute.
 ***/
int 
objGetAttrValue(pObject this, char* attrname, int data_type, pObjData val)
    {
    int bytes,maxbytes,maxread,readcnt;
    char readbuf[256];
    char* ptr;
    int rval;

	ASSERTMAGIC(this, MGK_OBJECT);

#ifdef _OBJATTR_CONV
	/** Caller is using OLD API syntax **/
	if (data_type < 0 || data_type > 256)
	    {
	    val = (pObjData)data_type;
	    data_type = objGetAttrType(this, attrname);
	    }
#endif

    	/** How about content? **/
	if (!strcmp(attrname,"objcontent"))
	    {
	    /** Check type **/
	    if (data_type != DATA_T_STRING) 
		{
		mssError(1,"OSML","Type mismatch in retrieving 'objcontent' attribute");
		return -1;
		}

	    /** Initialize the string **/
	    if (!this->ContentPtr)
	        {
		this->ContentPtr = (pXString)nmMalloc(sizeof(XString));
		xsInit(this->ContentPtr);
		}
	    else
	        {
		xsCopy(this->ContentPtr,"",-1);
		}

	    /** What is the max length? **/
	    ptr = (char*)mssGetParam("textsize");
	    if (!ptr) maxbytes = 65536;
	    else maxbytes = strtol(ptr,NULL,0);

	    /** Now read the content into the string. **/
	    bytes = 0;
	    while(bytes < maxbytes)
	        {
		maxread = 256;
		if (maxread > (maxbytes - bytes)) maxread = maxbytes - bytes;
		if (bytes == 0) readcnt = objRead(this, readbuf, maxread, 0, FD_U_SEEK);
		else readcnt = objRead(this, readbuf, maxread, 0,0);
		if (readcnt == -1) return -1;
		if (readcnt == 0) break;
		xsConcatenate(this->ContentPtr, readbuf, readcnt);
		bytes += readcnt;
		}
	    val->String = this->ContentPtr->String;
	    return 0;
	    }

	/** Call the driver. **/
	rval = this->Driver->GetAttrValue(this->Data, attrname, data_type, val, &(this->Session->Trx));

    	/** Inner/content type, and OSML has a better idea than driver? **/
	if ((!strcmp(attrname,"inner_type") || !strcmp(attrname,"content_type")) && rval==0 && this->Type)
	    {
	    if (obj_internal_IsA(this->Type->Name, val->String) > 0)
	        {
	        val->String = this->Type->Name;
		}
	    }
 
    return rval;
    }


/*** objGetFirstAttr -- gets the first attribute associated with the 
 *** object.  NOTE: will not return the 'name' or 'types' attributes.
 ***/
char*
objGetFirstAttr(pObject this)
    {
    ASSERTMAGIC(this, MGK_OBJECT);
    return this->Driver->GetFirstAttr(this->Data,&(this->Session->Trx));
    }


/*** objGetNextAttr -- gets the next attribute; call this multiple
 *** times after calling objGetFirstAttr until one of the functions
 *** returns NULL.
 ***/
char*
objGetNextAttr(pObject this)
    {
    ASSERTMAGIC(this, MGK_OBJECT);
    return this->Driver->GetNextAttr(this->Data, &(this->Session->Trx));
    }


/*** objSetAttrValue -- sets the value of an attribute.  An object 
 *** can be renamed by setting the "name" attribute.
 ***/
int 
objSetAttrValue(pObject this, char* attrname, int data_type, pObjData val)
    {
    
	ASSERTMAGIC(this, MGK_OBJECT);

#ifdef _OBJATTR_CONV
	if (data_type < 0 || data_type > 256)
	    {
	    val = (pObjData)data_type;
	    data_type = objGetAttrType(this, attrname);
	    }
#endif

    return this->Driver->SetAttrValue(this->Data, attrname, data_type, val, &(this->Session->Trx));
    }


/*** objAddAttr - adds a new attribute to an object.  Not all objects
 *** support this.
 ***/
int
objAddAttr(pObject this, char* attrname, int type, pObjData val)
    {
    ASSERTMAGIC(this, MGK_OBJECT);
    return this->Driver->AddAttr(this->Data, attrname, type, val, &(this->Session->Trx));
    }


/*** objOpenAttr -- open an attribute for reading/writing as if it were
 *** an independent object with content.
 ***/
pObject 
objOpenAttr(pObject this, char* attrname, int mode)
    {
    pObject obj;
    void* obj_data;

	ASSERTMAGIC(this, MGK_OBJECT);

	/** Verify length **/
	if (strlen(attrname) + strlen(this->Pathname->Pathbuf) + 1 > 255) return NULL;

	/** Open object with the driver. **/
	obj_data = this->Driver->OpenAttr(this->Data, attrname, mode, &(this->Session->Trx));
	if (!obj_data) return NULL;

	/** Allocate memory and initialize the object descriptor **/
	obj = (pObject)nmMalloc(sizeof(Object));
	if (!obj) return NULL;
	obj->Data = obj_data;
	obj->Obj = this;
	obj->Magic = MGK_OBJECT;
	xaAddItem(&(this->Attrs), (void*)obj);
	obj->Driver = this->Driver;
	obj->Pathname = (pPathname)nmMalloc(sizeof(Pathname));
	obj->Pathname->OpenCtlBuf = NULL;
	obj_internal_CopyPath(obj->Pathname,this->Pathname);
	obj->SubPtr++;
	obj->Mode = mode;
	obj->Session = this->Session;
	obj->ContentPtr = NULL;
	obj->Type = NULL;

    return obj;
    }


/*** objPresentationHints - return information on how a particular
 *** attribute should be presented to the end-user.  This is normally
 *** handled by the driver, but if the driver omits this function, this
 *** routine will make a 'best guess' approximation at some presentation
 *** hints.  The calling routine must take care to nmFree() the returned
 *** data structure.
 ***/
pObjPresentationHints
objPresentationHints(pObject this, char* attrname)
    {
    pObjPresentationHints ph = NULL;

	ASSERTMAGIC(this, MGK_OBJECT);

    	/** Check with lowlevel driver **/
	if (this->Driver->PresentationHints != NULL)
	    {
	    ph = this->Driver->PresentationHints(this->Data, attrname, &(this->Session->Trx));
	    }

	/** Didn't get any presentation-hints or no function in the driver? **/
	if (!ph)
	    {
	    ph = (pObjPresentationHints)nmMalloc(sizeof(ObjPresentationHints));
	    memset(ph,0,sizeof(ObjPresentationHints));
	    xaInit(&(ph->EnumList),16);
	    }

    return ph;
    }



/*** obj_internal_InfToHints - convert a StructInf structure tree to a
 *** presentation hints structure.  The StructInf should be of the format
 *** of a system/parameter.
 ***/
pObjPresentationHints
obj_internal_InfToHints(pStructInf inf, int data_type)
    {
    pObjPresentationHints ph;
    pParamObjects tmplist;
    char* ptr;
    char* newptr;
    int n,cnt;

    	/** Allocate a new ph structure **/
	ph = (pObjPresentationHints)nmMalloc(sizeof(ObjPresentationHints));
	memset(ph,0,sizeof(ObjPresentationHints));
	xaInit(&(ph->EnumList),16);

	/** Check for constraint, default, min, and max expressions. **/
	stAttrValue(stLookup(inf,"constraint"),NULL,(char**)&(ph->Constraint),0);
	stAttrValue(stLookup(inf,"default"),NULL,(char**)&(ph->DefaultExpr),0);
	stAttrValue(stLookup(inf,"min"),NULL,(char**)&(ph->MinValue),0);
	stAttrValue(stLookup(inf,"max"),NULL,(char**)&(ph->MaxValue),0);

	/** Compile expressions, if any **/
	if (ph->Constraint || ph->DefaultExpr || ph->MinValue || ph->MaxValue)
	    {
	    tmplist = expCreateParamList();
	    expAddParamToList(tmplist,"this",NULL,EXPR_O_CURRENT);
	    if (ph->Constraint)
	        {
		ph->Constraint = expCompileExpression(ph->Constraint, tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		if (!ph->Constraint)
		    {
		    mssError(0,"OSML","Error in 'constraint' expression");
		    nmFree(ph,sizeof(ObjPresentationHints));
		    expFreeParamList(tmplist);
		    return NULL;
		    }
		ph->DefaultExpr = expCompileExpression(ph->DefaultExpr, tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		if (!ph->DefaultExpr)
		    {
		    mssError(0,"OSML","Error in 'default' expression");
		    nmFree(ph,sizeof(ObjPresentationHints));
		    expFreeParamList(tmplist);
		    return NULL;
		    }
		ph->MinValue = expCompileExpression(ph->MinValue, tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		if (!ph->MinValue)
		    {
		    mssError(0,"OSML","Error in 'min' expression");
		    nmFree(ph,sizeof(ObjPresentationHints));
		    expFreeParamList(tmplist);
		    return NULL;
		    }
		ph->MaxValue = expCompileExpression(ph->MaxValue, tmplist, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		if (!ph->MaxValue)
		    {
		    mssError(0,"OSML","Error in 'max' expression");
		    nmFree(ph,sizeof(ObjPresentationHints));
		    expFreeParamList(tmplist);
		    return NULL;
		    }
		}
	    expFreeParamList(tmplist);
	    }

	/** Enumerated values list, given explicitly? **/
	ptr = NULL;
	cnt = 0;
	while(stAttrValue(stLookup(inf,"enumlist"),&n, &ptr, cnt) >= 0)
	    {
	    /** Check for string enum or integer enum. **/
	    if (ptr)
	        {
		newptr = nmSysStrdup(ptr);
		}
	    else
	        {
		newptr = (char*)nmSysMalloc(16);
		sprintf(newptr,"%d",n);
		}
	    xaAddItem(&(ph->EnumList), (void*)newptr);
	    cnt++;
	    ptr = NULL;
	    }

	/** Or, enumerated value list, given via query? **/
	if (cnt == 0 && stAttrValue(stLookup(inf,"enumquery"),NULL,&ptr,0) >= 0)
	    {
	    ph->EnumQuery = nmSysStrdup(ptr);
	    }

	/** Has a presentation format? **/
	if (stAttrValue(stLookup(inf,"format"),NULL,&ptr,0) >= 0)
	    {
	    ph->Format = nmSysStrdup(ptr);
	    }

	/** Set VisualLength attributes? **/
	if (stAttrValue(stLookup(inf,"length"),&n,NULL,0) >= 0)
	    {
	    ph->VisualLength = n;
	    }
	else
	    {
	    switch(data_type)
	        {
		case DATA_T_INTEGER: ph->VisualLength = 13; break;
		case DATA_T_STRING: ph->VisualLength = 32; break;
		case DATA_T_DATETIME: ph->VisualLength = 20; break;
		case DATA_T_MONEY: ph->VisualLength = 16; break;
		case DATA_T_DOUBLE: ph->VisualLength = 18; break;
		default: ph->VisualLength = 16; break;
		}
	    }
	if (stAttrValue(stLookup(inf,"height"),&n,NULL,0) >= 0)
	    ph->VisualLength2 = n;
	else
	    ph->VisualLength2 = 1;

	/** Check for read-only bits in a bitmask **/
	ph->BitmaskRO = 0;
	cnt = 0;
	while(stAttrValue(stLookup(inf,"readonlybits"), &n, NULL,cnt) >= 0)
	    {
	    ph->BitmaskRO |= (1<<n);
	    cnt++;
	    }

	/** Check for style information. **/
	cnt = 0;
	ph->Style = 0;
	while(stAttrValue(stLookup(inf,"style"),NULL,&ptr,cnt) >= 0)
	    {
	    if (!strcmp(ptr,"bitmask")) ph->Style |= OBJ_PH_STYLE_BITMASK;
	    else if (!strcmp(ptr,"list")) ph->Style |= OBJ_PH_STYLE_LIST;
	    else if (!strcmp(ptr,"buttons")) ph->Style |= OBJ_PH_STYLE_BUTTONS;
	    else if (!strcmp(ptr,"allownull")) ph->Style |= OBJ_PH_STYLE_ALLOWNULL;
	    else if (!strcmp(ptr,"strnull")) ph->Style |= OBJ_PH_STYLE_STRNULL;
	    else if (!strcmp(ptr,"grouped")) ph->Style |= OBJ_PH_STYLE_GROUPED;
	    else if (!strcmp(ptr,"readonly")) ph->Style |= OBJ_PH_STYLE_READONLY;
	    else if (!strcmp(ptr,"hidden")) ph->Style |= OBJ_PH_STYLE_HIDDEN;
	    else if (!strcmp(ptr,"password")) ph->Style |= OBJ_PH_STYLE_PASSWORD;
	    else if (!strcmp(ptr,"multiline")) ph->Style |= OBJ_PH_STYLE_MULTILINE;
	    }

	/** Check for group ID and Name **/
	ph->GroupID = -1;
	ph->GroupName = NULL;
	if (stAttrValue(stLookup(inf,"groupid"),&n,NULL,0) >= 0) ph->GroupID = n;
	if (stAttrValue(stLookup(inf,"groupname"),NULL,&ptr,0) >= 0)
	    {
	    ph->GroupName = nmSysStrdup(ptr);
	    }

	/** Description of field? **/
	if (stAttrValue(stLookup(inf,"description"),NULL,&ptr,0) >= 0)
	    {
	    ph->FriendlyName = nmSysStrdup(ptr);
	    }

    return ph;
    }


/*** obj_internal_FreeHints - release the Hints structure as well as its
 *** associated allocated strings and such.
 ***/
int
objFreeHints(pObjPresentationHints ph)
    {
    int i;

    	/** Free expressions? **/
	if (ph->Constraint) expFreeExpression(ph->Constraint);
	if (ph->DefaultExpr) expFreeExpression(ph->DefaultExpr);
	if (ph->MinValue) expFreeExpression(ph->MinValue);
	if (ph->MaxValue) expFreeExpression(ph->MaxValue);

	/** Free strings? **/
	if (ph->EnumQuery) nmSysFree(ph->EnumQuery);
	if (ph->Format) nmSysFree(ph->Format);
	if (ph->GroupName) nmSysFree(ph->GroupName);
	if (ph->FriendlyName) nmSysFree(ph->FriendlyName);

	/** How about the list of possible values? **/
	for(i=0;i<ph->EnumList.nItems;i++) nmSysFree(ph->EnumList.Items[i]);
	xaDeInit(&(ph->EnumList));

    return 0;
    }

