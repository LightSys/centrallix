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
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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

/*** Structure used by this driver internally. ***/
typedef struct
    {
    pObject	Obj;
    int		Mask;
    char	Pathname[256];
    char*	AttrValue; /* GetAttrValue has to return a refence to memory that won't be free()ed */
    pMimeHeader	Header;
    pMimeHeader	MessageRoot;
    pMimeData	MimeDat;
    pXHashEntry	CurrAttr;
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
    char *node_name;
    char *buffer;
    char *ptr;
    int i, size, found_match = 0;
    char nullbuf[1];

    /** Allocate and initialize the MIME structure **/
    inf = (pMimeInfo)nmMalloc(sizeof(MimeInfo));
    if (!inf) goto error;
    memset(inf,0,sizeof(MimeInfo));

    msg = libmime_AllocateHeader();
    if (!msg) goto error;

    /** Set object parameters **/
    inf->MimeDat = (pMimeData)nmMalloc(sizeof(MimeData));
    if (!inf->MimeDat) goto error;
    memset(inf->MimeDat,0,sizeof(MimeData));

    inf->MimeDat->Parent = obj->Prev;
    inf->MimeDat->ReadFn = objRead;
    inf->MimeDat->WriteFn = objWrite;
    inf->MimeDat->Buffer[0] = 0;
    inf->MimeDat->EncBuffer[0] = 0;
    inf->MessageRoot = msg;
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
    node_path = obj_internal_PathPart(obj->Pathname, obj->Pathname->nElements-1, 1);
    libmime_SetFilename(msg, node_path);

    /** assume we're only going to handle one level...		  **/
    /** no longer. It now works for multipart messages. HKJ & JRS **/
    obj->SubCnt=1;

    /** While we have a multipart message and there are more elements in the path,
     ** go through all elements and see if we have another multipart element.
     ** If so, repeat the search.
     **/
    while (!libmime_GetIntAttr(inf->Header, "Content-Type", "ContentMainType", &i) &&
	    i == MIME_TYPE_MULTIPART &&
	    obj->Pathname->nElements >= obj->SubPtr+obj->SubCnt)
	{
	/** assume we don't have a match **/
	found_match = 0;

	/** at least one more element of path to worry about **/
	ptr = obj_internal_PathPart(obj->Pathname, obj->SubPtr+obj->SubCnt-1, 1);
	for (i=0; i < xaCount(&(inf->Header->Parts)); i++)
	    {
	    phdr = xaGetItem(&(inf->Header->Parts), i);
	    if (!libmime_GetStringAttr(phdr, "Name", NULL, &node_name) && !strcmp(node_name, ptr))
		{
		/** FIXME FIXME FIXME FIXME
		 **  Memory lost, where did it go?  Nobody knows, and nobody can find out
		 ** FIXME FIXME FIXME FIXME
		 **/
		inf->Header = phdr;
		inf->InternalType = MIME_INTERNAL_MESSAGE;
		obj->SubCnt++;
		found_match = 1;
		break;
		}
	    }
	/** Break if there is no matching subpart **/
	if (!found_match) break;
	}

    /** Reset the file seek pointer. **/
    if (objRead(obj->Prev, nullbuf, 0, 0, FD_U_SEEK) < 0)
	{
	mssErrorErrno(0, "MIME", "Improperly reset mime object file pointer.");
	goto error;
	}

    /** If dealing with the base mime file, check to see if it has been initialized (aka 'created'). **/
    if(objRead(obj->Prev, nullbuf, 1, 0, obj->Mode) > 0)
	{
	found_match = 1;
	}

    /** If CREAT, EXCL, and a match, error. **/
    if ((inf->Obj->Mode & O_CREAT) &&
	(inf->Obj->Mode & O_EXCL) &&
	(found_match))
	{
	mssError(1, "MIME", "Mime object exists but create and exclusive flags are set. Cannot create mime object.");
	goto error;
	}


    /** If not CREAT and match, error **/
    if (!(inf->Obj->Mode & O_CREAT) &&
	(!found_match))
	{
	mssError(1, "MIME", "Mime object not found but create flag not set.");
	goto error;
	}

    /** CREAT and no match... create the file! **/
    if ((inf->Obj->Mode & O_CREAT) &&
	(!found_match))
	{
	if (mimeCreate(obj, mask, systype, usrtype, oxt))
	    {
	    mssError(0, "MIME", "Could not create new mime object.");
	    goto error;
	    }

	/** Deallocate anything from this go-round before trying again. **/
	mimeClose(inf, oxt);

	/** Open the object we just created, super-ensuring we don't make it again. **/
	inf = mimeOpen(obj, mask & ~O_CREAT, systype, usrtype, oxt);
	if (!inf)
	    {
	    mssError(0, "MIME", "Failed to open newly created mime object.");
	    goto error;
	    }
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

    if (inf->MimeDat)
	{
	nmFree(inf->MimeDat, sizeof(MimeData));
	}

    /** probably needs to be more done here, but I have _no_ clue what's going on :) -- JDR **/
    /** Making this do stuff, but may still need more work. Justin Southworth and Hazen Johnson **/
    if (inf->MessageRoot)
	{
	libmime_DeallocateHeader(inf->MessageRoot);
	}

    if (inf)
	{
	nmFree(inf,sizeof(MimeInfo));
	}
    return 0;
    }


/***
 ***  mimeCreate - Create a new mime object.
 ***/
int
mimeCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    XString initialContents;

	xsInit(&initialContents);

	/** Hardcode default values for new mime object. **/
	xsConcatenate(&initialContents, "Mime-Version: 1.0\n", -1);
	xsConcatenate(&initialContents, "Content-Type: text/plain\n\n", -1);

	/** Creating a new mime file. **/
	if (obj->SubPtr == obj->Pathname->nElements)
	    {
	    if (objWrite(obj->Prev, initialContents.String, initialContents.Length, 0, obj->Mode) < 0)
		{
		mssError(0, "MIME", "Could not write to new mime object.");
		goto error;
		}
	    }
	/** Adding a subobject to an existing mime file. **/
	else
	    {
	    xsCopy(&initialContents, "Hi, Jonathan.", 13);
	    }

	xsDeInit(&initialContents);
    return 0;

    error:
	xsDeInit(&initialContents);
	return -1;
    }


