#ifndef _HT_RENDER_H
#define _HT_RENDER_H

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
/* Module: 	ht_render.h,ht_render.c					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 19, 1998					*/
/* Description:	HTML Page rendering engine that interacts with the 	*/
/*		various widget drivers to produce a dynamic HTML page.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: ht_render.h,v 1.4 2002/03/09 19:21:20 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/ht_render.h,v $

    $Log: ht_render.h,v $
    Revision 1.4  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.3  2001/11/03 02:09:55  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.2  2001/10/22 17:19:41  gbeeley
    Added a few utility functions in ht_render to simplify the structure and
    authoring of widget drivers a bit.

    Revision 1.1.1.1  2001/08/13 18:00:52  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:31:19  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

#include "mtask.h"
#include "obj.h"
#include "xarray.h"

#define HT_SBUF_SIZE	(256)


/** Widget parameter structure, also used for positioning params **/
typedef struct
    {
    char	ParamName[32];		/* Name of parameter */
    int		DataType;		/* DATA_T_xxx, from obj.h */
    XArray	EnumValues;		/* If enumerated, values listing */
    }
    HtParam, *pHtParam;


/** Widget event or action structure, used for handling connectors **/
typedef struct
    {
    char	Name[32];		/* Name of event or action */
    XArray	Parameters;		/* Listing of parameters */
    }
    HtEventAction, *pHtEventAction;


/** Widget driver information structure **/
typedef struct 
    {
    char	Name[64];		/* Driver name */
    char	WidgetName[64];		/* Name of widget. */
    int		(*Render)();		/* Function to render the page */
    int		(*Verify)();		/* Function to pre-verify parameters */
    XArray	PosParams;		/* Positioning parameter listing. */
    XArray	Properties;		/* Properties this thing will have. */
    XArray	Events;			/* Events for this widget type */
    XArray	Actions;		/* Actions on this widget type */
    }
    HtDriver, *pHtDriver;


/** Widget parameter value structure. **/
typedef struct
    {
    pHtParam	Parameter;		/* Param that this value is for. */
    int		IntVal;			/* if DATA_T_INTEGER, the int value */
    char*	StringVal;		/* if DATA_T_STRING, the string value */
    int		StringAlloc;		/* set if string was malloc()'d. */
    DateTime	DateVal;		/* if DATA_T_DATETIME, the date value */
    int		EnumVal;		/* if DATA_T_ENUM, the enumerated id */
    }
    HtValue, *pHtValue;


/** Widget parameterized tree structure for pre-rendering **/
typedef struct _HT
    {
    pHtDriver	Driver;			/* Driver to handle this object. */
    XArray	Values;			/* Parameters for widget construction */
    XArray	LocValues;		/* Positioning params for this widget */
    XArray	Children;
    }
    HtTree, *pHtTree;


/** Structure for holding string-string-value data. **/
typedef struct
    {
    char*	Name;
    char*	Value;
    int		Alloc;
    int		NameSize;
    int		ValueSize;
    }
    StrValue, *pStrValue;

#define HTR_F_NAMEALLOC		1
#define HTR_F_VALUEALLOC	2


/** Structure for a named array, for using arrays in lookups. **/
typedef struct
    {
    char	Name[128];		/* name of the array */
    XArray	Array;			/* List of data... */
    XHashTable	HashTable;		/* Lookup mechanism */
    }
    HtNameArray, *pHtNameArray;


/** When page is being actualized, we use this: **/
typedef struct
    {
    XHashTable	NameFunctions;		/* Name-to-function map */
    XArray	Functions;		/* List of functions. */
    XHashTable	NameGlobals;		/* Name-to-global map */
    XArray	Globals;		/* List of globals. */
    XArray	Inits;			/* List of init strings */
    XArray	HtmlBody;		/* html body page buffers */
    pObject	HtmlBodyFile;		/* output file if too big */
    XArray	HtmlHeader;		/* html header page buffers */
    pObject	HtmlHeaderFile;		/* output file if too big */
    XArray	HtmlBodyParams;		/* params for <body> tag */
    HtNameArray	EventScripts;		/* various event script code */
    }
    HtPage, *pHtPage;


/** Session structure, used for page creation **/
typedef struct
    {
    HtPage	Page;			/* the generated page... */
    pHtTree	Tree;			/* tree page metainfo structure */
    int		DisableBody;
    }
    HtSession, *pHtSession;


/** Rendering engine functions **/
int htrAddHeaderItem(pHtSession s, char* html_text);
int htrAddBodyItem(pHtSession s, char* html_text);
int htrAddBodyParam(pHtSession s, char* html_param);
int htrAddEventHandler(pHtSession s, char* event_src, char* event, char* drvname, char* handler_code);
int htrAddScriptFunction(pHtSession s, char* fn_name, char* fn_text, int flags);
int htrAddScriptGlobal(pHtSession s, char* var_name, char* initialization, int flags);
int htrAddScriptInit(pHtSession s, char* init_text);
int htrDisableBody(pHtSession s);
int htrRenderWidget(pHtSession session, pObject widget_obj, int z, char* parentname, char* parentobj);
int htrRenderSubwidgets(pHtSession s, pObject widget_obj, char* docname, char* layername, int zlevel);

/** Administrative functions **/
int htrRegisterDriver(pHtDriver drv);
int htrInitialize();
int htrRender(pFile output, pObject appstruct);
int htrAddAction(pHtDriver drv, char* action_name);
int htrAddEvent(pHtDriver drv, char* event_name);
int htrAddParam(pHtDriver drv, char* eventaction, char* param_name, int datatype);
pHtDriver htrAllocDriver();


#endif /* _HT_RENDER_H */

