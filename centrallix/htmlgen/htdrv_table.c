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


#define HTTBL_MAX_COLS		(24)

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTBL;

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
    int x,y,w,h;
    int id;
    int mode;
    int data_mode;		/* 0="rows" or 1="properties" */
    int outer_border;
    int inner_border;
    int inner_padding;
    pStructInf col_infs[HTTBL_MAX_COLS];
    int ncols;
    int windowsize;
    int rowheight;
    int cellhspacing;
    int cellvspacing;
    int followcurrent;
    int dragcols;
    int colsep;
    int gridinemptyrows;
    int allow_selection;
    int show_selection;
    int reverse_order;
    int overlap_scrollbar;	/* scrollbar overlaps with table */
    int hide_scrollbar;		/* don't show scrollbar at all */
    int demand_scrollbar;	/* only show scrollbar when needed */
    int has_header;		/* table has header/title row? */
    } httbl_struct;

int
httblRenderDynamic(pHtSession s, pWgtrNode tree, int z, httbl_struct* t)
    {
    int colid;
    int colw;
    char *coltype;
    char *coltitle;
    char *ptr;
    char *colalign;
    int i;
    pWgtrNode sub_tree;
    int subcnt = 0;
    char *nptr;
    int h;
    int first_offset = (t->has_header)?(t->rowheight):0;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
	    {
	    mssError(1,"HTTBL","Netscape 4 DOM or W3C DOM support required");
	    return -1;
	    }

	/** STYLE for the layer **/
	htrAddStylesheetItem_va(s,"\t#tbld%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; } \n",t->id,t->x,t->y,(t->overlap_scrollbar)?(t->w):(t->w-18),z+1);
	htrAddStylesheetItem_va(s,"\t#tbld%POSscroll { POSITION:absolute; VISIBILITY:%STR; LEFT:%INTpx; TOP:%INTpx; WIDTH:18px; HEIGHT:%POSpx; Z-INDEX:%POS; }\n",t->id,(t->hide_scrollbar || t->demand_scrollbar)?"hidden":"inherit",t->x+t->w-18,t->y+first_offset,t->h-first_offset,z+1);
	htrAddStylesheetItem_va(s,"\t#tbld%POSbox { POSITION:absolute; VISIBILITY:inherit; LEFT:0px; TOP:18px; WIDTH:16px; HEIGHT:16px; Z-INDEX:%POS; BORDER: solid 1px; BORDER-COLOR: white gray gray white; }\n",t->id,z+2);

	/** HTML body <DIV> element for the layer. **/
	htrAddBodyItem_va(s,"<DIV ID=\"tbld%POSpane\"></DIV>\n",t->id);
	htrAddBodyItem_va(s,"<DIV ID=\"tbld%POSscroll\">\n",t->id);
	htrAddBodyItem(s,"<TABLE border=0 cellspacing=0 cellpadding=0 width=18>\n");
	htrAddBodyItem(s,"<TR><TD><IMG SRC=/sys/images/ico13b.gif NAME=u></TD></TR>\n");
	htrAddBodyItem_va(s,"<TR><TD height=%POS></TD></TR>\n",t->h-2*18-first_offset-t->cellvspacing);
	htrAddBodyItem(s,"<TR><TD><IMG SRC=/sys/images/ico12b.gif NAME=d></TD></TR>\n");
	htrAddBodyItem(s,"</TABLE>\n");
	/*htrAddBodyItem_va(s,"<DIV ID=\"tbld%POSbox\"><IMG SRC=/sys/images/ico14b.gif NAME=b></DIV>\n",t->id);*/
	htrAddBodyItem_va(s,"<DIV ID=\"tbld%POSbox\"></DIV>\n",t->id);

	htrAddScriptGlobal(s,"tbld_current","null",0);
	htrAddScriptGlobal(s,"tbldb_current","null",0);
	htrAddScriptGlobal(s,"tbldb_start","null",0);
	htrAddScriptGlobal(s,"tbldbdbl_current","null",0);

	htrAddScriptInclude(s, "/sys/js/htdrv_table.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	htrAddWgtrObjLinkage_va(s, tree, "htr_subel(_parentctr, \"tbld%POSpane\")",t->id);

	htrAddScriptInit_va(s,"    tbld_init({tablename:'%STR&SYM', table:nodes[\"%STR&SYM\"], scroll:htr_subel(wgtrGetContainer(wgtrGetParent(nodes[\"%STR&SYM\"])),\"tbld%POSscroll\"), boxname:\"tbld%POSbox\", name:\"%STR&SYM\", height:%INT, width:%INT, innerpadding:%INT, innerborder:%INT, windowsize:%INT, rowheight:%INT, cellhspacing:%INT, cellvspacing:%INT, textcolor:\"%STR&JSSTR\", textcolorhighlight:\"%STR&JSSTR\", titlecolor:\"%STR&JSSTR\", rowbgnd1:\"%STR&JSSTR\", rowbgnd2:\"%STR&JSSTR\", rowbgndhigh:\"%STR&JSSTR\", hdrbgnd:\"%STR&JSSTR\", followcurrent:%INT, dragcols:%INT, colsep:%INT, colsep_bgnd:\"%STR&JSSTR\", gridinemptyrows:%INT, reverse_order:%INT, allow_selection:%INT, show_selection:%INT, overlap_sb:%INT, hide_sb:%INT, demand_sb:%INT, osrc:%['%STR&SYM'%]%[null%], dm:%INT, hdr:%INT, newrow_bgnd:\"%STR&JSSTR\", newrow_textcolor:\"%STR&JSSTR\", cols:[",
		t->name,t->name,t->name,t->id,t->id,t->name,t->h,
		(t->overlap_scrollbar)?t->w:t->w-18,
		t->inner_padding,t->inner_border,t->windowsize,t->rowheight,
		t->cellvspacing, t->cellhspacing,t->textcolor, 
		t->textcolorhighlight, t->titlecolor,t->row_bgnd1,t->row_bgnd2,
		t->row_bgndhigh,t->hdr_bgnd,t->followcurrent,t->dragcols,
		t->colsep,t->colsep_bgnd,t->gridinemptyrows, t->reverse_order,
		t->allow_selection, t->show_selection,
		t->overlap_scrollbar, t->hide_scrollbar, t->demand_scrollbar,
		*(t->osrc) != '\0', t->osrc, *(t->osrc) == '\0',
		t->data_mode, t->has_header,
		t->newrow_bgnd, t->newrow_textcolor);
	
	for(colid=0;colid<t->ncols;colid++)
	    {
	    stAttrValue(stLookup(t->col_infs[colid],"title"),NULL,&coltitle,0);
	    stAttrValue(stLookup(t->col_infs[colid],"align"),NULL,&colalign,0);
	    stAttrValue(stLookup(t->col_infs[colid],"type"),NULL,&coltype,0);
	    stAttrValue(stLookup(t->col_infs[colid],"width"),&colw,NULL,0);
	    htrAddScriptInit_va(s,"[\"%STR&JSSTR\",\"%STR&JSSTR\",%INT,\"%STR&JSSTR\",%POS,\"%STR&JSSTR\"],",
		    t->col_infs[colid]->Name,coltitle,colw,coltype,stLookup(t->col_infs[colid],"group")?1:0,colalign);
	    }

	htrAddScriptInit(s,"null]});\n");

	for (i=0;i<xaCount(&(tree->Children));i++)
	    {
	    sub_tree = xaGetItem(&(tree->Children), i);
	    wgtrGetPropertyValue(sub_tree, "outer_type", DATA_T_STRING,POD(&ptr));
	    wgtrGetPropertyValue(sub_tree, "name", DATA_T_STRING,POD(&nptr));
	    if (strcmp(ptr, "widget/table-row-detail") == 0)
		{
		if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&h)) != 0) h = t->rowheight;
		htrAddStylesheetItem_va(s,"\t#tbld%POSsub%POS { POSITION:absolute; VISIBILITY:hidden; LEFT:0px; TOP:0px; WIDTH:%POSpx; HEIGHT:%POSpx; Z-INDEX:%POS; } \n",
			t->id, ++subcnt, t->w-18, h, z+2);
		htrAddBodyItem_va(s,"<DIV ID=\"tbld%POSsub%POS\"></DIV>\n", t->id, subcnt);
		htrRenderSubwidgets(s, sub_tree, z+3);
		htrAddBodyItem(s,"</DIV>\n");
		htrAddWgtrObjLinkage_va(s, sub_tree, "htr_subel(_parentctr, \"tbld%POSsub%POS\")", t->id, subcnt);
		htrCheckAddExpression(s, sub_tree, nptr, "visible");
		}
	    else if (strcmp(ptr,"widget/table-column") != 0) //got columns earlier
		{
		htrRenderWidget(s, sub_tree, z+3);
		}
	    }

	htrAddBodyItem(s,"</DIV>\n");

	htrAddEventHandlerFunction(s,"document","MOUSEOVER","tbld","tbld_mouseover");
	htrAddEventHandlerFunction(s,"document","MOUSEOUT","tbld","tbld_mouseout");
	htrAddEventHandlerFunction(s,"document","MOUSEDOWN","tbld","tbld_mousedown");
	htrAddEventHandlerFunction(s, "document","MOUSEMOVE","tbld","tbld_mousemove");
	htrAddEventHandlerFunction(s, "document","MOUSEUP","tbld","tbld_mouseup");
	if (s->Capabilities.Dom1HTML)
	    htrAddEventHandlerFunction(s, "document", "CONTEXTMENU", "tbld", "tbld_contextmenu");

    return 0;
    }


