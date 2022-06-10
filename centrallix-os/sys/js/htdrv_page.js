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

/* This file contains a bunch of helper functions for DOM/CSS
 * manipulation, for 'boxes' and 'areas', for keyboard event handling,
 * for page 'status', wrappers for setTimeout (as a NS4 hack),
 * 'expression' functions, async request management, reveal/obscure,
 * debugging, Control Message management, some event handlers for
 * mouse events.
 */


var pg_msglist = '';
var pg_init_ts = (new Date()).valueOf();

var pg_spinner_id = null;
var pg_spinner = null;

var pg_layer = null;

var pg_modallist = [];

var pg_msg = {};
pg_msg.MSG_ERROR=1;
pg_msg.MSG_QUERY=2;
pg_msg.MSG_GOODBYE=4;
pg_msg.MSG_REPMSG=8;
pg_msg.MSG_EVENT=16;

var pg_explog = [];


function pg_scriptavailable(s)
    {
    if (!s.src) return true;
    var s_name = s.src;
    s_name = s_name.replace(/\/CXDC:[0-9]*$/, "");
    var pos = s_name.lastIndexOf("/");
    var file = s_name.substr(pos+1);
    return pg_scripts[file]?true:false;
    }


//START SECTION: DOM/CSS helper functions -----------------------------------

/** returns an attribute of the element in pixels **/
function pg_get_style(element,attr)
    {
    if(!element)
	{
	alert("NULL ELEMENT, attr " + attr + "is unknown.");
	return null;
	}
    if(cx__capabilities.Dom1HTML && cx__capabilities.Dom2CSS)
	{
	if(attr == 'zIndex') attr = 'z-index';
	if(attr.substring(0,5) == 'clip.')
	    {
	    //return eval('element.' + attr);
	    return element.clip[attr.substr(5)];
	    }	
	var comp_style = window.getComputedStyle(element,null);
	if (comp_style.getPropertyCSSValue)
	    {
	    var cssValue = comp_style.getPropertyCSSValue(attr);
	    if (!cssValue) alert(element.id + '.' + attr);
	    if(cssValue.cssValueType != CSSValue.CSS_PRIMITIVE_VALUE)
		{
		alert(attr + ': ' + cssValue.cssValueType);
		return null;
		}
	    if(cssValue.primitiveType >= CSSPrimitiveValue.CSS_STRING)
		return cssValue.getStringValue();
	    if (cssValue.primitiveType == CSSPrimitiveValue.CSS_NUMBER)
		return cssValue.getFloatValue(CSSPrimitiveValue.CSS_NUMBER);
	    return cssValue.getFloatValue(CSSPrimitiveValue.CSS_PX);
	    }
	else
	    {
	    var val = comp_style[attr];
	    var num = parseFloat(val);
	    if (isNaN(num))
		return val;
	    else
		return num;
	    }
	}
    else if(cx__capabilities.Dom0NS)
	{
	return eval('element.' + attr);
	}
    else if(cx__capabilities.Dom0IE)
        {        
        if(attr.substr(0,5) == 'clip.')
            {
            if(attr == 'clip.width')
                {
                return getClipWidth(element);
                }
            else if(attr == 'clip.height')
                {
                return getClipHeight(element);
                }
            else if(attr == 'clip.top')
                {
                return getClipTop(element);
                }
            else if(attr == 'clip.right')
                {
                return getClipRight(element);
                }
            else if(attr == 'clip.bottom')
                {
                return getClipBottom(element);
                }
            else if(attr == 'clip.left')
                {
                return getClipLeft(element);
                }
            else
                {
                alert("clip property " + attr + " is undefined in " + element);
                return null;
                }                
            }
        else if(attr == 'width')
            {
            var _w = parseInt(element.currentStyle.width);
            if (isNaN(_w))
            	return element.offsetWidth;
            return _w;
            }
        else if(attr == 'height')
            {
            var _h = parseInt(element.currentStyle.height);
            if (isNaN(_h))
            	return element.offsetHeight;
            return _h;
            }
        else if(attr == 'top')
            {            
            return element.currentStyle.top;
            }
        else if(attr == 'visibility')
            {            
            return element.currentStyle.visibility;
            }
        else if(attr == 'zIndex')
            {            
            return element.currentStyle.zIndex;
            }
        else 
            {
            alert(" attr " + attr + " need to be implemeneted in pg_get_style of htdrv_page.js");
            }
        return null;
        }
    else
	{
	alert('cannot calculate CSS values for this browser');
	}
    }

function pg_set_style(element,attr, value)
    {
    if(cx__capabilities.Dom1HTML && cx__capabilities.Dom2CSS)
	{
	if(attr.substr(0,5) == 'clip.')
	    {
	    element[attr] = value;
	    return;
	    }
	if (isNaN(parseInt(value)) || (String(value)).indexOf(" ") >= 0)
	    element.style.setProperty(attr,value,"");
	else
	    element.style.setProperty(attr,parseInt(value) + "px","");
	return;
	}
    else if(cx__capabilities.Dom0NS)
	{
	element[attr] = value;
	return;
	}
    // Jason Yip
    else if(cx__capabilities.Dom0IE)
	{
	if(attr == 'visibility') 
	    {
	    element.runtimeStyle.visibility = value;
	    return;
	    }
	else if(attr == 'left')
	    {
	    element.style.pixelLeft = value;
	    return;
	    }
	else if(attr == 'top')
	    {
	    element.style.pixelTop = value;
	    return;
	    }
	else if(attr == 'clip.top')
	    {
	    setClipTop(element, value);
	    return;
	    }
	else if(attr == 'clip.right')
	    {
	    setClipRight(element, value);
	    return;
	    }
	else if(attr == 'clip.bottom')
	    {
	    setClipBottom(element, value);
	    return;
	    }
	else if(attr == 'clip.left')
	    {
	    setClipLeft(element, value);
	    return;
	    }	    
	else if(attr == 'clip.height')
	    {
	    setClipHeight(element, value);
	    return;
	    }
	else if(attr == 'clip.width')
	    {
	    setClipWidth(element, value);
	    return;
	    }
	else if(attr == 'pageX')
	    {
	    setPageX(element, value);
	    return;
	    }
	else if(attr == 'pageY')
	    {
	    setPageY(element, value);	    
	    return;
	    }
	else if(attr == 'zIndex')
	    {
	    element.runtimeStyle.zIndex = value;
	    return;
	    }
	else if(attr == 'bgColor')
	    {
	    element.runtimeStyle.backgroundColor = value;
	    return;
	    }
	else if(attr == 'position')
	    {
	    element.runtimeStyle.position = value;
	    return;
	    }	    
	else if(attr == 'width')
	    {
	    element.runtimeStyle.width = value;
	    return;
	    }	    
	else if(attr == 'height')
	    {
	    element.runtimeStyle.height = value;
	    return;
	    }	    
	else
	    {
	    alert(attr + " is not implemented.");
	    return;
	    }
	return;
	}	
    else
	{
	alert('cannot set CSS values for this browser');
	return;
	}
    }

function pg_set_style_string(element,attr, value)
    {
    if(cx__capabilities.Dom1HTML && cx__capabilities.Dom2CSS)
	{
	element.style.setProperty(attr,value,"");
	return;
	}
    else if(cx__capabilities.Dom0NS)
	{
	element[attr] = value;
	return;
	}
    else if(cx__capabilities.Dom0IE)
	{
	if(attr == 'visibility') 
	    {
	    element.runtimeStyle.visibility = value;
	    return;
	    }
	else if(attr == 'position')
	    {
	    element.runtimeStyle.position = value;
	    return;
	    }
	else if(attr == 'width')
	    {
	    element.runtimeStyle.width = value;
	    return;
	    }	    
	else
	    {
	    alert(attr + " is not implemented in set_style_string in htdrv_page.js.");
	    }
	}	
    else
	{
	alert('cannot set CSS values for this browser');
	return;
	}
    }

// pg_show_containers() - makes sure containers, from innermost to
// outermost, are displayed to the user.  Used when a control receives
// keyboard focus to make sure control is visible to user.
function pg_show_containers(l, x, y)
    {
    var orig_l = l;
    while (l && !wgtrIsNode(l))
	{
	if (l.showcontainer && l.showcontainer(orig_l, x, y) == false)
	    return false;
	l = pg_get_container(l);
	}
    while(l)
	{
	if (l.showcontainer && l.showcontainer(orig_l, x, y) == false)
	    return false;
	l = wgtrGetParent(l);
	}
    return true;
    }


// pg_get_container() - figure out what layer directly contains 
// the current one.
function pg_get_container(l)
    {
    if (cx__capabilities.Dom0NS)
	{
	if (typeof l.parentLayer == 'undefined') return l;
	else return l.parentLayer;
	}
    else if (cx__capabilities.Dom1HTML)
	{
	if (!l) return null;
	do  {
	    l = l.parentNode;
	    } while (l && l != window && l.tagName != 'BODY' && l.tagName != 'DIV' && l.tagName != 'IFRAME');
	return l;
	}
    return null;
    }


// pg_toplevel_layer() - returns the layer which contains the given
// one, at the top level (i.e., a direct child of the page itself)
function pg_toplevel_layer(l)
    {
    if (cx__capabilities.Dom0NS)
	{
	while (l != window && l.parentLayer != window) l = l.parentLayer;
	}
    else
	{
	while (l.tagName != 'BODY' && l.parentNode.tagName != 'BODY' && 
		l != window && l.parentNode != window) 
	    l = l.parentNode;
	}
    return l;
    }


/** Function to get the links attached to a layer **/
function pg_links(o)
    {
    if(cx__capabilities.Dom1HTML)
	{
	if(o.contentDocument)
	    {
	    return o.contentDocument.getElementsByTagName("a");
	    }
	else
	    {
	    return o.getElementsByTagName("a");
	    }
	}
    else if(cx__capabilities.Dom0NS || cx__capabilities.Dom0IE)
	{
	return o.document.links;
	}
    else
	{
	return null;
	}
    }

/** Function to get the layers attached to a layer **/
function pg_layers(o)
    {
    if(!o)
	return null;
    if(cx__capabilities.Dom1HTML)
	{
	var divs = o.getElementsByTagName("DIV");
	return divs;
	}
    else if(cx__capabilities.Dom0NS || cx__capabilities.Dom0IE)
	{
	if(!o.document)
	    {
	    return o.layers;
	    }
	return o.document.layers;
	}
    else
	{
	return null;
	}
    }

/** Function to get the images attached to a layer **/
function pg_images(o)
    {
    if(!o)
	return null;
    if(cx__capabilities.Dom1HTML)
	{
	return o.getElementsByTagName("img");
	}
    else if(cx__capabilities.Dom0NS || cx__capabilities.Dom0IE)
	{
	if(!o.document)
	    {
	    return o.images;
	    }
	return o.document.images;
	}
    else
	{
	return null;
	}
    }

/** function to set an attribute **/
function pg_set(o,a,v)
    {
    if(cx__capabilities.Dom1HTML)
	{
	return o.setAttribute(a,v);
	}
    else
	{
	o[a]=v;
	}
    }

function pg_get(o,a)
    {
    if(cx__capabilities.Dom1HTML)
	{
	return o.getAttribute(a);
	}
    else
	{
	return o[a];
	}
    }

//END SECTION: DOM/CSS helper functions -----------------------------------

//START SECTION: pinging functions ---------------------------------------
/* these functions deal with pinging the server. The client pings the
 * server because ...? //SETH:
 */

function pg_ping_init(l,i)
    {
    l.tid=setInterval(pg_ping_send,i,l);    		
    }

function pg_ping_recieve()
    {
    var link = null;
    var links = this.contentDocument.getElementsByTagName("a");
    if (links && links.length > 0)
	link = this.contentDocument.getElementsByTagName("a")[0];

    if(!link || link.target==='ERR')
	{
	clearInterval(this.tid);
	if (!window.pg_disconnected)
	    confirm('you have been disconnected from the server');
	window.pg_disconnected = true;
	}
    else if (link && link.target !== 'OK')
	{
	pg_servertime_notz = new Date(link.target);
	pg_clienttime = new Date();
	pg_clockoffset = pg_clienttime - pg_servertime_notz;
	}
    }
    
function pg_ping_send(p)
    {
    pg_serialized_load(p, '/INTERNAL/ping?cx__akey=' + window.akey, pg_ping_recieve);
    }

