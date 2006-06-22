#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include "cxlib/mtask.h"
#include "cxlib/mtlexer.h"
#include "obj.h"
#include "expression.h"
#include "cxlib/mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Creation:	December 8, 1999					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.    */
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/*		--> obj_rootnode.c: implements a pseudo-driver for the	*/
/*		rootnode source of the objectsystem.  Stored in the	*/
/*		->Prev pointer of an Object in a top-level open call.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: obj_rootnode.c,v 1.7 2006/06/22 00:26:00 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/objectsystem/obj_rootnode.c,v $

    $Log: obj_rootnode.c,v $
    Revision 1.7  2006/06/22 00:26:00  gbeeley
    - only open rootnode source file when needed, to avoid exhausting file
      handles when lots of objects are open.

    Revision 1.6  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

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

    Revision 1.4  2002/07/12 21:50:46  gbeeley
    Rootnode driver was stat()ing the default rootnode, not the configured
    one in OSYS.RootPath.  Caused "driver rootnode does not support last
    modification" type errors when rootnode was not in the default
    location.

    Revision 1.3  2002/02/21 06:32:53  jorupp
    updated obj_rootnode.c to use supplied rootnode
    removed warning in obj.h

    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:00:59  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:01  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


static char* attrnames[] =
    {
    "last_modification",
    NULL
    };


typedef struct
    {
    pFile	fd;
    pObject	Obj;
    int		AttrID;
    DateTime	MTime;
    }
    RootData, *pRootData;


/*** rootOpen - open a new instance of the rootnode object.
 ***/
void*
rootOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pRootData inf;

    	/** Open the rootnode file **/
	inf = (pRootData)nmMalloc(sizeof(RootData));
	memset(inf,0,sizeof(RootData));
	inf->fd = NULL;
	inf->Obj = obj;

    	/** Set the object to be a rootnode type. **/
	obj->Flags |= OBJ_F_ROOTNODE;
	obj->Driver = OSYS.RootDriver;
	obj->Data = (void*)inf;

    return (void*)inf;
    }


/*** rootClose - close an instance of the rootnode.
 ***/
int
rootClose(void* inf_v, pObjTrxTree* oxt)
    {
    pRootData inf = (pRootData)inf_v;

    	/** Close the rootnode file **/
	if (inf->fd)
	    {
	    fdClose(inf->fd,0);
	    inf->fd = NULL;
	    }

	/** Free the memory and return **/
	nmFree(inf, sizeof(RootData));

    return 0;
    }


/*** rootRead - read from the rootnode file.
 ***/
int
rootRead(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pRootData inf = (pRootData)inf_v;
    int rval;
    if (!inf->fd)
	{
	inf->fd = fdOpen(OSYS.RootPath, O_RDONLY, 0600);
	if (!inf->fd)
	    {
	    mssErrorErrno(1,"OSML","Could not access rootnode");
	    return -1;
	    }
	}
    rval = fdRead(inf->fd, buffer, cnt, offset, flags);
    if (rval < cnt)
	{
	fdClose(inf->fd, 0);
	inf->fd = NULL;
	}
    return rval;
    }


/*** rootWrite - write to the rootnode file.  Currently disabled.
 ***/
int
rootWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** rootGetFirstAttr - no attributes in root node, so return NULL.
 ***/
char*
rootGetFirstAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pRootData inf = (pRootData)inf_v;
    inf->AttrID = 0;
    return attrnames[inf->AttrID++];
    }


/*** rootGetNextAttr - no attributes in root node, so return NULL.
 ***/
char*
rootGetNextAttr(void* inf_v, pObjTrxTree* oxt)
    {
    pRootData inf = (pRootData)inf_v;
    if (attrnames[inf->AttrID])
        return attrnames[inf->AttrID++];
    else
        return NULL;
    }


/*** rootGetAttrType - return the type of an attribute.
 ***/
