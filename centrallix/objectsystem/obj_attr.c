#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "stparse.h"
#include "expression.h"
#include "cxlib/magic.h"
#include "cxlib/util.h"


/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2004 LightSys Technology Services, Inc.		*/
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

    $Id: obj_attr.c,v 1.19 2010/09/13 23:30:29 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/objectsystem/obj_attr.c,v $

    $Log: obj_attr.c,v $
    Revision 1.19  2010/09/13 23:30:29  gbeeley
    - (admin) prepping for 0.9.1 release, update text files, etc.
    - (change) removing some 'unused local variables'

    Revision 1.18  2010/09/09 01:33:27  gbeeley
    - (feature) adding cx__pathname and cx__pathpartN system attributes
      which can be retrieved on an object.  Useful for when doing a WILDCARD
      select and needing to find out what filled the wildcard spots in the
      pathname.

    Revision 1.17  2009/07/14 22:08:08  gbeeley
    - (feature) adding cx__download_as object attribute which is used by the
      HTTP interface to set the content disposition filename.
    - (feature) adding "filename" property to the report writer to use the
      cx__download_as feature to specify a filename to the browser to "Save
      As...", so reports have a more intelligent name than just "report.rpt"
      (or whatnot) when downloaded.

    Revision 1.16  2009/06/26 18:34:28  gbeeley
    - (change) prevent virtual attributes from being added to an object if the
      property already exists in the object itself
    - (bugfix) runserver() expression fixes within OSML

    Revision 1.15  2007/09/18 18:03:14  gbeeley
    - (bugfix) when doing runserver() stuff in attributes, do not assume that
      the underlying driver knows how to tell us that the "standard four"
      attributes are all DATA_T_STRING.

    Revision 1.14  2007/04/08 03:52:00  gbeeley
    - (bugfix) various code quality fixes, including removal of memory leaks,
      removal of unused local variables (which create compiler warnings),
      fixes to code that inadvertently accessed memory that had already been
      free()ed, etc.
    - (feature) ability to link in libCentrallix statically for debugging and
      performance testing.
    - Have a Happy Easter, everyone.  It's a great day to celebrate :)

    Revision 1.13  2007/03/21 04:48:09  gbeeley
    - (feature) component multi-instantiation.
    - (feature) component Destroy now works correctly, and "should" free the
      component up for the garbage collector in the browser to clean it up.
    - (feature) application, component, and report parameters now work and
      are normalized across those three.  Adding "widget/parameter".
    - (feature) adding "Submit" action on the form widget - causes the form
      to be submitted as parameters to a component, or when loading a new
      application or report.
    - (change) allow the label widget to receive obscure/reveal events.
    - (bugfix) prevent osrc Sync from causing an infinite loop of sync's.
    - (bugfix) use HAVING clause in an osrc if the WHERE clause is already
      spoken for.  This is not a good long-term solution as it will be
      inefficient in many cases.  The AML should address this issue.
    - (feature) add "Please Wait..." indication when there are things going
      on in the background.  Not very polished yet, but it basically works.
    - (change) recognize both null and NULL as a null value in the SQL parsing.
    - (feature) adding objSetEvalContext() functionality to permit automatic
      handling of runserver() expressions within the OSML API.  Facilitates
      app and component parameters.
    - (feature) allow sql= value in queries inside a report to be runserver()
      and thus dynamically built.

    Revision 1.12  2007/03/06 16:16:55  gbeeley
    - (security) Implementing recursion depth / stack usage checks in
      certain critical areas.
    - (feature) Adding ExecMethod capability to sysinfo driver.

    Revision 1.11  2005/09/24 20:15:43  gbeeley
    - Adding objAddVirtualAttr() to the OSML API, which can be used to add
      an attribute to an object which invokes callback functions to get the
      attribute values, etc.
    - Changing objLinkTo() to return the linked-to object (same thing that
      got passed in, but good for style in reference counting).
    - Cleanup of some memory leak issues in objOpenQuery()

    Revision 1.10  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.9  2004/07/02 00:23:24  mmcgill
    Changes include, but are not necessarily limitted to:
        - fixed test_obj hints printing, added printing of hints to show command
        to make them easier to read.
        - added objDuplicateHints, for making deep copies of hints structures.
        - made sure GroupID and VisualLength2 were set to their proper defualts
          inf objPresentationHints() [obj_attr.c]
        - did a bit of restructuring in the sybase OS driver:
    	* moved the type conversion stuff in sybdGetAttrValue into a seperate
    	  function (sybd_internal_GetCxValue, sybd_internal_GetCxType). In
    	* Got rid of the Types union, made it an ObjData struct instead
    	* Stored column lengths in ColLengths
    	* Fixed a couple minor bugs
        - Roughed out a preliminary hints implementation for the sybase driver,
          in such a way that it shouldn't be *too* big a deal to add support for
          user-defined types.

    Revision 1.8  2004/06/12 04:02:28  gbeeley
    - preliminary support for client notification when an object is modified.
      This is a part of a "replication to the client" test-of-technology.

    Revision 1.7  2003/08/01 18:51:30  gbeeley
    Move a little bit of work out of the osdrivers in GetAttrType.

    Revision 1.6  2003/03/10 15:41:42  lkehresman
    The CSV objectsystem driver (objdrv_datafile.c) now presents the presentation
    hints to the OSML.  To do this I had to:
      * Move obj_internal_InfToHints() to a global function objInfToHints.  This
        is now located in utility/hints.c and the include is in include/hints.h.
      * Added the presentation hints function to the CSV driver and called it
        datPresentationHints() which returns a valid objPresentationHints object.
      * Modified test_obj.c to fix a crash bug and reformatted the output to be
        a little bit easier to read.
      * Added utility/hints.c to Makefile.in (somebody please check and make sure
        that I did this correctly).  Note that you will have to reconfigure
        centrallix for this change to take effect.

    Revision 1.5  2002/11/22 19:29:37  gbeeley
    Fixed some integer return value checking so that it checks for failure
    as "< 0" and success as ">= 0" instead of "== -1" and "!= -1".  This
    will allow us to pass error codes in the return value, such as something
    like "return -ENOMEM;" or "return -EACCESS;".

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


