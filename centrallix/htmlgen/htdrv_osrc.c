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
/* Module:      htdrv_osrc.c                                            */
/* Author:      John Peebles & Joe Heth                                 */
/* Creation:    Feb. 24, 2000                                           */
/* Description: HTML Widget driver for an object system                 */
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: htdrv_osrc.c,v 1.4 2002/03/09 19:21:20 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_osrc.c,v $

    $Log: htdrv_osrc.c,v $
    Revision 1.4  2002/03/09 19:21:20  gbeeley
    Basic security overhaul of the htmlgen subsystem.  Fixed many of my
    own bad sprintf habits that somehow worked their way into some other
    folks' code as well ;)  Textbutton widget had an inadequate buffer for
    the tb_init() call, causing various problems, including incorrect labels,
    and more recently, javascript errors.

    Revision 1.3  2002/03/09 02:38:48  jheth
    Make OSRC work with Form - Query at least

    Revision 1.2  2002/03/02 03:06:50  jorupp
    * form now has basic QBF functionality
    * fixed function-building problem with radiobutton
    * updated checkbox, radiobutton, and editbox to work with QBF
    * osrc now claims it's global name

    Revision 1.1  2002/02/27 01:38:51  jheth
    Initial commit of object source

    Revision 1.5  2002/02/23 19:35:28 jpeebles/jheth 

 **END-CVSDATA***********************************************************/

/** globals **/
static struct {
   int     idcnt;
} HTOSRC;

/* 
   htosrcVerify - not written yet.
*/
int htosrcVerify() {
   return 0;
}

/* 
   htosrcRender - generate the HTML code for the page.
   
   Don't know what this is, but we're keeping it for now - JJP, JDH
*/
   int htosrcRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj) {
   int id;
   char name[40];
   char *ptr;
   char *sbuf3;
   char *nptr;

   sbuf3 = nmMalloc(200);
   
   /** Get an id for this. **/
   id = (HTOSRC.idcnt++);

   /** Get name **/
   if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
   memccpy(name,ptr,0,39);
   name[39] = 0;

   /** Write named global **/
   nptr = (char*)nmMalloc(strlen(name)+1);
   strcpy(nptr,name);

   /** create our instance variable **/
   //htrAddScriptGlobal(s, nptr, "null",HTR_F_NAMEALLOC); 

  /** Ok, write the style header items. **/
  snprintf(sbuf3, 200, "    <STYLE TYPE=\"text/css\">\n");
  htrAddHeaderItem(s,sbuf3);
  snprintf(sbuf3, 200, "\t#osrc%dloader { POSITION:absolute; VISIBILITY:inherit; LEFT:0; TOP:0; clip:rect(1,1); Z-INDEX:0; }\n",id);
  htrAddHeaderItem(s,sbuf3);
  snprintf(sbuf3, 200, "    </STYLE>\n");
  htrAddHeaderItem(s,sbuf3);
	 
   
   htrAddScriptFunction(s, "osrc_action_clear", "\n"
      "function osrc_action_clear()\n"
      "    {\n"
      "    }\n",0);


   htrAddScriptFunction(s, "osrc_action_query", "\n"
      "function osrc_action_query(query)\n"
      "    {\n"
      "    //Send query as GET request\n"
      "    query = escape(query);\n"
      "    this.src = '/?ls__mode=osml&ls__req=' + query;\n"
      "    alert(query);\n" 
      "    }\n",0);
   
   htrAddScriptFunction(s, "osrc_action_delete", "\n"
      "function osrc_action_delete()\n"
      "    {\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_action_create", "\n"
      "function osrc_action_create()\n"
      "    {\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_action_modify", "\n"
      "function osrc_action_modify()\n"
      "    {\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_cb_query_continue", "\n"
      "function osrc_cb_query_continue()\n"
      "    {\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_cb_query_cancel", "\n"
      "function osrc_cb_query_cancel()\n"
      "    {\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_cb_request_object", "\n"
      "function osrc_cb_request_object()\n"
      "    {\n"
      "    }\n",0);



   /**  OSRC Initializer **/
   htrAddScriptFunction(s, "osrc_init", "\n"
      "function osrc_init(loader)\n"
      "    {\n"      
      "    loader.ActionClear=osrc_action_clear;\n"
      "    loader.ActionQuery=osrc_action_query;\n"
      "    loader.ActionDelete=osrc_action_delete;\n"
      "    loader.ActionCreate=osrc_action_create;\n"
      "    loader.ActionModify=osrc_action_modify;\n"
            
      "    loader.QueryContinue = osrc_cb_query_continue;\n"
      "    loader.QueryCancel = osrc_cb_query_cancel;\n"
      "    loader.RequestObject = osrc_cb_request_object;\n"
      "    return loader;\n"
      "    }\n", 0);


   /** Script initialization call. **/
   snprintf(sbuf3, 200, "osrc_current=osrc_init(%s.layers.osrc%dloader);\n", parentname, id);
   htrAddScriptInit(s, sbuf3);

   /** HTML body <DIV> element for the layers. **/
   snprintf(sbuf3, 200, "   <DIV ID=\"osrc%dloader\">\n",id);
   htrAddBodyItem(s, sbuf3);
   snprintf(sbuf3, 200, "   </DIV>\n");
   htrAddBodyItem(s, sbuf3);
   
   nmFree(sbuf3, 200);
   
   htrRenderSubwidgets(s, w_obj, parentname, parentobj, z);
   
   /** We set osrc_current=null so that orphans can't find us  **/
   htrAddScriptInit(s, "   osrc_current=null;\n\n");

   return 0;
}


/* 
   htosrcInitialize - register with the ht_render module.
*/
int htosrcInitialize() {
   pHtDriver drv;
   /*pHtEventAction action;
   pHtParam param;*/

   /** Allocate the driver **/
   drv = (pHtDriver)nmMalloc(sizeof(HtDriver));
   if (!drv) return -1;

   /** Fill in the structure. **/
   strcpy(drv->Name,"DHTML OSRC Driver");
   strcpy(drv->WidgetName,"osrc");
   drv->Render = htosrcRender;
   drv->Verify = htosrcVerify;
   xaInit(&(drv->PosParams),16);
   xaInit(&(drv->Properties),16);
   xaInit(&(drv->Events),16);
   xaInit(&(drv->Actions),16);

   /** Add a 'executemethod' action **/
   htrAddAction(drv,"Clear");
   htrAddAction(drv,"Query");
   htrAddAction(drv,"Delete");
   htrAddAction(drv,"Create");
   htrAddAction(drv,"Modify");


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

   HTOSRC.idcnt = 0;

   return 0;
}
