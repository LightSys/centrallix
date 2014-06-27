/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	libmime							*/
/* Author:	Luke Ehresman (LME)					*/
/* Creation:	August 12, 2002						*/
/* Description:	Provides MIME parsing facilities used mainly in the	*/
/*		MIME object system driver (objdrv_mime.c)		*/
/************************************************************************/

#include <string.h>
#include <errno.h>
#include "cxlib/mtsession.h"
#include "obj.h"
#include "mime.h"

/*** libmime_CreateIntAttr - Creates and stores an integer attribute in the
 *** given Mime header.
 ***/
int
libmime_CreateIntAttr(pMimeHeader this, char* name, int data)
    {
    pMimeAttr attr;

	/** Allocate the Mime attribute. **/
	attr = (pMimeAttr)nmMalloc(sizeof(MimeAttr));
	if (!attr)
	    {
	    return -1;
	    }
	memset(attr, 0, sizeof(MimeAttr));

	/** Populate the Mime attribute. **/
	attr->Name = name;
	attr->Ptod = ptodCreateInt(data);

	/** Add the Mime attribute to the attributes array. **/
	xhAdd(&this->Attrs, name, (char*)attr);

    return 0;
    }

/*** libmime_CreateStringAttr - Creates and stores a string attribute in the
 *** given Mime header.
 ***/
int
libmime_CreateStringAttr(pMimeHeader this, char* attr, char* param, char* data, int flags)
    {
    pTObjData ptod = NULL;

	/** Create the parameter/attribute and get the ptod. **/
	ptod = libmime_CreateAttrParam(this, attr, param);
	if (!ptod)
	    {
	    return -1;
	    }

	/** Populate the ptod. **/
	ptod = ptodCreateString(data, flags);

    return 0;
    }

/*** libmime_CreateStringArrayAttr - Creates and stores a string array attribute
 *** in the given Mime header.
 ***/
int
libmime_CreateStringArrayAttr(pMimeHeader this, char* attr, char* param)
    {
    pStringVec attrVec = NULL;

	/** Allocate the attribute vector. **/
	attrVec = (pStringVec)nmMalloc(sizeof(StringVec));
	if (!attrVec)
	    {
	    return -1;
	    }
	memset(attrVec, 0, sizeof(StringVec));

	/** Create the attribute with the allocated StringVec. **/
	libmime_CreateAttr(this, attr, param, attrVec, DATA_T_STRINGVEC);

    return 0;
    }

/*** libmime_CreateAttr - Creates and stores a generic attribute in the
 *** given Mime header.
 ***/
int
libmime_CreateAttr(pMimeHeader this, char* attr, char* param, void* data, int datatype)
    {
    pTObjData ptod = NULL;

	/** Create the attribute/parameter and get the ptod. **/
	ptod = libmime_CreateAttrParam(this, attr, param);
	if (!ptod)
	    {
	    return -1;
	    }

	/** Populate the ptod. **/
	ptod = ptodCreate(data, datatype);

    return 0;
    }

/*** libmime_CreateArrayAttr - Creates a generic array attribute in the
 *** given Mime header.
 ***/
int
libmime_CreateArrayAttr(pMimeHeader this, char* attr, char* param)
    {
    pXArray array = NULL;

	/** Allocate the generic array. **/
	array = (pXArray)nmMalloc(sizeof(XArray));
	if (!array)
	    {
	    return -1;
	    }
	memset(array, 0, sizeof(XArray));
	xaInit(array, 4);

	/** Create an attribute containing the generic array. **/
	libmime_CreateAttr(this, attr, param, array, DATA_T_ARRAY);

    return 0;
    }

/*** libmime_CreateAttrParam - Creates an attribute or parameter according to the
 *** given names and returns the ptod from the appropriate structure.
 ***/
