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
/* Copyright (C) 2000-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	htdrv_remotectl.c        				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	June 5th, 2000   					*/
/* Description:	Widget driver for the remote control "server" side,	*/
/*		which accepts target/action/parameter sequences from	*/
/*		the HTTP net driver over a given channel ID.  Channel	*/
/*		ID must be specified.  The "remotemgr" widget is used	*/
/*		to send requests through a channel to this "remotectl"	*/
/*		widget.							*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_remotectl.c,v 1.3 2002/06/09 23:44:46 nehresma Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/Attic/htdrv_remotectl.c,v $

    $Log: htdrv_remotectl.c,v $
    Revision 1.3  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.2  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.1.1.1  2001/08/13 18:00:50  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:57  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTRMT;


/*** htrmtVerify - not written yet.
 ***/
int
htrmtVerify()
    {
    return 0;
    }


/*** htrmtRender - generate the HTML code for the page.
 ***/
int
htrmtRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[320];
    char svr[128] = "";
    pObject sub_w_obj;
    pObjQuery qy;
    int id,cnt;
    char* nptr;
    unsigned int ch;

    	/** Get an id for this. **/
	id = (HTRMT.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"channel",POD(&ch)) != 0) 
	    {
	    mssError(1,"HTRMT","RemoteCtl widget must have a 'channel' property");
	    return -1;
	    }

	/** Get source html objectsystem entry. **/
	if (objGetAttrValue(w_obj,"server",POD(&ptr)) == 0)
	    {
	    snprintf(svr,128,"%s",ptr);
	    }

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	snprintf(name,64,"%s",ptr);

	/** Invisible loader layer to get cmds from the channel on centrallix **/
	snprintf(sbuf,320,"    <STYLE TYPE=\"text/css\">\n");
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,320,"\t#rc%dload { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:0; WIDTH:1; Z-INDEX:0; }\n",id);
	htrAddHeaderItem(s,sbuf);
	snprintf(sbuf,320,"    </STYLE>\n");
	htrAddHeaderItem(s,sbuf);

        /** Write named global **/
        nptr = (char*)nmMalloc(strlen(name)+1);
        strcpy(nptr,name);
        htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Function that gets run when we got a block of cmds from the channel **/
	htrAddScriptFunction(s, "rc_loaded", "\n"
		"function rc_loaded()\n"
		"    {\n"
		"    }\n", 0);
    
	/** Function to initialize the remote ctl server **/
	htrAddScriptFunction(s, "rc_init", "\n"
		"function rc_init(l,c,s)\n"
		"    {\n"
		"    l.channel = c;\n"
		"    l.server = s;\n"
		"    l.address = 'http://' + s + '/?ls__mode=readchannel&ls__channel=' + c;\n"
		"    l.onload = rc_loaded;\n"
		"    l.source = l.address;\n"
		"    return l;\n"
		"    }\n", 0);

            /** This function handles the 'LoadPage' action **/
            htrAddScriptFunction(s, "ht_loadpage", "\n"
                    "function ht_loadpage(aparam)\n"
                    "    {\n"
                    "    this.source = aparam.Source;\n"
                    "    }\n", 0);
    
            /** This function gets run when the user assigns to the 'source' property **/
            htrAddScriptFunction(s, "ht_sourcechanged", "\n"
                    "function ht_sourcechanged(prop,oldval,newval)\n"
                    "    {\n"
                    "    if (newval.substr(0,5)=='http:')\n"
                    "        {\n"
		    "        tmpl = this.curLayer;\n"
		    "        tmpl.visibility = 'hidden';\n"
		    "        this.curLayer = this.altLayer;\n"
		    "        this.altLayer = tmpl;\n"
                    "        this.curLayer.onload = ht_reloaded;\n"
                    "        this.curLayer.bgColor = null;\n"
                    "        this.curLayer.load(newval,this.clip.width);\n"
                    "        }\n"
                    "    return newval;\n"
                    "    }\n", 0);
    
            /** This function is called when the layer is reloaded. **/
            htrAddScriptFunction(s, "ht_reloaded", "\n"
                    "function ht_reloaded(e)\n"
                    "    {\n"
                    "    e.target.mainLayer.watch('source',ht_sourcechanged);\n"
                    "    e.target.clip.height = e.target.document.height;\n"
		    "    e.target.visibility = 'inherit';\n"
                    /*"    e.target.document.captureEvents(Event.CLICK);\n"
                    "    e.target.document.onclick = ht_click;\n"*/
                    "    for(i=0;i<e.target.document.links.length;i++)\n"
                    "        {\n"
                    "        e.target.document.links[i].layer = e.target.mainLayer;\n"
		    "        e.target.document.links[i].kind = 'ht';\n"
                    "        }\n"
                    "    pg_resize(e.target.mainLayer.parentLayer);\n"
                    "    }\n", 0);
    
            /** This function is called when the user clicks on a link in the html pane **/
            htrAddScriptFunction(s, "ht_click", "\n"
                    "function ht_click(e)\n"
                    "    {\n"
		    "    e.target.layer.source = e.target.href;\n"
                    "    return false;\n"
                    "    }\n", 0);
    
            /** Our initialization processor function. **/
            htrAddScriptFunction(s, "ht_init", "\n"
                    "function ht_init(l,l2,source,pdoc,w,h,p)\n"
                    "    {\n"
		    "    l.mainLayer = l;\n"
		    "    l2.mainLayer = l;\n"
		    "    l.curLayer = l;\n"
		    "    l.altLayer = l2;\n"
                    "    l.LSParent = p;\n"
                    "    l.kind = 'ht';\n"
                    "    l2.kind = 'ht';\n"
                    "    l.pdoc = pdoc;\n"
                    "    l2.pdoc = pdoc;\n"
                    "    if (h != -1)\n"
		    "        {\n"
		    "        l.clip.height = h;\n"
		    "        l2.clip.height = h;\n"
		    "        }\n"
                    "    if (w != -1)\n"
		    "        {\n"
		    "        l.clip.width = w;\n"
		    "        l2.clip.width = w;\n"
		    "        }\n"
                    "    if (source.substr(0,5) == 'http:')\n"
                    "        {\n"
                    "        l.onload = ht_reloaded;\n"
                    "        l.load(source,w);\n"
                    "        }\n"
                    "    l.source = source;\n"
                    "    l.watch('source', ht_sourcechanged);\n"
                    "    l.ActionLoadPage = ht_loadpage;\n"
		    "    l.document.Layer = l;\n"
		    "    l2.document.Layer = l2;\n"
                    "    }\n" ,0);

	    /** Event handler for click-on-link. **/
	    htrAddEventHandler(s, "document","CLICK","ht",
	    	    "    if (e.target != null && e.target.kind == 'ht')\n"
		    "        {\n"
		    "        return ht_click(e);\n"
		    "        }\n");
    
            /** Script initialization call. **/
            snprintf(sbuf,320,"    ht_init(%s.layers.ht%dpane,%s.layers.ht%dpane2,\"%s\",%s,%d,%d,%s);\n",
                    parentname, id, parentname, id, src, parentname, w,h, parentobj);
            htrAddScriptInit(s, sbuf);
            snprintf(sbuf,320,"    %s = %s.layers.ht%dpane;\n",nptr,parentname,id);
            htrAddScriptInit(s, sbuf);
    
            /** HTML body <DIV> element for the layer. **/
            snprintf(sbuf,320,"<DIV ID=\"ht%dpane2\"></DIV><DIV ID=\"ht%dpane\">\n",id,id);
            htrAddBodyItem(s, sbuf);
	    }

        /** If prefix text given, put it. **/
        if (objGetAttrValue(w_obj, "prologue", POD(&ptr)) == 0)
            {
            htrAddBodyItem(s, ptr);
            }

        /** If full text given, put it. **/
        if (objGetAttrValue(w_obj, "content", POD(&ptr)) == 0)
            {
            htrAddBodyItem(s, ptr);
            }

        /** If source is an objectsystem entry... **/
        if (src[0] && strncmp(src,"http:",5))
            {
            content_obj = objOpen(w_obj->Session,src,O_RDONLY,0600,"text/html");
            if (content_obj)
                {
                while((cnt = objRead(content_obj, sbuf, 159,0,0)) > 0)
                    {
                    sbuf[cnt]=0;
                    htrAddBodyItem(s, sbuf);
                    }
                objClose(content_obj);
                }
            }

        /** If post text given, put it. **/
        if (objGetAttrValue(w_obj, "epilogue", POD(&ptr)) == 0)
            {
            htrAddBodyItem(s, ptr);
            }

        /** Check for more sub-widgets within the html entity. **/
	if (mode == 1)
	    {
            snprintf(sbuf,320,"%s.document",nptr,id);
	    }
	else
	    {
	    memccpy(sbuf,parentname,0,319);
	    sbuf[319]=0;
	    nptr = parentobj;
	    }
        qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
        if (qy)
            {
            while(sub_w_obj = objQueryFetch(qy, O_RDONLY))
                {
                htrRenderWidget(s, sub_w_obj, z+2, sbuf, nptr);
                objClose(sub_w_obj);
                }
            objQueryClose(qy);
            }

        /** End the containing layer. **/
        if (mode == 1) htrAddBodyItem(s, "</DIV>\n");

    return 0;
    }


