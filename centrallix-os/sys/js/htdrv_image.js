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
    if (e.kind == 'img') 
	{
	cn_activate(e.layer, 'Click');
	cn_activate(e.layer, 'MouseUp');
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function im_mousedown(e)
    {
    if (e.kind == 'img') cn_activate(e.layer, 'MouseDown');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function im_mouseover(e)
    {
    if (e.kind == 'img') cn_activate(e.layer, 'MouseOver');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function im_mouseout(e)
    {
    if (e.kind == 'img') cn_activate(e.layer, 'MouseOut');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function im_mousemove(e)
    {
    if (e.kind == 'img') cn_activate(e.layer, 'MouseMove');
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
	newurl += "cx__akey=" + window.akey.substr(0,49);
	}

    this.source = newurl;
    //pg_set(this.img, "src", newurl);
    pg_serialized_load(this.img, newurl, null, true);
    }

function im_init(l)
    {
    htr_init_layer(l,l,"im");
    ifc_init_widget(l);
    var imgs = pg_images(l);
    l.img = imgs[0];
    l.source = pg_get(l.img, "src");

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

    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("LoadImage", im_action_load_image);

    return l;
    }


// Load indication
if (window.pg_scripts) pg_scripts['htdrv_image.js'] = true;
