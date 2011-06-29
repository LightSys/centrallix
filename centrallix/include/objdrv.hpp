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
/*      a driver useing c++ (and a few other helping classes)           */
/************************************************************************/
#ifndef OBJDRV_HPP
#define	OBJDRV_HPP
#ifndef __cplusplus
#error "This is a C++ class, and it looks like your using it in C"
#endif
#include <string>
#include <map>
#include <list>
#include <string.h>
#include "centrallix.h"
#include "obj.h"
#include "st_node.h"

class objdrv;

/**
 * used by queries to the driver.
 * @brief query to the driver
 */
class query_t{
public:
    objdrv       *Data;
    std::string  NameBuff;
    int          ItemCnt;
    /**
     * creates a new query object
     * @param data the object to query
     */
    query_t(objdrv *data);
    /**
     * Grabs the next object in the query results
     * @param obj
     * @param mode
     * @param oxt  transaction context
     * @return
     */
    virtual objdrv* Fetch(pObject obj, int mode, pObjTrxTree* oxt);
    /**
     * Closes the query (allowing it to free any resources)
     * @param oxt transaction context
     * @return 0 on succeeds
     */
    virtual int Close(pObjTrxTree* oxt);
};//end class query

/**
 * An attribute, makes tracking both value
 * and connected type simple
 * @brief attribute (of the particular object)
 */
class Attribute{
public:
    /**Type of attribute*/
    int Type;
    /**The actual value*/
    pObjData Value;

    //TODO include constructor for all types here

    /**
     * builds based on arbatrary value
     * @param type
     * @param value
     */
    Attribute(int type,pObjData value);
    
    /**
     * builds based on a c++ string
     * @param type
     * @param value
     */
    Attribute(std::string value);

    /**
     * builds based on a int
     * @param type
     * @param value
     */
    Attribute(int value);

    /**
     * Destructor
     */
    ~Attribute();
};

/**
 * Writes an attribute to the stream, using the correct type
 * @brief ostream operator for Attribute
 * @param out stream to output on
 * @param att Attribute * to display
 * @return out
 */
std::ostream &operator <<(std::ostream &out,Attribute *att);

/**
 * a base class, which should be extended as your driver
 * @brief base class for object drivers
 */
class objdrv {
public:
    /**System name for this object */
    std::string Pathname;
    /**System object for this object*/
    pObject	Obj;
    /**Map of the attributes of this object, maintained by the wrapper*/
    std::map<std::string,Attribute *> Attributes;
    /**Points to the current attribute in the attribute listing, don't touch*/
    std::map<std::string,Attribute *>::iterator CurrentAtrrib;

    /**
     * Creates a new object
     * @param obj     system object being opened
     * @param mask    permission mask
     * @param systype
     * @param usrtype
     * @param oxt     transaction context
     */
    objdrv(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt);
    //from file handeling
    /**
     * Close the object
     * @param oxt transaction context
     * @return 0 on succeeding
     */
    virtual int Close(pObjTrxTree* oxt);
    /**
     * Deletes this object
     * @param obj the object being deleted
     * @param oxt transaction context
     * @return 0 on succeeding
     */
    virtual int Delete(pObject obj, pObjTrxTree* oxt);
    /**
     * Reads data from this object
     * @param buffer buffer to read data into
     * @param maxcnt maximum data to read
     * @param offset
     * @param flags
     * @param oxt    transaction context
     * @return       amount read, or -1 if reading is not posable
     */
    virtual int Read(char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt);
    /**
     *
     * @param buffer  buffer to write data from
     * @param cnt     amount of data to write
     * @param offset
     * @param flags
     * @param oxt     transaction context
     * @return        amount read, or -1 if writing is not posable
     */
    virtual int Write(char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt);
    /**
     * gets the presentation hints for an attribute
     * @param attrname attribute we are interested in
     * @param oxt      transaction context
     * @return         hints (or NULL is OK too)
     */
    virtual pObjPresentationHints PresentationHints(char* attrname, pObjTrxTree* oxt);
    /**
     * gets the metadata for this object
     * @param info  points to storage for the information
     * @return      0 on succeeding
     */
    virtual int Info(pObjectInfo info);
    /**
     * Called to commit changes made to an object
     * @param oxt  transaction context
     * @return     0 on succeeding
     */
    virtual int Commit(pObjTrxTree* oxt);
    /**
     * opens a new query into this object
     * @param query   query paramiters
     * @param oxt     transaction context
     * @return        a new query object
     */
    virtual query_t* OpenQuery(pObjQuery query, pObjTrxTree* oxt);
    /**
     * called whenever an attribute is changed on the object
     * @param attrname attribute which has been updated
     * @param oxt      transaction context
     * @return         true if the change should be undone
     */
    virtual bool UpdateAttr(std::string attrname, pObjTrxTree* oxt);
    //others
    virtual bool IsEmpty();
};//end class objdrv

/**
 * get an instance of the class
 * @param obj     system object being opened
 * @param mask    permission mask
 * @param systype
 * @param usrtype
 * @param oxt     transaction context
 * @return pointer to the new object
 */
objdrv *GetInstance(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt);
/**
 * gets the name of the driver
 * @return name as a c string
 */
char *GetName();
/**
 * gets the list of mime types this driver can handle
 * @return list of mime types
 */
std::list<std::string> GetTypes();

#endif	/* OBJDRV_HPP */

