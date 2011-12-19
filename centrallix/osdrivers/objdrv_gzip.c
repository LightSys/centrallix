#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/newmalloc.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "cxlib/mtsession.h"
#include <zlib.h>
#include <time.h>
/** module definintions **/
#include "centrallix.h"

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
/* Module: 	GZIP objects system driver				*/
/* Author:	Jonathan Rupp	    					*/
/* Creation:	August 18, 2002	    					*/
/* Description:	a GZIP objectsystem driver -- 'nuff said		*/
/*									*/
/************************************************************************/



#define GZIP_BUFFER_SIZE 1024

/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	    Pathname[256];
    unsigned char   Header[10];
    unsigned char*  Extra;
    unsigned char*  Filename;
    unsigned char*  Comment;
    unsigned char   HCRC[2];
    pObject	    Obj;
    z_stream	    stream;
    unsigned char   buffer[GZIP_BUFFER_SIZE];
    int		    lookaheadlen;
    unsigned char   lookahead[8];
    unsigned char   eof;
    int		    mode;
    unsigned int    offset;
    DateTime	    lastmod;
    }
    GzipData, *pGzipData;

#define GZIP_DEBUG_FUNCTION_TRACE 0x01
#define GZIP_DEBUG_HEADER 0x02
#define GZIP_DEBUG_READ 0x04
#define GZIP_DEBUG_BUFFER_DUMP 0x08
#define GZIP_DEBUG_DECOMPRESSION 0x10
#define GZIP_DEBUG_ATTRIBUTES 0x20

#define GZIP_DEBUG 0


/** masks for flag byte in header **/
#define GZIP_FLAG_TEXT 0x01
#define GZIP_FLAG_HCRC 0x02
#define GZIP_FLAG_EXTRA 0x04
#define GZIP_FLAG_NAME 0x08
#define GZIP_FLAG_COMMENT 0x10

/** current mode (which type of session z_stream represents **/
#define GZIP_MODE_NONE 0
#define GZIP_MODE_READ 1
#define GZIP_MODE_WRITE 2
    

#define GZIP(x) ((pGzipData)(x))

/***
 *** gzip_internal_DumpBuffer - a debugging function -- dumps the buffer passed it in hex
 ***/
void
gzip_internal_DumpBuffer(const unsigned char* buf,int len)
    {
    int i;
    for(i=0;i<len-len%8;i+=8)
	printf("%8p: %02x%02x %02x%02x %02x%02x %02x%02x\n",buf+i,
		*(buf+i+1),
		*(buf+i+2),
		*(buf+i+3),
		*(buf+i+4),
		*(buf+i+5),
		*(buf+i+6),
		*(buf+i+7),
		*(buf+i+8));
    }

/***
 *** gzip_internal_FillBuffer - if the input buffer for the z_stream is empty, it reads more
 ***  data from the parent object and fills it up
 ***/
