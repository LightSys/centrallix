#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "ht_render.h"
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtsession.h"

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
/* Module: 	htdrv_editbox.c         				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 22, 2001 					*/
/* Description:	HTML Widget driver for a single-line editbox.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_editbox.c,v 1.2 2001/11/03 02:09:54 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_editbox.c,v $

    $Log: htdrv_editbox.c,v $
    Revision 1.2  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.1  2001/10/23 00:25:09  gbeeley
    Added rudimentary single-line editbox widget.  No data source linking
    or anything like that yet.  Fixed a few bugs and made a few changes to
    other controls to make this work more smoothly.  Page widget still needs
    some key de-bounce and key repeat overhaul.  Arrow keys don't work in
    Netscape 4.xx.

 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTEB;


/*** htebVerify - not written yet.
 ***/
int
htebVerify()
    {
    return 0;
    }


/*** htebRender - generate the HTML code for the editbox widget.
 ***/
int
htebRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[200];
    /*char sbuf2[160];*/
    char main_bg[128];
    int x=-1,y=-1,w,h;
    int id;
    int is_raised = 1;
    char* nptr;
    char* c1;
    char* c2;
    int maxchars;

    	/** Get an id for this. **/
	id = (HTEB.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) 
	    {
	    mssError(1,"HTEB","Editbox widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0)
	    {
	    mssError(1,"HTEB","Editbox widget must have a 'height' property");
	    return -1;
	    }
	
	/** Maximum characters to accept from the user **/
	if (objGetAttrValue(w_obj,"maxchars",POD(&maxchars)) != 0) maxchars=255;

	/** Background color/image? **/
	if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
	    sprintf(main_bg,"bgcolor='%.40s'",ptr);
	else if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
	    sprintf(main_bg,"background='%.110s'",ptr);
	else
	    strcpy(main_bg,"");

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	strcpy(name,ptr);

	/** Style of editbox - raised/lowered **/
	if (objGetAttrValue(w_obj,"style",POD(&ptr)) == 0 && !strcmp(ptr,"lowered")) is_raised = 0;
	if (is_raised)
	    {
	    c1 = "white_1x1.png";
	    c2 = "dkgrey_1x1.png";
	    }
	else
	    {
	    c1 = "dkgrey_1x1.png";
	    c2 = "white_1x1.png";
	    }

	/** Ok, write the style header items. **/
	sprintf(sbuf,"    <STYLE TYPE=\"text/css\">\n");
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"\t#eb%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"\t#eb%dcon1 { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,1,1,w-2,z+1);
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"\t#eb%dcon2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,1,1,w-2,z+1);
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"    </STYLE>\n");
	htrAddHeaderItem(s,sbuf);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Global for ibeam cursor layer **/
	htrAddScriptGlobal(s, "eb_ibeam", "null", 0);
	htrAddScriptGlobal(s, "eb_metric", "null", 0);
	htrAddScriptGlobal(s, "eb_current", "null", 0);

	/** Editbox text encoding function **/
	htrAddScriptFunction(s, "eb_encode", "\n"
		"function eb_encode(s)\n"
		"    {\n"
		"    rs = '';\n"
		"    for(i=0;i<s.length;i++)\n"
		"        {\n"
		"        if (s[i] == '<') rs += '&lt;';\n"
		"        else if (s[i] == '>') rs += '&gt;';\n"
		"        else if (s[i] == '&') rs += '&amp;';\n"
		"        else if (s[i] == ' ') rs += '&nbsp;';\n"
		"        else rs += s[i];\n"
		"        }\n"
		"    return rs;\n"
		"    }\n", 0);

	/** Get value function **/
	htrAddScriptFunction(s, "eb_getvalue", "\n"
		"function eb_getvalue()\n"
		"    {\n"
		"    return this.content;\n"
		"    }\n", 0);

	/** Set value function **/
	htrAddScriptFunction(s, "eb_setvalue", "\n"
		"function eb_setvalue(v,f)\n"
		"    {\n"
		"    eb_settext(this,v);\n"
		"    }\n", 0);

	/** Enable control function **/
	htrAddScriptFunction(s, "eb_enable", "\n"
		"function eb_enable()\n"
		"    {\n"
		"    }\n", 0);

	/** Disable control function **/
	htrAddScriptFunction(s, "eb_disable", "\n"
		"function eb_disable()\n"
		"    {\n"
		"    }\n", 0);

	/** Readonly-mode function **/
	htrAddScriptFunction(s, "eb_readonly", "\n"
		"function eb_readonly()\n"
		"    {\n"
		"    }\n", 0);

	/** Editbox set-text-value function **/
	htrAddScriptFunction(s, "eb_settext", "\n"
		"function eb_settext(l,txt)\n"
		"    {\n"
		"    l.HiddenLayer.document.write('<PRE>' + eb_encode(txt) + '</PRE> ');\n"
		"    l.HiddenLayer.document.close();\n"
		"    l.HiddenLayer.visibility = 'inherit';\n"
		"    l.ContentLayer.visibility = 'hidden';\n"
		"    tmp = l.ContentLayer;\n"
		"    l.ContentLayer = l.HiddenLayer;\n"
		"    l.HiddenLayer = tmp;\n"
		"    l.content=txt;\n"
		"    }\n", 0);

	/** Editbox keyboard handler **/
	htrAddScriptFunction(s, "eb_keyhandler", "\n"
		"function eb_keyhandler(l,e,k)\n"
		"    {\n"
		"    txt = l.content;\n"
		"    if (k >= 32 && k < 127)\n"
		"        {\n"
		"        newtxt = txt.substr(0,l.cursorCol) + String.fromCharCode(k) + txt.substr(l.cursorCol,txt.length);\n"
		"        l.cursorCol++;\n"
		"        }\n"
		"    else if (k == 8 && l.cursorCol > 0)\n"
		"        {\n"
		"        newtxt = txt.substr(0,l.cursorCol-1) + txt.substr(l.cursorCol,txt.length);\n"
		"        l.cursorCol--;\n"
		"        }\n"
		"    else\n"
		"        {\n"
		"        return true;\n"
		"        }\n"
		"    eb_ibeam.visibility = 'hidden';\n"
		"    eb_ibeam.moveToAbsolute(l.ContentLayer.pageX + l.cursorCol*eb_metric.charWidth, l.ContentLayer.pageY);\n"
		"    eb_settext(l,newtxt);\n"
		"    adj = 0;\n"
		"    if (eb_ibeam.pageX < l.pageX + 1)\n"
		"        adj = l.pageX + 1 - eb_ibeam.pageX;\n"
		"    else if (eb_ibeam.pageX > l.pageX + l.clip.width - 1)\n"
		"        adj = (l.pageX + l.clip.width - 1) - eb_ibeam.pageX;\n"
		"    if (adj != 0)\n"
		"        {\n"
		"        eb_ibeam.pageX += adj;\n"
		"        l.ContentLayer.pageX += adj;\n"
		"        l.HiddenLayer.pageX += adj;\n"
		"        }\n"
		"    eb_ibeam.visibility = 'inherit';\n"
		"    return false;\n"
		"    }\n", 0);

	/** Set focus to a new editbox **/
	htrAddScriptFunction(s, "eb_select", "\n"
		"function eb_select(x,y,l,c,n)\n"
		"    {\n"
		"    l.cursorCol = Math.round((x + l.pageX - l.ContentLayer.pageX)/eb_metric.charWidth);\n"
		"    if (l.cursorCol > l.content.length) l.cursorCol = l.content.length;\n"
		"    if (eb_current) eb_current.cursorlayer = null;\n"
		"    eb_current = l;\n"
		"    eb_current.cursorlayer = eb_ibeam;\n"
		"    eb_ibeam.visibility = 'hidden';\n"
		"    eb_ibeam.moveAbove(eb_current);\n"
		"    eb_ibeam.moveToAbsolute(eb_current.ContentLayer.pageX + eb_current.cursorCol*eb_metric.charWidth, eb_current.ContentLayer.pageY);\n"
		"    eb_ibeam.zIndex = eb_current.zIndex + 2;\n"
		"    eb_ibeam.visibility = 'inherit';\n"
		"    l.form.focusnotify(l);\n"
		"    return 1;\n"
		"    }\n", 0);

	/** Take focus away from editbox **/
	htrAddScriptFunction(s, "eb_deselect", "\n"
		"function eb_deselect()\n"
		"    {\n"
		"    eb_ibeam.visibility = 'hidden';\n"
		"    if (eb_current) eb_current.cursorlayer = null;\n"
		"    eb_current = null;\n"
		"    return true;\n"
		"    }\n", 0);

	/** Editbox initializer **/
	htrAddScriptFunction(s, "eb_init", "\n"
		"function eb_init(l,c1,c2)\n"
		"    {\n"
		"    l.document.Layer = l;\n"
		"    l.ContentLayer = c1;\n"
		"    l.HiddenLayer = c2;\n"
		"    if (!eb_ibeam || !eb_metric)\n"
		"        {\n"
		"        eb_metric = new Layer(24);\n"
		"        eb_metric.visibility = 'hidden';\n"
		"        eb_metric.document.write('<pre>xx</pre>');\n"
		"        eb_metric.document.close();\n"
		"        w2 = eb_metric.clip.width;\n"
		"        h1 = eb_metric.clip.height;\n"
		"        eb_metric.document.write('<pre>x\\nx</pre>');\n"
		"        eb_metric.document.close();\n"
		"        eb_metric.charHeight = eb_metric.clip.height - h1;\n"
		"        eb_metric.charWidth = w2 - eb_metric.clip.width;\n"
		"        eb_ibeam = new Layer(1);\n"
		"        eb_ibeam.visibility = 'hidden';\n"
		"        eb_ibeam.document.write('<body bgcolor=' + page.dtcolor1 + '></body>');\n"
		"        eb_ibeam.document.close();\n"
		"        eb_ibeam.resizeTo(1,eb_metric.charHeight);\n"
		"        }\n"
		"    c1.mainlayer = l;\n"
		"    c2.mainlayer = l;\n"
		"    c1.kind = 'eb';\n"
		"    c2.kind = 'eb';\n"
		"    l.content = '';\n"
		"    l.keyhandler = eb_keyhandler;\n"
		"    l.getfocushandler = eb_select;\n"
		"    l.losefocushandler = eb_deselect;\n"
		"    l.getvalue = eb_getvalue;\n"
		"    l.setvalue = eb_setvalue;\n"
		"    l.setoptions = null;\n"
		"    l.enable = eb_enable;\n"
		"    l.disable = eb_disable;\n"
		"    l.readonly = eb_readonly;\n"
		"    l.isFormStatusWidget = false;\n"
		"    pg_addarea(l, -1,-1,l.clip.width+1,l.clip.height+1, 'ebox', 'ebox', 1);\n"
		"    c1.y = ((l.clip.height - eb_metric.charHeight)/2);\n"
		"    c2.y = ((l.clip.height - eb_metric.charHeight)/2);\n"
		"    if (fm_current) fm_current.register(l);\n"
		"    l.form = fm_current;\n"
		"    return l;\n"
		"    }\n", 0);

	/** Script initialization call. **/
	sprintf(sbuf,"    %s = eb_init(%s.layers.eb%dbase, %s.layers.eb%dbase.document.layers.eb%dcon1,%s.layers.eb%dbase.document.layers.eb%dcon2);\n",
		nptr, parentname, id, 
		parentname, id, id, 
		parentname, id, id);
	htrAddScriptInit(s, sbuf);

	/** HTML body <DIV> element for the base layer. **/
	sprintf(sbuf,"<DIV ID=\"eb%dbase\">\n",id);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf,"    <TABLE width=%d cellspacing=0 cellpadding=0 border=0 %s>\n",w,main_bg);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf,"        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c1);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c1,w-2);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n",c1);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "        <TR><TD><IMG SRC=/sys/images/%s height=%d width=1></TD>\n",c1,h-2);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "            <TD>&nbsp;</TD>\n");
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "            <TD><IMG SRC=/sys/images/%s height=%d width=1></TD></TR>\n",c2,h-2);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c2);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c2,w-2);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n    </TABLE>\n\n",c2);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "<DIV ID=\"eb%dcon1\"></DIV>\n",id);
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "<DIV ID=\"eb%dcon2\"></DIV>\n",id);
	htrAddBodyItem(s, sbuf);

	/** Check for objects within the editbox. **/
	/** The editbox can have no subwidgets **/
	/*sprintf(sbuf,"%s.mainlayer.document",nptr);
	sprintf(sbuf2,"%s.mainlayer",nptr);
	htrRenderSubwidgets(s, w_obj, sbuf, sbuf2, z+2);*/

	/** End the containing layer. **/
	htrAddBodyItem(s, "</DIV>\n");

    return 0;
    }


/*** htebInitialize - register with the ht_render module.
 ***/
int
htebInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Single-line Editbox Driver");
	strcpy(drv->WidgetName,"editbox");
	drv->Render = htebRender;
	drv->Verify = htebVerify;

	/** Add a 'set value' action **/
	htrAddAction(drv,"SetValue");
	htrAddParam(drv,"SetValue","Value",DATA_T_STRING);	/* value to set it to */
	htrAddParam(drv,"SetValue","Trigger",DATA_T_INTEGER);	/* whether to trigger the Modified event */

	/** Value-modified event **/
	htrAddEvent(drv,"Modified");
	htrAddParam(drv,"Modified","NewValue",DATA_T_STRING);
	htrAddParam(drv,"Modified","OldValue",DATA_T_STRING);
	
	/** Register. **/
	htrRegisterDriver(drv);

	HTEB.idcnt = 0;

    return 0;
    }
