#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
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
/* Module:	prtmgmt_v3_api.c                                        */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	January 24, 2002                                        */
/*									*/
/* Description:	This module implements most of the printing subsystem	*/
/*		API.  prtmgmt_main and prtmgmt_session implement the	*/
/*		remainder of the functions.				*/
/************************************************************************/



/*** prtGetPageRef() - returns a handle to the current page.
 ***/
int
prtGetPageRef(pPrtSession s)
    {
    pPrtObjStream obj, found_obj;
    int handle_id = -1;
    pPrtHandle h;

	/** Point to the document object. **/
	obj = s->StreamHead;

	/** Find the current page & get the handle. **/
	found_obj = NULL;
	while(obj)
	    {
	    if (obj->ObjType->TypeID == PRT_OBJ_T_PAGE)
		{
		found_obj = obj;
		h = (pPrtHandle)xhLookup(&PRTMGMT.HandleTableByPtr, (void*)&obj);
		if (h) handle_id = h->HandleID;
		}
	    obj = obj->ContentHead;
	    }

	/** Found a page object, but no handle allocated yet? **/
	if (handle_id == -1 && found_obj)
	    {
	    handle_id = prtAllocHandle(found_obj);
	    }
	
    return handle_id;
    }


/*** prtSetFocus() - sets the default object which will be the target of
 *** further API commands.
 ***/
int
prtSetFocus(pPrtSession s, int handle_id)
    {

	/** Set it. **/
	s->FocusHandle = handle_id;

    return 0;
    }


/*** prtSetPageNumber() - sets the page number for the current page.  The
 *** passed handle can be any object other than the document object; the
 *** system will traverse up the tree until it finds a page object, and
 *** will set the page number there.
 ***/
int
prtSetPageNumber(int handle_id, int new_pagenum)
    {
    pPrtObjStream obj;

	/** Lookup the handle, then find the page containing it. **/
	obj = (pPrtObjStream)prtHandlePtr(handle_id);
	obj = prt_internal_GetPage(obj);
	if (!obj) return -1;
	obj->ObjID = new_pagenum;

    return 0;
    }


/*** prtGetPageNumber() - return the page number of the page containing the
 *** given object.
 ***/
int
prtGetPageNumber(int handle_id)
    {
    pPrtObjStream obj;

	/** Lookup the handle, then find the page containing it. **/
	obj = (pPrtObjStream)prtHandlePtr(handle_id);
	obj = prt_internal_GetPage(obj);
	if (!obj) return -1;

    return obj->ObjID;
    }


/*** prtSetJustification() - sets the current justification mode for a textflow
 *** type of area.  Ignored for other object types.
 ***/
int
prtSetJustification(int handle_id, int just_mode)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);

	if (!obj) return -1;
	obj->Justification = just_mode;

    return 0;
    }


/*** prtGetJustification() - returns the current justification mode for a 
 *** textflow type of area.
 ***/
int
prtGetJustification(int handle_id)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    return obj?(obj->Justification):(-1);
    }


/*** prtSetLineHeight() - sets the current line height, and thus line spacing.
 *** This is normally automatically set from the point size, but can be 
 *** also set manually.  Height is not in points, but in vertical units.
 ***/
int
prtSetLineHeight(int handle_id, double line_height)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream set_obj;
    pPrtSession s = PRTSESSION(obj);

	if (!obj) return -1;

	/** Add an empty string object to contain the attr change. **/
	if (obj->ObjType->TypeID == PRT_OBJ_T_AREA)
	    set_obj = prt_internal_CreateEmptyObj(obj);
	else
	    set_obj = prt_internal_AddEmptyObj(obj);
	if (!set_obj)
	    return -1;

	obj->LineHeight = prtUnitY(PRTSESSION(obj),line_height);
	set_obj->LineHeight = prtUnitY(PRTSESSION(obj),line_height);
	obj->ConfigLineHeight = obj->LineHeight;
	set_obj->ConfigLineHeight = set_obj->LineHeight;

	if (obj->ObjType->TypeID == PRT_OBJ_T_STRING)
	    {
	    set_obj->Height = prt_internal_GetFontHeight(set_obj);
	    set_obj->ConfigHeight = set_obj->Height;
	    }
	set_obj->YBase = prt_internal_GetFontBaseline(set_obj);
	if (obj->ObjType->TypeID == PRT_OBJ_T_AREA)
	    obj->LayoutMgr->AddObject(obj,set_obj);

	prt_internal_DispatchEvents(s);

    return 0;
    }