//END SECTION: pinging functions ---------------------------------------


function pg_get_computed_clip(o) //SETH: ??
    {
    if(cx__capabilities.Dom2CSS)
	return getComputedStyle(o,null).getPropertyCSSValue('clip').getRectValue();
    else if(cx__capabilities.Dom0NS)
	return o.clip;
    else if(cx__capabilities.Dom0IE)
	return o.runtimeStyle.clip;
    else
	return null;
    }

function pg_get_clip(o)
    {
    if(cx__capabilities.Dom2CSS2)
	{
	var clip = o.style.getPropertyCSSValue('clip');
	if(clip)
	    return clip.getRectValue();
	else
	    {
	    var computed = getComputedStyle(o,null).getPropertyCSSValue('clip').cssText;
	    o.style.setProperty('clip',computed,"");
	    clip = o.style.getPropertyValue('clip');	    
	    clip = o.style.getPropertyCSSValue('clip');	    
	    return clip.getRectValue;
	    }
	}
    else if(cx__capabilities.Dom0NS)
	return o.clip;
    else if(cx__capabilities.Dom0IE)
	return o.runtimeStyle.clip;	
    else
	return null;
    }


/** Function to emulate getElementById **/
function pg_getelementbyid(nm)
    {
    if (this.layers)
	return this.layers[nm];
    else if (this.all)
	return this.all[nm];
    else
	return null;
    }

/** Function to walk the DOM and set up getElementById emulation **/
function pg_set_emulation(d)
    {
    var a = null;
    var i = 0;
    d.getElementById = pg_getelementbyid;
    if (d.document) 
	{
	d = d.document;
	d.getElementById = pg_getelementbyid;
	}
    if (d.layers)
	a = d.layers;
    else if (d.all)
	a = d.all;
    else
	a = null;
    if (a)
	{
	for(i=0;i<a.length;i++)
	    {
	    pg_set_emulation(a[i]);
	    }
	}
    }

/** Function to set modal mode to a layer. **/
function pg_setmodal(l, is_modal)
    {
    // Find l in the modal list
    var pos = pg_modallist.indexOf(l);
    if (!is_modal)
	{
	if (pos >= 0)
	    {
	    pg_modallist.splice(pos, 1);
	    }
	else if (l == pg_modallayer)
	    {
	    pg_modallayer = pg_modallist.pop();
	    if (pg_modallayer === undefined)
		pg_modallayer = null;
	    }
	}
    else
	{
	if (pos === -1 && l !== pg_modallayer)
	    {
	    if (pg_modallayer)
		pg_modallist.push(pg_modallayer);
	    pg_modallayer = l;
	    }
	}
    /*if (pg_modallist.length && !l)
	l = pg_modallist.pop();
    else if (l && pg_modallayer)
	pg_modallist.push(pg_modallayer);
    pg_modallayer = l;*/
    if (!window.pg_masklayer)
        {
	pg_masklayer = htr_new_layer(pg_width, null);
	setClipWidth(pg_masklayer, pg_width);
	setClipHeight(pg_masklayer, pg_height);
	resizeTo(pg_masklayer, pg_width, pg_height);
	if (cx__capabilities.CSS2)
	    htr_setbgimage(pg_masklayer, "/sys/images/black_trans_50.png");
	else
	    htr_setbgimage(pg_masklayer, "/sys/images/black_trans_2x2.gif");
	}
    if (pg_modallayer)
	{
	var l = pg_modallayer;
	if (l.mainlayer) l = l.mainlayer;
	moveBelow(pg_masklayer, l);
	moveTo(pg_masklayer, 0, 0);
	setClipWidth(pg_masklayer, pg_width);
	setClipHeight(pg_masklayer, pg_height);
	htr_setvisibility(pg_masklayer, 'inherit');
	}
    else
	{
	htr_setvisibility(pg_masklayer, 'hidden');
	}
    }

/** Function to find out whether image or layer is in a layer **/
function pg_isinlayer(outer,inner)
    {
    if (inner == outer) return true;
    if(!outer) return true;
    if(!inner) return false;
    var i = 0;
    if(cx__capabilities.Dom1HTML)
        {
	while(inner)
	    {
	    if (inner == outer) return true;
	    if (inner == window || inner == document) break;
	    inner = inner.parentNode;
	    }
        }
    else
        {
		for(i=0;i<outer.layers.length;i++)
	        {
			if (outer.layers[i] == inner) return true;
			if (pg_isinlayer(outer.layers[i], inner)) return true;
			}
	}
//    for(i=0;i<outer.document.images.length;i++)
//		{
//		if (outer.document.images[i] == inner) return true;
//		}
    return false;
    }

/** Function to make four layers into a box  //SETH: ?? what's a 'box'?
* pl - a layer
* x,y - x,y-cord
* w,h - widht, height
* s - ?
* tl - top layer
* bl - bottom layer
* rl - right layer
* ll - left layer
* c1 - color1, for tl and ll
* c2 - color2, for bl and rl
* z - zIndex
**/
function pg_mkbox(pl, x,y,w,h, s, tl,bl,rl,ll, c1,c2, z)
    {
    
    htr_setvisibility(tl, 'hidden');
    htr_setvisibility(bl, 'hidden');
    htr_setvisibility(rl, 'hidden');
    htr_setvisibility(ll, 'hidden');
    //abc();
    if (cx__capabilities.Dom0NS || cx__capabilities.Dom1HTML)
/*        {
    	tl.bgColor = c1;
    	ll.bgColor = c1;
    	bl.bgColor = c2;
    	rl.bgColor = c2;
    	}
    else if (cx__capabilities.Dom1HTML) */
        {
    	htr_setbgcolor(tl,c1);
    	htr_setbgcolor(ll,c1);
    	htr_setbgcolor(bl,c2);
    	htr_setbgcolor(rl,c2);
        }
    //alert("x, y --" + x + " " + y);
    
    resizeTo(tl,w,1);
    setClipWidth(tl,w);
    setClipHeight(tl,1);
    //if (cx__capabilities.Dom1HTML && pl)
    //	pl.parentLayer.appendChild(tl);
    moveAbove(tl,pl);
    //moveToAbsolute(tl,x,y);
    $(tl).offset({left:x, top:y});
    htr_setzindex(tl,z);

    resizeTo(bl,w+s-1,1);
    setClipWidth(bl,w+s-1);
    setClipHeight(bl,1);
    //if (cx__capabilities.Dom1HTML && pl)
    //	pl.parentLayer.appendChild(bl);
    moveAbove(bl,pl);
    //moveToAbsolute(bl,x,y+h-s+1);
    $(bl).offset({left:x, top:y+h-s+1});
    htr_setzindex(bl,z);

    resizeTo(ll,1,h);
    setClipHeight(ll,h);
    setClipWidth(ll,1);
    //if (cx__capabilities.Dom1HTML && pl)
    //	pl.parentLayer.appendChild(ll);
    moveAbove(ll,pl);
    //moveToAbsolute(ll,x,y);
    $(ll).offset({left:x, top:y});
    htr_setzindex(ll,z);

    resizeTo(rl,1,h+1);
    setClipHeight(rl,h+1);
    setClipWidth(rl,1);
    //if (cx__capabilities.Dom1HTML && pl)
    //	pl.parentLayer.appendChild(rl);
    moveAbove(rl,pl);
    //moveToAbsolute(rl,x+w-s+1,y);
    $(rl).offset({left:x+w-s+1, top:y});
    htr_setzindex(rl,z);
    
    htr_setvisibility(tl, 'inherit');
    htr_setvisibility(bl, 'inherit');
    htr_setvisibility(rl, 'inherit');
    htr_setvisibility(ll, 'inherit');
    //alert(rl.style.cssText);
    return;
    }

/** To hide a box **/
function pg_hidebox(tl,bl,rl,ll)
    {
    htr_setvisibility(tl,'hidden');
    htr_setvisibility(bl,'hidden');
    htr_setvisibility(rl,'hidden');
    htr_setvisibility(ll,'hidden');
    
    if (cx__capabilities.Dom0NS)
        {    
        tl.moveAbove(document.layers.pgtvl);
        bl.moveAbove(document.layers.pgtvl);
        rl.moveAbove(document.layers.pgtvl);
        ll.moveAbove(document.layers.pgtvl);
        }
    else if (cx__capabilities.Dom1HTML)
        {
        moveAbove(tl, document.getElementById("pgtvl"));
        moveAbove(bl, document.getElementById("pgtvl"));
        moveAbove(rl, document.getElementById("pgtvl"));
        moveAbove(ll, document.getElementById("pgtvl"));
        }
        
    return;
    }

/** Function to make a new clickable "area" **INTERNAL** **/
function pg_area(pl,x,y,w,h,cls,nm,f) //SETH: ?? what's an 'area'?
    {
    this.layer = pl;
    this.x = x;
    this.y = y;
    this.width = w;
    this.height = h;
    this.name = nm;
    this.cls = cls;
    this.flags = f;
    return this;
    }

/** Function to resize a given page area **/
function pg_resize_area(a,w,h,xo,yo)
    {
    if (xo == null) xo = a.x;
    if (yo == null) yo = a.y;
    var x=getPageX(a.layer)+xo;
    var y=getPageY(a.layer)+yo;
    if (cx__capabilities.Dom0NS)
	{
	var tl=document.layers.pgtop;
	var bl=document.layers.pgbtm;
	var ll=document.layers.pglft;
	var rl=document.layers.pgrgt;
	}
    else if (cx__capabilities.Dom1HTML)
	{
	var tl=document.getElementById("pgtop");
	var bl=document.getElementById("pgbtm");
	var ll=document.getElementById("pglft");
	var rl=document.getElementById("pgrgt");
	}
    a.width = w;
    a.height = h;
    a.x = xo;
    a.y = yo;
    if (htr_getvisibility(tl) == 'inherit')
	{
	resizeTo(tl, w,1);
	setClipWidth(tl, w);
	resizeTo(bl, w+1,1);
	setClipWidth(bl, w+1);
	resizeTo(rl, 1,h+1);
	setClipHeight(rl, h+1);
	resizeTo(ll, 1,h);
	setClipHeight(ll, h);
	moveToAbsolute(tl, x,y);
	moveToAbsolute(bl, x,y+h);
	moveToAbsolute(ll, x,y);
	moveToAbsolute(rl, x+w,y);
	}
    if (cx__capabilities.Dom0NS)
	{
	tl=document.layers.pgktop;
	bl=document.layers.pgkbtm;
	ll=document.layers.pgklft;
	rl=document.layers.pgkrgt;
	}
    else
	{
	tl=document.getElementById("pgktop");
	bl=document.getElementById("pgkbtm");
	ll=document.getElementById("pgklft");
	rl=document.getElementById("pgkrgt");
	}
    if (htr_getvisibility(tl) == 'inherit')
	{
	resizeTo(tl, w,1);
	setClipWidth(tl, w);
	resizeTo(bl, w+1,1);
	setClipWidth(bl, w+1);
	resizeTo(rl, 1,h+1);
	setClipHeight(rl, h+1);
	resizeTo(ll, 1,h);
	setClipHeight(ll, h);
	moveToAbsolute(tl, x,y);
	moveToAbsolute(bl, x,y+h);
	moveToAbsolute(ll, x,y);
	moveToAbsolute(rl, x+w,y);
	}
    }

/** Function to add a new area to the arealist **/
function pg_addarea(pl,x,y,w,h,cls,nm,f)
    {    
    var a = new pg_area(pl,x,y,w,h,cls,nm,f);
    //pg_arealist.splice(0,0,a);
    pg_arealist.push(a);
    return a;
    }

/** Function to remove an existing area... **/
function pg_removearea(a)
    {
    for(var i=0;i<pg_arealist.length;i++)
	{
	if (pg_arealist[i] == a)
	    {
	    if (a == pg_curarea) pg_removemousefocus();
	    if (a == pg_curkbdarea) pg_removekbdfocus();
	    pg_arealist.splice(i,1);
	    return 1;
	    }
	}
    return 0;
    }

