#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "mtsession.h"
#include "centrallix.h"
#include "mtlexer.h"

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
/* Module: 	<driver filename goes here>				*/
/* Author:	<author name goes here>					*/
/* Creation:	<create data goes here>					*/
/* Description:	<replace this> This file is a prototype objectsystem	*/
/*		driver skeleton.  It is used for 'getting started' on	*/
/*		a new objectsystem driver.				*/
/*									*/
/*		To use, some global search/replace must be done.	*/
/*		Replace all occurrenced of mbox, MBOX, and Mbox with your	*/
/*		driver's prefix in the same capitalization.  In vi,	*/
/*									*/
/*			:1,$s/Mbox/Pop/g					*/
/*			:1,$s/MBOX/POP/g					*/
/*			:1,$s/mbox/pop/g					*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_mbox.c,v 1.1 2002/11/06 02:37:59 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_mbox.c,v $

    $Log: objdrv_mbox.c,v $
    Revision 1.1  2002/11/06 02:37:59  jorupp
     * added mailbox (mbox) support (read-only) -- I want to read my mail in centrallix :)

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

    Revision 1.3  2002/07/29 01:18:07  jorupp
     * added the include and calls to build as a module

    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:01:00  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:31:02  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[256];
    int		Flags;
    pObject	Obj;
    int		Mask;
    int		CurAttr;
    XArray	arr;
    int		mnum; /** -1 for mailbox itself, 0-2^31-1 for the actual message **/
    int		nextoffset;
    }
    MboxData, *pMboxData;


#define MBOX(x) ((pMboxData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pMboxData	Data;
    char	NameBuf[256];
    int		ItemCnt;
    }
    MboxQuery, *pMboxQuery;

/*** GLOBALS ***/
struct
    {
    int		dmy_global_variable;
    }
    MBOX_INF;

#define MBOX_DEBUG 0x00
#define MBOX_DEBUG_INIT 0x01
#define MBOX_DEBUG_PARSE 0x02
#define MBOX_DEBUG_READ 0x04
#define MBOX_DEBUG_CLOSE 0x08
#define MBOX_DEBUG_OPEN 0x10
    

/*** mboxOpen - open an object.
 ***/
void*
mboxOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pMboxData inf;
    int rval;
    char* node_path;
    pSnNode node = NULL;
    char* ptr;
#define BUF_SIZE 1024
#define SCHAR 6
    char buf[BUF_SIZE];
    int inbuf;
    int lastread;
    int i;
    int *tmp,*tmp2;

	/** Allocate the structure **/
	inf = (pMboxData)nmMalloc(sizeof(MboxData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(MboxData));
	inf->Obj = obj;
	inf->Mask = mask;

	xaInit(&inf->arr,40);

	obj->SubCnt=1;
	if(MBOX_DEBUG & MBOX_DEBUG_INIT) printf("%s was offered: (%i,%i,%i) %s\n",__FILE__,obj->SubPtr,
		obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));

	ptr = obj_internal_PathPart(obj->Pathname,obj->SubPtr+obj->SubCnt-1,1);
	if(ptr)
	    {
	    i=sscanf(ptr,"msg%i.msg",&inf->mnum);
	    if(i==0)
		inf->mnum=-1;
	    else
		obj->SubCnt++;
	    }
	else
	    inf->mnum=-1;

	if(MBOX_DEBUG & MBOX_DEBUG_INIT) printf("%s took: (%i,%i,%i) %s\n",__FILE__,obj->SubPtr,
		obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));

	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,obj->SubCnt+1));

	if(MBOX_DEBUG & MBOX_DEBUG_INIT) printf("%s is: %s\n",__FILE__,inf->Pathname);


	if(MBOX_DEBUG & MBOX_DEBUG_PARSE) printf("beginning parse of %s\n",inf->Pathname);

	inbuf=lastread=objRead(obj->Prev,buf,BUF_SIZE,0,FD_U_SEEK);

	/** a message starts at the beginning... **/
	if(inbuf>SCHAR)
	    {
	    tmp = (int*)nmMalloc(sizeof(int));
	    *tmp = 0;
	    if(MBOX_DEBUG & MBOX_DEBUG_PARSE)
		printf("found match at: %i\n",*tmp);
	    xaAddItem(&inf->arr,tmp);
	    }

	/** scroll forward, looking for the special characters ('From ' at the beginning of a line) indicating a new message **/
	while(inbuf > SCHAR)
	    {
	    for(i=0;i<inbuf-SCHAR;i++)
		{
		if(!strncmp(&buf[i],"\nFrom ",6))
		    {
		    tmp = (int*)nmMalloc(sizeof(int));
		    *tmp = lastread-inbuf+i+1;
		    if(MBOX_DEBUG & MBOX_DEBUG_PARSE)
			printf("found match at: %i\n",*tmp);
		    xaAddItem(&inf->arr,tmp);
		    }
		}
	    /** since we're looking for SCHAR characters, we need to take the last SCHAR characters 
	     *  and move them to the front, then read more data after them **/
	    memmove(buf,&buf[inbuf-SCHAR],SCHAR);
	    inbuf=objRead(obj->Prev,buf+SCHAR,BUF_SIZE-SCHAR,0,0);
	    lastread+=inbuf;
	    inbuf+=SCHAR;
	    }

	if(MBOX_DEBUG & MBOX_DEBUG_PARSE)
	    printf("done with parse of %s\n",inf->Pathname);
	
    if(MBOX_DEBUG & MBOX_DEBUG_OPEN)
	printf("open returning %p\n",inf);
    return (void*)inf;
    }