/*** prtGetLineHeight() - returns the current line height, in terms of the
 *** current vertical units.
 ***/
double
prtGetLineHeight(int handle_id)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    return obj?(prtUsrUnitY(PRTSESSION(obj),obj->LineHeight)):(-1);
    }


/*** prtSetTextStyle() - sets the current text style from a given text style
 *** structure.
 ***/
int
prtSetTextStyle(int handle_id, pPrtTextStyle style)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream set_obj;
    pPrtSession s = PRTSESSION(obj);

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Add an empty string object to contain the attr change. **/
	if (obj->ObjType->TypeID == PRT_OBJ_T_AREA)
	    set_obj = prt_internal_CreateEmptyObj(obj);
	else
	    set_obj = prt_internal_AddEmptyObj(obj);
	if (!set_obj)
	    return -1;

	/** Set the style. **/
	memcpy(&(set_obj->TextStyle), style, sizeof(PrtTextStyle));
	set_obj->TextStyle.FontSize = s->Formatter->GetNearestFontSize(s->FormatterData, set_obj->TextStyle.FontSize);
	if (set_obj->ConfigLineHeight >= 0.0)
	    set_obj->LineHeight = set_obj->ConfigLineHeight;
	else
	    set_obj->LineHeight = prt_internal_FontToLineHeight(set_obj);

	/** Only adjust height if a string object **/
	if (set_obj->ObjType->TypeID == PRT_OBJ_T_STRING)
	    {
	    set_obj->Height = prt_internal_GetFontHeight(set_obj);
	    set_obj->ConfigHeight = set_obj->Height;
	    }

	set_obj->YBase = prt_internal_GetFontBaseline(set_obj);
	if (obj->ObjType->TypeID == PRT_OBJ_T_AREA)
	    obj->LayoutMgr->AddObject(obj,set_obj);

	prt_internal_DispatchEvents(s);

    return 0;
    }


/*** prtGetTextStyle() - returns the current text style settings as a whole
 *** structure.
 ***/
int
prtGetTextStyle(int handle_id, pPrtTextStyle *style)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream get_obj;

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Check for a child object **/
	if (obj->ContentTail != NULL)
	    get_obj = obj->ContentTail;
	else
	    get_obj = obj;

	/** Get the style. **/
	memcpy(*style, &(get_obj->TextStyle), sizeof(PrtTextStyle));

    return 0;
    }


/*** prtSetAttr() - sets only the font attributes of the text (including italics,
 *** boldface, and underlining).
 ***/
int
prtSetAttr(int handle_id, int attrs)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream set_obj;
    pPrtSession s = PRTSESSION(obj);

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Add an empty string object to contain the attr change. **/
	set_obj = prt_internal_AddEmptyObj(obj);
	if (!set_obj)
	    return -1;
	set_obj->TextStyle.Attr = attrs;

	prt_internal_DispatchEvents(s);

    return 0;
    }


/*** prtGetAttr() - gets only the font attributes of the text (including italics,
 *** boldface, and underlining).
 ***/
int
prtGetAttr(int handle_id)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream tgt_obj;

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Check for a child object **/
	if (obj->ContentTail != NULL)
	    tgt_obj = obj->ContentTail;
	else
	    tgt_obj = obj;

    return tgt_obj->TextStyle.Attr;
    }


/*** prtSetFont() - sets the current font face.
 ***/
