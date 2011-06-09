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
/* Module: 	objdrv.hpp                      			*/
/* Author:	Micah Shennum   					*/
/* Creation:	Jun 07 2011     					*/
/* Description: Defines the objdrv class, which is implemented to make  */
/*      a driver useing c++                                             */
/************************************************************************/
#ifndef OBJDRV_HPP
#define	OBJDRV_HPP
#ifndef __cplusplus
#error "This is a C++ class, and it looks like your using it in C"
#endif
#include <string>
#include "centrallix.h"
#include "obj.h"
#include "st_node.h"

class objdrv;

/*** Structure used by queries for this driver. ***/
class query_t{
public:
    objdrv       *Data;
    std::string  NameBuff;
    int          ItemCnt;
    query_t(objdrv *data);
    virtual objdrv* Fetch(pObject obj, int mode, pObjTrxTree* oxt);
    virtual int Close(pObjTrxTree* oxt);
};//end class query

//atribute of an object
class Attribute{
public:
    int Type;
    pObjData Value;
    
    Attribute(int type,pObjData value){
        Type=type;
        Value= value;
    }

    Attribute(int type,std::string value){
        Type=type;
        Value= new ObjData;
        Value->String=(char *)value.c_str();
    }

    Attribute(int type,int value){
        Type=type;
        Value= new ObjData;
        Value->Integer=value;
    }
};

class objdrv {
public:
    std::string Pathname;
    pObject	Obj;
    std::map<std::string,Attribute *> Attributes;
    std::map<std::string,Attribute *>::iterator CurrentAtrrib;
    
    objdrv(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt);
    //from file handeling 
    virtual int Close(pObjTrxTree* oxt);
    virtual int Delete(pObject obj, pObjTrxTree* oxt);
    virtual int Read(char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt);
    virtual int Write(char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt);
    virtual pObjPresentationHints PresentationHints(char* attrname, pObjTrxTree* oxt);
    virtual int Info(pObjectInfo info_struct);
    virtual int Commit(pObjTrxTree* oxt);
    virtual query_t* OpenQuery(pObjQuery query, pObjTrxTree* oxt);
    virtual bool UpdateAttr(std::string attrname, pObjTrxTree* oxt);
    //others
    virtual bool IsEmpty();
};//end class objdrv

//get an instance of the class
objdrv *GetInstance(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt);
char *GetName();
std::list<std::string> GetTypes();

/*
 * You need define:
MODULE_DESC("ObjectSystem Driver Description");
MODULE_VERSION(0,0,0);
*/
#endif	/* OBJDRV_HPP */

