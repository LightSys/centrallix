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
/* Module:      htdrv_dropdown.c                                        */
/* Author:      Luke Ehresman (LME)                                     */
/* Creation:    Mar. 5, 2002                                            */
/* Description: HTML Widget driver for a drop down list                 */
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_dropdown.c,v 1.1 2002/03/07 00:20:00 lkehresman Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_dropdown.c,v $

    $Log: htdrv_dropdown.c,v $
    Revision 1.1  2002/03/07 00:20:00  lkehresman
    Added shell for the drop down list form widget.


 **END-CVSDATA***********************************************************/

/** globals **/
static struct {
   int     idcnt;
} HTDD;


/* 
   htddVerify - not written yet.
*/
int htddVerify() {
   return 0;
}


/* 
   htddRender - generate the HTML code for the page.
*/
int htddRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj) {
   char sbuf[256];
   int x,y,w,h;
   int id;

   /** Get an id for this. **/
   id = (HTDD.idcnt++);

   /** Get x,y of this object **/
   if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
   if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;
   if (objGetAttrValue(w_obj,"width",POD(&w)) != 0) {
	mssError(1,"HTDD","Drop Down widget must have a 'width' property");
	return -1;
   }
   if (objGetAttrValue(w_obj,"height",POD(&h)) != 0) {
	mssError(1,"HTDD","Drop Down widget must have a 'height' property");
	return -1;
   }

   /** Ok, write the style header items. **/
   sprintf(sbuf,"    <STYLE TYPE=\"text/css\">\n");
   htrAddHeaderItem(s,sbuf);
   sprintf(sbuf,"\t#dd%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; HEIGHT:13; WIDTH:13; Z-INDEX:%d; }\n",id,x,y,z);
   htrAddHeaderItem(s,sbuf);
   sprintf(sbuf,"    </STYLE>\n");
   htrAddHeaderItem(s,sbuf);

   /** Get Value function **/
   htrAddScriptFunction(s, "dd_getvalue", "\n"
	"function dd_getvalue() {\n"
	"}\n", 0);
   
   /** Set Value function **/
   htrAddScriptFunction(s, "dd_setvalue", "\n"
	"function dd_setvalue(v) {\n"
	"}\n", 0);
   
   /** Clear Value function **/
   htrAddScriptFunction(s, "dd_clearvalue", "\n"
	"function dd_clearvalue() {\n"
	"}\n", 0);
   
   /** Reset Value function **/
   htrAddScriptFunction(s, "dd_resetvalue", "\n"
	"function dd_resetvalue() {\n"
	"}\n", 0);
   
   /** Set Options function 
   *** The elements are passed in in the form of a hash where the
   *** key to the hash is the data value, and the value of the
   *** hash is the displayed label.
   **/
   htrAddScriptFunction(s, "dd_setoptions", "\n"
	"function dd_setoptions(ary) {\n"
	"}\n", 0);
   
   /** Enable function **/
   htrAddScriptFunction(s, "dd_enable", "\n"
	"function dd_enable() {\n"
	"}\n", 0);
   
   /** Read-Only function **/
   htrAddScriptFunction(s, "dd_readonly", "\n"
	"function dd_readonly() {\n"
	"}\n", 0);
   
   /** Disable function **/
   htrAddScriptFunction(s, "dd_disable", "\n"
	"function dd_disable() {\n"
	"}\n", 0);
   

   /** Form Status initializer **/
   htrAddScriptFunction(s, "dd_init", "\n"
	"function dd_init(l) {\n"
	"   l.kind = 'dropdown';\n"
	"   if (fm_current) fm_current.Register(l);\n"
	"}\n", 0);

   /** Script initialization call. **/
   sprintf(sbuf,"    dd_init(%s.layers.dd%dmain);\n", parentname, id);
   htrAddScriptInit(s, sbuf);

   /** HTML body <DIV> element for the layers. **/
   sprintf(sbuf,"<DIV ID=\"dd%dmain\">\n", id);
   sprintf(sbuf,"</DIV>\n");
   htrAddBodyItem(s, sbuf);

   return 0;
}


/* 
   htddInitialize - register with the ht_render module.
*/
int htddInitialize() {
   pHtDriver drv;

   /** Allocate the driver **/
   drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML Form Status Driver");
   strcpy(drv->WidgetName,"dropdown");
   drv->Render = htddRender;
   drv->Verify = htddVerify;
   xaInit(&(drv->PosParams),16);
   xaInit(&(drv->Properties),16);
   xaInit(&(drv->Events),16);
   xaInit(&(drv->Actions),16);

#if 00
   /** Add the 'load page' action **/
   action = (pHtEventAction)nmSysMalloc(sizeof(HtEventAction));
   strcpy(action->Name,"LoadPage");
   xaInit(&action->Parameters,16);
   param = (pHtParam)nmSysMalloc(sizeof(HtParam));
   strcpy(param->ParamName,"Source");
   param->DataType = DATA_T_STRING;
   xaAddItem(&action->Parameters,(void*)param);
   xaAddItem(&drv->Actions,(void*)action);
#endif

   /** Register. **/
   htrRegisterDriver(drv);

   HTDD.idcnt = 0;

   return 0;
}
