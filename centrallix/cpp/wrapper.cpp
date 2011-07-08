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
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "obj.h"
#include "cxlib/mtsession.h"
/** module definintions **/
#include "centrallix.h"

#include "objdrv.hpp"

// *iron* hard execption catcher: whump! and it's caught
#define CATCH_ERR(e)  catch(...){\
                        std::cerr << "Uncaught exception in "<<__FUNCTION__<<std::endl;\
                        return e;\
                        }

//these are here since I did not want to place them in objdrv.hpp
//but they are useful to the wrapper
extern char *moduleName;
extern char *modulePrefix;
extern char *moduleDescription;
extern int   moduleCapabilities;
/*** cppOpen - open an object.
 ***/
void*
cppOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
     try{
        objdrv *inf;
        /** Allocate the structure! **/
        inf = GetInstance(obj, mask, systype, usrtype, oxt);
        return (void*) inf;
        }CATCH_ERR(NULL);
    }


/*** cppClose - close an open object.
 ***/
int
cppClose(void* inf_v, pObjTrxTree* oxt)
    {
     try{
        int tmp;
        objdrv *inf = (objdrv *)inf_v;
        tmp=inf->Close(oxt);
        FreeInstance(inf,oxt);
        return tmp;
     }CATCH_ERR(-1);
    }

/*** cppCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
cppCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    try{
        void* inf;
        /** Call open() then close() **/
        obj->Mode = O_CREAT | O_EXCL;
        inf = cppOpen(obj, mask, systype, usrtype, oxt);
        if (!inf) return -1;
        cppClose(inf, oxt);
        return 0;
        }CATCH_ERR(-1);
    }


/*** cppDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
cppDeleteObj(void* inf_v, pObjTrxTree* oxt)
    {
    try{
        objdrv *inf=(objdrv *)inf_v;
	inf->Delete(oxt);
        /** Release, don't call close because that might write data to a deleted object **/
	FreeInstance(inf,oxt);
        return 0;
        }CATCH_ERR(-1);
    }

/*** cppRead - calls the objdrv's read method
 ***/
int
cppRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    try{
        objdrv *inf = (objdrv *)inf_v;
        return inf->Read(buffer, maxcnt, offset, flags, oxt);
        }CATCH_ERR(-1);
    }

/*** cppWrite - calls the objdrv's write method
 ***/
int
cppWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    try{
        objdrv *inf = (objdrv *)inf_v;
        return inf->Write(buffer,cnt,offset,flags,oxt);
        }CATCH_ERR(-1);
    }

/*** cppOpenQuery - open a directory query.
 ***/
void*
cppOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    try{
        objdrv *inf = (objdrv *)inf_v;
        return (void*)inf->OpenQuery(query,oxt);
        }CATCH_ERR(NULL);
    }

/*** cppQueryFetch - get the next directory entry as an open object.
 ***/
void*
cppQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt){
    try{
        query_t *qy = (query_t *)qy_v;
        return (void*)qy->Fetch(obj,mode,oxt);
        }CATCH_ERR(NULL);
}

/*** cppQueryClose - close the query.
 ***/
int
cppQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    try{
        query_t *qy = (query_t *)qy_v;
        qy->Close(oxt);
        delete qy;
        return 0;
    }CATCH_ERR(-1);
    }

/*** cppGetAttrType - get the type (DATA_T_cpp) of an attribute by name.
 ***/
int
cppGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    try{
        objdrv *inf = (objdrv *)inf_v;
        if(!inf->GetAtrribute(std::string(attrname)))
            return -1;
        return inf->GetAtrribute(std::string(attrname))->Type;
        }CATCH_ERR(-1);
    }

/** cppGetAttrValue - get the value of an attribute by name.  The 'val'
 * pointer must point to an appropriate data type.
 */
