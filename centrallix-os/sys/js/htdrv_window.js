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

//$(".wn")

var wn_popped = {};

function wn_deinit()
    {
    // If we moved it to top level, move it back so it can get cleaned up.
    if (this.orig_parent)
	this.orig_parent.appendChild(this);

    // Remove references
    for(var i in wn_list)
	if (wn_list[i] == this)
	    {
	    wn_list.splice(i,1);
	    break;
	    }
    if (wn_current == this)
	wn_current = null;
    if (wn_topwin == this)
	wn_topwin = null;
    }

function wn_init(param)
    {
    var l = param.mainlayer;
    var titlebar = param.titlebar;
    htr_init_layer(l,l,"wn");
    htr_init_layer(param.clayer,l,"wn");
    ifc_init_widget(l);
    l.destroy_widget = wn_deinit;

    if (titlebar)
	{
	htr_init_layer(titlebar,l,"wn");
	titlebar.subkind = 'titlebar';
	}
    else
	titlebar = l;

    l.keep_kbd_focus = true;
    l.ContentLayer = param.clayer;
    l.ContentLayer.maxheight = l.ContentLayer.minheight = getClipHeight(l.ContentLayer);
    l.ContentLayer.maxwidth = l.ContentLayer.minwidth = getClipWidth(l.ContentLayer);

    l.orig_width = pg_get_style(l,'clip.width');
    l.orig_height = pg_get_style(l,'clip.height');
    l.orig_right = pg_get_style(l,'clip.right');
    l.orig_left = pg_get_style(l,'clip.left');
    l.orig_bottom = pg_get_style(l,'clip.bottom');
    l.orig_top = pg_get_style(l,'clip.top');

    l.gshade = param.gshade;
    l.closetype = param.closetype;
    l.is_modal = param.modal;
    l.working = false;
    l.shaded = false;
    l.loaded = false;

    /** make sure the images are set up **/
    l.has_titlebar = 0;
    for(var i=0;i<pg_images(titlebar).length;i++)
	{
	pg_images(titlebar)[i].layer = titlebar;
	pg_images(titlebar)[i].kind = 'wn';
	if (pg_images(titlebar)[i].name == 'close')
	    l.has_titlebar = 1;
	}

    l.orig_parent = null;
    l.is_toplevel = false;
    if (param.toplevel == 1)
	{
	var tl = pg_toplevel_layer(l);
	if (tl && tl != window && tl != document && tl != l)
	    {
	    l.orig_parent = l.parentNode;
	    l.is_toplevel = true;
	    var x = getPageX(l);
	    var y = getPageY(l);
	    moveAbove(l, tl);
	    moveToAbsolute(l, x, y);
	    }
	}
    
    wn_list.push(l);
    wn_bring_top(l);

    // Actions
    l.ifcProbeAdd(ifAction).Add("SetVisibility", wn_setvisibility);
    l.ifcProbe(ifAction).Add("ToggleVisibility", wn_togglevisibility);
    l.ifcProbe(ifAction).Add("Open", wn_openwin);
    l.ifcProbe(ifAction).Add("Close", wn_closewin);
    l.ifcProbe(ifAction).Add("Popup", wn_popup);

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");
    ie.Add("Load");
    ie.Add("Open");
    ie.Add("Close");

    // Register as a triggerer of reveal/obscure events
    l.SetVisibilityBH = wn_setvisibility_bh;
    l.SetVisibilityTH = wn_setvisibility_th;
    l.Reveal = wn_cb_reveal;
    l.wn_popup_bh = wn_popup_bh;
    pg_reveal_register_triggerer(l);
    pg_reveal_register_listener(l);

    l.is_visible = 0;

    if (htr_getvisibility(l) == 'inherit' || htr_getvisibility(l) == 'visible')
	{
	l.is_visible = 1;
	pg_addsched_fn(window, "pg_reveal_event", [l,l,'Reveal'], 0);
	}

    // Show container API
    l.showcontainer = wn_showcontainer;

    if (l.is_modal && l.is_visible) pg_setmodal(l);

    return l;
    }

