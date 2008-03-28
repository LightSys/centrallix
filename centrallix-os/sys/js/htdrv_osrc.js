// Copyright (C) 1998-2008 LightSys Technology Services, Inc.
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
    this.ifcProbe(ifAction).Invoke("QueryObject", {qy:null, client:null, ro:this.readonly});
    }


function osrc_action_order_object(aparam) //order)
    {
    this.pendingorderobject=aparam.orderobj;
    this.ifcProbe(ifAction).Invoke("QueryObject", {query:this.queryobject, client:null, ro:this.readonly});
    }


function osrc_action_query_param(aparam)
    {
    var qo = [];
    var t;
    for (var i in aparam)
	{
	if (i == '_Origin') continue;
	if (typeof aparam[i] == 'string')
	    t = 'string';
	else if (typeof aparam[i] == 'number')
	    t = 'integer';
	else
	    t = 'string';
	qo.push({oid:i, value:aparam[i], type:t});
	}
    qo.joinstring = 'AND';
    this.ifcProbe(ifAction).Invoke("QueryObject", {query:qo, client:null, ro:this.readonly});
    }


function osrc_action_query_object(aparam) //q, formobj, readonly)
    {
    this.QueueRequest({Request:'QueryObject', Param:aparam});
    this.Dispatch();
    }

function osrc_query_object_handler(aparam)
    {
    var q = aparam.query;
    var formobj = aparam.client;
    var readonly = aparam.ro;

    if(this.pending)
	{
	alert('There is already a query or movement in progress...');
	return 0;
	}

    if (q) this.ApplyRelationships(q);

    if (!aparam.fromsync)
	this.SyncID = osrc_syncid++;

    this.pendingqueryobject=q;
    var statement=this.sql;
    //if (statement.indexOf(' WHERE ') != -1)
//	var sep = ' HAVING ';
  //  else
	var sep = ' WHERE ';
    var firstone=true;
    
    if(this.filter)
	{
	statement+= (sep + '('+this.filter+')');
	firstone=false;
	}
    //if(!confirm(osrc_make_filter(q)))
    //    return 0;
    if(q && q.joinstring && q[0])
	{
	if(firstone)
	    statement+=sep;
	else
	    statement+=' '+q.joinstring+' ';
	statement+=osrc_make_filter(q);
	}
    
    firstone=true;
    if(this.pendingorderobject)
	for(var i in this.pendingorderobject)
	    {
	    //alert('asdf'+this.pendingorderobject[i]);
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
    if (!readonly)
	statement += ' FOR UPDATE'
    //statement+=';';
    this.ifcProbe(ifAction).Invoke("Query", {query:statement, client:formobj});
    }


function osrc_make_filter_integer(col,val)
    {
    if (val == null)
	return ':"' + col.oid + '" is null ';
    else if (typeof val != 'number' && val.search(/-/)>=0)
	{
	var parts = val.split(/-/);
	return '(:"' + col.oid + '">=' + parts[0] + ' AND :"' + col.oid + '"<=' + parts[1] + ')';
	}
    else
	return ':"' + col.oid + '" = ' + val;
    }
    

function osrc_make_filter(q)
    {
    var firstone=true;
    var statement='';
    for(var i in q)
	{
	if(i!='oid' && i!='joinstring')
	    {
	    var str;
	    if(q[i].joinstring)
		{
		str=osrc_make_filter(q[i]);
		}
	    else
		{
		var val=q[i].value;
		if (val == null) continue;

		switch(q[i].type)
		    {
		    case 'integer':
			str = osrc_make_filter_integer(q[i], val);
			break;

		    case 'integerarray':
			if (val == null)
			    str = ':"' + q[i].oid + '" is null ';
			else if (val.length)
			    {
			    str = "(";
			    for(var j=0;j<val.length;j++)
				{
				if (j) str += " OR ";
				str += "(";
				str += osrc_make_filter_integer(q[i], val[j]);
				str += ")";
				}
			    str += ")";
			    }
			break;
		    case 'stringarray':
			if (val == null)
			    {
			    str=':"'+q[i].oid+'" is null ';
			    }
			else
			    {
			    if (val[0].search(/^\*.+\*$/)>=0)
				{
				str='(charindex("'+val[0].substring(1,val[0].length-1)+'",:'+q[i].oid+')>0';
				}
			    else if(val[0].search(/^\*/)>=0) //* at beginning
				{
				val[0] = val[0].substring(1); //pop off *
				str='(right(:'+q[i].oid+','+val[0].length+')="'+val[0]+'"';
				}
			    else if(val[0].search(/\*$/)>=0) //* at end
				{
				val[0]=val[0].substring(0,val[0].length-1); //chop off *
				str='(substring(:'+q[i].oid+','+1+','+val[0].length+')="'+val[0]+'"';
				}
			    else if(val[0].indexOf('*')>=0) //* in middle
				{
				var ind = val[0].indexOf('*');
				var val1 = val[0].substring(0,ind);
				var val2 = val[0].substring(ind+1);
				str='((right(:'+q[i].oid+','+val2.length+')="'+val2+'"';
				str+=' AND ';
				str+='substring(:'+q[i].oid+',1,'+val1.length+')="'+val1+'")';
				}
			    else
				str='(:'+q[i].oid+'='+'"'+val[0]+'"';

			    for(var j=1;j<val.length;j++)
				{
				if (val[j].search(/^\*.+\*$/)>=0)
				    {
				    str+=' OR charindex("'+val[j].substring(1,val[j].length-1)+'",:'+q[i].oid+')>0';
				    }
				else if(val[j].search(/^\*/)>=0) //* at beginning
				    {
				    val[j] = val[j].substring(1); //pop off *
				    str+=' OR right(:'+q[i].oid+','+val[j].length+')="'+val[j]+'"';
				    }
				else if(val[j].search(/\*$/)>=0) //* at end
				    {
				    val[j]=val[j].substring(0,val[j].length-1); //chop off *
				    str+=' OR substring(:'+q[i].oid+','+1+','+val[j].length+')="'+val[j]+'"';
				    }
				else if(val[j].indexOf('*')>=0) //* in middle
				    {
				    var ind = val[j].indexOf('*');
				    var val1 = val[j].substring(0,ind);
				    var val2 = val[j].substring(ind+1);
				    str+=' OR (right(:'+q[i].oid+','+val2.length+')="'+val2+'"';
				    str+=' AND ';
				    str+='substring(:'+q[i].oid+',1,'+val1.length+')="'+val1+'")';
				    }
				else
				    str+=' OR :'+q[i].oid+'='+'"'+val[j]+'"';
				}
			    str+=')';
			    }
			break;
		    case 'datetimearray':
			str='(:'+q[i].oid;
			var dtfirst=true;
			for(var j in val)
			    {
			    if(!dtfirst) str+= ' AND :"' + q[i].oid + '"';
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
		    default:
			//htr_alert(val, 1);
			if(typeof val.substring == 'undefined') // assume integer
			    str = osrc_make_filter_integer(q[i], val);
			else if(val.substring(0,2)=='>=')
			    str=':"'+q[i].oid+'" >= '+val.substring(2);
			else if(val.substring(0,2)=='<=')
			    str=':"'+q[i].oid+'" <= '+val.substring(2);
			else if(val.substring(0,2)=='=>')
			    str=':"'+q[i].oid+'" >= '+val.substring(2);
			else if(val.substring(0,2)=='=<')
			    str=':"'+q[i].oid+'" <= '+val.substring(2);
			else if(val.substring(0,1)=='>')
			    str=':"'+q[i].oid+'" > '+val.substring(1);
			else if(val.substring(0,1)=='<')
			    str=':"'+q[i].oid+'" < '+val.substring(1);
			else if(val.indexOf('-')>=0)
			    {
			    //assume integer range in string
			    var ind = val.indexOf('-');
			    var val1 = val.substring(0,ind);
			    var val2 = val.substring(ind+1);
			    str='(:"'+q[i].oid+'">='+val1+' AND :"'+q[i].oid+'"<='+val2+')';
			    }
			else
			    {
			    if (val == null)
				str=':"'+q[i].oid+'" is null ';
			    else
				if (val.search(/^\*.+\*$/)>=0)
				    {
				    str='charindex("'+val.substring(1,val.length-1)+'",:"'+q[i].oid+'")>0';
				    }
				else if(val.search(/^\*/)>=0) //* at beginning
				    {
				    val = val.substring(1); //pop off *
				    str='right(:"'+q[i].oid+'",'+val.length+')="'+val+'"';
				    }
				else if(val.search(/\*$/)>=0) //* at end
				    {
				    val=val.substring(0,val.length-1); //chop off *
				    str='substring(:"'+q[i].oid+'",'+1+','+val.length+')="'+val+'"';
				    }
				else if(val.indexOf('*')>=0) //* in middle
				    {
				    var ind = val.indexOf('*');
				    var val1 = val.substring(0,ind);
				    var val2 = val.substring(ind+1);
				    str='(right(:"'+q[i].oid+'",'+val2.length+')="'+val2+'"';
				    str+=' AND ';
				    str+='substring(:"'+q[i].oid+'",1,'+val1.length+')="'+val1+'")';
				    }
				else
				    str=':"'+q[i].oid+'"='+'"'+val+'"';
			    }
			break;
		    }
		}
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


function osrc_go_nogo(go_func, nogo_func)
    {
    this._unsaved_cnt = 0;
    this._go_nogo_pending = true;
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
	    if (this.child[i].IsDiscardReady() == true)
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
	this._go_func();
	}
    }


function osrc_action_query(aparam) //q, formobj)
    {
    this.QueueRequest({Request:'Query', Param:aparam});
    this.Dispatch();
    }

function osrc_query_handler(aparam)
    {
    var q = aparam.query;
    var formobj = aparam.client;

    //Is discard ready
    //if(!formobj.IsDiscardReady()) return 0;
    //Send query as GET request
    //this.formobj = formobj;
    if(this.pending)
	{
	alert('There is already a query or movement in progress...');
	return 0;
	}
    this.easyquery=q;
    this.pendingquery=htutil_escape(q);
    this.pending=true;
    //var someunsaved=false;
    // Check if any children are modified and call IsDiscardReady if they are
    this.GoNogo(osrc_cb_query_continue_2, osrc_cb_query_cancel_2);
    /*for(var i in this.child)
	 {
	 if(this.child[i].IsUnsaved)
	     {
	     this.child[i]._osrc_ready=false;
	     this.child[i].IsDiscardReady();
	     someunsaved=true;
	     }
	 else
	     {
	     this.child[i]._osrc_ready=true;
	     }
	 }
    //if someunsaved is false, there were no unsaved forms, so no callbacks
    //  so, we'll just fake one using the first form....
    if(someunsaved) return 0;
    this.QueryContinue(this.child[0]);*/
    }

function osrc_action_delete(aparam) //up,formobj)
    {
    var up = aparam.data;
    var formobj = aparam.client;

    //Delete an object through OSML
    var src = this.baseobj + '?cx__akey='+akey+'&ls__mode=osml&ls__req=delete&ls__sid=' + this.sid + '&ls__oid=' + up.oid;
    this.formobj = formobj;
    this.deleteddata=up;
    //this.onload = osrc_action_delete_cb;
    //pg_set(this,'src',src);
    pg_serialized_load(this, src, osrc_action_delete_cb);
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
	    this.LastRecord--;
	    if (this.OSMLRecord > 0) this.OSMLRecord--;

	    // Notify osrc clients (forms/tables/etc)
	    for(var i in this.child)
		this.child[i].ObjectDeleted(recnum);

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
	this.formobj.OperationComplete(true);
	}
    else
	{
	// delete failed
	this.formobj.OperationComplete(false);
	}
    this.formobj=null;
    delete this.deleteddata;
    return 0;
    }

function osrc_action_create(aparam) //up,formobj)
    {
    var up = aparam.data;
    var formobj = aparam.client;

    this.formobj=formobj;
    this.createddata=up;
    //First close the currently open query
    if(this.qid)
	{
	pg_serialized_load(this,"/?cx__akey="+akey+"&ls__mode=osml&ls__req=queryclose&ls__sid="+this.sid+"&ls__qid="+this.qid, osrc_action_create_cb2);
	this.qid=null;
	return 0;
	}
    else if (!this.sid)
	{
	this.replica=new Array();
	this.LastRecord=0;
	this.FirstRecord=1;
	this.CurrentRecord=1;
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
    var src = this.baseobj + '/*?cx__akey='+akey+'&ls__mode=osml&ls__req=create&ls__reopen_sql=' + htutil_escape(this.sql) + '&ls__sid=' + this.sid;
    this.ApplyRelationships(this.createddata);
    //htr_alert(this.createddata, 2);
    for(var i in this.createddata) if(i!='oid')
	{
	if (this.createddata[i]['value'] == null)
	    src+='&'+htutil_escape(this.createddata[i]['oid'])+'=';
	else
	    src+='&'+htutil_escape(this.createddata[i]['oid'])+'='+htutil_escape(this.createddata[i]['value']);
	}
    pg_serialized_load(this, src, osrc_action_create_cb);
    }

function osrc_action_create_cb()
    {
    var links = pg_links(this);
    if(links && links[0] && links[0].target != 'ERR')
	{
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
	this.formobj.OperationComplete(true);
	for(var i in this.child)
	    this.child[i].ObjectCreated(recnum);
	this.GiveAllCurrentRecord();
	}
    else
	{
	this.formobj.OperationComplete(false);
	}
    this.formobj=null;
    delete this.createddata;
    }

function osrc_action_modify(aparam) //up,formobj)
    {
    if (aparam)
	{
	this.modifieddata = aparam.data;
	this.formobj = aparam.client;
	}

    // Need to close an open query first?
    if(this.qid)
	{
	pg_serialized_load(this,"/?cx__akey="+akey+"&ls__mode=osml&ls__req=queryclose&ls__sid="+this.sid+"&ls__qid="+this.qid, osrc_action_modify);
	this.qid=null;
	return 0;
	}
    //Modify an object through OSML
    //up[adsf][value];
    var src='/?cx__akey='+akey+'&ls__mode=osml&ls__req=setattrs&ls__sid=' + this.sid + '&ls__oid=' + this.modifieddata.oid;
    this.ApplyRelationships(this.modifieddata);
    for(var i in this.modifieddata) if(i!='oid')
	{
	src+='&'+htutil_escape(this.modifieddata[i]['oid'])+'='+htutil_escape(this.modifieddata[i]['value']);
	}
    pg_serialized_load(this, src, osrc_action_modify_cb);
    }

function osrc_action_modify_cb()
    {
    var links = pg_links(this);
    var success = links && links[0] && (links[0].target != 'ERR');
    if(success)
	{
	var recnum=this.CurrentRecord;
	var cr=this.replica[this.CurrentRecord];
	if(cr)
	    for(var i in this.modifieddata) // update replica
		for(var j in cr)
		    if(cr[j].oid==this.modifieddata[i].oid)
			cr[j].value=this.modifieddata[i].value;

	// Check new/corrected data provided by server
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
		    //alert(cr[j].value + " != " + server_rec[i].value);
		    }
		}
	
	this.formobj.OperationComplete(true);
	for(var i in this.child)
	    this.child[i].ObjectModified(recnum);
	if (diff)
	    this.GiveAllCurrentRecord();
	}
    else
	{
	this.formobj.OperationComplete(false);
	}
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
	    this._go_func();
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

	this.ClearReplica();
	this.moveop=true;

	this.OpenSession(this.OpenQuery);
	}
    else
	{ // movement
	this.MoveToRecordCB(this.RecordToMoveTo, true);
	this.RecordToMoveTo=null;
	}
    this.pending=false;
    this.Dispatch();
    }

function osrc_cb_query_cancel()
    {
    if (this._go_nogo_pending)
	{
	this._go_nogo_pending = false;
	this._nogo_func();
	}
    }

function osrc_cb_query_cancel_2()
    {
    this.pendingquery=null;
    this.pending=false;
    this.Dispatch();
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
    }

function osrc_cb_savable_changed(p,o,n)
    {
    var osrc = this.__osrc_osrc;
    if (o && !n)
	osrc.savable_client_count--;
    else if (!o && n)
	osrc.savable_client_count++;
    if (osrc.is_client_savable && osrc.savable_client_count == 0)
	osrc.is_client_savable = false;
    else if (!osrc.is_client_savable && osrc.savable_client_count > 0)
	osrc.is_client_savable = true;
    return n;
    }

function osrc_open_session(cb)
    {
    //Open Session
    //alert('open');
    if(this.sid)
	{
	this.__osrc_cb = cb;
	this.__osrc_cb();
	}
    else
	{
	pg_serialized_load(this, '/?cx__akey='+akey+'&ls__mode=osml&ls__req=opensession', cb);
	}
    }

function osrc_open_query()
    {
    //Open Query
    if(!this.sid)
	{
	var lnks = pg_links(this);
	if (!lnks || !lnks[0] || !lnks[0].target)
	    return false;
	this.sid=pg_links(this)[0].target;
	}
    if(this.qid)
	{
	pg_serialized_load(this,"/?cx__akey="+akey+"&ls__mode=osml&ls__req=queryclose&ls__sid="+this.sid+"&ls__qid="+this.qid, osrc_open_query);
	this.qid=null;
	return 0;
	}
    //pg_serialized_load(this,"/?cx__akey="+akey+"&ls__mode=osml&ls__req=multiquery&ls__sid="+this.sid+"&ls__sql=" + this.query, osrc_get_qid);
    pg_serialized_load(this,"/?cx__akey="+akey+"&ls__mode=osml&ls__req=multiquery&ls__sid="+this.sid+"&ls__autofetch=1&ls__objmode=0&ls__notify=" + this.request_updates + "&ls__rowcount=" + this.replicasize + "&ls__sql=" + this.query, osrc_get_qid);
    this.querysize = this.replicasize;
    }

function osrc_get_qid()
    {
    //return;
    var lnk = pg_links(this);
    if (lnk[0])
	this.qid=lnk[0].target;
    else
	this.qid = null;
    //confirm(this.baseobj + " ==> " + this.qid);
    if (!this.qid)
	{
	this.pending=false;
	this.GiveAllCurrentRecord();
	this.Dispatch();
	}
    else
	{
	for(var i in this.child)
	    this.child[i].DataAvailable();
	if (lnk.length > 1)
	    {
	    // did an autofetch - we have the data already
	    this.ClearReplica();
	    this.moveop = true;
	    this.FetchNext();
	    }
	else
	    {
	    // start the ball rolling for the fetch
	    this.ifcProbe(ifAction).Invoke("First", {from_internal:true});
	    }
	}
/** normally don't actually load the data...just let children know that the data is available **/
    }

function osrc_parse_one_attr(lnk)
    {
    var col = {type:lnk.hash.substr(1), oid:htutil_unpack(lnk.host), hints:lnk.search};
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
    var obj=new Array();
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
    this.replica=new Array();
    this.LastRecord=0;
    this.FirstRecord=1;
    }

function osrc_parse_one_row(lnk, i)
    {
    var row = new Array();
    var cnt = 0;
    var tgt = lnk[i].target;
    while(i < lnk.length && (lnk[i].target == tgt || lnk[i].target == 'R'))
	{
	row[cnt] = this.ParseOneAttr(lnk[i]);
	cnt++;
	i++;
	}
    return row;
    }

function osrc_do_fetch(rowcnt)
    {
    this.querysize = rowcnt?rowcnt:1;
    pg_serialized_load(this, "/?cx__akey="+akey+"&ls__mode=osml&ls__req=queryfetch&ls__sid="+this.sid+"&ls__qid="+this.qid+"&ls__objmode=0&ls__notify=" + this.request_updates + (rowcnt?("&ls__rowcount="+rowcnt):"") + (this.startat?("&ls__startat="+this.startat):""), osrc_fetch_next);
    }

function osrc_end_query()
    {
    //this.formobj.OperationComplete(); /* don't need this...I think....*/
    var qid=this.qid
    this.qid=null;
    /* return the last record as the current one if it was our target otherwise, don't */
    if (this.LastRecord >= this.FirstRecord && this.replica[this.LastRecord])
	this.replica[this.LastRecord].__osrc_is_last = true;
    /*if(this.moveop)
	{*/
	/*this.GiveAllCurrentRecord();
	}
    else
	{
	this.TellAllReplicaMoved();
	}
    this.pending=false;
    if(this.doublesync)
	this.DoubleSyncCB();*/
    this.FoundRecord();
    if(qid)
	{
	pg_serialized_load(this, "/?cx__akey="+akey+"&ls__mode=osml&ls__req=queryclose&ls__sid="+this.sid+"&ls__qid="+qid, osrc_close_query);
	}
    this.Dispatch();
    this.ifcProbe(ifEvent).Activate("EndQuery", {});
    return 0;
    }

function osrc_found_record()
    {
    if(this.CurrentRecord>this.LastRecord)
	this.CurrentRecord=this.LastRecord;
    if(this.doublesync)
	this.DoubleSyncCB();
    if(this.moveop)
	this.GiveAllCurrentRecord();
    else
	this.TellAllReplicaMoved();
    this.pending=false;
    this.osrc_oldoid_cleanup();
    }

function osrc_fetch_next()
    {
    pg_debug(this.id + ": FetchNext() ==> " + pg_links(this).length + "\n");
    //alert('fetching....');
    if(!this.qid)
	{
	if (pg_diag) confirm("ERR: " + this.baseobj + " ==> " + this.qid);
	//alert('something is wrong...');
	//alert(this.src);
	}
    var lnk=pg_links(this);
    var lc=lnk.length;
    //confirm(this.baseobj + " ==> " + lc + " links");
    if(lc < 2)
	{ // query over
	this.EndQuery();
	return 0;
	}
    var colnum=0;
    var i = 1;
    var rowcnt = 0;
    while (i < lc)
	{
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
	    }
	}
    pg_debug("   Fetch returned " + rowcnt + " rows, querysize was " + this.querysize + ".\n");

    // make sure we bring this.LastRecord back down to the top of our replica...
    while(!this.replica[this.LastRecord])
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
	this.DoFetch(this.readahead);
	}
    else
	{
	// we've got the one we need 
	if((this.LastRecord-this.FirstRecord+1)<this.replicasize && rowcnt >= this.querysize)
	    {
	    // make sure we have a full replica if possible
	    this.DoFetch(this.replicasize - (this.LastRecord - this.FirstRecord + 1));
	    }
	else
	    {
	    this.FoundRecord();
	    }
	}
    }

