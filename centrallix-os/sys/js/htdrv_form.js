// Copyright (C) 1998-2001 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function form_cb_register(element)
    {
    if(element.kind=="formstatus")
	{
	this.statuswidgets.push(element);
	element.setvalue(this.mode);
	}
    else
	{
	this.elements.push(element);
	}
    }

/** A child 'control' has changed it's data **/
function form_cb_data_notify(control)
    {
    control._form_IsChanged=true;
    if(this.mode=='New' || this.mode=='Modify')
	{
	if(!this.IsUnsaved)
	    {
	    this.IsUnsaved=true;
	    this.is_savable = true;
	    this.SendEvent('StatusChange');
	    this.SendEvent('DataChange');
	    }
	}
    }

/** A child 'control' got or lost focus **/
function form_cb_focus_notify(control)
    {
    if(this.mode=='View')
	{
	this.ChangeMode('Modify');
	}
    if(this.mode=='NoData')
	{
	this.ChangeMode('New');
	}
    }

/** the tab key was pressed in child 'control' **/
function form_cb_tab_notify(control)
    {
    var ctrlnum = 0;
    var origctrl;
    for(var i=0;i<this.elements.length;i++)
	{
	if(this.elements[i].id==control.id)
	    {
	    ctrlnum=i;
	    break;
	    }
	}
    origctrl = ctrlnum;
    while(1)
	{
	ctrlnum = (ctrlnum+1)%this.elements.length;
	if (this.elements[ctrlnum])
	    {
	    if (pg_removekbdfocus())
		{
		if (pg_setkbdfocus(this.elements[ctrlnum], null, null, null)) break;
		}
	    }
	if (ctrlnum == origctrl) break;
	}
    //if(this.elements[ctrlnum+1])
//	{
	/* bounce the tab up the call tree */
//	} else {
	/* set the focus */
//	}
    }

/** User pressed escape key **/
function form_cb_esc_notify(control)
    {
    /** Do a discard **/
    if (control == pg_curkbdlayer && pg_removekbdfocus())
	this.ifcProbe(ifAction).Invoke("Discard");
    }

/** User pressed RETURN key **/
function form_cb_ret_notify(control)
    {
    /** In query mode, exec the query **/
    if (this.mode == 'Query')
	{
	if (control == pg_curkbdlayer && pg_removekbdfocus())
	    return this.ifcProbe(ifAction).Invoke("QueryExec");
	}
    /** In new or modify mode, do a save **/
    if (this.mode == 'New' || this.mode == 'Modify')
	{
	if (control == pg_curkbdlayer && pg_removekbdfocus())
	    {
	    if (this.is_savable)
		return this.ifcProbe(ifAction).Invoke("Save");
	    else
		return this.ifcProbe(ifAction).Invoke("Discard");
	    }
	}
    }

/** Objectsource says there's data ready **/
function form_cb_data_available(aparam)
    {
    this.cb["DataAvailable"].run();
    }

/** Objectsource wants us to dump our data **/
function form_cb_is_discard_ready()
    {
    if(!this.IsUnsaved)
	{
	this.osrc.QueryContinue(this);
	return 0;
	}
    var savefunc=new Function("this.cb['OperationCompleteSuccess'].add(this,new Function('this.osrc.QueryContinue(this);'));this.cb['OperationCompleteFail'].add(this,new Function('this.osrc.QueryCancel(this);'));this.ifcProbe(ifAction).Invoke(\"Save\");");
    this.cb['_3bConfirmSave'].add(this,savefunc);
    this.cb['_3bConfirmDiscard'].add(this,new Function('this.ClearAll();this.osrc.QueryContinue(this);'));
    this.cb['_3bConfirmCancel'].add(this,new Function('this.osrc.QueryCancel(this);'));
    this.show3bconfirm();
    return 0;
    
    if(confirm("OK to save or discard changes, CANCEL to stay here"))
	{
	if(confirm("OK to save changes, CANCEL to discard them."))
	    { /* save */
	    this.cb["OperationCompleteSuccess"].add(this,new Function("this.osrc.QueryContinue(this);"))
	    this.cb["OperationCompleteFail"].add(this,new Function("this.osrc.QueryCancel(this);"))
	    this.ifcProbe(ifAction).Invoke("Save");
	    }
	else
	    { /* cancel (discard) */
	    this.osrc.QueryContinue(this);
	    }
	}
    else
	{ /* cancel (don't allow new query) */
	this.osrc.QueryCancel(this);
	}
    }

