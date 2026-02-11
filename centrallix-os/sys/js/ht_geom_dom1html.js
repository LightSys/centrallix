// Copyright (C) 1998-2006 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

// Cross browser Geometry DOM1HTML

// Add some useful functions to Math that will be needed elsewhere.
// Math.clamp = (min, val, max) => Math.min(Math.max(min, val), max);
// Math.isBetween = (lowerBound, num, upperBound) => lowerBound < num && num < upperBound;

// Dev debug versions.
Math.clamp = (min, val, max) =>
    {
    if (min > max || isNaN(min) || isNaN(val) || isNaN(max))
	{
	console.warn(`Math.clamp(${min}, ${val}, ${max});`);
	console.trace();
	}
    return Math.min(Math.max(min, val), max);
    }
Math.isBetween = (lowerBound, num, upperBound) =>
    {
    if (lowerBound > upperBound || isNaN(lowerBound) || isNaN(num) || isNaN(upperBound))
        console.warn(`Math.isBetween(${lowerBound}, ${num}, ${upperBound});`);
    return lowerBound < num && num < upperBound;
    }



/*** Experimental system for turning off clipping CSS.
 *** The clip values are still stored and can be queried
 *** for legacy compatibility, but they will not output
 *** any clip rectangles or clip paths in the CSS or HTML.
 ***/
/** Ensure clipping is disabled for a layer / HTML node. **/
function disableClippingCSS(l)
    {
    console.log(`Turning off clipping for ${l.clip.obj.id}.`);
    l.clip.noclip = true;
    updateClippingCSS(l);
    }
/** Ensure clipping is enabled for a layer / HTML node. **/
function enableClippingCSS(l)
    {
    l.clip.noclip = false;
    updateClippingCSS(l);
    }
/** Update clipping without changing any specific values. **/
function updateClippingCSS(l)
    {
    setClipTop(l, getClipTop(l));
    }

/** Debug function for finding clipped dom nodes. **/
function getClipped()
    {
    return Array
	.from(Window.clipped)
	.filter(id=>id)
	.map(id=>document.getElementById(id));
    }

// Clip Width
function getClipWidth(l) 
    { 
    return l.clip.width; 
    }

function setClipWidth(l, value) 
    { 
    l.clip.width = value; 
    }

// Clip Height
function getClipHeight(l) 
    { 
    return l.clip.height; 
    }

function getRuntimeClipHeight(l)
    {
    return getClipHeight(l);
    }

function setClipHeight(l, value) 
    { 
    l.clip.height = value; 
    }

// Clip Top
function getClipTop(l) 
    { 
    return l.clip.top; 
    }

function setClipTop(l, value) 
    { 
    l.clip.top = value; 
    }

// Clip Bottom
function getClipBottom(l) 
    { 
    return l.clip.bottom;
    }

function setClipBottom(l, value) 
    { 
    l.clip.bottom = value; 
    }

// Clip Left
function getClipLeft(l) 
    { 
    return l.clip.left;
    }

function setClipLeft(l, value) 
    { 
    l.clip.left = value; 
    }

// Clip Right
function getClipRight(l) 
    { 
    return l.clip.right;
    }

function setClipRight(l, value) 
    {
    l.clip.right = value; 
    }

function setClipItem(l, side, value)
    {
    l.clip[side] = value;
    }

function setClip(ly, t, r, b, l)
    {
    //ly.clip.top = t;
    //ly.clip.right = r;
    //ly.clip.bottom = b;
    //ly.clip.left = l;
    ly.clip.setall(t,r,b,l);
    }

function getClipItem(l, side)
    {
    return l.clip[side];
    }

// Page X
function getPageX(l) 
    { 
    var pn = l;
    var left;
    var rval = 0;
    while(pn.tagName != "BODY")
	{
	if (pn.__pg_left == null)
	    {
	    left = pg_get_style(pn,'left');
	    left = parseInt(left);
	    if(isNaN(left))
		pn.__pg_left = 0;
	    else
		pn.__pg_left = left;
	    }
	rval += pn.__pg_left;
	do  {
	    pn = pn.parentNode;
	    }
	    while(pn.tagName != "DIV" && pn.tagName != "IMG" && pn.tagName != "BODY")
	}
    return rval;
    }

function setPageX(l, value) 
    { 
    if(l.nodeName == "BODY")
	return;
    var pval = getPageX(l.parentNode);
    setRelativeX(l, value - pval);
    }
    
