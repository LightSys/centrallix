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

function cmpd_init(node, param)
    {
    // component is access point for stuff inside, shell is access point for stuff outside.
    var component = node;
    var shell = wgtrGetNode(param.gns, param.gname);

    // interface init
    ifc_init_widget(component);
    component.ifcProbeAdd(ifAction);
    component.ifcProbeAdd(ifEvent);
    shell.RegisterComponent(component);

    component.addAction = cmpd_add_action;
    component.addEvent = cmpd_add_event;
    component.addProp = cmpd_add_prop;
    component.handleAction = cmpd_handle_action;
    component.ifcProbe(ifAction).Add('ModifyProperty', cmpd_action_modify_property);
    component.shell = shell;
    component.is_visual = param.vis;
    component.FindContainer = cmpd_find_container;

    // Obscure/Reveal
    component.HandleReveal = cmpd_handle_reveal;
    component.Reveal = cmpd_cb_reveal;
    pg_reveal_register_triggerer(component);

    return component;
    }

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

function cmpd_endinit(c)
    {
    return;
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
    cn_activate(this.component, a, aparam);
    return;
    }

// when an action is called internally, trigger an external event on the shell
function cmpd_handle_action(a,aparam)
    {
    cn_activate(this.shell, a, aparam);
    return;
    }

// set the context of the component
function cmpd_shell_set_context(l)
    {
    this.context = l;
    return;
    }