// Popup - pops up a window in the way that a menu might pop up.
function wn_popup(aparam)
    {
    var pop_to = null;
    var pop_to_x = 0;
    var pop_to_y = 0;
    var pop_to_height = 0;
    var pop_to_width = 0;

    for (var w in wn_popped)
	{
	wn_popped[w].ifcProbe(ifAction).Invoke("Close", {});
	}

    if (aparam.PopTo)
	{
	if (wgtrIsNode(aparam.PopTo))
	    pop_to = aparam.PopTo;
	else 
	    pop_to = wgtrGetNode(this, aparam.PopTo);
	}
    if (pop_to)
	{
	var geom = wgtrGetGeom(pop_to);
	if (geom)
	    {
	    pop_to_x = geom.x;
	    pop_to_y = geom.y;
	    pop_to_width = geom.width;
	    pop_to_height = geom.height;
	    }
	}
    if (aparam.X) pop_to_x = parseInt(aparam.X);
    if (aparam.Y) pop_to_y = parseInt(aparam.Y);
    if (aparam.Height) pop_to_height = parseInt(aparam.Height);
    if (aparam.Width) pop_to_width = parseInt(aparam.Width);

    if (aparam.OffsetX) pop_to_x += parseInt(aparam.OffsetX);
    if (aparam.OffsetY) pop_to_y += parseInt(aparam.OffsetY);

    if (pop_to_x || pop_to_y)
	pg_positionpopup(this, pop_to_x, pop_to_y, pop_to_height, pop_to_width);

    if (aparam.ExtendTo)
	this.extended_region = wgtrGetGeom(aparam.ExtendTo);

    this.popped_above = wgtrGlobalFindContainer(wgtrGetParent(this), "widget/childwindow");
    if (this.popped_above)
	this.popped_above.has_popup = this;

    wn_popped[this.id] = this;
    return this.ifcProbe(ifAction).Invoke('Open',aparam);
    //pg_addsched_fn(this, 'wn_popup_bh', [aparam], 0);
    }

function wn_popup_bh(aparam)
    {
    return this.ifcProbe(ifAction).Invoke('Open',aparam);
    }

// Called when we need to make sure the container is visible, typically
// because a widget within it needs to have keyboard focus.  In the case
// of a window, we un-shade it, make it visible, and bring it to the
// top.
function wn_showcontainer()
    {
    if (htr_getvisibility(this) != 'inherit')
	this.SetVisibilityTH(true);
    if (this.shaded)
	wn_windowshade(this);
    wn_bring_top(this);
    if (this.is_toplevel) return false; /* prevent bubble up if made toplevel */
    return true;
    }

// Called when our reveal/obscure request has been acted upon.
// context 'c' == whether to be visible (true) or not (false).
function wn_cb_reveal(e)
    {
    if ((e.eventName == 'RevealOK' && e.c == true) || (e.eventName == 'ObscureOK' && e.c == false))
	this.SetVisibilityBH(e.c);
    if (e.eventName == 'Reveal' && htr_getvisibility(this) == 'inherit')
	{
	if (!this.loaded)
	    {
	    this.loaded = true;
	    this.ifcProbe(ifEvent).Activate("Load", {});
	    }
	this.ifcProbe(ifEvent).Activate("Open", {});
	}
    return true;
    }

// Top Half of set visibility routine - before obscure/reveal checks.
function wn_setvisibility_th(v)
    {
    var cur_vis = htr_getvisibility(this);
    if (v && (cur_vis != 'inherit' && cur_vis != 'visible'))
	pg_reveal_event(this, v, 'RevealCheck');
    else if (!v && (cur_vis == 'inherit' || cur_vis == 'visible'))
	pg_reveal_event(this, v, 'ObscureCheck');
    else if (v)
	wn_bring_top(this);
    return;
    }

// Bottom Half of set visibility routine - after obscure/reveal checks.
// this is where we really close it or make it visible.
function wn_setvisibility_bh(v)
    {
    if (!v)
	{
	pg_reveal_event(this, v, 'Obscure');
	this.ifcProbe(ifEvent).Activate("Close", {});
	wn_close(this);
	}
    else
	{
	pg_reveal_event(this, v, 'Reveal');
	if (!this.loaded)
	    {
	    this.loaded = true;
	    this.ifcProbe(ifEvent).Activate("Load", {});
	    }
	if (this.do_cascade && wn_topwin && getPageX(this) == getPageX(wn_topwin) && getPageY(this) == getPageY(wn_topwin) && wn_topwin != this)
	    moveBy(this, 16, 16);
	wn_bring_top(this);
	htr_setvisibility(this,'inherit');
	this.is_visible = 1;
	if (this.is_modal) pg_setmodal(this);
	this.ifcProbe(ifEvent).Activate("Open", {});
	}
    }

