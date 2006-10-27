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

// Cross browser Geometry DOM0NS

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

function setClip(l, t, r, b, l)
    {
    l.clip.top = t;
    l.clip.right = r;
    l.clip.bottom = b;
    l.clip.left = l;
    }

function getClipItem(l, side)
    {
    return l.clip[side];
    }

// Page X
function getPageX(l) 
    { 
    return l.pageX; 
    }

function setPageX(l, value) 
   { 
   l.pageX = value; 
   }
    
// Page Y
function getPageY(l) 
   { 
   return l.pageY; 
   }

function setPageY(l, value) 
    { 
    l.pageY = value; 
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
    if (cx__capabilities.Dom0NS)
	{
	if (o == document)
	    return o.width;
	else
	    return o.document.width;
	}
    else
	{
	if (o == document)
	    return o.body.scrollWidth;
	else
	    return o.scrollWidth;
	}
    }

function getdocHeight(o)
    {
    if (!o) o = document;
    if (cx__capabilities.Dom0NS)
	{
	if (o == document)
	    return o.height;
	else
	    return o.document.height;
	}
    else
	{
	if (o == document)
	    return o.body.scrollHeight;
	else
	    return o.scrollHeight;
	}
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
   return l.x
   }

function setRelativeX(l, value)
   {
   l.x = value;
   }

function getRelativeY(l)
   {
   return l.y
   }

function setRelativeY(l, value)
   {
   l.y = value;
   }

function moveToAbsolute(l, x, y)
    {
    l.moveToAbsolute(x, y);
    }

function moveTo(l, x, y)
    {
    l.moveTo(x, y);
    }


function moveBy(l, xo, yo)
    {
    l.moveBy(xo,yo);
    }

    
function resizeTo(l, w, h)
    {
    l.resizeTo(w, h);
    }

function moveAbove(lt, lb) 
    {    
    lt.moveAbove(lb);
    }
    
function moveBelow(lt, lb) 
    {    
    lt.moveBelow(lb);
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

