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
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"
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

/**CVSDATA***************************************************************

    $Id: objdrv_mime.c,v 1.19 2002/08/29 16:23:03 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_mime.c,v $

    $Log: objdrv_mime.c,v $
    Revision 1.19  2002/08/29 16:23:03  lkehresman
    * Fixed bug that wasn't correctly setting the end seek point for the
      message.
    * Fixed bug that wasn't properly returning the size of the decoded
      chunk, and thus was causing infinite loops when printing a base64
      encoded message.
    * Added "flags" parameter to libmime_PartRead() and made it so it can
      handle both seeking and non-seeking requests (FD_U_SEEK)
    * Fixed mimeRead() so that it handles the InternalSeek for the mime
      driver side of things (this is different than the InternalSeek that
      is used inside the libmime parser itself, it is necessary to keep
      them distinct).

    Revision 1.18  2002/08/29 14:27:55  lkehresman
    Brand spankin' new base64 decoding algorithm.  This one is much better
    than the previous one.  It includes internal buffering, sliding
    windows, and arbitrary seek points without having to read and decode
    the whole document again.

    Also changed the MimeData structure in objdrv_mime.c to be called MimeInfo
    because I added a new data structure internal to the mime parser called
    MimeData.  The new names are more descriptive.

    Revision 1.17  2002/08/27 20:49:39  lkehresman
    I should have checked my code before committing.  I had the conditionals
    mixed up in mimeRead().

    Revision 1.16  2002/08/27 20:42:39  lkehresman
    * Made mimeRead() take advantage of the libmime_PartRead() function that
      decodes the part on the fly
    * Added some documentation to libmime_PartRead()
    * Removed unnecessary code in the binary encoding

    Revision 1.15  2002/08/27 20:14:15  lkehresman
    Added (unoptimized) support for multipart message reading including
    decoding of encoded parts.  No quoted-printable support yet, that will
    come later.  This also includes seeking to arbitrary parts inside the
    encoded parts.

    Revision 1.13  2002/08/26 17:36:52  lkehresman
    * Added some documentation to the functions in libmime
    * Cleaned up some quite a bit of the code in the MIME parser
    * DeInit'ed some xstrings that I forgot to do earlier

    Revision 1.12  2002/08/26 14:21:24  lkehresman
    Fixed innumerable bugs with the multipart mime parsing, it now successfully
    detects and parses multipart messages into an internal data structure.

    Revision 1.11  2002/08/22 20:11:28  lkehresman
    * Renamed MimeMsg to MimeHeader to be more descriptive
    * Mime Boundary detection and storing of seek points
    * Header parsing of MIME parts

    Revision 1.10  2002/08/22 13:44:53  lkehresman
    * defined the 7 top-level media types in mime.h
    * added better support for printing any of the 5 discrete top-level media
      types as per rfc2046 section 3.  Multipart and Message types can not be
      printed.

    Revision 1.9  2002/08/14 14:24:18  lkehresman
    Coded the mimeRead() function so that content can be read from a message.

    Revision 1.8  2002/08/12 19:27:17  lkehresman
    Split the MIME support out into separate files that are now located in
    the utility/mime/ directory.  This will help keep the MIME driver nice
    and clean, and will make development on the MIME support more sensible.

    Revision 1.7  2002/08/12 17:38:00  lkehresman
    Added support for To, Cc, and From lists of email addresses

    Revision 1.5  2002/08/10 02:34:52  lkehresman
    * Removed duplicated StrTrim fuction calls
    * Added rfc2822 complient address-list parsing
        - Groups are detected, but ignored currently, this will come later
        - Nested comments work everywhere except in displayable names
        - escape characters work everywhere except in displayable names

    Revision 1.4  2002/08/10 02:09:45  gbeeley
    Yowzers!  Implemented the first half of the conversion to the new
    specification for the obj[GS]etAttrValue OSML API functions, which
    causes the data type of the pObjData argument to be passed as well.
    This should improve robustness and add some flexibilty.  The changes
    made here include:

        * loosening of the definitions of those two function calls on a
          temporary basis,
        * modifying all current objectsystem drivers to reflect the new
          lower-level OSML API, including the builtin drivers obj_trx,
          obj_rootnode, and multiquery.
        * modification of these two functions in obj_attr.c to allow them
          to auto-sense the use of the old or new API,
        * Changing some dependencies on these functions, including the
          expSetParamFunctions() calls in various modules,
        * Adding type checking code to most objectsystem drivers.
        * Modifying *some* upper-level OSML API calls to the two functions
          in question.  Not all have been updated however (esp. htdrivers)!

    Revision 1.3  2002/08/09 20:01:34  lkehresman
    * Cleaned up the string manipulation in several functions so the original
      header information is not changed.
    * Implemented (at least the shell of) functions to handle RFC[2]822 parsing
      of generic structured header elements based on sections 3 and 4 of rfc2822.
      These handle header folding, whitespaces, and soon will handle comments
      within header fields as well.
    * Created data structures for handling email addresses and lists, and created
      and documented function stubs to handle the parsing of these fields (To,
      Cc, From, Sender, etc).
    * Removed some erronious debugging information
    * Restructured the controlling header parsing function so that it makes it
      easier to perform data validation and data scrubbing on the header fields.

    Revision 1.2  2002/08/09 02:38:49  jorupp
     * added basic attribute support to mime driver
     * set obj->SubCnt=1 in mimeOpen() <-- let the OSML the mime driver is processing one level of the path

    Revision 1.1  2002/08/09 01:55:05  lkehresman
    Removing the old "mailmsg" driver which wasn't working anyway, and added the
    shell for the mime driver.  It still needs a lot of work, but initial headers
    are being parsed.


 **END-CVSDATA***********************************************************/


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
    pMimeData	MimeDat;
    int		NextAttr;
    int		InternalSeek;
    }
    MimeInfo, *pMimeInfo;

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
    pLxSession lex;
    pMimeInfo inf;
    pMimeHeader msg;
    pMimeHeader tmp;
    char *node_path;
    char *buffer;
    int i,size;

    if (MIME_DEBUG) fprintf(stderr, "\n");
    if (MIME_DEBUG) fprintf(stderr, "MIME: mimeOpen called with \"%s\" content type.  Parsing as such.\n", systype->Name);
    if (MIME_DEBUG) fprintf(stderr, "objdrv_mime.c was offered: (%i,%i,%i) %s\n",obj->SubPtr,
	    obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));

    /** Allocate and initialize the MIME structure **/
    inf = (pMimeInfo)nmMalloc(sizeof(MimeInfo));
    if (!inf) return NULL;
    msg = (pMimeHeader)nmMalloc(sizeof(MimeHeader));
    memset(inf,0,sizeof(MimeInfo));
    memset(msg,0,sizeof(MimeHeader));
    /** Set object parameters **/
    node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);
    strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));
    inf->MimeDat = (pMimeData)nmMalloc(sizeof(MimeData));
    memset(inf->MimeDat,0,sizeof(MimeData));
    inf->MimeDat->Parent = obj->Prev;
    inf->MimeDat->ReadFn = objRead;
    inf->MimeDat->WriteFn = objWrite;
    inf->MimeDat->Buffer[0] = 0;
    inf->MimeDat->EncBuffer[0] = 0;
    inf->Header = msg;
    inf->Obj = obj;
    inf->Mask = mask;
    inf->InternalSeek = 0;
    lex = mlxGenericSession(obj->Prev, objRead, MLX_F_LINEONLY|MLX_F_NODISCARD);
    if (libmime_ParseHeader(obj, msg, 0, 0, lex) < 0)
	{
	if (MIME_DEBUG) fprintf(stderr, "MIME: There was an error parsing message header in mimeOpen().\n");
	mlxCloseSession(lex);
	return NULL;
	}
    if (libmime_ParseMultipartBody(obj, msg, msg->MsgSeekStart, msg->MsgSeekEnd, lex) < 0)
	{
	if (MIME_DEBUG) fprintf(stderr, "MIME: There was an error parsing message entity in mimeOpen().\n");
	mlxCloseSession(lex);
	return NULL;
	}

    /*
    fprintf(stderr, "\n-----------------------------------------------------------------\n");
    for (i=0; i < xaCount(&msg->Parts); i++)
	{
	tmp = (pMimeHeader)xaGetItem(&msg->Parts, i);
	fprintf(stderr,"--[PART: s(%10d),e(%10d)]----------------------------\n", (int)tmp->MsgSeekStart, (int)tmp->MsgSeekEnd);
	buffer = (char*)nmMalloc(1024);
	size = libmime_PartRead(inf->MimeDat, tmp, buffer, 80, 0, FD_U_SEEK);
	buffer[size] = 0;
	printf("--%d--%s--\n", size,buffer);
	nmFree(buffer, 1024);
	}
    fprintf(stderr, "-----------------------------------------------------------------\n\n");
    mlxCloseSession(lex);
    */

    /** assume we're only going to handle one level **/
    obj->SubCnt=1;
    if(MIME_DEBUG) printf("objdrv_mime.c is taking: (%i,%i,%i) %s\n",obj->SubPtr,
	    obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));
    return (void*)inf;
    }


