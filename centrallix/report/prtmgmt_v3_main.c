#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "barcode.h"
#include "report.h"
#include "cxlib/mtask.h"
#include "cxlib/magic.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"
#include "prtmgmt_v3/prtmgmt_v3.h"
#include "htmlparse.h"
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
/* Module:	prtmgmt.c,prtmgmt.h                                     */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	December 12, 2001                                       */
/*									*/
/* Description:	This module provides the Version-3 printmanagement	*/
/*		subsystem functionality.				*/
/************************************************************************/


/*** GLOBAL Stuff... ***/
PrtGlobals PRTMGMT;


/*** external init functions ***/
int prt_txtlm_Initialize();
int prt_tablm_Initialize();
int prt_collm_Initialize();
int prt_paglm_Initialize();


/*** prtRegisterUnits - adds a units-of-measure conversion factor to the
 *** system.  Conversion factors for x and y are typically different, since
 *** the system natively tracks units based on 1/10ths of an inch horizontally
 *** and 1/6ths of an inch vertically (standard US forms character matrix
 *** sizing).
 ***/
int 
prtRegisterUnits(char* units_name, double x_adj, double y_adj)
    {
    pPrtUnits u;

	/** Allocate the units structure. **/
	u = (pPrtUnits)nmMalloc(sizeof(PrtUnits));
	if (!u) return -1;

	/** Fill it in and add it. **/
	memccpy(u->Name, units_name, 0, 31);
	u->Name[31] = '\0';
	u->AdjX = x_adj;
	u->AdjY = y_adj;
	xaAddItem(&PRTMGMT.UnitsList, (void*)u);

    return 0;
    }


/*** prtAllocType - allocates a memory structure describing a print stream
 *** object type.
 ***/
pPrtObjType 
prtAllocType()
    {
    pPrtObjType ot;

	/** Alloc and init the structure. **/
	ot = (pPrtObjType)nmMalloc(sizeof(PrtObjType));
	if (!ot) return NULL;
	SETMAGIC(ot,MGK_PRTOBJTYPE);

    return ot;
    }


/*** prtRegisterType - adds the object type to the list in the system.
 ***/
int 
prtRegisterType(pPrtObjType type)
    {

	/** Add it to the list. **/
	ASSERTMAGIC(type, MGK_PRTOBJTYPE);
	xaAddItem(&PRTMGMT.TypeList, (void*)type);

    return 0;
    }


/*** prtAllocLayoutMgr - allocates a layout manager structure.
 ***/
pPrtLayoutMgr 
prtAllocLayoutMgr()
    {
    pPrtLayoutMgr lm;

	/** Alloc and init the structure **/
	lm = (pPrtLayoutMgr)nmMalloc(sizeof(PrtLayoutMgr));
	if (!lm) return NULL;
	SETMAGIC(lm,MGK_PRTLM);

    return lm;
    }


/*** prtRegisterLayoutMgr - register the layout manager with the system so
 *** that it can be referenced by obj type declarations later on.
 ***/
int 
prtRegisterLayoutMgr(pPrtLayoutMgr layout_mgr)
    {

	/** Add it to the list. **/
	ASSERTMAGIC(layout_mgr, MGK_PRTLM);
	xaAddItem(&PRTMGMT.LayoutMgrList, (void*)layout_mgr);

    return 0;
    }


/*** prtLookupLayoutMgr - find a layout manager, by name.
 ***/
pPrtLayoutMgr
prtLookupLayoutMgr(char* layout_mgr)
    {
    pPrtLayoutMgr lm;
    int i;

	/** Search our list for the lm. **/
	for(i=0;i<PRTMGMT.LayoutMgrList.nItems;i++)
	    {
	    lm = (pPrtLayoutMgr)(PRTMGMT.LayoutMgrList.Items[i]);
	    ASSERTMAGIC(lm, MGK_PRTLM);
	    if (!strcmp(lm->Name, layout_mgr)) return lm;
	    }

    return NULL;
    }


/*** prtLookupUnits - find the units structure that corresponds to the given
 *** units name.
 ***/
