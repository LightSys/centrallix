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

    $Id: htdrv_osrc.c,v 1.2 2002/03/02 03:06:50 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_osrc.c,v $

    $Log: htdrv_osrc.c,v $
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

   /** Get an id for this. **/
   id = (HTCB.idcnt++);

   /** Get name **/
   if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
   strcpy(name,ptr);

   /** Write named global **/
   nptr = (char*)nmMalloc(strlen(name)+1);
   strcpy(nptr,name);

   /** create our instance variable **/
   htrAddScriptGlobal(s, nptr, "null",HTR_F_NAMEALLOC); 


   htrAddScriptFunction(s, "osrc_clear", "\n"
      "function osrc_clear()\n"
      "    {\n"
      "    }\n",0);


   htrAddScriptFunction(s, "osrc_query", "\n"
      "function osrc_query()\n"
      "    {\n"
      "    }\n",0);
   
   htrAddScriptFunction(s, "osrc_delete", "\n"
      "function osrc_delete()\n"
      "    {\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_create", "\n"
      "function osrc_create()\n"
      "    {\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_modify", "\n"
      "function osrc_modify()\n"
      "    {\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_query_continue", "\n"
      "function osrc_query_continue()\n"
      "    {\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_query_cancle", "\n"
      "function osrc_query_cancle()\n"
      "    {\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_request_object", "\n"
      "function osrc_request_object()\n"
      "    {\n"
      "    }\n",0);



   /**  OSRC Initializer **/
   htrAddScriptFunction(s, "osrc_init", "\n"
      "function osrc_init(name)\n"
      "    {\n"      
      "    osrc = new Object();\n"
      "    osrc.name=name;\n"	
      
      "    osrc.ActionClear=osrc_action_clear;\n"
      "    osrc.ActionQuery=osrc_action_query;\n"
      "    osrc.ActionDelete=osrc_action_delete;\n"
      "    osrc.ActionCreate=osrc_action_create;\n"
      "    osrc.ActionModify=osrc_action_modify;\n"
            
      "    osrc.QueryContinue = osrc_cb_query_continue;\n"
      "    osrc.QueryCancel = osrc_cb_query_cancel;\n"
      "    osrc.RequestObject = osrc_cb_request_object;\n"

      "    return osrc;\n"
      "    }\n", 0);


   /** Script initialization call. **/
   sbuf3 = nmMalloc(200);
   snprintf(sbuf3, 200, "\n %s=osrc_current=osrc_init(%s);\n", name, name);
   htrAddScriptInit(s, sbuf3);
   nmFree(sbuf3, 200);

   /** HTML body <DIV> element for the layers. **/
   sprintf(sbuf,"   <DIV ID=\"cb%dmain\">\n",id);
   htrAddBodyItem(s, sbuf);
   sprintf(sbuf,"     <IMG SRC=/sys/images/checkbox_unchecked.gif>\n");
   htrAddBodyItem(s, sbuf);
   sprintf(sbuf,"   </DIV>\n");
   htrAddBodyItem(s, sbuf);

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
