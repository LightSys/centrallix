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

function wn_init(l,ml,gs,ct,titlebar)
    {
    htr_init_layer(l,l,"wn");
    htr_init_layer(ml,l,"wn");
    /** NS4 version doesn't use a separate div for the title bar **/
    if(cx__capabilities.Dom1HTML)
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
    l.ContentLayer = ml;

    l.orig_width = pg_get_style(l,'clip.width');
    l.orig_height = pg_get_style(l,'clip.height');
    l.orig_right = pg_get_style(l,'clip.right');
    l.orig_left = pg_get_style(l,'clip.left');
    l.orig_bottom = pg_get_style(l,'clip.bottom');
    l.orig_top = pg_get_style(l,'clip.top');

    l.gshade = gs;
    l.closetype = ct;
    l.working = false;
    l.shaded = false;

    /** make sure the images are set up **/
    for(var i=0;i<pg_images(titlebar).length;i++)
	{
	pg_images(titlebar)[i].layer = l;
	pg_images(titlebar)[i].kind = 'wn';
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
	pg_addsched_fn(window, "pg_reveal_event", new Array(l,l,'Reveal'));
	}

    return l;
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
    if (v && (cur_vis != 'inherit'))
	pg_reveal_event(this, v, 'RevealCheck');
    else if (!v && (cur_vis == 'inherit'))
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
    l.clicked = 0;
    }

function wn_windowshade(l)
    {
    if (l.clicked == 1)
	{
	clearTimeout(l.tid);
	l.clicked = 0;
	var duration = 200;
	var speed = 30;
//	st = new Date();
	if (!l.shaded && !l.working)
	    {
	    if (l.gshade)
		{
		var size = Math.ceil((pg_get_style(l,'clip.hieght')-24)*speed/duration);
		l.working = true;
		wn_graphical_shade(l,24,speed,size);
		}
	    else
		{
		pg_set_style(l,'clip.height',24);
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
		pg_set_style(l,'clip.height',l.orig_height);
		}
	    l.shaded = false;
	    }
	}
    else
	{
	l.clicked = 1;
	clearTimeout(l.tid);
	l.tid = setTimeout(wn_unset_windowshade, 500, l);
	}
    }

function wn_manual_unshade(l)
    {
    l.ContentLayer.pageY += l.ContentLayer.clip.top;
    l.ContentLayer.clip.top = 0;
    }

function wn_graphical_shade(l,to,speed,size)
    {
    var height = pg_get_style(l,'clip.height');
    if (to < height)
    	{
	if (height - size < to)
	    {
	    pg_set_style(l,'clip.height',to);
	    l.working = false;
	    return;
	    }
	else 
	    pg_set_style(l,'clip.height',height-size);
	pg_set_style(l.ContentLayer,'clip.top',pg_get_style(l.ContentLayer,'clip.top')+size);
	pg_set_style(l.ContentLayer,'pageY',pg_get_style(l.ContentLayer,'pageY')-size);
	}
    else
        {
	if (height + size > to)
	    {
	    pg_set_style(l,'clip.height',to);
	    l.working = false;
	    return;
	    }
	else 
	    pg_set_style(l,'clip.height',height+size);
	pg_set_style(l.ContentLayer,'clip.top',pg_get_style(l.ContentLayer,'clip.top')-size);
	pg_set_style(l.ContentLayer,'pageY',pg_get_style(l.ContentLayer,'pageY')+size);
	}
    setTimeout(wn_graphical_shade,speed,l,to,speed,size);
    }

function wn_close(l)
    {
    if (l.closetype == 0)
	{
	if(cx__capabilities.Dom2CSS2)
	    {
	    l.style.setProperty('visibility','hidden','');
	    }
	else if(cx__capabilities.Dom0NS)
	    {
	    l.visibility = 'hidden';
	    }
	else
	    {
	    alert("can't close");
	    }
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
		var toX = Math.ceil(l.clip.width/2);
		sizeX = Math.ceil(toX*speed/duration);
		}
	    if (l.closetype & 2)
		{
		var toY = Math.ceil(l.clip.height/2);
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
	l.clip.right -= sizeX;
	l.clip.left += sizeX;
	if (l.clip.width <= 0) var reset = true;
	}
    if (sizeY > 0)
    	{
	l.clip.bottom -= sizeY;
	l.clip.top += sizeY;
	if (l.clip.height <= 0) var reset = true;
	}
    if (reset)
    	{
	    l.visibility = 'hidden';
	    l.clip.width = l.orig_width;
	    l.clip.right = l.orig_right;
	    l.clip.left = l.orig_left;
	    l.clip.height = l.orig_height;
	    l.clip.bottom = l.orig_bottom;
	    l.clip.top = l.orig_top;
	    ft = new Date();
//	    alert(ft-st);
	    if (l.shaded) wn_manual_unshade(l);
	    return;
	}
    setTimeout(wn_graphical_close,speed,l,speed,sizeX,sizeY);
    }

function wn_togglevisibility(aparam)
    {
    if (this.visibility == 'hide')
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
        else if (wn_newx+wn_current.clip.width > window.innerWidth-ha-pg_attract && wn_newx+wn_current.clip.width < window.innerWidth-ha+pg_attract)
		newx = window.innerWidth-ha-pg_get_style(wn_current,'clip.width');
        else if (wn_newx+wn_current.clip.width < 25) newx = 25-pg_get_style(wn_current,'clip.width');
        else if (wn_newx > window.innerWidth-35-ha) newx = window.innerWidth-35-ha;
	else newx = wn_newx;
        if (wn_newy<0) newy = 0;
        else if (wn_newy > window.innerHeight-12-va) newy = window.innerHeight-12-va;
        else if (wn_newy < pg_attract) newy = 0;
        else if (wn_newy+wn_current.clip.height > window.innerHeight-va-pg_attract && wn_newy+wn_current.clip.height < window.innerHeight-va+pg_attract)
		newy = window.innerHeight-va-pg_get_style(wn_current,'clip.height');
        else newy = wn_newy;
        wn_current.moveToAbsolute(newx,newy);
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
            pg_set(e.target,'src','/sys/images/02close.gif');
        else if (
                (cx__capabilities.Dom0NS && e.pageY < ly.mainlayer.pageY + 24) ||
                (cx__capabilities.Dom1HTML && ly.subkind == 'titlebar' )
             )
            {
            wn_current = ly.mainlayer;
            wn_msx = e.pageX;
            wn_msy = e.pageY;
            wn_newx = null;
            wn_newy = null;
            wn_moved = 0;
            wn_windowshade(ly.mainlayer);
            }
        cn_activate(ly.mainlayer, 'MouseDown');
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function wn_mouseup(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (e.target != null && e.target.name == 'close' && e.target.kind == 'wn')
        {
        pg_set(e.target,'src','/sys/images/01close.gif');
	ly.mainlayer.SetVisibilityTH(false);
        }
    else if (ly.document != null && pg_images(ly).length > 6 && pg_images(ly)[6].name == 'close')
        {
        pg_set(pg_images(ly)[6],'src','/sys/images/01close.gif');
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
        clearTimeout(ly.mainlayer.tid);
        if (wn_newx == null)
            {
            wn_newx = wn_current.pageX + e.pageX-wn_msx;
            wn_newy = wn_current.pageY + e.pageY-wn_msy;
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




