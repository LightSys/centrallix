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
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "stparse_ne.h"
#include "st_node.h"
#include "cxlib/mtsession.h"

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

    $Id: objdrv_struct.c,v 1.12 2007/06/06 15:16:36 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_struct.c,v $

    $Log: objdrv_struct.c,v $
    Revision 1.12  2007/06/06 15:16:36  gbeeley
    - (change) getting the obj_inherit module into the build

    Revision 1.11  2007/04/08 03:52:00  gbeeley
    - (bugfix) various code quality fixes, including removal of memory leaks,
      removal of unused local variables (which create compiler warnings),
      fixes to code that inadvertently accessed memory that had already been
      free()ed, etc.
    - (feature) ability to link in libCentrallix statically for debugging and
      performance testing.
    - Have a Happy Easter, everyone.  It's a great day to celebrate :)

    Revision 1.10  2007/03/02 22:30:39  gbeeley
    - (bugfix) problem with the setting up of the Pathname structure in this
      driver was causing subtree select to crash on structure file items.

    Revision 1.9  2005/02/26 06:42:40  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.8  2004/06/12 00:10:15  mmcgill
    Chalk one up under 'didn't understand the build process'. The remaining
    os drivers have been updated, and the prototype for objExecuteMethod
    in obj.h has been changed to match the changes made everywhere it's
    called - param is now of type pObjData, not void*.

    Revision 1.7  2003/05/30 17:39:53  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.6  2003/04/04 05:02:44  gbeeley
    Added more flags to objInfo dealing with content and seekability.
    Added objInfo capability to objdrv_struct.

    Revision 1.5  2002/08/13 01:51:13  gbeeley
    Added mssError warning/error message if the attribute could not be found
    or a type mismatch occurred.

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

    Revision 1.3  2001/10/16 23:53:02  gbeeley
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

    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:01:09  gbeeley
    Centrallix Core initial import

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
    void*	VecData;
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
    pStruct open_inf;
    pStructInf find_inf, attr_inf, search_inf;
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
		    open_inf = obj->Pathname->OpenCtl[obj->SubPtr]->SubInf[i];
		    if (strncmp(open_inf->Name,"ls__",4) && open_inf->StrVal)
		        {
			attr_inf = stAddAttr(node->Data, open_inf->Name);
			endptr = NULL;
			n = strtol(open_inf->StrVal,&endptr,0);
			if (endptr && *endptr == '\0')
			    stSetAttrValue(attr_inf, DATA_T_INTEGER, POD(&n), 0);
			else 
			    stSetAttrValue(attr_inf, DATA_T_STRING, POD(&(open_inf->StrVal)), 0);
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
	    for(j=0;j<search_inf->nSubInf;j++) if (!strcmp(ptr,search_inf->SubInf[j]->Name) && stStructType(search_inf->SubInf[j]) == ST_T_SUBGROUP)
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
		if (stStructType(inf->Data->SubInf[i]) == ST_T_SUBGROUP)
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
		if (stStructType(inf->Data->SubInf[i]) == ST_T_SUBGROUP)
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
stxRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    /*pStxData inf = STX(inf_v);*/
    return -1;
    }


/*** stxWrite - Again, no content.  This fails.
 ***/
int
stxWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
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
	      stStructType(qy->Data->Data->SubInf[qy->ItemCnt]) != ST_T_SUBGROUP) qy->ItemCnt++;

	/** No more left? **/
	if (qy->ItemCnt >= qy->Data->Data->nSubInf) return NULL;
	qy->CurInf = qy->Data->Data->SubInf[qy->ItemCnt];

	/** Build the filename. **/
	if (obj_internal_AddToPath(obj->Pathname, qy->CurInf->Name) < 0)
	    return NULL;

	/** Alloc the structure **/
	inf = (pStxData)nmMalloc(sizeof(StxData));
	if (!inf) return NULL;
	strtcpy(inf->Pathname, obj->Pathname->Pathbuf, sizeof(inf->Pathname));
	inf->Node = qy->Data->Node;
	inf->Data = qy->CurInf;
	inf->Node->OpenCnt++;
	inf->Obj = obj;
	inf->VecData = NULL;
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
    int t;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type") ||
	    !strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;

    	/** Lookup the subgroup inf **/
	find_inf = stLookup(inf->Data, attrname);
	if (!find_inf || stStructType(find_inf) != ST_T_ATTRIB) 
	    {
	    /*mssError(1,"STX","Could not locate requested structure file attribute");*/
	    return -1;
	    }

	/** Examine the expr to determine the type **/
	t = stGetAttrType(find_inf, 0);
	if (find_inf->Value && find_inf->Value->NodeType == EXPR_N_LIST)
	    {
	    if (t == DATA_T_INTEGER) return DATA_T_INTVEC;
	    else return DATA_T_STRINGVEC;
	    }
	else if (find_inf->Value)
	    {
	    return t;
	    }

    return -1;
    }


