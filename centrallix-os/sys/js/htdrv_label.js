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

function lbl_mouseup(e)
    {
    if (e.kind == 'lbl') 
	{
	cn_activate(e.layer, 'Click');
	cn_activate(e.layer, 'MouseUp');
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function lbl_mousedown(e)
    {
    if (e.kind == 'lbl') cn_activate(e.layer, 'MouseDown');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function lbl_mouseover(e)
    {
    if (e.kind == 'lbl') cn_activate(e.layer, 'MouseOver');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function lbl_mouseout(e)
    {
    if (e.kind == 'lbl') cn_activate(e.layer, 'MouseOut');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function lbl_mousemove(e)
    {
    if (e.kind == 'lbl') cn_activate(e.layer, 'MouseMove');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function lb_actionsetvalue(aparam)
    {
    if (aparam.Value) this.setvalue(aparam.Value);
    }

function lb_getvalue()
    {
    return this.content;
    }

function lb_setvalue(v)
    {
    this.content = v;
    this.Update();
    }

function lb_clearvalue()
    {
    this.content = '';
    this.Update();
    }

function lb_enable()
    {
    }

function lb_disable()
    {
    }

function lb_update()
    {
    pg_serialized_write(this, this.stylestr + htutil_encode(this.content) + "</font></td></tr></table>", null);
    }

// DO NOT COPY! TOP SECRET FUNCTION!
function lbl_init(l, wparam)
    {
    htr_init_layer(l,l,'lbl');
    ifc_init_widget(l);

    // Params
    l.content = wparam.text;
    l.fieldname = wparam.field;
    l.stylestr = wparam.style;

    // Callbacks
    l.getvalue = lb_getvalue;
    l.setvalue = lb_setvalue;
    l.clearvalue = lb_clearvalue;
    l.enable = lb_enable;
    l.disable = lb_disable;

    l.Update = lb_update;

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("Click");
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");
    ie.Add("DataChange");

    // Actions
    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("SetValue", lb_actionsetvalue);

    // Register with form.
    l.enabled = 'disable';
    l.isFormStatusWidget = false;
    if (l.fieldname)
	{
	l.form = wgtrFindContainer(l, "widget/form");
	if (l.form) l.form.Register(l);
	}
    else
	{
	l.form = null;
	}

    return l;
    }
