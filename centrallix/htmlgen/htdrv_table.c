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
/* Copyright (C) 1999-2007 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_table.c           				*/
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


#define HTTBL_MAX_COLS		(32)

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
    char sbuf[160];
    char tbl_bgnd[128];
    char hdr_bgnd[128];
    char row_bgnd1[128];
    char row_bgnd2[128];
    char row_bgndhigh[128];
    char colsep_bgnd[128];
    char textcolor[64];
    char textcolorhighlight[64];
    char titlecolor[64];
    char newrow_bgnd[128];
    char newrow_textcolor[64];
    char osrc[64];
    char row_border[64];
    char row_shadow_color[64];
    int row_shadow;
    int row_shadow_radius;
    int row_radius;
    int x,y,w,h;
    int id;
    int data_mode;		/* 0="rows" or 1="properties" */
    int outer_border;
    int inner_border;
    int inner_padding;
    httbl_col* col_infs[HTTBL_MAX_COLS];
    int ncols;
    int windowsize;
    int min_rowheight;
    int max_rowheight;
    int cellhspacing;
    int cellvspacing;
    int followcurrent;
    int dragcols;
    int colsep;
    int gridinemptyrows;
    int allow_selection;
    int show_selection;
    int initial_selection;
    int reverse_order;
    int overlap_scrollbar;	/* scrollbar overlaps with table */
    int hide_scrollbar;		/* don't show scrollbar at all */
    int demand_scrollbar;	/* only show scrollbar when needed */
    int has_header;		/* table has header/title row? */
    int rowcache_size;		/* number of rows the table caches for display */
    } httbl_struct;