/** Add a universal resize manager function. **/
function pg_resize(l)
    {
    var maxheight=0;
    var maxwidth=0;
    var layers = pg_layers(l);
    if(!layers)
	{
	alert("Cannot resize " + l.id + l.name + "(" + l + ") -- no child layers");
	return;
	}
    for(var i=0;i<layers.length;i++)
	{
	var cl = layers[i];
	var visibility = htr_getvisibility(cl);
	if (visibility == 'show' || visibility == 'visible' || visibility == 'inherit') 
	    {
	    //var clh = getRelativeY(cl) + getClipHeight(cl) + getClipTop(cl);
	    var clh = getRelativeY(cl) + getClipBottom(cl);
	    //var clw = getRelativeX(cl) + getClipWidth(cl) + getClipLeft(cl);
	    var clw = getRelativeX(cl) + getClipRight(cl);
	    if(clh > maxheight)
		maxheight = clh;
	    if(clw > maxwidth)
		maxwidth = clw;
	    }
	}

    if (l.maxheight && maxheight > l.maxheight) maxheight = l.maxheight;
    if (l.minheight && maxheight < l.minheight) maxheight = l.minheight;
    if (l.maxwidth && maxwidth > l.maxwidth) maxwidth = l.maxwidth;
    if (l.minwidth && maxwidth < l.minwidth) maxwidth = l.minwidth;

    if (l!=window) 
	{
	maxheight -= getClipTop(l);
	maxwidth -= getClipLeft(l);
	setClipHeight(l, maxheight);
	setClipWidth(l, maxwidth);
	}
    else 
	{
	if(cx__capabilities.Dom0NS)
	    {
	    l.document.height = maxheight;
	    l.document.width = maxwidth;
	    }
	else
	    {
	    alert("Cannot call pg_resize on a window on a non-NS4 browser");
	    }
	}
    if (l.resized)
	l.resized(maxwidth, maxheight);
    }

/** Add a universal "is visible" function that handles inherited visibility. **/
function pg_isvisible(l)
    {
    var visibility = pg_get_style(l,'visibility');
    if (visibility == 'show') return 1;
    if (visibility == 'visible') return 1;
    else if (visibility == 'hidden') return 0;
    else if (l == window || l.parentLayer == null || l.parentNode == null) return 1;
    else 
	{
	if(l.parentLayer)
	    return pg_isvisible(l.parentLayer);
	else
	    return pg_isvisible(l.parentNode);
	}
    }

/// This routine searches for the 'windowing container' of a widget, such
/// as a childwindow or the main document itself.
function pg_searchwin(l)
    {
    if (l.kind && l.kind == 'wn') return l.mainlayer;
    if (l == document || l == window) return l;
    if (l.parentLayer)
	return pg_searchwin(l.parentLayer);
    else
	return pg_searchwin(l.parentNode);
    }

/// This routine brings the given layer to a stacking position above the
/// 'window' that it is in - which is used for doing popups.
function pg_stackpopup(p,l)
    {
    var win = pg_searchwin(l);
    var doclayers = pg_layers(document);
    var found_win = null;
    var min_z = 1000;
    for(var i = 0; i < doclayers.length; i++)
	{
	if (doclayers[i].kind == 'wn' && htr_getzindex(doclayers[i]) > min_z)
	    {
	    found_win = doclayers[i];
	    min_z = htr_getzindex(doclayers[i]);
	    }
	}

    if (win == window || win == document)
	{
	// work around firebug bug
	if (doclayers[0].id == '_firebugConsole')
	    found_win = doclayers[1];
	else
	    found_win = doclayers[0];
	moveAbove(p,found_win);
	htr_setzindex(p, min_z + 1000);
	}
    else
	{
	moveAbove(p,win);
	htr_setzindex(p, min_z + 1000);
	}
    }

/// This function handles the positioning of a popup such that it will 
/// always appear fully on the screen.  Pass the layer being popped up
/// as well as the coordinates and height of the layer it is being
/// popped to, in page-absolute coordinates.
function pg_positionpopup(p, x, y, h, w)
    {
    var posx, posy;

    // Determine vertical (Y) position of where we can pop up...
    if (y + h + getClipHeight(p) < getInnerHeight() - 1)
	posy = y + h;
    else if (y - getClipHeight(p) >= 0)
	posy = y - getClipHeight(p);
    else
	posy = getInnerHeight() - getClipHeight(p);

    // Now determine horizontal (X) position...
    if (x + getClipWidth(p) <= getInnerWidth())
	posx = x;
    else if (x + w - getClipWidth(p) >= 0)
	posx = x + w - getClipWidth(p);
    else
	posx = getInnerWidth() - getClipWidth(p);

    // Set the position
    moveToAbsolute(p, posx, posy);

    return;
    }

/** Cursor flash **/
function pg_togglecursor()
    {
    if (pg_curkbdlayer != null && pg_curkbdlayer.cursorlayer != null)
	{
	var cl = pg_curkbdlayer.cursorlayer;

	//status = cl.currentStyle.visibility;
	//status = cl.style.left + " " + cl.style.top;
	//resizeTo(cl, 100, 100);
	
	//status = cl.runtimeStyle.zIndex;
	
	if (htr_getvisibility(cl) != 'inherit')
	    htr_setvisibility(cl,'inherit');
	else
	    htr_setvisibility(cl,'hidden');
	}
    setTimeout(pg_togglecursor,333);
    }

/** Keyboard input handling **/
function pg_addkey(s,e,mod,modmask,mlayer,klayer,tgt,action,aparam) //SETH: ??
    {
    var kd = {};
    kd.startcode = s;
    kd.endcode = e;
    kd.mod = mod;
    kd.modmask = modmask;
    kd.mouselayer = mlayer;
    kd.kbdlayer = klayer;
    kd.target_obj = tgt;
    kd.fnname = 'Action' + action;
    kd.aparam = aparam;
    pg_keylist.splice(0,0,kd);
    pg_keylist.sort(pg_cmpkey);
    return kd;
    }
    
function pg_cmpkey(k1,k2) //SETH: ??
    {
    return (k1.endcode-k1.startcode) - (k2.endcode-k2.startcode);
    }
    
function pg_removekey(kd)
    {
    for(var i=0;i<pg_keylist.length;i++)
	{
	if (pg_keylist[i] == kd)
	    {
	    pg_keylist.splice(i,1);
	    return 1;
	    }
	}
    return 0;
    }
    
function pg_keytimeout()
    {
    pg_keyschedid = 0;
    if (pg_lastkey != -1)
	{
	e = new Object();
	e.which = pg_lastkey;
	e.modifiers = pg_lastmodifiers;
	pg_keyhandler(pg_lastkey, pg_lastmodifiers, e);
	delete e;
	pg_keytimeoutid = setTimeout(pg_keytimeout, 50);
	}
    }

function pg_keyuphandler(k,m,e)
    {
    return true;
    }

function pg_keypresshandler(k,m,e)
    {
    return pg_keyhandler_internal(k,m,e);
    }

function pg_keyhandler(k,m,e)
    {
    //alert(this.caller);

    // block non-special codes for IE here - handle em in keypress, not keydown.
    //if (cx__capabilities.Dom0IE || cx__capabilities.Dom2Events)
    if (cx__capabilities.Dom0IE)
	{
	if (k >= 32 && k != 46)
	    return true;

	// IE passes DEL in as code 46 for keydown.  ASC(46), the period,
	// is passed as code 190 here but code 46 in the keypress event.  
	// Go figure.
	if (k == 46) 
	    k = 127;
	}

    var r = pg_keyhandler_internal(k,m,e);
    return  r;
    }

function pg_keyhandler_internal(k,m,e)
    {
    // can't capture ctrl-V, ctrl-C, ctrl-X as keypresses.
    //if (e.ctrlKey && (k == 118 || k == 99 || k == 120))
    //	return true;

    //htr_alert_obj(e,1);
    // layer.keyhandler is a callback routine that is optional 
    // on any layer requesting focus with pg_addarea().
    // It is set up in the corresponding widget drivers.
    if (pg_curkbdlayer != null && 
	pg_curkbdlayer.keyhandler != null)
	{
	if (pg_curkbdlayer.keyhandler(pg_curkbdlayer,e,k) == true)
	    return false;       
	else
	    return true;
	}
      
    for(var i=0;i<pg_keylist.length;i++)
	{
	if (k >= pg_keylist[i].startcode && k <= pg_keylist[i].endcode && (pg_keylist[i].kbdlayer == null || pg_keylist[i].kbdlayer == pg_curkbdlayer) && (pg_keylist[i].mouselayer == null || pg_keylist[i].mouselayer == pg_curlayer) && (m & pg_keylist[i].modmask) == pg_keylist[i].mod)
	    {
	    pg_keylist[i].aparam.KeyCode = k;
	    if (pg_keylist[i].target_obj[pg_keylist[i].fnname](pg_keylist[i].aparam)) return true;
	    return false;
	    }
	}
    return true;
    }
 
function pg_status_init() //SETH: ??
    {
    pg_status = null;
    if(cx__capabilities.Dom1HTML)
	{
	pg_status = document.getElementById("pgstat");
	}
    else if(cx__capabilities.Dom0NS)
	{
	pg_status = document.layers.pgstat;
	}
    else if(cx__capabilities.Dom0IE)
	{
	pg_status = document.all.pgstat;
	}
    else
	{
	return false;
	}
    
    if (!pg_status)
	return false;
    pg_status.zIndex = 1000000;
    pg_status.visibility = 'visible';
    }
 
function pg_status_close()
    {
    if (!pg_status) return false;
    pg_status.visibility = 'hide';
    }


function pg_appwindowsync()
    {
    this.AppWindowBuild({});
    this.AppWindowPropagate();
    }

function pg_appwindowbuild(seen)
    {
    seen[wgtrGetNamespace(this)] = this.pg_appwindows[wgtrGetNamespace(this)];
    for(var win in this.pg_appwindows)
	{
	if (this.pg_appwindows[win].wobj != 'undefined' && wgtrGetNamespace(this.pg_appwindows[win].wobj) != win)
	    {
	    // has been reloaded, ignore this instance
	    delete pg_appwindows[win];
	    }
	else if (typeof seen[win] == 'undefined' && typeof this.pg_appwindows[win].wobj != 'undefined' && !this.pg_appwindows[win].wobj.closed)
	    {
	    // good window, unseen as of yet, and it is open... query it for its list.
	    var otherlist = this.pg_appwindows[win].wobj.AppWindowBuild(seen);
	    for(var otherwin in otherlist)
		{
		this.pg_appwindows[otherwin] = otherlist[otherwin];
		}
	    }
	}

    return this.pg_appwindows;
    }

function pg_appwindowprop()
    {
    for(var win in this.pg_appwindows)
	{
	var w = this.pg_appwindows[win];
	for(var win2 in this.pg_appwindows)
	    {
	    var w2 = this.pg_appwindows[win2];
	    if (w2.wobj && !w2.wobj.closed)
		{
		if (!w.wobj || w.wobj.closed)
		    delete w2.wobj.pg_appwindows[win];
		else
		    w2.wobj.pg_appwindows[win] = w;
		}
	    }
	}
    }

