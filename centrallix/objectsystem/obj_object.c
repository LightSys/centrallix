#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "xhashqueue.h"
#include "expression.h"
#include "magic.h"
#include "htmlparse.h"
#include "mtsession.h"
#include "stparse_ne.h"

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
/* Module: 	obj.h, obj_*.c    					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 26, 1998					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.    */
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/*		--> obj_object.c: implements the generic object access	*/
/*		methods, including open/close/create/delete.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: obj_object.c,v 1.4 2002/04/25 17:59:59 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/objectsystem/obj_object.c,v $

    $Log: obj_object.c,v $
    Revision 1.4  2002/04/25 17:59:59  gbeeley
    Added better magic number support in the OSML API.  ObjQuery and
    ObjSession structures are now protected with magic numbers, and
    support for magic numbers in Object structures has been improved
    a bit.

    Revision 1.3  2002/03/23 05:09:16  gbeeley
    Fixed a logic error in net_http's ls__startat osml feature.  Improved
    OSML error text.

    Revision 1.2  2001/10/16 23:53:02  gbeeley
    Added expressions-in-structure-files support, aka version 2 structure
    files.  Moved the stparse module into the core because it now depends
    on the expression subsystem.  Almost all osdrivers had to be modified
    because the structure file api changed a little bit.  Also fixed some
    bugs in the structure file generator when such an object is modified.
    The stparse module now includes two separate tree-structured data
    structures: StructInf and Struct.  The former is the new expression-
    enabled one, and the latter is a much simplified version.  The latter
    is used in the url_inf in net_http and in the OpenCtl for objects.
    The former is used for all structure files and attribute "override"
    entries.  The methods for the latter have an "_ne" addition on the
    function name.  See the stparse.h and stparse_ne.h files for more
    details.  ALMOST ALL MODULES THAT DIRECTLY ACCESSED THE STRUCTINF
    STRUCTURE WILL NEED TO BE MODIFIED.

    Revision 1.1.1.1  2001/08/13 18:00:59  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:00  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** obj_internal_IsA - determines the possible relationship between two
 *** datatypes, from types.cfg, and returns a value indicating the 
 *** relationship between the two:
 ***
 ***    return 0 if the types are literally the same,
 ***    return a negative integer if the first type is more general than
 ***       the second type,
 ***    return a positive integer if the first type is a more specific kind
 ***       of the second type, and
 ***    return OBJSYS_NOT_ISA (0x80000000) if the types are not related.
 ***/
int
obj_internal_IsA(char* type1, char* type2)
    {
    int i,l = OBJSYS_NOT_ISA;
    pContentType t1, t2;

    	/** Shortcut: are they the same? **/
	if (!strcasecmp(type1,type2)) return 0;

	/** Lookup the types **/
	t1 = (pContentType)xhLookup(&OSYS.Types, type1);
	if (!t1) return OBJSYS_NOT_ISA;
	t2 = (pContentType)xhLookup(&OSYS.Types, type2);
	if (!t2) return OBJSYS_NOT_ISA;

	/** Search the relation list in t1 for t2. **/
	for(i=0;i<t1->RelatedTypes.nItems;i++)
	    {
	    if (t2 == (pContentType)(t1->RelatedTypes.Items[i]))
	        {
		l = (int)(t1->RelationLevels.Items[i]);
		break;
		}
	    }

    return l;
    }


/*** obj_internal_DoPathSegment - process a char* segment of a pathname and
 *** build on the results in the Pathname structure given.  This routine 
 *** parses the segment into '/' separated elements, and also parses out
 *** optional ?=&=&= type parameters encoded within the path elements.
 ***/
