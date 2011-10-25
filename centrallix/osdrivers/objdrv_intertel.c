#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "cxlib/mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	objdrv_intertel.c					*/
/* Author:	Greg Beeley (GRB)      					*/
/* Creation:	02-January-2001        					*/
/* Description:	Objectsystem driver to interface with a local serial	*/
/*		port that is wired to the OAI serial port on an		*/
/*		Inter-Tel AXXESS phone switch with "OAI Events" and	*/
/*		"OAI 3rd Party Call Control" enabled in the premium	*/
/*		features selection.					*/
/*									*/
/*		This driver tracks (and has pseudo-subobjects for)	*/
/*		raw events, stations, active calls, call history, and	*/
/*		trunks.							*/
/*									*/
/*		Assumption: serial port will be configured for hardware	*/
/*		flow control, 9600 baud, 8N1, min 1 time 5		*/
/************************************************************************/


#define ITL_MAX_EVENTS	1024	/* Max events to keep track of per node */

/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[256];
    int		Flags;
    pObject	Obj;
    int		Mask;
    int		CurAttr;
    pSnNode	Node;
    int		Type;		/* ITL_T_xxx object types */
    }
    ItlData, *pItlData;

#define ITL_T_PBX	1	/* Top-level - the switch itself */
#define ITL_T_RAWEVENTS	2	/* List of last <n> raw events */
#define ITL_T_STATIONS	3	/* List of stations (phones) */
#define ITL_T_CALLS	4	/* Active calls */
#define ITL_T_CALLHIST	5	/* Call history, including active */
#define ITL_T_TRUNKS	6	/* Trunks (phone lines, T1, etc) */
#define ITL_T_EVENT	7	/* one raw event */
#define ITL_T_STATION	8	/* one station */
#define ITL_T_CALL	9	/* one call, active or history */
#define ITL_T_TRUNK	10	/* one trunk */


#define ITL(x) ((pItlData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pItlData	Data;
    char	NameBuf[256];
    int		ItemCnt;
    }
    ItlQuery, *pItlQuery;

/*** GLOBALS ***/
struct
    {
    int		dmy_global_variable;
    }
    ITL_INF;


/*** itlOpen - open an object.
 ***/
void*
itlOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pItlData inf;
    int rval;
    char* node_path;
    pSnNode node = NULL;
    char* ptr;

	/** Allocate the structure **/
	inf = (pItlData)nmMalloc(sizeof(ItlData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(ItlData));
	inf->Obj = obj;
	inf->Mask = mask;

	/** Determine the node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

	/** If CREAT and EXCL, we only create, failing if already exists. **/
	if ((obj->Mode & O_CREAT) && (obj->Mode & O_EXCL) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(node_path, usrtype);
	    if (!node)
	        {
		nmFree(inf,sizeof(ItlData));
		mssError(0,"ITL","Could not create new node object");
		return NULL;
		}
	    }
	
	/** Otherwise, try to open it first. **/
	if (!node)
	    {
	    node = snReadNode(node_path);
	    }

	/** If no node, and user said CREAT ok, try that. **/
	if (!node && (obj->Mode & O_CREAT) && (obj->SubPtr == obj->Pathname->nElements))
	    {
	    node = snNewNode(node_path, usrtype);
	    }

	/** If _still_ no node, quit out. **/
	if (!node)
	    {
	    nmFree(inf,sizeof(ItlData));
	    mssError(0,"ITL","Could not open structure file");
	    return NULL;
	    }

	/** Set object params. **/
	inf->Node = node;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));
	inf->Node->OpenCnt++;

    return (void*)inf;
    }


/*** itlClose - close an open object.
 ***/
int
itlClose(void* inf_v, pObjTrxTree* oxt)
    {
    pItlData inf = ITL(inf_v);

    	/** Write the node first, if need be. **/
	snWriteNode(inf->Node);
	
	/** Release the memory **/
	inf->Node->OpenCnt --;
	nmFree(inf,sizeof(ItlData));

    return 0;
    }


