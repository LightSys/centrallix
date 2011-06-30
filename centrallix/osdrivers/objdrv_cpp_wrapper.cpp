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
#include "obj.h"
#include "cxlib/mtsession.h"
/** module definintions **/
#include "centrallix.h"

#include "objdrv.hpp"

/*** cppOpen - open an object.
 ***/
void*
cppOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    //fprintf(stderr,"opening a cpp object\n");
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
    //fprintf(stderr,"closeing a cpp object\n");
    objdrv *inf = (objdrv *)inf_v;
    inf->Close(oxt);
    //delete inf;

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
	inf = (objdrv *)cppOpen(obj, 0, NULL, (char *)"", oxt);
	if (!inf) return -1;

	inf->Delete(obj,oxt);
        
        /** Release, don't call close because that might write data to a deleted object **/
	delete inf;

    return 0;
    }

/*** cppRead - calls the objdrv's read method
 ***/
int
cppRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    //fprintf(stderr,"reading a cpp object\n");
    objdrv *inf = (objdrv *)inf_v;
    return inf->Read(buffer, maxcnt, offset, flags, oxt);
    }

/*** cppWrite - calls the objdrv's write method
 ***/
int
cppWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    //fprintf(stderr,"writing a cpp object\n");
    objdrv *inf = (objdrv *)inf_v;
    return inf->Write(buffer,cnt,offset,flags,oxt);
    }

/*** cppOpenQuery - open a directory query.
 ***/
void*
cppOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    //fprintf(stderr,"beigining query of a cpp object\n");
    objdrv *inf = (objdrv *)inf_v;
    return (void*)inf->OpenQuery(query,oxt);
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

/*** cppGetAttrType - get the type (DATA_T_cpp) of an attribute by name.
 ***/
int
cppGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    //fprintf(stderr,"typing att of a cpp object\n");
    objdrv *inf = (objdrv *)inf_v;
    if(!inf->GetAtrribute(std::string(attrname)))
        return -1;
    return inf->GetAtrribute(std::string(attrname))->Type;
    }

/*** cppGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
cppGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt){
    //fprintf(stderr,"valuing att of a cpp object\n");
    objdrv *inf = (objdrv *)inf_v;
    Attribute *data=inf->GetAtrribute(std::string(attrname));
    if(data==NULL)return -1;
    if(datatype != data->Type)return -1;
    pObjData tmpval =data->Value;
    //now the fun copy part
    /// @TODO include all types here
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
        default://blindly copy anything we don't understand
            memcpy(val,tmpval, sizeof(ObjData));
            std::cerr<<attrname<<" of type "<<datatype<<" not properly handled"<<std::endl;
    }
    return 0;
}

/*** cppGetNextAttr - get the next attribute name for this object.
 ***/
char*
cppGetNextAttr(void* inf_v, pObjTrxTree* oxt){
    //fprintf(stderr,"next att of a cpp object\n");
    objdrv *inf = (objdrv *)inf_v;
    std::string tmp;
    if(inf->CurrentAtrrib==inf->Attributes.end()){
        inf->CurrentAtrrib=inf->Attributes.begin();
        return NULL;
    }
    tmp=inf->CurrentAtrrib->first;
    inf->CurrentAtrrib++;
    //check against the black list
//    if(!tmp.compare("name") || !tmp.compare("annotation") || !tmp.compare("content_type"))
//        return cppGetNextAttr(inf_v,oxt);
    return (char *)(strdup(tmp.c_str()));
}

/*** cppGetFirstAttr - get the first attribute name for this object.
 ***/
char*
cppGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    //fprintf(stderr,"begining att list of a cpp object\n");
    objdrv *inf = (objdrv *)inf_v;
    inf->CurrentAtrrib=inf->Attributes.begin();
    return cppGetNextAttr(inf_v, oxt);
}


/*** cppSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
cppSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt){
    //fprintf(stderr,"setting att of a cpp object\n");
    objdrv *inf = (objdrv *)inf_v;
    if(!inf->GetAtrribute(std::string(attrname)))
        return -1;
    if(inf->SetAtrribute(std::string(attrname),new Attribute(datatype,val),oxt))
        return -1;
    return 0;
}

/*** cppAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
cppAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree* oxt)
    {
    //fprintf(stderr,"adding att to a cpp object\n");
    objdrv *inf = (objdrv *)inf_v;
    if(inf->SetAtrribute(std::string(attrname),new Attribute(type,val),oxt))
        return -1;
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
    //fprintf(stderr,"hinting about a cpp object\n");
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
cppInfo(void* inf_v, pObjectInfo info)
    {
    //fprintf(stderr,"infoing a cpp object\n");
    objdrv *inf = (objdrv *)inf_v;
    return inf->Info(info);
    }

/*** cppCommit - commit any changes made to the underlying data source.
 ***/
int
cppCommit(void* inf_v, pObjTrxTree* oxt)
    {
    //fprintf(stderr,"committing a cpp object\n");
    objdrv *inf = (objdrv *)inf_v;
    return inf->Commit(oxt);
    }

/*** cppInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
cppInitialize()
    {
    //fprintf(stderr,"starting a cpp system\n");
    pObjDriver drv;
    std::list<std::string> Types=GetTypes();
	// Allocate the driver
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	// Setup the structure
	strcpy(drv->Name,GetName());
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),1);
        for(std::list<std::string>::iterator item=Types.begin();
                item!=Types.end();item++)
                xaAddItem(&(drv->RootContentTypes),
                        (void*)strdup(item->c_str()));

	// Setup the function references.
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

	// Register the driver
	if (objRegisterDriver(drv) < 0){
            fprintf(stderr,"cpp system Failed!\n");
            return -1;
        }

    return 0;
    }

MODULE_INIT(cppInitialize);
MODULE_IFACE(CX_CURRENT_IFACE);
