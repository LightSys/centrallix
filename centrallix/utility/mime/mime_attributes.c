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
#include "obj.h"
#include "mime.h"

/*** libmime_ParseAttr - Parses an attribute and stores it in the Mime header as
 *** an attribute.
 ***/
int
libmime_ParseAttr(pMimeHeader this, char* name, char* data, int attrSeekStart, int attrSeekEnd, int nameOffset)
    {
    int seekEnd, seekStart;
    char* beginPtr = NULL;
    char* paramName = NULL;
    char* token = NULL;
    char* currentOffset = NULL;

	/** Append all data up to the next semicolon. **/
	token  = strtok_r(data, ";", &currentOffset);
	if (!token)
	    {
	    token = currentOffset;
	    }
	libmime_StringTrim(token);

	/** Calculate the seek offset for the attribute value. **/
	seekStart = attrSeekStart;
	seekEnd = seekStart + strlen(token) + (token - data) + nameOffset;

	/** Call the appropriate parse function for the attribute. **/
	/** Handle special attributes. **/
	if (!strcasecmp(name, "Content-Type"))
	    {
	    libmime_SetContentType(this, data);
	    }
	else if (!strcasecmp(name, "Content-Transfer-Encoding"))
	    {
	    return libmime_SetTransferEncoding(this, data);
	    }
	/** Check for integer attributes. **/
	else if (!strcasecmp(name, "Content-Length"))
	    {
	    libmime_CreateIntAttr(this, name, NULL, strtol(token, NULL, 10));
	    }
	/** Check for email address attributes. **/
	else if (!strcasecmp(name, "Reply-To") ||
		 !strcasecmp(name, "Sender"))
	    {
	    libmime_ParseEmailAttr(this, name, token);
	    }
	/** Check for email list attributes. **/
	else if (!strcasecmp(name, "To")   ||
		 !strcasecmp(name, "From") ||
		 !strcasecmp(name, "Cc")   ||
		 !strcasecmp(name, "Bcc"))
	    {
	    libmime_ParseEmailListAttr(this, name, token);
	    }
	/** Check for string list attributes. **/
	else if (!strcasecmp(name, "Keywords"))
	    {
	    libmime_ParseCsvAttr(this, name, token);
	    }
	/** Everything else defaults to string. **/
	else
	    {
	    libmime_CreateStringAttr(this, name, NULL, token, 0);
	    }

	/** Set the offset values in the attribute structure. **/
	libmime_GetMimeAttr(this, name)->ValueSeekStart = seekStart;
	libmime_GetMimeAttr(this, name)->ValueSeekEnd = seekEnd;

	/** Attempt to find the first parameter. **/
	token = strtok_r(NULL, "=", &currentOffset);

	/** Process all parameters until the end of the line. **/
	while (token)
	    {
	    /** Store the parameter name. **/
	    paramName = token;

	    /** Get the value of the parameter. **/
	    token = strtok_r(NULL, ";", &currentOffset);

	    /** If this is the last parameter and there is no closing semi-colon...  **/
	    if (!token)
		{
		/** Just use the final token. **/
		token = currentOffset;
		}
	    libmime_StringTrim(token);

	    /** Trim the parameter name. **/
	    beginPtr = paramName;
	    libmime_StringTrim(paramName);

	    /** Add the offset from the beginning of the untrimmed parameter name to the beginning of the actual parameter name. **/
	    seekStart = seekEnd + (paramName - beginPtr) + 1; /* NOTE: +1 skips the semicolon. */

	    /** Add the length of the value and the parameter name to the start offset to find the end offset of the parameter. **/
	    seekEnd = seekStart + strlen(token) + (token - paramName);

	    /** Store the parameter. **/
	    libmime_CreateStringAttr(this, name, paramName, token, 0);

	    /** Set the offset values in the parameter structure. **/
	    libmime_GetMimeParam(this, name, paramName)->ValueSeekStart = seekStart;
	    libmime_GetMimeParam(this, name, paramName)->ValueSeekEnd = seekEnd;

	    /** Attempt to get the next parameter. **/
	    token = strtok_r(NULL, "=", &currentOffset);
	    }

    return 0;
    }

/*** libmime_ParseEmailAttr - Parses an email address attribute and adds it to
 *** the Mime header.
 ***/
