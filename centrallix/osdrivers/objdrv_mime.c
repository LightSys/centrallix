#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#endif
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "stparse.h"
#include "st_node.h"
#include "centrallix.h"
#include "mime.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_mime.c						*/
/* Author:	Luke Ehresman <LME>					*/
/* Creation:	August 2, 2002						*/
/* Description:	MIME objectsystem driver.				*/
/*              Much of this drivers structure is based off of the      */
/*              MIME parser that Greg Beeley wrote as an extension to   */
/*              Elm in 1996.                                            */
/************************************************************************/



/* ***********************************************************************
** DEFINITONS                                                           **
** **********************************************************************/

/*** GLOBALS ***/

/*** Parsed message tree, shared by an open message and the parts fetched from it. ***/
typedef struct
    {
    pMimeHeader	Root;
    int		LinkCnt;
    }
    MimeTree, *pMimeTree;

/*** Structure used by this driver internally. ***/
typedef struct
    {
    pObject	Obj;
    int		Mask;
    char	Pathname[256];
    char*	AttrValue; /* GetAttrValue has to return a refence to memory that won't be free()ed */
    pMimeHeader	Header;
    pMimeTree	Tree;
    pMimeData	MimeDat;
    pXHashEntry	CurrAttr;
    pXHashEntry	CurrParam;
    char*	ParamAttrName; /* "<header>.<param>" name from GetNextAttr, valid until its next call */
    int		InternalSeek;
    int		InternalType;
    }
    MimeInfo, *pMimeInfo;

typedef struct
    {
    pMimeInfo	Data;
    int		ItemCnt;
    }
    MimeQuery, *pMimeQuery;

#define MIME_INTERNAL_MESSAGE    1
#define MIME_INTERNAL_ATTACHMENT 2

#define MIME(x) ((pMimeInfo)(x))

/* ***********************************************************************
** API FUNCTIONS                                                        **
** **********************************************************************/

/***
 ***  mimeOpen
 ***/
