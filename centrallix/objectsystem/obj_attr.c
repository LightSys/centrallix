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
		!strcmp(attrname,"outer_type") || !strcmp(attrname,"annotation") || !strncmp(attrname,"cx__pathpart",12) || 
		!strncmp(attrname,"content_charset",12))
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
	if (!strcmp(attrname,"annotation") || !strcmp(attrname,"content_charset")) return DATA_T_STRING;

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

	/*if (!strcmp(attrname, "cx__rowid"))
	    return DATA_T_INTEGER;*/

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

	/*if (!strcmp(attrname, "cx__rowid"))
	    {
	    if (data_type != DATA_T_INTEGER)
		{
		mssError(1,"OSML","Type mismatch in accessing 'cx__rowid' attribute");
		return -1;
		}
	    if (this->RowID >= 0)
		{
		val->Integer = this->RowID;
		return 0;
		}
	    else
		{
		return 1;
		}
	    }*/

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
	if (!strcmp(attrname,"name") || !strcmp(attrname,"inner_type") || !strcmp(attrname,"outer_type") ||
		!strcmp(attrname, "content_type") || !strcmp(attrname, "annotation") || !strcmp(attrname,"objcontent") ||
		!strcmp(attrname, "content_charset"))
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
	    if (objIsRelatedType(this->Type->Name, val->String) > 0)
	        {
	        val->String = this->Type->Name;
		}
	    }

	/** Ensure annotation is valid **/
	if (rval != 0 && !strcmp(attrname, "annotation"))
	    {
	    rval = 0;
	    val->String = "";
	    }
	
	/** Avoid any errors from drivers that do not suport content_charset **/
	if(rval < 0 && !strcmp(attrname, "content_charset"))
	    {
	    rval = 0;
	    val->String = NULL;
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

	/** Nulls not allowed on system attributes **/
	/** NOTE: for now, since content_charset is not widely supported, let it be set to NULL **/
	if (!val && (!strcmp(attrname,"name") || !strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type") || !strcmp(attrname,"outer_type")))
	    {
	    mssError(1, "OSML", "'%s' attribute cannot be set to NULL", attrname);
	    return -1;
	    }

    	/** How about content? **/
	if (!strcmp(attrname,"objcontent"))
	    {
	    if (data_type == DATA_T_STRING) 
		{
		/** String value **/
		if (!val)
		    rval = this->Driver->Write(this->Data, "", 0, 0, OBJ_U_SEEK | OBJ_U_TRUNCATE | OBJ_U_PACKET, &(this->Session->Trx));
		else
		    rval = this->Driver->Write(this->Data, val->String, strlen(val->String), 0, OBJ_U_SEEK | OBJ_U_TRUNCATE | OBJ_U_PACKET, &(this->Session->Trx));
		}
	    else if (data_type == DATA_T_BINARY)
		{
		/** Binary value **/
		if (!val || !val->Binary.Data)
		    rval = this->Driver->Write(this->Data, "", 0, 0, OBJ_U_SEEK | OBJ_U_TRUNCATE | OBJ_U_PACKET, &(this->Session->Trx));
		else
		    rval = this->Driver->Write(this->Data, val->Binary.Data, val->Binary.Size, 0, OBJ_U_SEEK | OBJ_U_TRUNCATE | OBJ_U_PACKET, &(this->Session->Trx));
		}
	    else
		{
		mssError(1,"OSML","Type mismatch in setting 'objcontent' attribute");
		return -1;
		}

	    return rval;
	    }

	/** Virtual attrs **/
	for(va=this->VAttrs; va; va=va->Next)
	    {
	    if (!strcmp(attrname, va->Name))
		{
		if (va->SetFn)
		    return va->SetFn(this->Session, this, attrname, va->Context, data_type, val);
		else
		    {
		    mssError(1,"OSML","Set Value: Attribute '%s' is readonly", attrname);
		    return -1;
		    }
		}
	    }

	rval = this->Driver->SetAttrValue(this->Data, attrname, data_type, val, &(this->Session->Trx));
	if (rval >= 0)
	    {
	    tod.Flags = 0;
	    if (val)
		memcpy(&(tod.Data), val, sizeof(ObjData));
	    else
		tod.Flags = DATA_TF_NULL;
	    tod.DataType = data_type;
	    obj_internal_RnNotifyAttrib(this, attrname, &tod, 0);
	    /*str = objDataToStringTmp(data_type, (data_type == DATA_T_INTEGER || data_type == DATA_T_DOUBLE)?val:val->Generic, 0);
	    if (!str) str = "";
	    obj_internal_TrxLog(this, "setattr", "%STR&DQUOT,%INT,%STR&DQUOT", attrname, data_type, str);*/
	    }

	/** Avoid any errors from drivers that do not suport content_charset **/
	if(rval < 0 && !strcmp(attrname, "content_charset"))
	    rval = 0;

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


/*** objGetEvalContext() - gets an object list (objlist) to be used in
 *** the evaluation of property values that use runserver() expressions.
 ***/
void*
objGetEvalContext(pObject this)
    {
    return this->EvalContext;
    }

