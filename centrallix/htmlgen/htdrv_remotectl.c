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

    $Id: htdrv_remotectl.c,v 1.8 2002/12/04 00:19:11 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/Attic/htdrv_remotectl.c,v $

    $Log: htdrv_remotectl.c,v $
    Revision 1.8  2002/12/04 00:19:11  gbeeley
    Did some cleanup on the user agent selection mechanism, moving to a
    bitmask so that drivers don't have to register twice.  Theme will be
    handled differently, but provision is made for 'classes' of widgets
    such as dhtml vs. xul.  Started work on some utility functions to
    resolve some ns47 vs. w3c issues.

    Revision 1.7  2002/07/25 18:08:36  mcancel
    Taking out the htrAddScriptFunctions out... moving the javascript code out of the c file into the js files and a little cleaning up... taking out whole deleted functions in a few and found another htrAddHeaderItem that needed to be htrAddStylesheetItem.

    Revision 1.6  2002/07/19 21:17:49  mcancel
    Changed widget driver allocation to use the nifty function htrAllocDriver instead of calling nmMalloc.

    Revision 1.5  2002/07/16 18:23:20  lkehresman
    Added htrAddStylesheetItem() function to help consolidate the output of
    the html generator.  Now, all stylesheet definitions are included in the
    same <style></style> tags rather than each widget having their own.  I
    have modified the current widgets to take advantage of this.  In the
    future, do not use htrAddHeaderItem(), but use this new function.

    NOTE:  There is also a htrAddStylesheetItem_va() function if you need it.

    Revision 1.4  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

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
	htrAddStylesheetItem_va(s,"\t#rc%dload { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:0; WIDTH:1; Z-INDEX:0; }\n",id);

        /** Write named global **/
        nptr = (char*)nmMalloc(strlen(name)+1);
        strcpy(nptr,name);
        htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);

	/** Function that gets run when we got a block of cmds from the channel **/
    

	    /** Event handler for click-on-link. **/
	    htrAddEventHandler(s, "document","CLICK","ht",
	    	    "    if (e.target != null && e.target.kind == 'ht')\n"
		    "        {\n"
		    "        return ht_click(e);\n"
		    "        }\n");
    
            /** Script initialization call. **/
            htrAddScriptInit_va(s,"    ht_init(%s.layers.ht%dpane,%s.layers.ht%dpane2,\"%s\",%s,%d,%d,%s);\n",
                    parentname, id, parentname, id, src, parentname, w,h, parentobj);
            htrAddScriptInit_va(s,"    %s = %s.layers.ht%dpane;\n",nptr,parentname,id);
    
            /** HTML body <DIV> element for the layer. **/
            htrAddBodyItem_va(s,"<DIV ID=\"ht%dpane2\"></DIV><DIV ID=\"ht%dpane\">\n",id,id);
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

        /** Allocate the driver **/
        drv = htrAllocDriver();
        if (!drv) return -1;

        /** Fill in the structure. **/
        strcpy(drv->Name,"DHTML remote control driver");
        strcpy(drv->WidgetName,"remotectl");
        drv->Render = htrmtRender;
        drv->Verify = htrmtVerify;
        htrAddSupport(drv, HTR_UA_NETSCAPE_47);

        /** Register. **/
        htrRegisterDriver(drv);

        HTRMT.idcnt = 0;

    return 0;
    }
