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

function osrc_init_query()
    {
    //alert();
    if(this.init==true)
	return;
    this.init=true;
    this.ActionQueryObject(null,null);
    }

function osrc_action_order_object(order)
    {
    this.pendingorderobject=order;
    this.ActionQueryObject(this.queryobject);
    }

function osrc_action_query_object(q, formobj)
    {
    if(this.pending)
	{
	alert('There is already a query or movement in progress...');
	return 0;
	}
    this.pendingqueryobject=q;
    var statement=this.sql;
    var firstone=true;
    
    if(this.filter)
	{
	statement+=' WHERE ('+this.filter+')';
	firstone=false;
	}
    //if(!confirm(osrc_make_filter(q)))
    //    return 0;
    if(q && q.joinstring && q[0])
	{
	if(firstone)
	    statement+=' WHERE ';
	else
	    statement+=' '+q.joinstring+' ';
	statement+=osrc_make_filter(q);
	}
    
    firstone=true;
    if(this.pendingorderobject)
	for(i in this.pendingorderobject)
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
    //statement+=';';
    this.ActionQuery(statement,formobj);
    
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
		switch(q[i].type)
		    {
		    case 'integer':
			str=':'+q[i].oid+'='+val;
			break;
		    default:
			if(val.substring(0,2)=='>=')
			    str=':'+q[i].oid+' >= '+val.substring(2);
			else if(val.substring(0,2)=='<=')
			    str=':'+q[i].oid+' <= '+val.substring(2);
			else if(val.substring(0,2)=='=>')
			    str=':'+q[i].oid+' >= '+val.substring(2);
			else if(val.substring(0,2)=='=<')
			    str=':'+q[i].oid+' <= '+val.substring(2);
			else if(val.substring(0,1)=='>')
			    str=':'+q[i].oid+' > '+val.substring(1);
			else if(val.substring(0,1)=='<')
			    str=':'+q[i].oid+' < '+val.substring(1);
			else if((/\\*/).test(val))
			    str=':'+q[i].oid+' LIKE "'+val+'"';
			else
			    str=':'+q[i].oid+'='+'"'+val+'"';
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


function osrc_action_query(q, formobj)
    {
    //Is discard ready
    //if(!formobj.IsDiscardReady()) return 0;
    //Send query as GET request
    //this.formobj = formobj;
    if(this.pending)
	{
	alert('There is already a query or movement in progress...');
	return 0;
	}
    //var win=window.open();
    //win.document.write("<PRE>"+q+"</PRE>");
    //win.document.close();
    this.easyquery=q;
    this.pendingquery=String(escape(q)).replace("/","%2F","g");
    this.pending=true;
    var someunsaved=false;
/** Check if any children are modified and call IsDiscardReady if they are **/
    for(var i in this.children)
	 {
	 if(this.children[i].IsUnsaved)
	     {
	     this.children[i]._osrc_ready=false;
	     this.children[i].IsDiscardReady();
	     someunsaved=true;
	     }
	 else
	     {
	     this.children[i]._osrc_ready=true;
	     }
	 }
    //if someunsaved is false, there were no unsaved forms, so no callbacks
    //  so, we'll just fake one using the first form....
    if(someunsaved) return 0;
    this.QueryContinue(this.children[0]);
    }

function osrc_action_delete()
    {
    //Delete an object through OSML
    this.formobj.ObjectDeleted();
    this.formobj.OperationComplete();
    return 0;
    }

function osrc_action_create()
    {
    //Create an object through OSML
    //?ls__mode=osml&ls__req=setattrs'
    this.formobj.ObjectCreated();
    this.formobj.OperationComplete();
    return 0;
    }

function osrc_action_modify(up,formobj)
    {
    //Modify an object through OSML
    //aparam[adsf][value];
    
    //this.src='/?ls__mode=osml&ls__req=setattrs&ls__sid=' + this.sid + '&ls__oid=' + this.oid + '&attrname=valuename&attrname=valuename'
    //full_name=MonthThirteen&num_days=1400
    var src='/?ls__mode=osml&ls__req=setattrs&ls__sid=' + this.sid + '&ls__oid=' + up.oid;
    for(var i in up) if(i!='oid')
	{
	src+='&'+escape(up[i]['oid'])+'='+escape(up[i]['value']);
	}
    this.formobj=formobj;
    this.modifieddata=up;
    this.onload=osrc_action_modify_cb;
    this.src=src;
    }

function osrc_action_modify_cb()
    {
/** I don't know how to tell if we succeeded....assume we did... **/
    if(true)
	{
	var recnum=this.CurrentRecord;
	var cr=this.replica[this.CurrentRecord];
	if(cr)
	    for(var i in this.modifieddata) // update replica
		for(var j in cr)
		    if(cr[j].oid==this.modifieddata[i].oid)
			cr[j].value=this.modifieddata[i].value;
	
	this.formobj.OperationComplete(true);
	for(var i in this.children)
	    this.children[i].ObjectModified(recnum);
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
    if(!this.pending) return 0;
    //Current form ready
    if(o)
    	o._osrc_ready = true;
    //If all other forms are ready then go
    for(var i in this.children)
	 {
	 if(this.children[i]._osrc_ready == false)
	      {
	      return 0;
	      }
	 }
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
	for(var i in this.children)
	     {
	     this.children[i]._osrc_ready=false;
	     }

	this.TargetRecord=1;/* the record we're aiming for -- go until we get it*/
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
	this.moveop=true;

	this.OpenSession();
	}
    else
	{ // movement
	this.MoveToRecordCB(this.RecordToMoveTo);
	this.RecordToMoveTo=null;
	}
    this.pending=false;
    }

function osrc_cb_query_cancel()
    {
    this.pendingquery = null;
    for(var i in this.children)
	 {
	 this.children[i]._osrc_ready=false;
	 }
    this.pendingquery=null;
    this.pending=false;
    }

function osrc_cb_request_object(aparam)
    {
    return 0;
    }

function osrc_cb_register(aparam)
    {
    this.children.push(aparam);
    }

function osrc_open_session()
    {
    //Open Session
    //alert('open');
    if(this.sid)
	{
	this.OpenQuery();
	}
    else
	{
	this.onload = osrc_open_query;
	this.src = '/?ls__mode=osml&ls__req=opensession'
	}
    }

function osrc_open_query()
    {
    //Open Query
    //alert('sid' + this.document.links[0].target);
    if(!this.sid) this.sid=this.document.links[0].target;
    if(this.qid)
	{
	this.onload=osrc_open_query;
	this.src="/?ls__mode=osml&ls__req=queryclose&ls__sid="+this.sid+"&ls__qid="+this.qid;
	this.qid=null;
	return 0;
	}
    this.onload = osrc_get_qid;
    this.src="/?ls__mode=osml&ls__req=multiquery&ls__sid="+this.sid+"&ls__sql=" + this.query;
    //this.src = '/escape.html';
    }

function osrc_get_qid()
    {
    //return;
    //alert('qid ' + this.document.links[0].target);
    this.qid=this.document.links[0].target;
    for(var i in this.children)
	this.children[i].DataAvailable();
    this.ActionFirst();
/** normally don't actually load the data...just let children know that the data is available **/
    }

function osrc_fetch_next()
    {
    //alert('fetching....');
    if(!this.qid)
	{
	alert('somethign is wrong...');
	alert(this.src);
	}
    var lnk=this.document.links;
    var lc=lnk.length;
    if(lc < 2)
	{ // query over
	//this.formobj.OperationComplete(); /* don't need this...I think....*/
	var qid=this.qid
	this.qid=null;
/* return the last record as the current one if it was our target otherwise, don't */
	if(this.moveop)
	    {
	    if(this.CurrentRecord>this.LastRecord)
		this.CurrentRecord=this.LastRecord;
	    this.GiveAllCurrentRecord();
	    }
	else
	    {
	    this.TellAllReplicaMoved();
	    }
	this.pending=false;
	if(this.doublesync)
	    this.ActionDoubleSyncCB();
	if(qid)
	    {
	    this.onload=osrc_close_query;
	    this.src="/?ls__mode=osml&ls__req=queryclose&ls__sid="+this.sid+"&ls__qid="+qid;
	    }
	return 0;
	}
    var row='';
    var colnum=0;
    for (var i = 1; i < lc; i++)
	{
	if(row!=lnk[i].target)
	    { /** This is a different (or 1st) row of the result set **/
	    row=lnk[i].target;
	    this.OSMLRecord++; /* this holds the last record we got, so now will hold current record number */
	    this.replica[this.OSMLRecord]=new Array();
	    this.replica[this.OSMLRecord].oid=row;
	    if(this.LastRecord<this.OSMLRecord)
		{
		this.LastRecord=this.OSMLRecord;
		while(this.LastRecord-this.FirstRecord>=this.replicasize)
		    { /* clean up replica */
		    this.oldoids.push(this.replica[this.FirstRecord].oid);
		    delete this.replica[this.FirstRecord];
		    this.FirstRecord++;
		    }
		}
	    if(this.FirstRecord>this.OSMLRecord)
		{
		this.FirstRecord=this.OSMLRecord;
		while(this.LastRecord-this.FirstRecord>=this.replicasize)
		    { /* clean up replica */
		    this.oldoids.push(this.replica[this.LastRecord].oid);
		    delete this.replica[this.LastRecord];
		    this.LastRecord--;
		    }
		}
	    colnum=0;
	    //alert('New row: '+row+'('+this.OSMLRecord+')');
	    }
	colnum++;
	this.replica[this.OSMLRecord][colnum] = new Array();
	this.replica[this.OSMLRecord][colnum]['value'] = htutil_rtrim(unescape(lnk[i].text));
	this.replica[this.OSMLRecord][colnum]['type'] = lnk[i].hash.substr(1);
	this.replica[this.OSMLRecord][colnum]['oid'] = lnk[i].host;
	}
/** make sure we bring this.LastRecord back down to the top of our replica...**/
    while(!this.replica[this.LastRecord])
	this.LastRecord--;
    if(this.LastRecord<this.TargetRecord)
	{ /* We're going farther down this... */
	this.onload = osrc_fetch_next;
	this.src="/?ls__mode=osml&ls__req=queryfetch&ls__sid="+this.sid+"&ls__qid="+this.qid+"&ls__objmode=0&ls__encode=1&ls__rowcount="+this.readahead;
	}
    else
	{ /* we've got the one we need */
	if((this.LastRecord-this.FirstRecord+1)<this.replicasize)
	    { /* make sure we have a full replica if possible */
	    this.onload = osrc_fetch_next;
	    this.src="/?ls__mode=osml&ls__req=queryfetch&ls__sid="+this.sid+"&ls__qid="+this.qid+"&ls__objmode=0&ls__encode=1&ls__rowcount="+(this.replicasize-(this.LastRecord-this.FirstRecord+1));
	    }
	else
	    {
	    if(this.doublesync)
		this.ActionDoubleSyncCB();
	    if(this.moveop)
		this.GiveAllCurrentRecord();
	    else
		this.TellAllReplicaMoved();
	    this.pending=false;
	    this.onload=osrc_oldoid_cleanup;
	    this.onload();
	    }
	}
    }

function osrc_oldoid_cleanup()
    {
    this.onload=null;
    if(this.oldoids && this.oldoids[0])
	{
	this.pending=true;
	this.onload=osrc_oldoid_cleanup_cb;
	var src='';
	for(var i in this.oldoids)
	    src+=this.oldoids[i];
	if(this.sid)
	    this.src = '/?ls__mode=osml&ls__req=close&ls__sid='+this.sid+'&ls__oid=' + src;
	else
	    alert('session is invalid');
	}
    }
 
function osrc_oldoid_cleanup_cb()
    {
    this.pending=false;
    //alert('cb recieved');
    this.onload=null;
    delete this.oldoids;
    this.oldoids=new Array();
    }
 
function osrc_close_query()
    {
    //Close Query
    this.qid=null;
    this.onload=osrc_oldoid_cleanup;
    this.onload();
    //this.onload = osrc_close_session;
    //this.src = '/?ls__mode=osml&ls__req=queryclose&ls__qid=' + this.qid;
    }
 
function osrc_close_object()
    {
    //Close Object
    this.onload = osrc_close_session;
    this.src = '/?ls__mode=osml&ls__req=close&ls__oid=' + this.oid;
    }
 
function osrc_close_session()
    {
    //Close Session
    this.onload = null;
    this.onload=osrc_oldoid_cleanup;
    this.src = '/?ls__mode=osml&ls__req=closesession&ls__sid=' + this.sid;
    this.qid=null;
    this.sid=null;
    }


function osrc_move_first(formobj)
    {
    this.MoveToRecord(1);
    }

function osrc_give_all_current_record()
    {
    //confirm('give_all_current_record start');
    for(var i in this.children)
	this.children[i].ObjectAvailable(this.replica[this.CurrentRecord]);
    //confirm('give_all_current_record done');
    }

function osrc_tell_all_replica_moved()
    {
    //confirm('tell_all_replica_moved start');
    for(var i in this.children)
	if(this.children[i].ReplicaMoved)
	    this.children[i].ReplicaMoved();
    //confirm('tell_all_replica_moved done');
    }


function osrc_move_to_record(recnum)
    {
    if(recnum<1)
	{
	alert("Can't move past beginning.");
	return 0;
	}
    if(this.pending)
	{
	//alert('you got ahead');
	return 0;
	}
    this.pending=true;
    var someunsaved=false;
    this.RecordToMoveTo=recnum;
    for(var i in this.children)
	 {
	 if(this.children[i].IsUnsaved)
	     {
	     //alert('child: '+i+' : '+this.children[i].IsUnsaved+' isn\\'t saved...IsDiscardReady');
	     this.children[i]._osrc_ready=false;
	     this.children[i].IsDiscardReady();
	     someunsaved=true;
	     }
	 else
	     {
	     this.children[i]._osrc_ready=true;
	     }
	 }
    //if someunsaved is false, there were no unsaved forms, so no callbacks
    //  we can just continue
    if(someunsaved) return 0;
    this.MoveToRecordCB(recnum);
    }

function osrc_move_to_record_cb(recnum)
    {
    //confirm(recnum);
    this.moveop=true;
    if(recnum<1)
	{
	alert("Can't move past beginning.");
	return 0;
	}
    this.RecordToMoveTo=recnum;
    for(var i in this.children)
	 {
	 if(this.children[i].IsUnsaved)
	     {
	     //confirm('child: '+i+' : '+this.children[i].IsUnsaved+' isn\\'t saved...');
	     return 0;
	     }
	 }
/* If we're here, we're ready to go */
    this.TargetRecord=this.CurrentRecord=recnum;
    if(this.CurrentRecord <= this.LastRecord && this.CurrentRecord >= this.FirstRecord)
	{
	this.GiveAllCurrentRecord();
	this.pending=false;
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
		this.onload=osrc_open_query_startat;
		this.src="/?ls__mode=osml&ls__req=queryclose&ls__sid="+this.sid+"&ls__qid="+this.qid;
		}
	    else
		{
		this.onload=osrc_open_query_startat;
		this.onload();
		}
	    return 0;
	    }
	else
	    { /* data is farther on, act normal */
	    if(this.qid)
		{
		this.onload=osrc_fetch_next;
		if(this.CurrentRecord == Number.MAX_VALUE)
		    this.src="/?ls__mode=osml&ls__req=queryfetch&ls__encode=1&ls__sid="+this.sid+"&ls__qid="+this.qid+"&ls__objmode=0"; /* rowcount defaults to a really high number if not set */
		else
		    this.src="/?ls__mode=osml&ls__req=queryfetch&ls__encode=1&ls__sid="+this.sid+"&ls__qid="+this.qid+"&ls__objmode=0&ls__rowcount="+this.readahead;
		}
	    else
		{
		this.pending=false;
		this.CurrentRecord=this.LastRecord;
		this.GiveAllCurrentRecord();
		}
	    return 0;
	    }
	}
    }

