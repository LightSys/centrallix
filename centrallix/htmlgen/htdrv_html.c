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

    $Id: htdrv_html.c,v 1.1 2001/08/13 18:00:49 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_html.c,v $

    $Log: htdrv_html.c,v $
    Revision 1.1  2001/08/13 18:00:49  gbeeley
    Initial revision

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
	    strcpy(src,ptr);
	    }

	/** Check for a 'mode' - dynamic or static.  Default is static. **/
	if (objGetAttrValue(w_obj,"mode",POD(&ptr)) == 0 && !strcmp(ptr,"dynamic")) mode = 1;

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	strcpy(name,ptr);

	/** Ok, write the style header items. **/
	if (mode == 1)
	    {
	    sprintf(sbuf,"    <STYLE TYPE=\"text/css\">\n");
	    htrAddHeaderItem(s,sbuf);
	
	    /** Only give x and y if supplied. **/
	    if (x==-1 || y==-1)
	        {
	        sprintf(sbuf,"\t#ht%dpane { POSITION:relative; VISIBILITY:inherit; WIDTH:%d; Z-INDEX:%d; }\n",id,w,z);
	        htrAddHeaderItem(s,sbuf);
	        sprintf(sbuf,"\t#ht%dpane2 { POSITION:relative; VISIBILITY:hidden; WIDTH:%d; Z-INDEX:%d; }\n",id,w,z);
	        htrAddHeaderItem(s,sbuf);
	        }
	    else
	        {
	        sprintf(sbuf,"\t#ht%dpane { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	        htrAddHeaderItem(s,sbuf);
	        sprintf(sbuf,"\t#ht%dpane2 { POSITION:absolute; VISIBILITY:hidden; LEFT:%d; TOP:%d; WIDTH:%d; Z-INDEX:%d; }\n",id,x,y,w,z);
	        htrAddHeaderItem(s,sbuf);
	        }
	    sprintf(sbuf,"    </STYLE>\n");
	    htrAddHeaderItem(s,sbuf);

            /** Write named global **/
            nptr = (char*)nmMalloc(strlen(name)+1);
            strcpy(nptr,name);
            htrAddScriptGlobal(s, nptr, "null", HTR_F_NAMEALLOC);
    
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
            sprintf(sbuf,"    ht_init(%s.layers.ht%dpane,%s.layers.ht%dpane2,\"%s\",%s,%d,%d,%s);\n",
                    parentname, id, parentname, id, src, parentname, w,h, parentobj);
            htrAddScriptInit(s, sbuf);
            sprintf(sbuf,"    %s = %s.layers.ht%dpane;\n",nptr,parentname,id);
            htrAddScriptInit(s, sbuf);
    
            /** HTML body <DIV> element for the layer. **/
            sprintf(sbuf,"<DIV ID=\"ht%dpane2\"></DIV><DIV ID=\"ht%dpane\">\n",id,id);
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
            sprintf(sbuf,"%s.document",nptr);
	    }
	else
	    {
	    strcpy(sbuf,parentname);
	    nptr = parentobj;
	    }
        qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
        if (qy)
            {
            while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
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


/*** hthtmlInitialize - register with the ht_render module.
 ***/
int
hthtmlInitialize()
    {
    pHtDriver drv;
    pHtEventAction action;
    pHtParam param;

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

        /** Add the 'load page' action **/
        action = (pHtEventAction)nmSysMalloc(sizeof(HtEventAction));
        strcpy(action->Name,"LoadPage");
        xaInit(&action->Parameters,16);
        param = (pHtParam)nmSysMalloc(sizeof(HtParam));
        strcpy(param->ParamName,"Source");
        param->DataType = DATA_T_STRING;
        xaAddItem(&action->Parameters,(void*)param);
        xaAddItem(&drv->Actions,(void*)action);

        /** Register. **/
        htrRegisterDriver(drv);

        HTHTML.idcnt = 0;

    return 0;
    }