int
libmime_ParseEmailAttr(pMimeHeader this, char* name, char* data)
    {
    pEmailAddr emailAddr = NULL;

	/** Allocate the email address. **/
	emailAddr = (pEmailAddr)nmMalloc(sizeof(EmailAddr));
	if (!emailAddr)
	    {
	    mssError(1, "MIME", "Failed to allocate the email address structure");
	    return -1;
	    }

	/** Parse the email address. **/
	if (!libmime_ParseAddress(data, emailAddr))
	    {
	    mssError(1, "MIME", "Failed to parse the email address");
	    return -1;
	    }

	/** Create the email attribute. **/
	libmime_CreateStringAttr(this, name, NULL, emailAddr->AddressLine, 0);

	/** Store the struct as a parameter. **/
	libmime_CreateAttr(this, name, "Struct", emailAddr, 0);

    return 0;
    }

/*** libmime_ParseEmailListAttr - Parses an email list attribute and adds it to
 *** the Mime header.
 ***/
int
libmime_ParseEmailListAttr(pMimeHeader this, char* name, char* data)
    {
    XArray emailList;
    int i;

	/** Initialize the email list. **/
	xaInit(&emailList, 4);

	/** Parse the email list into an XArray of email structs. **/
	libmime_ParseAddressList(data, &emailList);

	/** Construct and store the string list of email address lines as an attribute. **/
	for (i = 0; i < emailList.nItems; i++)
	    {
	    libmime_AddStringArrayAttr(this, name, NULL, ((pEmailAddr)xaGetItem(&emailList, i))->AddressLine);
	    }

	/** Store the email structure list in a parameter. **/
	libmime_AppendArrayAttr(this, name, "Struct", &emailList);

	/** Deinitialize the email list. **/
	xaDeInit(&emailList);
    return 0;
    }

/*** libmime_ParseCsvAttr - Parses a csv list and stores it in an attribute in
 *** the given Mime header.
 ***/
int
libmime_ParseCsvAttr(pMimeHeader this, char* name, char* data)
    {
    char* token = NULL;
    char* currentOffset = NULL;

	/** Get the first item in the list. **/
	token = strtok_r(data, ",", &currentOffset);

	/** If there are no commas, the entire string is a single item. **/
	if (!token)
	    {
	    token = currentOffset;
	    }

	/** Add all items in the list up until the end of the string. **/
	while (token)
	    {
	    /** Trim the token. **/
	    libmime_StringTrim(token);

	    /** Add the item to the list. **/
	    libmime_AddStringArrayAttr(this, name, NULL, token);

	    /** Attempt to get the next item in the list. **/
	    token = strtok_r(NULL, ",", &currentOffset);
	    }

	/** Trim the final token. **/
	libmime_StringTrim(currentOffset);

	/** Add the final token to the attribute list. **/
	libmime_AddStringArrayAttr(this, name, NULL, currentOffset);

    return 0;
    }

/*** libmime_CreateIntAttr - Creates and stores an integer attribute in the
 *** given Mime header.
 ***/
int
libmime_CreateIntAttr(pMimeHeader this, char* attr, char* param, int data)
    {
    pTObjData* pPtod = NULL;

	/** Create the parameter/attribute and get the ptod. **/
	pPtod = libmime_CreateAttrParam(this, attr, param);
	if (!pPtod)
	    {
	    mssError(0, "MIME", "Could not create integer attribute.");
	    return -1;
	    }

	/** Populate the ptod. **/
	*pPtod = ptodCreateInt(data);

    return 0;
    }

/*** libmime_CreateStringAttr - Creates and stores a string attribute in the
 *** given Mime header.
 ***/
int
libmime_CreateStringAttr(pMimeHeader this, char* attr, char* param, char* data, int flags)
    {
    pTObjData* pPtod = NULL;

	/** Create the parameter/attribute and get the ptod. **/
	pPtod = libmime_CreateAttrParam(this, attr, param);
	if (!pPtod)
	    {
	    mssError(0, "MIME", "Could not create string attribute.");
	    return -1;
	    }

	/** Populate the ptod. **/
	*pPtod = ptodCreateString(data, flags);

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
	    mssError(0, "MIME", "Could not create string array attribute.");
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
    pTObjData* pPtod = NULL;

	/** Create the attribute/parameter and get the ptod. **/
	pPtod = libmime_CreateAttrParam(this, attr, param);
	if (!pPtod)
	    {
	    mssError(0, "MIME", "Could not create generic attribute.");
	    return -1;
	    }

	/** Populate the ptod. **/
	*pPtod = ptodCreate(data, datatype);

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
	    mssError(0, "MIME", "Could not create array attribute.");
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
pTObjData*
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
	    if (libmime_xhAdd(&this->Attrs, attrName, (char*)attr) == -1)
		{
		mssError(1, "MIME", "Attribute or parameter already exists.");
		return NULL;
		}

	    /** Return the pointer to the relevant ptod. **/
	    return &attr->Ptod;
	    }
	/** Otherwise we are creating a parameter. **/
	else
	    {
	    /** Find the appropriate attribute. **/
	    attr = (pMimeAttr)libmime_xhLookup(&this->Attrs, attrName);
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
	    memset(param, 0, sizeof(MimeParam));

	    /** Set the name of the parameter. **/
	    param->Name = paramName;

	    /** If necessary, initialize the parameter table. **/
	    if (!attr->Params.nRows)
		{
		xhInit(&attr->Params, 7, 0);
		}

	    /** Add the Mime parameter to the parameter hash. **/
	    libmime_xhAdd(&attr->Params, paramName, (char*)param);

	    /** Return the pointer to the relevant ptod.**/
	    return &param->Ptod;
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
    pTObjData* pPtod = NULL;

    pPtod = libmime_GetPtodPointer(this, attr, param);
    if (!pPtod)
	{
	return NULL;
	}
    return *pPtod;
    }