int
cppGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt){
    try{
        objdrv *inf = (objdrv *)inf_v;
        Attribute *data=inf->GetAtrribute(std::string(attrname));
        if(data==NULL)return -1;
        if(datatype != data->Type)return -1;
        pObjData tmpval =data->Value;
        //now the fun copy part
        ///@todo DATA_T_ARRAY, DATA_T_CODE
        switch(datatype){
            case DATA_T_STRING:
                val->String = tmpval->String;
                break;
            case DATA_T_INTEGER:
                val->Integer = tmpval->Integer;
                break;
            case DATA_T_DOUBLE:
                val->Double = tmpval->Double;
                break;
            case DATA_T_DATETIME:
                val->DateTime = tmpval->DateTime;
                break;
            case DATA_T_MONEY:
                val->Money = tmpval->Money;
                break;
            case DATA_T_INTVEC:
                val->IntVec = tmpval->IntVec;
                break;
            case DATA_T_STRINGVEC:
                val->StringVec = tmpval->StringVec;
                break;
            case DATA_T_BINARY:
                val->Binary.Size = tmpval->Binary.Size;
                val->Binary.Data = tmpval->Binary.Data;
                break;
            default://blindly copy anything we don't understand
                memcpy(val,tmpval, sizeof(ObjData));
                std::cerr<<attrname<<" of type "<<datatype<<" not properly handled"<<std::endl;
        }
        return 0;
        }CATCH_ERR(-1);
}

/*** cppGetNextAttr - get the next attribute name for this object.
 ***/
char*
cppGetNextAttr(void* inf_v, pObjTrxTree* oxt){
    try{
    objdrv *inf = (objdrv *)inf_v;
    std::string tmp;
    if(inf->CurrentAtrrib==inf->Attributes.end()){
        inf->CurrentAtrrib=inf->Attributes.begin();
        return NULL;
    }
    tmp=inf->CurrentAtrrib->first;
    inf->CurrentAtrrib++;
    //check against the black list
    if(!tmp.compare("name") || !tmp.compare("annotation") || !tmp.compare("content_type")
            || !tmp.compare("inner_type") || !tmp.compare("outer_type") || !tmp.compare("last_modification"))
        return cppGetNextAttr(inf_v,oxt);
    return inf->CentrallixString(tmp);
    }CATCH_ERR(NULL);
}

/*** cppGetFirstAttr - get the first attribute name for this object.
 ***/
char*
cppGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    //NOT putting try / catch here, as cppGetNextAttr should be on top of things
    objdrv *inf = (objdrv *)inf_v;
    inf->CurrentAtrrib=inf->Attributes.begin();
    return cppGetNextAttr(inf_v, oxt);
}


/*** cppSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
cppSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt){
    try{
        objdrv *inf = (objdrv *)inf_v;
        if(!inf->GetAtrribute(std::string(attrname)))
            return -1;
        if(inf->SetAtrribute(std::string(attrname),new Attribute(datatype,val),oxt))
            return -1;
        return 0;
        }CATCH_ERR(-1);
}

/*** cppAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
cppAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree* oxt)
    {
    try{
        objdrv *inf = (objdrv *)inf_v;
        if(inf->SetAtrribute(std::string(attrname),new Attribute(type,val),oxt))
            return -1;
        return 0;
        }CATCH_ERR(-1);
    }

/*** cppGetNextMethod -- return successive names of methods after the First one.
 ***/
char *cppGetNextMethod(void* inf_v, pObjTrxTree oxt){
    try{
        objdrv *inf = (objdrv *)inf_v;
        if(inf->CurrentMethod == inf->Methods->end())
            return NULL;
        return inf->CentrallixString(*(inf->CurrentMethod++));
        }CATCH_ERR(NULL);
}

/*** cppGetFirstMethod -- return name of First method available on the object.
 ***/
char *cppGetFirstMethod(void* inf_v, pObjTrxTree oxt){
    try{
        objdrv *inf = (objdrv *)inf_v;
        //get and store list so that we get a consistent view
        if(inf->Methods)delete inf->Methods;
        inf->Methods = inf->GetMethods();
        inf->CurrentMethod = inf->Methods->begin();
        return cppGetNextMethod(inf_v, oxt);
    }CATCH_ERR(NULL);
}

