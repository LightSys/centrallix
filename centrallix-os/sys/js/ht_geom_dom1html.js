// Copyright (C) 1998-2026 LightSys Technology Services, Inc.
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

/*** Coordinate spaces:
 *** - Design px: The numbers from .app/.cmp files only exist on the server.
 *** - Layout px: The CSS px values on the page, like: styles, offsetWidth,
 ***   jQuery's .width(), getRelative*().  All geometry from the server uses
 ***   these, and so does widget code.
 *** - Viewport px: The values from the browser including page zoom, used by:
 ***   event clientX/pageX, window sizes, scroll offsets,
 ***   getBoundingClientRect(), and thus jQuery's .offset() and .position().
 ***   These need to be converted to layout px before widgets use them.
 ***   ResizeObserver sizes use layout px in Chrome vs. viewport px in Firefox,
 ***   so observers ignore them and call getters to get layout values.
 ***
 *** Use the functions in this file or cx__event's pageX/pageY to get layout px
 *** to avoid duplicating the zoom calculations from here.
 ***/

/*** The page's CSS zoom, in viewport px per layout px.  For example, a value of
 *** 2 means content appears in the viewport at double the size of its layout.
 ***/
var cx__zoom = 1;

/*** Converts a coordinate or length from viewport px to layout px.
 ***
 *** @param v The value in viewport px.
 *** @returns The value in layout px.
 ***/
function toLayoutPx(v)
    {
    return v / cx__zoom;
    }

// Add useful Math functions.
Math.clamp = (min, val, max) => Math.min(Math.max(min, val), max);
Math.isBetween = (lowerBound, num, upperBound) => (lowerBound < num && num < upperBound);

/*** Whether to enable noclip (which disables generation of clipping CSS) by
 *** default. This requires code to explicitly call enableClippingCSS() to
 *** generate clipping CSS.
 ***/
const default_noclip_value = true;

/*** Experimental system for turning off clipping CSS.
 *** The clip values are still stored and can be queried
 *** for legacy compatibility, but they will not output
 *** any clip rectangles or clip paths in the CSS or HTML.
 ***/