/** Objectsource says our object is available **/
function form_cb_object_available(data)
    {
    if (data)
	{
	if(this.mode!='View') this.ChangeMode('View');
	}
    else
	{
	if(this.mode!='NoData') this.ChangeMode('NoData');
	this.recid = 1;
	this.lastrecid = 1;
	}
    this.data=data;
    this.ClearAll();
    if (this.data)
	{
	if (this.didsearch && (this.didsearchlast || data.id == this.recid))
	    this.lastrecid = data.id;
	this.recid = data.id;
	for(var i in this.elements)
	    {
	    if(this.elements[i]._form_fieldid==undefined)
		{
		for(var j in this.data)
		    {
		    if(this.elements[i].fieldname && this.elements[i].fieldname==this.data[j].oid)
			{
			this.elements[i]._form_type=data[j].type;
			this.elements[i]._form_fieldid=j;
			}
		    }
		if(this.elements[i]._form_fieldid==undefined)
		    this.elements[i]._form_fieldid=null;
		}
	    if(this.elements[i]._form_fieldid!=null && this.data[this.elements[i]._form_fieldid].value)
		{
		this.elements[i].setvalue(this.data[this.elements[i]._form_fieldid].value);
		cx_set_hints(this.elements[i], this.data[this.elements[i]._form_fieldid].hints, 'data');
		}
	    else
		{
		this.elements[i].clearvalue();
		}
	    }
	}
    this.didsearch = false;
    this.didsearchlast = false;
    }

/** Objectsource says the operation is complete **/
function form_cb_operation_complete(r)
    {
    if(r)
	this.cb['OperationCompleteSuccess'].run();
    else
	this.cb['OperationCompleteFail'].run();
    }

/** Objectsource says the object was deleted **/
function form_cb_object_deleted()
    {
    this.cb['ObjectDeleted'].run();
    }

/** Objectsource says the object was created **/
function form_cb_object_created()
    {
    this.cb['ObjectCreated'].run();
    }

/** Objectsource says the object was modified **/
function form_cb_object_modified()
    {
    this.cb['ObjectModified'].run();
    }

/** Moves form to "NoData" mode **/
/**   If unsaved data exists (New or Modify mode), prompt for Save/Discard **/
/** Clears any rows in osrc replica (doesn't delete from DB) **/
function form_action_clear(aparam)
    {
    if(this.mode=="NoData")
	return; /* Already in NoData Mode */
    if(this.IsUnsaved && (this.mode=="New" || this.mode=="Modify"))
	{
	if(confirm("OK to save or discard changes, CANCEL to stay here"))
	    {
	    if(confirm("OK to save changes, CANCEL to discard them."))
		{ /* save */
		this.cb["OperationCompleteSuccess"].add(this,new Function("this.ifcProbe(ifAction).Invoke(\"Clear\");"));
		this.ifcProbe(ifAction).Invoke("Save");
		return 0;
		}
	    else
		{ /* discard is ok */
		this.ClearAll();
		}
	    }
	else
	    { /* user wants to stay here */
	    return 0;
	    }
	}
    this.ClearAll();
    }

/** in New - clear, remain in New **/
/** in View - delete the record, move to View mode on the next record, or NoData **/
/** in Query - clear, remain in Query **/
function form_action_delete(aparam)
    {
    switch(this.mode)
	{
	case "New":
	    this.ClearAll();
	    break;
	case "View":
	    dataobj = this.BuildDataObj();
	    this.osrc.ifcProbe(ifAction).Invoke("Delete", {data:dataobj, client:this});
	    break;
	case "Query":
	    this.ClearAll();
	    break;
	default:
	    break; /* else do nothing */
	}
    }

/** in Modify, cancel changes, switch to 'View' **/
/** in New, cancel changes, switch to 'NoData' **/
/** in Query, clear, remain in query mode **/
function form_action_discard(aparam)
    {
    switch(this.mode)
	{
	case "Modify":
	    this.ClearAll();
	    if (this.osrc) this.osrc.MoveToRecord(this.recid);
	    this.ChangeMode("View");
	    break;
	case "New":
	    this.ClearAll();
	    this.ChangeMode("NoData");
	    break;
	case "Query":
	    this.ClearAll();
	    break;
	}
    }

