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

function tb_init(l,l2,top,btm,rgt,lft,w,h,p,ts,nm)
    {
    l.LSParent = p;
    l.nofocus = true;
    l2.nofocus = true;
    top.nofocus = true;
    rgt.nofocus = true;
    btm.nofocus = true;
    lft.nofocus = true;
    l.document.kind = 'tb';
    l2.document.kind = 'tb';
    l.document.layer = l;
    l2.document.layer = l;
    l.buttonName = nm;
    l.l2 = l2;
    l.tp = top;
    l.btm = btm;
    l.lft = lft;
    l.rgt = rgt;
    l.kind = 'tb';
    l.clip.width = w;
    if (h != -1) l.clip.height = h;
    top.bgColor = '#FFFFFF';
    lft.bgColor = '#FFFFFF';
    btm.bgColor = '#7A7A7A';
    rgt.bgColor = '#7A7A7A';
    lft.clip.height = l.clip.height;
    rgt.clip.height = l.clip.height;
    rgt.pageX = l.pageX + l.clip.width - 2;
    btm.pageY = l.pageY + l.clip.height - 2;
    l.mode = 0;
    l.tristate = ts;
    }

function tb_setmode(layer,mode)
    {
    if (mode != layer.mode)
	{
	layer.mode = mode;
	if (layer.tristate == 0 && mode == 0) mode = 1;
	switch(mode)
	    {
	    case 0: /* no point no click */
		layer.rgt.visibility = 'hidden';
		layer.lft.visibility = 'hidden';
		layer.tp.visibility = 'hidden';
		layer.btm.visibility = 'hidden';
		break;
	    case 1: /* point, but no click */
		layer.rgt.visibility = 'inherit';
		layer.lft.visibility = 'inherit';
		layer.tp.visibility = 'inherit';
		layer.btm.visibility = 'inherit';
		layer.tp.bgColor = '#FFFFFF';
		layer.lft.bgColor = '#FFFFFF';
		layer.btm.bgColor = '#7A7A7A';
		layer.rgt.bgColor = '#7A7A7A';
		break;
	    case 2: /* point and click */
		layer.rgt.visibility = 'inherit';
		layer.lft.visibility = 'inherit';
		layer.tp.visibility = 'inherit';
		layer.btm.visibility = 'inherit';
		layer.tp.bgColor = '#7A7A7A';
		layer.lft.bgColor = '#7A7A7A';
		layer.btm.bgColor = '#FFFFFF';
		layer.rgt.bgColor = '#FFFFFF';
		break;
	    }
	}
    }