/** Ensure clipping is disabled for a layer / HTML node. **/
function disableClippingCSS(l)
    {
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

/*** The page's design {width, height} and zoom {strength} (a percent), or null
 *** if the page does not zoom.
 ***/
var cx__zoom_inputs = null;

/** The smallest zoom a page shrinks to, matching NHT_ZOOM_MIN on the server. **/
const CX_ZOOM_MIN = 0.4;

/*** Picks the zoom for a viewport size the same way nht_i_GetZoom() does on
 *** the server.  Pages grow by the zoom strength power of the extra space, and
 *** shrink by all of the missing space down to CX_ZOOM_MIN.
 ***
 *** @param vw The width of the viewport, in viewport px.
 *** @param vh The height of the viewport, in viewport px.
 *** @returns The zoom, in viewport px per layout px, or 0 if the page does
 ***          not zoom.
 ***/
function getZoom(vw, vh)
    {
    if (!cx__zoom_inputs) return 0;
    const { width, height, strength } = cx__zoom_inputs;
    if (!(width > 0 && height > 0 && vw > 0 && vh > 0)) return 0;

    const ratio = Math.min(vw / width, vh / height);
    if (ratio > 1) return Math.pow(ratio, strength / 100);
    return Math.max(ratio, CX_ZOOM_MIN);
    }

/*** Rezooms the page for the current viewport size, so its layout fills the
 *** viewport divided by the zoom.  Resize handlers that measure the page must
 *** run after this.
 ***/
function updateZoom()
    {
    const zoom = getZoom(window.innerWidth, window.innerHeight);
    if (!(zoom > 0) || zoom === cx__zoom) return;
    cx__zoom = zoom;
    document.documentElement.style.zoom = zoom;
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

/*** Page coordinates are document coordinates in layout px: they include the
 *** scroll offset, so they compare directly against cx__event's pageX/pageY.
 ***/

// Page X
function getPageX(l)
    {
    if (!l || !l.getBoundingClientRect) return null;
    return toLayoutPx(l.getBoundingClientRect().left + window.scrollX);
    }

function setPageX(l, value)
    {
    setPageXY(l, value, null);
    }

// Page Y
function getPageY(l)
    {
    if (!l || !l.getBoundingClientRect) return null;
    return toLayoutPx(l.getBoundingClientRect().top + window.scrollY);
    }

function setPageY(l, value)
    {
    setPageXY(l, null, value);
    }

/*** Moves a DOM node to the specified page coordinates.  The move is a delta
 *** from where the node actually sits, so the positions of it and its ancestors do
 *** not matter.
 ***
 *** One call to this function is faster than calling setPageX() & setPageY().
 ***
 *** @param l The DOM node being moved.
 *** @param x The new page x coordinate, or null to leave x unchanged.
 *** @param y The new page y coordinate, or null to leave y unchanged.
 ***/
function setPageXY(l, x, y)
    {
    if (!l || !l.getBoundingClientRect || l.tagName === "BODY") return;
    const rect = l.getBoundingClientRect();

    if (x != null)
	{
	const dx = x - toLayoutPx(rect.left + window.scrollX);
	if (dx) setRelativeX(l, Math.round(getRelativeX(l) + dx));
	}
    if (y != null)
	{
	const dy = y - toLayoutPx(rect.top + window.scrollY);
	if (dy) setRelativeY(l, Math.round(getRelativeY(l) + dy));
	}

    return;
    }

/*** Gets a DOM node's border box in page coordinates.
 ***
 *** @param l The DOM node.
 *** @returns The {left, top, right, bottom, width, height} of the node.
 ***/
function getPageRect(l)
    {
    const rect = l.getBoundingClientRect();
    const left   = toLayoutPx(rect.left + window.scrollX);
    const top    = toLayoutPx(rect.top + window.scrollY);
    const width  = toLayoutPx(rect.width);
    const height = toLayoutPx(rect.height);
    return { left, top, right: left + width, bottom: top + height, width, height };
    }

/*** Gets a DOM node's position within its offset parent (similar to jQuery's
 *** .position()), but in layout px.
 ***
 *** @param l The DOM node.
 *** @returns The {left, top} of the node's margin box, relative to the
 ***          offset parent's padding box.
 ***/
function getPosition(l)
    {
    const parent       = $(l).offsetParent()[0];
    const rect         = getPageRect(l);
    const parent_rect  = getPageRect(parent);
    const style        = getComputedStyle(l);
    const parent_style = getComputedStyle(parent);
    return {
	left: rect.left - parent_rect.left - parseFloat(parent_style.borderLeftWidth) - parseFloat(style.marginLeft),
	top:  rect.top  - parent_rect.top  - parseFloat(parent_style.borderTopWidth)  - parseFloat(style.marginTop),
	};
    }

function getInnerHeight()
    {
    return toLayoutPx(window.innerHeight);
    }

function getInnerWidth()
    {
    return toLayoutPx(window.innerWidth);
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
    return toLayoutPx(window.pageXOffset);
    }

function getpageYOffset()
    {
    return toLayoutPx(window.pageYOffset);
    }

/*** Get the size of a DOM node's parent container.
 *** 
 *** @param l The DOM node.
 *** @returns The width and height of the parent container.
 ***/
function getParentSize(l)
    {
    const parentRect = getPageRect(l.parentNode);
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

 
/*** We ignore the current value of __pg_left in the following functions even
 *** though it might be correct and faster than querying the DOM. However, the
 *** layout may have changed since last time, so we always requery the DOM.
 ***/
function getRelative(l, d)
    {
    if (!l)
	{
	console.error('Call to getRelative(', l, `, '${d}')`);
	return 0;
	}

    /*** A node with no layout box (display:none, or not in the document yet)
     *** has a length of 'auto', not a number.  In this case, report the cached value
     *** and skip caching an 'auto'.
     ***/
    const val = parseInt(pg_get_style(l, d), 10);
    if (isNaN(val)) return l['__pg_' + d] ?? 0;

    return l['__pg_' + d] = val;
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

    /** Offsets read back as written, so skip rereading (which forces an expensive layout flush). **/
    if (!isNaN(parsedValue) && (d === 'left' || d === 'top'))
	return l['__pg_' + d] = parsedValue;

    /*** Read back the value the browser actually used, which may differ from
     *** the one we asked for (a percentage or a calc(), for instance).  If the
     *** node has no layout box to measure, cache the number we asked for when
     *** we have one, so that getRelative() has something usable to report.
     ***/
    const used = parseInt(pg_get_style(l, d));
    if (!isNaN(used)) return l['__pg_' + d] = used;
    if (!isNaN(parsedValue)) return l['__pg_' + d] = parsedValue;
    return l['__pg_' + d] ?? 0;
    }

function setRelativeX(l, value) { return setRelative(l, value, 'left'); }
function setRelativeY(l, value) { return setRelative(l, value, 'top'); }
function setRelativeW(l, value) { return setRelative(l, value, 'width'); }
function setRelativeH(l, value) { return setRelative(l, value, 'height'); }

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
 *** @param {'left'|'top'|'width'|'height'} d The dimension being set.
 *** @returns The size in px that the browser actually used, as read back by
 ***          setRelative().  This may differ from the px in the generated
 ***          calc(), which is only the size in the server's adaptive layout.
 ***          On the failure paths, nothing is set and `value` is returned.
 ***/
function setResponsive(l, value, d) {
    /** Convert the value to a number, if possible. **/
    const parsedValue = parseInt(value);
    if (!isNaN(parsedValue)) value = parsedValue;
    
    /** Server-layout values are always numbers. **/
    if (typeof(value) !== 'number')
	{
	console.warn(`setResponsive(${l.id}, ?, '${d}'): Expected a parseable number but got:`, value);
	return value;
	}
    
    /** The server names its flexibilities x/y/w/h, not by CSS dimension. **/
    const fl_d = { left:'x', top:'y', width:'w', height:'h' }[d];
    if (!fl_d)
	{
	console.warn(`setResponsive() - FAIL: Unknown dimension ${d} (should be left, top, width, or height)`);
	/** We can't set or even query the dimension, so nothing changed. **/
	return value;
	}

    /** The flexibility specified by the server. **/
    let fl_scale = l['__fl_scale_' + fl_d] ?? wgtrGetServerProperty(l, 'fl_scale_' + fl_d);
    if (fl_scale == undefined)
	{
	/** The server did not specify a flexibility, even though one was expected. **/
	const missing_attr_name = ((wgtrIsNode(l)) ? 'wgtr.' : '__') + 'fl_scale_' + fl_d;
	console.warn('setResponsive() - FAIL: Missing ' + missing_attr_name + ' for', l);
	fl_scale = 0;
	}
    
    /** Inflexible elements don't need to be responsive. **/
    if (fl_scale <= 0) return setRelative(l, value, d);
    
    /** The parent width or height expected by the server in the adaptive layout. **/
    let fl_d2 = fl_d;
    if (fl_d2 === 'x') fl_d2 = 'w';
    if (fl_d2 === 'y') fl_d2 = 'h';

    const fl_parent = l['__fl_parent_' + fl_d2] ?? wgtrGetServerProperty(l, 'fl_parent_' + fl_d2);
    if (fl_parent == undefined)
	{
	/** I wonder if anyone reviewers will see this: Easter egg #7. **/
	const missing_attr_name = ((wgtrIsNode(l)) ? 'wgtr.' : '__') + 'fl_parent_' + fl_d2;
	console.warn('setResponsive() - FAIL: Missing ' + missing_attr_name + ' for', l);
	}

    /** Generate and set the CSS. **/
    const css = `calc(${value}px + (100% - ${fl_parent}px) * ${fl_scale})`;
    return setRelative(l, css, d);
}

/** Call these functions instead of calling setResponsive() directly, which leads to less readable code. **/
function setResponsiveX(l, value) { return setResponsive(l, value, 'left'); }
function setResponsiveY(l, value) { return setResponsive(l, value, 'top'); }
function setResponsiveW(l, value) { return setResponsive(l, value, 'width'); }
function setResponsiveH(l, value) { return setResponsive(l, value, 'height'); }

/** Moves a DOM node to a location within the window. **/
function moveToAbsolute(l, x, y)
    {
    setPageXY(l, x, y);
    }

/*** Moves a DOM node to a location inside its parent container.
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
	setResponsiveY(l, y);
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
	return getInnerWidth();
    else
	return getClipWidth(l);
    }

function getHeight(l)
    {
    if (l == window || l == document)
	return getInnerHeight();
    else
	return getClipHeight(l);
    }

// Copyright (C) 1998-2026 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function ClipObject_SetAll(top,right,bottom,left)
    {
    var str = "rect(" 
	    + top + "px, " 
	    + right + "px, " 
	    + bottom + "px, "
	    + left + "px)";
    this.arr = {1:top,2:right,3:bottom,4:left};
    if (!this.hasOwnProperty('noclip')) this.noclip = default_noclip_value;
    this.obj.style.setProperty('clip', (this.noclip) ? "" : str);
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

Object.defineProperties(ClipObject.prototype, {
    top: {
	get() { return this.getpart(1); },
	set(val) { this.setall(val, this.right, this.bottom, this.left); },
	configurable: true,
	enumerable: true,
    },
    right: {
	get() { return this.getpart(2); },
	set(val) { this.setall(this.top, val, this.bottom, this.left); },
	configurable: true,
	enumerable: true,
    },
    bottom: {
	get() { return this.getpart(3); },
	set(val) { this.setall(this.top, this.right, val, this.left); },
	configurable: true,
	enumerable: true,
    },
    left: {
	get() { return this.getpart(4); },
	set(val) { this.setall(this.top, this.right, this.bottom, val); },
	configurable: true,
	enumerable: true,
    },
    width: {
	get() { return this.right - this.left; },
	set(val) { this.right = this.left + val; },
	configurable: true,
	enumerable: true,
    },
    height: {
	get() { return this.bottom - this.top; },
	set(val) { this.bottom = this.top + val; },
	configurable: true,
	enumerable: true,
    },
});

Object.defineProperty(HTMLElement.prototype, "clip", {
    get() { return (this.cx__clip) ? this.cx__clip : (this.cx__clip = new ClipObject(this));},
    configurable: true,
    enumerable: true,
});

// Load indication
if (window.pg_scripts) pg_scripts['ht_geom_dom1html.js'] = true;
