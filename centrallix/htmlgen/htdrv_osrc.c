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

    $Id: htdrv_osrc.c,v 1.17 2002/04/25 23:02:52 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/htmlgen/htdrv_osrc.c,v $

    $Log: htdrv_osrc.c,v $
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
   int replicasize;

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

   /** try to catch mistakes that would probably make Netscape REALLY buggy... **/
   if(replicasize==1 && readahead==0) readahead=1;
   if(readahead>replicasize) replicasize=readahead;
   if(replicasize<1 || readahead<1)
      {
      mssError(1,"HTOSRC","You must give positive values for replicasize and readahead");
      return -1;
      }


   /** Write named global **/
   nptr = (char*)nmMalloc(strlen(name)+1);
   strcpy(nptr,name);

   /** create our instance variable **/
   //htrAddScriptGlobal(s, nptr, "null",HTR_F_NAMEALLOC); 

  /** Ok, write the style header items. **/
  snprintf(sbuf3, 200, "    <STYLE TYPE=\"text/css\">\n");
  htrAddHeaderItem(s,sbuf3);
  snprintf(sbuf3, 200, "\t#osrc%dloader { POSITION:absolute; VISIBILITY:hidden; LEFT:0; TOP:300;  WIDTH:500; HEIGHT:500; Z-INDEX:-20; }\n",id);
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
      "function osrc_action_modify(aparam,formobj)\n"
      "    {\n"
      "    //Modify an object through OSML\n"
      "    //aparam[adsf][value];\n"
      "    \n"
      "    //this.src='/?ls__mode=osml&ls__req=setattrs&ls__sid=' + this.sid + '&ls__oid=' + this.oid + '&attrname=valuename&attrname=valuename'\n"
      "    //full_name=MonthThirteen&num_days=1400\n"
      "    var src='/?ls__mode=osml&ls__req=setattrs&ls__sid=' + this.sid + '&ls__oid=' + this.replica[this.CurrentRecord].oid;\n"
      "    for(var i in aparam)\n"
      "        {\n"
      "        src+='&'+escape(i)+'='+escape(aparam[i]);\n"
      "        }\n"
      "    this.formobj=formobj;\n"
      "    this.modifieddata=aparam;\n"
      "    this.onload=osrc_action_modify_cb;\n"
      "    this.src=src;\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_action_modify_cb", "\n"
      "function osrc_action_modify_cb()\n"
      "    {\n"
      "    this.formobj.ObjectModified();\n"
      /** I don't know how to tell if we succeeded....assume we did... **/
      "    if(true)\n"
      "        {\n"
      "        var cr=this.replica[this.CurrentRecord];\n"
      "        for(var i in this.modifieddata) // update replica\n"
      "            for(var j in cr)\n"
      "                if(cr[j].oid==i)\n"
      "                    cr[j].value=this.modifieddata[i];\n"
      "        \n"
      "        this.formobj.OperationComplete(true);\n"
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
      "    o._osrc_ready = true;\n"
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
      "        this.pendingquery=null;\n"
      "        for(var i in this.children)\n"
      "             {\n"
      "             this.children[i]._osrc_ready=false;\n"
      "             }\n"

      "        this.CurrentRecord=0;\n" /* the current record */
      "        this.OSMLRecord=0;\n" /* the last record we got from the OSML */

      /** Clear replica **/
      "        if(this.replica) delete this.replica;\n"
      "        this.replica=new Array();\n"
      "        this.LastRecord=0;\n"
      "        this.FirstRecord=0;\n"

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
      "    this.onload = osrc_open_query;\n"
      "    this.src = '/?ls__mode=osml&ls__req=opensession'\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_open_query", "\n"
      "function osrc_open_query()\n"
      "    {\n"
      "    //Open Query\n"
      "    //alert('sid' + this.document.links[0].target);\n"
      "    this.sid=this.document.links[0].target;\n"
      "    this.onload = osrc_get_qid;\n"
      "    this.src=\"/?ls__mode=osml&ls__req=multiquery&ls__sid=\"+this.sid+\"&ls__sql=\" + this.query;\n"
      "    //this.src = '/escape.html';\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_get_qid", "\n"
      "function osrc_get_qid()\n"
      "    {\n"
      "    //alert('qid ' + this.document.links[0].target);\n"
      "    this.qid=this.document.links[0].target;\n"
      "    for(var i in this.children)\n"
      "        this.children[i].DataAvailable();\n"
      /** Don't actually load the data...just let children know that the data is available **/
      "    //this.onload = osrc_fetch_next;\n"
      "    //this.src=\"/?ls__mode=osml&ls__req=queryfetch&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid+\"&ls__objmode=0&ls__rowcount=1\";\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_fetch_next", "\n"
      "function osrc_fetch_next()\n"
      "    {\n"
      "    //alert('fetching....');\n"
      "    var lnk=this.document.links;\n"
      "    var lc=lnk.length;\n"
      "    if(lc < 2)\n"
      "        {\n"	// query over
      "        //this.formobj.OperationComplete();\n" /* don't need this...I think....*/
      "        this.CurrentRecord=this.LastRecord;\n" /* return the last record as the current one */
      "        this.GiveAllCurrentRecord();\n"
      "        this.pending=false;\n"
      "        if(this.qid)\n"
      "            {\n"
      "            this.onload=osrc_close_query;\n"
      "            this.src=\"/?ls__mode=osml&ls__req=closequery&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid;\n"
      "            }\n"
      "        this.pending=false;\n"
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
      "    if(this.LastRecord<this.CurrentRecord)\n"
      "        {\n" /* We're going farther down this... */
      "        this.onload = osrc_fetch_next;\n"
      "        this.src=\"/?ls__mode=osml&ls__req=queryfetch&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid+\"&ls__objmode=0&ls__rowcount=\"+this.readahead;\n"
      "        }\n"
      "    else\n"
      "        {\n" /* we've got the one we need */
      "        this.GiveAllCurrentRecord();\n"
      "        this.pending=false;\n"
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
      "    for(var i in this.children)\n"
      "        this.children[i].ObjectAvailable(this.replica[this.CurrentRecord]);\n"
      "    }\n",0);

   htrAddScriptFunction(s, "osrc_move_to_record", "\n"
      "function osrc_move_to_record(recnum)\n"
      "    {\n"
      "    //confirm(recnum);\n"
      "    if(recnum<1)\n"
      "        {\n"
      "        alert(\"Can't move past beginning.\");\n"
      "        return 0;\n"
      "        }\n"
      "    if(this.pending)\n"
      "        {\n"
      "        alert('you got ahead');\n"
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
      "    this.CurrentRecord=recnum;\n"
      "    //alert(this.CurrentRecord);\n"
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
      "                this.startat=this.FirstRecord-this.readahead;\n"
      "                }\n"
      "            else\n"
      "                {\n"
      "                this.startat=this.CurrentRecord;\n"
      "                }\n"
      "            this.onload=osrc_open_query_startat;\n"
      "            this.src=\"/?ls__mode=osml&ls__req=closequery&ls__sid=\"+this.sid+\"&ls__qid=\"+this.qid;\n"
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
      "    this.src='/?ls__mode=osml&ls__req=queryfetch&ls__sid='+this.sid+'&ls__qid='+this.qid+'&ls__objmode=0&ls__rowcount='+this.readahead+'&ls__startat='+this.startat;\n"
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


/**  OSRC Initializer **/
   htrAddScriptFunction(s, "osrc_init", "\n"
      "function osrc_init(loader,ra,rs)\n"
      "    {\n"
      "    loader.readahead=ra;\n"
      "    loader.replicasize=rs;\n"
      "    loader.GiveAllCurrentRecord=osrc_give_all_current_record;\n"
      "    loader.MoveToRecord=osrc_move_to_record;\n"
      "    loader.MoveToRecordCB=osrc_move_to_record_cb;\n"
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
      /**"    loader.StoreReplica=osrc_store_replica;\n"**/
      "    loader.QueryContinue = osrc_cb_query_continue;\n"
      "    loader.QueryCancel = osrc_cb_query_cancel;\n"
      "    loader.RequestObject = osrc_cb_request_object;\n"
      "    loader.Register = osrc_cb_register;\n"
  
      "    loader.ActionFirst = osrc_move_first;\n"
      "    loader.ActionNext = osrc_move_next;\n"
      "    loader.ActionPrev = osrc_move_prev;\n"
      "    loader.ActionLast = osrc_move_last;\n"
      
      "    return loader;\n"
      "    }\n", 0);


   /** Script initialization call. **/
   snprintf(sbuf3, 200, "osrc_current=osrc_init(%s.layers.osrc%dloader,%i,%i);\n", parentname, id,readahead,replicasize);
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
