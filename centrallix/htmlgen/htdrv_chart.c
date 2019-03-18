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
    char name[64];
    char sbuf[160];
    char cht_bgnd[128];
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
    } htcht_struct;

int
htchtRenderDynamic(pHtSession s, pWgtrNode tree, int z, htcht_struct* t)
    {
	if(!s->Capabilities.Dom0NS && !s->Capabilities.Dom1HTML)
	    {
	    mssError(1,"HTTBL","Netscape 4 DOM or W3C DOM support required");
	    return -1;
	    }

	/** STYLE for the layer **/
	htrAddStylesheetItem_va(s,"\t#chtd%POSpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%INTpx; TOP:%INTpx; WIDTH:%POSpx; Z-INDEX:%POS; } \n",t->id,t->x,t->y,(t->overlap_scrollbar)?(t->w):(t->w-18),z+0);
	htrAddStylesheetItem_va(s,"\t#chtd%POSbox { POSITION:absolute; VISIBILITY:inherit; LEFT:0px; TOP:18px; WIDTH:16px; HEIGHT:16px; Z-INDEX:%POS; BORDER: solid 1px; BORDER-COLOR: white gray gray white; }\n",t->id,z+1);

	htrAddScriptGlobal(s,"chtd_current","null",0);
	htrAddScriptGlobal(s,"chtdb_current","null",0);
	htrAddScriptGlobal(s,"chtdx_current","null",0);
	htrAddScriptGlobal(s,"chtdb_start","null",0);
	htrAddScriptGlobal(s,"chtdbdbl_current","null",0);

	htrAddScriptInclude(s, "/sys/js/htdrv_chart.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	htrAddScriptInit_va(s,"    chtd_init({item:'foo'");

	htrAddScriptInit(s,"});\n");

	htrAddBodyItem_va(s,"<DIV ID=\"chtd%POSpane\">\n",t->id); 

	htrAddBodyItem(s,"<P> CHART HERE <\P>");
	htrAddBodyItem(s,"</DIV>\n");

    return 0;
    }



/*** htchtRender - generate the HTML code for the page.
 ***/
int
htchtRender(pHtSession s, pWgtrNode tree, int z)
    {
    char* ptr;
    int rval;
    htcht_struct* t;

	/** Don't try to render table-column, etc.  We do that elsewhere **/
	wgtrGetPropertyValue(tree,"outer_type",DATA_T_STRING,POD(&ptr));
	if (strcmp(ptr, "widget/chart") != 0)
	    return 0;

	t = (htcht_struct*)nmMalloc(sizeof(htcht_struct));
	if (!t) return -1;
	memset(t, 0, sizeof(htcht_struct));

	t->x=-1;
	t->y=-1;
    
    	/** Get an id for thit. **/
	t->id = (HTTBL.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (wgtrGetPropertyValue(tree,"x",DATA_T_INTEGER,POD(&(t->x))) != 0) t->x = -1;
	if (wgtrGetPropertyValue(tree,"y",DATA_T_INTEGER,POD(&(t->y))) != 0) t->y = -1;
	if (wgtrGetPropertyValue(tree,"width",DATA_T_INTEGER,POD(&(t->w))) != 0) t->w = -1;
	if (wgtrGetPropertyValue(tree,"height",DATA_T_INTEGER,POD(&(t->h))) != 0)
	    {
	    mssError(1,"HTTBL","'height' property is required");
	    return -1;
	    }

	/** Get name **/
	if (wgtrGetPropertyValue(tree,"name",DATA_T_STRING,POD(&ptr)) != 0) 
	    {
	    nmFree(t, sizeof(htcht_struct));
	    return -1;
	    }
	strtcpy(t->name,ptr,sizeof(t->name));

	if (wgtrGetPropertyValue(tree,"objectsource",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(t->osrc,ptr,sizeof(t->osrc));
	else
	    strcpy(t->osrc,"");
	
	/** Text color information **/
	if (wgtrGetPropertyValue(tree,"textcolor",DATA_T_STRING,POD(&ptr)) == 0)
	    strtcpy(t->textcolor,ptr,sizeof(t->textcolor));


	rval = htchtRenderDynamic(s, tree, z, t);

	nmFree(t, sizeof(htcht_struct));

    return rval;
    }


/*** htchtInitialize - register with the ht_render module.
 ***/
int
htchtInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Chart Driver");
	strcpy(drv->WidgetName,"chart");
	drv->Render = htchtRender;

	htrAddEvent(drv,"Click");
	htrAddEvent(drv,"DblClick");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTTBL.idcnt = 0;

    return 0;
    }
