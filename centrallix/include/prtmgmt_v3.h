#ifndef _PRTMGMT_V3_H
#define _PRTMGMT_V3_H

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

/**CVSDATA***************************************************************

    $Id: prtmgmt_v3.h,v 1.4 2002/10/21 22:55:11 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/include/prtmgmt_v3.h,v $

    $Log: prtmgmt_v3.h,v $
    Revision 1.4  2002/10/21 22:55:11  gbeeley
    Added font/size test in test_prt to test the alignment of different fonts
    and sizes on one line or on separate lines.  Fixed lots of bugs in the
    font baseline alignment logic.  Added prt_internal_Dump() to debug the
    document's structure.  Fixed a YSort bug where it was not sorting the
    YPrev/YNext pointers but the Prev/Next ones instead, and had a loop
    condition problem causing infinite looping as well.  Fixed some problems
    when adding an empty obj to a stream of objects and then modifying
    attributes which would change the object's geometry.

    There are still some glitches in the line spacing when different font
    sizes are used, however.

    Revision 1.3  2002/10/18 22:01:38  gbeeley
    Printing of text into an area embedded within a page now works.  Two
    testing options added to test_prt: text and printfile.  Use the "output"
    option to redirect output to a file or device instead of to the screen.
    Word wrapping has also been tested/debugged and is functional.  Added
    font baseline logic to the design.

    Revision 1.2  2002/04/25 04:30:13  gbeeley
    More work on the v3 print formatting subsystem.  Subsystem compiles,
    but report and uxprint have not been converted yet, thus problems.

    Revision 1.1  2002/01/27 22:50:05  gbeeley
    Untested and incomplete print formatter version 3 files.
    Initial checkin.


 **END-CVSDATA***********************************************************/

#include "xarray.h"
#include "xhash.h"


/*** Layout Manager Structure ***/
typedef struct _PLM
    {
    int			Magic;
    char		Name[32];
    int			(*AddObject)();		/* adds a child object to a parent */
    int			(*ChildResizeReq)();	/* can we resize the child object in this container? */
    int			(*ChildResized)();	/* child object in this container *was* resized. */
    int			(*InitContainer)();	/* init a newly created container */
    int			(*DeinitContainer)();	/* de-init a container (before deallocation) */
    int			(*Break)();		/* request a break on this container (e.g., pagebreak) */
    int			(*ChildBreakReq)();	/* child object is requesting this container to break */
    int			(*Resize)();		/* request a resize on this container */
    }
    PrtLayoutMgr, *pPrtLayoutMgr;


/*** Print Object Type structure ***/
typedef struct _POT
    {
    int			Magic;
    int			TypeID;			/* PRT_OBJ_T_xxx */
    char		TypeName[32];
    pPrtLayoutMgr	PrefLayoutMgr;
    }
    PrtObjType, *pPrtObjType;


/*** Text style definition ***/
typedef struct _PTS
    {
    int			FontID;			/* ID from PRTMGMT.FontList mapping */
    int			FontSize;		/* point size of font */
    int			Attr;			/* PRT_OBJ_A_xxx */
    int			Color;			/* 0x00RRGGBB */
    }
    PrtTextStyle, *pPrtTextStyle;


/*** Print Object Stream structure ***/
typedef struct _POS
    {
    int			Magic;
    struct _POS*	Next;			/* next sequentially in stream */
    struct _POS*	Prev;			/* previous sequentially in stream */
    struct _POS*	ContentHead;		/* Contents of this container */
    struct _POS*	ContentTail;
    struct _POS*	Parent;			/* Container of this object */
    struct _POS*	LinkNext;		/* Pointer to object where content should continue */
    struct _POS*	LinkPrev;		/* Pointer to object where content continued from */
    struct _POS*	YNext;
    struct _POS*	YPrev;
    pPrtObjType		ObjType;
    pPrtLayoutMgr	LayoutMgr;
    void*		Session;		/* pPrtObjSession */
    int			nOpens;			/* total open subcontainers + container */
    int			Flags;			/* PRT_OBJ_F_xxx */
    int			BGColor;		/* 0x00RRGGBB */
    int			FGColor;		/* 0x00RRGGBB */
    int			ObjID;			/* such as page number for PAGE objects */
    PrtTextStyle	TextStyle;
    int			Justification;		/* PRT_JUST_T_xxx */
    double		PageX;
    double		PageY;
    double		X;			/* Relative X position to container origin */
    double		Y;			/* Relative Y position to container origin */
    double		YBase;			/* Relative Y baseline to the object's Y position */
    double		Width;			/* Width of object */
    double		Height;			/* Height of object */
    double		MarginLeft;
    double		MarginRight;
    double		MarginTop;
    double		MarginBottom;
    double		LineHeight;		/* Height of lines... */
    unsigned char*	Content;		/* Text content or image bitmap */
    }
    PrtObjStream, *pPrtObjStream;