int
gzip_internal_FillBuffer(pGzipData inf)
    {
    if (inf->stream.avail_in == 0)
	{
	/** lab == lookahead bytes moved to main buffer **/
	int lab;
	lab=0;
	if(GZIP_DEBUG & GZIP_DEBUG_READ)
	    printf("we should be bringing in more data....\n");
	if(GZIP_BUFFER_SIZE<8)
	    {
	    printf("GZIP_BUFFER_SIZE is too small!!!!\n");
	    return -1;
	    }
	if(inf->lookaheadlen!=0)
	    {
	    if(inf->lookaheadlen!=8)
		{
		mssError(0,"GZIP","lookahead must be 0 or 8");
		return -1;
		}
	    /** we have bytes in the lookahead -- move them back into main buffer (which is empty) **/
	    memcpy(inf->buffer,inf->lookahead,8);
	    lab=8;
	    }
	if(GZIP_DEBUG & GZIP_DEBUG_READ)
	    printf("lab: %i -- getting: %i\n",lab,GZIP_BUFFER_SIZE-lab);
        inf->stream.avail_in=(uInt)objRead(inf->Obj->Prev,(char*)inf->buffer+lab, GZIP_BUFFER_SIZE-lab, 0, 0);
	if(GZIP_DEBUG & GZIP_DEBUG_READ)
	    printf("bytes read: %i\n",inf->stream.avail_in);
	if ((int)inf->stream.avail_in < 0)
	    {
	    /** there's no more data left to read.  
	     ** the lookahead bytes are in both inf->buffer and inf->lookahead, and that's fine
	     ** we don't update inf->stream.avail_in to indicate they're there though
	     **/
	    if(GZIP_DEBUG & GZIP_DEBUG_READ)
		printf("no data left to read: %i\n",inf->stream.avail_in);
	    inf->eof = 1;
	    return -1;
	    }
	inf->stream.avail_in+=lab;
	if(inf->stream.avail_in<8)
	    {
	    if(GZIP_DEBUG & GZIP_DEBUG_READ)
		printf("there's not enough data to fill the lookahead buffer, getting more\n");
	    /** there's not enough data to fill the lookahead buffer, get more **/
	    /** note: this can _only_ happen on the first read, as the old data 
	     ** from the lookahead buffer would be present after that **/
	    return gzip_internal_FillBuffer(inf);
	    }
	inf->stream.avail_in-=8;
	memcpy(inf->lookahead,inf->buffer+inf->stream.avail_in,8);
	inf->lookaheadlen=8;
	inf->stream.next_in = inf->buffer;
	if(GZIP_DEBUG & GZIP_DEBUG_READ & GZIP_DEBUG_BUFFER_DUMP)
	    {
	    printf("data read from parent object: %i\n",inf->stream.avail_in);
	    gzip_internal_DumpBuffer(inf->buffer,GZIP_BUFFER_SIZE);
	    gzip_internal_DumpBuffer(inf->lookahead,8);
	    }
	if(GZIP_DEBUG & GZIP_DEBUG_READ)
	    printf("bytes passed on: %i\n",inf->stream.avail_in);
	}
    return inf->stream.avail_in;
    }

/***
 *** gzipGetByte - gets one byte of raw input, keeping buffers filled
 ***/
unsigned char
gzipGetByte(pGzipData inf)
    {
    if (!inf->Obj || inf->eof ) return '\0';
    if(gzip_internal_FillBuffer(inf)<0)
	return '\0';
    inf->stream.avail_in--;
    if(GZIP_DEBUG & GZIP_DEBUG_READ)
	printf("read: %02x\n",*(inf->stream.next_in));
    return *(inf->stream.next_in++);
    }

/***
 *** gzip_internal_ParseHeaders -- reads and stores gzip headers 
 ***/
