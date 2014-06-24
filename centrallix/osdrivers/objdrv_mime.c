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
    int		NextAttr;
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

/*
**  mimeOpen
*/
void*
mimeOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pLxSession lex = NULL;
    pMimeInfo inf;
    pMimeHeader msg;
    pMimeHeader tmp;
    char *node_path;
    char *buffer;
    char *ptr;
    int i, size, found_match = 0;
    char nullbuf[1];

    if (MIME_DEBUG) fprintf(stderr, "\n");
    if (MIME_DEBUG) fprintf(stderr, "MIME: mimeOpen called with \"%s\" content type.  Parsing as such.\n", systype->Name);
    if (MIME_DEBUG) fprintf(stderr, "objdrv_mime.c was offered: (%i,%i,%i) %s\n",obj->SubPtr,
	    obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));

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
    if (MIME_DEBUG)
	{
	fprintf(stderr, "\n-----------------------------------------------------------------\n");
	for (i=0; i < xaCount(&msg->Parts); i++)
	    {
	    tmp = (pMimeHeader)xaGetItem(&msg->Parts, i);
	    fprintf(stderr,"--[PART: s(%10d),e(%10d)]----------------------------\n", (int)tmp->MsgSeekStart, (int)tmp->MsgSeekEnd);
	    buffer = (char*)nmMalloc(1024);
	    size = libmime_PartRead(inf->MimeDat, tmp, buffer, 1023, 0, FD_U_SEEK);
	    buffer[size] = 0;
	    printf("--%d--%s--\n", size,buffer);
	    nmFree(buffer, 1024);
	    }
	fprintf(stderr, "-----------------------------------------------------------------\n\n");
	}
    mlxCloseSession(lex);
    lex = NULL;

    /** Find and set the filename of the root node **/
    node_path = obj_internal_PathPart(obj->Pathname, obj->Pathname->nElements-1, 1);
    libmime_SetStringAttr(msg, "Filename", node_path, 0);

    /** assume we're only going to handle one level... **/
    /** no longer. */
    obj->SubCnt=1;

    /** While we have a multipart message and there are more elements in the path,
     ** go through all elements and see if we have another multipart element.
     ** If so, repeat the search.
     **/
    while (libmime_GetIntAttr(inf->Header, "ContentMainType") == MIME_TYPE_MULTIPART &&
	   obj->Pathname->nElements >= obj->SubPtr+obj->SubCnt)
	{
	/** assume we don't have a match **/
	found_match = 0;

	/** at least one more element of path to worry about **/
	ptr = obj_internal_PathPart(obj->Pathname, obj->SubPtr+obj->SubCnt-1, 1);
	for (i=0; i < xaCount(&(inf->Header->Parts)); i++)
	    {
	    pMimeHeader phdr;

	    phdr = xaGetItem(&(inf->Header->Parts), i);
	    if (!strcmp(libmime_GetStringAttr(phdr, "Filename"), ptr))
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

    if(MIME_DEBUG) printf("objdrv_mime.c is taking: (%i,%i,%i) %s\n",obj->SubPtr,
	    obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));
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


/*
**  mimeClose
*/
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


/*
**  mimeDelete
*/
int
mimeDelete(pObject obj, pObjTrxTree* oxt)
    {
    return 0;
    }


/*
**  mimeRead
*/
int
mimeRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    int size;
    pMimeInfo inf = (pMimeInfo)inf_v;

    /** Check recursion **/
    if (thExcessiveRecursion())
	{
	mssError(1,"MIME","Could not read data: resource exhaustion occurred");
	return -1;
	}

    if (libmime_GetIntAttr(inf->Header, "ContentMainType") == MIME_TYPE_MULTIPART)
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


/*
**  mimeWrite
*/
int
mimeWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    return 0;
    }


/*
**  mimeOpenQuery
*/
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


/*
**  mimeQueryFetch
*/
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


/*
**  mimeQueryClose
*/
int
mimeQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    nmFree(qy_v, sizeof(MimeQuery));
    return 0;
    }


