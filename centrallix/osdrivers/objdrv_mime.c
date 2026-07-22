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
/* Copyright (C) 1998-2015 LightSys Technology Services, Inc.		*/
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

#define MIME_ENC_LINE_LEN	76

/*** mime_internal_TransferDecode() - transfer data from a message part
 *** to a file or object, decoding (if needed) along the way.  Returns the
 *** number of decoded bytes.
 ***/
int
mime_internal_TransferDecode(pMimeInfo inf, pMimeHeader src_msg, void* dst, int (*dst_write)())
    {
    //int encoding = libmime_GetIntAttr(src_msg, "Transfer-Encoding");
    int rcnt, wcnt, wcnt_total, total_xfer;
    char xfer_buf[256];

	//if (encoding < 0) encoding = MIME_ENC_7BIT;

	total_xfer = 0;
	while((rcnt = libmime_PartRead(inf, src_msg, xfer_buf, sizeof(xfer_buf), 0, (total_xfer == 0)?FD_U_SEEK:0)) > 0)
	    {
	    if (rcnt < 0)
		return rcnt;
	    if (rcnt == 0)
		{
		mssError(1, "MIME", "Incomplete read of message body while decoding.");
		return -1;
		}
	    wcnt_total = 0;
	    while(wcnt_total < rcnt)
		{
		wcnt = dst_write(dst, xfer_buf+wcnt_total, rcnt-wcnt_total, 0, 0);
		if (wcnt < 0)
		    return wcnt;
		if (wcnt == 0)
		    break;
		wcnt_total += wcnt;
		}
	    total_xfer += wcnt_total;
	    }

    return total_xfer;
    }


/*** mime_internal_TransferEncode() - transfers data from an object or file
 *** into a message part, encoding (if needed) along the way.
 ***/
