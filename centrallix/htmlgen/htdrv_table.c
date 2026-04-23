#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2026 LightSys Technology Services, Inc.		*/
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
/*									*/
/* Module: 	htdrv_table.c						*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 29, 1999 					*/
/* Description:	HTML Widget driver for a data-driven table.  Has three	*/
/*		different modes -- static, dynamicpage, and dynamicrow.	*/
/*									*/
/*		Static means an inline table that can't be updated	*/
/*		without a parent container being completely reloaded.	*/
/*		DynamicPage means a table in a layer that can be	*/
/*		reloaded dynamically as a whole when necessary.  Good	*/
/*		when you need forward/back without reloading the page.	*/
/*		DynamicRow means each row is its own layer.  Good when	*/
/*		you need to insert rows dynamically and delete rows	*/
/*		dynamically at the client side without reloading the	*/
/*		whole table contents.					*/
/*									*/
/*		A static table's query is performed on the server side	*/
/*		and the HTML is generated at the server.  Both dynamic	*/
/*		types are built from a client-side query.  Static 	*/
/*		tables are generally best when the data will be read-	*/
/*		only.  Dynamicrow tables use the most client resources.	*/
/************************************************************************/

/*** This file uses the optional Comment Anchors VSCode extension, documented
 *** with CommentAnchorsExtension.md in centrallix-sysdoc.
 ***/


#define HTTBL_MAX_COLS		(64)

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTBL;


typedef struct
    {
    char wname[64];
    char wnamespace[32];
    char title[64];
    char fieldname[64];
    char sort_fieldname[64];
    char caption_fieldname[64];
    char caption_textcolor[64];
    char wrap[16];
    char align[16];
    char type[16];
    int width;
    int image_maxwidth;
    int image_maxheight;
    char group[64];
    }
    httbl_col;


typedef struct
    {
    char name[64];
    char hdr_bgnd[128];
    char colsep_bgnd[128];
    char textcolor[64];
    char textcolorhighlight[64];
    char titlecolor[64];
    char newrow_bgnd[128];
    char newrow_textcolor[64];
    char osrc[64];
    httbl_col* col_infs[HTTBL_MAX_COLS];
    int ncols;
    int id;
    int x,y,w,h;
    int data_mode;		/* 0="rows" or 1="properties" */
    int inner_padding;
    int windowsize;
    int min_rowheight;
    int max_rowheight;
    int cellhspacing;
    int cellvspacing;
    int dragcols;
    int colsep;
    int colsep_mode;
    int grid_in_empty_rows;
    int allow_selection;
    int show_selection;
    int initial_selection;
    int allow_deselection;
    int overlap_scrollbar;	/* scrollbar overlaps with table */
    int hide_scrollbar;		/* don't show scrollbar at all */
    int demand_scrollbar;	/* only show scrollbar when needed */
    int has_header;		/* table has header/title row? */
    int rowcache_size;		/* number of rows the table caches for display */
    } httbl_struct;


