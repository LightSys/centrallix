#include <stdlib.h>
#include <string.h>
#include "obj.h"
#include "hints.h"
#include "expression.h"
#include "cxlib/strtcpy.h"
#include "cxlib/newmalloc.h"
#include "cxlib/datatypes.h"
#include "stparse.h"
#include "stparse_ne.h"
#include "param.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2012 LightSys Technology Services, Inc.		*/
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
/* Module:	param.c, param.h                                        */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	26-May-2012                                             */
/*									*/
/* Description:	Based on WgtrAppParam from the wgtr module.  This	*/
/*		provides a simple ability to parse and free parameters	*/
/*		that are in the form of a presentation hints record but	*/
/*		with a data type and name supplied, too.		*/
/*									*/
/************************************************************************/


/*** paramFree - free a parameter
 ***/
int
paramFree(pParam param)
    {

	/*** Clean up and free the structure ***/
	if (param->Hints)
	    objFreeHints(param->Hints);
	if (param->Value)
	    ptodFree(param->Value);
	nmFree(param, sizeof(Param));

    return 0;
    }


/*** paramCreate() - create a parameter.
 ***/
pParam
paramCreate(char* name)
    {
    pParam param = NULL;

	/** Allocate **/
	param = (pParam)nmMalloc(sizeof(Param));
	if (!param)
	    goto error;
	memset(param, 0, sizeof(Param));

	/** Set up **/
	strtcpy(param->Name, name, sizeof(param->Name));
	param->Value = ptodAllocate();
	if (!param->Value)
	    goto error;

	return param;

    error:
	if (param)
	    paramFree(param);
	return NULL;
    }


/*** paramCreateFromInf() - using a StructInf tree, obtain the parameter
 *** settings and build a Param structure.
 ***/
pParam
paramCreateFromInf(pStructInf inf)
    {
    pParam param = NULL;
    char* str;
    int t;

	/** Allocate **/
	param = (pParam)nmMalloc(sizeof(Param));
	if (!param) goto error;
	memset(param, 0, sizeof(Param));

	/** Allocate the typed obj data **/
	param->Value = ptodAllocate();
	if (!param->Value)
	    goto error;

	/** Get name **/
	if (stGetAttrValue(stLookup(inf, "name"), DATA_T_STRING, POD(&str), 0) != 0)
	    str = inf->Name;
	if (!str || !*str)
	    {
	    mssError(1, "PARAM", "Parameter does not have a valid name.");
	    goto error;
	    }
	strtcpy(param->Name, str, sizeof(param->Name));

	/** Get the data type **/
	if (stGetAttrValue(stLookup(inf, "type"), DATA_T_STRING, POD(&str), 0) != 0)
	    {
	    mssError(0, "PARAM", "Parameter '%s' must have a valid type", param->Name);
	    goto error;
	    }
	if (!strcmp(str,"object"))
	    t = DATA_T_STRING;
	else
	    t = objTypeID(str);
	if (t < 0)
	    {
	    mssError(1, "PARAM", "Invalid type '%s' for parameter '%s'", str, param->Name);
	    goto error;
	    }
	param->Value->DataType = t;

	/** Set up hints... OK if null **/
	param->Hints = objInfToHints(inf, t);

	return param;

    error:
	if (param)
	    paramFree(param);
	return NULL;
    }


/*** paramCreatefromObject() - using an open object, query for parameter
 *** settings and build a Param structure.
 ***/
pParam
paramCreateFromObject(pObject obj)
    {
    pParam param = NULL;
    char* str;
    int t;

	/** Allocate **/
	param = (pParam)nmMalloc(sizeof(Param));
	if (!param) goto error;

	/** Set up hints... OK if null **/
	param->Hints = hntObjToHints(obj);

	/** Allocate the typed obj data **/
	param->Value = ptodAllocate();
	if (!param->Value)
	    goto error;

	/** Get name **/
	if (objGetAttrValue(obj, "name", DATA_T_STRING, POD(&str)) != 0)
	    goto error;
	strtcpy(param->Name, str, sizeof(param->Name));

	/** Get the data type **/
	if (objGetAttrValue(obj, "type", DATA_T_STRING, POD(&str)) != 0)
	    {
	    mssError(0, "PARAM", "Parameter '%s' must have a valid type", param->Name);
	    goto error;
	    }
	if (!strcmp(str,"object"))
	    t = DATA_T_STRING;
	else
	    t = objTypeID(str);
	if (t < 0)
	    {
	    mssError(1, "PARAM", "Invalid type '%s' for parameter '%s'", str, param->Name);
	    goto error;
	    }
	param->Value->DataType = t;

	return param;

    error:
	if (param)
	    paramFree(param);
	return NULL;
    }


/*** paramSetValueDirect - set the value of a parameter with
 *** separate attr type/value instead of ptod
 ***/
int
paramSetValueDirect(pParam param, int datatype, pObjData value, int defer_hints, pParamObjects objlist, pObjSession sess)
    {
    pTObjData ptod;

	/** Set up ptod container **/
	ptod = ptodAllocate();
	if (!ptod)
	    return -1;
	ptod->DataType = datatype;
	objCopyData(value, &ptod->Data, datatype);
	if (value)
	    ptod->Flags &= ~DATA_TF_NULL;
	ptod->Flags |= DATA_TF_UNMANAGED;

	/** Copy **/
	paramSetValue(param, ptod, defer_hints, objlist, sess);

	/** Free **/
	ptodFree(ptod);

    return 0;
    }


