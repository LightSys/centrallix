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
/* Module: 	htdrv_alert.c      					*/
/* Author:	Jonathan Rupp (JDR)		 			*/
/* Creation:	February 23, 2002 					*/
/* Description:	This is a very simple widget that will give the user  	*/
/*		a message.						*/
/************************************************************************/

/**CVSDATA***************************************************************
 
    $Log: htdrv_alerter.c,v $
    Revision 1.6  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.5  2002/03/12 04:01:48  jorupp
    * started work on a tree-view interface to the javascript DOM
         didn't go so well...

    Revision 1.4  2002/03/09 20:24:10  jorupp
    * modified ActionViewDOM to handle recursion and functions better
    * removed some redundant code

    Revision 1.3  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.2  2002/03/09 06:39:14  jorupp
    * Added ViewDOM action to alerter
        pass object to display as parameter

    Revision 1.1  2002/03/08 02:07:13  jorupp
    * initial commit of alerter widget
    * build callback listing object for form
    * form has many more of it's callbacks working


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTALRT;


/*** htalrtVerify - not written yet.
 ***/
int
htalrtVerify()
    {
    return 0;
    }


/*** htalrtRender - generate the HTML code for the alert -- not much..
 ***/
int
htalrtRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    //char sbuf[200];
    //char sbuf2[160];
    char *sbuf3;
    int id;
    char* nptr;
    
    	/** Get an id for this. **/
	id = (HTALRT.idcnt++);

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = '\0';

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);

	htrAddScriptGlobal(s, nptr, "null",HTR_F_NAMEALLOC); /* create our instance variable */

	htrAddScriptFunction(s, "alrt_action_alert", "\n"
		"function alrt_action_alert(sendthis)\n"
		"    {\n"
		"    window.alert(sendthis[\"param\"]);\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "alrt_action_confirm", "\n"
		"function alrt_action_confirm(sendthis)\n"
		"    {\n"
		"    window.confirm(sendthis[\"param\"]);\n"
		"    }\n", 0);
#if 0
	Example Connector parameters for using ActionViewDOM
	    event="Click";
	    target="alerter";
	    action="ConfirmDOM";
	    param="eval(prompt(\"variable\",\"navBtn1\"))";
		or
	    param="(navBtn1)";

	note that global variables can be used (second example),
	  but a paren must be present somewhere in the call
#endif

	htrAddScriptFunction(s, "alrt_action_view_DOM", "\n"
		"function alrt_action_view_DOM(param)\n"
		"    {\n"
		"    var walkthis=param[\"param\"];\n"
		/*
		"    var funcbehav;\n"
		"    if(typeof(param[\"param\"])==\"array\")\n"
		"        {\n"
		"        walkthis=param[\"param\"][0];\n"
		"        funcbehav=param[\"param\"][1];\n"
		"        }\n"
		"    else\n"
		"        {\n"
		"        walkthis=param[\"param\"];\n"
		"        funcbehav=1;\n"
		"        }\n"
		*/
		"    alrt_view_DOM(walkthis);\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "alrt_view_DOM", "\n"
		"function alrt_view_DOM(walkthis)\n"
		"    {\n"
		"    var DOM=alrt_walk_DOM(walkthis,\"Base Object\",typeof(walkthis),0);\n"
		"    var win=window.open();\n"
		"    win.document.write(\"<PRE>\"+DOM+\"</PRE>\\n\");\n"
		"    win.document.close();\n"
		"    delete DOM\n"
		"    }\n", 0);

	  
	htrAddScriptFunction(s, "alrt_walk_DOM", "\n"
		"function alrt_walk_DOM(walkthis,name,type,depth)\n"
		"    {\n"
		"    if(depth>=7)\n" /* no inf loop */
		"        return \"\";\n"
		"    if(type==\"function\")\n"
		"        {\n"
		"        var fname=((/function([^(]*)/).exec(walkthis));\n"
		"        if(fname && fname[1])\n"
		"            {\n"
		"            return \"<LI>\"+name+\" (\"+type+\"): <b>\"+fname[1]+\"</b></LI>\\n\"\n"
		"            }\n"
		/*
		"        if(funcbehav==1)\n"
		"            {\n"
		"            return \"<LI>\"+name+\" (\"+type+\"): <small>\"+walkthis+\"</small></LI>\\n\"\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            var fname=String(walkthis).exec(/function ([^(]) ?/);\n"
		"            confirm(fname);\n"
		"            return \"<LI>\"+name+\" (\"+type+\"): \"+fname[1]+\"</LI>\\n\"\n"
		"            }\n"
		*/
		"        }\n"
		"    if(type!=\"object\" && type!=\"array\")\n"
		"        {\n"
		"        return \"<LI>\"+name+\" (\"+type+\"): \"+walkthis+\"</LI>\\n\";\n"
		"        }\n"
		"    var ret=\"<LI>\"+name+\": </LI>\\n<UL>\\n\";\n"
		"    for(var i in walkthis)\n"
		"        {\n"
		"        if(\n"	/* don't walk into the following..... */
		"            ((/parent/i).test(i)) || \n"
		"            ((/form/i).test(i)) || \n"
		"            ((/document/i).test(i)) || \n"
		"            ((/osrc/i).test(i)) || \n"
		"            ((/above/i).test(i)) || \n"
		"            ((/below/i).test(i)) || \n"
		"            ((/window/i).test(i)) )\n"
		"            {\n"
		"            if(walkthis[i] && walkthis[i].name)\n"
		"                {\n"
		"                ret+=\"<LI>\"+i+\" (\"+type+\"): <b>\"+walkthis[i].name+\"</b> <small>(recursive)</small></LI>\\n\";\n"
		"                }\n"
		"            else\n"
		"                {\n"
		"                ret+=\"<LI>\"+i+\" (\"+type+\"): <b>\"+walkthis[i]+\"</b></LI>\\n\";\n"
		"                }\n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            ret+=alrt_walk_DOM(walkthis[i],i,typeof(walkthis[i]),depth+1);\n"
		"            }\n"
		"        }\n"
		"    ret+=\"</UL>\\n\";\n"
		"    return ret;\n"
		"    }\n", 0);