/*** stxGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
stxGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pStxData inf = STX(inf_v);
    pStructInf find_inf;
    int rval;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"STX","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = inf->Data->Name;
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"STX","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (stLookup(inf->Data,"content"))
	        val->String = "application/octet-stream";
	    else
	        val->String = "system/void";
	    return 0;
	    }
	else if (!strcmp(attrname,"outer_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"STX","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = inf->Data->UsrType;
	    return 0;
	    }

	/** Look through the attribs in the subinf **/
	find_inf = stLookup(inf->Data, attrname);

	/** If annotation, and not found, return "" **/
	if (!find_inf && !strcmp(attrname,"annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"STX","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = "";
	    return 0;
	    }

	/** Not found? **/
	if (!find_inf || stStructType(find_inf) != ST_T_ATTRIB) return -1;

	/** Vector or scalar? **/
	if (find_inf->Value->NodeType == EXPR_N_LIST)
	    {
	    if (inf->VecData) nmSysFree(inf->VecData);
	    if (stGetAttrType(find_inf, 0) == DATA_T_INTEGER)
		{
		if (datatype != DATA_T_INTVEC)
		    {
		    mssError(1,"STX","Type mismatch getting attribute '%s' (should be intvec)", attrname);
		    return -1;
		    }
		inf->VecData = stGetValueList(find_inf, DATA_T_INTEGER, &(inf->IVvalue.nIntegers));
		val->IntVec = &(inf->IVvalue);
		val->IntVec->Integers = (int*)(inf->VecData);
		}
	    else
		{
		if (datatype != DATA_T_STRINGVEC)
		    {
		    mssError(1,"STX","Type mismatch getting attribute '%s' (should be stringvec)", attrname);
		    return -1;
		    }
		inf->VecData = stGetValueList(find_inf, DATA_T_STRING, &(inf->SVvalue.nStrings));
		val->StringVec = &(inf->SVvalue);
		val->StringVec->Strings = (char**)(inf->VecData);
		}
	    return 0;
	    }
	else
	    {
	    rval = stGetAttrValue(find_inf, datatype, val, 0);
	    if (rval < 0)
		{
		mssError(1,"STX","Type mismatch or non-existent attribute '%s'", attrname);
		}
	    return rval;
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
	      (stStructType(inf->Data->SubInf[inf->CurAttr]) != ST_T_ATTRIB || 
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
stxSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pStxData inf = STX(inf_v);
    pStructInf find_inf;
    int t;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"STX","Type mismatch setting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (inf->Data == inf->Node->Data)
	        {
	        if (!strcmp(inf->Obj->Pathname->Pathbuf,".")) return -1;
	        if (strlen(inf->Obj->Pathname->Pathbuf) - 
	            strlen(strrchr(inf->Obj->Pathname->Pathbuf,'/')) + 
		    strlen(val->String) + 1 > 255)
		    {
		    mssError(1,"STX","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,val->String);
	        if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssError(1,"STX","SetAttr 'name': could not rename structure file node object");
		    return -1;
		    }
	        strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname);
		}
	    strcpy(inf->Data->Name,val->String);
	    return 0;
	    }

	/** Set content type if that was requested. **/
	if (!strcmp(attrname,"content_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"STX","Type mismatch setting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    strcpy(inf->Data->UsrType,val->String);
	    return 0;
	    }

	/** Otherwise, set the integer or string value **/
	find_inf = stLookup(inf->Data,attrname);
	if (!find_inf) 
	    {
	    mssError(1,"STX","Requested structure file attribute not found");
	    return -1;
	    }
	if (stStructType(find_inf) != ST_T_ATTRIB) return -1;

	/** Set value of attribute **/
	t = stGetAttrType(find_inf, 0);
	if (t <= 0) return -1;
	if (datatype != t)
	    {
	    mssError(1,"STX","Type mismatch setting attribute '%s' [requested=%s, actual=%s",
		    attrname, obj_type_names[datatype], obj_type_names[t]);
	    return -1;
	    }
	stSetAttrValue(find_inf, t, val, 0);

	/** Set dirty flag **/
	inf->Node->Status = SN_NS_DIRTY;

    return 0;
    }


/*** stxAddAttr - add an attribute to an object.  This works for the structure
 *** driver, where attributes are easily added.
 ***/
int
stxAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt)
    {
    pStxData inf = STX(inf_v);
    pStructInf new_inf;
    char* ptr;

    	/** Add the attribute **/
	new_inf = stAddAttr(inf->Data, attrname);
	if (type == DATA_T_STRING)
	    {
	    ptr = (char*)nmSysStrdup(val->String);
	    stAddValue(new_inf, ptr, 0);
	    }
	else if (type == DATA_T_INTEGER)
	    {
	    stAddValue(new_inf, NULL, val->Integer);
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
stxExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** stxInfo - return additional informational flags about an
 *** object.
 ***/
int
stxInfo(void* inf_v, pObjectInfo info)
    {
    pStxData inf = STX(inf_v);
    int i;

	/** Setup the flags, and we know the subobject count btw **/
	info->Flags = (OBJ_INFO_F_CAN_HAVE_SUBOBJ | OBJ_INFO_F_SUBOBJ_CNT_KNOWN |
		OBJ_INFO_F_CAN_ADD_ATTR | OBJ_INFO_F_CANT_SEEK | OBJ_INFO_F_CANT_HAVE_CONTENT |
		OBJ_INFO_F_NO_CONTENT | OBJ_INFO_F_SUPPORTS_INHERITANCE);
	info->nSubobjects = 0;
	for(i=0;i<inf->Data->nSubInf;i++)
	    {
	    if (stStructType(inf->Data->SubInf[i]) == ST_T_SUBGROUP) info->nSubobjects++;
	    }
	if (info->nSubobjects)
	    info->Flags |= OBJ_INFO_F_HAS_SUBOBJ;
	else
	    info->Flags |= OBJ_INFO_F_NO_SUBOBJ;

    return 0;
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
	drv->Capabilities = OBJDRV_C_INHERIT;
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
	drv->Info = stxInfo;

	nmRegister(sizeof(StxData),"StxData");
	nmRegister(sizeof(StxQuery),"StxQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }

