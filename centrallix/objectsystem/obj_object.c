#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/xhashqueue.h"
#include "expression.h"
#include "cxlib/magic.h"
#include "htmlparse.h"
#include "cxlib/mtsession.h"
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
		l = (intptr_t)(t1->RelationLevels.Items[i]);
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
    char* mypath_segment;

    	/** Scan through, breaking at '/' characters **/
	mypath_segment = nmSysStrdup(path_segment);
	bufendptr = strchr(pathinfo->Pathbuf,'\0');
	ptr = strtok(mypath_segment,"/");
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
		if (ch == '\0') ch = '_';
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
		    nmSysFree(mypath_segment);
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
			inf->StrVal = (void*)(intptr_t)(pathinfo->OpenCtlCnt+1);
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
	nmSysFree(mypath_segment);

	*bufendptr = '\0';

    return 0;
    }


/*** obj_internal_FreePathStruct - release a pathname structure when it
 *** occurs inside another structure.
 ***/
int
obj_internal_FreePathStruct(pPathname this)
    {
    int i;

    	/** Release the memory for the open-ctl-buf **/
	if (this->OpenCtlBuf) nmSysFree(this->OpenCtlBuf);
	this->OpenCtlBuf = NULL;

	/** Release the subinf structures, if needed. **/
	for(i=0;i<this->nElements;i++) 
	    if (this->OpenCtl[i]) 
		{
		stFreeInf_ne(this->OpenCtl[i]);
		this->OpenCtl[i] = NULL;
		}

    return 0;
    }


/*** obj_internal_FreePath - release a pathname structure, and free its
 *** subcomponents as well.
 ***/