function osrc_oldoid_cleanup()
    {
    if(this.oldoids && this.oldoids[0])
	{
	this.pending=true;
	var src='';
	for(var i in this.oldoids)
	    src+=this.oldoids[i];
	if(this.sid)
	    //pg_set(this,'src','/?ls__mode=osml&ls__req=close&ls__sid='+this.sid+'&ls__oid=' + src);
	    pg_serialized_load(this, '/?cx__akey='+akey+'&ls__mode=osml&ls__req=close&ls__sid='+this.sid+'&ls__oid=' + src, osrc_oldoid_cleanup_cb);
	else
	    alert('session is invalid');
	}
    else
	this.Dispatch();
    }
 
function osrc_oldoid_cleanup_cb()
    {
    this.pending=false;
    //alert('cb recieved');
    delete this.oldoids;
    this.oldoids=new Array();
    this.Dispatch();
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
    pg_serialized_load(this, '/?cx__akey='+akey+'&ls__mode=osml&ls__req=close&ls__oid=' + this.oid, osrc_close_session);
    //this.onload = osrc_close_session;
    //pg_set(this,'src','/?ls__mode=osml&ls__req=close&ls__oid=' + this.oid);
    }
 
function osrc_close_session()
    {
    //Close Session
    pg_serialized_load(this, '/?cx__akey='+akey+'&ls__mode=osml&ls__req=closesession&ls__sid=' + this.sid, osrc_oldoid_cleanup);
    //this.onload=osrc_oldoid_cleanup;
    //pg_set(this,'src','/?ls__mode=osml&ls__req=closesession&ls__sid=' + this.sid);
    this.qid=null;
    this.sid=null;
    }