function pg_init(l,a,gs,ct) //SETH: ??
    {
    window.windowlist = {};
    window.pg_appwindows = {};
    pg_attract = a;
    if (cx__capabilities.Dom0NS) pg_set_emulation(document);
    htr_init_layer(window,window,"window");
    if (typeof window.name == 'undefined')
	window.name = 'window';
    pg_reveal_register_triggerer(window);

    // pg_init runs early in the init phase.  We delay the Reveal event
    // until after everything else has had a chance to run.  pg_reveal_event
    // also does addsched(), so that means everything has a chance to
    // schedule 0-timeout events before the Reveal occurs.
    pg_addsched_fn(window, function() { pg_reveal_event(window,null,'Reveal'); }, [], 0);

    pg_addsched_fn(window, function() { pg_msg_init(); }, [], 0);
    ifc_init_widget(window);

    l.templates = [];
    window.AppWindowSync = pg_appwindowsync;
    window.AppWindowBuild = pg_appwindowbuild;
    window.AppWindowPropagate = pg_appwindowprop;

    // update app window lists
    pg_appwindows[wgtrGetNamespace(window)] = {wname:wgtrGetName(window), wobj:window, opener:window.opener, namespace:wgtrGetNamespace(window)};
    if (window.opener && typeof window.opener.akey != 'undefined' && window.opener.akey.substr(0,49) == window.akey.substr(0,49))
	{
	// link with opener and propagate window lists
	pg_appwindows[wgtrGetNamespace(window.opener)] = {wname:wgtrGetName(window.opener), wobj:window.opener, opener:window.opener.opener, namespace:wgtrGetNamespace(window.opener)};
	window.AppWindowSync();
	}

    // Actions
    var ia = window.ifcProbeAdd(ifAction);
    ia.Add("LoadPage", pg_load_page);
    ia.Add("Launch", pg_launch);
    ia.Add("Close", pg_close);
    ia.Add("Alert", pg_alert);

    // Events
    var ie = window.ifcProbeAdd(ifEvent);
    ie.Add("RightClick");
    ie.Add("Load");

    // Reveal
    l.Reveal = pg_reveal_cb;

    // Check time
    if (window.pg_servertime && window.pg_clienttime && window.pg_servertime_notz)
	{
	// Warn the user if their clock & server's clock are not in sync.
	if (Math.abs(pg_servertime - pg_clienttime) > 60*10*1000)
	    {
	    var mins = Math.floor(Math.abs(pg_servertime - pg_clienttime) / 1000 / 60);
	    alert("Please double-check your computer's date, time, and timezone; it is " + mins + " minutes different than the server's date & time");
	    }

	// Milliseconds our TZ is different than the server.
	// A positive value means our TZ is further east.
	pg_clockoffset = pg_clienttime - pg_servertime_notz;
	}

    pg_addsched_fn(window, function() { window.ifcProbe(ifEvent).Activate("Load", {}); }, [], 100);

    // This input receives paste events on behalf of the entire app
    window.paste_input = document.createElement('input');
    $(window.paste_input).css
	({
	'left': '0px',
	'top': '-30px',
	'position': 'absolute',
	'width': '100px',
	'height': '20px',
	});
    document.body.appendChild(window.paste_input);
    //window.paste_input.focus();

    return window;
    }

function pg_cleanup()
    {
    // remove this window from the app window lists
    if (window.pg_appwindows && window.pg_appwindows[wgtrGetNamespace(window)])
	{
	window.pg_appwindows[wgtrGetNamespace(window)].wobj = null;
	window.AppWindowPropagate();
	}

    // Deinit the tree.
    wgtrDeinitTree(window);
    }

function pg_alert(aparam)
    {
    if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);
    if (pg_keyschedid) pg_delsched(pg_keyschedid);
    pg_keyschedid = 0;
    pg_keytimeoutid = 0;
    alert(aparam.Message);
    }

function pg_reveal_cb(e)
    {
    if (e.eventName == 'ObscureOK')
	pg_close_bh();
    }

function pg_close(aparam)
    {
    pg_reveal_event(window, null, 'ObscureCheck');
    }

function pg_close_bh()
    {
    window.close();
    }

function pg_load_page(aparam) //SETH: ??
    {
    var newurl = '';
    if (typeof aparam.Source != 'undefined')
	newurl = aparam.Source;
    else
	newurl = window.location.href;
    for(var p in aparam)
	{
	if (p == '_Origin' || p == 'Source' || p == '_EventName') continue;
	var v = aparam[p];
	if (newurl.lastIndexOf('?') > newurl.lastIndexOf('/'))
	    newurl += '&';
	else
	    newurl += '?';
	newurl += (htutil_escape(p) + '=' + htutil_escape(v));
	}

    // session linkage
    if (newurl.substr(0,1) == '/')
	{
	if (newurl.lastIndexOf('?') > newurl.lastIndexOf('/'))
	    newurl += '&';
	else
	    newurl += '?';
	newurl += "cx__akey=" + window.akey.substr(0,49);
	}

    window.location.href = newurl;
    }

function pg_launch(aparam)
    {
    // launch an app/rpt in a new window
    var w_name;
    var w_exists = false;
    if (aparam.Name == null)
	w_name = "new_window";
    else
	w_name = aparam.Name;
    w_name = wgtrGetNamespace(window) + '_' + w_name;

    // build the URL with parameters
    var url = new String(aparam.Source);
    for(var p in aparam)
	{
	if (p == '_Origin' || p == '_EventName' || p == 'Multi' || p == 'Name' || p == 'Width' || p == 'Height' || p == 'Source' || p == 'LinkApp')
	    continue;
	var v = aparam[p];
	var r = wgtrCheckReference(v);
	if (r) v = r;
	if (url.lastIndexOf('?') > url.lastIndexOf('/'))
	    url += '&';
	else
	    url += '?';
	url += (htutil_escape(p) + '=' + htutil_escape(v));
	}

    if (obscure_data)
	{
	if (url.lastIndexOf('?') > url.lastIndexOf('/'))
	    url += '&';
	else
	    url += '?';
	url += "cx__obscure=yes";
	}

    // session linkage
    if (url.substr(0,1) == '/')
	{
	if (url.lastIndexOf('?') > url.lastIndexOf('/'))
	    url += '&';
	else
	    url += '?';
	if (aparam.LinkApp !== null && (aparam.LinkApp == 'yes' || aparam.LinkApp == 1))
	    url += "cx__akey=" + window.akey;
	else
	    url += "cx__akey=" + window.akey.substr(0,49);
	}

    // Find a unique name for the new window.
    if (aparam.Multi != null && (aparam.Multi == true || aparam.Multi == 1))
	{
	for(var i = 0; i < 64; i++) // 64 max multi-instanced windows
	    {
	    var w_instance_name = w_name + '_' + i;
	    if (!window.windowlist[w_instance_name] || !window.windowlist[w_instance_name].close)
		{
		w_name = w_instance_name;
		break;
		}
	    }
	}

    // Mailto?  We handle this differently if so.
    if (url.substr(0,7) == 'mailto:')
	{
	$('<iframe src="' + htutil_encode(url) + '">').appendTo('body').css('display', 'none');
	return;
	}

    // Already exists?
    if (window.windowlist[w_name] && window.windowlist[w_name].close) w_exists = true;
    if (!aparam.Multi && w_exists) 
	{
	window.windowlist[w_name].close();
	w_exists = false;
	}

    // Compute the height
    var h = aparam.Height;
    if (window.devicePixelRatio)
	h *= window.devicePixelRatio;

    // Open it.
    if (!w_exists) 
	{
	if (aparam.UseragentMenu != null && aparam.UseragentMenu && aparam.UseragentMenu != 'no')
	    var menubar = ",menubar=yes";
	else
	    var menubar = ",menubar=no";
	if (aparam.UseragentResize != null && aparam.UseragentResize && aparam.UseragentResize != 'no')
	    var resizable = ",resizable=yes";
	else
	    var resizable = ",resizable=no";
	if (aparam.UseragentScroll != null && aparam.UseragentScroll && aparam.UseragentScroll != 'no')
	    var scroll = ",scrollbars=yes";
	else
	    var scroll = ",scrollbars=no";
	window.windowlist[w_name] = window.open(url, w_name, "toolbar=no" + scroll + ",innerHeight=" + h + ",innerWidth=" + aparam.Width + ",personalbar=no,status=no" + menubar + resizable);
	}
    }

function pg_mvpginpt(ly)
    {

    if (!cx__capabilities.Dom0IE)
        {
        pg_layer = ly;
        }
    var a=(getdocHeight()-getInnerHeight()-2)>=0?16:1;
    var b=(getdocWidth()-getInnerWidth()-2)>=0?22:5;
    
    moveTo(pg_layer,getInnerWidth()-a+getpageXOffset(), getInnerHeight()-b+getpageYOffset());
    if (a>1||b>5) 
    	setTimeout(pg_mvpginpt, 500, pg_layer);
    }

//START SECTION: setTimout wrappers ------------------------------------------
//these are wrappers for setTimeout as a NS4 hack since NS4 had a small, limited number of timers that could run simultaniously.

// see pg_addsched_fn
function pg_addschedtolist(s)
    {
    var len = pg_schedtimeoutlist.length;
    var insert = len; 
    var reset_timer = (!len) || (s.tm < pg_schedtimeoutlist[0].tm);

    if (reset_timer)
	pg_stopschedtimeout();
    pg_msglist += ('' + pg_timestamp() + ': adding item ' + s.id + ' at time ' + s.tm + ' (' + (s.exp?s.exp:s.func) + ')\n');
    if (len > 0)
	{
	for(var i=0;i<len;i++)
	    {
	    if (s.tm < pg_schedtimeoutlist[i].tm)
		{
		insert = i;
		break;
		}
	    }
	}
    if (insert == len)
	pg_schedtimeoutlist.push(s);
    else
	pg_schedtimeoutlist.splice(insert, 0, s);
    if (reset_timer)
	pg_startschedtimeout();
    }

// see pg_addsched_fn
function pg_stopschedtimeout()
    {
    if (pg_schedtimeout)
	{
	clearTimeout(pg_schedtimeout);
	pg_schedtimeout = null;
	}
    }

// see pg_addsched_fn
function pg_startschedtimeout()
    {
    if(!pg_schedtimeout && pg_schedtimeoutlist.length > 0) 
	{
	pg_schedtimeoutstamp = pg_timestamp();
	var len = pg_schedtimeoutlist[0].tm - pg_schedtimeoutstamp;
	if (len < 0)
	    len = 0;
	if (!pg_isloaded)
	    len = Math.max(len,100);
	pg_schedtimeout = setTimeout(pg_dosched, len);
	}
    }

// see pg_addsched_fn
function pg_addsched(e,o,t)
    {
    var sched = {exp:e, obj:o, tm:pg_timestamp() + t, id:pg_schedtimeoutid++};
    pg_addschedtolist(sched);
    return sched.id;
    }

//this is a wrapper for setTimeout as a NS4 hack since NS4 had a small, limited number of timers that could run simultaniously.
function pg_addsched_fn(o,f,p,t)
    {
    var sched = {func:f, obj:o, param:p, tm:pg_timestamp() + t, id:pg_schedtimeoutid++};
    pg_addschedtolist(sched);
    return sched.id;
    }

// see pg_addsched_fn
function pg_delsched(id)
    {
    for(var i=0;i<pg_schedtimeoutlist.length;i++)
	{
	if (pg_schedtimeoutlist[i].id == id)
	    {
	    pg_stopschedtimeout();
	    pg_schedtimeoutlist.splice(i, 1);
	    pg_startschedtimeout();
	    return true;
	    }
	}
    return false;
    }

function pg_dosched(do_all)
    {
    pg_schedtimeout = null;
    var sched_item = null;
    var now = pg_timestamp();
    window.pg_isloaded = true;
    if (pg_schedtimeoutlist.length > 0)
    	{
	// Make a note of current scheduler event id, so we don't do any events
	// newly added by the below scheduler callbacks until a setTimeout()
	// expires, even for 0-length events.
	if (do_all)
	    var maxid = 99999999;
	else
	    var maxid = pg_schedtimeoutid - 1;
	while (pg_schedtimeoutlist.length > 0 && pg_schedtimeoutlist[0].tm <= now && pg_schedtimeoutlist[0].id <= maxid)
	    {
	    sched_item = pg_schedtimeoutlist.shift();
	    //pg_debug('' + (pg_timestamp()) + ': running item ' + sched_item.id + '\n');

	    var s = sched_item;
	    pg_msglist += ('' + pg_timestamp() + ': doing item ' + s.id + ' sched for ' + s.tm + ' (' + (s.exp?s.exp:s.func) + ')\n');
	    if (sched_item.exp)
		{
		// evaluate expression
		var _context = null;
		var _this = sched_item.obj;
		if (wgtrIsNode(sched_item.obj))
		    _context = wgtrGetRoot(sched_item.obj);
		with (sched_item.obj) { eval(sched_item.exp); }
		}
	    else
		{
		// call function
		var _context = null;
		if (wgtrIsNode(sched_item.obj))
		    _context = wgtrGetRoot(sched_item.obj);
		if (typeof sched_item.func == 'function')
		    sched_item.func.apply(sched_item.obj, sched_item.param);
		else
		    sched_item.obj[sched_item.func].apply(sched_item.obj, sched_item.param);
		}
	    }
	}

    pg_startschedtimeout();
    }

function pg_timestamp()
    {
    return (new Date()).valueOf() - pg_init_ts;
    }

//END SECTION: setTimout wrappers ------------------------------------------

//START SECTION: 'expression' functions ----------------------------------------

