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
/* Module: 	htdrv_page.c           					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 19, 1998					*/
/* Description:	HTML Widget driver for the overall HTML page.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_page.c,v 1.1 2001/08/13 18:00:50 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_page.c,v $

    $Log: htdrv_page.c,v $
    Revision 1.1  2001/08/13 18:00:50  gbeeley
    Initial revision

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
    char sbuf[256];
    char kbfocus1[64] = "#ffffff";	/* kb focus = 3d raised */
    char kbfocus2[64] = "#7a7a7a";
    char msfocus1[64] = "#000000";	/* ms focus = black rectangle */
    char msfocus2[64] = "#000000";
    char dtfocus1[64] = "#000080";	/* dt focus = navyblue rectangle */
    char dtfocus2[64] = "#000080";
    pObject sub_w_obj;
    pObjQuery qy;

    	/** If not at top-level, don't render the page. **/
	/** Z is set to 10 for the top-level page. **/
	if (z != 10) return 0;

    	/** Check for a title. **/
	if (objGetAttrValue(w_obj,"title",POD(&ptr)) == 0)
	    {
	    sprintf(sbuf,"    <TITLE>%s</TITLE>\n",ptr);
	    htrAddHeaderItem(s, sbuf);
	    }

	/** Check for bgcolor. **/
	if (objGetAttrValue(w_obj,"bgcolor",POD(&ptr)) == 0)
	    {
	    sprintf(sbuf," BGCOLOR=%s",ptr);
	    htrAddBodyParam(s, sbuf);
	    }
	if (objGetAttrValue(w_obj,"background",POD(&ptr)) == 0)
	    {
	    sprintf(sbuf," BACKGROUND=%s",ptr);
	    htrAddBodyParam(s, sbuf);
	    }

	/** Check for text color **/
	if (objGetAttrValue(w_obj,"textcolor",POD(&ptr)) == 0)
	    {
	    sprintf(sbuf," TEXT=%s",ptr);
	    htrAddBodyParam(s, sbuf);
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
	    kbfocus1[63]=0;
	    }

	/** Mouse Focus Indicator colors 1 and 2 **/
	if (objGetAttrValue(w_obj,"mousefocus1",POD(&ptr)) == 0)
	    {
	    memccpy(msfocus1,ptr,0,63);
	    kbfocus1[63]=0;
	    }
	if (objGetAttrValue(w_obj,"mousefocus2",POD(&ptr)) == 0)
	    {
	    memccpy(msfocus2,ptr,0,63);
	    kbfocus1[63]=0;
	    }

	/** Data Focus Indicator colors 1 and 2 **/
	if (objGetAttrValue(w_obj,"datafocus1",POD(&ptr)) == 0)
	    {
	    memccpy(dtfocus1,ptr,0,63);
	    kbfocus1[63]=0;
	    }
	if (objGetAttrValue(w_obj,"datafocus2",POD(&ptr)) == 0)
	    {
	    memccpy(dtfocus2,ptr,0,63);
	    kbfocus1[63]=0;
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
		"    </STYLE>\n" );
	sprintf(sbuf, "<DIV ID=\"pgtop\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n");
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "<DIV ID=\"pgbtm\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n");
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "<DIV ID=\"pgrgt\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n");
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "<DIV ID=\"pglft\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n");
	htrAddBodyItem(s, sbuf);
	sprintf(sbuf, "<DIV ID=\"pgtvl\"></DIV>\n");
	htrAddBodyItem(s, sbuf);
	htrAddBodyItem(s,
		"<DIV ID=\"pgktop\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n"
		"<DIV ID=\"pgkbtm\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1152 HEIGHT=1></DIV>\n"
		"<DIV ID=\"pgkrgt\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n"
		"<DIV ID=\"pgklft\"><IMG SRC=/sys/images/trans_1.gif WIDTH=1 HEIGHT=864></DIV>\n");
	
	/** Add event code to handle mouse in/out of the area.... **/
	htrAddEventHandler(s, "document", "MOUSEMOVE","pg",
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
		"    if (e.target == pg_curlayer) pg_curlayer = null;\n"
		"    if (e.target != null && pg_curarea != null && e.target == pg_curarea.layer)\n"
		"        {\n"
		"        pg_hidebox(document.layers.pgtop,document.layers.pgbtm,document.layers.pgrgt,document.layers.pglft);\n"
		"        pg_curarea = null;\n"
		"        }\n" );
	htrAddEventHandler(s, "document", "MOUSEOVER", "pg",
		"    if (e.target != null && e.target.pageX != null)\n"
		"        {\n"
		"        pg_curlayer = e.target;\n"
		"        }\n" );

	/** CLICK event handler is for making mouse focus the keyboard focus **/
	htrAddEventHandler(s, "document", "MOUSEDOWN", "pg",
		"    if (pg_curarea != null)\n"
		"        {\n"
		"        pg_curkbdarea = pg_curarea;\n"
		"        pg_curkbdlayer = pg_curlayer;\n"
		"        x = pg_curarea.layer.pageX+pg_curarea.x;\n"
		"        y = pg_curarea.layer.pageY+pg_curarea.y;\n"
		"        w = pg_curarea.width;\n"
		"        h = pg_curarea.height;\n"
		"        if (pg_curarea.callback)\n"
		"            {\n"
		"            v=pg_curarea.callback(pg_curarea.layer,pg_curarea.cls,pg_curarea.name);\n"
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
		"        }\n");

	/** This resets the keyboard focus. **/
	htrAddEventHandler(s, "document", "MOUSEUP", "pg",
		"    setTimeout('document.layers.pginpt.document.tmpform.x.focus()',10);\n");

	/** Set colors for the focus layers **/
	sprintf(sbuf, "    page.kbcolor1 = '%s';\n    page.kbcolor2 = '%s';\n",kbfocus1,kbfocus2);
	htrAddScriptInit(s, sbuf);
	sprintf(sbuf, "    page.mscolor1 = '%s';\n    page.mscolor2 = '%s';\n",msfocus1,msfocus2);
	htrAddScriptInit(s, sbuf);
	sprintf(sbuf, "    page.dtcolor1 = '%s';\n    page.dtcolor2 = '%s';\n",dtfocus1,dtfocus2);
	htrAddScriptInit(s, sbuf);
	htrAddScriptInit(s, "    document.LSParent = null;\n");

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
		"    rl.resizeTo(1,h);\n"
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
		"function pg_area(pl,x,y,w,h,cls,nm,ckfn)\n"
		"    {\n"
		"    this.layer = pl;\n"
		"    this.x = x;\n"
		"    this.y = y;\n"
		"    this.width = w;\n"
		"    this.height = h;\n"
		"    this.callback = ckfn;\n"
		"    this.name = nm;\n"
		"    this.cls = cls;\n"
		"    return this;\n"
		"    }\n", 0);

	/** Function to add a new area to the arealist **/
	htrAddScriptFunction(s, "pg_addarea", "\n"
		"function pg_addarea(pl,x,y,w,h,cls,nm,ckfn)\n"
		"    {\n"
		"    a = new pg_area(pl,x,y,w,h,cls,nm,ckfn);\n"
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
		"    for(i=0;i<l.document.layers.length;i++)\n"
		"        {\n"
		"        cl = l.document.layers[i];\n"
		"        if ((cl.visibility == 'show' || cl.visibility == 'inherit') && cl.y + cl.clip.height > maxheight)\n"
		"            maxheight = cl.y + cl.clip.height;\n"
		"        }\n"
		"    if (l!=window) l.clip.height = maxheight;\n"
		"    else l.document.height = maxheight;\n"
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
		"    if (pg_curlayer != null && pg_curlayer.cursorlayer != null)\n"
		"        {\n"
		"        if (pg_curlayer.cursorlayer.visibility != 'inherit')\n"
		"            pg_curlayer.cursorlayer.visibility = 'inherit';\n"
		"        else\n"
		"            pg_curlayer.cursorlayer.visibility = 'hidden';\n"
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
	htrAddEventHandler(s, "document", "KEYPRESS", "pg",
		"    k = e.which;\n"
		"    if (k > 65280) k -= 65280;\n"
		"    if (k >= 128) k -= 128;\n"
		"    if (k == pg_lastkey) return false;\n"
		"    pg_lastkey = k;\n"
		"    pg_togglecursor();\n"
		"    alert('Code=' + k + '; Mod=' + e.modifiers);\n"
		"    setTimeout('pg_lastkey = -1;',100);\n"
		"    if (pg_curkbdlayer != null && pg_curkbdlayer.keyhandler != null && pg_curkbdlayer.keyhandler(e,k) == true) return false;\n"
		"    for(i=0;i<pg_keylist.length;i++)\n"
		"        {\n"
		"        if (k >= pg_keylist[i].startcode && k <= pg_keylist[i].endcode && (pg_keylist[i].kbdlayer == null || pg_keylist[i].kbdlayer == pg_curkbdlayer) && (pg_keylist[i].mouselayer == null || pg_keylist[i].mouselayer == pg_curlayer) && (e.modifiers & pg_keylist[i].modmask) == pg_keylist[i].mod)\n"
		"            {\n"
		"            pg_keylist[i].aparam.KeyCode = k;\n"
		"            pg_keylist[i].target_obj[pg_keylist[i].fnname](pg_keylist[i].aparam);\n"
		"            return false;\n"
		"            }\n"
		"        }\n"
		"    return false;\n");
	htrAddScriptInit(s,
		"    document.layers.pginpt.moveTo(window.innerWidth-2, 20);\n"
		"    document.layers.pginpt.visibility = 'inherit';\n"
		"    setTimeout('document.layers.pginpt.document.tmpform.x.focus()',100);\n");
	htrAddBodyItem(s, "<DIV ID=pginpt><FORM name=tmpform action><textarea name=x tabindex=1 rows=1></textarea></FORM></DIV>\n");

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
