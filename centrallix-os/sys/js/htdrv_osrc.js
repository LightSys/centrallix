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
//

// Merge in our object types
$.extend($CX.Types,
    {
    DateTime: function(d)
	{
	if (d && d.year)
	    {
	    Object.assign(this, d);
	    this.dateobj = new Date(this.year, this.month - 1, this.day, this.hour, this.minute, this.second);
	    }
	else if (typeof d == 'object' && Date.prototype.isPrototypeOf(d))
	    {
	    this.dateobj = d;
	    this.year = d.getFullYear();
	    this.month = d.getMonth() + 1;
	    this.day = d.getDate();
	    this.hour = d.getHours();
	    this.minute = d.getMinutes();
	    this.second = d.getSeconds();
	    }
	else
	    {
	    d = this.dateobj = new Date();
	    this.year = d.getFullYear();
	    this.month = d.getMonth() + 1;
	    this.day = d.getDate();
	    this.hour = d.getHours();
	    this.minute = d.getMinutes();
	    this.second = d.getSeconds();
	    }
	},
    Money: function()
	{
	},
    osrcObject: function(obj)
	{
	if (obj) this.copyFrom(obj);
	},
    osrcAttr: function(attr)
	{
	if (typeof attr == 'object' && $CX.Types.osrcAttr.prototype.isPrototypeOf(attr))
	    {
	    // Type osrcAttr
	    this.v = attr.v;
	    this.t = attr.t;
	    this.h = attr.h;
	    this.s = attr.s;
	    this.y = attr.y;
	    this.a = attr.a;
	    this.__cx_filter = Object.assign({}, attr.__cx_filter);
	    }
	else
	    {
	    // Generic value
	    this.v = attr;
	    }
	}
    });


// Money methods
//   === money format: ===
//   I = a leading 'I' is not printed but indicates the currency is in "international format" with commas and periods reversed.
//   Z = a leading 'Z' is not printed but means zeros should be printed as "-0-"
//   z = a leading 'z' is not printed but means zeros should be printed as "0"
//   B = a leading 'B' is not printed but means zeros should be printed as "" (blank)
//   # = optional digit unless after the '.' or first before '.'
//   0 = mandatory digit, no zero suppression
//   , = insert a comma (or a period, if in international format)
//   . = decimal point (only one allowed) (prints a comma if in international format)
//   $ = dollar sign
//   + = mandatory sign, whether + or - (if 0, +)
//   - = optional sign, space if + or 0
//   ^ = if last digit, round it (trunc is default).  Otherwise, like '0'.  NOT YET IMPL.
//  () = surround # with () if it is negative.
//  [] = surround # with () if it is positive.
//     = (space) optional digit, but put space in its place if suppressing 0's.
//   * = (asterisk) optional digit, put asterisk in its place if suppressing 0s.
//
$CX.Types.Money.prototype.format = function(f)
    {
    var ch;
    var is_intl=false;
    var is_acczero=false;
    var is_plainzero=false;
    var is_blankzero=false;
    var str = '';

    for(var i=0; i<=f.length; i++)
	{
	ch = (i<f.length)?(f.charAt(i)):'';
	switch(ch)
	    {
	    case '$': str += ch; break;
	    }
	}

    return str;
    }

$CX.Types.Money.prototype.toString = function()
    {
    return this.format($CX.Globals.moneyFormat);
    }

// DateTime methods
$CX.Types.DateTime.prototype.adjFromServer = function()
    {
    var d = this.dateobj = new Date(this.year, this.month - 1, this.day, this.hour, this.minute, this.second);
    d.setTime(d.getTime() + pg_clockoffset);
    this.year = d.getFullYear();
    this.month = d.getMonth() + 1;
    this.day = d.getDate();
    this.hour = d.getHours();
    this.minute = d.getMinutes();
    this.second = d.getSeconds();
    }

$CX.Types.DateTime.prototype.format = function(f)
    {
    var ch;
    var item = '';
    var str = '';
    for(var i=0; i<=f.length; i++)
	{
	ch = (i<f.length)?(f.charAt(i)):'';
	if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
	    item += ch;
	else
	    {
	    if (item.length > 0)
		{
		switch(item)
		    {
		    case 'dd': str += ((this.day < 10)?'0':'') + this.day; break;
		    case 'MMM': str += (new Intl.DateTimeFormat([], {month: 'short'})).format(this.dateobj); break;
		    case 'yyyy': str += this.year; break;
		    case 'HH': str += ((this.hour < 10)?'0':'') + this.hour; break;
		    case 'mm': str += ((this.minute < 10)?'0':'') + this.minute; break;
		    }
		}
	    str += ch;
	    item = '';
	    }
	}
    return str;
    }

$CX.Types.DateTime.prototype.toString = function()
    {
    return this.format($CX.Globals.dateFormat);
    }

//
// osrcObject methods
//
$CX.Types.osrcObject.prototype.getAttrCount = function()
    {
    var acnt = 0;
    for(var aname in this)
	if (typeof this[aname] == 'object' && $CX.Types.osrcAttr.prototype.isPrototypeOf(this[aname]))
	    acnt++;
    return acnt;
    }

$CX.Types.osrcObject.prototype.getAttrList = function()
    {
    var alist = {};
    for(var aname in this)
	if (typeof this[aname] == 'object' && $CX.Types.osrcAttr.prototype.isPrototypeOf(this[aname]))
	    alist[aname] = true;
    return alist;
    }

$CX.Types.osrcObject.prototype.getID = function()
    {
    return this.__cx_id;
    }

$CX.Types.osrcObject.prototype.setID = function(id)
    {
    return this.__cx_id = id;
    }

$CX.Types.osrcObject.prototype.setJoin = function(join)
    {
    return this.__cx_joinstring = join;
    }

$CX.Types.osrcObject.prototype.getJoin = function()
    {
    return this.__cx_joinstring;
    }

$CX.Types.osrcObject.prototype.getAttr = function(a)
    {
    if (a != '__cx_joinstring' && a != '__cx_id' && a != '__cx_handle' && a != '__proto__' && typeof this[a] == 'object')
	{
	if (!this[a].a) this[a].a = a;
	return this[a];
	}
    return null;
    }

$CX.Types.osrcObject.prototype.getAttrValue = function(a)
    {
    var attr = this.getAttr(a);
    return attr?attr.get():null;
    }

$CX.Types.osrcObject.prototype.getAttrType = function(a)
    {
    var attr = this.getAttr(a);
    return attr?attr.getType():null;
    }

$CX.Types.osrcObject.prototype.copyFrom = function(o)
    {
    if (typeof o == 'object')
	{
	if ($CX.Types.osrcObject.prototype.isPrototypeOf(o))
	    {
	    // osrcObject type
	    var attrlist = o.getAttrList();
	    for(var a in attrlist)
		{
		this[a] = new $CX.Types.osrcAttr(o.getAttr(a));
		}
	    }
	else
	    {
	    // Generic object -- just copy property names and values
	    for(var p in o)
		{
		this[p] = new $CX.Types.osrcAttr(o[p]);
		}
	    }
	}
    }