function pg_explisten(exp, obj, prop)
    {
    obj.pg_expchange = pg_expchange;
    if (obj.ifcProbe && obj.ifcProbe(ifValue) && obj.ifcProbe(ifValue).Exists(prop))
	obj.ifcProbe(ifValue).Watch(prop, null, pg_expchange);
    else
	htr_watch(obj,prop,"pg_expchange");
    }

function pg_expaddpart(exp, obj, prop)
    {
    var ref;
    var objname = obj.__WgtrName;
    if (obj.reference && (ref = obj.reference()))
	obj = ref;
    for(var i=0; i<exp.ParamList.length; i++)
	{
	var item = exp.ParamList[i];
	if (obj == item[2] && prop == item[1]) return;
	if (objname == item[0] && !item[2] && prop == item[1])
	    {
	    item[2] = obj;
	    return;
	    }
	}
    //var _context = window[exp.Context];
    //var nodelist = wgtrNodeList(_context);
    var item=[objname, prop, obj];
    exp.ParamList.push(item);
    pg_explisten(exp, obj, prop);
    }

function pg_expression(o,p,e,l,c)
    {
    var expobj = {};
    expobj.Objname = o;
    expobj.Propname = p;
    expobj.Expression = e;
    expobj.ParamList = l;
    expobj.Context = c;
    //var _context = window[c];
    var _context = c;
    //var nodelist = wgtrNodeList(_context);
    var node = wgtrGetNode(_context, expobj.Objname);
    var _this = node;
    window.__cur_exp = expobj;
    //wgtrSetProperty(node, expobj.Propname, eval(expobj.Expression));
    wgtrSetProperty(node, expobj.Propname, expobj.Expression(_context,_this));
    pg_explist.push(expobj);
    for(var i=0; i<l.length; i++)
	{
	var item = l[i];
	var ref;
	if (item[0] == "*") continue; // cannot handle global listening yet
	//item[2] = nodelist[item[0]]; // get obj reference
	item[2] = wgtrGetNodeUnchecked(_context, item[0]); // get obj reference
	if (item[2])
	    {
	    if (item[2].reference && (ref = item[2].reference()))
	    	item[2] = ref;
	    pg_explisten(expobj, item[2], item[1]);
	    }
	}
    }

function pg_expchange_cb(exp) //SETH: ??
    {
    //var _context = window[exp.Context];
    var _context = exp.Context;
    var node = wgtrGetNode(_context, exp.Objname);
    var _this = node;
    window.__cur_exp = exp;
    //var v = eval(exp.Expression);
    var v = exp.Expression(_context,_this);
    //pg_explog.push('assign: obj ' + node.__WgtrName + ', prop ' + exp.Propname + ', nv ' + v + ', exp ' + exp.Expression);
    wgtrSetProperty(node, exp.Propname, v);
    }

function pg_expchange(p,o,n)
    {
    if (o==n) return n;
    /*if (this && this.__WgtrName == 'donor_osrc' && p == 'p_given_name' && pg_username == 'dbeeley')
	{
	pg_explog.push('expchange: id ' + this.id + ' ' + p + ' => ' + o + ' to ' + n + ' pg_explist len ' + pg_explist.length + ' paramlist len ' + pg_explist[8].ParamList.length);
	if (!pg_explist[8]) pg_explog.push('no exp');
	else if (!pg_explist[8].ParamList[0]) pg_explog.push('no param');
	else if (!pg_explist[8].ParamList[0][2]) pg_explog.push('no ref');
	else if (pg_explist[8].ParamList[0][2] != this) pg_explog.push('obj discrep ' + this.__WgtrName + ' != ' + pg_explist[8].ParamList[0][2].__WgtrName);
	else if (pg_explist[8].ParamList[0][1] != p) pg_explog.push('attr discrep ' + p + ' != ' + pg_explist[8].ParamList[0][1]);
	else pg_explog.push('no prob found');
	}*/
    var str = '';
    for(var i=0;i<pg_explist.length;i++)
	{
	//str += '' + i + ':';
	var exp = pg_explist[i];
	for(var j=0;j<exp.ParamList.length;j++)
	    {
	    //str += '' + j + ',';
	    var item = exp.ParamList[j];
	    //if (this == item[2]) str += 't';
	    //if (p == item[1]) str += 'p';
	    //if (this && this.__WgtrName == 'donor_osrc' && p == 'p_given_name' && pg_username == 'dbeeley' && i == 8 && j == 0)
	//	str += ' ' + this.id + ' ';
	    //var prevcmp = (this == item[2]);
	    // THE BELOW LINE IS NEEDED TO WORK AROUND A FIREFOX BUG
	    var str = this?(' ' + this.id + ' ' + this.__WgtrName):'';
	    //if (!prevcmp && this == item[2]) pg_explog.push('wow!');
	    if (this == item[2] && p == item[1])
		{
		//alert("eval " + exp.Objname + "." + exp.Propname + " = " + exp.Expression);
		//pg_explog.push('change: obj ' + ((this && this.__WgtrName)?this.__WgtrName:'(unknown)') + ', prop ' + p + ', ov ' + o + ', nv ' + n + ', exp ' + exp.Expression);
		pg_addsched_fn(window, 'pg_expchange_cb', [exp], 0);
		}
	    }
	//str += '.  ';
	}
    /*if (this && this.__WgtrName == 'donor_osrc' && p == 'p_given_name' && pg_username == 'dbeeley')
	{
	pg_explog.push(str);
	pg_explog.push('expchange end: ' + p + ' => ' + o + ' to ' + n);
	}*/
    return n;
    }

//END SECTION: 'expression' functions ----------------------------------------

function pg_reclaim_objects()
    {
    pg_hidebox(document.getElementById("pgtop"),document.getElementById("pgbtm"),document.getElementById("pgrgt"),document.getElementById("pglft"));
    pg_hidebox(document.getElementById("pgktop"),document.getElementById("pgkbtm"),document.getElementById("pgkrgt"),document.getElementById("pgklft"));
    if (ibeam_current) moveAbove(ibeam_current, document.getElementById("pgtvl"));
    }

//SETH: this function seems to implement the 'blur' event.
function pg_removemousefocus()
    {
    if (pg_curarea.layer.losemousefocushandler) 
	pg_curarea.layer.losemousefocushandler(pg_curarea.layer, pg_curarea.cls, pg_curarea.name, pg_curarea);
    if (cx__capabilities.Dom0NS)
        {
    	if (pg_curarea.flags & 1) pg_hidebox(document.layers.pgtop,document.layers.pgbtm,document.layers.pgrgt,document.layers.pglft);
    	}
    else if (cx__capabilities.Dom1HTML)
    	{
    	if (pg_curarea.flags & 1) pg_hidebox(document.getElementById("pgtop"),document.getElementById("pgbtm"),document.getElementById("pgrgt"),document.getElementById("pglft"));
    	}    
    pg_curarea = null;
    return true;
    }

function pg_findfocusarea(l, xo, yo)
    {
    for(var i=0;i<pg_arealist.length;i++) 
	{	
	if (l == pg_arealist[i].layer && ((xo == null && yo == null) || (xo >= pg_arealist[i].x &&
	    yo >= pg_arealist[i].y && xo < pg_arealist[i].x+pg_arealist[i].width &&
	    yo < pg_arealist[i].y+pg_arealist[i].height && l != pg_arealist[i])))
	    {
	    if (l == pg_arealist[i]) break;
	    //alert("focus area -- " + i);
	    return pg_arealist[i];
	    }
	}
    return null;
    }

function pg_setmousefocus(l, xo, yo)
    {
    var a = pg_findfocusarea(l, xo, yo);
    if (a && a != pg_curarea)
	{
	pg_curarea = a;
	if (pg_curarea.flags & 1)
	    {
	    if (!pg_curarea.layer.getmousefocushandler || pg_curarea.layer.getmousefocushandler(xo, yo, a.layer, a.cls, a.name, a))
		{
		// wants mouse focus
		var offs = $(pg_curarea.layer).offset();
		//var x = getPageX(pg_curarea.layer)+pg_curarea.x;
		//var y = getPageY(pg_curarea.layer)+pg_curarea.y;
		var x = offs.left+pg_curarea.x;
		var y = offs.top+pg_curarea.y;
		
		var w = pg_curarea.width;
		var h = pg_curarea.height;
		if (cx__capabilities.Dom0NS)
		    {
		    pg_mkbox(l, x,y,w,h, 1, document.layers.pgtop,document.layers.pgbtm,document.layers.pgrgt,document.layers.pglft, page.mscolor1, page.mscolor2, document.layers.pgktop.zIndex-1);
		    }
		else if (cx__capabilities.Dom1HTML)
		    {
		    pg_mkbox(l, x,y,w,h, 1, document.getElementById("pgtop"),document.getElementById("pgbtm"),document.getElementById("pgrgt"),document.getElementById("pglft"), page.mscolor1, page.mscolor2, htr_getzindex(document.getElementById("pgktop"))-1);
		    }
		}
	    }
	}
    }

function pg_removekbdfocus(p)
    {
    if (pg_curkbdlayer)
	{
	if (pg_curkbdlayer.losefocushandler && !pg_curkbdlayer.losefocushandler(p)) return false;
	pg_curkbdlayer = null;
	pg_curkbdarea = null;
	if (cx__capabilities.Dom0NS)
	    {
	    pg_mkbox(null,0,0,0,0, 1, document.layers.pgktop,document.layers.pgkbtm,document.layers.pgkrgt,document.layers.pgklft, page.kbcolor1, page.kbcolor2, document.layers.pgtop.zIndex+100);
	    }
	else if (cx__capabilities.Dom1HTML)
	    {
	    pg_mkbox(null,0,0,0,0, 1, document.getElementById("pgktop"),document.getElementById("pgkbtm"),document.getElementById("pgkrgt"),document.getElementById("pgklft"), page.kbcolor1, page.kbcolor2, pg_get_style(document.getElementById("pgtop"), 'zIndex')+100);
	    }
	}
    return true;
    }

function pg_setdatafocus(a)
    {
    var x = getPageX(a.layer)+a.x;
    var y = getPageY(a.layer)+a.y;
    var w = a.width;
    var h = a.height;
    var l = a.layer; 

    // hide old data focus box
    if (l.pg_dttop != null)
	{
	// data focus moving within a control - remove old one
	pg_hidebox(l.pg_dttop,l.pg_dtbtm,l.pg_dtrgt,l.pg_dtlft);
	}
    else
	{
//THESE NEEDED TO USE htr_new_layer(w,p) FUNCTION FROM ht_render.js
//also, this was not working correctly for the DOM1 anyway.
//htr_new_layer seems to get it right
//	if (cx__capabilities.Dom1HTML)
//	    {
//	    l.pg_dttop = document.createElement("div");
//	    l.pg_dttop.style.width = 1;
//	    l.pg_dtbtm = document.createElement("div");
//	    l.pg_dtbtm.style.width = 1;
//	    l.pg_dtrgt = document.createElement("div");
//	    l.pg_dtrgt.style.width = 2;
//	    l.pg_dtlft = document.createElement("div");
//	    l.pg_dtlft.style.width = 2;
//	    }
//	else if (cx__capabilities.Dom0NS)
//	    {
//	    l.pg_dttop = new Layer(1152);
//	    l.pg_dtbtm = new Layer(1152);
//	    l.pg_dtrgt = new Layer(2);
//	    l.pg_dtlft = new Layer(2);
//	    }
	
	// mk new data focus box for this control.
	// the htr_new_layer function takes care of Dom0 vs Dom1 differences
	l.pg_dttop = htr_new_layer(1,l);
	l.pg_dtbtm = htr_new_layer(1,l);
	l.pg_dtrgt = htr_new_layer(2,l);
	l.pg_dtlft = htr_new_layer(2,l);
	}

    // draw new data focus box
    if (cx__capabilities.Dom0NS)
	{	        
	pg_mkbox(l,x-1,y-1,w+2,h+2, 1, l.pg_dttop,l.pg_dtbtm,l.pg_dtrgt,l.pg_dtlft, page.dtcolor1, page.dtcolor2, document.layers.pgtop.zIndex+100);
	}
    else if (cx__capabilities.Dom1HTML)
	{
	pg_mkbox(l,x-1,y-1,w+2,h+2, 1, l.pg_dttop,l.pg_dtbtm,l.pg_dtrgt,l.pg_dtlft, page.dtcolor1, page.dtcolor2, pg_get_style(document.getElementById("pgtop"),'zIndex')+100);
	}
    }