function wn_unset_windowshade(l)
    {
    if (cx__capabilities.Dom0IE)
        {
        l = wn_layer;
        }
    l.clicked = 0;
    l.tid = null;
    }

function wn_windowshade_ie(l)
    {
    wn_windowshade(l);
    }

function wn_windowshade_ns_moz(l)
    {
    if (l.clicked == 1)
	{
	if (l.tid) clearTimeout(l.tid);
	l.tid = null;
	l.clicked = 0;
	wn_windowshade(l);
	}
    else
	{
	l.clicked = 1;
	if (l.tid) clearTimeout(l.tid);
	l.tid = setTimeout(wn_unset_windowshade, 500, l);
	}
    }

function wn_windowshade(l)
    {
    // for IE we use the dbl click event, for NS and Moz we use two mousedown's
    var duration = 200;
    var speed = 30;
//  st = new Date();
    var boxoffset = cx__capabilities.CSSBox?2:0;
    if (!l.shaded && !l.working)
	{
	if (l.gshade)
	    {
	    var size = Math.ceil((getClipHeight(l)-24)*speed/duration);
	    l.working = true;
	    wn_graphical_shade(l,24,speed,size);
	    }
	else
	    {
	    setClipHeight(l, 24);
	    resizeTo(l, getClipWidth(l)-boxoffset, 24);
	    }
	l.shaded = true;
	}
    else if (!l.working)
	{
	if (l.gshade)
	    {
	    var size = Math.ceil((l.orig_height-24)*speed/duration);
	    l.working = true;
	    wn_graphical_shade(l,l.orig_height,speed,size);
	    }
	else
	    {
	    setClipHeight(l, l.orig_height);
	//	resizeTo(l, getClipWidth(l)+2, l.orig_height+2);
	//    else
		resizeTo(l, getClipWidth(l)-boxoffset, l.orig_height-boxoffset);
	    }
	l.shaded = false;
	}
    }

function wn_manual_unshade(l)
    {
    moveBy(l.ContentLayer, 0, getClipTop(l.ContentLayer));
    setClipTop(l.ContentLayer, 0);
    }

function wn_graphical_shade(l,to,speed,size)
    {
    var height = getClipHeight(l);
    if (to < height)
    	{
	if (height - size < to)
	    {
	    setClipHeight(l, to);
	    l.working = false;
	    return;
	    }
	else
	    setClipHeight(l, height - size);
	setClipTop(l.ContentLayer, getClipTop(l.ContentLayer) + size);
	moveBy(l.ContentLayer, 0, -size);
	}
    else
        {
	if (height + size > to)
	    {
	    setClipHeight(l, to);
	    l.working = false;
	    return;
	    }
	else
	    setClipHeight(l, height + size);
	setClipTop(l.ContentLayer, getClipTop(l.ContentLayer) - size);
	moveBy(l.ContentLayer, 0, size);
	}
    setTimeout(wn_graphical_shade,speed,l,to,speed,size);
    }

function wn_close(l)
    {
    if (l.is_modal) pg_setmodal(null);
    if (wn_popped[l.id]) delete wn_popped[l.id];
    //l.is_modal = false;
    l.no_close = false;
    l.extended_region = null;
    if (l.popped_above)
	{
	l.popped_above.has_popup = null;
	l.popped_above = null;
	}
    if (l.has_popup)
	{
	wn_close(l.has_popup);
	}
    if (l.closetype == 0 || !cx__capabilities.Dom0NS)
	{
	htr_setvisibility(l,'hidden');
	l.is_visible = 0;
	}
    else
        {
	if(cx__capabilities.Dom0NS)
	    {
	    st = new Date();
	    var speed = 20;
	    var duration = 150;
	    var sizeX = 0;
	    var sizeY = 0;
	    if (l.closetype & 1)
		{
		var toX = Math.ceil(getClipWidth(l)/2);
		sizeX = Math.ceil(toX*speed/duration);
		}
	    if (l.closetype & 2)
		{
		var toY = Math.ceil(getClipHeight(l)/2);
		sizeY = Math.ceil(toY*speed/duration);
		}
	    wn_graphical_close(l,speed,sizeX,sizeY);
	    }
	else
	    {
	    alert("close type " + l.closetype + " is not implimented for this browser");
	    }
	}
    }