int
gzip_internal_ParseHeaders(pGzipData inf)
    {
    time_t i;
    pDateTime dt;
    struct tm time;
    struct tm *ptime;

    /** if the header has already been read, don't read it again.... **/
    if(inf->Header[0]==0x1f && inf->Header[1]==0x8b && inf->Header[2]==0x08)
	return 0;

    /** ensure that we're not in the middle of compression or decompression **/
    if(inf->mode != GZIP_MODE_NONE)
	return -1;


    /** seek to the beginning of the parent object **/
    objRead(inf->Obj->Prev,NULL,0,0,FD_U_SEEK);

    /** Read gzip header **/
    for(i=0;i<10;i++)
	inf->Header[i]=gzipGetByte(inf);

    /** Check header to ensure it's a gzip header and is deflate **/
    if(inf->Header[0]!=0x1f || inf->Header[1]!=0x8b || inf->Header[2]!=0x08)
	{
	mssError(0,"GZIP","Unable to parse header, got 0x%02x%02x%02x, wanted 0x1f8b08",inf->Header[0],inf->Header[1],inf->Header[2]);
	return -1;
	}

    /** Get extra fields if present **/
    if(inf->Header[3] & GZIP_FLAG_EXTRA)
	{
	short len,i;
	len = gzipGetByte(inf) | gzipGetByte(inf)<<8;
	inf->Extra = (unsigned char*) nmSysMalloc(len+2);
	/** put the 2 bytes indicating the length in inf->Extra **/
	inf->Extra[0]=len & 0xff;
	inf->Extra[1]=len>>8 & 0xff;
	i=2;
	while(i-2<len)
	    inf->Extra[i++]=gzipGetByte(inf);
	}

    /** Read original filename if present **/
    if(inf->Header[3] & GZIP_FLAG_NAME)
	{
	char buf[GZIP_BUFFER_SIZE];
	int i;
	i=0;
	do
	    {
	    buf[i++]=gzipGetByte(inf);
	    } while(i<GZIP_BUFFER_SIZE && buf[i-1]!='\0');
	inf->Filename=(unsigned char*)nmSysMalloc(i);
	memcpy(inf->Filename,buf,i);
	}
    
    /** Read file comment if present **/
    if(inf->Header[3] & GZIP_FLAG_COMMENT)
	{
	char buf[GZIP_BUFFER_SIZE];
	int i;
	i=0;
	do
	    {
	    buf[i++]=gzipGetByte(inf);
	    } while(i<GZIP_BUFFER_SIZE && buf[i-1]!='\0');
	inf->Comment=(unsigned char*)nmSysMalloc(i);
	memcpy(inf->Comment,buf,i);
	}

    /** Read header CRC if present **/
    /** note: not processing it right now **/
    if(inf->Header[3] & GZIP_FLAG_HCRC)
	{
	inf->HCRC[0]=gzipGetByte(inf);
	inf->HCRC[1]=gzipGetByte(inf);
	}

    /** use parent last_modification attribute if it exists,
     **  otherwise use the one from the gzip header
     **/
    i=objGetAttrValue(inf->Obj->Prev,"last_modification",DATA_T_DATETIME,POD(&dt));
    if(i==0)
	{
	memcpy(&(inf->lastmod),dt,sizeof(DateTime));
	}
    else
	{
	i=inf->Header[4] | inf->Header[5]<<8 | inf->Header[6]<<16 | inf->Header[7]<<24;
	/** use the reentrant version, incase we ever go to pthreads **/
	if((ptime=gmtime_r(&i,&time)))
	    {
	    /** dates are relative to 1/1/1900, both in the DateTime and the struct tm**/
	    inf->lastmod.Part.Second=ptime->tm_sec;
	    inf->lastmod.Part.Minute=ptime->tm_min;
	    inf->lastmod.Part.Hour=ptime->tm_hour;
	    inf->lastmod.Part.Day=ptime->tm_mday;
	    inf->lastmod.Part.Month=ptime->tm_mon;
	    inf->lastmod.Part.Year=ptime->tm_year; 
	    }
	}


    if(GZIP_DEBUG & GZIP_DEBUG_HEADER)
	{
	printf("Processed gzip header (0x%02x) as:\n",inf->Header[3]);
	printf("  Extra fields: %s\n",inf->Extra?"yes":"no");
	printf("  Filename: %s\n",inf->Filename);
	printf("  Comment: %s\n",inf->Comment);
	printf("  Header CRC (0000 if none): %02x%02x\n",inf->HCRC[0],inf->HCRC[1]);
	}

    return 0;
    }

/*** gzipOpen - open an object.
 ***/
void*
gzipOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pGzipData inf;
    char* node_path;
    char* ptr;
    int i;

	/** Allocate the structure **/
	inf = (pGzipData)nmMalloc(sizeof(GzipData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(GzipData));
	inf->Obj = obj;

	/** Determine the node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);


	/** Set object params. **/
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));

	/** we want only one level **/
	obj->SubCnt=1;

    return (void*)inf;
    }


/*** gzipClose - close an open object.
 ***/
int
gzipClose(void* inf_v, pObjTrxTree* oxt)
    {
    pGzipData inf = GZIP(inf_v);

	nmFree(inf,sizeof(GzipData));

    return 0;
    }


/*** gzipCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
gzipCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    int fd;
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = gzipOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	gzipClose(inf, oxt);

    return 0;
    }


/*** gzipDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
gzipDelete(pObject obj, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** gzipRead - Reads data from compressed file
 ***/
