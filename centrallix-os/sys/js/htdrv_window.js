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

function wn_init(l,ml,gs,ct)
    {
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
    l.document.layer = l;
    l.mainlayer = l;
    l.ContentLayer = ml;
    ml.document.layer = ml;
    ml.mainlayer = l;
    l.orig_width = l.clip.width;
    l.orig_right = l.clip.right;
    l.orig_left = l.clip.left;
    l.orig_height = l.clip.height;
    l.orig_bottom = l.clip.bottom;
    l.orig_top = l.clip.top;
    l.gshade = gs;
    l.closetype = ct;
    l.working = false;
    l.shaded = false;
    l.kind = 'wn';
    ml.kind = 'wn';
    for(i=0;i<l.document.images.length;i++)
	{
	l.document.images[i].layer = l;
	l.document.images[i].kind = 'wn';
	}
    wn_bring_top(l);
    l.ActionSetVisibility = wn_setvisibility;
    l.ActionToggleVisibility = wn_togglevisibility;
    l.RegisterOSRC = wn_register_osrc;
    return l;
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
		var size = Math.ceil((l.clip.height-24)*speed/duration);
		l.working = true;
		wn_graphical_shade(l,24,speed,size);
		}
	    else
		{
		l.clip.height = 24;
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
		l.clip.height = l.orig_height;
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
    if (to < l.clip.height)
    	{
	if (l.clip.height - size < to)
	    {
	    l.clip.height = to;
//	    ft = new Date();
//	    alert(ft-st);
	    l.working = false;
	    return;
	    }
	else l.clip.height-=size;
	l.ContentLayer.clip.top +=size;
	l.ContentLayer.pageY -= size;
	}
    else
        {
	if (l.clip.height + size > to)
	    {
	    l.clip.height = to;
//	    ft=new Date();
//	    alert(ft-st);
	    l.working = false;
	    return;
	    }
	else l.clip.height+=size;
	l.ContentLayer.pageY += size;
	l.ContentLayer.clip.top -= size;
	}
    setTimeout(wn_graphical_shade,speed,l,to,speed,size);
    }

function wn_close(l,type)
    {
    if (l.closetype == 0) l.visibility = 'hidden';
    else
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
	aparam.IsVisible = 1;
	this.ActionSetVisibility(aparam);
	}
    else
	{
	this.visibility = 'hide';
	}
    }

function wn_setvisibility(aparam)
    {
    if (aparam.IsVisible == null || aparam.IsVisible == 1 || aparam.IsVisible == '1')
	{
	wn_bring_top(this);
	this.visibility = 'inherit';
	if(!(aparam.NoInit && aparam.NoInit!=false && aparam.NoInit!=0))
	    {
	    for (var t in this.osrc)
		this.osrc[t].InitQuery();
	    }
	}
    else
	{
	this.visibility = 'hidden';
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
		newx = window.innerWidth-ha-wn_current.clip.width;
        else if (wn_newx+wn_current.clip.width < 25) newx = 25-wn_current.clip.width;
        else if (wn_newx > window.innerWidth-35-ha) newx = window.innerWidth-35-ha;
	else newx = wn_newx;
        if (wn_newy<0) newy = 0;
        else if (wn_newy > window.innerHeight-12-va) newy = window.innerHeight-12-va;
        else if (wn_newy < pg_attract) newy = 0;
        else if (wn_newy+wn_current.clip.height > window.innerHeight-va-pg_attract && wn_newy+wn_current.clip.height < window.innerHeight-va+pg_attract)
		newy = window.innerHeight-va-wn_current.clip.height;
        else newy = wn_newy;
        wn_current.moveToAbsolute(newx,newy);
    	wn_current.clicked = 0;
        }
    return true;
    }

function wn_adjust_z(l,zi)
    {
    if (zi < 0) l.zIndex += zi;
    for(i=0;i<l.document.layers.length;i++)
	{
	//wn_adjust_z(l.document.layers[i],zi);
	}
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