/** in NoData, move to New **/
/** in View, move to Modify (note this can be implicit on click -- can be disabled) **/
function form_action_edit(aparam)
    {
    switch(this.mode)
	{
	case "NoData":
	    this.ChangeMode("New");
	    break;
	case "View":
	    this.ChangeMode("Modify");
	    break;
	}
    }

function form_show_3bconfirm()
    {
    var discard,save,cancel;
    if (!this._3bconfirmwindow || !wgtrIsNode(this._3bconfirmwindow) || wgtrGetType(this._3bconfirmwindow) != 'widget/childwindow')
	{
	alert('Some data is unsaved.  Please either save or cancel your changes first.');
	this._3bconfirm_cancel();
	return false;
	}
    var nodes = wgtrAllSubNodes(this._3bconfirmwindow);
    for(var n in nodes)
	{
	if (nodes[n].buttonName == '_3bConfirmCancel' || nodes[n].buttonText == 'Cancel')
	    cancel = nodes[n];
	if (nodes[n].buttonName == '_3bConfirmSave' || nodes[n].buttonText == 'Save')
	    save = nodes[n];
	if (nodes[n].buttonName == '_3bConfirmDiscard' || nodes[n].buttonText == 'Discard')
	    discard = nodes[n];
	}
    if(!(save && discard && cancel))
	{
	alert("You didn't use all the buttons, or you named them wrong.");
	return 0;
	}
/** set up pointers so the EventClick method can find this form... **/
    save._form_form=this;
    discard._form_form=this;
    cancel._form_form=this;
    
    save.ifcProbe(ifEvent).Clear("Click");
    save.ifcProbe(ifEvent).Hook("Click", this._3bconfirm_save);
    discard.ifcProbe(ifEvent).Clear("Click");
    discard.ifcProbe(ifEvent).Hook("Click", this._3bconfirm_discard);
    cancel.ifcProbe(ifEvent).Clear("Click");
    cancel.ifcProbe(ifEvent).Hook("Click", this._3bconfirm_cancel);
    //save.EventClick=this._3bconfirm_save;
    //discard.EventClick=this._3bconfirm_discard;
    //cancel.EventClick=this._3bconfirm_cancel;
    
    var funclate=new Function("this.cb['_3bConfirmCancel'].clear();this.cb['_3bConfirmDiscard'].clear();this.cb['_3bConfirmSave'].clear();");
    var func=new Function("pg_setmodal(null);var v=new Object();v.IsVisible=0;this._3bconfirmwindow.ifcProbe(ifAction).Invoke(\"Close\", {});");
    
/** funclate will fire last (or very close, with a level of 1000) **/
    this.cb['_3bConfirmCancel'].add(this,funclate,null,1000);
    this.cb['_3bConfirmDiscard'].add(this,funclate,null,1000);
    this.cb['_3bConfirmSave'].add(this,funclate,null,1000);
    
/** func will fire first (or very close, with a level of -1000) **/
    this.cb['_3bConfirmCancel'].add(this,func,null,-1000);
    this.cb['_3bConfirmDiscard'].add(this,func,null,-1000);
    this.cb['_3bConfirmSave'].add(this,func,null,-1000);
    
/** set modal and make the user do something
 **   modal will be unset and we'll get control back via callbacks **/
    this._3bconfirmwindow.ifcProbe(ifAction).Invoke("Open", {IsModal:true, NoClose:true});
    //pg_setmodal(this._3bconfirmwindow.ContentLayer)
    
    }

function form_3bconfirm_cancel()
    {
    if(this._form_form)
	this._form_form.cb['_3bConfirmCancel'].run();
    }

function form_3bconfirm_discard()
    {
    if(this._form_form)
	this._form_form.cb['_3bConfirmDiscard'].run();
    }

function form_3bconfirm_save()
    {
    if(this._form_form)
	this._form_form.cb['_3bConfirmSave'].run();
    }

function form_test_3bconfirm()
    {
    this.cb['_3bConfirmCancel'].add(this,new Function("alert('cancel was clicked');"))
    this.cb['_3bConfirmDiscard'].add(this,new Function("alert('discard was clicked');"))
    this.cb['_3bConfirmSave'].add(this,new Function("alert('save was clicked');"))
    this.show3bconfirm();
    }

