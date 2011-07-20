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

//**********************************
//This file is responsible for Events (and a few other things)
//**********************************

/** values to be returned from event handling functions
    should return one of the first two and one of the last two, ored together.
    (is this for IE only?)  **/
var EVENT_CONTINUE = 0;
var EVENT_HALT = 1;
var EVENT_ALLOW_DEFAULT_ACTION = 0;
var EVENT_PREVENT_DEFAULT_ACTION = 2;

function Money(n)
    {
    if (n)
	this.v = parseInt(n*1000);
    else
	this.v = 0;
    return this;
    }

function cx_count_divs(kind)
    {
    var divs = document.getElementsByTagName("div");
    var cnts = {};
    var str = "";
    var idx;
    for(var i = 0; i < divs.length; i++)
	{
	if (divs[i].kind)
	    idx = divs[i].kind;
	else
	    idx = 'none';
	cnts[idx] = cnts[idx]?(cnts[idx]+1):1;
	}
    for(var k in cnts)
	{
	if (!kind || k == kind)
	    str += k + ": " + cnts[k] + "\n";
	}
    return str;
    }

// Functions used for cxsql-to-js support
function cxjs_user_name()
    {
    return pg_username;
    }
function cxjs_getdate()
    {
    var dt = new Date();
    var dtmin = (dt.getMinutes()<10)?('0' + dt.getMinutes()):dt.getMinutes();
    var dtsec = (dt.getSeconds()<10)?('0' + dt.getSeconds()):dt.getSeconds();
    var dtstr = '' + (dt.getMonth()+1) + '/' + (dt.getDate()) + '/' + (dt.getFullYear()) + ' ' + (dt.getHours()) + ':' + dtmin + ':' + dtsec;
    return dtstr;
    }
function cxjs_convert(dt,v)
    {
    if (v == null || dt == null) return null;
    if (dt == 'integer') return parseInt(v);
    if (dt == 'string') return '' + v;
    return v;
    }
function cxjs_substring(s,p,l)
    {
    if (s == null || p == null) return null;
    if (l == null)
	return s.substr(p-1);
    else
	return s.substr(p-1,l);
    }
function cxjs_eval(x)
    {
    var _this = null;
    var _context = null;
    if (x == null) return null;
    if (typeof x == 'object') x = x.toString();
    return eval(x);
    }
function cxjs_isnull(v,d)
    {
    if (v == null)
	return d;
    else
	return v;
    }
function cxjs_ltrim(s)
    {
    if (s == null) return null;
    return String(s).replace(/^ */, "");
    }
function cxjs_rtrim(s)
    {
    if (s == null) return null;
    return String(s).replace(/ *$/, "");
    }
function cxjs_plus(a, b)
    {
    if (a == null || b == null) return null;
    if ((typeof a == 'string') || (typeof b == 'string'))
	return String(a) + String(b);
    else
	return a + b;
    }

function cxjs_condition(c, vtrue, vfalse)
    {
    if (c == null) return null;
    if (c)
	return vtrue;
    else
	return vfalse;
    }