int
obj_internal_FreePath(pPathname this)
    {

    	/** Check link cnt **/
	if ((--this->LinkCnt) > 0) return 0;

	obj_internal_FreePathStruct(this);

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
	this->EvalContext = NULL;
	this->Prev = NULL;
	this->Next = NULL;
	this->ContentPtr = NULL;
	this->Pathname = NULL;
	this->Obj = NULL;
	this->Flags = 0;
	this->Data = NULL;
	this->Type = NULL;
	this->NotifyItem = NULL;
	this->VAttrs = NULL;
	this->ILowLevelDriver = NULL;
	this->TLowLevelDriver = NULL;
	this->Driver = NULL;
	this->AttrExp = NULL;
	this->AttrExpName = NULL;
	this->LinkCnt = 1;
	memset(&(this->AdditionalInfo), 0, sizeof(ObjectInfo));
	xaInit(&(this->Attrs),16);

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
    pObjSession s = this->Session;
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
	        if (del->Data) 
		    {
		    if (del->Flags & OBJ_F_DELETE)
			del->Driver->DeleteObj(del->Data,&(s->Trx));
		    else
			del->Driver->Close(del->Data,&(s->Trx));
		    }
		if (del->AttrExpName) nmSysFree(del->AttrExpName);
		if (del->AttrExp) expFreeExpression(del->AttrExp);
		xaDeInit(&(del->Attrs));
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


/*** obj_internal_TypeFromSfHeader - try to determine the type from an
 *** object with content that is possibly a structure file.  We only return
 *** a type here if the object is a structure file and only if that structure
 *** file has a valid type in its top level group.
 ***/
pContentType
obj_internal_TypeFromSfHeader(pObject obj)
    {
    char read_buf[16];
    int cnt, rval;
    char type_buf[64];
    pContentType type;

	/** Read in the first bit of the object. **/
	if ((cnt=objRead(obj, read_buf, sizeof(read_buf)-1, 0, OBJ_U_SEEK)) <= 0)
	    return NULL;
	read_buf[cnt] = '\0';

	/** Reset file seek pointer **/
	objRead(obj, read_buf, 0, 0, OBJ_U_SEEK);

	/** Is it a structure file? **/
	if (strncmp(read_buf, "$Version=2$", 11) != 0)
	    return NULL;

	/** Ok, looks like a structure file.  Try to parse it enough to find
	 ** the toplevel group type.
	 **/
	rval = stProbeTypeGeneric(obj, objRead, type_buf, sizeof(type_buf));
	objRead(obj, read_buf, 0, 0, OBJ_U_SEEK);
	if (rval < 0)
	    return NULL;

	/** Do we have a valid type? **/
	type = (pContentType)xhLookup(&OSYS.Types, (void*)type_buf);
	if (!type)
	    return NULL;

	/** Is the type a subtype of system/structure? **/
	rval = obj_internal_IsA(type->Name, "system/structure");
	if (rval == OBJSYS_NOT_ISA || rval < 0)
	    return NULL;

    return type;
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
    pContentType apparent_type,ck_type = NULL,orig_ck_type, sf_type;
    pObjDriver drv;
    pDirectoryCache dc = NULL;
    pXHQElement xe;
    char prevname[256];
    int used_openas;
    pObject cached_obj = NULL;
    pObjectInfo obj_info;

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
	        if (inf->StrVal) inf->StrVal = pathinfo->OpenCtlBuf + (((intptr_t)(inf->StrVal)) - 1);
		}
	    }

	/** Make sure supplied name is "*" if using autokeying **/
	if (mode & OBJ_O_AUTONAME)
	    {
	    if (strcmp(pathinfo->Elements[pathinfo->nElements-1],"*"))
		{
		/** name isn't *, or mode didn't have O_CREAT, then no autoname. **/
		/** GRB - don't require O_CREAT at this point 2-jul-2009 **/
		mode &= ~OBJ_O_AUTONAME;
		}
	    else
		{
		/** This is inherent with autoname **/
		mode |= OBJ_O_EXCL;
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
	    xe = xhqLookup(&(s->DirectoryCache), obj_internal_PathPart(pathinfo, 0, j));
	    if (xe)
	        {
		/** Lookup was successful - get the data for it **/
		dc = (pDirectoryCache)xhqGetData(&(s->DirectoryCache), xe, 0);
		this = dc->NodeObj;

		/** Make sure the access mode is what we need **/
		if (((this->Mode & O_ACCMODE) == O_RDONLY && ((mode & O_ACCMODE) == O_RDWR || (mode & O_ACCMODE) == O_WRONLY)) ||
			((this->Mode & O_ACCMODE) == O_WRONLY && ((mode & O_ACCMODE) == O_RDWR || (mode & O_ACCMODE) == O_RDONLY)))
		    {
		    /** Discard cached entry, in hopes of caching the one we have. **/
		    this = NULL;
		    xhqRemove(&(s->DirectoryCache), xe, 0);
		    dc = NULL;
		    break;
		    }

		/** Link to the intermediate object to lock it open. **/
		objLinkTo(this);
		cached_obj = first_obj = this;

		/** Unlink from the directory cache entry in the hashqueue **/
		xhqUnlink(&(s->DirectoryCache), xe, 0);
		dc = NULL;
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
	    }

	/** Now enter the intermediate-open loop, calling relevant drivers according **/
	/** to 1) actual type (from objGetAttrValue(inner_type)), and 2) perceived **/
	/** type (from the filename and extension) **/
	prevname[0] = '\0';
	apparent_type = NULL;
	while(1)
	    {
	    /** Get the name and type from the previous open. **/
	    if (objGetAttrValue(this,"name",DATA_T_STRING,POD(&name)) != 0)
		{
		/** Name might be NULL if Autoname in progress **/
		name = "*";
		}
	    objGetAttrValue(this,"inner_type",DATA_T_STRING,POD(&type));

	    /** If the driver "claimed" the last path element, we're done. **/
	    if (this->SubPtr + this->SubCnt - 1 > this->Pathname->nElements) break;

	    /** Determine the apparent/perceived type from the name **/
	    apparent_type = NULL;

	    /** Do NOT operate based on "apparent type" if the name is the same as the previous **/
	    /** intermediate object's name, and the SubCnt of the previous obj was 1. **/
	    if (!this || this->SubCnt != 1 || strcmp(name, prevname))
	        {
		/** Check for forced-leaf condition -- in that case we don't use the apparent type **/
		obj_info = objInfo(this);
		if (!obj_info || !(obj_info->Flags & OBJ_INFO_F_FORCED_LEAF))
		    {
		    apparent_type = obj_internal_TypeFromName(name);
		    }
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

	    /** If our type is only application/octet-stream, try to determine
	     ** a more specific type by testing to see if the object is a
	     ** structure file (looking for $Version=2$) and pulling the type
	     ** out of the structure file toplevel group.
	     **/
	    if (!strcmp(ck_type->Name, "application/octet-stream"))
		{
		sf_type = obj_internal_TypeFromSfHeader(this);
		if (sf_type)
		    {
		    /** Found a structure file with a valid type in its header.
		     **/
		    ck_type = sf_type;
		    }
		}

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

	    /** Find out what driver handles this type.   Since type was determined from
	     ** name, inner_type of Prev object, and/or ls__type open-as param, ignore
	     ** drivers for now that key off of the outer type. 
	     **/
	    orig_ck_type = ck_type;
	    do  {
	        drv = (pObjDriver)xhLookup(&OSYS.DriverTypes, (void*)ck_type->Name);
		if (!drv || (drv->Capabilities & OBJDRV_C_OUTERTYPE) || (!used_openas && (drv->Capabilities & OBJDRV_C_NOAUTO)))
		    {
		    if (ck_type->IsA.nItems > 0)
		        {
			ck_type = (pContentType)(ck_type->IsA.Items[0]);
			}
		    else
		        {
			/** If the entire path has been used, this is a successful return. **/
			if (this->SubPtr + this->SubCnt > this->Pathname->nElements) break;

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
	    this->Mode = mode;

	    /** Set object type **/
	    if (used_openas) this->Type = ck_type;

	    /** Driver requested transactions? **/
	    if ((this->Driver->Capabilities & OBJDRV_C_TRANS) && OSYS.TransLayer)
	        {
	        this->TLowLevelDriver = this->Driver;
	        this->Driver = OSYS.TransLayer;
	        }

	    /** Driver requested inheritance and caller is OK with it? **/
	    if ((this->Driver->Capabilities & OBJDRV_C_INHERIT) && OSYS.InheritanceLayer && !(mode & OBJ_O_NOINHERIT))
		{
		this->ILowLevelDriver = this->Driver;
		this->Driver = OSYS.InheritanceLayer;
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

	    /** Modify the object mode if not end of path **/
	    if (this->SubPtr + this->SubCnt < this->Pathname->nElements) 
		{
		this->Mode &= ~(OBJ_O_CREAT | OBJ_O_EXCL);
		}
	    }

	/** Set object type **/
	if (name && !this->Type) this->Type = obj_internal_TypeFromName(name);

	/** Cache the upper-level node for this object? **/
	if (this->Prev && this->Prev->Driver != OSYS.RootDriver && 
	    !(this->Prev->TLowLevelDriver == OSYS.RootDriver && this->Prev->Driver == OSYS.TransLayer) && 
	    !(this->Prev->ILowLevelDriver == OSYS.RootDriver && this->Prev->Driver == OSYS.InheritanceLayer) && 
	    !(this->Prev->Flags & OBJ_F_NOCACHE))
	    {
	    /** Only cache if not already cached. **/
	    if (cached_obj != this->Prev)
	        {
		dc = (pDirectoryCache)nmMalloc(sizeof(DirectoryCache));
		dc->NodeObj = this->Prev;
		objLinkTo(this->Prev);
		strcpy(dc->Pathname, obj_internal_PathPart(this->Prev->Pathname, 0, this->SubPtr));
		xe = xhqAdd(&(s->DirectoryCache), dc->Pathname, dc);
		if (!xe)
		    {
		    /** Already cached -- oops!  Probably bug! **/
		    objClose(this->Prev);
		    nmFree(dc, sizeof(DirectoryCache));
		    }
		else
		    {
		    xhqUnlink(&(s->DirectoryCache), xe, 0);
		    }
		}
	    }

	/** Reset the path. **/
	obj_internal_PathPart(this->Pathname,0,0);

    return this;
    }


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
	if (!path) return NULL;
	memset(path, 0, sizeof(Pathname));
	path->OpenCtlBuf = NULL;

	/** Initialize structure and incremental pointer **/
        bufptr = path->Pathbuf+1;
        strcpy(path->Pathbuf,".");
	path->nElements = 1;
	path->Elements[0] = path->Pathbuf;
	path->LinkCnt = 1;

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


/*** obj_internal_AddToPath() - adds a pathname element to an existing
 *** path structure.  Returns the index of the element on success, or
 *** -1 on failure.
 ***/
int
obj_internal_AddToPath(pPathname path, char* new_element)
    {
    int new_index;
    char* pathend_ptr;
    int new_len;

	/** Find end of current pathname text **/
	if (path->nElements == 0)
	    pathend_ptr = path->Pathbuf;
	else
	    pathend_ptr = path->Elements[path->nElements-1] + strlen(path->Elements[path->nElements-1]);

	/** Do we have space in the pathname for another element? **/
	if (path->nElements+1 >= OBJSYS_MAX_ELEMENTS)
	    {
	    mssError(1,"OSML","Path exceeded internal limits - too many subobject names (limit is %d names)", OBJSYS_MAX_ELEMENTS);
	    return -1;
	    }
	new_len = strlen(new_element);
	if ((pathend_ptr - path->Pathbuf) + new_len + 1 >= (OBJSYS_MAX_PATH-1))
	    {
	    mssError(1,"OSML","Path exceeded internal limits - too long (limit is %d characters)", OBJSYS_MAX_PATH-1);
	    return -1;
	    }

	/** Okay, we can append it.  Go for it. **/
	new_index = (path->nElements++);
	if (pathend_ptr != path->Pathbuf) *(pathend_ptr++) = '/';
	path->Elements[new_index] = pathend_ptr;
	path->OpenCtl[new_index] = NULL;
	strcpy(pathend_ptr, new_element);

    return new_index;
    }


/*** obj_internal_RenamePath - renames a single element of a pathname.
 ***/
int
obj_internal_RenamePath(pPathname path, int element_id, char* new_element)
    {
    int orig_len, new_len, offset;
    char* pathend_ptr;
    int i;

	if (element_id <= 0 || element_id >= path->nElements) return -1;
	new_len = strlen(new_element);
	orig_len = strlen(obj_internal_PathPart(path, element_id, 1));

	/** How much space do we need - and do we have it? **/
	offset = new_len - orig_len;
	pathend_ptr = path->Elements[path->nElements-1] + strlen(path->Elements[path->nElements-1]);
	if ((pathend_ptr - path->Pathbuf) + offset >= (OBJSYS_MAX_PATH-1))
	    {
	    mssError(1,"OSML","Pathname exceeded internal limits - too long (limit is %d characters)", OBJSYS_MAX_PATH-1);
	    return -1;
	    }

	/** Put the new name in there **/
	memmove(path->Elements[element_id] + new_len, path->Elements[element_id] + orig_len, pathend_ptr - (path->Elements[element_id] + orig_len) + 1);
	memcpy(path->Elements[element_id], new_element, new_len);
	for(i=element_id + 1; i < path->nElements; i++)
	    path->Elements[i] += offset;

    return 0;
    }


/*** objOpen - open an object for access to its content, attributes, and
 *** methods.  Optionally create a new object.  Open 'mode' uses flags
 *** like the UNIX open() call.
 ***/
pObject 
objOpen(pObjSession session, char* path, int mode, int permission_mask, char* type)
    {
    pObject this;

	ASSERTMAGIC(session, MGK_OBJSESSION);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "objOpen(%p, \"%s\") = ", session, path);

	/** Lookup the path, etc. **/
	this = obj_internal_ProcessOpen(session, path, mode, permission_mask, type);
	if (!this) return NULL;
	this->Obj = NULL;
	this->Session = session;

	/** Add to open objects this session. **/
	xaAddItem(&(session->OpenObjects),(void*)this);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "%p/%s\n", this, this->Driver->Name);

    return this;
    }


/*** objClose - close an open object.  If this is an open attribute, then
 *** we close it but not the object it belongs to.  If the object has open
 *** attributes, we close them before closing the object.
 ***/
int 
objClose(pObject this)
    {
    pObjVirtualAttr va, del;

	ASSERTMAGIC(this,MGK_OBJECT);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "objClose(%p/%3.3s/%s) (LinkCnt now %d)\n", (this), this->Driver->Name, this->Pathname->Pathbuf, this->LinkCnt-1);

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

	    /** Any notify requests open on this? **/
	    while (this->NotifyItem)
		obj_internal_RnDelete(this->NotifyItem);

	    /** Dismantle the structure **/
	    if (this->ContentPtr)
	        {
	        xsDeInit(this->ContentPtr);
	        nmFree(this->ContentPtr, sizeof(XString));
	        this->ContentPtr = NULL;
	        }

	    /** Release virtual attrs **/
	    for(va = this->VAttrs; va;)
		{
		del = va;
		va = va->Next;
		del->FinalizeFn(this->Session, this, del->Name, del->Context);
		nmFree(del, sizeof(ObjVirtualAttr));
		}
	    this->VAttrs = NULL;
	    }
	obj_internal_FreeObj(this);

    return 0;
    }


/*** objLinkTo - link to an open object, which increments the object's link
 *** count.  The object must then be objClose()ed one more time before the
 *** close actually processes.
 ***/
pObject
objLinkTo(pObject this)
    {
    pObject search;

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "objLinkTo(%p/%3.3s/%s) (LinkCnt now %d)\n", this, this->Driver->Name, this->Pathname->Pathbuf, this->LinkCnt+1);

	/** Link to each object in chain **/
	search = this;
	while(search) 
	    {
	    ASSERTMAGIC(search,MGK_OBJECT);
	    search->LinkCnt++;
	    search = search->Prev;
	    }

    return this;
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
    int rval;

	/** Lookup the directory path. **/
	tmp = obj_internal_ProcessOpen(session, path, O_RDWR, 0, "");
	if (!tmp) 
	    {
	    mssError(0,"OSML","Failed to delete object - pathname invalid");
	    return -1;
	    }
	tmp->Obj = NULL;
	tmp->Session = session;

	/** If driver supports newer objDeleteObj call, use that
	 ** instead of objDelete.
	 **/
	if (tmp->Driver->DeleteObj)
	    {
	    /** New driver->DeleteObj() will be called on close **/
	    tmp->Flags |= OBJ_F_DELETE;
	    rval = 0;
	    }
	else
	    {
	    /** Pass along the old version of the delete call. **/
	    rval = tmp->Driver->Delete(tmp, &(session->Trx));
	    tmp->Data = NULL;
	    }

	/** Clean up.  For DeleteObj(), this actually does the work. **/
	if (obj_internal_FreeObj(tmp) < 0)
	    rval = -1;

    return rval;
    }


/*** objInfo - get additional information about an object.  The returned
 *** structure should be considered readonly, valid only while the object
 *** is still open, and need not be freed.  Its value will be overwritten
 *** on subsequent calls to objInfo for the same object.  Returns NULL on
 *** error.
 ***/
pObjectInfo
objInfo(pObject this)
    {
    if (this->Driver->Info)
	if (this->Driver->Info(this->Data, &(this->AdditionalInfo)) < 0)
	    return NULL;
    return &(this->AdditionalInfo);
    }


/*** objDeleteObj - delete an already open object.  This basically marks
 *** the object for deletion at the driver level, and then does a close
 *** operation.  That way, the object only actually gets deleted once 
 *** no one else has it open anymore.
 ***
 *** If this was the last open (linked to object handle) for this object,
 *** the delete happens immediately.  In any event, the caller should
 *** not use the object handle after passing it to this routine.
 ***/
int
objDeleteObj(pObject this)
    {
	ASSERTMAGIC(this,MGK_OBJECT);

	/** Mark it for deletion **/
	if (this->Driver->DeleteObj == NULL)
	    {
	    mssError(1,"OSML","objDeleteObj: %s objects do not support deletion",this->Driver->Name);
	    return -1;
	    }
	this->Flags |= OBJ_F_DELETE;

    return objClose(this);
    }


/*** objGetPathname - return the full pathname of an open object.
 ***/
char*
objGetPathname(pObject this)
    {
    ASSERTMAGIC(this,MGK_OBJECT);
    return (obj_internal_PathPart(this->Pathname, 0, 0) + 1); 
    }

/*((pDirectoryCache)((pObjSession)(OSYS.OpenSessions.Items[0]))->DirectoryCache.Queue.Next->Next->Next->Next->Next->Next->Next->DataPtr)->Pathname*/
void
obj_internal_DumpDC(pObjSession session)
    {
    pXHQElement xe = session->DirectoryCache.Queue.Next;
    pDirectoryCache dc;

	while (xe && xe != &(session->DirectoryCache.Queue))
	    {
	    dc = xe->DataPtr;
	    xe = xe->Next;
	    printf("%s\n", dc->Pathname);
	    }
	    
    return;
    }