/** in View, Query, or NoData, clear, move to New **/
/** in Modify, give save/discard/cancel prompt **/
/** (if from NoData or View mode after query, auto-fill criteria) **/
function form_action_new(aparam)
    {
    switch(this.mode)
	{
	case "Query":
	case "NoData":
	case "View":
	case "Modify":
	    this.ChangeMode("New");
	    //this.ClearAll(); -- GRB this is done in change_mode now because of pres hints stuff
	    /* if there was a query run, fill in it's values... */
	    break;
	}
    }

function form_action_view(aparam)
    {
    this.ChangeMode("View");
    }

/** tell osrc to go to first record **/
function form_action_first(aparam)
    {
    this.osrc.ifcProbe(ifAction).Invoke("First", {});
    return 0;
    }

/** tell osrc to go to last record **/
function form_action_last(aparam)
    {
    this.didsearchlast = true;
    this.didsearch = true;
    this.osrc.ifcProbe(ifAction).Invoke("Last", {});
    return 0;
    }

/** Save changed data, move the osrc **/
function form_action_next(aparam)
    {
    this.didsearch = true;
    this.osrc.ifcProbe(ifAction).Invoke("Next", {});
    return 0;
    }

/** Save changed data, move the osrc **/
function form_action_prev(aparam)
    {
    this.osrc.ifcProbe(ifAction).Invoke("Prev", {});
    return 0;
    }

/** Helper function -- called from other mode change functions **/
function form_change_mode(newmode)
    {
    if (newmode == 'Modify' && !this.allowmodify)
	return;
    else if (newmode == 'View' && !this.allowview)
	return;
    else if (newmode == 'Query' && !this.allowquery)
	return;
    else if (newmode == 'New' && !this.allownew)
	return;
    else if (newmode == 'NoData' && !this.allownodata)
	return;

    if (newmode == this.mode) return;

    // Control button behavior
    this.is_discardable = (newmode == 'Query' || newmode == 'New' || newmode == 'Modify');
    this.is_editable = (newmode == 'View');
    this.is_newable = (newmode == 'View' || newmode == 'NoData' || newmode == 'Query');
    this.is_queryable = (newmode == 'View' || newmode == 'NoData');
    this.is_queryexecutable = (newmode == 'Query');

    //confirm('Form is going from '+this.mode+' to '+newmode+' mode.');
    if(this.mode=='Modify' && this.IsUnsaved)
	{
	var savefunc=new Function("this.cb['OperationCompleteSuccess'].add(this,new Function('this.ifcProbe(ifAction).Invoke(\""+newmode+"\", {});'));this.ifcProbe(ifAction).Invoke(\"Save\", {});");
	var discardfunc=new Function("this.IsUnsaved=false;this.is_savable=false;this.ChangeMode('"+newmode+"');");
	this.cb['_3bConfirmDiscard'].add(this,discardfunc);
	this.cb['_3bConfirmSave'].add(this,savefunc);
	this.show3bconfirm();
	return;
	}
    this.oldmode = this.mode;
    this.mode = newmode;
    this.IsUnsaved = false;
    this.is_savable = false;
    
    // Set focus to initial control
    if ((newmode == 'Query' || newmode == 'New' || newmode == 'Modify') && this.elements[0] && !pg_curkbdlayer)
	{
	for(var i = 0; i < this.elements.length; i++)
	    {
	    if (pg_removekbdfocus())
		{
		if (pg_setkbdfocus(this.elements[i], null, null, null)) break;
		}
	    }
	}

    if(this.mode=='Query')
	{
	this.ClearAll();
/** This would auto-fill the form boxes with the last query, but has
 **   other issues -- how do you tell what fields to query on?
 **   -- you can't go on what fields were changed, can't use true value, etc.
 **   ---- disabled for now -- Jonathan Rupp 6/23/2002
 **/
/*
	for(var j in this.osrc.queryobject)
	    if(j!='oid' && j!='joinstring')
		for(var i in this.elements)
		    if(this.elements[i].fieldname==this.osrc.queryobject[j].oid)
			this.elements[i].setvalue(this.osrc.queryobject[j].value);
*/
	}
    
//"    alert(newmode + ' - ' + this.oldmode);
    if(this.mode=='NoData' || this.mode=='View')
	{
	this.EnableModifyAll();
	}
    if(this.mode=='Query' || this.mode=='New')
	{
	this.EnableNewAll();
	this.ClearAll();
	}
    for(var i in this.statuswidgets)
	{
	this.statuswidgets[i].setvalue(this.mode);
	}

    // on New or Modify, call appropriate hints routines
    if (newmode == 'New')
	{
	for(var e in this.elements)
	    {
	    if (this.elements[e].cx_hints) 
		{
		cx_hints_setup(this.elements[e]);
		cx_hints_startnew(this.elements[e]);
		}
	    }
	}
    else if (newmode == 'Modify')
	{
	for(var e in this.elements)
	    {
	    if (this.elements[e].cx_hints) cx_hints_startmodify(this.elements[e]);
	    }
	}

    this.SendEvent('StatusChange');
    this.SendEvent(this.mode);
    return 1;
    }

