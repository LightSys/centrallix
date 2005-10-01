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


var pg_layer = null;

var pg_msg = new Object();
pg_msg.MSG_ERROR=1;
pg_msg.MSG_QUERY=2;
pg_msg.MSG_GOODBYE=4;
pg_msg.MSG_REPMSG=8;
pg_msg.MSG_EVENT=16;

/** returns an attribute of the element in pixels **/
function pg_get_style(element,attr)
    {
    if(!element)
	{
	alert("NULL ELEMENT, attr " + attr + "is unknow.");
	return null;
	}
    if(cx__capabilities.Dom1HTML && cx__capabilities.Dom2CSS)
	{
	if(attr == 'zIndex') attr = 'z-index';
	if(attr.substring(0,5) == 'clip.')
	    {
	    return eval('element.' + attr);
	    }	
	var comp_style = window.getComputedStyle(element,null);
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
	element.style.setProperty(attr,value + "px","");
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

function pg_ping_init(l,i)
    {
    if(cx__capabilities.Dom0IE)
        {
        //alert(l.currentStyle.position);
    	l.tid=setInterval(pg_ping_send, i);
	}
    else
    	{
    	l.tid=setInterval(pg_ping_send,i,l);    		
    	}
    }

function pg_ping_recieve()
    {
    var link;
    //confirm("recieving");
    if(cx__capabilities.Dom1HTML)
	{
	link = this.contentDocument.getElementsByTagName("a")[0];
	}
    else if(cx__capabilities.Dom0NS)
	{
	link = this.document.links[0];
	}
    else
	{
	return false;
	}
    if(link.target!=='OK')
	{
	clearInterval(this.tid);
	confirm('you have been disconnected from the server');
	}
    }
    
function pg_ping_send(p)
    {
    //confirm('sending');
    if (cx__capabilities.Dom0IE)
        {
        p = document.getElementById('pgping');
	}
	        
    p.onload=pg_ping_recieve;
    
    if(cx__capabilities.Dom1HTML)
	{
	p.setAttribute('src','/INTERNAL/ping');
	}
    else if(cx__capabilities.Dom0NS)
	{
	//alert(p);
	p.src='/INTERNAL/ping';
	}
    }

/** Function to get the links attacked to a layer **/
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
	return o.getElementsByTagName("DIV");
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

function pg_get_computed_clip(o)
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
function pg_setmodal(l)
    {
    pg_modallayer = l;
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
		for(i=0;i<outer.childNodes.length;i++)
			{
			if (outer.childNodes[i] == inner) return true;
			if (pg_isinlayer(outer.childNodes[i], inner)) return true;
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

/** Function to make four layers into a box 
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
    if (cx__capabilities.Dom0NS)
        {
    	tl.bgColor = c1;
    	ll.bgColor = c1;
    	bl.bgColor = c2;
    	rl.bgColor = c2;
    	}
    else if (cx__capabilities.Dom1HTML)
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
    if (cx__capabilities.Dom1HTML && pl)
    	pl.parentLayer.appendChild(tl);
    moveAbove(tl,pl);
    moveToAbsolute(tl,x,y);
    htr_setzindex(tl,z);

    resizeTo(bl,w+s-1,1);
    setClipWidth(bl,w+s-1);
    setClipHeight(bl,1);
    if (cx__capabilities.Dom1HTML && pl)
    	pl.parentLayer.appendChild(bl);
    moveAbove(bl,pl);
    moveToAbsolute(bl,x,y+h-s+1);
    htr_setzindex(bl,z);

    resizeTo(ll,1,h);
    setClipHeight(ll,h);
    setClipWidth(ll,1);
    if (cx__capabilities.Dom1HTML && pl)
    	pl.parentLayer.appendChild(ll);
    moveAbove(ll,pl);
    moveToAbsolute(ll,x,y);
    htr_setzindex(ll,z);

    resizeTo(rl,1,h+1);
    setClipHeight(rl,h+1);
    setClipWidth(rl,1);
    if (cx__capabilities.Dom1HTML && pl)
    	pl.parentLayer.appendChild(rl);
    moveAbove(rl,pl);
    moveToAbsolute(rl,x+w-s+1,y);
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
function pg_area(pl,x,y,w,h,cls,nm,f)
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
function pg_resize_area(a,w,h)
    {
    var x=getPageX(a.layer)+a.x;
    var y=getPageY(a.layer)+a.y;
    var tl=document.layers.pgtop;
    var bl=document.layers.pgbtm;
    var ll=document.layers.pglft;
    var rl=document.layers.pgrgt;
    a.width = w;
    a.height = h;
    if (tl.visibility == 'inherit')
	{
	resizeTo(tl, w,1);
	resizeTo(bl, w+1,1);
	resizeTo(rl, 1,h+1);
	resizeTo(ll, 1,h);
	moveToAbsolute(tl, x,y);
	moveToAbsolute(bl, x,y+h);
	moveToAbsolute(ll, x,y);
	moveToAbsolute(rl, x+w,y);
	}
    tl=document.layers.pgktop;
    bl=document.layers.pgkbtm;
    ll=document.layers.pgklft;
    rl=document.layers.pgkrgt;
    if (tl.visibility == 'inherit')
	{
	resizeTo(tl, w,1);
	resizeTo(bl, w+1,1);
	resizeTo(rl, 1,h+1);
	resizeTo(ll, 1,h);
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
    pg_arealist.splice(0,0,a);
    return a;
    }

/** Function to remove an existing area... **/
function pg_removearea(a)
    {
    for(i=0;i<pg_arealist.length;i++)
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
    for(i=0;i<layers.length;i++)
	{
	var cl = layers[i];
	var visibility = pg_get_style(cl,'visibility');
	if (visibility == 'show' || visibility == 'visible' || visibility == 'inherit') 
	    {
	    var clh = getRelativeY(cl) + getClipHeight(cl) + getClipTop(cl);
	    var clw = getRelativeX(cl) + getClipWidth(cl) + getClipLeft(cl);
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
/// as an childwindow or the main document itself.
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
    if (win == window || win == document)
	{
	var doclayers = pg_layers(document);
	var found_win = null;
	var min_z = 1000;
	moveAbove(p,doclayers[0]);
	for(var i = 0; i < doclayers.length; i++)
	    {
	    if (doclayers[i].kind == 'wn' && doclayers[i].zIndex < min_z)
		{
		found_win = doclayers[i];
		min_z = doclayers[i].zIndex;
		}
	    }
	p.zIndex = min_z - 1;
	}
    else
	{
	moveAbove(p,win);
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
    if (y + h + getClipHeight(p) <= getInnerHeight())
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
function pg_addkey(s,e,mod,modmask,mlayer,klayer,tgt,action,aparam)
    {
    kd = new Object();
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
    
function pg_cmpkey(k1,k2)
    {
    return (k1.endcode-k1.startcode) - (k2.endcode-k2.startcode);
    }
    
function pg_removekey(kd)
    {
    for(i=0;i<pg_keylist.length;i++)
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
    }

function pg_keypresshandler(k,m,e)
    {
    return pg_keyhandler_internal(k,m,e);
    }

function pg_keyhandler(k,m,e)
    {
    //alert(this.caller);
    pg_lastmodifiers = m;

    // block non-special codes for IE here - handle em in keypress, not keydown.
    if (cx__capabilities.Dom0IE || cx__capabilities.Dom2Events)
	{
	if (k >= 32 && k != 46)
	    return true;

	// IE passes DEL in as code 46 for keydown.  ASC(46), the period,
	// is passed as code 190 here but code 46 in the keypress event.  
	// Go figure.
	if (k == 46) 
	    k = 127;
	}

    return pg_keyhandler_internal(k,m,e);
    }

function pg_keyhandler_internal(k,m,e)
    {
    //htr_alert_obj(e,1);
    // layer.keyhandler is a callback routine that is optional 
    // on any layer requesting focus with pg_addarea().
    // It is set up in the corresponding widget drivers.
    if (pg_curkbdlayer != null && 
    pg_curkbdlayer.keyhandler != null && 
    pg_curkbdlayer.keyhandler(pg_curkbdlayer,e,k) == true) 
    	return false;       
      
    for(i=0;i<pg_keylist.length;i++)
	{
	if (k >= pg_keylist[i].startcode && k <= pg_keylist[i].endcode && (pg_keylist[i].kbdlayer == null || pg_keylist[i].kbdlayer == pg_curkbdlayer) && (pg_keylist[i].mouselayer == null || pg_keylist[i].mouselayer == pg_curlayer) && (m & pg_keylist[i].modmask) == pg_keylist[i].mod)
	    {
	    pg_keylist[i].aparam.KeyCode = k;
	    pg_keylist[i].target_obj[pg_keylist[i].fnname](pg_keylist[i].aparam);
	    return false;
	    }
	}
    return false;
    }
 
function pg_status_init()
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

// tc_init(document.cxSubElement("tc0base"), 0, "background='sys/images/slate2.gif'", "");
function pg_init(l,a,gs,ct)
    {
    l.ActionLoadPage = pg_load_page;
    l.ActionLaunch = pg_launch;
    window.windowlist = new Object();
    pg_attract = a;
    if (cx__capabilities.Dom0NS) pg_set_emulation(document);
    htr_init_layer(window,window,"window");
    pg_reveal_register_triggerer(window);
    pg_reveal_event(window,null,'Reveal');
    pg_addsched('pg_msg_init()', window);
    return l;
    }

function pg_load_page(aparam)
    {
    window.location.href = aparam.Source;
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
    if (aparam.Multi != null && aparam.Multi == true)
	{
	for(var i = 0; i < 32; i++) // 32 max multi-instanced windows
	    {
	    if (window.windowlist[w_name + '_' + i] == null || window.windowlist[w_name + '_' + i].close == null)
		w_name = w_name + '_' + i;
	    }
	}
    if (window.windowlist[w_name] != null && window.windowlist[w_name].close != null) w_exists = true;
    if ((aparam.Multi == null || aparam.Multi == false) && w_exists) 
	{
	window.windowlist[w_name].close();
	w_exists = false;
	}
    if (!w_exists) window.windowlist[w_name] = window.open(aparam.Source, w_name, "toolbar=no,scrollbars=no,innerHeight=" + aparam.Height + ",innerWidth=" + aparam.Width + ",resizable=no,personalbar=no,menubar=no,status=no");
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


function pg_addsched(e,o)
    {
    var sched = {exp:e, obj:o};
    pg_schedtimeoutlist.push(sched);
    if(!pg_schedtimeout) 
	{
	if (window.pg_isloaded)
	    pg_schedtimeout = setTimeout(pg_dosched, 0);
	else
	    pg_schedtimeout = setTimeout(pg_dosched, 100);
	}
    }

function pg_addsched_fn(o,f,p)
    {
    var sched = {func:f, obj:o, param:p};
    pg_schedtimeoutlist.push(sched);
    if(!pg_schedtimeout) 
	{
	if (window.pg_isloaded)
	    pg_schedtimeout = setTimeout(pg_dosched, 0);
	else
	    pg_schedtimeout = setTimeout(pg_dosched, 100);
	}
    }

function pg_expchange(p,o,n)
    {
    if (o==n) return n;
    for(var i=0;i<pg_explist.length;i++)
	{
	var exp = pg_explist[i];
	for(var j=0;j<exp.ParamList.length;j++)
	    {
	    var item = exp.ParamList[j];
	    if (this == item[2] && p == item[1])
		{
		//alert("eval " + exp.Objname + "." + exp.Propname + " = " + exp.Expression);
		pg_addsched(exp.Objname + "." + exp.Propname + " = " + exp.Expression, window);
		}
	    }
	}
    return n;
    }


function pg_dosched()
    {
    pg_schedtimeout = null;
    var sched_item = null;
    window.pg_isloaded = true;
    if (pg_schedtimeoutlist.length > 0)
    	{
	sched_item = pg_schedtimeoutlist.splice(0,1)[0];
	if (sched_item.exp)
	    {
	    // evaluate expression
	    //alert('evaluating ' + sched_item.exp);
	    with (sched_item.obj) { eval(sched_item.exp); }
	    }
	else
	    {
	    // call function
	    sched_item.obj[sched_item.func].apply(sched_item.obj, sched_item.param);
	    }
	}

    if (pg_schedtimeoutlist.length > 0)
	{
	pg_schedtimeout = setTimeout(pg_dosched,0);
	}
    else
	{
	pg_schedtimeout = null;
	}
    }

function pg_expression(o,p,e,l)
    {
    var expobj = new Object();
    expobj.Objname = o;
    expobj.Propname = p;
    expobj.Expression = e;
    expobj.ParamList = l;
    eval(expobj.Objname + "." + expobj.Propname + " = " + expobj.Expression);
    pg_explist.push(expobj);
    for(var i=0; i<l.length; i++)
	{
	var item = l[i];
	item[2] = eval(item[0]); // get obj reference
	item[2].pg_expchange = pg_expchange;
	htr_watch(item[2],item[1],"pg_expchange");
	//item[2].watch(item[1], pg_expchange);
	}
    }


function pg_removemousefocus()
    {
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
	    // wants mouse focus
	    var x = getPageX(pg_curarea.layer)+pg_curarea.x;
	    var y = getPageY(pg_curarea.layer)+pg_curarea.y;
	    
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

function pg_removekbdfocus()
    {
    if (pg_curkbdlayer)
	{
	if (pg_curkbdlayer.losefocushandler && !pg_curkbdlayer.losefocushandler()) return false;
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
	// mk new data focus box for this control.
	if (cx__capabilities.Dom1HTML)
	    {
	    l.pg_dttop = document.createElement("div");
	    l.pg_dttop.style.width = 1152;
	    l.pg_dtbtm = document.createElement("div");
	    l.pg_dtbtm.style.width = 1152;
	    l.pg_dtrgt = document.createElement("div");
	    l.pg_dtrgt.style.width = 2;
	    l.pg_dtlft = document.createElement("div");
	    l.pg_dtlft.style.width = 2;
	    }
	else if (cx__capabilities.Dom0NS)
	    {
	    l.pg_dttop = new Layer(1152);
	    l.pg_dtbtm = new Layer(1152);
	    l.pg_dtrgt = new Layer(2);
	    l.pg_dtlft = new Layer(2);
	    }		    
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
    if (!a)
	{
	a = pg_findfocusarea(l, xo, yo);
	}

    var x = getPageX(a.layer)+a.x;
    var y = getPageY(a.layer)+a.y;
    var w = a.width;
    var h = a.height;
    var prevLayer = pg_curkbdlayer;
    var prevArea = pg_curkbdarea;
    var v = 0;
    pg_curkbdarea = a;
    pg_curkbdlayer = l;

    if (pg_curkbdlayer && pg_curkbdlayer.getfocushandler)
	{
	v=pg_curkbdlayer.getfocushandler(xo,yo,a.layer,a.cls,a.name,a);
	if (v & 1)
	    {
	    // mk box for kbd focus
	    if (prevArea != a)
		{
		if (cx__capabilities.Dom0NS)
		    {
		    pg_mkbox(l ,x,y,w,h, 1, document.layers.pgktop,document.layers.pgkbtm,document.layers.pgkrgt,document.layers.pgklft, page.kbcolor1, page.kbcolor2, document.layers.pgtop.zIndex+100);
		    }
		else if (cx__capabilities.Dom1HTML)
		    {		    
		    pg_mkbox(l ,x,y,w,h, 1, document.getElementById("pgktop"),document.getElementById("pgkbtm"),document.getElementById("pgkrgt"),document.getElementById("pgklft"), page.kbcolor1, page.kbcolor2, htr_getzindex(document.getElementById("pgtop"))+100);
		    }
		}
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

    return v?true:false;
    }


// pg_show_containers() - makes sure containers, from innermost to
// outermost, are displayed to the user.  Used when a control receives
// keyboard focus to make sure control is visible to user.
function pg_show_containers(l)
    {
    if (l == window || l == document) return true;
    if (l.showcontainer && l.showcontainer() == false)
	return false;
    if (cx__capabilities.Dom0NS)
	return pg_show_containers(l.parentLayer);
    else if (cx__capabilities.Dom1HTML)
	return pg_show_containers(l.parentNode);
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


// pg_serialized_load() - schedules the reload of a layer, hidden or visible,
// from the server in a manner that keeps things serialized so server loads
// don't overlap.
function pg_serialized_load(l, newsrc, cb)
    {
    pg_debug('pg_serialized_load: ' + pg_loadqueue.length + ': ' + l.name + ' loads ' + newsrc + '\n');
    pg_loadqueue.push({lyr:l, src:newsrc, cb:cb});
    pg_debug('pg_serialized_load: ' + pg_loadqueue.length + '\n');
    if (!pg_loadqueue_busy) //pg_addsched_fn(window, 'pg_serialized_load_doone', new Array());
	pg_serialized_load_doone();
    }

// pg_serialized_write() - schedules the writing of content to a layer, so that
// we don't have the document open while stuff is happening from the server.
function pg_serialized_write(l, text, cb)
    {
    pg_loadqueue.push({lyr:l, text:text, cb:cb});
    if (!pg_loadqueue_busy) //pg_addsched_fn(window, 'pg_serialized_load_doone', new Array());
	pg_serialized_load_doone();
    }

// pg_serialized_load_doone() - loads the next item off of the
// serialized loader list.
function pg_serialized_load_doone()
    {
    if (pg_loadqueue.length == 0) 
	{
	pg_loadqueue_busy = false;
	return;
	}
    var one_item = pg_loadqueue.shift(); // remove first item
    if  (!one_item.text) pg_debug('pg_serialized_load_doone: ' + pg_loadqueue.length + ': ' + one_item.src + ' into ' + one_item.lyr.name + '\n');
    pg_loadqueue_busy = true;
    one_item.lyr.__pg_onload = one_item.cb;
    if (one_item.src)
	{
	one_item.lyr.onload = pg_serialized_load_cb;
	pg_set(one_item.lyr, 'src', one_item.src);
	}
    else if (one_item.text)
	{
	if (cx__capabilities.Dom0NS)
	    {
	    one_item.lyr.document.write(one_item.text);
	    one_item.lyr.document.close();
	    }
	else
	    {
	    one_item.lyr.innerHTML = one_item.text;
	    }
	if (one_item.lyr.__pg_onload) one_item.lyr.__pg_onload();
	pg_loadqueue_busy = false;
	if (pg_loadqueue.length > 0) pg_addsched_fn(window, 'pg_serialized_load_doone', new Array());
	}
    }

// pg_serialized_load_cb() - called when a load finishes
function pg_serialized_load_cb()
    {
    if (this.__pg_onload) this.__pg_onload();
    pg_loadqueue_busy = false;
    pg_debug('pg_serialized_load_cb: ' + pg_loadqueue.length + ': ' + this.name + '\n');
    if (pg_loadqueue.length > 0) pg_addsched_fn(window, 'pg_serialized_load_doone', new Array());
    }


// pg_reveal_register_listener() - when a layer/div requests to be
// notified when it is made visible to the user or is hidden from
// the user.
function pg_reveal_register_listener(l)
    {
    // If listener reveal fn not set up, set it to the standard.
    if (!l.__pg_reveal_listener_fn) l.__pg_reveal_listener_fn = l.Reveal;
    l.__pg_reveal_is_listener = true;

    // Search for the triggerer in question.
    var trigger_layer = l;
    do  {
	if (cx__capabilities.Dom0NS)
	    trigger_layer = trigger_layer.parentLayer;
	else 
	    trigger_layer = trigger_layer.parentNode;
	} while (trigger_layer && !trigger_layer.__pg_reveal_is_triggerer && trigger_layer != window && trigger_layer != document);
    if (!trigger_layer) trigger_layer = window;

    // Add us to the triggerer
    if (trigger_layer && trigger_layer.__pg_reveal) trigger_layer.__pg_reveal.push(l);
    l.__pg_reveal_listener_visible = trigger_layer.__pg_reveal_visible && trigger_layer.__pg_reveal_parent_visible;
    return l.__pg_reveal_listener_visible;
    }

// pg_reveal_register_triggerer() - when a layer/div states that it can
// generate reveal/obscure events that need to be arbitrated by this
// system.
function pg_reveal_register_triggerer(l)
    {
    l.__pg_reveal_is_triggerer = true;
    l.__pg_reveal = new Array();
    l.__pg_reveal_visible = false;
    l.__pg_reveal_listener_fn = pg_reveal_internal;
    l.__pg_reveal_triggerer_fn = l.Reveal;
    l.__pg_reveal_busy = false;
    if (l != window && l != document)
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
    pg_debug('reveal_internal: ' + this.name + ' ' + e.eventName + ' from ' + e.triggerer.name + '\n');
    // Parent telling us something we already know?  (this is just a sanity check)
    if ((this.__pg_reveal_parent_visible) && e.eventName == 'RevealCheck') return pg_reveal_check_ok(e);
    if ((!this.__pg_reveal_parent_visible) && e.eventName == 'ObscureCheck') return pg_reveal_check_ok(e);
    if (this.__pg_reveal_parent_visible && e.eventName == 'Reveal') return true;
    if (!this.__pg_reveal_parent_visible && e.eventName == 'Obscure') return true;

    // Does this change actually affect our listeners?  (don't bother them if it doesn't)
    var was_visible = (this.__pg_reveal_parent_visible && this.__pg_reveal_visible);
    var going_to_be_visible = ((e.eventName == 'RevealCheck' || e.eventName == 'Reveal') && this.__pg_reveal_visible);
    var vis_changing = (was_visible != going_to_be_visible);

    // For Reveal and Obscure, simply filter the events down.
    if (vis_changing && (e.eventName == 'Reveal' || e.eventName == 'Obscure')) 
	{
	this.__pg_reveal_parent_visible = (e.eventName == 'Reveal');
	return pg_reveal_send_events(this, e.eventName);
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
	pg_debug('reveal_internal: passing it on down to ' + this.__pg_reveal[0].name + '\n');
	pg_addsched_fn(this.__pg_reveal[0], "__pg_reveal_listener_fn", new Array(our_e));
	}
    return true;
    }

// pg_reveal_event() - called by a triggerer to indicate that the triggerer
// desires to initiate an event.
function pg_reveal_event(l,c,e_name)	
    {
    pg_debug('reveal_event: ' + l.name + ' ' + e_name + '\n');
    if (l.__pg_reveal_busy) return false;

    // not a check event
    if (e_name == 'Reveal' || e_name == 'Obscure')
	{
	if (l.__pg_reveal_visible == (e_name == 'Reveal')) return true; // already set
	l.__pg_reveal_visible = (e_name == 'Reveal');
	pg_reveal_send_events(l, e_name);
	return true;
	}

    // short circuit check process?
    if (e_name == 'RevealCheck' && (l.__pg_reveal.length == 0 || !l.__pg_reveal_parent_visible || l.__pg_reveal_visible))
	{
	var e = {eventName:'RevealOK', c:c};
	l.__pg_reveal_visible = true;
	pg_addsched_fn(l, "Reveal", new Array(e));
	return true;
	}
    if (e_name == 'ObscureCheck' && (l.__pg_reveal.length == 0 || !l.__pg_reveal_visible || !l.__pg_reveal_parent_visible))
	{
	var e = {eventName:'ObscureOK', c:c};
	l.__pg_reveal_visible = false;
	pg_addsched_fn(l, "Reveal", new Array(e));
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
    pg_addsched_fn(l.__pg_reveal[0], "__pg_reveal_listener_fn", new Array(e));

    return true;
    }

// pg_reveal_check_ok() - called when the listener approves the check event.
function pg_reveal_check_ok(e)
    {
    pg_debug('reveal_check_ok: ' + e.triggerer.__pg_reveal[e.listener_num].name + ' said OK\n');
    e.listener_num++;
    if (e.triggerer.__pg_reveal.length > e.listener_num)
	{
	pg_debug('    -- passing it on down to ' + e.triggerer.__pg_reveal[e.listener_num].name + '\n');
	pg_addsched_fn(e.triggerer.__pg_reveal[e.listener_num], "__pg_reveal_listener_fn", new Array(e));
	}
    else
	{
	pg_debug('reveal_check_ok: OK to ' + e.eventName + ' ' + e.triggerer.name + '\n');
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
	    pg_addsched_fn(e.triggerer, "Reveal", new Array(triggerer_e));
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
	pg_addsched_fn(e.triggerer, "Reveal", new Array(triggerer_e));
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
	pg_debug('    -- sending ' + e + ' to ' + t.__pg_reveal[i].name + '\n');
	pg_addsched_fn(t.__pg_reveal[i], "__pg_reveal_listener_fn", new Array(listener_e));
	}
    return true;
    }


// set up for debug logging
function pg_debug_register_log(l)
    {
    pg_debug_log = l;
    }

// send debug msg
function pg_debug(msg)
    {
    if (pg_debug_log) pg_debug_log.ActionAddText({Text:msg, ContentType:'text/plain'});
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


// event handlers
function pg_mousemove(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (pg_modallayer)
        {
        if (!pg_isinlayer(pg_modallayer, ly)) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    if (pg_curlayer != null)
        {
        pg_setmousefocus(pg_curlayer, e.pageX - getPageX(pg_curlayer), e.pageY - getPageY(pg_curlayer));
        }
    if (e.target != null && pg_curarea != null && ((ly.mainlayer && ly.mainlayer != pg_curarea.layer) /*|| (e.target == pg_curarea.layer)*/))
        {
        pg_removemousefocus();
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pg_mouseout(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (pg_modallayer)
        {
        if (!pg_isinlayer(pg_modallayer, ly)) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    if (ibeam_current && e.target == ibeam_current)
        {
        pg_curlayer = pg_curkbdlayer;
        pg_curarea = pg_curkbdarea;
        return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    if (e.target == pg_curlayer) pg_curlayer = null;
    if (e.target != null && pg_curarea != null && ((ly.mainlayer && ly.mainlayer == pg_curarea.layer) /*|| (e.target == pg_curarea.layer)*/))
        {
        pg_removemousefocus();
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pg_mouseover(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (pg_modallayer)
        {
        if (!pg_isinlayer(pg_modallayer, ly)) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    if (ibeam_current && e.target == ibeam_current)
        {
        pg_curlayer = pg_curkbdlayer;
        pg_curarea = pg_curkbdarea;
        return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    if (e.target != null && getPageX(e.target) != null)
        {
        pg_curlayer = e.target;
        if (pg_curlayer.mainlayer != null) pg_curlayer = pg_curlayer.mainlayer;
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pg_mousedown(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (pg_modallayer)
        {
        if (!pg_isinlayer(pg_modallayer, ly)) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    //if (pg_curlayer) alert('cur layer kind = ' + ly.kind + ' ' + ly.id);
    if (ibeam_current && e.target.layer == ibeam_current) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
    if (e.target != null && pg_curarea != null && ((ly.mainlayer && ly.mainlayer != pg_curarea.layer) /*|| (e.target == pg_curarea.layer)*/))
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
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pg_mouseup(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (pg_modallayer)
        {
        if (!pg_isinlayer(pg_modallayer, ly)) return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pg_keydown(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (cx__capabilities.Dom0NS)
	{
        k = e.which;
        if (k > 65280) k -= 65280;
        if (k >= 128) k -= 128;
        if (k == pg_lastkey) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        pg_lastkey = k;
        if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);
        pg_addsched("pg_keytimeoutid = setTimeout(pg_keytimeout, 200)",window);
        if (pg_keyhandler(k, e.modifiers, e))
	    return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
	else
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	}
    else if (cx__capabilities.Dom0IE)
	{
        k = e.keyCode;
        if (k > 65280) k -= 65280;
        //if (k >= 128) k -= 128;
        if (k == pg_lastkey) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        pg_lastkey = k;
        if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);
        pg_addsched("pg_keytimeoutid = setTimeout(pg_keytimeout, 200)",window);
        if (pg_keyhandler(k, e.modifiers, e))
	    return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
	else
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	}
    else if (cx__capabilities.Dom2Events)
	{
	k = e.Dom2Event.which;
        if (k == pg_lastkey) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        pg_lastkey = k;
        if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);
        pg_addsched("pg_keytimeoutid = setTimeout(pg_keytimeout, 200)",window);
        if (pg_keyhandler(k, e.Dom2Event.modifiers, e.Dom2Event))
	    return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
	else
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pg_keyup(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (cx__capabilities.Dom0NS)
	{
        k = e.which;
        if (k > 65280) k -= 65280;
        if (k >= 128) k -= 128;
        if (k == pg_lastkey) pg_lastkey = -1;
        if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);
        pg_keytimeoutid = null;
	}
    else if (cx__capabilities.Dom0IE)
	{
        k = e.keyCode;
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
	k = e.Dom2Event.which;
        if (k == pg_lastkey) pg_lastkey = -1;
        if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);
        pg_keytimeoutid = null;
        if (pg_keyuphandler(k, e.modifiers, e))
	    return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
	else
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function pg_keypress(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (cx__capabilities.Dom0IE)
	{
        k = e.keyCode;
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
	k = e.Dom2Event.which;
	if (k == 8) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        if (k == pg_lastkey) pg_lastkey = -1;
        if (pg_keytimeoutid) clearTimeout(pg_keytimeoutid);
	pg_keytimeoutid = null;
        if (pg_keypresshandler(k, e.Dom2Event.modifiers, e.Dom2Event))
	    return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
	else
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }
