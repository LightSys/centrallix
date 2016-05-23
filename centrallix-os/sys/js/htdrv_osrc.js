// Copyright (C) 1998-2014 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function osrc_init_query()
    {
    if(this.init==true)
	return;
    this.init=true;
    this.ifcProbe(ifAction).Invoke("QueryObject", {query:[], client:null, ro:this.readonly});
    }


function osrc_action_order_object(aparam) //order)
    {
    this.pendingorderobject=aparam.orderobj;
    if (this.querytext)
	this.ifcProbe(ifAction).Invoke("QueryText", {query:this.querytext, client:null, ro:this.readonly, field_list:this.querytext_fields, cx__case_insensitive:this.querytext_icase});
    else
	this.ifcProbe(ifAction).Invoke("QueryObject", {query:this.queryobject, client:null, ro:this.readonly});
    }

function osrc_criteria_from_aparam(aparam)
    {
    var qo = [];
    var t;
    var v;
    qo.joinstring = 'AND';
    for (var i in aparam)
	{
	if (i == 'cx__enable_lists') continue;
	if (i == 'cx__case_insensitive') continue;
	if (i == 'joinstring' && (new String(aparam[i])).toLowerCase() == 'or')
	    {
	    qo.joinstring = 'OR';
	    continue;
	    }
	v = aparam[i];
	if (i == '_Origin' || i == '_EventName') continue;
	if (typeof v == 'string' && (new String(v)).indexOf(',') > 0 && aparam.cx__enable_lists)
	    {
	    t = 'stringarray';
	    v = (new String(v)).split(/,/);
	    }
	else if ((typeof v == 'string' || (typeof v == 'object' && v != null && v.constructor == String)) && aparam.cx__case_insensitive)
	    t = 'istring';
	else if (typeof v == 'string' || (typeof v == 'object' && v != null && v.constructor == String))
	    t = 'string';
	else if (typeof v == 'number')
	    t = 'integer';
	else
	    t = 'string';
	qo.push({oid:i, value:v, type:t});
	}
    return qo;
    }

function osrc_action_query_param(aparam)
    {
    this.init = true;
    if (this.query_delay_schedid)
	{
	pg_delsched(this.query_delay_schedid);
	this.query_delay_schedid = null;
	}
    var qo = osrc_criteria_from_aparam(aparam);
    this.ifcProbe(ifAction).Invoke("QueryObject", {query:qo, client:null, ro:this.readonly});
    }


function osrc_refresh_timer()
    {
    this.refresh_schedid = null;
    this.req_ind_act = false;
    if (this.revealed_children > 0)
	this.ifcProbe(ifAction).Invoke('Refresh', {});
    else
	this.refresh_schedid = pg_addsched_fn(this, 'RefreshTimer', [], this.refresh_interval);
    }


function osrc_action_refresh(aparam)
    {
    var tr = this.CurrentRecord;
    if (!tr || tr < 1) tr = 1;
    this.doing_refresh = true;

    // Keep track of current object by name
    if (this.replica[this.CurrentRecord])
	{
	for(var j=0; j<this.replica[this.CurrentRecord].length;j++)
	    {
	    if (this.replica[this.CurrentRecord][j].oid == 'name')
		{
		this.refresh_objname = this.replica[this.CurrentRecord][j].value;
		break;
		}
	    }
	}
    else
	this.refresh_objname = null;

    if (this.querytext)
	this.ifcProbe(ifAction).Invoke("QueryText", {query:this.querytext, client:null, ro:this.readonly, field_list:this.querytext_fields, cx__case_insensitive:this.querytext_icase, targetrec:tr});
    else
	this.ifcProbe(ifAction).Invoke("QueryObject", {query:this.queryobject, client:null, ro:this.readonly, targetrec:tr});
    }


function osrc_action_change_source(aparam)
    {
    if (!this.baseobj) return null;
    if (!aparam.Source) return null;
    if (aparam.Source == '' || aparam.Source == this.baseobj) return;
    var l = (new String(this.baseobj)).length;
    var newl = (new String(aparam.Source)).length;
    var s = new String(this.sql);
    var p = s.indexOf(this.baseobj);
    while (p >= 0)
	{
	s = s.substr(0,p) + aparam.Source + s.substr(p+l);
	p = s.indexOf(this.baseobj, p+newl);
	}
    this.sql = s;
    var s = new String(this.query);
    var p = s.indexOf(this.baseobj);
    while (p >= 0)
	{
	s = s.substr(0,p) + aparam.Source + s.substr(p+l);
	p = s.indexOf(this.baseobj, p+newl);
	}
    this.query = s;
    this.baseobj = aparam.Source;
    if (typeof aparam.Refresh == 'undefined' || aparam.Refresh)
	this.ifcProbe(ifAction).Invoke("Refresh", {});
    }


function osrc_action_query_text(aparam)
    {
    this.init = true;
    if (this.query_delay_schedid)
	{
	pg_delsched(this.query_delay_schedid);
	this.query_delay_schedid = null;
	}
    this.QueueRequest({Request:'QueryText', Param:aparam});
    this.Dispatch();
    }

function osrc_query_text_handler(aparam)
    {
    var formobj = aparam.client;
    var appendrows = (aparam.cx__appendrows)?true:false;
    var statement=this.sql;
    var case_insensitive = (aparam.cx__case_insensitive)?true:false;

    var sel_re = /^\s*(set\s+rowcount\s+[0-9]+\s+)?select\s+/i;
    var is_select = sel_re.test(this.sql);

    if (this.use_having)
	var osrcsep = ' HAVING ';
    else
	var osrcsep = ' WHERE ';
    if (aparam.use_having)
	var sep = ' HAVING ';
    else
	var sep = osrcsep;

    var fieldlist = (new String(aparam.field_list)).split(',');
    var searchlist = (new String(aparam.query?aparam.query:'')).split(' ');
    var objname = aparam.objname?aparam.objname:null;
    if (aparam.min_length > 0)
	var min_length = aparam.min_length;
    else
	var min_length = 2;

    this.move_target = aparam.targetrec;

    if (!aparam.fromsync)
	this.SyncID = osrc_syncid++;

    // Evaluate default expression on parameters...
    for(var pn in this.params)
	{
	this.params[pn].pwgt.ifcProbe(ifAction).Invoke("SetValue", {Value:null});
	}

    // build the search string from the criteria and field list
    var filter = '';
    var firstone=true;
    if (searchlist.length > 0 && fieldlist.length > 0)
	{
	for(var i=0; i<searchlist.length; i++)
	    {
	    var s = searchlist[i];
	    if (!s) continue;
	    if (!firstone) filter += ' and ';
	    filter += '(';

	    var firstfield=true;
	    for(var j=0; j<fieldlist.length; j++)
		{
		var f = fieldlist[j];
		var criteria = s;

		// asterisks on either side of field name indicate substring searching with wildcards.
		if (f.indexOf('*') == 0 && criteria.substr(0, 1) != '*')
		    {
		    criteria = '*' + criteria;
		    f = f.substr(1);
		    }
		if (f.lastIndexOf('*') == f.length - 1 && criteria.substr(criteria.length - 1, 1) != '*')
		    {
		    criteria = criteria + '*';
		    f = f.substr(0, f.length - 1);
		    }

		if (!firstfield) filter += ' or ';
		filter += this.MakeFilterString({oid:f, obj:objname}, criteria, case_insensitive);

		firstfield = false;
		}
	    filter += ')';
	    firstone = false;
	    }
	if (filter.length > 0)
	    statement += (sep + '('+filter+')');
	}

    // add any preset filtering
    if (this.filter)
	{
	/*if (!firstone)
	    filter += ' and ';
	else*/
	    filter += osrcsep;
	statement += '(' + this.filter + ')';
	firstone = false;
	}

    // add any relationships
    var rel = [];
    this.ApplyRelationships(rel, false);
    if (rel[0])
	{
	/*if (!firstone)
	    statement += ' and ';
	else*/
	    statement += osrcsep;
	rel.joinstring = 'AND';
	statement += '(' + this.MakeFilter(rel) + ')';
	firstone = false;
	}

    // add any order-by
    var firstone=true;
    if(this.pendingorderobject && is_select)
	for(var i in this.pendingorderobject)
	    {
	    if(firstone)
		{
		statement+=' ORDER BY '+this.pendingorderobject[i];
		firstone=false;
		}
	    else
		{
		statement+=', '+this.pendingorderobject[i];
		}
	    }

    this.querytext = aparam.query;
    this.querytext_fields = aparam.field_list;
    this.querytext_icase = case_insensitive;
    this.queryobject = null;

    this.ifcProbe(ifAction).Invoke("Query", {query:statement, client:formobj, appendrows:appendrows});
    }


function osrc_action_query_object(aparam) //q, formobj, readonly)
    {
    this.init = true;
    if (this.query_delay_schedid)
	{
	pg_delsched(this.query_delay_schedid);
	this.query_delay_schedid = null;
	}
    this.QueueRequest({Request:'QueryObject', Param:aparam});
    this.Dispatch();
    }

function osrc_query_object_handler(aparam)
    {
    var formobj = aparam.client;
    var q = aparam.query;
    var readonly = aparam.ro;
    var appendrows = (aparam.cx__appendrows)?true:false;
    if (typeof aparam.cx__appendrows != 'undefined')
	delete aparam.cx__appendrows;
    var params = [];
    var filter = [];

    if(this.pending)
	{
	alert('There is already a query or movement in progress...');
	return 0;
	}

    this.move_target = aparam.targetrec;

    if (typeof q != 'undefined' && q !== null) this.ApplyRelationships(q, false);

    for(var i in q)
	{
	if (i == 'joinstring')
	    filter.joinstring = q.joinstring;
	else if (i == 'oid')
	    filter.oid = q.oid;
	else if (this.params[q[i].oid])
	    params.push(q[i]);
	else
	    filter.push(q[i]);
	}
    if (!filter.joinstring)
	filter.joinstring = 'AND';
   
    for(var pn in this.params)
	{
	var found = false;
	for(var i in params)
	    {
	    if (params[i].oid == pn && typeof params[i].value != 'undefined')
		{
		found = true;
		this.params[pn].pwgt.ifcProbe(ifAction).Invoke("SetValue", {Value:params[i].value});
		break;
		}
	    }
	if (!found)
	    {
	    this.params[pn].pwgt.ifcProbe(ifAction).Invoke("SetValue", {Value:null});
	    }
	}

    if (!aparam.fromsync)
	this.SyncID = osrc_syncid++;

    var sel_re = /^\s*(set\s+rowcount\s+[0-9]+\s+)?select\s+/i;
    var is_select = sel_re.test(this.sql);

    this.pendingqueryobject=q;
    this.querytext = null;
    var statement=this.sql;

    if (this.use_having)
	var sep = ' HAVING ';
    else
	var sep = ' WHERE ';
    var firstone=true;
    
    if(this.filter)
	{
	statement+= (sep + '('+this.filter+')');
	firstone=false;
	}
    if(filter && filter.joinstring && filter[0])
	{
	var filt = this.MakeFilter(filter);
	if (filt)
	    {
	    if(firstone)
		statement+=sep;
	    else
		statement+=' '+q.joinstring+' ';
	    statement+=filt;
	    }
	}
    
    firstone=true;
    if(this.pendingorderobject && is_select)
	for(var i in this.pendingorderobject)
	    {
	    if(firstone)
		{
		statement+=' ORDER BY '+this.pendingorderobject[i];
		firstone=false;
		}
	    else
		{
		statement+=', '+this.pendingorderobject[i];
		}
	    }
    if (!readonly && is_select)
	statement += ' FOR UPDATE'
    this.ifcProbe(ifAction).Invoke("Query", {query:statement, client:formobj, appendrows:appendrows});
    }


function osrc_make_filter_colref(col)
    {
    return (col.obj?(':"' + col.obj + '"'):'') + ':"' + col.oid + '"';
    }


function osrc_make_filter_integer(col,val)
    {
    if (val == null && (typeof col.nullisvalue == 'undefined' || col.nullisvalue == true))
	return this.MFCol(col) + ' is null ';
    else if (val == null)
	return this.MFCol(col) + ' = null ';
    else if (!col.plainsearch && typeof val != 'number' && (new String(val)).search(/-/)>=0)
	{
	var parts = (new String(val)).split(/-/);
	return '(' + this.MFCol(col) + ' >=' + parts[0] + ' AND ' + this.MFCol(col) + ' <=' + parts[1] + ')';
	}
    else
	return this.MFCol(col) + ' = ' + val;
    }


function osrc_make_filter_string(col, val, icase)
    {
    var str = '';
    var ifunc = '';
    var colref = this.MFCol(col);
    if (icase)
	ifunc = 'upper';
    if (val == null && (typeof col.nullisvalue == 'undefined' || col.nullisvalue == true))
	str=colref + ' is null ';
    else if (val == null)
	str=colref + ' = null ';
    else if (col.plainsearch)
	str=ifunc + '(' + colref + ')='+ifunc+'("'+val+'")';
    else
	if (val.search(/^\*.+\*$/)>=0)
	    {
	    str='charindex(' + ifunc + '("'+val.substring(1,val.length-1)+'"),' + ifunc + '(' + colref + '))>0';
	    }
	else if(val.search(/^\*/)>=0) //* at beginning
	    {
	    val = val.substring(1); //pop off *
	    str='right(' + ifunc + '(' + colref + '),'+val.length+')=' + ifunc + '("'+val+'")';
	    }
	else if(val.search(/\*$/)>=0) //* at end
	    {
	    val=val.substring(0,val.length-1); //chop off *
	    str='substring(' + ifunc + '(' + colref + '),'+1+','+val.length+')=' + ifunc + '("'+val+'")';
	    }
	else if(val.indexOf('*')>=0) //* in middle
	    {
	    var ind = val.indexOf('*');
	    var val1 = val.substring(0,ind);
	    var val2 = val.substring(ind+1);
	    str='(right(' + ifunc + '(' + colref + '),'+val2.length+')=' + ifunc + '("'+val2+'")';
	    str+=' AND ';
	    str+='substring(' + ifunc + '(' + colref + '),1,'+val1.length+')=' + ifunc + '("'+val1+'"))';
	    }
	else
	    str=ifunc + '(' + colref + ')='+ifunc+'("'+val+'")';
    return str;
    }
    

function osrc_make_filter(q)
    {
    var firstone=true;
    var statement='';
    var isnot;
    for(var i in q)
	{
	isnot = false;
	if(i!='oid' && i!='joinstring')
	    {
	    var str;
	    if (q[i].force_empty)
		{
		//str = '1 == 0';
		str = " (" + this.MFCol(q[i]) + " = null and 1 == 0) ";
		}
	    else if(q[i].joinstring)
		{
		str=this.MakeFilter(q[i]);
		}
	    else
		{
		var val=q[i].value;
		//if (val == null) continue;

		if (typeof val == 'string')
		    val = new String(val);

		if (val && val.substring && val.substring(0,1) == '~')
		    {
		    val = val.substring(1);
		    isnot = true;
		    }
		else if (val && val.length && val[0] && val[0].substring && val[0].substring(0,1) == '~')
		    {
		    val[0] = val[0].substring(1);
		    isnot = true;
		    }

		if (typeof q[i].type == "undefined" && this.type_list[q[i].oid])
		    q[i].type = this.type_list[q[i].oid];

		var colref = this.MFCol(q[i]);

		switch(q[i].type)
		    {
		    case 'integer':
			str = this.MakeFilterInteger(q[i], val);
			break;

		    case 'integerarray':
			if (val == null && (typeof col.nullisvalue == 'undefined' || col.nullisvalue == true))
			    str = colref + 'is null ';
			else if (val == null)
			    str = colref + ' = null ';
			else if (val.length)
			    {
			    str = "(";
			    for(var j=0;j<val.length;j++)
				{
				if (j) str += " OR ";
				str += "(";
				str += this.MakeFilterInteger(q[i], val[j]);
				str += ")";
				}
			    str += ")";
			    }
			break;
		    case 'undefinedarray':
			if (val == null && (typeof col.nullisvalue == 'undefined' || col.nullisvalue == true))
			    {
			    str=colref + ' is null ';
			    }
			else if (val == null)
			    {
			    str=colref + ' = null ';
			    }
			else if (val.length == 0)
			    {
			    continue;
			    }
			else
			    {
			    str = "(";
			    for(var j=0;j<val.length;j++)
				{
				if (j) str += " OR ";
				str += "(";
				if ((new String(parseInt(val[j]))) == (new String(val[j])))
				    str += this.MakeFilterInteger(q[i], val[j]);
				else
				    str += this.MakeFilterString(q[i], val[j]);
				str += ")";
				}
			    str += ")";
			    }
			break;
		    case 'stringarray':
			if (val == null && (typeof col.nullisvalue == 'undefined' || col.nullisvalue == true))
			    {
			    str=colref + ' is null ';
			    }
			else if (val == null)
			    {
			    str=colref + ' = null ';
			    }
			else
			    {
			    str = "(";
			    for(var j=0;j<val.length;j++)
				{
				if (j) str += " OR ";
				str += "(";
				str += this.MakeFilterString(q[i], val[j]);
				str += ")";
				}
			    str += ")";
			    }
			break;
		    case 'datetimearray':
			str='(' + colref;
			var dtfirst=true;
			for(var j in val)
			    {
			    if(!dtfirst) str+= ' AND ' + colref;
			    dtfirst=false;
			    if(val[j].substring(0,2)=='>=')
				str+=' >= \"'+val[j].substring(2)+'\"';
			    else if(val[j].substring(0,2)=='<=')
				str+=' <= \"'+val[j].substring(2)+'\"';
			    else if(val[j].substring(0,2)=='=>')
				str+=' >= \"'+val[j].substring(2)+'\"';
			    else if(val[j].substring(0,2)=='=<')
				str+=' <= \"'+val[j].substring(2)+'\"';
			    else if(val[j].substring(0,1)=='>')
				str+=' > \"'+val[j].substring(1)+'\"';
			    else if(val[j].substring(0,1)=='<')
				str+=' < \"'+val[j].substring(1)+'\"';
			    else if(val[j].substring(0,1)=='=')
				str+=' = \"'+val[j].substring(1)+'\"';
			    }
			str+=')';
			break;

		    case 'string':
		    case 'istring':
			str = this.MakeFilterString(q[i], val, q[i].type == 'istring');
			break;

		    default:
			//htr_alert(val, 1);
			if(!val || typeof val.substring == 'undefined') // assume integer
			    str = this.MakeFilterInteger(q[i], val);
			else if(val.substring(0,2)=='>=')
			    str=colref + ' >= '+val.substring(2);
			else if(val.substring(0,2)=='<=')
			    str=colref + ' <= '+val.substring(2);
			else if(val.substring(0,2)=='=>')
			    str=colref + ' >= '+val.substring(2);
			else if(val.substring(0,2)=='=<')
			    str=colref + ' <= '+val.substring(2);
			else if(val.substring(0,1)=='>')
			    str=colref + ' > '+val.substring(1);
			else if(val.substring(0,1)=='<')
			    str=colref + ' < '+val.substring(1);
			else if(val.indexOf('-')>=0)
			    {
			    //assume integer range in string
			    var ind = val.indexOf('-');
			    var val1 = val.substring(0,ind);
			    var val2 = val.substring(ind+1);
			    str='(' + colref + ' >='+val1+' AND ' + colref + ' <='+val2+')';
			    }
			else
			    {
			    str = this.MakeFilterString(q[i], val);
			    }
			break;
		    }
		}
	    if (isnot)
		str = "not (" + str + ")";
	    if(firstone)
		{
		statement+=' ('+str+')';
		}
	    else
		{
		statement+=' '+q.joinstring+' ('+str+')';
		}
	    firstone=false;
	    }
	}
    return statement;
    }


// This function sees if any children are 'unsure' or have unsaved
// data. This function could be called right before closing a window
// where nogo_func would be a confirmation box (to lose unsaved data
// or not) could appear if needed.
function osrc_go_nogo(go_func, nogo_func, context)
    {
    if (this._go_nogo_pending)
	{
	this._go_nogo_queue.push([go_func, nogo_func, context]);
	return;
	}
    if (!this._go_nogo_queue)
	this._go_nogo_queue = [];
    this._unsaved_cnt = 0;
    this._go_nogo_pending = true;
    this._go_nogo_context = context;
    this._go_func = go_func;
    this._nogo_func = nogo_func;

    // First, take inventory of how many unsaved or unsure children
    // are out there.
    for(var i in this.child)
	{
	if ((typeof this.child[i].IsUnsaved) == 'undefined' || this.child[i].IsUnsaved == true)
	    {
	    this._unsaved_cnt++;
	    this.child[i]._osrc_ready = false;
	    }
	else
	    {
	    this.child[i]._osrc_ready = true;
	    }
	}

    // Now check with each, give it a chance to save its data
    for(var i in this.child)
	{
	if (this.child[i]._osrc_ready == false)
	    {
	    if (this.child[i].IsDiscardReady(this) == true)
		this._unsaved_cnt--;

	    // Somebody already did a QueryCancel?
	    if (!this._go_nogo_pending) break;
	    }
	}

    // If none were unsaved or all have given the "go", then go ahead
    // and perform the originally desired operation, otherwise wait on
    // callbacks with QueryContinue or QueryCancel.
    if (this._unsaved_cnt == 0 && this._go_nogo_pending)
	{
	this._go_nogo_pending = false;
	this._go_func(context);
	if (this._go_nogo_queue.length)
	    this.GoNogo.apply(this, this._go_nogo_queue.shift());
	}
    }


function osrc_action_query(aparam) //q, formobj)
    {
    this.init = true;
    if (this.query_delay_schedid)
	{
	pg_delsched(this.query_delay_schedid);
	this.query_delay_schedid = null;
	}
    this.QueueRequest({Request:'Query', Param:aparam});
    this.Dispatch();
    }

function osrc_query_handler(aparam)
    {
    var q = aparam.query;
    var formobj = aparam.client;

    if(this.pending)
	{
	alert('There is already a query or movement in progress...');
	return 0;
	}
    this.do_append = aparam.appendrows?true:false;
    this.lastquery=q;
    this.pendingquery=q;
    this.SetPending(true);

    // Check if any children are modified and call IsDiscardReady if they are
    this.GoNogo(osrc_cb_query_continue_2, osrc_cb_query_cancel_2, null);
    }

function osrc_action_delete(aparam) //up,formobj)
    {
    var up = aparam.data;
    var formobj = aparam.client;

    //Delete an object through OSML
    //var src = this.baseobj + '?cx__akey='+akey+'&ls__mode=osml&ls__req=delete&ls__sid=' + this.sid + '&ls__oid=' + up.oid;
    this.formobj = formobj;
    this.deleteddata=up;
    this.DoRequest('delete', this.baseobj, {ls__oid:up.oid}, osrc_action_delete_cb);
    //this.formobj.ObjectDeleted();
    //this.formobj.OperationComplete();
    return 0;
    }

function osrc_action_delete_cb()
    {
    var links = pg_links(this);
    if(links && links[0] && links[0].target != 'ERR')
	{
	var recnum=this.CurrentRecord;
	var cr=this.replica[recnum];
	if(cr)
	    {
	    // Remove the deleted row
	    delete this.replica[recnum];

	    // Adjust replica row id's to fill up the 'hole'
	    for(var i=recnum; i<this.LastRecord; i++)
		{
		this.replica[i] = this.replica[i+1];
		this.replica[i].id = i;
		}
	    delete this.replica[this.LastRecord];
	    if (this.FinalRecord == this.LastRecord)
		this.FinalRecord--;
	    this.LastRecord--;
	    if (this.OSMLRecord > 0) this.OSMLRecord--;

	    // Notify osrc clients (forms/tables/etc)
	    for(var i in this.child)
		this.child[i].ObjectDeleted(recnum, this);

	    // Need to fetch another record (delete was on last one in replica)?
	    if (this.CurrentRecord > this.LastRecord)
		{
		this.CurrentRecord--;
		this.MoveToRecord(this.CurrentRecord+1, true);
		}
	    else
		{
		this.MoveToRecord(this.CurrentRecord, true);
		}
	    }
	if (this.formobj) this.formobj.OperationComplete(true, this);
	}
    else
	{
	// delete failed
	if (this.formobj) this.formobj.OperationComplete(false, this);
	}
    this.formobj=null;
    delete this.deleteddata;
    return 0;
    }

function osrc_action_create(aparam)
    {
    var newobj = [];
    for(var p in aparam)
	newobj.push({oid:p, value:aparam[p]});
    this.ifcProbe(ifAction).Invoke("CreateObject", {client:null, data:newobj});
    }

function osrc_action_create_object(aparam) //up,formobj)
    {
    var up = aparam.data;
    var formobj = aparam.client;

    this.formobj=formobj;
    this.createddata=up;
    //First close the currently open query
    if(this.qid)
	{
	this.DoRequest('queryclose', '/', {ls__qid:this.qid}, osrc_action_create_cb2);
	this.qid=null;
	return 0;
	}
    else if (!this.sid)
	{
	this.replica = [];
	this.LastRecord=0;
	this.FinalRecord=null;
	this.FirstRecord=1;
	this.CurrentRecord=1;
	this.OSMLRecord=0;
	this.OpenSession(this.CreateCB2);
	return 0;
	}
    this.CreateCB2();
    return 0;
    }

function osrc_action_create_cb2()
    {
    //Create an object through OSML
    if(!this.sid) this.sid=pg_links(this)[0].target;
    //var src = this.baseobj + '/*?cx__akey='+akey+'&ls__mode=osml&ls__req=create&ls__reopen_sql=' + htutil_escape(this.sql) + '&ls__sid=' + this.sid;
    this.ApplyRelationships(this.createddata, true);
    //htr_alert(this.createddata, 2);
    /*for(var i in this.createddata) if(i!='oid')
	{
	if (this.createddata[i]['value'] == null)
	    src+='&'+htutil_escape(this.createddata[i]['oid'])+'=';
	else
	    src+='&'+htutil_escape(this.createddata[i]['oid'])+'='+htutil_escape(this.createddata[i]['value']);
	}*/
    var reqparam = {ls__reopen_sql:this.sql, ls__sqlparam:this.EncodeParams()};
    if (this.use_having) reqparam.ls__reopen_having = 1;
    for(var i in this.createddata) if(i!='oid')
	{
	if (this.createddata[i]['value'] == null)
	    reqparam[this.createddata[i]['oid']] = 'N:';
	else
	    reqparam[this.createddata[i]['oid']] = 'V:' + this.createddata[i]['value'];
	}
    this.DoRequest('create', this.baseobj + '/*', reqparam, osrc_action_create_cb);
    }

function osrc_action_create_cb()
    {
    var links = pg_links(this);
    if(links && links[0] && links[0].target != 'ERR')
	{
	if (this.FinalRecord == this.LastRecord)
	    this.FinalRecord++;
	this.LastRecord++;
	this.CurrentRecord = this.LastRecord;
	var recnum=this.CurrentRecord;
	var cr=this.replica[this.CurrentRecord];
	if(!cr) cr = [];

	for(var i in this.createddata) // update replica
	    {
	    /*for(var j in cr)
		if(cr[j].oid==this.createddata[i].oid)
		    cr[j].value=this.createddata[i].value;*/
	    cr[i] = [];
	    cr[i].oid = this.createddata[i].oid;
	    cr[i].value = this.createddata[i].value;
	    cr[i].id = i;
	    }
	cr.oid = links[0].target;
	this.replica[this.CurrentRecord] = cr;

	// Check new/corrected data provided by server
	var server_rec = this.ParseOneRow(links, 1);
	var max_j = 0;
	for(var i in server_rec)
	    {
	    found = 0;
	    for(var j in cr)
		{
		if (j == 'oid') continue;
		if (cr[j].oid == server_rec[i].oid)
		    {
		    cr[j].value = server_rec[i].value;
		    cr[j].type = server_rec[i].type;
		    cr[j].hints = server_rec[i].hints;
		    found = 1;
		    }
		if (parseInt(j) > max_j) max_j = parseInt(j);
		}
	    if (!found)
		{
		max_j++;
		cr[max_j] = {};
		cr[max_j].oid = server_rec[i].oid;
		cr[max_j].value = server_rec[i].value;
		cr[max_j].type = server_rec[i].type;
		cr[max_j].id = max_j;
		cr[max_j].hints = server_rec[i].hints;
		}
	    }

	//alert(this.replica[this.CurrentRecord].oid);
	this.in_create = false;
	this.SyncID = osrc_syncid++;
	if (this.formobj) this.formobj.OperationComplete(true, this);
	pg_serialized_load(this, 'about:blank', null, true);
	for(var i in this.child)
	    this.child[i].ObjectCreated(recnum, this);
	this.GiveAllCurrentRecord('create');
	this.ifcProbe(ifEvent).Activate("Created", {});
	}
    else
	{
	this.in_create = false;
	if (this.formobj) this.formobj.OperationComplete(false, this);
	}
    this.formobj=null;
    delete this.createddata;
    }

function osrc_action_refresh_object(aparam)
    {
    this.QueueRequest({Request:'RefreshObject', Param:aparam});
    this.Dispatch();
    }