function pg_setkbdfocus(l, a, xo, yo)
    {
    var from_kbd = false;
    if (xo == null && yo == null)
	{
	xo = 0;
	yo = 0;
	from_kbd = true;
	}
    if (!a)
	{
	a = pg_findfocusarea(l, xo, yo);
	}
    if (!a)
	{
	a = pg_findfocusarea(l, null, null);
	}
    if (!a)
	return false;

    var offs=$(a.layer).offset();
    //var x = getPageX(a.layer)+a.x;
    //var y = getPageY(a.layer)+a.y;
    var x = offs.left+a.x;
    var y = offs.top+a.y;
    var w = a.width;
    var h = a.height;
    var prevLayer = pg_curkbdlayer;
    var prevArea = pg_curkbdarea;
    var v = 0;
    pg_curkbdarea = a;
    pg_curkbdlayer = l;

    if (pg_curkbdlayer && pg_curkbdlayer.getfocushandler)
	{
	v=pg_curkbdlayer.getfocushandler(xo,yo,a.layer,a.cls,a.name,a,from_kbd);
	if (v & 1)
	    {
	    // mk box for kbd focus
	    //if (prevArea != a)
	//	{
	//	if (cx__capabilities.Dom0NS)
	//	    {
	//	    pg_mkbox(l ,x,y,w,h, 1, document.layers.pgktop,document.layers.pgkbtm,document.layers.pgkrgt,document.layers.pgklft, page.kbcolor1, page.kbcolor2, document.layers.pgtop.zIndex+100);
	//	    }
	//	else if (cx__capabilities.Dom1HTML)
	//	    {		    
		    pg_mkbox(l ,x,y,w,h, 1, document.getElementById("pgktop"),document.getElementById("pgkbtm"),document.getElementById("pgkrgt"),document.getElementById("pgklft"), page.kbcolor1, page.kbcolor2, htr_getzindex(document.getElementById("pgtop"))+100);
	//	    }
	//	}
	    }
	if (v & 2)
	    {
	    pg_setdatafocus(a);
	    }
	}

    // If area got keyboard or data focus, make sure the containers are
    // 'shown' as needed.
    if (v)
	{
	pg_show_containers(l);
	}
    else
	{
	pg_curkbdarea = null;
	pg_curkbdlayer = null;
	}

    return v?true:false;
    }


function pg_getrelcoord(l, sub_l)
    {
    var x = 0;
    var y = 0;
    while(sub_l && sub_l != l && sub_l != window)
	{
	x += getRelativeX(sub_l);
	y += getRelativeY(sub_l);
	sub_l = pg_get_container(sub_l);
	}
    return [x, y];
    }


//START SECTION: async request handling ----------------------------------

// pg_loadqueue_additem() - adds an item to the load queue, sorted by 'level'
function pg_loadqueue_additem(item)
    {
    var i = pg_loadqueue.length;
    while (i && pg_loadqueue[i-1].level > item.level) i--;
    pg_loadqueue.splice(i, 0, item);
    }

// Remove from the load queue
function pg_loadqueue_remove(item)
    {
    for(var i=0; i<pg_loadqueue.length; i++)
	{
	if (pg_loadqueue[i] == item)
	    {
	    pg_loadqueue.splice(i,1);
	    if (item.active)
		{
		item.active = false;
		if (item.lyr) item.lyr.__load_busy = false;
		pg_loadqueue_busy--;
		}
	    break;
	    }
	}
    }

// pg_serialized_write() - schedules the writing of content to a layer, so that
// we don't have the document open while stuff is happening from the server.
function pg_serialized_write(l, text, cb)
    {
    //pg_debug('pg_serialized_write: ' + pg_loadqueue.length + ': ' + l.name + ' loads "' + text.substring(0,100) + '"\n');
    //pg_loadqueue.push({lyr:l, text:text, cb:cb});
    pg_loadqueue_additem({level:1, type:'write', lyr:l, text:text, cb:cb, retry_cnt:0, silent:true, active:false});
    //pg_debug('pg_serialized_write: ' + pg_loadqueue.length + '\n');
    pg_serialized_load_doone();
    }


// pg_serialized_func() - schedules the running of a function (callback) in
// queued order.  Higher 'level' will wait for all lower 'level' items to
// complete (even if scheduled later) before it runs.
function pg_serialized_func(level, obj, func, params)
    {
    pg_loadqueue_additem({level:level, type:'func', lyr:obj, cb:func, params:params, retry_cnt:0, silent:true, active:false});
    //pg_serialized_load_doone();
    pg_loadqueue_check();
    }


// pg_serialized_load() - a wrapper for async requests. It schedules
// the reload of a layer, hidden or visible, from the server in a
// manner that keeps things serialized so server loads don't overlap.
function pg_serialized_load(l, newsrc, cb, silent)
    {
    pg_debug('pg_serialized_load: ' + pg_loadqueue.length + ': ' + l.name + ' loads ' + newsrc + '\n');
    pg_loadqueue_additem({level:1, type:'src', lyr:l, src:newsrc, cb:cb, retry_cnt:0, silent:silent, active:false});
    pg_debug('pg_serialized_load: ' + pg_loadqueue.length + '\n');
    pg_serialized_load_doone();
    }

// pg_serialized_load_doone() - loads the next item off of the
// serialized loader list. This is where the old-style async req is
// actually made.
function pg_serialized_load_doone()
    {
    if (pg_loadqueue_busy >= pg_max_requests) return;

    pg_loadqueue_check_spinner();

    // Find an item from the load queue
    //var one_item = pg_loadqueue.shift(); 
    var one_item = null;
    for(var i=0; i<pg_loadqueue.length; i++)
	{
	var item = pg_loadqueue[i];
	if (!item.active && (!item.lyr || !item.lyr.__load_busy))
	    {
	    // Activate
	    one_item = item;
	    one_item.lyr.__load_busy = true;
	    one_item.active = true;
	    pg_loadqueue_busy++;
	    break;
	    }
	}
    if (!one_item) return; // none found

    if  (!one_item.text) pg_debug('pg_serialized_load_doone: ' + pg_loadqueue.length + ': ' + one_item.lyr.name + ' loads ' + one_item.src + '\n');
    one_item.lyr.__pg_onload = one_item.cb;
    switch(one_item.type)
	{
	case 'src':
	    one_item.lyr.onload = () => { pg_serialized_load_cb(one_item); };
	    one_item.lyr.onerror = () => { pg_serialized_load_error_cb(one_item); };
	    pg_set(one_item.lyr, 'src', one_item.src);
	    break;

	case 'write':
	    if ((typeof one_item.text) == 'undefined') break;

	    one_item.lyr.innerHTML = one_item.text;

	    if (one_item.lyr.__pg_onload) 
		{
		one_item.lyr.__pg_onload_cb = pg_serialized_load_cb;
		pg_addsched_fn(one_item.lyr, '__pg_onload_cb', [one_item], 0);
		}
	    else
		{
		pg_debug('pg_serialized_load_doone: ' + pg_loadqueue.length + ': ' + one_item.lyr.name + ' no cb\n');
		pg_loadqueue_remove(one_item);
		}
	    pg_loadqueue_check();
	    break;

	case 'func':
	    one_item.cb.apply(one_item.lyr, one_item.params);
	    pg_loadqueue_remove(one_item);
	    pg_loadqueue_check();
	    break;
	}
    }

// pg_serialized_load_cb() - called when a load finishes
function pg_serialized_load_cb(item)
    {
    pg_loadqueue_remove(item);
    //if (pg_loadqueue_busy < 0)
//	pg_loadqueue_busy = 0;

    if (item.lyr && item.lyr.__pg_onload) 
	item.lyr.__pg_onload();

    pg_loadqueue_check();
    }

// need some specialized error handling here in the future.
function pg_serialized_load_error_cb(item)
    {
    pg_loadqueue_remove(item);
    //if (pg_loadqueue_busy < 0)
//	pg_loadqueue_busy = 0;
    pg_loadqueue_check();
    }

function pg_loadqueue_check()
    {
    if (pg_loadqueue.length > 0)
	pg_addsched_fn(window, 'pg_serialized_load_doone', [], 0);
    pg_loadqueue_check_spinner();
    }

function pg_loadqueue_check_spinner()
    {
    // Do we need the busy spinner?
    var spinner = false;
    for(var i=0; i<pg_loadqueue.length; i++)
	{
	if (pg_loadqueue[i].silent === false)
	    {
	    spinner = true;
	    break;
	    }
	}

    if (spinner) 
	{
	// Create and/or make visible
	if (!pg_spinner)
	    {
	    pg_spinner = htr_new_layer(96);
	    htr_write_content(pg_spinner, "<center><img src=\"/sys/images/wait_spinner.gif\"</img></center>");
	    moveToAbsolute(pg_spinner, (pg_width-100)/2, (pg_height-24)/2);
	    htr_setzindex(pg_spinner, 99999);
	    }
	if (pg_spinner_id) pg_delsched(pg_spinner_id);
	pg_spinner_id = null;
	pg_spinner.vis = true;

	htr_setvisibility(pg_spinner, "inherit");
	}
    else
	{
	// Clear the busy spinner
	pg_clear_spinner();
	return;
	}
    }

function pg_clear_spinner()
    {
    if (pg_spinner)
	{
	if (pg_spinner_id) pg_delsched(pg_spinner_id);
	pg_spinner.vis = false;
	pg_spinner_id = pg_addsched_fn(window, function() { pg_spinner.vis || htr_setvisibility(pg_spinner, "hidden"); }, [], 150);
	}
    }

//END SECTION: async request handling ------------------------------------

//START SECTION: reveal/obscure ------------------------------------------

// Reveal/Obscure properties:
//
// __pg_reveal			An array of listeners attached to the given triggerer
// __pg_reveal_visible		Whether the triggerer is itself visible (regardless of containers)
// __pg_reveal_parent_visible	Whether the parent triggerer is displayed
// __pg_reveal_listener_visible	Whether the listener is displayed

// pg_reveal_register_listener() - when a layer/div requests to be
// notified when it is made visible to the user or is hidden from
// the user.
function pg_reveal_register_listener(l)
    {
    // Already set up?
    if (l.__pg_reveal_is_listener)
	return l.__pg_reveal_listener_visible;

    // If listener reveal fn not set up, set it to the standard.
    if (!l.__pg_reveal_listener_fn) l.__pg_reveal_listener_fn = l.Reveal;
    l.__pg_reveal_is_listener = true;

    // Search for the triggerer in question.
    var trigger_layer = l;
    do  {
	trigger_layer = wgtrGetParent(trigger_layer);
	} while (trigger_layer && !trigger_layer.__pg_reveal_is_triggerer && !wgtrIsRoot(trigger_layer));
    if (!trigger_layer) trigger_layer = wgtrGetRoot(l);

    // Add us to the triggerer
    if (trigger_layer && trigger_layer.__pg_reveal)
	{
	trigger_layer.__pg_reveal.push(l);
	l.__pg_reveal_listener_visible = trigger_layer.__pg_reveal_visible && trigger_layer.__pg_reveal_parent_visible;
	return l.__pg_reveal_listener_visible;
	}
    else
	{
	return false;
	}
    }


// pg_reveal_is_visible() - return true if an object is revealed, false if
// it is obscured.  The object need not be a listener or triggerer.
function pg_reveal_is_visible(l)
    {
    // listener
    if (typeof l.__pg_reveal_listener_visible != 'undefined')
	return l.__pg_reveal_listener_visible;

    // not listener - find a triggerer and go with that
    while (l && !l.__pg_reveal_is_triggerer && !wgtrIsRoot(l)) l = wgtrGetParent(l);

    // found a triggerer
    if (l && l.__pg_reveal_is_triggerer)
	return l.__pg_reveal_visible && l.__pg_reveal_parent_visible;

    // otherwise, fail
    return false;
    }


