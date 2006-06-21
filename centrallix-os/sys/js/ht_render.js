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

/** values to be returned from event handling functions
    should return one of the first two and one of the last two, ored together **/
var EVENT_CONTINUE = 0;
var EVENT_HALT = 1;
var EVENT_ALLOW_DEFAULT_ACTION = 0;
var EVENT_PREVENT_DEFAULT_ACTION = 2;

// Functions used for cxsql-to-js support
function cxjs_user_name()
    {
    return pg_username;
    }
function cxjs_getdate()
    {
    var dt = new Date();
    var dtstr = '' + (dt.getMonth()+1) + '/' + (dt.getDate()) + '/' + (dt.getFullYear()) + ' ' + (dt.getHours()) + ':' + (dt.getMinutes()) + ':' + (dt.getSeconds());
    return dtstr;
    }
function cxjs_convert(dt,v)
    {
    if (dt == 'integer') return parseInt(v);
    if (dt == 'string') return '' + v;
    return v;
    }
function cxjs_substring(s,p,l)
    {
    if (l == null)
	return s.substr(p);
    else
	return s.substr(p,l);
    }
function cxjs_eval(x)
    {
    return eval(x);
    }

// Cross-browser support functions
function htr_event(e)
    {
    var cx__event = new Object();
    if(cx__capabilities.Dom2Events)
	{
	cx__event.Dom2Event = e;
	cx__event.type = e.type;
	cx__event.which = e.button+1;

	// move up from text nodes and spans to containers
	var t = e.target;
	while(t.nodeType == Node.TEXT_NODE || t.nodeName == 'SPAN' || 
	    (t.nodeType == Node.ELEMENT_NODE && !(t.tagName == 'DIV' || t.tagName == 'IMG')))
	    t = t.parentNode;

	cx__event.target = t;

	cx__event.pageX = e.clientX + window.pageXOffset;
	cx__event.pageY = e.clientY + window.pageYOffset;
	}
    else if(cx__capabilities.Dom0NS)
	{
	cx__event.NSEvent = e;
	cx__event.type = e.type;
	cx__event.target = e.target;
	cx__event.pageX = e.pageX;
	cx__event.pageY = e.pageY;
	cx__event.which = e.which;
	cx__event.modifiers = e.modifiers;

	cx__event.x = e.x;
	cx__event.y = e.y;
	cx__event.width = e.width;
	cx__event.height = e.height;
	cx__event.layerX = e.layerX;
	cx__event.layerY = e.layerY;
	cx__event.which = e.which;
	cx__event.modifiers = e.modifiers;
	cx__event.data = e.data;
	cx__event.screenX = e.screenX;
	cx__event.screenY = e.screenY;
	}
    else if(cx__capabilities.Dom0IE)
	{
	e = window.event
	
	cx__event.IEEvent = e;
	cx__event.type = e.type;

	// supported by IE 6
	var t = e.srcElement;
	while(t.nodeType == 3 || t.nodeName == 'SPAN' || 
	    (t.nodeType == 1 && !(t.tagName == 'DIV' || t.tagName == 'IMG')))
	    t = t.parentNode;

	cx__event.target = t;
	
	//cx__event.target = htr_get_parent_div(e.srcElement);

	//status = "ht_render.js target nodeName " + cx__event.target.nodeName + " " + cx__event.target.id;

	cx__event.pageX = e.clientX;
	cx__event.pageY = e.clientY;
	cx__event.which = e.button;
	cx__event.keyCode = e.keyCode;
	cx__event.modifiers = e.accessKey;

	cx__event.x = e.offsetX;
	cx__event.y = e.offsetY;
	/*
	cx__event.width = e.width;
	cx__event.height = e.height;
	cx__event.layerX = e.layerX;
	cx__event.layerY = e.layerY;
	cx__event.which = e.which;
	cx__event.modifiers = e.modifiers;
	cx__event.data = e.data;*/
	cx__event.screenX = e.screenX;
	cx__event.screenY = e.screenY;
	}
    return cx__event;
    }

function htr_alert(obj,maxlevels)
    {
    alert(htr_obj_to_text(obj,0,maxlevels));
    }

function htr_alert_obj(obj,maxlevels)
    {
    var w;
    if (window.alertwin)
	w = window.alertwin;
    else
	w = window.open();
    w.document.open();
    w.document.write("<pre>\n");
    w.document.write(htr_obj_to_text(obj,0,maxlevels));
    w.document.write("</pre>\n");
    w.document.close();
    window.alertwin = w;
    }