function osrc_open_query_startat()
    {
    this.onload = osrc_get_qid_startat;
    this.src="/?ls__mode=osml&ls__req=multiquery&ls__sid="+this.sid+"&ls__sql=" + this.query;
    }

function osrc_get_qid_startat()
    {
    this.qid=this.document.links[0].target;
    this.OSMLRecord=this.startat-1;
    this.onload=osrc_fetch_next;
    //this.FirstRecord=this.startat;
    if(this.startat-this.TargetRecord+1<this.replicasize)
	{
	this.src='/?ls__mode=osml&ls__req=queryfetch&ls__encode=1&ls__sid='+this.sid+'&ls__qid='+this.qid+'&ls__objmode=0&ls__rowcount='+(this.TargetRecord-this.startat+1)+'&ls__startat='+this.startat;
	}
    else
	{
	this.src='/?ls__mode=osml&ls__req=queryfetch&ls__encode=1&ls__sid='+this.sid+'&ls__qid='+this.qid+'&ls__objmode=0&ls__rowcount='+this.replicasize+'&ls__startat='+this.startat;
	}
    this.startat=null;
    }


function osrc_move_next(formobj)
    {
    this.MoveToRecord(this.CurrentRecord+1);
    }

function osrc_move_prev(formobj)
    {
    this.MoveToRecord(this.CurrentRecord-1);
    }

