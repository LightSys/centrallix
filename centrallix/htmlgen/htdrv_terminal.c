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
/* Module: 	htdrv_terminal.c      					*/
/* Author:	Jonathan Rupp (JDR)					*/
/* Creation:	February 20, 2002 					*/
/* Description:	This is visual widget that emulates a vt100 terminal 	*/
/*									*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Log: htdrv_terminal.c,v $
    Revision 1.1  2002/12/24 09:51:56  jorupp
     * yep, this is what it looks like -- inital commit of the terminal widget :)
       -- the first Mozilla-only widget
     * it's not even close to a 'real' form yet -- mozilla takes so much CPU time
       rendering the table that it's pretty useless


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTTERM;


/*** httermVerify - not written yet.
 ***/
int
httermVerify()
    {
    return 0;
    }


/*** httermRender - generate the HTML code for the form 'glue'
 ***/
int
httermRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    int id;
    char* nptr;
    pObject sub_w_obj;
    pObjQuery qy;
#define MAX_COLORS 8
#define MAX_COLOR_LEN 32
    char colors[MAX_COLORS][MAX_COLOR_LEN]={"black","red","green","yellow","blue","purple","aqua","white"};
    int i;
    XString source;
    int rows, cols, fontsize, x, y;

    
    	/** Get an id for this. **/
	id = (HTTERM.idcnt++);

	/** read the color specs, leaving the defaults if they don't exist **/
	for(i=0;i<MAX_COLORS;i++)
	    {
	    char color[32];
	    sprintf(color,"color%i",i);
	    if (objGetAttrValue(w_obj,color,DATA_T_STRING,POD(&ptr)) == 0) 
		{
		strncpy(colors[i],ptr,MAX_COLOR_LEN);
		colors[i][MAX_COLOR_LEN-1]='\0';
		}
	    }

	if (objGetAttrValue(w_obj,"x",DATA_T_INTEGER,POD(&x)) != 0) 
	    {
	    mssError(0,"TERM","x is required");
	    return -1;
	    }

	if (objGetAttrValue(w_obj,"y",DATA_T_INTEGER,POD(&y)) != 0) 
	    {
	    mssError(0,"TERM","y is required");
	    return -1;
	    }

	if (objGetAttrValue(w_obj,"rows",DATA_T_INTEGER,POD(&rows)) != 0) 
	    {
	    mssError(0,"TERM","rows is required");
	    return -1;
	    }

	if (objGetAttrValue(w_obj,"cols",DATA_T_INTEGER,POD(&cols)) != 0) 
	    {
	    mssError(0,"TERM","cols is required");
	    return -1;
	    }

	if (objGetAttrValue(w_obj,"fontsize",DATA_T_INTEGER,POD(&fontsize)) != 0) 
	    {
	    fontsize = 12;
	    }

	xsInit(&source);

	if (objGetAttrValue(w_obj,"source",DATA_T_STRING,POD(&ptr)) == 0) 
	    {
	    xsCopy(&source,ptr,strlen(ptr));
	    }
	else
	    {
	    xsDeInit(&source);
	    mssError(0,"TERM","source is required");
	    return -1;
	    }
	
	/** Get name **/
	if (objGetAttrValue(w_obj,"name",DATA_T_STRING,POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);

	/** create our instance variable **/
	htrAddScriptGlobal(s, nptr, "null",HTR_F_NAMEALLOC); 

	/** Script include to add functions **/
	htrAddScriptInclude(s, "/sys/js/htdrv_terminal.js", 0);
	htrAddScriptInclude(s, "/sys/js/ht_utils_string.js", 0);

	/** HTML body <IFRAME> element to use as the base **/
	htrAddBodyItem_va(s,"    <DIV ID=\"term%dbase\"></DIV>\n",id);
	htrAddBodyItem_va(s,"    <IFRAME ID=\"term%dreader\"></IFRAME>\n",id);
	htrAddBodyItem_va(s,"    <IFRAME ID=\"term%dwriter\"></IFRAME>\n",id);
	
	/** write the stylesheet header element **/
	htrAddStylesheetItem_va(s,"        #term%dbase { POSITION:absolute; VISIBILITY:inherit; LEFT:%i; TOP:%i;  WIDTH:%i; HEIGHT:%i; Z-INDEX:%i; }\n",id,x,y,cols*fontsize,rows*fontsize,z);
	htrAddStylesheetItem_va(s,"        #term%dreader { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:0;  WIDTH:1; HEIGHT:1; Z-INDEX:-20; }\n",id);
	htrAddStylesheetItem_va(s,"        #term%dwriter { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:0;  WIDTH:1; HEIGHT:1; Z-INDEX:-20; }\n",id);
	htrAddStylesheetItem_va(s,"        .fixed%d {font-family: fixed; }\n",id);

	/** init line **/
	htrAddScriptInit_va(s,"    %s=terminal_init(%s,%d,'%s',%d,%d,new Array(",name,parentname,id,source.String,rows,cols);
	for(i=0;i<MAX_COLORS;i++)
	    {
	    if(i!=0)
		htrAddScriptInit(s,",");
	    htrAddScriptInit_va(s,"'%s'",colors[i]);
	    }
	htrAddScriptInit(s,"));\n");

	/** Check for and render all subobjects. **/
	qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
	if (qy)
	    {
	    while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
	        {
		objGetAttrValue(sub_w_obj, "outer_type", DATA_T_STRING,POD(&ptr));
		if (strcmp(ptr,"widget/connector") == 0)
		    htrRenderWidget(s, sub_w_obj, z+1, "", name);
		else
		    /** probably shouldn't render anything other than connectors, but who knows... **/
		    htrRenderWidget(s, sub_w_obj, z+1, parentname, parentobj);
		objClose(sub_w_obj);
		}
	    objQueryClose(qy);
	    }

    return 0;
    }


/*** httermInitialize - register with the ht_render module.
 ***/
int
httermInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Terminal Widget");
	strcpy(drv->WidgetName,"terminal");
	drv->Render = httermRender;
	drv->Verify = httermVerify;

	/** our first Mozilla-only widget :) **/
	htrAddSupport(drv, HTR_UA_MOZILLA);

	/** Add our actions **/
	htrAddAction(drv,"Disconnect");
	htrAddAction(drv,"Connect");

	/** Add our Events **/
	htrAddEvent(drv,"ConnectionOpen");
	htrAddEvent(drv,"ConnectionClose");

	/** Register. **/
	htrRegisterDriver(drv);

	HTTERM.idcnt = 0;

    return 0;
    }