function osrc_refresh_object_handler(aparam)
    {
    // Need a "last query" and a valid current record to proceed
    if (!this.lastquery || !this.CurrentRecord || !this.replica || !this.replica[this.CurrentRecord]) return false;

    // Build list of primary keys
    var row = this.replica[this.CurrentRecord];
    var keys = {};
    var keycnt = 0;
    for(var c in row)
	{
	if (c == 'oid') continue;
	var col = row[c];
	var ph = cx_parse_hints(col.hints);
	if (ph.Style & cx_hints_style.key)
	    {
	    keys[col.oid] = col;
	    keycnt ++;
	    }
	}
    if (!keycnt) return false;

    // Start with the lastquery SQL.
    var sql = this.lastquery;

    // Append logic to search for just this row
    var first = true;
    for(var k in keys)
	{
	if (first)
	    {
	    if (this.use_having)
		sql += " HAVING ";
	    else
		sql += " WHERE ";
	    }
	else 
	    sql += " AND ";
	sql += this.MakeFilter([keys[k]]);
	first = false;
	}
    sql += " LIMIT 1";

    // Now issue the query
    this.DoRequest("multiquery", "/", 
	    {
	    ls__sql:sql,
	    ls__rowcount:1,
	    ls__objmode:0,
	    ls__autofetch:1,
	    ls__autoclose:1,
	    ls__notify:this.request_updates,
	    ls__sqlparam:this.EncodeParams()
	    }, osrc_refresh_object_cb);
    return true;
    }

function osrc_refresh_object_cb()
    {
    var links = pg_links(this);
    var success = links && links[0] && (links[0].target != 'ERR');
    if(success && links.length > 1)
	{
	// Check new/corrected data provided by server
	var cr=this.replica[this.CurrentRecord];
	var server_rec = this.ParseOneRow(links, 1);
	var diff = 0;
	for(var i in server_rec)
	    for(var j in cr)
		{
		if (cr[j].oid == server_rec[i].oid && cr[j].value != server_rec[i].value)
		    {
		    cr[j].value = server_rec[i].value;
		    cr[j].type = server_rec[i].type;
		    diff = 1;
		    }
		}

	// if any changes, display them
	if (diff)
	    {
	    this.SyncID = osrc_syncid++;
	    this.GiveAllCurrentRecord('refresh');
	    }
	}
    }

function osrc_action_modify(aparam) //up,formobj)
    {
    this.doing_refresh = false;
    if (aparam)
	{
	this.modifieddata = aparam.data;
	this.formobj = aparam.client;
	}

    // initiated by a connector?  use current record and convert the data
    if (aparam && (!aparam.data || !aparam.data.oid))
	{
	this.formobj = null;
	this.modifieddata = [];
	if (this.CurrentRecord && this.replica[this.CurrentRecord])
	    this.modifieddata.oid = this.replica[this.CurrentRecord].oid;
	else
	    this.modifieddata.oid = null;

	for(var a in aparam)
	    {
	    if (a == '_EventName' || a == '_Origin') continue;
	    this.modifieddata.push({oid:a, value:aparam[a]});
	    }
	}

    // Need to close an open query first?
    if(this.qid)
	{
	this.DoRequest('queryclose', '/', {ls__qid:this.qid}, osrc_action_modify);
	this.qid=null;
	return 0;
	}
    //Modify an object through OSML
    //up[adsf][value];
    var reqparam = {ls__oid:this.modifieddata.oid, ls__reopen_sql:this.sql, ls__sqlparam:this.EncodeParams()};
    if (this.use_having) reqparam.ls__reopen_having = 1;

    //var src='/?cx__akey='+akey+'&ls__mode=osml&ls__req=setattrs&ls__sid=' + this.sid + '&ls__oid=' + this.modifieddata.oid;
    this.ApplyRelationships(this.modifieddata, false);
    for(var i in this.modifieddata) if(i!='oid')
	{
	if (this.modifieddata[i]['value'] == null)
	    reqparam[this.modifieddata[i]['oid']] = 'N:';
	else
	    reqparam[this.modifieddata[i]['oid']] = 'V:' + this.modifieddata[i]['value'];
	//src+='&'+htutil_escape(this.modifieddata[i]['oid'])+'='+htutil_escape(this.modifieddata[i]['value']);
	}
    if (this.send_updates)
	this.DoRequest('setattrs', '/', reqparam, osrc_action_modify_cb);
    else
	{
	this.ImportModifiedData(this.modifieddata);
	this.ChangeCurrentRecord();
	this.osrc_action_modify_cb_2(false);
	}
    }

function osrc_action_modify_cb()
    {
    var links = pg_links(this);
    var success = links && links[0] && (links[0].target != 'ERR');
    if(success)
	{
	this.ImportModifiedData(this.modifieddata);

	// Check new/corrected data provided by server
	var cr=this.replica[this.CurrentRecord];
	var server_rec = this.ParseOneRow(links, 1);
	var diff = 0;
	for(var i in server_rec)
	    {
	    var found = 0;
	    for(var j in cr)
		{
		if (cr[j].oid == server_rec[i].oid)
		    {
		    found = 1;
		    if (cr[j].value != server_rec[i].value)
			{
			var oldval = cr[j].value;
			cr[j].value = server_rec[i].value;
			cr[j].type = server_rec[i].type;
			this.ifcProbe(ifValue).Changing(cr[j].oid, cr[j].value, true, oldval, true);
			diff = 1;
			//alert(cr[j].value + " != " + server_rec[i].value);
			}
		    }
		}
	    if (!found)
		{
		// we haven't seen this property before, add it.
		cr.push( {value:server_rec[i].value, type:server_rec[i].type, oid:server_rec[i].oid} );
		this.ifcProbe(ifValue).Changing(server_rec[i].oid, server_rec[i].value, true, null, true);
		diff = 1;
		}
	    }
	this.osrc_action_modify_cb_2(diff);
	}
    else
	{
	if (this.formobj) this.formobj.OperationComplete(false, this);
	this.formobj=null;
	delete this.modifieddata;
	}
    }

function osrc_import_modified_data(data)
    {
    var cr=this.replica[this.CurrentRecord];
    if(cr)
	for(var i in this.modifieddata) // update replica
	    for(var j in cr)
		if(cr[j].oid==this.modifieddata[i].oid)
		    {
		    var oldval = cr[j].value;
		    cr[j].value=this.modifieddata[i].value;
		    this.ifcProbe(ifValue).Changing(cr[j].oid, cr[j].value, true, oldval, true);
		    }
    }

function osrc_action_modify_cb_2(diff)
    {
    this.SyncID = osrc_syncid++;
    pg_serialized_load(this, 'about:blank', null, true);
    if (this.formobj)
	this.formobj.OperationComplete(true, this);
    for(var i in this.child)
	this.child[i].ObjectModified(this.CurrentRecord, this.replica[this.CurrentRecord], this);
    this.ChangeCurrentRecord();
    if (diff)
	this.GiveAllCurrentRecord('modify');
    if (!this.formobj)
	this.ifcProbe(ifEvent).Activate('Modified', {});
    this.formobj=null;
    delete this.modifieddata;
    }

function osrc_cb_query_continue(o)
    {
    //if there is no pending query, don't save the status
    //  this is here to protect against form1 vetoing the move, then form2 reporting it is ready to go
    //if(!this.pending) return 0;
    //Current form ready
    //if(o)
    //	o._osrc_ready = true;
    if (o && o._osrc_ready == false)
	{
	o._osrc_ready = true;
	this._unsaved_cnt--;
	if (this._unsaved_cnt <= 0 && this._go_nogo_pending)
	    {
	    this._go_nogo_pending = false;
	    this._go_func(this._go_nogo_context);
	    if (this._go_nogo_queue.length)
		this.GoNogo.apply(this, this._go_nogo_queue.shift());
	    }
	}
    //If all other forms are ready then go
    //for(var i in this.child)
//	 {
//	 if(this.child[i]._osrc_ready == false)
//	      {
//	      return 0;
//	      }
//	 }
    }


function osrc_cb_query_continue_2()
    {
    //everyone looks ready, let's go
    this.init=true;
    if(this.pendingquery) // this could be a movement or a new query....
	{  // new query
	this.query=this.pendingquery;
	this.queryobject=this.pendingqueryobject;
	this.orderobject=this.pendingorderobject;
	this.pendingquery=null;
	this.pendingqueryobject=null;
	this.pendingorderobject=null;
	/*for(var i in this.child)
	     {
	     this.child[i]._osrc_ready=false;
	     }*/

	if (!this.do_append) this.ClearReplica();
	this.moveop=true;

	this.OpenSession(this.OpenQuery);
	}
    else
	{ // movement
	this.MoveToRecordCB(this.RecordToMoveTo, true);
	this.RecordToMoveTo=null;
	}
    //this seems premature - GRB
    //this.SetPending(false);
    /*this.pending=false;
    this.Dispatch();*/
    }

function osrc_cb_query_cancel()
    {
    if (this._go_nogo_pending)
	{
	this._go_nogo_pending = false;
	this._nogo_func(this._go_nogo_context);
	if (this._go_nogo_queue.length)
	    this.GoNogo.apply(this, this._go_nogo_queue.shift());
	}
    }

function osrc_cb_query_cancel_2()
    {
    this.pendingquery=null;
    this.SetPending(false);
    /*this.pending=false;
    this.Dispatch();*/
    }

function osrc_cb_request_object(aparam)
    {
    return 0;
    }

function osrc_cb_set_view_range(client, startrec, endrec)
    {
    client.__osrc_viewrange = [startrec, endrec];
    return;
    }

function osrc_cb_register(client)
    {
    this.child.push(client);
    client.__osrc_osrc = this;
    client.__osrc_viewrange = null;
    if (typeof client.is_savable != 'undefined')
	{
	if (client.is_savable)
	    this.savable_client_count++;
	client.__osrc_savable_changed = osrc_cb_savable_changed;
	htr_watch(client, 'is_savable', '__osrc_savable_changed');
	}
    else if (typeof client.is_client_savable != 'undefined')
	{
	if (client.is_client_savable)
	    this.savable_client_count++;
	client.__osrc_savable_changed = osrc_cb_savable_changed;
	htr_watch(client, 'is_client_savable', '__osrc_savable_changed');
	}
    if (this.savable_client_count > 0 && !this.is_client_savable)
	{
	this.is_client_savable = true;
	this.ifcProbe(ifValue).Changing("is_client_savable", 1, true, 0, true);
	}
    if (typeof client.is_discardable != 'undefined')
	{
	if (client.is_discardable)
	    this.discardable_client_count++;
	client.__osrc_discardable_changed = osrc_cb_discardable_changed;
	htr_watch(client, 'is_discardable', '__osrc_discardable_changed');
	}
    else if (typeof client.is_client_discardable != 'undefined')
	{
	if (client.is_client_discardable)
	    this.discardable_client_count++;
	client.__osrc_discardable_changed = osrc_cb_discardable_changed;
	htr_watch(client, 'is_client_discardable', '__osrc_discardable_changed');
	}
    if (this.discardable_client_count > 0 && !this.is_client_discardable)
	{
	this.is_client_discardable = true;
	this.ifcProbe(ifValue).Changing("is_client_discardable", 1, true, 0, true);
	}
    }

function osrc_cb_discardable_changed(p,o,n)
    {
    var osrc = this.__osrc_osrc;
    if (o && !n)
	osrc.discardable_client_count--;
    else if (!o && n)
	osrc.discardable_client_count++;
    if (osrc.is_client_discardable && osrc.discardable_client_count == 0)
	{
	osrc.is_client_discardable = false;
	osrc.ifcProbe(ifValue).Changing("is_client_discardable", 0, true, 1, true);
	}
    else if (!osrc.is_client_discardable && osrc.discardable_client_count > 0)
	{
	osrc.is_client_discardable = true;
	osrc.ifcProbe(ifValue).Changing("is_client_discardable", 1, true, 0, true);
	}
    return n;
    }

function osrc_cb_savable_changed(p,o,n)
    {
    var osrc = this.__osrc_osrc;
    if (o && !n)
	osrc.savable_client_count--;
    else if (!o && n)
	osrc.savable_client_count++;
    if (osrc.is_client_savable && osrc.savable_client_count == 0)
	{
	osrc.is_client_savable = false;
	osrc.ifcProbe(ifValue).Changing("is_client_savable", 0, true, 1, true);
	}
    else if (!osrc.is_client_savable && osrc.savable_client_count > 0)
	{
	osrc.is_client_savable = true;
	osrc.ifcProbe(ifValue).Changing("is_client_savable", 1, true, 0, true);
	}
    return n;
    }

function osrc_open_session(cb)
    {
    //Open Session
    //alert('open');
    if(this.sid || cb == osrc_open_query)
	{
	this.__osrc_cb = cb;
	this.__osrc_cb();
	}
    else
	{
	this.DoRequest('opensession', '/', {}, cb);
	}
    }

function osrc_open_query()
    {
    //Open Query
    /*if(!this.sid)
	{
	var lnks = pg_links(this);
	if (!lnks || !lnks[0] || !lnks[0].target)
	    return false;
	this.sid=pg_links(this)[0].target;
	}*/
    if(this.qid && this.sid)
	{
	this.DoRequest('queryclose', '/', {ls__qid:this.qid}, osrc_open_query);
	this.qid=null;
	return 0;
	}
    this.query_ended = false;
    var reqobj = {ls__autoclose_sr:'1', ls__autofetch:'1', ls__objmode:'0', ls__notify:this.request_updates, ls__rowcount:this.replicasize, ls__sql:this.query, ls__sqlparam:this.EncodeParams()};
    if (!this.sid)
	reqobj.ls__newsess = 'yes';
    this.DoRequest('multiquery', '/', reqobj, osrc_get_qid);
    this.querysize = this.replicasize;
    }

function osrc_get_qid()
    {
    //return;
    var lnk = pg_links(this);
    this.data_start = 1;
    if (!this.sid && lnk && lnk[0] && lnk[0].target)
	{
	this.sid = lnk[0].target;
	this.data_start = 2;
	}

    if (lnk && lnk[this.data_start-1])
	this.qid=lnk[this.data_start-1].target;
    else
	this.qid = null;

    //confirm(this.baseobj + " ==> " + this.qid);
    if (!this.qid)
	{
	/*this.pending=false;*/
	this.move_target = null;
	this.GiveAllCurrentRecord('get_qid');
	this.SetPending(false);
	/*this.Dispatch();*/
	}
    else
	{
	this.query_delay = pg_timestamp() - this.request_start_ts;
	for(var i in this.child)
	    this.child[i].DataAvailable(this, this.doing_refresh?'refresh':'query');
	if (this.move_target)
	    var tgt = this.move_target;
	else
	    var tgt = 1;
	this.move_target = null;
	if (lnk.length > 1)
	    {
	    // did an autofetch - we have the data already
	    if (!this.do_append) this.ClearReplica();
	    this.TargetRecord = [tgt,tgt];
	    this.CurrentRecord = tgt;
	    this.moveop = true;
	    this.FetchNext();
	    }
	else
	    {
	    // start the ball rolling for the fetch
	    //this.ifcProbe(ifAction).Invoke("First", {from_internal:true});
	    this.ifcProbe(ifAction).Invoke("FindObject", {ID:tgt, from_internal:true});
	    }
	}
    /** normally don't actually load the data...just let children know that the data is available **/
    }

