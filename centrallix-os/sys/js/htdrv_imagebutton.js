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
    this.img.src=this.nImage.src;
    }

function ib_disable()
    {
    this.enabled=false;
    this.img.src=this.dImage.src;
    }

function ib_setenable(p,o,n)
    {
    if (n)
	this.img.src=this.nImage.src;
    else
	this.img.src=this.dImage.src;
    return n;
    }

function ib_init(l,n,p,c,d,w,h,po,nm,enable)
    {
    l.LSParent = po;
    l.nofocus = true;
    l.img = l.document.images[0];
    l.img.layer = l;
    l.img.kind = 'ib';
    l.kind = 'ib';
    l.mainlayer = l;
    l.clip.width = w;
    l.buttonName = nm;
    if (h == -1) l.nImage = new Image();
    else l.nImage = new Image(w,h);
    l.nImage.src = n;
    if (h == -1) l.pImage = new Image();
    else l.pImage = new Image(w,h);
    l.pImage.src = p;
    if (h == -1) l.cImage = new Image();
    else l.cImage = new Image(w,h);
    l.cImage.src = c;
    if (h == -1) l.dImage = new Image();
    else l.dImage = new Image(w,h);
    l.dImage.src = d;
    l.ActionEnable = ib_enable;
    l.ActionDisable = ib_disable;
    l.enabled = null;
    l.watch("enabled",ib_setenable);
    l.enabled = enable;
    }