pTObjData
libmime_CreateAttrParam(pMimeHeader this, char* attrName, char* paramName)
    {
    pMimeAttr attr = NULL;
    pMimeParam param = NULL;

	/** If the parameter argument is empty, we are creating an attribute. **/
	if(!paramName || !strlen(paramName))
	    {
	    /** Allocate the new attribute. **/
	    attr = (pMimeAttr)nmMalloc(sizeof(MimeAttr));
	    if (!attr)
		{
		mssError(1, "MIME", "Could not allocate a new attribute");
		return NULL;
		}
	    memset(attr, 0, sizeof(MimeAttr));

	    /** Set the name of the attribute. **/
	    attr->Name = attrName;

	    /** Add the Mime attribute to the attributes array. **/
	    xhAdd(&this->Attrs, attrName, (char*)attr);

	    /** Return the pointer to the relevant ptod. **/
	    return attr->Ptod;
	    }
	/** Otherwise we are creating a parameter. **/
	else
	    {
	    /** Find the appropriate attribute. **/
	    attr = (pMimeAttr)xhLookup(&this->Attrs, attrName);
	    if (!attr)
		{
		mssError(1, "MIME", "Could not find the given attribute (%s)", attrName);
		return NULL;
		}

	    /** Allocate the new parameter. **/
	    param = (pMimeParam)nmMalloc(sizeof(MimeParam));
	    if (!param)
		{
		mssError(1, "MIME", "Could not allocate a new parameter");
		return NULL;
		}
	    memeset(param, 0, sizeof(MimeParam));

	    /** Set the name of the parameter. **/
	    param->Name = paramName;

	    /** Add the Mime parameter to the parameter hash. **/
	    xhAdd(&attr->Params, paramName, (char*)param);

	    /** Return the pointer to the relevant ptod.**/
	    return param->Ptod;
	    }

    return NULL;
    }

/*** libmime_GetPtodFromHeader - Gets a PTOD from a header based on the passed
 *** in attribute and param. If param is NULL (or ""), we assume we want the
 *** value of the attr, otherwise search for the param in the XHashTable.
 ***
 *** returns pointer on success, NULL on failure.
 ***/
pTObjData
libmime_GetPtodFromHeader(pMimeHeader this, char* attr, char* param)
    {
    return *libmime_GetPtodPointer(this, attr, param);
    }

/*** libmime_GetPtodPointer - Gets the pointer to the PTOD from an attribute/parameter
 *** based on the passed in attribute and param. If param is NULL (or ""), we assume
 *** we want the value of the attr, otherwise search for the param in the XHashTable.
 ***
 *** returns pointer on success, NULL on failure
 ***/
*pTObjData
libmime_GetPtodPointer(pMimeHeader this, char* attr, char* param)
    {
    void *ptr = NULL;

	/** Get the attribute value **/
	ptr = (pMimeAttr)xhLookup(&this->Attrs, attr);
	if (!ptr)
	    {
	    mssError(1, "MIME", "Could not find attribute in header.");
	    return NULL;
	    }

	/** If param, search the XHashTable, otherwise return default. **/
	if (param && !strcmp(param, ""))
	    {
	    /** Get the param value **/
	    ptr = (pMimeParam)xhLookup(&((pMimeAttr)ptr)->Params, param);
	    if (!ptr)
		{
		mssError(1, "MIME", "Could not find parameter in attribute.");
		return NULL;
		}

	    return ((pMimeParam)ptr)->Ptod;
	    }

    /** No param, so give the default **/
    return &((pMimeAttr)ptr)->Ptod;
    }

/*** libmime_GetIntAttr - Gets an integer attribute from the
 *** given Mime header.
 ***/
int
libmime_GetIntAttr(pMimeHeader this, char* attr, char* param)
    {
    pTObjData ptod = NULL;

	ptod = libmime_GetPtodFromHeader(this, attr, param);
	if (!ptod)
	    {
	    mssError(0, "MIME", "Could not find integer value. Result is not valid.");
	    return -1;
	    }

    return ptod->Data.Integer;
    }

/*** libmime_GetStringAttr - Gets a string attribute from the
 *** given Mime header.
 ***/
char*
libmime_GetStringAttr(pMimeHeader this, char* attr, char* param)
    {
    pTObjData ptod = NULL;

	ptod = libmime_GetPtodFromHeader(this, attr, param);
	if (!ptod)
	    {
	    mssError(0, "MIME", "Could not find string value. Result is not valid.");
	    return NULL;
	    }

    return ptod->Data.String;
    }

/*** libmime_GetStringArrayAttr - Gets a string array attribute from the
 *** given Mime header.
 ***/
pStringVec
libmime_GetStringArrayAttr(pMimeHeader this, char* attr, char* param)
    {
    pTObjData ptod = NULL;

	ptod = libmime_GetPtodFromHeader(this, attr, param);
	if (!ptod)
	    {
	    mssError(0, "MIME", "Could not find string array value. Result is not valid.");
	    return NULL;
	    }

    return ptod->Data.StringVec;
    }

/*** libmime_GetAttr - Gets a generic attribute from the
 *** given Mime header.
 ***/
