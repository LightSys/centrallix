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

function ClipObject(o)
    {
    this.obj = o;

    this.setall = function (top,right,bottom,left)
	{
	var str = "rect(" 
		+ top + "px, " 
		+ right + "px, " 
		+ bottom + "px, "
		+ left + "px)";
	this.obj.style.setProperty('clip',str,"");
	}

    this.getpart = function (n)
	{
	var clip = this.obj.style.clip;
	if(!clip)
	    clip = getComputedStyle(this.obj,null).getPropertyCSSValue('clip').cssText;
	var a = /rect\((.*), (.*), (.*), (.*)\)/.exec(clip);
	return parseInt(a[n]);
	}
    }

ClipObject.prototype.top getter = function () 
    {
    return this.getpart(1);
    }

ClipObject.prototype.right getter = function () 
    {
    return this.getpart(2);
    }

ClipObject.prototype.width getter = function () 
    {
    return this.right;
    }

ClipObject.prototype.bottom getter = function () 
    {
    return this.getpart(3);
    }

ClipObject.prototype.height getter = function () 
    {
    return this.bottom;
    }

ClipObject.prototype.left getter = function () 
    {
    return this.getpart(4);
    }

ClipObject.prototype.top setter = function (val) 
    {
    this.setall(val,this.right,this.bottom,this.left);
    }

ClipObject.prototype.right setter = function (val) 
    {
    this.setall(this.top,val,this.bottom,this.left);
    }

ClipObject.prototype.width setter = function (val) 
    {
    this.right = val;
    }

ClipObject.prototype.bottom setter = function (val) 
    {
    this.setall(this.top,this.right,val,this.left);
    }

ClipObject.prototype.height setter = function (val) 
    {
    this.bottom = val;
    }

ClipObject.prototype.left setter = function (val) 
    {
    this.setall(this.top,this.right,this.bottom,val);
    }

HTMLElement.prototype.clip getter = function () 
    { 
    return new ClipObject(this); 
    }


HTMLElement.prototype.moveToAbsolute = function (x,y)
    {
    this.pageX = x;
    this.pageY = y;
    }

HTMLElement.prototype.pageX getter = function ()
    {
    if(this.nodeName == "BODY")
	return 0;
    return this.parentNode.pageX + pg_get_style(this,'left');
    }

HTMLElement.prototype.pageX setter = function (val)
    {
    if(this.nodeName == "BODY")
	return;
    pg_set_style(this,'left', val - this.parentNode.pageX);
    }

HTMLElement.prototype.pageY getter = function ()
    {
    if(this.nodeName == "BODY")
	return 0;
    return this.parentNode.pageY + pg_get_style(this,'top');
    }

HTMLElement.prototype.pageY setter = function (val)
    {
    if(this.nodeName == "BODY")
	return;
    pg_set_style(this,'top', val - this.parentNode.pageY);
    }