int
mime_internal_TransferEncode(pMimeInfo inf, void* src, int (*src_read)(), pMimeHeader dst_msg)
    {
    int encoding = -1;
    char dec_buf[57]; /* must be divisible by 3 */
    char enc_buf[sizeof(dec_buf)*5];
    char* output;
    int rcnt, rcnt_blk, enc_cnt;
    int wcnt, wcnt_total;
    int xfer_cnt = 0;

	libmime_GetIntAttr(dst_msg, "Transfer-Encoding", NULL, &encoding);
	if (encoding < 0) encoding = MIME_ENC_7BIT;

	while(1)
	    {
	    /** Read in a block of data **/
	    rcnt_blk = 0;
	    while (rcnt_blk < sizeof(dec_buf))
		{
		rcnt = src_read(src, dec_buf+rcnt_blk, sizeof(dec_buf)-rcnt_blk, 0, 0);
		if (rcnt < 0)
		    return rcnt;
		if (rcnt == 0)
		    break;
		rcnt_blk += rcnt;
		}

	    /** Encode **/
	    switch(encoding)
		{
		case MIME_ENC_BASE64:
		    enc_cnt = libmime_EncodeBase64(enc_buf, dec_buf, sizeof(enc_buf), rcnt_blk);
		    output = enc_buf;
		    break;
		case MIME_ENC_7BIT:
		case MIME_ENC_8BIT:
		case MIME_ENC_BINARY:
		    enc_cnt = rcnt_blk;
		    output = dec_buf;
		    break;
		case MIME_ENC_QP:
		    mssError(1, "MIME", "quoted printable encoding not yet supported");
		    return -1;
		}

	    /** Output, in line-terminated chunks if necessary **/
	    wcnt_total = 0;
	    while (wcnt_total < enc_cnt)
		{
		wcnt = objWrite(inf->Obj->Prev, output + wcnt_total, enc_cnt - wcnt_total, 0, 0);
		if (wcnt <= 0)
		    return -1;
		wcnt_total += wcnt;
		}
	    xfer_cnt += wcnt_total;
	    if (encoding == MIME_ENC_BASE64)
		{
		wcnt = objWrite(inf->Obj->Prev, "\r\n", 2, 0, 0);
		if (wcnt < 2)
		    return -1;
		xfer_cnt += 2;
		}
	    }

    return xfer_cnt;
    }


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
    pContentType apparentType = NULL;
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
    inf->MimeDat->DecodedBuffer[0] = 0;
    inf->MimeDat->EncodedBuffer[0] = 0;
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
	obj_internal_PathPart(obj->Pathname, 0, 0);

	/** If creating a new object with no specified type, infer the type. **/
	if (strcmp(obj->Prev->Driver->Name, "MIME - MIME Parsing Driver"))
	    {
	    apparentType = objTypeFromName(obj->Pathname->Pathbuf);
	    if ((apparentType && mimeCreate(obj, mask, systype, apparentType->Name, oxt))
		    || (!apparentType && mimeCreate(obj, mask, systype, usrtype, oxt)))
		{
		mssError(0, "MIME", "Could not create new mime object.");
		goto error;
		}
	    }
	/** Otherwise, simply pass the type along. **/
	else
	    {
	    if (mimeCreate(obj, mask, systype, usrtype, oxt))
		{
		mssError(0, "MIME", "Could not create new mime object.");
		goto error;
		}
	    }

	/** Deallocate anything from this go-round before trying again. **/
	mimeClose(inf, oxt);

	/** Make sure that we do not get stuck in a recursive loop. **/
	if (thExcessiveRecursion())
	    {
	    mssError(0, "MIME", "Open/Create call has entered an infinite recursive loop.");
	    }

	/** Open the object we just created, super-ensuring we don't make it again. **/
	obj->Mode &= ~O_CREAT;
	inf = mimeOpen(obj, mask & ~O_CREAT, systype, usrtype, oxt);
	if (!inf)
	    {
	    mssError(0, "MIME", "Failed to open newly created mime object.");
	    goto error;
	    }

	/** Reset the path. **/
	obj_internal_PathPart(obj->Pathname, 0, 0);

	/** Reset the path for any autoname that occured.
	 ** This is to preserve standard behaviour outside the Mime driver
	 ** where the name is still expected to be '*'
	 **/
	if (obj->Mode & OBJ_O_AUTONAME &&
		obj_internal_RenamePath(obj->Pathname, obj->Pathname->nElements-1, "*"))
	    {
	    mssError(0, "MIME", "Failed to reset to generic path.");
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
    XString fileName;
    pLxSession lex = NULL;
    pMimeHeader msg = NULL;
    pMimeHeader phdr = NULL;
    pMimeHeader msgRoot = NULL;
    pMimeInfo inf = NULL;
    char fileHash[9];
    int i, foundMatch = 0;
    char *nodeName, *pathString, *boundary = NULL, *parentName = NULL;

    pFile fd = NULL;
    char buf [MIME_BUFSIZE + 1];
    long targetOffset, currentOffset, targetBufSize, insertionSize;
    int rcnt;
    int xfer_size;
    char* old_contenttype;
    char* old_xferencoding;
    XString hdr_data;
    int hdrlen;
    char* ptr;
    pMimeAttr attr;
    char ctbuf[128];
    int orig_hdrlen, new_hdrlen;
    char new_nodename[128];

	xsInit(&initialContents);
	xsInit(&fileName);
	xsInit(&hdr_data);

	/** Store the name of the Mime object. **/
	nodeName = obj_internal_PathPart(obj->Pathname, obj->Pathname->nElements - 1, 1);

	/** Handle automatic naming. **/
	if (obj->Mode & OBJ_O_AUTONAME &&
		!strcmp(nodeName, "*"))
	    {
	    nodeName = new_nodename;
	    strcpy(nodeName, "msgpart_");
	    libmime_internal_MakeARandomFilename(new_nodename, 6);

	    /** Alter the pathname for the next open. **/
	    if (obj_internal_RenamePath(obj->Pathname, obj->Pathname->nElements - 1, nodeName))
		{
		mssError(0, "MIME", "Failed to rename generic element in the path.");
		goto error;
		}
	    }

	/** Use the passed in type, as long as it isn't the default (system/object). **/
	if (!strcmp(usrtype, "system/object") ||
		!strcmp(usrtype, "message/rfc822"))
	    {
	    xsConcatPrintf(&initialContents, "Content-Type: text/plain; name=%s\n\n", nodeName);
	    }
	else
	    {
	    xsConcatPrintf(&initialContents, "Content-Type: %s; name=%s\n\n", usrtype, nodeName);
	    }

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
	    /** Allocate the Mime header. **/
	    msg = libmime_AllocateHeader();
	    if (!msg)
		{
		mssError(1, "MIME", "Could not allocate new message header.");
		return -1;
		}

	    /** Parse the current Mime file into the Mime object tree. **/
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
	    parentName = obj_internal_PathPart(obj->Pathname, obj->SubPtr - 1, 1);
	    libmime_SetFilename(msg, parentName);

	    /** Remember the root of the Mime object tree. **/
	    msgRoot = msg;

	    /** Find the parent object to the new Mime subobject. **/
	    libmime_GetStringAttr(msg, "Name", NULL, &parentName);

	    /** While we have a multipart message and there are more elements in the path,
	     ** go through all elements and see if we have another multipart element.
	     ** If so, repeat the search.
	     **/
	    foundMatch = 1;
	    obj->SubCnt=1;
	    while (obj->Pathname->nElements >= obj->SubPtr+obj->SubCnt &&
		    foundMatch)
		{
		/** assume we don't have a match **/
		foundMatch = 0;

		/** at least one more element of path to worry about **/
		pathString = obj_internal_PathPart(obj->Pathname, obj->SubPtr+obj->SubCnt-1, 1);
		for (i=0; i < xaCount(&(msg->Parts)); i++)
		    {
		    phdr = xaGetItem(&(msg->Parts), i);
		    if (!libmime_GetStringAttr(phdr, "Name", NULL, &ptr) && !strcmp(ptr, pathString))
			{
			msg = phdr;
			obj->SubCnt++;
			foundMatch = 1;
			parentName = ptr;
			break;
			}
		    }
		}

	    /** Make certain that we have found the parent to the last element in the path. **/
	    if (obj->Pathname->nElements != obj->SubPtr + obj->SubCnt)
		{
		mssError(1, "MIME", "Unable to find the Mime parent of the object being created.");
		goto error;
		}

	    /** Build the temporary filename. **/
	    memset(fileHash, 0, 9);
	    libmime_internal_MakeARandomFilename(fileHash, 8);
	    xsConcatPrintf(&fileName, "/tmp/%s%s", parentName, fileHash);

	    /** Get the boundary string for the parent object. **/
	    libmime_GetStringAttr(msg, "Content-Type", "Boundary", &boundary);
	    if (!boundary)
		{
		/** Generate a new boundary string. **/
		boundary = nmSysMalloc(sizeof(char) * 14);
		if (!boundary)
		    {
		    mssError(1, "MIME", "Failed to allocate boundary string for a new multipart object.");
		    goto error;
		    }
		memset(boundary, 0, sizeof(char) * 14);
		strtcpy(boundary, "-----");
		libmime_internal_MakeARandomFilename(boundary, 8);

		/** Store the boundary as a parameter if the multipart already exists. **/
		if (!libmime_GetMimeParam(msg, "Content-Type", "Boundary") &&
			!libmime_GetIntAttr(msg, "Content-Type", "ContentMainType", &i) &&
			i == MIME_TYPE_MULTIPART)
		    {
		    inf = (pMimeInfo)nmMalloc(sizeof(MimeInfo));
		    if (!inf)
			{
			mssError(1, "MIME", "Failed to allocate a Mime info structure to store the new boundary string.");
			goto error;
			}
		    memset(inf, 0, sizeof(MimeInfo));
		    inf->Header = msg;
		    inf->Obj = obj;

		    mimeSetAttrValue(inf, "Content-Type.Boundary", DATA_T_STRING, POD(&boundary), NULL);

		    nmFree(inf, sizeof(MimeInfo));
		    }
		}

	    /** Create the temporary file. **/
	    fd = fdOpen(fileName.String, O_CREAT | O_RDWR, 0755);
	    if (!fd)
		{
		mssError(1, "MIME", "Could not create temporary file.");
		goto error;
		}

	    /** Check if the parent object is a multipart. **/
	    if (!libmime_GetIntAttr(msg, "Content-Type", "ContentMainType", &i) &&
		    i != MIME_TYPE_MULTIPART)
		{
		targetOffset = msg->HdrSeekStart;
		}
	    /** Check if the message has subobjects. **/
	    else if (!msg->Parts.nItems)
		{
		/** If not, seek to the end of the parent object. **/
		targetOffset = msg->MsgSeekEnd;
		}
	    else
		{
		/** Otherwise, seek to the end of the last Mime subobject. **/
		phdr = xaGetItem(&msg->Parts, msg->Parts.nItems-1);
		targetOffset = phdr->MsgSeekEnd;
		}

	    /** Initialize the offsets for reading the initial portion of the Mime file. **/
	    currentOffset = 0;
	    objSeek(obj->Prev, 0);

	    /** Copy up to the target offset before inserting the new objects. **/
	    xfer_size = objTransfer(obj->Prev, objRead, fd, fdWrite, targetOffset - currentOffset);
	    if (xfer_size < targetOffset - currentOffset)
		{
		mssError(0, "MIME", "Unable to copy original contents to temp file");
		goto error;
		}
	    currentOffset += xfer_size;

	    /** Add the boundary to the beginning of the initial contents. **/
	    pathString = (char*)nmSysStrdup(initialContents.String); /* Hijack pathString. */
	    xsPrintf(&initialContents, "--%s\n%s", boundary, pathString);
	    nmSysFree(pathString);

	    /** If adding a subobject to a non-multipart, make a new one. **/
	    if (!libmime_GetIntAttr(msg, "Content-Type", "ContentMainType", &i) &&
		    i != MIME_TYPE_MULTIPART)
		{
		/** Get the current content type and xfer encoding **/
		if (libmime_GetStringAttr(msg, "Content-Type", NULL, &old_contenttype) != 0)
		    old_contenttype = "text/plain";
		if (libmime_GetStringAttr(msg, "Content-Transfer-Encoding", NULL, &old_xferencoding) != 0)
		    old_xferencoding = "7bit";

		/** Get the original header data **/
		hdrlen = msg->HdrSeekEnd - msg->HdrSeekStart;
		objSeek(obj->Prev, msg->HdrSeekStart);
		if (objTransfer(obj->Prev, objRead, &hdr_data, xsWrite, hdrlen) != hdrlen)
		    {
		    mssError(0, "MIME", "Unable to copy original header to memory");
		    goto error;
		    }
		orig_hdrlen = strlen(hdr_data.String);

		/** Replace the content type and content-transfer-encoding **/
		attr = libmime_GetMimeAttr(msg, "Content-Type");
		snprintf(ctbuf, sizeof(ctbuf), "Content-Type: multipart/mixed; boundary=%s; name=%s\n", boundary, parentName);
		if (attr)
		    xsSubst(&hdr_data, attr->AttrSeekStart - msg->HdrSeekStart, attr->AttrSeekEnd - attr->AttrSeekStart, ctbuf, -1);
		else
		    xsConcatenate(&hdr_data, ctbuf, -1);
		attr = libmime_GetMimeAttr(msg, "Content-Transfer-Encoding");
		if (attr)
		    xsSubst(&hdr_data, attr->AttrSeekStart - msg->HdrSeekStart, attr->AttrSeekEnd - attr->AttrSeekStart, "Content-Transfer-Encoding: 7bit\n", -1);
		else
		    xsConcatenate(&hdr_data, "Content-Transfer-Encoding: 7bit\n", -1);
		new_hdrlen = strlen(hdr_data.String);

		/** Write the modified main header back out **/
		if (fdWrite(fd, hdr_data.String, new_hdrlen, 0, FD_U_PACKET) < 0)
		    {
		    mssError(0, "MIME", "Unable to write modified message header to file");
		    goto error;
		    }

		insertionSize = fdPrintf(fd, "\n--%s\nContent-Type: %s\nContent-Transfer-Encoding: %s\n\n", boundary, old_contenttype, old_xferencoding);

		/** Initialize the offsets for reading message of the Mime file. **/
		currentOffset = msg->MsgSeekStart;
		targetOffset = msg->MsgSeekEnd;

		/** Copy up to the target offset to copy the message. **/
		objSeek(obj->Prev, currentOffset);
		xfer_size = objTransfer(obj->Prev, objRead, fd, fdWrite, targetOffset - currentOffset);
		if (xfer_size < targetOffset - currentOffset)
		    {
		    mssError(0, "MIME", "Unable to copy original object content to temp file");
		    goto error;
		    }
		currentOffset += xfer_size;

		/** Indicate the target offset to copy back into the OSML. **/
		targetOffset = msg->HdrSeekStart;

		/** Write the final boundary for the new multipart subobject. **/
		insertionSize += fdPrintf(fd, 
			"\n"
			"--%s\n"
			"Content-Type: %s; name=%s\n"
			"\n"
			"\n"
			"--%s--\n",
			boundary,
			(!strcmp(usrtype, "system/object") || !strcmp(usrtype, "message/rfc822"))?"text/plain":usrtype,
			nodeName,
			boundary);

		/** Store the new offsets for the message. **/
		//msg->HdrSeekStart += insertionSize;
		msg->HdrSeekEnd += (new_hdrlen - orig_hdrlen);
		msg->MsgSeekStart += (new_hdrlen - orig_hdrlen);
		msg->MsgSeekEnd += ((new_hdrlen - orig_hdrlen) + insertionSize);

		/** Calculate the final offset. **/
		currentOffset += insertionSize;

		/** Warn the user that the creation path is invalid. **/
		//mssError(0, "MIME", "WARNING: Adding a subobject to a non-multipart will reorient the directory structure. The creation path is now invalid.");
		}
	    else
		{
		/** If writing the first subobject, also add a terminating boundary. **/
		if (!msg->Parts.nItems)
		    {
		    xsConcatPrintf(&initialContents, "--%s--\n", boundary);
		    }

		/** Write the new subobject to the temporary file. **/
		currentOffset += fdWrite(fd, initialContents.String, initialContents.Length, 0, 0);

		/** Indicate the target offset to copy back into the OSML. **/
		targetOffset = phdr->MsgSeekEnd;
		}

	    /** Copy the rest of the file. **/
	    xfer_size = objTransfer(obj->Prev, objRead, fd, fdWrite, -1);
	    if (xfer_size < 0)
		{
		mssError(0, "MIME", "Unable to copy modified contents to temporary file.");
		goto error;
		}

	    /** Write the changes back to the Object System. **/
	    libmime_SaveTemporaryFile(fd, obj, targetOffset);

	    /** Recalculate the terminating offset of the parent object. **/
	    msg->MsgSeekEnd = currentOffset;

	    /** Count the new object in SubCnt. **/
	    obj->SubCnt++;

	    /** Deallocate the Mime object tree. **/
	    libmime_DeallocateHeader(msgRoot);

	    /** Close the temp file. **/
	    fdClose(fd, 0);

	    /** Delete the temp file. **/
	    if (remove(fileName.String))
		{
		mssError(1, "MIME", "Failed to delete temporary file.");
		goto error;
		}
	    }

	xsDeInit(&initialContents);
	xsDeInit(&fileName);
	xsDeInit(&hdr_data);
    return 0;

    error:
	xsDeInit(&initialContents);
	xsDeInit(&fileName);
	xsDeInit(&hdr_data);

	if (lex) mlxCloseSession(lex);
	if (msgRoot) libmime_DeallocateHeader(msgRoot);
	if (fd) fdClose(fd, 0);

	return -1;
    }


