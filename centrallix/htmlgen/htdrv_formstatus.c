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
/* Module:      htdrv_formstatus.c                                      */
/* Author:      Luke Ehresman (LME)                                     */
/* Creation:    Mar. 5, 2002                                            */
/* Description: HTML Widget driver for a form status                    */
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_formstatus.c,v 1.3 2002/03/09 19:21:20 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_formstatus.c,v $

    $Log: htdrv_formstatus.c,v $
    Revision 1.3  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.2  2002/03/07 00:12:58  lkehresman
    Reworked form status widget to take advantage of the new icons

    Revision 1.1  2002/03/06 23:31:07  lkehresman
    Added form status widget.


 **END-CVSDATA***********************************************************/

/** globals **/
static struct {
   int     idcnt;
} HTFS;


/* 
   htfsVerify - not written yet.
*/
int htfsVerify() {
   return 0;
}


/* 
   htfsRender - generate the HTML code for the page.
*/
int htfsRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj) {
   char sbuf[HT_SBUF_SIZE];
   int x=-1,y=-1;
   int id;

   /** Get an id for this. **/
   id = (HTFS.idcnt++);

   /** Get x,y of this object **/
   if (objGetAttrValue(w_obj,"x",POD(&x)) != 0) x=0;
   if (objGetAttrValue(w_obj,"y",POD(&y)) != 0) y=0;

   /** Ok, write the style header items. **/
   snprintf(sbuf,HT_SBUF_SIZE,"    <STYLE TYPE=\"text/css\">\n");
   htrAddHeaderItem(s,sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"\t#fs%dmain { POSITION:absolute; VISIBILITY:inherit; LEFT:%d; TOP:%d; HEIGHT:13; WIDTH:13; Z-INDEX:%d; }\n",id,x,y,z);
   htrAddHeaderItem(s,sbuf);
   snprintf(sbuf,HT_SBUF_SIZE,"    </STYLE>\n");
   htrAddHeaderItem(s,sbuf);

   /** Clear function **/
   htrAddScriptFunction(s, "fs_setvalue", "\n"
	"function fs_setvalue(m) {\n"
	"   this.currentMode = m;\n"
	"   if (this.currentMode == 'View') {;\n" 
	"      this.document.images[0].src = '/sys/images/formstat01.gif';\n"
	"   } else if (this.currentMode == 'Modify') {\n"
	"      this.document.images[0].src = '/sys/images/formstat02.gif';\n"
	"   } else if (this.currentMode == 'New') {\n"
	"      this.document.images[0].src = '/sys/images/formstat03.gif';\n"
	"   } else if (this.currentMode == 'Query') {\n"
	"      this.document.images[0].src = '/sys/images/formstat04.gif';\n"
	"   } else {\n"
	"      this.document.images[0].src = '/sys/images/formstat05.gif';\n"
	"   }\n"
	"}\n", 0);

   

   /** Form Status initializer **/
   htrAddScriptFunction(s, "fs_init", "\n"
	"function fs_init(l) {\n"
	"   l.kind = 'formstatus';\n"
	"   l.currentMode = 'No Data';\n"
	"   l.isFormStatusWidget = true;\n"
	"   l.setvalue = fs_setvalue;\n"
	"   if (fm_current) fm_current.Register(l);\n"
	/*
	"   cnt=0;\n"
	"   while (confirm('again?')) {\n"
	"      if (cnt == 0) {\n"
	"         l.setvalue('Modify');\n"
	"      } else if (cnt == 1) {\n"
	"         l.setvalue('Modify');\n"
	"      } else if (cnt == 2) {\n"
	"         l.setvalue('New');\n"
	"      } else if (cnt == 3) {\n"
	"         l.setvalue('Query');\n"
	"      } else {\n"
	"         l.setvalue('No Data');\n"
	"         cnt = -1;\n"
	"      }\n"
	"      cnt++\n"
	"   }\n"
	*/
	"}\n", 0);

   /** Script initialization call. **/
   snprintf(sbuf,HT_SBUF_SIZE,"    fs_init(%s.layers.fs%dmain);\n", parentname, id);
   htrAddScriptInit(s, sbuf);

   /** HTML body <DIV> element for the layers. **/
   snprintf(sbuf,HT_SBUF_SIZE,"   <DIV ID=\"fs%dmain\"><IMG SRC=/sys/images/formstat01.gif></DIV>\n", id);
   htrAddBodyItem(s, sbuf);

   return 0;
}


/* 
   htfsInitialize - register with the ht_render module.
*/
int htfsInitialize() {
   pHtDriver drv;
   /*pHtEventAction action;
   pHtParam param;*/

   /** Allocate the driver **/
   drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML Form Status Driver");
   strcpy(drv->WidgetName,"formstatus");
   drv->Render = htfsRender;
   drv->Verify = htfsVerify;
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

   HTFS.idcnt = 0;

   return 0;
}
