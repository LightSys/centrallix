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

    $Id: ht_render.h,v 1.21 2004/06/25 16:46:31 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/ht_render.h,v $

    $Log: ht_render.h,v $
    Revision 1.21  2004/06/25 16:46:31  gbeeley
    - Auto-detect size of user-agent's window

    Revision 1.20  2004/04/29 16:26:43  gbeeley
    - Fixes to get FourTabs.app working again in NS4/Moz, and in IE5.5/IE6.
    - Added inline-include feature to help with debugging in IE, which does
      not specify the correct file in its errors.  To use it, just append
      "?ls__collapse_includes=yes" to your .app URL.

    Revision 1.19  2004/03/10 10:44:02  jasonyip

    Corrected the comment - should be javascript 1.5

    Revision 1.18  2004/03/10 10:40:10  jasonyip

    I have added JS15 for javascript 1.5 Capabilities.

    Revision 1.17  2004/02/24 20:23:38  gbeeley
    - adding htrGetBoolean and htrParamValue to header file

    Revision 1.16  2003/11/22 16:37:18  jorupp
     * add support for moving event handler scripts to the .js code
     	note: the underlying implimentation in ht_render.c_will_ change, this was
    	just to get opinions on the API and output
     * moved event handlers for htdrv_window from the .c to the .js

    Revision 1.15  2003/11/22 16:34:37  jorupp
     * add definitions for htrAddStyleSheetItem*
     * add GCC __attribute__ definition to _va functions (adds warning about mismatched format type options)
    	note: I've also added a check that disables the attributes if you're not using GCC.  I can't
    	test this, as all I have for a compiler is GCC.  Let me know if it doesn't work.

    Revision 1.14  2003/11/18 06:01:11  gbeeley
    - adding utility method htrGetBackground to simplify bgcolor/image

    Revision 1.13  2003/06/21 23:07:26  jorupp
     * added framework for capability-based multi-browser support.
     * checkbox and label work in Mozilla, and enough of ht_render and page do to allow checkbox.app to work
     * highly unlikely that keyboard events work in Mozilla, but hey, anything's possible.
     * updated all htdrv_* modules to list their support for the "dhtml" class and make a simple
     	capability check before in their Render() function (maybe this should be in Verify()?)

    Revision 1.12  2003/05/30 17:39:50  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.11  2002/12/04 00:19:13  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.10  2002/07/16 18:23:21  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.9  2002/06/09 23:44:47  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.8  2002/05/03 01:41:39  jheth
    Defined global variable HT_FIELDNAME_SIZE, set to 60 - Used in some visible widgets

    Revision 1.7  2002/04/28 06:00:38  jorupp
     * added htrAddScriptCleanup* stuff
     * added cleanup stuff to osrc

    Revision 1.6  2002/04/25 22:51:30  gbeeley
    Added vararg versions of some key htrAddThingyItem() type of routines
    so that all of this sbuf stuff doesn't have to be done, as we have
    been bumping up against the limits on the local sbuf's due to very
    long object names.  Modified label, editbox, and treeview to test
    out (and make kardia.app work).

    Revision 1.5  2002/04/25 04:27:21  gbeeley
    Added new AddInclude() functionality to the html generator, so include
    javascript files can be added.  Untested.

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
#include "expression.h"
#include "xarray.h"

#define HT_SBUF_SIZE	(256)
/** This is the max fieldname size -- John Peebles and Joe Heth **/
#define HT_FIELDNAME_SIZE (60)

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


/** Widget set class.  A class refers generally to the deployment language
 ** to be used (such as DHTML vs. XUL).  A 'user agent' in Centrallix htmlgen
 ** is a specific combination of a browser and a class (such as moz-dhtml or
 ** moz-xul or whatever).  The 'class' is merely used to allow the user to
 ** control which type of widget set is used for their browser if it supports
 ** more than one type.
 **/
typedef struct
    {
    char	ClassName[32];
    XArray	Agents;
    XHashTable	WidgetDrivers;		/* widget -> driver map */
    }
    HtClass, *pHtClass;

    
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
    XArray	Cleanups;		/* List of cleanup strings */
    XArray	HtmlBody;		/* html body page buffers */
    pObject	HtmlBodyFile;		/* output file if too big */
    XArray	HtmlHeader;		/* html header page buffers */
    pObject	HtmlHeaderFile;		/* output file if too big */
    XArray	HtmlStylesheet;		/* html stylesheet buffers */
    pObject	HtmlStylesheetFile;	/* output file if too big */
    XArray	HtmlExpressionInit;	/* expressions init */
    XArray	HtmlBodyParams;		/* params for <body> tag */
    XArray	Includes;		/* script includes */
    XHashTable	NameIncludes;		/* hash lookup for includes */
    HtNameArray	EventScripts;		/* various event script code */
    }
    HtPage, *pHtPage;

#define HT_CAPABILITY_NOT_SUPPORTED 0
#define HT_CAPABILITY_SUPPORTED 1

