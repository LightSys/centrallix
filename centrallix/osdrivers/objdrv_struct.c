#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "mtsession.h"

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
/* Module: 	objdrv_struct.c     					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 19, 1998					*/
/* Description:	Structure object driver -- used primarily for storing	*/
/*		application structure information.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: objdrv_struct.c,v 1.1 2001/08/13 18:01:09 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_struct.c,v $

    $Log: objdrv_struct.c,v $
    Revision 1.1  2001/08/13 18:01:09  gbeeley
    Initial revision

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:31:08  gbeeley
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
    pStructInf	Data;
    pSnNode	Node;
    IntVec	IVvalue;
    StringVec	SVvalue;
    }
    StxData, *pStxData;


#define STX(x) ((pStxData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pStxData	Data;
    char	NameBuf[256];
    int		ItemCnt;
    pStructInf	CurInf;
    }
    StxQuery, *pStxQuery;

/*** GLOBALS ***/
struct
    {
    int		dmy;
    }
    STX_INF;


/*** stxOpen - open a file or directory.
 ***/
void*
stxOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pStxData inf;
    char* node_path;
    char* endptr;
    pSnNode node = NULL;
    pStructInf search_inf, find_inf, attr_inf;
    int i,j,n;
    char* ptr;

	/** Allocate the structure **/
	inf = (pStxData)nmMalloc(sizeof(StxData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(StxData));
	inf->Obj = obj;
	inf->Mask = mask;

	/** Determine the node path **/
	node_path = obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr);

	/** Check node access.  IF newly created node object, handle differently. **/
	if ((obj->Prev->Flags & OBJ_F_CREATED) && (obj->Mode & O_CREAT))
	    {
	    /** Make the node **/
	    node = snNewNode(obj->Prev,"system/structure");
	    if (!node)
	        {
		mssError(0,"STX","Could not create new structure file");
		nmFree(inf, sizeof(StxData));
		return NULL;
		}

	    /** Any parameters to set? **/
	    if (obj->Pathname->OpenCtl[obj->SubPtr])
	        {
		for(i=0;i<obj->Pathname->OpenCtl[obj->SubPtr]->nSubInf;i++)
		    {
		    search_inf = obj->Pathname->OpenCtl[obj->SubPtr]->SubInf[i];
		    if (strncmp(search_inf->Name,"ls__",4) && search_inf->StrVal[0])
		        {
			attr_inf = stAddAttr(node->Data, search_inf->Name);
			endptr = NULL;
			n = strtol(search_inf->StrVal[0],&endptr,0);
			if (endptr && *endptr == '\0')
			    stAddValue(attr_inf, NULL, n);
			else 
			    stAddValue(attr_inf, nmSysStrdup(search_inf->StrVal[0]), 0);
			}
		    }
		}

	    /** Write the node. **/
	    snWriteNode(obj->Prev, node);
	    }
	else
	    {
	    /** Open an existing node. **/
	    node = snReadNode(obj->Prev);
	    if (node && (obj->Mode & O_CREAT) && (obj->Mode & O_EXCL) && obj->SubPtr == obj->Pathname->nElements)
	        {
		mssError(0,"STX","Structure file already exists");
		nmFree(inf, sizeof(StxData));
		return NULL;
		}
	    if (!node)
	        {
		mssError(0,"STX","Could not read structure file");
		nmFree(inf, sizeof(StxData));
		return NULL;
		}
	    }

	/** Search down the struct tree if we opened a sub-structure. **/
	search_inf = node->Data;
	obj->SubCnt = obj->Pathname->nElements - obj->SubPtr + 1;
	for(i=obj->SubPtr;i<obj->Pathname->nElements;i++)
	    {
	    ptr = obj_internal_PathPart(obj->Pathname,i,1);
	    find_inf = NULL;
	    for(j=0;j<search_inf->nSubInf;j++) if (!strcmp(ptr,search_inf->SubInf[j]->Name) && search_inf->SubInf[j]->Type == ST_T_SUBGROUP)
	        {
		if (i == obj->Pathname->nElements-1 && (obj->Mode & O_CREAT) && (obj->Mode & O_EXCL))
		    {
		    nmFree(inf,sizeof(StxData));
		    mssError(1,"STX","Structure file sub-group already exists");
		    return NULL;
		    }
		find_inf = search_inf->SubInf[j];
		break;
		}
	    if (!find_inf && i == obj->Pathname->nElements-1 && (obj->Mode & O_CREAT))
	        {
		find_inf = stAddGroup(search_inf, ptr, usrtype);
		node->Status = SN_NS_DIRTY;
		}
	    else if (!find_inf)
	        {
		nmFree(inf,sizeof(StxData));
		mssError(1,"STX","Structure file sub-group does not exist");
		return NULL;
		}
	    search_inf = find_inf;

	    /** Stop searching because the current level is final? **/
	    if (stAttrValue(stLookup(search_inf,"final"),NULL,&ptr,0) == 0 && !strcasecmp(ptr,"yes"))
	        {
		obj->SubCnt = i - obj->SubPtr + 1;
	        break;
		}
	    }

	/** Set object params. **/
	inf->Node = node;
	inf->Data = search_inf;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));
	inf->Node->OpenCnt++;

    return (void*)inf;
    }


