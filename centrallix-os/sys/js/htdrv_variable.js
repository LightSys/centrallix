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

function vbl_actionsetvalue(aparam)
    {
    if ((typeof aparam.Value) != 'undefined')
	this.setvalue(aparam.Value);
    if (this.form)
	this.form.DataNotify(this, true);
    cn_activate(this,"DataModify", {});
    }

function vbl_getvalue()
    {
    return this.value;
    }

function vbl_setvalue(v)
    {
    this.value = v;
    }

function vbl_clearvalue()
    {
    this.value = null;
    }

function vbl_enable()
    {
    }

function vbl_disable()
    {
    }

function vbl_init(l, wparam)
    {
    htr_init_layer(l,l,'vbl');
    ifc_init_widget(l);

    // Params
    l.datatype = wparam.type;
    l.fieldname = wparam.field;
    l.value = l.initial_value = wparam.value;

    // Callbacks
    l.getvalue = vbl_getvalue;
    l.setvalue = vbl_setvalue;
    l.clearvalue = vbl_clearvalue;
    l.enable = vbl_enable;
    l.disable = vbl_disable;

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("DataChange");
    ie.Add("DataModify");

    // Actions
    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("SetValue", vbl_actionsetvalue);

    // Values
    var iv = l.ifcProbeAdd(ifValue);
    iv.Add("value", "value");

    // Register with form.
    l.enabled = 'disable';
    l.isFormStatusWidget = false;
    if (l.fieldname)
	{
	if (wparam.form)
	    l.form = wgtrGetNode(l, wparam.form);
	else
	    l.form = wgtrFindContainer(l, "widget/form");
	if (l.form)
	    l.form.Register(l);
	}
    else
	{
	l.form = null;
	}

    return l;
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_variable.js'] = true;