// Page Y
function getPageY(l) 
    { 
    var pn = l;
    var top;
    var rval = 0;
    while(pn.tagName != "BODY")
	{
	if (pn.__pg_top == null)
	    {
	    top = pg_get_style(pn,'top');
	    top = parseInt(top);
	    if(isNaN(top))
		pn.__pg_top = 0;
	    else
		pn.__pg_top = top;
	    }
	rval += pn.__pg_top;
	do  {
	    pn = pn.parentNode;
	    }
	    while(pn.tagName != "DIV" && pn.tagName != "IMG" && pn.tagName != "BODY")
	}
    return rval;
    }

function setPageY(l, value) 
    { 
    if(l.nodeName == "BODY")
	return;
    var pval = getPageY(l.parentNode);
    setRelativeY(l, value - pval);
    }

function getInnerHeight()
    {
    return window.innerHeight;
    }

function getInnerWidth()
    {
    return window.innerWidth;
    }

function getdocWidth(o)
    {
    if (!o) o = document;
    if (o == document)
	return o.body.scrollWidth;
    else
	return o.scrollWidth;
    }

function getdocHeight(o)
    {
    if (!o) o = document;
    if (o == document)
	return o.body.scrollHeight;
    else
	return o.scrollHeight;
    }

function getpageXOffset()
    {
    return window.pageXOffset ;
    }

function getpageYOffset()
    {
    return window.pageYOffset;
    }

/*** Get the size of a DOM node's parent container.
 *** 
 *** @param l The DOM node.
 *** @returns The width and height of the parent container.
 ***/
function getParentSize(l)
    {
    const parentRect = l.parentNode.getBoundingClientRect();
    return { width: parentRect.width, height: parentRect.height };
    }

/*** Get the width of a DOM node's parent container.
 *** 
 *** @param l The DOM node.
 *** @returns The width of the parent container.
 ***/
function getParentW(l)
    {
    return getParentSize(l).width;
    }

/*** Get the height of a DOM node's parent container.
 *** 
 *** @param l The DOM node.
 *** @returns The height of the parent container.
 ***/
function getParentH(l)
    {
    return getParentSize(l).height;
    }


/*** Problem:
 *** If the programmer calls setRelativeX() (or a similar function, such as moveTo() or moveBy()),
 *** they might be using a value they got from the server, based on the resolution when the page
 *** was first loaded. However, they might also be using a value they got dynamically by calling
 *** some function to check the actual size of an element. Previously, this distinction did not
 *** matter because these values would be the same. However, now that pages can be resized on the
 *** client, it does matter.
 ***/

 
/*** We ignore the current value of __pg_left in the following functions even
 *** though it might be correct and faster than querying the DOM. However, the
 *** layout may have changed since last time, so we always requery the DOM.
 ***/
function getRelative(l, d)
    {
    const val = parseInt(pg_get_style(l, d, NaN));
    return l['__pg_' + d] = (isNaN(val)) ? 0 : val;
    }

function getRelativeX(l) { return getRelative(l, 'left'); }
function getRelativeY(l) { return getRelative(l, 'top'); }
function getRelativeW(l) { return getRelative(l, 'width'); }
function getRelativeH(l) { return getRelative(l, 'height'); }

/*** Sets the location of a DOM node relative to its parent container.
 *** 
 *** @param l The DOM node being set. (Assumed to be defined.)
 *** @param value The new location. This can be a CSS string.
 *** @param {'left'|'top'|'width'|'height'} d The dimension being set.
 ***/
function setRelative(l, value, d)
    {
    /** Convert the value to a number, if possible. **/
    const parsedValue = parseInt(value);
    if (!isNaN(parsedValue)) value = parsedValue;

    pg_set_style(l, d, value);
    l['__pg_' + d + '_style'] = value;
    return l['__pg_' + d] = parseInt(pg_get_style(l, d));
    }

function setRelativeX(l, value) { return setRelative(l, value, 'left'); }
function setRelativeY(l, value) { return setRelative(l, value, 'top'); }
function setRelativeW(l, value) { return setRelative(l, value, 'width'); }
function setRelativeH(l, value) { return setRelative(l, value, 'height'); }

/*** Use these functions only in situations where performances is required
 *** and you (a) value does not need to be parsed and (b) you do not need
 *** the return value of the function.
 ***/
function fast_setRelativeX(l, value)
    {
    pg_set_style(l, 'left', value);
    l['__pg_x_style'] = value;
    }
function fast_setRelativeY(l, value)
    {
    pg_set_style(l, 'top', value);
    l['__pg_y_style'] = value;
    }