int
obj_internal_DoPathSegment(pPathname pathinfo, char* path_segment)
    {
    char* ptr;
    char* bufendptr;
    char ch;
    char* paramnameptr;
    char* endptr;
    char* startptr;
    pStruct inf;
    int element_cnt = 0;

    	/** Scan through, breaking at '/' characters **/
	bufendptr = strchr(pathinfo->Pathbuf,'\0');
	ptr = strtok(path_segment,"/");
	while(ptr)
	    {
	    /** Replace the '\0' with a '/' again? **/
	    if (element_cnt != 0) ptr[-1] = '/';

	    /** Copy from beginning of element up until end or until '?' **/
	    startptr = bufendptr;
	    pathinfo->Elements[pathinfo->nElements] = bufendptr;
	    while(*ptr && *ptr != '?')
	        {
		if ((bufendptr - pathinfo->Pathbuf) >= OBJSYS_MAX_PATH - 2) break;
		ch = htsConvertChar(&ptr);
		if (ch == '/') ch = '_';
		*(bufendptr++) = ch;
		}
	    *(bufendptr++) = '/';

	    /** Was this a '.' directory?  Ignore it and proceed to next item if so. **/
	    if (startptr[0] == '.' && startptr[1] == '/')
	        {
		ptr = strtok(NULL,"/");
		element_cnt++;
		bufendptr = pathinfo->Elements[pathinfo->nElements];
		continue;
		}

	    /** How about a '..' directory?  Back up one element if we can. **/
	    if (startptr[0] == '.' && startptr[1] == '.' && startptr[2] == '/')
	        {
		/** User trying to .. past root?  We check against 1 because first element is '.' **/
		if (pathinfo->nElements == 1)
		    {
		    mssError(1,"OSML","Invalid attempt to reference parent directory of /");
		    return -1;
		    }
		ptr = strtok(NULL,"/");
		bufendptr = pathinfo->Elements[--pathinfo->nElements];
		element_cnt++;
		continue;
		}

	    /** ?=&= parameters were used for this element?  Parse 'em if so. **/
	    if (*ptr == '?')
	        {
		/** Allocate the structinf to hold the param information **/
		pathinfo->OpenCtl[pathinfo->nElements] = stCreateStruct_ne(NULL);

		/** Loop: copy each parameter and value **/
		ptr++;
		while(*ptr)
		    {
		    /** Find the end of the paramname (the '=', '&', or zero) and copy paramname. **/
		    if (*ptr == '\0') break;
		    if (*ptr == '&') ptr++;
		    for(endptr=ptr;*endptr != '\0' && *endptr != '=' && *endptr != '&';endptr++);
		    if (ptr == endptr) continue;
		    inf = stAddAttr_ne(pathinfo->OpenCtl[pathinfo->nElements],"");
		    paramnameptr = inf->Name;
		    while(ptr < endptr && paramnameptr - inf->Name < 63) *(paramnameptr++) = htsConvertChar(&ptr);
		    *paramnameptr = '\0';

		    /** Find end of param value (end-string or '&') and copy it to stringbuf **/
		    if (*ptr == '=')
		        {
			/** We set the string ptr to the _offset_ in CtlBuf because if we use a real **/
			/** pointer the realloc() might change the location of the buffer and invalidate **/
			/** the pointers.  We convert the offsets to pointers later.  The +1 below is **/
			/** because otherwise the first pointer is at offset 0, which will be interpreted **/
			/** incorrectly. **/
			inf->StrVal = (void*)(pathinfo->OpenCtlCnt+1);
			inf->StrAlloc = 0;
			ptr++;
			for(endptr=ptr;*endptr != '\0' && *endptr != '&';endptr++);
			while(ptr < endptr)
			    {
			    ch = htsConvertChar(&ptr);
			    if (pathinfo->OpenCtlCnt+1 >= pathinfo->OpenCtlLen) 
			        {
				pathinfo->OpenCtlLen += 256;
				pathinfo->OpenCtlBuf = (char*)nmSysRealloc(pathinfo->OpenCtlBuf,pathinfo->OpenCtlLen);
				}
			    pathinfo->OpenCtlBuf[pathinfo->OpenCtlCnt++] = ch;
			    }
			pathinfo->OpenCtlBuf[pathinfo->OpenCtlCnt++] = '\0';
			}
		    }
		}
	    else
	        {
		pathinfo->OpenCtl[pathinfo->nElements] = NULL;
		}

	    /** Get the next element of the pathname segment. **/
	    pathinfo->nElements++;
	    ptr = strtok(NULL, "/");
	    element_cnt++;
	    }

	*bufendptr = '\0';

    return 0;
    }


/*** obj_internal_FreePath - release a pathname structure, and free its
 *** subcomponents as well.
 ***/
int
obj_internal_FreePath(pPathname this)
    {
    int i;

    	/** Check link cnt **/
	if ((--this->LinkCnt) > 0) return 0;

    	/** Release the memory for the open-ctl-buf **/
	if (this->OpenCtlBuf) nmSysFree(this->OpenCtlBuf);

	/** Release the subinf structures, if needed. **/
	for(i=0;i<this->nElements;i++) if (this->OpenCtl[i]) stFreeInf_ne(this->OpenCtl[i]);

	/** Now release the pathname structure itself **/
	nmFree(this, sizeof(Pathname));

    return 0;
    }


/*** obj_internal_AllocObj - allocate a new Object structure and initialize
 *** some of the various fields.
 ***/
pObject
obj_internal_AllocObj()
    {
    pObject this;

    	/** Allocate the thing **/
	this = (pObject)nmMalloc(sizeof(Object));
	if (!this) return NULL;

	/** Initialize it **/
	this->Magic = MGK_OBJECT;
	this->Prev = NULL;
	this->Next = NULL;
	this->ContentPtr = NULL;
	this->Pathname = NULL;
	this->Obj = NULL;
	this->Flags = 0;
	this->Data = NULL;
	this->Type = NULL;

    return this;
    }


/*** obj_internal_FreeObj - release an object and release the various 
 *** components associated with it, including the object chain if more
 *** than one is chained together from the open operation.  The release
 *** is done in ->Prev order and ->Next is ignored.
 ***/
int
obj_internal_FreeObj(pObject this)
    {
    pObject del;
    pPathname pathinfo;

    	/** Release the chain of objects **/
        while(this)
            {
	    ASSERTMAGIC(this,MGK_OBJECT);
	    del = this;
	    this = this->Prev;
	    if ((--del->LinkCnt) <= 0)
	        {
	        pathinfo = del->Pathname;
	        if (del->Data) del->Driver->Close(del->Data,NULL);
	        nmFree(del,sizeof(Object));
	    
	        /** Release the pathname data.  Do it in the loop because we **/
	        /** need to decr the link cnt for each referencing Object struct. **/
                if (pathinfo) obj_internal_FreePath(pathinfo);
		}
	    }

    return 0;
    }