int
httblRenderStatic(pHtSession s, pWgtrNode tree, int z, httbl_struct* t)
    {
    pObject qy_obj;
    pObjQuery qy;
    char* ptr;
    char* sql;
    int rowid,type,rval;
    char* attr;
    char* str;
    ObjData od;
    int colid;
    int n;

	htrAddScriptInclude(s, "/sys/js/htdrv_table.js", 0);

	/** flag ourselves as not having an associated layer **/
	tree->RenderFlags |= HT_WGTF_NOOBJECT;

	htrAddBodyItem_va(s,"<TABLE %[width=%POS%] border=%POS cellspacing=0 cellpadding=0 %STR><TR><TD>\n", 
		t->w >= 0, t->w - (t->outer_border + (t->outer_border?1:0))*2, t->outer_border, t->tbl_bgnd);
	htrAddBodyItem_va(s,"<TABLE border=0 background=/sys/images/trans_1.gif cellspacing=%POS cellpadding=%POS %[width=%POS%]>\n",
		t->inner_border, t->inner_padding, t->w >= 0, t->w - (t->outer_border + (t->outer_border?1:0))*2);
	if (wgtrGetPropertyValue(tree,"sql",DATA_T_STRING,POD(&sql)) != 0)
	    {
	    mssError(1,"HTTBL","Static datatable must have SQL property");
	    return -1;
	    }
	qy = objMultiQuery(s->ObjSession, sql, NULL, 0);
	if (!qy)
	    {
	    mssError(0,"HTTBL","Could not open query for static datatable");
	    return -1;
	    }
	rowid = 0;
	while((qy_obj = objQueryFetch(qy, O_RDONLY)))
	    {
	    if (rowid == 0)
		{
		/** Do table header if header data provided. **/
		htrAddBodyItem_va(s,"    <TR %STR>", t->hdr_bgnd);
		if (t->ncols == 0)
		    {
		    for(colid=0,attr = objGetFirstAttr(qy_obj); attr; colid++,attr = objGetNextAttr(qy_obj))
			{
			if (colid==0)
			    {
			    htrAddBodyItem_va(s,"<TH align=left><IMG name=\"xy_%STR&SYM_\" src=/sys/images/trans_1.gif align=top>", t->name);
			    }
			else
			    htrAddBodyItem(s,"<TH align=left>");
			if (*(t->titlecolor))
			    {
			    htrAddBodyItem_va(s,"<FONT color='%STR&HTE'>",t->titlecolor);
			    }
			htrAddBodyItem(s,attr);
			if (*(t->titlecolor)) htrAddBodyItem(s,"</FONT>");
			htrAddBodyItem(s,"</TH>");
			}
		    }
		else
		    {
		    for(colid = 0; colid < t->ncols; colid++)
			{
			attr = t->col_infs[colid]->Name;
			if (colid==0)
			    {
			    htrAddBodyItem_va(s,"<TH align=left><IMG name=\"xy_%STR&SYM_\" src=/sys/images/trans_1.gif align=top>", t->name);
			    }
			else
			    {
			    htrAddBodyItem(s,"<TH align=left>");
			    }
			if (*(t->titlecolor))
			    {
			    htrAddBodyItem_va(s,"<FONT color='%STR&HTE'>",t->titlecolor);
			    }
			if (stAttrValue(stLookup(t->col_infs[colid],"title"), NULL, &ptr, 0) == 0)
			    htrAddBodyItem(s,ptr);
			else
			    htrAddBodyItem(s,attr);
			if (*(t->titlecolor)) htrAddBodyItem(s,"</FONT>");
			htrAddBodyItem(s,"</TH>");
			}
		    }
		htrAddBodyItem(s,"</TR>\n");
		}
	    htrAddBodyItem_va(s,"    <TR %STR>", (rowid&1)?((*(t->row_bgnd2))?t->row_bgnd2:t->row_bgnd1):t->row_bgnd1);

	    /** Build the row contents -- loop through attrs and convert to strings **/
	    colid = 0;
	    if (t->ncols == 0)
		attr = objGetFirstAttr(qy_obj);
	    else
		attr = t->col_infs[colid]->Name;
	    while(attr)
		{
		if (t->ncols && stAttrValue(stLookup(t->col_infs[colid],"width"),&n,NULL,0) == 0 && n >= 0)
		    {
		    htrAddBodyItem_va(s,"<TD width=%POS nowrap>",n*7);
		    }
		else
		    {
		    htrAddBodyItem(s,"<TD nowrap>");
		    }
		type = objGetAttrType(qy_obj,attr);
		rval = objGetAttrValue(qy_obj,attr,type,&od);
		if (rval == 0)
		    {
		    if (type == DATA_T_INTEGER || type == DATA_T_DOUBLE)
			str = objDataToStringTmp(type, (void*)(&od), 0);
		    else
			str = objDataToStringTmp(type, (void*)(od.String), 0);
		    }
		else if (rval == 1)
		    {
		    str = "NULL";
		    }
		else
		    {
		    str = NULL;
		    }
		if (colid==0)
		    {
		    htrAddBodyItem_va(s,"<IMG name=\"xy_%STR&SYM_%STR&HTE\" src=/sys/images/trans_1.gif align=top>", t->name, str?str:"");
		    }
		if (*(t->textcolor))
		    {
		    htrAddBodyItem_va(s,"<FONT COLOR=\"%STR&HTE\">",t->textcolor);
		    }
		if (str) htrAddBodyItem(s,str);
		if (*(t->textcolor))
		    {
		    htrAddBodyItem(s,"</FONT>");
		    }
		htrAddBodyItem(s,"</TD>");

		/** Next attr **/
		if (t->ncols == 0)
		    attr = objGetNextAttr(qy_obj);
		else
		    attr = (colid < t->ncols-1)?(t->col_infs[++colid]->Name):NULL;
		}
	    htrAddBodyItem(s,"</TR>\n");
	    objClose(qy_obj);
	    rowid++;
	    }
	objQueryClose(qy);
	htrAddBodyItem(s,"</TABLE></TD></TR></TABLE>\n");

	/** Call init function **/
 	htrAddScriptInit_va(s,"    tbls_init({parentLayer:wgtrGetContainer(wgtrGetParent(nodes[\"%STR&SYM\"])), name:\"%STR&SYM\", width:%INT, cp:%INT, cs:%INT});\n",t->name,t->name,t->w,t->inner_padding,t->inner_border);
 
    return 0;
    }