int
httblRenderDynamic(pHtSession s, pWgtrNode tree, int z, httbl_struct* t)
    {
    char* ptr;

	/** Verify browser capabilities. **/
	if (!s->Capabilities.Dom1HTML || !s->Capabilities.Dom2CSS)
	    {
	    mssError(1, "HTTBL", "Unsupported browser: W3C DOM1 HTML and DOM2 CSS support required.");
	    goto err;
	    }

	/** Write CSS for the table base element. **/
	const int content_width = (t->overlap_scrollbar) ? (t->w) : (t->w - 18);
	if (htrAddStylesheetItem_va(s,
	    "\t\t#tbld%POSbase { "
		"position:absolute; "
		"visibility:inherit; "
		"left:"ht_flex_format"; "
		"top:"ht_flex_format"; "
		"width:"ht_flex_format"; "
		"height:"ht_flex_format"; "
		"z-index:%POS; "
	    "}\n",
	    t->id,
	    ht_flex_x(t->x, tree),
	    ht_flex_y(t->y, tree),
	    ht_flex_w(content_width, tree),
	    ht_flex_h(t->h, tree),
	    z + 0
	) != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write base table CSS.");
	    goto err;
	    }
	
	/** Write CSS for the table scrollbar. **/
	const int row_start_y = (t->has_header) ? (t->min_rowheight + t->cellvspacing) : 0;
	if (htrAddStylesheetItem_va(s,
	    "\t\t#tbld%POSscroll { "
		"position:absolute; "
		"visibility:%STR; "
		"left:"ht_flex_format"; "
		"top:"ht_flex_format"; "
		"width:18px; "
		"height:"ht_flex_format"; "
		"z-index:%POS; "
	    "}\n",
	    t->id,
	    (t->hide_scrollbar || t->demand_scrollbar) ? "hidden" : "inherit", /** TODO: Greg - This logic looks fishy. Why does `demand_scrollbar = true` hide the scrollbar?? **/
	    ht_flex(t->x + t->w - 18, ht_get_parent_w(tree), ht_get_fl_x(tree) + ht_get_fl_w(tree)),
	    ht_flex_y(t->y + row_start_y, tree),
	    ht_flex_h(t->h - row_start_y, tree),
	    z + 0
	) != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write table scrollbar CSS.");
	    goto err;
	    }
	
	/** Write CSS for the table scroll thumb. **/
	if (htrAddStylesheetItem_va(s,
	    "\t\t#tbld%POSthumb { "
		"position:absolute; "
		"visibility:inherit; "
		"left:0px; "
		"top:18px; "
		"width:16px; "
		"height:16px; "
		"z-index:%POS; "
		"border:solid 1px; "
		"border-color:white gray gray white; "
	    "}\n",
	    t->id,
	    z + 1
	) != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write table scroll thumb CSS.");
	    goto err;
	    }
	
	/** Link to the table base object. **/
	if (htrAddWgtrObjLinkage_va(s, tree, "tbld%POSbase", t->id) != 0) goto err;

	/** Add globals for scripts. **/
	if (htrAddScriptGlobal(s, "tbld_current",     "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "tbldb_current",    "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "tbldb_start",      "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "tbldbdbl_current", "null", 0) != 0) goto err;
	if (htrAddScriptGlobal(s, "tbldx_current",    "null", 0) != 0) goto err;

	/** Include scripts. **/
	if (htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0) != 0) goto err;
	if (htrAddScriptInclude(s, "/sys/js/htdrv_table.js", 0) != 0) goto err;
	
	/** Begin writing the js initialization call. **/
	if (htrAddScriptInit_va(s, "\ttbld_init({") != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write JS to start table init call.");
	    goto err;
	    }
	
	/** Write identification data. **/
	const int has_osrc = (t->osrc != NULL && t->osrc[0] != '\0');
	if (htrAddScriptInit_va(s,
	    "name:'%STR&SYM', "
	    "table:wgtrGetNodeRef(ns, '%STR&SYM'), "
	    "scroll:htr_subel("
		"wgtrGetParentContainer(wgtrGetNodeRef(ns, '%STR&SYM')), "
		"'tbld%POSscroll'"
	    "), "
	    "osrc:%['%STR&SYM'%]%[null%], ",
	    t->name, t->name, t->name, t->id,
	    (has_osrc), t->osrc, (!has_osrc)
	) != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write JS table identification data.");
	    goto err;
	    }
	
	/** Write layout data. **/
	if (htrAddScriptInit_va(s,
	    "height:%INT, "
	    "width:%INT, "
	    "innerpadding:%INT, "
	    "min_rowheight:%INT, "
	    "max_rowheight:%INT, "
	    "cellhspacing:%INT, "
	    "cellvspacing:%INT, ",
	    t->h, content_width,
	    t->inner_padding,
	    t->min_rowheight, t->max_rowheight,
	    t->cellhspacing, t->cellvspacing
	) != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write JS table layout data.");
	    goto err;
	    }
	
	/** Write selection data. **/
	if (htrAddScriptInit_va(s,
	    "allow_selection:%INT, "
	    "show_selection:%INT, "
	    "initial_selection:%INT, "
	    "allow_deselection:%INT, ",
	    t->allow_selection,
	    t->show_selection,
	    t->initial_selection,
	    t->allow_deselection
	) != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write JS table selection data.");
	    goto err;
	    }
	
	/** Write scrollbar data. **/
	if (htrAddScriptInit_va(s,
	    "thumb_name:'tbld%POSthumb', "
	    "demand_sb:%INT, ",
	    t->id,
	    t->demand_scrollbar
	) != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write JS table scrollbar data.");
	    goto err;
	    }
	
	/** Write theme data (colors, backgrounds, etc.). **/
	if (htrAddScriptInit_va(s,
	    "textcolor:'%STR&JSSTR', "
	    "textcolorhighlight:'%STR&JSSTR', "
	    "titlecolor:'%STR&JSSTR', "
	    "colsep_bgnd:'%STR&JSSTR', "
	    "hdrbgnd:'%STR&JSSTR', "
	    "newrow_bgnd:'%STR&JSSTR', "
	    "newrow_textcolor:'%STR&JSSTR', ",
	    t->textcolor, t->textcolorhighlight,
	    t->titlecolor, t->colsep_bgnd, t->hdr_bgnd,
	    t->newrow_bgnd, t->newrow_textcolor
	) != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write JS table theme data.");
	    goto err;
	    }
	
	/** Write general row data. **/
	if (htrAddScriptInit_va(s,
	    "dm:%INT, "
	    "hdr:%INT, "
	    "grid_in_empty_rows:%INT, "
	    "windowsize:%INT, "
	    "rcsize:%INT, ",
	    t->data_mode,
	    t->has_header,
	    t->grid_in_empty_rows,
	    t->windowsize,
	    t->rowcache_size
	) != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write JS table general row data.");
	    goto err;
	    }
	
	/** Write general column data. **/
	if (htrAddScriptInit_va(s,
	    "dragcols:%INT, "
	    "colsep:%INT, "
	    "colsep_mode:%INT, ",
	    t->dragcols, t->colsep, t->colsep_mode
	) != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write JS table general column data.");
	    goto err;
	    }
	
	/** Write the cols array with data for each column. **/
	/** ANCHOR[id=table-column] **/
	if (htrAddScriptInit_va(s, "cols:[") != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write JS for the start of the table columns list.");
	    goto err;
	    }
	for (int colid = 0; colid < t->ncols; colid++)
	    {
	    httbl_col* col = t->col_infs[colid];
	    if (htrAddScriptInit_va(s,
		"{ "
		    "name:'%STR&JSSTR', "
		    "ns:'%STR&JSSTR', "
		    "fieldname:'%STR&JSSTR', "
		    "sort_fieldname:'%STR&JSSTR', "
		    "title:'%STR&JSSTR', "
		    "width:%INT, "
		    "type:'%STR&JSSTR', "
		    "group:%POS, "
		    "align:'%STR&JSSTR', "
		    "wrap:'%STR&JSSTR', "
		    "caption_fieldname:'%STR&JSSTR', "
		    "caption_textcolor:'%STR&JSSTR', "
		    "image_maxwidth:%POS, "
		    "image_maxheight:%POS, "
		" }, ",
		col->wname,
		col->wnamespace,
		col->fieldname,
		col->sort_fieldname,
		col->title,
		col->width,
		col->type,
		col->group[0] ? 1 : 0,
		col->align,
		col->wrap,
		col->caption_fieldname,
		col->caption_textcolor,
		col->image_maxwidth,
		col->image_maxheight
	    ) != 0)
		{
		mssError(0, "HTTBL",
		    "Failed to write JS for table column #%d/%d, aka. \"%s\".",
		    colid + 1, t->ncols, col->wname
		);
		goto err;
		}
	    }

	/** Null terminate the array, and finish writing the init call. **/
	if (htrAddScriptInit(s, "null]});\n") != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write JS to finish init call.");
	    goto err;
	    }

	/** Write HTML for the table base container. **/
	if (htrAddBodyItem_va(s, "<div id='tbld%POSbase'>\n", t->id) != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write HTML opening tag for table container.");
	    goto err;
	    }

	/** Write the table row detail elements. **/
	/** ANCHOR[id=table-row-detail] **/
	int sub_count = 0;
	pWgtrNode children[32]; /** Warning: Large local variable in stack. **/
	const int detail_count = wgtrGetMatchingChildList(tree, "widget/table-row-detail", children, sizeof(children)/sizeof(pWgtrNode));
	for (int i = 0; i < detail_count; i++)
	    {
	    pWgtrNode sub_tree = children[i];
	    
	    /** Only affect table-row-detail widgets. **/
	    wgtrGetPropertyValue(sub_tree, "outer_type", DATA_T_STRING,POD(&ptr));
	    if (strcmp(ptr, "widget/table-row-detail") == 0)
		{
		sub_count++;
		if (htrCheckNSTransition(s, tree, sub_tree) != 0) goto err_detail;

		/** Write CSS for the table row detail. **/
		int h;
		if (wgtrGetPropertyValue(sub_tree, "height", DATA_T_INTEGER, POD(&h)) != 0) h = t->min_rowheight;
		if (htrAddStylesheetItem_va(s,
		    "\t\t#tbld%POSsub%POS { "
			"position:absolute; "
			"visibility:hidden; "
			"left:0px; "
			"top:0px; "
			"width:"ht_flex_format"; "
			"height:"ht_flex_format"; "
			"z-index:%POS; "
		    "}\n",
		    t->id, sub_count,
		    ht_flex_w(t->w - (t->demand_scrollbar ? 0 : 18), tree),
		    ht_flex_h(h, tree),
		    z + 1
		) != 0)
		    {
		    mssError(0, "HTTBL", "Failed to write HTML opening tag for table container.");
		    goto err_detail;
		    }
		
		/** Write HTML (including subwidgets). **/
		if (htrAddBodyItem_va(s, "<div id='tbld%POSsub%POS'>\n", t->id, sub_count) != 0)
		    {
		    mssError(0, "HTTBL", "Failed to write HTML opening tag for table row detail.");
		    goto err_detail;
		    }
		if (htrRenderSubwidgets(s, sub_tree, z + 2) != 0)
		    {
		    mssError(0, "HTTBL", "Failed to write widgets in table row detail.");
		    goto err_detail;
		    }
		if (htrAddBodyItem(s, "</div>\n") != 0)
		    {
		    mssError(0, "HTTBL", "Failed to write HTML closing tag for table container.");
		    goto err_detail;
		    }
		
		/** Add linkage. **/
		if (htrAddWgtrObjLinkage_va(s, sub_tree, "tbld%POSsub%POS", t->id, sub_count)) goto err_detail;
		
		/** Add 'display_for'. **/
		if (wgtrGetPropertyValue(sub_tree, "name", DATA_T_STRING, POD(&ptr)) != 0)
		    {
		    mssError(1, "HTTBL", "Failed to get name of table row detail widget.");
		    goto err_detail;
		    }
		if (htrCheckAddExpression(s, sub_tree, ptr, "display_for") < 0) goto err;

		if (htrCheckNSTransitionReturn(s, tree, sub_tree) != 0) goto err;
		}
	    
	    /** Success. **/
	    continue;
	    
    err_detail:
	    mssError(0, "HTTBL", "Failed to write HTML opening tag for table container.");
	    goto err;
	    }
	
	/** Render other children. **/
	if (htrRenderSubwidgets(s, tree, z + 2) != 0) goto err;

	/** Close the base container. **/
	if (htrAddBodyItem(s, "</div>\n") != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write HTML closing tag for table container.");
	    goto err;
	    }

	/** Write HTML for the scrollbar. **/
	if (htrAddBodyItem_va(s,
	    "<div id='tbld%POSscroll'>\n"
		"<table border='0' cellspacing='0' cellpadding='0' width='18'>\n"
		    "<tr><td><img src='/sys/images/ico13b.gif' name='u'></td></tr>\n"
		    "<tr><td id='tbld%POSscrarea' style='height:"ht_flex_format"'></td></tr>\n"
		    "<tr><td><img src='/sys/images/ico12b.gif' name='d'></td></tr>\n"
		"</table>\n"
		"<div id='tbld%POSthumb'></div>\n"
	    "</div>\n",
	    t->id,
	    t->id, ht_flex_h(t->h - row_start_y - 2*18, tree),
	    t->id
	) != 0)
	    {
	    mssError(0, "HTTBL", "Failed to write HTML for table scrollbar.");
	    goto err;
	    }

	/** Register event handlers. **/
	if (htrAddEventHandlerFunction(s, "document", "CONTEXTMENU", "tbld", "tbld_contextmenu") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "KEYDOWN",     "tbld", "tbld_keydown")     != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEDOWN",   "tbld", "tbld_mousedown")   != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEMOVE",   "tbld", "tbld_mousemove")   != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOUT",    "tbld", "tbld_mouseout")    != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEOVER",   "tbld", "tbld_mouseover")   != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "MOUSEUP",     "tbld", "tbld_mouseup")     != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "TOUCHCANCEL", "tbld", "tbld_touchcancel") != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "TOUCHEND",    "tbld", "tbld_touchend")    != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "TOUCHMOVE",   "tbld", "tbld_touchmove")   != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "TOUCHSTART",  "tbld", "tbld_touchstart")  != 0) goto err;
	if (htrAddEventHandlerFunction(s, "document", "WHEEL",       "tbld", "tbld_wheel")       != 0) goto err;

	/** Success. **/
	return 0;

    err:
	mssError(0, "HTTBL",
	    "Failed to render \"%s\":\"%s\" (id: %d).",
	    tree->Name, tree->Type, t->id
	);
	return -1;
    }