/*** obj_internal_DiscardDC - this is the XHQ callback routine for the
 *** DirectoryCache when a cached item is being discarded via the LRU
 *** discard algorithm.  We need to clean up after the thing by closing
 *** the object.
 ***/
int
obj_internal_DiscardDC(pXHashQueue xhq, pXHQElement xe, int locked)
    {
    pDirectoryCache dc;

    	/** Close the object **/
	dc = (pDirectoryCache)xhqGetData(xhq, xe, XHQ_UF_PRELOCKED);
	objClose(dc->NodeObj);
	nmFree(dc, sizeof(DirectoryCache));

    return 0;
    }


/*** obj_internal_TypeFromName - determine the apparent content type of
 *** an object given its name.
 ***/
pContentType
obj_internal_TypeFromName(char* name)
    {
    pContentType apparent_type = NULL, ck_type;
    char* dot_ptr;
    int i;

        /** First step is to check the extension against the types-by-extension hash table **/
        dot_ptr = strrchr(name,'.');
        if (dot_ptr) apparent_type = (pContentType)xhLookup(&OSYS.TypeExtensions, (void*)(dot_ptr+1));

        /** Second step is to look for a type-name-expression that evaluates TRUE for this name **/
        if (!apparent_type)
            {
	    for(i=0;i<OSYS.TypeList.nItems;i++)
	        {
	        ck_type = (pContentType)(OSYS.TypeList.Items[i]);
	        if (ck_type->TypeNameExpression)
	            {
		    expModifyParam(ck_type->TypeNameObjList, "this", (pObject)name);
		    if (expEvalTree((pExpression)(ck_type->TypeNameExpression), (pParamObjects)(ck_type->TypeNameObjList)) >= 0)
		        {
		        if (((pExpression)(ck_type->TypeNameExpression))->DataType == DATA_T_INTEGER &&
		            !(((pExpression)(ck_type->TypeNameExpression))->Flags & EXPR_F_NULL) &&
			    ((pExpression)(ck_type->TypeNameExpression))->Integer != 0)
			    {
			    apparent_type = ck_type;
			    break;
			    }
			}
		    }
		}
	    }

    return apparent_type;
    }


/*** obj_internal_ProcessOpen - take the given pathname and open params
 *** and process it into an open pObject pointer, with Prev and Next 
 *** links set to build a driver-chain and the various Driver fields set
 *** to the drivers handling each level of the open operation.  This 
 *** routine _does_ open the root object first.
 ***/