function form_send_event(event)
    {
    //confirm(eval('this.Event'+event));
    if (!this.ifcProbe(ifEvent).Exists(event)) return 1;
    //if(!eval('this.Event'+event)) return 1;
    var evobj = new Object();
    evobj.Caller = this;
    evobj.Status = this.mode;
    evobj.IsUnsaved = this.IsUnsaved;
    evobj.is_savable = this.is_savable;
    cn_activate(this, event, evobj);
    delete evobj;
    }

/** Clears all children, also resets IsChanged/IsUnsaved flag to false **/
function form_clear_all()
    {
    for(var i in this.elements)
	{
	this.elements[i].clearvalue();
	this.elements[i]._form_IsChanged=false;
	}
    this.IsUnsaved=false;
    this.is_savable = false;
    //this.osrc.ifcProbe(ifAction).Invoke("Clear");
    }

/** Disables all children **/
function form_disable_all()
    {
    for(var i in this.elements)
	{
	this.elements[i].disable();
	}
    }

/** Enables all children (for modify) **/
function form_enable_modify_all()
    {
    for(var i in this.elements)
	{
	if(this.elements[i].enablemodify)
	    this.elements[i].enablemodify();
	else
	    this.elements[i].enable();
	}
    }

/** Enables all children (for modify) **/
function form_enable_new_all()
    {
    for(var i in this.elements)
	{
	if(this.elements[i].enablenew)
	    this.elements[i].enablenew();
	else
	    this.elements[i].enable();
	}
    }


/** make all children readonly**/
function form_readonly_all()
    {
    for(var i in this.elements)
	{
	this.elements[i].readonly();
	}
    }

/** Change to query mode **/
function form_action_query()
    {
    if(!this.allowquery) {alert('Query mode not allowed');return 0;}
    return this.ChangeMode('Query');
    }

/** go to query mode, or exec query **/
function form_action_querytoggle()
    {
    if(this.mode=='Query')
	return this.ifcProbe(ifAction).Invoke("QueryExec");
    else
	return this.ifcProbe(ifAction).Invoke("Query");
    }

/** Execute query **/
function form_action_queryexec()
    {
    if(!this.allowquery) {alert('Query not allowed');return 0;}
    if(!(this.mode=='Query')) {alert("You can't execute a query if you're not in Query mode.");return 0;}
    var where = new String;
    var firstone = true;
/** build an query object to give the osrc **/
    var query=new Array();
    query.oid=null;
    query.joinstring='AND';
    this.recid = 1;
    this.lastrecid = null;
    
    for(var i in this.statuswidgets)
	{
	this.statuswidgets[i].setvalue('QueryExec');
	}
    for(var i in this.elements)
	{
	if(this.elements[i]._form_IsChanged)
	    {
	    var v = this.elements[i].getvalue();
	    if (v != null)
		{
		var t=new Object();
		t.oid=this.elements[i].fieldname;
		t.value=v;
		t.type=this.elements[i]._form_type;
		query.push(t);
		}
	    }
	}
/** Done with the query -- YEAH **/
    //if(confirm('Send to "'+this.osrc.name+'"(osrc):'+query))
	{
	this.Pending=true;
	this.IsUnsaved=false;
	this.is_savable = false;
	//this.cb['DataAvailable'].add(this,new Function('this.osrc.ifcProbe(ifAction).Invoke("First", this)'));
	this.osrc.ifcProbe(ifAction).Invoke("QueryObject", {query:query, client:this, ro:this.readonly});
	}
    delete query;
    }