/*
**  mimeGetAttrType
*/
int
mimeGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {

    if (!strcmp(attrname, "name")) return DATA_T_STRING;
    if (!strcmp(attrname, "content_type")) return DATA_T_STRING;
    if (!strcmp(attrname, "annotation")) return DATA_T_STRING;
    if (!strcmp(attrname, "inner_type")) return DATA_T_STRING;
    if (!strcmp(attrname, "outer_type")) return DATA_T_STRING;
    if (!strcmp(attrname, "subject")) return DATA_T_STRING;
    if (!strcmp(attrname, "charset")) return DATA_T_STRING;
    if (!strcmp(attrname, "transfer_encoding")) return DATA_T_STRING;
    if (!strcmp(attrname, "mime_version")) return DATA_T_STRING;
    if (!strcmp(attrname, "content-disposition")) return DATA_T_STRING;

    return -1;
    }


/*
**  mimeGetAttrValue
*/
int
mimeGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pMimeInfo inf = MIME(inf_v);
    char tmp[32];

    if (inf->AttrValue)
	{
	nmSysFree(inf->AttrValue);
	inf->AttrValue = NULL;
	}
    if (!strcmp(attrname, "inner_type"))
	{
	return mimeGetAttrValue(inf_v, "content_type", DATA_T_STRING, val, oxt);
	}
    if (!strcmp(attrname, "annotation"))
	{
	val->String = "";
	return 0;
	}
    if (!strcmp(attrname, "name"))
	{
	val->String = libmime_GetStringAttr(inf->Header, "Filename");
	return 0;
	}
    if (!strcmp(attrname, "outer_type"))
	{
	/** malloc an arbitrary value -- we won't know the real value until the snprintf **/
	inf->AttrValue = (char*)nmSysMalloc(128);
	snprintf(inf->AttrValue, 128, "%s/%s", TypeStrings[libmime_GetIntAttr(inf->Header, "ContentMainType")], libmime_GetStringAttr(inf->Header, "ContentSubType"));
	val->String = inf->AttrValue;
	return 0;
	}
    if (!strcmp(attrname, "content_type"))
	{
	/** malloc an arbitrary value -- we won't know the real value until the snprintf **/
	inf->AttrValue = (char*)nmSysMalloc(128);
	snprintf(inf->AttrValue, 128, "%s/%s", TypeStrings[libmime_GetIntAttr(inf->Header, "ContentMainType")], libmime_GetStringAttr(inf->Header, "ContentSubType"));
	val->String = inf->AttrValue;
	return 0;
	}
    if (!strcmp(attrname, "subject"))
	{
	val->String = inf->Header->Subject;
	return 0;
	}
    if (!strcmp(attrname, "charset"))
	{
	val->String = inf->Header->Charset;
	return 0;
	}
    if (!strcmp(attrname, "transfer_encoding"))
	{
	val->String = EncodingStrings[inf->Header->TransferEncoding];
	return 0;
	}
    if (!strcmp(attrname, "mime_version"))
	{
	val->String = inf->Header->MIMEVersion;
	return 0;
	}
    if (!strcmp(attrname, "content-disposition"))
	{
	val->String = libmime_GetStringAttr(inf->Header, "ContentDisposition");
	return 0;
	}

    return -1;
    }


/*
**  mimeGetNextAttr
*/
char*
mimeGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pMimeInfo inf = MIME(inf_v);
    switch (inf->NextAttr++)
	{
	case 0: return "content_type";
	case 1: return "subject";
	case 2: return "charset";
	case 3: return "transfer_encoding";
	case 4: return "mime_version";
	case 5: return "content-disposition";
	}
    return NULL;
    }


/*
**  mimeGetFirstAttr
*/
char*
mimeGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pMimeInfo inf = MIME(inf_v);
    inf->NextAttr=0;
    return mimeGetNextAttr(inf,oxt);
    return NULL;
    }


/*
**  mimeSetAttrValue
*/
int
mimeSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    return -1;
    }


/*
**  mimeAddAttr
*/
int
mimeAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt)
    {
    return -1;
    }


/*
**  mimeOpenAttr
*/
void*
mimeOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*
**  mimeGetFirstMethod
*/
char*
mimeGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*
**  mimeGetNextMethod
*/
char*
mimeGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*
**  mimeExecuteMethod
*/
int
mimeExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }

/*** mimeInfo - Return the capabilities of the object
 ***/
int
mimeInfo(void* inf_v, pObjectInfo info)
    {
    pMimeInfo inf = MIME(inf_v);
	
	info->Flags |= ( OBJ_INFO_F_CANT_ADD_ATTR | OBJ_INFO_F_CANT_SEEK );
	if (libmime_GetIntAttr(inf->Header, "ContentMainType") == MIME_TYPE_MULTIPART)
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


/*
**  mimeInitialize
*/
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