void*
libmime_GetAttr(pMimeHeader this, char* attr, char* param)
    {
    pTObjData ptod = NULL;

	ptod = libmime_GetPtodFromHeader(this, attr, param);
	if (!ptod)
	    {
	    mssError(0, "MIME", "Cound not find generic value. Result is not valid.");
	    return NULL;
	    }

    return ptod->Data.Generic;
    }

/*** libmime_GetArrayAttr - Gets a generic array attribute from the given
 *** Mime header.
 ***/
pXArray
libmime_GetArrayAttr(pMimeHeader this, char* attr, char* param)
    {
    pTObjData ptod = NULL;

	ptod = libmime_GetPtodFromHeader(this, attr, param);
	if (!ptod)
	    {
	    mssError(0, "MIME", "Cound not find array value. Result is not valid.");
	    return NULL;
	    }

    return (pXArray)ptod->Data.Generic;
    }

/*** libmime_SetIntAttr - Sets an integer attribute in the
 *** given Mime header.
 ***/
int
libmime_SetIntAttr(pMimeHeader this, char* name, int data)
    {
    pMimeAttr attr;
    pMimeAttr oldAttr;

	/** Get the old attribute. **/
	oldAttr = (pMimeAttr)xhLookup(&this->Attrs, name);

	/** If the attribute does not yet exist, create it. **/
	if (!oldAttr)
	    {
	    return libmime_CreateIntAttr(this, name, data);
	    }

	/** Allocate the Mime attribute. **/
	attr = (pMimeAttr)nmMalloc(sizeof(MimeAttr));
	if (!attr)
	    {
	    return -1;
	    }
	memset(attr, 0, sizeof(MimeAttr));

	/** Populate the Mime attribute. **/
	attr->Name = name;
	attr->Ptod = ptodCreateInt(data);

	/** Get the old attribute. **/
	oldAttr = (pMimeAttr)xhLookup(&this->Attrs, name);

	/** Replace the current attribute with the new one. **/
	xhReplace(&this->Attrs, name, (char*)attr);

	/** Deallocate the old attribute. **/
	ptodFree(oldAttr->Ptod);
	nmFree(oldAttr, sizeof(MimeAttr));

    return 0;
    }

/*** libmime_SetStringAttr - Sets a string attribute in the
 *** given Mime header.
 ***/
int
libmime_SetStringAttr(pMimeHeader this, char* name, char* data, int flags)
    {
    pMimeAttr attr;
    pMimeAttr oldAttr;

	/** Get the old attribute. **/
	oldAttr = (pMimeAttr)xhLookup(&this->Attrs, name);

	/** If the attribute does not yet exist, create it. **/
	if (!oldAttr)
	    {
	    /** NOTE: We assume that the user wants no flags if none are specified. **/
	    if (flags < 0) flags = 0;

	    return libmime_CreateStringAttr(this, name, data, flags);
	    }

	/** Allocate the Mime attribute. **/
	attr = (pMimeAttr)nmMalloc(sizeof(MimeAttr));
	if (!attr)
	    {
	    return -1;
	    }
	memset(attr, 0, sizeof(MimeAttr));

	/** Populate the Mime attribute. **/
	attr->Name = name;
	attr->Ptod = ptodCreateString(data, oldAttr->Ptod->Flags);

	/** If the flags should not change (aka, flags < 0) **/
	if (flags < 0)
	    {
	    attr->Ptod->Flags = oldAttr->Ptod->Flags;
	    }

	/** Replace the current attribute with the new one. **/
	xhReplace(&this->Attrs, name, (char*)attr);

	/** Deallocate the old attribute. **/
	ptodFree(oldAttr->Ptod);
	nmFree(oldAttr, sizeof(MimeAttr));

    return 0;
    }

/*** libmime_SetAttr - Sets a generic attribute in the
 *** given Mime header.
 ***/
int
libmime_SetAttr(pMimeHeader this, char* name, void* data, int datatype)
    {
    pMimeAttr attr;
    pMimeAttr oldAttr;

	/** Get the old attribute. **/
	oldAttr = (pMimeAttr)xhLookup(&this->Attrs, name);

	/** If the attribute does not yet exist, create it. **/
	if (!oldAttr)
	    {
	    return libmime_CreateAttr(this, name, data, datatype);
	    }

	/** Allocate the Mime attribute. **/
	attr = (pMimeAttr)nmMalloc(sizeof(MimeAttr));
	if (!attr)
	    {
	    return -1;
	    }
	memset(attr, 0, sizeof(MimeAttr));

	/** Populate the Mime attribute. **/
	attr->Name = name;
	attr->Ptod = ptodCreate(data, datatype);

	/** Replace the current attribute with the new one. **/
	xhReplace(&this->Attrs, name, (char*)attr);

	/** Deallocate the old attribute. **/
	ptodFree(oldAttr->Ptod);
	nmFree(oldAttr, sizeof(MimeAttr));

    return 0;
    }