function htr_obj_to_text(obj,level,maxlevels)
    {
    if(level >= maxlevels)
	return "";
    var j = "";
    for(var i in obj)
	{
	var attr = obj[i];
	if(typeof(attr)=='function')
	    attr = "function";
	if(i == 'innerHTML' || i == 'outerHTML')
	    attr = "[ HTML REMOVED ]";
	j+= htr_build_tabs(level) + i + ": " + attr + "\n";
	if(attr == obj[i])
	    j += htr_obj_to_text(obj[i],level+1,maxlevels);
	}
    return j;
    }

function htr_build_tabs(level)
    {
    if(level==0)
	return "";
    return "	"+htr_build_tabs(level-1);
    }

function htr_watch(obj, attr, func)
    {
    if (!obj.htr_watchlist) 
	{
	obj.htr_watchlist = new Array();
	}
    if (cx__capabilities.Dom0IE) 
        {
        obj.onpropertychange = htr_watchchanged;
	} 
	else
	{
    	obj.watch(attr,htr_watchchanged);
	}
    var watchitem = new Object();
    watchitem.attr = attr;
    watchitem.func = func;
    watchitem.obj = obj;
    obj.htr_watchlist.push(watchitem);
    }

function htr_unwatch(obj, attr, func)
    {
    for (var i=0;i<obj.htr_watchlist.length;i++)
	{
	if (obj.htr_watchlist[i].attr == attr && obj.htr_watchlist[i].func == func)
	    {
	    obj.htr_watchlist.splice(i,1);
	    i--;
	    break;
	    }
	}
    }

function htr_watchchanged(prop,oldval,newval)
    {
    var setprop = newval;
   
    for (var i=0;i<this.htr_watchlist.length;i++)
	{
	if (this.htr_watchlist[i].attr == prop)
	    {
	    setprop = this[this.htr_watchlist[i].func](prop,oldval,newval);
	    if (setprop == oldval) return oldval;
	    }
	}
    return setprop;
    }

function htr_get_watch_newval(e)
    {
    if(e.propertyName.substr(0,6) == "style.")
        {
        return e.srcElement.style[e.propertyName.substr(6)];
        }
    else
        {
        return e.srcElement[e.propertyName];
        }
    }

function htr_init_layer(l,ml,kind)
    {
    if (l.document && l.document != document)
	{
	l.document.layer = l;
	}
    else
	l.layer = l;
	
    if (cx__capabilities.Dom1HTML)
	l.parentLayer = l.parentNode;
	
    l.mainlayer = ml;    
    l.kind = kind;
    l.cxSubElement = htr_get_subelement;
    if (l.document) l.document.cxSubElement = htr_get_subelement;
    }

function htr_search_element(e,id)
    {
    var sl = e.firstChild;
    var ck_sl = null;
    while(sl)
	{
	if ((sl.tagName == 'DIV' || sl.tagName == 'IFRAME') && sl.id == id)
	    return sl;
	if (sl.tagName == 'TABLE' || sl.tagName == 'TBODY' || sl.tagName == 'TR' || sl.tagName == 'TD')
	    if ((ck_sl = htr_search_element(sl, id)))
		return ck_sl;
	sl = sl.nextSibling;
	}
    }

function htr_get_subelement(id)
    {
    if (cx__capabilities.Dom0NS)
	{
	if (this.document)
	    return this.document.layers[id];
	else
	    return this.layers[id];
	}
    else if (cx__capabilities.Dom1HTML)
	{
	if (!this.tagName) return this.getElementById(id);
	return htr_search_element(this, id);
	}
    return null;
    }

function htr_extract_bgcolor(s)
    {
    if (s.substr(0,17) == "background-color:")
	{
	var cp = s.indexOf(":");
	return s.substr(cp+2,s.length-cp-3);
	}
    else if (s.substr(0,8) == "bgcolor=")
	{
	var qp = s.indexOf("'");
	if (qp < 1)
	    return s.substr(8);
	else
	    return s.substr(qp+1,s.length-qp-2);
	}
    return null;
    }

function htr_extract_bgimage(s)
    {
    if (s.substr(0,17) == "background-image:")
	{
	var qp = s.indexOf("'");
	return s.substr(qp+1,s.length-qp-4);
	}
    else if (s.substr(0,11) == "background=")
	{
	var qp = s.indexOf("'");
	return s.substr(qp+1,s.length-qp-2);
	}
    return null;
    }

function htr_getvisibility(l)
    {
    if (cx__capabilities.Dom0NS)
        {
	return l.visibility;
	}
    else if (cx__capabilities.Dom0IE)
        {
        return l.currentStyle.visibility;
	}
    else if (cx__capabilities.Dom1HTML)
        {
	if (!l.style.visibility)
	    return getComputedStyle(l,null).getPropertyCSSValue('visibility').cssText;
	else
	    return l.style.visibility;
	}
    return null;
    }

