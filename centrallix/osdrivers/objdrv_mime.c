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
    if (objRead(obj->Prev, nullbuf, 0, 0, FD_U_SEEK) < 0)
	{
	mssErrorErrno(0, "MIME", "Improperly reset mime object file pointer.");
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
	mssError(1, "MIME", "Mime object exists but create and exclusive flags are set. Cannot create mime object.");
	goto error;
	}


    /** If not CREAT and match, error **/
    if (!(inf->Obj->Mode & O_CREAT) &&
	(!foundMatch))
	{
	mssError(1, "MIME", "Mime object not found but create flag not set.");
	goto error;
	}

    /** CREAT and no match... create the file! **/
    if ((inf->Obj->Mode & O_CREAT) &&
	(!foundMatch))
	{
	if (mimeCreate(obj, mask, systype, usrtype, oxt))
	    {
	    mssError(0, "MIME", "Could not create new mime object.");
	    goto error;
	    }

	/** Deallocate anything from this go-round before trying again. **/
	mimeClose(inf, oxt);

	/** Open the object we just created, super-ensuring we don't make it again. **/
	obj->Mode &= ~O_CREAT;
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
    pMimeInfo inf = MIME(inf_v);

    char* messageName = NULL;
    XString messageFileName;
    pFile messageFile = NULL;

    char* rootName = NULL;
    XString rootFileName;
    pFile rootFile = NULL;

    char* fileHash = NULL;
    char buf [MIME_BUFSIZE+1];
    long readSize, currentOffset, targetOffset;
    int internalSeek;

	/** Cache the internal seek. **/
	internalSeek = inf->InternalSeek;

	/** Generate a unique hash for the temporary message file. **/
	fileHash = (char*)nmSysMalloc(sizeof(char) * 9);
	if (!fileHash) return -1;
	memset(fileHash, 0, sizeof(char) * 9);
	libmime_internal_MakeARandomFilename(fileHash, 8);

	/** Get the name of the message. **/
	libmime_GetStringAttr(inf->Header, "Name", NULL, &messageName);

	/** Build the name of the temporary message file. **/
	xsInit(&messageFileName);
	xsConcatPrintf(&messageFileName, "/tmp/%s%s", messageName, fileHash);

	/** Create the temporary message file. **/
	messageFile = fdOpen(messageFileName.String, O_RDWR | O_CREAT | O_EXCL, 0x755);
	if (!messageFile)
	    {
	    mssError(1, "MIME", "Could not create temporary file.");
	    return -1;
	    }

	/** Seek to the beginning of the message contents. **/
	objRead(inf->Obj, NULL, 0, 0, FD_U_SEEK);
	currentOffset = inf->Header->MsgSeekStart;
	targetOffset = inf->Header->MsgSeekEnd;

	/** Copy the message contents into the temporary file. **/
	for (readSize = (targetOffset - currentOffset < MIME_BUFSIZE ? targetOffset - currentOffset : MIME_BUFSIZE);
		readSize > 0;
		readSize = (targetOffset - currentOffset < MIME_BUFSIZE ? targetOffset - currentOffset : MIME_BUFSIZE))
	    {
	    memset(buf, 0, MIME_BUFSIZE + 1);
	    currentOffset += objRead(inf->Obj, buf, MIME_BUFSIZE, 0, 0);

	    if (fdWrite(messageFile, buf, strlen(buf), 0, 0) < 0)
		{
		mssError(1, "MIME", "Unable to copy message contents to temporary file.");
		return -1;
		}
	    }

	/** Set the internal seek to the indicated offset if seeking. **/
	if (flags & OBJ_U_SEEK)
	    {
	    inf->InternalSeek = offset;
	    }
	/** Otherwise, seek to the previous file location. **/
	else
	    {
	    inf->InternalSeek = internalSeek; /* Be kind. Rewind! */
	    fdWrite(messageFile, NULL, 0, inf->InternalSeek, FD_U_SEEK);
	    }

	/** Write to the temporary file as indicated by the function arguments. **/
	inf->InternalSeek += fdWrite(messageFile, buffer, cnt, offset, flags);

	/** Get the name of the entire Mime file. **/
	libmime_GetStringAttr(inf->MessageRoot, "Name", NULL, &rootName);

	/** Generate a new file hash for the new temporary file. **/
	memset(fileHash, 0, 9);
	libmime_internal_MakeARandomFilename(fileHash, 8);

	/** Build the name of the temporary file to store the entire Mime file. **/
	xsInit(&rootFileName);
	xsConcatPrintf(&rootFileName, "/tmp/%s%s", rootName, fileHash);

	/** Open a temporary file to compile the entire Mime file. **/
	rootFile = fdOpen(rootFileName.String, O_RDWR | O_CREAT | O_EXCL, 0x755);

	/** Seek to the beginning of the Mime file. **/
	objRead(inf->Obj->Prev, NULL, 0, 0, FD_U_SEEK);

	/** Set the offset variables to read to the beginning of the message. **/
	currentOffset = 0;
	targetOffset = inf->Header->MsgSeekStart;

	/** Copy the pre-message contents of the Mime file into the temporary file. **/
	for (readSize = (targetOffset - currentOffset < MIME_BUFSIZE ? targetOffset - currentOffset : MIME_BUFSIZE);
		readSize > 0;
		readSize = (targetOffset - currentOffset < MIME_BUFSIZE ? targetOffset - currentOffset : MIME_BUFSIZE))
	    {
	    memset(buf, 0, MIME_BUFSIZE);
	    currentOffset += objRead(inf->Obj->Prev, buf, readSize, 0, 0);

	    if (fdWrite(rootFile, buf, strlen(buf), 0, 0) < 0)
		{
		mssError(1, "MIME", "Unable to copy pre-message contents to temporary file.");
		return -1;
		}
	    }

	/** Seek to the beginning of the temporary message file. **/
	fdWrite(messageFile, NULL, 0, 0, FD_U_SEEK);

	/** Copy the contents of the temporary message file into the compiling file. **/
	memset(buf, 0, MIME_BUFSIZE);
	readSize = fdRead(messageFile, buf, MIME_BUFSIZE, 0, 0);
	currentOffset += readSize;
	while (readSize > 0)
	    {
	    if (fdWrite(rootFile, buf, strlen(buf), 0, 0) < 0)
		{
		mssError(1, "MIME", "Unable to copy modified contents to temporary file.");
		return -1;
		}

	    memset(buf, 0, MIME_BUFSIZE);

	    readSize = fdRead(messageFile, buf, MIME_BUFSIZE, 0, 0);
	    currentOffset += readSize;
	    }

	/** Seek to the end of the message in the Mime file. **/
	objRead(inf->Obj->Prev, NULL, 0, inf->Header->MsgSeekEnd, FD_U_SEEK);

	/** Copy the post-message contents of the Mime file into the temporary file. **/
	memset(buf, 0, MIME_BUFSIZE);
	while (objRead(inf->Obj->Prev, buf, MIME_BUFSIZE, 0, 0) > 0)
	    {
	    if (fdWrite(rootFile, buf, strlen(buf), 0, 0) < 0)
		{
		mssError(1, "MIME", "Unable to copy modified contents to temporary file.");
		return -1;
		}
	    memset(buf, 0, MIME_BUFSIZE);
	    }

	/** Recalculate the offset at the end of the message. **/
	inf->Header->MsgSeekEnd = currentOffset;

	/** Write the changes back to the Object System. **/
	libmime_SaveTemporaryFile(rootFile, inf->Obj, inf->Header->MsgSeekStart);

	/** Close the files. **/
	fdClose(rootFile, 0);
	fdClose(messageFile, 0);

	/** Delete the temporary files. **/
	if (remove(rootFileName.String))
	    {
	    mssError(1, "MIME", "Unable to delete the temporary compiling file.");
	    }

	if (remove(messageFileName.String))
	    {
	    mssError(1, "MIME", "Unable to delete the temporary message file.");
	    }

	/** Deinitialize some stuffz. **/
	xsDeInit(&rootFileName);
	nmSysFree(fileHash);

    return 0;

    error:
	if (fileHash) nmSysFree(fileHash);
	if (messageFile) fdClose(messageFile, 0);
	if (rootFile) fdClose(rootFile, 0);

	xsDeInit(&messageFileName);
	xsDeInit(&rootFileName);

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
    int ret;

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
	    ret = DATA_T_STRING;
	    }
	else
	    {
	    /** If no parameter was specified, return data about the attribute. **/
	    if (!paramName)
		{
		ret = attr->Ptod->DataType;
		}
	    else
		{
		/** Get the indicated parameter. **/
		param = libmime_GetMimeParam(inf->Header, attrName, paramName);
		if (!param)
		    {
		    ret = DATA_T_STRING;
		    }
		else
		    {
		    ret = param->Ptod->DataType;
		    }
		}
	    }
	/** Free the local copy of attrname. **/
	nmSysFree(local_attrname);

    /** Return the apropriate data type. **/
    return ret;

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
	param = libmime_GetMimeParam(inf->Header, attrName, paramName);
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
	if (!strcasecmp(attr->Name, "Content-Type") ||
		!strcasecmp(attr->Name, "Name"))
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
 ***  mimeSetAttrValue - attrname is of the form "<attr_name>.<param_name>" (minus the angle-y thingys)
 ***/
