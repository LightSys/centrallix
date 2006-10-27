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

    return ml;
    }
