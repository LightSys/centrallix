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
#include "obj.h"
#include "st_node.h"
#include "query.hpp"

class objdrv;

/*** Structure used by queries for this driver. ***/
class query_t{
    objdrv       *Data;
    std::string  NameBuff;
    int          ItemCnt;
    query_t(objdrv *data);
};//end class query

class objdrv {
public:
    std::string Pathname;
    int		Flags;
    pObject	Obj;
    int		Mask;
    int		CurAttr;
    pSnNode	Node;
        
    //from file handeling 
    virtual void* Open(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt);
    virtual int Close(pObjTrxTree* oxt);
    virtual int Create(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt);
    virtual int Delete(pObject obj, pObjTrxTree* oxt);
    virtual int Read(char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt);
    virtual int Write(char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt);
    virtual query_t* OpenQuery(pObjQuery query, pObjTrxTree* oxt);
    virtual void* QueryFetch(pObject obj, int mode, pObjTrxTree* oxt);
    virtual int QueryClose(void* qy_v, pObjTrxTree* oxt);
    virtual int GetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt);
    virtual int GetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt);
    virtual char* GetNextAttr(void* inf_v, pObjTrxTree oxt);
    virtual char* GetFirstAttr(void* inf_v, pObjTrxTree oxt);
    virtual int SetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt);
    virtual int AddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt);
    virtual void* OpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt);
    virtual pObjPresentationHints PresentationHints(char* attrname, pObjTrxTree* oxt);
    virtual int Info(pObjectInfo info_struct);
    virtual int Commit(pObjTrxTree* oxt);
    
    //others
    virtual bool IsEmpty();
};//end class objdrv

//get an instance of the class
objdrv *GetInstance();

#endif	/* OBJDRV_HPP */