/*** itlCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
itlCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    int fd;
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = itlOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	itlClose(inf, oxt);

    return 0;
    }


/*** itlDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
itlDelete(pObject obj, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    pItlData inf, find_inf, search_inf;
    int is_empty = 1;
    int i;

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pItlData)itlOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Check to see if user is deleting the 'node object'. **/
	if (obj->Pathname->nElements == obj->SubPtr)
	    {
	    if (inf->Node->OpenCnt > 1) 
	        {
		itlClose(inf, oxt);
		mssError(1,"ITL","Cannot delete structure file: object in use");
		return -1;
		}

	    /** Need to do some checking to see if, for example, a non-empty object can't be deleted **/
	    /** YOU WILL NEED TO REPLACE THIS CODE WITH YOUR OWN. **/
	    is_empty = 0;
	    if (!is_empty)
	        {
		itlClose(inf, oxt);
		mssError(1,"ITL","Cannot delete: object not empty");
		return -1;
		}
	    stFreeInf(inf->Node->Data);

	    /** Physically delete the node, and then remove it from the node cache **/
	    unlink(inf->Node->NodePath);
	    snDelete(inf->Node);
	    }
	else
	    {
	    /** Delete of sub-object processing goes here **/
	    }

	/** Release, don't call close because that might write data to a deleted object **/
	nmFree(inf,sizeof(ItlData));

    return 0;
    }


/*** itlRead - Structure elements have no content.  Fails.
 ***/
int
itlRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pItlData inf = ITL(inf_v);
    return -1;
    }


/*** itlWrite - Again, no content.  This fails.
 ***/
int
itlWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pItlData inf = ITL(inf_v);
    return -1;
    }


/*** itlOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
itlOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pItlData inf = ITL(inf_v);
    pItlQuery qy;

	/** Allocate the query structure **/
	qy = (pItlQuery)nmMalloc(sizeof(ItlQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(ItlQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;
    
    return (void*)qy;
    }


/*** itlQueryFetch - get the next directory entry as an open object.
 ***/
void*
itlQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pItlQuery qy = ((pItlQuery)(qy_v));
    pItlData inf;
    char* new_obj_name = "newobj";

    	/** PUT YOUR OBJECT-QUERY-RETRIEVAL STUFF HERE **/
	/** RETURN NULL IF NO MORE ITEMS. **/
	return NULL;

	/** Build the filename. **/
	/** REPLACE NEW_OBJ_NAME WITH YOUR NEW OBJECT NAME OF THE OBJ BEING FETCHED **/
	if (strlen(new_obj_name) + 1 + strlen(qy->Data->Obj->Pathname->Pathbuf) > 255) 
	    {
	    mssError(1,"ITL","Query result pathname exceeds internal representation");
	    return NULL;
	    }
	sprintf(obj->Pathname->Pathbuf,"%s/%s",qy->Data->Obj->Pathname->Pathbuf,new_obj_name);

	/** Alloc the structure **/
	inf = (pItlData)nmMalloc(sizeof(ItlData));
	if (!inf) return NULL;
	strcpy(inf->Pathname, obj->Pathname->Pathbuf);
	inf->Node = qy->Data->Node;
	inf->Node->OpenCnt++;
	inf->Obj = obj;
	qy->ItemCnt++;

    return (void*)inf;
    }


/*** itlQueryClose - close the query.
 ***/
int
itlQueryClose(void* qy_v, pObjTrxTree* oxt)
    {

	/** Free the structure **/
	nmFree(qy_v,sizeof(ItlQuery));

    return 0;
    }


/*** itlGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
itlGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pItlData inf = ITL(inf_v);
    int i;
    pStructInf find_inf;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"inner_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

	/** Check for attributes in the node object if that was opened **/
	if (inf->Obj->Pathname->nElements == inf->Obj->SubPtr)
	    {
	    }

	/** Put checking for your own attributes here. **/
	/** You will want to likely make a list of 'em in a global array **/
	/** or something like that. **/

    return -1;
    }