/*** httblRender - generate the HTML code for the page.
 ***/
int
httblRender(pHtSession s, pWgtrNode tree, int z)
    {
    pWgtrNode sub_tree;
    char* ptr;
    int n, i;
    httbl_struct* t;
    int rval;
    pWgtrNode children[HTTBL_MAX_COLS];
    httbl_col* col;

	/*** Only render "widget/table". Other widgets (e.g. table-column or
	 *** table-row-detail) are rendered elsewhere (see links below).
	 *** 
	 *** LINK #table-column
	 *** LINK #table-row-detail
	 ***/
	wgtrGetPropertyValue(tree,"outer_type",DATA_T_STRING,POD(&ptr));
	if (strcmp(ptr, "widget/table") != 0)
	    return 0;

	t = (httbl_struct*)nmMalloc(sizeof(httbl_struct));
	if (!t) return -1;
	memset(t, 0, sizeof(httbl_struct));

	/** Get an id. **/
	t->id = (HTTBL.idcnt++);

	/** Get name. **/
	if (wgtrGetPropertyValue(tree, "name", DATA_T_STRING, POD(&ptr)) != 0) 
	    {
	    nmFree(t, sizeof(httbl_struct));
	    return -1;
	    }
	strtcpy(t->name, ptr, sizeof(t->name));

	/** Get object source path. **/
	if (wgtrGetPropertyValue(tree, "objectsource", DATA_T_STRING, POD(&ptr)) != 0)
	    strcpy(t->osrc, "");
	else
	    strtcpy(t->osrc, ptr, sizeof(t->osrc));

	/** Get the location, size, and layout data. **/
	if (wgtrGetPropertyValue(tree, "x", DATA_T_INTEGER, POD(&(t->x))) != 0) t->x = -1;
	if (wgtrGetPropertyValue(tree, "y", DATA_T_INTEGER, POD(&(t->y))) != 0) t->y = -1;
	if (wgtrGetPropertyValue(tree, "width", DATA_T_INTEGER, POD(&(t->w))) != 0) t->w = -1;
	if (wgtrGetPropertyValue(tree, "height", DATA_T_INTEGER, POD(&(t->h))) != 0)
	    {
	    mssError(1,"HTTBL","'height' property is required");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree, "rowheight", DATA_T_INTEGER, POD(&n)) != 0)
	    {
	    t->min_rowheight = s->ClientInfo->ParagraphHeight + 2;
	    t->max_rowheight = -1;
	    }
	else
	    {
	    t->min_rowheight = t->max_rowheight = n;
	    }
	if (wgtrGetPropertyValue(tree, "inner_padding", DATA_T_INTEGER, POD(&(t->inner_padding))) != 0) t->inner_padding = 0;
	if (wgtrGetPropertyValue(tree, "min_rowheight", DATA_T_INTEGER, POD(&(t->min_rowheight))) != 0); /* Keep value from above. */
	if (wgtrGetPropertyValue(tree, "max_rowheight", DATA_T_INTEGER, POD(&(t->max_rowheight))) != 0); /* Keep value from above. */
	if (wgtrGetPropertyValue(tree, "cellhspacing",  DATA_T_INTEGER, POD(&(t->cellhspacing)))  != 0) t->cellhspacing = 1;
	if (wgtrGetPropertyValue(tree, "cellvspacing",  DATA_T_INTEGER, POD(&(t->cellvspacing)))  != 0) t->cellvspacing = 1;
	
	/** Get selection data. **/
	t->allow_selection = htrGetBoolean(tree, "allow_selection", 1);
	t->show_selection = htrGetBoolean(tree, "show_selection", 1);
	if (wgtrGetPropertyType(tree, "initial_selection") == DATA_T_STRING
	    && wgtrGetPropertyValue(tree, "initial_selection", DATA_T_STRING, POD(&ptr)) == 0
	    && strcasecmp(ptr, "noexpand") == 0)
	    t->initial_selection = 2;
	else
	    t->initial_selection = htrGetBoolean(tree, "initial_selection", 1);
	t->allow_deselection = htrGetBoolean(tree, "allow_deselection", (t->initial_selection) ? 0 : 1);

	/** Get scrollbar data. **/
	t->overlap_scrollbar = htrGetBoolean(tree, "overlap_scrollbar", 0);
	t->hide_scrollbar = htrGetBoolean(tree, "hide_scrollbar", 0);
	t->demand_scrollbar = htrGetBoolean(tree, "demand_scrollbar", 0);
	
	/** Get theme data (colors, backgrounds, etc.). **/
	if (wgtrGetPropertyValue(tree, "textcolor", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(t->textcolor, ptr, sizeof(t->textcolor));
	if (wgtrGetPropertyValue(tree, "textcolorhighlight", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(t->textcolorhighlight, ptr, sizeof(t->textcolorhighlight));
	if (wgtrGetPropertyValue(tree, "titlecolor", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(t->titlecolor, ptr, sizeof(t->titlecolor));
	if (!*t->titlecolor) strcpy(t->titlecolor, t->textcolor);
	htrGetBackground(tree, "colsep", !s->Capabilities.Dom0NS, t->colsep_bgnd, sizeof(t->colsep_bgnd));
	htrGetBackground(tree, "hdr", !s->Capabilities.Dom0NS, t->hdr_bgnd, sizeof(t->hdr_bgnd));
	htrGetBackground(tree, "newrow", !s->Capabilities.Dom0NS, t->newrow_bgnd, sizeof(t->newrow_bgnd));
	if (wgtrGetPropertyValue(tree, "textcolornew", DATA_T_STRING, POD(&ptr)) == 0)
	    strtcpy(t->newrow_textcolor, ptr, sizeof(t->newrow_textcolor));
	
	/** Get general row data. **/
	if (wgtrGetPropertyValue(tree, "data_mode", DATA_T_STRING, POD(&ptr)) != 0)
	    t->data_mode = 0;
	else
	    {
	    if (strcasecmp(ptr, "rows") == 0) t->data_mode = 0;
	    else if (strcasecmp(ptr, "properties") == 0) t->data_mode = 1;
	    else
		{
		mssError(1, "TBL", "Invalid value for attribute 'data_mode': %s", ptr);
		return -1;
		}
	    }
	t->has_header = htrGetBoolean(tree, "titlebar", 1); /* Whether to render a header row. */
	t->grid_in_empty_rows = htrGetBoolean(tree, "gridinemptyrows", 1); /* Whether to show the grid in empty rows. */
	if (wgtrGetPropertyValue(tree, "windowsize", DATA_T_INTEGER, POD(&(t->windowsize))) != 0) t->windowsize = -1;
	if (wgtrGetPropertyValue(tree, "rowcache_size", DATA_T_INTEGER, POD(&(t->rowcache_size))) != 0) t->rowcache_size = 0;

	/** Get general column data. **/
	t->dragcols = htrGetBoolean(tree, "dragcols", 1);
	if (wgtrGetPropertyValue(tree, "colsep", DATA_T_INTEGER, POD(&(t->colsep))) != 0) t->colsep = 1;
	if (wgtrGetPropertyValue(tree, "colsep_mode", DATA_T_STRING, POD(&ptr)) != 0)
	    t->colsep_mode = 0;
	else
	    {
	    if (strcasecmp(ptr, "full") == 0) t->colsep_mode = 0;
	    else if (strcasecmp(ptr, "header") == 0) t->colsep_mode = 1;
	    else
		{
		mssError(1, "TBL", "Invalid value for attribute 'colsep_mode': %s", ptr);
		return -1;
		}
	    }

	/** Get specific column data for each column. **/
	t->ncols = wgtrGetMatchingChildList(tree, "widget/table-column", children, sizeof(children)/sizeof(pWgtrNode));
	for (i=0;i<t->ncols;i++)
	    {
	    sub_tree = children[i];
	    
	    /** Only read table-column widgets. **/
	    wgtrGetPropertyValue(sub_tree, "outer_type", DATA_T_STRING,POD(&ptr));
	    if (strcmp(ptr, "widget/table-column") == 0)
		{
		/** Allocate space to store the column data. **/
		col = (httbl_col*)nmMalloc(sizeof(httbl_col));
		memset(col, 0, sizeof(*col));
		
		/** Basic column info. **/
		t->col_infs[i] = col;
		strtcpy(col->wname, wgtrGetName(sub_tree), sizeof(col->wname));
		strtcpy(col->wnamespace, wgtrGetNamespace(sub_tree), sizeof(col->wnamespace));

		/** The object system doesn't need to render this widget (we will do that). **/
		sub_tree->RenderFlags |= HT_WGTF_NOOBJECT;

		/** Get column properties **/
		if (wgtrGetPropertyValue(sub_tree, "fieldname", DATA_T_STRING, POD(&ptr)) == 0)
		    strtcpy(col->fieldname, ptr, sizeof(col->fieldname));
		if (wgtrGetPropertyValue(sub_tree, "sort_fieldname", DATA_T_STRING, POD(&ptr)) == 0)
		    strtcpy(col->sort_fieldname, ptr, sizeof(col->sort_fieldname));
		if (wgtrGetPropertyValue(sub_tree, "caption_fieldname", DATA_T_STRING,POD(&ptr)) == 0)
		    strtcpy(col->caption_fieldname, ptr, sizeof(col->caption_fieldname));
		if (wgtrGetPropertyValue(sub_tree, "caption_textcolor", DATA_T_STRING,POD(&ptr)) == 0)
		    strtcpy(col->caption_textcolor, ptr, sizeof(col->caption_textcolor));
		wgtrGetPropertyValue(sub_tree, "width", DATA_T_INTEGER,POD(&(col->width)));
		wgtrGetPropertyValue(sub_tree, "image_maxwidth", DATA_T_INTEGER,POD(&(col->image_maxwidth)));
		wgtrGetPropertyValue(sub_tree, "image_maxheight", DATA_T_INTEGER,POD(&(col->image_maxheight)));
		if (wgtrGetPropertyValue(sub_tree, "title", DATA_T_STRING,POD(&ptr)) == 0)
		    strtcpy(col->title, ptr, sizeof(col->title));
		else
		    strtcpy(col->title, col->fieldname, sizeof(col->title));
		wgtrGetPropertyValue(sub_tree, "name", DATA_T_STRING, POD(&ptr));
		htrCheckAddExpression(s, sub_tree, ptr, "title");
		htrCheckAddExpression(s, sub_tree, ptr, "visible");
		if (wgtrGetPropertyValue(sub_tree, "align", DATA_T_STRING,POD(&ptr)) == 0)
		    strtcpy(col->align, ptr, sizeof(col->align));
		else
		    strcpy(col->align, "left");
		if (wgtrGetPropertyValue(sub_tree, "wrap", DATA_T_STRING,POD(&ptr)) == 0)
		    strtcpy(col->wrap, ptr, sizeof(col->wrap));
		else
		    strcpy(col->wrap, "no");
		if (wgtrGetPropertyValue(sub_tree, "type", DATA_T_STRING,POD(&ptr)) == 0 && (!strcmp(ptr,"text") || !strcmp(ptr,"check") || !strcmp(ptr,"checkbox") || !strcmp(ptr,"image") || !strcmp(ptr,"code") || !strcmp(ptr,"link") || !strcmp(ptr,"progress")))
		    strtcpy(col->type, ptr, sizeof(col->type));
		else
		    strcpy(col->type, "text");
		if (htrGetBoolean(sub_tree, "group_by", 0) == 1)
		    strcpy(col->group, "yes");
		}
	    }

	/** Render the table. **/
	rval = httblRenderDynamic(s, tree, z, t);
	
	/** Clean up. **/
	for(i=0;i<t->ncols;i++)
	    nmFree(t->col_infs[i], sizeof(httbl_col));
	nmFree(t, sizeof(httbl_struct));

    return rval;
    }


/*** httblInitialize - register with the ht_render module.
 ***/
int
httblInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver struct. **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Initialize driver values. **/
	strcpy(drv->Name,"DHTML DataTable Driver");
	strcpy(drv->WidgetName,"table");
	drv->Render = httblRender;
	xaAddItem(&(drv->PseudoTypes), "table-column");
	xaAddItem(&(drv->PseudoTypes), "table-row-detail");

	/** Add driver events. **/
	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"DblClick");

	/** Register the driver, with dhtml support. **/
	htrRegisterDriver(drv);
	htrAddSupport(drv, "dhtml");

	/** Initialize the ID counter. **/
	HTTBL.idcnt = 0;

    return 0;
    }