int
prtSetFont(int handle_id, char* fontname)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream set_obj;
    int newid;
    pPrtSession s = PRTSESSION(obj);

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Add an empty string object to contain the attr change. **/
	if (obj->ObjType->TypeID == PRT_OBJ_T_AREA)
	    set_obj = prt_internal_CreateEmptyObj(obj);
	else
	    set_obj = prt_internal_AddEmptyObj(obj);
	if (!set_obj)
	    return -1;

	/** Set the font id, and recalc the height/baseline **/
	newid = prtLookupFont(fontname);
	if (newid < 0)
	    {
	    mssError(1,"PRT","Invalid font name: %s", fontname);
	    return -1;
	    }
	set_obj->TextStyle.FontID = newid; 
	if (obj->ObjType->TypeID == PRT_OBJ_T_STRING)
	    {
	    set_obj->Height = prt_internal_GetFontHeight(set_obj);
	    set_obj->ConfigHeight = set_obj->Height;
	    }
	set_obj->YBase = prt_internal_GetFontBaseline(set_obj);
	if (obj->ObjType->TypeID == PRT_OBJ_T_AREA)
	    obj->LayoutMgr->AddObject(obj,set_obj);

	prt_internal_DispatchEvents(s);

    return 0;
    }


/*** prtGetFont() - get the name of the current font.  Note that fonts
 *** may have many names for one given font type; a correct name is 
 *** returned but it might not be the same one as was used in the SetFont
 *** call.
 ***/
char*
prtGetFont(int handle_id)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream tgt_obj;

	/** Check the obj **/
	if (!obj) return NULL;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Check for a child object **/
	if (obj->ContentTail != NULL)
	    tgt_obj = obj->ContentTail;
	else
	    tgt_obj = obj;

    return prtLookupFontName(tgt_obj->TextStyle.FontID);
    }


/*** prtSetFontSize() - sets the current font size, in points.  Automatically
 *** adjusts the line height accordingly.
 ***/
int
prtSetFontSize(int handle_id, int fontsize)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream set_obj;
    pPrtSession s = PRTSESSION(obj);

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Add an empty string object to contain the attr change. **/
	if (obj->ObjType->TypeID == PRT_OBJ_T_AREA)
	    set_obj = prt_internal_CreateEmptyObj(obj);
	else
	    set_obj = prt_internal_AddEmptyObj(obj);
	if (!set_obj)
	    return -1;

	/** Set the size and recalc the height/baseline **/
	set_obj->TextStyle.FontSize = s->Formatter->GetNearestFontSize(s->FormatterData, fontsize);
	if (set_obj->ConfigLineHeight >= 0.0)
	    set_obj->LineHeight = set_obj->ConfigLineHeight;
	else
	    set_obj->LineHeight = prt_internal_FontToLineHeight(set_obj);
	if (obj->ObjType->TypeID == PRT_OBJ_T_STRING)
	    {
	    set_obj->Height = prt_internal_GetFontHeight(set_obj);
	    set_obj->ConfigHeight = set_obj->Height;
	    }
	set_obj->YBase = prt_internal_GetFontBaseline(set_obj);
	if (obj->ObjType->TypeID == PRT_OBJ_T_AREA)
	    obj->LayoutMgr->AddObject(obj,set_obj);

	prt_internal_DispatchEvents(s);

    return 0;
    }


/*** prtGetFontSize() - returns the current font size in points.
 ***/
int
prtGetFontSize(int handle_id)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream tgt_obj;

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Check for a child object **/
	if (obj->ContentTail != NULL)
	    tgt_obj = obj->ContentTail;
	else
	    tgt_obj = obj;

    return tgt_obj->TextStyle.FontSize;
    }


/*** prtSetColor() - sets the current font color.  Colors are expressed as 
 *** 0x00RRGGBB.  Note that this does NOT set the foreground color of the
 *** container.  To do that, use prtSetFGColor().
 ***/
int
prtSetColor(int handle_id, int color)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream set_obj;
    pPrtSession s = PRTSESSION(obj);

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Add an empty string object to contain the attr change. **/
	set_obj = prt_internal_AddEmptyObj(obj);
	if (!set_obj)
	    return -1;
	set_obj->TextStyle.Color = color;

	prt_internal_DispatchEvents(s);

    return 0;
    }