pPrtUnits
prtLookupUnits(char* units_name)
    {
    pPrtUnits u;
    int i;

	/** Search for it in the globals **/
	for(i=0;i<PRTMGMT.UnitsList.nItems;i++)
	    {
	    u = (pPrtUnits)(PRTMGMT.UnitsList.Items[i]);
	    if (!strcmp(units_name,u->Name)) return u;
	    }

    return NULL;
    }


/*** prtRegisterFont - registers a fontname-to-fontid mapping.  IDs are used
 *** in the objstream, whereas names are used in the API.
 ***/
int
prtRegisterFont(char* font_name, int font_id)
    {
    pPrtFontDesc pf;

	/** Allocate font structure **/
	pf = (pPrtFontDesc)nmMalloc(sizeof(PrtFontDesc));
	if (!pf) return -1;

	/** Set it up and add to the globals **/
	pf->FontID = font_id;
	memccpy(pf->FontName, font_name, 0, 39);
	pf->FontName[39] = '\0';
	xaAddItem(&PRTMGMT.FontList, (void*)pf);

    return 0;
    }


/*** prtLookupFont() - find the id for a given font name.
 ***/
int
prtLookupFont(char* font_name)
    {
    pPrtFontDesc pf;
    int i;

	/** Search for it. **/
	for(i=0;i<PRTMGMT.FontList.nItems;i++)
	    {
	    pf = (pPrtFontDesc)(PRTMGMT.FontList.Items[i]);
	    if (!strcmp(pf->FontName, font_name)) return pf->FontID;
	    }

    return -1;
    }


/*** prtLookupFontName() - find the name for a font given the id.  Note -
 *** font ids are not unique for each given name!
 ***/
char*
prtLookupFontName(int font_id)
    {
    pPrtFontDesc pf;
    int i;

	/** Search for it. **/
	for(i=0;i<PRTMGMT.FontList.nItems;i++)
	    {
	    pf = (pPrtFontDesc)(PRTMGMT.FontList.Items[i]);
	    if (pf->FontID == font_id) return pf->FontName;
	    }

    return NULL;
    }


/*** prtAllocHandle() - generate a new handle id for a given pointer.  The
 *** handles are used for referencing pointers that can sometimes change,
 *** such as objstream objects.
 ***/
int
prtAllocHandle(void* ptr)
    {
    pPrtHandle h;

	/** Alloc the structure **/
	h = (pPrtHandle)nmMalloc(sizeof(PrtHandle));
	if (!h) return -1;
	SETMAGIC(h, MGK_PRTHANDLE);

	/** Grab a handle id **/
	h->HandleID = PRTMGMT.NextHandleID++;
	if (h->HandleID == 1<<30) h->HandleID = 0;

	/** Fill in the pointer. **/
	h->Ptr.Generic = ptr;

	/** Add to the table **/
	xhAdd(&PRTMGMT.HandleTable, (void*)&(h->HandleID), (void*)h);
	xhAdd(&PRTMGMT.HandleTableByPtr, (void*)&(h->Ptr.Generic), (void*)h);

    return h->HandleID;
    }


/*** prtHandlePtr() - return the pointer for a given handle id
 ***/
void*
prtHandlePtr(int handle_id)
    {
    pPrtHandle h;

	/** Look it up **/
	h = (pPrtHandle)xhLookup(&PRTMGMT.HandleTable, (void*)&handle_id);
	if (!h) return NULL;
	ASSERTMAGIC(h, MGK_PRTHANDLE);

    return h->Ptr.Generic;
    }


/*** prtUpdateHandle() - update the pointer for a given handle so that future
 *** requests for prtHandlePtr() return the new value.
 ***/
int
prtUpdateHandle(int handle_id, void* ptr)
    {
    pPrtHandle h;

	/** Look it up **/
	h = (pPrtHandle)xhLookup(&PRTMGMT.HandleTable, (void*)&handle_id);
	if (!h) return -1;
	ASSERTMAGIC(h, MGK_PRTHANDLE);

	/** Update the pointer **/
	xhRemove(&PRTMGMT.HandleTableByPtr, (void*)&(h->Ptr.Generic));
	h->Ptr.Generic = ptr;
	xhAdd(&PRTMGMT.HandleTableByPtr, (void*)&(h->Ptr.Generic), (void*)h);

    return 0;
    }


