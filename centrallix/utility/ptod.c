#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "cxlib/datatypes.h"
#include "cxlib/newmalloc.h"
#include "ptod.h"
#include "obj.h"


/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2004 LightSys Technology Services, Inc.		*/
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
/* Module:	ptod.h,ptod.c                                           */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	May 1, 2004                                             */
/*									*/
/* Description:	This module defines the PTOD atom data type as well as	*/
/*		routines for managing this very basic component within	*/
/*		centrallix.						*/
/************************************************************************/



/*** ptodAllocate() - create a new, uninitialized PTOD object without any
 *** storage for pointed-to data (like money, datetime, etc.).
 ***/
pTObjData
ptodAllocate()
    {
    pTObjData ptod;

	/** Allocate the memory **/
	ptod = (pTObjData)nmMalloc(sizeof(TObjData));

	/** Init data type, etc. **/
	ptod->Data.Generic = NULL;
	ptod->DataType = DATA_T_UNAVAILABLE;
	ptod->Flags = DATA_TF_NULL;
	ptod->LinkCnt = 1;
	ptod->AttachedLen = 0;

    return ptod;
    }


/*** ptod_internal_FreeData - release data value (ObjData) from ptod, if
 *** the ptod is managed.
 ***/
int
ptod_internal_FreeData(pTObjData ptod)
    {

	/** Managed? free data memory as well if so **/
	if (!(ptod->Flags & DATA_TF_UNMANAGED) && !(ptod->Flags & DATA_TF_ATTACHED))
	    {
	    switch(ptod->DataType)
		{
		case DATA_T_STRING:
		    if (ptod->Data.String) nmSysFree(ptod->Data.String);
		    break;
		case DATA_T_BINARY:
		    if (ptod->Data.Binary.Data) nmSysFree(ptod->Data.Binary.Data);
		    break;
		case DATA_T_MONEY:
		    if (ptod->Data.Money) nmFree(ptod->Data.Money, sizeof(MoneyType));
		    break;
		case DATA_T_DATETIME:
		    if (ptod->Data.DateTime) nmFree(ptod->Data.DateTime, sizeof(DateTime));
		    break;
		case DATA_T_STRINGVEC:
		    /** FIXME - how do we want to handle managed intvec/stringvec? **/
		    if (ptod->Data.StringVec) nmFree(ptod->Data.StringVec, sizeof(StringVec));
		    break;
		case DATA_T_INTVEC:
		    if (ptod->Data.IntVec) nmFree(ptod->Data.IntVec, sizeof(IntVec));
		    break;
		default:
		    break;
		}
	    }
	ptod->Data.Generic = NULL;

    return 0;
    }


/*** ptodFree() - unlink from the PTOD object, and if the link count goes
 *** to zero, free the memory.  If it was marked as being managed, also
 *** free memory for the pointed-to data itself.
 ***/
int
ptodFree(pTObjData ptod)
    {

	/** Link count to 0? **/
	if ((--ptod->LinkCnt) != 0) return 0;

	/** Free data memory **/
	ptod_internal_FreeData(ptod);

	/** Ok, now free the ptod itself **/
	if (ptod->Flags & DATA_TF_ATTACHED)
	    nmSysFree(ptod);
	else
	    nmFree(ptod, sizeof(TObjData));

    return 0;
    }


/*** ptodLink() - link to a ptod object so that it doesn't get released until
 *** all references are removed
 ***/
pTObjData
ptodLink(pTObjData ptod)
    {

	/** link **/
	ptod->LinkCnt++;

    return ptod;
    }


/*** ptodCopy() - copy a ptod object.  The unmanaged/attached settings on the
 *** destination ptod control how the destination looks after the copy; thus
 *** the caller should set those flags as desired before doing the copy.  If
 *** the destination ptod is unmanaged, the data pointers will point where the
 *** source ptod points, and it is up to the application to keep the source
 *** data around until the destination ptod is destroyed or made independent.
 ***
 *** If the destination is attached, and its attached length is insufficient,
 *** an error (-1) is returned.  AttachedLen must be valid in the destination
 *** prior to calling this function, if the dst is marked as attached.
 ***/