function form_build_dataobj()
    {
    var dataobj=new Array();
    for(var i in this.elements)
	{
	if(this.elements[i]._form_IsChanged)
	    {
	    var t=new Object();
	    t.oid=this.elements[i].fieldname;
	    t.value=this.elements[i].getvalue();
	    t.type=this.elements[i]._form_type;
	    dataobj.push(t);
	    }
	}
    if (this.mode == "New" || !this.data)
	dataobj.oid = 0;
    else
	dataobj.oid=this.data.oid;
    return dataobj;
    }

function form_action_save_success()
    {
    this.IsUnsaved=false;
    this.is_savable=false;
    this.Pending=false;
    this.EnableModifyAll();
    this.ifcProbe(ifAction).Invoke("View");
    for(var i in this.elements)
	this.elements[i]._form_IsChanged=false;
    this.cb['OperationCompleteFail'].clear();
    this.SendEvent("DataSaved");
    }

function form_action_save()
    {
    if(!this.IsUnsaved)
	{
	return 0;
	}
    //this.cb['OperationCompleteSuccess'].add(this,
    //	   new Function("this.IsUnsaved=false;this.is_savable=false;this.Pending=false;this.EnableModifyAll();this.ifcProbe(ifAction).Invoke(\"View\");this.cb['OperationCompleteFail'].clear();"),null,-100);
    this.cb['OperationCompleteSuccess'].add(this, new Function("this.ActionSaveSuccessCB();"), null, -100);
    this.cb['OperationCompleteFail'].add(this,
	   new Function("this.Pending=false;this.EnableModifyAll();confirm('Data Save Failed');this.cb['OperationCompleteSuccess'].clear();"),null,-100);
    
/** build the object to pass to objectsource **/
    dataobj = this.BuildDataObj();
    this.DisableAll();
    this.Pending=true;
    if (this.osrc)
	{
	if (this.mode == 'New')
	    this.osrc.ifcProbe(ifAction).Invoke("Create", {data:dataobj, client:this});
	else
	    this.osrc.ifcProbe(ifAction).Invoke("Modify", {data:dataobj, client:this});
	}
    else
	{
	this.Pending=false;
	this.ChangeMode('View');
	}
    }


function form_action_submit(aparam)
    {
    var node;
    if (aparam.Target)
	node = wgtrGetNode(this, aparam.Target);
    else
	node = wgtrFindContainer(this, "widget/page");
    if (!node) return false;
    var param = new Object;
    for(var ap in aparam)
	{
	if (ap != 'Target' && ap != 'NewPage')
	    param[ap] = aparam[ap];
	}
    for(var i in this.elements)
	{
	var v = this.elements[i].getvalue();
	if (v != null)
	    {
	    param[this.elements[i].fieldname] = v;
	    }
	}
    var nodetype = wgtrGetType(node);
    if (nodetype == "widget/component")
	node.ifcProbe(ifAction).Invoke("Instantiate", param);
    else if (nodetype == "widget/page" && aparam.NewPage)
	node.ifcProbe(ifAction).Invoke("Launch", param);
    else if (nodetype == "widget/page")
	node.ifcProbe(ifAction).Invoke("LoadPage", param);
    else
	return false;
    }


/** Helper function to build a query */
function form_build_query(base,where,ro)
    {
    re=/where/i;
    if(!where)
	if(ro)
	    return base;
	else
	    return base+' FOR UPDATE';
    if(re.test(base))
	if(ro)
	   return base+' AND '+where;
	else
	   return base+' AND '+where+' FOR UPDATE';
    else
	if(ro)
	    return base+' WHERE '+where;
	else
	    return base+' WHERE '+where+' FOR UPDATE';
    }


/***
 ***   The following callback helper functions might be
 ***   useful if available to all widgets.
 ***   For now, just the form will use them though
 ***/

/** Callback object init **/
function form_cbobj(n) //FIXME FIXME
    {
    this.arr=new Array();
    this.add=form_cbobj_add;
    this.run=form_cbobj_run;
    this.clear=form_cbobj_clear;
    this.name=n;
    }