/*** paramSetValue - set the value of a parameter
 ***/
int
paramSetValue(pParam param, pTObjData value, int defer_hints, pParamObjects objlist, pObjSession sess)
    {
    char* str;
    pTObjData rollback;

	/** Value supplied? **/
	if (value)
	    {
	    if (value->DataType == DATA_T_CODE)
		{
		/** We're not yet importing expression data **/
		param->Value->Flags |= DATA_TF_NULL;
		}
	    else if (value->DataType != param->Value->DataType && param->Value->DataType != DATA_T_UNAVAILABLE)
		{
		/** type mismatch? **/
		mssError(1,"PARAM","Type mismatch");
		return -1;
		}
	    else
		{
		if (defer_hints || !param->Hints)
		    {
		    /** Set it directly **/
		    ptodCopy(value, param->Value);
		    }
		else
		    {
		    /** Set it but save old value in case we need to roll back **/
		    rollback = ptodAllocate();
		    ptodCopy(param->Value, rollback);
		    ptodCopy(value, param->Value);

		    /** Check hints, and roll back the param update if the check fails. **/
		    if (hntVerifyHints(param->Hints, param->Value, &str, objlist, objlist?(objlist->Session):sess) < 0)
			{
			ptodCopy(rollback, param->Value);
			ptodFree(rollback);
			return -1;
			}
		    ptodFree(rollback);
		    }
		}
	    }
	else
	    param->Value->Flags |= DATA_TF_NULL;

    return 0;
    }


/*** paramSetValueFromInfNe - take a pStruct (stparse_ne) structure and pull
 *** the parameter value out of it, and set the parameter's value.
 ***/
int
paramSetValueFromInfNe(pParam param, pStruct inf, int defer_hints, pParamObjects objlist, pObjSession sess)
    {
    char* str;
    pTObjData ptod;

	/** No value?  Set NULL **/
	if (!inf)
	    {
	    param->Value->Flags |= DATA_TF_NULL;
	    return 0;
	    }

	/** Use value from inf **/
	str = NULL;
	stAttrValue_ne(inf, &str);
	if (str)
	    {
	    ptod = ptodAllocate();
	    ptod->DataType = param->Value->DataType;

	    /** Empty is NULL if datatype is non-string, otherwise there
	     ** is no good way to indicate an empty string vs a null
	     ** string, so it has to be handled via hints (strnull).
	     **/
	    if (ptod->DataType != DATA_T_STRING && !*str)
		{
		ptod->Flags |= DATA_TF_NULL;
		}
	    else
		{
		if (objDataFromStringAlloc(&(ptod->Data), param->Value->DataType, str) < 0)
		    goto error;
		ptod->Flags &= ~(DATA_TF_NULL);
		}
	    paramSetValue(param, ptod, defer_hints, objlist, sess);
	    ptodFree(ptod);
	    }
	else
	    {
	    /** Null **/
	    param->Value->Flags |= DATA_TF_NULL;
	    }

	return 0;

    error:
	mssError(1, "PARAM", "Parameter '%s' specified incorrectly", param->Name);
	return -1;
    }


/*** paramEvalHints - evaluate the presentation hints attached to the
 *** parameter, including setting a default value if the existing value
 *** is NULL.  The supplied object list is used in evaluating any
 *** expressions (min/max/default/etc) in the hints.
 ***/
int
paramEvalHints(pParam param, pParamObjects objlist, pObjSession sess)
    {
    char* str;

	/** set default value and/or verify that the given value is valid **/
	if (hntVerifyHints(param->Hints, param->Value, &str, objlist, objlist?(objlist->Session):sess) < 0)
	    {
	    mssError(1, "PARAM", "Parameter '%s' specified incorrectly: %s", param->Name, str);
	    return -1;
	    }

    return 0;
    }


/*** paramGetType - return the data type of the parameter
 **/
int
paramGetType(pParam param)
    {
    return param->Value->DataType;
    }


/** paramGetValueUntyped - obtain the value of the parameter.  Return value
 ** is -1 on error, 0 on success, 1 on null
 **/
int
paramGetValueUntyped(pParam param, pObjData value)
    {

	/** Null? **/
	if (param->Value->Flags & DATA_TF_NULL)
	    return 1;

    return objCopyData(&param->Value->Data, value, param->Value->DataType);
    }


/** paramGetValue - obtain the value of the parameter.  Return value
 ** is -1 on error, 0 on success, 1 on null
 **/
int
paramGetValue(pParam param, int datatype, pObjData value)
    {

	/** Null? **/
	if (param->Value->Flags & DATA_TF_NULL)
	    return 1;

	/** Implicit conversion? **/
	if (datatype == DATA_T_INTEGER && param->Value->DataType == DATA_T_STRING)
	    {
	    value->Integer = objDataToInteger(DATA_T_STRING, param->Value->Data.String, NULL);
	    return 0;
	    }
	else if (datatype == DATA_T_STRING && param->Value->DataType == DATA_T_INTEGER)
	    {
	    value->String = objDataToStringTmp(DATA_T_INTEGER, &param->Value->Data.Integer, 0);
	    return 0;
	    }

	/** Type mismatch? **/
	if (datatype != param->Value->DataType)
	    return -1;

    return paramGetValueUntyped(param, value);
    }

