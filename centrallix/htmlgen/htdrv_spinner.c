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

/*This file was based on the edit box file revision 1.2*/


/**CVSDATA***************************************************************

    $Id: htdrv_spinner.c,v 1.4 2002/03/16 03:57:55 bones120 Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_spinner.c,v $

    $Log: htdrv_spinner.c,v $
    Revision 1.4  2002/03/16 03:57:55  bones120
    Finally, it works...sort of :)

    Revision 1.3  2002/03/16 02:45:46  bones120
    Making the spinner interact with the form without dying!

    Revision 1.2  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

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
    HTSPNR;


/*** htspnrVerify - not written yet.
 ***/
int
htspnrVerify()
    {
    return 0;
    }


/*** htspnrRender - generate the HTML code for the spinner widget.
 ***/
int
htspnrRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
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
	id = (HTSPNR.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) 
	    {
	    mssError(1,"HTSPNR","Spinner widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0)
	    {
	    mssError(1,"HTSPNR","Spinner widget must have a 'height' property");
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
	name[63]=0;

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
	snprintf(sbuf,512,"    <STYLE TYPE=\"text/css\">\n");
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,512,"\t#spnr%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,512,"\t#spnr%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,1,1,w-12,z);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,512,"\t#spnr%dcon1 { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,1,1,w-2-12,z+1);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,512,"\t#spnr%dcon2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,1,1,w-2-12,z+1);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,512,"\t#spnr_button_up { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",1+w-12,1,w,z);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,512,"\t#spnr_button_down { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",1+w-12,1+9,w,z);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,512,"    </STYLE>\n");
	htrAddHeaderItem(s,sbuf);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Global for ibeam cursor layer **/
	htrAddScriptGlobal(s, "spnr_ibeam", "null", 0);
	htrAddScriptGlobal(s, "spnr_metric", "null", 0);
	htrAddScriptGlobal(s, "spnr_current", "null", 0);

	/** Get value function **/
	htrAddScriptFunction(s, "spnr_getvalue", "\n"
		"function spnr_getvalue()\n"
		"    {\n"
		"    return parseFloat(this.content);\n"
		"    }\n", 0);

	/** Set value function **/
	htrAddScriptFunction(s, "spnr_setvalue", "\n"
		"function spnr_setvalue(v,f)\n"
		"    {\n"
		"    if(this.form) this.form.DataNotify(this);\n"
		"    spnr_settext(this,v);\n"
		"    }\n", 0);

	/** Enable control function **/
	htrAddScriptFunction(s, "spnr_enable", "\n"
		"function spnr_enable()\n"
		"    {\n"
		"    }\n", 0);

	/** Disable control function **/
	htrAddScriptFunction(s, "spnr_disable", "\n"
		"function spnr_disable()\n"
		"    {\n"
		"    }\n", 0);

	/** Readonly-mode function **/
	htrAddScriptFunction(s, "spnr_readonly", "\n"
		"function spnr_readonly()\n"
		"    {\n"
		"    }\n", 0);

	/** Clear value function **/
	htrAddScriptFunction(s, "spnr_clearvalue", "\n"
		"function spnr_clearvalue()\n"
		"    {\n"
		"    }\n", 0);

	/** Editbox set-text-value function **/
	htrAddScriptFunction(s, "spnr_settext", "\n"
		"function spnr_settext(l,txt)\n"
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
	htrAddScriptFunction(s, "spnr_keyhandler", "\n"
		"function spnr_keyhandler(l,e,k)\n"
		"    {\n"
		"    txt = l.content;\n"
		"    if (k >= 49 && k < 58)\n"
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
		"    spnr_ibeam.visibility = 'hidden';\n"
		"    spnr_ibeam.moveToAbsolute(l.ContentLayer.pageX + l.cursorCol*spnr_metric.charWidth, l.ContentLayer.pageY);\n"
		"    spnr_settext(l,newtxt);\n"
		"    adj = 0;\n"
		"    if (spnr_ibeam.pageX < l.pageX + 1)\n"
		"        adj = l.pageX + 1 - spnr_ibeam.pageX;\n"
		"    else if (spnr_ibeam.pageX > l.pageX + l.clip.width - 1)\n"
		"        adj = (l.pageX + l.clip.width - 1) - spnr_ibeam.pageX;\n"
		"    if (adj != 0)\n"
		"        {\n"
		"        spnr_ibeam.pageX += adj;\n"
		"        l.ContentLayer.pageX += adj;\n"
		"        l.HiddenLayer.pageX += adj;\n"
		"        }\n"
		"    spnr_ibeam.visibility = 'inherit';\n"
		"    return false;\n"
		"    }\n", 0);

	/** Set focus to a new editbox **/
	htrAddScriptFunction(s, "spnr_select", "\n"
		"function spnr_select(x,y,l,c,n)\n"
		"    {\n"
		"    l.cursorCol = Math.round((x + l.pageX - l.ContentLayer.pageX)/spnr_metric.charWidth);\n"
		"    if (l.cursorCol > l.content.length) l.cursorCol = l.content.length;\n"
		"    if (spnr_current) spnr_current.cursorlayer = null;\n"
		"    spnr_current = l;\n"
		"    spnr_current.cursorlayer = spnr_ibeam;\n"
		"    spnr_ibeam.visibility = 'hidden';\n"
		"    spnr_ibeam.moveAbove(spnr_current);\n"
		"    spnr_ibeam.moveToAbsolute(spnr_current.ContentLayer.pageX + spnr_current.cursorCol*spnr_metric.charWidth, spnr_current.ContentLayer.pageY);\n"
		"    spnr_ibeam.zIndex = spnr_current.zIndex + 2;\n"
		"    spnr_ibeam.visibility = 'inherit';\n"
		"    l.form.focusnotify(l);\n"
		"    return 1;\n"
		"    }\n", 0);

	/** Take focus away from editbox **/
	htrAddScriptFunction(s, "spnr_deselect", "\n"
		"function spnr_deselect()\n"
		"    {\n"
		"    spnr_ibeam.visibility = 'hidden';\n"
		"    if (spnr_current) spnr_current.cursorlayer = null;\n"
		"    spnr_current = null;\n"
		"    return true;\n"
		"    }\n", 0);

	htrAddEventHandler(s, "document","MOUSEDOWN", "spnr", 
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
	htrAddScriptFunction(s, "spnr_init", "\n"
		"function spnr_init(main,l,c1,c2)\n"
		"    {\n"
		"    l.content = 0;\n"
		"    l.mainlayer=main;\n"
		"    l.document.Layer = l;\n"
		"    main.ContentLayer = c1;\n"
		"    main.ContentLayer.document.write('0');\n"
		"    main.ContentLayer.document.close();\n"
		"    main.HiddenLayer = c2;\n"
		"    main.HiddenLayer.document.write('0');\n"
		"    main.HiddenLayer.document.close();\n"
		"    main.form=fm_current;\n"
		"    if (!spnr_ibeam || !spnr_metric)\n"
		"        {\n"
		"        spnr_metric = new Layer(24);\n"
		"        spnr_metric.visibility = 'hidden';\n"
		"        spnr_metric.document.write('<pre>xx</pre>');\n"
		"        spnr_metric.document.close();\n"
		"        w2 = spnr_metric.clip.width;\n"
		"        h1 = spnr_metric.clip.height;\n"
		"        spnr_metric.document.write('<pre>x\\nx</pre>');\n"
		"        spnr_metric.document.close();\n"
		"        spnr_metric.charHeight = spnr_metric.clip.height - h1;\n"
		"        spnr_metric.charWidth = w2 - spnr_metric.clip.width;\n"
		"        spnr_ibeam = new Layer(1);\n"
		"        spnr_ibeam.visibility = 'hidden';\n"
		"        spnr_ibeam.document.write('<body bgcolor=' + page.dtcolor1 + '></body>');\n"
		"        spnr_ibeam.document.close();\n"
		"        spnr_ibeam.resizeTo(1,spnr_metric.charHeight);\n"
		"        }\n"
		"    c1.mainlayer = main;\n"
		"    c2.mainlayer = main;\n"
		"    c1.kind = 'spinner';\n"
		"    c2.kind = 'spinner';\n"
		"    main.kind = 'spinner';\n"
		"    main.layers.spnr_button_up.document.images[0].kind='spinner';\n"
		"    main.layers.spnr_button_down.document.images[0].kind='spinner';\n"
		"    main.layers.spnr_button_up.document.images[0].subkind='up';\n"
		"    main.layers.spnr_button_down.document.images[0].subkind='down';\n"
		"    main.layers.spnr_button_up.document.images[0].eb_layers=main;\n"
		"    main.layers.spnr_button_down.document.images[0].eb_layers=main;\n"
		"    l.keyhandler = spnr_keyhandler;\n"
		"    l.getfocushandler = spnr_select;\n"
		"    l.losefocushandler = spnr_deselect;\n"
		"    main.getvalue = spnr_getvalue;\n"
		"    main.setvalue = spnr_setvalue;\n"
		"    main.setoptions = null;\n"
		"    main.enable = spnr_enable;\n"
		"    main.disable = spnr_disable;\n"
		"    main.readonly = spnr_readonly;\n"
		"    main.clearvalue = spnr_clearvalue;\n"
		"    main.isFormStatusWidget = false;\n"
		"    pg_addarea(main, -1,-1,main.clip.width+1,main.clip.height+1, 'spinner', 'spinner', 1);\n"
		"    c1.y = ((l.clip.height - spnr_metric.charHeight)/2);\n"
		"    c2.y = ((l.clip.height - spnr_metric.charHeight)/2);\n"
		"    if (fm_current) fm_current.Register(main);\n"
		"    if (fm_current) main.form = fm_current;\n"
		"    return main;\n"
		"    }\n", 0);

	/** Script initialization call. **/
	snprintf(sbuf,512,"    %s = spnr_init(%s.layers.spnr%dmain, %s.layers.spnr%dmain.layers.spnr%dbase, %s.layers.spnr%dmain.layers.spnr%dbase.document.layers.spnr%dcon1, %s.layers.spnr%dmain.layers.spnr%dbase.document.layers.spnr%dcon2);\n",
		nptr, parentname, id, 
                parentname, id, id,
		parentname, id, id, id, 
		parentname, id, id, id);
	htrAddScriptInit(s, sbuf);

	/** HTML body <DIV> element for the base layer. **/

	snprintf(sbuf,512,"<DIV ID=\"spnr%dmain\">\n",id);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,512,"<DIV ID=\"spnr%dbase\">\n",id);
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
	snprintf(sbuf,512, "<DIV ID=\"spnr%dcon1\"></DIV>\n",id);
	htrAddBodyItem(s, sbuf);
	snprintf(sbuf,512, "<DIV ID=\"spnr%dcon2\"></DIV>\n",id);
	htrAddBodyItem(s, sbuf);
	htrAddBodyItem(s, "</DIV>\n");
	/*Add the spinner buttons*/
	snprintf(sbuf,512, "<DIV ID=\"spnr_button_up\"><IMG SRC=\"/sys/images/spnr_up.gif\"></DIV>\n");
	htrAddBodyItem(s,sbuf);
	snprintf(sbuf,512, "<DIV ID=\"spnr_button_down\"><IMG SRC=\"/sys/images/spnr_down.gif\"></DIV>\n");
	htrAddBodyItem(s,sbuf);
  	htrAddBodyItem(s, "</DIV>\n"); 
    return 0;
    }


/*** htspnrInitialize - register with the ht_render module.
 ***/
int
htspnrInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Spinner Box Driver");
	strcpy(drv->WidgetName,"spinner");
	drv->Render = htspnrRender;
	drv->Verify = htspnrVerify;

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

	HTSPNR.idcnt = 0;

    return 0;
    }