/*** mboxClose - close an open object.
 ***/
int
mboxClose(void* inf_v, pObjTrxTree* oxt)
    {
    pMboxData inf = MBOX(inf_v);

    if(MBOX_DEBUG & MBOX_DEBUG_CLOSE)
	printf("closed: %p\n",inf);

	nmFree(inf,sizeof(MboxData));

    return 0;
    }


/*** mboxCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
mboxCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    int fd;
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = mboxOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	mboxClose(inf, oxt);

    return 0;
    }


/*** mboxDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
mboxDelete(pObject obj, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    pMboxData inf, find_inf, search_inf;
    int is_empty = 1;
    int i;

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pMboxData)mboxOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Check to see if user is deleting the 'node object'. **/
	if (obj->Pathname->nElements == obj->SubPtr)
	    {
	    /** Need to do some checking to see if, for example, a non-empty object can't be deleted **/
	    /** YOU WILL NEED TO REPLACE THIS CODE WITH YOUR OWN. **/
	    is_empty = 0;
	    if (!is_empty)
	        {
		mboxClose(inf, oxt);
		mssError(1,"MBOX","Cannot delete: object not empty");
		return -1;
		}
	    }
	else
	    {
	    /** Delete of sub-object processing goes here **/
	    }

	/** Release, don't call close because that might write data to a deleted object **/
	nmFree(inf,sizeof(MboxData));

    return 0;
    }


/*** mboxRead
 ***/
int
mboxRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pMboxData inf = MBOX(inf_v);
    if(inf->mnum == -1 )
	{
	return -1;
	}
    else
	{
	int *tmp,*tmp2;
	int start,end;
	int i;
	tmp = xaGetItem(&inf->arr,inf->mnum);
	tmp2 = xaGetItem(&inf->arr,inf->mnum+1);
	if(tmp)
	    start=*tmp;
	else
	    start=0;
	if(tmp2)
	    end=*tmp2;
	else
	    end=-1;

	if(flags & FD_U_SEEK)
	    {
	    if(MBOX_DEBUG & MBOX_DEBUG_READ)
		printf("set seek to: %i\n",offset);
	    inf->nextoffset=offset;
	    }
	
	/** if there is no end, don't override the passed in maxcnt **/
	if(end>0 && maxcnt>end-start-inf->nextoffset)
	    maxcnt=end-start-inf->nextoffset;

	/** seek to the right spot and grab the data **/
	i=objRead(inf->Obj->Prev,buffer,maxcnt,start+inf->nextoffset,FD_U_SEEK);

	/** update the offset **/
	inf->nextoffset+=i;

	if(MBOX_DEBUG & MBOX_DEBUG_READ)
	    printf("read: %p %i %i %i\n",inf,maxcnt,inf->nextoffset,i);

	return i;
	}
    }


/*** mboxWrite - Don't want to try dynamic resize :)
 ***/
int
mboxWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pMboxData inf = MBOX(inf_v);
    return -1;
    }