void*
mimeOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pLxSession lex = NULL;
    pMimeInfo inf;
    pMimeHeader msg;
    pMimeHeader phdr;
    char *node_path;
    char *nodeName;
    char *buffer;
    char *ptr;
    int i, size, foundMatch = 0;
    char nullbuf[1];

    /** Allocate and initialize the MIME structure **/
    inf = (pMimeInfo)nmMalloc(sizeof(MimeInfo));
    if (!inf) goto error;
    memset(inf,0,sizeof(MimeInfo));

    msg = libmime_AllocateHeader();
    if (!msg) goto error;

    /** Share the parsed tree with any parts fetched from this message. **/
    inf->Tree = (pMimeTree)nmMalloc(sizeof(MimeTree));
    if (!inf->Tree)
	{
	libmime_DeallocateHeader(msg);
	goto error;
	}
    inf->Tree->Root = msg;
    inf->Tree->LinkCnt = 1;

    /** Set object parameters **/
    inf->MimeDat = (pMimeData)nmMalloc(sizeof(MimeData));
    if (!inf->MimeDat) goto error;
    memset(inf->MimeDat,0,sizeof(MimeData));

    inf->MimeDat->Parent = obj->Prev;
    inf->MimeDat->ReadFn = objRead;
    inf->MimeDat->WriteFn = objWrite;
    inf->MimeDat->DecodedBuffer[0] = 0;
    inf->MimeDat->EncodedBuffer[0] = 0;
    inf->Header = msg;
    inf->Obj = obj;
    inf->Mask = mask;
    inf->InternalSeek = 0;
    inf->InternalType = MIME_INTERNAL_MESSAGE;

    lex = mlxGenericSession(obj->Prev, objRead, MLX_F_LINEONLY|MLX_F_NODISCARD|MLX_F_EOF);
    if (libmime_ParseHeader(lex, msg, 0, 0) < 0)
	{
	mssError(0, "MIME", "There was an error parsing message header in mimeOpen().");
	goto error;
	}
    if (libmime_ParseMultipartBody(lex, msg, msg->MsgSeekStart, msg->MsgSeekEnd) < 0)
	{
	mssError(0, "MIME", "There was an error parsing message body in mimeOpen().");
	goto error;
	}
    mlxCloseSession(lex);
    lex = NULL;

    /** Find and set the filename of the root node **/
    node_path = obj_internal_PathPart(obj->Pathname, obj->SubPtr - 1, 1);
    libmime_SetFilename(msg, node_path);

    /** assume we're only going to handle one level...		  **/
    /** no longer. It now works for multipart messages. HKJ & JRS **/
    obj->SubCnt=1;

    /** While we have a multipart message and there are more elements in the path,
     ** go through all elements and see if we have another multipart element.
     ** If so, repeat the search.
     **/
    while (obj->Pathname->nElements >= obj->SubPtr+obj->SubCnt)
	{
	/** assume we don't have a match **/
	foundMatch = 0;

	/** at least one more element of path to worry about **/
	ptr = obj_internal_PathPart(obj->Pathname, obj->SubPtr+obj->SubCnt-1, 1);
	for (i=0; i < xaCount(&(inf->Header->Parts)); i++)
	    {
	    phdr = xaGetItem(&(inf->Header->Parts), i);
	    if (!libmime_GetStringAttr(phdr, "Name", NULL, &nodeName) && !strcmp(nodeName, ptr))
		{
		/** FIXME FIXME FIXME FIXME
		 **  Memory lost, where did it go?  Nobody knows, and nobody can find out
		 ** FIXME FIXME FIXME FIXME
		 **/
		inf->Header = phdr;
		inf->InternalType = MIME_INTERNAL_MESSAGE;
		obj->SubCnt++;
		foundMatch = 1;
		break;
		}
	    }
	/** Break if there is no matching subpart **/
	if (!foundMatch) break;
	}

    /** Reset the file seek pointer. **/
    if (objSeek(obj->Prev, 0) < 0)
	{
	mssError(0, "MIME", "Improperly reset mime object file pointer.");
	goto error;
	}

    /** If dealing with the base mime file, check to see if it has been initialized (aka 'created'). **/
    if(objRead(obj->Prev, nullbuf, 1, 0, obj->Mode) > 0 &&
	    obj->Pathname->nElements == obj->SubPtr)
	{
	foundMatch = 1;
	}

    /** If CREAT, EXCL, and a match, error. **/
    if ((inf->Obj->Mode & O_CREAT) &&
	(inf->Obj->Mode & O_EXCL) &&
	(foundMatch))
	{
	/** Exclusive create is satisfied with a pre-filled root node. **/
	if (obj->Pathname->nElements != obj->SubPtr)
	    {
	    mssError(1, "MIME", "Mime object exists but create and exclusive flags are set. Cannot create mime object.");
	    goto error;
	    }
	}

    /** No match, error. **/
    if (!foundMatch)
	{
	if (inf->Obj->Mode & O_CREAT)
	    mssError(1, "MIME", "The MIME driver does not support creating objects.");
	else
	    mssError(1, "MIME", "Mime object not found.");
	goto error;
	}

    return (void*)inf;

    error:

	if (lex)
	    {
	    mlxCloseSession(lex);
	    }

	if (inf)
	    {
	    mimeClose(inf, NULL);
	    }
	return NULL;
    }


/***
 ***  mimeClose
 ***/
int
mimeClose(void* inf_v, pObjTrxTree* oxt)
    {
    pMimeInfo inf = MIME(inf_v);

    /** free any memory used to return an attribute **/
    if (inf->AttrValue)
	{
	nmSysFree(inf->AttrValue);
	inf->AttrValue=NULL;
	}
    if (inf->ParamAttrName)
	{
	nmSysFree(inf->ParamAttrName);
	inf->ParamAttrName = NULL;
	}

    if (inf->MimeDat)
	{
	nmFree(inf->MimeDat, sizeof(MimeData));
	}

    /** Reduce the link count and free the tree once nothing links to it. **/
    if (inf->Tree && --inf->Tree->LinkCnt == 0)
	{
	libmime_DeallocateHeader(inf->Tree->Root);
	nmFree(inf->Tree, sizeof(MimeTree));
	}

    if (inf)
	{
	nmFree(inf,sizeof(MimeInfo));
	}
    return 0;
    }