/*
**  mimeClose
*/
int
mimeClose(void* inf_v, pObjTrxTree* oxt)
    {
    pMimeInfo inf = MIME(inf_v);

    /** free any memory used to return an attribute **/
    if(inf->AttrValue)
	{
	free(inf->AttrValue);
	inf->AttrValue=NULL;
	}
    return 0;
    }


/*
**  mimeCreate
*/
int
mimeCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    return 0;
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

    if (!offset && !inf->InternalSeek)
	inf->InternalSeek = 0;
    else if (offset)
	inf->InternalSeek = offset;
    size = libmime_PartRead(inf->MimeDat, inf->Header, buffer, maxcnt, inf->InternalSeek, 0);
    inf->InternalSeek += size;

    return size;
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
    return 0;
    }


/*
**  mimeQueryFetch
*/
void*
mimeQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    return 0;
    }


/*
**  mimeQueryClose
*/
int
mimeQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
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

    return -1;
    }


/*
**  mimeGetAttrValue
*/
int
mimeGetAttrValue(void* inf_v, char* attrname, int datatype, void* val, pObjTrxTree* oxt)
    {
    pMimeInfo inf = MIME(inf_v);
    char tmp[32];

    if (inf->AttrValue)
	{
	free(inf->AttrValue);
	inf->AttrValue = NULL;
	}
    if (!strcmp(attrname, "inner_type"))
	{
	return mimeGetAttrValue(inf_v, "content_type", DATA_T_STRING, val, oxt);
	}
    if (!strcmp(attrname, "annotation"))
	{
	*(char**)val = "";
	return 0;
	}
    if (!strcmp(attrname, "name"))
	{
	*(char**)val = "";
	return 0;
	}
    if (!strcmp(attrname, "outer_type"))
	{
	*((char**)val) = "text/plain";
	return 0;
	}
    if (!strcmp(attrname, "content_type"))
	{
	/** malloc an arbitrary value -- we won't know the real value until the snprintf **/
	inf->AttrValue = (char*)malloc(128);
	snprintf(inf->AttrValue, 128, "%s/%s", TypeStrings[inf->Header->ContentMainType-1], inf->Header->ContentSubType);
	*((char**)val) = inf->AttrValue;
	return 0;
	}
    if (!strcmp(attrname, "subject"))
	{
	*((char**)val) = inf->Header->Subject;
	return 0;
	}
    if (!strcmp(attrname, "charset"))
	{
	*((char**)val) = inf->Header->Charset;
	return 0;
	}
    if (!strcmp(attrname, "transfer_encoding"))
	{
	*((char**)val) = EncodingStrings[inf->Header->TransferEncoding-1];
	return 0;
	}
    if (!strcmp(attrname, "mime_version"))
	{
	*((char**)val) = inf->Header->MIMEVersion;
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
mimeSetAttrValue(void* inf_v, char* attrname, int datatype, void* val, pObjTrxTree oxt)
    {
    return 0;
    }


/*
**  mimeAddAttr
*/
int
mimeAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
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
mimeExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree oxt)
    {
    return -1;
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

    strcpy(drv->Name, "MIME - MIME Parsing Driver");
    drv->Capabilities = 0;
    xaInit(&(drv->RootContentTypes), 16);
    xaAddItem(&(drv->RootContentTypes), "message/rfc822");
    xaAddItem(&(drv->RootContentTypes), "multipart/mixed");
    xaAddItem(&(drv->RootContentTypes), "multipart/alternative");
    xaAddItem(&(drv->RootContentTypes), "multipart/form-data");

    if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

MODULE_INIT(mimeInitialize);
MODULE_PREFIX("mime");
MODULE_DESC("MIME ObjectSystem Driver");
MODULE_VERSION(0,1,0);
MODULE_IFACE(CX_CURRENT_IFACE);
