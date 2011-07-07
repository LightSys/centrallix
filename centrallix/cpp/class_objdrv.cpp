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
//define cleanup code such that it can be used anywhere!
#define FREE_ALL_ATRRIBS() for(CurrentAtrrib=Attributes.begin();\
                                CurrentAtrrib != Attributes.end();\
                                CurrentAtrrib++)\
                                delete CurrentAtrrib->second
#define FREE_ALL_STRINGS() for(std::map<std::string,char *>::iterator str=Strings.begin();\
                                str != Strings.end();\
                                str++)\
                                nmSysFree(str->second)
#define FREE_ALL_HINTS()   for(std::list<pObjPresentationHints>::iterator hint=Hints.begin();\
                                hint != Hints.end();\
                                hint++)\
                            nmFree(*hint,sizeof(ObjPresentationHints))
//let's start with the depressing sounding ones

//clean up time!
objdrv::~objdrv(){
    FREE_ALL_ATRRIBS();
    FREE_ALL_STRINGS();
    FREE_ALL_HINTS();
}

//drop all the attributes
int objdrv::Close(pObjTrxTree* oxt){
//    FREE_ALL_ATRRIBS();
//    FREE_ALL_STRINGS();
//    FREE_ALL_HINTS();
    return 0;
}

//drop all the attributes
int objdrv::Delete(pObject obj, pObjTrxTree* oxt){
//    FREE_ALL_ATRRIBS();
//    FREE_ALL_STRINGS();
//    FREE_ALL_HINTS();
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

char *objdrv::CentrallixString(std::string text){
    //if we've done this before, don't bother to do it again
    if(Strings.find(text) != Strings.end())
        return Strings[text];
    //other wise, go ahead and build a new one
    char *string=(char *)nmSysMalloc(text.length()+1);
    bzero(string,text.length()+1);
    memcpy(string,text.c_str(),text.length());
    Strings[text]=string;
    return string;
}

//stolen from obj_attr.c:734-741, thanks gbeeley & mmcgill
pObjPresentationHints objdrv::NewHints(){
   pObjPresentationHints ph = (pObjPresentationHints)
           nmMalloc(sizeof(ObjPresentationHints));
   memset(ph,0,sizeof(ObjPresentationHints));
   xaInit(&(ph->EnumList),16);
   /** init the non-0 default values **/
   ph->GroupID=-1;
   ph->VisualLength2=1;
   Hints.push_back(ph);
   return ph;
}

pObjPresentationHints objdrv::PresentationHints(std::string attrname, pObjTrxTree* oxt){
    return NULL;
}

std::list<std::string> *objdrv::GetMethods(){
    return new std::list<std::string>();
}

int objdrv::RunMethod(std::string methodname, pObjData param, pObjTrxTree oxt){
    return -1;
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
    //check existance, and save if available
    if(this->Attributes.find(name)==this->Attributes.end())tmp=NULL;
    this->Attributes[name]=value;
    //check that what we have done is OK
    if(this->UpdateAttr(name,oxt)){
        delete this->Attributes[name];
        if(tmp)this->Attributes[name]=tmp;
        return true;
    }//end objected
    //drop the backup 
    delete tmp;
    return false;
}//end SetAtrribute

//the constructor!
objdrv::objdrv(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt){
    //do some generic setup
    Pathname = std::string(obj_internal_PathPart(obj->Pathname, 0, 0));
    Methods = NULL;
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