pObject
obj_internal_ProcessOpen(pObjSession s, char* path, int mode, int mask, char* usrtype)
    {
    pObject this,first_obj;
    pPathname pathinfo;
    int element_id;
    int i,j,v;
    pStruct inf;
    char* name;
    char* type;
    pContentType apparent_type,ck_type = NULL,orig_ck_type;
    pObjDriver drv;
    pDirectoryCache dc = NULL;
    pXHQElement xe;
    char prevname[256];
    int used_openas;

    	/** First, create the pathname structure and parse the ctl information **/
	pathinfo = (pPathname)nmMalloc(sizeof(Pathname));
	if (!pathinfo) return NULL;
	memset(pathinfo,0,sizeof(Pathname));
	pathinfo->OpenCtlBuf = (char*)nmSysMalloc(256);
	pathinfo->OpenCtlLen = 256;
	pathinfo->OpenCtlCnt = 0;

	/** Add path segments for optional CWD, and then given path. **/
	strcpy(pathinfo->Pathbuf,"./");
	pathinfo->Elements[0] = pathinfo->Pathbuf;
	pathinfo->nElements = 1;
	pathinfo->LinkCnt = 0;
	if (path[0] != '/') 
	    {
	    if (obj_internal_DoPathSegment(pathinfo, s->CurrentDirectory) < 0)
	        {
		obj_internal_FreePath(pathinfo);
		return NULL;
		}
	    }
	if (obj_internal_DoPathSegment(pathinfo, path) < 0)
	    {
	    obj_internal_FreePath(pathinfo);
	    return NULL;
	    }

	/** Remove trailing '/' **/
	*(strrchr(pathinfo->Pathbuf,'/')) = '\0';

	/** Ok, got all params.  Now change the offsets to pointers in the OpenCtl **/
	/** structinf (see above comments in DoPathSegment) **/
	for(j=0;j<pathinfo->nElements;j++) if (pathinfo->OpenCtl[j])
	    {
	    for(i=0;i<pathinfo->OpenCtl[j]->nSubInf;i++)
	        {
	        inf = pathinfo->OpenCtl[j]->SubInf[i];
	        if (inf->StrVal) inf->StrVal = pathinfo->OpenCtlBuf + (((int)(inf->StrVal)) - 1);
		}
	    }

	/** Alrighty then!  The pathname is parsed.  Now start doing opens. **/
	/** First, we check the directory cache for a better starting point. **/
	/** If nothing found there, start with opening the root node. **/
	/** BUG! -- this doesn't take into account opening objects with open-as. **/
	this = NULL;
	for(j=pathinfo->nElements-1;j>=2;j--)
	    {
	    /** Try a prefix of the pathname. **/
	    xe = xhqLookup(&OSYS.DirectoryCache, obj_internal_PathPart(pathinfo, 0, j));
	    if (xe)
	        {
		/** Lookup was successful - get the data for it **/
		dc = (pDirectoryCache)xhqGetData(&OSYS.DirectoryCache, xe, 0);
		this = dc->NodeObj;

		/** Link to the intermediate object to lock it open. **/
		objLinkTo(this);
		first_obj = this;

		/** Unlink from the directory cache entry in the hashqueue **/
		xhqUnlink(&OSYS.DirectoryCache, xe, 0);
		break;
		}
	    }
	obj_internal_PathPart(pathinfo,0,0);

	/** Nothing found?  Open root node and start there. **/
	if (!this)
	    {
	    element_id = 0;
	    this = obj_internal_AllocObj();
	    this->Data = OSYS.RootDriver->Open(this, 0, NULL, NULL, NULL);
	    this->Pathname = pathinfo;
	    pathinfo->LinkCnt++;
	    if (!this->Data)
	        {
	        obj_internal_FreeObj(this);
	        return NULL;
	        }
	    this->SubPtr = 0;
	    this->SubCnt = 2;
	    this->Session = s;
	    first_obj = this;
	    this->LinkCnt = 1;
	    }

	/** Now enter the intermediate-open loop, calling relevant drivers according **/
	/** to 1) actual type (from objGetAttrValue(inner_type)), and 2) perceived **/
	/** type (from the filename and extension) **/
	prevname[0] = '\0';
	apparent_type = NULL;
	while(1)
	    {
	    /** Get the name and type from the previous open. **/
	    objGetAttrValue(this,"name",POD(&name));
	    objGetAttrValue(this,"inner_type",POD(&type));

	    /** If the driver "claimed" the last path element, we're done. **/
	    if (this->SubPtr + this->SubCnt - 1 > this->Pathname->nElements) break;

	    /** Determine the apparent/perceived type from the name **/
	    apparent_type = NULL;

	    /** Do NOT operate based on "apparent type" if the name is the same as the previous **/
	    /** intermediate object's name, and the SubCnt of the previous obj was 1. **/
	    if (!this || this->SubCnt != 1 || strcmp(name, prevname))
	        {
		apparent_type = obj_internal_TypeFromName(name);
		}

	    strcpy(prevname, name);

	    /** Still no apparent type?  If so, last step is to assume "system/object", the most generic type **/
	    if (!apparent_type)
	        {
		apparent_type = (pContentType)xhLookup(&OSYS.Types,(void*)"system/object");
		if (!apparent_type)
		    {
		    mssError(1,"OSML","Object access failed - No 'system/object' type!");
		    obj_internal_FreeObj(this);
		    return NULL;
		    }
		}

	    /** Ok, got reported type and apparent type.  See which is more specific. **/
	    v = obj_internal_IsA(type, apparent_type->Name);
	    if (v < 0 || v == OBJSYS_NOT_ISA) ck_type = apparent_type;
	    else ck_type = (pContentType)xhLookup(&OSYS.Types, (void*)type);

	    /** Check for ls__type "open-as" processing **/
	    used_openas = 0;
	    if ((inf = pathinfo->OpenCtl[this->SubPtr + this->SubCnt - 2]) != NULL && (this->SubCnt != 1 || strcmp(name,prevname)))
	        {
		if (stAttrValue_ne(stLookup_ne(inf,"ls__type"),&type) >= 0 && type)
		    {
		    if (obj_internal_IsA(type, ck_type->Name) != OBJSYS_NOT_ISA)
		        {
			ck_type = (pContentType)xhLookup(&OSYS.Types, (void*)type);
			used_openas = 1;
			}
		    }
		}

	    /** Find out what driver handles this type. **/
	    orig_ck_type = ck_type;
	    do  {
	        drv = (pObjDriver)xhLookup(&OSYS.DriverTypes, (void*)ck_type->Name);
		if (!drv)
		    {
		    if (ck_type->IsA.nItems > 0)
		        {
			ck_type = (pContentType)(ck_type->IsA.Items[0]);
			}
		    else
		        {
			/** If the entire path has been used, this is a successful return. **/
			if (this->SubPtr + this->SubCnt >= this->Pathname->nElements) break;

			/** Otherwise, error out **/
			obj_internal_PathPart(this->Pathname,0,0);
		        mssError(1,"OSML","Object '%s' access failed - no driver found",this->Pathname->Pathbuf+1);
		        obj_internal_FreeObj(this);
			return NULL;
			}
		    }
	        }
		while (!drv);

	    /** If entire path has been used but no driver, successful exit. **/
	    if (!drv && this->SubPtr + this->SubCnt >= this->Pathname->nElements) 
	        {
	        if (used_openas) this->Type = orig_ck_type;
		break;
		}

	    /** Got the driver.  Now perform the open operation after setting up the object **/
	    this->Next = obj_internal_AllocObj();
	    this->Next->Prev = this;
	    this->Next->SubPtr = this->SubPtr + this->SubCnt - 1;
	    this->Next->Pathname = pathinfo;
	    pathinfo->LinkCnt++;
	    this = this->Next;
	    this->Driver = drv;
	    this->Session = s;
	    this->LinkCnt = 1;
	    this->Mode = mode;
	    if (used_openas) this->Type = ck_type;

	    /** Driver requested transactions? **/
	    if ((this->Driver->Capabilities & OBJDRV_C_TRANS) && OSYS.TransLayer)
	        {
	        this->LowLevelDriver = this->Driver;
	        this->Driver = OSYS.TransLayer;
	        }
	
	    /** Do the open **/
	    this->Data = this->Driver->Open(this, mask, ck_type, usrtype, &(s->Trx));
	    if (!this->Data)
	        {
		obj_internal_PathPart(this->Pathname,0,0);
	        mssError(0,"OSML","Object '%s' access failed - driver open failed", this->Pathname->Pathbuf+1);
	        obj_internal_FreeObj(this);
		return NULL;
		}
	    }

	/** Check for content-type? **/
	if (name && !this->Type) this->Type = obj_internal_TypeFromName(name);

	/** Cache the upper-level node for this object? **/
	if (this->Prev && this->Prev->Driver != OSYS.RootDriver && !(this->Prev->LowLevelDriver == OSYS.RootDriver && this->Prev->Driver == OSYS.TransLayer))
	    {
	    /** Only cache if not already cached. **/
	    if (!dc || dc->NodeObj != this->Prev)
	        {
		dc = (pDirectoryCache)nmMalloc(sizeof(DirectoryCache));
		dc->NodeObj = this->Prev;
		objLinkTo(this->Prev);
		strcpy(dc->Pathname, obj_internal_PathPart(this->Prev->Pathname, 0, this->SubPtr));
		xe = xhqAdd(&OSYS.DirectoryCache, dc->Pathname, dc);
		if (!xe)
		    {
		    /** Already cached -- oops!  Probably bug! **/
		    objClose(this->Prev);
		    nmFree(dc, sizeof(DirectoryCache));
		    }
		else
		    {
		    xhqUnlink(&OSYS.DirectoryCache, xe, 0);
		    }
		}
	    }

	/** Reset the path. **/
	obj_internal_PathPart(this->Pathname,0,0);

    return this;
    }

