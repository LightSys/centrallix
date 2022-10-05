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

function getRelativeX(l)
    {
    if (l.__pg_left != null) return l.__pg_left;
    var left = parseInt(pg_get_style(l,'left'));
    l.__pg_left = isNaN(left)?0:left;
    return l.__pg_left;
    }

function setRelativeX(l, value)
    {
    pg_set_style(l,'left',(l.__pg_left = parseInt(value)));
    return l.__pg_left;
    }

function getRelativeY(l)
    {
    if (l.__pg_top != null) return l.__pg_top;
    return (l.__pg_top = parseInt(pg_get_style(l,'top')));
    }

function setRelativeY(l, value)
    {
    pg_set_style(l,'top',(l.__pg_top = parseInt(value)));
    return l.__pg_top;
    }

function moveToAbsolute(l, x, y)
    {
    setPageX(l,x);
    setPageY(l,y);
    }

function moveTo(l, x, y)
    {
    //pg_set_style_string(this,'position','absolute');
    setRelativeX(l,x);
    setRelativeY(l,y);
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

function ClipObject_SetAll(top,right,bottom,left)
    {
    var str = "rect(" 
	    + top + "px, " 
	    + right + "px, " 
	    + bottom + "px, "
	    + left + "px)";
    this.arr = {1:top,2:right,3:bottom,4:left};
    this.obj.style.setProperty('clip',str,"");
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
