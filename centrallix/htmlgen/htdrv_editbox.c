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

    $Id: htdrv_editbox.c,v 1.11 2002/03/23 01:18:09 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_editbox.c,v $

    $Log: htdrv_editbox.c,v $
    Revision 1.11  2002/03/23 01:18:09  lkehresman
    Fixed focus detection and form notification on editbox and anything that
    uses keyboard input.

    Revision 1.10  2002/03/20 21:13:12  jorupp
     * fixed problem in imagebutton point and click handlers
     * hard-coded some values to get a partially working osrc for the form
     * got basic readonly/disabled functionality into editbox (not really the right way, but it works)
     * made (some of) form work with discard/save/cancel window

    Revision 1.9  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.8  2002/03/05 01:55:09  lkehresman
    Added "clearvalue" method to form widgets

    Revision 1.7  2002/03/05 01:22:26  lkehresman
    Changed where the DataNotify form method is getting called.  Previously it
    would only get called when the edit box lost focus.  This was bad because
    clicking a button doesn't make the edit box lose focus.  Now DataNotify is
    getting called on key events.  Much better, and more of what you would expect.

    Revision 1.6  2002/03/02 03:06:50  jorupp
    * form now has basic QBF functionality
    * fixed function-building problem with radiobutton
    * updated checkbox, radiobutton, and editbox to work with QBF
    * osrc now claims it's global name

    Revision 1.5  2002/02/27 02:37:19  jorupp
    * moved editbox I-beam movement functionality to function
    * cleaned up form, added comments, etc.

    Revision 1.4  2002/02/23 04:28:29  jorupp
    bug fixes in form, I-bar in editbox is reset on a setvalue()

    Revision 1.3  2002/02/22 23:48:39  jorupp
    allow editbox to work without form, form compiles, doesn't do much

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
    char sbuf[HT_SBUF_SIZE];
    /*char sbuf2[160];*/
    char main_bg[128];
    int x=-1,y=-1,w,h;
    int id;
    int is_raised = 1;
    char* nptr;
    char* c1;
    char* c2;
    int maxchars;
    char fieldname[30];

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
	memccpy(name,ptr,0,63);
	name[63] = 0;

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

	if (objGetAttrValue(w_obj,"fieldname",POD(&ptr)) == 0) 
	    {
	    strncpy(fieldname,ptr,30);
	    }
	else 
	    { 
	    fieldname[0]='\0';
	    } 

	/** Ok, write the style header items. **/
	snprintf(sbuf,HT_SBUF_SIZE,"    <STYLE TYPE=\"text/css\">\n");
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,HT_SBUF_SIZE,"\t#eb%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,HT_SBUF_SIZE,"\t#eb%dcon1 { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,1,1,w-2,z+1);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,HT_SBUF_SIZE,"\t#eb%dcon2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,1,1,w-2,z+1);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,HT_SBUF_SIZE,"    </STYLE>\n");
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

	/** Helper function -- update cursor **/
	htrAddScriptFunction(s, "eb_update_cursor", "\n"
		"function eb_update_cursor(eb,val)\n"
		"    {\n"
		"    if(eb.cursorCol>val.length);\n"
		"        {\n"
		"        eb.cursorCol=val.length;\n"
		"        }\n"
		"    if(eb.cursorlayer == eb_ibeam)\n"
		"        {\n"
		"        eb_ibeam.moveToAbsolute(eb.ContentLayer.pageX + eb.cursorCol*eb_metric.charWidth, eb.ContentLayer.pageY);\n"
		"        }\n"
		"    }\n",0);

	/** Set value function **/
	htrAddScriptFunction(s, "eb_setvalue", "\n"
		"function eb_setvalue(v,f)\n"
		"    {\n"
		"    eb_settext(this,String(v));\n"
		"    eb_update_cursor(this,String(v));\n"
		"    }\n", 0);
	
	/** Clear function **/
	htrAddScriptFunction(s, "eb_clearvalue", "\n"
		"function eb_clearvalue()\n"
		"    {\n"
		"    eb_settext(this,new String(''));\n"
		"    eb_update_cursor(this,String(''));\n"
		"    }\n", 0);

	/** Enable control function **/
	htrAddScriptFunction(s, "eb_enable", "\n"
		"function eb_enable()\n"
		"    {\n"
		"    this.enabled='full';\n"
		"    }\n", 0);

	/** Disable control function **/
	htrAddScriptFunction(s, "eb_disable", "\n"
		"function eb_disable()\n"
		"    {\n"
		"    this.enabled='disabled';\n"
		"    }\n", 0);

	/** Readonly-mode function **/
	htrAddScriptFunction(s, "eb_readonly", "\n"
		"function eb_readonly()\n"
		"    {\n"
		"    this.enabled='readonly';\n"
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
		"    if(eb_current.enabled!='full') return 1;\n"
		"    txt = l.content;\n"
		"    if (k == 9)\n"
		"        {\n"
		"        if(l.form) l.form.TabNotify(this)\n"
		"        }\n"
		"    if (k >= 32 && k < 127)\n"
		"        {\n"
		"        newtxt = txt.substr(0,l.cursorCol) + String.fromCharCode(k) + txt.substr(l.cursorCol,txt.length);\n"
		"        eb_ibeam.moveToAbsolute(l.ContentLayer.pageX + l.cursorCol*eb_metric.charWidth, l.ContentLayer.pageY);\n"
		"        l.cursorCol++;\n"
		"        l.changed=true;\n"
		"        }\n"
		"    else if (k == 8 && l.cursorCol > 0)\n"
		"        {\n"
		"        newtxt = txt.substr(0,l.cursorCol-1) + txt.substr(l.cursorCol,txt.length);\n"
		"        l.cursorCol--;\n"
		"        l.changed=true;\n"
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
		"    if(l.form) l.form.FocusNotify(l);\n"
		"    if(l.enabled=='disabled') return 0;\n"
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
		"    return 1;\n"
		"    }\n", 0);

	/** Take focus away from editbox **/
	htrAddScriptFunction(s, "eb_deselect", "\n"
		"function eb_deselect()\n"
		"    {\n"
		"    eb_ibeam.visibility = 'hidden';\n"
		"    if (eb_current) eb_current.cursorlayer = null;\n"
		"    if(eb_current && eb_current.changed)\n"
		"        {\n"
		"        eb_current.changed=false;\n"
		"        if (eb_current.form)\n"
		"            {\n"
		"            eb_current.form.DataNotify(eb_current);\n"
		"            }\n"
		"        }\n"
		"    eb_current = null;\n"
		"    return true;\n"
		"    }\n", 0);

	/** Editbox initializer **/
	htrAddScriptFunction(s, "eb_init", "\n"
		"function eb_init(l,c1,c2,fieldname)\n"
		"    {\n"
		"    l.kind = 'editbox'\n"
		"    l.document.Layer = l;\n"
		"    l.ContentLayer = c1;\n"
		"    l.HiddenLayer = c2;\n"
		"    l.fieldname = fieldname\n"
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
		"    l.clearvalue = eb_clearvalue;\n"
		"    l.setoptions = null;\n"
		"    l.enable = eb_enable;\n"
		"    l.disable = eb_disable;\n"
		"    l.readonly = eb_readonly;\n"
		"    l.isFormStatusWidget = false;\n"
		"    pg_addarea(l, -1,-1,l.clip.width+1,l.clip.height+1, 'ebox', 'ebox', 1);\n"
		"    c1.y = ((l.clip.height - eb_metric.charHeight)/2);\n"
		"    c2.y = ((l.clip.height - eb_metric.charHeight)/2);\n"
		"    if (fm_current) fm_current.Register(l);\n"
		"    l.form = fm_current;\n"
		"    l.changed = false;\n"
		"    l.enabled = 'full';\n"
		"    return l;\n"
		"    }\n", 0);

	/** Script initialization call. **/
	snprintf(sbuf,250,"    %s = eb_init(%s.layers.eb%dbase, %s.layers.eb%dbase.document.layers.eb%dcon1,%s.layers.eb%dbase.document.layers.eb%dcon2,\"%s\");\n",
		nptr, parentname, id, 
		parentname, id, id, 
		parentname, id, id,
		fieldname);
	htrAddScriptInit(s, sbuf);

	/** HTML body <DIV> element for the base layer. **/
	snprintf(sbuf, HT_SBUF_SIZE, "<DIV ID=\"eb%dbase\">\n",id);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf, HT_SBUF_SIZE, "    <TABLE width=%d cellspacing=0 cellpadding=0 border=0 %s>\n",w,main_bg);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf, HT_SBUF_SIZE, "        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c1);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf, HT_SBUF_SIZE, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c1,w-2);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf, HT_SBUF_SIZE, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n",c1);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf, HT_SBUF_SIZE, "        <TR><TD><IMG SRC=/sys/images/%s height=%d width=1></TD>\n",c1,h-2);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf, HT_SBUF_SIZE, "            <TD>&nbsp;</TD>\n");
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf, HT_SBUF_SIZE, "            <TD><IMG SRC=/sys/images/%s height=%d width=1></TD></TR>\n",c2,h-2);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf, HT_SBUF_SIZE, "        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c2);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf, HT_SBUF_SIZE, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c2,w-2);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf, HT_SBUF_SIZE, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n    </TABLE>\n\n",c2);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf, HT_SBUF_SIZE, "<DIV ID=\"eb%dcon1\"></DIV>\n",id);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf, HT_SBUF_SIZE, "<DIV ID=\"eb%dcon2\"></DIV>\n",id);
	htrAddBodyItem(s, sbuf);

	/** Check for objects within the editbox. **/
	/** The editbox can have no subwidgets **/
	/*sprintf(sbuf,"%s.mainlayer.document",nptr);*/
	/*sprintf(sbuf2,"%s.mainlayer",nptr);*/
	/*htrRenderSubwidgets(s, w_obj, sbuf, sbuf2, z+2);*/

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