/***
 ***  mimeDelete
 ***/
int
mimeDelete(pObject obj, pObjTrxTree* oxt)
    {
    return 0;
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
	size = libmime_PartRead(inf->MimeDat, inf->Header, buffer, maxcnt, inf->InternalSeek, 0);
	inf->InternalSeek += size;
	return size;
	}
    }


/***
 ***  mimeWrite
 ***/
int
mimeWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    return 0;
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

	/** Create a local copy of the attrname parameter so we can modify it. **/
	local_attrname = nmSysStrdup(attrname);

	/** Split the given attribute name into attribute and parameter. **/
	libmime_GetAttrParamNames(local_attrname, &attrName, &paramName);

	/** Handle special attributes in the attribute list. **/
	if (!strcmp(attrName, "Transfer-Encoding")) return DATA_T_STRING;

	/** The attribute wasn't readable. **/
	if (!attrName)
	    {
	    goto error;
	    }

	/** Get the indicated attribute. **/
	attr = (pMimeAttr)libmime_xhLookup(&inf->Header->Attrs, attrName);
	if (!attr)
	    {
	    if (!strcmp(attrname, "name")) return DATA_T_STRING;
	    if (!strcmp(attrname, "content_type")) return DATA_T_STRING;
	    if (!strcmp(attrname, "annotation")) return DATA_T_STRING;
	    if (!strcmp(attrname, "inner_type")) return DATA_T_STRING;
	    if (!strcmp(attrname, "outer_type")) return DATA_T_STRING;

	    return DATA_T_STRING;
	    }

	/** If no parameter was specified, return data about the attribute. **/
	if (!paramName)
	    {
	    return attr->Ptod->DataType;
	    }

	/** Get the indicated parameter. **/
	param = (pMimeParam)libmime_xhLookup(&attr->Params, paramName);
	if (!param)
	    {
	    goto error;
	    }

	/** Free the local copy of attrname. **/
	nmSysFree(local_attrname);

    /** Return the apropriate data type. **/
    return param->Ptod->DataType;

    error:
	if (local_attrname)
	    {
	    nmSysFree(local_attrname);
	    }

	return -1;
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

	/** Create a local copy of the attrname parameter so we can modify it. **/
	local_attrname = nmSysStrdup(attrname);

	/** Deallocate the previous result if necessary. **/
	if (inf->AttrValue)
	    {
	    nmSysFree(inf->AttrValue);
	    inf->AttrValue = NULL;
	    }

	/** Handle special attributes. **/
	if (!strcmp(attrname, "Transfer-Encoding"))
	    {
	    libmime_GetIntAttr(inf->Header, "Transfer-Encoding", NULL, &int_attr);
	    val->String = EncodingStrings[int_attr];
	    return 0;
	    }

	/** Split the given attribute name into attribute and parameter. **/
	libmime_GetAttrParamNames(local_attrname, &attrName, &paramName);

	/** The attribute wasn't readable. **/
	if (!attrName)
	    {
	    goto error;
	    }

	/** Get the indicated attribute. **/
	attr = (pMimeAttr)libmime_xhLookup(&inf->Header->Attrs, attrName);
	if (!attr)
	    {
	    if (!strcmp(attrName, "annotation"))
		{
		val->String = "";
		return 0;
		}
	    if (!strcmp(attrName, "name"))
		{
		return libmime_GetStringAttr(inf->Header, "Name", NULL, &val->String);
		}
	    if (!strcmp(attrName, "outer_type")   ||
		!strcmp(attrName, "content_type") ||
		!strcmp(attrName, "inner_type"))
		{
		return libmime_GetStringAttr(inf->Header, "Content-Type", NULL, &val->String);
		}

	    goto error;
	    }

	/** If no parameter was specified, return the attribute. **/
	if (!paramName)
	    {
	    /** Return the data stored in the attribute. **/
	    val->Generic = attr->Ptod->Data.Generic;
	    return 0;
	    }

	/** Get the indicated parameter. **/
	param = (pMimeParam)libmime_xhLookup(&attr->Params, paramName);
	if (!param)
	    {
	    goto error;
	    }


	/** Return the data stored in the parameter. **/
	val->Generic = param->Ptod->Data.Generic;

	/** Free the local copy of attrname. **/
	nmSysFree(local_attrname);

    return 0;

    error:
	if (local_attrname)
	    {
	    nmSysFree(local_attrname);
	    }

	return -1;
    }