#if 0
		"function alrt_search(arr,obj)\n"
		"    {\n"
		"    for(var i in arr)\n"
		"        {\n"
		"        if(arr[i]==obj)\n"
		"            return 1;\n"
		"        }\n"
		"    return 0;\n"
		"    }\n", 0);
#endif



/***  ---- Jonathan Rupp - 3/11/02 ----
 *** the tree_DOM functions are my attempt to get a tree-view type
 *** display of the Javascript DOM.  Needless to say, it didn't go
 *** so well.  I tried to get it working, but it only partially works
 *** (when you expand a list, it doesn't adjust the others, and 
 *** you can't un-expand a list)
 ***/
	htrAddScriptFunction(s, "alrt_action_view_tree_DOM", "\n"
		"function alrt_action_view_tree_DOM(param)\n"
		"    {\n"
		"    var viewthis=param[\"param\"];\n"
		"    alrt_view_tree_DOM(viewthis);\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "alrt_tree_DOM_compress", "\n"
		"function alrt_tree_DOM_compress(s,l)\n"
		"    {\n"
		"    if(l.document.layers.length)\n"
		"        {\n"
		"        for(var i=l.document.layers.length-1;i>-1;i--)\n"
		"            {\n"
		"            s(s,l.document.layers[i]);\n"
		"            }\n"
		"        }\n"
		"    l.clip.height=0;\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "alrt_tree_DOM_onMD", "\n"
		"function alrt_tree_DOM_onMD()\n"
		"    {\n"
		"    pl=this.layer;\n"
		"    //if(!pl) pl=this;\n"
		"    if(pl.isExpanded)\n"
		"        {\n"	/* already expanded, compress */
		"        pl.window.confirm(\"Compressing at level \"+pl.level);\n"
		"        pl.window.compress(pl.window.compress,pl);\n"
		"        pl.window.pg_resize(pl);\n"
		"        }\n"
		"    else\n"
		"        {\n"	/* expand it */
		"        var top=pl.top+pl.clip.height;\n"
		"        for(var i in pl.obj)\n"
		"            {\n"
		"            l=new Layer(1024,pl);\n"
		"            l.isExpanded=false;\n"
		"            l.level=pl.level+1;\n"
		"            l.root=pl.root;\n"
		"            l.obj=pl.obj[i];\n"
		"            l.objname=i;\n"
		"            l.window=pl.window;\n"
		"            l.bgColor=pl.document.bgColor;\n"
		"            l.top=top;\n"
		"            l.left=pl.left+20;\n"
		
		"            l2=new Layer(1024,l);\n"
		"            l2.bgColor=l.bgColor;\n"
		"            l2.layer=l;\n"
		
		"            switch(typeof(l.obj))\n"
		"                {\n"
		"                case \"object\":\n"
		"                case \"array\":\n"
		"                case \"function\":\n"
		"                    l2.document.write(l.objname+\" (\"+typeof(l.obj)+\"):\");\n"
		"                    l2.captureEvents(Event.MOUSEDOWN);\n"
		"                    l2.onMouseDown=l.window.treeonMD;\n"
		"                    break;\n"
		"                default:\n"
		"                    l2.document.write(l.objname+\" (\"+typeof(l.obj)+\"):\"+l.obj);\n"
		"                    break;\n"
		"                }\n"
		"            \n"
		"            l2.document.close();\n"
		"            \n"
		"            top+=l2.clip.height;\n"
		"            l.visibility='inherit';\n"
		"            l2.visibility='inherit';\n"
		"            pl.window.pg_resize(l);\n"
		"            }\n"
		"        pl.window.pg_resize(pl);\n"
		"        pl.isExpanded=true;\n"
		"        }\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "alrt_view_tree_DOM", "\n"
		"function alrt_view_tree_DOM(viewthis)\n"
		"    {\n"
		"    var win=window.open();\n"
		"    win.objtoview=navBtn1;\n"
		"    win.buildbasetree=alrt_build_base_tree_DOM;\n"
		"    win.treeonMD=alrt_tree_DOM_onMD;\n"
		"    win.compress=alrt_tree_DOM_compress;\n"
		"    win.pg_resize=pg_resize;\n"
		"    win.document.write(\"<html>\\n\");\n"
		"    win.document.write(\"<head>\\n\");\n"
		"    win.document.write(\"  <STYLE TYPE=\\\"text/css\\\">\\n\");\n"
		"    win.document.write(\"    dr { POSITION: absolute; VISIBILITY:inherit; TOP:0; LEFT:0; HEIGHT:1000; WIDTH:800; }\\n\");\n"
		"    win.document.write(\"  </STYLE>\\n\");\n"
		"    win.document.write(\"</head>\\n\");\n"
		"    win.document.write(\"<body onload=\\\"window.buildbasetree();\\\">\\n\");\n"
		"    \n" /* I don't know why, but I have to use LAYER here... */
		"    win.document.write(\"<LAYER ID=\\\"dr\\\">\");\n"
		"    win.document.write(\"  <LAYER ID=\\\"data\\\">\");\n"
		"    if(win.objtoview && win.objtoview.name)\n"
		"        {\n"
		"        win.document.write(win.objtoview.name+\" (\"+typeof(win.objtoview)+\"):\");\n"
		"        }\n"
		"    else\n"
		"        {\n"
		"        win.document.write(\"Base Object (\"+typeof(win.objtoview)+\"):\");\n"
		"        }\n"
		"    win.document.write(\"  </LAYER>\\n\");\n"
		"    win.document.write(\"</LAYER>\\n\");\n"
		"    \n"
		"    win.document.write(\"</body>\\n\");\n"
		"    win.document.write(\"</html>\");\n"
		"    win.document.close();\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "alrt_build_base_tree_DOM", "\n"
		"function alrt_build_base_tree_DOM()\n"
		"    {\n"
		"    l=this.document.dr;\n"
		"    //alrt_view_DOM(l);\n"
		"    l.isExpanded=false;\n"
		"    l.level=0;\n"
		"    l.root=l;\n"
		"    l.obj=this.objtoview\n"
		"    l.window=this;\n"
		"    l.children=null;\n"
		"    l.bgColor=this.document.bgColor;\n"
		"    var l2=l.document.data;\n"
		"    l2.layer=l;\n"
		"    l2.captureEvents(Event.MOUSEDOWN);\n"
		"    l2.onMouseDown=l.window.treeonMD;\n"
		"    l.visibility='inherit';\n"
		"    }\n", 0);
	
	/** Alert initializer **/
	htrAddScriptFunction(s, "alrt_init", "\n"
		"function alrt_init()\n"
		"    {\n"
		"    alrt = new Object();\n"
		"    alrt.ActionAlert = alrt_action_alert;\n"
		"    alrt.ActionConfirm = alrt_action_confirm;\n"
		"    alrt.ActionViewDOM = alrt_action_view_DOM;\n"
		"    alrt.ActionViewTreeDOM = alrt_action_view_tree_DOM;\n"
		"    return alrt;\n"
		"    }\n",0);

	sbuf3 = nmMalloc(200);
	snprintf(sbuf3,200,"    %s=alrt_init();\n",name);
	htrAddScriptInit(s,sbuf3);
	nmFree(sbuf3,200);

    return 0;
    }


/*** htalrtInitialize - register with the ht_render module.
 ***/
int
htalrtInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Alert Widget");
	strcpy(drv->WidgetName,"alerter");
	drv->Render = htalrtRender;
	drv->Verify = htalrtVerify;
    strcpy(drv->Target, "Netscape47x:default");

	/** Add a 'executemethod' action **/
	htrAddAction(drv,"Alert");
	htrAddParam(drv,"Alert","Parameter",DATA_T_STRING);
	htrAddAction(drv,"Confirm");
	htrAddParam(drv,"Confirm","Parameter",DATA_T_STRING);
	htrAddAction(drv,"ViewDOM");
	htrAddParam(drv,"ViewDOM","Paramater",DATA_T_STRING);


	/** Register. **/
	htrRegisterDriver(drv);

	HTALRT.idcnt = 0;

    return 0;
    }