/*** stxClose - close an open file or directory.
 ***/
int
stxClose(void* inf_v, pObjTrxTree* oxt)
    {
    pStxData inf = STX(inf_v);

    	/** Write the node first **/
	snWriteNode(inf->Obj->Prev, inf->Node);
	
	/** Release the memory **/
	inf->Node->OpenCnt --;
	nmFree(inf,sizeof(StxData));

    return 0;
    }


/*** stxCreate - create a new file without actually opening that 
 *** file.
 ***/
int
stxCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = stxOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	stxClose(inf, oxt);

    return 0;
    }


/*** stxDelete - delete an existing file or directory.
 ***/
int
stxDelete(pObject obj, pObjTrxTree* oxt)
    {
    pStxData inf;
    int is_empty = 1;
    int i;

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pStxData)stxOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** If a subinf, delete that subtree only if empty. **/
	if (inf->Data != inf->Node->Data)
	    {
	    for(i=0;i<inf->Data->nSubInf;i++)
	        {
		if (inf->Data->SubInf[i]->Type == ST_T_SUBGROUP)
		    {
		    is_empty = 0;
		    break;
		    }
		}
	    if (!is_empty)
	        {
		stxClose(inf, oxt);
		mssError(1,"STX","Cannot delete structure file subgroup: not empty");
		return -1;
		}
	    stFreeInf(inf->Data);
	    inf->Node->Status = SN_NS_DIRTY;
	    snWriteNode(inf->Obj->Prev,inf->Node);
	    inf->Node->OpenCnt--;
	    }
	/** Otherwise, if not a subinf, delete the whole file if not empty **/
	else
	    {
	    if (inf->Node->OpenCnt > 1) 
	        {
		stxClose(inf, oxt);
		mssError(1,"STX","Cannot delete structure file: object in use");
		return -1;
		}
	    for(i=0;i<inf->Data->nSubInf;i++)
	        {
		if (inf->Data->SubInf[i]->Type == ST_T_SUBGROUP)
		    {
		    is_empty = 0;
		    break;
		    }
		}
	    if (!is_empty)
	        {
		stxClose(inf, oxt);
		mssError(1,"STX","Cannot delete structure file: not empty");
		return -1;
		}
	    stFreeInf(inf->Data);
	    unlink(inf->Node->NodePath);
	    snDelete(inf->Node);
	    }

	/** Release, don't call close because that might write data to a deleted object **/
	nmFree(inf,sizeof(StxData));

    return 0;
    }


/*** stxRead - Structure elements have no content.  Fails.
 ***/
int
stxRead(void* inf_v, char* buffer, int maxcnt, int flags, int offset, pObjTrxTree* oxt)
    {
    /*pStxData inf = STX(inf_v);*/
    return -1;
    }


/*** stxWrite - Again, no content.  This fails.
 ***/
int
stxWrite(void* inf_v, char* buffer, int cnt, int flags, int offset, pObjTrxTree* oxt)
    {
    /*pStxData inf = STX(inf_v);*/
    return -1;
    }