/** Calls all registered functions **/
function form_cbobj_run()
    {
    if(this.arr.length<1)
	{
	//confirm('There are no callbacks registered for '+this.name+'.  This is a problem');
	return false;
	}
    this.arr.sort(form_cbobj_compare);
    for(var i in this.arr)
	{
	var j=this.arr[i];
	//if(j && confirm(j))
	if(j)
	    if(j[3])
		j[2].apply(j[1],j[3]);
	    else
		j[2].apply(j[1]);
	}
    this.clear();
    }

/** Adds a new registered function **/
function form_cbobj_add(obj,func,param,key)
    {
    if(obj==undefined) { obj = null; }
    if(func==undefined) { func = null; }
    if(param==undefined) { param = null; }
    if(key==undefined) { key=0; }
    this.arr.push(new Array(key,obj,func,param));
    }

/** Clears all registered callbacks **/
function form_cbobj_clear()
    {
    for(var i in this.arr)
	delete this.arr[i];
    }

/** Compares two arrays based on the first element **/
function form_cbobj_compare(a,b)
    { /* sort in order based on keys (element 0) */
    if(!a || !b) return 0;
    if(a[0]>b[0]) return 1;
    if(a[0]==b[0]) return 0;
    if(a[0]<b[0]) return -1;
    }

/** Determines if form is in QBF (query) mode **/
function form_cb_is_query_mode()
    {
    return (this.mode == 'Query');
    }

/** Called when a form element is 'revealed' or 'obscured' **/
function form_cb_reveal(element,event)
    {
    //alert(this.name + ' got ' + event.eventName);
    switch(event.eventName)
	{
	case 'Reveal':
	    this.revealed_elements++;
	    if (this.revealed_elements != 1) return 0;
	    if (this.osrc) this.osrc.Reveal(this);
	    break;
	case 'Obscure':
	    this.revealed_elements--;
	    if (this.revealed_elements != 0) return 0;
	    if (this.osrc) this.osrc.Obscure(this);
	    break;
	case 'RevealCheck':
	    pg_reveal_check_ok(event);
	    break;
	case 'ObscureCheck':
	    // unsaved data?
	    if (!this.allowobscure)
		{
		if (this.IsUnsaved)
		    {
		    this._orsevent = event;
		    var savefunc=new Function("this.cb['OperationCompleteSuccess'].add(this,new Function('pg_reveal_check_ok(this._orsevent);'));this.cb['OperationCompleteFail'].add(this,new Function('pg_reveal_check_veto(this._orsevent);'));this.ifcProbe(ifAction).Invoke(\"Save\");");
		    this.cb['_3bConfirmSave'].add(this,savefunc);
		    this.cb['_3bConfirmDiscard'].add(this,new Function('this.ifcProbe(ifAction).Invoke("Discard");pg_reveal_check_ok(this._orsevent);'));
		    this.cb['_3bConfirmCancel'].add(this,new Function('pg_reveal_check_veto(this._orsevent);'));
		    this.show3bconfirm();
		    }
		else
		    {
		    this.ifcProbe(ifAction).Invoke("Discard");
		    pg_reveal_check_ok(event);
		    }
		}
	    else
		{
		pg_reveal_check_ok(event);
		}
	    break;
	}
    return 0;
    }

/** Form initializer
 ** aq - allow query?
 ** an - allow new?
 ** am - allow modify?
 ** av - allow view?
 ** and - allow no-data?
 ** me - multi-entry mode?
 ** name - name of the form
 ** _3b - three-button confirm window
 ** ro - is the form readonly?
 **/
