// Copyright (C) 1998-2007 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function pa_actionsetvalue(aparam)
    {
    if (typeof aparam.Value != 'undefined') 
	{
	this.setvalue(cx_hints_checkmodify(this, this.value, aparam.Value, this.datatype));
	}
    }

function pa_getvalue()
    {
    return this.value;
    }

function pa_import_events_actions()
    {
    if (this.value.ifcProbe && this.value.ifcProbe(ifEvent))
	{
	var el = this.value.ifcProbe(ifEvent).GetEventList();
	for(var e in el)
	    {
	    this.ifcProbe(ifEvent).Add(el[e]);
	    this.value.ifcProbe(ifEvent).Hook(el[e], pa_event, this);
	    }
	}
    if (this.value.ifcProbe && this.value.ifcProbe(ifAction))
	{
	var al = this.value.ifcProbe(ifAction).GetActionList();
	for(var a in al)
	    {
	    this.ifcProbe(ifAction).Add(al[a], pa_action);
	    }
	}
    }

function pa_setvalue(v)
    {
    if (this.datatype == 'object')
	{
	this.value = wgtrDereference(v);
	if (this.value)
	    {
	    this.ImportEventsAndActions();

	    // if this is a component, schedule to import again, as list may have changed.
	    if (wgtrGetType(this.value) == "widget/component")
		pg_addsched_fn(this, "ImportEventsAndActions", [], 0);
	    }
	}
    else if (this.datatype == 'integer')
	{
	if (v === null)
	    this.value = v;
	else
	    this.value = parseInt(v);
	}
    else
	this.value = v;

    if (this.value === null && !this.in_startnew)
	{
	// Prevent null default recursion
	this.in_startnew = true;
	cx_hints_startnew(this);
	this.in_startnew = false;
	}
    }

function pa_verify()
    {
    cx_hints_startnew(this);
    this.setvalue(cx_hints_checkmodify(this, this.value, this.newvalue, this.datatype));

    // Notify container
    var p = wgtrGetParent(this);
    //if (p && p.ParamNotify) p.ParamNotify(wgtrGetName(this), this, this.datatype, this.value);
    if (p && p.ParamNotify) p.ParamNotify(this.realname, this, this.datatype, this.value);
    }

function pa_reference()
    {
    return this.value;
    }

function pa_event(eparam)
    {
    var e = eparam._EventName;
    return this.ifcProbe(ifEvent).Activate(e, eparam);
    }

function pa_action(aparam, aname)
    {
    if (this.value && this.value.ifcProbe(ifAction) && this.value.ifcProbe(ifAction).Exists(aname))
	return this.value.ifcProbe(ifAction).Invoke(aname, aparam);
    }

function pa_init(l, wparam)
    {
    ifc_init_widget(l);

    // Params
    l.realname = wparam.name?wparam.name:wgtrGetName(l);
    l.newvalue = wparam.val;
    l.value = null;
    l.in_startnew = false;
    if (l.newvalue != null)
	{
	l.newvalue = htutil_unpack(l.newvalue);
	}
    l.datatype = wparam.type;
    l.findcontainer = wparam.findc;
    l.destroy_widget = pa_deinit;

    if (!l.newvalue && l.findcontainer && wgtrGetType(wgtrGetParent(l)) == 'widget/component-decl')
	{
	l.newvalue = wgtrCheckReference(wgtrGetParent(l).FindContainer(l.findcontainer));
	}

    // Callbacks
    l.getvalue = pa_getvalue;
    l.setvalue = pa_setvalue;
    l.Verify = pa_verify;
    l.ImportEventsAndActions = pa_import_events_actions;
    if (l.datatype == 'object')
	l.reference = pa_reference;

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("DataChange");
    ie.EnableLateConnectBinding();

    // Actions
    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("SetValue", pa_actionsetvalue);

    return l;
    }

function pa_deinit()
    {
    // Unhook from events so we don't shoot fish in a nonexistent barrel.
    if (this.value)
	{
	if (this.value.ifcProbe && this.value.ifcProbe(ifEvent))
	    {
	    var el = this.value.ifcProbe(ifEvent).GetEventList();
	    for(var e in el)
		{
		this.value.ifcProbe(ifEvent).UnHook(el[e], pa_event, this);
		}
	    }
	}
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_parameter.js'] = true;