/*** prtGetColor() - returns the current font color as 0x00RRGGBB.
 ***/
int
prtGetColor(int handle_id)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream tgt_obj;

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Check for a child object **/
	if (obj->ContentTail != NULL)
	    tgt_obj = obj->ContentTail;
	else
	    tgt_obj = obj;

    return tgt_obj->TextStyle.Color;
    }


/*** prtSetHPos() - sets the horizontal position on the current line.  This
 *** is NOT used for setting the position of a container - to do that, include
 *** the position in the prtAddObject() call.
 ***/
int
prtSetHPos(int handle_id, double x)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream tgt_obj;
    pPrtSession s = PRTSESSION(obj);

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);
	if (obj->ObjType->TypeID != PRT_OBJ_T_AREA) return -1;

	/** Add an empty object so we can change the position **/
	tgt_obj = (pPrtObjStream)prt_internal_AddEmptyObj(obj);
	if (!tgt_obj) return -1;

	/** Set the position only if it will fit. **/
	tgt_obj->Flags |= PRT_OBJ_F_XSET;
	if ((x + PRT_FP_FUDGE) >= tgt_obj->X && (x - PRT_FP_FUDGE) <= prtInnerWidth(obj))
	    {
	    tgt_obj->X = x;
	    tgt_obj->Flags |= PRT_OBJ_F_XSET;
	    }
	else
	    {
	    return -1;
	    }

	prt_internal_DispatchEvents(s);

    return 0;
    }


/*** prtSetVPos() - sets the vertical position in the current textflow type
 *** container.  This does set the HPos to zero.
 ***/
int
prtSetVPos(int handle_id, double y)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream tgt_obj;
    pPrtSession s = PRTSESSION(obj);

	/** Check the obj and its type (must be an AREA) **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);
	if (obj->ObjType->TypeID != PRT_OBJ_T_AREA) return -1;

	/** Add an empty object so we can change the position **/
	tgt_obj = (pPrtObjStream)prt_internal_AddEmptyObj(obj);
	if (!tgt_obj) return -1;

	/** Set the position only if it will fit. **/
	tgt_obj->Flags |= (PRT_OBJ_F_XSET | PRT_OBJ_F_YSET);
	if ((y + PRT_FP_FUDGE) >= tgt_obj->Y + tgt_obj->Height && (!(obj->Flags & PRT_OBJ_F_FIXEDSIZE) || (y - PRT_FP_FUDGE) <= prtInnerHeight(obj)))
	    {
	    tgt_obj->X = 0;
	    tgt_obj->Y = y;
	    }
	else
	    {
	    return -1;
	    }

	prt_internal_DispatchEvents(s);

    return 0;
    }


/*** prtWriteString - put a string of text into the document.
 ***/