#if 00
/*** obj_internal_GetDriver - determine the driver for a top-level file
 *** given the filename (determine via the file's extension) and stat()
 *** information in the directory cache structure.
 ***/
pObjDriver
obj_internal_GetDriver(pDirectoryCache dc_info)
    {
    pObjDriver drv = NULL;
    pContentType ct;
    char* dot_ptr;
    char* slash_ptr;
    char sbuf[256];
    pFile fd;
    int cnt;

	/** Does this file have an extension? **/
	dot_ptr = strrchr(dc_info->Pathname,'.');
	if (dot_ptr)
	    {
	    /** Not an extension if '/' comes after the '.' **/
	    if (strchr(dot_ptr,'/')) dot_ptr = NULL;
	    }

	/** If extension, we can lookup the driver directly. **/
	if (dot_ptr)
	    {
	    dc_info->Type = (pContentType)xhLookup(&(OSYS.TypeExtensions),dot_ptr+1);
	    if (dc_info->Type)
		{
	        drv = (pObjDriver)xhLookup(&(OSYS.DriverTypes),dc_info->Type->Name);
		}
	    }

	/** Did we find the driver yet?  If not, can't depend on .ext **/
	if (!drv)
	    {
	    /** Was the thing a directory?  If not, look for a .type file **/
	    if (!(S_ISDIR((dc_info->fileinfo.st_mode))))
		{
		slash_ptr = strrchr(dc_info->Pathname,'/');
		if (slash_ptr)
		    {
		    *slash_ptr = '\0';
		    if ((slash_ptr - dc_info->Pathname) + 6 <= 255)
			{
			sprintf(sbuf,"%s/.type",dc_info->Pathname);
			if (access(sbuf,F_OK) == 0)
			    {
			    fd = fdOpen(sbuf,O_RDONLY,0600);
			    if (fd)
				{
				if ((cnt=fdRead(fd,sbuf,64,0,0)) > 0)
				    {
				    sbuf[cnt] = 0;
				    if (strchr(sbuf,'\n')) *(strchr(sbuf,'\n')) = 0;
				    dc_info->Type = (pContentType)xhLookup(&(OSYS.Types),sbuf);
				    if (dc_info->Type) 
					drv = (pObjDriver)xhLookup(&(OSYS.DriverTypes),sbuf);
				    }
				fdClose(fd,0);
				}
			    }
			}
		    *slash_ptr = '/';
		    }

		/** Was not a directory and still didn't find driver? **/
		if (!drv)
		    {
		    /** Get default plain file driver. **/
		    dc_info->Flags &= ~DC_F_ISDIR;
		    dc_info->Type = (pContentType)xhLookup(&(OSYS.Types),"system/directory");
		    if (dc_info->Type)
			drv = (pObjDriver)xhLookup(&(OSYS.DriverTypes),"system/directory");
		    }
		}
	    else
		{
		/** Was a directory.  Get directory driver. **/
		dc_info->Flags |= DC_F_ISDIR;
		dc_info->Type = (pContentType)xhLookup(&(OSYS.Types),"system/file");
		if (dc_info->Type)
		    drv = (pObjDriver)xhLookup(&(OSYS.DriverTypes),"system/file");
		}
	    }

    return drv;
    }
#endif


/*** obj_internal_NormalizePath - construct a completed normalized path
 *** from the given path and the given current working directory.
 ***/
