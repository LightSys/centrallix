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
#include "centrallix.h"

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
/* Module: 	htdrv_page.c           					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 19, 1998					*/
/* Description:	HTML Widget driver for the overall HTML page.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_page.c,v 1.13 2002/06/02 22:13:21 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_page.c,v $

    $Log: htdrv_page.c,v $
    Revision 1.13  2002/06/02 22:13:21  jorupp
     * added disable functionality to image button (two new Actions)
     * bugfixes

    Revision 1.12  2002/05/31 19:22:03  lkehresman
    * Added option to dropdown to allow specification of number of elements
      to display at one time (default 3).
    * Fixed some places that were getting truncated prematurely.

    Revision 1.11  2002/05/08 00:46:30  jorupp
     * added client-side support for the session watchdog.  The client will ping every timer/2 seconds (just to be safe)

    Revision 1.10  2002/04/28 03:19:53  gbeeley
    Fixed a bit of a bug in ht_render where it did not properly set the
    length on the StrValue structures when adding script functions.  This
    was basically causing some substantial heap corruption.

    Revision 1.9  2002/03/23 01:18:09  lkehresman
    Fixed focus detection and form notification on editbox and anything that
    uses keyboard input.

    Revision 1.8  2002/03/16 06:53:34  gbeeley
    Added modal-layer function at the page level.  Calling pg_setmodal(l)
    causes all mouse activity outside of layer l to be ignored.  Useful
    when you need to require the user to act on a certain window/etc.
    Call pg_setmodal(null) to clear the modal status.  Note: keep the
    modal layer simple!  The algorithm does quite a bit of poking around
    to figure out whether the activity is targeted within the given modal
    layer or not.  NOTE:  for modal windows, if you want to keep the user
    from clicking the 'x' on the window, set the window's mainLayer to be
    modal instead of the window itself, although the user will not be able
    to even drag the window in that case, which might be desirable.

    Revision 1.7  2002/03/15 22:40:47  gbeeley
    Modified key input logic in the page widget to improve key debouncing
    and make key repeat rate and delay a bit more natural and more
    consistent across different machines and platforms.

    Revision 1.6  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.5  2002/02/27 01:38:51  jheth
    Initial commit of object source

    Revision 1.4  2002/02/22 23:48:39  jorupp
    allow editbox to work without form, form compiles, doesn't do much

    Revision 1.3  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.2  2001/10/23 00:25:09  gbeeley
    Added rudimentary single-line editbox widget.  No data source linking
    or anything like that yet.  Fixed a few bugs and made a few changes to
    other controls to make this work more smoothly.  Page widget still needs
    some key de-bounce and key repeat overhaul.  Arrow keys don't work in
    Netscape 4.xx.

    Revision 1.1.1.1  2001/08/13 18:00:50  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:54  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** htpageVerify - not written yet.
 ***/
int
htpageVerify()
    {
    return 0;
    }

/*** htpageRender - generate the HTML code for the page.
 ***/
