#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "stparse.h"
#include "mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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

/**CVSDATA***************************************************************

    $Id: htdrv_table.c,v 1.5 2002/03/09 19:21:20 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_table.c,v $

    $Log: htdrv_table.c,v $
    Revision 1.5  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.4  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.3  2001/10/23 00:25:09  gbeeley
    Added rudimentary single-line editbox widget.  No data source linking
    or anything like that yet.  Fixed a few bugs and made a few changes to
    other controls to make this work more smoothly.  Page widget still needs
    some key de-bounce and key repeat overhaul.  Arrow keys don't work in
    Netscape 4.xx.

    Revision 1.2  2001/10/16 23:53:01  gbeeley
    Added expressions-in-structure-files support, aka version 2 structure
    files.  Moved the stparse module into the core because it now depends
    on the expression subsystem.  Almost all osdrivers had to be modified
    because the structure file api changed a little bit.  Also fixed some
    bugs in the structure file generator when such an object is modified.
    The stparse module now includes two separate tree-structured data
    structures: StructInf and Struct.  The former is the new expression-
    enabled one, and the latter is a much simplified version.  The latter
    is used in the url_inf in net_http and in the OpenCtl for objects.
    The former is used for all structure files and attribute "override"
    entries.  The methods for the latter have an "_ne" addition on the
    function name.  See the stparse.h and stparse_ne.h files for more
    details.  ALMOST ALL MODULES THAT DIRECTLY ACCESSED THE STRUCTINF
    STRUCTURE WILL NEED TO BE MODIFIED.

    Revision 1.1.1.1  2001/08/13 18:00:51  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:55  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTBL;


/*** httblVerify - not written yet.
 ***/
int
httblVerify()
    {
    return 0;
    }


/*** httblRender - generate the HTML code for the page.
 ***/