/***
 ***  mimeDelete - TODO: Actually implement this if you're feeling bored or something.
 ***/
int
mimeDelete(pObject obj, pObjTrxTree* oxt)
    {
    XString fileName;
    pLxSession lex = NULL;
    pMimeHeader msg = NULL;
    pMimeHeader phdr = NULL;
    pMimeHeader msgParent = NULL;
    pMimeHeader msgRoot = NULL;
    char fileHash[9];
    int i, foundMatch = 0;
    char *nodeName, *pathString, *boundary = NULL;

	xsInit(&fileName);

	/** Store the name of the Mime object. **/
	nodeName = obj_internal_PathPart(obj->Pathname, obj->Pathname->nElements - 1, 1);

	/** TODO: Parse the Mime file to generate the Mime object tree. **/

	/** Allocate the Mime header. **/
	msg = libmime_AllocateHeader();
	if (!msg)
	    {
	    mssError(1, "MIME", "Could not allocate new message header.");
	    return -1;
	    }

	/** Parse the current Mime file into the Mime object tree. **/
	lex = mlxGenericSession(obj->Prev, objRead, MLX_F_LINEONLY|MLX_F_NODISCARD|MLX_F_EOF);
	if (libmime_ParseHeader(lex, msg, 0, 0) < 0)
	    {
	    mssError(0, "MIME", "There was an error parsing message header in mimeOpen().");
	    return -1;
	    }
	if (libmime_ParseMultipartBody(lex, msg, msg->MsgSeekStart, msg->MsgSeekEnd) < 0)
	    {
	    mssError(0, "MIME", "There was an error parsing message body in mimeOpen().");
	    return -1;
	    }
	mlxCloseSession(lex);
	lex = NULL;

	/** Remember the root of the Mime object tree. **/
	msgRoot = msg;

	/** Find the object to delete. **/

	/** While there are more elements in the path,
	 ** go through all elements and see if we have found the object.
	 ** If so, repeat the search.
	 **/
	foundMatch = 1;
	obj->SubCnt=1;
	while (obj->Pathname->nElements >= obj->SubPtr+obj->SubCnt &&
		foundMatch)
	    {
	    /** assume we don't have a match **/
	    foundMatch = 0;

	    /** at least one more element of path to worry about **/
	    pathString = obj_internal_PathPart(obj->Pathname, obj->SubPtr+obj->SubCnt-1, 1);
	    for (i=0; i < xaCount(&(msg->Parts)); i++)
		{
		phdr = xaGetItem(&(msg->Parts), i);
		if (!libmime_GetStringAttr(phdr, "Name", NULL, &nodeName) && !strcmp(nodeName, pathString))
		    {
		    msgParent = msg;
		    msg = phdr;
		    obj->SubCnt++;
		    foundMatch = 1;
		    break;
		    }
		}
	    }

	/** If the object was not found or did not have a parent object (i.e. the root object). **/
	if (!foundMatch || !msgParent)
	    {
	    return -1;
	    }

	/** Create a temp file. **/
	/** Copy up to the beginning of the message. **/
	/** Copy after the end of the message. **/
	/** Save the temporary file into the OSML. **/

	/** Or we can just not support it... **/
	mssError(1, "MIME", "The Mime Driver does not currently support deletion.");

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
	size = libmime_PartRead(inf->MimeDat, inf->Header, buffer, maxcnt, inf->InternalSeek, 0);
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
    int rcnt, wcnt, xfer_size;
    int is_7bit = 1, was_7bit = 1;
    int i;
    int xfer_encoding = -1;

	/** Nature of data being written -- 7bit clean? **/
	for(i=0;i<cnt;i++)
	    {
	    if (buffer[i] & 0x80)
		{
		is_7bit = 0;
		break;
		}
	    }
	libmime_GetIntAttr(inf->Header, "Transfer-Encoding", NULL, &xfer_encoding);
	if (xfer_encoding == MIME_ENC_BASE64)
	    was_7bit = 0;

	/** Cache the internal seek. **/
	internalSeek = inf->InternalSeek;

	/** Generate a unique hash for the temporary message file. **/
	fileHash = (char*)nmSysMalloc(sizeof(char) * 9);
	if (!fileHash) goto error;
	memset(fileHash, 0, sizeof(char) * 9);
	libmime_internal_MakeARandomFilename(fileHash, 8);

	/** Get the name of the message. **/
	libmime_GetStringAttr(inf->Header, "Name", NULL, &messageName);

	/** Build the name of the temporary message file. **/
	xsInit(&messageFileName);
	xsConcatPrintf(&messageFileName, "/tmp/%s%s", messageName, fileHash);

	/** Create the temporary message file. **/
	messageFile = fdOpen(messageFileName.String, O_RDWR | O_CREAT, 0755);
	if (!messageFile)
	    {
	    mssError(1, "MIME", "Could not create temporary file.");
	    goto error;
	    }

	/** Seek to the beginning of the message contents. **/
	objSeek(inf->Obj, 0);
	currentOffset = inf->Header->MsgSeekStart;
	targetOffset = inf->Header->MsgSeekEnd;

	/** Copy the message contents into the temporary file. **/
	//xfer_size = objTransfer(inf->Obj, objRead, messageFile, fdWrite, targetOffset - currentOffset);
	xfer_size = mime_internal_TransferDecode(inf, inf->Obj, objRead, messageFile, fdWrite, targetOffset - currentOffset);
	if (xfer_size < targetOffset - currentOffset)
	    {
	    mssError(0, "MIME", "Unable to copy message contents to temporary file.");
	    goto error;
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
	wcnt = fdWrite(messageFile, buffer, cnt, offset, flags);
	inf->InternalSeek += wcnt;

	/** Get the name of the entire Mime file. **/
	libmime_GetStringAttr(inf->MessageRoot, "Name", NULL, &rootName);

	/** Generate a new file hash for the new temporary file. **/
	memset(fileHash, 0, 9);
	libmime_internal_MakeARandomFilename(fileHash, 8);

	/** Build the name of the temporary file to store the entire Mime file. **/
	xsInit(&rootFileName);
	xsConcatPrintf(&rootFileName, "/tmp/%s%s", rootName, fileHash);

	/** Open a temporary file to compile the entire Mime file. **/
	rootFile = fdOpen(rootFileName.String, O_RDWR | O_CREAT, 0755);

	/** Seek to the beginning of the Mime file. **/
	objSeek(inf->Obj->Prev, 0);

	/** Set the offset variables to read to the beginning of the message. **/
	targetOffset = inf->Header->MsgSeekStart;

	/** Copy the pre-message contents of the Mime file into the temporary file. **/
	xfer_size = objTransfer(inf->Obj->Prev, objRead, rootFile, fdWrite, targetOffset);
	if (xfer_size < targetOffset)
	    {
	    mssError(0, "MIME", "Unable to copy pre-message contents to temporary file.");
	    goto error;
	    }
	currentOffset = targetOffset;

	/** Seek to the beginning of the temporary message file. **/
	fdWrite(messageFile, NULL, 0, 0, FD_U_SEEK);

	/** Copy the contents of the temporary message file into the compiling file. **/
	xfer_size = objTransfer(messageFile, fdRead, rootFile, fdWrite, -1);
	if (xfer_size < 0)
	    {
	    mssError(0, "MIME", "Unable to copy modified contents to temporary file.");
	    goto error;
	    }
	currentOffset += xfer_size;

	/** Seek to the end of the message in the Mime file. **/
	objSeek(inf->Obj->Prev, inf->Header->MsgSeekEnd);

	/** Copy the post-message contents of the Mime file into the temporary file. **/
	xfer_size = objTransfer(inf->Obj->Prev, objRead, rootFile, fdWrite, -1);
	if (xfer_size < 0)
	    {
	    mssError(0, "MIME", "Unable to copy modified contents to temporary file.");
	    goto error;
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

    return wcnt;

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

	/** For certain attributes, we defer to obj->Prev **/
	if (!strcmp(attrname, "envelope_from") || !strcmp(attrname, "envelope_to"))
	    return objGetAttrType(inf->Obj->Prev, attrname);

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

	/** For certain attributes, we defer to obj->Prev **/
	if (!strcmp(attrname, "envelope_from") || !strcmp(attrname, "envelope_to"))
	    return objGetAttrValue(inf->Obj->Prev, attrname, datatype, val);

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
	    if (!strcmp(attrName, "outer_type"))
		{
		val->String = "message/rfc822";
		return 0;
		}
	    else if (!strcmp(attrName, "content_type") || !strcmp(attrName, "inner_type"))
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
    int filename_len;
    int rcnt;
    int xfer_size;

	/** For certain attributes, we defer to obj->Prev **/
	if (!strcmp(attrname, "envelope_from") || !strcmp(attrname, "envelope_to"))
	    return objSetAttrValue(inf->Obj->Prev, attrname, datatype, val);

	tempAttrName = nmSysStrdup(attrname);

	libmime_GetAttrParamNames(tempAttrName, &attrName, &paramName); /* Currently always returns 0. */

	/** Malloc the string. **/
	filename_len = strlen(attrname) + 22;
	filename = (char*)nmSysMalloc(filename_len); /* "/tmp/<attrname><16randomchars>\0" */

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
	strtcpy(filename, "/tmp/", filename_len);
	strtcat(filename, attrname, filename_len);
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

	/** Value was present but unset? **/
	if (targetStartOffset == 0 && targetEndOffset == 0)
	    {
	    nmSysFree(filename);
	    nmSysFree(tempAttrName);
	    return mimeAddAttr(inf, attrname, datatype, val, oxt);
	    }

	/** Force a create of the temp file. **/
	fd = fdOpen(filename, O_RDWR | O_CREAT, 0755);

	if (!fd)
	    {
	    mssError(1, "MIME", "Could not create the temp file.");
	    goto error;
	    }

	/** Read the current file into buf up to where we want to change it. **/
	objSeek(inf->Obj->Prev, 0);
	xfer_size = objTransfer(inf->Obj->Prev, objRead, fd, fdWrite, targetStartOffset);
	if (xfer_size < targetStartOffset)
	    {
	    mssError(0, "MIME", "Could not copy message to temp file");
	    goto error;
	    }

	/** Write the new value. (paramName will be NULL if we're writing an attribute) **/
	libmime_WriteAttrParam(fd, inf->Header, attrName, paramName, datatype, val);

	/** Subtract red, add blueberries **/
	/** Or, subtract the offset skipped from the beginning offset to the
	 ** post-attribute offset.
	 **/
	inf->Header->HdrSeekEnd -= targetEndOffset - targetStartOffset;
	inf->Header->MsgSeekStart -= targetEndOffset - targetStartOffset;
	inf->Header->MsgSeekEnd -= targetEndOffset - targetStartOffset;

	/** Do all the generic post stuff. **/
	objSeek(inf->Obj->Prev, targetEndOffset);
	xfer_size = objTransfer(inf->Obj->Prev, objRead, fd, fdWrite, -1);
	if (xfer_size < 0)
	    {
	    mssError(0, "MIME", "Could not copy source data to temp file");
	    goto error;
	    }

	/** Save the file. **/
	libmime_SaveTemporaryFile(fd, inf->Obj, targetStartOffset);

	/** Close the temp file. **/
	fdClose(fd, 0);

	/** Be kind! Rewind! (Yes... again) **/
	objSeek(inf->Obj->Prev, 0);

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

    char filehash[8];
    XString filename;
    pFile fd = NULL;

    char* tempAttrName = NULL;
    char* attrName = NULL;
    char* paramName = NULL;
    pMimeAttr attr = NULL;
    pMimeParam param = NULL;

    long targetBufSize = 0;
    long targetOffset = 0;
    char buf[MIME_BUFSIZE+1];
    int rcnt;
    int xfer_size;

	tempAttrName = nmSysStrdup(attrname);

	/** Initialize the filename string. **/
	xsInit(&filename);

	/** Parse out the attribute and parameter names. **/
	libmime_GetAttrParamNames(tempAttrName, &attrName, &paramName); /* Currently always returns 0. */

	/** If this is an attribute. **/
	if (!paramName || !strlen(paramName))
	    {
	    memset(filehash, 0, 8);
	    libmime_internal_MakeARandomFilename(filehash, 7);

	    /** Construct the filename. **/
	    xsConcatPrintf(&filename, "/tmp/%s%s.msg", attrName, filehash);

	    /** Find the offset at the end of the header. **/
	    targetOffset = inf->Header->HdrSeekEnd;
	    }
	/** Otherwise, this is a parameter. **/
	else
	    {
	    memset(filehash, 0, 8);
	    libmime_internal_MakeARandomFilename(filehash, 7);

	    /** Construct the filename. **/
	    xsConcatPrintf(&filename, "/tmp/%s%s.msg", paramName, filehash);

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
	fd = fdOpen(filename.String, O_RDWR | O_CREAT, 0755);

	/** Check that the temporary file was opened. **/
	if (!fd)
	    {
	    mssError(1, "MIME", "Could not create the temp file.");
	    goto error;
	    }

	/** Copy up to the end of header offset into the temporary file. **/
	objSeek(inf->Obj->Prev, 0);
	xfer_size = objTransfer(inf->Obj->Prev, objRead, fd, fdWrite, targetOffset);
	if (xfer_size < targetOffset)
	    {
	    mssError(0, "MIME", "Could not copy original message to temp file");
	    goto error;
	    }

	/** Add the attribute to the file.  Add a semicolon to the end of the
	 ** attribute if we are adding a parameter.
	 **/
	if (paramName && strlen(paramName))
	    {
	    if (fdWrite(fd, ";", 1, 0, 0) != 1)
		{
		mssError(1, "MIME", "Could not write attribute to the temp file.");
		goto error;
		}

	    /** Update the message offsets. **/
	    inf->Header->HdrSeekEnd += 1;
	    inf->Header->MsgSeekStart += 1;
	    inf->Header->MsgSeekEnd += 1;
	    }
	libmime_WriteAttrParam(fd, inf->Header, attrName, paramName, datatype, val);

	/** Add the separation line between the header and the body. **/
	if (!paramName || !strlen(paramName))
	    {
	    fdWrite(fd, "\n", sizeof(char) * 1, 0, FD_U_PACKET);

	    /** Update the message offsets. **/
	    inf->Header->HdrSeekEnd += 1;
	    inf->Header->MsgSeekStart += 1;
	    inf->Header->MsgSeekEnd += 1;
	    }

	/** Copy up to the end of the file. **/
	xfer_size = objTransfer(inf->Obj->Prev, objRead, fd, fdWrite, -1);
	if (xfer_size < 0)
	    {
	    mssError(0, "MIME", "Could not copy original message data to the temp file.");
	    goto error;
	    }

	/** Save the file. **/
	libmime_SaveTemporaryFile(fd, inf->Obj, targetOffset);

	/** Close the temporary file. **/
	fdClose(fd, 0);

	/** NOTICE: We are being kind by rewinding. **/
	objSeek(inf->Obj->Prev, 0);

	/** Delete the temp file. **/
	if (remove(filename.String))
	    {
	    mssError(1, "MIME", "Could not remove temp file ('%s'). Possible issues changing the file in the future.", filename.String);
	    }

	/** Deinitialize the filename string. **/
	xsDeInit(&filename);
	nmSysFree(tempAttrName);

    return 0;

    error:
	/** Close the temporary file. **/
	if (fd)
	    {
	    fdClose(fd, 0);
	    }

	/** Deinitialize the filename string. **/
	xsDeInit(&filename);
	if (tempAttrName)
	    nmSysFree(tempAttrName);

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
