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

function getdocWidth()
    {
    return document.width;
    }

function getdocHeight()
    {
    return document.height;
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