function osrc_move_first(aparam)
    {
    this.MoveToRecord(1, aparam.from_internal);
    }

function osrc_give_all_current_record()
    {
    //confirm('give_all_current_record start');
    for(var i in this.child)
	this.child[i].ObjectAvailable(this.replica[this.CurrentRecord]);
    this.ifcProbe(ifEvent).Activate("DataFocusChanged", {});
    //confirm('give_all_current_record done');
    }

function osrc_tell_all_replica_moved()
    {
    //confirm('tell_all_replica_moved start');
    for(var i in this.child)
	if(this.child[i].ReplicaMoved)
	    this.child[i].ReplicaMoved();
    //confirm('tell_all_replica_moved done');
    }


function osrc_move_to_record(recnum, from_internal)
    {
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
    this.pending=true;
    //var someunsaved=false;
    this.RecordToMoveTo=recnum;
    if (!from_internal)
	this.SyncID = osrc_syncid++;
    this.GoNogo(osrc_cb_query_continue_2, osrc_cb_query_cancel_2);
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
	this.GiveAllCurrentRecord();
	this.pending=false;
	this.Dispatch();
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
		pg_serialized_load(this,"/?cx__akey="+akey+"&ls__mode=osml&ls__req=queryclose&ls__sid="+this.sid+"&ls__qid="+this.qid,osrc_open_query_startat);
		//this.onload=osrc_open_query_startat;
		//pg_set(this,'src',"/?ls__mode=osml&ls__req=queryclose&ls__sid="+this.sid+"&ls__qid="+this.qid);
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
		    this.DoFetch(100);
		    }
		else if (recnum == 1)
		    {
		    // fill replica if empty
		    this.DoFetch(this.replicasize);
		    }
		else
		    {
		    this.DoFetch(this.readahead);
		    }
		}
	    else
		{
		this.pending=false;
		this.CurrentRecord=this.LastRecord;
		this.GiveAllCurrentRecord();
		this.Dispatch();
		}
	    return 0;
	    }
	}
    }