// Either invoke with newAttr(attrname, value, type) or as newAttr(attrname, attrobj)
$CX.Types.osrcObject.prototype.newAttr = function(a,v,t)
    {
    if (a)
	{
	this[a] = new $CX.Types.osrcAttr(v);
	if (t) this[a].t = t;
	return this[a];
	}
    }

$CX.Types.osrcObject.prototype.removeAttr = function(a)
    {
    var a = this.getAttr(a);
    if (a)
	{
	delete this[a];
	}
    return a;
    }

//
// osrcAttr methods
//
$CX.Types.osrcAttr.prototype.get = function()
    {
    return this.v;
    }

$CX.Types.osrcAttr.prototype.set = function(v)
    {
    return this.v = v;
    }

$CX.Types.osrcAttr.prototype.getSystem = function()
    {
    return this.y;
    }

$CX.Types.osrcAttr.prototype.getType = function()
    {
    return this.t;
    }

$CX.Types.osrcAttr.prototype.setType = function(t)
    {
    return this.t = t;
    }

$CX.Types.osrcAttr.prototype.getHints = function()
    {
    return this.h;
    }

$CX.Types.osrcAttr.prototype.getFilter = function()
    {
    if (!this.__cx_filter)
	this.__cx_filter = {};
    return this.__cx_filter;
    }


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
    var qo = new $CX.Types.osrcObject();
    qo.setJoin('AND');

    // Add a criteria item for each aparam item
    for (var i in aparam)
	{
	var v = aparam[i];
	var t = 'undefined';

	if (i == 'cx__enable_lists' || i == 'cx__case_insensitive' || i == '_Origin' || i == '_EventName')
	    continue;
	if (i == 'joinstring' && (new String(aparam[i])).toLowerCase() == 'or')
	    {
	    qo.setJoin('OR');
	    continue;
	    }

	// Determine type
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

	// Add the attribute to the query object
	qo.newAttr(i, v, t);
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
    if (!this.qy_reveal_only || this.revealed_children > 0)
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
    this.refresh_objname = null;
    if (this.replica[this.CurrentRecord])
	{
	if (this.replica[this.CurrentRecord].name)
	    {
	    this.refresh_objname = this.replica[this.CurrentRecord].name.get();
	    }
	}

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
    var s = new String(this.getSQL());
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
    var initiating_client = aparam.client;
    var appendrows = (aparam.cx__appendrows)?true:false;
    var statement=this.getSQL();
    var case_insensitive = (aparam.cx__case_insensitive)?true:false;

    var sel_re = /^\s*(set\s+rowcount\s+[0-9]+\s+)?select\s+/i;
    var is_select = sel_re.test(this.getSQL());

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
	filter += osrcsep;
	statement += '(' + this.filter + ')';
	firstone = false;
	}

    // add any relationships
    var rel = new $CX.Types.osrcObject();
    this.ApplyRelationships(rel, false, false);
    if (!$.isEmptyObject(rel))
	{
	statement += osrcsep;
	rel.setJoin('AND');
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

    this.ifcProbe(ifAction).Invoke("Query", {query:statement, client:initiating_client, appendrows:appendrows});
    }


function osrc_action_query_object(aparam) //q, initiating_client, readonly)
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
    var initiating_client = aparam.client;
    var q = aparam.query; // type $CX.Types.osrcObject
    var readonly = aparam.ro;
    var appendrows = (aparam.cx__appendrows)?true:false;
    if (typeof aparam.cx__appendrows != 'undefined')
	delete aparam.cx__appendrows;
    var params = {};
    var filter = new $CX.Types.osrcObject();

    // Backwards compat API
    if (Array.prototype.isPrototypeOf(q))
	{
	var new_q = new $CX.Types.osrcObject();
	for(var i=0; i<q.length; i++)
	    {
	    new_q.newAttr(q[i].oid, q[i].value, q[i].type);
	    }
	q = new_q;
	}

    if(this.pending)
	{
	alert('There is already a query or movement in progress...');
	return 0;
	}

    this.move_target = aparam.targetrec;

    if (typeof q != 'undefined' && q !== null)
	this.ApplyRelationships(q, false, false);

    // Copy ID and join info to filter
    if (q.getJoin())
	filter.setJoin(q.getJoin());
    else
	filter.setJoin('AND');
    filter.setID(q.getID());
    
    // Go through the query's specified criteria
    var al = q.getAttrList();
    for(var a in al)
	{
	var oneattr = q.getAttr(a);
	if (this.params[a])
	    params[a] = oneattr;
	else
	    filter.newAttr(a, oneattr);
	}
  
    // Go through query parameters to see if we need to set any.
    for(var pn in this.params)
	{
	var found = false;
	for(var a in params)
	    {
	    if (a == pn && typeof params[a].get() != 'undefined')
		{
		found = true;
		this.params[pn].pwgt.ifcProbe(ifAction).Invoke("SetValue", {Value:params[a].get()});
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
    var is_select = sel_re.test(this.getSQL());

    this.pendingqueryobject=q;
    this.querytext = null;
    var statement=this.getSQL();

    if (this.use_having)
	var sep = ' HAVING ';
    else
	var sep = ' WHERE ';
    var firstone=true;
    
    if (this.filter)
	{
	statement += (sep + '(' + this.filter + ')');
	firstone=false;
	}
    if (filter && filter.getJoin() && filter.getAttrCount() > 0)
	{
	var filt = this.MakeFilter(filter);
	if (filt)
	    {
	    if (firstone)
		statement += sep;
	    else
		statement += ' ' + q.getJoin() + ' ';
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
    this.ifcProbe(ifAction).Invoke("Query", {query:statement, client:initiating_client, appendrows:appendrows});
    }


function osrc_make_filter_colref(col)
    {
    var filt = col.getFilter();
    return (filt.obj?(':"' + filt.obj + '"'):'') + ':"' + col.a + '"';
    }


function osrc_make_filter_integer(col, val)
    {
    var filt = col.getFilter();

    if (val == null && (typeof filt.nullisvalue == 'undefined' || filt.nullisvalue == true))
	return this.MFCol(col) + ' is null ';
    else if (val == null)
	return this.MFCol(col) + ' = null ';
    else if (!filt.plainsearch && typeof val != 'number' && (new String(val)).search(/-/)>=0)
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
    var filt = col.getFilter();

    if (icase)
	ifunc = 'upper';
    if (val == null && (typeof filt.nullisvalue == 'undefined' || filt.nullisvalue == true))
	str=colref + ' is null ';
    else if (val == null)
	str=colref + ' = null ';
    else if (filt.plainsearch)
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
    var al = q.getAttrList();
    for(var a in al)
	{
	var col = q.getAttr(a);
	var filt = col.getFilter();
	isnot = false;
	var str;
	if (filt.force_empty)
	    {
	    str = " (" + this.MFCol(col) + " = null and 1 == 0) ";
	    }
	else
	    {
	    var val=col.get();

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

	    // Use a "remembered" type for this attribute, if not supplied?
	    if (typeof col.getType() == "undefined" && this.type_list[a])
		col.setType(this.type_list[a]);

	    var colref = this.MFCol(col);

	    switch(col.getType())
		{
		case 'criteria':
		    str = this.MakeFilter(val);
		    break;

		case 'integer':
		    str = this.MakeFilterInteger(col, val);
		    break;

		case 'integerarray':
		    if (val == null && (typeof filt.nullisvalue == 'undefined' || filt.nullisvalue == true))
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
			    str += this.MakeFilterInteger(col, val[j]);
			    str += ")";
			    }
			str += ")";
			}
		    break;
		case 'undefinedarray':
		    if (val == null && (typeof filt.nullisvalue == 'undefined' || filt.nullisvalue == true))
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
				str += this.MakeFilterInteger(col, val[j]);
			    else
				str += this.MakeFilterString(col, val[j]);
			    str += ")";
			    }
			str += ")";
			}
		    break;
		case 'stringarray':
		    if (val == null && (typeof filt.nullisvalue == 'undefined' || filt.nullisvalue == true))
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
			    str += this.MakeFilterString(col, val[j]);
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
		    str = this.MakeFilterString(col, val, col.getType() == 'istring');
		    break;

		default:
		    //htr_alert(val, 1);
		    if(!val || typeof val.substring == 'undefined') // assume integer
			str = this.MakeFilterInteger(col, val);
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
			str = this.MakeFilterString(col, val);
			}
		    break;
		}
	    }

	if (isnot)
	    str = "not (" + str + ")";

	if (firstone)
	    statement += ' (' + str + ')';
	else
	    statement += ' ' + q.getJoin() + ' (' + str + ')';

	firstone=false;
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


function osrc_action_query(aparam) //q, initiating_client)
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
    var initiating_client = aparam.client;

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

function osrc_action_delete(aparam) //up,initiating_client)
    {
    var up = aparam.data;
    var initiating_client = aparam.client;

    // Delete an object through OSML
    this.initiating_client = initiating_client;
    this.deleteddata=up;
    this.DoRequest('delete', this.baseobj, {ls__oid:up.oid}, osrc_action_delete_cb);

    return 0;
    }

function osrc_action_delete_cb()
    {
    var links = pg_links(this);
    var initiating_client = this.initiating_client;
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
		this.replica[i].__cx_id = i;
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
	    this.SyncID = osrc_syncid++; // force any client osrc's to resync
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
	if (initiating_client) initiating_client.OperationComplete(true, this);
	}
    else
	{
	// delete failed
	if (initiating_client) initiating_client.OperationComplete(false, this);
	}
    this.initiating_client=null;
    delete this.deleteddata;
    return 0;
    }