/*** itlGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
itlGetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree* oxt)
    {
    pItlData inf = ITL(inf_v);
    pStructInf find_inf;
    char* ptr;
    int i;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    *((char**)val) = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	/** REPLACE MYOBJECT/TYPE WITH AN APPROPRIATE TYPE. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    *((char**)val) = "system/void";
	    return 0;
	    }
	else if (!strcmp(attrname,"outer_type"))
	    {
	    switch(inf->Type)
	        {
		case ITL_T_PBX: *((char**)val) = "application/intertel-oai"; break;
		case ITL_T_EVENT: *((char**)val) = "intertel-oai/event"; break;
		case ITL_T_STATION: *((char**)val) = "intertel-oai/station"; break;
		case ITL_T_CALL: *((char**)val) = "intertel-oai/call"; break;
		case ITL_T_RAWEVENTS: *((char**)val) = "intertel-oai/eventlist"; break;
		case ITL_T_STATIONS: *((char**)val) = "intertel-oai/stationlist"; break;
		case ITL_T_CALLS: *((char**)val) = "intertel-oai/activecalls"; break;
		case ITL_T_CALLHIST: *((char**)val) = "intertel-oai/callhistory"; break;
		case ITL_T_TRUNKS: *((char**)val) = "intertel-oai/trunklist"; break;
		default: return -1;
		}
	    return 0;
	    }

	/** DO YOUR ATTRIBUTE LOOKUP STUFF HERE **/
	/** AND RETURN 0 IF GOT IT OR 1 IF NULL **/
	/** CONTINUE ON DOWN IF NOT FOUND. **/

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    *(char**)val = "";
	    return 0;
	    }

	mssError(1,"ITL","Could not locate requested attribute");

    return -1;
    }


/*** itlGetNextAttr - get the next attribute name for this object.
 ***/
char*
itlGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pItlData inf = ITL(inf_v);

	/** REPLACE THE IF(0) WITH A CONDITION IF THERE ARE MORE ATTRS **/
	if (0)
	    {
	    /** PUT YOUR ATTRIBUTE-NAME RETURN STUFF HERE. **/
	    inf->CurAttr++;
	    }

    return NULL;
    }


/*** itlGetFirstAttr - get the first attribute name for this object.
 ***/
char*
itlGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pItlData inf = ITL(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = itlGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** itlSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
itlSetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree oxt)
    {
    pItlData inf = ITL(inf_v);
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
		    mssError(1,"ITL","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,*(char**)(val));
	        if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssError(1,"ITL","SetAttr 'name': could not rename structure file node object");
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

	/** Set dirty flag **/
	inf->Node->Status = SN_NS_DIRTY;

    return 0;
    }


/*** itlAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
itlAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    pItlData inf = ITL(inf_v);
    pStructInf new_inf;
    char* ptr;

    return -1;
    }


/*** itlOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
itlOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** itlGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
itlGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** itlGetNextMethod -- same as above.  Always fails. 
 ***/
char*
itlGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** itlExecuteMethod - No methods to execute, so this fails.
 ***/
int
itlExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** itlInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
itlInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&ITL_INF,0,sizeof(ITL_INF));
	ITL_INF.dmy_global_variable = 0;

	/** Setup the structure **/
	strcpy(drv->Name,"ITL - Inter-Tel OAI Driver");		/** <--- PUT YOUR DESCRIPTION HERE **/
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"application/intertel-oai");	/** <--- PUT YOUR OBJECT/TYPE HERE **/

	/** Setup the function references. **/
	drv->Open = itlOpen;
	drv->Close = itlClose;
	drv->Create = itlCreate;
	drv->Delete = itlDelete;
	drv->OpenQuery = itlOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = itlQueryFetch;
	drv->QueryClose = itlQueryClose;
	drv->Read = itlRead;
	drv->Write = itlWrite;
	drv->GetAttrType = itlGetAttrType;
	drv->GetAttrValue = itlGetAttrValue;
	drv->GetFirstAttr = itlGetFirstAttr;
	drv->GetNextAttr = itlGetNextAttr;
	drv->SetAttrValue = itlSetAttrValue;
	drv->AddAttr = itlAddAttr;
	drv->OpenAttr = itlOpenAttr;
	drv->GetFirstMethod = itlGetFirstMethod;
	drv->GetNextMethod = itlGetNextMethod;
	drv->ExecuteMethod = itlExecuteMethod;
	drv->PresentationHints = NULL;

	nmRegister(sizeof(ItlData),"ItlData");
	nmRegister(sizeof(ItlQuery),"ItlQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

