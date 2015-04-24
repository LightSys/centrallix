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


#include "cxlib/mtask.h"
#include "obj.h"
#include "expression.h"
#include "cxlib/xarray.h"
#include "wgtr.h"

#define HT_SBUF_SIZE	(256)
/** This is the max fieldname size -- John Peebles and Joe Heth **/
#define HT_FIELDNAME_SIZE (60)


/*** Extensions to data types ***/
#define HT_DATA_T_BOOLEAN   (100)


/** WGTR deployment method private data value, used to hold various
 ** dhtml specific add-on values for particular widgets
 **/
typedef struct
    {
    char*	ObjectLinkage;		/* object linkage to DOM */
    char*	ContainerLinkage;	/* subobj container linkage to DOM */
    char*	InitFunc;		/* initialization function */
    char*	Param;			/* initialization parameter */
    }
    HtDMPrivateData, *pHtDMPrivateData;


/** Namespace / subtree structure.  Used to identify the heirarchy of
 ** namespaces/subtrees within the application.
 **/
typedef struct _HTN
    {
    char	DName[64];		/* deployment name for subtree root */
    char	ParentCtr[64];		/* name of parent container */
    int		IsSubnamespace;
    int		HasScriptInits;
    struct _HTN* Parent;
    struct _HTN* FirstChild;
    struct _HTN* NextSibling;
    }
    HtNamespace, *pHtNamespace;


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


/** WGTR RenderFlag defines **/
#define HT_WGTF_NOOBJECT 1		/* wgt node does not have a corresponding DHTML object */
#define HT_WGTF_NORENDER 2		/* do not deploy this node to the client. */

/** Widget driver information structure **/
typedef struct 
    {
    char	Name[64];		/* Driver name */
    char	WidgetName[64];		/* Name of widget. */
    int		(*Render)();		/* Function to render the page */
    XArray	PosParams;		/* Positioning parameter listing. */
    XArray	Properties;		/* Properties this thing will have. */
    XArray	Events;			/* Events for this widget type */
    XArray	Actions;		/* Actions on this widget type */
    XArray	PseudoTypes;		/* Pseudo-types this widget driver will handle */
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


/*** structure for event handlers for a DOM event ***/
typedef struct
    {
    char	DomEvent[40];
    XArray	Handlers;		/*  xarray of char*  */
    }
    HtDomEvent, *pHtDomEvent;


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
    XArray	Wgtr;			/* code for wgtr-building function */
    /*HtNameArray	EventScripts;*/		/* various event script code */
    XArray	EventHandlers;		/*  xarray of pHtDomEvent  */
    HtNamespace	RootNamespace;
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
    unsigned int CSSBox:1; /* Puts border outside declared box width */
    unsigned int CSSClip:1; /* Clipping rectangle starts at border, not content */
    unsigned int HTML40:1; /* W3C HTML 4.0 */
    unsigned int JS15:1; /* Javacript 1.5 */
    unsigned int XMLHttpRequest:1; /* MSIE/Mozilla XMLHttpRequest method capability */
    }
    HtCapabilities, *pHtCapabilities;

/** Session structure, used for page creation **/
typedef struct
    {
    HtPage	Page;			/* the generated page... */
    pHtTree	Tree;			/* tree page metainfo structure */
    int		DisableBody;
    char*	Tmpbuf;			/* temp buffer used in _va() functions */
    size_t	TmpbufSize;
    HtCapabilities Capabilities;	/* the capabilities supported by the browser */
    pHtClass	Class;			/* the widget class to use (see centrallix/htmlgen/README section: 'server-side classes')**/
    pStruct	Params;			/* params from the user */
    pObjSession	ObjSession;		/* objectsystem session */
    int		Width;			/* target container (browser) width in pixels */
    int		Height;			/* target container height in pixels */
    char*	GraftPoint;		/* name of target container */
    pWgtrVerifySession VerifySession;	/* name of the current verification session */
    pWgtrClientInfo ClientInfo;
    pHtNamespace Namespace;		/* current namespace */
    int		IsDynamic;
    }
    HtSession, *pHtSession;



/** Flags for layer addition routines **/
#define HTR_LAYER_F_DEFAULT	0	/* no flags */
#define HTR_LAYER_F_DYNAMIC	1	/* layer will be reloaded from server */

#ifndef __GNUC__
#define __attribute__(a) /* hide function attributes from non-GCC compilers */
#endif

/** Flags for div formatting/styling **/
#define HTR_DIV_F_VISIBLE	1	/* set if div is visible */
#define HTR_DIV_F_OVERFLOW	2	/* set if overflow is visible */

