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

    $Id: htdrv_spinner.c,v 1.8 2002/07/25 18:08:36 mcancel Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_spinner.c,v $

    $Log: htdrv_spinner.c,v $
    Revision 1.8  2002/07/25 18:08:36  mcancel
    Taking out the htrAddScriptFunctions out... moving the javascript code out of the c file into the js files and a little cleaning up... taking out whole deleted functions in a few and found another htrAddHeaderItem that needed to be htrAddStylesheetItem.

    Revision 1.7  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.6  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.5  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

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
	    snprintf(main_bg,128,"bgcolor='%.40s'",ptr);
	else if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
	    snprintf(main_bg,128,"background='%.110s'",ptr);
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
	htrAddStylesheetItem_va(s,"\t#spnr%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	htrAddStylesheetItem_va(s,"\t#spnr%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,1,1,w-12,z);
	htrAddStylesheetItem_va(s,"\t#spnr%dcon1 { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,1,1,w-2-12,z+1);
	htrAddStylesheetItem_va(s,"\t#spnr%dcon2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,1,1,w-2-12,z+1);
	htrAddStylesheetItem_va(s,"\t#spnr_button_up { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",1+w-12,1,w,z);
	htrAddStylesheetItem_va(s,"\t#spnr_button_down { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",1+w-12,1+9,w,z);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);
	htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Global for ibeam cursor layer **/
	htrAddScriptGlobal(s, "spnr_ibeam", "null", 0);
	htrAddScriptGlobal(s, "spnr_metric", "null", 0);
	htrAddScriptGlobal(s, "spnr_current", "null", 0);

   	htrAddScriptInclude(s,"/sys/js/htdrv_spinner.js",0);

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

	/** Script initialization call. **/
	htrAddScriptInit_va(s,"    %s = spnr_init(%s.layers.spnr%dmain, %s.layers.spnr%dmain.layers.spnr%dbase, %s.layers.spnr%dmain.layers.spnr%dbase.document.layers.spnr%dcon1, %s.layers.spnr%dmain.layers.spnr%dbase.document.layers.spnr%dcon2);\n",
		nptr, parentname, id, 
                parentname, id, id,
		parentname, id, id, id, 
		parentname, id, id, id);

	/** HTML body <DIV> element for the base layer. **/

	htrAddBodyItem_va(s,"<DIV ID=\"spnr%dmain\">\n",id);
	htrAddBodyItem_va(s,"<DIV ID=\"spnr%dbase\">\n",id);
	htrAddBodyItem_va(s,"    <TABLE width=%d cellspacing=0 cellpadding=0 border=0 %s>\n",w-12,main_bg);
	htrAddBodyItem_va(s,"        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c1);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c1,w-2-12);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n",c1);
	htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%s height=%d width=1></TD>\n",c1,h-2);
	htrAddBodyItem_va(s, "            <TD>&nbsp;</TD>\n");
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s height=%d width=1></TD></TR>\n",c2,h-2);
	htrAddBodyItem_va(s, "        <TR><TD><IMG SRC=/sys/images/%s></TD>\n",c2);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s height=1 width=%d></TD>\n",c2,w-2-12);
	htrAddBodyItem_va(s, "            <TD><IMG SRC=/sys/images/%s></TD></TR>\n    </TABLE>\n\n",c2);
	htrAddBodyItem_va(s, "<DIV ID=\"spnr%dcon1\"></DIV>\n",id);
	htrAddBodyItem_va(s, "<DIV ID=\"spnr%dcon2\"></DIV>\n",id);
	htrAddBodyItem(s, "</DIV>\n");
	/*Add the spinner buttons*/
	htrAddBodyItem_va(s, "<DIV ID=\"spnr_button_up\"><IMG SRC=\"/sys/images/spnr_up.gif\"></DIV>\n");
	htrAddBodyItem_va(s, "<DIV ID=\"spnr_button_down\"><IMG SRC=\"/sys/images/spnr_down.gif\"></DIV>\n");
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
	strcpy(drv->Target, "Netscape47x:default");


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
