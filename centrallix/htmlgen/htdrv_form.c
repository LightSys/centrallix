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
/* Module: 	htdrv_form.c      					*/
/* Author:	Jonathan Rupp (JDR) and Aaron Williams 			*/
/* Creation:	February 20, 2002 					*/
/* Description:	This is the non-visual widget that interfaces the 	*/
/*		objectsource widget and the visual sub-widgets		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Log: htdrv_form.c,v $
    Revision 1.7  2002/03/02 21:57:00  jorupp
    * Editbox supports as many</>/=/<=/>=/<=> clauses you can fit to query for data
        (<=> is the LIKE operator)

    Revision 1.6  2002/03/02 03:06:50  jorupp
    * form now has basic QBF functionality
    * fixed function-building problem with radiobutton
    * updated checkbox, radiobutton, and editbox to work with QBF
    * osrc now claims it's global name

    Revision 1.5  2002/02/27 02:37:19  jorupp
    * moved editbox I-beam movement functionality to function
    * cleaned up form, added comments, etc.

    Revision 1.4  2002/02/23 19:35:28  lkehresman
    * Radio button widget is now forms aware.
    * Fixes a couple of oddities in the checkbox.
    * Fixed some formatting issues in the form.

    Revision 1.3  2002/02/23 04:28:29  jorupp
    bug fixes in form, I-bar in editbox is reset on a setvalue()

    Revision 1.2  2002/02/22 23:48:39  jorupp
    allow editbox to work without form, form compiles, doesn't do much

    Revision 1.1  2002/02/21 18:15:14  jorupp
    Initial commit of form widget


 **END-CVSDATA***********************************************************/

/** globals **/
static struct 
    {
    int		idcnt;
    }
    HTFORM;


/*** htformVerify - not written yet.
 ***/
int
htformVerify()
    {
    return 0;
    }


/*** htformRender - generate the HTML code for the form 'glue'
 ***/
