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
/* Author:	Jonathan Rupp (JDR)					*/
/* Creation:	February 20, 2002 					*/
/* Description:	This is the non-visual widget that interfaces the 	*/
/*		objectsource widget and the visual sub-widgets		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Log: htdrv_form.c,v $
    Revision 1.25  2002/04/25 03:13:50  jorupp
     * added label widget
     * bug fixes in form and osrc

    Revision 1.24  2002/04/25 01:13:43  jorupp
     * increased buffer size for query in form
     * changed sybase driver to not put strings in two sets of quotes on update

    Revision 1.23  2002/04/10 00:36:20  jorupp
     * fixed 'visible' bug in imagebutton
     * removed some old code in form, and changed the order of some callbacks
     * code cleanup in the OSRC, added some documentation with the code
     * OSRC now can scroll to the last record
     * multiple forms (or tables?) hitting the same osrc now _shouldn't_ be a problem.  Not extensively tested however.

    Revision 1.22  2002/04/05 06:39:12  jorupp
     * Added ReadOnly parameter to form
     * If ReadOnly is not present, updates will now work properly!
     * still need to work on stopping updates on client side when readonly is set

    Revision 1.21  2002/04/05 06:10:11  gbeeley
    Updating works through a multiquery when "FOR UPDATE" is specified at
    the end of the query.  Fixed a reverse-eval bug in the expression
    subsystem.  Updated form so queries are not terminated by a semicolon.
    The DAT module was accepting it as a part of the pathname, but that was
    a fluke :)  After "for update" the semicolon caused all sorts of
    squawkage...

    Revision 1.20  2002/03/28 05:21:22  jorupp
     * form no longer does some redundant status checking
     * cleaned up some unneeded stuff in form
     * osrc properly impliments almost everything (will prompt on unsaved data, etc.)

    Revision 1.19  2002/03/23 00:32:13  jorupp
     * osrc now can move to previous and next records
     * form now loads it's basequery automatically, and will not load if you don't have one
     * modified form test page to be a bit more interesting

    Revision 1.18  2002/03/20 21:13:12  jorupp
     * fixed problem in imagebutton point and click handlers
     * hard-coded some values to get a partially working osrc for the form
     * got basic readonly/disabled functionality into editbox (not really the right way, but it works)
     * made (some of) form work with discard/save/cancel window

    Revision 1.17  2002/03/17 20:45:45  gbeeley
    Re-fixed security update introduced at file rev 1.12 but lost somehow.

    Revision 1.16  2002/03/17 03:51:03  jorupp
    * treeview now returns value on function call (in alert window)
    * implimented basics of 3-button confirm window on the form side
        still need to update several functions to use it

    Revision 1.15  2002/03/16 02:04:05  jheth
    osrc widget queries and passes data back to form widget

    Revision 1.14  2002/03/14 05:11:49  jorupp
     * bugfixes

    Revision 1.13  2002/03/14 03:29:51  jorupp
     * updated form to prepend a : to the fieldname when using for a query
     * updated osrc to take the query given it by the form, submit it to the server,
        iterate through the results, and store them in the replica
     * bug fixes to treeview (DOMviewer mode) -- added ability to change scaler values

    Revision 1.11  2002/03/09 02:38:48  jheth
    Make OSRC work with Form - Query at least

    Revision 1.10  2002/03/08 02:07:13  jorupp
    * initial commit of alerter widget
    * build callback listing object for form
    * form has many more of it's callbacks working

    Revision 1.9  2002/03/05 01:55:23  jorupp
    * switch to using clearvalue() instead of setvalue('') to clear form elements
    * document basequery/basewhere

    Revision 1.8  2002/03/05 00:46:34  jorupp
    * Fix a problem in Luke's radiobutton fix
    * Add the corresponding checks in the form

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
    char* basequery;
    char* basewhere;
    int allowquery, allownew, allowmodify, allowview, allownodata, multienter;
    char _3bconfirmwindow[30];
    int readonly;
#define FORM_BUF_SIZE 4096
    
    basequery=nmMalloc(FORM_BUF_SIZE);
    basewhere=nmMalloc(FORM_BUF_SIZE);
    
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
	if (objGetAttrValue(w_obj,"ReadOnly",POD(&readonly)) != 0) 
	    readonly=0;

	/*** 03/16/02 Jonathan Rupp
	 ***   added _3bconfirmwindow, the name of a window that has 
	 ***     three button objects, _3bConfirmCancel, 
	 ***     _3bConfirmCancel, and _3bConfirmCancel.  There can
	 ***     be no connectors attached to these buttons.
	 ***     this window should be set to hidden -- the form
	 ***     will make it visible when needed, and hide it again
	 ***     afterwards.
	 ***   There is a Action to test this new functionality:
	 ***     .Actiontest3bconfirm()
	 ***     -- once this is used elsewhere, this function will
	 ***        be removed
	 ***/
	
	if (objGetAttrValue(w_obj,"_3bconfirmwindow",POD(&ptr)) == 0)
	    snprintf(_3bconfirmwindow,30,"%s",ptr);
	else
	    strcpy(_3bconfirmwindow,"null");

	
	/*** 
	 *** (03/01/02) Jonathan Rupp -- added two new paramters
	 ***      basequery -- the part of the SQL statement that is never
	 ***        modified, even in QBF form (do not terminate with ';')
	 ***      basewhere -- an initial WHERE clause (do not include WHERE)
	 ***   example:
	 ***     basequery="SELECT a,b,c from data"
	 ***     +- no WHERE clause by default, can be added in QBF
	 ***     basequery="SELECT a,b,c from data"
	 ***     basewhere="a=true"
	 ***     +- by default, only show fields where a is true
	 ***          QBF will override
	 ***     basequery="SELECT a,b,c from data WHERE b=false"
	 ***     +- only will show rows where b is false
	 ***          can't be overridden in QBF, can be added to
	 ***     basequery="SELECT a,b,c from data WHERE b=false"
	 ***     basewhere="c=true"
	 ***     +- only will show rows where b is false
	 ***          in addition, at start, will only show where c is true
	 ***          condition (c=true) can be overridden, (b=false) can't
	 ***/

	if (objGetAttrValue(w_obj,"basequery",POD(&ptr)) == 0)
	    snprintf(basequery,FORM_BUF_SIZE,"%s",ptr);
	else
	    {
	    mssError(1,"HTFORM","Form must have a 'basequery' property");
	    return -1;
	    }
	
	if (objGetAttrValue(w_obj,"basewhere",POD(&ptr)) == 0)
	    snprintf(basewhere,FORM_BUF_SIZE,"%s",ptr);
	else
	    strcpy(basewhere,"");

	/** Get name **/
	if (objGetAttrValue(w_obj,"name",POD(&ptr)) != 0) return -1;
	memccpy(name,ptr,0,63);
	name[63] = 0;

	/** Write named global **/
	nptr = (char*)nmMalloc(strlen(name)+1);
	strcpy(nptr,name);

	/** create our instance variable **/
	htrAddScriptGlobal(s, nptr, "null",HTR_F_NAMEALLOC); 

	htrAddScriptFunction(s, "form_cb_register", "\n"
		"function form_cb_register(aparam)\n"
		"    {\n"
		"    if(aparam.kind==\"formstatus\")\n"
		"        {\n"
		"        this.statuswidgets.push(aparam);\n"
		"        aparam.setvalue(this.mode);\n"
		"        }\n"
		"    else\n"
		"        {\n"
		"        this.elements.push(aparam);\n"
		"        }\n"
		"    }\n", 0);

	/** A child 'control' has changed it's data **/
	htrAddScriptFunction(s, "form_cb_data_notify", "\n"
		"function form_cb_data_notify(control)\n"
		"    {\n"
		"    control._form_IsChanged=true;\n"
		"    if(this.mode=='New' || this.mode=='Modify')\n"
		"        {\n"
		"        this.IsUnsaved=true;\n"
		"        }\n"
		"    }\n", 0);

	/** A child 'control' got or lost focus **/
	htrAddScriptFunction(s, "form_cb_focus_notify", "\n"
		"function form_cb_focus_notify(control)\n"
		"    {\n"
		"    if(this.mode=='View')\n"
		"        {\n"
		"        this.ChangeMode('Modify');\n"
		"        }\n"
		"    if(this.mode=='No Data')\n"
		"        {\n"
		"        this.ChangeMode('New');\n"
		"        }\n"
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
		"    this.cb[\"DataAvailable\"].run();\n"
		"    }\n", 0);

	/** Objectsource wants us to dump our data **/
	htrAddScriptFunction(s, "form_cb_is_discard_ready", "\n"
		"function form_cb_is_discard_ready()\n"
		"    {\n"
		"    if(!this.IsUnsaved)\n"
		"        {\n"
		"        this.osrc.QueryContinue(this);\n"
		"        return 0;\n"
		"        }\n"
		"    var savefunc=new Function(\"this.cb['OperationCompleteSuccess'].add(this,new Function('this.osrc.QueryContinue(this);'));this.cb['OperationCompleteFail'].add(this,new Function('this.osrc.QueryCancel(this);'));this.ActionSave();\");\n"
		"    this.cb['_3bConfirmSave'].add(this,savefunc);\n"
		"    this.cb['_3bConfirmDiscard'].add(this,new Function('this.ClearAll();this.osrc.QueryContinue(this);'));\n"
		"    this.cb['_3bConfirmCancel'].add(this,new Function('this.osrc.QueryCancel(this);'));\n"
		"    this.show3bconfirm();\n"
		"    return 0;\n"
		"    \n"
		"    \n"
		"    if(confirm(\"OK to save or discard changes, CANCEL to stay here\"))\n"
		"        {\n"
		"        if(confirm(\"OK to save changes, CANCEL to discard them.\"))\n"
		"            {\n" /* save */
		"            this.cb[\"OperationCompleteSuccess\"].add(this,new Function(\"this.osrc.QueryContinue(this);\"))\n"
		"            this.cb[\"OperationCompleteFail\"].add(this,new Function(\"this.osrc.QueryCancel(this);\"))\n"
		"            this.ActionSave();\n"
		"            }\n"
		"        else\n"
		"            {\n" /* cancel (discard) */
		"            this.osrc.QueryContinue(this);\n"
		"            }\n"
		"        }\n"
		"    else\n"
		"        {\n" /* cancel (don't allow new query) */
		"        this.osrc.QueryCancel(this);\n"
		"        }\n"
		"    }\n", 0);

	/** Objectsource says our object is available **/
	htrAddScriptFunction(s, "form_cb_object_available", "\n"
		"function form_cb_object_available(data)\n"
		"    {\n"
		"    if(this.mode!='View')\n"
		"        {\n"
		"        this.ChangeMode('View');\n"
		"        }\n"
		"    this.data=data;\n"
		"    this.ClearAll();\n"
		"    for(var i in this.elements)\n"
		"        {\n"
		"        for(var j in this.data)\n"
		"            {\n"
		"            if(this.elements[i].fieldname && this.elements[i].fieldname==this.data[j].oid)\n"
		"                {\n"
		"                this.elements[i].setvalue(this.data[j].value);\n"
		"                }\n"
		"            }\n"
		"        }\n"
		"    }\n", 0);

	/** Objectsource says the operation is complete **/
	htrAddScriptFunction(s, "form_cb_operation_complete", "\n"
		"function form_cb_operation_complete(r)\n"
		"    {\n"
		"    if(r)\n"
		"        this.cb['OperationCompleteSuccess'].run();\n"
		"    else\n"
		"        this.cb['OperationCompleteFail'].run();\n"
		"    }\n", 0);

	/** Objectsource says the object was deleted **/
	htrAddScriptFunction(s, "form_cb_object_deleted", "\n"
		"function form_cb_object_deleted()\n"
		"    {\n"
		"    this.cb['ObjectDeleted'].run();\n"
		"    }\n", 0);

	/** Objectsource says the object was created **/
	htrAddScriptFunction(s, "form_cb_object_created", "\n"
		"function form_cb_object_created()\n"
		"    {\n"
		"    this.cb['ObjectCreated'].run();\n"
		"    }\n", 0);

	/** Objectsource says the object was modified **/
	htrAddScriptFunction(s, "form_cb_object_modified", "\n"
		"function form_cb_object_modified()\n"
		"    {\n"
		"    this.cb['ObjectModified'].run();\n"
		"    }\n", 0);
	
	/** Moves form to "No Data" mode **/
	/**   If unsaved data exists (New or Modify mode), prompt for Save/Discard **/
	/** Clears any rows in osrc replica (doesn't delete from DB) **/
	htrAddScriptFunction(s, "form_action_clear", "\n"
		"function form_action_clear(aparam)\n"
		"    {\n"
		"    if(this.mode==\"No Data\")\n"
		"        return;\n"	/* Already in No Data Mode */
		"    if(this.IsUnsaved && (this.mode==\"New\" || this.mode==\"Modify\"))\n"
		"        {\n"
		"        if(confirm(\"OK to save or discard changes, CANCEL to stay here\"))\n"
		"            {\n"
		"            if(confirm(\"OK to save changes, CANCEL to discard them.\"))\n"
		"                {\n" /* save */
		"                this.cb[\"OperationCompleteSuccess\"].add(this,new Function(\"this.ActionClear();\"));\n"
		"                this.ActionSave();\n"
		"                return 0;\n"
		"                }\n"	
		"            else\n"
		"                {\n" /* discard is ok */
		"                this.ClearAll();\n"
		"                }\n"
		"            }\n"
		"        else\n"
		"            {\n" /* user wants to stay here */
		"            return 0;\n"
		"            }\n"
		"        }\n"
		"    this.ClearAll();\n"
		"    }\n", 0);

	/** in New - clear, remain in New **/
	/** in View - delete the record, move to View mode on the next record, or No Data **/
	/** in Query - clear, remain in Query **/
	htrAddScriptFunction(s, "form_action_delete", "\n"
		"function form_action_delete(aparam)\n"
		"    {\n"
		"    switch(this.mode)\n"
		"        {\n"
		"        case \"New\":\n"
		"            this.ClearAll();\n"
		"            break;\n"
		"        case \"View\":\n"
		"            break;\n"
		"        case \"Query\":\n"
		"            this.ClearAll();\n"
		"            break;\n"
		"        default:\n"
		"            break;\n" /* else do nothing */
		"        }\n"
		"    }\n", 0);

	/** in Modify, cancel changes, switch to 'View' **/
	/** in New, cancel changes, switch to 'No Data' **/
	/** in Query, clear, remain in query mode **/
	htrAddScriptFunction(s, "form_action_discard", "\n"
		"function form_action_discard(aparam)\n"
		"    {\n"
		"    switch(this.mode)\n"
		"        {\n"
		"        case \"Modify\":\n"
		"            this.ClearAll();\n"
		"            this.ChangeMode(\"View\");\n"
		"            break;\n"
		"        case \"New\":\n"
		"            this.ClearAll();\n"
		"            this.ChangeMode(\"No Data\");\n"
		"            break;\n"
		"        case \"Query\":\n"
		"            this.ClearAll();\n"
		"            break;\n"
		"        }\n"
		"    }\n", 0);

	/** in No Data, move to New **/
	/** in View, move to Modify (note this can be implicit on click -- can be disabled) **/
	htrAddScriptFunction(s, "form_action_edit", "\n"
		"function form_action_edit(aparam)\n"
		"    {\n"
		"    switch(this.mode)\n"
		"        {\n"
		"        case \"No Data\":\n"
		"            this.ChangeMode(\"New\");\n"
		"            break;\n"
		"        case \"View\":\n"
		"            this.ChangeMode(\"Modify\");\n"
		"            break;\n"
		"        }\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "form_show_3bconfirm", "\n"
		"function form_show_3bconfirm()\n"
		"    {\n"
		"    var discard,save,cancel;\n"
		"    var lay=this._3bconfirmwindow.mainLayer.layers;\n"
		"    for(var i in lay)\n"
		"        {\n"
		"        if(lay[i] && lay[i].buttonName)\n"
		"            {\n"
		"            switch(lay[i].buttonName)\n"
		"                {\n"
		"                case '_3bConfirmCancel':\n"
		"                    cancel=lay[i];\n"
		"                    break;\n"
		"                case '_3bConfirmSave':\n"
		"                    save=lay[i];\n"
		"                    break;\n"
		"                case '_3bConfirmDiscard':\n"
		"                    discard=lay[i];\n"
		"                    break;\n"
		"                }\n"
		"            }\n"
		"        }\n"
		"    if(!(save && discard && cancel))\n"
		"        {\n"
		"        alert(\"You didn't use all the buttons, or you named them wrong.\");\n"
		"        return 0;\n"
		"        }\n"
		/** set up pointers so the EventClick method can find this form... **/
		"    save._form_form=this;\n"
		"    discard._form_form=this;\n"
		"    cancel._form_form=this;\n"
		"    save.EventClick=this._3bconfirm_save;\n"
		"    discard.EventClick=this._3bconfirm_discard;\n"
		"    cancel.EventClick=this._3bconfirm_cancel;\n"
		"    \n"
		"    var funclate=new Function(\"this.cb['_3bConfirmCancel'].clear();this.cb['_3bConfirmDiscard'].clear();this.cb['_3bConfirmSave'].clear();\");\n"
		"    var func=new Function(\"pg_setmodal(null);var v=new Object();v.IsVisible=0;this._3bconfirmwindow.ActionSetVisibility(v);\");\n"
		"    \n"
		/** funclate will fire last (or very close, with a level of 1000) **/
		"    this.cb['_3bConfirmCancel'].add(this,funclate,null,1000);\n"
		"    this.cb['_3bConfirmDiscard'].add(this,funclate,null,1000);\n"
		"    this.cb['_3bConfirmSave'].add(this,funclate,null,1000);\n"
		"    \n"
		/** func will fire first (or very close, with a level of -1000) **/
		"    this.cb['_3bConfirmCancel'].add(this,func,null,-1000);\n"
		"    this.cb['_3bConfirmDiscard'].add(this,func,null,-1000);\n"
		"    this.cb['_3bConfirmSave'].add(this,func,null,-1000);\n"
		"    \n"
		/** set modal and make the user do something
		 **   modal will be unset and we'll get control back via callbacks **/
		"    this._3bconfirmwindow.ActionSetVisibility(1);\n"
		"    pg_setmodal(this._3bconfirmwindow.mainLayer)\n"
		"    \n"
		"    }\n", 0);
	
	htrAddScriptFunction(s, "form_3bconfirm_cancel", "\n"
		"function form_3bconfirm_cancel()\n"
		"    {\n"
		"    if(this._form_form)\n"
		"        this._form_form.cb['_3bConfirmCancel'].run();\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "form_3bconfirm_discard", "\n"
		"function form_3bconfirm_discard()\n"
		"    {\n"
		"    if(this._form_form)\n"
		"        this._form_form.cb['_3bConfirmDiscard'].run();\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "form_3bconfirm_save", "\n"
		"function form_3bconfirm_save()\n"
		"    {\n"
		"    if(this._form_form)\n"
		"        this._form_form.cb['_3bConfirmSave'].run();\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "form_test_3bconfirm", "\n"
		"function form_test_3bconfirm()\n"
		"    {\n"
		"    this.cb['_3bConfirmCancel'].add(this,new Function(\"alert('cancel was clicked');\"))\n"
		"    this.cb['_3bConfirmDiscard'].add(this,new Function(\"alert('discard was clicked');\"))\n"
		"    this.cb['_3bConfirmSave'].add(this,new Function(\"alert('save was clicked');\"))\n"
		"    this.show3bconfirm();\n"
		"    }\n", 0);

	/** in View, Query, or No Data, clear, move to New **/
	/** in Modify, give save/discard/cancel prompt **/
	/** (if from No Data or View mode after query, auto-fill criteria) **/
	htrAddScriptFunction(s, "form_action_new", "\n"
		"function form_action_new(aparam)\n"
		"    {\n"
		"    switch(this.mode)\n"
		"        {\n"
		"        case \"Query\":\n"
		"        case \"No Data\":\n"
		"        case \"View\":\n"
		"            this.ChangeMode(\"New\");\n"
		"            \n" /* if there was a query run, fill in it's values... */
		"            break;\n"
		"        case \"Modify\":\n"
		"            if(this.Unsaved)\n"
		"                {\n"
		"                var savefunc=new Function(\"this.cb['OperationCompleteSuccess'].add(this,new Function('this.ActionNew();'));this.ActionSave();\")\n"
		"                var discardfunc=new Function(\"this.Unsaved=false;this.ChangeMode('New');\")\n"
		"                this.cb['_3bConfirmDiscard'].add(this,discardfunc);\n"
		"                this.cb['_3bConfirmSave'].add(this,savefunc);\n"
		"                this.show3bconfirm();\n"
		"                }\n"
		"            break;\n"
		"        }\n"
		"    }\n", 0);

	/** tell osrc to go to first record **/
	htrAddScriptFunction(s, "form_action_first", "\n"
		"function form_action_first(aparam)\n"
		"    {\n"
		"    this.osrc.ActionFirst();\n"
		"    return 0;\n"
		"    }\n", 0);

	/** tell osrc to go to last record **/
	htrAddScriptFunction(s, "form_action_last", "\n"
		"function form_action_last(aparam)\n"
		"    {\n"
		"    this.osrc.ActionLast();\n"
		"    return 0;\n"
		"    }\n", 0);

	/** Save changed data, move the osrc **/
	htrAddScriptFunction(s, "form_action_next", "\n"
		"function form_action_next(aparam)\n"
		"    {\n"
		"    this.osrc.ActionNext();\n"
		"    return 0;\n"
		"    }\n", 0);

	/** Save changed data, move the osrc **/
	htrAddScriptFunction(s, "form_action_prev", "\n"
		"function form_action_prev(aparam)\n"
		"    {\n"
		"    this.osrc.ActionPrev();\n"
		"    return 0;\n"
		"    }\n", 0);

	/** Helper function -- called from other mode change functions **/
	htrAddScriptFunction(s, "form_change_mode", "\n"
		"function form_change_mode(newmode)\n"
		"    {\n"
		"    //alert('Form is going from '+this.mode+' to '+newmode+' mode.');\n"
		"    this.oldmode = this.mode;\n"
		"    this.mode = newmode;\n"
		"    this.IsUnsaved = false;\n"
		/*
		"    var event = new Object();\n"
		"    event.Caller = this;\n"
		"    //cn_activate(this, 'StatusChange', event);\n" doesn't work -- FIXME
		"    delete event;\n"
		*/
		"    if(this.mode=='No Data' || this.mode=='View')\n"
		"        {\n"
		"        this.ReadOnlyAll();\n"
		"        }\n"
		"    if(this.oldmode=='No Data' || this.oldmode=='View')\n"
		"        {\n"
		"        this.EnableAll();\n"
		"        }\n"
		"    for(var i in this.statuswidgets)\n"
		"        {\n"
		"        this.statuswidgets[i].setvalue(this.mode);\n"
		"        }\n"
		"    return 1;\n"
		"    }\n", 0);
	
	/** Clears all children, also resets IsChanged/IsUnsaved flag to false **/
	htrAddScriptFunction(s, "form_clear_all", "\n"
		"function form_clear_all()\n"
		"    {\n"
		"    for(var i in this.elements)\n"
		"        {\n"
		"        this.elements[i].clearvalue();\n"
		"        this.elements[i]._form_IsChanged=false;\n"
		"        }\n"
		"    this.IsUnsaved=false;\n"
		"    //this.osrc.ActionClear();\n"
		"    }\n", 0);

	/** Disables all children **/
	htrAddScriptFunction(s, "form_disable_all", "\n"
		"function form_disable_all()\n"
		"    {\n"
		"    for(var i in this.elements)\n"
		"        {\n"
		"        this.elements[i].disable();\n"
		"        }\n"
		"    }\n", 0);

	/** Enables all children **/
	htrAddScriptFunction(s, "form_enable_all", "\n"
		"function form_enable_all()\n"
		"    {\n"
		"    for(var i in this.elements)\n"
		"        {\n"
		"        this.elements[i].enable();\n"
		"        }\n"
		"    }\n", 0);

	/** make all children readonly**/
	htrAddScriptFunction(s, "form_readonly_all", "\n"
		"function form_readonly_all()\n"
		"    {\n"
		"    for(var i in this.elements)\n"
		"        {\n"
		"        this.elements[i].readonly();\n"
		"        }\n"
		"    }\n", 0);
	
	/** Change to query mode **/
	htrAddScriptFunction(s, "form_action_query", "\n"
		"function form_action_query()\n"
		"    {\n"
		"    if(!this.allowquery) {alert('Query mode not allowed');return 0;}\n"
		"    this.ClearAll();\n"
		"    return this.ChangeMode('Query');\n"
		"    }\n", 0);

	/** Execute query **/
	htrAddScriptFunction(s, "form_action_queryexec", "\n"
		"function form_action_queryexec()\n"
		"    {\n"
		"    if(!this.allowquery) {alert('Query not allowed');return 0;}\n"
		"    if(!(this.mode=='Query')) {alert('Must be in QBF mode');return 0;}\n"
		"    var where = new String;\n"
		"    var firstone = true;\n"
		/** Build the SQL query, do I have to?...... **/
		"    for(var i in form.elements)\n"
		"        {\n"
		"        if(form.elements[i]._form_IsChanged)\n"
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
		"                case 'checkbox':\n"
		"                    if(ele.getvalue())\n"
		"                        {\n"
		"                        where+=ele.fieldname+'=true';\n"
		"                        }\n"
		"                    else\n"
		"                        {\n"
		"                        where+=ele.fieldname+'=false';\n"
		"                        }\n"
		"                    break;\n"
		"                case 'editbox':\n"
		/* Editbox supports as many</>/=/<=/>=/<=> clauses you can fit to query for data */
		/*   <=> is the LIKE operator...... */
		"                    var val=ele.getvalue();\n"
		"                    var res=String(val).match(/(<=>|<=|>=|=<|=>|<|>|=) ?([^<>=]*? ?)/g);\n"
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
		"                                where+=' AND ';\n"
		"                                }\n"
		"                            var res2;\n"
		"                            if ((/ $/).test(res[i]))\n" /* Horrible hack - FIXME*/
		"                                {\n"
		"                                res2=(/(<=>|<=|>=|=<|=>|<|>|=) ?(.*?) /).exec(res[i]);\n"
		"                                }\n"
		"                            else\n"
		"                                {\n"
		"                                res2=(/(<=>|<=|>=|=<|=>|<|>|=) ?(.*?)/).exec(res[i]);\n"
		"                                };\n"
		"                            if(res2[1]=='<=>')\n"
		"                                {\n"
		"                                where+=':'+ele.fieldname+' LIKE \"'+res2[2]+'\"';\n"
		"                                }\n"
		"                            else\n"
		"                                {\n"
		"                                where+=':'+ele.fieldname+res2[1]+'\"'+res2[2]+'\"';\n"
		"                                }\n"
		"                            }\n"
		"                        }\n"
		"                    else\n"
		"                        {\n"
		"                        where+=':'+ele.fieldname+'=\"'+ele.getvalue()+'\"';\n"
		"                        }\n"
		"                    break;\n"
		"                case 'radiobutton':\n"
		"                    where+=':'+ele.fieldname+'=\"'+ele.getvalue()+'\"';\n"
		"                    break;\n"
		"                default:\n"
		"                    confirm('don\\'t know what to do with \"'+ele.kind+'\"');\n"
		"                    break;\n"
		"                }\n"
		"            }\n"
		"        }\n"
		/** Done with the query -- YEAH **/
		"    var query;\n"
		"    if(where=='')\n"
		"        {\n"
		"        query=this.basequery;\n"
		"        }\n"
		"    else\n"
		"        {\n"
		"        query=form_build_query(this.basequery,where,this.readonly);\n"
		"        }\n"
		"    delete where;\n"
		"    if(confirm('Send to \"'+this.osrc.name+'\"(osrc):'+query))\n"
		"        {\n"
		"        form.Pending=true;\n"
		"        form.IsUnsaved=false;\n"
		"        this.cb['DataAvailable'].add(this,new Function('this.osrc.ActionFirst(this)'));\n"
		"        this.osrc.ActionQuery(query, this);\n"
		"        }\n"
		"    delete query;\n"
		"    }\n", 0);

	htrAddScriptFunction(s, "form_action_save", "\n"
		"function form_action_save()\n"
		"    {\n"
		"    if(!this.IsUnsaved)\n"
		"    	 {\n"
		"    	 alert('There isn\\'t any reason to save.');\n"
		"    	 return 0;\n"
		"        }\n"
		"    this.cb['OperationCompleteSuccess'].add(this,\n"
		"           new Function(\"this.IsUnsaved=false;this.Pending=false;this.EnableAll();this.cb['OperationCompleteFail'].clear();\"),null,-100);\n"
		"     this.cb['OperationCompleteFail'].add(this,\n"
		"           new Function(\"this.Pending=false;this.EnableAll();confirm('Data Save Failed');this.cb['OperationCompleteSuccess'].clear();\"),null,-100);\n"
	    
		/** build the object to pass to objectsource **/
		"    var dataobj=new Object();\n"
		"    for(var i in form.elements)\n"
		"        {\n"
		"        var j=form.elements[i];\n"
		"        if(j._form_IsChanged)\n"
		"            dataobj[j.fieldname]=j.getvalue();\n"
		"        }\n"
		"    this.DisableAll();\n"
		"    this.Pending=true;\n"
		"    this.osrc.ActionModify(dataobj,this);\n"
		"    }\n", 0);

	/** Send Initial Query */
	htrAddScriptFunction(s, "form_init_query", "\n"
		"function form_init_query()\n"
		"    {\n"
		"    this.Pending=true;\n"
		"    this.IsUnsaved=false;\n"
		"    this.cb['DataAvailable'].add(this,new Function('this.osrc.ActionFirst(this)'));\n"
		"    this.osrc.ActionQuery(this.currentquery)\n"
		"    }\n", 0);


	/** Helper function to build a query */
	htrAddScriptFunction(s, "form_build_query", "\n"
		"function form_build_query(base,where,ro)\n"
		"    {\n"
		"    re=/where/i;\n"
		"    if(!where)\n"
		"        if(ro)\n"
		"            return base;\n"
		"        else\n"
		"            return base+' FOR UPDATE';\n"
		"    if(re.test(base))\n"
		"        if(ro)\n"
		"           return base+' AND '+where;\n"
		"        else\n"
		"           return base+' AND '+where+' FOR UPDATE';\n"
		"    else\n"
		"        if(ro)\n"
		"            return base+' WHERE '+where;\n"
		"        else\n"
		"            return base+' WHERE '+where+' FOR UPDATE';\n"
		"    }\n", 0);


	/***
	 ***   The following callback helper functions might be
	 ***   useful if available to all widgets.
	 ***   For now, just the form will use them though
	 ***/

	/** Callback object init **/
	htrAddScriptFunction(s, "form_cbobj", "\n"
		"function form_cbobj(n)\n" //FIXME FIXME
		"    {\n"
		"    this.arr=new Array();\n"
		"    this.add=form_cbobj_add;\n"
		"    this.run=form_cbobj_run;\n"
		"    this.clear=form_cbobj_clear;\n"
		"    this.name=n;\n"
		"    }\n", 0);

	/** Calls all registered functions **/
	htrAddScriptFunction(s, "form_cbobj_run", "\n"
		"function form_cbobj_run()\n"
		"    {\n"
		"    if(this.arr.length<1)\n"
		"        {\n"
		"        confirm('There are no callbacks registered for '+this.name+'.  This is a problem');\n"
		"        return false;\n"
		"        }\n"
		"    this.arr.sort(form_cbobj_compare);\n"
		"    for(var i in this.arr)\n"
		"        {\n"
		"        var j=this.arr[i];\n"
		"        //if(j && confirm(j))\n"
		"        if(j)\n"
		"            if(j[3])\n"
		"                j[2].apply(j[1],j[3]);\n"
		"            else\n"
		"                j[2].apply(j[1]);\n"
		"        }\n"
		"    this.clear();\n"
		"    }\n", 0);

	/** Adds a new registered function **/
	htrAddScriptFunction(s, "form_cbobj_add", "\n"
		"function form_cbobj_add(obj,func,param,key)\n"
		"    {\n"
		"    if(obj==undefined) { obj = null; }\n"
		"    if(func==undefined) { func = null; }\n"
		"    if(param==undefined) { param = null; }\n"
		"    if(key==undefined) { key=0; }\n"
		"    this.arr.push(new Array(key,obj,func,param));\n"
		"    }\n", 0);
	
	/** Clears all registered callbacks **/
	htrAddScriptFunction(s, "form_cbobj_clear", "\n"
		"function form_cbobj_clear()\n"
		"    {\n"
		"    for(var i in this.arr)\n"
		"        delete this.arr[i];\n"
		"    }\n", 0);

	/** Compares two arrays based on the first element **/
	htrAddScriptFunction(s, "form_cbobj_compare", "\n"
		"function form_cbobj_compare(a,b)\n"
		"    {\n" /* sort in order based on keys (element 0) */
		"    if(!a || !b) return 0;\n"
		"    if(a[0]>b[0]) return 1;\n"
		"    if(a[0]==b[0]) return 0;\n"
		"    if(a[0]<b[0]) return -1;\n"
		"    }\n", 0);

	/** Form initializer **/
	htrAddScriptFunction(s, "form_init", "\n"
		"function form_init(aq,an,am,av,and,me,name,bq,bw,_3b,ro)\n"
		"    {\n"
		"    form = new Object();\n"
		"    form.readonly=ro;\n" 
		"    form.basequery = bq;\n"
		"    form.currentquery = form_build_query(bq,bw,form.readonly);\n"
		"    form.elements = new Array();\n"
		"    form.statuswidgets = new Array();\n"
		"    form.mode = 'No Data';\n"
		"    form.cobj = null;\n" /* current 'object' (record) */
		"    form.oldmode = null;\n"
		"    if(osrc_current)\n"
		"        {\n"
		"        form.osrc = osrc_current;\n"
		"        form.osrc.Register(form);\n"
		"        }\n"
		"    form.IsUnsaved = false;\n"
		"    form.name = name;\n"
		"    form.Pending = false;\n"
		/** remember what to do after callbacks.... **/
		"    form.cb = new Array();\n"
		"    form.cb['DataAvailable'] = new form_cbobj('DataAvailable');\n"
		"    form.cb['ObjectAvailable'] = new form_cbobj('ObjectAvailable');\n"
		"    form.cb['OperationCompleteSuccess'] = new form_cbobj('OperationCompleteSuccess');\n"
		"    form.cb['OperationCompleteFail'] = new form_cbobj('OperationCompleteFail');\n"
		"    form.cb['ObjectDeleted'] = new form_cbobj('ObjectDeleted');\n"
		"    form.cb['ObjectCreated'] = new form_cbobj('ObjectCreated');\n"
		"    form.cb['ObjectModified'] = new form_cbobj('ObjectModified');\n"
		"    form.cb['_3bConfirmCancel'] = new form_cbobj('_3bConfirmCancel');\n"
		"    form.cb['_3bConfirmSave'] = new form_cbobj('_3bConfirmSave');\n"
		"    form.cb['_3bConfirmDiscard'] = new form_cbobj('_3bConfirmDiscard');\n"
		/** initialize params from .app file **/
		"    form.allowquery = aq;\n"
		"    form.allownew = an;\n"
		"    form.allowmodify = am;\n"
		"    form.allowview = av;\n"
		"    form.allownodata = and;\n"
		"    form.multienter = me;\n"
		/** initialize actions and callbacks **/
		"    form._3bconfirmwindow = _3b;\n"
		"    form._3bconfirm_discard = form_3bconfirm_discard;\n"
		"    form._3bconfirm_cancel = form_3bconfirm_cancel;\n"
		"    form._3bconfirm_save = form_3bconfirm_save;\n"
		"    form.show3bconfirm = form_show_3bconfirm;\n"
		"    form.Actiontest3bconfirm = form_test_3bconfirm;\n"
		"    form.ActionClear = form_action_clear;\n"
		"    form.ActionDelete = form_action_delete;\n"
		"    form.ActionDiscard = form_action_discard;\n"
		"    form.ActionEdit = form_action_edit;\n"
		"    form.ActionFirst = form_action_first;\n"
		"    form.ActionLast = form_action_last;\n"
		"    form.ActionNew = form_action_new;\n"
		"    form.ActionPrev = form_action_prev;\n"
		"    form.ActionNext = form_action_next;\n"
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
		/** noone else should call these.... **/
		"    form.ClearAll = form_clear_all;\n"
		"    form.DisableAll = form_disable_all;\n"
		"    form.EnableAll = form_enable_all;\n"
		"    form.ReadOnlyAll = form_readonly_all;\n"
		"    form.ChangeMode = form_change_mode;\n"
		"    form.InitQuery = form_init_query;\n"
		"    return form;\n"
		"    }\n",0);
	//nmFree(sbuf3,800);

	/** Write out the init line for this instance of the form
	 **   the name of this instance was defined to be global up above
	 **   and fm_current is defined in htdrv_page.c 
	 **/
	sbuf3 = nmMalloc(FORM_BUF_SIZE);
	snprintf(sbuf3,FORM_BUF_SIZE,"\n    %s=fm_current=form_init(%i,%i,%i,%i,%i,%i,'%s','%s','%s',%s,%i);\n",
		name,allowquery,allownew,allowmodify,allowview,allownodata,multienter,name,
		basequery,basewhere,_3bconfirmwindow,readonly);
	htrAddScriptInit(s,sbuf3);
	nmFree(sbuf3,FORM_BUF_SIZE);

	/** Check for and render all subobjects. **/
	/** non-visual, don't consume a z level **/
	htrRenderSubwidgets(s, w_obj, parentname, parentobj, z);
	

	htrAddScriptInit(s,"    fm_current.InitQuery();");

	/** Make sure we don't claim orphans **/
	htrAddScriptInit(s,"    fm_current = null;\n\n");

	nmFree(basequery,FORM_BUF_SIZE);
	nmFree(basewhere,FORM_BUF_SIZE);

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
