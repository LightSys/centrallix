// Copyright (C) 1998-2003 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function sb_init(l,tname,p,is_h,r)
    {
    var tlayer=null;
    for(i=0;i<l.layers.length;i++)
	{
	var ml=l.layers[i];
	if(ml.name==tname) tlayer=ml;
	}
    for(i=0;i<l.document.images.length;i++)
	{
	var img=l.document.images[i];
	if(img.name=='d' || img.name=='u' || img.name=='b')
	    {
	    img.pane=l;
	    img.layer = img;
	    img.thum=tlayer;
	    img.kind='sb';
	    img.mainlayer=l;
	    }
	}
    tlayer.document.images[0].kind='sb';
    tlayer.document.images[0].layer = tlayer.document.images[0];
    tlayer.document.images[0].mainlayer=l;
    tlayer.document.images[0].thum=tlayer;
    tlayer.document.images[0].pane=l;
    tlayer.nofocus = true;
    tlayer.document.layer = tlayer;
    tlayer.mainlayer = l;
    tlayer.kind = 'sb';
    l.thum = tlayer;
    l.document.layer = l;
    l.mainlayer = l;
    l.kind = 'sb';
    l.LSParent = p;
    l.range = r;
    l.value = 0;
    l.is_horizontal = is_h;
    l.controlsize = is_h?(getClipWidth(l) - 18*3):(getClipHeight(l) - 18*3);
    l.watch('range', sb_range_changed);
    l.ActionMoveTo = sb_action_move_to;
    l.SetThumb = sb_set_thumb;
    return l;
    }

function sb_action_move_to(aparam)
    {
    v = new Number(aparam.Value);
    v = v.valueOf();
    this.SetThumb(this.range, v);
    this.value = v;
    }

function sb_set_thumb(r,v)
    {
    if (this.is_horizontal)
	{
	if (r > 0) this.thum.x = 18 + this.controlsize*v/(r);
	}
    else
	{
	if (r > 0) this.thum.y = 18 + this.controlsize*v/(r);
	}
    }

function sb_range_changed(p,o,n)
    {
    // do the hokey pokey because 'n' might be a string (silly debug treeview...)
    n = new Number(n);
    n = n.valueOf();

    // Set thumb position and possibly the scrollbar's value
    if (n < 0) n = 0;
    //this.range = n;
    if (this.value > n) this.value = n;
    this.SetThumb(n,this.value);
    return n;
    }

function sb_do_mv()
    {
    var ti=sb_target_img;
    if (ti.kind=='sb' && sb_mv_incr != 0)
	{
	// Determine new offset
	var new_value = ti.mainlayer.value + sb_mv_incr;
	if (new_value > ti.mainlayer.range) new_value = Math.round(ti.mainlayer.range);
	if (new_value < 0) new_value = 0;

	// Set thumb position
	ti.mainlayer.SetThumb(ti.mainlayer.range, new_value);

	// Set offset
	ti.mainlayer.value = Math.round(new_value);
	}
    return true;
    }

function sb_tm_mv()
    {
    sb_do_mv();
    sb_mv_timeout=setTimeout(sb_tm_mv,50);
    return false;
    }