/*** libmime_GetPtodPointer - Gets the pointer to the PTOD from an attribute/parameter
 *** based on the passed in attribute and param. If param is NULL (or ""), we assume
 *** we want the value of the attr, otherwise search for the param in the XHashTable.
 ***
 *** returns pointer on success, NULL on failure
 ***/
pTObjData*
libmime_GetPtodPointer(pMimeHeader this, char* attr, char* param)
    {
    void *ptr = NULL;

	/** Get the attribute value **/
	ptr = (pMimeAttr)libmime_xhLookup(&this->Attrs, attr);
	if (!ptr) return NULL;

	/** If param, search the XHashTable, otherwise return default. **/
	if (param && strlen(param))
	    {
		if (!((pMimeAttr)ptr)->Params.nRows) return NULL;
		/** Get the param value **/
		ptr = (pMimeParam)libmime_xhLookup(&((pMimeAttr)ptr)->Params, param);
		if (!ptr) return NULL;

		return &((pMimeParam)ptr)->Ptod;
	    }

    /** No param, so give the default **/
    return &((pMimeAttr)ptr)->Ptod;
    }

/*** libmime_GetAttrParamNames - Parses the attribute and parameter
 *** names from a given raw string from an OS query.
 ***/
int
libmime_GetAttrParamNames(char* raw, char** attr, char** param)
    {
	/** Separate the raw string on a separating dash. **/
	*attr = raw;
	*param = strchr(raw, '.');

	/** If there is no separator, the entire string is the attribute name. **/
	if (!*param)
	    {
	    *attr = raw;
	    *param = NULL;

	    return 0;
	    }

	/** Null terminate the attribute and point param to the character after it. **/
	**param = '\0';
	(*param)++;

    return 0;
    }

/*** libmime_GetMimeAttr - Gets the given Mime attribute structure from the given
 *** Mime header.
 ***/
pMimeAttr
libmime_GetMimeAttr(pMimeHeader this, char* attr)
    {
    return (pMimeAttr)libmime_xhLookup(&this->Attrs, attr);
    }

/*** libmime_GetMimeParam - Gets the given Mime parameter structure from the given
 *** Mime header.
 ***/
pMimeParam
libmime_GetMimeParam(pMimeHeader this, char* attr, char* param)
    {
    pMimeAttr attrStruct;

	attrStruct = libmime_GetMimeAttr(this, attr);

	/** Makes sure the struct exists, and that
	 ** we don't look for a param in an uninitialized hash.
	 **/
	if (!attrStruct || !attrStruct->Params.nRows) return NULL;

    return (pMimeParam)libmime_xhLookup(&attrStruct->Params, param);
    }

/*** libmime_GetIntAttr - Gets an integer attribute from the
 *** given Mime header and stores it into ret.
 *** returns 0 on success, -1 on failure
 ***/
int
libmime_GetIntAttr(pMimeHeader this, char* attr, char* param, int* ret)
    {
    pTObjData ptod = NULL;

	ptod = libmime_GetPtodFromHeader(this, attr, param);
	if (!ptod) return -1;
	*ret = ptod->Data.Integer;

    return 0;
    }

/*** libmime_GetStringAttr - Gets a string attribute from the
 *** given Mime header.
 *** returns 0 on success, -1 on failure
 ***/
int
libmime_GetStringAttr(pMimeHeader this, char* attr, char* param, char** ret)
    {
    pTObjData ptod = NULL;

	ptod = libmime_GetPtodFromHeader(this, attr, param);
	if (!ptod) return -1;

	*ret = ptod->Data.String;

    return 0;
    }

