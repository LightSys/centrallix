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

function wn_init(l,ml,h)
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
    l.orig_height = h;
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

function wn_unset_windowshade()
    {
    wn_clicked = 0;
    }

function wn_windowshade(layer)
    {
    if (wn_clicked == 1)
	{
	wn_clicked = 0;
	if (layer.clip.height != 23)
	    layer.clip.height = 23;
	else
	    layer.clip.height = layer.orig_height;
	}
    else
	{
	wn_clicked = 1;
	setTimeout("wn_unset_windowshade()", 1200);//2sec delay
	}
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
        }
    wn_clicked = 0;
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
    wn_clicked = 0;
    return true;
    }

function wn_bring_top(l)
    {
    if (wn_topwin == l) return true;
    wn_adjust_z(l, wn_top_z - l.zIndex + 4);
    wn_topwin = l;
    wn_clicked = 0;
    }