/*** libmime_AddStringArrayAttr - Adds a string to the string array attribute
 *** in the given Mime header.
 ***/
int
libmime_AddStringArrayAttr(pMimeHeader this, char* attr, char* param, char* data)
    {
    pTObjData ptod = NULL;
    pStringVec stringVec;
    char** tempVec = NULL;
    int i;

	/** Get the old attribute/parameter ptod. **/
	ptod = libmime_GetPtodFromHeader(this, attr, param);

	/** If the attribute/parametr wasn't found, create it. **/
	if (!ptod)
	    {
	    libmime_CreateStringArrayAttr(this, attr, param);
	    ptod = libmime_GetPtodFromHeader(this, attr, param);
	    }

	/** Get the string vector from the attribute. **/
	stringVec = ptod->Data.StringVec;

	/** Allocate a new string array. **/
	tempVec = (char**)nmMalloc(sizeof(char)*(stringVec->nStrings+1));
	if (!tempVec)
	    {
	    return -1;
	    }
	memset(tempVec, 0, sizeof(char)*(stringVec->nStrings+1));

	/** Copy the previous contents to the new vector. **/
	for (i = 0; i < stringVec->nStrings; i++)
	    {
	    tempVec[i] = stringVec->Strings[i];
	    }

	/** Append the new data to the string vector. **/
	tempVec[i] = nmSysStrdup(data);
	if (!tempVec[i])
	    {
	    return -1;
	    }

	/** Replace the old vector. **/
	nmFree(stringVec->Strings, sizeof(char)*stringVec->nStrings);
	stringVec->Strings = tempVec;
	stringVec->nStrings++;

    return 0;
    }

/*** libmime_AppendStringArrayAttr - Appends an XArray of strings to the string
 *** array attribute in the given Mime header.
 ***/
int
libmime_AppendStringArrayAttr(pMimeHeader this, char* attr, char* param, pXArray dataList)
    {
    pTObjData ptod = NULL;
    pStringVec stringVec;
    char** tempVec;
    int i, j;

	/** Get the old attribute/parameter ptod. **/
	ptod = libmime_GetPtodFromHeader(this, attr, param);

	/** If the attribute/parametr wasn't found, create it. **/
	if (!ptod)
	    {
	    libmime_CreateStringArrayAttr(this, attr, param);
	    ptod = libmime_GetPtodFromHeader(this, attr, param);
	    }

	/** Get the string vector from the attribute. **/
	stringVec = ptod->Data.StringVec;

	/** Allocate a new string vector. **/
	tempVec = (char**)nmMalloc(sizeof(char)*stringVec->nStrings + dataList->nItems);
	if (!tempVec)
	    {
	    return -1;
	    }
	memset(tempVec, 0, sizeof(char)*stringVec->nStrings + dataList->nItems);

	/** Copy the old string vector to the new one. **/
	for(i = 0; i < stringVec->nStrings; i++)
	    {
	    tempVec[i] = stringVec->Strings[i];
	    }

	/** Append the contents of the XArray data list. **/
	for(j = 0; j < dataList->nItems; j++)
	    {
	    tempVec[i+j] = nmSysStrdup((char*)xaGetItem(dataList, j));
	    if (!tempVec[i+j])
		{
		return -1;
		}
	    }

	/** Replace the old string vector with the new one. **/
	nmFree(stringVec->Strings, sizeof(char)*stringVec->nStrings);
	stringVec->Strings = tempVec;
	stringVec->nStrings += dataList->nItems;

    return 0;
    }

/*** libmime_AddArrayAttr - Adds a generic element to a generic array
 *** attribute in the given Mime header.
 ***/
int
libmime_AddArrayAttr(pMimeHeader this, char* attr, char* param, void* data)
    {
    pMimeAttr oldAttr = NULL;
    pXArray array;

	/** Get the old attribute/parameter ptod. **/
	ptod = libmime_GetPtodFromHeader(this, attr, param);

	/** If the attribute/parametr wasn't found, create it. **/
	if (!ptod)
	    {
	    libmime_CreateStringArrayAttr(this, attr, param);
	    ptod = libmime_GetPtodFromHeader(this, attr, param);
	    }

	/** Get the array from the attribute. **/
	array = (pXArray)oldAttr->Ptod->Data.Generic;

	/** Add the item to the XArray. **/
	xaAddItem(array, data);

    return 0;
    }

/*** libmime_AppendArrayAttr - Adds an XArray of generic elements to a generic
 *** array attribute in the given Mime header.
 ***/
int
libmime_AppendArrayAttr(pMimeHeader this, char* attr, char* param, pXArray dataList)
    {
    pTObjData ptod = NULL;
    pMimeAttr oldAttr = NULL;
    pXArray array;
    int i;

	/** Get the old attribute/parameter ptod. **/
	ptod = libmime_GetPtodFromHeader(this, attr, param);

	/** If the attribute/parametr wasn't found, create it. **/
	if (!ptod)
	    {
	    libmime_CreateStringArrayAttr(this, attr, param);
	    ptod = libmime_GetPtodFromHeader(this, attr, param);
	    }

	/** Get the array from the attribute. **/
	array = (pXArray)oldAttr->Ptod->Data.Generic;

	/** Add the items from the data list to the XArray. **/
	for (i = 0; i < xaCount(dataList); i++)
	    {
	    xaAddItem(array, xaGetItem(dataList, i));
	    }

    return 0;
    }

/*** libmime_ClearAttr - Deallocates an attribute and all its contents.
 *** Designed as a callback for the xhClear function.
 ***/
int
libmime_ClearAttr(char* attr_c, void* arg)
    {
    pMimeAttr attr = (pMimeAttr)attr_c;

	/** Take care of any special deallocation needs. **/
	libmime_ClearSpecials(attr->Ptod);

	/** Clear the parameters. **/
	if (attr->Params.nRows)
	    {
	    xhClear(&attr->Params, libmime_ClearParam, NULL);
	    xhDeInit(&attr->Params);
	    }

	/** Free the data memory of the attribute. **/
	ptodFree(attr->Ptod);

	/** Free the attribute memory. **/
	nmFree(attr, sizeof(MimeAttr));

    return 0;
    }

/*** libmime_ClearParam - Deallocates a parameter and all its contents.
 *** Designed as a callback for the xhClear function.
 ***/
int
libmime_ClearParam(char* param_c, void* arg)
    {
    pMimeParam param = (pMimeParam)param_c;

	/** Take care of any special deallocation needs. **/
	libmime_ClearSpecials(param->Ptod);

	/** Free the data memory of the parameter. **/
	ptodFree(param->Ptod);

	/** Free the parameter memory. **/
	nmFree(param, sizeof(MimeAttr));

    return 0;
    }

/*** libmime_ClearSpecials - Deallocate any special cases in the given
 *** attribute/parameter ptod.
 *** NOTE: This function assumes a few things about how attributes are
 *** allocated:
 ***     - StingVec attributes contain arrays of nmSysStrdup/nmSysMalloc
 ***       strings which should be freed.
 ***     - Array type attributes contain an XArray of pEmailAttr stucts
 ***       which should be freed.
 ***/
int
libmime_ClearSpecials(pTObjData ptod)
    {
    pStringVec stringVec = NULL;
    pXArray array = NULL;
    pEmailAddr addr = NULL;
    int i;

	/** If the data in the ptod is unmanaged, we have to take care of it. **/
	if (ptod->Flags & DATA_TF_UNMANAGED)
	    {
	    /** TODO: Update ptod code to handle StringVec correctly when freeing.
	     ** (Once we decide what correctly is) **/
	    if (ptod->DataType == DATA_T_STRINGVEC)
		{
		/** Get the StringVec from the ptod. **/
		stringVec = (pStringVec)ptod->Data.StringVec;

		/** Deallocate each string in the StringVec. **/
		for (i = 0; i < stringVec->nStrings; i++)
		    {
		    nmSysFree(stringVec->Strings[i]);
		    }

		/** Deallocate the StringVec string array. **/
		nmFree(stringVec->Strings, sizeof(char)*stringVec->nStrings);
		}
	    /** Handle our custom XArray type attribute. Yeah hijacked type names! **/
	    else if (ptod->DataType == DATA_T_ARRAY)
		{
		/** Get the XArray from the ptod. **/
		array = (pXArray)ptod->Data.Generic;

		/** Deallocate each email in the list. **/
		for (i = 0; i < array->nItems; i++)
		    {
		    addr = xaGetItem(array, i);
		    if (addr)
			{
			nmFree(addr, sizeof(EmailAddr));
			addr = NULL;
			}
		    }

		/** Deinit the XArray. **/
		xaDeInit(array);
		}
	    }

    return 0;
    }


