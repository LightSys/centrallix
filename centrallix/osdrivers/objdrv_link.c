#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "cxlib/mtsession.h"
#include "cxlib/util.h"
/** module definintions **/
#include "centrallix.h"

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
/* Module: 	objdrv_link.c						*/
/* Author:	Greg Beeley (GRB)      					*/
/* Creation:	07-Mar-2011            					*/
/* Description:	Symbolic link within the ObjectSystem.  The .lnk file	*/
/* itself contains a pathname to another object in the ObjectSystem.	*/
/*									*/
/*			:1,$s/Lnk/Pop/g					*/
/*			:1,$s/LNK/POP/g					*/
/*			:1,$s/lnk/pop/g					*/
/************************************************************************/


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[OBJSYS_MAX_PATH + 1];
    pObject	Obj;
    char	LinkPathname[OBJSYS_MAX_PATH + 2];
    char	RefPathname[OBJSYS_MAX_PATH + 2];
    pObject	LLObj;		/* underlying object that is pointed to */
    }
    LnkData, *pLnkData;


#define LNK(x) ((pLnkData)(x))


/*** GLOBALS ***/
struct
    {
    pObjDriver	ThisDriver;
    }
    LNK_INF;


/*** lnkOpen - open an object.
 ***/
void*
lnkOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pLnkData inf = NULL;
    int rval;
    char* ptr;
    char* ourpath;

	/** Allocate the structure **/
	inf = (pLnkData)nmMalloc(sizeof(LnkData));
	if (!inf)
	    goto error;
	memset(inf,0,sizeof(LnkData));
	inf->Obj = obj;

	/** Read in the link pathname.  We read one more char than spec
	 ** requires, so we can tell if the path is too long without having
	 ** to do a second objRead().
	 **/
	if ((rval = objRead(obj->Prev, inf->LinkPathname, OBJSYS_MAX_PATH+1, 0, OBJ_U_SEEK)) < 1)
	    {
	    /** Returned -1 or 0.  No can do. **/
	    mssError(1,"LNK","Could not read link reference");
	    goto error;
	    }
	inf->LinkPathname[rval] = '\0';

	/** Next, do some sanity checks **/
	if (verifyUTF8(inf->LinkPathname) != UTIL_VALID_CHAR)
	    {
	    /** contains invalid character **/
	    mssError(1,"LNK","Malformed symbolic link - contains an invalid or partial character");
	    goto error;
	    }
	if (rval != strlen(inf->LinkPathname))
	    {
	    /** Whoops - contains a \0.  Bad. **/
	    mssError(1,"LNK","Malformed symbolic link - contains a NUL");
	    goto error;
	    }
	if ((ptr = strpbrk(inf->LinkPathname, "\r\n")) != NULL)
	    {
	    /** Strip \n or \r\n off of end **/
	    if (!strcmp(ptr, "\n") || !strcmp(ptr, "\r\n"))
		{
		ptr[0] = '\0';
		}
	    else
		{
		/** Whoops - unexpected place for the \n **/
		mssError(1,"LNK","Malformed symbolic link - bad CR/NL sequence");
		goto error;
		}
	    }
	if (rval == sizeof(inf->LinkPathname)-1)
	    {
	    /** Whoops - path is too long **/
	    mssError(1,"LNK","Link reference too long");
	    goto error;
	    }

	/** We have to force open as a simple object, and not allow the
	 ** OSML to cascade the open to other drivers.  We need the OSML
	 ** to cascade the open using *this* chain of drivers.
	 **
	 ** Also: prepend the path leading up to the .lnk object itself,
	 ** if the link path is not absolute.
	 **/
	if (inf->LinkPathname[0] != '/')
	    ourpath = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr - 1);
	else
	    ourpath = "";

	rval = snprintf(inf->RefPathname, sizeof(inf->RefPathname), "%s/%s?ls__type=system%%2fobject", ourpath, inf->LinkPathname);
	if (rval <= 0 || rval > sizeof(inf->RefPathname) - 1)
	    {
	    /** Whoops - combined path too long. **/
	    mssError(1,"LNK","Link reference too long");
	    goto error;
	    }

	/** Try to open the path. **/
	inf->LLObj = objOpen(obj->Session, inf->RefPathname, obj->Mode, mask, usrtype);
	if (inf->LLObj == NULL)
	    {
	    mssError(0,"LNK","Could not open link reference");
	    goto error;
	    }
	objUnmanageObject(obj->Session, inf->LLObj);

	/** SubCnt is 1.  That is because all we handle is the filename.lnk
	 ** object itself.
	 **/
	obj->SubCnt = 1;

	return (void*)inf;

    error:
	if (inf)
	    {
	    if (inf->LLObj)
		objClose(inf->LLObj);
	    inf->LLObj = NULL;
	    nmFree(inf, sizeof(LnkData));
	    inf = NULL;
	    }
	return NULL;
    }


/*** lnkClose - close an open object.
 ***/
int
lnkClose(void* inf_v, pObjTrxTree* oxt)
    {
    pLnkData inf = LNK(inf_v);

	/** Close underlying object. **/
	if (inf->LLObj)
	    objClose(inf->LLObj);
	inf->LLObj = NULL;

	/** Release the memory **/
	nmFree(inf,sizeof(LnkData));

    return 0;
    }