pPathname
obj_internal_NormalizePath(char* cwd, char* name)
    {
    char buf[256];
    char* ptr;
    char* bufptr;
    pPathname path;
    int l;

    	/** Allocate the path structure **/
	path = (pPathname)nmMalloc(sizeof(Pathname));
	path->OpenCtlBuf = NULL;
	if (!path) return NULL;

	/** Initialize structure and incremental pointer **/
        bufptr = path->Pathbuf+1;
        strcpy(path->Pathbuf,".");
	path->nElements = 1;
	path->Elements[0] = path->Pathbuf;

    	/** Process through the CWD first if relative path. **/
	if (name[0] != '/')
	    {
            strcpy(buf,cwd);
            ptr = strtok(buf,"/");
            while(ptr)
                {
                /** If '..', then we back up one pathname element **/
                if (!strcmp(ptr,"..")) 
                    {
                    path->nElements--;
                    if (path->nElements < 1) 
                        {
                        nmFree(path,sizeof(Pathname));
			mssError(1,"OSML","Illegal attempt to reference parent of ObjectSystem root");
                        return NULL;
                        }
                    bufptr = strrchr(path->Pathbuf,'/');
                    *bufptr = 0;
                    }
                
                /** If not '.', add it to the path **/
                else if (strcmp(ptr,"."))
                    {
                    l = strlen(ptr);
		    if ((bufptr - path->Pathbuf) + l + 1 > 255)
		        {
			nmFree(path,sizeof(Pathname));
			mssError(1,"OSML","Pathname length exceeds internal limits");
			return NULL;
			}
                    *(bufptr++) = '/';
                    path->Elements[path->nElements++] = bufptr;
                    memcpy(bufptr,ptr,l);
                    bufptr = bufptr+l;
                    *bufptr=0;
                    }
    
                /** Get next path element. **/
                ptr = strtok(NULL,"/");
                }
	    }

	/** Now add the normal filename requested. **/
        strcpy(buf,name);
        ptr = strtok(buf,"/");
        while(ptr)
            {
            /** If '..', then we back up one pathname element **/
            if (!strcmp(ptr,"..")) 
                {
                path->nElements--;
                if (path->nElements < 1) 
                    {
                    nmFree(path,sizeof(Pathname));
		    mssError(1,"OSML","Illegal attempt to reference parent of ObjectSystem root");
                    return NULL;
                    }
                bufptr = strrchr(path->Pathbuf,'/');
                *bufptr = 0;
                }
            
            /** If not '.', add it to the path **/
            else if (strcmp(ptr,"."))
                {
                l = strlen(ptr);
		if ((bufptr - path->Pathbuf) + l + 1 > 255)
		    {
		    nmFree(path,sizeof(Pathname));
		    mssError(1,"OSML","Pathname length exceeds internal limits");
		    return NULL;
		    }
                *(bufptr++) = '/';
                path->Elements[path->nElements++] = bufptr;
                memcpy(bufptr,ptr,l);
                bufptr = bufptr+l;
                *bufptr=0;
                }

            /** Get next path element. **/
            ptr = strtok(NULL,"/");
            }

    return path;
    }


/*** obj_internal_PathPart - returns a pointer to a portion of the path,
 *** determined by start element and length (0 if length = all).  This
 *** modifies Pathbuf in place, so the caller must be aware of this.
 ***/
char*
obj_internal_PathPart(pPathname path, int start_element, int length)
    {
    int i;

    	/** Off end of path? **/
	if (start_element >= path->nElements) return NULL;

	/** Restricted length? **/
	if (length != 0)
	    {
    	    /** Mask/unmask zeros where the / slashes are. **/
	    for(i=start_element+1;i<start_element+length && i<path->nElements;i++)
	        {
		path->Elements[i][-1] = '/';
		}
	    if (start_element+length < path->nElements)
	        {
		path->Elements[start_element+length][-1] = '\0';
		}
	    }
	else
	    {
    	    /** Mask/unmask zeros where the / slashes are. **/
	    for(i=start_element+1;i<path->nElements;i++)
	        {
		path->Elements[i][-1] = '/';
		}
	    }

    return path->Elements[start_element];
    }


/*** obj_internal_PathPrefixCnt - returns -1 if the path prefix given 
 *** doesn't prefix the second, 0 if they are the same, and returns
 *** <n> if it is the prefix and the full path has <n> more elements
 *** than the prefix path.
 ***/
int
obj_internal_PathPrefixCnt(pPathname full_path, pPathname prefix)
    {
    char* fp_full;
    char* prefix_full;

	/** Get the pathnames. **/
	fp_full = obj_internal_PathPart(full_path,0,0);
	prefix_full = obj_internal_PathPart(prefix,0,0);

	/** No prefix? **/
	if (strncmp(fp_full, prefix_full, strlen(prefix_full))) return -1;

    return full_path->nElements - prefix->nElements;
    }


/*** obj_internal_CopyPath - copies one pathname structure to another, resetting
 *** the element pointers to point to the correct locations.
 ***/
