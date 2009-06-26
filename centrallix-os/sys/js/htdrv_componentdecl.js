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

function cmpd_init(node, param) // I think that: param.gns = ? namespace, param.gname = ? name
    {
    // component is access point for stuff inside, shell is access point for stuff outside.
    var component = node;
    var shell = wgtrGetNode(param.gns, param.gname);

    // interface init
    ifc_init_widget(component);
    var ia = component.ifcProbeAdd(ifAction);
    ia.Add("TriggerEvent", cmpd_action_trigger_event);
    var ie = component.ifcProbeAdd(ifEvent);
    shell.RegisterComponent(component);
    wgtrRegisterContainer(component, shell);

    component.addAction = cmpd_add_action;
    component.addEvent = cmpd_add_event;
    component.addProp = cmpd_add_prop;
    component.postInit = cmpd_post_init;
    component.handleAction = cmpd_handle_action;
    component.ifcProbe(ifAction).Add('ModifyProperty', cmpd_action_modify_property);
    component.ifcProbe(ifEvent).Add('LoadComplete');
    component.shell = shell;
    component.is_visual = param.vis;
    component.FindContainer = cmpd_find_container;

    // expose events/actions on a subwidget
    if (param.expe) component.expe = param.expe;
    if (param.expa) component.expa = param.expa;
    if (param.expp) component.expp = param.expp;

    //if (param.expe || param.expa || param.expp)
    //	pg_addsched_fn(component, "postInit", [], 0);

    // Obscure/Reveal
    component.HandleReveal = cmpd_handle_reveal;
    component.HandleLoadComplete = cmpd_handle_load_complete;
    component.Reveal = cmpd_cb_reveal;
    pg_reveal_register_triggerer(component);

    // Show Container
    component.showcontainer = cmpd_showcontainer;

    return component;
    }

function cmpd_endinit(c)
    {
    if (c.expe || c.expa || c.expp) c.postInit();
    return;
    }

function cmpd_post_init()
    {
    if (this.expe)
	{
	var ewidget = wgtrGetNode(this, this.expe);
	if (ewidget && ewidget.ifcProbe(ifEvent))
	    {
	    var elist = ewidget.ifcProbe(ifEvent).GetEventList();
	    for(var e in elist)
		{
		if (!this.ifcProbe(ifAction).Exists(elist[e]))
		    {
		    if (!this.shell.ifcProbe(ifEvent).Exists(elist[e]))
			this.shell.ifcProbe(ifEvent).Add(elist[e]);
		    this.ifcProbe(ifAction).Add(elist[e], new Function('aparam','return this.handleAction("' + elist[e] + '",aparam);'));
		    ewidget.ifcProbe(ifEvent).Connect(elist[e], wgtrGetName(this), elist[e], null);
		    }
		}
	    }
	}
    if (this.expa)
	{
	var awidget = wgtrGetNode(this, this.expa);
	if (awidget && awidget.ifcProbe(ifAction))
	    {
	    var alist = awidget.ifcProbe(ifAction).GetActionList();
	    for(var a in alist)
		{
		if (!this.shell.ifcProbe(ifAction).Exists(alist[a]))
		    {
		    this.addAction(alist[a]);
		    this.ifcProbe(ifEvent).Connect(alist[a], wgtrGetName(awidget), alist[a], null);
		    }
		}
	    }
	}

    if (this.expp)
	{
	var w = wgtrGetNode(this, this.expp);
	if (w)
	    {
	    this.shell.registerPropTarget(this, w);
	    }
	}

    // notify parent component?
    var pc = wgtrFindContainer(this.shell, "widget/component-decl");
    if (pc)
	{
	pc.postInit();
	}
    }


// Called when something *inside* the component wants to trigger what
// would normally be an action invoked from outside.  This results in
// an event internally with the name specified by aparam.EventName
function cmpd_action_trigger_event(aparam)
    {
    if (!aparam.EventName) return;
    return cn_activate(this, aparam.EventName, aparam);
    }


// >> Makes sure containers, from innermost to outermost, are
// displayed to the user.  Used when a control receives keyboard focus
// to make sure control is visible to user.
function cmpd_showcontainer(l,x,y)
    {
    pg_show_containers(this.shell);
    return true;
    }

// This is a static form function to find the form that contains the
// given element.  Replacement for the old fm_current logic.
function cmpd_find_container(t)
    {
    return wgtrFindContainer(this.shell, t);
    }

function cmpd_cb_reveal(e)
    {
    switch(e.eventName)
	{
	case 'RevealOK':
	case 'ObscureOK':
	case 'RevealFailed':
	case 'ObscureFailed':
	    return this.shell.HandleReveal(e.eventName, e.c);
	}
    return true;
    }

function cmpd_handle_reveal(ename, ctx)
    {
    return pg_reveal_event(this, ctx, ename);
    }

function cmpd_handle_load_complete()
    {
    this.ifcProbe(ifEvent).Activate('LoadComplete', {});
    }

function cmpd_add_action(a)
    {
    //this.shell.ifcProbe(ifAction).Add(a, new Function('aparam','this.handleAction("' + a + '",aparam);'));
    this.shell.AddAction(this, a);
    this.ifcProbe(ifEvent).Add(a);
    return;
    }

function cmpd_add_event(e)
    {
    this.ifcProbe(ifAction).Add(e, new Function('aparam','this.handleAction("' + e + '",aparam);'));
    this.shell.ifcProbe(ifEvent).Add(e);
    return;
    }

function cmpd_add_prop(p)
    {
    this.shell[p] = null;
    htr_watch(this.shell, p, cmpd_shell_prop_change);
    this.ifcProbe(ifEvent).Add('MODIFY' + p);
    return;
    }

// This is called by the ModifyProperty action in the internal component;
// update the shell's exposed client prop.  Don't re-trigger an internal
// event on this unless requested.
function cmpd_action_modify_property(aparam)
    {
    if (aparam.PropertyName && aparam.NewValue) 
	{
	if (aparam.TriggerEvent) htr_unwatch(this.shell, aparam.PropertyName, cmpd_shell_prop_change);
	this.shell[aparam.PropertyName] = aparam.NewValue;
	if (aparam.TriggerEvent) htr_watch(this.shell, aparam.PropertyName, cmpd_shell_prop_change);
	}
    return;
    }

// this happens when a client prop is changed; cause an internal event on the component
function cmpd_shell_prop_change(p,o,n)
    {
    cn_activate(this.component, 'MODIFY' + p, {OldValue:o, NewValue:n});
    return n;
    }

// when an action is called externally, trigger an internal event on the component
function cmpd_shell_handle_action(a,aparam)
    {
    return cn_activate(this.component, a, aparam);
    }

// when an action is called internally, trigger an external event on the shell
function cmpd_handle_action(a,aparam)
    {
    return cn_activate(this.shell, a, aparam);
    }

// set the context of the component
function cmpd_shell_set_context(l)
    {
    this.context = l;
    return;
    }