int
httblRenderDynamic(pHtSession s, pWgtrNode tree, int z, httbl_struct* t)
    {
    int colid;
    char *ptr;
    int i;
    pWgtrNode sub_tree;
    int subcnt = 0;
    char *nptr;
    int h;
    int first_offset = (t->has_header)?(t->min_rowheight + t->cellvspacing):0;
    pWgtrNode children[32];
    int detailcnt;
    httbl_col* col;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
	    {
	    mssError(1,"HTTBL","Netscape 4 DOM or W3C DOM support required");
	    return -1;
	    }

	/** STYLE for the layer **/
	htrAddStylesheetItem_va(s,"\t#tbld%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; } \n",t->id,t->x,t->y,(t->overlap_scrollbar)?(t->w):(t->w-18),z+0);
	htrAddStylesheetItem_va(s,"\t#tbld%POSscroll { POSITION:absolute; VISIBILITY:%STR; LEFT:%INTpx; TOP:%INTpx; WIDTH:18px; HEIGHT:%POSpx; Z-INDEX:%POS; }\n",t->id,(t->hide_scrollbar || t->demand_scrollbar)?"hidden":"inherit",t->x+t->w-18,t->y+first_offset,t->h-first_offset,z+0);
	htrAddStylesheetItem_va(s,"\t#tbld%POSbox { POSITION:absolute; VISIBILITY:inherit; LEFT:0px; TOP:18px; WIDTH:16px; HEIGHT:16px; Z-INDEX:%POS; BORDER: solid 1px; BORDER-COLOR: white gray gray white; }\n",t->id,z+1);

	htrAddScriptGlobal(s,"tbld_current","null",0);
	htrAddScriptGlobal(s,"tbldb_current","null",0);
	htrAddScriptGlobal(s,"tbldx_current","null",0);
	htrAddScriptGlobal(s,"tbldb_start","null",0);
	htrAddScriptGlobal(s,"tbldbdbl_current","null",0);

	htrAddScriptInclude(s, "/sys/js/htdrv_table.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	htrAddWgtrObjLinkage_va(s, tree, "tbld%POSpane",t->id);

	htrAddScriptInit_va(s,"    tbld_init({tablename:'%STR&SYM', table:wgtrGetNodeRef(ns,\"%STR&SYM\"), scroll:htr_subel(wgtrGetParentContainer(wgtrGetNodeRef(ns,\"%STR&SYM\")),\"tbld%POSscroll\"), boxname:\"tbld%POSbox\", name:\"%STR&SYM\", height:%INT, width:%INT, innerpadding:%INT, innerborder:%INT, windowsize:%INT, min_rowheight:%INT, max_rowheight:%INT, cellhspacing:%INT, cellvspacing:%INT, textcolor:\"%STR&JSSTR\", textcolorhighlight:\"%STR&JSSTR\", titlecolor:\"%STR&JSSTR\", rowbgnd1:\"%STR&JSSTR\", rowbgnd2:\"%STR&JSSTR\", rowbgndhigh:\"%STR&JSSTR\", hdrbgnd:\"%STR&JSSTR\", followcurrent:%INT, dragcols:%INT, colsep:%INT, colsep_bgnd:\"%STR&JSSTR\", gridinemptyrows:%INT, reverse_order:%INT, allow_selection:%INT, show_selection:%INT, initial_selection:%INT, overlap_sb:%INT, hide_sb:%INT, demand_sb:%INT, osrc:%['%STR&SYM'%]%[null%], dm:%INT, hdr:%INT, newrow_bgnd:\"%STR&JSSTR\", newrow_textcolor:\"%STR&JSSTR\", rcsize:%INT, cols:[",
		t->name,t->name,t->name,t->id,t->id,t->name,t->h,
		(t->overlap_scrollbar)?t->w:t->w-18,
		t->inner_padding,t->inner_border,t->windowsize,t->min_rowheight, t->max_rowheight,
		t->cellhspacing, t->cellvspacing,t->textcolor, 
		t->textcolorhighlight, t->titlecolor,t->row_bgnd1,t->row_bgnd2,
		t->row_bgndhigh,t->hdr_bgnd,t->followcurrent,t->dragcols,
		t->colsep,t->colsep_bgnd,t->gridinemptyrows, t->reverse_order,
		t->allow_selection, t->show_selection, t->initial_selection,
		t->overlap_scrollbar, t->hide_scrollbar, t->demand_scrollbar,
		*(t->osrc) != '\0', t->osrc, *(t->osrc) == '\0',
		t->data_mode, t->has_header,
		t->newrow_bgnd, t->newrow_textcolor,
		t->rowcache_size);
	
	for(colid=0;colid<t->ncols;colid++)
	    {
	    col = t->col_infs[colid];
	    htrAddScriptInit_va(s,"{name:\"%STR&JSSTR\",ns:\"%STR&JSSTR\",fieldname:\"%STR&JSSTR\",title:\"%STR&JSSTR\",width:%INT,type:\"%STR&JSSTR\",group:%POS,align:\"%STR&JSSTR\",wrap:\"%STR&JSSTR\",caption_fieldname:\"%STR&JSSTR\",caption_textcolor:\"%STR&JSSTR\",image_maxwidth:%POS,image_maxheight:%POS},",
		    col->wname,
		    col->wnamespace,
		    col->fieldname,
		    col->title,
		    col->width,
		    col->type,
		    col->group[0]?1:0,
		    col->align,
		    col->wrap,
		    col->caption_fieldname,
		    col->caption_textcolor,
		    col->image_maxwidth,
		    col->image_maxheight
		    );
	    }

	htrAddScriptInit(s,"null]});\n");

	htrAddBodyItem_va(s,"<DIV ID=\"tbld%POSpane\">\n",t->id);

	detailcnt = wgtrGetMatchingChildList(tree, "widget/table-row-detail", children, sizeof(children)/sizeof(pWgtrNode));
	//for (i=0;i<xaCount(&(tree->Children));i++)
	for (i=0;i<detailcnt;i++)
	    {
	    sub_tree = children[i];
	    //sub_tree = xaGetItem(&(tree->Children), i);
	    //
	    wgtrGetPropertyValue(sub_tree, "outer_type", DATA_T_STRING,POD(&ptr));
	    wgtrGetPropertyValue(sub_tree, "name", DATA_T_STRING,POD(&nptr));

	    if (strcmp(ptr, "widget/table-row-detail") == 0)
		{
		htrCheckNSTransition(s, tree, sub_tree);

		if (wgtrGetPropertyValue(sub_tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = t->min_rowheight;
		htrAddStylesheetItem_va(s,"\t#tbld%POSsub%POS { POSITION:absolute; VISIBILITY:hidden; LEFT:0px; TOP:0px; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; } \n",
			t->id, ++subcnt, t->w-(t->demand_scrollbar?0:18), h, z+1);
		htrAddBodyItem_va(s,"<DIV ID=\"tbld%POSsub%POS\">\n", t->id, subcnt);
		htrRenderSubwidgets(s, sub_tree, z+2);
		htrAddBodyItem(s,"</DIV>\n");
		htrAddWgtrObjLinkage_va(s, sub_tree, "tbld%POSsub%POS", t->id, subcnt);
		htrCheckAddExpression(s, sub_tree, nptr, "display_for");

		htrCheckNSTransitionReturn(s, tree, sub_tree);
		}
	    //else if (strcmp(ptr,"widget/table-column") != 0) //got columns earlier
		//{
		//htrRenderWidget(s, sub_tree, z+3);
		//}
	    }
	htrRenderSubwidgets(s, tree, z+2);

	htrAddBodyItem(s,"</DIV>\n");

	/** HTML body <DIV> element for the scrollbar layer. **/
	htrAddBodyItem_va(s,"<div id=\"tbld%POSscroll\">\n",t->id);
	htrAddBodyItem(s,"<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" width=\"18\">\n");
	htrAddBodyItem(s,"<tr><td><img src=\"/sys/images/ico13b.gif\" name=\"u\"></td></tr>\n");
	htrAddBodyItem_va(s,"<tr><td id=\"tbld%POSscrarea\" height=\"%POS\"></td></tr>\n", t->id, t->h-2*18-first_offset);
	htrAddBodyItem(s,"<tr><td><img src=\"/sys/images/ico12b.gif\" name=\"d\"></td></tr>\n");
	htrAddBodyItem(s,"</table>\n");
	/*htrAddBodyItem_va(s,"<DIV ID=\"tbld%POSbox\"><IMG SRC=/sys/images/ico14b.gif NAME=b></DIV>\n",t->id);*/
	htrAddBodyItem_va(s,"<div id=\"tbld%POSbox\"></div>\n",t->id);
	htrAddBodyItem(s,"</div>\n");

	htrAddEventHandlerFunction(s,"document","MOUSEOVER","tbld","tbld_mouseover");
	htrAddEventHandlerFunction(s,"document","MOUSEOUT","tbld","tbld_mouseout");
	htrAddEventHandlerFunction(s,"document","MOUSEDOWN","tbld","tbld_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE","tbld","tbld_mousemove");
	htrAddEventHandlerFunction(s, "document","MOUSEUP","tbld","tbld_mouseup");
	htrAddEventHandlerFunction(s, "document","WHEEL","tbld","tbld_wheel");
	htrAddEventHandlerFunction(s, "document","KEYDOWN","tbld","tbld_keydown");
	htrAddEventHandlerFunction(s, "document","TOUCHSTART","tbld","tbld_touchstart");
	htrAddEventHandlerFunction(s, "document","TOUCHEND","tbld","tbld_touchend");
	htrAddEventHandlerFunction(s, "document","TOUCHMOVE","tbld","tbld_touchmove");
	htrAddEventHandlerFunction(s, "document","TOUCHCANCEL","tbld","tbld_touchcancel");
	if (s->Capabilities.Dom1HTML)
	    htrAddEventHandlerFunction(s, "document", "CONTEXTMENU", "tbld", "tbld_contextmenu");

    return 0;
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

	/** Don't try to render table-column, etc.  We do that elsewhere **/
	wgtrGetPropertyValue(tree,"outer_type",DATA_T_STRING,POD(&ptr));
	if (strcmp(ptr, "widget/table") != 0)
	    return 0;

	t = (httbl_struct*)nmMalloc(sizeof(httbl_struct));
	if (!t) return -1;
	memset(t, 0, sizeof(httbl_struct));

	t->x=-1;
	t->y=-1;
    
    	/** Get an id for thit. **/
	t->id = (HTTBL.idcnt++);

	/** Backwards compat for the time being **/
	wgtrRenameProperty(tree, "row_bgcolor1", "row1_bgcolor");
	wgtrRenameProperty(tree, "row_background1", "row1_background");
	wgtrRenameProperty(tree, "row_bgcolor2", "row2_bgcolor");
	wgtrRenameProperty(tree, "row_background2", "row2_background");
	wgtrRenameProperty(tree, "row_bgcolorhighlight", "rowhighlight_bgcolor");
	wgtrRenameProperty(tree, "row_backgroundhighlight", "rowhighlight_background");

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&(t->x))) != 0) t->x = -1;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&(t->y))) != 0) t->y = -1;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&(t->w))) != 0) t->w = -1;
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&(t->h))) != 0)
	    {
	    mssError(1,"HTTBL","'height' property is required");
	    return -1;
	    }
	if (wgtrGetPropertyValue(tree,"windowsize",DATA_T_INTEGER,POD(&(t->windowsize))) != 0) t->windowsize = -1;
	if (wgtrGetPropertyValue(tree,"rowheight",DATA_T_INTEGER,POD(&n)) == 0)
	    {
	    t->min_rowheight = t->max_rowheight = n;
	    }
	else
	    {
	    t->min_rowheight = s->ClientInfo->ParagraphHeight + 2;
	    t->max_rowheight = -1;
	    }
	wgtrGetPropertyValue(tree,"min_rowheight",DATA_T_INTEGER,POD(&(t->min_rowheight)));
	wgtrGetPropertyValue(tree,"max_rowheight",DATA_T_INTEGER,POD(&(t->max_rowheight)));
	if (wgtrGetPropertyValue(tree,"cellhspacing",DATA_T_INTEGER,POD(&(t->cellhspacing))) != 0) t->cellhspacing = 1;
	if (wgtrGetPropertyValue(tree,"cellvspacing",DATA_T_INTEGER,POD(&(t->cellvspacing))) != 0) t->cellvspacing = 1;

	if (wgtrGetPropertyValue(tree,"colsep",DATA_T_INTEGER,POD(&(t->colsep))) != 0) t->colsep = 1;

	if (wgtrGetPropertyValue(tree,"rowcache_size",DATA_T_INTEGER,POD(&(t->rowcache_size))) != 0) t->rowcache_size = 0;

	t->dragcols = htrGetBoolean(tree, "dragcols", 1);
	t->gridinemptyrows = htrGetBoolean(tree, "gridinemptyrows", 1);
	t->allow_selection = htrGetBoolean(tree, "allow_selection", 1);
	t->show_selection = htrGetBoolean(tree, "show_selection", 1);
	if (wgtrGetPropertyType(tree, "initial_selection") == DATA_T_STRING && wgtrGetPropertyValue(tree,"initial_selection",DATA_T_STRING,POD(&ptr)) == 0 && !strcasecmp(ptr,"noexpand"))
	    t->initial_selection = 2;
	else
	    t->initial_selection = htrGetBoolean(tree, "initial_selection", 1);
	t->reverse_order = htrGetBoolean(tree, "reverse_order", 0);

	t->overlap_scrollbar = htrGetBoolean(tree, "overlap_scrollbar", 0);
	t->hide_scrollbar = htrGetBoolean(tree, "hide_scrollbar", 0);
	t->demand_scrollbar = htrGetBoolean(tree, "demand_scrollbar", 0);
	t->has_header = htrGetBoolean(tree, "titlebar", 1);

	/** Which data mode to use? **/
	if (wgtrGetPropertyValue(tree,"data_mode", DATA_T_STRING, POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr, "rows"))
		t->data_mode = 0;
	    else if (!strcmp(ptr, "properties"))
		t->data_mode = 1;
	    }

	/** Should we follow the current record around? **/
	t->followcurrent = htrGetBoolean(tree, "followcurrent", 1);

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) 
	    {
	    nmFree(t, sizeof(httbl_struct));
	    return -1;
	    }
	strtcpy(t->name,ptr,sizeof(t->name));

	if (wgtrGetPropertyValue(tree,"objectsource",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(t->osrc,ptr,sizeof(t->osrc));
	else
	    strcpy(t->osrc,"");
	
	/** Get background color/image for table header **/
	htrGetBackground(tree, NULL, !s->Capabilities.Dom0NS, t->tbl_bgnd, sizeof(t->tbl_bgnd));

	/** Get background color/image for header row **/
	htrGetBackground(tree, "hdr", !s->Capabilities.Dom0NS, t->hdr_bgnd, sizeof(t->hdr_bgnd));

	/** Get background color/image for rows **/
	htrGetBackground(tree, "row1", !s->Capabilities.Dom0NS, t->row_bgnd1, sizeof(t->row_bgnd1));
	htrGetBackground(tree, "row2", !s->Capabilities.Dom0NS, t->row_bgnd2, sizeof(t->row_bgnd2));
	htrGetBackground(tree, "rowhighlight", !s->Capabilities.Dom0NS, t->row_bgndhigh, sizeof(t->row_bgndhigh));
	htrGetBackground(tree, "colsep", !s->Capabilities.Dom0NS, t->colsep_bgnd, sizeof(t->colsep_bgnd));
	htrGetBackground(tree, "newrow", !s->Capabilities.Dom0NS, t->newrow_bgnd, sizeof(t->newrow_bgnd));

	/** Get borders and padding information **/
	wgtrGetPropertyValue(tree,"outer_border",DATA_T_INTEGER,POD(&(t->outer_border)));
	wgtrGetPropertyValue(tree,"inner_border",DATA_T_INTEGER,POD(&(t->inner_border)));
	wgtrGetPropertyValue(tree,"inner_padding",DATA_T_INTEGER,POD(&(t->inner_padding)));

	/** Row decorations **/
	wgtrGetPropertyValue(tree, "row_border_color", DATA_T_STRING, POD(&t->row_border));
	wgtrGetPropertyValue(tree, "row_shadow_color", DATA_T_STRING, POD(&t->row_shadow_color));
	wgtrGetPropertyValue(tree, "row_shadow_offset", DATA_T_INTEGER, POD(&t->row_shadow));
	wgtrGetPropertyValue(tree, "row_shadow_radius", DATA_T_INTEGER, POD(&t->row_shadow_radius));
	wgtrGetPropertyValue(tree, "row_border_radius", DATA_T_INTEGER, POD(&t->row_radius));

	/** Text color information **/
	if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(t->textcolor,ptr,sizeof(t->textcolor));

	/** Text color information **/
	if (wgtrGetPropertyValue(tree,"textcolorhighlight",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(t->textcolorhighlight,ptr,sizeof(t->textcolorhighlight));

	/** Text color information for "new row" in process of being created **/
	if (wgtrGetPropertyValue(tree,"textcolornew",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(t->newrow_textcolor,ptr,sizeof(t->newrow_textcolor));

	/** Title text color information **/
	if (wgtrGetPropertyValue(tree,"titlecolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(t->titlecolor,ptr,sizeof(t->titlecolor));
	if (!*t->titlecolor) strcpy(t->titlecolor,t->textcolor);

	/** Get column data **/
	t->ncols = wgtrGetMatchingChildList(tree, "widget/table-column", children, sizeof(children)/sizeof(pWgtrNode));
	for (i=0;i<t->ncols;i++)
	    {
	    sub_tree = children[i];
	    wgtrGetPropertyValue(sub_tree, "outer_type", DATA_T_STRING,POD(&ptr));
	    if (!strcmp(ptr,"widget/table-column") != 0)
		{
		col = (httbl_col*)nmMalloc(sizeof(httbl_col));
		memset(col, 0, sizeof(*col));
		t->col_infs[i] = col;
		strtcpy(col->wname, wgtrGetName(sub_tree), sizeof(col->wname));
		strtcpy(col->wnamespace, wgtrGetNamespace(sub_tree), sizeof(col->wnamespace));

		/** no layer associated with this guy **/
		sub_tree->RenderFlags |= HT_WGTF_NOOBJECT;

		/** Get column properties **/
		if (wgtrGetPropertyValue(sub_tree, "fieldname", DATA_T_STRING, POD(&ptr)) == 0)
		    strtcpy(col->fieldname, ptr, sizeof(col->fieldname));
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
		if (wgtrGetPropertyValue(sub_tree, "align", DATA_T_STRING,POD(&ptr)) == 0)
		    strtcpy(col->align, ptr, sizeof(col->align));
		else
		    strcpy(col->align, "left");
		if (wgtrGetPropertyValue(sub_tree, "wrap", DATA_T_STRING,POD(&ptr)) == 0)
		    strtcpy(col->wrap, ptr, sizeof(col->wrap));
		else
		    strcpy(col->wrap, "no");
		if (wgtrGetPropertyValue(sub_tree, "type", DATA_T_STRING,POD(&ptr)) == 0 && (!strcmp(ptr,"text") || !strcmp(ptr,"check") || !strcmp(ptr,"image") || !strcmp(ptr,"code") || !strcmp(ptr,"link")))
		    strtcpy(col->type, ptr, sizeof(col->type));
		else
		    strcpy(col->type, "text");
		if (htrGetBoolean(sub_tree, "group_by", 0) == 1)
		    strcpy(col->group, "yes");
		}
	    }

	rval = httblRenderDynamic(s, tree, z, t);
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

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML DataTable Driver");
	strcpy(drv->WidgetName,"table");
	drv->Render = httblRender;
	xaAddItem(&(drv->PseudoTypes), "table-column");
	xaAddItem(&(drv->PseudoTypes), "table-row-detail");

	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"DblClick");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTBL.idcnt = 0;

    return 0;
    }