int
obj_internal_CopyPath(pPathname dest, pPathname src)
    {
    int i,j;
    pStruct new_inf;

    	/** Copy the raw data. **/
	if (dest->OpenCtlBuf) nmSysFree(dest->OpenCtlBuf);
	memcpy(dest,src,sizeof(Pathname));

	/** Alloc a new ctlbuf **/
	if (src->OpenCtlBuf)
	    {
	    dest->OpenCtlBuf = (char*)nmSysMalloc(dest->OpenCtlLen);
	    memcpy(dest->OpenCtlBuf, src->OpenCtlBuf, src->OpenCtlLen);
	    }

	/** Update the pointers **/
	for(i=0;i<dest->nElements;i++) dest->Elements[i] += ((char*)dest - (char*)src);
	for(i=0;i<dest->nElements;i++)
	    {
	    if (dest->OpenCtl[i])
	        {
		dest->OpenCtl[i] = stAllocInf_ne();
		strcpy(dest->OpenCtl[i]->Name, src->OpenCtl[i]->Name);
		for(j=0;j<src->OpenCtl[i]->nSubInf;j++)
		    {
		    new_inf = stAddAttr_ne(dest->OpenCtl[i], src->OpenCtl[i]->SubInf[j]->Name);
		    new_inf->StrAlloc = 0;
		    new_inf->StrVal = src->OpenCtl[i]->SubInf[j]->StrVal + (dest->OpenCtlBuf - src->OpenCtlBuf);
		    }
		}
	    }

    return 0;
    }


#if 00
/*** obj_internal_ProcessPath - handles the preprocessing needs for the
 *** Open, Create, and Delete calls.  Determines the driver, content type,
 *** does the directory cache lookup, and so forth.
 ***/
pObject
obj_internal_ProcessPath(pObjSession session,char* path,int mode,char* type)
    {
    pObject this;
    pDirectoryCache dc_ptr,del;
    int l,i;
    char* ptr;
    struct stat fileinfo;

	/** Go ahead and memory alloc the object **/
	this = (pObject)nmMalloc(sizeof(Object));
	if (!this) return NULL;
	this->ContentPtr = NULL;

	/** Normalize the path, possibly adding the CWD. **/
	this->Pathname = obj_internal_NormalizePath(session->CurrentDirectory, path);
	if (!(this->Pathname)) return NULL;

	/** Ok, start looking in the directory cache for each directory prefix. **/
	for(dc_ptr=NULL,i=this->Pathname->nElements;i;i--)
	    {
	    ptr = obj_internal_PathPart(this->Pathname,0,i);
	    dc_ptr = (pDirectoryCache)xhLookup(&(OSYS.DirectoryCache),ptr);
	    if (dc_ptr) 
		{
		this->SubPtr = i;
		OSYS.UseCnt++;
		dc_ptr->last_use = OSYS.UseCnt;
		break;
		}
	    if (stat(ptr,&fileinfo) == 0)
	        {
	        /** Allocate a directory cache info structure **/
	        dc_ptr = (pDirectoryCache)nmMalloc(sizeof(DirectoryCache));
	        if (!dc_ptr) 
		    {
		    nmFree(this->Pathname,sizeof(Pathname));
		    nmFree(this,sizeof(Object));
		    return NULL;
		    }
	        memset(dc_ptr,0,sizeof(DirectoryCache));

		/** Setup the dc ptr structure **/
		strcpy(dc_ptr->Pathname, ptr);
		this->SubPtr = i;
		memcpy(&(dc_ptr->fileinfo),&fileinfo,sizeof(struct stat));

	        /** Found a top-level file.  Find its driver. **/
	        dc_ptr->Driver = obj_internal_GetDriver(dc_ptr);
	        if (!dc_ptr->Driver)
		    {
		    nmFree(dc_ptr,sizeof(DirectoryCache));
		    nmFree(this->Pathname,sizeof(Pathname));
		    nmFree(this,sizeof(Object));
		    return NULL;
		    }
		break;
		}
	    }

	/** If not cached, look for the driver linkage file. **/
	if (!dc_ptr)
	    {
	    nmFree(this->Pathname,sizeof(Pathname));
	    nmFree(this,sizeof(Object));
	    return NULL;
	    }

	/** Returned directory driver and user requested create? **/
	/** Also make sure user wanted a file _in_ the directory **/
	if ((mode & O_CREAT) && this->SubPtr == this->Pathname->nElements-1 && 
	    (dc_ptr->Flags & DC_F_ISDIR))
	    {
	    /** Ok, try and use user's content type. **/
	    dc_ptr->Driver = (pObjDriver)xhLookup(&(OSYS.DriverTypes),type);
	    if (!dc_ptr->Driver)
	        {
	        nmFree(dc_ptr,sizeof(DirectoryCache));
		nmFree(this->Pathname,sizeof(Pathname));
	        nmFree(this,sizeof(Object));
	        return NULL;
	        }
	    this->SubPtr = this->Pathname->nElements;
	    }

	/** Got the driver. **/
	this->Driver = dc_ptr->Driver;
	this->Type = dc_ptr->Type;

	/** Cache it if new **/
	if (dc_ptr->last_use == 0)
	    {
	    if (OSYS.DirectoryQueue.nItems >= 1024)
	        {
		del = ((pDirectoryCache)(OSYS.DirectoryQueue.Items[0]));
	        xhRemove(&(OSYS.DirectoryCache),del->Pathname);
	        xaRemoveItem(&(OSYS.DirectoryQueue),0);
		nmFree(del,sizeof(DirectoryCache));
	        }
	    dc_ptr->last_use = (OSYS.UseCnt++);
	    xaAddItem(&(OSYS.DirectoryQueue),(char*)dc_ptr);
	    xhAdd(&(OSYS.DirectoryCache), dc_ptr->Pathname, (char*)dc_ptr);
	    }

	/** Driver requested transactions? **/
	if ((this->Driver->Capabilities & OBJDRV_C_TRANS) && OSYS.TransLayer)
	    {
	    this->LowLevelDriver = this->Driver;
	    this->Driver = OSYS.TransLayer;
	    }

	obj_internal_PathPart(this->Pathname,0,0);

    return this;
    }