/*** cppExecuteMethod - Execute a method, by name.
 ***/
int cppExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt){
    try{
        objdrv *inf = (objdrv *)inf_v;
        return inf->RunMethod(std::string(methodname),param,oxt);
        }CATCH_ERR(-1);
}


/*** cppPresentationHints - Return a structure containing "presentation hints"
 *** data, which is basically metadata about a particular attribute, which
 *** can include information which relates to the visual representation of
 *** the data on the client.
 ***/
pObjPresentationHints
cppPresentationHints(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    try{
        objdrv *inf = (objdrv *)inf_v;
        return inf->PresentationHints(std::string(attrname),oxt);
        }CATCH_ERR(NULL);
    }

/*** cppInfo - return object metadata - about the object, not about a 
 *** particular attribute.
 ***/
int
cppInfo(void* inf_v, pObjectInfo info)
    {
    try{
        objdrv *inf = (objdrv *)inf_v;
        return inf->Info(info);
        }CATCH_ERR(-1);
    }

/*** cppCommit - commit any changes made to the underlying data source.
 ***/
int
cppCommit(void* inf_v, pObjTrxTree* oxt)
    {
    try{
        objdrv *inf = (objdrv *)inf_v;
        return inf->Commit(oxt);
        }CATCH_ERR(-1);
    }

/*** cppInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
cppInitialize()
    {
    try{
    pObjDriver drv;
    std::list<std::string> Types=GetTypes();
	// Allocate the driver
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	// Setup the structure
	strncpy(drv->Name,moduleName,64);
	drv->Capabilities = moduleCapabilities;
	xaInit(&(drv->RootContentTypes),1);
        for(std::list<std::string>::iterator item=Types.begin();
                item!=Types.end();item++)
                xaAddItem(&(drv->RootContentTypes),
                        (void*)strdup(item->c_str()));

	// Setup the function references.
	drv->Open = (void* (*)())cppOpen;
	drv->Close = (int (*)())cppClose;
	drv->Create = (int (*)())cppCreate;
	drv->Delete = NULL;
        drv->DeleteObj = (int (*)())cppDeleteObj;
	drv->OpenQuery = (void* (*)())cppOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = (void* (*)())cppQueryFetch;
	drv->QueryClose = (int (*)())cppQueryClose;
        drv->GetQueryCoverageMask = NULL;
	drv->Read = (int (*)())cppRead;
	drv->Write = (int (*)())cppWrite;
	drv->GetAttrType = (int (*)())cppGetAttrType;
	drv->GetAttrValue = (int (*)())cppGetAttrValue;
	drv->GetFirstAttr = (char* (*)())cppGetFirstAttr;
	drv->GetNextAttr = (char* (*)())cppGetNextAttr;
	drv->SetAttrValue = (int (*)())cppSetAttrValue;
	drv->AddAttr = (int (*)())cppAddAttr;
	drv->OpenAttr = NULL;
	drv->GetFirstMethod = (char* (*)())cppGetFirstMethod;
	drv->GetNextMethod = (char* (*)())cppGetNextMethod;
	drv->ExecuteMethod = (int (*)())cppExecuteMethod;
	drv->PresentationHints = (_PH* (*)())cppPresentationHints;
	drv->Info = (int (*)())cppInfo;
	drv->Commit = (int (*)())cppCommit;

	// Register the driver
	if (objRegisterDriver(drv) < 0){
            std::cerr<<"registering cpp system for "
                    <<modulePrefix<<" Failed!"<<std::endl;
            return -1;
        }

    return 0;
    }CATCH_ERR(-1);
    }//end cppInitialize

MODULE_INIT(cppInitialize);
MODULE_IFACE(CX_CURRENT_IFACE);