int
gzipRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pGzipData inf = GZIP(inf_v);
    int rval;
    if(inf->mode == GZIP_MODE_NONE)
	{
	if(gzip_internal_ParseHeaders(inf)<0)
	    return -1;

	/** start decompression **/
	if(inflateInit2(&(inf->stream), -MAX_WBITS)!=Z_OK)
	    return -1;

	/* according to gzio.c in zlib-1.1.4:
	 *
	 * windowBits is passed < 0 to tell that there is no zlib header.
	 * Note that in this case inflate *requires* an extra "dummy" byte
	 * after the compressed stream in order to complete decompression and
	 * return Z_STREAM_END. Here the gzip CRC32 ensures that 4 bytes are
	 * present after the compressed stream.
	 */

	inf->mode=GZIP_MODE_READ;
	}
    if(gzip_internal_FillBuffer(inf)<0)
	return -1;
    inf->stream.next_out=(unsigned char*)buffer;
    inf->stream.avail_out=maxcnt;
    while((rval=inflate(&(inf->stream),Z_SYNC_FLUSH))==Z_OK)
	{
	if(inf->stream.avail_out==0)
	    {
	    /** we have all the data we were asked for **/
	    if(GZIP_DEBUG & GZIP_DEBUG_DECOMPRESSION & GZIP_DEBUG_BUFFER_DUMP)
		{
		printf("decompressed buffer (data left):\n");
		gzip_internal_DumpBuffer((unsigned char*)buffer,maxcnt);
		}
	    inf->offset+=maxcnt;
	    return maxcnt;
	    }
	/** we must need data **/
	else if(gzip_internal_FillBuffer(inf)<0)
	    {
	    /** no more data left **/
	    break;
	    }
	printf("avail_in: %i\n",inf->stream.avail_in);
	}

    switch(rval)
	{
	case Z_DATA_ERROR:
	    /** there was a problem with the input data (bad checksum, etc.) **/
	    /** we will _always_ get this at the end of stream -- zlib doesn't like the CRC at the end.... **/
	    /** if there are less than 4 bytes left, it's probably zlib complaining about the crc at the end... **/
	    printf("inf->buffer: %p\n",inf->buffer);
	    printf("inf->eof: %i\n",inf->eof);
	    printf("inf->stream.next_in: %p\n",inf->stream.next_in);
	    printf("inf->stream.avail_in: %i\n",inf->stream.avail_in);
	    mssError(0,"GZIP","data error detected");
	    break;
	case Z_OK:
	case Z_STREAM_END:
	    /** there was no more available input data **/
	    break;
	case Z_MEM_ERROR:
	    mssError(0,"GZIP","memory error!");
	    break;
	case Z_BUF_ERROR:
	    /** if the last 4 lookahead bytes (LSB) match the uncompressed length % 2^32, there is no problem **/
	    if(inf->offset%0xffffffff != (inf->lookahead[4] | inf->lookahead[5]<<8 | inf->lookahead[6]<<16 | inf->lookahead[7]<<24) )
		{
		mssError(0,"GZIP","buffer error!");
		}
	    break;
	default:
	    mssError(0,"GZIP","inflate() returned unknown error");
	    break;
	}

    /** figure out how many bytes we actually read, and end the inflate session **/
    rval=maxcnt-inf->stream.avail_out; 
    inf->offset+=rval;
    if(inflateEnd(&(inf->stream))==Z_STREAM_ERROR)
	mssError(0,"GZIP",inf->stream.msg);
    inf->mode=GZIP_MODE_NONE;
    if(GZIP_DEBUG & GZIP_DEBUG_DECOMPRESSION)
	{
	printf("decompressed buffer (ending...):\n");
	gzip_internal_DumpBuffer((unsigned char*)buffer,maxcnt);
	}
    return rval;

    }


/*** gzipWrite - Again, no content.  This fails.
 ***/
int
gzipWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pGzipData inf = GZIP(inf_v);
    return -1;
    }


/*** gzipOpenQuery - open a directory query.  This driver has no subobjects
 ***/
void*
gzipOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** gzipQueryFetch - get the next directory entry as an open object.
 ***/
void*
gzipQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** gzipQueryClose - close the query.
 ***/
int
gzipQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    return -1;
    }

/*** gzipGetAttrType - get the type (DATA_T_gzip) of an attribute by name.
 ***/
int
gzipGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pGzipData inf = GZIP(inf_v);

    	/** built in attributes **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;
	if (!strcmp(attrname,"content_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;
	if (!strcmp(attrname,"inner_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"outer_type")) return DATA_T_STRING;

	/** last_modification comes from the underlying object **/
	if(!strcmp(attrname,"last_modification"))
	    if(inf->lastmod.Value>0)
		{
		return DATA_T_DATETIME;
		}

    return -1;
    }


