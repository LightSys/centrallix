#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2003 LightSys Technology Services, Inc.		*/
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
/* Creation:	May 3, 2003     					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.    */
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/*		obj_inherit.c:  Implements the dynamic inheritance	*/
/*		mechanism in the OSML					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: obj_inherit.c,v 1.1 2003/05/30 17:39:52 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/objectsystem/obj_inherit.c,v $

    $Log: obj_inherit.c,v $
    Revision 1.1  2003/05/30 17:39:52  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

 **END-CVSDATA***********************************************************/


/** Data structures for handling the call indirection **/
typedef struct
    {
    pObject	Obj;
    void*	LLParam;
    }
    ObjInhPtr, *pObjInhPtr;

typedef struct
    {
    pObject	Obj;
    pObjQuery	Query;
    void*	LLParam;
    }
    ObjInhQuery, *pObjInhQuery;



/*** oihOpen - open a new object.
 ***/
void*
oihOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjInhTree* oxt)
    {
    pObjInhPtr inf;

    	/** Allocate the inf. **/
	inf = (pObjInhPtr)nmMalloc(sizeof(ObjInhPtr));
	if (!inf) return NULL;
	inf->Obj = obj;

	/** Make the driver call. **/
	inf->LLParam = obj->ILowLevelDriver->Open(obj, mask, systype, usrtype, oxt);

	/** Did it fail?  Remove any new oih if so. **/
	if ((inf->LLParam) == NULL)
	    {
	    nmFree(inf,sizeof(ObjInhPtr));
	    inf = NULL;
	    }

    return (void*)inf;
    }


/*** oihClose - close an open object of some sort.
 ***/
int
oihClose(void* this_v, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)(this_v);
    int rval;
    
    	/** Call the driver to make the close operation. **/
	rval = this->Obj->ILowLevelDriver->Close(this->LLParam, oxt);

	/** Free the ptr structure. **/
	nmFree(this,sizeof(ObjInhPtr));

    return rval;
    }


/*** oihCommit - commit changes
 ***/
int
oihCommit(void* this_v, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)(this_v);
    int rval;

    	/** Call the driver to make the close operation. **/
	if (!this->Obj->ILowLevelDriver->Commit) return 0;
	rval = this->Obj->ILowLevelDriver->Commit(this->LLParam, oxt);

    return rval;
    }


/*** oihCreate - create a new object without opening it.  Similar
 *** logic to oihOpen.
 ***/
int
oihCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjInhTree* oxt)
    {
    int rval;

	/** Make the driver call. **/
	rval = obj->ILowLevelDriver->Create(obj, mask, systype, usrtype, oxt);

    return rval;
    }


/*** oihDelete - delete an object.  Logic to find the object is similar to
 *** that found in oihCreate and oihOpen.
 ***/
int
oihDelete(pObject obj, pObjInhTree* oxt)
    {
    int rval;

	/** Make the driver call. **/
	rval = obj->ILowLevelDriver->Delete(obj, oxt);

    return rval;
    }


/*** oihSetAttrValue - set the value of an attribute.
 ***/
int
oihSetAttrValue(void* this_v, char* attrname, int datatype, void* val, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)(this_v);
    int rval;

	/** Call the driver. **/
	rval = this->Obj->ILowLevelDriver->SetAttrValue(this->LLParam, attrname, datatype, val, oxt);

    return rval;
    }


/*** oihOpenAttr - open an attribute as if it were an object, for
 *** reading and/or writing to its content.
 ***/
void*
oihOpenAttr(void* this_v, char* attrname, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)this_v;
    pObjInhPtr new_this;
    void* rval;

	/** Call the low level driver **/
	rval = this->Obj->ILowLevelDriver->OpenAttr(this->Obj,attrname,oxt);
	if (!rval) return NULL;

	/** Did the call succeed? **/
	if (rval)
	    {
	    new_this = (pObjInhPtr)nmMalloc(sizeof(ObjInhPtr));
	    if (!new_this) return NULL;
	    new_this->LLParam = rval;
	    new_this->Obj = this->Obj;
	    }

    return (void*)new_this;
    }


