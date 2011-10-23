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

// Cross browser Geometry DOM0IE

// Clip Width
function getClipWidth(l) 
    { 
    var _right = parseInt(l.currentStyle.clipRight.split("px")[0]);
    var _left = parseInt(l.currentStyle.clipLeft.split("px")[0]);    
    var _width = _right - _left;
    
    if (isNaN(_width))
        {
        return l.offsetWidth;
	}
    
    return _width;
    }

function getRuntimeClipWidth(l) 
    {
    var cpvals = getRuntimeClipArray(l);
    return parseInt(cpvals[1]) - parseInt(cpvals[3]);
    }
    
// Setting the clip.width value to w is the same as:
// clip.right = clip.left + w;
function setClipWidth(l, value) 
    {    
    var curly = l.currentStyle;
    var c_left = curly.clipLeft;
    if (isNaN(parseInt(c_left)))
	{
	//c_left = l.offsetLeft;
	c_left = 0;
	var c_right = c_left + value;
	c_left = "" + c_left + "px";
	}
    else
	{
	var c_right = parseInt(c_left.split("px")[0]) + value;
	}
    
    setClip(l, curly.clipTop, "" + c_right + "px", curly.clipBottom, c_left);    
    }

// Clip Height
function getClipHeight(l) 
    { 
    var _bottom = parseInt(l.currentStyle.clipBottom.split("px")[0]);
    var _top = parseInt(l.currentStyle.clipTop.split("px")[0]);
    var _height = _bottom - _top;
    
    if (isNaN(_height))
        {
        return l.offsetHeight;
        }
    
    return _height;
    }

function getRuntimeClipHeight(l) 
    {
    var cpvals = getRuntimeClipArray(l);
    return parseInt(cpvals[2]) - parseInt(cpvals[0]);
    }

// Setting the clip.height to h is the same as:
// clip.bottom = clip.top + h;
function setClipHeight(l, value) 
    { 
    var curly = l.currentStyle;
    var c_top = curly.clipTop;
    if (isNaN(parseInt(c_top)))
	{
	//c_top = l.offsetTop;
	c_top = 0;
	var c_bottom = c_top + value;
	c_top = "" + c_top + "px";
	}
    else
	{
	var c_bottom = parseInt(c_top.split("px")[0]) + value;
	}
    c_bottom += "px";
    
    setClip(l, c_top, curly.clipRight, c_bottom, curly.clipLeft);
    }

// Clip Top
function getClipTop(l) 
    { 
    var rval = parseInt(l.currentStyle.clipTop); 
    if (isNaN(rval))
	return /*l.offsetTop*/ 0;
    else
	return rval;
    }

function setClipTop(l, value) 
    {
    var curly = l.currentStyle;
    setClip(l, "" + value + "px", curly.clipRight, curly.clipBottom, curly.clipLeft);
    }

// Clip Bottom
function getClipBottom(l) 
    { 
    var rval = parseInt(l.currentStyle.clipBottom);
    if (isNaN(rval)) return l.offsetHeight;
    return rval;
    }

function setClipBottom(l, value) 
    { 
    if (l.currentStyle)
        {
        curly = l.currentStyle;
        value += "px";
        setClip(l, curly.clipTop, curly.clipRight, value, curly.clipLeft);
	}
    else
    	{
    	var cpvals = getRuntimeClipArray(l);
    	setClip(l, cpvals[0], cpvals[1], value, cpvals[3]);
    	}    
    }

// Clip Left
function getClipLeft(l) 
    { 
    var rval = parseInt(l.currentStyle.clipLeft);
    if (isNaN(rval)) return 0;
    return rval;
    }

function setClipLeft(l, value) 
    { 
    var curly = l.currentStyle;
    setClip(l, curly.clipTop, curly.clipRight, curly.clipBottom, "" + value + "px");
    }

// Clip Right
function getClipRight(l) 
    { 
    var rval = parseInt(l.currentStyle.clipRight);
    if (isNaN(rval)) return l.offsetWidth;
    return rval;
    }

function setClipRight(l, value) 
    {     
    var curly;
    if (l.currentStyle)
        {
        curly = l.currentStyle;
        value += "px";
        setClip(l, curly.clipTop, value, curly.clipBottom, curly.clipLeft);
	}
    else
    	{
    	var cpvals = getRuntimeClipArray(l);
    	setClip(l, cpvals[0], value, cpvals[2], cpvals[3]);
    	}
    }