/***
 ***  mimeCreate
 ***/
int
mimeCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    mssError(1, "MIME", "The MIME driver does not support creating objects.");
    return -1;
    }


/***
 ***  mimeDelete
 ***/
int
mimeDelete(pObject obj, pObjTrxTree* oxt)
    {
    mssError(1, "MIME", "The MIME driver does not support deleting objects.");
    return -1;
    }


/***
 ***  mimeRead
 ***/
int
mimeRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    int size;
    int main_type;
    pMimeInfo inf = (pMimeInfo)inf_v;

    /** Check recursion **/
    if (thExcessiveRecursion())
	{
	mssError(1,"MIME","Could not read data: resource exhaustion occurred");
	return -1;
	}

    if (!libmime_GetIntAttr(inf->Header, "Content-Type", "ContentMainType", &main_type) && main_type == MIME_TYPE_MULTIPART)
	{
	return -1;
	}
    else
	{
	if (!offset && !inf->InternalSeek)
	    inf->InternalSeek = 0;
	else if (offset || (flags & FD_U_SEEK))
	    inf->InternalSeek = offset;
	size = libmime_PartRead(inf->MimeDat, inf->Header, buffer, maxcnt, inf->InternalSeek, FD_U_SEEK);
	if (size < 0)
	    return size;
	inf->InternalSeek += size;
	}

    return size;
    }


/***
 ***  mimeWrite
 ***/
int
mimeWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    mssError(1, "MIME", "The MIME driver does not support writing content.");
    return -1;
    }


/***
 ***  mimeOpenQuery
 ***/
void*
mimeOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pMimeQuery qy;
    pMimeInfo inf;

    inf = (pMimeInfo)inf_v;

    /** Don't open a query when there are no attachments **/
    if ( xaCount(&(inf->Header->Parts)) == 0)
	return NULL;

    qy = (pMimeQuery)nmMalloc(sizeof(MimeQuery));
    if (!qy) return NULL;
    memset(qy,0,sizeof(MimeQuery));

    qy->Data = inf;
    qy->ItemCnt = 0;

    return (void*)qy;
    }


/***
 ***  mimeQueryFetch
 ***/
void*
mimeQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pMimeInfo inf = NULL;
    pMimeQuery qy;

    qy = (pMimeQuery)qy_v;
    if (xaCount(&(qy->Data->Header->Parts))-1 < qy->ItemCnt)
	{
	return NULL;
	}

    /** Shouldn't this be taken care of by OSML??? **/
    obj->SubPtr = qy->Data->Obj->SubPtr;
    obj->SubCnt = qy->Data->Obj->SubCnt;

    inf = (pMimeInfo)nmMalloc(sizeof(MimeInfo));
    if (!inf) goto error;
    memset(inf,0,sizeof(MimeInfo));

    inf->MimeDat = (pMimeData)nmMalloc(sizeof(MimeData));
    if (!inf->MimeDat) goto error;
    memset(inf->MimeDat, 0, sizeof(MimeData));

    memcpy(inf->MimeDat, qy->Data->MimeDat, sizeof(MimeData));
    inf->Obj = obj;
    inf->Mask = mode;
    inf->Header = NULL;
    inf->InternalSeek = 0;
    inf->InternalType = MIME_INTERNAL_MESSAGE;

    inf->Header = xaGetItem(&(qy->Data->Header->Parts), qy->ItemCnt);
    inf->Tree = qy->Data->Tree;
    inf->Tree->LinkCnt++;
    qy->ItemCnt++;

    return (void*)inf;

    error:
	if (inf)
	    {
	    mimeClose(inf, NULL);
	    }

	return NULL;
    }


/***
 ***  mimeQueryClose
 ***/
int
mimeQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    nmFree(qy_v, sizeof(MimeQuery));
    return 0;
    }


/***
 ***  mimeGetAttrType
 ***
 ***  NOTE: If you want to query a parameter of an attribute,
 ***  use the syntax: <attr_name>.<param_name>
 ***/