/*** mboxOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
mboxOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pMboxData inf = MBOX(inf_v);
    pMboxQuery qy;

	/** only do a query if this is the 'mailbox' object, not a message **/
	if(inf->mnum != -1)
	    return NULL;

	/** Allocate the query structure **/
	qy = (pMboxQuery)nmMalloc(sizeof(MboxQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(MboxQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;
    
    return (void*)qy;
    }


/*** mboxQueryFetch - get the next directory entry as an open object.
 ***/
void*
mboxQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pMboxQuery qy = ((pMboxQuery)(qy_v));
    pMboxData inf;
    char name[64];
    int i;
    int *tmp;
    int *tmp2;
    char *ptr;

	/** I _think_ these should be this way... **/
	obj->SubPtr=qy->Data->Obj->SubPtr;
	obj->SubCnt=qy->Data->Obj->SubCnt;
	
	/** if there are no more entries, return NULL **/
	if(qy->ItemCnt>=qy->Data->arr.nItems)
	    return NULL;

	/** Build the filename. **/
	snprintf(name,64,"msg%i.msg",qy->ItemCnt);

	/** Shamelessly stolen from objdrv_sybase.c :) **/
	ptr = memchr(obj->Pathname->Elements[obj->Pathname->nElements-1],'\0',256);
	if ((ptr - obj->Pathname->Pathbuf) + 1 + strlen(name) >= 255)
	    {
	    mssError(1,"MBOX","Pathname too long for internal representation");
	    nmFree(inf,sizeof(MboxData));
	    return NULL;
	    }
	*(ptr++) = '/';
	strcpy(ptr,name);
	obj->Pathname->Elements[obj->Pathname->nElements++] = ptr;
	obj->SubCnt++;

	/** Alloc the structure **/
	inf = (pMboxData)nmMalloc(sizeof(MboxData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(MboxData));
	strcpy(inf->Pathname, obj->Pathname->Pathbuf);
	inf->Obj = obj;

	/** copy the array of offsets **/
	xaInit(&inf->arr,40);
	for(i=0;i<qy->Data->arr.nItems;i++)
	    {
	    tmp = nmMalloc(sizeof(int));
	    tmp2 = xaGetItem(&qy->Data->arr,i);
	    *tmp=*tmp2;
	    xaAddItem(&inf->arr,tmp);
	    }
	inf->mnum=qy->ItemCnt;


	qy->ItemCnt++;

    if(MBOX_DEBUG & MBOX_DEBUG_OPEN)
	printf("QueryFetch returning %p\n",inf);
    return (void*)inf;
    }


/*** mboxQueryClose - close the query.
 ***/
int
mboxQueryClose(void* qy_v, pObjTrxTree* oxt)
    {

	/** Free the structure **/
	nmFree(qy_v,sizeof(MboxQuery));

    return 0;
    }


/*** mboxGetAttrType - get the type (DATA_T_mbox) of an attribute by name.
 ***/
int
mboxGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pMboxData inf = MBOX(inf_v);
    int i;
    pStructInf find_inf;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

	/** outer_type and inner_type are strings **/
	if (!strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"inner_type")) return DATA_T_STRING;

	/** Check for attributes in the node object if that was opened **/
	if (inf->Obj->Pathname->nElements == inf->Obj->SubPtr)
	    {
	    }

	/** Put checking for your own attributes here. **/
	/** You will want to likely make a list of 'em in a global array **/
	/** or something like that. **/

    return -1;
    }


/*** mboxGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
mboxGetAttrValue(void* inf_v, char* attrname, int datatype, void* val, pObjTrxTree* oxt)
    {
    pMboxData inf = MBOX(inf_v);
    pStructInf find_inf;
    char* ptr;
    int i;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    *((char**)val) = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
	    return 0;
	    }
	
	/** If outer-type, return as appropriate **/
	if (!strcmp(attrname,"outer_type"))
	    {
	    *((char**)val) = "system/mailbox";
	    return 0;
	    }

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    *(char**)val = "";
	    return 0;
	    }

	if (!strcmp(attrname,"inner_type") || !strcmp(attrname,"inner_type") )
	    {
	    if(inf->mnum==-1)
		*((char**)val) = "system/void";
	    else
		*((char**)val) = "message/rfc822";
	    return 0;
	    }


	mssError(1,"MBOX","Could not locate requested attribute: %s",attrname);

    return -1;
    }