#define PRTSESSION(x) ((pPrtSession)((x)->Session))


/*** Units-of-measure conversion data ***/
typedef struct _PU
    {
    char		Name[32];
    double		AdjX;
    double		AdjY;
    }
    PrtUnits, *pPrtUnits;


/*** Font ID descriptor ***/
typedef struct _PFN
    {
    int			FontID;
    char		FontName[40];
    }
    PrtFontDesc, *pPrtFontDesc;


/*** Resolution definition structure ***/
typedef struct _PRE
    {
    int			Xres;
    int			Yres;
    int			Colors;		/* PRT_COLOR_T_xxx */
    }
    PrtResolution, *pPrtResolution;


/*** Formatter structure ***/
typedef struct _PFM
    {
    int			Magic;
    char		Name[32];
    int			Priority;
    void*		(*Probe)();
    int			(*Generate)();
    int			(*GetNearestFontSize)();
    double		(*GetCharacterMetric)();
    double		(*GetCharacterBaseline)();
    int			(*Close)();
    }
    PrtFormatter, *pPrtFormatter;


/*** Output Driver structure ***/
typedef struct _PD
    {
    int			Magic;
    char		Name[32];
    char		ContentType[64];
    void*		(*Open)();
    int			(*Close)();
    pXArray		(*GetResolutions)();
    int			(*SetResolution)();
    int			(*SetTextStyle)();
    int			(*GetNearestFontSize)();
    double		(*GetCharacterMetric)();
    double		(*GetCharacterBaseline)();
    int			(*SetHPos)();
    int			(*SetVPos)();
    int			(*WriteText)();
    int			(*WriteRasterData)();
    int			(*WriteFF)();
    }
    PrtOutputDriver, *pPrtOutputDriver;


/*** Print Session structure ***/
typedef struct _PS
    {
    int			Magic;
    pPrtObjStream	StreamHead;
    double		PageWidth;
    double		PageHeight;
    pPrtUnits		Units;
    int			(*WriteFn)();
    void*		WriteArg;
    pPrtFormatter	Formatter;
    int			ResolutionX;	/* in DPI, always */
    int			ResolutionY;	/* in DPI, always */
    void*		FormatterData;
    int			FocusHandle;
    }
    PrtSession, *pPrtSession;


/*** Print Object Handle structure ***/
typedef struct _PHD
    {
    int			Magic;
    int			HandleID;
    union
        {
	pPrtObjStream	Object;
	pPrtSession	Session;
	void*		Generic;
	}
	Ptr;
    }
    PrtHandle, *pPrtHandle;


/*** Print management global structure ***/
typedef struct _PG
    {
    XArray		UnitsList;
    XArray		TypeList;
    XArray		LayoutMgrList;
    XArray		FormatterList;
    XArray		FormatDriverList;
    XArray		FontList;
    XHashTable		HandleTable;
    XHashTable		HandleTableByPtr;
    int			NextHandleID;
    }
    PrtGlobals, *pPrtGlobals;

extern PrtGlobals PRTMGMT;


#define PRT_OBJ_F_FLOWAROUND	    1		/* multiple lines can flow around this */
#define PRT_OBJ_F_NOLINESEQ	    2		/* do not honor sequence within the line */
#define PRT_OBJ_F_OPEN     	    4		/* container is open for writing */
#define PRT_OBJ_F_NEWLINE  	    8		/* object starts a new line (for textflow) */
#define PRT_OBJ_F_SOFTNEWLINE	    16		/* object starts a soft new line (from wordwrapping) */
#define PRT_OBJ_F_ALLOWBREAK	    32		/* allow object to be broken for pagination/wordwrap */
#define PRT_OBJ_F_REPEAT	    64		/* object repeats on subsequent pages */
#define PRT_OBJ_F_CALLBACK	    128		/* do an API callback to get the object's content */
#define PRT_OBJ_F_FIXEDSIZE	    256		/* object's size cannot be resized */
#define PRT_OBJ_F_REQCOMPLETE	    512		/* require completion of content before emitting the page */
#define PRT_OBJ_F_XSET		    1024	/* X location is pre-set. */
#define PRT_OBJ_F_YSET		    2048	/* Y location is pre-set. */

