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
		"    }\n", 0);
	
	/** Objectsource says our object is available **/
	htrAddScriptFunction(s, "form_cb_object_available", "\n"
		"function form_cb_object_available(aparam)\n"
		"    {\n"
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
		"        }\n"
		"    this.IsUnsaved=false;\n"
		"    return form_change_mode(this,\"Query\");\n"
		"    }\n", 0);

	/** Execute query **/
	htrAddScriptFunction(s, "form_action_queryexec", "\n"
		"function form_action_queryexec()\n"
		"    {\n"
		"    if(!this.allowquery) {alert(\"Query not allowed\");return 0;}\n"
		"    for(var i in form.elements)\n"
		"        {\n"
		/*"        form.elements[i].Clear();\n" -- change soon */
		"        form.elements[i].setvalue('');\n"
		"        }\n"
		"    query=new String;\n"
		/* Build the query, I think, oh joy......*/
		"    form.Pending=true;\n"
		"    this.osrc.ActionQuery(query);\n"
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
		/** either have to build the query, or build the object to
		 ** pass to objectsource **/
		"    \n"
		"    }\n", 0);



	/** Form initializer **/
	htrAddScriptFunction(s, "form_init", "\n"
		"function form_init(aq,an,am,av,and,me,name)\n"
		"    {\n"
		"    form = new Object();\n"
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
	snprintf(sbuf3,200,"\n    %s=fm_current=form_init(%i,%i,%i,%i,%i,%i,%s);\n",
		name,allowquery,allownew,allowmodify,allowview,allownodata,multienter,name);
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