/*** stxOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
stxOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pStxData inf = STX(inf_v);
    pStxQuery qy;

	/** Allocate the query structure **/
	qy = (pStxQuery)nmMalloc(sizeof(StxQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(StxQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;
	qy->CurInf = NULL;
    
    return (void*)qy;
    }


/*** stxQueryFetch - get the next directory entry as an open object.
 ***/
void*
stxQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pStxQuery qy = ((pStxQuery)(qy_v));
    pStxData inf;

	/** Find a subgroup item **/
	while(qy->ItemCnt < qy->Data->Data->nSubInf && 
	      qy->Data->Data->SubInf[qy->ItemCnt]->Type != ST_T_SUBGROUP) qy->ItemCnt++;

	/** No more left? **/
	if (qy->ItemCnt >= qy->Data->Data->nSubInf) return NULL;
	qy->CurInf = qy->Data->Data->SubInf[qy->ItemCnt];

	/** Build the filename. **/
	if (strlen(qy->CurInf->Name) + 1 + strlen(qy->Data->Obj->Pathname->Pathbuf) > 255) 
	    {
	    mssError(1,"STX","Query result pathname exceeds internal representation");
	    return NULL;
	    }
	sprintf(obj->Pathname->Pathbuf,"%s/%s",qy->Data->Obj->Pathname->Pathbuf,qy->CurInf->Name);

	/** Alloc the structure **/
	inf = (pStxData)nmMalloc(sizeof(StxData));
	if (!inf) return NULL;
	strcpy(inf->Pathname, obj->Pathname->Pathbuf);
	inf->Node = qy->Data->Node;
	inf->Data = qy->CurInf;
	inf->Node->OpenCnt++;
	inf->Obj = obj;
	qy->ItemCnt++;

    return (void*)inf;
    }


/*** stxQueryClose - close the query.
 ***/
int
stxQueryClose(void* qy_v, pObjTrxTree* oxt)
    {

	/** Free the structure **/
	nmFree(qy_v,sizeof(StxQuery));

    return 0;
    }


/*** stxGetAttrType - get the type (DATA_T_xxx) of an attribute by name.
 ***/
int
stxGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pStxData inf = STX(inf_v);
    pStructInf find_inf;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

    	/** Lookup the subgroup inf **/
	find_inf = stLookup(inf->Data, attrname);
	if (!find_inf || find_inf->Type != ST_T_ATTRIB) 
	    {
	    /*mssError(1,"STX","Could not locate requested structure file attribute");*/
	    return -1;
	    }

	/** If StrVal[0] valid, string else integer **/
	if (find_inf->nVal == 1 && find_inf->StrVal[0] != NULL) return DATA_T_STRING;
	else if (find_inf->nVal == 1) return DATA_T_INTEGER;

	/** If more than one value, return string or int vec **/
	if (find_inf->nVal > 1 && find_inf->StrVal[0] != NULL) return DATA_T_STRINGVEC;
	else if (find_inf->nVal > 1) return DATA_T_INTVEC;

    return -1;
    }


/*** stxGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
stxGetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree* oxt)
    {
    pStxData inf = STX(inf_v);
    pStructInf find_inf;
    int i;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    *((char**)val) = inf->Data->Name;
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    if (stLookup(inf->Data,"content"))
	        *((char**)val) = "application/octet-stream";
	    else
	        *((char**)val) = "system/void";
	    return 0;
	    }
	else if (!strcmp(attrname,"outer_type"))
	    {
	    *((char**)val) = inf->Data->UsrType;
	    return 0;
	    }

	/** Look through the attribs in the subinf **/
	for(i=0;i<inf->Data->nSubInf;i++) 
	    {
	    find_inf = inf->Data->SubInf[i];
	    if (!strcmp(attrname,find_inf->Name) && find_inf->Type == ST_T_ATTRIB)
	        {
		if (find_inf->StrVal[0] != NULL && find_inf->nVal > 1)
		    {
		    inf->SVvalue.Strings = find_inf->StrVal;
		    inf->SVvalue.nStrings = find_inf->nVal;
		    *(pStringVec*)val = &(inf->SVvalue);
		    }
		else if (find_inf->nVal > 1)
		    {
		    inf->IVvalue.Integers = find_inf->IntVal;
		    inf->IVvalue.nIntegers = find_inf->nVal;
		    *(pIntVec*)val = &(inf->IVvalue);
		    }
		else if (find_inf->StrVal[0] != NULL) *(char**)val = find_inf->StrVal[0];
		else *(int*)val = find_inf->IntVal[0];
		return 0;
		}
	    }

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    *(char**)val = "";
	    return 0;
	    }

	/*mssError(1,"STX","Could not locate requested structure file attribute");*/

    return -1;
    }


/*** stxGetNextAttr - get the next attribute name for this object.
 ***/
char*
stxGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pStxData inf = STX(inf_v);

	/** Get the next attr from the list unless last one already **/
	while(inf->CurAttr < inf->Data->nSubInf && 
	      (inf->Data->SubInf[inf->CurAttr]->Type != ST_T_ATTRIB || 
	       !strcmp(inf->Data->SubInf[inf->CurAttr]->Name,"annotation"))) inf->CurAttr++;
	if (inf->CurAttr >= inf->Data->nSubInf) return NULL;

    return inf->Data->SubInf[inf->CurAttr++]->Name;
    }


/*** stxGetFirstAttr - get the first attribute name for this object.
 ***/