function wn_graphical_close(l,speed,sizeX,sizeY)
    {
    if (sizeX > 0)
    	{
	setClipRight(l, getClipRight(l) - sizeX);
	setClipLeft(l, getClipLeft(l) + sizeX);
	if (getClipWidth(l) <= 0) var reset = true;
	}
    if (sizeY > 0)
    	{
	setClipBottom(l, getClipBottom(l) - sizeY);
	setClipTop(l, getClipTop(l) + sizeY);
	if (getClipHeight(l)<= 0) var reset = true;
	}
    if (reset)
    	{
	    l.visibility = 'hidden';
	    l.is_visible = 0;
	    setClipWidth(l, l.orig_width);
	    setClipRight(l, l.orig_right);
	    setClipLeft(l, l.orig_left);
	    setClipHeight(l, l.orig_height);
	    setClipBottom(l, l.orig_bottom);
	    setClipTop(l, l.orig_top);
	    ft = new Date();
	    if (l.shaded) wn_manual_unshade(l);
	    return;
	}
    setTimeout(wn_graphical_close,speed,l,speed,sizeX,sizeY);
    }

function wn_togglevisibility(aparam)
    {
    var vis = htr_getvisibility(this);
    if (vis != 'inherit' && vis != 'visible')
	{
	this.SetVisibilityTH(true);
	}
    else
	{
	this.SetVisibilityTH(false);
	}
    }

function wn_closewin(aparam)
    {
    aparam.IsVisible = 0;
    return this.ifcProbe(ifAction).Invoke('SetVisibility',aparam);
    }

function wn_openwin(aparam)
    {
    aparam.IsVisible = 1;
    return this.ifcProbe(ifAction).Invoke('SetVisibility',aparam);
    }

function wn_setvisibility(aparam)
    {
    if (aparam.IsVisible == null || aparam.IsVisible == 1 || aparam.IsVisible == '1' || aparam.IsVisible == true)
	{
	if (typeof aparam.IsModal != 'undefined') this.is_modal = aparam.IsModal;
	this.no_close = aparam.NoClose;
	this.do_cascade = aparam.Cascade;
	this.SetVisibilityTH(true);
	}
    else
	{
	this.SetVisibilityTH(false);
	}
    }

function wn_domove2() 
    {
    
    }

function wn_domove()
    {
    if (wn_current != null)
        {
        var ha=(document.height-window.innerHeight-2)>=0?15:0;
        var va=(document.width-window.innerWidth-2)>=0?15:0;
        var newx,newy;
        if (wn_newx < pg_attract && wn_newx > -pg_attract) newx = 0;
        else if (wn_newx+getClipWidth(wn_current) > window.innerWidth-ha-pg_attract && wn_newx+ getClipWidth(wn_current) < window.innerWidth-ha+pg_attract)
		newx = window.innerWidth-ha-pg_get_style(wn_current,'clip.width');
        else if (wn_newx+getClipWidth(wn_current) < 25) newx = 25-pg_get_style(wn_current,'clip.width');
        else if (wn_newx > window.innerWidth-35-ha) newx = window.innerWidth-35-ha;
	else newx = wn_newx;
        if (wn_newy<0) newy = 0;
        else if (wn_newy > window.innerHeight-12-va) newy = window.innerHeight-12-va;
        else if (wn_newy < pg_attract) newy = 0;
        else if (wn_newy+getClipHeight(wn_current) > window.innerHeight-va-pg_attract && wn_newy+getClipHeight(wn_current) < window.innerHeight-va+pg_attract)
		newy = window.innerHeight-va-pg_get_style(wn_current,'clip.height');
        else newy = wn_newy;
        moveToAbsolute(wn_current,newx,newy);
    	wn_current.clicked = 0;
        }
    return true;
    }

function wn_adjust_z(l,zi)
    {
    var cur_z = htr_getzindex(l);
    if (zi && (typeof cur_z != undefined))
	{
	cur_z += zi;
	htr_setzindex(l,cur_z);
	}
    if (cur_z > wn_top_z) wn_top_z = cur_z;
    return true;
    }