/*** libmime_GetStringArrayAttr - Gets a string array attribute from the
 *** given Mime header.
 ***/
int
libmime_GetStringArrayAttr(pMimeHeader this, char* attr, char* param, pStringVec* ret)
    {
    pTObjData ptod = NULL;

	ptod = libmime_GetPtodFromHeader(this, attr, param);
	if (!ptod) return -1;

    *ret = ptod->Data.StringVec;

    return 0;
    }

/*** libmime_GetAttr - Gets a generic attribute from the
 *** given Mime header.
 ***/
int
libmime_GetAttr(pMimeHeader this, char* attr, char* param, void** ret)
    {
    pTObjData ptod = NULL;

	ptod = libmime_GetPtodFromHeader(this, attr, param);
	if (!ptod) return -1;

	*ret = ptod->Data.Generic;

    return 0;
    }

/*** libmime_GetArrayAttr - Gets a generic array attribute from the given
 *** Mime header.
 ***/
int
libmime_GetArrayAttr(pMimeHeader this, char* attr, char* param, pXArray* ret)
    {
    pTObjData ptod = NULL;

	ptod = libmime_GetPtodFromHeader(this, attr, param);
	if (!ptod) return -1;

	*ret = (pXArray)ptod->Data.Generic;

    return 0;
    }

/*** libmime_SetIntAttr - Sets an integer attribute in the
 *** given Mime header.
 ***/
int
libmime_SetIntAttr(pMimeHeader this, char* attr, char* param, int data)
    {
    pTObjData ptod;

	/** Get the old ptod. **/
	ptod = libmime_GetPtodFromHeader(this, attr, param);

	/** If our pointer to our other pointer is NULL or our pointer is NULL: create the attr/param. **/
	if (!ptod)
	    {
	    if (libmime_CreateIntAttr(this, attr, param, data))
		{
		mssError(0, "MIME", "Unable to create integer attribute");
		return -1;
		}
	    return 0;
	    }

	/** Change the data. **/
	ptod->Data.Integer = data;

    return 0;
    }

/*** libmime_SetStringAttr - Sets a string attribute in the
 *** given Mime header.
 *** returns - 0 on success, -1 on failure
 ***/
int
libmime_SetStringAttr(pMimeHeader this, char* attr, char* param, char* data, int flags)
    {
    pTObjData *pPtod = NULL;

	/** Use the new flags if we are passed specific values. **/
	if (flags < 0)
	    {
	    /** Use flags from old ptod if we have it, otherwise assume 0 **/
	    if (pPtod) flags = (*pPtod)->Flags;
	    else flags = 0;
	    }

	/** Get the old ptod. **/
	pPtod = libmime_GetPtodPointer(this, attr, param);

	/** If our pointer to our other pointer is NULL or our pointer is NULL: create the attr/param. **/
	if (!pPtod || !*pPtod)
	    {
	    if (libmime_CreateStringAttr(this, attr, param, data, flags))
		{
		mssError(0, "MIME", "Unable to create string attribute");
		return -1;
		}
	    return 0;
	    }

	/** Free the old ptod. **/
	ptodFree(*pPtod);

	/** Make the new ptod. **/
	*pPtod = ptodCreateString(data, flags);

    return 0;
    }

/*** libmime_SetAttr - Sets a generic attribute in the
 *** given Mime header.
 ***/