// pg_reveal_register_triggerer() - when a layer/div states that it can
// generate reveal/obscure events that need to be arbitrated by this
// system.
function pg_reveal_register_triggerer(l)
    {
    l.__pg_reveal_is_triggerer = true;
    l.__pg_reveal = [];
    l.__pg_reveal_visible = false;
    l.__pg_reveal_listener_fn = pg_reveal_internal;
    l.__pg_reveal_triggerer_fn = l.Reveal;
    l.__pg_reveal_busy = false;
    if (!wgtrIsRoot(l))
	l.__pg_reveal_parent_visible = pg_reveal_register_listener(l);
    else
	l.__pg_reveal_parent_visible = true;
    return;
    }

// pg_reveal_internal() - this is basically the listener-style callback for
// a triggerer that is used to link events generated by a parent triggerer
// down to listeners on this triggerer.
function pg_reveal_internal(e)
    {
    pg_debug('reveal_internal: ' + wgtrGetName(this) + ' ' + e.eventName + ' from ' + wgtrGetName(e.triggerer) + '\n');
    // Parent telling us something we already know?  (this is just a sanity check)
    if ((this.__pg_reveal_parent_visible) && e.eventName == 'RevealCheck') return pg_reveal_check_ok(e);
    if ((!this.__pg_reveal_parent_visible) && e.eventName == 'ObscureCheck') return pg_reveal_check_ok(e);
    if (this.__pg_reveal_parent_visible && e.eventName == 'Reveal') return true;
    if (!this.__pg_reveal_parent_visible && e.eventName == 'Obscure') return true;

    // Does this change actually affect our listeners?  (don't bother them if it doesn't)
    var was_visible = (this.__pg_reveal_parent_visible && this.__pg_reveal_visible);
    var going_to_be_visible = ((e.eventName == 'RevealCheck' || e.eventName == 'Reveal') && this.__pg_reveal_visible);
    var vis_changing = (was_visible != going_to_be_visible);

    // Is this both a triggerer and a listener?
    if (vis_changing && this.__pg_reveal_is_listener && this.__pg_reveal_is_triggerer && (e.eventName == 'Reveal' || e.eventName == 'Obscure'))
	{
	this.Reveal(e);
	}

    // For Reveal and Obscure, simply filter the events down.
    if (e.eventName == 'Reveal' || e.eventName == 'Obscure') 
	{
	this.__pg_reveal_parent_visible = (e.eventName == 'Reveal');
	if (vis_changing)
	    return pg_reveal_send_events(this, e.eventName);
	else
	    return true;
	}

    // For RevealCheck and ObscureCheck, start the notification process.
    if ((e.eventName == 'RevealCheck' || e.eventName == 'ObscureCheck'))
	{
	pg_debug('reveal_internal: checking visibility\n');
	if (!vis_changing) return pg_reveal_check_ok(e);
	pg_debug('reveal_internal: checking child cnt\n');
	if (this.__pg_reveal.length == 0) return pg_reveal_check_ok(e);
	this.__pg_reveal_busy = true;
	var our_e = new Object();
	our_e.eventName = e.eventName;
	our_e.parent_e = e;
	our_e.triggerer = this;
	our_e.listener_num = 0;
	our_e.origName = null;   // not needed
	our_e.triggerer_c = null;   // not needed
	pg_debug('reveal_internal: passing it on down to ' + wgtrGetName(this.__pg_reveal[0]) + '\n');
	pg_addsched_fn(this.__pg_reveal[0], "__pg_reveal_listener_fn", [our_e], 0);
	return true;
	}
    }

// pg_reveal_event() - called by a triggerer to indicate that the triggerer
// desires to initiate an event.
function pg_reveal_event(l,c,e_name)	
    {
    pg_debug('reveal_event: ' + wgtrGetName(l) + ' ' + e_name + '\n');
    if (l.__pg_reveal_busy) return false;

    // not a check event
    if (e_name == 'Reveal' || e_name == 'Obscure')
	{
	if (l.__pg_reveal_visible == (e_name == 'Reveal')) return true; // already set
	l.__pg_reveal_visible = (e_name == 'Reveal');
	if (l.__pg_reveal_parent_visible)
	    pg_reveal_send_events(l, e_name);
	return true;
	}

    // short circuit check process?
    if (e_name == 'RevealCheck' && (l.__pg_reveal.length == 0 || !l.__pg_reveal_parent_visible || l.__pg_reveal_visible))
	{
	l.__pg_reveal_visible = true;
	pg_addsched_fn(l, "Reveal", [{eventName:'RevealOK', c:c}], 0);
	return true;
	}
    if (e_name == 'ObscureCheck' && (l.__pg_reveal.length == 0 || !l.__pg_reveal_visible || !l.__pg_reveal_parent_visible))
	{
	l.__pg_reveal_visible = false;
	pg_addsched_fn(l, "Reveal", [{eventName:'ObscureOK', c:c}], 0);
	return true;
	}

    // send initial check event.
    l.__pg_reveal_busy = true;
    e = new Object();
    e.eventName = e_name;
    e.origName = e_name;
    e.triggerer = l;
    e.parent_e = null;   // not needed
    e.triggerer_c = c;
    e.listener_num = 0;
    pg_addsched_fn(l.__pg_reveal[0], "__pg_reveal_listener_fn", [e], 0);

    return true;
    }

// pg_reveal_check_ok() - called when the listener approves the check event.
function pg_reveal_check_ok(e)
    {
    pg_debug('reveal_check_ok: ' + wgtrGetName(e.triggerer.__pg_reveal[e.listener_num]) + ' said OK\n');
    e.listener_num++;
    if (e.triggerer.__pg_reveal.length > e.listener_num)
	{
	pg_debug('    -- passing it on down to ' + wgtrGetName(e.triggerer.__pg_reveal[e.listener_num]) + '\n');
	pg_addsched_fn(e.triggerer.__pg_reveal[e.listener_num], "__pg_reveal_listener_fn", [e], 0);
	}
    else
	{
	pg_debug('reveal_check_ok: OK to ' + e.eventName + ' ' + wgtrGetName(e.triggerer) + '\n');
	if (e.parent_e)
	    {
	    e.triggerer.__pg_reveal_busy = false;
	    return pg_reveal_check_ok(e.parent_e);
	    }
	else
	    {
	    // update visibility
	    //if (e.origName == 'Reveal') e.triggerer.__pg_reveal_visible = true;
	    //else e.triggerer.__pg_reveal_visible = false;

	    // notify listeners
	    //pg_reveal_send_events(e.triggerer.__pg_reveal, e.origName);

	    // notify triggerer
	    var triggerer_e = new Object();
	    if (e.origName == 'RevealCheck') triggerer_e.eventName = 'RevealOK';
	    else triggerer_e.eventName = 'ObscureOK';
	    triggerer_e.c = e.triggerer_c;
	    pg_addsched_fn(e.triggerer, "Reveal", [triggerer_e], 0);
	    e.triggerer.__pg_reveal_busy = false;
	    }
	}
    return true;
    }

// pg_reveal_check_veto() - when a listener veto's a triggerer's check
// event.
function pg_reveal_check_veto(e)
    {
    if (e.parent_e)
	{
	e.triggerer.__pg_reveal_busy = false;
	return pg_reveal_check_veto(e.parent_e);
	}
    else
	{
	// notify triggerer
	var triggerer_e = new Object();
	if (e.origName == 'Reveal') triggerer_e.eventName = 'RevealFailed';
	else triggerer_e.eventName = 'ObscureFailed';
	triggerer_e.c = e.triggerer_c;
	pg_addsched_fn(e.triggerer, "Reveal", [triggerer_e], 0);
	e.triggerer.__pg_reveal_busy = false;
	}
    return true;
    }

// pg_reveal_send_events() - send an event to all listeners
function pg_reveal_send_events(t, e)
    {
    var listener_e = new Object();
    listener_e.eventName = e;
    listener_e.triggerer = t;
    for (var i=0; i<t.__pg_reveal.length; i++)
	{
	if ((e == 'Reveal') == t.__pg_reveal[i].__pg_reveal_listener_visible) continue;
	t.__pg_reveal[i].__pg_reveal_listener_visible = (e == 'Reveal');
	pg_debug('    -- sending ' + e + ' to ' + wgtrGetName(t.__pg_reveal[i]) + '\n');
	pg_addsched_fn(t.__pg_reveal[i], "__pg_reveal_listener_fn", [listener_e], 0);
	}
    return true;
    }

//END SECTION: reveal/obscure ------------------------------------------

//START SECTION: debugging ---------------------------------------------

// set up for debug logging
function pg_debug_register_log(l)
    {
    pg_debug_log = l;
    }

// send debug msg
function pg_debug(msg)
    {
    if (pg_debug_log)
	{
	var ia = pg_debug_log.ifcProbe(ifAction);
	if (ia && ia.Invoke) ia.Invoke("AddText", {Text:msg, ContentType:'text/plain'});
	}
    }

// send error msg - for now just redirect to debug
function pg_error(msg)
    {
    pg_debug(msg);
    }

// log function calls and return values.
function pg_log_fn(fnname)
    {
    if (typeof(window[fnname]) != 'function') 
        {
	pg_debug(fnname + ' is not a function.\n');
	return;
	}
    var oldfn = window[fnname].toString();
    var openparen = oldfn.indexOf('(');
    var closeparen = oldfn.indexOf(')');
    var openbrace = oldfn.indexOf('{');
    var closebrace = oldfn.lastIndexOf('}');
    var argstext = oldfn.substring(openparen+1,closeparen);
    var args = argstext.split(',');
    var body = oldfn.substring(openbrace+1,closebrace);
    body = "var __debugmsg = '" + fnname + " called with ('; for(var __i=0;__i<arguments.length;__i++) { if (__i > 0) __debugmsg += ', '; switch(typeof(arguments[__i])) { case 'string': __debugmsg += \"'\" + arguments[__i] + \"'\"; break; case 'function': __debugmsg += arguments[__i].name + '()'; break; default: __debugmsg += arguments[__i]; break; } } __debugmsg += ')\\n'; pg_debug(__debugmsg); function __internal(" + argstext + ") {" + body + "} var __rval = __internal.apply(this,arguments); pg_debug('" + fnname + " returned: ' + __rval + '\\n'); return __rval;";
    //args.push(body);
    var fndecl = 'new Function(';
    for(var i=0;i<args.length;i++)
	{
	if (args[i] == '') continue;
	fndecl += '"' + args[i] + '", ';
	}
    fndecl += 'body)';
    window[fnname] = eval(fndecl);
    return;
    }


//END SECTION: debugging ---------------------------------------------

//START SECTION: Control Message management --------------------------

/// The below set of functions handle the Control Message mechanism from
/// the server.  Widgets / objects may request to receive control messages
/// of certain types, and they will be routed as requested when they
/// come in.

// This function registers an object to receive control messages via the
// object.ControlMsg callback.
function pg_msg_request(o, m)
    {
    pg_msg_handlers.push({obj:o, msgtypes:m});
    if (!pg_msg_timeout)
	pg_msg_timeout = setTimeout('pg_serialized_load(pg_msg_layer, "/INTERNAL/control?cx_cm_nowait=1", pg_msg_received);', 2000);
    return;
    }

// This function initializes the control message delivery mechanism
function pg_msg_init()
    {
    pg_msg_layer = document.getElementById('pgmsg');
    pg_msg_layer.onload = pg_msg_received;
    //pg_addsched('pg_set(pg_msg_layer, "src", "/INTERNAL/control")');
    //pg_set(pg_msg_layer, "src", null);
    if (pg_msg_handlers.length > 0 && !pg_msg_timeout)
	pg_msg_timeout = setTimeout('pg_serialized_load(pg_msg_layer, "/INTERNAL/control?cx_cm_nowait=1", pg_msg_received);', 2000);
    return;
    }

// Called when a message has been received.
function pg_msg_received()
    {
    var lnks = pg_links(this);
    if (lnks.length > 1)
	{
	for(var i = 0; i<pg_msg_handlers.length; i++)
	    {
	    pg_msg_handlers[i].obj.ControlMsg(lnks);
	    }
	}
    pg_msg_timeout = setTimeout('pg_serialized_load(pg_msg_layer, "/INTERNAL/control?cx_cm_nowait=1", pg_msg_received);', 2000);
    return;
    }

//END SECTION: Control Message management --------------------------