/*** oihGetAttrValue -- even this is passthru for now.  Sigh.
 ***/
int
oihGetAttrValue(void* this_v, char* attrname, int datatype, void* val, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)(this_v);
    return this->Obj->ILowLevelDriver->GetAttrValue(this->LLParam, attrname, datatype, val, oxt);
    }


/*** oihOpenQuery -- passthru for now.
 ***/
void*
oihOpenQuery(void* this_v, char* query, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)(this_v);
    pObjInhQuery qy;

    	/** Allocate the query **/
	qy = (pObjInhQuery)nmMalloc(sizeof(ObjInhQuery));
	if (!qy) return NULL;

	/** Call the low level driver. **/
	qy->Obj = this->Obj;
	qy->LLParam = this->Obj->ILowLevelDriver->OpenQuery(this->LLParam,query,oxt);
	if (!(qy->LLParam))
	    {
	    nmFree(qy,sizeof(ObjInhQuery));
	    return NULL;
	    }

    return (void*)qy;
    }


/*** oihQueryDelete -- passthru for now.
 ***/
int
oihQueryDelete(void* qy_v, pObjInhTree* oxt)
    {
    pObjInhQuery qy = (pObjInhQuery)(qy_v);
    return qy->Obj->ILowLevelDriver->QueryDelete(qy->LLParam, oxt);
    }


/*** oihQueryFetch -- passthru for now.
 ***/
void*
oihQueryFetch(void* qy_v, pObject obj, int mode, pObjInhTree* oxt)
    {
    pObjInhPtr subobj;
    pObjInhQuery qy = (pObjInhQuery)qy_v;

    	/** Allocate the subobject **/
	subobj = (pObjInhPtr)nmMalloc(sizeof(ObjInhPtr));
	if (!subobj) return NULL;
	
	/** Call the lowlevel driver **/
	subobj->LLParam = qy->Obj->ILowLevelDriver->QueryFetch(qy->LLParam, obj, mode, oxt);
	if (!(subobj->LLParam)) 
	    {
	    nmFree(subobj,sizeof(ObjInhPtr));
	    return NULL;
	    }
	subobj->Obj = obj;

    return subobj;
    }


/*** oihQueryClose -- passthru for now.
 ***/
int
oihQueryClose(void* qy_v, pObjInhTree* oxt)
    {
    pObjInhQuery qy = (pObjInhQuery)qy_v;

    	/** Free the object and call the lowlevel. **/
	qy->Obj->ILowLevelDriver->QueryClose(qy->LLParam,oxt);
	nmFree(qy,sizeof(ObjInhQuery));

    return 0;
    }


/*** oihWrite -- passthru for now.  Later will be included as a part of
 *** the trans layer.
 ***/
int
oihWrite(void* this_v, char* buffer, int cnt, int offset, int flags, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)(this_v);
    return this->Obj->ILowLevelDriver->Write(this->LLParam,buffer,cnt,offset,flags,oxt);
    }


/*** oihRead -- passthru to lowlevel for now.  Read and Write will
 *** not be passthru later when we implement the whole trans layer.
 ***/
int
oihRead(void* this_v, char* buffer, int maxcnt, int offset, int flags, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)(this_v);
    return this->Obj->ILowLevelDriver->Read(this->LLParam,buffer,maxcnt,offset,flags,oxt);
    }


/*** oihGetAttrType -- passthru to lowlevel.
 ***/
int
oihGetAttrType(void* this_v, char* attrname, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)(this_v);
    return this->Obj->ILowLevelDriver->GetAttrType(this->LLParam,attrname,oxt);
    }


/*** oihAddAttr -- passthru to lowlevel for now.  If we ever add more
 *** transaction functionality, this may have to change.
 ***/