function osrc_action_create(aparam)
    {
    var focus = aparam.cx__focus;
    delete aparam.cx__focus;
    var newobj = new $CX.Types.osrcObject(aparam);
    this.ifcProbe(ifAction).Invoke("CreateObject", {client:null, data:newobj, focus:focus});
    }


function osrc_action_create_object(aparam) //up,initiating_client
    {
    // Process create parameters
    this.initiating_client = aparam.client;
    this.create_focus = aparam.focus?aparam.focus:false;
    this.createddata = aparam.data;

    // First close the currently open query, if any.
    if (this.qid)
	{
	this.DoRequest('queryclose', '/', {ls__qid:this.qid}, osrc_action_create_cb2);
	this.qid=null;
	return 0;
	}

    // Go get a session if we don't have one already.
    if (!this.sid)
	{
	this.ClearReplica();
	this.OpenSession(this.CreateCB2);
	return 0;
	}

    // Otherwise, proceed on to the create.
    this.CreateCB2(null);
    return 0;
    }


// Create an object through OSML
function osrc_action_create_cb2(data)
    {
    // Capture the session ID if we just opened a session.
    if (!this.sid && data && data.session)
	this.sid = data.session;

    // Apply any constraints to the data to be used in the create
    this.ApplyRelationships(this.createddata, true, false);
    this.ApplySequence(this.createddata);

    // Set up the create options
    var reqparam = {ls__reopen_sql:this.getSQL(), ls__sqlparam:this.EncodeParams()};
    if (this.use_having) reqparam.ls__reopen_having = 1;
    var attrlist = this.createddata.getAttrList();
    for(var a in attrlist)
	{
	var oneattr = this.createddata.getAttr(a);
	if (oneattr.get() == null)
	    reqparam[a] = 'N:';
	else
	    reqparam[a] = 'V:' + oneattr.get();
	}

    // Issue the request
    this.DoRequest('create', this.baseobj + '/*', reqparam, osrc_action_create_cb);
    }