function osrc_parse_one_attr(lnk)
    {
    var col = {type:lnk.hash.substr(1), oid:htutil_unpack(lnk.host.substr(1)), hints:lnk.search};
    this.type_list[col.oid] = col.type;
    switch(lnk.text.charAt(0))
	{
	case 'V': col.value = htutil_rtrim(unescape(lnk.text.substr(2))); break;
	case 'N': col.value = null; break;
	case 'E': col.value = '** ERROR **'; break;
	}
    return col;
    }


function osrc_new_replica_object(id, oid)
    {
    var obj = [];
    obj.oid=oid;
    obj.id = id;
    return obj;
    }

function osrc_prune_replica(most_recent_id)
    {
    if(this.LastRecord < most_recent_id)
	{
	this.LastRecord = most_recent_id;
	while(this.LastRecord-this.FirstRecord >= this.replicasize)
	    {
	    // don't go past current record
	    if (this.FirstRecord == this.CurrentRecord || this.FirstRecord == this.TargetRecord[0]) break;
	    var found = false;
	    for(var c in this.child)
		{
		if (this.child[c].__osrc_viewrange && this.FirstRecord == this.child[c].__osrc_viewrange[0])
		    {
		    found = true;
		    break;
		    }
		}
	    if (found) break;

	    // clean up replica
	    this.oldoids.push(this.replica[this.FirstRecord].oid);
	    delete this.replica[this.FirstRecord];
	    this.FirstRecord++;
	    }
	}
    if(this.FirstRecord > most_recent_id)
	{
	this.FirstRecord = most_recent_id;
	while(this.LastRecord-this.FirstRecord >= this.replicasize)
	    { 
	    // don't go past current record
	    if (this.LastRecord == this.CurrentRecord || this.LastRecord == this.TargetRecord[1]) break;
	    for(var c in this.child)
		{
		var found = false;
		if (this.child[c].__osrc_viewrange && this.LastRecord == this.child[c].__osrc_viewrange[1])
		    {
		    found = true;
		    break;
		    }
		}
	    if (found) break;

	    // clean up replica
	    if (this.replica[this.LastRecord])
		{
		this.oldoids.push(this.replica[this.LastRecord].oid);
		delete this.replica[this.LastRecord];
		}
	    this.LastRecord--;
	    }
	}
    }

function osrc_action_clear(aparam)
    {
    this.SyncID = osrc_syncid++;
    this.lastSync = [];
    this.ClearReplica();
    this.GiveAllCurrentRecord('clear');
    }

function osrc_clear_replica()
    {
    this.TargetRecord = [1,1];/* the record we're aiming for -- go until we get it*/
    this.CurrentRecord=1;/* the current record */
    this.OSMLRecord=0;/* the last record we got from the OSML */

    /** Clear replica **/
    if(this.replica)
	for(var i in this.replica)
	    this.oldoids.push(this.replica[i].oid);
    
    if(this.replica) delete this.replica;
    this.replica = [];
    this.LastRecord=0;
    this.FinalRecord=null;
    this.FirstRecord=1;
    }

function osrc_parse_one_row(lnk, i)
    {
    var row = [];
    var cnt = 0;
    var tgt = lnk[i].target;
    while(i < lnk.length && (lnk[i].target == tgt || lnk[i].target == 'R'))
	{
	row[cnt] = this.ParseOneAttr(lnk[i]);
	row[cnt].system = (cnt < 4);
	cnt++;
	i++;
	}
    return row;
    }

function osrc_do_fetch(rowcnt, at_end)
    {
    this.querysize = rowcnt?rowcnt:1;
    var reqparam = {ls__qid:this.qid, ls__objmode:'0', ls__notify:this.request_updates};
    if (rowcnt)
	reqparam.ls__rowcount = rowcnt;
    if (this.startat)
	reqparam.ls__startat = this.startat;
    if (at_end)
	reqparam.ls__tail = 1;
    if (this.query_delay_schedid)
	{
	pg_delsched(this.query_delay_schedid);
	this.query_delay_schedid = null;
	}
    this.DoRequest('queryfetch', '/', reqparam, osrc_fetch_next);
    }

function osrc_query_timeout()
    {
    var qid = this.qid;
    this.query_delay_schedid = null;
    if (qid)
	{
	this.qid = null;
	this.DoRequest('queryclose', '/', {ls__qid:qid}, osrc_close_query);
	}
    this.Dispatch();
    return 0;
    }

function osrc_end_query()
    {
    //this.formobj.OperationComplete(); /* don't need this...I think....*/
    var qid=this.qid
    this.qid=null;
    /* return the last record as the current one if it was our target otherwise, don't */
    if (this.LastRecord >= this.FirstRecord && this.replica[this.LastRecord])
	{
	this.replica[this.LastRecord].__osrc_is_last = true;
	this.FinalRecord = this.LastRecord;
	}
    this.query_ended = true;
    this.FoundRecord();
    if(qid)
	{
	this.DoRequest('queryclose', '/', {ls__qid:qid}, osrc_close_query);
	}
    this.Dispatch();
    this.ifcProbe(ifEvent).Activate("EndQuery", {FinalRecord:this.FinalRecord, LastRecord:this.LastRecord, FirstRecord:this.FirstRecord, CurrentRecord:this.CurrentRecord});
    this.doing_refresh = false;

    // Handle auto-refresh timer
    if (this.refresh_schedid)
	{
	pg_delsched(this.refresh_schedid);
	this.refresh_schedid = null;
	}
    if (this.refresh_interval)
	{
	this.refresh_schedid = pg_addsched_fn(this, 'RefreshTimer', [], this.refresh_interval);
	}
    return 0;
    }

function osrc_found_record()
    {
    if(this.CurrentRecord>this.LastRecord)
	this.CurrentRecord=this.LastRecord;
    if(this.doublesync)
	this.DoubleSyncCB();
    if(this.moveop)
	this.GiveAllCurrentRecord('change');
    else
	this.TellAllReplicaMoved();
    /*this.pending=false;*/
    this.SetPending(false);
    this.osrc_oldoid_cleanup();
    if (this.query_delay)
	{
	if (this.query_delay_schedid)
	    {
	    pg_delsched(this.query_delay_schedid);
	    this.query_delay_schedid = null;
	    }
	var d = this.query_delay * 2 + 1000;
	if (d < 3000) d = 3000;
	if (d > 30000) d = 30000;
	this.query_delay_schedid = pg_addsched_fn(this, 'QueryTimeout', [], d);
	}
    }

function osrc_fetch_next()
    {
    pg_debug(this.id + ": FetchNext() ==> " + pg_links(this).length + "\n");
    //alert('fetching....');
    if(!this.qid)
	{
	//if (pg_diag) confirm("ERR: " + this.baseobj + " ==> " + this.qid);
	if (pg_diag) confirm("fetch_next: error - no qid.  first/last/cur/osml: " + this.FirstRecord + "/" + this.LastRecord + "/" + this.CurrentRecord + "/" + this.OSMLRecord + "\n");
	//alert('something is wrong...');
	//alert(this.src);
	}
    var lnk=pg_links(this);
    var lc=lnk.length;
    //confirm(this.baseobj + " ==> " + lc + " links");
    if(lc <= this.data_start)
	{ // query over
	this.EndQuery();
	return 0;
	}
    var colnum=0;
    var i = this.data_start;
    var rowcnt = 0;
    while (i < lc)
	{
	if (lnk[i].target == 'QUERYCLOSED')
	    {
	    this.qid = null;
	    break;
	    }
	if (lnk[i].target == 'SKIPPED')
	    {
	    this.OSMLRecord += parseInt(lnk[i].text);
	    //this.FirstRecord = this.OSMLRecord + 1;
	    i++;
	    this.querysize++; // indicate that we hit end of result set
	    continue;
	    }
	this.OSMLRecord++; // this holds the last record we got, so now will hold current record number
	this.replica[this.OSMLRecord] =
		this.NewReplicaObj(this.OSMLRecord, lnk[i].target);
	this.PruneReplica(this.OSMLRecord);
	var row = this.ParseOneRow(lnk, i);
	rowcnt++;
	i += row.length;
	for(var j=0; j<row.length; j++)
	    {
	    this.replica[this.OSMLRecord][j] = row[j];
	    if (this.doing_refresh && this.refresh_objname && row[j].oid == 'name' && this.refresh_objname == row[j].value)
		this.CurrentRecord = this.OSMLRecord;
	    }
	}
    this.data_start = 1; // reset it
    pg_debug("   Fetch returned " + rowcnt + " rows, querysize was " + this.querysize + ".\n");

    // make sure we bring this.LastRecord back down to the top of our replica...
    while(!this.replica[this.LastRecord] && this.LastRecord > 0)
	this.LastRecord--;

    if(this.LastRecord<this.TargetRecord[1])
	{ 
	// didn't get a full fetch?  end query if so
	if (rowcnt < this.querysize)
	    {
	    this.EndQuery();
	    return 0;
	    }

	// Wow - how many records does the user want?
	if ((this.LastRecord % 500) < ((this.LastRecord - this.querysize) % 500))
	    {
	    if (!confirm("You have already retrieved " + this.LastRecord + " records.  Do you want to continue?"))
		{
		// pause here.
		this.FoundRecord();
		return 0;
		}
	    }

	// We're going farther down this...
	this.DoFetch(this.readahead, false);
	}
    else
	{
	// we've got the one we need 
	if((this.LastRecord-this.FirstRecord+1)<this.replicasize && rowcnt >= this.querysize)
	    {
	    // make sure we have a full replica if possible
	    this.DoFetch(this.replicasize - (this.LastRecord - this.FirstRecord + 1), false);
	    }
	else
	    {
	    if (rowcnt < this.querysize)
		this.EndQuery();
	    else
		this.FoundRecord();
	    }
	}
    }

function osrc_oldoid_cleanup()
    {
    if(this.oldoids && this.oldoids[0])
	{
	this.SetPending(true);
	/*this.pending=true;*/
	var src='';
	for(var i in this.oldoids)
	    src+=this.oldoids[i];
	if(this.sid)
	    this.DoRequest('close', '/', {ls__oid:src}, osrc_oldoid_cleanup_cb);
	else
	    alert('session is invalid');
	}
    else
	{
	pg_serialized_load(this, 'about:blank', null, true);
	this.Dispatch();
	}
    }
 
function osrc_oldoid_cleanup_cb()
    {
    /*this.pending=false;*/
    //alert('cb recieved');
    delete this.oldoids;
    this.oldoids = [];
    this.SetPending(false);
    pg_serialized_load(this, 'about:blank', null, true);
    /*this.Dispatch();*/
    }
 
function osrc_close_query()
    {
    //Close Query
    this.qid=null;
    this.osrc_oldoid_cleanup();
    //confirm("closing " + this.baseobj);
    //this.onload = osrc_close_session;
    //pg_set(this,'src','/?ls__mode=osml&ls__req=queryclose&ls__qid=' + this.qid);
    }
 
function osrc_close_object()
    {
    //Close Object
    this.DoRequest('close', '/', {ls__oid:this.oid}, osrc_close_session);
    }
 
function osrc_close_session()
    {
    //Close Session
    this.DoRequest('closesession', '/', {}, osrc_oldoid_cleanup);
    this.qid=null;
    this.sid=null;
    }


function osrc_action_find_object(aparam)
    {
    this.QueueRequest({Request:'FindObject', Param:aparam});
    this.Dispatch();
    }

function osrc_find_object_handler(aparam)
    {
    var from_internal = (aparam.from_internal)?true:false;
    if (typeof aparam.ID != 'undefined')
	{
	// Find by record #
	var id = parseInt(aparam.ID);
	if (!id) id = 1;
	this.MoveToRecord(id, from_internal);
	}
    else if (typeof aparam.Name != 'undefined')
	{
	// Find by object name
	for(var i in this.replica)
	    {
	    var rec = this.replica[i];
	    for(var j in rec)
		{
		var col = rec[j];
		if (col.oid == 'name')
		    {
		    if (col.value == aparam.Name)
			this.MoveToRecord(i, from_internal);
		    break;
		    }
		}
	    }
	}
    else
	{
	// find arbitrarily
	if (aparam._Origin) delete aparam._Origin;
	if (aparam._EventName) delete aparam._EventName;
	for(var i in this.replica)
	    {
	    var rec = this.replica[i];
	    var matched = true;
	    for(var j in rec)
		{
		var col = rec[j];
		matched = true;
		for(var k in aparam) 
		    {
		    if (col.oid == k && col.value != aparam[k])
			matched = false;
		    }
		if (!matched) break;
		}
	    if (matched)
		{
		this.MoveToRecord(i, from_internal);
		break;
		}
	    }
	}
    }


function osrc_move_first(aparam)
    {
    this.MoveToRecord(1, aparam.from_internal);
    }


function osrc_change_current_record()
    {
    var newprevcurrent = [];

    // first, build the list of fields we're working with.  We look both in the
    // replica and in prevcurrent, since field lists can be irregular (different
    // for different records), as well as can contain NULL values.
    var fieldlist = {};
    if (this.prevcurrent)
	{
	for(var i=0; i<this.prevcurrent.length; i++)
	    fieldlist[this.prevcurrent[i].oid] = true;
	}
    if (this.replica && this.replica[this.CurrentRecord])
	{
	for(var i=0; i<this.replica[this.CurrentRecord].length; i++)
	    fieldlist[this.replica[this.CurrentRecord][i].oid] = true;
	}

    // Determine old and new values for the fields.
    for(var attrname in fieldlist)
	{
	// Old value -- see this.prevcurrent
	var oldval = null;
	if (this.prevcurrent)
	    {
	    for(var j in this.prevcurrent)
		{
		if (typeof this.prevcurrent[j] != 'object') continue;
		if (this.prevcurrent[j].oid == attrname)
		    {
		    oldval = this.prevcurrent[j].value;
		    break;
		    }
		}
	    }

	// New value -- see the replica.
	var newval = null;
	if (this.replica[this.CurrentRecord])
	    {
	    for(var j in this.replica[this.CurrentRecord])
		{
		if (typeof this.replica[this.CurrentRecord][j] != 'object') continue;
		if (this.replica[this.CurrentRecord][j].oid == attrname)
		    {
		    newval = this.replica[this.CurrentRecord][j].value;
		    break;
		    }
		}
	    }

	// Issue a Changing ifValue operation if the old and new are different.
	if (oldval != newval)
	    {
	    //	pg_explog.push('Changing: ' + oldval + ' to ' + newval);
	    this.ifcProbe(ifValue).Changing(attrname, newval, true, oldval, true);
	    }

	// Only record the value in prevcurrent if the *new* value is not null.
	if (newval)
	    newprevcurrent.push({oid:attrname, value:newval});
	}

    // Catch values that had no previous value and so were not in prevcurrent
    /*for(var j in this.replica[this.CurrentRecord])
	{
	if (typeof this.replica[this.CurrentRecord][j] != 'object') continue;
	var newval = this.replica[this.CurrentRecord][j].value;
	var oldval = null;
	var attrname = this.replica[this.CurrentRecord][j].oid;
	for(var i in this.prevcurrent)
	    {
	    if (typeof this.prevcurrent[i] != 'object') continue;
	    if (this.prevcurrent[i].oid == attrname)
		{
		oldval = this.prevcurrent[i].value;
		break;
		}
	    }
	if (oldval != newval)
	    this.ifcProbe(ifValue).Changing(attrname, newval, true, oldval, true);
	if (newval)
	    newprevcurrent.push({oid:attrname, value:newval});
	}*/
    this.prevcurrent = newprevcurrent;
    }