function cxjs_quote(s)
    {
    s = String(s);
    return '"' + s.replace(/([\"])/g, "\\$1") + '"';
    }

function cxjs_charindex(needle, haystack)
    {
    if (needle === null || haystack === null) return null;
    return (new String(haystack)).indexOf(needle) + 1;
    }

function cxjs_char_length(str)
    {
    if (str == null) return null;
    return String(str).length;
    }

function cxjs_upper(str)
    {
    if (str === null) return null;
    return String(str).toUpperCase();
    }

function cxjs_lower(str)
    {
    if (str === null) return null;
    return String(str).toLowerCase();
    }

// Cross-browser support functions
function htr_event(e)
    {
    var cx__event = {};
    cx__event.cx = true;

    // Browser specific stuff
    if(cx__capabilities.Dom2Events)
	{
	cx__event.Dom2Event = e;
	cx__event.type = e.type;
	cx__event.which = e.button+1;
	cx__event.modifiers = e.modifiers;
	cx__event.shiftKey = e.shiftKey;
	if (e.type == 'keypress' || e.type == 'keydown' || e.type == 'keyup')
	    {
	    cx__event.key = e.which;
	    switch(e.keyCode)
		{
		case e.DOM_VK_HOME:	cx__event.keyName = 'home'; break;
		case e.DOM_VK_END:	cx__event.keyName = 'end'; break;
		case e.DOM_VK_LEFT:	cx__event.keyName = 'left'; break;
		case e.DOM_VK_RIGHT:	cx__event.keyName = 'right'; break;
		case e.DOM_VK_UP:	cx__event.keyName = 'up'; break;
		case e.DOM_VK_DOWN:	cx__event.keyName = 'down'; break;
		case e.DOM_VK_TAB:	cx__event.keyName = 'tab'; break;
		case e.DOM_VK_ENTER:	cx__event.keyName = 'enter'; break;
		case e.DOM_VK_RETURN:	cx__event.keyName = 'enter'; break;
		case e.DOM_VK_ESCAPE:	cx__event.keyName = 'escape'; break;
		case e.DOM_VK_F3:	cx__event.keyName = 'f3'; break;
		default:		cx__event.keyName = null; break;
		}
	    }

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
	cx__event.key = e.which;
	cx__event.keyName = null;

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
	cx__event.pageX = e.clientX;
	cx__event.pageY = e.clientY;
	cx__event.which = e.button;
	cx__event.keyCode = e.keyCode;
	cx__event.modifiers = e.accessKey;
	cx__event.key = e.keyCode;
	cx__event.keyName = null;

	cx__event.x = e.offsetX;
	cx__event.y = e.offsetY;
	cx__event.screenX = e.screenX;
	cx__event.screenY = e.screenY;
	}

    // Non-browser specific stuff
    cx__event.layer = ((typeof cx__event.target.layer) != "undefined" && cx__event.target.layer != null)?cx__event.target.layer:cx__event.target;
    if (cx__event.layer.mainlayer) 
	cx__event.mainlayer = cx__event.layer.mainlayer;
    cx__event.kind = cx__event.layer.kind;
    if (cx__event.mainlayer) 
	cx__event.mainkind = cx__event.mainlayer.kind;
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

function htr_cwatch(obj, attr, fobj, func)
    {
    if (!obj.htr_watchlist) 
	{
	obj.htr_watchlist = [];
	}
    if (cx__capabilities.Dom0IE) 
        {
        obj.onpropertychange = htr_watchchanged;
	} 
    else
	{
    	obj.watch(attr,htr_watchchanged);
	}
    var watchitem = {};
    watchitem.attr = attr;
    watchitem.func = func;
    watchitem.obj = obj;
    watchitem.fobj = fobj;
    obj.htr_watchlist.push(watchitem);
    }

//uses (or mimicks) firefox's .watch method.
function htr_watch(obj, attr, func)
    {
    htr_cwatch(obj, attr, null, func);
    }

function htr_uncwatch(obj, attr, fobj, func)
    {
    for (var i=0;i<obj.htr_watchlist.length;i++)
	{
	var wl = obj.htr_watchlist[i];
	if (wl.attr == attr && wl.func == func && wl.fobj == fobj)
	    {
	    obj.htr_watchlist.splice(i,1);
	    i--;
	    break;
	    }
	}
    }

function htr_unwatch(obj, attr, func)
    {
    htr_uncwatch(obj, attr, null, func);
    }

function htr_watchchanged(prop,oldval,newval)
    {
    var setprop = newval;
   
    for (var i=0;i<this.htr_watchlist.length;i++)
	{
	var wl = this.htr_watchlist[i];
	if (wl.attr == prop)
	    {
	    if (wl.fobj)
		setprop = wl.fobj[wl.func](this,prop,oldval,newval);
	    else
		setprop = this[wl.func](prop,oldval,newval);
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

function htr_parselinks(lnks)
    {
    var rows = [];
    if (lnks && lnks.length > 0)
	{
	var row = {};
	var tgt = null;
	var colcnt = 0;
	for(var i = 0; i<lnks.length; i++)
	    {
	    var lnk = lnks[i];
	    if (lnk.target != tgt && lnk.target != 'R')
		{
		if (colcnt) rows.push(row);
		row = {};
		colcnt = 0;
		tgt = lnk.target;
		}
	    var col = {type:lnk.hash.substr(1), oid:htutil_unpack(lnk.host), hints:lnk.search};
	    switch(lnk.text.charAt(0))
		{
		case 'V': col.value = htutil_rtrim(unescape(lnk.text.substr(2))); break;
		case 'N': col.value = null; break;
		case 'E': col.value = '** ERROR **'; break;
		}
	    if (col.type == 'integer') col.value = parseInt(col.value);
	    if (typeof row[col.oid] == 'undefined')
		{
		row[col.oid] = col;
		colcnt++;
		}
	    }
	if (colcnt) rows.push(row);
	}

    return rows;
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

function htr_set_event_target(l, et)
    {
    if (l.document && l.document != document)
	l.document.layer = et;
    else
	l.layer = et;
    }

function htr_search_element(e, id)
    {
    var found = document.getElementById(id);
    if (!found || (found.tagName != 'DIV' && found.tagName != 'IFRAME'))
	return null;
    var search = found.parentNode;
    while(search)
	{
	if (search == e) return found;
	if (search == document || search == window) return null;
	search = search.parentNode;
	}
    return null;
    }

function htr_get_layers(l)
    {
    if (cx__capabilities.Dom0NS)
	{
	if (l.document)
	    return l.document.layers;
	else
	    return l.layers;
	}
    else
	{
	//alert(l);
	var sl = l.firstChild;
	var lyrs = new Array();
	if (sl.tagName == 'HTML') sl = sl.firstChild;
	if (sl.tagName == 'BODY') sl = sl.firstChild;
	while(sl)
	    {
	    if (sl.tagName == 'DIV' || sl.tagName == 'IFRAME')
		lyrs.push(sl);
	    sl = sl.nextSibling;
	    }
	return lyrs;
	}
    }

function htr_get_subelement(id)
    {
    return htr_subel(this, id);
    }

function htr_subel(l, id)
    {
    if (cx__capabilities.Dom0NS)
	{
	if (!l) 
	    {
	    alert('parent undefined: ' + id);
	    return null;
	    }
	if (l.document)
	    {
	    if (!l.document.layers[id]) 
		{
		//htr_alert(wgtrGetContainer(wgtrGetNode(window[ns],'win')),1);
		alert('no subobject: ' + id + ' for layer ' + l.id);
		}
	    return l.document.layers[id];
	    }
	else
	    {
	    if (!l.layers) alert('object has no layers: ' + l.id + ' looking for subobject: ' + id);
	    if (!l.layers[id]) 
		{
		//htr_alert(wgtrGetContainer(wgtrGetNode(window[ns],'win')),1);
		alert('no subobject: ' + id + ' for layer ' + l.id);
		//alert(startup);
		}
	    return l.layers[id];
	    }
	}
    else if (cx__capabilities.Dom0IE)
	{
	if (l.document)
	    return l.document.all[id];
	else
	    return l.all[id];
	}
    else if (cx__capabilities.Dom1HTML)
	{
	if (!l) alert('parent undefined: ' + id);
	//if (!l.getElementById) alert('no func on ' + l.id + ' looking for ' + id);
	//if (!l.tagName) return l.getElementById(id);
	if (!l.tagName) return document.getElementById(id);
	return htr_search_element(l, id);
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
    else if (s.substr(0,8) == "bgcolor=" || s.substr(0,8) == "bgColor=")
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
    var v = null;
    if (cx__capabilities.Dom0NS)
        {
	v = l.visibility;
	}
    else if (cx__capabilities.Dom0IE)
        {
        v = l.currentStyle.visibility;
	}
    else if (cx__capabilities.Dom1HTML)
        {
	if (!l.style.visibility)
	    v =  getComputedStyle(l,null).getPropertyCSSValue('visibility').cssText;
	else
	    v = l.style.visibility;
	}
    if (v == 'visible') v = 'inherit';
    return v;
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
	pg_serialized_load(l.background, v, null);
    else if (cx__capabilities.Dom1HTML)
	{
	if (!v)
	    l.style.backgroundImage = null;
	else
	    l.style.backgroundImage = "URL('" + v + "')";
	}
    return null;
    }

function htr_setbackground(l, v)
    {
    var bgimg = v?htr_extract_bgimage(v):null;
    var bgcol = v?htr_extract_bgcolor(v):null;
    if (bgimg) htr_setbgimage(l, bgimg);
    if (bgcol) htr_setbgcolor(l, bgcol);
    if (!bgimg && !bgcol)
	{
	htr_setbgimage(l, null);
	htr_setbgcolor(l, null);
	}
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
* Since a div node can be a child of another div node, trace back to the topmost div
**/
function htr_get_parent_div(o)
    {
    if(o && o.parentNode && o.nodeName != "BODY" && o.nodeName != "DIV" || o.parentNode.nodeName == "DIV") 
        {
	return htr_get_parent_div(o.parentNode);
        }
    return o;
    }


function htr_new_loader(p)
    {
    var nl = null;

	if (cx__capabilities.Dom0NS)
	    {
	    nl = htr_new_layer(pg_width, p);
	    }
	else if (cx__capabilities.Dom1HTML)
	    {
	    if (!p || p == document || p == window) p = document.body;
	    nl = document.createElement('iframe');
	    nl.style.width = pg_width + "px";
	    pg_set_style(nl, 'position','absolute');
	    p.appendChild(nl);
	    }

    return nl;
    }


function htr_new_layer(w,p)
    {
    var nl;

	if (cx__capabilities.Dom0NS)
	    {
	    if (p == document) p = window;
	    if (!p)
		nl = new Layer(w);
	    else
		nl = new Layer(w,p);
	    }
	else if (cx__capabilities.Dom1HTML)
	    {
	    if (!p || p == document || p == window) p = document.body;
	    nl = document.createElement('div');
	    if (w) nl.style.width = w + "px";
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


function htr_addeventhandler(t,h)
    {
    if (!pg_handlers[t])
	pg_handlers[t] = [];
    pg_handlers[t][h] = eval(h);
    }


// This one is special-cased due to the volume of events that it must
// handle.  We use timing tricks to reduce the number of mousemove events
// to a manageable number, relative to the performance available on the
// client useragent.
//
function htr_mousemovehandler(e)
    {
    // stack the event
    pg_mousemoveevents.push(htr_event(e));
    // are we busy?
    if (!pg_handlertimeout)
	pg_handlertimeout = setTimeout(htr_mousemovehandler_cb, 0);
    if (cx__capabilities.Dom2Events)
	{
	e.preventDefault();
	e.stopPropagation();
	}
    return false;
    }

function htr_mousemovehandler_cb()
    {
    pg_handlertimeout = null;
    // for now, just ignore previous mousemove events and only
    // process the most recent one.
    var e = pg_mousemoveevents.pop();
    pg_mousemoveevents = [];
    htr_eventhandler(e);
    }


// Universal event handler script. Recieves a browser event variable
// and converts different browsers' events into a Centrallix
// event. Also, because IE and FF manage default_action manipulation
// differently, this function ensures correct manipulation of
// default_action. (IE prevents/allow default action by the handler's
// return function while FF has a separate function)
function htr_eventhandler(e)
    {
    if (!e.cx)
	e = htr_event(e);
    //if (e.keyName == 'escape' && e.type == 'keypress') window.esccnt = (window.esccnt)?(window.esccnt + 1):1;
    var handler_return;
    var prevent_default = false;
    var handlerlist = pg_handlers[e.type];

    for(var h in handlerlist)
	{
	if (typeof handlerlist[h] != 'function') //SETH: @@ there's no need to have string keys
	    alert(h + ' ' + typeof handlerlist[h]);
	handler_return = handlerlist[h](e);
	if (handler_return & EVENT_PREVENT_DEFAULT_ACTION)
	    prevent_default = true;
	if (handler_return & EVENT_HALT)
	    {
	    if (prevent_default && cx__capabilities.Dom2Events && e.type != 'mousemove')
		{
		e.Dom2Event.preventDefault();
		e.Dom2Event.stopPropagation();
		}

	    // catch all 0-length scheduled stuff before returning to browser control.
	    if (e.type != 'mousemove')
		{
		pg_stopschedtimeout();
		pg_dosched(true);
		}

	    return !prevent_default;
	    }
	}

    if (prevent_default && cx__capabilities.Dom2Events && e.type != 'mousemove')
	{
	e.Dom2Event.preventDefault();
	e.Dom2Event.stopPropagation();
	}

    // catch all 0-length scheduled stuff before returning to browser control.
    if (e.type != 'mousemove')
	{
	pg_stopschedtimeout();
	pg_dosched(true);
	}

    return !prevent_default;
    }

function htr_addeventlistener(eventType,obj,handler)
    {
    pg_capturedevents || (pg_capturedevents = []);

    if (cx__capabilities.Dom2Events)
	{
	if (typeof pg_capturedevents[eventType] == 'undefined')
	    {
	    pg_capturedevents[eventType] = handler;
	    obj.addEventListener(eventType, handler, true);
	    }
	}
    else
	{
	obj['on' + eventType] = handler;
	}
    }

function htr_captureevents(elist)
    {
    // This is a workaround for an apparent Gecko bug that causes events
    // to fire twice if captureEvents is called twice, EVEN IF ALL EVENTS
    // SPECIFIED THE SECOND TIME WERE NOT SPECIFIED THE FIRST TIME!
    // N.B. - captureEvents() is deprecated and may not be available in
    // the future.

    if (!cx__capabilities.Dom2Events)
	{
	document.releaseEvents(pg_capturedevents);
	pg_capturedevents |= elist;
	document.captureEvents(pg_capturedevents);
	}
    }

// Load indication
if (window.pg_scripts) pg_scripts['ht_render.js'] = true;
