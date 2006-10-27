// Copyright (C) 1998-2004 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function sb_init(param)
    {
    var tlayer=null;
    var l = param.layer;
    tlayer = htr_subel(l, param.tname);
    /*for(i=0;i<l.layers.length;i++)
	{
	var ml=l.layers[i];
	if(ml.name==param.tname) tlayer=ml;
	}*/
    var imgs = pg_images(l);
    for(i=0;i<imgs.length;i++)
	{
	var img=imgs[i];
	if(img.name=='d' || img.name=='u' || img.name=='b')
	    {
	    img.pane=l;
	    img.layer = img;
	    img.thum=tlayer;
	    img.kind='sb';
	    img.mainlayer=l;
	    }
	}
    imgs = pg_images(tlayer);
    imgs[0].kind='sb';
    imgs[0].layer = imgs[0];
    imgs[0].mainlayer=l;
    imgs[0].thum=tlayer;
    imgs[0].pane=l;
    tlayer.nofocus = true;
    htr_init_layer(tlayer, l, 'sb');
    htr_init_layer(l, l, 'sb');
    ifc_init_widget(l);
    l.thum = tlayer;
    //l.LSParent = param.parent;
    l.range = param.range;
    l.value = 0;
    l.is_horizontal = param.isHorizontal;
    l.controlsize = param.isHorizontal?(getClipWidth(l) - 18*3):(getClipHeight(l) - 18*3);
    l.sb_range_changed = sb_range_changed;
    htr_watch(l,'range','sb_range_changed');

    // Actions
    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("MoveTo", sb_action_move_to);

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");

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
	if (r > 0) moveTo(this.thum, 18 + this.controlsize*v/(r), 0);
	}
    else
	{
	if (r > 0) moveTo(this.thum, 0, 18 + this.controlsize*v/(r));
	}
    }

function sb_range_changed(p,o,n)
    {
    if(cx__capabilities.Dom0IE)
	{
    	if(e.propertyName.substr(0,6) == "style.")
    	    {
	    n = e.srcElement.style[e.propertyName.substr(6)];
	    }
    	else
	    {
	    n = e.srcElement[e.propertyName];
	    }
	}
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

function sb_mousedown(e)
    {
    sb_target_img=e.target;
    if (sb_target_img != null && sb_target_img.kind=='sb' && (sb_target_img.name=='u' || sb_target_img.name=='d'))
        {
        if (sb_target_img.name=='u') sb_mv_incr=-10; else sb_mv_incr=+10;
        pg_set(sb_target_img,'src',htutil_subst_last(sb_target_img.src,"c.gif"));
        sb_do_mv();
        sb_mv_timeout = setTimeout(sb_tm_mv,300);
        return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    else if (sb_target_img != null && sb_target_img.kind=='sb' && sb_target_img.name=='t')
        {
        sb_click_x = e.pageX;
        sb_click_y = e.pageY;
        sb_thum_x = getPageX(sb_target_img.thum);
        sb_thum_y = getPageY(sb_target_img.thum);
        return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    else if (sb_target_img != null && sb_target_img.kind=='sb' && sb_target_img.name=='b')
        {
        sb_mv_incr=sb_target_img.mainlayer.controlsize + (18*3);
        if (!sb_target_img.mainlayer.is_horizontal && e.pageY < getPageY(sb_target_img.thum)+9) sb_mv_incr = -sb_mv_incr;
        if (sb_target_img.mainlayer.is_horizontal && e.pageX < getPageX(sb_target_img.thum)+9) sb_mv_incr = -sb_mv_incr;
        sb_do_mv();
        sb_mv_timeout = setTimeout(sb_tm_mv,300);
        return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    else sb_target_img = null;
    if (e.kind == 'sb') cn_activate(e.mainlayer, 'MouseDown');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function sb_mousemove(e)
    {
    var ti=sb_target_img;
    if (ti != null && ti.kind=='sb' && ti.name=='t')
        {
        if (ti.mainlayer.is_horizontal)
            {
            var new_x=sb_thum_x + (e.pageX-sb_click_x);
            if (new_x > getPageX(ti.pane)+18+ti.mainlayer.controlsize) new_x=getPageX(ti.pane)+18+ti.mainlayer.controlsize;
            if (new_x < getPageX(ti.pane)+18) new_x=getPageX(ti.pane)+18;
            setPageX(ti.thum,new_x);
            ti.mainlayer.value = Math.round((new_x - (getPageX(ti.pane)+18))*ti.mainlayer.range/ti.mainlayer.controlsize);
            }
        else
            {
            var new_y=sb_thum_y + (e.pageY-sb_click_y);
            if (new_y > getPageY(ti.pane)+18+ti.mainlayer.controlsize) new_y=getPageY(ti.pane)+18+ti.mainlayer.controlsize;
            if (new_y < getPageY(ti.pane)+18) new_y=getPageY(ti.pane)+18;
            setPageY(ti.thum,new_y);
            ti.mainlayer.value = Math.round((new_y - (getPageY(ti.pane)+18))*ti.mainlayer.range/ti.mainlayer.controlsize);
            }
        return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    if (e.kind == 'sb') cn_activate(e.mainlayer, 'MouseMove');
    if (sb_cur_mainlayer && e.kind != 'sb')
        {
        cn_activate(sb_cur_mainlayer, 'MouseOut');
        sb_cur_mainlayer = null;
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function sb_mouseup(e)
    {
    if (sb_mv_timeout != null)
        {
        clearTimeout(sb_mv_timeout);
        sb_mv_timeout = null;
        sb_mv_incr = 0;
        }
    if (sb_target_img != null)
        {
        if (sb_target_img.name != 'b')
            pg_set(sb_target_img,'src',htutil_subst_last(sb_target_img.src,"b.gif"));
        sb_target_img = null;
        }
    if (e.kind == 'sb') cn_activate(e.mainlayer, 'MouseUp');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function sb_mouseover(e)
    {
    if (e.kind == 'sb')
        {
        if (!sb_cur_mainlayer)
            {
            cn_activate(e.mainlayer, 'MouseOver');
            sb_cur_mainlayer = e.mainlayer;
            }
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }


