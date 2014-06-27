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
    /** testing merge stuffs **/

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
libmime_CreateStringAttr(pMimeHeader this, char* name, char* data, int flags)
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
	attr->Ptod = ptodCreateString(data, flags);

	/** Add the Mime attribute to the attributes array. **/
	xhAdd(&this->Attrs, name, (char*)attr);

    return 0;
    }

/*** libmime_CreateStringArrayAttr - Creates and stores a string array attribute
 *** in the given Mime header.
 ***/
int
libmime_CreateStringArrayAttr(pMimeHeader this, char* name)
    {
    pStringVec attrVec;

	/** Allocate the attribute vector. **/
	attrVec = (pStringVec)nmMalloc(sizeof(StringVec));
	if (!attrVec)
	    {
	    return -1;
	    }
	memset(attrVec, 0, sizeof(StringVec));

	/** Create the attribute with the allocated StringVec. **/
	libmime_CreateAttr(this, name, attrVec, DATA_T_STRINGVEC);

    return 0;
    }

/*** libmime_CreateAttr - Creates and stores a generic attribute in the
 *** given Mime header.
 ***/
int
libmime_CreateAttr(pMimeHeader this, char* name, void* data, int datatype)
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
	attr->Ptod = ptodCreate(data, datatype);

	/** Add the Mime attribute to the attributes array. **/
	xhAdd(&this->Attrs, name, (char*)attr);

    return 0;
    }

/*** libmime_CreateArrayAttr - Creates a generic array attribute in the
 *** given Mime header.
 ***/
int
libmime_CreateArrayAttr(pMimeHeader this, char* name)
    {
    pXArray array;

	/** Allocate the generic array. **/
	array = (pXArray)nmMalloc(sizeof(XArray));
	if (!array)
	    {
	    return -1;
	    }
	memset(array, 0, sizeof(XArray));
	xaInit(array, 4);

	/** Create an attribute containing the generic array. **/
	libmime_CreateAttr(this, name, array, DATA_T_ARRAY);

    return 0;
    }

/*** libmime_GetIntAttr - Gets an integer attribute from the
 *** given Mime header.
 ***/
int
libmime_GetIntAttr(pMimeHeader this, char* name)
    {
    return ((pMimeAttr)xhLookup(&this->Attrs, name))->Ptod->Data.Integer;
    }

/*** libmime_GetStringAttr - Gets a string attribute from the
 *** given Mime header.
 ***/
char*
libmime_GetStringAttr(pMimeHeader this, char* name)
    {
    return ((pMimeAttr)xhLookup(&this->Attrs, name))->Ptod->Data.String;
    }

/*** libmime_GetStringArrayAttr - Gets a string array attribute from the
 *** given Mime header.
 ***/
pStringVec
libmime_GetStringArrayAttr(pMimeHeader this, char* name)
    {
    return ((pMimeAttr)xhLookup(&this->Attrs, name))->Ptod->Data.StringVec;
    }

/*** libmime_GetAttr - Gets a generic attribute from the
 *** given Mime header.
 ***/
void*
libmime_GetAttr(pMimeHeader this, char* name)
    {
    return ((pMimeAttr)xhLookup(&this->Attrs, name))->Ptod->Data.Generic;
    }

/*** libmime_GetArrayAttr - Gets a generic array attribute from the given
 *** Mime header.
 ***/
pXArray libmime_GetArrayAttr(pMimeHeader this, char*name)
    {
    return (pXArray)((pMimeAttr)xhLookup(&this->Attrs, name))->Ptod->Data.Generic;
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
libmime_AddStringArrayAttr(pMimeHeader this, char* name, char* data)
    {
    pMimeAttr oldAttr;
    pStringVec stringVec;
    char** tempVec;
    int i;

	/** Get the old attribute. **/
	oldAttr = (pMimeAttr)xhLookup(&this->Attrs, name);

	/** If the attribute does not yet exist, create it. **/
	if (!oldAttr)
	    {
	    libmime_CreateStringArrayAttr(this, name);
	    oldAttr = (pMimeAttr)xhLookup(&this->Attrs, name);
	    }

	/** Get the string vector from the attribute. **/
	stringVec = oldAttr->Ptod->Data.StringVec;

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
libmime_AppendStringArrayAttr(pMimeHeader this, char* name, pXArray dataList)
    {
    pMimeAttr oldAttr;
    pStringVec stringVec;
    char** tempVec;
    int i, j;

	/** Get the old attribute. **/
	oldAttr = (pMimeAttr)xhLookup(&this->Attrs, name);

	/** If the attribute does not yet exist, create it. **/
	if (!oldAttr)
	    {
	    libmime_CreateStringArrayAttr(this, name);
	    oldAttr = (pMimeAttr)xhLookup(&this->Attrs, name);
	    }

	/** Get the string vector from the attribute. **/
	stringVec = oldAttr->Ptod->Data.StringVec;

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
libmime_AddArrayAttr(pMimeHeader this, char* name, void* data)
    {
    pMimeAttr oldAttr = NULL;
    pXArray array;

	/** Get the attribute. **/
	oldAttr = (pMimeAttr)xhLookup(&this->Attrs, name);

	/** If the attribute wasn't found, create it. **/
	if (!oldAttr)
	    {
	    libmime_CreateArrayAttr(this, name);
	    oldAttr = (pMimeAttr)xhLookup(&this->Attrs, name);
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
libmime_AppendArrayAttr(pMimeHeader this, char* name, pXArray dataList)
    {
    pMimeAttr oldAttr = NULL;
    pXArray array;
    int i;

	/** Get the attribute. **/
	oldAttr = (pMimeAttr)xhLookup(&this->Attrs, name);

	/** If the attribute wasn't found, create it. **/
	if (!oldAttr)
	    {
	    libmime_CreateArrayAttr(this, name);
	    oldAttr = (pMimeAttr)xhLookup(&this->Attrs, name);
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