/** Rendering engine functions **/
int htrAddHeaderItem(pHtSession s, char* html_text);
/*int htrAddHeaderItem_va(pHtSession s, char* fmt, ... ) __attribute__((format(printf, 2, 3)));*/
int htrAddHeaderItem_va(pHtSession s, char* fmt, ... );
int htrAddBodyItem(pHtSession s, char* html_text);
/*int htrAddBodyItem_va(pHtSession s, char* fmt, ... ) __attribute__((format(printf, 2, 3)));*/
int htrAddBodyItem_va(pHtSession s, char* fmt, ... );
int htrAddBodyParam(pHtSession s, char* html_param);
/*int htrAddBodyParam_va(pHtSession s, char* fmt, ... ) __attribute__((format(printf, 2, 3)));*/
int htrAddBodyParam_va(pHtSession s, char* fmt, ... );
int htrAddEventHandler(pHtSession s, char* event_src, char* event, char* drvname, char* handler_code);
int htrAddEventHandlerFunction(pHtSession s, char* event_src, char* event, char* drvname, char* function);
int htrAddScriptFunction(pHtSession s, char* fn_name, char* fn_text, int flags);
int htrAddScriptGlobal(pHtSession s, char* var_name, char* initialization, int flags);
int htrAddScriptInit(pHtSession s, char* init_text);
/*int htrAddScriptInit_va(pHtSession s, char* fmt, ... ) __attribute__((format(printf, 2, 3)));*/
int htrAddScriptInit_va(pHtSession s, char* fmt, ... );
int htrAddScriptCleanup(pHtSession s, char* init_text);
/*int htrAddScriptCleanup_va(pHtSession s, char* fmt, ... ) __attribute__((format(printf, 2, 3)));*/
int htrAddScriptCleanup_va(pHtSession s, char* fmt, ... );
int htrAddScriptInclude(pHtSession s, char* filename, int flags);
int htrAddStylesheetItem(pHtSession s, char* html_text);
/*int htrAddStylesheetItem_va(pHtSession s, char* fmt, ... ) __attribute__((format(printf, 2, 3)));*/
int htrAddStylesheetItem_va(pHtSession s, char* fmt, ... );
int htrFormatDiv(pHtSession s, char* id, int flags, int x, int y, int w, int h, int z, char* style_prefix);

int htrAddExpression(pHtSession s, char* objname, char* property, pExpression exp);
int htrCheckAddExpression(pHtSession s, pWgtrNode tree, char* w_name, char* property);
int htrDisableBody(pHtSession s);
int htrRenderWidget(pHtSession session, pWgtrNode widget, int z);
int htrRenderSubwidgets(pHtSession s, pWgtrNode widget, int zlevel);
int htrCheckNSTransition(pHtSession s, pWgtrNode parent, pWgtrNode child);
int htrCheckNSTransitionReturn(pHtSession s, pWgtrNode parent, pWgtrNode child);

int htrAddScriptWgtr(pHtSession s, char* wgtr_text);
/*int htrAddScriptWgtr_va(pHtSession s, char* fmt, ... ) __attribute__((format(printf, 2, 3))); */
int htrAddScriptWgtr_va(pHtSession s, char* fmt, ... );
int htrAddWgtrObjLinkage(pHtSession s, pWgtrNode widget, char* linkage);
/*int htrAddWgtrObjLinkage_va(pHtSession s, pWgtrNode widget, char* fmt, ...) __attribute__((format(printf, 3, 4)));*/
int htrAddWgtrObjLinkage_va(pHtSession s, pWgtrNode widget, char* fmt, ...);
int htrAddWgtrCtrLinkage(pHtSession s, pWgtrNode widget, char* linkage);
/*int htrAddWgtrCtrLinkage_va(pHtSession s, pWgtrNode widget, char* fmt, ...) __attribute__((format(printf, 3, 4)));*/
int htrAddWgtrCtrLinkage_va(pHtSession s, pWgtrNode widget, char* fmt, ...);
int htrAddWgtrInit(pHtSession s, pWgtrNode widget, char* func, char* paramfmt, ...);
int htrAddNamespace(pHtSession s, pWgtrNode container, char* nspace, int is_subns);
int htrLeaveNamespace(pHtSession s);

/** Utility routines **/
int htrGetBackground(pWgtrNode tree, char* prefix, int as_style, char* buf, int buflen);
int htrGetBoolean(pWgtrNode obj, char* attr, int default_value);

/** Content-intelligent (useragent-sensitive) rendering engine functions **/
int htrAddBodyItemLayer_va(pHtSession s, int flags, char* id, int cnt, const char* fmt, ...);
int htrAddBodyItemLayerStart(pHtSession s, int flags, char* id, int cnt);
int htrAddBodyItemLayerEnd(pHtSession s, int flags);

/** Administrative functions **/
int htrRegisterDriver(pHtDriver drv);
int htrInitialize();
int htrRender(pFile output, pObjSession s, pWgtrNode tree, pStruct params, pWgtrClientInfo c_info);
int htrAddAction(pHtDriver drv, char* action_name);
int htrAddEvent(pHtDriver drv, char* event_name);
int htrAddParam(pHtDriver drv, char* eventaction, char* param_name, int datatype);
pHtDriver htrAllocDriver();
int htrAddSupport(pHtDriver drv, char* className);
char* htrParamValue(pHtSession s, char* paramname);
pHtDriver htrLookupDriver(pHtSession s, char* type_name);
int htrBuildClientWgtr(pHtSession s, pWgtrNode tree);

/** For the rule module... **/
int htruleRegister(char* ruletype, ...);

#endif /* _HT_RENDER_H */

