#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "cxlib/mtsession.h"
/** module definintions **/
#include "centrallix.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2011 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_cpp_wrapper.cpp         				*/
/* Author:	Micah Shennum                           		*/
/* Creation:	Jun 6 2011                                      	*/
/* Description:	This goes through the nasty bussness of binding a objdrv*/
/*               class into the object driver system                    */
/************************************************************************/

#include "objdrv.hpp"

/*** GLOBALS ***/
typedef struct
    {
    int		dmy_global_variable;
    }
    CPP_INF_t;
    
CPP_INF_t CPP_INF;

/*** cppOpen - open an object.
 ***/
void*
cppOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    objdrv *inf;
    char* node_path;
    pSnNode node = NULL;

	/** Allocate the structure! **/
        inf=GetInstance(obj,mask,systype,usrtype,oxt);
	if (!inf) return NULL;

	/** Determine the node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

	/** If CREAT and EXCL, we only create, failing if already exists. **/
	if ((obj->Mode & O_CREAT) && (obj->Mode & O_EXCL) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(obj->Prev, usrtype);
	    if (!node)
	        {
                delete inf;
		mssError(0,"CPP","Could not create new node object");
		return NULL;
		}
	    }
	
	/** Otherwise, try to open it first. **/
	if (!node)
	    {
	    node = snReadNode(obj->Prev);
	    }

	/** If no node, and user said CREAT ok, try that. **/
	if (!node && (obj->Mode & O_CREAT) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(obj->Prev, usrtype);
	    }

	/** If _still_ no node, quit out. **/
	if (!node)
	    {
	    delete inf;
	    mssError(0,"CPP","Could not open structure file");
	    return NULL;
	    }

	/** Set object params. **/
	inf->Node = node;
	inf->Pathname=std::string(obj_internal_PathPart(obj->Pathname,0,0));
	inf->Node->OpenCnt++;

    return (void*)inf;
    }


/*** cppClose - close an open object.
 ***/
int
cppClose(void* inf_v, pObjTrxTree* oxt)
    {
    objdrv *inf = (objdrv *)inf_v;
    inf->Close(oxt);
    	/** Write the node first, if need be. **/
	snWriteNode(inf->Obj->Prev, inf->Node);
	
	/** Release the memory **/
	inf->Node->OpenCnt --;
	delete inf;

    return 0;
    }

int objdrv::Close(pObjTrxTree* oxt){
    return 0;
}

/*** cppCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
cppCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = cppOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	cppClose(inf, oxt);

    return 0;
    }


/*** cppDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
cppDelete(pObject obj, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    objdrv *inf;
    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (objdrv *)cppOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Check to see if user is deleting the 'node object'. **/
	if (obj->Pathname->nElements == obj->SubPtr)
	    {
	    if (inf->Node->OpenCnt > 1) 
	        {
		cppClose(inf, oxt);
		mssError(1,"CPP","Cannot delete structure file: object in use");
		return -1;
		}

	    /** Need to do some checking to see if, for example, a non-empty object can't be deleted **/
	    if (!inf->IsEmpty())
	        {
		cppClose(inf, oxt);
		mssError(1,"CPP","Cannot delete: object not empty");
		return -1;
		}
	    stFreeInf(inf->Node->Data);

            inf->Delete(obj,oxt);
            /** Physically delete the node, and then remove it from the node cache **/
	    unlink(inf->Node->NodePath);
	    snDelete(inf->Node);
	    }
	else
	    {
	    /** Delete of sub-object processing goes here **/
	    }

	/** Release, don't call close because that might write data to a deleted object **/
	delete inf;

    return 0;
    }

int objdrv::Delete(pObject obj, pObjTrxTree* oxt){
    return 0;
}

/*** cppRead - Structure elements have no content.  Fails.
 ***/
int
cppRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    objdrv *inf = (objdrv *)inf_v;
    return inf->Read(buffer, maxcnt, offset, flags, oxt);
    }

int objdrv::Read(char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt){
    return 0;
}

/*** cppWrite - Again, no content.  This fails.
 ***/
int
cppWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    objdrv *inf = (objdrv *)inf_v;
    return inf->Write(buffer,cnt,offset,flags,oxt);
    }

int objdrv::Write(char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt){
    return 0;
}

