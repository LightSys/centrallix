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

function ib_enable()
    {
    this.enabled=true;
    pg_set(this.img,'src',this.nImage.src);
    }

function ib_disable()
    {
    this.enabled=false;
    pg_set(this.img,'src',this.dImage.src);
    }

function ib_setenable(prop,oldval,newval)
    {
    var e = window.event;    
    var source = new Object();    
    (cx__capabilities.Dom0IE)? source = e.srcElement : source = this;
    
    if (newval)
	pg_set(source.img,'src',source.nImage.src);
    else
	pg_set(source.img,'src',source.dImage.src);
    return newval;
    }

function ib_init(l,n,p,c,d,w,h,po,nm,enable)
    {
    l.LSParent = po;
    l.nofocus = true;
    if(cx__capabilities.Dom0NS)
	{
	l.img = l.document.images[0];
	}
    else if(cx__capabilities.Dom1HTML)
	{
	l.img = l.getElementsByTagName("img")[0];
	}
    else
	{
	alert('browser not supported');
	}
    l.img.layer = l;
    l.img.kind = 'ib';
    l.kind = 'ib';
    l.layer = l;
    l.mainlayer = l;
    setClipWidth(l, w);

    l.buttonName = nm;
    if (h == -1) l.nImage = new Image();
    else l.nImage = new Image(w,h);
    pg_set(l.nImage,'src',n);
    if (h == -1) l.pImage = new Image();
    else l.pImage = new Image(w,h);
    pg_set(l.pImage,'src',p);
    if (h == -1) l.cImage = new Image();
    else l.cImage = new Image(w,h);
    pg_set(l.cImage,'src',c);
    if (h == -1) l.dImage = new Image();
    else l.dImage = new Image(w,h);
    pg_set(l.dImage,'src',d);
    l.ActionEnable = ib_enable;
    l.ActionDisable = ib_disable;
    l.enabled = null;
    if (cx__capabilities.Dom0IE)
    	{
    	l.attachEvent("onpropertychange",ib_setenable);
	}
    else
    	{
	l.watch("enabled",ib_setenable);
	}
    l.enabled = enable;
    }