/*** prtUpdateHandleByPtr() - update the pointer for a given handle so that future
 *** requests for prtHandlePtr() return the new value.
 ***/
int
prtUpdateHandleByPtr(void* old_ptr, void* ptr)
    {
    pPrtHandle h;

	/** Look it up **/
	h = (pPrtHandle)xhLookup(&PRTMGMT.HandleTableByPtr, (void*)&old_ptr);
	if (!h) return -1;
	ASSERTMAGIC(h, MGK_PRTHANDLE);

	/** Update the pointer **/
	xhRemove(&PRTMGMT.HandleTableByPtr, (void*)&(h->Ptr.Generic));
	h->Ptr.Generic = ptr;
	xhAdd(&PRTMGMT.HandleTableByPtr, (void*)&(h->Ptr.Generic), (void*)h);

    return 0;
    }


/*** prtLookupHandle() - return the handle for a given object.
 ***/
int
prtLookupHandle(void* ptr)
    {
    pPrtHandle h;

	/** Look it up **/
	h = (pPrtHandle)xhLookup(&PRTMGMT.HandleTableByPtr, (void*)&ptr);
	if (!h) return -1;
	ASSERTMAGIC(h, MGK_PRTHANDLE);

    return h->HandleID;
    }


/*** prtFreeHandle() - release the handle and memory used by it.  The handle
 *** id will then become invalid and will not be reused until the id's wrap
 *** around (at 2^30 id's).
 ***/
int
prtFreeHandle(int handle_id)
    {
    pPrtHandle h;

	/** Look it up **/
	h = (pPrtHandle)xhLookup(&PRTMGMT.HandleTable, (void*)&handle_id);
	if (!h) return -1;
	ASSERTMAGIC(h, MGK_PRTHANDLE);

	/** Remove from table and release it. **/
	xhRemove(&PRTMGMT.HandleTable, (void*)&handle_id);
	xhRemove(&PRTMGMT.HandleTableByPtr, (void*)&(h->Ptr.Generic));
	nmFree(h, sizeof(PrtHandle));

    return 0;
    }


/*** prtAllocFormatter() - allocates a print formatter module
 *** structure.
 ***/
pPrtFormatter
prtAllocFormatter()
    {
    pPrtFormatter fmt;

	/** Allocate and initialize **/
	fmt = (pPrtFormatter)nmMalloc(sizeof(PrtFormatter));
	if (!fmt) return NULL;
	memset(fmt,0,sizeof(PrtFormatter));
	fmt->Priority = htonl(1000);
	SETMAGIC(fmt,MGK_PRTFMTR);

    return fmt;
    }


/*** prtRegisterFormatter() - register a print formatter module
 ***/
int
prtRegisterFormatter(pPrtFormatter fmt)
    {

	ASSERTMAGIC(fmt, MGK_PRTFMTR);

	/** Add it to the list of formatters **/
	xaAddItemSortedInt32(&(PRTMGMT.FormatterList), (void*)fmt, ((char*)&(fmt->Priority)) - (char*)fmt);

    return 0;
    }


/*** prtInitialize() - init the print management system, set up globals, and
 *** build our lists of types, layout managers, and such.
 ***/