/*** cppOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
cppOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    objdrv *inf = (objdrv *)inf_v;
    return (void*)inf->OpenQuery(query,oxt);
    }

//constructor for query_t
query_t::query_t(objdrv *data){
        Data=data;
        ItemCnt=0;
}

//default open query
query_t *objdrv::OpenQuery(pObjQuery query, pObjTrxTree* oxt){
    return new query_t(this);
}

/*** cppQueryFetch - get the next directory entry as an open object.
 ***/
void*
cppQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt){
    query_t *qy = (query_t *)qy_v;
    return (void*)qy->Fetch(obj,mode,oxt);
}

objdrv *query_t::Fetch(pObject obj, int mode, pObjTrxTree* oxt){
    return NULL;
}//end Fetch

/*** cppQueryClose - close the query.
 ***/
int
cppQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
        query_t *qy = (query_t *)qy_v;
        qy->Close(oxt);
        delete qy;
    return 0;
    }

int query_t::Close(pObjTrxTree* oxt){
    return 0;
}//end Close

/*** cppGetAttrType - get the type (DATA_T_cpp) of an attribute by name.
 ***/
int
cppGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    objdrv *inf = (objdrv *)inf_v;
    int i;
    pStructInf find_inf;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"inner_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

	/** Check for attributes in the node object if that was opened **/
	if (inf->Obj->Pathname->nElements == inf->Obj->SubPtr)
	    {
	    }

	/** Put checking for your own attributes here. **/
	/** You will want to likely make a list of 'em in a global array **/
	/** or something like that. **/

    return -1;
    }


/*** cppGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
cppGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    objdrv *inf = (objdrv *)inf_v;
    pStructInf find_inf;
    char* ptr;
    int i;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    val->String = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	/** REPLACE MYOBJECT/TYPE WITH AN APPROPRIATE TYPE. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    /*val->String = "application/octet-stream";*/
	    val->String = "system/void";
	    return 0;
	    }

	/** Object type. **/
	/** REPLACE WITH SOMETHING MORE COHERENT **/
	if (!strcmp(attrname,"outer_type"))
	    {
	    val->String = "system/object";
	    return 0;
	    }

	/** DO YOUR ATTRIBUTE LOOKUP STUFF HERE **/
	/** AND RETURN 0 IF GOT IT OR 1 IF NULL **/
	/** CONTINUE ON DOWN IF NOT FOUND. **/

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    val->String = "";
	    return 0;
	    }

	mssError(1,"CPP","Could not locate requested attribute");

    return -1;
    }


/*** cppGetNextAttr - get the next attribute name for this object.
 ***/
char*
cppGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    objdrv *inf = (objdrv *)inf_v;

	/** REPLACE THE IF(0) WITH A CONDITION IF THERE ARE MORE ATTRS **/
	if (0)
	    {
	    /** PUT YOUR ATTRIBUTE-NAME RETURN STUFF HERE. **/
	    inf->CurAttr++;
	    }

    return NULL;
    }


/*** cppGetFirstAttr - get the first attribute name for this object.
 ***/
char*
cppGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    objdrv *inf = (objdrv *)inf_v;
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = cppGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** cppSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
cppSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    objdrv *inf = (objdrv *)inf_v;

	/** Choose the attr name **/
	/** Changing name of node object? **/
	if (!strcmp(attrname,"name"))
	    {
	    if (inf->Obj->Pathname->nElements == inf->Obj->SubPtr)
	        {
	        if (!strcmp(inf->Obj->Pathname->Pathbuf,".")) return -1;
	        inf->Pathname=std::string(inf->Obj->Pathname->Pathbuf);
                std::string::iterator ch=inf->Pathname.begin();
                while(ch!=inf->Pathname.end()) if(*(ch++)='/')break;
                inf->Pathname.erase(ch,inf->Pathname.end());
                inf->Pathname.append(val->String);
                if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname.c_str()) < 0)
		    {
		    mssError(1,"CPP","SetAttr 'name': could not rename structure file node object");
		    return -1;
		    }
	        strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname.c_str());
		}
	    return 0;
	    }

	/** Set content type if that was requested. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    /** SET THE TYPE HERE, IF APPLICABLE, AND RETURN 0 ON SUCCESS **/
	    return -1;
	    }
	if (!strcmp(attrname,"outer_type"))
	    {
	    /** SET THE TYPE HERE, IF APPLICABLE, AND RETURN 0 ON SUCCESS **/
	    return -1;
	    }

	/** DO YOUR SEARCHING FOR ATTRIBUTES TO SET HERE **/

	/** Set dirty flag **/
	inf->Node->Status = SN_NS_DIRTY;

    return 0;
    }