int
mimeSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pMimeInfo inf = MIME(inf_v);
    char *tempAttrName = NULL;
    char *attrName = NULL;
    char *paramName = NULL;
    pMimeAttr attr = NULL;
    pMimeParam param = NULL;

    pFile fd = NULL;
    char buf[MIME_BUFSIZE+1];
    int readOffset = 0;
    int inc, targetStartOffset, targetEndOffset;
    char* filename;

	tempAttrName = nmSysStrdup(attrname);

	libmime_GetAttrParamNames(tempAttrName, &attrName, &paramName); /* Currently always returns 0. */

	/** Malloc the string. **/
	filename = (char*)nmSysMalloc((strlen(attrname) + 22)); /* "/tmp/<attrname><16randomchars>\0" */

	/** Do we have the attribute? **/
	attr = libmime_GetMimeAttr(inf->Header, attrName);
	if (!attr)
	    {
	    /** The attr doesn't exist, so try to create it. **/
	    if (mimeAddAttr(inf, attrname, datatype, val, oxt))
		{
		mssError(1, "MIME", "Could not find or create the attribute to set.");
		goto error;
		}
	    else
		{
		return 0;
		}
	    }

	/** Do general setup. **/
	/** Set the filename string. **/
	strtcpy(filename, "/tmp/", 5);
	strtcpy(filename, attrname, strlen(attrname));
	libmime_internal_MakeARandomFilename(filename, 16);

	if (paramName) /* Trying to set or add a parameter. */
	    {
	    /** Have an attribute, how about a param? **/
	    param = libmime_GetMimeParam(inf->Header, attrName, paramName);
	    if (param)
		{
		 targetStartOffset = param->ValueSeekStart;
		 targetEndOffset = param->ValueSeekEnd;
		}
	    else /* Creating a new param. */
		{
		if (mimeAddAttr(inf, attrname, datatype, val, oxt))
		    {
		    mssError(1, "MIME", "Could not find or create the param to set.");
		    goto error;
		    }
		else
		    {
		    return 0;
		    }
		}
	    }
	else /* No paramName, so change the attr. */
	    {
	    targetStartOffset = attr->ValueSeekStart;
	    targetEndOffset = attr->ValueSeekEnd;
	    }

	/** Force a create of the temp file. **/
	fd = fdOpen(filename, O_RDWR | O_CREAT | O_EXCL, 0x755);

	if (!fd)
	    {
	    mssError(1, "MIME", "Could not create the temp file.");
	    goto error;
	    }

	/** Read the current file into buf up to where we want to change it. **/
	objRead(inf->Obj->Prev, NULL, 0, 0, FD_U_SEEK);
	memset(buf, 0, MIME_BUFSIZE+1);

	for (inc = MIME_BUFSIZE < targetStartOffset - readOffset ? MIME_BUFSIZE : targetStartOffset - readOffset;
		inc > 0;
		inc = MIME_BUFSIZE < targetStartOffset - readOffset ? MIME_BUFSIZE : targetStartOffset - readOffset)
	    {
	    readOffset += objRead(inf->Obj->Prev, buf, inc, 0, 0);
	    /** Write the pre-change part. **/
	    if (fdWrite(fd, buf, strlen(buf), 0, 0) < 0)
		{
		mssError(1, "MIME", "Could not write to the temp file.");
		goto error;
		}
	    /** Hijack the inc to form the maxcnt **/
	    memset(buf, 0, MIME_BUFSIZE+1);
	    }

	/** Write the new value. (paramName will be NULL if we're writing an attribute) **/
	libmime_WriteAttrParam(fd, inf->Header, attrName, paramName, datatype, val);

	/** Do all the generic post stuff. **/
	objRead(inf->Obj->Prev, NULL, 0, targetEndOffset, FD_U_SEEK);
	memset(buf, 0, MIME_BUFSIZE+1);
	while (objRead(inf->Obj->Prev, buf, MIME_BUFSIZE, 0, 0) > 0)
	    {
	    /** Write the post-change part. **/
	    if (fdWrite(fd, buf, strlen(buf), 0, 0) < 0)
		{
		mssError(1, "MIME", "Could not write to the temp file.");
		goto error;
		}
	    memset(buf, 0, MIME_BUFSIZE+1);
	    }

	/** Save the file. **/
	libmime_SaveTemporaryFile(fd, inf->Obj, targetStartOffset);

	/** Close the temp file. **/
	fdClose(fd, 0);

	/** Delete the temp file. **/
	if (remove(filename))
	    {
	    mssError(1, "MIME", "Could not remove temp file ('%s'). Possible issues changing the file in the future.", filename);
	    }

	/** Free the temp name. **/
	nmSysFree(tempAttrName);

	nmSysFree(filename);

    /** We're on the happy path! **/
    return 0;

    error:
	/** Deallocate the xstring **/
	if (filename) nmSysFree(filename);
	if (fd) fdClose(fd, 0);
	if (tempAttrName) nmSysFree(tempAttrName);

	return -1;

    }