function osrc_give_all_current_record(why)
    {
    //confirm('give_all_current_record start');
    /*for(var j in this.replica[this.CurrentRecord])
	{
	var col = this.replica[this.CurrentRecord][j];
	if (typeof col == 'object')
	    this.ifcProbe(ifValue).Changing(col.oid, col.value, true);
	}*/
    this.ChangeCurrentRecord();
    if (this.LastRecord >= this.FirstRecord && this.replica[this.LastRecord] && this.replica[this.LastRecord].__osrc_is_last)
	{
	this.replica[this.CurrentRecord].__osrc_last_record = this.LastRecord;
	this.FinalRecord = this.LastRecord;
	}
    for(var i in this.child)
	this.child[i].ObjectAvailable(this.replica[this.CurrentRecord], this, (why=='create')?'create':(this.doing_refresh?'refresh':'change'));
    this.ifcProbe(ifEvent).Activate("DataFocusChanged", {});
    this.doing_refresh = false;
    //confirm('give_all_current_record done');
    }

function osrc_tell_all_replica_moved()
    {
    //confirm('tell_all_replica_moved start');
    for(var i in this.child)
	if(this.child[i].ReplicaMoved)
	    this.child[i].ReplicaMoved(this);
    //confirm('tell_all_replica_moved done');
    }


function osrc_move_to_record(recnum, from_internal)
    {
    if (typeof recnum != 'number') recnum = parseInt(recnum);
    this.QueueRequest({Request:'MoveTo', Param:{recnum:recnum, from_internal:from_internal}});
    this.Dispatch();
    }

function osrc_move_to_record_handler(param)
    {
    var recnum = param.recnum;
    var from_internal = param.from_internal;
    if(recnum<1)
	{
	//alert("Can't move past beginning.");
	return 0;
	}
    if(this.pending)
	{
	//alert('you got ahead');
	return 0;
	}
    this.SetPending(true);
    //this.pending=true;
    //var someunsaved=false;
    this.RecordToMoveTo=recnum;
    if (!from_internal)
	this.SyncID = osrc_syncid++;
    this.GoNogo(osrc_cb_query_continue_2, osrc_cb_query_cancel_2, null);
    /*for(var i in this.child)
	 {
	 if(this.child[i].IsUnsaved)
	     {
	     //alert('child: '+i+' : '+this.child[i].IsUnsaved+' isn\\'t saved...IsDiscardReady');
	     this.child[i]._osrc_ready=false;
	     this.child[i].IsDiscardReady();
	     someunsaved=true;
	     }
	 else
	     {
	     this.child[i]._osrc_ready=true;
	     }
	 }*/
    //if someunsaved is false, there were no unsaved forms, so no callbacks
    //  we can just continue
    /*if(someunsaved) return 0;
    this.MoveToRecordCB(recnum);*/
    }

function osrc_move_to_record_cb(recnum)
    {
    pg_debug(this.id + ": MoveTo(" + recnum + ")\n");
    //confirm(recnum);
    this.moveop=true;
    if(recnum<1)
	{
	//alert("Can't move past beginning.");
	return 0;
	}
    this.RecordToMoveTo=recnum;
    for(var i in this.child)
	 {
	 if(this.child[i].IsUnsaved)
	     {
	     //confirm('child: '+i+' : '+this.child[i].IsUnsaved+' isn\\'t saved...');
	     return 0;
	     }
	 }
/* If we're here, we're ready to go */
    this.TargetRecord = [recnum, recnum];
    this.CurrentRecord = recnum;
    if(this.CurrentRecord <= this.LastRecord && this.CurrentRecord >= this.FirstRecord)
	{
	this.GiveAllCurrentRecord('change');
	this.SetPending(false);
	/*this.pending=false;
	this.Dispatch();*/
	return 1;
	}
    else
	{
	if(this.CurrentRecord < this.FirstRecord)
	    { /* data is further back, need new query */
	    if(this.FirstRecord-this.CurrentRecord<this.readahead)
		{
		this.startat=(this.FirstRecord-this.readahead)>0?(this.FirstRecord-this.readahead):1;
		}
	    else
		{
		this.startat=this.CurrentRecord;
		}
	    if(this.qid)
		{
		this.DoRequest('queryclose', '/', {ls__qid:this.qid}, osrc_open_query_startat);
		}
	    else
		{
		this.osrc_open_query_startat();
		}
	    return 0;
	    }
	else
	    { /* data is farther on, act normal */
	    if(this.qid)
		{
		if(this.CurrentRecord == Number.MAX_VALUE)
		    {
		    /* rowcount defaults to a really high number if not set */
		    this.DoFetch(this.replicasize, true);
		    }
		else if (recnum == 1)
		    {
		    // fill replica if empty
		    this.DoFetch(this.replicasize, false);
		    }
		else
		    {
		    this.DoFetch(this.readahead, false);
		    }
		}
	    else if (!this.query_ended)
		{
		if (this.CurrentRecord == Number.MAX_VALUE)
		    {
		    this.osrc_open_query_tail();
		    }
		else
		    {
		    this.startat = this.LastRecord + 1;
		    this.osrc_open_query_startat();
		    }
		}
	    else
		{
		//this.pending=false;
		this.CurrentRecord=this.LastRecord;
		this.GiveAllCurrentRecord('change');
		this.SetPending(false);
		//this.Dispatch();
		}
	    return 0;
	    }
	}
    }

function osrc_open_query_tail()
    {
    /*if(this.FirstRecord > this.startat && this.FirstRecord - this.startat < this.replicasize)
	this.querysize = this.FirstRecord - this.startat;
    else*/
	this.querysize = this.replicasize;
    this.query_ended = false;
    this.DoRequest('multiquery', '/', {ls__tail:1, ls__autoclose_sr:1, ls__autofetch:1, ls__objmode:0, ls__notify:this.request_updates, ls__rowcount:this.querysize, ls__sql:this.query, ls__sqlparam:this.EncodeParams()}, osrc_get_qid_startat);
    }

function osrc_open_query_startat()
    {
    if(this.FirstRecord > this.startat && this.FirstRecord - this.startat < this.replicasize)
	this.querysize = this.FirstRecord - this.startat;
    else
	this.querysize = this.replicasize;
    this.query_ended = false;
    this.DoRequest('multiquery', '/', {ls__startat:this.startat, ls__autoclose_sr:1, ls__autofetch:1, ls__objmode:0, ls__notify:this.request_updates, ls__rowcount:this.querysize, ls__sql:this.query, ls__sqlparam:this.EncodeParams()}, osrc_get_qid_startat);
    }

function osrc_get_qid_startat()
    {
    var lnk = pg_links(this);
    this.qid=lnk[0].target;
    if (!this.qid)
	{
	this.startat = null;
	//this.pending=false;
	this.GiveAllCurrentRecord('get_qid');
	this.SetPending(false);
	//this.Dispatch();
	return;
	}
    this.OSMLRecord=(this.startat)?(this.startat-1):0;
    //this.FirstRecord=this.startat;
    /*if(this.startat-this.TargetRecord+1<this.replicasize)
	{
	this.DoFetch(this.TargetRecord - this.startat + 1);
	}*/
    if (lnk.length > 1)
	{
	// did an autofetch - we have the data already
	this.query_delay = pg_timestamp() - this.request_start_ts;
	this.FetchNext();
	}
    else
	{
	if(this.FirstRecord - this.startat < this.replicasize)
	    {
	    this.DoFetch(this.FirstRecord - this.startat, false);
	    }
	else
	    {
	    this.DoFetch(this.replicasize, false);
	    }
	}
    this.startat=null;
    }


function osrc_move_next(aparam)
    {
    this.MoveToRecord(this.CurrentRecord+1, false);
    }

function osrc_move_prev(aparam)
    {
    this.MoveToRecord(this.CurrentRecord-1, false);
    }

function osrc_move_last(aparam)
    {
    this.MoveToRecord(Number.MAX_VALUE, false); /* FIXME */
    //alert("do YOU know where the end is? I sure don't.");
    }


function osrc_scroll_prev()
    {
    if(this.FirstRecord!=1) this.ScrollTo(this.FirstRecord-1);
    }

function osrc_scroll_next()
    {
    this.ScrollTo(this.LastRecord+1);
    }

function osrc_scroll_prev_page()
    {
    this.ScrollTo(this.FirstRecord>this.replicasize?this.FirstRecord-this.replicasize:1);
    }

function osrc_scroll_next_page()
    {
    this.ScrollTo(this.LastRecord+this.replicasize);
    }

function osrc_scroll_to(startrec, endrec)
    {
    pg_debug(this.id + ": ScrollTo(" + startrec + "," + endrec + ")\n");
    pg_debug("   first/last/cur/osml: " + this.FirstRecord + "/" + this.LastRecord + "/" + this.CurrentRecord + "/" + this.OSMLRecord + "\n");
    this.moveop=false;
    this.TargetRecord = [startrec, endrec];
    this.SyncID = osrc_syncid++;
    if(this.TargetRecord[1] <= this.LastRecord && this.TargetRecord[0] >= this.FirstRecord)
	{
	// check for a 'hole' in the replica
	var hole = false;
	for(var i=startrec; i<=endrec; i++)
	    {
	    if (!this.replica[i])
		{
		hole = i;
		break;
		}
	    }
	if (hole)
	    {
	    // need to fill in a hole
	    if (this.qid && hole == this.OSMLRecord+1)
		{
		this.DoFetch(this.scrollahead, false);
		}
	    else
		{
		this.startat = hole;
		this.osrc_open_query_startat();
		}
	    }
	else
	    {
	    // data is contiguous here -- return successfully
	    this.TellAllReplicaMoved();
	    this.SetPending(false);
	    }
	return 1;
	}
    else
	{
	if(this.TargetRecord[0] < this.FirstRecord)
	    {
	    /* data is further back, need new query */
	    if(this.FirstRecord-this.TargetRecord[0]<this.scrollahead)
		{
		this.startat=(this.FirstRecord-this.scrollahead)>0?(this.FirstRecord-this.scrollahead):1;
		}
	    else
		{
		this.startat=this.TargetRecord[0];
		}
	    if(this.qid)
		{
		this.DoRequest('queryclose', '/', {ls__qid:this.qid}, osrc_open_query_startat);
		}
	    else
		{
		this.osrc_open_query_startat();
		}
	    return 0;
	    }
	else
	    {
	    /* data is farther on, act normal */
	    if(this.qid)
		{
		if(this.TargetRecord[1] == Number.MAX_VALUE)
		    {
		    /* rowcount defaults to a really high number if not set */
		    this.DoFetch(100, false);
		    }
		else
		    {
		    /* need to increase replica size to accomodate? */
		    this.DoFetch(this.scrollahead, false);
		    }
		}
	    else if (!this.query_ended)
		{
		this.startat = this.LastRecord + 1;
		this.osrc_open_query_startat();
		}
	    else
		{
		//this.pending=false;
		this.TargetRecord[1]=this.LastRecord;
		this.TellAllReplicaMoved();
		this.SetPending(false);
		//this.Dispatch();
		}
	    return 0;
	    }
	}
    }


function osrc_cleanup()
    {
/** this last-second page load is screwing something up... **/
/**   sometimes it will cause a blank page to be loaded, others it's a 'bus error' crash **/
    return 0;
    if(this.qid)
	{ /* why does the browser load a blank page when you try to move away? */
	this.onLoad=null;
	pg_set(this,'src',"/?cx__akey="+akey+"&ls__mode=osml&ls__req=queryclose&ls__sid="+this.sid+"&ls__qid="+this.qid);
	this.qid=null;
	this.doing_refresh = false;
	}
    }