int
mimeGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pMimeInfo inf = MIME(inf_v);
    pMimeAttr attr = NULL;
    pMimeParam param = NULL;
    char *local_attrname = NULL;
    char *attrName = NULL, *paramName = NULL;
    int rval = -1;

	/** For certain attributes, we defer to obj->Prev **/
	if (!strcmp(attrname, "envelope_from") || !strcmp(attrname, "envelope_to"))
	    {
	    rval = objGetAttrType(inf->Obj->Prev, attrname);
	    goto end;
	    }

	/** Create a local copy of the attrname parameter so we can modify it. **/
	local_attrname = nmSysStrdup(attrname);
	if (!local_attrname)
	    {
	    mssError(1, "MIME", "Could not allocate a copy of attribute name \"%s\".", attrname);
	    goto end;
	    }

	/** Split the given attribute name into attribute and parameter. **/
	libmime_GetAttrParamNames(local_attrname, &attrName, &paramName);

	/** Handle special attributes in the attribute list. **/
	if (!strcasecmp(attrName, "Content-Transfer-Encoding"))
	    {
	    rval = DATA_T_STRING;
	    goto end;
	    }

	/** The attribute wasn't readable. **/
	if (!attrName)
	    {
	    goto end;
	    }

	/** Get the indicated attribute. **/
	attr = (pMimeAttr)libmime_xhLookup(&inf->Header->Attrs, attrName);
	if (!attr)
	    {
	    rval = DATA_T_STRING;
	    }
	else
	    {
	    /** If no parameter was specified, return data about the attribute. **/
	    if (!paramName)
		{
		rval = attr->Ptod->DataType;
		}
	    else
		{
		/** Get the indicated parameter. **/
		param = libmime_GetMimeParam(inf->Header, attrName, paramName);
		if (!param)
		    {
		    rval = DATA_T_STRING;
		    }
		else
		    {
		    rval = param->Ptod->DataType;
		    }
		}
	    }

    end:
	if (local_attrname)
	    {
	    nmSysFree(local_attrname);
	    }

	return rval;
    }


/***
 ***  mimeGetAttrValue
 ***
 ***  NOTE: If you want to query a parameter of an attribute,
 ***  use the syntax: <attr_name>.<param_name>
 ***/
int
mimeGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pMimeInfo inf = MIME(inf_v);
    pMimeAttr attr = NULL;
    pMimeParam param = NULL;
    int int_attr = 0;
    char tmp[32];
    char *local_attrname = NULL;
    char *attrName = NULL, *paramName = NULL;
    int rval = -1;

	/** For certain attributes, we defer to obj->Prev **/
	if (!strcmp(attrname, "envelope_from") || !strcmp(attrname, "envelope_to"))
	    {
	    rval = objGetAttrValue(inf->Obj->Prev, attrname, datatype, val);
	    goto end;
	    }

	/** Create a local copy of the attrname parameter so we can modify it. **/
	local_attrname = nmSysStrdup(attrname);
	if (!local_attrname)
	    {
	    mssError(1, "MIME", "Could not allocate a copy of attribute name \"%s\".", attrname);
	    goto end;
	    }

	/** Deallocate the previous result if necessary. **/
	if (inf->AttrValue)
	    {
	    nmSysFree(inf->AttrValue);
	    inf->AttrValue = NULL;
	    }

	/** Handle special attributes. **/
	if (!strcasecmp(attrname, "Content-Transfer-Encoding"))
	    {
	    libmime_GetIntAttr(inf->Header, "Transfer-Encoding", NULL, &int_attr);
	    val->String = EncodingStrings[int_attr];
	    rval = 0;
	    goto end;
	    }

	/** Split the given attribute name into attribute and parameter. **/
	libmime_GetAttrParamNames(local_attrname, &attrName, &paramName);

	/** The attribute wasn't readable. **/
	if (!attrName)
	    {
	    goto end;
	    }

	/** Get the indicated attribute. **/
	attr = (pMimeAttr)libmime_xhLookup(&inf->Header->Attrs, attrName);
	if (!attr)
	    {
	    if (!strcmp(attrName, "annotation"))
		{
		val->String = "";
		rval = 0;
		}
	    else if (!strcmp(attrName, "name"))
		{
		rval = libmime_GetStringAttr(inf->Header, "Name", NULL, &val->String);
		}
	    else if (!strcmp(attrName, "outer_type"))
		{
		val->String = "message/rfc822";
		rval = 0;
		}
	    else if (!strcmp(attrName, "content_type") || !strcmp(attrName, "inner_type"))
		{
		rval = libmime_GetStringAttr(inf->Header, "Content-Type", NULL, &val->String);
		}
	    else
		{
		/** A missing header defaults to null. **/
		rval = 1;
		}
	    goto end;
	    }

	/** If no parameter was specified, return the attribute. **/
	if (!paramName)
	    {
	    /** Return the data stored in the attribute. **/
	    val->Generic = attr->Ptod->Data.Generic;
	    rval = 0;
	    goto end;
	    }

	/** Get the indicated parameter. **/
	param = libmime_GetMimeParam(inf->Header, attrName, paramName);
	if (!param)
	    {
	    /** A missing header defaults to null. **/
	    rval = 1;
	    goto end;
	    }

	/** Return the data stored in the parameter. **/
	val->Generic = param->Ptod->Data.Generic;
	rval = 0;

    end:
	if (local_attrname)
	    {
	    nmSysFree(local_attrname);
	    }

	return rval;
    }


