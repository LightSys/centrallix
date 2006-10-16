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
    pg_set(ly.img, 'src', newsrc);
    }

function ib_mousedown(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.kind=='ib' && ly.layer.enabled==true)
        {
	ib_setmode(ly.layer, 'c');
        cn_activate(ly.layer, 'MouseDown');
        ib_cur_img = ly.layer.img;
        }
    return EVENT_ALLOW_DEFAULT_ACTION | EVENT_CONTINUE;
    }

function ib_mouseup(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ib_cur_img)
        {
        if (e.pageX >= getPageX(ib_cur_img.layer) &&
            e.pageX < getPageX(ib_cur_img.layer) + getClipWidth(ib_cur_img.layer) &&
            e.pageY >= getPageY(ib_cur_img.layer) &&
            e.pageY < getPageY(ib_cur_img.layer) + getClipHeight(ib_cur_img.layer))
            {
            cn_activate(ly.layer, 'Click');
            cn_activate(ly.layer, 'MouseUp');
	    ib_setmode(ib_cur_img.layer, 'p');
            }
        else
            {
	    ib_setmode(ib_cur_img.layer, 'n');
            }
        ib_cur_img = null;
        }
    return EVENT_ALLOW_DEFAULT_ACTION | EVENT_CONTINUE;
    }

function ib_mousemove(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.kind == 'ib' && ly.layer.enabled == true)
        {
        ly = ly.layer;
        if (ly.img && ly.img.src != ly.cImage.src) ib_setmode(ly, 'p'); 
        cn_activate(ly.layer, 'MouseMove');
        }
    return EVENT_ALLOW_DEFAULT_ACTION | EVENT_CONTINUE;
    }

function ib_mouseover(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.kind == 'ib' && ly.layer.enabled == true)
        {
        ly = ly.layer;
        if (ly.img && (ly.img.src != ly.cImage.src)) ib_setmode(ly, 'p');
        cn_activate(ly.layer, 'MouseOver');
        }
    return EVENT_ALLOW_DEFAULT_ACTION | EVENT_CONTINUE;
    }

function ib_mouseout(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.kind == 'ib' && ly.layer.enabled == true)
        {
        ly = ly.layer;
        if (ly.img && ly.img.src != ly.cImage.src) ib_setmode(ly, 'n'); 
        cn_activate(ly.layer, 'MouseMove');
        }
    return EVENT_ALLOW_DEFAULT_ACTION | EVENT_CONTINUE;
    }

function ib_init(param)
    {
    var l = param.layer;
    var w = param.width;
    var h = param.height;
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
    l.img.kind = 'ib';
    l.cursrc = param.n;
    setClipWidth(l, w);

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
    }