char*
stxGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pStxData inf = STX(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->CurAttr = 0;

	/** Return the next one. **/
	ptr = stxGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** stxSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
stxSetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree oxt)
    {
    pStxData inf = STX(inf_v);
    pStructInf find_inf;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (inf->Data == inf->Node->Data)
	        {
	        if (!strcmp(inf->Obj->Pathname->Pathbuf,".")) return -1;
	        if (strlen(inf->Obj->Pathname->Pathbuf) - 
	            strlen(strrchr(inf->Obj->Pathname->Pathbuf,'/')) + 
		    strlen(*(char**)(val)) + 1 > 255)
		    {
		    mssError(1,"STX","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,*(char**)(val));
	        if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssError(1,"STX","SetAttr 'name': could not rename structure file node object");
		    return -1;
		    }
	        strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname);
		}
	    strcpy(inf->Data->Name,*(char**)val);
	    return 0;
	    }

	/** Set content type if that was requested. **/
	if (!strcmp(attrname,"content_type"))
	    {
	    strcpy(inf->Data->UsrType,*(char**)val);
	    return 0;
	    }

	/** Otherwise, set the integer or string value **/
	find_inf = stLookup(inf->Data,attrname);
	if (!find_inf) 
	    {
	    mssError(1,"STX","Requested structure file attribute not found");
	    return -1;
	    }
	if (find_inf->Type != ST_T_ATTRIB) return -1;

	/** If int, set int else set string (determine by: is string null) **/
	if (find_inf->StrVal[0] == NULL)
	    {
	    find_inf->IntVal[0] = *(int*)val;
	    }
	else
	    {
	    if (find_inf->StrAlloc[0]) nmSysFree(find_inf->StrVal[0]);
	    find_inf->StrAlloc[0]=1;
	    find_inf->StrVal[0] = (char*)nmSysMalloc(strlen(*(char**)val));
	    strcpy(find_inf->StrVal[0], *(char**)val);
	    }
	
	/** Set dirty flag **/
	inf->Node->Status = SN_NS_DIRTY;

    return 0;
    }


/*** stxAddAttr - add an attribute to an object.  This works for the structure
 *** driver, where attributes are easily added.
 ***/
int
stxAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    pStxData inf = STX(inf_v);
    pStructInf new_inf;
    char* ptr;

    	/** Add the attribute **/
	new_inf = stAddAttr(inf->Data, attrname);
	if (type == DATA_T_STRING)
	    {
	    ptr = (char*)nmSysMalloc(strlen(*(char**)val));
	    strcpy(ptr, *(char**)val);
	    stAddValue(new_inf, ptr, 0);
	    new_inf->StrAlloc[0] = 1;
	    }
	else if (type == DATA_T_INTEGER)
	    {
	    stAddValue(new_inf, NULL, *(int*)val);
	    }
	else
	    {
	    stAddValue(new_inf, NULL, 0);
	    }

	/** Set dirty flag **/
	inf->Node->Status = SN_NS_DIRTY;

    return 0;
    }


/*** stxOpenAttr - open an attribute as if it were an object with content.
 *** the struct driver doesn't support this at this time.
 ***/
void*
stxOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** stxGetFirstMethod -- there are no methods, so this just always
 *** fails.
 ***/
char*
stxGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** stxGetNextMethod -- same as above.  Always fails. 
 ***/
char*
stxGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** stxExecuteMethod - No methods to execute, so this fails.
 ***/
int
stxExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** stxInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
stxInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&STX_INF,0,sizeof(STX_INF));
	/*xhInit(&STX_INF.NodeCache,255,0);*/

	/** Setup the structure **/
	strcpy(drv->Name,"STX - Structure File Driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"system/structure");

	/** Setup the function references. **/
	drv->Open = stxOpen;
	drv->Close = stxClose;
	drv->Create = stxCreate;
	drv->Delete = stxDelete;
	drv->OpenQuery = stxOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = stxQueryFetch;
	drv->QueryClose = stxQueryClose;
	drv->Read = stxRead;
	drv->Write = stxWrite;
	drv->GetAttrType = stxGetAttrType;
	drv->GetAttrValue = stxGetAttrValue;
	drv->GetFirstAttr = stxGetFirstAttr;
	drv->GetNextAttr = stxGetNextAttr;
	drv->SetAttrValue = stxSetAttrValue;
	drv->AddAttr = stxAddAttr;
	drv->OpenAttr = stxOpenAttr;
	drv->GetFirstMethod = stxGetFirstMethod;
	drv->GetNextMethod = stxGetNextMethod;
	drv->ExecuteMethod = stxExecuteMethod;

	nmRegister(sizeof(StxData),"StxData");
	nmRegister(sizeof(StxQuery),"StxQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