/*** lnkCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
lnkCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = lnkOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	lnkClose(inf, oxt);

    return 0;
    }


/*** lnkDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
lnkDelete(pObject obj, pObjTrxTree* oxt)
    {
    /** not supported **/
    return -1;
    }


/*** lnkRead - Structure elements have no content.  Fails.
 ***/
int
lnkRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pLnkData inf = LNK(inf_v);
    return objRead(inf->LLObj, buffer, maxcnt, offset, flags);
    }


/*** lnkWrite - Again, no content.  This fails.
 ***/
int
lnkWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pLnkData inf = LNK(inf_v);
    return objWrite(inf->LLObj, buffer, cnt, offset, flags);
    }


/*** lnkOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
lnkOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** lnkQueryFetch - get the next directory entry as an open object.
 ***/
void*
lnkQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** lnkQueryClose - close the query.
 ***/
int
lnkQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** lnkGetAttrType - get the type (DATA_T_lnk) of an attribute by name.
 ***/
int
lnkGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pLnkData inf = LNK(inf_v);
    return objGetAttrType(inf->LLObj, attrname);
    }


/*** lnkGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
lnkGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pLnkData inf = LNK(inf_v);
    return objGetAttrValue(inf->LLObj, attrname, datatype, val);
    }


/*** lnkGetNextAttr - get the next attribute name for this object.
 ***/
char*
lnkGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pLnkData inf = LNK(inf_v);
    return objGetNextAttr(inf->LLObj);
    }


/*** lnkGetFirstAttr - get the first attribute name for this object.
 ***/
char*
lnkGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pLnkData inf = LNK(inf_v);
    return objGetFirstAttr(inf->LLObj);
    }


/*** lnkSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
lnkSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pLnkData inf = LNK(inf_v);
    return objSetAttrValue(inf->LLObj, attrname, datatype, val);
    }


/*** lnkAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
lnkAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt)
    {
    pLnkData inf = LNK(inf_v);
    return objAddAttr(inf->LLObj, attrname, type, val);
    }


/*** lnkOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
lnkOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    pLnkData inf = LNK(inf_v);
    return objOpenAttr(inf->LLObj, attrname, mode);
    }


/*** lnkGetFirstMethod -- return name of First method available on the object.
 ***/
char*
lnkGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    pLnkData inf = LNK(inf_v);
    return objGetFirstMethod(inf->LLObj);
    }


/*** lnkGetNextMethod -- return successive names of methods after the First one.
 ***/
char*
lnkGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    pLnkData inf = LNK(inf_v);
    return objGetNextMethod(inf->LLObj);
    }


/*** lnkExecuteMethod - Execute a method, by name.
 ***/
int
lnkExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    pLnkData inf = LNK(inf_v);
    return objExecuteMethod(inf->LLObj, methodname, param);
    }


/*** lnkPresentationHints - Return a structure containing "presentation hints"
 *** data, which is basically metadata about a particular attribute, which
 *** can include information which relates to the visual representation of
 *** the data on the client.
 ***/
pObjPresentationHints
lnkPresentationHints(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pLnkData inf = LNK(inf_v);
    return objPresentationHints(inf->LLObj, attrname);
    }


/*** lnkInfo - return object metadata - about the object, not about a 
 *** particular attribute.
 ***/
int
lnkInfo(void* inf_v, pObjectInfo info_struct)
    {
    pLnkData inf = LNK(inf_v);
    pObjectInfo drv_info;
    drv_info = objInfo(inf->LLObj);
    if (!drv_info) return -1;
    memcpy(info_struct, drv_info, sizeof(ObjectInfo));
    return 0;
    }


/*** lnkCommit - commit any changes made to the underlying data source.
 ***/
int
lnkCommit(void* inf_v, pObjTrxTree* oxt)
    {
    /*pLnkData inf = LNK(inf_v);*/
    return 0;
    }


/*** lnkInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
lnkInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&LNK_INF,0,sizeof(LNK_INF));
	LNK_INF.ThisDriver = drv;

	/** Setup the structure **/
	strcpy(drv->Name,"LNK - Symbolic Link");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"system/symbolic-link");

	/** Setup the function references. **/
	drv->Open = lnkOpen;
	drv->Close = lnkClose;
	drv->Create = lnkCreate;
	drv->Delete = lnkDelete;
	drv->OpenQuery = lnkOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = lnkQueryFetch;
	drv->QueryClose = lnkQueryClose;
	drv->Read = lnkRead;
	drv->Write = lnkWrite;
	drv->GetAttrType = lnkGetAttrType;
	drv->GetAttrValue = lnkGetAttrValue;
	drv->GetFirstAttr = lnkGetFirstAttr;
	drv->GetNextAttr = lnkGetNextAttr;
	drv->SetAttrValue = lnkSetAttrValue;
	drv->AddAttr = lnkAddAttr;
	drv->OpenAttr = lnkOpenAttr;
	drv->GetFirstMethod = lnkGetFirstMethod;
	drv->GetNextMethod = lnkGetNextMethod;
	drv->ExecuteMethod = lnkExecuteMethod;
	drv->PresentationHints = lnkPresentationHints;
	drv->Info = lnkInfo;
	drv->Commit = lnkCommit;

	nmRegister(sizeof(LnkData),"LnkData");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