/*** Sets a dimension of a DOM element using coordinates in the server
 *** generated adaptive layout. It is RECOMMENDED to call a specific sub-
 *** function (aka. setResponsiveX(), setResponsiveY(), etc.) instead of
 *** calling this function directly to avoid passing dimension directly.
 ***
 *** WARNING: Ensure that any value passed is calculated ENTIRELY using
 *** values from the server (e.g. widget properties) and no values from
 *** real page dimensions are used, as these change when the page is
 *** resized after being loaded for the first time.
 ***
 *** @param l The DOM node being set. (Assumed to be defined.)
 *** @param value The new location in server-side px. This value must be
 *** 		  parseable as a number.
 *** @param {'x'|'y'|'w'|'h'} d The letter for the dimension being set.
 ***/
function setResponsive(l, value, d) {
    /** Convert the value to a number, if possible. **/
    const parsedValue = parseInt(value);
    if (!isNaN(parsedValue)) value = parsedValue;
    
    /** Server-layout values are always numbers. **/
    if (typeof(value) !== 'number')
	{
	console.warn(`setResponsive(${l.id}, ?, '${d}'): Expected value to be a parseable number but got:`, value);
	return value;
	}
    
    /** The flexibility specified by the server. **/
    var fl_scale = l['__fl_scale_' + d] ?? wgtrGetServerProperty(l, 'fl_scale_' + d);
    if (fl_scale == undefined || fl_scale == null)
	{
	/** The server did not specify a flexibility, even though one was expected. **/
	const warningMsg = 'setResponsive() - FAIL: Missing ' + ((wgtrIsNode(l)) ? 'wgtr.' : '__') + 'fl_scale_' + d;
	console.warn(warningMsg, l);
	fl_scale = 0;
	}
    
    /** Inflexible elements don't need to be responsive. **/
    if (fl_scale <= 0) return setRelative(l, value, d);
    
    /** The parent width expected by the server in the adaptive layout. **/
    var d2 = d;
    if (d2 == 'x') d2 = 'w';
    if (d2 == 'y') d2 = 'h';

    var fl_parent = l['__fl_parent_' + d2] ?? wgtrGetServerProperty(l, 'fl_parent_' + d2);
    if (fl_parent == undefined || fl_parent == null)
	{
	/** I wonder if anyone reviewers will see this: Easter egg #7. **/
	const warningMsg = 'setResponsive() - FAIL: Missing ' + ((wgtrIsNode(l)) ? 'wgtr.' : '__') + 'fl_parent_' + d2;
	console.warn(warningMsg, l);
	}

    /** Generate and set the CSS. **/
    const css = `calc(${value}px + (100% - ${fl_parent}px) * ${fl_scale})`;
    const prop = { x:'left', y:'top', w:'width', h:'height' }[d];
    return setRelative(l, css, prop);
}

/** Call these functions instead of calling setResponsive() directly, which leads to less readable code. **/
function setResponsiveX(l, value) { return setResponsive(l, value, 'x'); }
function setResponsiveY(l, value) { return setResponsive(l, value, 'y'); }
function setResponsiveW(l, value) { return setResponsive(l, value, 'w'); }
function setResponsiveH(l, value) { return setResponsive(l, value, 'h'); }

/** Moves a DOM node to a location within the window. **/
function moveToAbsolute(l, x, y)
    {
    setPageX(l,x);
    setPageY(l,y);
    }

/*** Moves a DOM node to a location inside it's parent container.
 *** 
 *** @param l The DOM node being moved.
 *** @param x The new x coordinate. Can be a CSS string (if responsive is false).
 *** @param y The new y coordinate. Can be a CSS string (if responsive is false).
 *** @param responsive Whether the given coordinates should be treated as
 *** 		       adaptive, 'server-side', coordinates where setResponsive()
 *** 		       should be invoked to give them responsive design.
 ***/
function moveTo(l, x, y, responsive = false)
    {
    if (responsive)
	{
	setResponsiveX(l, x);
	setResponsiveY(l, x);
	}
    else
	{
	setRelativeX(l, x);
	setRelativeY(l, y);
	}
    }


function moveBy(l, xo, yo)
    {
    if (xo) setRelativeX(l, getRelativeX(l) + xo);
    if (yo) setRelativeY(l, getRelativeY(l) + yo);
    }

    
function resizeTo(l, w, h)
    {
    pg_set_style(l,'width',w);
    pg_set_style(l,'height',h);
    }

function moveAbove(lt, lb) 
    {    
    if (lb)
	{
        lt.parentLayer = pg_get_container(lb);
	if (cx__capabilities.Dom1HTML && lt.parentLayer)
	    lt.parentLayer.appendChild(lt);
        var z = htr_getzindex(lb);
	if (isNaN(z) && lt.parentLayer)
	    z = htr_getzindex(lt.parentLayer);
        if (!isNaN(z))
	    htr_setzindex(lt,++z);
	else
	    htr_setzindex(lt,100);
	}
    }
    
