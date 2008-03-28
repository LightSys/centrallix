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
    if (aparam.Value) 
	{
	this.setvalue(cx_hints_checkmodify(this, this.value, aparam.Value, this.datatype));
	}
    }

function pa_getvalue()
    {
    return this.value;
    }

function pa_setvalue(v)
    {
    if (this.datatype == 'object')
	this.value = wgtrDereference(v);
    else
	this.value = v;
    }

function pa_verify()
    {
    cx_hints_startnew(this);
    this.setvalue(cx_hints_checkmodify(this, this.value, this.newvalue, this.datatype));
    }

function pa_reference()
    {
    return this.value;
    }

function pa_init(l, wparam)
    {
    ifc_init_widget(l);

    // Params
    l.newvalue = wparam.val;
    l.value = null;
    if (l.newvalue != null)
	{
	l.newvalue = htutil_unpack(l.newvalue);
	}
    l.datatype = wparam.type;
    l.findcontainer = wparam.findc;

    if (!l.newvalue && l.findcontainer && wgtrGetType(wgtrGetParent(l)) == 'widget/component-decl')
	{
	l.newvalue = wgtrCheckReference(wgtrGetParent(l).FindContainer(l.findcontainer));
	}

    // Callbacks
    l.getvalue = pa_getvalue;
    l.setvalue = pa_setvalue;
    l.Verify = pa_verify;
    if (l.datatype == 'object')
	l.reference = pa_reference;

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("DataChange");

    // Actions
    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("SetValue", pa_actionsetvalue);

    return l;
    }