/*** cppAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
cppAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt)
    {
    objdrv *inf = (objdrv *)inf_v;
    pStructInf new_inf;
    char* ptr;

    return -1;
    }


/*** cppOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
cppOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** cppGetFirstMethod -- return name of First method available on the object.
 ***/
char*
cppGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** cppGetNextMethod -- return successive names of methods after the First one.
 ***/
char*
cppGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** cppExecuteMethod - Execute a method, by name.
 ***/
int
cppExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** cppPresentationHints - Return a structure containing "presentation hints"
 *** data, which is basically metadata about a particular attribute, which
 *** can include information which relates to the visual representation of
 *** the data on the client.
 ***/
pObjPresentationHints
cppPresentationHints(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    objdrv *inf = (objdrv *)inf_v;
    return inf->PresentationHints(attrname,oxt);
    }

pObjPresentationHints objdrv::PresentationHints(char* attrname, pObjTrxTree* oxt){
    return NULL;
}


/*** cppInfo - return object metadata - about the object, not about a 
 *** particular attribute.
 ***/
int
cppInfo(void* inf_v, pObjectInfo info_struct)
    {
    objdrv *inf = (objdrv *)inf_v;
    return inf->Info(info_struct);
    }

int objdrv::Info(pObjectInfo info_struct){
    return 0;
}

/*** cppCommit - commit any changes made to the underlying data source.
 ***/
int
cppCommit(void* inf_v, pObjTrxTree* oxt)
    {
    objdrv *inf = (objdrv *)inf_v;
    return inf->Commit(oxt);
    }

int objdrv::Commit(pObjTrxTree* oxt){
    return 0;
}

bool objdrv::IsEmpty(){
    return true;
}

/*** cppInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
cppInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&CPP_INF,0,sizeof(CPP_INF));
	CPP_INF.dmy_global_variable = 0;

	/** Setup the structure **/
	strcpy(drv->Name,"CPP - fake module, will be cpp wrapper ");/** <--- PUT YOUR DESCRIPTION HERE **/
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),(void*)"cpp");/** <--- PUT YOUR OBJECT/TYPE HERE **/

	/** Setup the function references. **/
	drv->Open = (void* (*)())cppOpen;
	drv->Close = (int (*)())cppClose;
	drv->Create = (int (*)())cppCreate;
	drv->Delete = (int (*)())cppDelete;
	drv->OpenQuery = (void* (*)())cppOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = (void* (*)())cppQueryFetch;
	drv->QueryClose = (int (*)())cppQueryClose;
	drv->Read = (int (*)())cppRead;
	drv->Write = (int (*)())cppWrite;
	drv->GetAttrType = (int (*)())cppGetAttrType;
	drv->GetAttrValue = (int (*)())cppGetAttrValue;
	drv->GetFirstAttr = (char* (*)())cppGetFirstAttr;
	drv->GetNextAttr = (char* (*)())cppGetNextAttr;
	drv->SetAttrValue = (int (*)())cppSetAttrValue;
	drv->AddAttr = (int (*)())cppAddAttr;
	drv->OpenAttr = (void* (*)())cppOpenAttr;
	drv->GetFirstMethod = (char* (*)())cppGetFirstMethod;
	drv->GetNextMethod = (char* (*)())cppGetNextMethod;
	drv->ExecuteMethod = (int (*)())cppExecuteMethod;
	drv->PresentationHints = (_PH* (*)())cppPresentationHints;
	drv->Info = (int (*)())cppInfo;
	drv->Commit = (int (*)())cppCommit;

//	nmRegister(sizeof(objdrv),"CppData");
//	nmRegister(sizeof(query_t),"CppQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

MODULE_INIT(cppInitialize);
MODULE_PREFIX("cpp");
MODULE_IFACE(CX_CURRENT_IFACE);