/*** httblRender - generate the HTML code for the page.
 ***/
int
httblRender(pHtSession s, pWgtrNode tree, int z)
    {
    pWgtrNode sub_tree;
    char* ptr;
    pStructInf attr_inf;
    int n, i;
    httbl_struct* t;
    int rval;
    pWgtrNode children[HTTBL_MAX_COLS];

	t = (httbl_struct*)nmMalloc(sizeof(httbl_struct));
	if (!t) return -1;

	t->tbl_bgnd[0]='\0';
	t->hdr_bgnd[0]='\0';
	t->row_bgnd1[0]='\0';
	t->row_bgnd2[0]='\0';
	t->row_bgndhigh[0]='\0';
	t->textcolor[0]='\0';
	t->textcolorhighlight[0]='\0';
	t->titlecolor[0]='\0';
	t->x=-1;
	t->y=-1;
	t->mode=0;
	t->outer_border=0;
	t->inner_border=0;
	t->inner_padding=0;
	t->data_mode = 0;
    
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
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&(t->h))) != 0) t->h = -1;
	if (wgtrGetPropertyValue(tree,"windowsize",DATA_T_INTEGER,POD(&(t->windowsize))) != 0) t->windowsize = -1;
	if (wgtrGetPropertyValue(tree,"rowheight",DATA_T_INTEGER,POD(&(t->rowheight))) != 0) t->rowheight = s->ClientInfo->ParagraphHeight*4/3;
	if (wgtrGetPropertyValue(tree,"cellhspacing",DATA_T_INTEGER,POD(&(t->cellhspacing))) != 0) t->cellhspacing = 1;
	if (wgtrGetPropertyValue(tree,"cellvspacing",DATA_T_INTEGER,POD(&(t->cellvspacing))) != 0) t->cellvspacing = 1;

	if (wgtrGetPropertyValue(tree,"colsep",DATA_T_INTEGER,POD(&(t->colsep))) != 0) t->colsep = 1;

	t->dragcols = htrGetBoolean(tree, "dragcols", 1);
	t->gridinemptyrows = htrGetBoolean(tree, "gridinemptyrows", 1);
	t->allow_selection = htrGetBoolean(tree, "allow_selection", 1);
	t->show_selection = htrGetBoolean(tree, "show_selection", 1);
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
	
	/** Mode of table operation.  Defaults to 0 (static) **/
	if (wgtrGetPropertyValue(tree,"mode",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"static")) t->mode = 0;
	    else if (!strcmp(ptr,"dynamicpage")) t->mode = 1;
	    else if (!strcmp(ptr,"dynamicrow")) t->mode = 2;
	    else
	        {
		mssError(1,"HTTBL","Widget '%s' mode '%s' is invalid.",t->name,ptr);
		nmFree(t, sizeof(httbl_struct));
		return -1;
		}
	    }

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
		/** no layer associated with this guy **/
		sub_tree->RenderFlags |= HT_WGTF_NOOBJECT;
		wgtrGetPropertyValue(sub_tree, "name", DATA_T_STRING,POD(&ptr));
		if (wgtrGetPropertyValue(sub_tree, "fieldname", DATA_T_STRING, POD(&ptr)) < 0)
		    {
		    mssError(1, "HTTBL", "Couldn't get 'fieldname' for '%s'", sub_tree->Name);
		    nmFree(t, sizeof(httbl_struct));
		    return -1;
		    }
		t->col_infs[i] = stCreateStruct(ptr, "widget/table-column");
		attr_inf = stAddAttr(t->col_infs[i], "width");
		if (wgtrGetPropertyValue(sub_tree, "width", DATA_T_INTEGER,POD(&n)) == 0)
		    stAddValue(attr_inf, NULL, n);
		else
		    stAddValue(attr_inf, NULL, -1);
		attr_inf = stAddAttr(t->col_infs[i], "title");
		if (wgtrGetPropertyValue(sub_tree, "title", DATA_T_STRING,POD(&ptr)) == 0)
		    stAddValue(attr_inf, ptr, 0);
		else
		    stAddValue(attr_inf, t->col_infs[i]->Name, 0);
		attr_inf = stAddAttr(t->col_infs[i], "align");
		if (wgtrGetPropertyValue(sub_tree, "align", DATA_T_STRING,POD(&ptr)) == 0)
		    stAddValue(attr_inf, ptr, 0);
		else
		    stAddValue(attr_inf, "left", 0);
		attr_inf = stAddAttr(t->col_infs[i], "type");
		if (wgtrGetPropertyValue(sub_tree, "type", DATA_T_STRING,POD(&ptr)) == 0 && (!strcmp(ptr,"text") || !strcmp(ptr,"check") || !strcmp(ptr,"image") || !strcmp(ptr,"code")))
		    stAddValue(attr_inf, ptr, 0);
		else
		    stAddValue(attr_inf, "text", 0);
		if (htrGetBoolean(sub_tree, "group_by", 0) == 1)
		    {
		    attr_inf = stAddAttr(t->col_infs[i], "group");
		    stAddValue(attr_inf, "yes", 0);
		    }
		}
	    }
	if(t->mode==0)
	    {
	    rval = httblRenderStatic(s, tree, z, t);
	    nmFree(t, sizeof(httbl_struct));
	    }
	else
	    {
	    rval = httblRenderDynamic(s, tree, z, t);
	    nmFree(t, sizeof(httbl_struct));
	    }

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
