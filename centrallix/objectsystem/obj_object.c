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

/**CVSDATA***************************************************************

    $Id: obj_object.c,v 1.24 2005/10/18 22:48:59 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/objectsystem/obj_object.c,v $

    $Log: obj_object.c,v $
    Revision 1.24  2005/10/18 22:48:59  gbeeley
    - (bugfix) logic for autoname detection of "*" should be done after
      the path is parsed into path elements.

    Revision 1.23  2005/09/24 20:15:43  gbeeley
    - Adding objAddVirtualAttr() to the OSML API, which can be used to add
      an attribute to an object which invokes callback functions to get the
      attribute values, etc.
    - Changing objLinkTo() to return the linked-to object (same thing that
      got passed in, but good for style in reference counting).
    - Cleanup of some memory leak issues in objOpenQuery()

    Revision 1.22  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.21  2005/01/22 06:15:16  gbeeley
    - I broke it.  This fixes it back.

    Revision 1.20  2004/12/31 04:39:51  gbeeley
    - I think this is what we want.  make sure Type is set correctly on
      object open, even when openas is not used.

    Revision 1.19  2004/08/27 01:28:32  jorupp
     * cleaning up some compile warnings

    Revision 1.18  2004/06/22 16:06:53  mmcgill
    Added the flag OBJ_F_NOCACHE, which a driver can set in its xxxOpen call
    to tell OSML not to add the opened object to the Directory Cache.

    Revision 1.17  2004/06/12 04:02:28  gbeeley
    - preliminary support for client notification when an object is modified.
      This is a part of a "replication to the client" test-of-technology.

    Revision 1.16  2004/02/25 19:59:57  gbeeley
    - fixing problem in net_http; nht_internal_GET should not open the
      target_obj when operating in OSML-over-HTTP mode.
    - adding OBJ_O_AUTONAME support to sybase driver.  Uses select max()+1
      approach for integer fields which are left unspecified.

    Revision 1.15  2003/11/12 22:21:39  gbeeley
    - addition of delete support to osml, mq, datafile, and ux modules
    - added objDeleteObj() API call which will replace objDelete()
    - stparse now allows strings as well as keywords for object names
    - sanity check - old rpt driver to make sure it isn't in the build

    Revision 1.14  2003/09/02 15:37:13  gbeeley
    - Added enhanced command line interface to test_obj.
    - Enhancements to v3 report writer.
    - Fix for v3 print formatter in prtSetTextStyle().
    - Allow spec pathname to be provided in the openctl (command line) for
      CSV files.
    - Report writer checks for params in the openctl.
    - Local filesystem driver fix for read-only files/directories.
    - Race condition fix in UX printer osdriver
    - Banding problem workaround installed for image output in PCL.
    - OSML objOpen() read vs. read+write fix.

    Revision 1.13  2003/08/01 15:53:07  gbeeley
    Fix for objDelete on certain types of data sources which rely on the
    writability of obj->Prev during delete operations.

    Revision 1.12  2003/07/07 20:29:21  affert
    Fixed a bug.

    Revision 1.11  2003/05/30 17:39:52  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.10  2003/04/25 04:09:29  gbeeley
    Adding insert and autokeying support to OSML and to CSV datafile
    driver on a limited basis (in rowidkey mode only, which is the only
    mode currently supported by the csv driver).

    Revision 1.9  2003/04/25 02:43:28  gbeeley
    Fixed some object open nuances with node object caching where a cached
    object might be open readonly but we would need read/write.  Added a
    xhandle-based session identifier for future use by objdrivers.

    Revision 1.8  2003/04/24 19:28:12  gbeeley
    Moved the OSML open node object cache to the session level rather than
    global.  Otherwise, the open node objects could be accessed by the
    wrong user in the wrong session context, which is, er, "bad".

    Revision 1.7  2003/04/03 21:41:08  gbeeley
    Fixed xstring modification problem in test_obj as well as const path
    modification problem in the objOpen process.  Both were causing the
    cxsec stuff in xstring to squawk.

    Revision 1.6  2003/03/31 23:23:40  gbeeley
    Added facility to get additional data about an object, particularly
    with regard to its ability to have subobjects.  Added the feature at
    the driver level to objdrv_ux, and to the "show" command in test_obj.

    Revision 1.5  2002/08/10 02:09:45  gbeeley
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
	nmSysFree(mypath_segment);

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
	this->NotifyItem = NULL;
	this->VAttrs = NULL;
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
    int len;

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

	/** Make sure supplied name is "*" if using autokeying **/
	if (mode & OBJ_O_AUTONAME)
	    {
	    if (strcmp(pathinfo->Elements[pathinfo->nElements-1],"*") || !(mode & OBJ_O_CREAT))
		{
		/** name isn't *, or mode didn't have O_CREAT, then no autoname. **/
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
		    break;
		    }

		/** Link to the intermediate object to lock it open. **/
		objLinkTo(this);
		first_obj = this;

		/** Unlink from the directory cache entry in the hashqueue **/
		xhqUnlink(&(s->DirectoryCache), xe, 0);
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
	    if (!dc || dc->NodeObj != this->Prev)
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
	if (!path) return NULL;
	path->OpenCtlBuf = NULL;

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
    int len;


	ASSERTMAGIC(session, MGK_OBJSESSION);

	OSMLDEBUG(OBJ_DEBUG_F_APITRACE, "objOpen(%p, \"%s\") = ", session, path);

	/** Lookup the path, etc. **/
	/*this = obj_internal_ProcessPath(session, path, mode, type);*/
	this = obj_internal_ProcessOpen(session, path, mode, permission_mask, type);
	if (!this) return NULL;
	/*this->Mode = mode;*/ /* GRB ProcessOpen does this for us */
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
	tmp = obj_internal_ProcessOpen(session, path, O_RDWR, 0, "");
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

