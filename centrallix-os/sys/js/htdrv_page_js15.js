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
    this.obj.style.setProperty('clip',str,"");
    }

var ClipRegexp = /rect\((.*), (.*), (.*), (.*)\)/;
function ClipObject_GetPart(n)
    {
    if(n>4 || n<1)
	return null;
    var clip = this.obj.style.clip;
    if(!clip)
	clip = getComputedStyle(this.obj,null).getPropertyCSSValue('clip').cssText;
    var a = ClipRegexp.exec(clip);
    if(a)
	return parseInt(a[n]);
    else
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
	}
    }

function ClipObject(o)
    {
    this.obj = o;

    this.setall = ClipObject_SetAll;
    this.getpart = ClipObject_GetPart
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
    return this.right - this.left;
    }

ClipObject.prototype.bottom getter = function () 
    {
    return this.getpart(3);
    }

ClipObject.prototype.height getter = function () 
    {
    return this.bottom - this.top;
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
    this.right = this.left + val;
    }

ClipObject.prototype.bottom setter = function (val) 
    {
    this.setall(this.top,this.right,val,this.left);
    }

ClipObject.prototype.height setter = function (val) 
    {
    this.bottom = this.top + val;
    }

ClipObject.prototype.left setter = function (val) 
    {
    this.setall(this.top,this.right,this.bottom,val);
    }

HTMLElement.prototype.clip getter = function () 
    { 
    /** keep the same ClipObject around -- that way we can use watches on it **/
    if(this.cx__clip)
	return this.cx__clip;
    else
	return this.cx__clip = new ClipObject(this);
    }

function Element_MoveBy(x,y)
    {
    if (x) this.x = this.x + x;
    if (y) this.y = this.y + y;
    }

function Element_MoveTo (x,y)
    {
    pg_set_style_string(this,'position','absolute');
    this.x = x;
    this.y = y;
    }

function Element_MoveToAbsolute(x,y)
    {
    this.pageX = x;
    this.pageY = y;
    }

function Element_ResizeTo(w,h)
    {
    pg_set_style(this,'width',w);
    pg_set_style(this,'height',h);
    }

function Element_MoveAbove(lb)
    {
    if (lb)
	{
        this.parentLayer = pg_get_container(lb);
	if (cx__capabilities.Dom1HTML && this.parentLayer)
	    this.parentLayer.appendChild(this);
        var z = htr_getzindex(lb);
        htr_setzindex(this,++z);
	}
    }
function Element_MoveBelow(lb)
    {
    if (lb)
	{
        this.parentLayer = lb.parentLayer;
	if (cx__capabilities.Dom1HTML && this.parentLayer)
	    this.parentLayer.appendChild(this);
        var z = htr_getzindex(lb);
        htr_setzindex(this,--z);
	}
    }

HTMLElement.prototype.moveBy = Element_MoveBy; 
HTMLElement.prototype.moveTo = Element_MoveTo;
HTMLElement.prototype.moveToAbsolute = Element_MoveToAbsolute;
HTMLElement.prototype.resizeTo = Element_ResizeTo;
HTMLElement.prototype.moveAbove = Element_MoveAbove;
HTMLElement.prototype.moveBelow = Element_MoveBelow;

HTMLElement.prototype.x getter = function ()
    {
    if (this.__pg_left != null) return this.__pg_left;
    return parseInt(pg_get_style(this,'left'));
    }

HTMLElement.prototype.x setter = function (x)
    {
    pg_set_style(this,'left',(this.__pg_left =parseInt(x)));
    }

HTMLElement.prototype.y getter = function ()
    {
    if (this.__pg_top != null) return this.__pg_top;
    return parseInt(pg_get_style(this,'top'));
    }

HTMLElement.prototype.y setter = function (y)
    {
    pg_set_style(this,'top',(this.__pg_top = parseInt(y)));
    }

function Element_PageXGetter()
    {
    var pn = this;
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

function Element_PageXSetter(val)
    {
    if(this.nodeName == "BODY")
	return;
    var pval = this.parentNode.pageX;
    this.__pg_left = val - pval;
    this.x = val - pval;
    }

function Element_PageYGetter()
    {
    var pn = this;
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

function Element_PageYSetter(val)
    {
    if(this.nodeName == "BODY")
	return;
    var pval = this.parentNode.pageY;
    this.__pg_top = val - pval;
    this.y = val - pval;
    }

function _Layer_open()
    {
    this._is_open = true;
    this._tmptxt = "";
    }
function _Layer_write(t)
    {
    if (!this._is_open) this.open();
    this._tmptxt += t;
    }
function _Layer_writeln(t)
    {
    this.write(t + '\n');
    }
function _Layer_close()
    {
    this._is_open = false;
    this.innerHTML = this._tmptxt;
    }

function Layer (w,p)
    {
    var l = document.createElement("div");
    l.style.position = "absolute";
    if (w) l.style.width = w;
    if (p) l = p.appendChild(l); //SETH: $$$Layer.appendChild$$$ this won't do anything if p is window
    l.document = l;
    l.document._is_open = false;
    l.document._tmptxt = "";
    l.document.open = _Layer_open;
    l.document.write = _Layer_write;
    l.document.writeln = _Layer_writeln;
    l.document.close = _Layer_close;
    return l;
    }

HTMLElement.prototype.pageX getter = Element_PageXGetter;
HTMLElement.prototype.pageX setter = Element_PageXSetter;
HTMLElement.prototype.pageY getter = Element_PageYGetter;
HTMLElement.prototype.pageY setter = Element_PageYSetter;