function osrc_open_query_startat()
    {
    if(this.FirstRecord - this.startat < this.replicasize)
	this.querysize = this.FirstRecord - this.startat;
    else
	this.querysize = this.replicasize;
    pg_serialized_load(this,"/?cx__akey="+akey+"&ls__mode=osml&ls__req=multiquery&ls__sid="+this.sid+"&ls__autofetch=1&ls__objmode=0&ls__notify=" + this.request_updates + "&ls__rowcount=" + this.querysize + "&ls__sql=" + this.query, osrc_get_qid_startat);
    //pg_serialized_load(this, "/?cx__akey="+akey+"&ls__mode=osml&ls__req=multiquery&ls__sid="+this.sid+"&ls__sql="+this.query, osrc_get_qid_startat);
    }

function osrc_get_qid_startat()
    {
    var lnk = pg_links(this);
    this.qid=lnk[0].target;
    if (!this.qid)
	{
	this.startat = null;
	this.pending=false;
	this.GiveAllCurrentRecord();
	this.Dispatch();
	return;
	}
    this.OSMLRecord=this.startat-1;
    //this.FirstRecord=this.startat;
    /*if(this.startat-this.TargetRecord+1<this.replicasize)
	{
	this.DoFetch(this.TargetRecord - this.startat + 1);
	}*/
    if (lnk.length > 1)
	{
	// did an autofetch - we have the data already
	this.FetchNext();
	}
    else
	{
	if(this.FirstRecord - this.startat < this.replicasize)
	    {
	    this.DoFetch(this.FirstRecord - this.startat);
	    }
	else
	    {
	    this.DoFetch(this.replicasize);
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
	this.TellAllReplicaMoved();
	this.pending=false;
	this.Dispatch();
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
		//pg_set(this,'src',"/?ls__mode=osml&ls__req=queryclose&ls__sid="+this.sid+"&ls__qid="+this.qid);
		pg_serialized_load(this,"/?cx__akey="+akey+"&ls__mode=osml&ls__req=queryclose&ls__sid="+this.sid+"&ls__qid="+this.qid, osrc_open_query_startat);
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
		    this.DoFetch(100);
		    }
		else
		    {
		    /* need to increase replica size to accomodate? */
		    this.DoFetch(this.scrollahead);
		    }
		}
	    else
		{
		this.pending=false;
		this.TargetRecord[1]=this.LastRecord;
		this.TellAllReplicaMoved();
		this.Dispatch();
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
	}
    }