/*** objAddVirtualAttr() - adds a virtual attribute to an open object.  The
 *** virtual attribute only persists during the time the object is open,
 *** and is managed via external functions.
 ***/
int
objAddVirtualAttr(pObject this, char* attrname, void* context, int (*type_fn)(), int (*get_fn)(), int (*set_fn)(), int (*finalize_fn)())
    {
    pObjVirtualAttr va;
    char* findattr;

	ASSERTMAGIC(this, MGK_OBJECT);

	/** Must not already exist **/
	if (!strcmp(attrname, "objcontent") || !strcmp(attrname,"name") || !strcmp(attrname,"cx__pathname") ||
		!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type") ||
		!strcmp(attrname,"outer_type") || !strcmp(attrname,"annotation") || !strncmp(attrname,"cx__pathpart",12))
	    {
	    mssError(1, "OSML", "Virtual Attribute '%s' already exists in object.", attrname);
	    return -1;
	    }
	for(findattr=objGetFirstAttr(this); findattr; findattr=objGetNextAttr(this))
	    {
	    if (!strcmp(findattr, attrname))
		{
		mssError(1, "OSML", "Virtual Attribute '%s' already exists in object.", attrname);
		return -1;
		}
	    }

	/** Set it up **/
	va = (pObjVirtualAttr)nmMalloc(sizeof(ObjVirtualAttr));
	if (!va) return -1;
	memccpy(va->Name, attrname, 0, 32);
	va->Name[31] = '\0';
	va->Context = context;
	va->TypeFn = type_fn;
	va->GetFn = get_fn;
	va->SetFn = set_fn;
	va->FinalizeFn = finalize_fn;

	/** Link in with existing vattrs **/
	va->Next = this->VAttrs;
	this->VAttrs = va;

    return 0;
    }


/*** objGetAttrType -- returns the data type of a particular attribute.
 *** Data types are DATA_T_xxx, as defined in obj.h
 ***/