function osrc_move_last(formobj)
    {
    this.MoveToRecord(Number.MAX_VALUE); /* FIXME */
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

function osrc_scroll_to(recnum)
    {
    this.moveop=false;
    this.TargetRecord=recnum;
    if(this.TargetRecord <= this.LastRecord && this.TargetRecord >= this.FirstRecord)
	{
	this.TellAllReplicaMoved();
	this.pending=false;
	return 1;
	}
    else
	{
	if(this.TargetRecord < this.FirstRecord)
	    { /* data is further back, need new query */
	    if(this.FirstRecord-this.TargetRecord<this.scrollahead)
		{
		this.startat=(this.FirstRecord-this.scrollahead)>0?(this.FirstRecord-this.scrollahead):1;
		}
	    else
		{
		this.startat=this.TargetRecord;
		}
	    this.onload=osrc_open_query_startat;
	    if(this.qid)
		{
		this.src="/?ls__mode=osml&ls__req=queryclose&ls__sid="+this.sid+"&ls__qid="+this.qid;
		}
	    else
		{
		this.onload();
		}
	    return 0;
	    }
	else
	    { /* data is farther on, act normal */
	    if(this.qid)
		{
		this.onload=osrc_fetch_next;
		if(this.TargetRecord == Number.MAX_VALUE)
		    this.src="/?ls__mode=osml&ls__req=queryfetch&ls__encode=1&ls__sid="+this.sid+"&ls__qid="+this.qid+"&ls__objmode=0"; /* rowcount defaults to a really high number if not set */
		else
		    this.src="/?ls__mode=osml&ls__req=queryfetch&ls__encode=1&ls__sid="+this.sid+"&ls__qid="+this.qid+"&ls__objmode=0&ls__rowcount="+this.scrollahead;
		}
	    else
		{
		this.pending=false;
		this.TargetRecord=this.LastRecord;
		this.TellAllReplicaMoved();
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
	this.src="/?ls__mode=osml&ls__req=queryclose&ls__sid="+this.sid+"&ls__qid="+this.qid;
	this.qid=null
	}
    }

function osrc_action_sync(param)
    {
    this.parentosrc=param.ParentOSRC;
    this.ParentKey=new Array();
    this.ChildKey=new Array();
    var query = new Array();
    query.oid=null;
    query.joinstring='AND';
    var p=this.parentosrc.CurrentRecord;
    for(var i=1;i<10;i++)
	{
	this.ParentKey[i]=eval('param.ParentKey'+i);
	this.ChildKey[i]=eval('param.ChildKey'+i);
	if(this.ParentKey[i])
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
    this.ActionQueryObject(query,null);
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
    this.ActionQueryObject(query,null);
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
    this.childosrc.ActionQueryObject(query,null);
    }

/**  OSRC Initializer **/
function osrc_init(loader,ra,sa,rs,sql,filter,name)
    {
    if(window_current)
	window_current.RegisterOSRC(loader);
    loader.osrcname=name;
    loader.readahead=ra;
    loader.scrollahead=sa;
    loader.replicasize=rs;
    loader.sql=sql;
    loader.filter=filter;
    
    loader.GiveAllCurrentRecord=osrc_give_all_current_record;
    loader.MoveToRecord=osrc_move_to_record;
    loader.MoveToRecordCB=osrc_move_to_record_cb;
    loader.children = new Array();
    loader.oldoids = new Array();
    
    //loader.ActionClear=osrc_action_clear;
    loader.ActionQuery=osrc_action_query;
    loader.ActionQueryObject=osrc_action_query_object;
    loader.ActionOrderObject=osrc_action_order_object;
    loader.ActionDelete=osrc_action_delete;
    loader.ActionCreate=osrc_action_create;
    loader.ActionModify=osrc_action_modify;

    loader.OpenSession=osrc_open_session;
    loader.OpenQuery=osrc_open_query;
    loader.CloseQuery=osrc_close_query;
    loader.CloseObject=osrc_close_object;
    loader.CloseSession=osrc_close_session;
/**    loader.StoreReplica=osrc_store_replica; **/
    loader.QueryContinue = osrc_cb_query_continue;
    loader.QueryCancel = osrc_cb_query_cancel;
    loader.RequestObject = osrc_cb_request_object;
    loader.Register = osrc_cb_register;
  
    loader.ActionFirst = osrc_move_first;
    loader.ActionNext = osrc_move_next;
    loader.ActionPrev = osrc_move_prev;
    loader.ActionLast = osrc_move_last;

    loader.ScrollTo = osrc_scroll_to;
    loader.ScrollPrev = osrc_scroll_prev;
    loader.ScrollNext = osrc_scroll_next;
    loader.ScrollPrevPage = osrc_scroll_prev_page;
    loader.ScrollNextPage = osrc_scroll_next_page;

    loader.TellAllReplicaMoved = osrc_tell_all_replica_moved;

    loader.ActionSync = osrc_action_sync;
    loader.ActionDoubleSync = osrc_action_double_sync;
    loader.ActionDoubleSyncCB = osrc_action_double_sync_cb;
    loader.InitQuery = osrc_init_query;
    loader.cleanup = osrc_cleanup;

    return loader;
    }