function osrc_action_sync(param)
    {
    this.parentosrc=param.ParentOSRC;
    if (!this.parentosrc) 
	this.parentosrc = wgtrGetNode(this, param._Origin);
    this.ParentKey = [];
    this.ChildKey = [];

    // Does a rule apply?
    var rule = null;
    var on_norecs = 'allrecs';
    var on_null = 'allrecs';
    for(var onerel in this.relationships)
	{
	if (this.parentosrc && this.relationships[onerel].master == this.parentosrc)
	    {
	    rule = this.relationships[onerel];
	    on_null = rule.master_null_action;
	    on_norecs = rule.master_norecs_action;
	    if (on_norecs == 'sameasnull') on_norecs = on_null;
	    break;
	    }
	}

    // Prevent sync loops
    if (this.SyncID == this.parentosrc.SyncID)
	{
	return;
	}
    this.SyncID = this.parentosrc.SyncID;

    // Compile the list of criteria
    var query = [];
    query.oid=null;
    query.joinstring='AND';
    var p=this.parentosrc.CurrentRecord;
    var force_empty = false;
    for(var i=1;i<10;i++)
	{
	//this.ParentKey[i]=eval('param.ParentKey'+i);
	//this.ChildKey[i]=eval('param.ChildKey'+i);
	this.ParentKey[i]=param['ParentKey'+i];
	this.ChildKey[i]=param['ChildKey'+i];
	if(this.ParentKey[i])
	    {
	    if (!this.parentosrc.replica[p])
		{
		var t = new Object();
		t.plainsearch = true;
		t.oid = this.ChildKey[i];
		t.value = null;
		t.type = 'integer'; // type doesn't matter if it is null.
		if (on_norecs == 'nullisvalue')
		    t.nullisvalue = true;
		else if (on_norecs == 'norecs')
		    force_empty = t.force_empty = true;
		else
		    t.nullisvalue = false;
		query.push(t);
		}
	    else
		{
		for(var j in this.parentosrc.replica[p])
		    {
		    if(this.parentosrc.replica[p][j].oid==this.ParentKey[i])
			{
			var t = new Object();
			t.plainsearch = true;
			t.oid=this.ChildKey[i];
			t.value=this.parentosrc.replica[p][j].value;
			t.type=this.parentosrc.replica[p][j].type;
			if (t.value === null)
			    {
			    if (on_null == 'nullisvalue')
				t.nullisvalue = true;
			    else if (on_null == 'norecs')
				force_empty = t.force_empty = true;
			    else
				t.nullisvalue = false;
			    }
			query.push(t);
			}
		    }
		}
	    }
	}

    // Forcing empty (no records in master, or NULL key linkage)?
    if (force_empty && !this.was_forced_empty)
	{
	for(var c in this.child)
	    {
	    if (wgtrGetType(this.child[c]) == 'widget/form')
		{
		this.child[c].ifcProbe(ifAction).Invoke('Disable');
		}
	    }
	}
    else if (!force_empty && this.was_forced_empty)
	{
	for(var c in this.child)
	    {
	    if (wgtrGetType(this.child[c]) == 'widget/form')
		{
		this.child[c].ifcProbe(ifAction).Invoke('Enable');
		}
	    }
	}
    this.was_forced_empty = force_empty;

    // Did it change from last time?
    if (!this.lastSync)
	this.lastSync = [];
    var changed = false;
    for(var i=0;i<query.length;i++)
	{
	if (!this.lastSync[i])
	    {
	    changed = true;
	    }
	else if (this.lastSync[i].oid != query[i].oid || this.lastSync[i].value != query[i].value)
	    {
	    changed = true;
	    }
	this.lastSync[i] = {oid:query[i].oid, type:query[i].type, value:query[i].value};
	}
    for (var i=query.length;i<this.lastSync.length;i++)
	{
	this.lastSync[i] = {};
	}

    // Do the query
    if (changed)
	this.ifcProbe(ifAction).Invoke("QueryObject", {query:query, client:null,ro:this.readonly, fromsync:true});
    }

function osrc_action_double_sync(param)
    {
    this.doublesync=true;
    this.replicasize=100;
    this.readahead=100;
    this.parentosrc=param.ParentOSRC;
    this.childosrc=param.ChildOSRC;
    this.ParentKey = [];
    this.ParentSelfKey = [];
    this.SelfChildKey = [];
    this.ChildKey = [];
    var query = [];
    query.oid=null;
    query.joinstring='AND';
    var p=this.parentosrc.CurrentRecord;
    for(var i=1;i<10;i++)
	{
	this.ParentKey[i]=eval('param.ParentKey'+i);
	this.ParentSelfKey[i]=eval('param.ParentSelfKey'+i);
	this.SelfChildKey[i]=eval('param.SelfChildKey'+i);
	this.ChildKey[i]=eval('param.ChildKey'+i);
	if(this.ParentKey[i])
	    {
	    for(var j in this.parentosrc.replica[p])
		{
		if(this.parentosrc.replica[p][j].oid==this.ParentKey[i])
		    {
		    var t = new Object();
		    t.oid=this.ParentSelfKey[i];
		    t.value=this.parentosrc.replica[p][j].value;
		    t.type=this.parentosrc.replica[p][j].type;
		    query.push(t);
		    }
		}
	    }
	}
    this.ifcProbe(ifAction).Invoke("QueryObject", {query:query, client:null, ro:this.readonly, fromsync:true});
    }

function osrc_action_double_sync_cb()
    {
    var query = [];
    query.oid=null;
    query.joinstring='OR';
    if(this.LastRecord==0)
	{
	var t = new Object();
	t.oid=1;
	t.value=2;
	t.type='integer';
	query.push(t);
	}
    for(var p=this.FirstRecord; p<=this.LastRecord; p++)
	{
	var q = [];
	q.oid=null;
	q.joinstring='AND';
	for(var i=1;i<10;i++)
	    {
	    if(this.SelfChildKey[i])
		{
		for(var j in this.replica[p])
		    {
		    if(this.replica[p][j].oid==this.SelfChildKey[i])
			{
			var t = new Object();
			t.oid=this.ChildKey[i];
			t.value=this.replica[p][j].value;
			t.type=this.replica[p][j].type;
			q.push(t);
			}
		    }
		}
	    }
	query.push(q);
	}
    
    this.doublesync=false;
    this.childosrc.ifcProbe(ifAction).Invoke("QueryObject", {query:query, client:null, ro:this.readonly});
    }


// for each value in the replica, run a SQL statement
function osrc_action_do_sql(aparam)
    {
    if (!this.do_sql_loader)
	{
	this.do_sql_loader = htr_new_loader(this);
	this.do_sql_loader.osrc = this;
	}

    if (!aparam.SQL) return;
    this.do_sql_query = aparam.SQL;

    var p = aparam;
    if (p._Origin) delete p._Origin;
    if (p._EventName) delete p._EventName;
    delete p.SQL;

    this.do_sql_field = aparam.GroupingField;
    this.do_sql_used_values = {};
    this.do_sql_params = p;

    this.osrc_action_do_sql_cb();
    }


// Called when do_sql command is complete.
function osrc_action_do_sql_cb()
    {
    var osrc = (this.osrc)?(this.osrc):this;

    var grp_value = null;
    var common_values = {};
    var found_first = false;
    var attrval;
    for(var i in osrc.replica)
	{
	var obj = osrc.replica[i];
	if (typeof osrc.do_sql_field != 'undefined')
	    {
	    for(var j in obj)
		{
		var attr = obj[j];
		if (attr.oid == osrc.do_sql_field)
		    {
		    attrval = attr.value;
		    if (typeof osrc.do_sql_used_values[attr.value] != 'undefined')
			{
			grp_value = attr.value;
			osrc.do_sql_used_values[grp_value] = true;
			}
		    break;
		    }
		}
	    }
	if ((grp_value !== null && grp_value == attrval) || typeof osrc.do_sql_field == 'undefined')
	    {
	    for(var j in obj)
		{
		var attr = obj[j];
		if (!found_first)
		    common_values[attr.oid] = attr.value;
		else if (common_values[attr.oid] != attr.value)
		    delete common_values[attr.oid];
		}
	    found_first = true;
	    }
	}

    // no more data to do?
    if (!found_first) return;

    // copy in aparam values -- overrides common_values
    for (var p in osrc.do_sql_params)
	{
	common_values[p] = osrc.do_sql_params[p];
	}

    // run the request and wait for the callback.
    osrc.DoRequest('multiquery', '/', {ls__autoclose:'1', ls__autofetch:'1', ls__objmode:'0', ls__notify:0, ls__sql:osrc.do_sql_query, ls__sqlparam:osrc.Encode(common_values)}, osrc_action_do_sql_cb, osrc.do_sql_loader);
    }


/** called by child to get a template to build a new object for creation **/
function osrc_cb_new_object_template()
    {
    var obj = this.NewReplicaObj(0, 0);

    // Apply relationships and keys
    this.ApplyRelationships(obj, false);
    this.ApplyKeys(obj);

    // If 'force empty' because no master rec present, return null.
    for(var prop in obj)
	{
	if (obj[prop].force_empty)
	    return null;
	}

    return obj;
    }


function osrc_action_begincreate(aparam)
    {
    // Get the template
    var obj = this.NewObjectTemplate();
    if (!obj)
	return null;

    this.in_create = true;

    // Notify all children that we have a child that is creating an object
    for(var i in this.child)
	if (this.child[i] != aparam.client)
	    this.child[i].ObjectAvailable([], this, 'begincreate');

    return obj;
    }


function osrc_action_cancelcreate(aparam)
    {
    // We ignore this if we're not actually still in a create operation.
    if (this.in_create)
	{
	this.in_create = false;

	for(var i in this.child)
	    if (this.child[i] != aparam.client)
		this.child[i].ObjectAvailable(this.replica[this.CurrentRecord], this, 'cancelcreate');
	}
    }


function osrc_apply_keys(obj)
    {
    var cnt = 0;
    while(typeof obj[cnt] != 'undefined') cnt++;

    for(var i in this.rulelist)
	{
	var rl = this.rulelist[i];
	if (rl.ruletype == 'osrc_key')
	    {
	    switch(rl.keying_method)
		{
		case 'counterosrc':
		    if (rl.osrc.CurrentRecord && rl.osrc.replica && rl.osrc.replica[rl.osrc.CurrentRecord])
			{
			var pobj = rl.osrc.replica[rl.osrc.CurrentRecord];
			for(var j in pobj)
			    {
			    var col = pobj[j];
			    if (typeof col == 'object' && col.oid == rl.counter_attribute)
				{
				var found = false;
				for(var l in obj)
				    {
				    if (obj[l] == null || typeof obj[l] != 'object') continue;
				    if (obj[l].oid == rl.key_fieldname)
					{
					found = true;
					obj[l] = {type:col.type, value:col.value, hints:col.hints, oid:rl.key_fieldname};
					}
				    }
				if (!found)
				    obj[cnt++] = {type:col.type, value:col.value, hints:col.hints, oid:rl.key_fieldname};

				var nobj = [];
				nobj.oid = pobj.oid;
				nobj.push({value:parseInt(pobj[j].value?pobj[j].value:0) + 1, type:pobj[j].type, oid:pobj[j].oid});

				rl.osrc.ifcProbe(ifAction).Invoke("Modify", {data:nobj, client:this});
				break;
				}
			    }
			}
		    break;

		default:
		    break;
		}
	    }
	}

    return;
    }

function osrc_apply_rel(obj, in_create)
    {
    var cnt = 0;
    while(typeof obj[cnt] != 'undefined') cnt++;

    // First, check for relationships that might imply key values
    for(var i=0; i<this.relationships.length; i++)
	{
	var rule = this.relationships[i];
	if (rule.master)
	    {
	    var tgt = rule.master;
	    for(var k=0;k<rule.key.length;k++)
		{
		// Which property of obj do we need to modify?
		var found = false;
		var obj_index = null;
		for(var l in obj)
		    {
		    if (obj[l] == null || typeof obj[l] != 'object') continue;
		    if (obj[l].oid == rule.key[k])
			{
			found = true;
			if (!in_create || rule.enforce_create)
			    obj_index = l;
			}
		    }
		if (!found)
		    obj_index = cnt++;

		if (obj_index != null)
		    {
		    // Find the matching column in the master osrc replica
		    if (tgt.CurrentRecord && tgt.replica && tgt.replica[tgt.CurrentRecord])
			{
			for(var j in tgt.replica[tgt.CurrentRecord])
			    {
			    var col = tgt.replica[tgt.CurrentRecord][j];
			    if (col == null || typeof col != 'object') continue;
			    if (col.oid == rule.tkey[k])
				{
				obj[obj_index] = {type:col.type, value:col.value, hints:col.hints, oid:rule.key[k], obj:rule.obj};
				}
			    }
			}
		    else
			{
			// Master has no records.  Follow master_norecs_action.
			switch (rule.master_norecs_action)
			    {
			    case 'norecs':
				obj[obj_index] = {oid:rule.key[k], obj:rule.obj, force_empty:true};
				break;
			    case 'allrecs':
				break;
			    case 'sameasnull':
				obj[obj_index] = {oid:rule.key[k], obj:rule.obj, value:null};
				break;
			    }
			}

		    // Force plain search - no wildcards, etc.
		    obj[obj_index].plainsearch=true;

		    // Type not available?
		    if (obj[obj_index] && typeof obj[obj_index].type == "undefined" && this.type_list[obj[obj_index].oid])
			obj[obj_index].type = this.type_list[obj[obj_index].oid];

		    // Null value?
		    if (obj[obj_index] && obj[obj_index].value === null)
			{
			switch(rule.master_null_action)
			    {
			    case 'nullisvalue':
				obj[obj_index].nullisvalue = true;
				break;
			    case 'allrecs':
				obj[obj_index] = {};
				break;
			    case 'norecs':
				obj[obj_index].nullisvalue = false;
				obj[obj_index].force_empty = true;
				break;
			    }
			}
		    }
		}
	    }
	}

    return;
    }

/** called by child when all or part of the child is shown to the user **/
function osrc_cb_reveal(child)
    {
    if ((typeof child) == 'object' && child.eventName)
	{
	var e = child;
	switch (e.eventName) 
	    {
	    case 'Reveal':
		this.revealed_children++;
		break;
	    case 'Obscure':
		return this.Obscure(null);
		break;
	    case 'RevealCheck':
	    case 'ObscureCheck':
		pg_reveal_check_ok(e);
		break;
	    }
	}
    else
	{
	//alert(this.name + ' got Reveal');
	// Increment reveal counts
	this.revealed_children++;
	}
    if (this.revealed_children > 1) return 0;

    // User requested onReveal?  If so, do that here.
    //if (!this.bh_finished) return 0;
    var did_query = true;
    if ((this.autoquery == this.AQonFirstReveal || this.autoquery == this.AQonEachReveal) && !this.init)
	pg_addsched_fn(this,'InitQuery', [], 0);
    else if (this.autoquery == this.AQonEachReveal)
	this.ifcProbe(ifAction).SchedInvoke('QueryObject', {query:[], client:null, ro:this.readonly}, 0);
    else
	{
	did_query = false;
	this.Dispatch();
	}

    if (this.has_onreveal_relationship && this.hidden_change_cnt > 0)
	{
	this.hidden_change_cnt = 0;
	if (!did_query)
	    {
	    this.Resync(null);
	    did_query = true;
	    }
	}

    if (!did_query && this.resync_every_reveal)
	{
	// Force resync
	this.lastSync = null;
	this.Resync(null);
	}

    return 0;
    }

/** called by a child when all or part of the child is hidden from the user **/
function osrc_cb_obscure(child)
    {
    // Decrement reveal counts
    if (this.revealed_children > 0) this.revealed_children--;
    return 0;
    }

/** Control message handler **/
function osrc_cb_control_msg(m)
    {
    var s='';
    for(var i = 0; i<m.length;i++)
	s += m[i].target + ': ' + m[i].href + ' = ' + m[i].text + '\n';
    alert(s);
    return;
    }

function osrc_get_pending()
    {
    return (this.pending || this.masters_pending.length > 0)?1:0;
    }


// ifValue expression data defaults to the currently selected record.
// This allows for temporarily setting it to a different record.
//
function osrc_set_eval_record(recnum)
    {
    if (recnum >= this.FirstRecord && recnum <= this.LastRecord)
	this.EvalRecord = recnum;
    else if (recnum == null)
	this.EvalRecord = null;
    }