int
objGetAttrType(pObject this, char* attrname)
    {
    pObjVirtualAttr va;
    int rval, expval;
    pExpression exp;
    int n;
    char* endptr;

	ASSERTMAGIC(this, MGK_OBJECT);

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"OSML","Could not get attribute: resource exhaustion occurred");
	    return -1;
	    }

    	/** Builtin attribute 'objcontent' is always a string **/
	if (!strcmp(attrname, "objcontent")) return DATA_T_STRING;

	/** These 'system' attributes are also always strings **/
	if (!strcmp(attrname,"name") || !strcmp(attrname,"cx__pathname")) return DATA_T_STRING;
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type") ||
		!strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

	if (!strncmp(attrname,"cx__pathpart", 12))
	    {
	    endptr = NULL;
	    n = strtoul(attrname+12, &endptr, 10);
	    if (!n || !endptr || *endptr)
		{
		mssError(1,"OSML","Invalid cx__pathpart attribute: %s", attrname);
		return -1;
		}
	    return DATA_T_STRING;
	    }

	/** download-as attribute **/
	if (!strcmp(attrname, "cx__download_as"))
	    {
	    if (this->Driver->Capabilities & OBJDRV_C_DOWNLOADAS)
		return DATA_T_STRING;
	    else
		return -1;
	    }

	/** Virtual attrs **/
	for(va=this->VAttrs; va; va=va->Next)
	    {
	    if (!strcmp(attrname, va->Name))
		return va->TypeFn(this->Session, this, attrname, va->Context);
	    }

	/** Get the type from the lowlevel driver **/
	rval = this->Driver->GetAttrType(this->Data,attrname,&(this->Session->Trx));

	if (this->EvalContext && rval == DATA_T_CODE && (!this->AttrExpName || strcmp(attrname, this->AttrExpName)))
	    {
	    if (this->Driver->GetAttrValue(this->Data, attrname, rval, POD(&exp), &(this->Session->Trx)) == 0)
		{
		if (exp->Flags & (EXPR_F_RUNSERVER))
		    {
		    exp = expDuplicateExpression(exp);
		    if (exp)
			{
			if (this->AttrExpName)
			    nmSysFree(this->AttrExpName);
			this->AttrExpName = nmSysStrdup(attrname);
			if (this->AttrExp)
			    expFreeExpression(this->AttrExp);
			this->AttrExp = exp;
			}
		    }
		}
	    }

	/** Already an attr exp for this? **/
	if (this->EvalContext && rval == DATA_T_CODE && this->AttrExpName && !strcmp(this->AttrExpName, attrname))
	    {
	    expBindExpression(this->AttrExp, this->EvalContext, EXPR_F_RUNSERVER);
	    if ((expval = expEvalTree(this->AttrExp, this->EvalContext)) >= 0)
		{
		rval = ((pExpression)(this->AttrExp))->DataType;
		}
	    }

    return rval;
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
    pObjVirtualAttr va;
    int osmltype;
    pExpression exp;
    int used_expr;
    unsigned long n;
    char* endptr;
    int is_system_attr = 0;

	ASSERTMAGIC(this, MGK_OBJECT);

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"OSML","Could not get attribute: resource exhaustion occurred");
	    return -1;
	    }

#ifdef _OBJATTR_CONV
	/** Caller is using OLD API syntax **/
	if (data_type < 0 || data_type > 256)
	    {
	    val = (pObjData)data_type;
	    data_type = objGetAttrType(this, attrname);
	    }