/***
 ***  mimeGetNextAttr
 ***/
char*
mimeGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pMimeInfo inf = MIME(inf_v);
    pMimeAttr attr;

	/** Get the next element from the attributes hash. **/
	inf->CurrAttr = xhGetNextElement(&inf->Header->Attrs, inf->CurrAttr);

	/** If there are no more attributes, return NULL. **/
	if (!inf->CurrAttr)
	    {
	    return NULL;
	    }

	/** Get the attribute from the current hash element. **/
	attr = (pMimeAttr)inf->CurrAttr->Data;

	/** Handle special attributes. **/
	if (!strcasecmp(attr->Name, "Content-Type"))
	    {
	    return mimeGetNextAttr(inf_v, oxt);
	    }

    return attr->Name;
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

    return mimeGetNextAttr(inf_v, oxt);
    }


/***
 ***  mimeSetAttrValue
 ***/
int
mimeSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pMimeInfo inf = MIME(inf_v);
    char *attrName = NULL;
    char *paramName = NULL;
    pMimeAttr attr = NULL;
    pMimeParam param = NULL;

    pFile tempFile = NULL;
    char buf[MIME_BUFSIZE];
    int readOffset = 0;
    int inc;
    XString filename;

	libmime_GetAttrParamNames(attrname, &attrName, &paramName); /* Currently always returns 0. */

	/** Initialize the string. **/
	xsInit(&filename);

	/** Do we have the attribute? **/
	attr = libmime_GetMimeAttr(inf->Header, attrName);
	if (!attr)
	    {
	    /** No attribute, how about the param? **/
	    param = libmime_GetMimeParam(inf->Header, attrName, paramName);
	    if (!param)
		{
		/** Neither the attr nor param exists, error out. **/
		mssError(1, "MIME", "Could not find the attribute or parameter to set.");
		goto error;
		}
	    else /* Param exists, now change it. */
		{
		// TODO!!!
		}
	    }
	else /* Attr exists, now change it. */
	    {
	    /** Set the filename xstring. **/
	    xsConcatenate(&filename, "/tmp/", -1);
	    xsConcatenate(&filename, attr->Name, -1);
	    xsConcatenate(&filename, ".msg", -1);

	    /** Force a create of the temp file. **/
	    tempFile = fdOpen(filename.String, O_RDWR | O_CREAT | O_EXCL, 0x755);

	    if (!tempFile)
		{
		mssError(1, "MIME", "Could not create the temp file.");
		goto error;
		}

	    /** Read the current file into buf up to where we want to change it. **/
	    objRead(inf->Obj->Prev, NULL, 0, 0, FD_U_SEEK);
	    memset(buf, 0, MIME_BUFSIZE);
	    for (inc = MIME_BUFSIZE < (attr->ValueSeekStart - readOffset) ? MIME_BUFSIZE : (attr->ValueSeekStart - readOffset);
		    inc > 0;
		    inc = MIME_BUFSIZE < (attr->ValueSeekStart - readOffset) ? MIME_BUFSIZE : (attr->ValueSeekStart - readOffset))
		{
		readOffset += objRead(inf->Obj->Prev, buf, inc, 0, 0);
		/** Write the pre-change part. **/
		if (fdWrite(tempFile, buf, strlen(buf), 0, 0) < 0)
		    {
		    mssError(1, "MIME", "Could not write to the temp file.");
		    goto error;
		    }
		/** Hijack the inc to form the maxcnt **/
		memset(buf, 0, MIME_BUFSIZE);
		}

	    /** Seek to the correct location. Maybe don't have to do this? **/

	    /** Write the new value **/
	    libmime_WriteAttrParam(tempFile, inf->Header, attrName, NULL, datatype, val);

	    objRead(inf->Obj->Prev, NULL, 0, attr->ValueSeekEnd, FD_U_SEEK);

	    /** Write the post-change part. **/
	    memset(buf, 0, MIME_BUFSIZE);
	    while (objRead(inf->Obj->Prev, buf, MIME_BUFSIZE, 0, 0) > 0)
		{
		/** Write the pre-change part. **/
		if (fdWrite(tempFile, buf, strlen(buf), 0, 0) < 0)
		    {
		    mssError(1, "MIME", "Could not write to the temp file.");
		    goto error;
		    }
		memset(buf, 0, MIME_BUFSIZE);
		}
	    }

    /** We're on the happy path! **/
    return 0;

    error:
	/** Deallocate the xstring **/
	xsDeInit(&filename);

	return -1;

    }


/***
 ***  mimeAddAttr
 ***/
int
mimeAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt)
    {
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