/***
 ***  mimeAddAttr
 ***/
int
mimeAddAttr(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pMimeInfo inf = MIME(inf_v);

    char* filehash;
    XString filename;
    pFile fd;

    char* tempAttrName = NULL;
    char* attrName = NULL;
    char* paramName = NULL;
    pMimeAttr attr = NULL;
    pMimeParam param = NULL;

    long offset = 0;
    long targetBufSize = 0;
    long targetOffset = 0;
    char buf[MIME_BUFSIZE+1];

	tempAttrName = nmSysStrdup(attrname);

	/** Initialize the filename string. **/
	xsInit(&filename);

	/** Parse out the attribute and parameter names. **/
	libmime_GetAttrParamNames(tempAttrName, &attrName, &paramName); /* Currently always returns 0. */

	/** If this is an attribute. **/
	if (!paramName || !strlen(paramName))
	    {
	    filehash = (char*)nmSysMalloc(8);
	    memset(filehash, 0, 8);
	    libmime_internal_MakeARandomFilename(filehash, 7);

	    /** Construct the filename. **/
	    xsConcatPrintf(&filename, "/tmp/%s%s.msg", attrName, filehash);

	    nmSysFree(filehash);

	    /** Find the offset at the end of the header. **/
	    targetOffset = inf->Header->HdrSeekEnd;
	    }
	/** Otherwise, this is a parameter. **/
	else
	    {
	    /** Construct the filename. **/
	    xsConcatPrintf(&filename, "/tmp/%s.msg", paramName);

	    /** Get the attribute. **/
	    attr = libmime_GetMimeAttr(inf->Header, attrName);
	    if (!attr)
		{
		goto error;
		}

	    /** Init the hash if it isn't already. **/
	    if (!attr->Params.nRows)
		{
		xhInit(&attr->Params, 7, 0);
		}

	    /** Get the parameter. **/
	    param = libmime_GetMimeParam(inf->Header, attrName, paramName);

	    /** Find the offset at the end of the header. **/
	    targetOffset = attr->ValueSeekEnd;
	    }

	/** Open the temporary file. **/
	fd = fdOpen(filename.String, O_RDWR | O_CREAT | O_EXCL, 0x755);

	/** Check that the temporary file was opened. **/
	if (!fd)
	    {
	    mssError(1, "MIME", "Could not create the temp file.");
	    goto error;
	    }

	/** Copy up to the end of header offset into the temporary file. **/
	objRead(inf->Obj->Prev, NULL, 0, 0, FD_U_SEEK);
	memset(buf, 0, sizeof(char) * MIME_BUFSIZE+1);
	for (targetBufSize = (targetOffset - offset < MIME_BUFSIZE ? targetOffset - offset : MIME_BUFSIZE);
		targetBufSize > 0;
		targetBufSize = (targetOffset - offset < MIME_BUFSIZE ? targetOffset - offset : MIME_BUFSIZE))
	    {
	    /** Read the contents of the file. **/
	    offset += objRead(inf->Obj->Prev, buf, targetBufSize, 0, 0);

	    /** Write the pre-change part. **/
	    if (fdWrite(fd, buf, strlen(buf), 0, 0) < 0)
		{
		mssError(1, "MIME", "Could not write to the temp file.");
		goto error;
		}

	    /** Add a semicolon to the end of the attribute if we are adding a parameter. **/
	    if (paramName && strlen(paramName) && targetOffset - offset <= 0)
		{
		if (fdWrite(fd, ";", 1, 0, 0) < 0)
		    {
		    mssError(1, "MIME", "Could not write to the temp file.");
		    goto error;
		    }
		}

	    /** Reset the temp buffer to 0. **/
	    memset(buf, 0, MIME_BUFSIZE+1);
	    }

	/** Add the attribute to the file. **/
	libmime_WriteAttrParam(fd, inf->Header, attrName, paramName, datatype, val);

	/** Add the separation line between the header and the body. **/
	if (!paramName || !strlen(paramName))
	    {
	    fdWrite(fd, "\n", sizeof(char) * 1, 0, 0);
	    }

	/** Copy up to the end of the file. **/
	memset(buf, 0, sizeof(char) * MIME_BUFSIZE+1);
	while (objRead(inf->Obj->Prev, buf, MIME_BUFSIZE, 0, 0) > 0)
	    {
	    /** Write the post-change part. **/
	    if (fdWrite(fd, buf, strlen(buf), 0, 0) < 0)
		{
		mssError(1, "MIME", "Could not write to the temp file.");
		goto error;
		}

	    /** Reset the temp buffer to 0. **/
	    memset(buf, 0, MIME_BUFSIZE+1);
	    }

	/** Save the file. **/
	libmime_SaveTemporaryFile(fd, inf->Obj, targetOffset);

	/** Close the temporary file. **/
	fdClose(fd, 0);

	/** Delete the temp file. **/
	if (remove(filename.String))
	    {
	    mssError(1, "MIME", "Could not remove temp file ('%s'). Possible issues changing the file in the future.", filename.String);
	    }

	/** Deinitialize the filename string. **/
	xsDeInit(&filename);

    return 0;

    error:
	/** Close the temporary file. **/
	if (fd)
	    {
	    fdClose(fd, 0);
	    }

	/** Deinitialize the filename string. **/
	xsDeInit(&filename);

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
