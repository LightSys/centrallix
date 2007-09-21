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

function ms_mouseup(e)
    {
    if (e.kind == 'ms') 
	{
	cn_activate(e.mainlayer, 'Click');
	cn_activate(e.mainlayer, 'MouseUp');
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function ms_mousedown(e)
    {
    if (e.kind == 'ms') cn_activate(e.mainlayer, 'MouseDown');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function ms_mouseover(e)
    {
    if (e.kind == 'ms') cn_activate(e.mainlayer, 'MouseOver');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function ms_mouseout(e)
    {
    if (e.kind == 'ms') cn_activate(e.mainlayer, 'MouseOut');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function ms_mousemove(e)
    {
    if (e.kind == 'ms') cn_activate(e.mainlayer, 'MouseMove');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function ms_add_part(l, param)
    {
    htr_init_layer(l, this, "ms");
    l.always_vis = param.av;
    l.child_h = param.h;
    l.actual_h = param.h;
    l.child_y = param.y;
    l.minwidth = this.w;
    l.maxwidth = this.w;
    l.resized = ms_part_resize;
    this.partlist.push(l);
    this.total_height += l.child_h;
    if (param.av) this.always_vis_height += l.child_h;
    var sr = (this.total_height - this.h);
    this.scroll_range = (sr > 0)?sr:0;
    this.sbvalue_to_layout(0);
    return l;
    }

function ms_part_resize(w, h)	// this => multiscroll part
    {
    return true;
    }

function ms_sbvalue_to_layout(value)
    {
    if (value < 0) value = 0;
    if (value > this.scroll_range) value = this.scroll_range;

    var adj = value;
    var maxadj;
    var accum_y = 0;
    var accum_h = 0;
    for(var i=0;i<this.partlist.length;i++)
	{
	if (this.partlist[i].always_vis)
	    {
	    // Always visible.  Just position it.
	    this.partlist[i].child_y = accum_y;
	    setRelativeY(this.partlist[i], accum_y);
	    accum_y += this.partlist[i].actual_h;
	    }
	else
	    {
	    // Scroll this part?
	    if (adj && this.partlist[i].actual_h > this.h - this.always_vis_height)
		{
		maxadj = Math.min(adj, this.partlist[i].actual_h - (this.h - this.always_vis_height));
		adj -= maxadj;
		this.partlist[i].child_h = this.h - this.always_vis_height; 
		}
	    else
		{
		maxadj = 0;
		this.partlist[i].child_h = this.partlist[i].actual_h;
		}

	    // Clip this part?
	    if (this.partlist[i].child_h > 0)
		{
		maxadj = Math.min(adj, this.partlist[i].child_h);
		adj -= maxadj;
		this.partlist[i].child_h -= maxadj;
		}
	    if (this.partlist[i].child_h + accum_h > this.h - this.always_vis_height)
		{
		this.partlist[i].child_h = (this.h - this.always_vis_height) - accum_h;
		if (this.partlist[i].child_h < 0) this.partlist[i].child_h = 0;
		}

	    // Move the part into position
	    setRelativeY(this.partlist[i], accum_y - maxadj);
	    setClip(this.partlist[i], maxadj, this.w, maxadj + this.partlist[i].child_h, 0);
	    accum_y += this.partlist[i].child_h;
	    accum_h += this.partlist[i].child_h;
	    }
	}
    return;
    }

function ms_layout_to_sbvalue()
    {
    return;
    }

function ms_show_container(l, x, y)
    {
    return true;
    }

function ms_init(l, param)
    {
    htr_init_layer(l,l,"ms");
    ifc_init_widget(l);

    l.h = getClipHeight(l);
    l.w = getClipWidth(l);

    l.partlist = [];
    l.always_vis_height = 0;
    l.scroll_index = 0;
    l.total_height = 0;
    l.scroll_range = 0;

    l.addPart = ms_add_part;
    l.showcontainer = ms_show_container;

    l.layout_to_sbvalue = ms_layout_to_sbvalue;
    l.sbvalue_to_layout = ms_sbvalue_to_layout;

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("Click");
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");

    l.sbvalue_to_layout(0);

    return l;
    }