function osrc_get_value(n)
    {
    var v = null;
    if (n == 'is_client_savable')
	return this.is_client_savable?1:0;
    if (n == 'is_client_discardable')
	return this.is_client_discardable?1:0;
    /*if (n == 'cx__current_id')
	return this.CurrentRecord;
    if (n == 'cx__last_id')
	return this.LastRecord;
    if (n == 'cx__first_id')
	return this.FirstRecord;
    if (n == 'cx__final_id')
	return this.FinalRecord;*/
    var eval_rec = this.EvalRecord?this.EvalRecord:this.CurrentRecord;
    if (eval_rec && this.replica && this.replica[eval_rec])
	{
	for(var i in this.replica[eval_rec])
	    {
	    var col = this.replica[eval_rec][i];
	    if (col.oid == n)
		{
		if (col.value == null)
		    v = null;
		else if (col.type == 'integer')
		    v = parseInt(col.value);
		else
		    v = col.value;
		break;
		}
	    }
	}

    // Update prevcurrent - in some cases we let a null value sneak out
    // during a query pending, in that case we need to make sure we issue
    // a Changing() call once we have a real value.
    if (this.prevcurrent && eval_rec == this.CurrentRecord)
	{
	for(var j in this.prevcurrent)
	    {
	    if (typeof this.prevcurrent[j] != 'object') continue;
	    if (this.prevcurrent[j].oid == n)
		{
		this.prevcurrent[j].value = v;
		break;
		}
	    }
	}

    return v;
    }


function osrc_do_filter(r)
    {
    r.schedid = null;
    var query = [];
    query.oid=null;
    query.joinstring='AND';
    var f = r.filter_to_use;
    if (r.tw) f = f + '*';
    if (r.lw) f = '*' + f;
    if (f && r.prev_filter && f.valueOf() == r.prev_filter.valueOf()) return;
    r.prev_filter = f;
    var t={oid:r.field, value:f, type:'string'};
    query.push(t);
    this.ifcProbe(ifAction).Invoke("QueryObject", {query:query, client:this, ro:true});
    //alert('doing filter with ' + r.field + ' = ' + r.filter_to_use);
    }


function osrc_filter_changed(prop, oldv, newv)
    {
    var osrc = wgtrFindContainer(this, "widget/osrc");
    for(var i in osrc.rulelist)
	{
	var rl = osrc.rulelist[i];
	if (rl.ruletype == 'osrc_filter' && rl.widget == this)
	    {
	    if (rl.schedid)
		{
		pg_delsched(rl.schedid);
		rl.schedid = null;
		}
	    var nv = new String(newv);
	    if (nv.length >= rl.mc)
		{
		rl.schedid = pg_addsched_fn(osrc, 'osrc_do_filter', [rl], rl.qd);
		rl.filter_to_use = nv;
		}
	    }
	}
    return newv;
    }


function osrc_add_rule(rule_widget)
    {
    var rl = {ruletype:rule_widget.ruletype, widget:rule_widget};
    if (rl.ruletype == 'osrc_filter')
	{
	rl.mc = rule_widget.min_chars;
	if (rl.mc == null) rl.mc = 1;
	rl.qd = rule_widget.query_delay;
	if (rl.qd == null) rl.qd = 1000;
	rl.tw = rule_widget.trailing_wildcard;
	if (rl.tw == null) rl.tw = 1;
	rl.lw = rule_widget.leading_wildcard;
	if (rl.lw == null) rl.lw = 0;
	rl.field = rule_widget.fieldname;
	rl.prev_filter = null;
	rule_widget._osrc_filter_changed = osrc_filter_changed;
	htr_watch(rule_widget, "value", "_osrc_filter_changed");
	this.rulelist.push(rl);
	}
    else if (rl.ruletype == 'osrc_relationship')
	{
	rl.osrc = this;
	rl.aq = rule_widget.autoquery;
	if (typeof rl.aq == 'undefined') rl.aq = 1;
	rl.target_osrc = wgtrGetNode(this, rule_widget.target);
	rl.is_slave = rule_widget.is_slave;
	if (rl.is_slave == null)
	    rl.is_slave = 1;
	rl.revealed_only = rule_widget.revealed_only;
	if (rl.revealed_only == null)
	    rl.revealed_only = 0;
	rl.on_each_reveal = rule_widget.on_each_reveal;
	if (rl.on_each_reveal == null)
	    rl.on_each_reveal = 0;
	rl.enforce_create = rule_widget.enforce_create;
	if (rl.enforce_create == null)
	    rl.enforce_create = 1;
	rl.master_norecs_action = rule_widget.master_norecs_action;
	if (!rl.master_norecs_action) rl.master_norecs_action = 'sameasnull';
	rl.master_null_action = rule_widget.master_null_action;
	if (!rl.master_null_action) rl.master_null_action = 'nullisvalue';

	// Data local to this osrc
	if (rl.is_slave)
	    {
	    var slave = this;
	    var master = rl.target_osrc;
	    var skey = 'key_';
	    var mkey = 'target_key_';
	    var sobj = rule_widget.key_objname;
	    }
	else
	    {
	    var slave = rl.target_osrc;
	    var master = this;
	    var skey = 'target_key_';
	    var mkey = 'key_';
	    var sobj = rule_widget.target_key_objname;
	    }
	// default to key objectname specified for osrc
	if (!sobj)
	    sobj = slave.key_objname;
	var slaverule = {master:master, on_each_reveal:rl.on_each_reveal, revealed_only:rl.revealed_only, enforce_create:rl.enforce_create, autoquery:rl.aq, key:[], tkey:[], obj:sobj, master_norecs_action:rl.master_norecs_action, master_null_action:rl.master_null_action};
	var masterrule = {slave:slave, on_each_reveal:rl.on_each_reveal, revealed_only:rl.revealed_only, enforce_create:rl.enforce_create, autoquery:rl.aq, key:[], tkey:[], master_norecs_action:rl.master_norecs_action, master_null_action:rl.master_null_action};

	// Keys
	for(var keynum = 1; keynum <= 5; keynum++)
	    {
	    rl['key_' + keynum] = rule_widget['key_' + keynum];
	    rl['target_key_' + keynum] = rule_widget['target_key_' + keynum];
	    if (typeof rl[skey + keynum] != 'undefined')
		{
		slaverule.key.push(rl[skey + keynum]);
		slaverule.tkey.push(rl[mkey + keynum]);
		masterrule.key.push(rl[mkey + keynum]);
		masterrule.tkey.push(rl[skey + keynum]);
		}
	    }

	slave.relationships.push(slaverule);
	if (typeof master.relationships == 'undefined')
	    master.relationships = [];
	master.relationships.push(masterrule);
	}
    else if (rl.ruletype == 'osrc_key')
	{
	rl.key_fieldname = rule_widget.key_fieldname;
	rl.keying_method = rule_widget.keying_method;
	switch(rl.keying_method)
	    {
	    case 'maxplusone':
		rl.min_value = rule_widget.min_value;
		rl.max_value = rule_widget.max_value;
		break;

	    case 'counter':
		rl.object_path = rule_widget.object_path;
		rl.counter_attribute = rule_widget.counter_attribute;
		break;

	    case 'counterosrc':
		rl.osrc = null;
		if (rule_widget.osrc)
		    rl.osrc = wgtrGetNode(this, rule_widget.osrc);
		if (!rl.osrc)
		    rl.osrc = wgtrFindContainer(this, "widget/osrc");
		rl.counter_attribute = rule_widget.counter_attribute;
		break;

	    case 'sql':
		rl.sql = rule_widget.sql;
		break;

	    case 'value':
		rl.value_container = rule_widget;
		break;

	    default:
		alert("Invalid keying method '" + rl.keying_method + "' for osrc_key rule");
		break;
	    }
	this.rulelist.push(rl);
	}
    }


function osrc_queue_request(r)
    {
    if (r.Request == 'Query' || r.Request == 'QueryText' || r.Request == 'QueryObject')
	this.query_request_queue.push(r);
    else
	this.request_queue.push(r);
    }


function osrc_dispatch()
    {
    if (this.pending || this.masters_pending.length) return;
    var req = null;
    var requeue = [];
    while ((req = this.query_request_queue.shift()) != null)
	{
	switch(req.Request)
	    {
	    case 'Query':
		if (!this.qy_reveal_only || this.revealed_children > 0)
		    this.QueryHandler(req.Param);
		else
		    requeue.push(req);
		break;

	    case 'QueryText':
		this.QueryTextHandler(req.Param);
		break;

	    case 'QueryObject':
		this.QueryObjectHandler(req.Param);
		break;
	    }
	if (this.pending || this.masters_pending.length) break;
	}

    if (!this.pending && this.masters_pending.length == 0)
	{
	while ((req = this.request_queue.shift()) != null)
	    {
	    switch(req.Request)
		{
		case 'RefreshObject':
		    this.RefreshObjectHandler(req.Param);
		    break;

		case 'MoveTo':
		    this.MoveToRecordHandler(req.Param);
		    break;

		case 'FindObject':
		    this.FindObjectHandler(req.Param);
		    break;
		}
	    if (this.pending || this.masters_pending.length) break;
	    }
	}

    while((req = requeue.shift())) this.QueueRequest(req);
    return;
    }


// OSRC Client routines (for linking with another osrc)
function osrc_oc_resync(master_osrc)
    {
    if (!master_osrc)
	{
	// If master unknown, just issue a query that captures all relationship values
	this.ifcProbe(ifAction).Invoke("QueryObject", {query:[], client:null, ro:this.readonly});
	return;
	}

    // Find the responsible rule
    var found_rule = null;
    for(var i=0; i<this.relationships.length; i++)
	{
	if (this.relationships[i].master == master_osrc)
	    {
	    found_rule = this.relationships[i];
	    break;
	    }
	}
    if (!found_rule) return;
    if (found_rule.autoquery)
	{
	if (found_rule.revealed_only && this.revealed_children == 0)
	    {
	    this.hidden_change_cnt++;
	    return;
	    }
	var sync_param = {ParentOSRC:found_rule.master};
	for(var i=0; i<found_rule.key.length;i++)
	    {
	    sync_param['ParentKey'+(i+1)] = found_rule.tkey[i];
	    sync_param['ChildKey'+(i+1)] = found_rule.key[i];
	    }
	this.ifcProbe(ifAction).Invoke("Sync", sync_param);
	}
    }

function osrc_oc_data_available(master_osrc)
    {
    return;
    }

function osrc_oc_replica_moved(master_osrc)
    {
    this.Resync(master_osrc);
    return;
    }

function osrc_oc_is_discard_ready(master_osrc)
    {
    this.GoNogo(osrc_oc_is_discard_ready_yes, osrc_oc_is_discard_ready_no, master_osrc);
    return false;
    }

function osrc_oc_is_discard_ready_yes(master_osrc)
    {
    master_osrc.QueryContinue(this);
    }

function osrc_oc_is_discard_ready_no(master_osrc)
    {
    master_osrc.QueryCancel(this);
    }

function osrc_oc_object_available(o, master_osrc, why)
    {
    if (why == 'begincreate' && master_osrc.in_create)
	this.Clear();
    else if (why != 'create' || !this.in_create) //if (why != 'create' || this.replica.length != 0)
	this.Resync(master_osrc);
    return;
    }

function osrc_oc_object_created(o, master_osrc)
    {
    if (this.replica.length != 0)
	this.Resync(master_osrc);
    return;
    }

function osrc_oc_object_modified(o, row, master_osrc)
    {
    this.Resync(master_osrc);
    return;
    }

function osrc_oc_object_deleted(o, master_osrc)
    {
    this.Resync(master_osrc);
    return;
    }

function osrc_oc_operation_complete(is_success, master_osrc)
    {
    return true;
    }


// Bottom Half of the initialization - after everything has had a chance
// to osrc_init()
function osrc_init_bh()
    {
    var has_master = false;

    // Search for relationships... then register as an osrc client
    for(var i=0; i<this.relationships.length; i++)
	{
	var rule = this.relationships[i];
	if (rule.master)
	    {
	    has_master = true;
	    rule.master.Register(this);
	    if (rule.revealed_only)
		this.has_onreveal_relationship = true;
	    if (rule.on_each_reveal)
		this.resync_every_reveal = true;
	    //if (typeof rl.aq != 'undefined' && !rl.aq)
	    //    this.no_autoquery_on_resync = true;
		//pg_addsched_fn(this, "Resync", [], 0);
	    }
	}

    // Need to auto-determine AQnever vs AQonFirstReveal?
    if (this.autoquery == this.AQunset)
	{
	if (has_master)
	    this.autoquery = this.AQnever;
	else
	    this.autoquery = this.AQonFirstReveal;
	}

    // Do we have any clients?
    if (this.child.length == 0)
	pg_reveal_register_listener(this, true);

    this.bh_finished = true;

    // Autoquery on load?  Reveal event already occurred?
    if (this.autoquery == this.AQonLoad) 
	pg_addsched_fn(this,'InitQuery', [], 0);
    /*else if (this.revealed_children && (this.autoquery == this.AQonFirstReveal || this.autoquery == this.AQonEachReveal) && !this.init)
	pg_addsched_fn(this,'InitQuery', [], 0);*/
    }


function osrc_action_cancel_clients(aparam)
    {
    if (!this.is_client_discardable) return;

    // Do this in two steps - discard our immediate clients first, then pass the word
    // on to clients of clients.  This minimizes the chance of failures due to
    // relational integrity constraints.
    for (var c in this.child)
	{
	var cld = this.child[c];
	if (typeof cld.is_discardable != 'undefined' && cld.is_discardable)
	    cld.ifcProbe(ifAction).Invoke('Discard', {});
	}
    for (var c in this.child)
	{
	var cld = this.child[c];
	if (typeof cld.is_client_discardable != 'undefined' && cld.is_client_discardable)
	    cld.ifcProbe(ifAction).Invoke('DiscardClients', {});
	}
    }

function osrc_action_save_clients(aparam)
    {
    if (!this.is_client_savable) return;

    // Do this in two steps - save our immediate clients first, then pass the word
    // on to clients of clients.  This minimizes the chance of failures due to
    // relational integrity constraints.
    for (var c in this.child)
	{
	var cld = this.child[c];
	if (typeof cld.is_savable != 'undefined' && cld.is_savable)
	    cld.ifcProbe(ifAction).Invoke('Save', {});
	}
    for (var c in this.child)
	{
	var cld = this.child[c];
	if (typeof cld.is_client_savable != 'undefined' && cld.is_client_savable)
	    cld.ifcProbe(ifAction).Invoke('SaveClients', {});
	}
    }


