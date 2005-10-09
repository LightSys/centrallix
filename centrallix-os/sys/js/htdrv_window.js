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

function wn_init(param)
    {
    var l = param.layer;
    var titlebar = param.titlebar;
    htr_init_layer(l,l,"wn");
    htr_init_layer(param.mainlayer,l,"wn");
    /** NS4 version doesn't use a separate div for the title bar **/
    if(cx__capabilities.Dom1HTML && titlebar)
	{
	htr_init_layer(titlebar,l,"wn");
	titlebar.subkind = 'titlebar';
	l.titlebar = titlebar;
	}
    else
	{
	titlebar=l;
	}
    l.keep_kbd_focus = true;
    l.oldwin=window_current;
    window_current=l;
    l.osrc = new Array();
    var t = osrc_current;
    while(t)
	{
	l.osrc.push(t);
	t=t.oldosrc;
	}
    l.ContentLayer = param.mainlayer;
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
    l.working = false;
    l.shaded = false;

    /** make sure the images are set up **/
    l.has_titlebar = 0;
    for(var i=0;i<pg_images(titlebar).length;i++)
	{
	pg_images(titlebar)[i].layer = titlebar;
	pg_images(titlebar)[i].kind = 'wn';
	if (pg_images(titlebar)[i].name == 'close')
	    l.has_titlebar = 1;
	}

    wn_bring_top(l);
    l.ActionSetVisibility = wn_setvisibility;
    l.ActionToggleVisibility = wn_togglevisibility;
    l.RegisterOSRC = wn_register_osrc;

    // Register as a triggerer of reveal/obscure events
    l.SetVisibilityBH = wn_setvisibility_bh;
    l.SetVisibilityTH = wn_setvisibility_th;
    l.Reveal = wn_cb_reveal;
    pg_reveal_register_triggerer(l);
    if (htr_getvisibility(l) == 'inherit')
	{
	pg_addsched_fn(window, "pg_reveal_event", new Array(l,l,'Reveal'), 0);
	}

    // Show container API
    l.showcontainer = wn_showcontainer;

    return l;
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
    return true;
    }

// Called when our reveal/obscure request has been acted upon.
// context 'c' == whether to be visible (true) or not (false).
function wn_cb_reveal(e)
    {
    if ((e.eventName == 'RevealOK' && e.c == true) || (e.eventName == 'ObscureOK' && e.c == false))
	this.SetVisibilityBH(e.c);
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
    return;
    }

// Bottom Half of set visibility routine - after obscure/reveal checks.
// this is where we really close it or make it visible.
function wn_setvisibility_bh(v)
    {
    if (!v)
	{
	pg_reveal_event(this, v, 'Obscure');
	wn_close(this);
	}
    else
	{
	pg_reveal_event(this, v, 'Reveal');
	wn_bring_top(this);
	htr_setvisibility(this,'inherit');
	}
    }

function wn_register_osrc(t)
    {
    this.osrc.push(t);
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
    if (l.closetype == 0)
	{
	htr_setvisibility(l,'hidden');
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
    if (htr_getvisibility(this) != 'inherit')
	{
	//aparam.IsVisible = 1;
	//this.ActionSetVisibility(aparam);
	this.SetVisibilityTH(true);
	}
    else
	{
	//this.visibility = 'hide';
	this.SetVisibilityTH(false);
	}
    }

function wn_setvisibility(aparam)
    {
    if (aparam.IsVisible == null || aparam.IsVisible == 1 || aparam.IsVisible == '1')
	{
	/*wn_bring_top(this);
	this.visibility = 'inherit';
	if(!(aparam.NoInit && aparam.NoInit!=false && aparam.NoInit!=0))
	    {
	    for (var t in this.osrc)
		this.osrc[t].InitQuery();
	    }*/
	this.SetVisibilityTH(true);
	}
    else
	{
	this.SetVisibilityTH(false);
	//this.visibility = 'hidden';
	}
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
    if (zi < 0) l.zIndex += zi;
    /*
    for(i=0;i<l.document.layers.length;i++)
	{
	//wn_adjust_z(l.document.layers[i],zi);
	}
    */
    if (zi > 0) l.zIndex += zi;
    if (l.zIndex > wn_top_z) wn_top_z = l.zIndex;
//    wn_clicked = 0;
    return true;
    }

function wn_bring_top(l)
    {
    if (wn_topwin == l) return true;
    wn_adjust_z(l, wn_top_z - l.zIndex + 4);
    wn_topwin = l;
//    wn_clicked = 0;
    }

// FIXME: does this MOUSEDOWN work if for NS4 if there is no title?
function wn_mousedown(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.kind == 'wn')
        {
        if (e.target.name == 'close')
            pg_set(e.target,'src','/sys/images/02bigclose.gif');
        else if ((ly.mainlayer.has_titlebar && cx__capabilities.Dom0NS && e.pageY < ly.mainlayer.pageY + 24) ||
                (cx__capabilities.Dom1HTML && ly.subkind == 'titlebar' ))
            {
            wn_current = ly.mainlayer;
            wn_msx = e.pageX;
            wn_msy = e.pageY;
            wn_newx = null;
            wn_newy = null;
            wn_moved = 0;
	    if (!cx__capabilities.Dom0IE) wn_windowshade_ns_moz(ly.mainlayer);
	    return EVENT_CONTINUE | EVENT_PREVENT_DEFAULT_ACTION;
	    }
        cn_activate(ly.mainlayer, 'MouseDown');
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function wn_dblclick(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.kind == 'wn')
	{
        if ((ly.mainlayer.has_titlebar && cx__capabilities.Dom0NS && e.pageY < ly.mainlayer.pageY + 24) ||
                (cx__capabilities.Dom1HTML && ly.subkind == 'titlebar' ))
            {
            if (cx__capabilities.Dom0IE) wn_windowshade_ie(ly.mainlayer);
            }
	}
    }

function wn_mouseup(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (e.target != null && e.target.name == 'close' && e.target.kind == 'wn')
        {
        pg_set(e.target,'src','/sys/images/01bigclose.gif');
	ly.mainlayer.SetVisibilityTH(false);
        }
    else if (ly.document != null && pg_images(ly).length > 6 && pg_images(ly)[6].name == 'close')
        {
        pg_set(pg_images(ly)[6],'src','/sys/images/01bigclose.gif');
        }
    if (wn_current != null)
        {
        if (wn_moved == 0) wn_bring_top(wn_current);
        }
    if (ly.kind == 'wn') cn_activate(ly.mainlayer, 'MouseUp');
    wn_current = null;
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function wn_mousemove(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.kind == 'wn') cn_activate(ly.mainlayer, 'MouseMove');
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
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.kind == 'wn') cn_activate(ly.mainlayer, 'MouseOver');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function wn_mouseout(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.kind == 'wn') cn_activate(ly.mainlayer, 'MouseOut');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