int
httblRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[160];
    char tmpbuf[64];
    char tbl_bgnd[128] = "";
    char hdr_bgnd[128] = "";
    char row_bgnd1[128] = "";
    char row_bgnd2[128] = "";
    char textcolor[64] = "";
    char titlecolor[64] = "";
    pObject sub_w_obj, qy_obj;
    pObjQuery qy;
    int x=-1,y=-1,w,h;
    int id,n;
    int mode = 0;
    int outer_border = 0;
    int inner_border = 0;
    int inner_padding = 0;
    char* nptr;
    char* sql;
    int rowid,type,rval;
    char* attr;
    char* str;
    ObjData od;
    pStructInf col_infs[24];
    pStructInf attr_inf;
    int n_cols,colid;

    	/** Get an id for this. **/
	id = (HTTBL.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x = -1;
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y = -1;
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) w = -1;
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0) h = -1;

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Mode of table operation.  Defaults to 0 (static) **/
	if (objGetAttrValue(w_obj,"mode",POD(&ptr)) == 0)
	    {
	    if (!strcmp(ptr,"static")) mode = 0;
	    else if (!strcmp(ptr,"dynamicpage")) mode = 1;
	    else if (!strcmp(ptr,"dynamicrow")) mode = 2;
	    else
	        {
		mssError(1,"HTTBL","Widget '%s' mode '%s' is invalid.",name,ptr);
		return -1;
		}
	    }

	/** Get background color/image for table header **/
	if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
	    sprintf(tbl_bgnd,"background='%.110s'",ptr);
	else if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
	    sprintf(tbl_bgnd,"bgcolor='%.40s'",ptr);

	/** Get background color/image for header row **/
	if (objGetAttrValue(w_obj,"hdr_background",POD(&ptr)) == 0)
	    sprintf(hdr_bgnd,"background='%.110s'",ptr);
	else if (objGetAttrValue(w_obj,"hdr_bgcolor",POD(&ptr)) == 0)
	    sprintf(hdr_bgnd,"bgcolor='%.40s'",ptr);

	/** Get background color/image for rows **/
	if (objGetAttrValue(w_obj,"row_background1",POD(&ptr)) == 0)
	    sprintf(row_bgnd1,"background='%.110s'",ptr);
	else if (objGetAttrValue(w_obj,"row_bgcolor1",POD(&ptr)) == 0)
	    sprintf(row_bgnd1,"bgcolor='%.40s'",ptr);
	if (objGetAttrValue(w_obj,"row_background2",POD(&ptr)) == 0)
	    sprintf(row_bgnd2,"background='%.110s'",ptr);
	else if (objGetAttrValue(w_obj,"row_bgcolor2",POD(&ptr)) == 0)
	    sprintf(row_bgnd2,"bgcolor='%.40s'",ptr);

	/** Get borders and padding information **/
	objGetAttrValue(w_obj,"outer_border",POD(&outer_border));
	objGetAttrValue(w_obj,"inner_border",POD(&inner_border));
	objGetAttrValue(w_obj,"inner_padding",POD(&inner_padding));

	/** Text color information **/
	if (objGetAttrValue(w_obj,"textcolor",POD(&ptr)) == 0)
	    sprintf(textcolor,"%.63s",ptr);

	/** Title text color information **/
	if (objGetAttrValue(w_obj,"titlecolor",POD(&ptr)) == 0)
	    sprintf(titlecolor,"%.63s",ptr);
	if (!*titlecolor) strcpy(titlecolor,textcolor);

	/** Function to handle clicking of a table row **/
	htrAddScriptFunction(s, "tl_rowclick", "\n"
		"function tl_rowclick(x,y,l,cls,nm)\n"
		"    {\n"
		"    //alert(cls + ':' + nm);\n"
		"    return 3;\n"
		"    }\n", 0);

	/** Function to enable clickable table rows **/
	htrAddScriptFunction(s, "tl_init", "\n"
		"function tl_init(pl, nm, w, cp, cs)\n"
		"    {\n"
		"    if (w == -1) w = pl.clip.width;\n"
		"    ox = -1;\n"
		"    oy = -1;\n"
		"    nmstr = 'xy_' + nm;\n"
		"    for(i=0;i<pl.document.images.length;i++)\n"
		"        {\n"
		"        if (pl.document.images[i].name.substr(0,nmstr.length) == nmstr)\n"
		"            {\n"
		"            img = pl.document.images[i];\n"
		"            imgnm = pl.document.images[i].name.substr(nmstr.length+1,255);\n"
		"            if (ox != -1)\n"
		"                {\n"
		"                pl.getfocushandler = tl_rowclick;\n"
		"                pg_addarea(pl,img.x-cp-1,img.y-cp-1,w-(cs-1)*2,(img.y-oy)-(cs-1),nm,imgnm,3);\n"
		"                }\n"
		"            ox = img.x;\n"
		"            oy = img.y;\n"
		"            }\n"
		"        }\n"
		"    }\n", 0);

	/** Call init function **/
	snprintf(sbuf,160,"    tl_init(%s.Layer,'%s',%d,%d,%d);\n",parentname,nptr,w,inner_padding,inner_border);
	htrAddScriptInit(s,sbuf);

	/** Get column data **/
	n_cols = 0;
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		objGetAttrValue(sub_w_obj, "outer_type", POD(&ptr));
		if (!strcmp(ptr,"widget/table-column") != 0)
		    {
		    objGetAttrValue(sub_w_obj, "name", POD(&ptr));
		    col_infs[n_cols] = stCreateStruct(ptr, "widget/table-column");
		    attr_inf = stAddAttr(col_infs[n_cols], "width");
		    if (objGetAttrValue(sub_w_obj, "width", POD(&n)) == 0)
		        stAddValue(attr_inf, NULL, n);
		    else
		        stAddValue(attr_inf, NULL, -1);
		    attr_inf = stAddAttr(col_infs[n_cols], "title");
		    if (objGetAttrValue(sub_w_obj, "title", POD(&ptr)) == 0)
		        {
			str = nmSysStrdup(ptr);
		        stAddValue(attr_inf, str, 0);
			}
		    else
		        stAddValue(attr_inf, col_infs[n_cols]->Name, 0);
		    n_cols++;
		    }
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

	/** If mode 1 or 2 (dynamic), add a query-loader layer **/
	if (mode == 1 || mode == 2)
	    {
	    snprintf(sbuf,160,"    <STYLE TYPE=\"text/css\">\n");
	    htrAddHeaderItem(s,sbuf);
	    snprintf(sbuf,160,"\t#tl%dloader { POSITION:absolute; VISIBILITY:hidden; TOP:0; LEFT:0; }\n",id);
	    htrAddHeaderItem(s,sbuf);
	    snprintf(sbuf,160,"    </STYLE>\n");
	    htrAddHeaderItem(s,sbuf);
	    }

	/** HTML body <DIV> element for the layer. **/
	if (mode == 1 || mode == 2)
	    {
	    snprintf(sbuf,160,"<DIV ID=\"ht%dpane\">\n",id);
	    htrAddBodyItem(s, sbuf);
	    }

	/** Build the table from a query, if mode 0 **/
	if (mode == 0)
	    {
	    if (w != -1) snprintf(tmpbuf,64,"width=%d",w - (outer_border + (outer_border?1:0))*2); else tmpbuf[0] = 0;
	    snprintf(sbuf,160,"<TABLE %s border=%d cellspacing=0 cellpadding=0 %s><TR><TD>\n", tmpbuf, outer_border, tbl_bgnd);
	    htrAddBodyItem(s,sbuf);
	    if (w != -1) snprintf(tmpbuf,64,"width=%d",w - (outer_border + (outer_border?1:0))*2); else tmpbuf[0] = 0;
	    snprintf(sbuf,160,"<TABLE border=0 background=/sys/images/trans_1.gif cellspacing=%d cellpadding=%d %s>\n", inner_border, inner_padding, tmpbuf);
	    htrAddBodyItem(s,sbuf);
	    if (objGetAttrValue(w_obj,"sql",POD(&sql)) != 0)
	        {
		mssError(1,"HTTBL","Static datatable must have SQL property");
		return -1;
		}
	    qy = objMultiQuery(w_obj->Session, sql);
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
		    snprintf(sbuf,160,"    <TR %s>", hdr_bgnd);
		    htrAddBodyItem(s,sbuf);
		    if (n_cols == 0)
		        {
			for(colid=0,attr = objGetFirstAttr(qy_obj); attr; colid++,attr = objGetNextAttr(qy_obj))
			    {
			    if (colid==0)
			        {
				snprintf(sbuf,160,"<TH align=left><IMG name=\"xy_%s_%s\" src=/sys/images/trans_1.gif align=top>", nptr, "");
			        htrAddBodyItem(s,sbuf);
				}
			    else
			        htrAddBodyItem(s,"<TH align=left>");
			    if (*titlecolor)
			        {
				snprintf(sbuf,160,"<FONT color='%s'>",titlecolor);
				htrAddBodyItem(s,sbuf);
				}
			    htrAddBodyItem(s,attr);
			    if (*titlecolor) htrAddBodyItem(s,"</FONT>");
			    htrAddBodyItem(s,"</TH>");
			    }
			}
		    else
		        {
			for(colid = 0; colid < n_cols; colid++)
			    {
			    attr = col_infs[colid]->Name;
			    if (colid==0)
			        {
				snprintf(sbuf,160,"<TH align=left><IMG name=\"xy_%s_%s\" src=/sys/images/trans_1.gif align=top>", nptr, "");
			        htrAddBodyItem(s,sbuf);
				}
			    else
			        {
			        htrAddBodyItem(s,"<TH align=left>");
				}
			    if (*titlecolor)
			        {
				snprintf(sbuf,160,"<FONT color='%s'>",titlecolor);
				htrAddBodyItem(s,sbuf);
				}
			    if (stAttrValue(stLookup(col_infs[colid],"title"), NULL, &ptr, 0) == 0)
			        htrAddBodyItem(s,ptr);
			    else
			        htrAddBodyItem(s,attr);
			    if (*titlecolor) htrAddBodyItem(s,"</FONT>");
			    htrAddBodyItem(s,"</TH>");
			    }
			}
		    htrAddBodyItem(s,"</TR>\n");
		    }
		snprintf(sbuf,160,"    <TR %s>", (rowid&1)?((*row_bgnd2)?row_bgnd2:row_bgnd1):row_bgnd1);
	        htrAddBodyItem(s,sbuf);

		/** Build the row contents -- loop through attrs and convert to strings **/
		colid = 0;
		if (n_cols == 0)
		    attr = objGetFirstAttr(qy_obj);
		else
		    attr = col_infs[colid]->Name;
		while(attr)
		    {
		    if (n_cols && stAttrValue(stLookup(col_infs[colid],"width"),&n,NULL,0) == 0 && n >= 0)
		        {
			snprintf(sbuf,160,"<TD width=%d nowrap>",n*7);
			htrAddBodyItem(s,sbuf);
			}
		    else
		        {
		        htrAddBodyItem(s,"<TD nowrap>");
			}
		    type = objGetAttrType(qy_obj,attr);
		    rval = objGetAttrValue(qy_obj,attr,&od);
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
			snprintf(sbuf,160,"<IMG name=\"xy_%s_%s\" src=/sys/images/trans_1.gif align=top>", nptr, str?str:"");
		        htrAddBodyItem(s,sbuf);
			}
		    if (*textcolor)
		        {
			snprintf(sbuf,160,"<FONT COLOR=%s>",textcolor);
			htrAddBodyItem(s,sbuf);
			}
		    if (str) htrAddBodyItem(s,str);
		    if (*textcolor)
		        {
			htrAddBodyItem(s,"</FONT>");
			}
		    htrAddBodyItem(s,"</TD>");

		    /** Next attr **/
		    if (n_cols == 0)
		        attr = objGetNextAttr(qy_obj);
		    else
		        attr = (colid < n_cols-1)?(col_infs[++colid]->Name):NULL;
		    }
	        htrAddBodyItem(s,"</TR>\n");
		objClose(qy_obj);
		rowid++;
		}
	    objQueryClose(qy);
	    htrAddBodyItem(s,"</TABLE></TD></TR></TABLE>\n");
	    }

	/** Check for more sub-widgets within the table. **/
	snprintf(sbuf,160,"%s.document",nptr);
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		objGetAttrValue(sub_w_obj, "outer_type", POD(&ptr));
		if (strcmp(ptr,"widget/table-column") != 0)
		    htrRenderWidget(s, sub_w_obj, z+1, parentname, parentobj);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

	/** End the containing layer. **/
	if (mode == 1 || mode == 2) htrAddBodyItem(s, "</DIV>\n");

    return 0;
    }


/*** httblInitialize - register with the ht_render module.
 ***/
int
httblInitialize()
    {
    pHtDriver drv;
    /*pHtEventAction action;
    pHtParam param;*/

    	/** Allocate the driver **/
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML DataTable Driver");
	strcpy(drv->WidgetName,"table");
	drv->Render = httblRender;
	drv->Verify = httblVerify;
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);

#if 00
	/** Add the 'load page' action **/
	action = (pHtEventAction)nmSysMalloc(sizeof(HtEventAction));
	strcpy(action->Name,"LoadPage");
	xaInit(&action->Parameters,16);
	param = (pHtParam)nmSysMalloc(sizeof(HtParam));
	strcpy(param->ParamName,"Source");
	param->DataType = DATA_T_STRING;
	xaAddItem(&action->Parameters,(void*)param);
	xaAddItem(&drv->Actions,(void*)action);
#endif

	/** Register. **/
	htrRegisterDriver(drv);

	HTTBL.idcnt = 0;

    return 0;
    }
