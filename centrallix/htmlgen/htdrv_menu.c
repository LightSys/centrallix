#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtsession.h"
#include "cxlib/strtcpy.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2000-2005 LightSys Technology Services, Inc.		*/
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
/* Module:      htdrv_menu.c                                            */
/* Author:      Greg Beeley (GRB)                                       */
/* Creation:    Oct. 7, 2005                                            */
/* Description: HTML Widget driver for the menu, popup, dropdown,	*/
/*		vertical, or horizontal.				*/
/************************************************************************/

/** globals **/
static struct 
    {
    int     idcnt;
    }
    HTMN;


int
htmenu_internal_AddDot(pHtSession s, int mcnt, char* nptr, int is_horizontal, int row_height)
    {
    htrAddBodyItem_va(s,"<td valign=\"%s\"><img align=\"%s\" name=\"xy_%s%d\" width=\"1\" height=\"%d\" src=\"/sys/images/trans_1.gif\"></td>", ((mcnt&1) || !is_horizontal)?"top":"bottom", ((mcnt&1) || !is_horizontal)?"top":"bottom", nptr, mcnt, ((mcnt&1) || !is_horizontal)?(row_height?row_height:1):1);
    return 0;
    }

int
htmenu_internal_AddItem(pHtSession s, pWgtrNode menu_item, int is_horizontal, int is_popup, int is_submenu, int is_onright, int row_h, int mcnt, char* nptr, pXString xs)
    {
    char* ptr;
    int rval, n;
    pExpression code;
    char name[64];

	xsPrintf(xs, "enabled:1, onright:%d", is_onright);

	if (!is_horizontal)
	    htrAddBodyItem_va(s, "<tr>");

	/** image used to track position **/
	htmenu_internal_AddDot(s, mcnt, nptr, is_horizontal, row_h);

	wgtrGetPropertyValue(menu_item,"name", DATA_T_STRING, POD(&ptr));
	strtcpy(name, ptr, sizeof(name));

	/** icon **/
	if (wgtrGetPropertyValue(menu_item,"icon",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    htrAddBodyItem_va(s, "<td valign=\"middle\"><img src=\"%s\"></td>", ptr);
	    xsConcatPrintf(xs, ", icon:'%s'", ptr);
	    }
	else
	    htrAddBodyItem_va(s, "<td>&nbsp;</td>");

	/** checkbox **/
	if ( (rval=htrGetBoolean(menu_item, "checked", -1)) >= 0)
	    {
	    htrAddBodyItem_va(s, "<td valign=\"middle\"><img name=\"cb_%d\" src=\"/sys/images/checkbox_%s.gif\"></td>", mcnt, rval?"checked":"unchecked");
	    xsConcatPrintf(xs, ", check:%s", rval?"true":"false");

	    /** User requesting expression for value? **/
	    if (wgtrGetPropertyType(menu_item,"value") == DATA_T_CODE)
		{
		wgtrGetPropertyValue(menu_item,"value",DATA_T_CODE,POD(&code));
		htrAddExpression(s, name, "value", code);
		}
	    }
	else
	    htrAddBodyItem_va(s, "<td></td>");

	/** Text **/
	if (wgtrGetPropertyValue(menu_item,"label",DATA_T_STRING,POD(&ptr)) == 0)
	    {
	    htrAddBodyItem_va(s, "<td nowrap valign=\"middle\">%s</td>", ptr);
	    xsConcatPrintf(xs, ", label:'%s'", ptr);
	    }
	else
	    htrAddBodyItem_va(s, "<td></td>");
	if (wgtrGetPropertyValue(menu_item, "value", DATA_T_STRING, POD(&ptr)) == 0)
	    xsConcatPrintf(xs, ", value:'%s'", ptr);
	else if (wgtrGetPropertyValue(menu_item, "value", DATA_T_INTEGER, POD(&n)) == 0)
	    xsConcatPrintf(xs, ", value:%d", n);

	/** Submenu arrow **/
	if (is_submenu && !is_horizontal)
	    {
	    htrAddBodyItem_va(s, "<td valign=\"middle\"><img src=\"/sys/images/menu_arrow.gif\"></td>");
	    }
	else
	    htrAddBodyItem_va(s, "<td>&nbsp;</td>");

	if (!is_horizontal)
	    htrAddBodyItem_va(s, "</tr>");

	if (is_submenu)
	    {
	    xsConcatPrintf(xs, ", submenu:'%s'", name);
	    }

	if (is_submenu) 
	    htrAddScriptInit_va(s, "    nodes[\"%s\"].AddItem({%s});\n", nptr, xs->String);
	else
	    htrAddScriptInit_va(s, "    wgtrReplaceNode(nodes[\"%s\"], nodes[\"%s\"].AddItem({%s}));\n", name, nptr, xs->String);

    return 0;
    }


int
htmenu_internal_AddSep(pHtSession s, int is_horizontal, int row_h, int mcnt, char* nptr)
    {

	if (!is_horizontal)
	    htrAddBodyItem_va(s, "<tr>");

	htmenu_internal_AddDot(s, mcnt, nptr, is_horizontal, row_h);

	/** If vertical, add a separating line.  If horiz, just add some space **/
	if (is_horizontal)  
	    {
	    htrAddBodyItem_va(s, "<td>&nbsp;&nbsp;&nbsp;</td>");
	    }
	else
	    {
	    htrAddBodyItem_va(s, "<td colspan=\"4\" height=\"4\" background=\"/sys/images/menu_sep.gif\"><img src=\"/sys/images/trans_1.gif\" height=\"4\" width=\"1\"></td></tr>");
	    }
	htrAddScriptInit_va(s, "    nodes[\"%s\"].AddItem({sep:true});\n", nptr);

    return 0;
    }


/*** htmenuRender - generate the HTML code for the menu widget.
 ***/
int 
htmenuRender(pHtSession s, pWgtrNode menu, int z) 
    {
    char bgstr[80];	    /* N.B. all these colors must be the same size */
    char highlight[80];
    char active[80];
    char textcolor[80];
    char name[64];
    char *ptr;
    int x,y,w,h;
    int col_w, row_h;
    int id, i, cnt, mcnt;
    int is_horizontal;
    int is_popup;
    int is_submenu;
    pWgtrNode sub_tree;
    pXString xs;
    int bx = 0;

	if(!s->Capabilities.Dom0NS && !s->Capabilities.CSS2)
	    {
	    mssError(1,"HTMENU","Netscape 4 DOM or W3C CSS2 support required");
	    return -1;
	    }
	if (s->Capabilities.CSS2) bx = 1;

	/** Get an id for this. **/
	id = (HTMN.idcnt++);

	/** Get x,y,height,& width of this object **/
	if (wgtrGetPropertyValue(menu,"x",DATA_T_INTEGER,POD(&x)) != 0) x=0;
	if (wgtrGetPropertyValue(menu,"y",DATA_T_INTEGER,POD(&y)) != 0) y=0;
	if (wgtrGetPropertyValue(menu,"height",DATA_T_INTEGER,POD(&h)) != 0) h = -1;
	if (wgtrGetPropertyValue(menu,"width",DATA_T_INTEGER,POD(&w)) != 0) w = -1;
	if (wgtrGetPropertyValue(menu,"column_width",DATA_T_INTEGER,POD(&col_w)) != 0) col_w = 0;
	if (wgtrGetPropertyValue(menu,"row_height",DATA_T_INTEGER,POD(&row_h)) != 0) row_h = 0;

	/** Colors/Backgrounds **/
	if (htrGetBackground(menu, NULL, s->Capabilities.CSS2, bgstr, sizeof(bgstr)) < 0)
	    strcpy(bgstr, "");
	if (htrGetBackground(menu, "highlight", s->Capabilities.CSS2, highlight, sizeof(highlight)) < 0)
	    strcpy(highlight, bgstr);
	if (htrGetBackground(menu, "active", s->Capabilities.CSS2, active, sizeof(highlight)) < 0)
	    strcpy(active, highlight);
	if (wgtrGetPropertyValue(menu, "textcolor", DATA_T_STRING, POD(&ptr)) != 0)
	    strcpy(textcolor, "black");
	else
	    strtcpy(textcolor, ptr, sizeof(textcolor));

	/** Mode of operation (popup/bar, horiz/vert) **/
	if (wgtrGetPropertyValue(menu, "direction", DATA_T_STRING, POD(&ptr)) == 0 && !strcmp(ptr,"vertical"))
	    is_horizontal = 0;
	else
	    is_horizontal = 1;
	is_popup = htrGetBoolean(menu, "popup", 0);
	if (is_popup < 0) is_popup = 0;

	/** Write the main style header item. **/
	if (h != -1 && w == -1)
	    {
	    htrAddStylesheetItem_va(s,"\t#mn%dmain { POSITION:absolute; VISIBILITY:%s; LEFT:%dpx; TOP:%dpx; HEIGHT:%dpx; Z-INDEX:%d; }\n", id,is_popup?"hidden":"inherit", x,y,h-2*bx,z);
	    htrAddStylesheetItem_va(s,"\t#mn%dcontent { POSITION:absolute; VISIBILITY: inherit; LEFT:0px; TOP:0px; HEIGHT:%dpx; Z-INDEX:%d; }\n", id, h-2*bx, z+1);
	    }
	else if (h == -1 && w != -1)
	    {
	    htrAddStylesheetItem_va(s,"\t#mn%dmain { POSITION:absolute; VISIBILITY:%s; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; Z-INDEX:%d; }\n", id,is_popup?"hidden":"inherit",x,y,w-2*bx,z);
	    htrAddStylesheetItem_va(s,"\t#mn%dcontent { POSITION:absolute; VISIBILITY: inherit; LEFT:0px; TOP:0px; WIDTH:%dpx; Z-INDEX:%d; }\n", id, w-2*bx, z+1);
	    }
	else if (h != -1 && w != -1)
	    {
	    htrAddStylesheetItem_va(s,"\t#mn%dmain { POSITION:absolute; VISIBILITY:%s; LEFT:%dpx; TOP:%dpx; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; CLIP:rect(0px,%dpx,%dpx,0px); }\n",
		    id,is_popup?"hidden":"inherit",x,y,w-2*bx,h-2*bx,z,w,h);
	    htrAddStylesheetItem_va(s,"\t#mn%dcontent { POSITION:absolute; VISIBILITY: inherit; LEFT:0px; TOP:0px; WIDTH:%dpx; HEIGHT:%dpx; Z-INDEX:%d; }\n", 
		    id, w-2*bx,h-2*bx, z+1);
	    }
	else
	    {
	    htrAddStylesheetItem_va(s,"\t#mn%dmain { POSITION:absolute; VISIBILITY:%s; LEFT:%dpx; TOP:%dpx; Z-INDEX:%d; }\n", id,is_popup?"hidden":"inherit",x,y,z);
	    htrAddStylesheetItem_va(s,"\t#mn%dcontent { POSITION:absolute; VISIBILITY: inherit; LEFT:0px; TOP:0px; Z-INDEX:%d; }\n", id, z+1);
	    }
	if (s->Capabilities.CSS2)
	    htrAddStylesheetItem_va(s,"\t#mn%dmain { overflow:hidden; border-style: solid; border-width: 1px; border-color: white gray gray white; %s }\n", id, bgstr);

	/** content layer **/
	if (s->Capabilities.CSS2)
	    htrAddStylesheetItem_va(s,"\t#mn%dcontent { overflow:hidden; cursor:default; }\n", id );

	/** highlight bar **/
	htrAddStylesheetItem_va(s, "\t#mn%dhigh { POSITION:absolute; VISIBILITY: hidden; LEFT:0px; TOP:0px; Z-INDEX:%d; }\n", id, z);
	if (s->Capabilities.CSS2)
	    htrAddStylesheetItem_va(s,"\t#mn%dhigh { overflow:hidden; }\n", id );

	/** Get name **/
	if (wgtrGetPropertyValue(menu,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	strtcpy(name,ptr,sizeof(name));

	/** Globals **/
	htrAddScriptGlobal(s, "mn_active", "new Array()", 0);
	htrAddScriptGlobal(s, "mn_current", "null", 0);
	htrAddScriptGlobal(s, "mn_deactivate_tmout", "null", 0);
	htrAddScriptGlobal(s, "mn_submenu_tmout", "null", 0);
	htrAddScriptGlobal(s, "mn_pop_x", "0", 0);
	htrAddScriptGlobal(s, "mn_pop_y", "0", 0);
	htrAddScriptGlobal(s, "mn_mouseangle", "0", 0);
	htrAddWgtrObjLinkage_va(s, menu, "htr_subel(mn_parent(_parentobj), \"mn%dmain\")",id);
	htrAddWgtrCtrLinkage_va(s, menu, "htr_subel(_obj, \"mn%dcontent\")",id);

	/** Scripts **/
	htrAddScriptInclude(s, "/sys/js/ht_utils_layers.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);
	htrAddScriptInclude(s, "/sys/js/htdrv_menu.js", 0);

	/** Initialization **/
	htrAddScriptInit_va(s,"    mn_init({layer:nodes[\"%s\"], clayer:wgtrGetContainer(nodes[\"%s\"]), hlayer:htr_subel(nodes[\"%s\"], \"mn%dhigh\"), bgnd:\"%s\", high:\"%s\", actv:\"%s\", txt:\"%s\", w:%d, h:%d, horiz:%d, pop:%d, name:\"%s\"});\n", 
		name, name, name, id, 
		bgstr, highlight, active, textcolor, 
		w, h, is_horizontal, is_popup, name);

	/** Event handlers **/
	htrAddEventHandlerFunction(s, "document", "MOUSEMOVE", "mn", "mn_mousemove");
	htrAddEventHandlerFunction(s, "document", "MOUSEOUT", "mn", "mn_mouseout");
	htrAddEventHandlerFunction(s, "document", "MOUSEOVER", "mn", "mn_mouseover");
	htrAddEventHandlerFunction(s, "document", "MOUSEDOWN", "mn", "mn_mousedown");

	/** Beginning of code for menu **/
	htrAddBodyItem_va(s,"<div id=\"mn%dmain\">", id);
	if (s->Capabilities.Dom0NS)
	    htrAddBodyItem_va(s,"<body %s>",bgstr);
	htrAddBodyItem_va(s,"<div id=\"mn%dcontent\"><table cellspacing=\"0\" cellpadding=\"0\" border=\"0\" %s>\n", id, s->Capabilities.Dom0NS?"":"width=\"100%\" height=\"100%\"");

	/** Only draw border if it is NS4 **/
	if (s->Capabilities.Dom0NS)
	    {
	    htrAddBodyItem_va(s,"<tr><td background=\"/sys/images/white_1x1.png\"><img src=\"/sys/images/trans_1.gif\" height=\"1\" width=\"1\"></td>");
	    if (w != -1)
		htrAddBodyItem_va(s,"<td background=\"/sys/images/white_1x1.png\"><img src=\"/sys/images/trans_1.gif\" height=\"1\" width=\"%d\"></td>", w-2);
	    else
		htrAddBodyItem_va(s,"<td background=\"/sys/images/white_1x1.png\"><img src=\"/sys/images/trans_1.gif\" height=\"1\" width=\"1\"></td>");
	    htrAddBodyItem_va(s,"<td background=\"/sys/images/white_1x1.png\"><img src=\"/sys/images/trans_1.gif\" height=\"1\" width=\"1\"></td></tr>\n");
	    if (h != -1)
		htrAddBodyItem_va(s,"<tr><td background=\"/sys/images/white_1x1.png\"><img src=\"/sys/images/trans_1.gif\" height=\"%d\" width=\"1\"></td><td>", h-2);
	    else
		htrAddBodyItem_va(s,"<tr><td background=\"/sys/images/white_1x1.png\"><img src=\"/sys/images/trans_1.gif\" height=\"1\" width=\"1\"></td><td>");
	    }
	else
	    htrAddBodyItem_va(s,"<tr><td valign=\"middle\">");

	/** Add 'meat' of menu... **/
	xs = (pXString)nmMalloc(sizeof(XString));
	xsInit(xs);
	mcnt=0;
	htrAddBodyItem_va(s,"<table cellspacing=\"1\" cellpadding=\"0\" border=\"0\" width=\"100%%\"><tr><td align=\"left\" valign=\"middle\">\n");
	htrAddBodyItem_va(s,"<table cellspacing=\"2\" cellpadding=\"0\" border=\"0\">%s\n", is_horizontal?"<tr>":"");
	cnt = xaCount(&(menu->Children));
	for (i=0;i<cnt;i++)
	    {
	    sub_tree = xaGetItem(&(menu->Children), i);
	    wgtrGetPropertyValue(sub_tree,"outer_type",DATA_T_STRING,POD(&ptr));
	    if (!strcmp(ptr,"widget/menuitem") || !strcmp(ptr,"widget/menu")) 
		{
		is_submenu = !strcmp(ptr,"widget/menu");
		if (!(is_horizontal && htrGetBoolean(sub_tree, "onright", 0) == 1))
		    {
		    htmenu_internal_AddItem(s, sub_tree, is_horizontal, is_popup, is_submenu, 0, row_h, mcnt, name, xs);
		    mcnt++;
		    }
		}
	    else if (!strcmp(ptr,"widget/menusep"))
		{
		htmenu_internal_AddSep(s, is_horizontal, row_h, mcnt, name);
		mcnt++;
		}
	    }
	if (is_horizontal)
	    htmenu_internal_AddDot(s, mcnt, name, is_horizontal, 1);
	else
	    {
	    htrAddBodyItem_va(s, "<tr>");
	    htmenu_internal_AddDot(s, mcnt, name, is_horizontal, 1);
	    htrAddBodyItem_va(s, "<td colspan=\"4\"></td></tr>");
	    }
	mcnt++;
	htrAddBodyItem_va(s,"%s</table></td>", is_horizontal?"</tr>":"");
	if (is_horizontal)
	    {
	    htrAddBodyItem_va(s,"<td align=\"right\" valign=\"middle\">\n");
	    htrAddBodyItem_va(s,"<table cellspacing=\"2\" cellpadding=\"0\" border=\"0\"><tr>\n");
	    for (i=0;i<cnt;i++)
		{
		sub_tree = xaGetItem(&(menu->Children), i);
		wgtrGetPropertyValue(sub_tree,"outer_type",DATA_T_STRING,POD(&ptr));
		if (!strcmp(ptr,"widget/menuitem") || !strcmp(ptr,"widget/menu")) 
		    {
		    is_submenu = !strcmp(ptr,"widget/menu");
		    if ((is_horizontal && htrGetBoolean(sub_tree, "onright", 0) == 1))
			{
			htmenu_internal_AddItem(s, sub_tree, is_horizontal, is_popup, is_submenu, 1, row_h, mcnt, name, xs);
			mcnt++;
			}
		    }
		else if (!strcmp(ptr,"widget/menusep"))
		    {
		    htmenu_internal_AddSep(s, is_horizontal, row_h, mcnt, name);
		    mcnt++;
		    }
		}
	    htmenu_internal_AddDot(s, mcnt, name, is_horizontal, 1);
	    mcnt++;
	    htrAddBodyItem_va(s,"</tr></table></td>\n");
	    }
	htrAddBodyItem_va(s,"</tr></table>\n");

	/** closing border for NS4 **/
	if (s->Capabilities.Dom0NS)
	    {
	    if (h != -1)
		htrAddBodyItem_va(s,"</td><td background=\"/sys/images/dkgrey_1x1.png\"><img src=\"/sys/images/trans_1.gif\" height=\"%d\" width=\"1\"></td></tr>\n", h-2);
	    else
		htrAddBodyItem_va(s,"</td><td background=\"/sys/images/dkgrey_1x1.png\"><img src=\"/sys/images/trans_1.gif\" height=\"1\" width=\"1\"></td></tr>\n");
	    htrAddBodyItem_va(s,"<tr><td background=\"/sys/images/dkgrey_1x1.png\"><img src=\"/sys/images/trans_1.gif\" height=\"1\" width=\"1\"></td>");
	    if (w != -1)
		htrAddBodyItem_va(s,"<td background=\"/sys/images/dkgrey_1x1.png\"><img src=\"/sys/images/trans_1.gif\" height=\"1\" width=\"%d\"></td>", w-2);
	    else
		htrAddBodyItem_va(s,"<td background=\"/sys/images/dkgrey_1x1.png\"><img src=\"/sys/images/trans_1.gif\" height=\"1\" width=\"1\"></td>");
	    htrAddBodyItem_va(s,"<td background=\"/sys/images/dkgrey_1x1.png\"><img src=\"/sys/images/trans_1.gif\" height=\"1\" width=\"1\"></td></tr>\n");
	    }
	else
	    htrAddBodyItem_va(s,"</td></tr>");

	/** Ending of layer **/
	if (s->Capabilities.Dom0NS)
	    htrAddBodyItem_va(s,"</table></div><div id=\"mn%dhigh\"></div></body></div>", id);
	else
	    htrAddBodyItem_va(s,"</table></div><div id=\"mn%dhigh\"></div></div>\n", id);

	xsDeInit(xs);
	nmFree(xs, sizeof(XString));

	/* Read and initialize the menu items */
	cnt = xaCount(&(menu->Children));
	for (i=0;i<cnt;i++)
	    {
	    sub_tree = xaGetItem(&(menu->Children), i);
	    wgtrGetPropertyValue(sub_tree,"outer_type",DATA_T_STRING,POD(&ptr));
	    if (!strcmp(ptr,"widget/menuitem")) 
		{
		htrRenderSubwidgets(s, sub_tree, z+1);
		/*sub_tree->RenderFlags |= HT_WGTF_NOOBJECT;*/
		} 
	    else if (!strcmp(ptr,"widget/menusep"))
		{
		sub_tree->RenderFlags |= HT_WGTF_NOOBJECT;
		}
	    else 
		{
		htrRenderWidget(s, sub_tree, z+1);
		}
	    }

    return 0;
    }


/* 
   htmenuInitialize - register with the ht_render module.
*/
int 
htmenuInitialize() 
    {
    pHtDriver drv;

	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Menu Widget Driver");
	strcpy(drv->WidgetName,"menu");
	drv->Render = htmenuRender;

	/** Register events **/
	htrAddEvent(drv,"MouseUp");
	htrAddEvent(drv,"MouseDown");
	htrAddEvent(drv,"MouseOver");
	htrAddEvent(drv,"MouseOut");
	htrAddEvent(drv,"MouseMove");
	htrAddEvent(drv,"DataChange");
	htrAddEvent(drv,"GetFocus");
	htrAddEvent(drv,"LoseFocus");

	/** Register. **/
	htrRegisterDriver(drv);

	htrAddSupport(drv, "dhtml");

	HTMN.idcnt = 0;

    return 0;
    }