function osrc_action_sync(param)
    {
    this.parentosrc=param.ParentOSRC;
    if (!this.parentosrc) 
	this.parentosrc = wgtrGetNode(this, param._Origin);
    this.ParentKey=new Array();
    this.ChildKey=new Array();

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
		t.oid = this.ChildKey[i];
		t.value = null;
		t.type = 'integer'; // type doesn't matter if it is null.
		query.push(t);
		}
	    else
		{
		for(var j in this.parentosrc.replica[p])
		    {
		    if(this.parentosrc.replica[p][j].oid==this.ParentKey[i])
			{
			var t = new Object();
			t.oid=this.ChildKey[i];
			t.value=this.parentosrc.replica[p][j].value;
			t.type=this.parentosrc.replica[p][j].type;
			query.push(t);
			}
		    }
		}
	    }
	}

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
    this.ParentKey=new Array();
    this.ParentSelfKey=new Array();
    this.SelfChildKey=new Array();
    this.ChildKey=new Array();
    var query = new Array();
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
    var query = new Array();
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
	var q = new Array();
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


/** called by child to get a template to build a new object for creation **/
function osrc_cb_new_object_template()
    {
    var obj = this.NewReplicaObj(0, 0);

    this.ApplyRelationships(obj);
    return obj;
    }