int
rootGetAttrType(void* inf_v, char* attr, pObjTrxTree* oxt)
    {

    	/** Name? String. **/
	if (!strcmp(attr,"name")) return DATA_T_STRING;

	/** Content type?  String. **/
	if (!strcmp(attr,"content_type") || !strcmp(attr,"inner_type") || !strcmp(attr,"outer_type")) 
	    return DATA_T_STRING;

	/** Annotation?  String. **/
	if (!strcmp(attr,"annotation")) return DATA_T_STRING;

	if (!strcmp(attr,"last_modification")) return DATA_T_DATETIME;

    return -1;
    }


/*** rootGetAttrValue - return the value of an attribute.
 ***/
int
rootGetAttrValue(void* inf_v, char* attr, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    pRootData inf = (pRootData)inf_v;
    struct tm* t;

	/** Verify the expected data type of the POD **/
	if (!strcmp(attr,"name") || !strcmp(attr,"inner_type") || !strcmp(attr,"outer_type") ||
		!strcmp(attr,"content_type") || !strcmp(attr,"annotation"))
	    {
	    if (datatype != DATA_T_STRING) 
		{
		mssError(1,"ROOT","Type mismatch getting '%s' attribute of rootnode",attr);
		return -1;
		}
	    }

    	/** Name?  Value is "" **/
	if (!strcmp(attr, "name"))
	    {
	    val->String = "";
	    return 0;
	    }

	/** Content type?  Value is root type for inner, system/object outer. **/
	if (!strcmp(attr, "inner_type") || !strcmp(attr, "content_type"))
	    {
	    val->String = OSYS.RootType->Name;
	    return 0;
	    }
	if (!strcmp(attr, "outer_type"))
	    {
	    val->String = "system/object";
	    return 0;
	    }

	/** Annotation?  Describe the OS. **/
	if (!strcmp(attr, "annotation"))
	    {
	    val->String = "OSML internal ObjectSystem root";
	    return 0;
	    }

	/** Last mod time? **/
	if (!strcmp(attr,"last_modification"))
	    {
	    if (datatype != DATA_T_DATETIME) 
		{
		mssError(1,"ROOT","Type mismatch getting '%s' attribute of rootnode",attr);
		return -1;
		}
	    if (inf->MTime.Value == 0)
		{
	        if (stat(OSYS.RootPath,&fileinfo) < 0) return -1;
		t = localtime(&(fileinfo.st_mtime));
		inf->MTime.Part.Second = t->tm_sec;
		inf->MTime.Part.Minute = t->tm_min;
		inf->MTime.Part.Hour = t->tm_hour;
		inf->MTime.Part.Day = t->tm_mday;
		inf->MTime.Part.Month = t->tm_mon;
		inf->MTime.Part.Year = t->tm_year;
		}
	    *((pDateTime*)val) = &(inf->MTime);
	    return 0;
	    }

    return -1;
    }


/*** rootSetAttrValue - disabled for now, no attributes to set.
 ***/
int
rootSetAttrValue(void* inf_v, char* attr, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** rootInitialize - this function is called during objsys initialization
 *** to install the root driver in the OSYS structure.
 ***/
int
rootInitialize()
    {
    pObjDriver drv;

    	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	memset(drv,0,sizeof(ObjDriver));

	/** Fill in the driver pointers. **/
	drv->Open = rootOpen;
	drv->Close = rootClose;
	drv->Create = NULL;
	drv->Delete = NULL;
	drv->GetFirstAttr = rootGetFirstAttr;
	drv->GetNextAttr = rootGetNextAttr;
	drv->GetAttrType = rootGetAttrType;
	drv->GetAttrValue = rootGetAttrValue;
	drv->SetAttrValue = rootSetAttrValue;
	drv->Read = rootRead;
	drv->Write = rootWrite;
	drv->OpenQuery = NULL;
	drv->QueryDelete = NULL;
	drv->QueryFetch = NULL;
	drv->QueryClose = NULL;
	drv->AddAttr = NULL;
	drv->OpenAttr = NULL;
	drv->GetFirstMethod = NULL;
	drv->GetNextMethod = NULL;
	drv->ExecuteMethod = NULL;
	drv->PresentationHints = NULL;
	strcpy(drv->Name,"OSML RootNode Driver");

	/** Set the root driver in the OSYS structure **/
	OSYS.RootDriver = drv;

    return 0;
    }