function form_init(form,param)
    {
    //var form = new Object();
    ifc_init_widget(form);
    form.readonly=param.ro;
    form.elements = new Array();
    form.statuswidgets = new Array();
    form.mode = 'NoData';
    form.cobj = null; /* current 'object' (record) */
    form.oldmode = null;
    form.didsearchlast = false;
    form.didsearch = false;
    form.revealed_elements = 0;
    form.osrc = wgtrFindContainer(form, "widget/osrc");
    //if (!form.osrc) alert('no osrc container!');
    if(form.osrc)
	{
	form.osrc.Register(form);
	}
    form.IsUnsaved = false;
    form.name = param.name;
    form.Pending = false;
/** remember what to do after callbacks.... **/
    form.cb = new Array();
    form.cb['DataAvailable'] = new form_cbobj('DataAvailable');
    form.cb['ObjectAvailable'] = new form_cbobj('ObjectAvailable');
    form.cb['OperationCompleteSuccess'] = new form_cbobj('OperationCompleteSuccess');
    form.cb['OperationCompleteFail'] = new form_cbobj('OperationCompleteFail');
    form.cb['ObjectDeleted'] = new form_cbobj('ObjectDeleted');
    form.cb['ObjectCreated'] = new form_cbobj('ObjectCreated');
    form.cb['ObjectModified'] = new form_cbobj('ObjectModified');
    form.cb['_3bConfirmCancel'] = new form_cbobj('_3bConfirmCancel');
    form.cb['_3bConfirmSave'] = new form_cbobj('_3bConfirmSave');
    form.cb['_3bConfirmDiscard'] = new form_cbobj('_3bConfirmDiscard');
    form._form_form = form;

/** indicator flags for other objects to use **/
    form.is_savable = false;
    form.is_discardable = false;
    form.is_editable = false;
    form.is_newable = true;
    form.is_queryable = true;
    form.is_queryexecutable = false;
    form.recid = 1;
    form.lastrecid = null;

/** initialize params from .app file **/
    form.allowquery = param.aq;
    form.allownew = param.an;
    form.allowmodify = param.am;
    form.allowview = param.av;
    form.allownodata = param.and;
    form.multienter = param.me;
    form.allowobscure = param.ao;
/** initialize actions and callbacks **/
    form._3bconfirmwindow = param._3b;
    form._3bconfirm_discard = form_3bconfirm_discard;
    form._3bconfirm_cancel = form_3bconfirm_cancel;
    form._3bconfirm_save = form_3bconfirm_save;
    form.show3bconfirm = form_show_3bconfirm;
    form.IsDiscardReady = form_cb_is_discard_ready;
    form.DataAvailable = form_cb_data_available;
    form.ObjectAvailable = form_cb_object_available;
    form.OperationComplete = form_cb_operation_complete;
    form.ObjectDeleted = form_cb_object_deleted;
    form.ObjectCreated = form_cb_object_created;
    form.ObjectModified = form_cb_object_modified;
    form.Register = form_cb_register;
    form.DataNotify = form_cb_data_notify;
    form.FocusNotify = form_cb_focus_notify;
    form.TabNotify = form_cb_tab_notify;
    form.EscNotify = form_cb_esc_notify;
    form.RetNotify = form_cb_ret_notify;
    form.IsQueryMode = form_cb_is_query_mode;
/** noone else should call these.... **/
    form.ClearAll = form_clear_all;
    form.DisableAll = form_disable_all;
    form.EnableModifyAll = form_enable_modify_all;
    form.EnableNewAll = form_enable_new_all;
    form.ReadOnlyAll = form_readonly_all;
    form.ChangeMode = form_change_mode;
    form.SendEvent = form_send_event;
    form.BuildDataObj = form_build_dataobj;
    form.Reveal = form_cb_reveal;
    //form.InitQuery = form_init_query;
    form.ActionSaveSuccessCB = form_action_save_success;

    // Actions
    var ia = form.ifcProbeAdd(ifAction);
    ia.Add("test3bconfirm", form_test_3bconfirm);
    ia.Add("Clear", form_action_clear);
    ia.Add("Delete", form_action_delete);
    ia.Add("Discard", form_action_discard);
    ia.Add("Edit", form_action_edit);
    ia.Add("View", form_action_view);
    ia.Add("First", form_action_first);
    ia.Add("Last", form_action_last);
    ia.Add("New", form_action_new);
    ia.Add("Prev", form_action_prev);
    ia.Add("Next", form_action_next);
    ia.Add("Query", form_action_query);
    ia.Add("QueryExec", form_action_queryexec);
    ia.Add("QueryToggle", form_action_querytoggle);
    ia.Add("Save", form_action_save);
    ia.Add("Submit", form_action_submit);

    // Events
    var ie = form.ifcProbeAdd(ifEvent);
    ie.Add("StatusChange");
    ie.Add("DataChange");
    ie.Add("DataSaved");
    ie.Add("NoData");
    ie.Add("View");
    ie.Add("New");
    ie.Add("Modify");
    ie.Add("Search");

    return form;
    }

