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

function cmpd_init(node, is_visual)
    {
    // component is access point for stuff inside, shell is access point for stuff outside.
    var component = new Object();
    var shell = new Object();

    // interface init
    ifc_init_widget(component);
    ifc_init_widget(shell);
    component.ifcProbeAdd(ifAction);
    component.ifcProbeAdd(ifEvent);
    shell.ifcProbeAdd(ifAction);
    shell.ifcProbeAdd(ifEvent);

    shell.setContext = cmpd_shell_set_context;
    shell.handleAction = cmpd_shell_handle_action;
    shell.context = null;
    component.addAction = cmpd_add_action;
    component.addEvent = cmpd_add_event;
    component.addProp = cmpd_add_prop;
    component.handleAction = cmpd_handle_action;
    component.ifcProbe(ifAction).Add('ModifyProperty', cmpd_action_modify_property);
    shell.ifcProbe(ifEvent).Add('ModifyProperty');
    component.shell = shell;
    component.is_visual = is_visual;
    shell.component = component;
    return component;
    }

function cmpd_endinit(c)
    {
    return;
    }

function cmpd_add_action(a)
    {
    this.shell.ifcProbe(ifAction).Add(a, new Function('aparam','this.handleAction("' + a + '",aparam);'));
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