int
libmime_SetAttr(pMimeHeader this, char* attr, char* param, void* data, int datatype)
    {
    pTObjData *pPtod = NULL;

	/** Get the old ptod. **/
	pPtod = libmime_GetPtodPointer(this, attr, param);

	/** If our pointer to our other pointer is NULL or our pointer is NULL: create the attr/param. **/
	if (!pPtod || !*pPtod)
	    {
	    if (libmime_CreateAttr(this, attr, param, data, datatype))
		{
		mssError(0, "MIME", "Unable to create attribute");
		return -1;
		}
	    return 0;
	    }

	/** Free the old ptod. **/
	ptodFree(*pPtod);

	/** Make the new ptod. **/
	*pPtod = ptodCreate(data, datatype);

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

	/** If the attribute/parameter wasn't found, create it. **/
	if (!ptod)
	    {
	    libmime_CreateStringArrayAttr(this, attr, param);
	    ptod = libmime_GetPtodFromHeader(this, attr, param);
	    if (!ptod)
		{
		mssError(0, "MIME", "Failed to create the string array attribute");
		return 0;
		}
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
	    if (!ptod)
		{
		mssError(0, "MIME", "Failed to create the string array attribute");
		return 0;
		}
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
    pTObjData ptod = NULL;
    pXArray array;

	/** Get the old attribute/parameter ptod. **/
	ptod = libmime_GetPtodFromHeader(this, attr, param);

	/** If the attribute/parametr wasn't found, create it. **/
	if (!ptod)
	    {
	    libmime_CreateStringArrayAttr(this, attr, param);
	    ptod = libmime_GetPtodFromHeader(this, attr, param);
	    if (!ptod)
		{
		mssError(0, "MIME", "Failed to create the array attribute");
		return 0;
		}
	    }

	/** Get the array from the attribute. **/
	array = (pXArray)ptod->Data.Generic;

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
    pXArray array;
    int i;

	/** Get the old attribute/parameter ptod. **/
	ptod = libmime_GetPtodFromHeader(this, attr, param);

	/** If the attribute/parametr wasn't found, create it. **/
	if (!ptod)
	    {
	    libmime_CreateArrayAttr(this, attr, param);
	    ptod = libmime_GetPtodFromHeader(this, attr, param);
	    if (!ptod)
		{
		mssError(0, "MIME", "Failed to create the array attribute");
		return 0;
		}
	    }

	/** Get the array from the attribute. **/
	array = (pXArray)ptod->Data.Generic;

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
	    libmime_xhDeInit(&attr->Params);
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

/*** libmime_WriteAttrParam - Adds/sets an attribute/parameter to the given file descriptor.
 ***
 *** NOTE: Assumes that the file descriptor points to the location to which
 *** the attribute/parameter should be written. When the function returns the file
 *** descriptor should point to the offset after the attribute/parameter value.
 ***/
int
libmime_WriteAttrParam(pFile fd, pMimeHeader msg, char* attrName, char* paramName, int type, pObjData val)
    {
    XString output;
    XString data;
    pTObjData ptod;
    int bytesWritten;
    int i;

	/** Initialize the strings. **/
	xsInit(&data);
	xsInit(&output);

	/** Convert the value into a string. **/
	if (type != DATA_T_INTEGER)
	    {
	    objDataToString(&data, type, val->Generic, 0);
	    }
	else
	    {
	    objDataToString(&data, type, &val->Integer, 0);
	    }

	/** Construct the output string. **/
	if (!paramName)
	    {
	    xsConcatPrintf(&output, "%s: %s", attrName, data.String);
	    }
	else
	    {
	    xsConcatPrintf(&output, " %s=%s", paramName, data.String);
	    }

	bytesWritten = fdWrite(fd, output.String, strlen(output.String), 0, 0);

	/** Add the new attribute to the header. **/
	if (bytesWritten < 0)
	    {
	    goto error;
	    }

	/** Correct the message offsets for the new attribute. **/
	msg->HdrSeekEnd += bytesWritten;
	msg->MsgSeekStart += bytesWritten;
	msg->MsgSeekEnd += bytesWritten;

	/** Add the attribute to the attribute array according to its datatype. **/
	switch (type)
	    {
	    case DATA_T_INTEGER:
		libmime_SetIntAttr(msg, attrName, paramName, val->Integer);
		break;
	    case DATA_T_STRING:
		libmime_SetStringAttr(msg, attrName, paramName, val->String, 0);
		break;
	    case DATA_T_STRINGVEC:
		/** Get the ptod and clear the string array. **/
		ptod = libmime_GetPtodFromHeader(msg, attrName, paramName);
		if (!libmime_ClearSpecials(ptod))
		    {
		    goto error;
		    }

		/** Add the strings given to the string array. **/
		for (i = 0; i < val->StringVec->nStrings; i++)
		    {
		    libmime_AddStringArrayAttr(msg, attrName, paramName, val->StringVec->Strings[i]);
		    }
		break;
	    case DATA_T_ARRAY:
		/** Get the ptod and clear the array. **/
		ptod = libmime_GetPtodFromHeader(msg, attrName, paramName);
		if (!libmime_ClearSpecials(ptod))
		    {
		    goto error;
		    }

		/** Add the values to the array. **/
		libmime_AppendArrayAttr(msg, attrName, paramName, (pXArray)val->Generic);
		break;
	    default:
		libmime_SetAttr(msg, attrName, paramName, val->Generic, type);
		break;
	    }

	/** Deinitialze the strings. **/
	xsDeInit(&output);
	xsDeInit(&data);

    return 0;

    error:
	xsDeInit(&output);
	xsDeInit(&data);

	return -1;
    }