/*** mboxGetNextAttr - get the next attribute name for this object.
 ***/
char*
mboxGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pMboxData inf = MBOX(inf_v);

	/** REPLACE THE IF(0) WITH A CONDITION IF THERE ARE MORE ATTRS **/
	if (0)
	    {
	    /** PUT YOUR ATTRIBUTE-NAME RETURN STUFF HERE. **/
	    inf->CurAttr++;
	    }

    return NULL;
    }


/*** mboxGetFirstAttr - get the first attribute name for this object.
 ***/
char*
mboxGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pMboxData inf = MBOX(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = mboxGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** mboxSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
mboxSetAttrValue(void* inf_v, char* attrname, int datatype, void* val, pObjTrxTree oxt)
    {
    pMboxData inf = MBOX(inf_v);
    pStructInf find_inf;

	/** Choose the attr name **/
	/** Changing name of node object? **/
	if (!strcmp(attrname,"name"))
	    {
	    if (inf->Obj->Pathname->nElements == inf->Obj->SubPtr)
	        {
	        if (!strcmp(inf->Obj->Pathname->Pathbuf,".")) return -1;
	        if (strlen(inf->Obj->Pathname->Pathbuf) - 
	            strlen(strrchr(inf->Obj->Pathname->Pathbuf,'/')) + 
		    strlen(*(char**)(val)) + 1 > 255)
		    {
		    mssError(1,"MBOX","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,*(char**)(val));
	        if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssError(1,"MBOX","SetAttr 'name': could not rename structure file node object");
		    return -1;
		    }
	        strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname);
		}
	    return 0;
	    }

	/** Set content type if that was requested. **/
	if (!strcmp(attrname,"content_type"))
	    {
	    /** SET THE TYPE HERE, IF APPLICABLE, AND RETURN 0 ON SUCCESS **/
	    return -1;
	    }

	/** DO YOUR SEARCHING FOR ATTRIBUTES TO SET HERE **/

    return 0;
    }


/*** mboxAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
mboxAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    pMboxData inf = MBOX(inf_v);
    pStructInf new_inf;
    char* ptr;

    return -1;
    }


/*** mboxOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
mboxOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** mboxGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
mboxGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** mboxGetNextMethod -- same as above.  Always fails. 
 ***/
char*
mboxGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** mboxExecuteMethod - No methods to execute, so this fails.
 ***/
int
mboxExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** mboxInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
mboxInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&MBOX_INF,0,sizeof(MBOX_INF));
	MBOX_INF.dmy_global_variable = 0;

	/** Setup the structure **/
	strcpy(drv->Name,"MBOX - a mailbox driver");		/** <--- PUT YOUR DESCRIPTION HERE **/
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"system/mailbox");	/** <--- PUT YOUR OBJECT/TYPE HERE **/

	/** Setup the function references. **/
	drv->Open = mboxOpen;
	drv->Close = mboxClose;
	drv->Create = mboxCreate;
	drv->Delete = mboxDelete;
	drv->OpenQuery = mboxOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = mboxQueryFetch;
	drv->QueryClose = mboxQueryClose;
	drv->Read = mboxRead;
	drv->Write = mboxWrite;
	drv->GetAttrType = mboxGetAttrType;
	drv->GetAttrValue = mboxGetAttrValue;
	drv->GetFirstAttr = mboxGetFirstAttr;
	drv->GetNextAttr = mboxGetNextAttr;
	drv->SetAttrValue = mboxSetAttrValue;
	drv->AddAttr = mboxAddAttr;
	drv->OpenAttr = mboxOpenAttr;
	drv->GetFirstMethod = mboxGetFirstMethod;
	drv->GetNextMethod = mboxGetNextMethod;
	drv->ExecuteMethod = mboxExecuteMethod;
	drv->PresentationHints = NULL;

	nmRegister(sizeof(MboxData),"MboxData");
	nmRegister(sizeof(MboxQuery),"MboxQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

MODULE_INIT(mboxInitialize);
MODULE_PREFIX("mbox");
MODULE_DESC("MBOX ObjectSystem Driver");
MODULE_VERSION(0,0,1);
MODULE_IFACE(CX_CURRENT_IFACE);