function osrc_action_create_cb(data)
    {
    // Did the create succeed?
    if (data && data.status == 'OK')
	{
	// Succeeded
	if (this.FinalRecord == this.LastRecord)
	    this.FinalRecord++;
	this.LastRecord++;
	this.CurrentRecord = this.LastRecord;
	var recnum=this.CurrentRecord;
	var cr=this.replica[this.CurrentRecord];
	if (!cr)
	    cr = new $CX.Types.osrcObject();

	// update replica with our data.
	cr.copyFrom(this.createddata);
	cr.__cx_id = this.CurrentRecord;
	this.replica[this.CurrentRecord] = cr;

	// New data from server?  Override 'createddata' if so.
	if (data.resultset.length == 1)
	    {
	    cr.copyFrom(data.resultset[0]);
	    cr.__cx_handle = data.resultset[0].__cx_handle;
	    }

	// Notify clients of the newly created object.
	this.in_create = false;
	this.SyncID = osrc_syncid++;
	if (this.initiating_client)
	    this.initiating_client.OperationComplete(true, this);
	for(var i in this.child)
	    this.child[i].ObjectCreated(recnum, this);
	this.GiveAllCurrentRecord('create');
	this.ifcProbe(ifEvent).Activate("Created", {});
	//if (this.create_focus)
	//    this.MoveToRecord(this.LastRecord, true);
	}
    else
	{
	// Did not succeed - let the calling client know.
	this.in_create = false;
	if (this.initiating_client)
	    this.initiating_client.OperationComplete(false, this);
	}

    this.initiating_client=null;
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
    if (!this.lastquery || !this.CurrentRecord || !this.replica || !this.replica[this.CurrentRecord])
	return false;

    // Build list of primary keys
    var row = this.replica[this.CurrentRecord];
    var keycnt = 0;
    var nameattr = null;
    var filter = new $CX.Types.osrcObject();
    var attrlist = row.getAttrList();
    for(var a in attrlist)
	{
	var attr = row.getAttr(a);
	var ph = cx_parse_hints(attr.getHints());
	if (ph.Style & cx_hints_style.key)
	    {
	    filter.newAttr(a, attr);
	    keycnt++;
	    }
	if (a == 'name')
	    nameattr = attr;
	}
    if (!keycnt)
	{
	if (!nameattr)
	    return false;
	else
	    filter.newAttr('name', nameattr);
	}

    // Start with the lastquery SQL.
    var sql = this.lastquery;

    // Append logic to search for just this row
    if (this.use_having)
	sql += " HAVING ";
    else
	sql += " WHERE ";
    sql += this.MakeFilter(filter);
    /*if (keycnt > 0)
	{
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
	}*/
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


function osrc_refresh_object_cb(data)
    {
    // Do we have a valid result from the refresh?
    if (data && data.status == 'OK' && data.resultset.length > 0)
	{
	// Check new/corrected data provided by server
	var cr = this.replica[this.CurrentRecord];
	var server_rec = data.resultset[0];
	var diff = false;
	var attrlist = server_rec.getAttrList();
	for(var a in attrlist)
	    {
	    var attr = server_rec.getAttr(a);
	    var cattr = cr.getAttr(a);
	    if (!cattr)
		cattr = cr[a] = new $CX.Types.osrcAttr();
	    if (cattr.get() != attr.get())
		{
		cattr.v = attr.get();
		cattr.t = attr.getType();
		diff = true;
		}
	    }

	// if any changes, tell our clients about them.
	if (diff)
	    {
	    this.SyncID = osrc_syncid++;
	    this.GiveAllCurrentRecord('refresh');
	    }
	}
    }


function osrc_action_modify(aparam) //up,initiating_client
    {
    this.doing_refresh = false;
    if (aparam)
	{
	this.modifieddata = aparam.data;
	this.initiating_client = aparam.client;
	}

    // initiated by a connector?  use current record and convert the data
    if (aparam && (!aparam.data || !aparam.data.oid))
	{
	this.initiating_client = null;
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
    var reqparam = {ls__oid:this.modifieddata.oid, ls__reopen_sql:this.getSQL(), ls__sqlparam:this.EncodeParams()};
    if (this.use_having) reqparam.ls__reopen_having = 1;

    //var src='/?cx__akey='+akey+'&ls__mode=osml&ls__req=setattrs&ls__sid=' + this.sid + '&ls__oid=' + this.modifieddata.oid;
    this.ApplyRelationships(this.modifieddata, false, true);
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
	if (this.initiating_client) this.initiating_client.OperationComplete(false, this);
	this.initiating_client=null;
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
    if (this.initiating_client)
	this.initiating_client.OperationComplete(true, this);
    for(var i in this.child)
	this.child[i].ObjectModified(this.CurrentRecord, this.replica[this.CurrentRecord], this);
    this.ChangeCurrentRecord();
    if (diff)
	this.GiveAllCurrentRecord('modify');
    if (!this.initiating_client)
	this.ifcProbe(ifEvent).Activate('Modified', {});
    this.initiating_client=null;
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
    if (this.initiating_client && this.initiating_client.OperationComplete)
	this.initiating_client.OperationComplete(false, this);
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

    if (this.replica && this.replica.length != 0)
	{
	pg_addsched_fn(this,'GiveOneCurrentRecord', [this.child.length - 1, 'change'], 0);
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
	cb.call(this, null);
	}
    else
	{
	this.DoRequest('opensession', '/', {}, cb);
	}
    }

function osrc_open_query()
    {
    //Open Query
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
    this.ifcProbe(ifEvent).Activate("BeginQuery", {});
    this.DoRequest('multiquery', '/', reqobj, osrc_get_qid);
    this.querysize = this.replicasize;
    }

function osrc_get_qid(data)
    {
    // Check for handles
    if (data)
	{
	// Get session handle
	if (!this.sid && data.session)
	    this.sid = data.session;

	// Get query handle
	if (data.queryclosed)
	    this.qid = null;
	else
	    this.qid = data.query;
	}

    // No valid query run?  Bail out if so.
    if (!data || !data.query)
	{
	this.move_target = null;
	this.GiveAllCurrentRecord('get_qid');
	this.SetPending(false);
	}
    else
	{
	// Valid query
	this.query_delay = pg_timestamp() - this.request_start_ts;
	for(var i in this.child)
	    this.child[i].DataAvailable(this, this.doing_refresh?'refresh':'query');
	if (this.move_target)
	    var tgt = this.move_target;
	else
	    var tgt = 1;
	this.move_target = null;
	if (data.resultset.length > 0)
	    {
	    // did an autofetch - we have the data already
	    if (!this.do_append)
		this.ClearReplica();
	    this.TargetRecord = [tgt,tgt];
	    this.CurrentRecord = tgt;
	    this.moveop = true;
	    this.FetchNext(data);
	    }
	else
	    {
	    // start the ball rolling for the fetch
	    this.ifcProbe(ifAction).Invoke("FindObject", {ID:tgt, from_internal:true});
	    }
	}
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
    var obj = new $CX.Types.osrcObject();
    obj.__cx_handle = oid;
    obj.__cx_id = id;
    return obj;
    }

function osrc_prune_replica(most_recent_id)
    {
    // Remove records from beginning of replica?
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
	    if (this.replica[this.FirstRecord])
		{
		this.oldoids.push(this.replica[this.FirstRecord].__cx_handle);
		delete this.replica[this.FirstRecord];
		}
	    this.FirstRecord++;
	    }
	}

    // Remove records from end of replica?
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
		this.oldoids.push(this.replica[this.LastRecord].__cx_handle);
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
    this.TargetRecord = [1,1];	// the record we're aiming for -- go until we get it
    this.CurrentRecord=1;	// the current record
    this.OSMLRecord=0;		// the last record we got from the OSML

    // Clear replica contents
    if(this.replica)
	for(var i in this.replica)
	    this.oldoids.push(this.replica[i].__cx_handle);
    
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
    var qid=this.qid
    this.qid=null;

    // If we retrieved any data at all, mark the last one as the Final Record.
    if (this.LastRecord >= this.FirstRecord && this.replica[this.LastRecord])
	{
	this.replica[this.LastRecord].__osrc_is_last = true;
	this.FinalRecord = this.LastRecord;
	}
    else if (this.LastRecord < this.FirstRecord)
	{
	// No data returned at all
	this.FinalRecord = this.LastRecord;
	}
    this.query_ended = true;
    this.FoundRecord();
    if(qid)
	{
	this.DoRequest('queryclose', '/', {ls__qid:qid}, osrc_close_query);
	}
    this.ifcProbe(ifEvent).Activate("EndQuery", {FinalRecord:this.FinalRecord, LastRecord:this.LastRecord, FirstRecord:this.FirstRecord, CurrentRecord:this.CurrentRecord});
    this.doing_refresh = false;
    this.Dispatch();

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
    this.SetPending(false);
    this.osrc_oldoid_cleanup();
    this.ifcProbe(ifEvent).Activate("Results", {FinalRecord:this.FinalRecord, LastRecord:this.LastRecord, FirstRecord:this.FirstRecord, CurrentRecord:this.CurrentRecord});
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

function osrc_fetch_next(data)
    {
    // No data supplied?
    if (!data || !data.query)
	{
	if (pg_diag) confirm("fetch_next: error - no qid.  first/last/cur/osml: " + this.FirstRecord + "/" + this.LastRecord + "/" + this.CurrentRecord + "/" + this.OSMLRecord + "\n");
	return 0;
	}

    // query over?
    if (data.resultset.length == 0)
	{
	this.EndQuery();
	return 0;
	}

    // Records skipped?
    if (data.skipped > 0)
	{
	this.OSMLRecord += data.skipped;
	this.querysize++;
	}

    // Import the data
    var rowcnt = 0;
    for(var i=0; i<data.resultset.length; i++)
	{
	// Store the retrieved object into the replica.
	rowcnt++;
	this.OSMLRecord++;
	var obj = this.replica[this.OSMLRecord] = data.resultset[i];
	obj.__cx_id = this.OSMLRecord;
	this.PruneReplica(this.OSMLRecord);

	// Record the data types we're observing in this object.
	for(var a in obj)
	    if (typeof obj[a] == 'object')
		this.type_list[a] = obj[a].getType();

	// If we're refreshing, compare the target object name.
	if (this.doing_refresh && this.refresh_objname && obj.getAttrValue('name') == this.refresh_objname)
	    this.CurrentRecord = this.OSMLRecord;
	}

    // make sure we bring this.LastRecord back down to the top of our replica...
    while (!this.replica[this.LastRecord] && this.LastRecord > 0)
	this.LastRecord--;

    // Did we get to the target record?
    if (this.LastRecord < this.TargetRecord[1])
	{ 
	if (rowcnt < this.querysize)
	    {
	    // didn't get a full fetch, but also did not find our record.  End query here.
	    this.EndQuery();
	    return 0;
	    }

	// Wow - how many records does the user want?  This check is
	// here as a failsafe catch point, to keep retrieval from going on
	// ad infinitum.
	if ((this.LastRecord % 500) < ((this.LastRecord - this.querysize) % 500))
	    {
	    if (!confirm("You have already retrieved " + this.LastRecord + " records.  Do you want to continue?"))
		{
		// pause here at the user's request.
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
	if ((this.LastRecord-this.FirstRecord+1) < this.replicasize && rowcnt >= this.querysize)
	    {
	    // Replica is not full and more data is likely available:
	    // Make sure we have a full replica if possible.
	    this.DoFetch(this.replicasize - (this.LastRecord - this.FirstRecord + 1), false);
	    }
	else
	    {
	    if (rowcnt < this.querysize)
		{
		// Replica is full and no more data is available
		this.EndQuery();
		}
	    else
		{
		// Replica is full but more data is likely available
		this.FoundRecord();
		}
	    }
	}
    }

function osrc_oldoid_cleanup()
    {
    if(this.oldoids && this.oldoids[0])
	{
	this.SetPending(true);
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
    var newprevcurrent = new $CX.Types.osrcObject();

    // first, build the list of fields we're working with.  We look both in the
    // replica and in prevcurrent, since field lists can be irregular (different
    // for different records), as well as can contain NULL values.
    var fieldlist = {};
    if (this.prevcurrent)
	{
	for(var f in this.prevcurrent)
	    if (f != '__cx_id' && f != '__cx_handle' && f != '__cx_filter')
		fieldlist[f] = true;
	}
    if (this.replica && this.replica[this.CurrentRecord])
	{
	for(var f in this.replica[this.CurrentRecord])
	    if (f != '__cx_id' && f != '__cx_handle' && f != '__cx_filter')
		fieldlist[f] = true;
	}

    // Determine old and new values for the fields.
    for(var attrname in fieldlist)
	{
	// Old value -- see this.prevcurrent
	var oldval = null;
	if (this.prevcurrent && typeof this.prevcurrent[attrname] == 'object')
	    {
	    oldval = this.prevcurrent[attrname].get();
	    }

	// New value -- see the replica.
	var newval = null;
	if (this.replica[this.CurrentRecord] && typeof this.replica[this.CurrentRecord][attrname] == 'object')
	    {
	    newval = this.replica[this.CurrentRecord][attrname].get();
	    }

	// Issue a Changing ifValue operation if the old and new are different.
	if (oldval != newval)
	    {
	    //	pg_explog.push('Changing: ' + oldval + ' to ' + newval);
	    this.ifcProbe(ifValue).Changing(attrname, newval, true, oldval, true);
	    }

	// Only record the value in prevcurrent if the *new* value is not null.
	if (newval !== null)
	    {
	    var a = new $CX.Types.osrcAttr();
	    a.v = newval;
	    newprevcurrent[attrname] = a;
	    }
	}

    this.prevcurrent = newprevcurrent;
    }

function osrc_give_one_current_record(id, why)
    {
    this.child[id].ObjectAvailable(this.replica[this.CurrentRecord], this, (why=='create')?'create':(this.doing_refresh?'refresh':why));
    }

function osrc_give_all_current_record(why)
    {
    this.ChangeCurrentRecord();
    if (this.LastRecord >= this.FirstRecord && this.replica[this.LastRecord] && this.replica[this.LastRecord].__osrc_is_last)
	{
	this.replica[this.CurrentRecord].__osrc_last_record = this.LastRecord;
	this.FinalRecord = this.LastRecord;
	}
    for(var i in this.child)
	this.GiveOneCurrentRecord(i, why);
    this.ifcProbe(ifEvent).Activate("DataFocusChanged", {});
    this.doing_refresh = false;
    }

function osrc_tell_all_replica_moved()
    {
    for(var i in this.child)
	if(this.child[i].ReplicaMoved)
	    this.child[i].ReplicaMoved(this);
    }


function osrc_move_to_record(recnum, source)
    {
    var from_internal = (source === true);
    this.initiating_client = (source !== false && source !== true)?source:null;
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
	return 0;
	}
    if(this.pending)
	{
	return 0;
	}
    this.SetPending(true);
    this.RecordToMoveTo=recnum;
    if (!from_internal)
	this.SyncID = osrc_syncid++;
    this.GoNogo(osrc_cb_query_continue_2, osrc_cb_query_cancel_2, null);
    }

function osrc_move_to_record_cb(recnum)
    {
    this.moveop=true;
    if(recnum<1)
	{
	return 0;
	}
    this.RecordToMoveTo=recnum;
    for(var i in this.child)
	 {
	 if(this.child[i].IsUnsaved)
	     {
	     return 0;
	     }
	 }
    this.TargetRecord = [recnum, recnum];
    this.CurrentRecord = recnum;
    if(this.CurrentRecord <= this.LastRecord && this.CurrentRecord >= this.FirstRecord)
	{
	this.GiveAllCurrentRecord('change');
	this.SetPending(false);
	return 1;
	}
    else
	{
	if(this.CurrentRecord < this.FirstRecord)
	    {
	    // data is further back, need new query
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
	    {
	    // data is farther on, act normal
	    if(this.qid)
		{
		if(this.CurrentRecord == Number.MAX_VALUE)
		    {
		    // rowcount defaults to a really high number if not set
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
		this.CurrentRecord=this.LastRecord;
		this.GiveAllCurrentRecord('change');
		this.SetPending(false);
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

function osrc_get_qid_startat(data)
    {
    // Check for handles
    if (data)
	{
	// Get session handle
	if (!this.sid && data.session)
	    this.sid = data.session;

	// Get query handle
	if (data.queryclosed)
	    this.qid = null;
	else
	    this.qid = data.query;
	}

    // No query performed?
    if (!data || !data.query)
	{
	this.startat = null;
	this.GiveAllCurrentRecord('get_qid');
	this.SetPending(false);
	return;
	}

    this.OSMLRecord=(this.startat)?(this.startat-1):0;

    // Do we have result set objects?
    if (data.resultset.length > 0)
	{
	// did an autofetch - we have the data already
	this.query_delay = pg_timestamp() - this.request_start_ts;
	this.FetchNext(data);
	}
    else
	{
	// Did not do autofetch -- grab the records.
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
    var query = new $CX.Types.osrcObject();
    query.setJoin('AND');
    var parentobj = this.parentosrc.getObject();
    var force_empty = false;
    for(var i=1;i<10;i++)
	{
	this.ParentKey[i]=param['ParentKey'+i];
	this.ChildKey[i]=param['ChildKey'+i];
	if(this.ParentKey[i])
	    {
	    var parentcol = null;
	    if (parentobj)
		parentcol = parentobj.getAttr(this.ParentKey[i]);
	    if (!parentobj || !parentcol)
		{
		// No current record, or if so, it has no attribute.
		// Type doesn't matter if it is null.
		var col = query.newAttr(this.ChildKey[i], null, 'integer');
		var filt = col.getFilter();
		filt.plainsearch = true;
		if (on_norecs == 'nullisvalue')
		    filt.nullisvalue = true;
		else if (on_norecs == 'norecs')
		    force_empty = filt.force_empty = true;
		else
		    filt.nullisvalue = false;
		}
	    else
		{
		// Current record with a valid attribute.
		var col = query.newAttr(this.ChildKey[i], parentcol);
		var filt = col.getFilter();
		filt.plainsearch = true;
		if (col.get() === null)
		    {
		    if (on_null == 'nullisvalue')
			filt.nullisvalue = true;
		    else if (on_null == 'norecs')
			force_empty = filt.force_empty = true;
		    else
			filt.nullisvalue = false;
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

    // Did it change from last time?  Get a merged list of the current and previous
    // properties, and compare the values to see if anything is different.
    if (!this.lastSync)
	this.lastSync = new $CX.Types.osrcObject();
    var changed = false;
    var al1 = this.lastSync.getAttrList();
    var al2 = query.getAttrList();
    Object.assign(al1, al2); // merge al2 into al1
    for(var a in al1)
	{
	var attr1 = this.lastSync.getAttr(a);
	var attr2 = query.getAttr(a);
	if ((attr1 && !attr2) || (!attr1 && attr2))
	    {
	    // Property exists in only one place
	    changed = true;
	    }
	else if (attr1 && attr2)
	    {
	    if (attr1.getType() != attr2.getType() || attr1.get() != attr2.get())
		{
		// Property types or values are different.
		changed = true;
		}
	    }
	}

    // Save a copy of the current criteria
    this.lastSync = new $CX.Types.osrcObject(query);

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
    var query = new $CX.Types.osrcObject();
    query.setJoin('AND');
    var parentobj = this.parentosrc.getObject();
    for(var i=1;i<10;i++)
	{
	this.ParentKey[i] = param['ParentKey' + i];
	this.ParentSelfKey[i] = param['ParentSelfKey' + i];
	this.SelfChildKey[i] = param['SelfChildKey' + i];
	this.ChildKey[i] = param['ChildKey' + i];
	if(this.ParentKey[i])
	    {
	    var parentattr = null;
	    if (parentobj)
		parentattr = parentobj.getAttr(this.ParentKey[i]);
	    if (parentobj && parentattr)
		{
		query.newAttr(this.ParentSelfKey[i], parentattr);
		}
	    }
	}
    this.ifcProbe(ifAction).Invoke("QueryObject", {query:query, client:null, ro:this.readonly, fromsync:true});
    }

function osrc_action_double_sync_cb()
    {
    var query = new $CX.Types.osrcObject();
    query.setJoin('OR');
    for(var p=this.FirstRecord; p<=this.LastRecord; p++)
	{
	var q = new $CX.Types.osrcObject();
	q.setJoin('AND');
	for(var i=1;i<10;i++)
	    {
	    if(this.SelfChildKey[i])
		{
		var obj = this.getObject(p);
		var attr = null;
		if (obj)    
		    attr = obj.getAttr(this.SelfChildKey[i]);
		if (obj && attr)
		    {
		    q.newAttr(this.ChildKey[i], attr);
		    }
		}
	    }
	query.newAttr('' + p, q, 'criteria');
	}
    
    this.doublesync=false;
    this.childosrc.ifcProbe(ifAction).Invoke("QueryObject", {query:query, client:null, ro:this.readonly});
    }


// for each row in the replica, call an action on another widget
function osrc_action_for_each(aparam)
    {
    // Find target of the foreach operation
    var foreach_target = wgtrGetNode(this, aparam.ForEachTarget);
    var foreach_action = aparam.ForEachAction;
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
    for(var i in osrc.replica)
	{
	var obj = osrc.getObject(i);
	var attr = obj.getAttr(osrc.do_sql_field);
	if (attr)
	    {
	    var attrval = attr.get();
	    if (typeof osrc.do_sql_used_values[attrval] != 'undefined')
		{
		grp_value = attrval;
		osrc.do_sql_used_values[grp_value] = true;
		}
	    }
	if ((grp_value !== null && grp_value == attrval) || typeof osrc.do_sql_field == 'undefined')
	    {
	    var attrlist = obj.getAttrList();
	    for(var a in attrlist)
		{
		var attr = obj.getAttr(a);
		var attrvalue = attr.get();
		if (!found_first)
		    common_values[a] = attrvalue;
		else if (common_values[a] != attrvalue)
		    delete common_values[a];
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
    this.ApplyRelationships(obj, false, false);
    this.ApplyKeys(obj);
    this.ApplySequence(obj);

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
	    this.child[i].ObjectAvailable(new $CX.Types.osrcObject(), this, 'begincreate');

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


function osrc_seq(direction)
    {
    // Find the sequence field
    var seqfield = null;
    this.rulelist.forEach(function(rule)
	{
	if (rule.ruletype == 'osrc_sequence')
	    seqfield = rule.field;
	});
    if (!seqfield)
	return;

    // Get the current sequence ID
    var cur_seq = this.GetValue(seqfield);
    if (!cur_seq)
	cur_seq = 0;

    // Find the record just before or after it in the sequence order.
    var previtem = null;
    var prevseq = null;
    this.replica.forEach(function(item, idx)
	{
	var attr = item.getAttr(seqfield);
	if (attr)
	    {
	    var attrval = parseInt(attr.get());
	    if (((direction == 'backward' && attrval < cur_seq) || (direction == 'forward' && attrval > cur_seq)) &&
		(prevseq === null || ((direction == 'backward' && attrval > prevseq) || (direction == 'forward' && attrval < prevseq))))
		{
		previtem = idx;
		prevseq = attrval;
		}
	    };
	});

    // Didn't find anything?  Nothing to do then.
    if (previtem === null)
	return;

    // Ok, we get to switch the sequence numbers of the current and previous item.
    var doneprev = donecurr = false;
    (function seqproc()
	{
	// Step 1: make sure query is closed.
	if (this.qid)
	    {
	    this.DoRequest('queryclose', '/', {ls__qid:this.qid}, seqproc);
	    this.qid=null;
	    return;
	    }

	// Step 2: update prior record's sequence
	var reqparam = {ls__reopen_sql:this.getSQL(), ls__sqlparam:this.EncodeParams()};
	if (!doneprev)
	    {
	    reqparam[seqfield] = cur_seq;
	    reqparam.ls__oid = this.replica[previtem].getID();
	    this.SetValue(seqfield, cur_seq, previtem);
	    this.DoRequest('setattrs', '/', reqparam, seqproc);
	    doneprev = true;
	    return;
	    }

	// Step 3: update current record's sequence
	if (!donecurr)
	    {
	    reqparam[seqfield] = prevseq;
	    reqparam.ls__oid = this.replica[this.CurrentRecord].getID();
	    this.SetValue(seqfield, prevseq);
	    this.DoRequest('setattrs', '/', reqparam, seqproc);
	    donecurr = true;
	    return;
	    }

	// Step 4: swap the replica entries and notify everyone.
	var tmpobj = this.replica[this.CurrentRecord];
	this.replica[this.CurrentRecord] = this.replica[previtem];
	this.replica[previtem] = tmpobj;
	this.CurrentRecord = previtem;
	this.GiveAllCurrentRecord('modify');
	this.ifcProbe(ifEvent).Activate("Sequenced", {});
	}).apply(this);
    }


// Move the current row backwards using an osrc_sequence rule
function osrc_seq_backward(aparam)
    {
    this.Sequence('backward');
    }


// Move the current row forwards using an osrc_sequence rule
function osrc_seq_forward(aparam)
    {
    this.Sequence('forward');
    }


function osrc_apply_sequence(obj)
    {
    for(var i in this.rulelist)
	{
	var rl = this.rulelist[i];
	if (rl.ruletype == 'osrc_sequence')
	    {
	    // Search for the sequence maximum in the replica.
	    var maxval = -1;
	    this.replica.forEach(function(item)
		{
		if (typeof item[rl.field] == 'object')
		    {
		    var ckval = parseInt(item.getAttrValue(rl.field));
		    if (ckval > maxval)
			maxval = ckval;
		    }
		});

	    // got maximum value in the osrc's replica.  Assign it now.
	    if (typeof obj[rl.field] == 'object')
		obj[rl.field].v = '' + (maxval + 1);
	    else
		{
		obj[rl.field] = new $CX.Types.osrcAttr();
		obj[rl.field].h = '';
		obj[rl.field].y = false;
		obj[rl.field].t = 'integer';
		obj[rl.field].v = '' + (maxval + 1);
		//obj.push({hints:"", oid:rl.field, system:false, type:"integer", value:'' + (maxval + 1)});
		}
	    }
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

function osrc_apply_rel(obj, in_create, in_modify)
    {
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
		var keyname = rule.key[k];
		var col = obj.getAttr(keyname);
		var tcol;
		var filt;
		if (col)
		    found = true;

		// If not already in the obj, we add it unless we're modifying
		// an already existing object, in which case tagging relationship
		// data causes unnecessary fields to be set.
		// 
		// GRB: merge conflict on this - I believe this is no longer an
		// issue with the updated code, since attr not found in the below
		// getAttr() does not do anything anyhow.
		//
		//if (!found)
		//    {
		//    if (in_modify)
		//	continue;
		//    }

		// Are we allowed to set/overwrite this value?
		if (!found || !in_create || rule.enforce_create)
		    {
		    // Find the matching column in the master osrc replica
		    if (tgt.CurrentRecord && tgt.replica && tgt.replica[tgt.CurrentRecord])
			{
			if ((tcol = tgt.replica[tgt.CurrentRecord].getAttr(rule.tkey[k])) != null)
			    {
			    // Found it -- set/overwrite the attribute
			    col = obj.newAttr(keyname, tcol);
			    filt = col.getFilter();
			    filt.obj = rule.obj;
			    }
			}
		    else
			{
			// Master has no records.  Follow master_norecs_action.
			switch (rule.master_norecs_action)
			    {
			    case 'norecs':
				col = obj.newAttr(keyname, null);
				filt = col.getFilter();
				filt.obj = rule.obj;
				filt.force_empty = true;
				break;
			    case 'allrecs':
				break;
			    case 'sameasnull':
				col = obj.newAttr(keyname, null);
				filt = col.getFilter();
				filt.obj = rule.obj;
				break;
			    }
			}

		    // Object fixups
		    if ((col = obj.getAttr(keyname)) != null)
			{
			var filt = col.getFilter();

			// Force plain search - no wildcards, etc.
			filt.plainsearch=true;

			// Type not supplied?  Check our cached type list if so.
			if (typeof col.getType() == "undefined" && this.type_list[keyname])
			    col.setType(this.type_list[keyname]);

			// Null value?
			if (col.get() === null)
			    {
			    switch(rule.master_null_action)
				{
				case 'nullisvalue':
				    filt.nullisvalue = true;
				    break;
				case 'allrecs':
				    if (!found)
					obj.removeAttr(keyname);
				    break;
				case 'norecs':
				    filt.nullisvalue = false;
				    filt.force_empty = true;
				    break;
				}
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

function osrc_set_value(n, v, recno)
    {
    var eval_rec = recno?recno:(this.EvalRecord?this.EvalRecord:this.CurrentRecord);
    var rec = this.getObject(eval_rec);
    //if (eval_rec && this.replica && this.replica[eval_rec])
    if (rec)
	{
	var attr = rec.getAttr(n);
	if (attr)
	    {
	    oldval = attr.get();
	    attr.set(v);
	    if (eval_rec == this.CurrentRecord || eval_rec === null)
		this.ifcProbe(ifValue).Changing(n, attr.get(), true, oldval, true);
	    }
	/*this.replica[eval_rec].forEach(function(field)
	    {
	    if (field.oid == n)
		{
		var oldval = field.value;
		if (v === null)
		    field.value = v;
		else
		    field.value = '' + v;
		if (eval_rec == this.CurrentRecord)
		    this.ifcProbe(ifValue).Changing(n, field.value, true, oldval, true);
		}
	    });*/
	}
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
    if (eval_rec && this.replica)
	{
	var eval_obj = this.getObject(eval_rec);
	var eval_attr = null;
	if (eval_obj)
	    eval_attr = eval_obj.getAttr(n);
	if (eval_obj && eval_attr)
	    {
	    v = eval_attr.get();
	    if (eval_attr.getType() == 'integer')
		v = parseInt(v);
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
    if (rl.ruletype == 'osrc_sequence')
	{
	rl.field = rule_widget.fieldname;
	this.rulelist.push(rl);
	}
    else if (rl.ruletype == 'osrc_filter')
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


function osrc_compare_requests(r1, r2)
    {
    return JSON.stringify(r1) === JSON.stringify(r2);
    }


function osrc_dispatch()
    {
    if (this.pending || this.masters_pending.length) return;
    var req = null;
    var requeue = [];
    while ((req = this.query_request_queue.shift()) != null)
	{
	// Peek to see if the next request(s) are identical
	while (this.query_request_queue.length > 0)
	    {
	    var req2 = this.query_request_queue[0];
	    if (this.CompareRequests(req, req2))
		this.query_request_queue.shift();
	    else
		break;
	    }
	
	// Process the request
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
    var act = ((this.ind_act && this.req_ind_act) || cmd == 'create' || cmd == 'setattrs') && cmd != 'close';
    if (!target) target = this;
    params.cx__akey = akey;
    params.ls__mode = 'osml';
    params.ls__req = cmd;
    if (this.sid) params.ls__sid = this.sid;
    if (((!this.ind_act || !this.req_ind_act) && cmd != 'create' && cmd != 'setattrs') || cmd == 'close')
	params.cx__noact = '1';
    //for(var p in params)
//	{
//	url += (first?'?':'&') + htutil_escape(p) + '=' + htutil_escape(params[p]);
//	first = false;
//	}
    this.request_start_ts = pg_timestamp();

    if (act)
	pg_spinner_inc();

    $.ajax(
	{
	dataType: "text", // could be "json", but we'll parse it ourselves so we can revive it.
	url: url,
	data: params
	})
	.done( (data, stat, xhr) =>
	    {
	    if (act)
		pg_spinner_dec();
	    data = JSON.parse(data, (key, value) =>
		{
		// We revive prototypes for the json via duck-typing.
		if (typeof value === 'object')
		    {
		    // null
		    if (value == null)
			return value;
		    // attribute
		    if (typeof value.v !== 'undefined' && value.t && typeof value.__cx_handle === 'undefined')
			value.__proto__ = $CX.Types.osrcAttr.prototype;
		    // object (i.e., row or tuple)
		    else if (value.__cx_handle)
			value.__proto__ = $CX.Types.osrcObject.prototype;
		    // money
		    else if (typeof value.wholepart !== 'undefined' && typeof value.fractionpart !== 'undefined')
			value.__proto__ = $CX.Types.Money.prototype;
		    // date/time
		    else if (typeof value.year !== 'undefined' && typeof value.month !== 'undefined')
			{
			value.__proto__ = $CX.Types.DateTime.prototype;
			value.adjFromServer();
			}
		    }
		return value;
		});
	    cb.call(this, data);
	    })
	.fail( (xhr, stat, err) =>
	    {
	    if (act)
		pg_spinner_dec();
	    });
    //pg_serialized_load(target, url, cb, !this.ind_act || !this.req_ind_act);
    this.req_ind_act = true;
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


function osrc_api_get_object(id)
    {
    if (id == null || id == undefined)
	id = this.osrcCurrentObjectID;
    if (!id || !this.replica[id])
	return null;
    return this.replica[id];
    }


function osrc_get_sql()
    {
    var newsql = wgtrGetServerProperty(this, "sql", this.sql);
    if (this.origbaseobj && this.baseobj && this.origbaseobj != this.baseobj)
	{
	var l = (new String(this.origbaseobj)).length;
	var newl = (new String(this.baseobj)).length;
	var s = new String(newsql);
	var p = s.indexOf(this.origbaseobj);
	while (p >= 0)
	    {
	    s = s.substr(0,p) + this.baseobj + s.substr(p+l);
	    p = s.indexOf(this.origbaseobj, p+newl);
	    }
	return s;
	}
    else
	return newsql;
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
    loader.getSQL = osrc_get_sql;
    loader.filter=param.filter;
    loader.baseobj=param.baseobj;
    loader.origbaseobj=param.baseobj;
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
    loader.ApplySequence = osrc_apply_sequence;
    loader.Sequence = osrc_seq;
    loader.EndQuery = osrc_end_query;
    loader.FoundRecord = osrc_found_record;
    loader.DoFetch = osrc_do_fetch;
    loader.FetchNext = osrc_fetch_next;
    loader.GoNogo = osrc_go_nogo;
    loader.QueueRequest = osrc_queue_request;
    loader.CompareRequests = osrc_compare_requests;
    loader.Dispatch = osrc_dispatch;
    loader.DoRequest = osrc_do_request;
    loader.EncodeParams = osrc_encode_params;
    loader.Encode = osrc_encode;
    loader.GiveAllCurrentRecord=osrc_give_all_current_record;
    loader.GiveOneCurrentRecord=osrc_give_one_current_record;
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

    // Replica access API
    loader.getObject = osrc_api_get_object;
    Object.defineProperty(loader, 'osrcFirstObjectID',
	{
	get: function() { return this.FirstRecord; }
	});
    Object.defineProperty(loader, 'osrcLastObjectID',
	{
	get: function() { return this.LastRecord; }
	});
    Object.defineProperty(loader, 'osrcCurrentObjectID',
	{
	get: function() { return this.CurrentRecord; }
	});
    Object.defineProperty(loader, 'osrcFinalObjectID',
	{
	get: function() { return this.FinalRecord; }
	});
   
    // Zero out the replica
    loader.ClearReplica();

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
    ia.Add("SeqBackward", osrc_seq_backward);
    ia.Add("SeqForward", osrc_seq_forward);
    ia.Add("ForEach", osrc_action_for_each);

    // Events
    var ie = loader.ifcProbeAdd(ifEvent);
    ie.Add("DataFocusChanged");
    ie.Add("EndQuery");
    ie.Add("Results");
    ie.Add("BeginQuery");
    ie.Add("Created");
    ie.Add("Modified");
    ie.Add("Sequenced");

    // Data Values
    var iv = loader.ifcProbeAdd(ifValue);
    iv.SetNonexistentCallback(osrc_get_value);
    iv.Add("cx__pending", osrc_get_pending, null);
    iv.Add("cx__current_id", "CurrentRecord", null);
    iv.Add("cx__last_id", "LastRecord", null);
    iv.Add("cx__first_id", "FirstRecord", null);
    iv.Add("cx__final_id", "FinalRecord", null);

    loader.GetValue = osrc_get_value;
    loader.SetValue = osrc_set_value;

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

    /*if (loader.autoquery == loader.AQonLoad) 
	pg_addsched_fn(loader,'InitQuery', [], 0);*/

    // Finish initialization...
    pg_addsched_fn(loader, "InitBH", [], 0);

    return loader;
    }


// Load indication
if (window.pg_scripts) pg_scripts['htdrv_osrc.js'] = true;