function osrc_apply_rel(obj)
    {
    var cnt = 0;
    while(typeof obj[cnt] != 'undefined') cnt++;

    // First, check for relationships that might imply key values
    for(var i in osrc_relationships)
	{
	var rl = osrc_relationships[i];
	if ((rl.osrc == this && rl.is_slave) || (rl.target_osrc == this && !rl.is_slave))
	    {
	    if (rl.osrc == this)
		{
		var tgt = rl.target_osrc;
		var srckey = 'key_';
		var tgtkey = 'target_key_';
		}
	    else
		{
		var tgt = rl.osrc;
		var srckey = 'target_key_';
		var tgtkey = 'key_';
		}
	    if (tgt.CurrentRecord && tgt.replica && tgt.replica[tgt.CurrentRecord])
		{
		for(var k=1;k<=5;k++)
		    {
		    if (!rl[tgtkey + k]) continue;
		    for(var j in tgt.replica[tgt.CurrentRecord])
			{
			var col = tgt.replica[tgt.CurrentRecord][j];
			if (col == null || typeof col != 'object') continue;
			if (col.oid == rl[tgtkey + k])
			    {
			    var found = false;
			    for(var l in obj)
				{
				if (obj[l] == null || typeof obj[l] != 'object') continue;
				if (obj[l].oid == rl[srckey + k])
				    {
				    found = true;
				    obj[l] = {type:col.type, value:col.value, hints:col.hints, oid:rl[srckey + k]};
				    }
				}
			    if (!found)
				obj[cnt++] = {type:col.type, value:col.value, hints:col.hints, oid:rl[srckey + k]};
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
    //alert(this.name + ' got Reveal');
    // Increment reveal counts
    this.revealed_children++;
    if (this.revealed_children > 1) return 0;

    // User requested onReveal?  If so, do that here.
    //if (!this.bh_finished) return 0;
    if ((this.autoquery == this.AQonFirstReveal || this.autoquery == this.AQonEachReveal) && !this.init)
	pg_addsched_fn(this,'InitQuery', [], 0);
    else if (this.autoquery == this.AQonEachReveal)
	{
	this.ifcProbe(ifAction).SchedInvoke('QueryObject', {query:null, client:null, ro:this.readonly}, 0);
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


function osrc_get_value(n)
    {
    var v = null;
    if (n == 'is_client_savable')
	return this.is_client_savable;
    if (this.CurrentRecord && this.replica && this.replica[this.CurrentRecord])
	{
	for(var i in this.replica[this.CurrentRecord])
	    {
	    var col = this.replica[this.CurrentRecord][i];
	    if (col.oid == n)
		return col.value;
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
	rule_widget._osrc_filter_changed = osrc_filter_changed;
	htr_watch(rule_widget, "value", "_osrc_filter_changed");
	this.rulelist.push(rl);
	}
    else if (rl.ruletype == 'osrc_relationship')
	{
	for(var keynum = 1; keynum <= 5; keynum++)
	    {
	    rl['key_' + keynum] = rule_widget['key_' + keynum];
	    rl['target_key_' + keynum] = rule_widget['target_key_' + keynum];
	    }
	rl.osrc = this;
	rl.target_osrc = wgtrGetNode(this, rule_widget.target);
	rl.is_slave = rule_widget.is_slave;
	if (rl.is_slave == null)
	    rl.is_slave = 1;
	osrc_relationships.push(rl);
	}
    }


function osrc_queue_request(r)
    {
    this.request_queue.push(r);
    }


function osrc_dispatch()
    {
    if (this.pending) return;
    var req = null;
    while ((req = this.request_queue.shift()) != null)
	{
	switch(req.Request)
	    {
	    case 'Query':
		this.QueryHandler(req.Param);
		break;

	    case 'QueryObject':
		this.QueryObjectHandler(req.Param);
		break;

	    case 'MoveTo':
		this.MoveToRecordHandler(req.Param);
		break;
	    }
	if (this.pending) break;
	}
    return;
    }


// OSRC Client routines (for linking with another osrc)
function osrc_oc_resync()
    {
    //alert('Resync: ' + this.sql);
    var sync_param = {ParentOSRC:this.master_osrc};
    for(var i=1; i<=10;i++)
	{
	sync_param['ParentKey'+i] = this.master_keys['master_'+i];
	sync_param['ChildKey'+i] = this.master_keys['slave_'+i];
	}
    this.ifcProbe(ifAction).Invoke("Sync", sync_param);
    }

function osrc_oc_data_available()
    {
    return;
    }

function osrc_oc_replica_moved()
    {
    this.Resync();
    return;
    }

function osrc_oc_is_discard_ready()
    {
    this.GoNogo(osrc_oc_is_discard_ready_yes, osrc_oc_is_discard_ready_no);
    return false;
    }

function osrc_oc_is_discard_ready_yes()
    {
    this.master_osrc.QueryContinue(this);
    }

function osrc_oc_is_discard_ready_no()
    {
    this.master_osrc.QueryCancel(this);
    }

function osrc_oc_object_available(o)
    {
    this.Resync();
    return;
    }

function osrc_oc_object_created(o)
    {
    this.Resync();
    return;
    }

function osrc_oc_object_modified(o)
    {
    this.Resync();
    return;
    }

function osrc_oc_object_deleted(o)
    {
    this.Resync();
    return;
    }

function osrc_oc_operation_complete(o)
    {
    return true;
    }


// Bottom Half of the initialization - after everything has had a chance
// to osrc_init()
function osrc_init_bh()
    {
    // Search for relationships... then register as an osrc client
    for(var i in osrc_relationships)
	{
	var rl = osrc_relationships[i];
	if ((rl.osrc == this && rl.is_slave) || (rl.target_osrc == this && !rl.is_slave))
	    {
	    if (rl.osrc == this)
		{
		this.master_osrc = rl.target_osrc;
		var masterkey = 'target_key_';
		var slavekey = 'key_';
		}
	    else
		{
		this.master_osrc = rl.osrc;
		var masterkey = 'key_';
		var slavekey = 'target_key_';
		}
	    this.master_osrc.Register(this);
	    //alert('Register: ' + this.sql);
	    this.master_keys = {};
	    for(var i=1;i<=10;i++)
		{
		this.master_keys['master_' + i] = rl[masterkey + i];
		this.master_keys['slave_' + i] = rl[slavekey + i];
		}
	    //pg_addsched_fn(this, "Resync", [], 0);
	    }
	}
    this.bh_finished = true;

    // Autoquery on load?  Reveal event already occurred?
    /*if (this.autoquery == this.AQonLoad) 
	pg_addsched_fn(this,'InitQuery', [], 0);
    else if (this.revealed_children && (this.autoquery == this.AQonFirstReveal || this.autoquery == this.AQonEachReveal) && !this.init)
	pg_addsched_fn(this,'InitQuery', [], 0);*/
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
    loader.sql=param.sql;
    loader.filter=param.filter;
    loader.baseobj=param.baseobj;
    loader.readonly = false;
    loader.autoquery = param.autoquery;
    loader.revealed_children = 0;
    loader.rulelist = [];
    loader.SyncID = osrc_syncid++;
    loader.bh_finished = false;
    loader.request_queue = [];

    // autoquery types - must match htdrv_osrc.c's enum declaration
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
    loader.ParseOneAttr = osrc_parse_one_attr;
    loader.ParseOneRow = osrc_parse_one_row;
    loader.NewReplicaObj = osrc_new_replica_object;
    loader.PruneReplica = osrc_prune_replica;
    loader.ClearReplica = osrc_clear_replica;
    loader.ApplyRelationships = osrc_apply_rel;
    loader.EndQuery = osrc_end_query;
    loader.FoundRecord = osrc_found_record;
    loader.DoFetch = osrc_do_fetch;
    loader.FetchNext = osrc_fetch_next;
    loader.GoNogo = osrc_go_nogo;
    loader.QueueRequest = osrc_queue_request;
    loader.Dispatch = osrc_dispatch;
    loader.GiveAllCurrentRecord=osrc_give_all_current_record;
    loader.MoveToRecord=osrc_move_to_record;
    loader.MoveToRecordCB=osrc_move_to_record_cb;
    loader.child = new Array();
    loader.oldoids = new Array();
    loader.sid = null;
    loader.qid = null;
    loader.savable_client_count = 0;

    loader.MoveToRecordHandler = osrc_move_to_record_handler;
    loader.QueryObjectHandler = osrc_query_object_handler;
    loader.QueryHandler = osrc_query_handler;
   
    // Actions
    var ia = loader.ifcProbeAdd(ifAction);
    //loader.ActionClear=osrc_action_clear;
    ia.Add("Query", osrc_action_query);
    ia.Add("QueryObject", osrc_action_query_object);
    ia.Add("QueryParam", osrc_action_query_param);
    ia.Add("OrderObject", osrc_action_order_object);
    ia.Add("Delete", osrc_action_delete);
    ia.Add("Create", osrc_action_create);
    ia.Add("Modify", osrc_action_modify);
    ia.Add("First", osrc_move_first);
    ia.Add("Next", osrc_move_next);
    ia.Add("Prev", osrc_move_prev);
    ia.Add("Last", osrc_move_last);
    ia.Add("Sync", osrc_action_sync);
    ia.Add("DoubleSync", osrc_action_double_sync);
    ia.Add("SaveClients", osrc_action_save_clients);

    // Events
    var ie = loader.ifcProbeAdd(ifEvent);
    ie.Add("DataFocusChanged");
    ie.Add("EndQuery");

    // Data Values
    var iv = loader.ifcProbeAdd(ifValue);
    iv.SetNonexistentCallback(osrc_get_value);

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

    loader.ScrollTo = osrc_scroll_to;
    loader.ScrollPrev = osrc_scroll_prev;
    loader.ScrollNext = osrc_scroll_next;
    loader.ScrollPrevPage = osrc_scroll_prev_page;
    loader.ScrollNextPage = osrc_scroll_next_page;

    loader.TellAllReplicaMoved = osrc_tell_all_replica_moved;

    loader.InitQuery = osrc_init_query;
    loader.cleanup = osrc_cleanup;

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

    // Debugging functions
    loader.print_debug = osrc_print_debug;

    // Request replication messages
    loader.request_updates = param.requestupdates?1:0;
    if (param.requestupdates) pg_msg_request(loader, pg_msg.MSG_REPMSG);
    loader.ControlMsg = osrc_cb_control_msg;

    if (loader.autoquery == loader.AQonLoad) 
	pg_addsched_fn(loader,'InitQuery', [], 0);

    // Finish initialization...
    pg_addsched_fn(loader, "InitBH", [], 0);

    return loader;
    }

