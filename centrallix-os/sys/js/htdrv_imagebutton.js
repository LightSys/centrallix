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

function ib_enable()
    {
    this.enabled=true;
    ib_setmode(this, 'n');
    }

function ib_disable()
    {
    this.enabled=false;
    ib_setmode(this, 'd');
    }

function ib_setenable(prop,oldval,newval)
    {
    var e = window.event;    
    var source = new Object();    
    (cx__capabilities.Dom0IE)? source = e.srcElement : source = this;
    if (newval == oldval) return newval;
    
    if (newval)
	ib_setmode(source, 'n');
    else
	ib_setmode(source, 'd');
    return newval;
    }

function ib_setmode(ly, mode)
    {
    var newsrc = ly.cursrc;
    switch(mode)
        {
	case 'n':
	    newsrc = ly.nImage.src;
	    break;
	case 'p':
	    newsrc = ly.pImage.src;
	    break;
	case 'c':
	    newsrc = ly.cImage.src;
	    break;
	case 'd':
	    newsrc = ly.dImage.src;
	    break;
	}
    if (newsrc == ly.cursrc) return;
    ly.cursrc = newsrc;
    ly.curmode = mode;
    pg_set(ly.img, 'src', newsrc);
    }

function ib_trigger()
    {
    if (this.do_repeat)
	{
	if (!this.trigger_schedid)
	    {
	    this.trigger_schedid = pg_addsched_fn(this, 'trigger', [], 500);
	    }
	else
	    {
	    this.trigger_schedid = pg_addsched_fn(this, 'trigger', [], 200);
	    }
	}
    cn_activate(this, 'MouseDown');
    }

function ib_mousedown(e)
    {
    if (e.kind=='ib' && e.mainlayer.enabled==true)
        {
	ib_setmode(e.mainlayer, 'c');
	e.mainlayer.trigger();
        ib_cur_img = e.mainlayer.img;
        }
    return EVENT_ALLOW_DEFAULT_ACTION | EVENT_CONTINUE;
    }

function ib_mouseup(e)
    {
    if (ib_cur_img)
        {
	var ly = ib_cur_img.layer;
        if (e.pageX >= getPageX(ly) &&
            e.pageX < getPageX(ly) + getClipWidth(ly) &&
            e.pageY >= getPageY(ly) &&
            e.pageY < getPageY(ly) + getClipHeight(ly))
            {
            cn_activate(e.mainlayer, 'Click');
            cn_activate(e.mainlayer, 'MouseUp');
	    if (ly.curmode != 'd') ib_setmode(ly, 'p');
            }
        else
            {
	    if (ly.curmode != 'd') ib_setmode(ly, 'n');
            }
	if (ly.trigger_schedid)
	    pg_delsched(ly.trigger_schedid);
	ly.trigger_schedid = null;
        ib_cur_img = null;
        }
    return EVENT_ALLOW_DEFAULT_ACTION | EVENT_CONTINUE;
    }

function ib_mousemove(e)
    {
    if (e.kind == 'ib' && e.mainlayer.enabled == true)
        {
        if (e.mainlayer.img && e.mainlayer.img.src != e.mainlayer.cImage.src) ib_setmode(e.mainlayer, 'p'); 
	if (e.mainlayer.tipid)
	    {
	    pg_canceltip(e.mainlayer.tipid);
	    e.mainlayer.tipid = pg_tooltip(e.mainlayer.tooltip, e.pageX, e.pageY);
	    }
        cn_activate(e.mainlayer, 'MouseMove');
        }
    return EVENT_ALLOW_DEFAULT_ACTION | EVENT_CONTINUE;
    }

function ib_mouseover(e)
    {
    if (e.kind == 'ib' && e.mainlayer.enabled == true)
        {
        if (e.mainlayer.img && (e.mainlayer.img.src != e.mainlayer.cImage.src)) ib_setmode(e.mainlayer, 'p');
	if (e.mainlayer.tooltip)
	    e.mainlayer.tipid = pg_tooltip(e.mainlayer.tooltip, e.pageX, e.pageY);
        cn_activate(e.mainlayer, 'MouseOver');
        }
    return EVENT_ALLOW_DEFAULT_ACTION | EVENT_CONTINUE;
    }

function ib_mouseout(e)
    {
    if (e.kind == 'ib' && e.mainlayer.enabled == true)
        {
        if (e.mainlayer.img && e.mainlayer.img.src != e.mainlayer.cImage.src) ib_setmode(e.mainlayer, 'n'); 
	if (e.mainlayer.tipid)
	    pg_canceltip(e.mainlayer.tipid);
	e.mainlayer.tipid = null;
        cn_activate(e.mainlayer.layer, 'MouseMove');
        }
    return EVENT_ALLOW_DEFAULT_ACTION | EVENT_CONTINUE;
    }

function ib_init(param)
    {
    var l = param.layer;
    var w = param.width;
    var h = param.height;
    l.tooltip = param.tooltip;
    l.do_repeat = param.repeat;
    //l.LSParent = param.parentobj;
    l.nofocus = true;
    if(cx__capabilities.Dom0NS)
	{
	l.img = l.document.images[0];
	}
    else if(cx__capabilities.Dom1HTML)
	{
	l.img = l.getElementsByTagName("img")[0];
	}
    else
	{
	alert('browser not supported');
	}
    htr_init_layer(l,l, 'ib');
    l.layer = l;
    ifc_init_widget(l);
    l.img.layer = l;
    l.img.mainlayer = l;
    l.img.kind = 'ib';
    l.cursrc = param.n;
    setClipWidth(l, w);

    l.trigger = ib_trigger;

    l.buttonName = param.name;
    if (h == -1) l.nImage = new Image();
    else l.nImage = new Image(w,h);
    pg_set(l.nImage,'src',param.n);
    if (h == -1) l.pImage = new Image();
    else l.pImage = new Image(w,h);
    pg_set(l.pImage,'src',param.p);
    if (h == -1) l.cImage = new Image();
    else l.cImage = new Image(w,h);
    pg_set(l.cImage,'src',param.c);
    if (h == -1) l.dImage = new Image();
    else l.dImage = new Image(w,h);
    pg_set(l.dImage,'src',param.d);
    l.enabled = null;
    if (cx__capabilities.Dom0IE)
    	{
    	l.attachEvent("onpropertychange",ib_setenable);
	}
    else
    	{
	l.watch("enabled",ib_setenable);
	}

    // Actions
    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("Enable", ib_enable);
    ia.Add("Disable", ib_disable);

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("Click");
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");
    
    l.enabled = param.enable;
    if (l.enabled)
	l.curmode = 'n';
    else
	l.curmode = 'd';
    }