int
prtWriteString(int handle_id, char* str)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream string_obj;
    int rval=0;
    char* special_char_ptr;
    int len;
    double x;

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);
	if (!str) return -1;

	/** Check for embedded special chars in the string **/
	while(*str)
	    {
	    /** Adding text containing a tab or newline? **/
	    special_char_ptr = strpbrk(str,"\n\t\14");
	    if (special_char_ptr)
		len = special_char_ptr - str;
	    else
		len = strlen(str);

	    /** Create a new textstring object **/
	    string_obj = prt_internal_AllocObjByID(PRT_OBJ_T_STRING);
	    if (!string_obj) return -1;
	    string_obj->Session = obj->Session;

	    /** Format it and add it to the container **/
	    string_obj->Content = nmSysMalloc(len+2);
	    string_obj->ContentSize = len+2;
	    strncpy((char*)string_obj->Content, str, len);
	    string_obj->Content[len] = 0;
	    prt_internal_CopyAttrs((obj->ContentTail)?(obj->ContentTail):obj,string_obj);
	    string_obj->Width = prt_internal_GetStringWidth((obj->ContentTail)?(obj->ContentTail):obj, (char*)string_obj->Content, -1);
	    string_obj->ConfigWidth = string_obj->Width;
	    string_obj->Height = prt_internal_GetFontHeight(string_obj);
	    string_obj->ConfigHeight = string_obj->Height;
	    string_obj->YBase = prt_internal_GetFontBaseline(string_obj);
	    rval = obj->LayoutMgr->AddObject(obj,string_obj);
	    if (rval < 0) break;

	    /** Handle might have changed - check it **/
	    obj = prtHandlePtr(handle_id);
	    prt_internal_DispatchEvents(PRTSESSION(obj));

	    /** Skipping over a special char? **/
	    if (special_char_ptr)
		{
		str = special_char_ptr+1;
		if (*special_char_ptr == '\n')
		    {
		    rval = prtWriteNL(handle_id);
		    if (rval < 0) break;
		    }
		else if (*special_char_ptr == '\t')
		    {
		    if (obj->ContentTail)
			x = floor(floor((obj->ContentTail->X + obj->ContentTail->Width)/8.0 + 0.00000001)*8.0 + 8.00000001);
		    else
			x = 8.0;
		    rval = prtSetHPos(handle_id, x);
		    if (rval < 0) rval = prtWriteNL(handle_id);
		    if (rval < 0) break;
		    }
		else if (*special_char_ptr == '\14')
		    {
		    rval = prtWriteFF(handle_id);
		    if (rval < 0) break;
		    }

		/** Handle might have changed - check it **/
		obj = prtHandlePtr(handle_id);
		}
	    else
		{
		break;
		}
	    }

	prt_internal_DispatchEvents(PRTSESSION(obj));

    return rval;
    }


/*** prtWriteNL - add a newline (break) into the document.
 ***/
int
prtWriteNL(int handle_id)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream nl_obj;
    int rval;
    pPrtSession s = PRTSESSION(obj);

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Create a new textstring object **/
	nl_obj = prt_internal_AllocObjByID(PRT_OBJ_T_STRING);
	if (!nl_obj) return -1;
	nl_obj->Session = obj->Session;
	nl_obj->Content = nmSysMalloc(2);
	nl_obj->ContentSize = 2;
	nl_obj->Content[0] = '\0';
	nl_obj->Width = 0.0;
	nl_obj->ConfigWidth = 0.0;
	prt_internal_CopyAttrs((obj->ContentTail)?(obj->ContentTail):obj,nl_obj);
	nl_obj->Height = prt_internal_GetFontHeight(nl_obj);
	nl_obj->ConfigHeight = nl_obj->Height;
	nl_obj->YBase = prt_internal_GetFontBaseline(nl_obj);
	nl_obj->Flags |= PRT_OBJ_F_NEWLINE;
	rval = obj->LayoutMgr->AddObject(obj,nl_obj);

	prt_internal_DispatchEvents(s);

    return rval;
    }


/*** prtWriteFF - write a forms feed into the document, usually into a text
 *** area of some sort.  This basically causes a page break operation to occur.
 ***/
int
prtWriteFF(int handle_id)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream new_obj;
    int rval;
    pPrtSession s = PRTSESSION(obj);

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Request a break operation on the object. **/
	if (!(obj->Flags & PRT_OBJ_F_ALLOWHARDBREAK)) return -1;
	rval = obj->LayoutMgr->Break(obj, &new_obj);

	prt_internal_DispatchEvents(s);

    return rval;
    }


/*** prtAddObject() - adds a new printing object within an existing object.  This
 *** normally isn't used for printing text - use prtWriteString() for that.  Use
 *** this function for adding areas, tables, multicolumn sections, and so forth.
 *** The object's geometry and position are specified as a part of this function
 *** call.
 ***
 *** Returns a handle id for the object.
 ***/