/*** gzipGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
gzipGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pGzipData inf = GZIP(inf_v);

	if(gzip_internal_ParseHeaders(inf)<0)
	    return -1;

	/** inner_type is an alias for content_type **/
	if(!strcmp(attrname,"inner_type"))
	    return gzipGetAttrValue(inf_v,"content_type",datatype,val,oxt);

	/** outer_type is "application/gzip" **/
	if(!strcmp(attrname,"outer_type"))
	    {
	    val->String="application/gzip";
	    return 0;
	    }

	/** Use filename if known, otherwise make a best guess **/
	if (!strcmp(attrname,"name"))
	    {
	    if(inf->Filename)
		val->String=(char*)inf->Filename;
	    else
		/** TODO: need to examine the current filename
		 **   if it ends in .gz, strip that off and return it
		 **   if it ends in .tgz, change it to .tar and return it (special case)
		 **   otherwise, return as is
		 **/
		val->String = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
	    return 0;
	    }

	if (!strcmp(attrname,"content_type"))
	    {
	    val->String = "application/octet-stream";
	    return 0;
	    }

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    val->String = "";
	    return 0;
	    }

	if(!strcmp(attrname,"last_modification"))
	    if(inf->lastmod.Value>0)
		{
		val->DateTime=&(inf->lastmod);
		return 0;
		}

	if(GZIP_DEBUG & GZIP_DEBUG_ATTRIBUTES)
	    mssError(1,"GZIP","Could not locate requested attribute: %s",attrname);

    return -1;
    }


/*** gzipGetNextAttr - get the next attribute name for this object.
 ***/
char*
gzipGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** gzipGetFirstAttr - get the first attribute name for this object.
 ***/
char*
gzipGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** gzipSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
gzipSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    return -1;
    }


/*** gzipAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
gzipAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt)
    {
    return -1;
    }


/*** gzipOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
gzipOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** gzipGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
gzipGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** gzipGetNextMethod -- same as above.  Always fails. 
 ***/
char*
gzipGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** gzipExecuteMethod - No methods to execute, so this fails.
 ***/
int
gzipExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree oxt)
    {
    return -1;
    }

/*** gzipInfo - Return the capabilities of the object
 ***/
int
gzipInfo(void* inf_v, pObjectInfo info)
    {
    pGzipData inf = GZIP(inf_v);

	info->Flags |= ( OBJ_INFO_F_NO_SUBOBJ | OBJ_INFO_F_CANT_HAVE_SUBOBJ | OBJ_INFO_F_CANT_ADD_ATTR |
		OBJ_INFO_F_CANT_SEEK | OBJ_INFO_F_CAN_HAVE_CONTENT | OBJ_INFO_F_HAS_CONTENT );
	return 0;
    }


/*** gzipInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
gzipInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Setup the structure **/
	strcpy(drv->Name,"GZIP - Gzip objectsystem driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"application/gzip");

	/** Setup the function references. **/
	drv->Open = gzipOpen;
	drv->Close = gzipClose;
	drv->Create = gzipCreate;
	drv->Delete = gzipDelete;
	drv->OpenQuery = gzipOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = gzipQueryFetch;
	drv->QueryClose = gzipQueryClose;
	drv->Read = gzipRead;
	drv->Write = gzipWrite;
	drv->GetAttrType = gzipGetAttrType;
	drv->GetAttrValue = gzipGetAttrValue;
	drv->GetFirstAttr = gzipGetFirstAttr;
	drv->GetNextAttr = gzipGetNextAttr;
	drv->SetAttrValue = gzipSetAttrValue;
	drv->AddAttr = gzipAddAttr;
	drv->OpenAttr = gzipOpenAttr;
	drv->GetFirstMethod = gzipGetFirstMethod;
	drv->GetNextMethod = gzipGetNextMethod;
	drv->ExecuteMethod = gzipExecuteMethod;
	drv->PresentationHints = NULL;
	drv->Info = gzipInfo;

	nmRegister(sizeof(GzipData),"GzipData");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

MODULE_INIT(gzipInitialize);
MODULE_PREFIX("gzip");
MODULE_DESC("GZIP ObjectSystem Driver");
MODULE_VERSION(0,1,0);
MODULE_IFACE(CX_CURRENT_IFACE);