/** these flags can be used by the api caller **/
#define PRT_OBJ_U_FLOWAROUND	    PRT_OBJ_F_FLOWAROUND
#define PRT_OBJ_U_NOLINESEQ	    PRT_OBJ_F_NOLINESEQ
#define PRT_OBJ_U_ALLOWBREAK	    PRT_OBJ_F_ALLOWBREAK
#define PRT_OBJ_U_REPEAT	    PRT_OBJ_F_REPEAT
#define PRT_OBJ_U_FIXEDSIZE	    PRT_OBJ_F_FIXEDSIZE
#define PRT_OBJ_U_REQCOMPLETE	    PRT_OBJ_F_REQCOMPLETE
#define PRT_OBJ_U_XSET		    PRT_OBJ_F_XSET
#define PRT_OBJ_U_YSET		    PRT_OBJ_F_YSET

#define PRT_OBJ_UFLAGMASK	    (PRT_OBJ_U_FLOWAROUND | PRT_OBJ_U_NOLINESEQ | PRT_OBJ_U_ALLOWBREAK | PRT_OBJ_U_REPEAT | PRT_OBJ_U_FIXEDSIZE | PRT_OBJ_U_REQCOMPLETE | PRT_OBJ_U_XSET | PRT_OBJ_U_YSET)

#define PRT_OBJ_A_BOLD		    1
#define PRT_OBJ_A_ITALIC	    2
#define PRT_OBJ_A_UNDERLINE	    4

#define PRT_OBJ_T_DOCUMENT	    1		/* whole document */
#define PRT_OBJ_T_PAGE		    2		/* one page of the document */
#define PRT_OBJ_T_AREA		    3		/* a textflow-managed area */
#define PRT_OBJ_T_STRING	    4		/* a string of text content */
#define PRT_OBJ_T_IMAGE		    5		/* an image/picture */
#define PRT_OBJ_T_RECT		    6		/* rectangle, solid color */
#define PRT_OBJ_T_TABLE		    7		/* tabular-data */
#define PRT_OBJ_T_TABLEROW	    8		/* table row */
#define PRT_OBJ_T_TABLECELL	    9		/* table cell */
#define PRT_OBJ_T_SECTION	    10		/* columnar section */

#define PRT_COLOR_T_MONO	    1		/* black/white only image */
#define PRT_COLOR_T_GREY	    2		/* greyscale image */
#define PRT_COLOR_T_FULL	    3		/* RGB color image */

#define PRT_FONT_T_MONOSPACE	    1		/* typically courier */
#define PRT_FONT_T_SANSSERIF	    2		/* typically helvetica */
#define PRT_FONT_T_SERIF	    3		/* typically times */
#define PRT_FONT_T_USBARCODE	    4		/* u.s.a. postal barcode */

#define PRT_JUST_T_LEFT		    0
#define PRT_JUST_T_RIGHT	    1
#define PRT_JUST_T_CENTER	    2
#define PRT_JUST_T_FULL		    3


/*** System functions ***/
int prtInitialize();
int prtRegisterUnits(char* units_name, double x_adj, double y_adj);
pPrtObjType prtAllocType();
int prtRegisterType(pPrtObjType type);
pPrtLayoutMgr prtAllocLayoutMgr();
int prtRegisterLayoutMgr(pPrtLayoutMgr layout_mgr);
pPrtLayoutMgr prtLookupLayoutMgr(char* layout_mgr);
pPrtUnits prtLookupUnits(char* units_name);
int prtRegisterFont(char* font_name, int font_id);
int prtLookupFont(char* font_name);
char* prtLookupFontName(int font_id);
int prtAllocHandle(void* ptr);
void* prtHandlePtr(int handle_id);
int prtUpdateHandle(int handle_id, void* ptr);
int prtUpdateHandleByPtr(void* old_ptr, void* ptr);
int prtFreeHandle(int handle_id);
pPrtFormatter prtAllocFormatter();
int prtRegisterFormatter(pPrtFormatter fmt);

