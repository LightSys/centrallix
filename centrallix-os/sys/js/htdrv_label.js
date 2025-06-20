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

if (typeof lb_current == 'undefined') var lb_current = null;

function lbl_mouseup(e)
    {
    if (e.kind == 'lbl') 
	{
	if (lb_current == e.layer) cn_activate(e.layer, 'Click');
	cn_activate(e.layer, 'MouseUp');
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function lbl_mousedown(e)
    {
    if (e.kind == 'lbl')
	{
	if (!lb_current)
	    lb_current = e.layer;
	cn_activate(e.layer, 'MouseDown');
	}
    if (e.layer.tipid) { pg_canceltip(e.layer.tipid); e.layer.tipid = null; }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function lbl_mouseover(e)
    {
    if (e.kind == 'lbl')
	{
	cn_activate(e.layer, 'MouseOver');
	lb_current = e.layer;
	if (e.layer.tooltip) e.layer.tipid = pg_tooltip(e.layer.tooltip, e.pageX, e.pageY);
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function lbl_mouseout(e)
    {
    if (e.kind == 'lbl')
	{
	cn_activate(e.layer, 'MouseOut');
	if (lb_current == e.layer) lb_current = null;
	if (e.layer.tipid) { pg_canceltip(e.layer.tipid); e.layer.tipid = null; }
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function lbl_mousemove(e)
    {
    if (e.kind == 'lbl')
	{
	cn_activate(e.layer, 'MouseMove');
	if (e.layer.tipid) { pg_canceltip(e.layer.tipid); e.layer.tipid = null; }
	if (e.layer.tooltip) e.layer.tipid = pg_tooltip(e.layer.tooltip, e.pageX, e.pageY);
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function lb_actionsetvalue(aparam)
    {
    if ((typeof aparam.Value) != 'undefined')
	{
	this.setvalue(aparam.Value);
	if (this.form) this.form.DataNotify(this, true);
	}
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
    this.content = this.orig_content;
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
    var v = htutil_nlbr(htutil_encode(htutil_obscure(this.content), true));
    //var txt = this.stylestr + (v?v:"") + "</font></td></tr></table>";
    var txt = v?v:"";
    $(this).find("span").html(txt).attr("style", htutil_getstyle(this, null, {}));
    /*if (cx__capabilities.Dom0NS) // only serialize this for NS4
	pg_serialized_write(this, txt, null);
    else
	htr_write_content(this, txt);*/
    this.CheckSize();
    }

function lb_cb_reveal(e)
    {
    if (this.form)
	var parents = [this.form];
    else
	var parents = this.precedents;

    switch (e.eventName) 
	{
	case 'Reveal':
	    for(var i=0; i<parents.length; i++)
		parents[i].Reveal(this,e);
	    break;
	case 'Obscure':
	    for(var i=0; i<parents.length; i++)
		{
		if (wgtrGetType(parents[i]) == 'widget/form')
		    parents[i].Reveal(this,e);
		else
		    parents[i].Obscure(this);
		}
	    //if (this.form) this.form.Reveal(this,e);
	    break;
	case 'RevealCheck':
	case 'ObscureCheck':
	    if (this.form) this.form.Reveal(this,e);
	    else pg_reveal_check_ok(e);
	    break;
	}

    return true;
    }

function lbl_check_size()
    {
    // Auto height adjust?
    pg_check_resize(this);
    }


// used by ifValue
function lbl_cb_getvalue(attr)
    {
    return this.getvalue();
    }
function lbl_cb_setvalue(attr, val)
    {
    this.setvalue(val);
    this.orig_content = this.content;
    return;
    }


// DO NOT COPY! TOP SECRET FUNCTION!
function lbl_init(l, wparam)
    {
    htr_init_layer(l,l,'lbl');
    ifc_init_widget(l);

    // Params
    l.content = wparam.text;
    l.orig_content = wparam.text;
    l.fieldname = wparam.field;
    l.stylestr = wparam.style;
    l.tooltip = wparam.tooltip;
    l.tipid = null;
    l.pointcolor = wparam.pfg;
    l.is_link = wparam.link;

    // Callbacks
    l.getvalue = lb_getvalue;
    l.setvalue = lb_setvalue;
    l.clearvalue = lb_clearvalue;
    l.enable = lb_enable;
    l.disable = lb_disable;

    l.Update = lb_update;
    l.CheckSize = lbl_check_size;

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

    // Values
    var iv = l.ifcProbeAdd(ifValue);
    iv.Add("value", lbl_cb_getvalue, lbl_cb_setvalue);

    // Register with form.
    l.enabled = 'disable';
    l.isFormStatusWidget = false;
    if (l.fieldname)
	{
	if (wparam.form)
	    l.form = wgtrGetNode(l, wparam.form);
	else
	    l.form = wgtrFindContainer(l, "widget/form");
	if (l.form) l.form.Register(l);
	}
    else
	{
	l.form = null;
	}

    // Request reveal/obscure notifications
    if (l.form)
	{
	l.Reveal = lb_cb_reveal;
	if (pg_reveal_register_listener(l)) 
	    {
	    // already visible
	    l.form.Reveal(l,{ eventName:'Reveal' });
	    }
	}
    else if (wgtrIsExpressionServerProperty(l, "value"))
	{
	var precedents = wgtrGetServerPropertyPrecedents(l, "value");
	l.precedents = [];
	l.Reveal = lb_cb_reveal;
	var is_vis = pg_reveal_register_listener(l);

	for(var i=0; i<precedents.length; i++)
	    {
	    var precedent = wgtrGetNodeUnchecked(l, precedents[i].obj);
	    if (precedent && (wgtrGetType(precedent) == 'widget/osrc' || wgtrGetType(precedent) == 'widget/form'))
		{
		l.precedents.push(precedent);
		if (is_vis)
		    precedent.Reveal(l, {eventName:'Reveal'} );
		}
	    }
	}

    $(l).find("span").attr("style", htutil_getstyle(l, null, {}));
    $(l).find("span").on("click", function() {});
    $(l).find("span").on("mousedown", function() {});
    $(l).find("span").on("mouseup", function() {});

    l.CheckSize();

    return l;
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_label.js'] = true;