function moveBelow(lt, lb) 
    {    
    if (lb)
	{
        lt.parentLayer = pg_get_container(lb);
	if (cx__capabilities.Dom1HTML && lt.parentLayer)
	    lt.parentLayer.appendChild(lt);
        var z = htr_getzindex(lb);
	if (isNaN(z) && lt.parentLayer)
	    z = htr_getzindex(lt.parentLayer);
        if (!isNaN(z))
	    htr_setzindex(lt,--z);
	else
	    htr_setzindex(lt,1);
	}
    }

function getWidth(l)
    {
    if (l == window || l == document)
	return window.innerWidth;
    else
	return getClipWidth(l);
    }

function getHeight(l)
    {
    if (l == window || l == document)
	return window.innerHeight;
    else
	return getClipHeight(l);
    }

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

Window.clipped = new Set(); // Debug set for saving clipped dom nodes.
function ClipObject_SetAll(top,right,bottom,left)
    {
    var str = "rect(" 
	    + top + "px, " 
	    + right + "px, " 
	    + bottom + "px, "
	    + left + "px)";
    this.arr = {1:top,2:right,3:bottom,4:left};
    if (this.noclip)
	{
	Window.clipped.delete(this.obj.id); // debug
	this.obj.style.setProperty('clip', "");
	}
    else
	{
	Window.clipped.add(this.obj.id); // debug
	this.obj.style.setProperty('clip', str, "");
	}

    }

var ClipRegexp = /rect\((.*), (.*), (.*), (.*)\)/;
function ClipObject_GetPart(n)
    {
    if(n>4 || n<1)
	return null;
    var a = this.arr;
    if (!a)
	{
	var clip = this.obj.style.clip;
	if(!clip)
	    //clip = getComputedStyle(this.obj,null).getPropertyCSSValue('clip').cssText;
	    clip = getComputedStyle(this.obj,null).clip;
	a = this.arr = ClipRegexp.exec(clip);
	}
    if (!a)
	a = this.arr = [0, 0, pg_get_style(this.obj, 'width'), pg_get_style(this.obj, 'height'), 0];
    /*if(a)*/
    return parseInt(a[n]);
    /*else
	{
	if(n == 1 || n == 4)
	    return 0;
	else
	    {
	    if(n == 2)
		return pg_get_style(this.obj,'width');
	    else
		return pg_get_style(this.obj,'height');
	    }
	}*/
    }

function ClipObject(o)
    {
    this.obj = o;

    this.setall = ClipObject_SetAll;
    this.getpart = ClipObject_GetPart;
    }

ClipObject.prototype.__defineGetter__("top", function () 
	{
	return this.getpart(1);
	}
    );

ClipObject.prototype.__defineGetter__("right", function () 
	{
	return this.getpart(2);
	}
    );

ClipObject.prototype.__defineGetter__("width", function () 
	{
	return this.right - this.left;
	}
    );

ClipObject.prototype.__defineGetter__("bottom", function () 
	{
	return this.getpart(3);
	}
    );

ClipObject.prototype.__defineGetter__("height", function () 
	{
	return this.bottom - this.top;
	}
    );

ClipObject.prototype.__defineGetter__("left", function () 
	{
	return this.getpart(4);
	}
    );

ClipObject.prototype.__defineSetter__("top", function (val) 
	{
	this.setall(val,this.right,this.bottom,this.left);
	}
    );

ClipObject.prototype.__defineSetter__("right", function (val) 
	{
	this.setall(this.top,val,this.bottom,this.left);
	}
    );

ClipObject.prototype.__defineSetter__("width", function (val) 
	{
	this.right = this.left + val;
	}
    );

ClipObject.prototype.__defineSetter__("bottom", function (val) 
	{
	this.setall(this.top,this.right,val,this.left);
	}
    );

ClipObject.prototype.__defineSetter__("height", function (val) 
	{
	this.bottom = this.top + val;
	}
    );

ClipObject.prototype.__defineSetter__("left", function (val) 
	{
	this.setall(this.top,this.right,this.bottom,val);
	}
    );

HTMLElement.prototype.__defineGetter__("clip", function () 
	{ 
	/** keep the same ClipObject around -- that way we can use watches on it **/
	if(this.cx__clip)
	    return this.cx__clip;
	else
	    return this.cx__clip = new ClipObject(this);
	}
    );

// Load indication
if (window.pg_scripts) pg_scripts['ht_geom_dom1html.js'] = true;