int
oihAddAttr(void* this_v, char* attrname, int type, void* val, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)(this_v);
    return this->Obj->ILowLevelDriver->AddAttr(this->LLParam, oxt);
    }


/*** oihGetFirstAttr - passthru to lowlevel.
 ***/
char*
oihGetFirstAttr(void* this_v, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)(this_v);
    return this->Obj->ILowLevelDriver->GetFirstAttr(this->LLParam,oxt);
    }


/*** oihGetNextAttr - passthru to lowlevel.
 ***/
char*
oihGetNextAttr(void* this_v, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)(this_v);
    return this->Obj->ILowLevelDriver->GetNextAttr(this->LLParam,oxt);
    }


/*** oihGetFirstMethod - passthru to lowlevel.
 ***/
char*
oihGetFirstMethod(void* this_v, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)(this_v);
    return this->Obj->ILowLevelDriver->GetFirstMethod(this->LLParam,oxt);
    }


/*** oihGetNextMethod - passthru to lowlevel.
 ***/
char*
oihGetNextMethod(void* this_v, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)(this_v);
    return this->Obj->ILowLevelDriver->GetNextMethod(this->LLParam,oxt);
    }


/*** oihExecuteMethod - passthru to lowlevel.
 ***/
int
oihExecuteMethod(void* this_v, char* methodname, void* param, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)(this_v);
    return this->Obj->ILowLevelDriver->ExecuteMethod(this->LLParam,methodname,param,oxt);
    }


/*** oihPresentationHints - passthru.
 ***/
pObjPresentationHints
oihPresentationHints(void* this_v, char* attrname, pObjInhTree* oxt)
    {
    pObjInhPtr this = (pObjInhPtr)(this_v);
    return this->Obj->ILowLevelDriver->PresentationHints(this->LLParam,attrname,oxt);
    }


/*** oihInitialize - initialize the inheritance layer and register with the
 *** OSML.  The OSML will recognize us as the inheritance layer based on the
 *** flag OBJDRV_C_ISINHERIT and will treat the registration differently.
 ***/
int
oihInitialize()
    {
    pObjDriver drv;

    	/** Allocate the object. **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv)
	    {
	    mssError(0,"OIH","Initialize - could not allocate driver structure");
	    return -1;
	    }
	memset(drv,0,sizeof(ObjDriver));

	/** Fill in the function members and capabilities, etc **/
	strcpy(drv->Name,"OIH - ObjectSystem Dynamic Inheritance Layer");
	drv->Capabilities = OBJDRV_C_LLQUERY | OBJDRV_C_ISINHERIT;
	xaInit(&(drv->RootContentTypes),16);
	drv->Open = oihOpen;
	drv->Close = oihClose;
	drv->Delete = oihDelete;
	drv->Create = oihCreate;
	drv->Read = oihRead;
	drv->Write = oihWrite;
	drv->OpenQuery = oihOpenQuery;
	drv->QueryDelete = oihQueryDelete;
	drv->QueryFetch = oihQueryFetch;
	drv->QueryClose = oihQueryClose;
	drv->GetAttrType = oihGetAttrType;
	drv->GetAttrValue = oihGetAttrValue;
	drv->SetAttrValue = oihSetAttrValue;
	drv->AddAttr = oihAddAttr;
	drv->OpenAttr = oihOpenAttr;
	drv->GetFirstAttr = oihGetFirstAttr;
	drv->GetNextAttr = oihGetNextAttr;
	drv->GetFirstMethod = oihGetFirstMethod;
	drv->GetNextMethod = oihGetNextMethod;
	drv->ExecuteMethod = oihExecuteMethod;
	drv->PresentationHints = oihPresentationHints;
	drv->Commit = oihCommit;

	nmRegister(sizeof(ObjInhPtr),"ObjInhPtr");
	nmRegister(sizeof(ObjInhQuery),"ObjInhQuery");

	/** Register it now. **/
	objRegisterDriver(drv);

    return 0;
    }

