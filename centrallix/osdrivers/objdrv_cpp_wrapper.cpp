#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <list>
#include <map>
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

/*** cppOpen - open an object.
 ***/
void*
cppOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    objdrv *inf;
    /** Allocate the structure! **/
    inf = GetInstance(obj, mask, systype, usrtype, oxt);
    if (!inf) return NULL;

    return (void*) inf;
    }


/*** cppClose - close an open object.
 ***/
int
cppClose(void* inf_v, pObjTrxTree* oxt)
    {
    objdrv *inf = (objdrv *)inf_v;
    inf->Close(oxt);
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
    objdrv *inf;
    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (objdrv *)cppOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	inf->Delete(obj,oxt);
        
        /** Release, don't call close because that might write data to a deleted object **/
	delete inf;

    return 0;
    }

int objdrv::Delete(pObject obj, pObjTrxTree* oxt){
    return 0;
}

/*** cppRead - calls the objdrv's read method
 ***/
int
cppRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    objdrv *inf = (objdrv *)inf_v;
    return inf->Read(buffer, maxcnt, offset, flags, oxt);
    }

int objdrv::Read(char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt){
    return -1;
}

/*** cppWrite - calls the objdrv's write method
 ***/
int
cppWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    objdrv *inf = (objdrv *)inf_v;
    return inf->Write(buffer,cnt,offset,flags,oxt);
    }

int objdrv::Write(char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt){
    return -1;
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
    if(inf->Attributes.find(std::string(attrname))==inf->Attributes.end())
        return -1;
    return inf->Attributes[std::string(attrname)]->Type;
    }

objdrv::objdrv(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt){
    //do some generic setup
    Pathname = std::string(obj_internal_PathPart(obj->Pathname, 0, 0));
    Attributes["name"]=new Attribute(DATA_T_STRING,obj_internal_PathPart(obj->Pathname, 0, 0));
    Attributes["annotation"]=new Attribute(DATA_T_STRING,"");
    Attributes["outer_type"]=new Attribute(DATA_T_STRING,"sytem/object");
    Attributes["inner_type"]=new Attribute(DATA_T_STRING,"sytem/void");
    Attributes["content_type"]=new Attribute(DATA_T_STRING,"sytem/void");
}

/*** cppGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
cppGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt){
    objdrv *inf = (objdrv *)inf_v;
    if(inf->Attributes.find(std::string(attrname))==inf->Attributes.end())
        return -1;
    if(datatype != inf->Attributes[std::string(attrname)]->Type)
        return -1;
    pObjData tmpval =inf->Attributes[std::string(attrname)]->Value;
    *val = *(tmpval);
//    switch(datatype){
//        case DATA_T_STRING:
//            val->String = inf->Attributes[std::string(attrname)]->Value->String;
//            break;
//        case DATA_T_INTEGER:
//            val->Integer = inf->Attributes[std::string(attrname)]->Value->Integer;
//            break;
//        case DATA_T_DOUBLE:
//            val->Double = inf->Attributes[std::string(attrname)]->Value->Double;
//            break;
//        default:
//            *val = *(inf->Attributes[std::string(attrname)]->Value);
//    }//end type switch
    return 0;
}

/*** cppGetNextAttr - get the next attribute name for this object.
 ***/
char*
cppGetNextAttr(void* inf_v, pObjTrxTree* oxt){
    objdrv *inf = (objdrv *)inf_v;
    std::string tmp;
    if(inf->CurrentAtrrib==inf->Attributes.end())return NULL;
    tmp=inf->CurrentAtrrib->first;
    inf->CurrentAtrrib++;
    if(tmp.compare("name") || tmp.compare("annotation") || tmp.compare("content_type")
            || tmp.compare("inner_type") || tmp.compare("outer_type"))
        return cppGetNextAttr(inf_v,oxt);
    return (char *)(tmp.c_str());
    return NULL;
}

/*** cppGetFirstAttr - get the first attribute name for this object.
 ***/
char*
cppGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    objdrv *inf = (objdrv *)inf_v;
    inf->CurrentAtrrib=inf->Attributes.begin();
    return cppGetNextAttr(inf_v, oxt);
}


/*** cppSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
cppSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt){
    Attribute *tmp;
    objdrv *inf = (objdrv *)inf_v;
    if(inf->Attributes.find(std::string(attrname))==inf->Attributes.end())
        return -1;
    tmp=inf->Attributes[std::string(attrname)];
    inf->Attributes[std::string(attrname)]=new Attribute(datatype,val);
    if(inf->UpdateAttr(std::string(attrname),oxt)){
        delete inf->Attributes[std::string(attrname)];
        inf->Attributes[std::string(attrname)]=tmp;
        return -1;
    }//end objected
    delete tmp;
    return 0;
}

/**
 * default update, handels name updates and objects to changing of types
 * @param attrname
 * @param oxt
 * @return 
 */
bool objdrv::UpdateAttr(std::string attrname, pObjTrxTree* oxt){
    /** Changing name of node object? **/
	if (!attrname.compare("name")){
	    if (Obj->Pathname->nElements == Obj->SubPtr)
	        {
	        if (!strcmp(Obj->Pathname->Pathbuf,".")) return -1;
	        Pathname=std::string(Obj->Pathname->Pathbuf);
                std::string::iterator ch=Pathname.begin();
                while(ch!=Pathname.end()) if(*(ch++)=='/')break;
                Pathname.erase(ch,Pathname.end());
                Pathname.append(Attributes[attrname]->Value->String);
                if (rename(Obj->Pathname->Pathbuf,Pathname.c_str()) < 0)
		    {
		    mssError(1,"CPP","SetAttr 'name': could not rename structure file node object");
		    return true;
		    }
	        strcpy(Obj->Pathname->Pathbuf, Pathname.c_str());
		}
	    return false;
	    }
	if (!attrname.compare("content_type") || !attrname.compare("inner_type"))return true;
	if (!attrname.compare("outer_type"))return true;

	/** Set dirty flag **/
	//Node->Status = SN_NS_DIRTY;
    return false;
}//end UpdateAttr

/*** cppAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
cppAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree* oxt)
    {
    objdrv *inf = (objdrv *)inf_v;
    inf->Attributes[std::string(attrname)]=new Attribute(type,val);
    if(inf->UpdateAttr(std::string(attrname),oxt)){
        delete inf->Attributes[std::string(attrname)];
        inf->Attributes.erase(std::string(attrname));
        return -1;
    }//end if objected
    return 0;
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

	/** Setup the structure **/
	strcpy(drv->Name,GetName());
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),(void*)GetType());

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

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

MODULE_INIT(cppInitialize);
MODULE_IFACE(CX_CURRENT_IFACE);