/*** htrmtInitialize - register with the ht_render module.
 ***/
int
htrmtInitialize()
    {
    pHtDriver drv;
    pHtEventAction action;
    pHtParam param;

        /** Allocate the driver **/
        drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
        if (!drv) return -1;

        /** Fill in the structure. **/
        strcpy(drv->Name,"DHTML remote control driver");
        strcpy(drv->WidgetName,"remotectl");
        drv->Render = htrmtRender;
        drv->Verify = htrmtVerify;
        xaInit(&(drv->PosParams),16);
        xaInit(&(drv->Properties),16);
        xaInit(&(drv->Events),16);
        xaInit(&(drv->Actions),16);
	strcpy(drv->Target, "Netscape47x:default");

        /** Add the 'load page' action **/
	/**
        action = (pHtEventAction)nmSysMalloc(sizeof(HtEventAction));
        strcpy(action->Name,"LoadPage");
        xaInit(&action->Parameters,16);
        param = (pHtParam)nmSysMalloc(sizeof(HtParam));
        strcpy(param->ParamName,"Source");
        param->DataType = DATA_T_STRING;
        xaAddItem(&action->Parameters,(void*)param);
        xaAddItem(&drv->Actions,(void*)action);
	**/

        /** Register. **/
        htrRegisterDriver(drv);

        HTRMT.idcnt = 0;

    return 0;
    }