#endif


/*** objOpen - open an object for access to its content, attributes, and
 *** methods.  Optionally create a new object.  Open 'mode' uses flags
 *** like the UNIX open() call.
 ***/
pObject 
objOpen(pObjSession session, char* path, int mode, int permission_mask, char* type)
    {
    pObject this;

	ASSERTMAGIC(session, MGK_OBJSESSION);

	/** Lookup the path, etc. **/
	/*this = obj_internal_ProcessPath(session, path, mode, type);*/
	this = obj_internal_ProcessOpen(session, path, mode, permission_mask, type);
	if (!this) return NULL;
	this->Mode = mode;
	this->Obj = NULL;
	this->Session = session;
	/*this->LinkCnt = 1;*/

	/** Ok, got the driver.  Now pass along the open() call. **/
	/*this->Data = this->Driver->Open(this,permission_mask,this->Type,type,&(session->Trx));
	if (!this->Data)
	    {
	    obj_internal_FreeObj(this);
	    return NULL;
	    }*/

	/** Setup the structure **/
	xaInit(&(this->Attrs),16);

	/** Add to open objects this session. **/
	xaAddItem(&(session->OpenObjects),(void*)this);

    return this;
    }


/*** objClose - close an open object.  If this is an open attribute, then
 *** we close it but not the object it belongs to.  If the object has open
 *** attributes, we close them before closing the object.
 ***/
int 
objClose(pObject this)
    {

	ASSERTMAGIC(this,MGK_OBJECT);

    	/** LinkCnt will go to zero? **/
	if (this->LinkCnt == 1 && !(this->Flags & OBJ_F_ROOTNODE))
	    {
	    /** Is this an attribute?  If so, tell object that it's closed **/
	    if (this->Obj) 
	        {
	        xaRemoveItem(&(this->Obj->Attrs),xaFindItem(&(this->Obj->Attrs),(void*)this));
	        }

	    /** Any open attributes?   Close em **/
	    while (this->Attrs.nItems > 0) objClose((pObject)(this->Attrs.Items[0]));

	    /** Remove from open objects this session. **/
	    xaRemoveItem(&(this->Session->OpenObjects),
	        xaFindItem(&(this->Session->OpenObjects),(void*)this));

	    /** Dismantle the structure **/
	    if (this->ContentPtr)
	        {
	        xsDeInit(this->ContentPtr);
	        nmFree(this->ContentPtr, sizeof(XString));
	        this->ContentPtr = NULL;
	        }
	    xaDeInit(&(this->Attrs));
	    }
	obj_internal_FreeObj(this);

    return 0;
    }


/*** objLinkTo - link to an open object, which increments the object's link
 *** count.  The object must then be objClose()ed one more time before the
 *** close actually processes.
 ***/
int
objLinkTo(pObject this)
    {
    while(this) 
        {
	ASSERTMAGIC(this,MGK_OBJECT);
	this->LinkCnt++;
	this = this->Prev;
	}
    return 0;
    }


/*** objCreate - creates a new object without actually opening it.  All
 *** attributes are set to default values in accordance with the appropriate
 *** objectsystem driver.
 ***/
int 
objCreate(pObjSession session, char* path, int permission_mask, char* type)
    {
    pObject tmp;

	/** Lookup the directory path. **/
	/*tmp = obj_internal_ProcessPath(session, path, O_CREAT, type);*/
	tmp = obj_internal_ProcessOpen(session, path, O_CREAT | O_EXCL, permission_mask, type);
	if (!tmp) 
	    {
	    mssError(0,"OSML","Failed to create object - pathname invalid");
	    return -1;
	    }

	/** Pass along the create call. **/
	/*if (tmp->Driver->Create(tmp,permission_mask,tmp->Type, type, &(session->Trx)) <0) 
	    {
	    obj_internal_FreeObj(tmp);
	    return -1;
	    }*/

	obj_internal_FreeObj(tmp);

    return 0;
    }


/*** objDelete - deletes an object without having to perform a query and
 *** then a query delete.
 ***/
int 
objDelete(pObjSession session, char* path)
    {
    pObject tmp;

	/** Lookup the directory path. **/
	/*tmp = obj_internal_ProcessPath(session, path, 0, "");*/
	tmp = obj_internal_ProcessOpen(session, path, 0, 0, "");
	if (!tmp) 
	    {
	    mssError(0,"OSML","Failed to delete object - pathname invalid");
	    return -1;
	    }

	/** Pass along the create call. **/
	if (tmp->Driver->Delete(tmp, &(session->Trx)) <0) 
	    {
	    obj_internal_FreeObj(tmp);
	    return -1;
	    }

	obj_internal_FreeObj(tmp);

    return 0;
    }

