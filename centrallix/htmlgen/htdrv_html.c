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

    $Id: htdrv_html.c,v 1.4 2002/06/09 23:44:46 nehresma Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_html.c,v $

    $Log: htdrv_html.c,v $
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
    int mode;
    char* nptr;
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
	    snprintf(sbuf,320,"    <STYLE TYPE=\"text/css\">\n");
	    htrAddHeaderItem(s,sbuf);
	
	    /** Only give x and y if supplied. **/
	    if (x==-1 || y==-1)
	        {
	        snprintf(sbuf,320,"\t#ht%dpane { POSITION:relative; VISIBILITY:inherit; WIDTH:%d; Z-INDEX:%d; }\n",id,w,z);
	        htrAddHeaderItem(s,sbuf);
	        snprintf(sbuf,320,"\t#ht%dpane2 { POSITION:relative; VISIBILITY:hidden; WIDTH:%d; Z-INDEX:%d; }\n",id,w,z);
	        htrAddHeaderItem(s,sbuf);
	        snprintf(sbuf,320,"\t#ht%dfader { POSITION:relative; VISIBILITY:hidden; WIDTH:%d; Z-INDEX:%d; }\n",id,w,z+1);
	        htrAddHeaderItem(s,sbuf);
	        }
	    else
	        {
	        snprintf(sbuf,320,"\t#ht%dpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	        htrAddHeaderItem(s,sbuf);
	        snprintf(sbuf,320,"\t#ht%dpane2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	        htrAddHeaderItem(s,sbuf);
	        snprintf(sbuf,320,"\t#ht%dfader { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z+1);
	        htrAddHeaderItem(s,sbuf);
	        }
	    snprintf(sbuf,320,"    </STYLE>\n");
	    htrAddHeaderItem(s,sbuf);

            /** Write named global **/
            nptr = (char*)nmMalloc(strlen(name)+1);
            strcpy(nptr,name);
            htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);
            htrAddScriptGlobal(s, "ht_fadeobj", "null", 0);
    
            /** This function handles the 'LoadPage' action **/
            htrAddScriptFunction(s, "ht_loadpage", "\n"
                    "function ht_loadpage(aparam)\n"
                    "    {\n"
		    "    this.transition = aparam.Transition;\n"
                    "    this.source = aparam.Source;\n"
                    "    }\n", 0);
    
            /** This function gets run when the user assigns to the 'source' property **/
            htrAddScriptFunction(s, "ht_sourcechanged", "\n"
                    "function ht_sourcechanged(prop,oldval,newval)\n"
                    "    {\n"
                    "    if (newval.substr(0,5)=='http:')\n"
                    "        {\n"
		    "        this.newsrc = newval;\n"
		    "        if (this.transition && this.transition != 'normal')\n"
		    "            {\n"
		    "            ht_startfade(this,this.transition,'out',ht_dosourcechange);\n"
		    "            }\n"
		    "        else\n"
		    "            ht_dosourcechange(this);\n"
                    "        }\n"
                    "    return newval;\n"
                    "    }\n", 0);

	    /** This function completes the doc source change **/
	    htrAddScriptFunction(s, "ht_dosourcechange", "\n"
		    "function ht_dosourcechange(l)\n"
		    "    {\n"
		    "    tmpl = l.curLayer;\n"
		    "    tmpl.visibility = 'hidden';\n"
		    "    l.curLayer = l.altLayer;\n"
		    "    l.altLayer = tmpl;\n"
                    "    l.curLayer.onload = ht_reloaded;\n"
                    "    l.curLayer.bgColor = null;\n"
                    "    l.curLayer.load(l.newsrc,l.clip.width);\n"
		    "    }\n", 0);

	    /** This function does the intermediate fading steps **/
	    htrAddScriptFunction(s, "ht_fadestep", "\n"
		    "function ht_fadestep()\n"
		    "    {\n"
		    "    ht_fadeobj.faderLayer.background.src = '/sys/images/fade_' + ht_fadeobj.transition + '_0' + ht_fadeobj.count + '.gif';\n"
		    "    ht_fadeobj.count++;\n"
		    "    if (ht_fadeobj.count == 5 || ht_fadeobj.count >= 9)\n"
		    "        {\n"
		    "        if (ht_fadeobj.completeFn) return ht_fadeobj.completeFn(ht_fadeobj);\n"
		    "        else return;\n"
		    "        }\n"
		    "    setTimeout(ht_fadestep,100);\n"
		    "    }\n", 0);

	    /** This function controls the fade transitions of a layer **/
	    htrAddScriptFunction(s, "ht_startfade", "\n"
		    "function ht_startfade(l,ftype,inout,fn)\n"
		    "    {\n"
		    "    ht_fadeobj = l;\n"
		    "    if (l.faderLayer.clip.height < l.curLayer.clip.height)\n"
		    "        l.faderLayer.clip.height=l.curLayer.clip.height;\n"
		    "    if (l.faderLayer.clip.width < l.curLayer.clip.width)\n"
		    "        l.faderLayer.clip.width=l.curLayer.clip.width;\n"
		    "    l.faderLayer.moveAbove(l.curLayer);\n"
		    "    l.faderLayer.visibility='inherit';\n"
		    "    l.completeFn = fn;\n"
		    "    if (inout == 'in')\n"
		    "        {\n"
		    "        l.count=5;\n"
		    "        setTimeout(ht_fadestep,20);\n"
		    "        }\n"
		    "    else\n"
		    "        {\n"
		    "        l.count=1;\n"
		    "        setTimeout(ht_fadestep,20);\n"
		    "        }\n"
		    "    };\n", 0);
    
            /** This function is called when the layer is reloaded. **/
            htrAddScriptFunction(s, "ht_reloaded", "\n"
                    "function ht_reloaded(e)\n"
                    "    {\n"
                    "    e.target.mainLayer.watch('source',ht_sourcechanged);\n"
                    "    e.target.clip.height = e.target.document.height;\n"
		    "    e.target.mainLayer.faderLayer.moveAbove(e.target);\n"
		    "    e.target.visibility = 'inherit';\n"
                    /*"    e.target.document.captureEvents(Event.CLICK);\n"
                    "    e.target.document.onclick = ht_click;\n"*/
                    "    for(i=0;i<e.target.document.links.length;i++)\n"
                    "        {\n"
                    "        e.target.document.links[i].layer = e.target.mainLayer;\n"
		    "        e.target.document.links[i].kind = 'ht';\n"
                    "        }\n"
                    "    pg_resize(e.target.mainLayer.parentLayer);\n"
		    "    if (e.target.mainLayer.transition && e.target.mainLayer.transition != 'normal')\n"
		    "        ht_startfade(e.target.mainLayer,e.target.mainLayer.transition,'in',null);\n"
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
                    "function ht_init(l,l2,fl,source,pdoc,w,h,p)\n"
                    "    {\n"
		    "    l.faderLayer = fl;\n"
		    "    l.mainLayer = l;\n"
		    "    l2.mainLayer = l;\n"
		    "    fl.mainLayer = l;\n"
		    "    l.curLayer = l;\n"
		    "    l.altLayer = l2;\n"
                    "    l.LSParent = p;\n"
                    "    l.kind = 'ht';\n"
                    "    l2.kind = 'ht';\n"
		    "    fl.kind = 'ht';\n"
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
            snprintf(sbuf,320,"    ht_init(%s.layers.ht%dpane,%s.layers.ht%dpane2,%s.layers.ht%dfader,\"%s\",%s,%d,%d,%s);\n",
                    parentname, id, parentname, id, parentname, id, src, parentname, w,h, parentobj);
            htrAddScriptInit(s, sbuf);
            snprintf(sbuf,320,"    %s = %s.layers.ht%dpane;\n",nptr,parentname,id);
            htrAddScriptInit(s, sbuf);
    
            /** HTML body <DIV> element for the layer. **/
            snprintf(sbuf,320,"<DIV background=\"/sys/images/fade_lrwipe_01.gif\" ID=\"ht%dfader\"></DIV><DIV ID=\"ht%dpane2\"></DIV><DIV ID=\"ht%dpane\">\n",id,id,id);
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
	htrAddParam(drv,"LoadPage","Source",DATA_T_STRING);
	htrAddParam(drv,"LoadPage","Transition",DATA_T_STRING);

        /** Register. **/
        htrRegisterDriver(drv);

        HTHTML.idcnt = 0;

    return 0;
    }