/** Capabilities structure **/
typedef struct
    {
    unsigned int Dom0NS:1; /* DOM Level 0 (ie. non-W3C DOM -- Netscape */
    unsigned int Dom0IE:1; /* DOM Level 0 (ie. non-W3C DOM -- IE */
    unsigned int Dom1HTML:1; /* Core W3C DOM Level 1 for HTML */
    unsigned int Dom1XML:1; /* Core W3C DOM Level 1 for XML */
    unsigned int Dom2Core:1; /* Core W3C DOM Level 2 */
    unsigned int Dom2HTML:1; /* W3C DOM Level 2 for HTML */
    unsigned int Dom2XML:1; /* W3C DOM Level 2 for XML */
    unsigned int Dom2Views:1; /* W3C DOM Level 2 Views */
    unsigned int Dom2StyleSheets:1; /* W3C DOM Level 2 StyleSheet traversal */
    unsigned int Dom2CSS:1; /* W3C DOM Level 2 CSS Styles */
    unsigned int Dom2CSS2:1; /* W3C DOM Level 2 CSS2Properties Interface */
    unsigned int Dom2Events:1; /* W3C DOM Level 2 Event handling */
    unsigned int Dom2MouseEvents:1; /* W3C DOM Level 2 MouseEvent handling */
    unsigned int Dom2HTMLEvents:1; /* W3C DOM Level 2 HTMLEvent handling */
    unsigned int Dom2MutationEvents:1; /* W3C DOM Level 2 MutationEvent handling */
    unsigned int Dom2Range:1; /* W3C DOM Level 2 range interfaces */
    unsigned int Dom2Traversal:1; /* W3C DOM Level 2 traversal interfaces */
    unsigned int CSS1:1; /* W3C CSS Level 1 */
    unsigned int CSS2:1; /* W3C CSS Level 2 */
    unsigned int HTML40:1; /* W3C HTML 4.0 */
    unsigned int JS15:1; /* Javacript 1.5 */
    }
    HtCapabilities, *pHtCapabilities;

/** Session structure, used for page creation **/
typedef struct
    {
    HtPage	Page;			/* the generated page... */
    pHtTree	Tree;			/* tree page metainfo structure */
    int		DisableBody;
    char*	Tmpbuf;			/* temp buffer used in _va() functions */
    int		TmpbufSize;
    HtCapabilities Capabilities;	/* the capabilities supported by the browser */
    pHtClass	Class;			/* the widget class to use **/
    pStruct	Params;			/* params from the user */
    pObjSession	ObjSession;		/* objectsystem session */
    int		Width;			/* target container (browser) width in pixels */
    int		Height;			/* target container height in pixels */
    char*	Parent;			/* name of target container */
    }
    HtSession, *pHtSession;



/** Flags for layer addition routines **/
#define HTR_LAYER_F_DEFAULT	0	/* no flags */
#define HTR_LAYER_F_DYNAMIC	1	/* layer will be reloaded from server */

#ifndef __GNUC__
#define __attribute__(a) /* hide function attributes from non-GCC compilers */
#endif

/** Rendering engine functions **/
int htrAddHeaderItem(pHtSession s, char* html_text);
int htrAddHeaderItem_va(pHtSession s, char* fmt, ... ) __attribute__((format(printf, 2, 3)));
int htrAddBodyItem(pHtSession s, char* html_text);
int htrAddBodyItem_va(pHtSession s, char* fmt, ... ) __attribute__((format(printf, 2, 3)));
int htrAddBodyParam(pHtSession s, char* html_param);
int htrAddBodyParam_va(pHtSession s, char* fmt, ... ) __attribute__((format(printf, 2, 3)));
int htrAddEventHandler(pHtSession s, char* event_src, char* event, char* drvname, char* handler_code);
int htrAddEventHandlerFunction(pHtSession s, char* event_src, char* event, char* drvname, char* function);
int htrAddScriptFunction(pHtSession s, char* fn_name, char* fn_text, int flags);
int htrAddScriptGlobal(pHtSession s, char* var_name, char* initialization, int flags);
int htrAddScriptInit(pHtSession s, char* init_text);
int htrAddScriptInit_va(pHtSession s, char* fmt, ... ) __attribute__((format(printf, 2, 3)));
int htrAddScriptCleanup(pHtSession s, char* init_text);
int htrAddScriptCleanup_va(pHtSession s, char* fmt, ... ) __attribute__((format(printf, 2, 3)));
int htrAddScriptInclude(pHtSession s, char* filename, int flags);
int htrAddStylesheetItem(pHtSession s, char* html_text);
int htrAddStylesheetItem_va(pHtSession s, char* fmt, ... ) __attribute__((format(printf, 2, 3)));

int htrAddExpression(pHtSession s, char* objname, char* property, pExpression exp);
int htrDisableBody(pHtSession s);
int htrRenderWidget(pHtSession session, pObject widget_obj, int z, char* parentname, char* parentobj);
int htrRenderSubwidgets(pHtSession s, pObject widget_obj, char* docname, char* layername, int zlevel);

/** Utility routines **/
int htrGetBackground(pObject obj, char* prefix, int as_style, char* buf, int buflen);
int htrGetBoolean(pObject obj, char* attr, int default_value);

/** Content-intelligent (useragent-sensitive) rendering engine functions **/
int htrAddBodyItemLayer_va(pHtSession s, int flags, char* id, int cnt, const char* fmt, ...);
int htrAddBodyItemLayerStart(pHtSession s, int flags, char* id, int cnt);
int htrAddBodyItemLayerEnd(pHtSession s, int flags);

/** Administrative functions **/
int htrRegisterDriver(pHtDriver drv);
int htrInitialize();
int htrRender(pFile output, pObject appstruct, pStruct params);
int htrAddAction(pHtDriver drv, char* action_name);
int htrAddEvent(pHtDriver drv, char* event_name);
int htrAddParam(pHtDriver drv, char* eventaction, char* param_name, int datatype);
pHtDriver htrAllocDriver();
int htrAddSupport(pHtDriver drv, char* className);
char* htrParamValue(pHtSession s, char* paramname);


#endif /* _HT_RENDER_H */