/*** Session-level functions ***/
pPrtSession prtOpenSession(char* output_type, int (*write_fn)(), void* write_arg);
int prtCloseSession(pPrtSession s);
int prtSetPageGeometry(pPrtSession s, double width, double height);
int prtGetPageGeometry(pPrtSession s, double *width, double *height);
int prtSetUnits(pPrtSession s, char* units_name);
char* prtGetUnits(pPrtSession s);
int prtSetResolution(pPrtSession s, int dpi);

/** Internal management functions **/
pPrtObjStream prt_internal_AllocObj(char* type);
pPrtObjStream prt_internal_AllocObjByID(int type_id);
int prt_internal_FreeObj(pPrtObjStream obj);
int prt_internal_Add(pPrtObjStream parent, pPrtObjStream new_child);
int prt_internal_CopyAttrs(pPrtObjStream src, pPrtObjStream dst);
int prt_internal_CopyGeom(pPrtObjStream src, pPrtObjStream dst);
double prt_internal_GetFontHeight(pPrtObjStream obj);
double prt_internal_GetFontBaseline(pPrtObjStream obj);
double prt_internal_GetStringWidth(pPrtObjStream obj, char* str, int n);
pPrtObjStream prt_internal_YSort(pPrtObjStream obj);
int prt_internal_FreeTree(pPrtObjStream obj);
int prt_internal_GeneratePage(pPrtSession s, pPrtObjStream page);
pPrtObjStream prt_internal_GetPage(pPrtObjStream obj);
pPrtObjStream prt_internal_AddEmptyObj(pPrtObjStream container);
pPrtObjStream prt_internal_CreateEmptyObj(pPrtObjStream container);
int prt_internal_Dump(pPrtObjStream obj);

/** Strict-formatter to output-driver interfacing **/
pPrtOutputDriver prt_strictfm_AllocDriver();
int prt_strictfm_RegisterDriver(pPrtOutputDriver drv);

/** These macros are used for units conversion **/
#define prtUnitX(s,x) ((x)*((s)->Units->AdjX))
#define prtUnitY(s,y) ((y)*((s)->Units->AdjY))
#define prtUsrUnitX(s,x) ((x)/((s)->Units->AdjX))
#define prtUsrUnitY(s,y) ((y)/((s)->Units->AdjY))

/*** General Printing Functions ***/
int prtGetPageRef(pPrtSession s);
int prtSetFocus(pPrtSession s, int handle_id);
int prtSetPageNumber(int handle_id, int new_pagenum);
int prtGetPageNumber(int handle_id);

/*** Formatting functions ***/
int prtSetJustification(int handle_id, int just_mode);	/* PRT_JUST_T_xxx */
int prtGetJustification(int handle_id);
int prtSetLineHeight(int handle_id,double line_height);
double prtGetLineHeight(int handle_id);
int prtSetTextStyle(int handle_id, pPrtTextStyle style);
int prtGetTextStyle(int handle_id, pPrtTextStyle *style);
int prtSetAttr(int handle_id, int attrs);		/* PRT_OBJ_A_xxx */
int prtGetAttr(int handle_id);
int prtSetFont(int handle_id, char* fontname);
char* prtGetFont(int handle_id);
int prtSetFontSize(int handle_id, int pt_size);
int prtGetFontSize(int handle_id);
int prtSetColor(int handle_id, int font_color);
int prtGetColor(int handle_id);
int prtSetHPos(int handle_id, double x);
int prtSetVPos(int handle_id, double y);

/*** Printing content functions ***/
int prtWriteString(int handle_id, char* str);
int prtWriteNL(int handle_id);
int prtWriteFF(int handle_id);

/*** Print object creation functions ***/
int prtAddObject(int handle_id, int obj_type, double x, double y, double width, double height, int flags);
int prtSetObjectCallback(int handle_id, void* (*callback_fn)(), int is_pre);
int prtEndObject(int handle_id);


#endif /* defined _PRTMGMT_V3_H */

