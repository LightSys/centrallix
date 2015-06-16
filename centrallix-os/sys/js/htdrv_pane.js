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

function pn_mouseup(e)
    {
    if (e.kind == 'pn') 
	{
	cn_activate(e.mainlayer, 'Click');
	cn_activate(e.mainlayer, 'MouseUp');
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pn_mousedown(e)
    {
    if (e.kind == 'pn') cn_activate(e.mainlayer, 'MouseDown');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pn_mouseover(e)
    {
    if (e.kind == 'pn') cn_activate(e.mainlayer, 'MouseOver');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pn_mouseout(e)
    {
    if (e.kind == 'pn') cn_activate(e.mainlayer, 'MouseOut');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pn_mousemove(e)
    {
    if (e.kind == 'pn') cn_activate(e.mainlayer, 'MouseMove');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pn_getval(attr)
    {
    return this.enabled;
    }

function pn_setval(attr, val)
    {
    if (val)
	this.enabled = true;
    else
	this.enabled = false;
    this.style.opacity = this.enabled?1.0:0.4;
    return this.enabled;
    }

function pn_setbackground(aparam)
    {
    if (aparam.Color) htr_setbgcolor(this, aparam.Color);
    else if (aparam.Image) htr_setbgimage(this, aparam.Image);
    else htr_setbackground(this, null);
    }

function pn_action_resize(aparam)
    {
    var w = aparam.Width?aparam.Width:pg_get_style(this, 'width');
    var h = aparam.Height?aparam.Height:pg_get_style(this, 'height');
    resizeTo(this, w, h);
    }

function pn_action_point(aparam)
    {
    var divs = htutil_point(this, aparam.X, aparam.Y, aparam.AtWidget, aparam.BorderColor, aparam.FillColor, this.point1, this.point2);
    this.point1 = divs.p1;
    this.point2 = divs.p2;
    }

function pn_init(param)
    {
    var l = param.layer;
    var ml = param.mainlayer;
    if((!cx__capabilities.Dom0NS) && cx__capabilities.CSS1)
	{
	ml = l;
	}

    htr_init_layer(ml,ml,"pn");
    if (ml != l) htr_init_layer(l,ml,"pn");
    ifc_init_widget(ml);

    ml.maxheight = getClipHeight(l)-2;
    ml.maxwidth = getClipWidth(l)-2;

    htutil_tag_images(l,'pn',ml,ml);

    // Events
    var ie = ml.ifcProbeAdd(ifEvent);
    ie.Add("Click");
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");

    // actions
    var ia = ml.ifcProbeAdd(ifAction);
    ia.Add("SetBackground", pn_setbackground);
    ia.Add("Resize", pn_action_resize);
    ia.Add("Point", pn_action_point);

    var iv = ml.ifcProbeAdd(ifValue);
    iv.Add("enabled", pn_getval, pn_setval);
    ml.enabled = param.enabled;
    if (param.enabled != null)
	iv.Changing("enabled", ml.enabled, true, null, true);

    return ml;
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_pane.js'] = true;