int
htformRender(pHtSession s, pObject w_obj, int z, char* parentname, char* parentobj)
    {
    char* ptr;
    char name[64];
    char tabmode[64];
    //char sbuf[200];
    //char sbuf2[160];
    char *sbuf3;
    int id;
    char* nptr;
    char basequery[300];
    char basewhere[300];
    int allowquery, allownew, allowmodify, allowview, allownodata, multienter;
    
    	/** Get an id for this. **/
	id = (HTFORM.idcnt++);

	/** Get params. **/
	if (objGetAttrValue(w_obj,"AllowQuery",POD(&allowquery)) != 0) 
	    allowquery=1;
	if (objGetAttrValue(w_obj,"AllowNew",POD(&allownew)) != 0) 
	    allownew=1;
	if (objGetAttrValue(w_obj,"AllowModify",POD(&allowmodify)) != 0) 
	    allowmodify=1;
	if (objGetAttrValue(w_obj,"AllowView",POD(&allowview)) != 0) 
	    allowview=1;
	if (objGetAttrValue(w_obj,"AllowNoData",POD(&allownodata)) != 0) 
	    allownodata=1;
	/** The way I read the specs -- overriding this resides in 
	 **   the code, not here -- JDR **/
	if (objGetAttrValue(w_obj,"MultiEnter",POD(&multienter)) != 0) 
	    multienter=0;
	if (objGetAttrValue(w_obj,"TabMode",POD(tabmode)) != 0) 
	    tabmode[0]='\0';

	if (objGetAttrValue(w_obj,"basequery",POD(&ptr)) == 0)
	    sprintf(basequery,"%.300s",ptr);
	else
	    strcpy(basequery,"");
	
	if (objGetAttrValue(w_obj,"basewhere",POD(&ptr)) == 0)
	    sprintf(basewhere,"%.300s",ptr);
	else
	    strcpy(basewhere,"");

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	strcpy(name,ptr);

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);

	/** create our instance variable **/
	htrAddScriptGlobal(s, nptr, "null",HTR_F_NAMEALLOC); 

	htrAddScriptFunction(s, "form_cb_register", "\n"
		"function form_cb_register(aparam)\n"
		"    {\n"
		"    this.elements.push(aparam);\n"
		"    }\n", 0);

	/** A child 'control' has changed it's data **/
	htrAddScriptFunction(s, "form_cb_data_notify", "\n"
		"function form_cb_data_notify(control)\n"
		"    {\n"
		"	this.IsUnsaved=true;\n"
		"       control.IsChanged=true;\n"
		"    }\n", 0);

	/** A child 'control' got or lost focus **/
	htrAddScriptFunction(s, "form_cb_focus_notify", "\n"
		"function form_cb_focus_notify(control)\n"
		"    {\n"
		"    }\n", 0);

	/** the tab key was pressed in child 'control' **/
	htrAddScriptFunction(s, "form_cb_tab_notify", "\n"
		"function form_cb_tab_notify(control)\n"
		"    {\n"
		"    var ctrlnum;\n"
		"    for(var i in this.elements)\n"
		"        {\n"
		"        if(this.elements[i].name=control.name)\n"
		"            {\n"
		"            ctrlnum=i;\n"
		"            }\n"
		"        }\n"
		"    if(this.elements[ctrlnum+1])\n"
		"        {\n"
		"        \n" 	/* bounce the tab up the call tree */
		"        } else {\n"
		"        \n"			/* set the focus */
		"        }\n"
		"    }\n", 0);

	/** Objectsource says there's data ready **/
	htrAddScriptFunction(s, "form_cb_data_available", "\n"
		"function form_cb_data_available(aparam)\n"
		"    {\n"
		"    }\n", 0);

	/** Objectsource wants us to dump our data **/
	htrAddScriptFunction(s, "form_cb_is_discard_ready", "\n"
		"function form_cb_is_discard_ready(aparam)\n"
		"    {\n"
		"    return 0;\n"	/* FIXME -- not ready yet */
		"    if(!this.IsUnsaved)\n"
		"        {\n"	/* the osrc really shouldn't have called us... */
		"        window.setTimeout(this.osrc.name+\".QueryContinue()\",100);\n"
		"        return 0;\n"
		"        }\n"
		"    if(confirm(\"OK to save or discard changes, CANCEL to stay here\"))\n"
		"        {\n"
		"        if(confirm(\"OK to save CANCEL to discard them.\"))\n"
		"            {\n"
		"            \n"
		"            }\n"
		"        else\n"
		"            {\n"
		"            window.setTimeout(this.osrc.name+\".QueryContinue()\",100);\n"
		"            }\n"
		"        }\n"
		"    else\n"
		"        {\n"
		"        window.setTimeout(this.osrc.name+\".QueryCancel()\",100);\n"
		"        }\n"
		"    }\n", 0);
	
	/** Objectsource says our object is available **/
	htrAddScriptFunction(s, "form_cb_object_available", "\n"
		"function form_cb_object_available(aparam)\n"
		"    {\n"
		"    \n"
		"    }\n", 0);

	/** Objectsource says the operation is complete **/
	htrAddScriptFunction(s, "form_cb_operation_complete", "\n"
		"function form_cb_operation_complete(aparam)\n"
		"    {\n"
		"    }\n", 0);

	/** Objectsource says the object was deleted **/
	htrAddScriptFunction(s, "form_cb_object_deleted", "\n"
		"function form_cb_object_deleted(aparam)\n"
		"    {\n"
		"    }\n", 0);

	/** Objectsource says the object was created **/
	htrAddScriptFunction(s, "form_cb_object_created", "\n"
		"function form_cb_object_created(aparam)\n"
		"    {\n"
		"    }\n", 0);

	/** Objectsource says the object was modified **/
	htrAddScriptFunction(s, "form_cb_object_modified", "\n"
		"function form_cb_object_modified(aparam)\n"
		"    {\n"
		"    }\n", 0);
	
	/** Moves form to "No Data" mode **/
	htrAddScriptFunction(s, "form_action_clear", "\n"
		"function form_action_clear(aparam)\n"
		"    {\n"
		"    }\n", 0);

	/**  **/
	htrAddScriptFunction(s, "form_action_delete", "\n"
		"function form_action_delete(aparam)\n"
		"    {\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "form_action_discard", "\n"
		"function form_action_discard(aparam)\n"
		"    {\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "form_action_edit", "\n"
		"function form_action_edit(aparam)\n"
		"    {\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "form_action_first", "\n"
		"function form_action_first(aparam)\n"
		"    {\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "form_action_last", "\n"
		"function form_action_last(aparam)\n"
		"    {\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "form_action_new", "\n"
		"function form_action_new(aparam)\n"
		"    {\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "form_action_next", "\n"
		"function form_action_next(aparam)\n"
		"    {\n"
		"    objsource.next();\n"
		"    if(this.IsUnsaved) this.ActionSave();\n"
		"    }\n", 0);

	/** Helper function -- called from other mode change functions **/
	htrAddScriptFunction(s, "form_change_mode", "\n"
		"function form_change_mode(form,newmode)\n"
		"    {\n"
		"    alert(\"Form is going from \"+form.mode+\" to \"+newmode+\" mode.\");\n"
		"    form.oldmode = form.mode;\n"
		"    form.mode = newmode;\n"
		"    form.changed = false;\n"
		"    var event = new Object();\n"
		"    event.Caller = form;\n"
		/*"    //cn_activate(form, 'StatusChange', event);\n" doesn't work -- FIXME*/
		"    delete event;\n"
		"    return 1;\n"
		"    }\n", 0);

	/** Change to query mode **/
	htrAddScriptFunction(s, "form_action_query", "\n"
		"function form_action_query()\n"
		"    {\n"
		"    if(!this.allowquery) {alert(\"Query mode not allowed\");return 0;}\n"
		"    for(var i in form.elements)\n"
		"        {\n"
		/*"        form.elements[i].Clear();\n" -- change soon */
		"        form.elements[i].setvalue('');\n"
		"        form.elements[i].IsChanged=false;\n"
		/* temp check"        confirm(form.elements[i].fieldname);\n"*/
		"        }\n"
		"    this.IsUnsaved=false;\n"
		"    return form_change_mode(this,\"Query\");\n"
		"    }\n", 0);

	/** Execute query **/
	htrAddScriptFunction(s, "form_action_queryexec", "\n"
		"function form_action_queryexec()\n"
		"    {\n"
		"    if(!this.allowquery) {alert(\"Query not allowed\");return 0;}\n"
		"    if(!(this.mode==\"Query\")) {alert(\"Must be in QBF mode\");return 0;}\n"
		"    var where = new String;\n"
		"    var firstone = true;\n"
		/** Build the SQL query, do I have to?...... **/
		"    for(var i in form.elements)\n"
		"        {\n"
		"        //confirm(form.elements[i].kind+\": \"+form.elements[i].getvalue());\n"
		"        if(form.elements[i].getvalue()!='')\n"
		"            {\n"
		"            if(firstone)\n"
		"                {\n"
		"                firstone=false;\n"
		"                }\n"
		"            else\n"
		"                {\n"
		"                where+=\" AND \";\n"
		"                }\n"
		"            var ele=form.elements[i];\n"
		"            switch(ele.kind)\n"
		"                {\n"
		"                case \"checkbox\":\n"
		"                    if(ele.getvalue())\n"
		"                        {\n"
		"                        where+=ele.fieldname+\"=true\";\n"
		"                        }\n"
		"                    else\n"
		"                        {\n"
		"                        where+=ele.fieldname+\"=false\";\n"
		"                        }\n"
		"                    break;\n"
		"                case \"editbox\":\n"
		/* Editbox supports as many</>/=/<=/>=/<=> clauses you can fit to query for data */
		/*   <=> is the LIKE operator...... */
		"                    var val=ele.getvalue();\n"
		"                    var res=String(val).match(/(<=>|<=|>=|=<|=>|<|>|=) ?([^<>=]*)/g);\n"
		"                    if(res)\n"
		"                        {\n"
		"                        var fone=true;\n"
		"                        for(var i in res)\n"
		"                            {\n"
		"                            if(fone)\n"
		"                                {\n"
		"                                fone=false;\n"
		"                                }\n"
		"                            else\n"
		"                                {\n"
		"                                where+=\" AND \";\n"
		"                                }\n"
		"                            var res2=(/(<=>|<=|>=|=<|=>|<|>|=) ?(.*)/).exec(res[i]);\n"
		"                            if(res2[1]==\"<=>\")\n"
		"                                {\n"
		"                                where+=ele.fieldname+\" LIKE \\\"\"+res2[2]+\"\\\"\";\n"
		"                                }\n"
		"                            else\n"
		"                                {\n"
		"                                where+=ele.fieldname+\"\"+res2[1]+\"\\\"\"+res2[2]+\"\\\"\";\n"
		"                                }\n"
		"                            }\n"
		"                        break;\n"
		"                        }\n"
		"                    where+=ele.fieldname+\"=\\\"\"+ele.getvalue()+\"\\\"\";\n"
		"                    break;\n"
		"                case \"radiobutton\":\n"
		"                    where+=ele.fieldname+\"=\\\"\"+ele.getvalue()+\"\\\"\";\n"
		"                    break;\n"
		"                default:\n"
		"                    confirm(\"don't know what to do with \\\"\"+ele.kind+\"\\\"\");\n"
		"                    break;\n"
		"                }\n"
		"            }\n"
		"        }\n"
		/** Done with the query -- YEAH **/
		"    var query;\n"
		"    if(where==\"\")\n"
		"        {\n"
		"        query=this.basequery+\";\";\n"
		"        }\n"
		"    else\n"
		"        {\n"
		"        query=form_build_query(this.basequery,where);\n"
		"        }\n"
		"    delete where;\n"
		"    if(confirm(\"Send to \\\"\"+this.osrc.name+\"\\\"(osrc):\\\n\"+query))\n"
		"        {\n"
		"        //form.Pending=true;\n"
		"        //form.IsUnsaved=false;\n"
		"        //this.osrc.ActionQuery(query);\n"
		"        }\n"
		"    delete query;\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "form_action_save", "\n"
		"function form_action_save()\n"
		"    {\n"
		"    if(!this.IsUnsaved)\n"
		"    	{\n"
		"    	alert(\"There isn't any reason to save.\");\n"
		"    	return 0;\n"
		"    	}\n"
		/** build the object to pass to objectsource **/
		"    \n"
		"    }\n", 0);

	/** Helper function to build a query */
	htrAddScriptFunction(s, "form_build_query", "\n"
		"function form_build_query(base,where)\n"
		"    {\n"
		"    re=/where/i;\n"
		"    if(re.test(base))\n"
		"        {\n"
		"        return base+\" \"+where+\";\";\n"
		"        }\n"
		"    else\n"
		"        {\n"
		"        return base+\" WHERE \"+where+\";\";\n"
		"        }\n"
		"    }\n", 0);


	/** Form initializer **/
	htrAddScriptFunction(s, "form_init", "\n"
		"function form_init(aq,an,am,av,and,me,name,bq,bw)\n"
		"    {\n"
		"    form = new Object();\n"
		"    form.basequery = bq;\n"
		"    form.currentquery = form_build_query(bq,bw);\n"
		"    form.elements = new Array();\n"
		"    form.mode = \"No Data\";\n"
		"    form.oldmode = null;\n"
		"    form.osrc = new Object();\n"
		"    form.IsUnsaved = false;\n"
		"    form.name = name;\n"
		"    form.Pending = false;\n"
		/** initialize params from .app file **/
		"    form.allowquery = aq;\n"
		"    form.allownew = an;\n"
		"    form.allowmodify = am;\n"
		"    form.allowview = av;\n"
		"    form.allownodata = and;\n"
		"    form.multienter = me;\n"
		/** initialize actions and callbacks **/
		"    form.ActionClear = form_action_clear;\n"
		"    form.ActionDelete = form_action_delete;\n"
		"    form.ActionDiscard = form_action_discard;\n"
		"    form.ActionEdit = form_action_edit;\n"
		"    form.ActionFirst = form_action_first;\n"
		"    form.ActionLast = form_action_last;\n"
		"    form.ActionNew = form_action_new;\n"
		"    form.ActionPrev = form_action_next;\n"
		"    form.ActionQuery = form_action_query;\n"
		"    form.ActionQueryExec = form_action_queryexec;\n"
		"    form.ActionSave = form_action_save;\n"
		"    form.IsDiscardReady = form_cb_is_discard_ready;\n"
		"    form.DataAvailable = form_cb_data_available;\n"
		"    form.ObjectAvailable = form_cb_object_available;\n"
		"    form.OperationComplete = form_cb_operation_complete;\n"
		"    form.ObjectDeleted = form_cb_object_deleted;\n"
		"    form.ObjectCreated = form_cb_object_created;\n"
		"    form.ObjectModified = form_cb_object_modified;\n"
		"    form.Register = form_cb_register;\n"
		"    form.DataNotify = form_cb_data_notify;\n"
		"    form.FocusNotify = form_cb_focus_notify;\n"
		"    form.TabNotify = form_cb_tab_notify;\n"

		"    return form;\n"
		"    }\n",0);
	//nmFree(sbuf3,800);

	/** Write out the init line for this instance of the form
	 **   the name of this instance was defined to be global up above
	 **   and fm_current is defined in htdrv_page.c 
	 **/
	sbuf3 = nmMalloc(200);
	snprintf(sbuf3,200,"\n    %s=fm_current=form_init(%i,%i,%i,%i,%i,%i,\"%s\",\"%s\",\"%s\");\n",
		name,allowquery,allownew,allowmodify,allowview,allownodata,multienter,name,basequery,basewhere);
	htrAddScriptInit(s,sbuf3);
	nmFree(sbuf3,200);

	/** Check for and render all subobjects. **/
	/** non-visual, don't consume a z level **/
	htrRenderSubwidgets(s, w_obj, parentname, parentobj, z);
	
	/** Make sure we don't claim orphans **/
	htrAddScriptInit(s,"    fm_current = null;\n\n");

    return 0;
    }


/*** htformInitialize - register with the ht_render module.
 ***/
int
htformInitialize()
    {
    pHtDriver drv;

    	/** Allocate the driver **/
	drv = htrAllocDriver();
	if (!drv) return -1;

	/** Fill in the structure. **/
	strcpy(drv->Name,"DHTML Form Widget");
	strcpy(drv->WidgetName,"form");
	drv->Render = htformRender;
	drv->Verify = htformVerify;

	/** Add our actions **/
	htrAddAction(drv,"Clear");
	htrAddAction(drv,"Delete");
	htrAddAction(drv,"Discard");
	htrAddAction(drv,"Edit");
	htrAddAction(drv,"First");
	htrAddAction(drv,"Last");
	htrAddAction(drv,"New");
	htrAddAction(drv,"Next");
	htrAddAction(drv,"Prev");
	htrAddAction(drv,"Query");
	htrAddAction(drv,"QueryExec");
	htrAddAction(drv,"Save");

	htrAddEvent(drv,"StatusChange"); /* Not documented */

	/** Register. **/
	htrRegisterDriver(drv);

	HTFORM.idcnt = 0;

    return 0;
    }
