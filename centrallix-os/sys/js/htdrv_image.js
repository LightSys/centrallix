// htdrv_image.js
//
// Copyright (C) 2004 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function im_mouseup(e)
    {
    if (e.kind == 'im') 
	{
	cn_activate(e.layer, 'Click');
	cn_activate(e.layer, 'MouseUp');
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function im_mousedown(e)
    {
    if (e.kind == 'im') cn_activate(e.layer, 'MouseDown');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function im_mouseover(e)
    {
    if (e.kind == 'im') cn_activate(e.layer, 'MouseOver');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function im_mouseout(e)
    {
    if (e.kind == 'im') cn_activate(e.layer, 'MouseOut');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function im_mousemove(e)
    {
    if (e.kind == 'im') cn_activate(e.layer, 'MouseMove');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function im_get_source(a)
    {
    return this.source;
    }

function im_set_source(a, v)
    {
    this.source = v;
    pg_set(this.img, "src", v);
    }

function im_action_load_image(aparam)
    {
    var newurl = '';
    if (typeof aparam.Source != 'undefined')
	newurl = aparam.Source;
    else
	return;

    // parameters
    for(var p in aparam)
	{
	if (p == '_Origin' || p == 'Source' || p == '_EventName') continue;
	var v = aparam[p];
	if (newurl.lastIndexOf('?') > newurl.lastIndexOf('/'))
	    newurl += '&';
	else
	    newurl += '?';
	newurl += (htutil_escape(p) + '=' + htutil_escape(v));
	}

    // session linkage
    if (newurl.substr(0,1) == '/')
	{
	if (newurl.lastIndexOf('?') > newurl.lastIndexOf('/'))
	    newurl += '&';
	else
	    newurl += '?';
	if (aparam.LinkApp !== null && (aparam.LinkApp == 'yes' || aparam.LinkApp == 1))
	    newurl += "cx__akey=" + window.akey;
	else
	    newurl += "cx__akey=" + window.akey.substr(0,49);
	}

    this.source = newurl;
    //pg_set(this.img, "src", newurl);
    pg_serialized_load(this.img, newurl, im_loaded, true);
    }

function im_loaded()
    {
    var l = this.mainlayer;
    }

function im_get_x(a)
    {
    return this.xoffset;
    }

function im_set_x(a, v)
    {
    if (isNaN(v) || v === undefined || v === null)
	this.xoffset = 0;
    else
	this.xoffset = v;
    $(this.img).css({"left":parseInt(this.xoffset) + "px"});
    }

function im_get_y(a)
    {
    return this.yoffset;
    }

function im_set_y(a, v)
    {
    if (isNaN(v) || v === undefined || v === null)
	this.yoffset = 0;
    else
	this.yoffset = v;
    $(this.img).css({"top":parseInt(this.yoffset) + "px"});
    }

function im_get_scale(a)
    {
    return this.scale;
    }

function im_set_scale(a, v)
    {
    if (isNaN(v) || v === undefined || v === null)
	this.scale = 1.0;
    else
	this.scale = v;
    $(this.img).width(parseInt(this.scale * $(this).width()));
    }

function im_action_offset(aparam)
    {
    im_set_x.call(this, "xoffset", aparam.X);
    im_set_y.call(this, "yoffset", aparam.Y);
    }

function im_action_scale(aparam)
    {
    im_set_scale.call(this, "scale", aparam.Scale);
    }

function im_init(l)
    {
    htr_init_layer(l,l,"im");
    ifc_init_widget(l);
    htutil_tag_images(l, "im", l, l);
    var imgs = pg_images(l);
    l.img = imgs[0];
    l.img.onload = im_loaded;
    l.source = pg_get(l.img, "src");

    // Image display controls
    l.scale = 1.0;
    l.xoffset = 0;
    l.yoffset = 0;

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("Click");
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");

    var iv = l.ifcProbeAdd(ifValue);
    iv.Add("source", im_get_source, im_set_source);
    iv.Add("xoffset", im_get_x, im_set_x);
    iv.Add("yoffset", im_get_y, im_set_y);
    iv.Add("scale", im_get_scale, im_set_scale);

    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("LoadImage", im_action_load_image);
    ia.Add("Offset", im_action_offset);
    ia.Add("Scale", im_action_scale);

    im_loaded.call(this);

    return l;
    }


// Load indication
if (window.pg_scripts) pg_scripts['htdrv_image.js'] = true;
