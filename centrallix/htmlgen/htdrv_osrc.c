/* vim: set sw=3: */

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

    $Id: htdrv_osrc.c,v 1.32 2002/06/03 05:45:35 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_osrc.c,v $

    $Log: htdrv_osrc.c,v $
    Revision 1.32  2002/06/03 05:45:35  jorupp
     * removing alerts

    Revision 1.31  2002/06/03 05:10:56  jorupp
     * removed a debugging message -- not needed

    Revision 1.30  2002/06/03 05:09:25  jorupp
     * impliment the form view mode correctly
     * fix scrolling back in the OSRC (from the table)
     * throw DataChange event when any data is changed

    Revision 1.29  2002/06/02 22:13:21  jorupp
     * added disable functionality to image button (two new Actions)
     * bugfixes

    Revision 1.28  2002/06/01 19:46:15  jorupp
     * mixed annoying problem where sometimes, OSRC would make the last record the active one instead of the first one

    Revision 1.27  2002/06/01 19:16:48  jorupp
     * down-sized the osrc's hidden layer, which was causing the page to be
        longer than it should be, which caused the up/down scrollbar to
        appear, which caused the textarea that handles keypress events to
        be off the page, which meant that it couldn't recieve keyboard focus,
        which meant that there were no keypress events.

    Revision 1.26  2002/05/31 05:03:32  jorupp
     * OSRC now can do a DoubleSync -- check kardia for an example

    Revision 1.25  2002/05/30 05:01:31  jorupp
     * OSRC has a Sync Action (best used to tie two OSRCs together on a table selection)
     * NOTE: with multiple tables in an app file, netscape seems to like to hang (the JS engine at least)
        while rendering the page.  uncomment line 1109 in htdrv_table.c to fix it (at the expense of extra alerts)
        -- I tried to figure this out, but was unsuccessful....

    Revision 1.24  2002/05/30 00:03:07  jorupp
     * this ^should^ allow nesting of the osrc and form, but who knows.....

    Revision 1.23  2002/05/06 22:29:36  jorupp
     * minor bug fixes that I found while documenting the OSRC

    Revision 1.22  2002/04/30 18:08:43  jorupp
     * more additions to the table -- now it can scroll~
     * made the osrc and form play nice with the table
     * minor changes to form sample

    Revision 1.21  2002/04/28 21:36:59  jorupp
     * only one session at a time open
     * only one query at a time open
     * disabled query close on exit, as it would crash the browser or load a blank page

    Revision 1.20  2002/04/28 06:00:38  jorupp
     * added htrAddScriptCleanup* stuff
     * added cleanup stuff to osrc

    Revision 1.19  2002/04/27 22:47:45  jorupp
     * re-wrote form and osrc interaction -- more happens now in the form
     * lots of fun stuff in the table....check form.app for an example (not completely working yet)
     * the osrc is still a little bit buggy.  If you get it screwed up, let me know how to reproduce it.

    Revision 1.18  2002/04/26 22:12:27  jheth
    Added nextPage() prevPage() functions to OSRC - Didn't test it though - something is broken. I can't make depend.

    Revision 1.17  2002/04/25 23:02:52  jorupp
     * added alternate alignment for labels (right or center should work)
     * fixed osrc/form bug

    Revision 1.16  2002/04/25 03:13:50  jorupp
     * added label widget
     * bug fixes in form and osrc

    Revision 1.15  2002/04/10 00:36:20  jorupp
     * fixed 'visible' bug in imagebutton
     * removed some old code in form, and changed the order of some callbacks
     * code cleanup in the OSRC, added some documentation with the code
     * OSRC now can scroll to the last record
     * multiple forms (or tables?) hitting the same osrc now _shouldn't_ be a problem.  Not extensively tested however.

    Revision 1.14  2002/03/28 05:21:23  jorupp
     * form no longer does some redundant status checking
     * cleaned up some unneeded stuff in form
     * osrc properly impliments almost everything (will prompt on unsaved data, etc.)

    Revision 1.13  2002/03/26 06:38:05  jorupp
    osrc has two new parameters: readahead and replicasize
    osrc replica now operates on a sliding window principle (holds a range of records, instead of all between the beginning and the current one)

    Revision 1.12  2002/03/23 00:32:13  jorupp
     * osrc now can move to previous and next records
     * form now loads it's basequery automatically, and will not load if you don't have one
     * modified form test page to be a bit more interesting

    Revision 1.11  2002/03/20 21:13:12  jorupp
     * fixed problem in imagebutton point and click handlers
     * hard-coded some values to get a partially working osrc for the form
     * got basic readonly/disabled functionality into editbox (not really the right way, but it works)
     * made (some of) form work with discard/save/cancel window

    Revision 1.10  2002/03/16 05:55:14  jheth
    Added Move First/Next/Previous/Last logic
    Query obtains oid and now closes object and session

    Revision 1.9  2002/03/16 02:04:05  jheth
    osrc widget queries and passes data back to form widget

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
int
htosrcRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
   {
   int id;
   char name[40];
   char *ptr;
   char *sbuf3;
   char *nptr;
   int readahead;
   int scrollahead;
   int replicasize;
   char *sql;
   char *filter;
   pObject sub_w_obj;
   pObjQuery qy;

   sbuf3 = nmMalloc(200);
   
   /** Get an id for this. **/
   id = (HTOSRC.idcnt++);

   /** Get name **/
   if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
   memccpy(name,ptr,0,39);
   name[39] = 0;

   if (objGetAttrValue(w_obj,"replicasize",POD(&replicasize)) != 0)
      replicasize=6;
   if (objGetAttrValue(w_obj,"readahead",POD(&readahead)) != 0)
      readahead=replicasize/2;
   if (objGetAttrValue(w_obj,"scrollahead",POD(&readahead)) != 0)
      scrollahead=readahead;

   /** try to catch mistakes that would probably make Netscape REALLY buggy... **/
   if(replicasize==1 && readahead==0) readahead=1;
   if(replicasize==1 && scrollahead==0) scrollahead=1;
   if(readahead>replicasize) replicasize=readahead;
   if(scrollahead>replicasize) replicasize=scrollahead;
   if(scrollahead<1) scrollahead=1;
   if(replicasize<1 || readahead<1)
      {
      mssError(1,"HTOSRC","You must give positive integer for replicasize and readahead");
      return -1;
      }

   if (objGetAttrValue(w_obj,"sql",POD(&ptr)) == 0)
      {
      sql=nmMalloc(strlen(ptr)+1);
      strcpy(sql,ptr);
      }
   else
      {
      mssError(1,"HTOSRC","You must give a sql parameter");
      }

   if (objGetAttrValue(w_obj,"filter",POD(&ptr)) == 0)
      {
      filter=nmMalloc(strlen(ptr)+1);
      strcpy(filter,ptr);
      }
   else
      {
      filter=nmMalloc(1);
      filter[0]='\0';
      }



   /** Write named global **/
   nptr = (char*)nmMalloc(strlen(name)+1);
   strcpy(nptr,name);

   /** create our instance variable **/
   //htrAddScriptGlobal(s, nptr, "null",HTR_F_NAMEALLOC); 

  /** Ok, write the style header items. **/
  htrAddHeaderItem(s,"    <STYLE TYPE=\"text/css\">\n");
  htrAddHeaderItem_va(s,"        #osrc%dloader { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:1;  WIDTH:1; HEIGHT:1; Z-INDEX:-20; }\n",id);
  htrAddHeaderItem(s,"    </STYLE>\n");
	 
#if 0   
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
#endif

   htrAddScriptFunction(s, "osrc_init_query", "\n"
      "function osrc_init_query()\n"
      "    {\n"
      "    //alert();\n"
      "    this.init=true;\n"
      "    this.ActionQueryObject(null,null);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_action_order_object", "\n"
      "function osrc_action_order_object(order)\n"
      "    {\n"
      "    this.pendingorderobject=order;\n"
      "    this.ActionQueryObject(this.queryobject);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_action_query_object", "\n"
      "function osrc_action_query_object(q, formobj)\n"
      "    {\n"
      "    if(this.pending)\n"
      "        {\n"
      "        alert('There is already a query or movement in progress...');\n"
      "        return 0;\n"
      "        }\n"
      "    this.pendingqueryobject=q;\n"
      "    var statement=this.sql;\n"
      "    var firstone=true;\n"
      "    \n"
      "    if(this.filter)\n"
      "        {\n"
      "        statement+=' WHERE ('+this.filter+')';\n"
      "        firstone=false;\n"
      "        }\n"
      "    //if(!confirm(osrc_make_filter(q)))\n"
      "    //    return 0;\n"
      "    if(q && q.joinstring && q[0])\n"
      "        {\n"
      "        if(firstone)\n"
      "            statement+=' WHERE ';\n"
      "        else\n"
      "            statement+=' '+q.joinstring+' ';\n"
      "        statement+=osrc_make_filter(q);\n"
      "        }\n"
      "    \n"
      "    firstone=true;\n"
      "    if(this.pendingorderobject)\n"
      "        for(i in this.pendingorderobject)\n"
      "            {\n"
      "            //alert('asdf'+this.pendingorderobject[i]);\n"
      "            if(firstone)\n"
      "                {\n"
      "                statement+=' ORDER BY '+this.pendingorderobject[i];\n"
      "                firstone=false;\n"
      "                }\n"
      "            else\n"
      "                {\n"
      "                statement+=', '+this.pendingorderobject[i];\n"
      "                }\n"
      "            }\n"
      "    //statement+=';';\n"
      "    this.ActionQuery(statement,formobj);\n"
      "    \n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_make_filter", "\n"
      "function osrc_make_filter(q)\n"
      "    {\n"
      "    var firstone=true;\n"
      "    var statement='';\n"
      "    for(var i in q)\n"
      "        {\n"
      "        if(i!='oid' && i!='joinstring')\n"
      "            {\n"
      "            var str;\n"
      "            if(q[i].joinstring)\n"
      "                {\n"
      "                str=osrc_make_filter(q[i]);\n"
      "                }\n"
      "            else\n"
      "                {\n"
      "                var val=q[i].value;\n"
      "                switch(q[i].type)\n"
      "                    {\n"
      "                    case 'integer':\n"
      "                        str=':'+q[i].oid+'='+val;"
      "                        break;\n"
      "                    default:\n"
      "                        /**/ if(val.substring(0,2)=='>=')\n"
      "                            str=':'+q[i].oid+' >= '+val.substring(1);\n"
      "                        else if(val.substring(0,2)=='<=')\n"
      "                            str=':'+q[i].oid+' <= '+val.substring(1);\n"
      "                        else if(val.substring(0,2)=='=>')\n"
      "                            str=':'+q[i].oid+' >= '+val.substring(1);\n"
      "                        else if(val.substring(0,2)=='=<')\n"
      "                            str=':'+q[i].oid+' <= '+val.substring(1);\n"
      "                        else if(val.substring(0,1)=='>')\n"
      "                            str=':'+q[i].oid+' > '+val.substring(1);\n"
      "                        else if(val.substring(0,1)=='<')\n"
      "                            str=':'+q[i].oid+' < '+val.substring(1);\n"
      "                        else if((/\\%/).test(val))\n"
      "                            str=':'+q[i].oid+' LIKE \"'+val+'\"';\n"
      "                        else\n"
      "                            str=':'+q[i].oid+'='+'\"'+val+'\"';\n"
      "                        break;\n"
      "                    }\n"
      "                }\n"
      "            if(firstone)\n"
      "                {\n"
      "                statement+=' ('+str+')';\n"
      "                }\n"
      "            else\n"
      "                {\n"
      "                statement+=' '+q.joinstring+' ('+str+')';\n"
      "                }\n"
      "            firstone=false;\n"
      "            }\n"
      "        }\n"
      "    return statement;\n"
      "    }\n",0);


   htrAddScriptFunction(s, "osrc_action_query", "\n"
      "function osrc_action_query(q, formobj)\n"
      "    {\n"
      "    //Is discard ready\n"
      "    //if(!formobj.IsDiscardReady()) return 0;\n"
      "    //Send query as GET request\n"
      "    //this.formobj = formobj;\n"
      "    if(this.pending)\n"
      "        {\n"
      "        alert('There is already a query or movement in progress...');\n"
      "        return 0;\n"
      "        }\n"
      "    //var win=window.open();\n"
      "    //win.document.write(\"<PRE>\"+q+\"</PRE>\");\n"
      "    //win.document.close();\n"
      "    this.easyquery=q;\n"
      "    this.pendingquery=String(escape(q)).replace(\"/\",\"%2F\",\"g\");\n"
      "    this.pending=true;\n"
      "    var someunsaved=false;\n"
      /** Check if any children are modified and call IsDiscardReady if they are **/
      "    for(var i in this.children)\n"
      "         {\n"
      "         if(this.children[i].IsUnsaved)\n"
      "             {\n"
      "             this.children[i]._osrc_ready=false;\n"
      "             this.children[i].IsDiscardReady();\n"
      "             someunsaved=true;\n"
      "             }\n"
      "         else\n"
      "             {\n"
      "             this.children[i]._osrc_ready=true;\n"
      "             }\n"
      "         }\n"
      "    //if someunsaved is false, there were no unsaved forms, so no callbacks\n"
      "    //  so, we'll just fake one using the first form....\n"
      "    if(someunsaved) return 0;\n"
      "    this.QueryContinue(this.children[0]);\n"
      "    }\n",0);

#if 0
   /** JDR - I don't think this function is used anywhere.... **/
   htrAddScriptFunction(s, "osrc_store_replica", "\n"
      "function osrc_store_replica()\n"
      "    {\n"
      "    alert('store replica');\n"
      "    if (this.document.links.length > 1)\n"
      "        {\n"
      "        //Store Links\n"
      "        alert('Store Links');\n"
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
      "        this.form.DataAvailable(data);\n"
      "        }\n"
      "    else\n"
      "        {\n"
      "        //Clear Method - No Links Returned\n"
      "        alert('Clear');\n"
      "        }\n" 
      "    //formobj.DataAvailable();\n"
      "    }\n",0);
#endif   

   htrAddScriptFunction(s, "osrc_action_delete", "\n"
      "function osrc_action_delete()\n"
      "    {\n"
      "    //Delete an object through OSML\n"
      "    this.formobj.ObjectDeleted();\n"
      "    this.formobj.OperationComplete();\n"
      "    return 0;\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_action_create", "\n"
      "function osrc_action_create()\n"
      "    {\n"
      "    //Create an object through OSML\n"
      "    //?ls__mode=osml&ls__req=setattrs'\n"
      "    this.formobj.ObjectCreated();\n"
      "    this.formobj.OperationComplete();\n"
      "    return 0;\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_action_modify", "\n"
      "function osrc_action_modify(up,formobj)\n"
      "    {\n"
      "    //Modify an object through OSML\n"
      "    //aparam[adsf][value];\n"
      "    \n"
      "    //this.src='/?ls__mode=osml&ls__req=setattrs&ls__sid=' + this.sid + '&ls__oid=' + this.oid + '&attrname=valuename&attrname=valuename'\n"
      "    //full_name=MonthThirteen&num_days=1400\n"
      "    var src='/?ls__mode=osml&ls__req=setattrs&ls__sid=' + this.sid + '&ls__oid=' + up.oid;\n"
      "    for(var i in up) if(i!='oid')\n"
      "        {\n"
      "        src+='&'+escape(up[i]['oid'])+'='+escape(up[i]['value']);\n"
      "        }\n"
      "    this.formobj=formobj;\n"
      "    this.modifieddata=up;\n"
      "    this.onload=osrc_action_modify_cb;\n"
      "    this.src=src;\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_action_modify_cb", "\n"
      "function osrc_action_modify_cb()\n"
      "    {\n"
      /** I don't know how to tell if we succeeded....assume we did... **/
      "    if(true)\n"
      "        {\n"
      "        var recnum=this.CurrentRecord;\n"
      "        var cr=this.replica[this.CurrentRecord];\n"
      "        if(cr)\n"
      "            for(var i in this.modifieddata) // update replica\n"
      "                for(var j in cr)\n"
      "                    if(cr[j].oid==this.modifieddata[i].oid)\n"
      "                        cr[j].value=this.modifieddata[i].value;\n"
      "        \n"
      "        this.formobj.OperationComplete(true);\n"
      "        for(var i in this.children)\n"
      "            this.children[i].ObjectModified(recnum);\n"
      "        }\n"
      "    else\n"
      "        {\n"
      "        this.formobj.OperationComplete(false);\n"
      "        }\n"
      "    this.formobj=null;\n"
      "    delete this.modifieddata;\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_cb_query_continue", "\n"
      "function osrc_cb_query_continue(o)\n"
      "    {\n"
      "    //if there is no pending query, don't save the status\n"
      "    //  this is here to protect against form1 vetoing the move, then form2 reporting it is ready to go\n"
      "    if(!this.pending) return 0;\n"
      "    //Current form ready\n"
      "    if(o)\n"
      "    	o._osrc_ready = true;\n"
      "    //If all other forms are ready then go\n"
      "    for(var i in this.children)\n"
      "         {\n"
      "         if(this.children[i]._osrc_ready == false)\n"
      "              {\n"
      "              return 0;\n"
      "              }\n"
      "         }\n"
      "    //everyone looks ready, let's go\n"
      "    if(this.pendingquery) // this could be a movement or a new query....\n"
      "        {  // new query\n"
      "        this.query=this.pendingquery;\n"
      "        this.queryobject=this.pendingqueryobject;\n"
      "        this.orderobject=this.pendingorderobject;\n"
      "        this.pendingquery=null;\n"
      "        this.pendingqueryobject=null;\n"
      "        this.pendingorderobject=null;\n"
      "        for(var i in this.children)\n"
      "             {\n"
      "             this.children[i]._osrc_ready=false;\n"
      "             }\n"

      "        this.TargetRecord=1;\n" /* the record we're aiming for -- go until we get it*/
      "        this.CurrentRecord=1;\n" /* the current record */
      "        this.OSMLRecord=0;\n" /* the last record we got from the OSML */

      /** Clear replica **/
      "        if(this.replica) delete this.replica;\n"
      "        this.replica=new Array();\n"
      "        this.LastRecord=0;\n"
      "        this.FirstRecord=1;\n"
      "        this.moveop=true;\n"

      "        this.OpenSession();\n"
      "        }\n"
      "    else\n"
      "        { // movement\n"
      "        this.MoveToRecordCB(this.RecordToMoveTo);\n"
      "        this.RecordToMoveTo=null;\n"
      "        }\n"
      "    this.pending=false;\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_cb_query_cancel", "\n"
      "function osrc_cb_query_cancel()\n"
      "    {\n"
      "    this.pendingquery = null;\n"
      "    for(var i in this.children)\n"
      "         {\n"
      "         this.children[i]._osrc_ready=false;\n"
      "         }\n"
      "    this.pendingquery=null;\n"
      "    this.pending=false;\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_cb_request_object", "\n"
      "function osrc_cb_request_object(aparam)\n"
      "    {\n"
      "    return 0;\n"
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
      "    //alert('open');\n"
      "    if(this.sid)\n"
      "        {\n"
      "        this.OpenQuery();\n"
      "        }\n"
      "    else\n"
      "        {\n"
      "        this.onload = osrc_open_query;\n"
      "        this.src = '/?ls__mode=osml&ls__req=opensession'\n"
      "        }\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_open_query", "\n"
      "function osrc_open_query()\n"
      "    {\n"
      "    //Open Query\n"
      "    //alert('sid' + this.document.links[0].target);\n"
      "    if(!this.sid) this.sid=this.document.links[0].target;\n"
      "    if(this.qid)\n"
      "        {\n"
      "        this.onload=osrc_open_query;\n"
      "        this.src=\"/?ls__mode=osml&ls__req=queryclose&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid;\n"
      "        this.qid=null;\n"
      "        return 0;\n"
      "        }\n"
      "    this.onload = osrc_get_qid;\n"
      "    this.src=\"/?ls__mode=osml&ls__req=multiquery&ls__sid=\"+this.sid+\"&ls__sql=\" + this.query;\n"
      "    //this.src = '/escape.html';\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_get_qid", "\n"
      "function osrc_get_qid()\n"
      "    {\n"
      "    //return;\n"
      "    //alert('qid ' + this.document.links[0].target);\n"
      "    this.qid=this.document.links[0].target;\n"
      "    for(var i in this.children)\n"
      "        this.children[i].DataAvailable();\n"
      /** should this be a special case for our initialization ?**/
      "    //if(this.init)\n"
      "    //    {\n"
      "        this.init=false;\n"
      "        this.ActionFirst();\n"
      "    //    }\n"
      /** normally don't actually load the data...just let children know that the data is available **/
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_fetch_next", "\n"
      "function osrc_fetch_next()\n"
      "    {\n"
      "    //alert('fetching....');\n"
      "    if(!this.qid)\n"
      "        {\n"
      "        alert('somethign is wrong...');\n"
      "        alert(this.src);\n"
      "        }\n"
      "    var lnk=this.document.links;\n"
      "    var lc=lnk.length;\n"
      "    if(lc < 2)\n"
      "        {\n"	// query over
      "        //this.formobj.OperationComplete();\n" /* don't need this...I think....*/
      "        var qid=this.qid\n"
      "        this.qid=null;\n"
      /* return the last record as the current one if it was our target otherwise, don't */
      "        if(this.moveop)\n"
      "            {\n"
      "            if(this.CurrentRecord>this.LastRecord)\n"
      "                this.CurrentRecord=this.LastRecord;\n"
      "            this.GiveAllCurrentRecord();\n"
      "            }\n"
      "        else\n"
      "            {\n"
      "            this.TellAllReplicaMoved();\n"
      "            }\n" 
      "        this.pending=false;\n"
      "        if(this.doublesync)\n"
      "            this.ActionDoubleSyncCB();\n"
      "        if(qid)\n"
      "            {\n"
      "            this.onload=osrc_close_query;\n"
      "            this.src=\"/?ls__mode=osml&ls__req=queryclose&ls__sid=\"+this.sid+\"&ls__qid=\"+qid;\n"
      "            }\n"
      "        return 0;\n"
      "        }\n"
      "    var row='';\n"
      "    var colnum=0;\n"
      "    for (var i = 1; i < lc; i++)\n"
      "        {\n"
      "        if(row!=lnk[i].target)\n"
      "            {\n"	/** This is a different (or 1st) row of the result set **/
      "            row=lnk[i].target;\n"
      "            this.OSMLRecord++;\n" /* this holds the last record we got, so now will hold current record number */
      "            this.replica[this.OSMLRecord]=new Array();\n"
      "            this.replica[this.OSMLRecord].oid=row;\n"
      "            if(this.LastRecord<this.OSMLRecord)\n"
      "                {\n"
      "                this.LastRecord=this.OSMLRecord;\n"
      "                while(this.LastRecord-this.FirstRecord>=this.replicasize)\n"
      "                    {\n" /* clean up replica */
      "                    delete this.replica[this.FirstRecord];\n"
      "                    this.FirstRecord++;\n"
      "                    }\n"
      "                }\n"
      "            if(this.FirstRecord>this.OSMLRecord)\n"
      "                {\n"
      "                this.FirstRecord=this.OSMLRecord;\n"
      "                while(this.LastRecord-this.FirstRecord>=this.replicasize)\n"
      "                    {\n" /* clean up replica */
      "                    delete this.replica[this.LastRecord];\n"
      "                    this.LastRecord--;\n"
      "                    }\n"
      "                }\n"
      "            colnum=0;\n"
      "            //alert('New row: '+row+'('+this.OSMLRecord+')');\n"
      "            }\n"
      "        colnum++;\n"
      "        this.replica[this.OSMLRecord][colnum] = new Array();\n"
      "        this.replica[this.OSMLRecord][colnum]['value'] = lnk[i].text\n"
      "        this.replica[this.OSMLRecord][colnum]['type'] = lnk[i].hash.substr(1);\n"
      "        this.replica[this.OSMLRecord][colnum]['oid'] = lnk[i].host;\n"
      "        }\n"
      /** make sure we bring this.LastRecord back down to the top of our replica...**/
      "    while(!this.replica[this.LastRecord])\n"
      "        this.LastRecord--;\n"
      "    if(this.LastRecord<this.TargetRecord)\n"
      "        {\n" /* We're going farther down this... */
      "        this.onload = osrc_fetch_next;\n"
      "        this.src=\"/?ls__mode=osml&ls__req=queryfetch&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid+\"&ls__objmode=0&ls__rowcount=\"+this.readahead;\n"
      "        }\n"
      "    else\n"
      "        {\n" /* we've got the one we need */
      "        if((this.LastRecord-this.FirstRecord+1)<this.replicasize)\n"
      "            {\n" /* make sure we have a full replica if possible */
      "            this.onload = osrc_fetch_next;\n"
      "            this.src=\"/?ls__mode=osml&ls__req=queryfetch&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid+\"&ls__objmode=0&ls__rowcount=\"+(this.replicasize-(this.LastRecord-this.FirstRecord+1));\n"
      "            }\n"
      "        else\n"
      "            {\n"
      "            if(this.doublesync)\n"
      "                this.ActionDoubleSyncCB();\n"
      "            if(this.moveop)\n"
      "                this.GiveAllCurrentRecord();\n"
      "            else\n"
      "                this.TellAllReplicaMoved();\n"
      "            this.pending=false;\n"
      "            }\n"
      "        }\n"
      "    }\n",0);
      
   htrAddScriptFunction(s, "osrc_close_query", "\n"
      "function osrc_close_query()\n"
      "    {\n"
      "    //Close Query\n"
      "    this.qid=null;\n"
      "    this.onload=null;\n"
      "    //this.onload = osrc_close_session;\n"
      "    //this.src = '/?ls__mode=osml&ls__req=queryclose&ls__qid=' + this.qid;\n"
      "    }\n",0);
 
   htrAddScriptFunction(s, "osrc_close_object", "\n"
      "function osrc_close_object()\n"
      "    {\n"
      "    //Close Object\n"
      "    this.onload = osrc_close_session;\n"
      "    this.src = '/?ls__mode=osml&ls__req=close&ls__oid=' + this.oid;\n"
      "    }\n",0);
 
   htrAddScriptFunction(s, "osrc_close_session", "\n"
      "function osrc_close_session()\n"
      "    {\n"
      "    //Close Session\n"
      "    this.onload = null;\n"
      "    this.src = '/?ls__mode=osml&ls__req=closesession&ls__sid=' + this.sid;\n"
      "    this.qid=null;\n"
      "    this.sid=null;\n"
      "    }\n",0);

      
   htrAddScriptFunction(s, "osrc_move_first", "\n"
      "function osrc_move_first(formobj)\n"
      "    {\n"
      "    this.MoveToRecord(1);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_give_all_current_record", "\n"
      "function osrc_give_all_current_record()\n"
      "    {\n"
      "    //confirm('give_all_current_record start');\n"
      "    for(var i in this.children)\n"
      "        this.children[i].ObjectAvailable(this.replica[this.CurrentRecord]);\n"
      "    //confirm('give_all_current_record done');\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_tell_all_replica_moved", "\n"
      "function osrc_tell_all_replica_moved()\n"
      "    {\n"
      "    //confirm('tell_all_replica_moved start');\n"
      "    for(var i in this.children)\n"
      "        if(this.children[i].ReplicaMoved)\n"
      "            this.children[i].ReplicaMoved();\n"
      "    //confirm('tell_all_replica_moved done');\n"
      "    }\n",0);


   htrAddScriptFunction(s, "osrc_move_to_record", "\n"
      "function osrc_move_to_record(recnum)\n"
      "    {\n"
      "    if(recnum<1)\n"
      "        {\n"
      "        alert(\"Can't move past beginning.\");\n"
      "        return 0;\n"
      "        }\n"
      "    if(this.pending)\n"
      "        {\n"
      "        //alert('you got ahead');\n"
      "        return 0;\n"
      "        }\n"
      "    this.pending=true;\n"
      "    var someunsaved=false;\n"
      "    this.RecordToMoveTo=recnum;\n"
      "    for(var i in this.children)\n"
      "         {\n"
      "         if(this.children[i].IsUnsaved)\n"
      "             {\n"
      "             //alert('child: '+i+' : '+this.children[i].IsUnsaved+' isn\\'t saved...IsDiscardReady');\n"
      "             this.children[i]._osrc_ready=false;\n"
      "             this.children[i].IsDiscardReady();\n"
      "             someunsaved=true;\n"
      "             }\n"
      "         else\n"
      "             {\n"
      "             this.children[i]._osrc_ready=true;\n"
      "             }\n"
      "         }\n"
      "    //if someunsaved is false, there were no unsaved forms, so no callbacks\n"
      "    //  we can just continue\n"
      "    if(someunsaved) return 0;\n"
      "    this.MoveToRecordCB(recnum);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_move_to_record_cb", "\n"
      "function osrc_move_to_record_cb(recnum)\n"
      "    {\n"
      "    //confirm(recnum);\n"
      "    this.moveop=true;\n"
      "    if(recnum<1)\n"
      "        {\n"
      "        alert(\"Can't move past beginning.\");\n"
      "        return 0;\n"
      "        }\n"
      "    this.RecordToMoveTo=recnum;\n" 
      "    for(var i in this.children)\n"
      "         {\n"
      "         if(this.children[i].IsUnsaved)\n"
      "             {\n"
      "             //confirm('child: '+i+' : '+this.children[i].IsUnsaved+' isn\\'t saved...');\n"
      "             return 0;\n"
      "             }\n"
      "         }\n"
/* If we're here, we're ready to go */
      "    this.TargetRecord=this.CurrentRecord=recnum;\n"
      "    if(this.CurrentRecord <= this.LastRecord && this.CurrentRecord >= this.FirstRecord)\n"
      "        {\n"
      "        this.GiveAllCurrentRecord();\n"
      "        this.pending=false;\n"
      "        return 1;\n"
      "        }\n"
      "    else\n"
      "        {\n"
      "        if(this.CurrentRecord < this.FirstRecord)\n"
      "            {\n" /* data is further back, need new query */
      "            if(this.FirstRecord-this.CurrentRecord<this.readahead)\n"
      "                {\n"
      "                this.startat=(this.FirstRecord-this.readahead)>0?(this.FirstRecord-this.readahead):1;\n"
      "                }\n"
      "            else\n"
      "                {\n"
      "                this.startat=this.CurrentRecord;\n"
      "                }\n"
      "            if(this.qid)\n"
      "                {\n"
      "                this.onload=osrc_open_query_startat;\n"
      "                this.src=\"/?ls__mode=osml&ls__req=queryclose&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid;\n"
      "                }\n"
      "            else\n"
      "                {\n"
      "                this.onload=osrc_open_query_startat;\n"
      "                this.onload();\n"
      "                }\n"
      "            return 0;\n"
      "            }\n"
      "        else\n"
      "            {\n" /* data is farther on, act normal */
      "            if(this.qid)\n"
      "                {\n"
      "                this.onload=osrc_fetch_next;\n"
      "                if(this.CurrentRecord == Number.MAX_VALUE)\n"
      "                    this.src=\"/?ls__mode=osml&ls__req=queryfetch&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid+\"&ls__objmode=0\";\n" /* rowcount defaults to a really high number if not set */
      "                else\n"
      "                    this.src=\"/?ls__mode=osml&ls__req=queryfetch&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid+\"&ls__objmode=0&ls__rowcount=\"+this.readahead;\n"
      "                }\n"
      "            else\n"
      "                {\n"
      "                this.pending=false;\n"
      "                this.CurrentRecord=this.LastRecord;\n"
      "                this.GiveAllCurrentRecord();\n"
      "                }\n"
      "            return 0;\n"
      "            }\n"
      "        }\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_open_query_startat", "\n"
      "function osrc_open_query_startat()\n"
      "    {\n"
      "    this.onload = osrc_get_qid_startat;\n"
      "    this.src=\"/?ls__mode=osml&ls__req=multiquery&ls__sid=\"+this.sid+\"&ls__sql=\" + this.query;\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_get_qid_startat", "\n"
      "function osrc_get_qid_startat()\n"
      "    {\n"
      "    this.qid=this.document.links[0].target;\n"
      "    this.OSMLRecord=this.startat-1;\n"
      "    this.onload=osrc_fetch_next;\n"
      "    //this.FirstRecord=this.startat;\n"
      "    if(this.startat-this.TargetRecord+1<this.replicasize)\n"
      "        {\n"
      "        this.src='/?ls__mode=osml&ls__req=queryfetch&ls__sid='+this.sid+'&ls__qid='+this.qid+'&ls__objmode=0&ls__rowcount='+(this.TargetRecord-this.startat+1)+'&ls__startat='+this.startat;\n"
      "        }\n"
      "    else\n"
      "        {\n"
      "        this.src='/?ls__mode=osml&ls__req=queryfetch&ls__sid='+this.sid+'&ls__qid='+this.qid+'&ls__objmode=0&ls__rowcount='+this.replicasize+'&ls__startat='+this.startat;\n"
      "        }\n"
      "    this.startat=null;\n"
      "    }\n",0);


   htrAddScriptFunction(s, "osrc_move_next", "\n"
      "function osrc_move_next(formobj)\n"
      "    {\n"
      "    this.MoveToRecord(this.CurrentRecord+1);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_move_prev", "\n"
      "function osrc_move_prev(formobj)\n"
      "    {\n"
      "    this.MoveToRecord(this.CurrentRecord-1);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_move_last", "\n"
      "function osrc_move_last(formobj)\n"
      "    {\n"
      "    this.MoveToRecord(Number.MAX_VALUE);\n" /* FIXME */
      "    //alert(\"do YOU know where the end is? I sure don't.\");\n"
      "    }\n",0);


   htrAddScriptFunction(s, "osrc_scroll_prev", "\n"
      "function osrc_scroll_prev()\n"
      "    {\n"
      "    if(this.FirstRecord!=1) this.ScrollTo(this.FirstRecord-1);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_scroll_next", "\n"
      "function osrc_scroll_next()\n"
      "    {\n"
      "    this.ScrollTo(this.LastRecord+1);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_scroll_prev_page", "\n"
      "function osrc_scroll_prev_page()\n"
      "    {\n"
      "    this.ScrollTo(this.FirstRecord>this.replicasize?this.FirstRecord-this.replicasize:1);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_scroll_next_page", "\n"
      "function osrc_scroll_next_page()\n"
      "    {\n"
      "    this.ScrollTo(this.LastRecord+this.replicasize);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_scroll_to", "\n"
      "function osrc_scroll_to(recnum)\n"
      "    {\n"
      "    this.moveop=false;\n"
      "    this.TargetRecord=recnum;\n"
      "    if(this.TargetRecord <= this.LastRecord && this.TargetRecord >= this.FirstRecord)\n"
      "        {\n"
      "        this.TellAllReplicaMoved();\n"
      "        this.pending=false;\n"
      "        return 1;\n"
      "        }\n"
      "    else\n"
      "        {\n"
      "        if(this.TargetRecord < this.FirstRecord)\n"
      "            {\n" /* data is further back, need new query */
      "            if(this.FirstRecord-this.TargetRecord<this.scrollahead)\n"
      "                {\n"
      "                this.startat=(this.FirstRecord-this.scrollahead)>0?(this.FirstRecord-this.scrollahead):1;\n"
      "                }\n"
      "            else\n"
      "                {\n"
      "                this.startat=this.TargetRecord;\n"
      "                }\n"
      "            this.onload=osrc_open_query_startat;\n"
      "            if(this.qid)\n"
      "                {\n"
      "                this.src=\"/?ls__mode=osml&ls__req=queryclose&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid;\n"
      "                }\n"
      "            else\n"
      "                {\n"
      "                this.onload();\n"
      "                }\n"
      "            return 0;\n"
      "            }\n"
      "        else\n"
      "            {\n" /* data is farther on, act normal */
      "            if(this.qid)\n"
      "                {\n"
      "                this.onload=osrc_fetch_next;\n"
      "                if(this.TargetRecord == Number.MAX_VALUE)\n"
      "                    this.src=\"/?ls__mode=osml&ls__req=queryfetch&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid+\"&ls__objmode=0\";\n" /* rowcount defaults to a really high number if not set */
      "                else\n"
      "                    this.src=\"/?ls__mode=osml&ls__req=queryfetch&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid+\"&ls__objmode=0&ls__rowcount=\"+this.scrollahead;\n"
      "                }\n"
      "            else\n"
      "                {\n"
      "                this.pending=false;\n"
      "                this.TargetRecord=this.LastRecord;\n"
      "                this.TellAllReplicaMoved();\n"
      "                }\n"
      "            return 0;\n"
      "            }\n"
      "        }\n"
      "    }\n",0);


   htrAddScriptFunction(s, "osrc_cleanup", "\n"
      "function osrc_cleanup()\n"
      "    {\n"
      /** this last-second page load is screwing something up... **/
      /**   sometimes it will cause a blank page to be loaded, others it's a 'bus error' crash **/
      "    return 0;\n" 
      "    if(this.qid)\n"
      "        {\n" /* why does the browser load a blank page when you try to move away? */
      "        this.onLoad=null;\n"
      "        this.src=\"/?ls__mode=osml&ls__req=queryclose&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid;\n"
      "        this.qid=null\n"
      "        }\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_action_sync", "\n"
      "function osrc_action_sync(param)\n"
      "    {\n"
      "    this.parentosrc=param.ParentOSRC;\n"
      "    this.ParentKey=new Array();\n"
      "    this.ChildKey=new Array();\n"
      "    var query = new Array();\n"
      "    query.oid=null;\n"
      "    query.joinstring='AND';\n"
      "    var p=this.parentosrc.CurrentRecord;\n"
      "    for(var i=1;i<10;i++)\n"
      "        {\n"
      "        this.ParentKey[i]=eval('param.ParentKey'+i);\n"
      "        this.ChildKey[i]=eval('param.ChildKey'+i);\n"
      "        if(this.ParentKey[i])\n"
      "            {\n"
      "            for(var j in this.parentosrc.replica[p])\n"
      "                {\n"
      "                if(this.parentosrc.replica[p][j].oid==this.ParentKey[i])\n"
      "                    {\n"
      "                    var t = new Object();\n"
      "                    t.oid=this.ChildKey[i];\n"
      "                    t.value=this.parentosrc.replica[p][j].value;\n"
      "                    t.type=this.parentosrc.replica[p][j].type;\n"
      "                    query.push(t);\n"
      "                    }\n"
      "                }\n"
      "            }\n"
      "        }\n"
      "    this.ActionQueryObject(query,null);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_action_double_sync", "\n"
      "function osrc_action_double_sync(param)\n"
      "    {\n"
      "    this.doublesync=true;\n"
      "    this.replicasize=100;\n"
      "    this.readahead=100;\n"
      "    this.parentosrc=param.ParentOSRC;\n"
      "    this.childosrc=param.ChildOSRC;\n"
      "    this.ParentKey=new Array();\n"
      "    this.ParentSelfKey=new Array();\n"
      "    this.SelfChildKey=new Array();\n"
      "    this.ChildKey=new Array();\n"
      "    var query = new Array();\n"
      "    query.oid=null;\n"
      "    query.joinstring='AND';\n"
      "    var p=this.parentosrc.CurrentRecord;\n"
      "    for(var i=1;i<10;i++)\n"
      "        {\n"
      "        this.ParentKey[i]=eval('param.ParentKey'+i);\n"
      "        this.ParentSelfKey[i]=eval('param.ParentSelfKey'+i);\n"
      "        this.SelfChildKey[i]=eval('param.SelfChildKey'+i);\n"
      "        this.ChildKey[i]=eval('param.ChildKey'+i);\n"
      "        if(this.ParentKey[i])\n"
      "            {\n"
      "            for(var j in this.parentosrc.replica[p])\n"
      "                {\n"
      "                if(this.parentosrc.replica[p][j].oid==this.ParentKey[i])\n"
      "                    {\n"
      "                    var t = new Object();\n"
      "                    t.oid=this.ParentSelfKey[i];\n"
      "                    t.value=this.parentosrc.replica[p][j].value;\n"
      "                    t.type=this.parentosrc.replica[p][j].type;\n"
      "                    query.push(t);\n"
      "                    }\n"
      "                }\n"
      "            }\n"
      "        }\n"
      "    this.ActionQueryObject(query,null);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_action_double_sync_cb", "\n"
      "function osrc_action_double_sync_cb()\n"
      "    {\n"
      "    var query = new Array();\n"
      "    query.oid=null;\n"
      "    query.joinstring='OR';\n"
      "    if(this.LastRecord==0)\n"
      "        {\n"
      "        var t = new Object();\n"
      "        t.oid=1;\n"
      "        t.value=2;\n"
      "        t.type='integer';\n"
      "        query.push(t);\n"
      "        }\n"
      "    for(var p=this.FirstRecord; p<=this.LastRecord; p++)\n"
      "        {\n"
      "        var q = new Array();\n"
      "        q.oid=null;\n"
      "        q.joinstring='AND';\n"
      "        for(var i=1;i<10;i++)\n"
      "            {\n"
      "            if(this.SelfChildKey[i])\n"
      "                {\n"
      "                for(var j in this.replica[p])\n"
      "                    {\n"
      "                    if(this.replica[p][j].oid==this.SelfChildKey[i])\n"
      "                        {\n"
      "                        var t = new Object();\n"
      "                        t.oid=this.ChildKey[i];\n"
      "                        t.value=this.replica[p][j].value;\n"
      "                        t.type=this.replica[p][j].type;\n"
      "                        q.push(t);\n"
      "                        }\n"
      "                    }\n"
      "                }\n"
      "            }\n"
      "        query.push(q);\n"
      "        }\n"
      "    \n"
      "    this.doublesync=false;\n"
      "    this.childosrc.ActionQueryObject(query,null);\n"
      "    }\n",0);



/**  OSRC Initializer **/
   htrAddScriptFunction(s, "osrc_init", "\n"
      "function osrc_init(loader,ra,sa,rs,sql,filter)\n"
      "    {\n"
      "    loader.readahead=ra;\n"
      "    loader.scrollahead=sa;\n"
      "    loader.replicasize=rs;\n"
      "    loader.sql=sql;\n"
      "    loader.filter=filter;\n"
      "    \n"
      "    loader.GiveAllCurrentRecord=osrc_give_all_current_record;\n"
      "    loader.MoveToRecord=osrc_move_to_record;\n"
      "    loader.MoveToRecordCB=osrc_move_to_record_cb;\n"
      "    loader.children = new Array();\n"
      "    //loader.ActionClear=osrc_action_clear;\n"
      "    loader.ActionQuery=osrc_action_query;\n"
      "    loader.ActionQueryObject=osrc_action_query_object;\n"
      "    loader.ActionOrderObject=osrc_action_order_object;\n"
      "    loader.ActionDelete=osrc_action_delete;\n"
      "    loader.ActionCreate=osrc_action_create;\n"
      "    loader.ActionModify=osrc_action_modify;\n"

      "    loader.OpenSession=osrc_open_session;\n"
      "    loader.OpenQuery=osrc_open_query;\n"
      "    loader.CloseQuery=osrc_close_query;\n"
      "    loader.CloseObject=osrc_close_object;\n"
      "    loader.CloseSession=osrc_close_session;\n"
      /**"    loader.StoreReplica=osrc_store_replica;\n"**/
      "    loader.QueryContinue = osrc_cb_query_continue;\n"
      "    loader.QueryCancel = osrc_cb_query_cancel;\n"
      "    loader.RequestObject = osrc_cb_request_object;\n"
      "    loader.Register = osrc_cb_register;\n"
  
      "    loader.ActionFirst = osrc_move_first;\n"
      "    loader.ActionNext = osrc_move_next;\n"
      "    loader.ActionPrev = osrc_move_prev;\n"
      "    loader.ActionLast = osrc_move_last;\n"

      "    loader.ScrollTo = osrc_scroll_to;\n"
      "    loader.ScrollPrev = osrc_scroll_prev;\n"
      "    loader.ScrollNext = osrc_scroll_next;\n"
      "    loader.ScrollPrevPage = osrc_scroll_prev_page;\n"
      "    loader.ScrollNextPage = osrc_scroll_next_page;\n"

      "    loader.TellAllReplicaMoved = osrc_tell_all_replica_moved;\n"

      "    loader.ActionSync = osrc_action_sync;\n"
      "    loader.ActionDoubleSync = osrc_action_double_sync;\n"
      "    loader.ActionDoubleSyncCB = osrc_action_double_sync_cb;\n"
      "    loader.InitQuery = osrc_init_query;\n"
      "    loader.cleanup = osrc_cleanup;\n"
      
      "    return loader;\n"
      "    }\n", 0);


   /** Script initialization call. **/
   htrAddScriptInit_va(s,"    %s=osrc_init(%s.layers.osrc%dloader,%i,%i,%i,'%s','%s');\n",
	 name,parentname, id,readahead,scrollahead,replicasize,sql,filter);
   //htrAddScriptCleanup_va(s,"    %s.layers.osrc%dloader.cleanup();\n", parentname, id);

   htrAddScriptInit_va(s,"    %s.oldosrc=osrc_current;\n",name);
   htrAddScriptInit_va(s,"    osrc_current=%s;\n",name);

   /** HTML body <DIV> element for the layers. **/
   htrAddBodyItem_va(s,"    <DIV ID=\"osrc%dloader\"></DIV>\n",id);
   
   qy = objOpenQuery(w_obj,"",NULL,NULL,NULL);
   if (qy)
   {
   while((sub_w_obj = objQueryFetch(qy, O_RDONLY)))
       {
       objGetAttrValue(sub_w_obj, "outer_type", POD(&ptr));
       if (strcmp(ptr,"widget/connector") == 0)
	   htrRenderWidget(s, sub_w_obj, z, "", name);
       else
	   htrRenderWidget(s, sub_w_obj, z, parentname, parentobj);
       objClose(sub_w_obj);
       }
   objQueryClose(qy);
   }
   
   /** We set osrc_current=null so that orphans can't find us  **/
   htrAddScriptInit(s, "    osrc_current.InitQuery();\n");
   htrAddScriptInit_va(s,"    osrc_current=%s.oldosrc;\n\n",name);


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

   htrAddAction(drv,"Sync");
   htrAddAction(drv,"ReverseSync");

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