function setClip(l, c_top, c_right, c_bottom, c_left)
    {
    var _top = c_top;
    var _bottom = c_bottom;
    var _left = c_left;
    var _right = c_right;
   
    l.runtimeStyle.clip = "rect(" + _top + " " + _right + " " + _bottom + " " + _left + ")";    
    }

function getRuntimeClipArray(l) 
    {
    return l.runtimeStyle.clip.split("rect(")[1].split(")")[0].split("px")
    }

function setClipItem(l,side,value)
    {
    switch(side)
	{
	case 'left': return setClipLeft(l,value);
	case 'right': return setClipRight(l,value);
	case 'top': return setClipTop(l,value);
	case 'bottom': return setClipBottom(l,value);
	case 'width': return setClipWidth(l,value);
	case 'height': return setClipHeight(l,value);
	}
    }

function getClipItem(l,side)
    {
    switch(side)
	{
	case 'left': return getClipLeft(l);
	case 'right': return getClipRight(l);
	case 'top': return getClipTop(l);
	case 'bottom': return getClipBottom(l);
	case 'width': return getClipWidth(l);
	case 'height': return getClipHeight(l);
	}
    }
    
// Page X
function getPageX(l) 
    {
    if (window.Event)
    	return l.clientX + document.body.scrollLeft;
    return getAbsLeft(l, 0);
    }


function setPageX(l, value) 
   {
   if (window.Event)
   	l.clientX = value - document.body.scrollLeft;
   l.style.left = value - getAbsLeft(l.parentNode, 0);
   }

// Recursively get the accumulated sum of the left offset
// to the top body node from the current node provided
function getAbsLeft(l, value)
    {    	
    if (l == null || l.nodeName == "#document-fragment") return value;
    if (l.nodeName != "BODY")
        return getAbsLeft(l.parentNode, value + l.offsetLeft);
    return value + l.offsetLeft;
    }
    
// Page Y
function getPageY(l) 
   {
   if (window.Event)
   	return l.clientY + document.body.scrollTop;
   return getAbsTop(l, 0);
   }

function setPageY(l, value) 
    {
    if (window.Event)
    	l.clientY = value - document.body.scrollTop;
    l.style.top = value - getAbsTop(l.parentNode, 0);
    }

function getAbsTop(l, value)
    {    	    
    if (l == null || l.nodeName == "#document-fragment") return value;
    if (l.nodeName != "BODY")
        return getAbsTop(l.parentNode, value + l.offsetTop);
    return value + l.offsetTop;
    }

function getInnerHeight()
    {
    return document.body.clientHeight;
    }

function getInnerWidth()
    {
    return document.body.clientWidth;
    }

function getpageXOffset()
    {
    return document.body.scrollLeft;
    }

function getpageYOffset()
    {
    return document.body.scrollTop;
    }

function getRelativeX(l)
   {
   return l.offsetLeft;
   }

function setRelativeX(l, value)
   {
   l.style.left = value + "px";
   }

function getRelativeY(l)
   {
   return l.offsetTop;
   }

function setRelativeY(l, value)
   {
   l.style.top = value;
   }

function getdocWidth(o)
    {
    if (!o) o = document;
    if (o == document)
	return o.body.clientWidth;
    else
	return o.clientWidth;
    }

function getdocHeight(o)
    {
    if (!o) o = document;
    if (o == document)
	return o.body.clientHeight;
    else
	return o.clientHeight;
    }

function moveToAbsolute(l, x, y)
    {    
    setPageX(l, x);
    setPageY(l, y);
    }

function moveTo(l, x, y)
    {
    l.style.left = x + "px";
    l.style.top = y + "px";
    }

function moveBy(l, x, y)
    {
    if (x) setRelativeX(l, getRelativeX(l)+x);
    if (y) setRelativeY(l, getRelativeY(l)+y);
    }
    
function resizeTo(l, w, h)
    {
    //setClipRight(l, w);
    //setClipBottom(l, h);
    l.runtimeStyle.width = w + 'px';
    l.runtimeStyle.height = h + 'px';
    }
    
function moveAbove(lt, lb) 
    {
    if (lb != null)
        {
        var z = lb.currentStyle.zIndex;
        lt.runtimeStyle.zIndex = ++z;
        lt.parentLayer = lb.parentLayer;
        }
    }
    
function moveBelow(lt, lb) 
    {    
    if (lb != null)
        {
    	var z = lb.currentStyle.zIndex;
    	lt.runtimeStyle.zIndex = --z;
    	lt.parentLayer = lb.parentLayer;
        }
    }

// Load indication
if (window.pg_scripts) pg_scripts['ht_geom_dom0ie.js'] = true;
