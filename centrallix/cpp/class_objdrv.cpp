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
/* Module: 	class_objdrv.cpp                      			*/
/* Author:	Micah Shennum   					*/
/* Creation:	Jun 30 2011     					*/
/* Description: contains the actual function code for the objdrv class  */
/*               and query_t class                                      */
/*                therefore, this file contains almost exclusively stubs*/
/************************************************************************/

#include "objdrv.hpp"

//let's start with the depressing sounding ones

int objdrv::Close(pObjTrxTree* oxt){
    return 0;
}

int objdrv::Delete(pObject obj, pObjTrxTree* oxt){
    return 0;
}

bool objdrv::IsEmpty(){
    return true;
}

//and now for input, output

int objdrv::Read(char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt){
    return -1;
}

int objdrv::Write(char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt){
    return -1;
}

int objdrv::Info(pObjectInfo info){
    return 0;
}

int objdrv::Commit(pObjTrxTree* oxt){
    return 0;
}

pObjPresentationHints objdrv::PresentationHints(char* attrname, pObjTrxTree* oxt){
    return NULL;
}

/*
 * default update, allows name change but objects to changing of types
 * @param attrname
 * @param oxt
 * @return
 */
bool objdrv::UpdateAttr(std::string attrname, pObjTrxTree* oxt){
    /** Changing name of node object? **/
	if (!attrname.compare("name"))return false;
	if (!attrname.compare("content_type") || !attrname.compare("inner_type"))return true;
	if (!attrname.compare("outer_type"))return true;
        return false;
}//end UpdateAttr

Attribute *objdrv::GetAtrribute(std::string name){
    if(this->Attributes.find(name)==this->Attributes.end())
        return NULL;
    return this->Attributes[name];
}//end GetAtrribute

bool objdrv::SetAtrribute(std::string name, Attribute *value, pObjTrxTree* oxt){
    Attribute *tmp=this->Attributes[name];
    if(this->Attributes.find(name)==this->Attributes.end())tmp=NULL;
    this->Attributes[name]=value;
    if(this->UpdateAttr(name,oxt)){
        delete this->Attributes[name];
        if(tmp)this->Attributes[name]=tmp;
        return true;
    }//end objected
    delete tmp;
    return false;
}//end SetAtrribute

//the constructor!
objdrv::objdrv(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt){
    //do some generic setup
    Pathname = std::string(obj_internal_PathPart(obj->Pathname, 0, 0));
}


//next comes the query stuff

//constructor for query_t
query_t::query_t(objdrv *data){
        Data=data;
        ItemCnt=0;
}

objdrv *query_t::Fetch(pObject obj, int mode, pObjTrxTree* oxt){
    return NULL;
}//end Fetch

int query_t::Close(pObjTrxTree* oxt){
    return 0;
}//end Close
