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

function tb_init(l,l2,l3,top,btm,rgt,lft,w,h,p,ts,nm)
    {
    l.LSParent = p;
    l.nofocus = true;
    l2.nofocus = true;
    if(!cx__capabilities.Dom2CSS)
	{
	top.nofocus = true;
	rgt.nofocus = true;
	btm.nofocus = true;
	lft.nofocus = true;
	}

    if(cx__capabilities.Dom0NS)
	{
	l.document.kind = 'tb';
	l2.document.kind = 'tb';
	l.document.layer = l;
	l2.document.layer = l;
	}
    else
	{
	l.kind = 'tb';
	l2.kind = 'tb';
	l.layer = l;
	l2.layer = l;
	}

    l.buttonName = nm;
    l.mainlayer = l;
    l.l2 = l2;
    l.l3 = l3;
    l.tp = top;
    l.btm = btm;
    l.lft = lft;
    l.rgt = rgt;
    l.kind = 'tb';
    l.lightBorderColor = '#FFFFFF';
    l.darkBorderColor = '#7A7A7A';
    if(!cx__capabilities.Dom2CSS)
	{
	l.clip.width = w;
	if (h != -1) l.clip.height = h;
	top.bgColor = l.lightBorderColor;
	lft.bgColor = l.lightBorderColor;
	btm.bgColor = l.darkBorderColor;
	rgt.bgColor = l.darkBorderColor;
	lft.clip.height = l.clip.height;
	rgt.clip.height = l.clip.height;
	rgt.pageX = l.pageX + l.clip.width - 2;
	btm.pageY = l.pageY + l.clip.height - 2;
	}
    l.tristate = ts;
    l.mode = -1;
    tb_setmode(l,0);
    if (l3.visibility == 'inherit')
	l.enabled = false;
    else
	l.enabled = true;
    l.watch('enabled', tb_setenable);
    }

function tb_setenable(prop, oldv, newv)
    {
    //alert('tb_setenable -- ' + this.id + ' -- ' + prop + ', ' + oldv + ', ' + newv);
    if (newv == true)
	{
	// make enabled
	pg_set_style_string(this.l2,'visibility','inherit');
	pg_set_style_string(this.l3,'visibility','hidden');
	}
    else
	{
	// make disabled
	pg_set_style_string(this.l2,'visibility','hidden');
	pg_set_style_string(this.l3,'visibility','inherit');
	}
    return newv;
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
		if(cx__capabilities.Dom2CSS)
		    {
		    layer.style.setProperty('border-width','0px',null);
		    layer.style.setProperty('margin','1px',null);
		    }
		else
		    {
		    layer.rgt.visibility = 'hidden';
		    layer.lft.visibility = 'hidden';
		    layer.tp.visibility = 'hidden';
		    layer.btm.visibility = 'hidden';
		    }
		break;
	    case 1: /* point, but no click */
		if(cx__capabilities.Dom2CSS)
		    {
		    layer.style.setProperty('border-width','1px',null);
		    layer.style.setProperty('margin','0px',null);
		    layer.style.setProperty('border-top-color',layer.lightBorderColor,null);
		    layer.style.setProperty('border-left-color',layer.lightBorderColor,null);
		    layer.style.setProperty('border-bottom-color',layer.darkBorderColor,null);
		    layer.style.setProperty('border-right-color',layer.darkBorderColor,null);
		    }
		else
		    {
		    layer.rgt.visibility = 'inherit';
		    layer.lft.visibility = 'inherit';
		    layer.tp.visibility = 'inherit';
		    layer.btm.visibility = 'inherit';
		    layer.tp.bgColor = layer.lightBorderColor;
		    layer.lft.bgColor = layer.lightBorderColor;
		    layer.btm.bgColor = layer.darkBorderColor;
		    layer.rgt.bgColor = layer.darkBorderColor;
		    }
		break;
	    case 2: /* point and click */
		if(cx__capabilities.Dom2CSS)
		    {
		    layer.style.setProperty('border-width','1px',null);
		    layer.style.setProperty('margin','0px',null);
		    layer.style.setProperty('border-top-color',layer.darkBorderColor,null);
		    layer.style.setProperty('border-left-color',layer.darkBorderColor,null);
		    layer.style.setProperty('border-bottom-color',layer.lightBorderColor,null);
		    layer.style.setProperty('border-right-color',layer.lightBorderColor,null);
		    }
		else
		    {
		    layer.rgt.visibility = 'inherit';
		    layer.lft.visibility = 'inherit';
		    layer.tp.visibility = 'inherit';
		    layer.btm.visibility = 'inherit';
		    layer.tp.bgColor = layer.darkBorderColor;
		    layer.lft.bgColor = layer.darkBorderColor;
		    layer.btm.bgColor = layer.lightBorderColor;
		    layer.rgt.bgColor = layer.lightBorderColor;
		    }
		break;
	    }
	}
    }