#endif

	/** download as **/
	if (!strcmp(attrname,"cx__download_as"))
	    {
	    if (!(this->Driver->Capabilities & OBJDRV_C_DOWNLOADAS))
		return -1;
	    if (data_type != DATA_T_STRING)
		{
		mssError(1,"OSML","Type mismatch in accessing 'cx__download_as' attribute");
		return -1;
		}
	    }

	/** Full pathname **/
	if (!strcmp(attrname,"cx__pathname"))
	    {
	    if (data_type != DATA_T_STRING)
		{
		mssError(1,"OSML","Type mismatch in retrieving '%s' attribute", attrname);
		return -1;
		}
	    val->String = objGetPathname(this);
	    is_system_attr = 1;
	    }

	/** Part of pathname **/
	if (!strncmp(attrname,"cx__pathpart", 12))
	    {
	    endptr = NULL;
	    n = strtoul(attrname+12, &endptr, 10);
	    if (!n || !endptr || *endptr)
		{
		mssError(1,"OSML","Invalid cx__pathpart attribute: %s", attrname);
		return -1;
		}
	    if (data_type != DATA_T_STRING)
		{
		mssError(1,"OSML","Type mismatch in retrieving '%s' attribute", attrname);
		return -1;
		}
	    /** null if past end of pathname **/
	    if (n >= this->Pathname->nElements)
		val->String = NULL;
	    val->String = obj_internal_PathPart(this->Pathname, n, 1);
	    is_system_attr = 1;
	    }

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
	    else maxbytes = strtoi(ptr,NULL,0);

	    /** Now read the content into the string. **/
	    bytes = 0;
	    while(bytes < maxbytes)
	        {
		maxread = 256;
		if (maxread > (maxbytes - bytes)) maxread = maxbytes - bytes;
		if (bytes == 0) readcnt = objRead(this, readbuf, maxread, 0, FD_U_SEEK);
		else readcnt = objRead(this, readbuf, maxread, 0,0);
		if (readcnt < 0) return readcnt;
		if (readcnt == 0) break;
		xsConcatenate(this->ContentPtr, readbuf, readcnt);
		bytes += readcnt;
		}
	    val->String = this->ContentPtr->String;
	    is_system_attr = 1;
	    }

	/** Virtual attrs **/
	for(va=this->VAttrs; va; va=va->Next)
	    {
	    if (!strcmp(attrname, va->Name))
		return va->GetFn(this->Session, this, attrname, va->Context, data_type, val);
	    }

	/** Get the type from the lowlevel driver **/
	used_expr = 0;
	if (!strcmp(attrname,"name") || !strcmp(attrname,"inner_type") || !strcmp(attrname,"outer_type") || !strcmp(attrname, "content_type") || !strcmp(attrname, "annotation"))
	    osmltype = DATA_T_STRING;
	else
	    osmltype = this->Driver->GetAttrType(this->Data,attrname,&(this->Session->Trx));

	/** System attribute that can be overridden by MQ module? **/
	if (is_system_attr && (!(this->Driver->Capabilities & OBJDRV_C_ISMULTIQUERY) || osmltype <= 0))
	    {
	    if (!val->String)
		return 1;
	    else
		return 0;
	    }

	if (this->EvalContext && osmltype == DATA_T_CODE && (!this->AttrExpName || strcmp(attrname, this->AttrExpName)))
	    {
	    if (this->Driver->GetAttrValue(this->Data, attrname, osmltype, POD(&exp), &(this->Session->Trx)) == 0)
		{
		if (exp->Flags & (EXPR_F_RUNSERVER | EXPR_F_HASRUNSERVER))
		    {
		    exp = expDuplicateExpression(exp);
		    if (exp)
			{
			if (this->AttrExpName)
			    nmSysFree(this->AttrExpName);
			this->AttrExpName = nmSysStrdup(attrname);
			if (this->AttrExp)
			    expFreeExpression(this->AttrExp);
			this->AttrExp = exp;
			}
		    }
		}
	    }

	if (this->EvalContext && osmltype == DATA_T_CODE && this->AttrExpName && !strcmp(this->AttrExpName, attrname))
	    {
	    expBindExpression(this->AttrExp, this->EvalContext, EXPR_F_RUNSERVER);
	    if ((rval = expEvalTree(this->AttrExp, this->EvalContext)) >= 0)
		{
		if ((((pExpression)(this->AttrExp))->Flags & EXPR_F_HASRUNSERVER) && data_type == DATA_T_CODE)
		    {
		    /** return exp tree **/
		    val->Generic = this->AttrExp;
		    rval = 0;
		    used_expr = 1;
		    }
		else
		    {
		    if (((pExpression)(this->AttrExp))->DataType != data_type)
			{
			mssError(1,"OSML","Type mismatch accessing value '%s'", attrname);
			return -1;
			}
		    if (((pExpression)(this->AttrExp))->Flags & EXPR_F_NULL)
			rval = 1;
		    used_expr = 1;
		    if (expExpressionToPod(this->AttrExp, data_type, val) < 0)
			rval = -1;
		    }
		}
	    }

	/** Call the driver. **/
	if (!used_expr)
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
    int rval;
    TObjData tod;
    pObjVirtualAttr va;
    
	ASSERTMAGIC(this, MGK_OBJECT);

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"OSML","Could not set attribute: resource exhaustion occurred");
	    return -1;
	    }