function wn_bring_top(l)
    {
    if (wn_topwin == l) return true;
    wn_adjust_z(l, wn_top_z - htr_getzindex(l) + 4);
    wn_topwin = l;
    if (l.has_popup)
	wn_bring_top(l.has_popup);
    }

// FIXME: does this MOUSEDOWN work if for NS4 if there is no title?
function wn_mousedown(e)
    {
    for (var w in wn_popped)
	{
	var wn = wn_popped[w];
	var pgx = getPageX(wn);
	var pgy = getPageY(wn);
	if ((!(e.pageX >= pgx && e.pageX < pgx + getClipWidth(wn) && e.pageY >= pgy && e.pageY < pgy + getClipHeight(wn))) && (!wn.extended_region || (!(e.pageX >= wn.extended_region.x && e.pageX < wn.extended_region.x + wn.extended_region.width && e.pageY >= wn.extended_region.y && e.pageY < wn.extended_region.y + wn.extended_region.width))))
	    {
	    wn.ifcProbe(ifAction).Invoke('SetVisibility',{IsVisible:0});
	    }
	}
    if (e.kind == 'wn')
        {
        if (e.target.name == 'close')
            pg_set(e.target,'src','/sys/images/02bigclose.gif');
        else if ((e.mainlayer.has_titlebar && cx__capabilities.Dom0NS && e.pageY < e.mainlayer.pageY + 24) ||
                (cx__capabilities.Dom1HTML && e.layer.subkind == 'titlebar' ))
            {
            wn_current = e.mainlayer;
            wn_msx = e.pageX;
            wn_msy = e.pageY;
            wn_newx = null;
            wn_newy = null;
            wn_moved = 0;
	    if (!cx__capabilities.Dom0IE) wn_windowshade_ns_moz(e.mainlayer);
	    return EVENT_CONTINUE | EVENT_PREVENT_DEFAULT_ACTION;
	    }
        cn_activate(e.mainlayer, 'MouseDown');
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function wn_dblclick(e)
    {
    if (e.kind == 'wn')
	{
        if ((e.mainlayer.has_titlebar && cx__capabilities.Dom0NS && e.pageY < e.mainlayer.pageY + 24) ||
                (cx__capabilities.Dom1HTML && e.layer.subkind == 'titlebar' ))
            {
            if (cx__capabilities.Dom0IE) wn_windowshade_ie(e.mainlayer);
            }
	}
    }

function wn_mouseup(e)
    {
    if (e.target != null && e.target.name == 'close' && e.target.kind == 'wn')
        {
        pg_set(e.target,'src','/sys/images/01bigclose.gif');
	if (e.mainlayer.no_close != true) e.mainlayer.SetVisibilityTH(false);
        }
    else if (e.layer.document != null && pg_images(e.layer).length > 6 && pg_images(e.layer)[6].name == 'close')
        {
        pg_set(pg_images(e.layer)[6],'src','/sys/images/01bigclose.gif');
        }
    if (wn_current != null)
        {
        if (wn_moved == 0) wn_bring_top(wn_current);
        }
    if (e.kind == 'wn') cn_activate(e.mainlayer, 'MouseUp');
    wn_current = null;
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function wn_mousemove(e)
    {
    if (e.kind == 'wn') cn_activate(e.mainlayer, 'MouseMove');
    if (wn_current != null)
        {
        wn_current.clicked = 0;
	if (wn_current.tid) clearTimeout(wn_current.tid);
	wn_current.tid = null;
        if (wn_newx == null)
            {
            wn_newx = getPageX(wn_current) + e.pageX-wn_msx;
            wn_newy = getPageY(wn_current) + e.pageY-wn_msy;
            }
        else
            {
            wn_newx += (e.pageX - wn_msx);
            wn_newy += (e.pageY - wn_msy);
            }
        setTimeout(wn_domove,60);
        wn_moved = 1;
        wn_msx = e.pageX;
        wn_msy = e.pageY;
        return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function wn_mouseover(e)
    {
    if (e.kind == 'wn') cn_activate(e.mainlayer, 'MouseOver');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function wn_mouseout(e)
    {
    if (e.kind == 'wn') cn_activate(e.mainlayer, 'MouseOut');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }


// Load indication
if (window.pg_scripts) pg_scripts['htdrv_window.js'] = true;
