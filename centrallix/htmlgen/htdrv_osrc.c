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

    $Id: htdrv_osrc.c,v 1.8 2002/03/14 05:11:49 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_osrc.c,v $

    $Log: htdrv_osrc.c,v $
    Revision 1.8  2002/03/14 05:11:49  jorupp
     * bugfixes

    Revision 1.7  2002/03/14 03:29:51  jorupp
     * updated form to prepend a : to the fieldname when using for a query
     * updated osrc to take the query given it by the form, submit it to the server,
        iterate through the results, and store them in the replica
     * bug fixes to treeview (DOMviewer mode) -- added ability to change scaler values

    Revision 1.6  2002/03/13 01:35:02  jheth
    Re-commit of Object Source - No Alerts

    Revision 1.5  2002/03/13 01:04:32  jheth
    Partial working Object Source - Functionality added but no reliable testing

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
  snprintf(sbuf3, 200, "\t#osrc%dloader { POSITION:absolute; VISIBILITY:inherit; LEFT:0; TOP:300;  WIDTH:500; HEIGHT:500; Z-INDEX:20; }\n",id);
  htrAddHeaderItem(s,sbuf3);
  snprintf(sbuf3, 200, "    </STYLE>\n");
  htrAddHeaderItem(s,sbuf3);
	 
   
   htrAddScriptFunction(s, "osrc_action_clear", "\n"
      "function osrc_action_clear()\n"
      "    {\n"
      "    //No Records Returned\n"
      "    for (var i in this.children)\n"
      "         {\n"
      "         this.children[i] = ClearAll();\n"
      "         }\n"
      "         \n"
      "    //Clear replica of data\n"
      "    delete data;\n"
      "    }\n",0);


   htrAddScriptFunction(s, "osrc_action_query", "\n"
      "function osrc_action_query(q)\n"
      "    {\n"
      "    //Is discard ready\n"
      "    //Send query as GET request\n"
      "    this.query = escape(q);\n"
      "    //this.query = 'select%20%3Aid%2C%20%3Afull_name%2C%20%3Anum_days%20from%20Months.csv'\n"
      "    //this.query = escape(\"select :full_name, :num_days from /samples/Months.csv/rows\");\n"
      "    //this.query = escape(\"SELECT :full_name, :num_days FROM /Months.csv/rows\");\n"
      "    this.query=String(this.query).replace(\"/\",\"%2F\",\"g\");\n"
      "    //alert(this.query);\n"
      "    this.replica=new Array();\n"
      "    this.OpenSession();\n"
      "    }\n",0);
      
   htrAddScriptFunction(s, "osrc_store_replica", "\n"
      "function osrc_store_replica()\n"
      "    {\n"
      "    alert(this.document.links[0].target);\n"
      "    if (this.document.links.length > 0)\n"
      "        {\n"
      "        //Store Links\n"
      "        //alert('Store Links');\n"
      "        data = new Object();\n"
      "        data.name = \"data\";\n"
      "        data.annotation = \"Data\";\n"
      "        data.Attributes = new Array();\n"
      "        for (var i in this.document.links)\n"
      "             {\n"
      "             data.Attributes[i] = new Array();\n"
      "             //Attr Type\n"
      "             data.Attributes[i][i] = this.document.links[i].target;\n"
      "             //Attr Value\n"
      "             data.Attributes[i][i+1] = this.document.links[i].text;\n"
      "             }\n"
      "        form.DataReady(data);\n"
      "        }\n"
      "    else\n"
      "        {\n"
      "        //Clear Method - No Anchors Returned\n"
      "        //alert('Clear');\n"
      "        }\n" 
      "    this.form.IsDiscardReady();\n"
      "    }\n",0);
   
   htrAddScriptFunction(s, "osrc_action_delete", "\n"
      "function osrc_action_delete()\n"
      "    {\n"
      "    //Delete an object\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_action_create", "\n"
      "function osrc_action_create()\n"
      "    {\n"
      "    //Create an object\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_action_modify", "\n"
      "function osrc_action_modify()\n"
      "    {\n"
      "    //Modify an object\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_cb_query_continue", "\n"
      "function osrc_cb_query_continue()\n"
      "    {\n"
      "    //Is Form Ready - Return\n"
      "    this.form.ready = TRUE;\n"
      "    return 0;\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_cb_query_cancel", "\n"
      "function osrc_cb_query_cancel()\n"
      "    {\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_cb_request_object", "\n"
      "function osrc_cb_request_object()\n"
      "    {\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_cb_register", "\n"
      "function osrc_cb_register(aparam)\n"
      "    {\n"
      "    this.children.push(aparam);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_open_session", "\n"
      "function osrc_open_session()\n"
      "    {\n"
      "    //Open Session\n"
      "    this.src = '/?ls__mode=osml&ls__req=opensession'\n"
      "    this.onLoad = osrc_open_query;\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_open_query", "\n"
      "function osrc_open_query()\n"
      "    {\n"
      "    //Open Query\n"
      "    //confirm(this.document.anchors.length);\n"
      "    this.sid=this.document.links[0].target;\n"
      "    this.onLoad = osrc_get_qid;\n"
      "    this.src=\"/?ls__mode=osml&ls__req=multiquery&ls__sid=\"+this.sid+\"&ls__sql=\" + this.query;\n"
      "    //this.src = '/escape.html';\n"
      "    //confirm(this.document.anchors.length);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_get_qid", "\n"
      "function osrc_get_qid()\n"
      "    {\n"
      "    this.qid=this.document.links[0].target;\n"
      "    this.onLoad = osrc_fetch_next;\n"
      "    this.src=\"/?ls__mode=osml&ls__req=queryfetch&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid+\"&ls__objmode=0&ls__rowcount=1\";\n"
      "    //alert(this.src);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_fetch_next", "\n"
      "function osrc_fetch_next()\n"
      "    {\n"
      "    //return 0;\n"
      "    var lnk=this.document.links;\n"
      "    var lc=lnk.length;\n"
      "    if(lc<2)\n"
      "        {\n"	// query over
      "        this.onLoad=osrc_close_query;\n"	//don't need to trap this...
      "        this.src=\"/?ls__mode=osml&ls__req=closequery&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid;\n"
      "        return 0;\n"
      "        }\n"
      "    var dataobj=new Array();\n"
      "    var row=lnk[1].target;\n"
      "    for (var l=1;l<lc;l++)\n"
      "        {\n"
      "        dataobj[lnk[l].host]=new Array();\n"
      "        dataobj[lnk[l].host][\"value\"]=lnk[l].text\n"
      "        dataobj[lnk[l].host][\"type\"]=lnk[l].hash.substr(1);\n"
      "        }\n"
      "    this.replica[row]=dataobj;\n"
      "    this.onLoad = osrc_fetch_next;\n"
      "    this.src=\"/?ls__mode=osml&ls__req=queryfetch&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid+\"&ls__objmode=0&ls__rowcount=1\";\n"
      "    }\n",0);
      
   htrAddScriptFunction(s, "osrc_close_query", "\n"
      "function osrc_close_query()\n"
      "    {\n"
      "    this.qid=null;\n"
      "    this.sid=null;\n"
      "    this.onLoad=null;\n"
      "    }\n",0);
 
   htrAddScriptFunction(s, "osrc_close_object", "\n"
      "function osrc_close_object()\n"
      "    {\n"
      "    //Close Object\n"
      "    this.src = '/?ls__mode=osml&ls__req=close&ls__oid=23456789';\n"
      "    this.onLoad = osrc_close_session;\n"
      "    }\n",0);
 
   htrAddScriptFunction(s, "osrc_close_session", "\n"
      "function osrc_close_session()\n"
      "    {\n"
      "    //Close Session\n"
      "    this.src = '/?ls__mode=osml&ls__req=closesession&ls__sid=01234567';\n"
      "    this.onLoad = osrc_store_replica;\n"
      "    }\n",0);

      
   /**  OSRC Initializer **/
   htrAddScriptFunction(s, "osrc_init", "\n"
      "function osrc_init(loader)\n"
      "    {\n"
      "    loader.children = new Array();\n"
      "    loader.ActionClear=osrc_action_clear;\n"
      "    loader.ActionQuery=osrc_action_query;\n"
      "    loader.ActionDelete=osrc_action_delete;\n"
      "    loader.ActionCreate=osrc_action_create;\n"
      "    loader.ActionModify=osrc_action_modify;\n"

      "    loader.OpenSession=osrc_open_session;\n"
      "    loader.OpenQuery=osrc_open_query;\n"
      "    loader.CloseQuery=osrc_close_query;\n"
      "    loader.CloseObject=osrc_close_object;\n"
      "    loader.CloseSession=osrc_close_session;\n"
      "    loader.StoreReplica=osrc_store_replica;\n"
            
      "    loader.QueryContinue = osrc_cb_query_continue;\n"
      "    loader.QueryCancel = osrc_cb_query_cancel;\n"
      "    loader.RequestObject = osrc_cb_request_object;\n"
      "    loader.Register = osrc_cb_register;\n"
      "    return loader;\n"
      "    }\n", 0);


   /** Script initialization call. **/
   snprintf(sbuf3, 200, "osrc_current=osrc_init(%s.layers.osrc%dloader);\n", parentname, id);
   htrAddScriptInit(s, sbuf3);

   /** HTML body <DIV> element for the layers. **/
   snprintf(sbuf3, 200, "   <DIV ID=\"osrc%dloader\"></DIV>\n",id);
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