function htr_setvisibility(l,v)
    {
    if (cx__capabilities.Dom0NS)
        {
	l.visibility = v;
	}
    else if (cx__capabilities.Dom0IE)
        {
        l.runtimeStyle.visibility = v;
	}	
    else if (cx__capabilities.Dom1HTML)
        {
	l.style.visibility = v;
	}
    return null;
    }

function htr_getbgcolor(l)
    {
    if (cx__capabilities.Dom0NS)
	return l.bgColor;
    else if (cx__capabilities.Dom1HTML)
	return l.style.backgroundColor;
    return null;
    }

function htr_setbgcolor(l,v)
    {
    if (cx__capabilities.Dom0NS)
	l.bgColor = v;
    else if (cx__capabilities.Dom1HTML)
	l.style.backgroundColor = v;
    return null;
    }

function htr_getbgimage(l)
    {
    if (cx__capabilities.Dom0NS)
	return l.background.src;
    else if (cx__capabilities.Dom1HTML)
	return l.style.backgroundImage;
    return null;
    }

function htr_setbgimage(l,v)
    {
    if (cx__capabilities.Dom0NS)
	l.background.src = v;
    else if (cx__capabilities.Dom1HTML)
	//pg_set_style_string(l,"backgroundImage",v);
	l.style.backgroundImage = "URL('" + v + "')";
    return null;
    }

function htr_getphyswidth(l)
    {
    if (cx__capabilities.Dom0NS)
	return l.document.width;
    else if (cx__capabilities.Dom1HTML)
	{
	//if (l.offsetWidth) return l.offsetWidth;
	return pg_get_style(l,"width");
	}
    return null;
    }

function htr_getviswidth(l)
    {
    if (cx__capabilities.Dom0NS)
	return l.clip.width;
    else if (cx__capabilities.Dom1HTML)
	return pg_get_style(l, "clip.width");
    return null;
    }

function htr_getvisheight(l)
    {
    if (cx__capabilities.Dom0NS)
	return l.clip.height;
    else if (cx__capabilities.Dom1HTML)
	return pg_get_style(l, "clip.height");
    return null;
    }

function htr_getzindex(l)
    {
    if (cx__capabilities.Dom0NS)
	return l.zIndex;
    else if (cx__capabilities.Dom0IE)
	return parseInt(l.currentStyle.zIndex);
    else if (cx__capabilities.Dom1HTML && l.style && l.style.zIndex)
	return parseInt(l.style.zIndex);
    else if (cx__capabilities.Dom1HTML)
	return pg_get_style(l, 'z-index');
    return null;
    }

function htr_setzindex(l,v)
    {
    if (cx__capabilities.Dom0NS)
	l.zIndex = v;
    else if (cx__capabilities.Dom0IE)
	l.runtimeStyle.zIndex = v;
    else if (cx__capabilities.Dom1HTML)
	l.style.zIndex = v;
    return null;
    }

/**
* IE's srcElement will always be the lowest level element you clicked on
* need to trace back to the parent div node
* Since a div node can be a child of anotjer div node, trace back to the topmost div
**/
function htr_get_parent_div(o)
    {
    if(o && o.parentNode && o.nodeName != "BODY" && o.nodeName != "DIV" || o.parentNode.nodeName == "DIV") 
        {
	return htr_get_parent_div(o.parentNode);
        }
    return o;
    }


function htr_new_layer(w,p)
    {
    var nl;

	if (cx__capabilities.Dom0NS)
	    {
	    nl = new Layer(w,p);
	    }
	else if (cx__capabilities.Dom1HTML)
	    {
	    nl = document.createElement('div');
	    nl.style.width = w;
	    pg_set_style(nl, 'position','absolute');
	    p.appendChild(nl);
	    }

    return nl;
    }

function htr_write_content(l,t)
    {

	if (cx__capabilities.Dom0NS)
	    {
	    l.document.write(t);
	    l.document.close();
	    }
	else if (cx__capabilities.Dom0IE || cx__capabilities.Dom1HTML)
	    {
	    l.innerHTML = t;
	    }

    return;
    }

function htr_set_parent(l,n,p)
    {
    l.WName = n;
    l.WParent = p;
    if (!l.WParent.WChildren) l.WParent.WChildren = new Array();
    l.WParent.WChildren[n] = l;
    while (p.nonvisual) p = p.WParent;
    l.VParent = p;
    if (!l.VParent.VChildren) l.VParent.VChildren = new Array();
    l.VParent.VChildren[n] = l;
    }

