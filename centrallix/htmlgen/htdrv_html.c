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
/* Module: 	htdrv_html.c        					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 3, 1998 					*/
/* Description:	HTML Widget driver for HTML text, either given as an	*/
/*		immediate string or pulled from an ObjectSystem source	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_html.c,v 1.8 2002/07/16 17:52:00 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_html.c,v $

    $Log: htdrv_html.c,v $
    Revision 1.8  2002/07/16 17:52:00  lkehresman
    Updated widget drivers to use include files

    Revision 1.7  2002/06/19 23:29:33  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.6  2002/06/19 19:08:55  lkehresman
    Changed all snprintf to use the *_va functions

    Revision 1.5  2002/06/19 16:31:04  lkehresman
    * Changed snprintf to *_va functions in several places
    * Allow fading to both static and dynamic pages

    Revision 1.4  2002/06/09 23:44:46  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.3  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.2  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.1.1.1  2001/08/13 18:00:49  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:54  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTHTML;


/*** hthtmlVerify - not written yet.
 ***/
int
hthtmlVerify()
    {
    return 0;
    }


/*** hthtmlRender - generate the HTML code for the page.
 ***/
int
hthtmlRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char sbuf[320];
    char src[128] = "";
    pObject sub_w_obj;
    pObjQuery qy;
    int x=-1,y=-1,w,h;
    int id,cnt;
    int mode = 0;
    char* nptr = NULL;
    pObject content_obj;

    	/** Get an id for this. **/
	id = (HTHTML.idcnt++);

    	/** Get x,y,w,h of this object **/
	if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=-1;
	if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=-1;
	if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) 
	    {
	    mssError(1,"HTHTML","HTML widget must have a 'width' property");
	    return -1;
	    }
	if (objGetAttrValue(w_obj,"height",POD(&h)) != 0) h = -1;

	/** Get source html objectsystem entry. **/
	if (objGetAttrValue(w_obj,"source",POD(&ptr)) == 0)
	    {
	    memccpy(src,ptr,0,127);
	    src[127] = 0;
	    }

	/** Check for a 'mode' - dynamic or static.  Default is static. **/
	if (objGetAttrValue(w_obj,"mode",POD(&ptr)) == 0 && !strcmp(ptr,"dynamic")) mode = 1;

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63]=0;

	/** Ok, write the style header items. **/
	if (mode == 1)
	    {
	    htrAddHeaderItem_va(s,"    <STYLE TYPE=\"text/css\">\n");
	
	    /** Only give x and y if supplied. **/
	    if (x==-1 || y==-1)
	        {
	        htrAddHeaderItem_va(s,"\t#ht%dpane { POSITION:relative; VISIBILITY:inherit; WIDTH:%d; Z-INDEX:%d; }\n",id,w,z);
	        htrAddHeaderItem_va(s,"\t#ht%dpane2 { POSITION:relative; VISIBILITY:hidden; WIDTH:%d; Z-INDEX:%d; }\n",id,w,z);
	        htrAddHeaderItem_va(s,"\t#ht%dfader { POSITION:relative; VISIBILITY:hidden; WIDTH:%d; Z-INDEX:%d; }\n",id,w,z+1);
	        }
	    else
	        {
	        htrAddHeaderItem_va(s,"\t#ht%dpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	        htrAddHeaderItem_va(s,"\t#ht%dpane2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	        htrAddHeaderItem_va(s,"\t#ht%dfader { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z+1);
	        }
	    htrAddHeaderItem_va(s,"    </STYLE>\n");

            /** Write named global **/
            nptr = (char*)nmMalloc(strlen(name)+1);
            strcpy(nptr,name);
            htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);
            htrAddScriptGlobal(s, "ht_fadeobj", "null", 0);
    
	    htrAddScriptInclude(s, "/sys/js/htdrv_html.js", 0);

	    /** Event handler for click-on-link. **/
	    htrAddEventHandler(s, "document","CLICK","ht",
	    	    "    if (e.target != null && e.target.kind == 'ht')\n"
		    "        {\n"
		    "        return ht_click(e);\n"
		    "        }\n");
    
            /** Script initialization call. **/
            htrAddScriptInit_va(s,"    ht_init(%s.layers.ht%dpane,%s.layers.ht%dpane2,%s.layers.ht%dfader,\"%s\",%s,%d,%d,%s);\n",
                    parentname, id, parentname, id, parentname, id, src, parentname, w,h, parentobj);
            htrAddScriptInit_va(s,"    %s = %s.layers.ht%dpane;\n",nptr,parentname,id);
    
            /** HTML body <DIV> element for the layer. **/
            htrAddBodyItem_va(s,"<DIV background=\"/sys/images/fade_lrwipe_01.gif\" ID=\"ht%dfader\"></DIV><DIV ID=\"ht%dpane2\"></DIV><DIV ID=\"ht%dpane\">\n",id,id,id);
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
            /*sprintf(sbuf,"%s.document",nptr,id);*/
            snprintf(sbuf,320,"%s.document",nptr);
	    }
	else
	    {
	    memccpy(sbuf,parentname,0,319);
	    sbuf[319] = '\0';
	    nptr = parentobj;
	    }
        qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
        if (qy)
            {
            while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
                {
                htrRenderWidget(s, sub_w_obj, z+3, sbuf, nptr);
                objClose(sub_w_obj);
                }
            objQueryClose(qy);
            }

        /** End the containing layer. **/
        if (mode == 1) htrAddBodyItem(s, "</DIV>\n");

    return 0;
    }


/*** hthtmlInitialize - register with the ht_render module.
 ***/
int
hthtmlInitialize()
    {
    pHtDriver drv;

        /** Allocate the driver **/
        drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
        if (!drv) return -1;

        /** Fill in the structure. **/
        strcpy(drv->Name,"HTML Textual Source Driver");
        strcpy(drv->WidgetName,"html");
        drv->Render = hthtmlRender;
        drv->Verify = hthtmlVerify;
        xaInit(&(drv->PosParams),16);
        xaInit(&(drv->Properties),16);
        xaInit(&(drv->Events),16);
        xaInit(&(drv->Actions),16);
	strcpy(drv->Target, "Netscape47x:default");

        /** Add the 'load page' action **/
	htrAddAction(drv,"LoadPage");
	htrAddParam(drv,"LoadPage","Mode",DATA_T_STRING);
	htrAddParam(drv,"LoadPage","Source",DATA_T_STRING);
	htrAddParam(drv,"LoadPage","Transition",DATA_T_STRING);

        /** Register. **/
        htrRegisterDriver(drv);

        HTHTML.idcnt = 0;

    return 0;
    }
