#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "barcode.h"
#include "report.h"
#include "mtask.h"
#include "magic.h"
#include "xarray.h"
#include "xstring.h"
#include "prtmgmt_v3.h"
#include "htmlparse.h"
#include "mtsession.h"

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

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3_api.c,v 1.1 2002/01/27 22:50:06 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/report/prtmgmt_v3_api.c,v $

    $Log: prtmgmt_v3_api.c,v $
    Revision 1.1  2002/01/27 22:50:06  gbeeley
    Untested and incomplete print formatter version 3 files.
    Initial checkin.


 **END-CVSDATA***********************************************************/


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
	obj = s->ContentHead;
	found_obj = NULL;
	while(obj)
	    {
	    if (obj->ObjType->TypeID == PRT_OBJ_T_PAGE)
		{
		found_obj = obj;
		h = (pPrtHandle)xhLookup(&PRTMGMT.HandleTableByPtr, (void*)&obj);
		if (h) handle_id = h->HandleID;
		}
	    obj = obj->Next;
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

	if (!obj) return -1;
	obj->LineHeight = prtUnitY(line_height);

    return 0;
    }


/*** prtGetLineHeight() - returns the current line height, in terms of the
 *** current vertical units.
 ***/
double
prtGetLineHeight(int handle_id)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    return obj?(prtUsrUnitY(obj->LineHeight)):(-1);
    }


/*** prtSetTextStyle() - sets the current text style from a given text style
 *** structure.
 ***/
int
prtSetTextStyle(int handle_id, pPrtTextStyle style)
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream set_obj;

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Add an empty string object to contain the attr change. **/
	set_obj = prt_internal_AddEmptyObj(obj);

	/** Set the style. **/
	memcpy(&(set_obj->TextStyle), style, sizeof(PrtTextStyle));
	set_obj->LineHeight = style->FontSize/12.0;

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

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Add an empty string object to contain the attr change. **/
	set_obj = prt_internal_AddEmptyObj(obj);
	set_obj->TextStyle.Attr = attrs;

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

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Add an empty string object to contain the attr change. **/
	set_obj = prt_internal_AddEmptyObj(obj);
	newid = prtLookupFont(fontname);
	if (newid < 0) return -1;
	set_obj->TextStyle.FontID = newid; 

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
	if (!obj) return -1;
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
prtSetFontSize(int handle_id, int fontsize);
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream set_obj;

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Add an empty string object to contain the attr change. **/
	set_obj = prt_internal_AddEmptyObj(obj);
	set_obj->TextStyle.FontSize = fontsize;
	set_obj->LineHeight = fontsize/12.0;

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
prtSetColor(int handle_id, int color);
    {
    pPrtObjStream obj = (pPrtObjStream)prtHandlePtr(handle_id);
    pPrtObjStream set_obj;

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);

	/** Add an empty string object to contain the attr change. **/
	set_obj = prt_internal_AddEmptyObj(obj);
	set_obj->TextStyle.Color = color;

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

	/** Check the obj **/
	if (!obj) return -1;
	ASSERTMAGIC(obj, MGK_PRTOBJSTRM);
	if (obj->ObjType->TypeID != PRT_OBJ_T_AREA) return -1;

	/** Check for a child object **/
	if (obj->ContentTail != NULL)
	    tgt_obj = obj->ContentTail;
	else
	    tgt_obj = obj;

    return 0;
    }


/*** prtSetVPos() - sets the vertical position in the current textflow type
 *** container.  This does set the HPos to zero -- to do that, also use the
 *** prtSetHPos() call.
 ***/
int
prtSetVPos(int handle_id, double y)
    {
    }