function osrc_do_request(cmd, url, params, cb, target)
    {
    var first = true;
    if (!target) target = this;
    params.cx__akey = akey;
    params.ls__mode = 'osml';
    params.ls__req = cmd;
    if (this.sid) params.ls__sid = this.sid;
    if (((!this.ind_act || !this.req_ind_act) && cmd != 'create' && cmd != 'setattrs') || cmd == 'close')
	params.cx__noact = '1';
    for(var p in params)
	{
	url += (first?'?':'&') + htutil_escape(p) + '=' + htutil_escape(params[p]);
	first = false;
	}
    this.request_start_ts = pg_timestamp();
    pg_serialized_load(target, url, cb, !this.ind_act || !this.req_ind_act);
    this.req_ind_act = true;
    //this.onload = cb;
    //target.src = url;
    }


function osrc_print_debug()
    {
    var str = "\n";
    str += "First Record.... " + this.FirstRecord + "\n";
    str += "Last Record..... " + this.LastRecord + "\n";
    str += "Current Record.. " + this.CurrentRecord + "\n";
    str += "Target Record... [" + this.TargetRecord[0] + "," + this.TargetRecord[1] + "]\n";
    str += "OSML Record..... " + this.OSMLRecord + "\n";
    return str;
    }


function osrc_param_notify(pname, pwgt, datatype, curval)
    {
    this.params[pname] = {pname:pname, pwgt:pwgt, datatype:datatype, val:curval};
    }


function osrc_encode(pl)
    {
    var s = '';

    for(var p in pl)
	{
	var v = pl[p];
	var dt;

	if (this.type_list[p])
	    dt = this.type_list[p];
	else if (typeof v == 'number')
	    dt = 'integer';
	else
	    dt = 'string';

	if (!s)
	    s += '?';
	else
	    s += '&';
	if (v != null)
	    s += htutil_escape(p) + '=' + htutil_escape(dt) + ':V:' + htutil_escape(v);
	else
	    s += htutil_escape(p) + '=' + htutil_escape(dt) + ':N:';
	}

    return s;
    }


// Read through the current query parameters, and encode them into a string to
// be passed to the server.
function osrc_encode_params()
    {
    var s = '';
    var pl = {};
    for(var i in this.params)
	{
	var p = this.params[i];
	var v = p.pwgt.getvalue();
	if (!s)
	    s += '?';
	else
	    s += '&';
	if (v != null)
	    s += htutil_escape(p.pname) + '=' + htutil_escape(p.datatype) + ':V:' + htutil_escape(v);
	else
	    s += htutil_escape(p.pname) + '=' + htutil_escape(p.datatype) + ':N:';
	}
    return s;
    }


function osrc_set_pending(p)
    {
    var was_pending = this.pending;
    var was_any_pending = (this.pending || this.masters_pending.length > 0);
    this.pending = p;
    if (was_any_pending != (this.pending || this.masters_pending.length > 0) )
	this.ifcProbe(ifValue).Changing("cx__pending", (this.pending || this.masters_pending.length > 0)?1:0, true, was_any_pending?1:0, true);
    if (!this.pending && was_pending)
	this.Dispatch();

    if (this.pending != p) return;

    for(var i=0; i<this.child.length; i++)
	{
	if (wgtrGetType(this.child[i]) == 'widget/osrc')
	    {
	    this.child[i].SetMasterPending(this, p);
	    }
	}
    }


function osrc_set_master_pending(master, p)
    {
    var was_any_pending = (this.pending || this.masters_pending.length > 0);
    if (master == this) return;
    var already_pending = -1;
    for(var i=0; i<this.masters_pending.length; i++)
	if (this.masters_pending[i] == master)
	    {
	    already_pending = i;
	    break;
	    }
    if (already_pending == -1 && p)
	this.masters_pending.push(master);
    else if (already_pending != -1 && !p)
	this.masters_pending.splice(i,1);

    if (was_any_pending != (this.pending || this.masters_pending.length > 0) )
	this.ifcProbe(ifValue).Changing("cx__pending", (this.pending || this.masters_pending.length > 0)?1:0, true, was_any_pending?1:0, true);
    if (this.masters_pending.length == 0)
	this.Dispatch();

    for(var i=0; i<this.child.length; i++)
	{
	if (wgtrGetType(this.child[i]) == 'widget/osrc')
	    {
	    this.child[i].SetMasterPending(master, p);
	    }
	}
    }


function osrc_destroy()
    {
    pg_set(this, "src", "about:blank");
//    alert('destroying osrc ' + this.__WgtrName);
//    if (this.sid)
//	this.DoRequest('closesession', '/', {}, osrc_destroy_bh);
//    else
//	this.DestroyBH();
    }

//function osrc_destroy_bh()
//    {
//    alert('bh destroying osrc ' + this.__WgtrName);
//    this.sid = null;
//    pg_set(this, "src", "about:blank");
//    }


/**  OSRC Initializer **/
function osrc_init(param)
    {
    var loader = param.loader;
    ifc_init_widget(loader);
    loader.osrcname=param.name;
    loader.readahead=param.readahead;
    loader.scrollahead=param.scrollahead;
    loader.replicasize=param.replicasize;
    loader.initreplicasize = param.replicasize;
    loader.ind_act = param.ind_act;
    loader.req_ind_act = true;
    loader.qy_reveal_only = param.qy_reveal_only;
    loader.refresh_interval = param.refresh;
    loader.sql=param.sql;
    loader.filter=param.filter;
    loader.baseobj=param.baseobj;
    loader.use_having = param.use_having;
    loader.readonly = false;
    loader.autoquery = param.autoquery;
    loader.key_objname = param.key_objname;
    loader.revealed_children = 0;
    loader.rulelist = [];
    if (typeof loader.relationships == 'undefined')
	loader.relationships = [];
    loader.SyncID = osrc_syncid++;
    loader.bh_finished = false;
    loader.request_queue = [];
    loader.query_request_queue = [];
    loader.params = [];
    loader.destroy_widget = osrc_destroy;
    //loader.DestroyBH = osrc_destroy_bh;

    loader.data_start = 1;
    loader.pending = false;
    loader.masters_pending = [];
    loader.any_pending = false;

    loader.SetPending = osrc_set_pending;
    loader.SetMasterPending = osrc_set_master_pending;

    // autoquery types - must match htdrv_osrc.c's enum declaration
    loader.AQunset = -1;
    loader.AQnever = 0;
    loader.AQonLoad = 1;
    loader.AQonFirstReveal = 2;
    loader.AQonEachReveal = 3;

    // Handle declarative rules
    loader.addRule = osrc_add_rule;

    loader.osrc_do_filter = osrc_do_filter;
    loader.osrc_filter_changed = osrc_filter_changed;
    loader.osrc_oldoid_cleanup = osrc_oldoid_cleanup;
    loader.osrc_oldoid_cleanup_cb = osrc_oldoid_cleanup_cb;
    loader.osrc_open_query_startat = osrc_open_query_startat;
    loader.osrc_open_query_tail = osrc_open_query_tail;
    loader.osrc_action_modify_cb_2 = osrc_action_modify_cb_2;
    loader.ImportModifiedData = osrc_import_modified_data;
    loader.ParseOneAttr = osrc_parse_one_attr;
    loader.ParseOneRow = osrc_parse_one_row;
    loader.NewReplicaObj = osrc_new_replica_object;
    loader.PruneReplica = osrc_prune_replica;
    loader.ClearReplica = osrc_clear_replica;
    loader.ApplyRelationships = osrc_apply_rel;
    loader.ApplyKeys = osrc_apply_keys;
    loader.EndQuery = osrc_end_query;
    loader.FoundRecord = osrc_found_record;
    loader.DoFetch = osrc_do_fetch;
    loader.FetchNext = osrc_fetch_next;
    loader.GoNogo = osrc_go_nogo;
    loader.QueueRequest = osrc_queue_request;
    loader.Dispatch = osrc_dispatch;
    loader.DoRequest = osrc_do_request;
    loader.EncodeParams = osrc_encode_params;
    loader.Encode = osrc_encode;
    loader.GiveAllCurrentRecord=osrc_give_all_current_record;
    loader.ChangeCurrentRecord=osrc_change_current_record;
    loader.MoveToRecord=osrc_move_to_record;
    loader.MoveToRecordCB=osrc_move_to_record_cb;
    loader.Clear = osrc_action_clear;
    loader.child =  [];
    loader.oldoids =  [];
    loader.sid = null;
    loader.qid = null;
    loader.savable_client_count = 0;
    loader.discardable_client_count = 0;
    loader.lastquery = null;
    loader.prevcurrent = null;
    loader.has_onreveal_relationship = false;
    loader.resync_every_reveal = false;
    loader.hidden_change_cnt = 0;
    loader.query_delay = 0;
    loader.type_list = [];
    loader.do_append = false;
    loader.query_ended = false;
    loader.in_create = false;

    loader.MoveToRecordHandler = osrc_move_to_record_handler;
    loader.QueryObjectHandler = osrc_query_object_handler;
    loader.QueryTextHandler = osrc_query_text_handler;
    loader.QueryHandler = osrc_query_handler;
    loader.RefreshObjectHandler = osrc_refresh_object_handler;
    loader.FindObjectHandler = osrc_find_object_handler;
   
    // Actions
    var ia = loader.ifcProbeAdd(ifAction);
    ia.Add("Query", osrc_action_query);
    ia.Add("QueryObject", osrc_action_query_object);
    ia.Add("QueryParam", osrc_action_query_param);
    ia.Add("QueryText", osrc_action_query_text);
    ia.Add("OrderObject", osrc_action_order_object);
    ia.Add("Delete", osrc_action_delete);
    ia.Add("CreateObject", osrc_action_create_object);
    ia.Add("Create", osrc_action_create);
    ia.Add("Modify", osrc_action_modify);
    ia.Add("First", osrc_move_first);
    ia.Add("Next", osrc_move_next);
    ia.Add("Prev", osrc_move_prev);
    ia.Add("Last", osrc_move_last);
    ia.Add("Sync", osrc_action_sync);
    ia.Add("DoubleSync", osrc_action_double_sync);
    ia.Add("SaveClients", osrc_action_save_clients);
    ia.Add("DiscardClients", osrc_action_cancel_clients);
    ia.Add("Refresh", osrc_action_refresh);
    ia.Add("RefreshObject", osrc_action_refresh_object);
    ia.Add("ChangeSource", osrc_action_change_source);
    ia.Add("DoSQL", osrc_action_do_sql);
    ia.Add("FindObject", osrc_action_find_object);
    ia.Add("Clear", osrc_action_clear);
    ia.Add("BeginCreateObject", osrc_action_begincreate);
    ia.Add("CancelCreateObject", osrc_action_cancelcreate);

    // Events
    var ie = loader.ifcProbeAdd(ifEvent);
    ie.Add("DataFocusChanged");
    ie.Add("EndQuery");
    ie.Add("Created");
    ie.Add("Modified");

    // Data Values
    var iv = loader.ifcProbeAdd(ifValue);
    iv.SetNonexistentCallback(osrc_get_value);
    iv.Add("cx__pending", osrc_get_pending, null);
    iv.Add("cx__current_id", "CurrentRecord", null);
    iv.Add("cx__last_id", "LastRecord", null);
    iv.Add("cx__first_id", "FirstRecord", null);
    iv.Add("cx__final_id", "FinalRecord", null);

    loader.SetEvalRecord = osrc_set_eval_record;

    loader.RefreshTimer = osrc_refresh_timer;
    loader.ParamNotify = osrc_param_notify;
    loader.CreateCB2 = osrc_action_create_cb2;
    loader.DoubleSyncCB = osrc_action_double_sync_cb;
    loader.OpenSession=osrc_open_session;
    loader.OpenQuery=osrc_open_query;
    loader.CloseQuery=osrc_close_query;
    loader.CloseObject=osrc_close_object;
    loader.CloseSession=osrc_close_session;
/**    loader.StoreReplica=osrc_store_replica; **/
    loader.QueryContinue = osrc_cb_query_continue;
    loader.QueryCancel = osrc_cb_query_cancel;
    loader.RequestObject = osrc_cb_request_object;
    loader.NewObjectTemplate = osrc_cb_new_object_template;
    loader.SetViewRange = osrc_cb_set_view_range;
    loader.InitBH = osrc_init_bh;
    loader.Register = osrc_cb_register;
    loader.Reveal = osrc_cb_reveal;
    loader.Obscure = osrc_cb_obscure;
    loader.MakeFilter = osrc_make_filter;
    loader.MakeFilterString = osrc_make_filter_string;
    loader.MakeFilterInteger = osrc_make_filter_integer;
    loader.MFCol = osrc_make_filter_colref;
    if (wgtrGetChildren(loader).length == 0)
	pg_reveal_register_listener(loader, true);

    loader.ScrollTo = osrc_scroll_to;
    loader.ScrollPrev = osrc_scroll_prev;
    loader.ScrollNext = osrc_scroll_next;
    loader.ScrollPrevPage = osrc_scroll_prev_page;
    loader.ScrollNextPage = osrc_scroll_next_page;

    loader.TellAllReplicaMoved = osrc_tell_all_replica_moved;

    loader.InitQuery = osrc_init_query;
    loader.cleanup = osrc_cleanup;

    // this is triggered <n> msec after a query returns data, so
    // we don't hold open locks on the server forever.
    loader.QueryTimeout = osrc_query_timeout;

    // OSRC Client interface -- for linking osrc's together
    loader.DataAvailable = osrc_oc_data_available;
    loader.ReplicaMoved = osrc_oc_replica_moved;
    loader.IsDiscardReady = osrc_oc_is_discard_ready;
    loader.ObjectAvailable = osrc_oc_object_available;
    loader.ObjectCreated = osrc_oc_object_created;
    loader.ObjectModified = osrc_oc_object_modified;
    loader.ObjectDeleted = osrc_oc_object_deleted;
    loader.OperationComplete = osrc_oc_operation_complete;

    // OSRC Client interface helpers
    loader.Resync = osrc_oc_resync;

    // Client side maintained properties
    loader.is_client_savable = false;
    loader.is_client_discardable = false;

    // Debugging functions
    loader.print_debug = osrc_print_debug;

    // Whether to send inserts/deletes/updates to the server (defaults to YES)
    loader.send_updates = param.send_updates?1:0;

    // Request replication messages
    loader.request_updates = param.requestupdates?1:0;
    if (param.requestupdates) pg_msg_request(loader, pg_msg.MSG_REPMSG);
    loader.ControlMsg = osrc_cb_control_msg;

    // do sql loader
    loader.do_sql_loader = null;
    loader.osrc_action_do_sql_cb = osrc_action_do_sql_cb;

    // Zero out the replica
    loader.ClearReplica();

    /*if (loader.autoquery == loader.AQonLoad) 
	pg_addsched_fn(loader,'InitQuery', [], 0);*/

    // Finish initialization...
    pg_addsched_fn(loader, "InitBH", [], 0);

    return loader;
    }


// Load indication
if (window.pg_scripts) pg_scripts['htdrv_osrc.js'] = true;