#ifdef _OBJATTR_CONV
	if (data_type < 0 || data_type > 256)
	    {
	    val = (pObjData)data_type;
	    data_type = objGetAttrType(this, attrname);
	    }
#endif

	/** Virtual attrs **/
	for(va=this->VAttrs; va; va=va->Next)
	    {
	    if (!strcmp(attrname, va->Name))
		return va->SetFn(this->Session, this, attrname, va->Context, data_type, val);
	    }

	rval = this->Driver->SetAttrValue(this->Data, attrname, data_type, val, &(this->Session->Trx));
	if (rval >= 0) 
	    {
	    memcpy(&(tod.Data), val, sizeof(ObjData));
	    tod.DataType = data_type;
	    tod.Flags = 0;
	    obj_internal_RnNotifyAttrib(this, attrname, &tod, 0);
	    /*str = objDataToStringTmp(data_type, (data_type == DATA_T_INTEGER || data_type == DATA_T_DOUBLE)?val:val->Generic, 0);
	    if (!str) str = "";
	    obj_internal_TrxLog(this, "setattr", "%STR&DQUOT,%INT,%STR&DQUOT", attrname, data_type, str);*/
	    
            /** Notify the new notification system of the change **/
            obj_internal_ObserverCheckObservers(objGetPathname(this), OBJ_OBSERVER_EVENT_MODIFY);
            }

    return rval;
    }


/*** objAddAttr - adds a new attribute to an object.  Not all objects
 *** support this.
 ***/
int
objAddAttr(pObject this, char* attrname, int type, pObjData val)
    {
    int rval;
    TObjData tod;
    
	ASSERTMAGIC(this, MGK_OBJECT);
	rval = this->Driver->AddAttr(this->Data, attrname, type, val, &(this->Session->Trx));
	if (rval >= 0) 
	    {
	    memcpy(&(tod.Data), val, sizeof(ObjData));
	    tod.DataType = type;
	    tod.Flags = 0;
	    obj_internal_RnNotifyAttrib(this, attrname, &tod, 0);
            
            /** Notify the new notification system of the change **/
            obj_internal_ObserverCheckObservers(objGetPathname(this), OBJ_OBSERVER_EVENT_MODIFY);
	    }

    return rval;
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
	/*obj = (pObject)nmMalloc(sizeof(Object));*/
	obj = obj_internal_AllocObj();
	if (!obj) return NULL;
	obj->Data = obj_data;
	obj->Obj = this;
	xaAddItem(&(this->Attrs), (void*)obj);
	obj->Driver = this->Driver;
	obj->Pathname = (pPathname)nmMalloc(sizeof(Pathname));
	obj->Pathname->OpenCtlBuf = NULL;
	obj_internal_CopyPath(obj->Pathname,this->Pathname);
	obj->SubPtr++;
	obj->Mode = mode;
	obj->Session = this->Session;

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
	    /** init the non-0 default values **/
	    ph->GroupID=-1;
	    ph->VisualLength2=1;
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

	nmFree(ph, sizeof(ObjPresentationHints));

    return 0;
    }


/*** objSetEvalContext() - sets an object list (objlist) to be used in
 *** the evaluation of property values that use runserver() expressions.
 ***/
int
objSetEvalContext(pObject this, void* objlist_v)
    {
    pParamObjects objlist = (pParamObjects)objlist_v;

	ASSERTMAGIC(this, MGK_OBJECT);
	
	this->EvalContext = (void*)objlist;

    return 0;
    }

