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
/* Module: 	htdrv_spinner.c         				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 22, 2001 					*/
/* Description:	HTML Widget driver for a single-line editbox.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_spinner.c,v 1.1 2002/03/09 02:42:01 bones120 Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_spinner.c,v $

    $Log: htdrv_spinner.c,v $
    Revision 1.1  2002/03/09 02:42:01  bones120
    Initial commit of the spinner box.

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
    HTSP;


/*** htspVerify - not written yet.
 ***/
int
htspVerify()
    {
    return 0;
    }


/*** htspRender - generate the HTML code for the spinner widget.
 ***/
int
htspRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[512];
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
	id = (HTSP.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) 
	    {
	    mssError(1,"HTSP","Spinner widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0)
	    {
	    mssError(1,"HTSP","Spinner widget must have a 'height' property");
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
	sprintf(sbuf,"\t#sp%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"\t#sp%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w-12,z);
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"\t#sp%dcon1 { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,1,1,w-2-12,z+1);
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"\t#sp%dcon2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,1,1,w-2-12,z+1);
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"\t#sp_button_up { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",x+w-12,y,w,z);
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"\t#sp_button_down { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",x+w-12,y+9,w,z);
	htrAddHeaderItem(s,sbuf);
	sprintf(sbuf,"    </STYLE>\n");
	htrAddHeaderItem(s,sbuf);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Global for ibeam cursor layer **/
	htrAddScriptGlobal(s, "sp_ibeam", "null", 0);
	htrAddScriptGlobal(s, "sp_metric", "null", 0);
	htrAddScriptGlobal(s, "sp_current", "null", 0);

	/** Get value function **/
	htrAddScriptFunction(s, "sp_getvalue", "\n"
		"function sp_getvalue()\n"
		"    {\n"
		"    return parseFloat(this.content);\n"
		"    }\n", 0);

	/** Set value function **/
	htrAddScriptFunction(s, "sp_setvalue", "\n"
		"function sp_setvalue(v,f)\n"
		"    {\n"
		"    sp_settext(this,v);\n"
		"    }\n", 0);

	/** Enable control function **/
	htrAddScriptFunction(s, "sp_enable", "\n"
		"function sp_enable()\n"
		"    {\n"
		"    }\n", 0);

	/** Disable control function **/
	htrAddScriptFunction(s, "sp_disable", "\n"
		"function sp_disable()\n"
		"    {\n"
		"    }\n", 0);

	/** Readonly-mode function **/
	htrAddScriptFunction(s, "sp_readonly", "\n"
		"function sp_readonly()\n"
		"    {\n"
		"    }\n", 0);

	/** Editbox set-text-value function **/
	htrAddScriptFunction(s, "sp_settext", "\n"
		"function sp_settext(l,txt)\n"
		"    {\n"
		"    l.HiddenLayer.document.write('<PRE>' + txt + '</PRE> ');\n"
		"    l.HiddenLayer.document.close();\n"
		"    l.HiddenLayer.visibility = 'inherit';\n"
		"    l.ContentLayer.visibility = 'hidden';\n"
		"    tmp = l.ContentLayer;\n"
		"    l.ContentLayer = l.HiddenLayer;\n"
		"    l.HiddenLayer = tmp;\n"
		"    if(!txt)\n"
		"        txt=0;\n"
		"    l.content= new String(txt);\n"
		"    }\n", 0);

	/** Editbox keyboard handler **/
	htrAddScriptFunction(s, "sp_keyhandler", "\n"
		"function sp_keyhandler(l,e,k)\n"
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
		"    sp_ibeam.visibility = 'hidden';\n"
		"    sp_ibeam.moveToAbsolute(l.ContentLayer.pageX + l.cursorCol*sp_metric.charWidth, l.ContentLayer.pageY);\n"
		"    sp_settext(l,newtxt);\n"
		"    adj = 0;\n"
		"    if (sp_ibeam.pageX < l.pageX + 1)\n"
		"        adj = l.pageX + 1 - sp_ibeam.pageX;\n"
		"    else if (sp_ibeam.pageX > l.pageX + l.clip.width - 1)\n"
		"        adj = (l.pageX + l.clip.width - 1) - sp_ibeam.pageX;\n"
		"    if (adj != 0)\n"
		"        {\n"
		"        sp_ibeam.pageX += adj;\n"
		"        l.ContentLayer.pageX += adj;\n"
		"        l.HiddenLayer.pageX += adj;\n"
		"        }\n"
		"    sp_ibeam.visibility = 'inherit';\n"
		"    return false;\n"
		"    }\n", 0);

	/** Set focus to a new editbox **/
	htrAddScriptFunction(s, "sp_select", "\n"
		"function sp_select(x,y,l,c,n)\n"
		"    {\n"
		"    l.cursorCol = Math.round((x + l.pageX - l.ContentLayer.pageX)/sp_metric.charWidth);\n"
		"    if (l.cursorCol > l.content.length) l.cursorCol = l.content.length;\n"
		"    if (sp_current) sp_current.cursorlayer = null;\n"
		"    sp_current = l;\n"
		"    sp_current.cursorlayer = sp_ibeam;\n"
		"    sp_ibeam.visibility = 'hidden';\n"
		"    sp_ibeam.moveAbove(sp_current);\n"
		"    sp_ibeam.moveToAbsolute(sp_current.ContentLayer.pageX + sp_current.cursorCol*sp_metric.charWidth, sp_current.ContentLayer.pageY);\n"
		"    sp_ibeam.zIndex = sp_current.zIndex + 2;\n"
		"    sp_ibeam.visibility = 'inherit';\n"
		"    l.form.focusnotify(l);\n"
		"    return 1;\n"
		"    }\n", 0);

	/** Take focus away from editbox **/
	htrAddScriptFunction(s, "sp_deselect", "\n"
		"function sp_deselect()\n"
		"    {\n"
		"    sp_ibeam.visibility = 'hidden';\n"
		"    if (sp_current) sp_current.cursorlayer = null;\n"
		"    sp_current = null;\n"
		"    return true;\n"
		"    }\n", 0);

	htrAddEventHandler(s, "document","MOUSEDOWN", "sp", 
		"\n"
		"   if (e.target != null && e.target.kind == 'spinner') {\n"
		"      if(e.target.subkind=='up')\n"
		"      {\n"
		"   	   e.target.eb_layers.setvalue(e.target.eb_layers.getvalue()+1);\n"
		"      }\n"
		"      if(e.target.subkind=='down')\n"
		"      {\n"
		"          e.target.eb_layers.setvalue(e.target.eb_layers.getvalue()-1);\n"
		"      }\n"
		"   }\n"
		"\n");

	/** Spinner box initializer **/
	htrAddScriptFunction(s, "sp_init", "\n"
		"function sp_init(main,l,c1,c2)\n"
		"    {\n"
		"    l.content = 0;\n"
		"    l.mainlayer=main;\n"
		"    l.document.Layer = l;\n"
		"    l.ContentLayer = c1;\n"
		"    l.ContentLayer.document.write('0');\n"
		"    l.ContentLayer.document.close();\n"
		"    l.HiddenLayer = c2;\n"
		"    l.HiddenLayer.document.write('0');\n"
		"    l.HiddenLayer.document.close();\n"
		"    if (!sp_ibeam || !sp_metric)\n"
		"        {\n"
		"        sp_metric = new Layer(24);\n"
		"        sp_metric.visibility = 'hidden';\n"
		"        sp_metric.document.write('<pre>xx</pre>');\n"
		"        sp_metric.document.close();\n"
		"        w2 = sp_metric.clip.width;\n"
		"        h1 = sp_metric.clip.height;\n"
		"        sp_metric.document.write('<pre>x\\nx</pre>');\n"
		"        sp_metric.document.close();\n"
		"        sp_metric.charHeight = sp_metric.clip.height - h1;\n"
		"        sp_metric.charWidth = w2 - sp_metric.clip.width;\n"
		"        sp_ibeam = new Layer(1);\n"
		"        sp_ibeam.visibility = 'hidden';\n"
		"        sp_ibeam.document.write('<body bgcolor=' + page.dtcolor1 + '></body>');\n"
		"        sp_ibeam.document.close();\n"
		"        sp_ibeam.resizeTo(1,sp_metric.charHeight);\n"
		"        }\n"
		"    c1.mainlayer = main;\n"
		"    c2.mainlayer = main;\n"
		"    c1.kind = 'spinner';\n"
		"    c2.kind = 'spinner';\n"
		"    main.kind = 'spinner';\n"
		"    main.layers.sp_button_up.document.images[0].kind='spinner';\n"
		"    main.layers.sp_button_down.document.images[0].kind='spinner';\n"
		"    main.layers.sp_button_up.document.images[0].subkind='up';\n"
		"    main.layers.sp_button_down.document.images[0].subkind='down';\n"
		"    main.layers.sp_button_up.document.images[0].eb_layers=l;\n"
		"    main.layers.sp_button_down.document.images[0].eb_layers=l;\n"
		"    l.keyhandler = sp_keyhandler;\n"
		"    l.getfocushandler = sp_select;\n"
		"    l.losefocushandler = sp_deselect;\n"
		"    l.getvalue = sp_getvalue;\n"
		"    l.setvalue = sp_setvalue;\n"
		"    l.setoptions = null;\n"
		"    l.enable = sp_enable;\n"
		"    l.disable = sp_disable;\n"
		"    l.readonly = sp_readonly;\n"
		"    l.isFormStatusWidget = false;\n"
		"    pg_addarea(l, -1,-1,l.clip.width+1,l.clip.height+1, 'ebox', 'ebox', 1);\n"
		"    c1.y = ((l.clip.height - sp_metric.charHeight)/2);\n"
		"    c2.y = ((l.clip.height - sp_metric.charHeight)/2);\n"
		"    if (fm_current) fm_current.Register(l);\n"
		"    if (fm_current) l.form = fm_current;\n"
		"    return l;\n"
		"    }\n", 0);

	/** Script initialization call. **/
	snprintf(sbuf,512,"    %s = sp_init(%s.layers.sp%dmain, %s.layers.sp%dmain.layers.sp%dbase, %s.layers.sp%dmain.layers.sp%dbase.document.layers.sp%dcon1, %s.layers.sp%dmain.layers.sp%dbase.document.layers.sp%dcon2);\n",
		nptr, parentname, id, 
                parentname, id, id,
		parentname, id, id, id, 
		parentname, id, id, id);
	htrAddScriptInit(s, sbuf);

	/** HTML body <DIV> element for the base layer. **/

	snprintf(sbuf,512,"<DIV ID=\"sp%dmain\">\n",id);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,512,"<DIV ID=\"sp%dbase\">\n",id);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,512,"    <TABLE width=%d cellspacing=0 cellpadding=0 border=0 %s>\n",w-12,main_bg);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,512,"        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c1);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,512, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c1,w-2-12);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,512, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n",c1);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,512, "        <TR><TD><IMG SRC=/sys/images/%s height=%d width=1></TD>\n",c1,h-2);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,512, "            <TD>&nbsp;</TD>\n");
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,512, "            <TD><IMG SRC=/sys/images/%s height=%d width=1></TD></TR>\n",c2,h-2);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,512, "        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c2);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,512, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c2,w-2-12);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,512, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n    </TABLE>\n\n",c2);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,512, "<DIV ID=\"sp%dcon1\"></DIV>\n",id);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,512, "<DIV ID=\"sp%dcon2\"></DIV>\n",id);
	htrAddBodyItem(s, sbuf);
	htrAddBodyItem(s, "</DIV>\n");
	/*Add the spinner buttons*/
	snprintf(sbuf,512, "<DIV ID=\"sp_button_up\"><IMG SRC=\"/sys/images/sp_up.gif\"></DIV>\n");
	htrAddBodyItem(s,sbuf);
	snprintf(sbuf,512, "<DIV ID=\"sp_button_down\"><IMG SRC=\"/sys/images/sp_down.gif\"></DIV>\n");
	htrAddBodyItem(s,sbuf);
  	htrAddBodyItem(s, "</DIV>\n"); 
    return 0;
    }


/*** htspInitialize - register with the ht_render module.
 ***/
int
htspInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Spinner Box Driver");
	strcpy(drv->WidgetName,"spinner");
	drv->Render = htspRender;
	drv->Verify = htspVerify;

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

	HTSP.idcnt = 0;

    return 0;
    }