int
prtInitialize()
    {
    pPrtObjType ot;

	/** Init the globals **/
	memset(&PRTMGMT, 0, sizeof(PRTMGMT));
	xaInit(&PRTMGMT.UnitsList, 16);
	xaInit(&PRTMGMT.TypeList, 16);
	xaInit(&PRTMGMT.LayoutMgrList, 16);
	xaInit(&PRTMGMT.FormatterList, 16);
	xaInit(&PRTMGMT.FormatDriverList, 16);
	xaInit(&PRTMGMT.FontList, 16);
	xhInit(&PRTMGMT.HandleTable, 255, sizeof(int));
	xhInit(&PRTMGMT.HandleTableByPtr, 255, sizeof(void*));
	PRTMGMT.NextHandleID = 1;

	/** Setup units-of-measure conversions **/
	prtRegisterUnits("us_forms", 1.0, 1.0);
	prtRegisterUnits("default", 1.0, 1.0);
	prtRegisterUnits("millimeters", (double)10.0/25.4, (double)6.0/25.4);
	prtRegisterUnits("centimeters", (double)10.0/2.54, (double)6.0/2.54);
	prtRegisterUnits("inches", 10.0, 6.0);
	prtRegisterUnits("points", (double)10.0/72.0, (double)6.0/72.0);

	/** Setup standard fonts **/
	prtRegisterFont("fixed", 1);
	prtRegisterFont("courier", 1);
	prtRegisterFont("monospace", 1);
	prtRegisterFont("helvetica", 2);
	prtRegisterFont("arial", 2);
	prtRegisterFont("sans serif", 2);
	prtRegisterFont("proportional", 2);
	prtRegisterFont("times new roman", 3);
	prtRegisterFont("times", 3);
	prtRegisterFont("cg times", 3);
	prtRegisterFont("serif", 3);

	/** Setup list of layout managers **/
	prt_textlm_Initialize();
	prt_tablm_Initialize();
	prt_collm_Initialize();
	prt_pagelm_Initialize();

	/** Setup list of object types **/
	ot = prtAllocType();
	ot->TypeID = PRT_OBJ_T_DOCUMENT;
	strcpy(ot->TypeName,"document");
	ot->PrefLayoutMgr = NULL;
	prtRegisterType(ot);

	ot = prtAllocType();
	ot->TypeID = PRT_OBJ_T_PAGE;
	strcpy(ot->TypeName,"page");
	ot->PrefLayoutMgr = prtLookupLayoutMgr("paged");
	prtRegisterType(ot);
	
	ot = prtAllocType();
	ot->TypeID = PRT_OBJ_T_AREA;
	strcpy(ot->TypeName,"area");
	ot->PrefLayoutMgr = prtLookupLayoutMgr("textflow");
	prtRegisterType(ot);
	
	ot = prtAllocType();
	ot->TypeID = PRT_OBJ_T_STRING;
	strcpy(ot->TypeName,"string");
	ot->PrefLayoutMgr = NULL;
	prtRegisterType(ot);
	
	ot = prtAllocType();
	ot->TypeID = PRT_OBJ_T_IMAGE;
	strcpy(ot->TypeName,"image");
	ot->PrefLayoutMgr = NULL;
	prtRegisterType(ot);
	
	ot = prtAllocType();
	ot->TypeID = PRT_OBJ_T_RECT;
	strcpy(ot->TypeName,"rect");
	ot->PrefLayoutMgr = NULL;
	prtRegisterType(ot);
	
	ot = prtAllocType();
	ot->TypeID = PRT_OBJ_T_TABLE;
	strcpy(ot->TypeName,"table");
	ot->PrefLayoutMgr = prtLookupLayoutMgr("tabular");
	prtRegisterType(ot);
	
	ot = prtAllocType();
	ot->TypeID = PRT_OBJ_T_TABLEROW;
	strcpy(ot->TypeName,"tablerow");
	ot->PrefLayoutMgr = prtLookupLayoutMgr("tabular");
	prtRegisterType(ot);
	
	ot = prtAllocType();
	ot->TypeID = PRT_OBJ_T_TABLECELL;
	strcpy(ot->TypeName,"tablecell");
	ot->PrefLayoutMgr = prtLookupLayoutMgr("tabular");
	prtRegisterType(ot);
	
	ot = prtAllocType();
	ot->TypeID = PRT_OBJ_T_SECTION;
	strcpy(ot->TypeName,"section");
	ot->PrefLayoutMgr = prtLookupLayoutMgr("columnar");
	prtRegisterType(ot);

	ot = prtAllocType();
	ot->TypeID = PRT_OBJ_T_SECTCOL;
	strcpy(ot->TypeName,"sectcol");
	ot->PrefLayoutMgr = prtLookupLayoutMgr("columnar");
	prtRegisterType(ot);

    return 0;
    }