int
prtAddObject(int handle_id, int obj_type, double x, double y, double width, double height, int flags, ...)
    {
    va_list va;
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream new_obj;
    int new_handle_id; 
    pPrtSession s = PRTSESSION(obj);

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Allocate the new object to be added. **/
	new_obj = prt_internal_AllocObjByID(obj_type);
	if (!new_obj) return -1;


	/** Set the object's position, flags, etc. **/
	new_obj->Flags |= (flags & PRT_OBJ_UFLAGMASK);
	new_obj->Height = height;
	new_obj->Width = width;
	new_obj->ConfigHeight = height;
	new_obj->ConfigWidth = width;
	if (flags & PRT_OBJ_U_XSET)
	    new_obj->X = x;
	else
	    new_obj->X = 0.0;
	if (flags & PRT_OBJ_U_YSET)
	    new_obj->Y = y;
	else
	    new_obj->Y = 0.0;

	/** Init container... including optional params **/
	prt_internal_CopyAttrs(obj, new_obj);
	new_obj->Session = obj->Session;
	va_start(va, flags);
	if (new_obj->LayoutMgr) new_obj->LayoutMgr->InitContainer(new_obj, NULL, va);
	va_end(va);

	/** Bolt a handle onto the new object... **/
	new_handle_id = prtAllocHandle(new_obj);

	/** Add the object to the given parent object. **/
	if (obj->LayoutMgr->AddObject(obj, new_obj) < 0)
	    {
	    prtFreeHandle(new_handle_id);
	    prt_internal_FreeObj(new_obj);
	    return -1;
	    }

	prt_internal_DispatchEvents(s);

    return new_handle_id;
    }


/*** prtSetObjectCallback() - sets up a callback to be used to fill in the 
 *** content of a container.  The "is_pre" parameter specifies whether the 
 *** container is populated *before* (pre) or *after* (post) the page itself
 *** has been completed.  is_pre callbacks are called when a new page is
 *** created or when the callback is set up.
 ***/
int
prtSetObjectCallback(int handle_id, void* (*callback_fn)(), int is_pre)
    {
    /*pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);*/
    return 0;
    }


/*** prtEndObject() - closes an object so that content may no longer be
 *** added to it.  This also releases the handle, after which point the
 *** handle_id becomes invalid/undefined.
 ***/
int
prtEndObject(int handle_id)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);

	if (!obj) return -1;
	ASSERTMAGIC(obj,MGK_PRTOBJSTRM);

	/** Release the handle **/
	prtFreeHandle(handle_id);

	/** Reduce the open count on the object. **/
	obj->nOpens--;

    return 0;
    }


/*** prtSetValue - sets a special setting on an object.  Used currently
 *** for layout manager specific settings.
 ***/
int
prtSetValue(int handle_id, char* attrname, ...)
    {
    va_list va;
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);

	if (!obj) return -1;
	ASSERTMAGIC(obj,MGK_PRTOBJSTRM);

	/** Check with the layout manager **/
	va_start(va, attrname);
	if (obj->LayoutMgr->SetValue)
	    return obj->LayoutMgr->SetValue(obj, attrname, va);
	va_end(va);

    return -1;
    }


/*** prtSetMargins - set the top/bottom/left/right margins for a container.
 *** values may be negative to indicate that they should not be set.
 ***/
int
prtSetMargins(int handle_id, double t, double b, double l, double r)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);

	if (!obj) return -1;
	ASSERTMAGIC(obj,MGK_PRTOBJSTRM);

	/** Set the appropriate values. **/
	if (t >= 0.0) obj->MarginTop = t;
	if (b >= 0.0) obj->MarginBottom = b;
	if (l >= 0.0) obj->MarginLeft = l;
	if (r >= 0.0) obj->MarginRight = r;

    return 0;
    }


/*** prtSetDataHints() - specify metadata "hints" about the data being 
 *** displayed.  Hints include the data type (DATA_T_xxx) and a flags
 *** field that is currently reserved.
 ***/
int
prtSetDataHints(int handle_id, int data_type, int flags)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);

	if (!obj) return -1;
	ASSERTMAGIC(obj,MGK_PRTOBJSTRM);

	obj->DataType = data_type;

    return 0;
    }