// tooltips
function pg_tooltip(msg, x, y)
    {
    if (!pg_tiplayer)
	pg_tiplayer = htr_new_layer(pg_width);
    htr_setvisibility(pg_tiplayer, "hidden");
    //pg_set_style(pg_tiplayer, "box-shadow", "2px 2px 4px black");
    pg_tipindex++;
    pg_tipinfo = {msg:msg, x:x, y:y};
    if (pg_tiptmout) pg_delsched(pg_tiptmout);
    pg_tiptmout = pg_addsched_fn(window, "pg_dotip", [], 500);
    return pg_tipindex;
    }

function pg_canceltip(id)
    {
    if (id == pg_tipindex)
	{
	if (pg_tiptmout) pg_delsched(pg_tiptmout);
	pg_tiptmout = null;
	if (pg_tiplayer) htr_setvisibility(pg_tiplayer, "hidden");
	pg_tipindex++;
	return true;
	}
    return false;
    }

function pg_dotip()
    {
    pg_tiptmout = null;
    var txt = '<table border="0" cellspacing="0" cellpadding="1" bgcolor="black"><tr><td><table border="0" cellspacing="0" cellpadding="1" bgcolor="#ffffc0"><tr><td><table border="0" cellspacing="0" cellpadding="0"><tr><td><img src="/sys/images/trans_1.gif"></td><td>' + htutil_encode(pg_tipinfo.msg) + '</td><td><img src="/sys/images/trans_1.gif"></td></tr></table></td></tr></table></td></tr></table>';
    //pg_serialized_write(pg_tiplayer, txt, pg_dotip_complete);
    htr_write_content(pg_tiplayer,txt);
    pg_dotip_complete();
    }

function pg_dotip_complete()
    {
    var imgs = pg_images(pg_tiplayer);
    var x1 = getRelativeX(imgs[0]);
    if (isNaN(x1)) x1 = imgs[0].offsetLeft + imgs[0].offsetParent.offsetLeft;
    var x2 = getRelativeX(imgs[1]);
    if (isNaN(x2)) x2 = imgs[1].offsetLeft + imgs[1].offsetParent.offsetLeft;
    var tipw = (x2 - x1) + 5;
    var pgx = pg_tipinfo.x;
    var pgy = pg_tipinfo.y + 20;
    if (pgx + tipw > pg_width) pgx = pg_width - tipw;
    if (pgx < 0) pgx = 0;
    //setClipWidth(pg_tiplayer, tipw);
    pg_set_style(pg_tiplayer, "width", tipw + "px");
    moveToAbsolute(pg_tiplayer, pgx, pgy);
    htr_setzindex(pg_tiplayer, 99999);
    htr_setvisibility(pg_tiplayer, "inherit");
    }


// Is the DIV or LAYER "l" restricted due to a modal dialog?
function pg_checkmodal(l)
    {
    var restricted = pg_modallayer && !pg_isinlayer(pg_modallayer, l) && !(wgtrIsNode(l) && wgtrIsNode(pg_modallayer) && wgtrIsChild(pg_modallayer, l));
    if (restricted && l.kind == 'dt_pn' && ((l.ml && wgtrIsNode(l.ml) && wgtrIsNode(pg_modallayer) && wgtrIsChild(pg_modallayer, l.ml)) || (l.parentElement && l.parentElement.ml && wgtrIsNode(l.parentElement.ml) && wgtrIsNode(pg_modallayer) && wgtrIsChild(pg_modallayer, l.parentElement.ml))))
	restricted = false;
    return restricted;
    }


// event handlers
function pg_mousemove(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (pg_tiptmout)
	{
	pg_tipinfo.x = e.pageX;
	pg_tipinfo.y = e.pageY;
	}
    if (pg_checkmodal(ly)) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
    /*if (pg_modallayer)
        {
        if (!pg_isinlayer(pg_modallayer, ly)) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }*/
    if (pg_curlayer != null)
        {
	var offs = $(pg_curlayer).offset();
        //pg_setmousefocus(pg_curlayer, e.pageX - getPageX(pg_curlayer), e.pageY - getPageY(pg_curlayer));
        pg_setmousefocus(pg_curlayer, e.pageX - offs.left, e.pageY - offs.top);
        }
    if (e.target != null && pg_curarea != null && ((e.mainlayer && e.mainlayer != pg_curarea.layer) /*|| (e.target == pg_curarea.layer)*/))
        {
        pg_removemousefocus();
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pg_mouseout(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (pg_checkmodal(ly)) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
    /*if (pg_modallayer)
        {
        if (!pg_isinlayer(pg_modallayer, ly)) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }*/
    if (ibeam_current && e.target == ibeam_current)
        {
        pg_curlayer = pg_curkbdlayer;
        pg_curarea = pg_curkbdarea;
        return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    if (e.target == pg_curlayer) pg_curlayer = null;
    if (e.target != null && pg_curarea != null && ((e.mainlayer && e.mainlayer == pg_curarea.layer) /*|| (e.target == pg_curarea.layer)*/))
        {
        pg_removemousefocus();
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pg_mouseover(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (pg_checkmodal(ly)) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
    /*if (pg_modallayer)
        {
        if (!pg_isinlayer(pg_modallayer, ly)) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }*/
    if (ibeam_current && e.target == ibeam_current)
        {
        pg_curlayer = pg_curkbdlayer;
        pg_curarea = pg_curkbdarea;
        return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    if (e.target != null && e.target != document && getPageX(e.target) != null)
        {
        pg_curlayer = e.target;
        if (pg_curlayer.mainlayer != null) pg_curlayer = pg_curlayer.mainlayer;
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pg_mousedown(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    pg_canceltip(pg_tipindex);
    if (pg_checkmodal(ly)) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
    /*if (pg_modallayer)
        {
        if (!pg_isinlayer(pg_modallayer, ly)) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }*/
    //if (pg_curlayer) alert('cur layer kind = ' + e.mainkind + ' ' + e.mainlayer.id);
    if (ibeam_current && e.target.layer == ibeam_current) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
    if (e.target != null && pg_curarea != null && ((e.mainlayer && e.mainlayer != pg_curarea.layer) /*|| (e.target == pg_curarea.layer)*/))
        {
        pg_removemousefocus();
        }
    if (pg_curarea != null)
        {
        if (pg_curlayer != pg_curkbdlayer)
            if (!pg_removekbdfocus()) return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
        pg_setkbdfocus(pg_curlayer, pg_curarea, e.pageX - getPageX(pg_curarea.layer), e.pageY-getPageY(pg_curarea.layer));
        }
    else if (!ly.keep_kbd_focus)
        {
        if (!pg_removekbdfocus()) return EVENT_HALT | EVENT_PREVENT_ALLOW_ACTION;
        pg_curkbdarea = null;
        pg_curkbdlayer = null;
        }
    if (e.target == window || e.target == document)
	{
	if (cx__capabilities.Dom0NS && (e.which == 3 || e.which == 2))
	    {
	    if (window.ifcProbe(ifEvent).Activate('RightClick', {X:e.pageX, Y:e.pageY}) != null)
		{
		return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
		}
	    }
	}
    //window.paste_input.focus();
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pg_contextmenu(e)
    {
    if (e.target == window || e.target == document)
	{
	if (window.ifcProbe(ifEvent).Activate('RightClick', {X:e.pageX, Y:e.pageY}) != null)
	    {
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	    }
	}
    //window.paste_input.focus();
    }

function pg_mouseup_ns4(e)
    {
    setTimeout('document.layers.pginpt.document.tmpform.x.focus()',10);
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pg_mouseup(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (pg_checkmodal(ly)) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
    /*if (pg_modallayer)
        {
        if (!pg_isinlayer(pg_modallayer, ly)) return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
        }*/
    //window.paste_input.focus();
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pg_keydown(e)
    {
    pg_canceltip(pg_tipindex);
    if (cx__capabilities.Dom0NS)
	{
        var k = e.which;
        if (k > 65280) k -= 65280;
        if (k >= 128) k -= 128;
        if (k == pg_lastkey && e.modifiers == pg_lastmodifiers) 
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        pg_lastkey = k;
	pg_lastmodifiers = e.modifiers;
        if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);
	if (pg_keyschedid) pg_delsched(pg_keyschedid);
        pg_keyschedid = pg_addsched_fn(window, function() { pg_keytimeoutid = setTimeout(pg_keytimeout, 200); }, [], 0);
        if (pg_keyhandler(k, e.modifiers, e))
	    return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
	else
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	}
    else if (cx__capabilities.Dom0IE)
	{
        var k = e.keyCode;
        if (k > 65280) k -= 65280;
        //if (k >= 128) k -= 128;
        if (k == pg_lastkey && e.modifiers == pg_lastmodifiers) 
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        pg_lastkey = k;
	pg_lastmodifiers = e.modifiers;
        if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);
	if (pg_keyschedid) pg_delsched(pg_keyschedid);
        pg_keyschedid = pg_addsched_fn(window, function() { pg_keytimeoutid = setTimeout(pg_keytimeout, 200); }, [], 0);
        if (pg_keyhandler(k, e.modifiers, e))
	    return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
	else
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	}
    else if (cx__capabilities.Dom2Events)
	{
	var k = e.Dom2Event.which;
        /*if (k == pg_lastkey && e.Dom2Event.modifiers == pg_lastmodifiers) 
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        pg_lastkey = k;
	pg_lastmodifiers = e.Dom2Event.modifiers;
        if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);
	if (pg_keyschedid) pg_delsched(pg_keyschedid);
        pg_keyschedid = pg_addsched_fn(window, function() { pg_keytimeoutid = setTimeout(pg_keytimeout, 200); }, [], 0);*/
        //if (pg_keyhandler(k, e.Dom2Event.modifiers, e.Dom2Event))
	//if (e.ctrlKey && k == 17)
	//    window.paste_input.focus();
        if (pg_keyhandler(k, e.modifiers, e))
	    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
	else
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pg_keyup(e)
    {
    if (cx__capabilities.Dom0NS)
	{
        var k = e.which;
        if (k > 65280) k -= 65280;
        if (k >= 128) k -= 128;
        if (k == pg_lastkey) pg_lastkey = -1;
        if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);
        pg_keytimeoutid = null;
	}
    else if (cx__capabilities.Dom0IE)
	{
        var k = e.keyCode;
        if (k > 65280) k -= 65280;
        //if (k >= 128) k -= 128;
        if (k == pg_lastkey) pg_lastkey = -1;
        if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);
        pg_keytimeoutid = null;
        if (pg_keyuphandler(k, e.modifiers, e))
	    return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
	else
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	}
    else if (cx__capabilities.Dom2Events)
	{
	var k = e.Dom2Event.which;
        if (k == pg_lastkey) pg_lastkey = -1;
        if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);
        pg_keytimeoutid = null;
        if (pg_keyuphandler(k, e.modifiers, e))
	    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
	else
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pg_keypress(e)
    {
    if (cx__capabilities.Dom0IE)
	{
        var k = e.keyCode;
        if (k > 65280) k -= 65280;
        //if (k >= 128) k -= 128;
        if (k == pg_lastkey) pg_lastkey = -1;
        if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);
        pg_keytimeoutid = null;
        if (pg_keypresshandler(k, e.modifiers, e))
	    return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
	else
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	}
    else if (cx__capabilities.Dom2Events)
	{
	var k = e.Dom2Event.which;
	//if ((k == 8 || k == 13) && k == pg_lastkey) 
	//    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        if (k == pg_lastkey) pg_lastkey = -1;
        if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);
	pg_keytimeoutid = null;
        //if (pg_keypresshandler(k, e.Dom2Event.modifiers, e.Dom2Event))
        if (pg_keypresshandler(k, e.modifiers, e))
	    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
	else
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }


// Automatic resize utilities
function pg_check_resize(l)
    {
    if (wgtrGetServerProperty(l, "r_height") == -1)
	{
	if (wgtrGetServerProperty(l, "height") != $(l).height())
	    {
	    if (wgtrGetParent(l).childresize)
		{
		var geom = wgtrGetParent(l).childresize(l, wgtrGetServerProperty(l, "width"), wgtrGetServerProperty(l, "height"), $(l).width(), $(l).height());
		if (geom)
		    {
		    wgtrSetServerProperty(l, "height", geom.height);
		    //$(l).height(geom.height);
		    }
		return geom;
		}
	    }
	}
    return null;
    }

function pg_scroll(e)
    {
    if (e.target == document)
	{
	return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	}
    else
	{
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
	}
    }


// Load indication
if (window.pg_scripts) pg_scripts['htdrv_page.js'] = true;