int
ptodCopy(pTObjData src, pTObjData dst)
    {

	/** Copy basic info **/
	dst->DataType = src->DataType;
	dst->Flags &= ~DATA_TF_NULL;
	dst->Flags |= (src->Flags & DATA_TF_NULL);

	/** Free old destination data (if needed) **/
	ptod_internal_FreeData(dst);

	/** Copy data, based on managed/attached status of ptod **/
	if (!(src->Flags & DATA_TF_NULL))
	    {
	    if (dst->Flags & DATA_TF_UNMANAGED)
		{
		/** unmanaged/unattached copy? **/
		objCopyData(&src->Data, &dst->Data, src->DataType);
		}
	    else if (dst->Flags & DATA_TF_ATTACHED)
		{
		/** Attached copy? **/
		if (dst->AttachedLen < src->AttachedLen)
		    {
		    mssError(1,"PTOD","Could not copy: destination size %d is less than source size %d.",dst->AttachedLen,src->AttachedLen);
		    return -1;
		    }
		if (src->AttachedLen)
		    memcpy(((char*)dst)+sizeof(TObjData), ((char*)src)+sizeof(TObjData), src->AttachedLen);
		switch(src->DataType)
		    {
		    case DATA_T_INTEGER:
		    case DATA_T_DOUBLE:
			objCopyData(&src->Data, &dst->Data, src->DataType);
			break;
		    case DATA_T_STRING:
		    case DATA_T_MONEY:
		    case DATA_T_DATETIME:
		    case DATA_T_INTVEC:
		    case DATA_T_STRINGVEC:
		    case DATA_T_BINARY:
		    default:
			dst->Data.Generic = ((char*)dst)+sizeof(TObjData);
			break;
		    }
		}
	    else
		{
		/** Managed copy **/
		switch(src->DataType)
		    {
		    case DATA_T_INTEGER:
		    case DATA_T_DOUBLE:
			objCopyData(&src->Data, &dst->Data, src->DataType);
			break;
		    case DATA_T_STRING:
			dst->Data.String = nmSysStrdup(src->Data.String);
			break;
		    case DATA_T_BINARY:
			dst->Data.Binary.Data = nmSysMalloc(src->Data.Binary.Size+1);
			dst->Data.Binary.Size = src->Data.Binary.Size;
			memcpy(dst->Data.Binary.Data, src->Data.Binary.Data, src->Data.Binary.Size+1);
			break;
		    case DATA_T_MONEY:
			dst->Data.Money = nmMalloc(sizeof(MoneyType));
			memcpy(dst->Data.Money, src->Data.Money, sizeof(MoneyType));
			break;
		    case DATA_T_DATETIME:
			dst->Data.DateTime = nmMalloc(sizeof(DateTime));
			memcpy(dst->Data.DateTime, src->Data.DateTime, sizeof(DateTime));
			break;
		    default:
			/** FIXME: how do we want to handle intvec/stringvec/binary/etc? **/
			break;
		    }
		}
	    }

    return 0;
    }


/*** ptodDuplicate() - duplicate a ptod object by allocating a new one.  Flags
 *** indicate UNMANAGED/ATTACHED status.
 ***/
pTObjData
ptodDuplicate(pTObjData ptod, int flags)
    {
	return NULL;
    }


/*** ptodPrint() - do a debugging print of the ptod data value to standard
 *** output.
 ***/
int
ptodPrint(pTObjData ptod)
    {
    char* type_names[] = { "(unknown)", "integer", "string", "double", "datetime", "intvec", "stringvec", "money", "array", "code", "binary" };
    int t;

	/** type **/
	t = ptod->DataType;
	if (t < 0 || t > sizeof(type_names)/sizeof(char*)) t = 0;

	printf("PTOD: type %s, ", type_names[t]);
	if (ptod->Flags & DATA_TF_UNASSURED) printf("unassured, ");
	if (ptod->Flags & DATA_TF_UNMANAGED) printf("unmanaged, ");
	if (ptod->Flags & DATA_TF_ATTACHED) printf("attached, ");
	if (ptod->Flags & DATA_TF_NULL) 
	    {
	    printf("null\n");
	    }
	else
	    {
	    if (t == DATA_T_INTEGER || t == DATA_T_DOUBLE)
		printf("%s\n", (char*)objDataToStringTmp(t, (void*)&(ptod->Data), 0));
	    else
		printf("%s\n", (char*)objDataToStringTmp(t, (void*)(ptod->Data.Generic), 0));
	    }

    return 0;
    }


/*** ptodToStringTmp() - Convert a ptod to string form.
 ***/
char*
ptodToStringTmp(pTObjData ptod)
    {
    char* str;

	/** Convert to string **/
	if (ptod->Flags & DATA_TF_NULL || ptod->DataType < 0 || ptod->DataType == DATA_T_UNAVAILABLE)
	    return "";
	else if (ptod->DataType == DATA_T_INTEGER || ptod->DataType == DATA_T_DOUBLE)
	    return (char*)objDataToStringTmp(ptod->DataType, (void*)&(ptod->Data), 0);
	else
	    return (char*)objDataToStringTmp(ptod->DataType, (void*)(ptod->Data.Generic), 0);

    }


int 
ptodTypeOf(pTObjData ptod)
    {
    return ptod->DataType;
    }