int
htpageRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char kbfocus1[64] = "#ffffff";	/* kb focus = 3d raised */
    char kbfocus2[64] = "#7a7a7a";
    char msfocus1[64] = "#000000";	/* ms focus = black rectangle */
    char msfocus2[64] = "#000000";
    char dtfocus1[64] = "#000080";	/* dt focus = navyblue rectangle */
    char dtfocus2[64] = "#000080";
    pObject sub_w_obj;
    pObjQuery qy;
    int watchdogtimer;

    	/** If not at top-level, don't render the page. **/
	/** Z is set to 10 for the top-level page. **/
	if (z != 10) return 0;

    	/** Check for a title. **/
	if (objGetAttrValue(w_obj,"title",POD(&ptr)) == 0)
	    {
	    htrAddHeaderItem_va(s, "    <TITLE>%s</TITLE>\n",ptr);
	    }

	/** Check for bgcolor. **/
	if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
	    {
	    htrAddBodyParam_va(s, " BGCOLOR=%s",ptr);
	    }
	if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
	    {
	    htrAddBodyParam_va(s, " BACKGROUND=%s",ptr);
	    }

	/** Check for text color **/
	if (objGetAttrValue(w_obj,"textcolor",POD(&ptr)) == 0)
	    {
	    htrAddBodyParam_va(s, " TEXT=%s",ptr);
	    }

	/** Keyboard Focus Indicator colors 1 and 2 **/
	if (objGetAttrValue(w_obj,"kbdfocus1",POD(&ptr)) == 0)
	    {
	    memccpy(kbfocus1,ptr,0,63);
	    kbfocus1[63]=0;
	    }
	if (objGetAttrValue(w_obj,"kbdfocus2",POD(&ptr)) == 0)
	    {
	    memccpy(kbfocus2,ptr,0,63);
	    kbfocus2[63]=0;
	    }

	/** Mouse Focus Indicator colors 1 and 2 **/
	if (objGetAttrValue(w_obj,"mousefocus1",POD(&ptr)) == 0)
	    {
	    memccpy(msfocus1,ptr,0,63);
	    msfocus1[63]=0;
	    }
	if (objGetAttrValue(w_obj,"mousefocus2",POD(&ptr)) == 0)
	    {
	    memccpy(msfocus2,ptr,0,63);
	    msfocus2[63]=0;
	    }

	/** Data Focus Indicator colors 1 and 2 **/
	if (objGetAttrValue(w_obj,"datafocus1",POD(&ptr)) == 0)
	    {
	    memccpy(dtfocus1,ptr,0,63);
	    dtfocus1[63]=0;
	    }
	if (objGetAttrValue(w_obj,"datafocus2",POD(&ptr)) == 0)
	    {
	    memccpy(dtfocus2,ptr,0,63);
	    dtfocus2[63]=0;
	    }

	/** Add global for page metadata **/
	htrAddScriptGlobal(s, "page", "new Object()", 0);

	/** Add a list of highlightable areas **/
	htrAddScriptGlobal(s, "pg_arealist", "new Array()", 0);
	htrAddScriptGlobal(s, "pg_keylist", "new Array()", 0);
	htrAddScriptGlobal(s, "pg_curarea", "null", 0);
	htrAddScriptGlobal(s, "pg_curlayer", "null", 0);
	htrAddScriptGlobal(s, "pg_curkbdlayer", "null", 0);
	htrAddScriptGlobal(s, "pg_curkbdarea", "null", 0);
	htrAddScriptGlobal(s, "pg_lastkey", "-1", 0);
	htrAddScriptGlobal(s, "pg_lastmodifiers", "null", 0);
	htrAddScriptGlobal(s, "pg_keytimeoutid", "null", 0);
	htrAddScriptGlobal(s, "pg_modallayer", "null", 0);
	htrAddScriptGlobal(s, "fm_current", "null", 0);
	htrAddScriptGlobal(s, "osrc_current", "null", 0);

	/** Add focus box **/
	htrAddHeaderItem(s, 
	        "    <STYLE TYPE=\"text/css\">\n"
		"        #pgtop { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(1,1); Z-INDEX:1000;}\n"
		"        #pgbtm { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(1,1); Z-INDEX:1000;}\n"
		"        #pgrgt { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(1,1); Z-INDEX:1000;}\n"
		"        #pglft { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(1,1); Z-INDEX:1000;}\n"
		"        #pgtvl { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:1; Z-INDEX:0; }\n"
		"        #pgktop { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(1,1); Z-INDEX:1000;}\n"
		"        #pgkbtm { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1152;HEIGHT:1; clip:rect(1,1); Z-INDEX:1000;}\n"
		"        #pgkrgt { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(1,1); Z-INDEX:1000;}\n"
		"        #pgklft { POSITION:absolute; VISIBILITY:hidden; LEFT:0;TOP:0;WIDTH:1;HEIGHT:864; clip:rect(1,1); Z-INDEX:1000;}\n"
		"        #pginpt { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:20; Z-INDEX:20; }\n"
		"        #pgping { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:0; Z-INDEX:0; }\n"
		"    </STYLE>\n" );
	htrAddBodyItem(s, "<DIV ID=\"pgtop\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgbtm\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgrgt\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pglft\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgtvl\"></DIV>\n");
	htrAddBodyItem(s,
		"<DIV ID=\"pgktop\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n"
		"<DIV ID=\"pgkbtm\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n"
		"<DIV ID=\"pgkrgt\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n"
		"<DIV ID=\"pgklft\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n");
	htrAddBodyItem(s, "<DIV ID=\"pgping\"></DIV>\n");

	htrAddScriptFunction(s, "pg_ping_init","\n"
		"function pg_ping_init(l,i)\n"
		"    {\n"
		"    l.tid=setInterval(pg_ping_send,i,l);\n"
		"    }\n"
		"\n",0);
	
	htrAddScriptFunction(s, "pg_ping_recieve","\n"
		"function pg_ping_recieve()\n"
		"    {\n"
		"    if(this.document.links[0].target!=='OK')\n"
		"        {\n"
		"        clearInterval(this.tid);\n"
		"        confirm('you have been disconnected from the server');\n"
		"        }\n"
		"    }\n"
		"\n",0);

	htrAddScriptFunction(s, "pg_ping_send","\n"
		"function pg_ping_send(p)\n"
		"    {\n"
		"    //confirm('sending');\n"
		"    p.onload=pg_ping_recieve;\n"
		"    p.src='/INTERNAL/ping';\n"
		"    }\n"
		"\n",0);

	stAttrValue(stLookup(stLookup(CxGlobals.ParsedConfig, "net_http"),"session_watchdog_timer"),&watchdogtimer,NULL,0);
	htrAddScriptInit_va(s,"    pg_ping_init(%s.layers.pgping,%i);\n",parentname,watchdogtimer/2*1000);

	/** Add event code to handle mouse in/out of the area.... **/
	htrAddEventHandler(s, "document", "MOUSEMOVE","pg",
		"    if (pg_modallayer)\n"
		"        {\n"
		"        if (e.target != null && e.target.layer != null) ly = e.target.layer;\n"
		"        else ly = e.target;\n"
		"        if (!pg_isinlayer(pg_modallayer, ly)) return false;\n"
		"        }\n"
		"    if (pg_curlayer != null)\n"
		"        {\n"
		"        for(i=0;i<pg_arealist.length;i++) if (pg_curlayer == pg_arealist[i].layer && e.x >= pg_arealist[i].x &&\n"
		"                e.y >= pg_arealist[i].y && e.x < pg_arealist[i].x+pg_arealist[i].width &&\n"
		"                e.y < pg_arealist[i].y+pg_arealist[i].height && pg_curarea != pg_arealist[i])\n"
		"            {\n"
		"            if (pg_curarea == pg_arealist[i]) break;\n"
		"            pg_curarea = pg_arealist[i];\n"
		"            x = pg_curarea.layer.pageX+pg_curarea.x;\n"
		"            y = pg_curarea.layer.pageY+pg_curarea.y;\n"
		"            w = pg_curarea.width;\n"
		"            h = pg_curarea.height;\n"
		"            pg_mkbox(pg_curlayer,x,y,w,h, 1, document.layers.pgtop,document.layers.pgbtm,document.layers.pgrgt,document.layers.pglft, page.mscolor1, page.mscolor2, document.layers.pgktop.zIndex-1);\n"
		"            break;\n"
		"            }\n"
		"        }\n" );
	htrAddEventHandler(s, "document", "MOUSEOUT", "pg",
		"    if (pg_modallayer)\n"
		"        {\n"
		"        if (e.target != null && e.target.layer != null) ly = e.target.layer;\n"
		"        else ly = e.target;\n"
		"        if (!pg_isinlayer(pg_modallayer, ly)) return false;\n"
		"        }\n"
		"    if (e.target == pg_curlayer) pg_curlayer = null;\n"
		"    if (e.target != null && pg_curarea != null && e.target == pg_curarea.layer)\n"
		"        {\n"
		"        pg_hidebox(document.layers.pgtop,document.layers.pgbtm,document.layers.pgrgt,document.layers.pglft);\n"
		"        pg_curarea = null;\n"
		"        }\n" );
	htrAddEventHandler(s, "document", "MOUSEOVER", "pg",
		"    if (pg_modallayer)\n"
		"        {\n"
		"        if (e.target != null && e.target.layer != null) ly = e.target.layer;\n"
		"        else ly = e.target;\n"
		"        if (!pg_isinlayer(pg_modallayer, ly)) return false;\n"
		"        }\n"
		"    if (e.target != null && e.target.pageX != null)\n"
		"        {\n"
		"        pg_curlayer = e.target;\n"
		"        while(pg_curlayer.mainlayer != null) pg_curlayer = pg_curlayer.mainlayer;\n"
		"        }\n" );

	/** CLICK event handler is for making mouse focus the keyboard focus **/
	htrAddEventHandler(s, "document", "MOUSEDOWN", "pg",
		"    if (pg_modallayer)\n"
		"        {\n"
		"        if (e.target != null && e.target.layer != null) ly = e.target.layer;\n"
		"        else ly = e.target;\n"
		"        if (!pg_isinlayer(pg_modallayer, ly)) return false;\n"
		"        }\n"
		"    if (pg_curarea != null)\n"
		"        {\n"
		"        x = pg_curarea.layer.pageX+pg_curarea.x;\n"
		"        y = pg_curarea.layer.pageY+pg_curarea.y;\n"
		"        w = pg_curarea.width;\n"
		"        h = pg_curarea.height;\n"
		"        if (pg_curkbdlayer && pg_curkbdlayer.losefocushandler)\n"
		"            {\n"
		"            if (!pg_curkbdlayer.losefocushandler()) return true;\n"
		"            }\n"
		"        pg_curkbdarea = pg_curarea;\n"
		"        pg_curkbdlayer = pg_curlayer;\n"
		"        if (pg_curkbdlayer.getfocushandler)\n"
		"            {\n"
		"            v=pg_curkbdlayer.getfocushandler(e.pageX-pg_curarea.layer.pageX,e.pageY-pg_curarea.layer.pageY,pg_curarea.layer,pg_curarea.cls,pg_curarea.name);\n"
		"            if (v & 1)\n"
		"                {\n"
		"                pg_mkbox(pg_curlayer,x,y,w,h, 1, document.layers.pgktop,document.layers.pgkbtm,document.layers.pgkrgt,document.layers.pgklft, page.kbcolor1, page.kbcolor2, document.layers.pgtop.zIndex+2);\n"
		"                }\n"
		"            if (v & 2)\n"
		"                {\n"
		"                if (pg_curlayer.pg_dttop != null)\n"
		"                    {\n"
		"                    pg_hidebox(pg_curlayer.pg_dttop,pg_curlayer.pg_dtbtm,pg_curlayer.pg_dtrgt,pg_curlayer.pg_dtlft);\n"
		"                    }\n"
		"                else\n"
		"                    {\n"
		"                    pg_curlayer.pg_dttop = new Layer(1152);\n"
		"                    //pg_curlayer.pg_dttop.document.write('<IMG SRC=/sys/images/trans_1.gif width=1152 height=1>');\n"
		"                    //pg_curlayer.pg_dttop.document.close();\n"
		"                    pg_curlayer.pg_dtbtm = new Layer(1152);\n"
		"                    //pg_curlayer.pg_dtbtm.document.write('<IMG SRC=/sys/images/trans_1.gif width=1152 height=1>');\n"
		"                    //pg_curlayer.pg_dtbtm.document.close();\n"
		"                    pg_curlayer.pg_dtrgt = new Layer(2);\n"
		"                    //pg_curlayer.pg_dtrgt.document.write('<IMG SRC=/sys/images/trans_1.gif height=864 width=1>');\n"
		"                    //pg_curlayer.pg_dtrgt.document.close();\n"
		"                    pg_curlayer.pg_dtlft = new Layer(2);\n"
		"                    //pg_curlayer.pg_dtlft.document.write('<IMG SRC=/sys/images/trans_1.gif height=864 width=1>');\n"
		"                    //pg_curlayer.pg_dtlft.document.close();\n"
		"                    }\n"
		"                pg_mkbox(pg_curlayer,x-1,y-1,w+2,h+2, 1,  pg_curlayer.pg_dttop,pg_curlayer.pg_dtbtm,pg_curlayer.pg_dtrgt,pg_curlayer.pg_dtlft, page.dtcolor1, page.dtcolor2, document.layers.pgtop.zIndex+1);\n"
		"                }\n"
		"            }\n"
		"        }\n"
		"    else if (pg_curkbdlayer != null)\n"
		"        {\n"
		"        if (!pg_curkbdlayer.losefocushandler()) return true;\n"
		"        pg_curkbdarea = null;\n"
		"        pg_curkbdlayer = null;\n"
		"        }\n");

	/** This resets the keyboard focus. **/
	htrAddEventHandler(s, "document", "MOUSEUP", "pg",
		"    if (pg_modallayer)\n"
		"        {\n"
		"        if (e.target != null && e.target.layer != null) ly = e.target.layer;\n"
		"        else ly = e.target;\n"
		"        if (!pg_isinlayer(pg_modallayer, ly)) return false;\n"
		"        }\n"
		"    setTimeout('document.layers.pginpt.document.tmpform.x.focus()',10);\n");

	/** Set colors for the focus layers **/
	htrAddScriptInit_va(s, "    page.kbcolor1 = '%s';\n    page.kbcolor2 = '%s';\n",kbfocus1,kbfocus2);
	htrAddScriptInit_va(s, "    page.mscolor1 = '%s';\n    page.mscolor2 = '%s';\n",msfocus1,msfocus2);
	htrAddScriptInit_va(s, "    page.dtcolor1 = '%s';\n    page.dtcolor2 = '%s';\n",dtfocus1,dtfocus2);
	htrAddScriptInit(s, "    document.LSParent = null;\n");

	/** Function to set modal mode to a layer. **/
	htrAddScriptFunction(s, "pg_setmodal", "\n"
		"function pg_setmodal(l)\n"
		"    {\n"
		"    pg_modallayer = l;\n"
		"    }\n", 0);

	/** Function to find out whether image or layer is in a layer **/
	htrAddScriptFunction(s, "pg_isinlayer", "\n"
		"function pg_isinlayer(outer,inner)\n"
		"    {\n"
		"    if (inner == outer) return true;\n"
		"    if(!outer) return true;\n"
		"    if(!inner) return false;\n"
		"    var i = 0;\n"
		"    for(i=0;i<outer.layers.length;i++)\n"
		"        {\n"
		"        if (outer.layers[i] == inner) return true;\n"
		"        if (pg_isinlayer(outer.layers[i], inner)) return true;\n"
		"        }\n"
		"    for(i=0;i<outer.document.images.length;i++)\n"
		"        {\n"
		"        if (outer.document.images[i] == inner) return true;\n"
		"        }\n"
		"    return false;\n"
		"    }\n", 0);

	/** Function to make four layers into a box **/
	htrAddScriptFunction(s, "pg_mkbox", "\n"
		"function pg_mkbox(pl, x,y,w,h, s, tl,bl,rl,ll, c1,c2, z)\n"
		"    {\n"
		"    tl.visibility = 'hidden';\n"
		"    bl.visibility = 'hidden';\n"
		"    rl.visibility = 'hidden';\n"
		"    ll.visibility = 'hidden';\n"
		"    tl.bgColor = c1;\n"
		"    ll.bgColor = c1;\n"
		"    bl.bgColor = c2;\n"
		"    rl.bgColor = c2;\n"
		"    tl.resizeTo(w,1);\n"
		"    tl.moveAbove(pl);\n"
		"    tl.moveToAbsolute(x,y);\n"
		"    tl.zIndex = z;\n"
		"    bl.resizeTo(w+s-1,1);\n"
		"    bl.moveAbove(pl);\n"
		"    bl.moveToAbsolute(x,y+h-s+1);\n"
		"    bl.zIndex = z;\n"
		"    ll.resizeTo(1,h);\n"
		"    ll.moveAbove(pl);\n"
		"    ll.moveToAbsolute(x,y);\n"
		"    ll.zIndex = z;\n"
		"    rl.resizeTo(1,h+1);\n"
		"    rl.moveAbove(pl);\n"
		"    rl.moveToAbsolute(x+w-s+1,y);\n"
		"    rl.zIndex = z;\n"
		"    tl.visibility = 'inherit';\n"
		"    bl.visibility = 'inherit';\n"
		"    rl.visibility = 'inherit';\n"
		"    ll.visibility = 'inherit';\n"
		"    return;\n"
		"    }\n", 0);

	/** To hide a box **/
	htrAddScriptFunction(s, "pg_hidebox", "\n"
		"function pg_hidebox(tl,bl,rl,ll)\n"
		"    {\n"
		"    tl.visibility = 'hidden';\n"
		"    bl.visibility = 'hidden';\n"
		"    rl.visibility = 'hidden';\n"
		"    ll.visibility = 'hidden';\n"
		"    tl.moveAbove(document.layers.pgtvl);\n"
		"    bl.moveAbove(document.layers.pgtvl);\n"
		"    rl.moveAbove(document.layers.pgtvl);\n"
		"    ll.moveAbove(document.layers.pgtvl);\n"
		"    return;\n"
		"    }\n", 0);

	/** Function to make a new clickable "area" **INTERNAL** **/
	htrAddScriptFunction(s, "pg_area", "\n"
		"function pg_area(pl,x,y,w,h,cls,nm,f)\n"
		"    {\n"
		"    this.layer = pl;\n"
		"    this.x = x;\n"
		"    this.y = y;\n"
		"    this.width = w;\n"
		"    this.height = h;\n"
		"    this.name = nm;\n"
		"    this.cls = cls;\n"
		"    this.flags = f;\n"
		"    return this;\n"
		"    }\n", 0);

	/** Function to add a new area to the arealist **/
	htrAddScriptFunction(s, "pg_addarea", "\n"
		"function pg_addarea(pl,x,y,w,h,cls,nm,f)\n"
		"    {\n"
		"    a = new pg_area(pl,x,y,w,h,cls,nm,f);\n"
		"    pg_arealist.splice(0,0,a);\n"
		"    return a;\n"
		"    }\n", 0);

	/** Function to remove an existing area... **/
	htrAddScriptFunction(s, "pg_removearea", "\n"
		"function pg_removearea(a)\n"
		"    {\n"
		"    for(i=0;i<pg_arealist.length;i++)\n"
		"        {\n"
		"        if (pg_arealist[i] == a)\n"
		"            {\n"
		"            pg_arealist.splice(i,1);\n"
		"            return 1;\n"
		"            }\n"
		"        }\n"
		"    return 0;\n"
		"    }\n", 0);

	/** Add a universal resize manager function. **/
	htrAddScriptFunction(s, "pg_resize", "\n"
		"function pg_resize(l)\n"
		"    {\n"
		"    maxheight=0;\n"
		"    maxwidth=0;\n"
		"    for(i=0;i<l.document.layers.length;i++)\n"
		"        {\n"
		"        cl = l.document.layers[i];\n"
		"        if ((cl.visibility == 'show' || cl.visibility == 'inherit') && cl.y + cl.clip.height > maxheight)\n"
		"            maxheight = cl.y + cl.clip.height;\n"
		"        if ((cl.visibility == 'show' || cl.visibility == 'inherit') && cl.x + cl.clip.width > maxwidth)\n"
		"            maxwidth = cl.x + cl.clip.width;\n"
		"        }\n"
		"    if (l.maxheight && maxheight > l.maxheight) maxheight = l.maxheight;\n"
		"    if (l.minheight && maxheight < l.minheight) maxheight = l.minheight;\n"
		"    if (l!=window) l.clip.height = maxheight;\n"
		"    else l.document.height = maxheight;\n"
		"    if (l.maxwidth && maxwidth > l.maxwidth) maxwidth = l.maxwidth;\n"
		"    if (l.minwidth && maxwidth < l.minwidth) maxwidth = l.minwidth;\n"
		"    if (l!=window) l.clip.width = maxwidth;\n"
		"    else l.document.width = maxwidth;\n"
		"    }\n", 0);

	/** Add a universal "is visible" function that handles inherited visibility. **/
	htrAddScriptFunction(s, "pg_isvisible", "\n"
		"function pg_isvisible(l)\n"
		"    {\n"
		"    if (l.visibility == 'show') return 1;\n"
		"    else if (l.visibility == 'hidden') return 0;\n"
		"    else if (l == window || l.parentLayer == null) return 1;\n"
		"    else return pg_isvisible(l.parentLayer);\n"
		"    }\n", 0);

	/** Cursor flash **/
	htrAddScriptFunction(s, "pg_togglecursor", "\n"
		"function pg_togglecursor()\n"
		"    {\n"
		"    if (pg_curkbdlayer != null && pg_curkbdlayer.cursorlayer != null)\n"
		"        {\n"
		"        if (pg_curkbdlayer.cursorlayer.visibility != 'inherit')\n"
		"            pg_curkbdlayer.cursorlayer.visibility = 'inherit';\n"
		"        else\n"
		"            pg_curkbdlayer.cursorlayer.visibility = 'hidden';\n"
		"        }\n"
		"    setTimeout(pg_togglecursor,333);\n"
		"    }\n", 0);
	htrAddScriptInit(s, "    pg_togglecursor();\n");

	/** Keyboard input handling **/
	htrAddScriptFunction(s, "pg_addkey", "\n"
		"function pg_addkey(s,e,mod,modmask,mlayer,klayer,tgt,action,aparam)\n"
		"    {\n"
		"    kd = new Object();\n"
		"    kd.startcode = s;\n"
		"    kd.endcode = e;\n"
		"    kd.mod = mod;\n"
		"    kd.modmask = modmask;\n"
		"    kd.mouselayer = mlayer;\n"
		"    kd.kbdlayer = klayer;\n"
		"    kd.target_obj = tgt;\n"
		"    kd.fnname = 'Action' + action;\n"
		"    kd.aparam = aparam;\n"
		"    pg_keylist.splice(0,0,kd);\n"
		"    pg_keylist.sort(pg_cmpkey);\n"
		"    return kd;\n"
		"    }\n", 0);
	htrAddScriptFunction(s, "pg_cmpkey", "\n"
		"function pg_cmpkey(k1,k2)\n"
		"    {\n"
		"    return (k1.endcode-k1.startcode) - (k2.endcode-k2.startcode);\n"
		"    }\n", 0);
	htrAddScriptFunction(s, "pg_removekey", "\n"
		"function pg_removekey(kd)\n"
		"    {\n"
		"    for(i=0;i<pg_keylist.length;i++)\n"
		"        {\n"
		"        if (pg_keylist[i] == kd)\n"
		"            {\n"
		"            pg_keylist.splice(i,1);\n"
		"            return 1;\n"
		"            }\n"
		"        }\n"
		"    return 0;\n"
		"    }\n", 0);
	htrAddScriptFunction(s, "pg_keytimeout", "\n"
		"function pg_keytimeout()\n"
		"    {\n"
		"    if (pg_lastkey != -1)\n"
		"        {\n"
		"        e = new Object();\n"
		"        e.which = pg_lastkey;\n"
		"        e.modifiers = pg_lastmodifiers;\n"
		"        pg_keyhandler(pg_lastkey, pg_lastmodifiers, e);\n"
		"        delete e;\n"
		"        pg_keytimeoutid = setTimeout(pg_keytimeout, 50);\n"
		"        }\n"
		"    }\n", 0);

	htrAddEventHandler(s, "document", "KEYDOWN", "pg",
		"    k = e.which;\n"
		"    if (k > 65280) k -= 65280;\n"
		"    if (k >= 128) k -= 128;\n"
		"    if (k == pg_lastkey) return false;\n"
		"    pg_lastkey = k;\n"
		"    /*pg_togglecursor();*/\n"
		"    if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);\n"
		"    pg_keytimeoutid = setTimeout(pg_keytimeout, 200);\n"
		"    return pg_keyhandler(k, e.modifiers, e);\n");

	htrAddScriptFunction(s, "pg_keyhandler", "\n"
		"function pg_keyhandler(k,m,e)\n"
		"    {\n"
		"    pg_lastmodifiers = m;\n"
		"    if (pg_curkbdlayer != null && pg_curkbdlayer.keyhandler != null && pg_curkbdlayer.keyhandler(pg_curkbdlayer,e,k) == true) return false;\n"
		"    for(i=0;i<pg_keylist.length;i++)\n"
		"        {\n"
		"        if (k >= pg_keylist[i].startcode && k <= pg_keylist[i].endcode && (pg_keylist[i].kbdlayer == null || pg_keylist[i].kbdlayer == pg_curkbdlayer) && (pg_keylist[i].mouselayer == null || pg_keylist[i].mouselayer == pg_curlayer) && (m & pg_keylist[i].modmask) == pg_keylist[i].mod)\n"
		"            {\n"
		"            pg_keylist[i].aparam.KeyCode = k;\n"
		"            pg_keylist[i].target_obj[pg_keylist[i].fnname](pg_keylist[i].aparam);\n"
		"            return false;\n"
		"            }\n"
		"        }\n"
		"    return false;\n"
		"    }\n", 0);


	htrAddBodyItem(s, "<DIV ID=pginpt><FORM name=tmpform action><textarea name=x tabindex=1 rows=1></textarea></FORM></DIV>\n");

	htrAddEventHandler(s, "document", "KEYUP", "pg",
		"    k = e.which;\n"
		"    if (k > 65280) k -= 65280;\n"
		"    if (k >= 128) k -= 128;\n"
		"    if (k == pg_lastkey) pg_lastkey = -1;\n"
		"    if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);\n"
		"    pg_keytimeoutid = null;\n");

	/** Check for more sub-widgets within the page. **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		htrRenderWidget(s, sub_w_obj, z+1, parentname, "document");
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

	htrAddScriptInit(s,
		"    document.layers.pginpt.moveTo(window.innerWidth-2, 20);\n"
		"    document.layers.pginpt.visibility = 'inherit';\n");
	htrAddScriptInit(s,"    document.layers.pginpt.document.tmpform.x.focus();\n");

    return 0;
    }


/*** htpageInitialize - register with the ht_render module.
 ***/
int
htpageInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"HTML Page Driver");
	strcpy(drv->WidgetName,"page");
	drv->Render = htpageRender;
	drv->Verify = htpageVerify;
	xaInit(&(drv->PosParams),16);
	xaInit(&(drv->Properties),16);
	xaInit(&(drv->Events),16);
	xaInit(&(drv->Actions),16);

	/** Register. **/
	htrRegisterDriver(drv);

    return 0;
    }