/***
 ***  mimeGetNextAttr
 ***/
char*
mimeGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pMimeInfo inf = MIME(inf_v);
    pMimeAttr attr;
    pMimeParam param;
    char* attrName;
    int len;

	while (1)
	    {
	    /** List the current header's parameters as "<header>.<param>". **/
	    if (inf->CurrAttr)
		{
		attr = (pMimeAttr)inf->CurrAttr->Data;
		attrName = (strcmp(attr->Name, "Transfer-Encoding")) ? attr->Name : "Content-Transfer-Encoding";
		while (attr->Params.nRows && (inf->CurrParam = xhGetNextElement(&attr->Params, inf->CurrParam)))
		    {
		    param = (pMimeParam)inf->CurrParam->Data;

		    /** Skip parameters the parser derives from the header. **/
		    if (!strcmp(param->Name, "ContentMainType") || !strcmp(param->Name, "ContentSubType") ||
			    !strcmp(param->Name, "List") || !strcmp(param->Name, "Struct"))
			continue;

		    /** Build the name, which lasts until the next call. **/
		    if (inf->ParamAttrName)
			nmSysFree(inf->ParamAttrName);
		    len = strlen(attrName) + strlen(param->Name) + 2;
		    inf->ParamAttrName = (char*)nmSysMalloc(len);
		    if (!inf->ParamAttrName)
			{
			mssError(1, "MIME", "Could not allocate the name of parameter \"%s\" of \"%s\".", param->Name, attrName);
			return NULL;
			}
		    snprintf(inf->ParamAttrName, len, "%s.%s", attrName, param->Name);
		    return inf->ParamAttrName;
		    }
		}

	    /** Move to the next header. **/
	    inf->CurrAttr = xhGetNextElement(&inf->Header->Attrs, inf->CurrAttr);
	    inf->CurrParam = NULL;
	    if (!inf->CurrAttr)
		return NULL;
	    attr = (pMimeAttr)inf->CurrAttr->Data;

	    /** Name and Content-Type are system attributes, so only Content-Type's parameters are listed. **/
	    if (!strcasecmp(attr->Name, "Name") || !strcasecmp(attr->Name, "Content-Type"))
		continue;

	    return (strcmp(attr->Name, "Transfer-Encoding")) ? attr->Name : "Content-Transfer-Encoding";
	    }
    }


/***
 ***  mimeGetFirstAttr
 ***/
char*
mimeGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pMimeInfo inf = MIME(inf_v);
    pMimeAttr attr;

	/** Set up to get the first element in the attribute list. **/
	inf->CurrAttr = NULL;
	inf->CurrParam = NULL;

    return mimeGetNextAttr(inf_v, oxt);
    }


/***
 ***  mimeSetAttrValue
 ***/
int
mimeSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    mssError(1, "MIME", "The MIME driver does not support setting attributes.");
    return -1;
    }


/***
 ***  mimeAddAttr
 ***/
int
mimeAddAttr(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    mssError(1, "MIME", "The MIME driver does not support adding attributes.");
    return -1;
    }


/***
 ***  mimeOpenAttr
 ***/
void*
mimeOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/***
 ***  mimeGetFirstMethod
 ***/
char*
mimeGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/***
 ***  mimeGetNextMethod
 ***/
char*
mimeGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/***
 ***  mimeExecuteMethod
 ***/
int
mimeExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }

/***
 *** mimeInfo - Return the capabilities of the object
 ***/
int
mimeInfo(void* inf_v, pObjectInfo info)
    {
    pMimeInfo inf = MIME(inf_v);
    int main_type;

	info->Flags |= ( OBJ_INFO_F_CANT_ADD_ATTR | OBJ_INFO_F_CANT_SEEK );
	if (!libmime_GetIntAttr(inf->Header, "Content-Type", "ContentMainType", &main_type) && main_type == MIME_TYPE_MULTIPART)
	    {
	    info->Flags |= ( OBJ_INFO_F_HAS_SUBOBJ | OBJ_INFO_F_CAN_HAVE_SUBOBJ | OBJ_INFO_F_SUBOBJ_CNT_KNOWN |
		OBJ_INFO_F_CANT_HAVE_CONTENT | OBJ_INFO_F_NO_CONTENT );
	    info->nSubobjects = xaCount(&(inf->Header->Parts));
	    }
	else
	    {
	    info->Flags |= ( OBJ_INFO_F_NO_SUBOBJ | OBJ_INFO_F_CANT_HAVE_SUBOBJ | OBJ_INFO_F_CAN_HAVE_CONTENT |
		OBJ_INFO_F_HAS_CONTENT );
	    }

	return 0;
    }


/***
 ***  mimeInitialize
 ***/
int
mimeInitialize()
    {
    pObjDriver drv;

    drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
    if (!drv) return -1;
    memset(drv, 0, sizeof(ObjDriver));

    /** Setup the function references. **/
    drv->Open = mimeOpen;
    drv->Close = mimeClose;
    drv->Create = mimeCreate;
    drv->Delete = mimeDelete;
    drv->OpenQuery = mimeOpenQuery;
    drv->QueryDelete = NULL;
    drv->QueryFetch = mimeQueryFetch;
    drv->QueryClose = mimeQueryClose;
    drv->Read = mimeRead;
    drv->Write = mimeWrite;
    drv->GetAttrType = mimeGetAttrType;
    drv->GetAttrValue = mimeGetAttrValue;
    drv->GetFirstAttr = mimeGetFirstAttr;
    drv->GetNextAttr = mimeGetNextAttr;
    drv->SetAttrValue = mimeSetAttrValue;
    drv->AddAttr = mimeAddAttr;
    drv->OpenAttr = mimeOpenAttr;
    drv->GetFirstMethod = mimeGetFirstMethod;
    drv->GetNextMethod = mimeGetNextMethod;
    drv->ExecuteMethod = mimeExecuteMethod;
    drv->Info = mimeInfo;

    strcpy(drv->Name, "MIME - MIME Parsing Driver");
    drv->Capabilities = 0;
    xaInit(&(drv->RootContentTypes), 16);
    xaAddItem(&(drv->RootContentTypes), "message/rfc822");
    xaAddItem(&(drv->RootContentTypes), "multipart/mixed");
    xaAddItem(&(drv->RootContentTypes), "multipart/alternative");
    xaAddItem(&(drv->RootContentTypes), "multipart/form-data");
    xaAddItem(&(drv->RootContentTypes), "multipart/parallel");
    xaAddItem(&(drv->RootContentTypes), "multipart/digest");

    if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

MODULE_INIT(mimeInitialize);
MODULE_PREFIX("mime");
MODULE_DESC("MIME ObjectSystem Driver");
MODULE_VERSION(0,1,0);
MODULE_IFACE(CX_CURRENT_IFACE);
