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

function tb_init(param)
    {
    var l = param.layer;
    var l2 = param.layer2;
    var l3 = param.layer3;
    l.LSParent = param.parent;
    l.nofocus = true;
    l2.nofocus = true;
    htr_init_layer(l,l,'tb');
    htr_init_layer(l2,l,'tb');
    htr_init_layer(l3,l,'tb');
    if(!cx__capabilities.Dom2CSS && !cx__capabilities.Dom0IE)
	{
	param.top.nofocus = true;
	param.right.nofocus = true;
	param.bottom.nofocus = true;
	param.left.nofocus = true;
	}
    l.buttonName = param.name;

    l.l2 = l2;
    l.l3 = l3;
    l.tp = param.top;
    l.btm = param.bottom;
    l.lft = param.left;
    l.rgt = param.right;
    l.orig_x = getRelativeX(l);
    l.orig_y = getRelativeY(l);
    l.orig_ct = parseInt(getClipTop(l));
    l.orig_cb = parseInt(getClipTop(l)) + parseInt(getClipHeight(l));
    l.orig_cr = parseInt(getClipRight(l));
    l.orig_cl = parseInt(getClipLeft(l));
    l.lightBorderColor = '#FFFFFF';
    l.darkBorderColor = '#7A7A7A';
    if(cx__capabilities.Dom0NS)
	{
	setClipWidth(l, param.width);
	if (param.height != -1) setClipHeight(l, param.height);
	pg_set_style(param.top,'bgColor',l.lightBorderColor);
	pg_set_style(param.left,'bgColor',l.lightBorderColor);
	pg_set_style(param.bottom,'bgColor',l.darkBorderColor);
	pg_set_style(param.right,'bgColor',l.darkBorderColor);
	setClipHeight(param.left, getClipHeight(l));
	setClipHeight(param.right, getClipHeight(l));
	setPageX(param.right,getPageX(l)+getClipWidth(l)-2);
	setPageY(param.bottom,getPageY(l)+getClipHeight(l)-2);
	}
    l.tristate = param.tristate;
    l.mode = -1;
    tb_setmode(l,0);
    if (htr_getvisibility(l3) == 'inherit' || htr_getvisibility(l3) == 'visible')
	l.enabled = false;
    else
	l.enabled = true;
	
    if (!cx__capabilities.Dom0IE)
        l.watch('enabled', tb_setenable);
    else
    	{
    	//alert("watch is not supported!");
    	l.onpropertychange = tb_propchange;
	}
    }

function tb_propchange(prop, oldv, newv)
    {
    if (prop == 'enabled') tb_setenable(prop, oldv, newv);
    }

function tb_setenable(prop, oldv, newv)
    {    
    if (cx__capabilities.Dom0IE) 
    	{
    	var e = window.event;
    	//alert(e.srcElement);
    	if(e.propertyName.substr(0,6) == "style.")
    	    {
	    newv = e.srcElement.style[e.propertyName.substr(6)];
	    }
	else
	    {
	    newv = e.srcElement[e.propertyName];
	    }
	}
    //status = 'tb_setenable -- ' + this.id + ' -- ' + prop + ', ' + oldv + ', ' + newv;
    if (typeof newv == "boolean")
    {
    	//alert("here--" + newv);
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
    }
    return newv;
    }

function tb_setmode(layer,mode)
    {
    if (layer.tristate == 0 && mode == 0) mode = 1;
    if (mode != layer.mode)
	{
	//status = layer.id + " " + mode;
	layer.mode = mode;
	//status = "tristate " + layer.id + " " + layer.tristate;
	switch(mode)
	    {
	    case 0: /* no point no click */
		moveTo(layer,layer.orig_x,layer.orig_y);
		if(cx__capabilities.Dom2CSS)
		    {
		    layer.style.setProperty('border-width','0px',null);
		    layer.style.setProperty('margin','1px',null);
		    }
		else if(!cx__capabilities.Dom0IE)
		    {
		    layer.rgt.visibility = 'hidden';
		    layer.lft.visibility = 'hidden';
		    layer.tp.visibility = 'hidden';
		    layer.btm.visibility = 'hidden';
		    }
		else if(cx__capabilities.Dom0IE)
		    {		    
		    /*layer.style.borderStyle = 'solid';
		    layer.style.borderWidth = '0px';
		    layer.style.margin = '1px';		    	
		    layer.style.padding = '1px';*/
		    setClip(layer, layer.orig_ct+1, layer.orig_cr-1, layer.orig_cb-1, layer.orig_cl+1);
		    }		    
		break;

	    case 1: /* point, but no click */
		moveTo(layer,layer.orig_x,layer.orig_y);
		if(cx__capabilities.Dom2CSS)
		    {
		    layer.style.setProperty('border-width','1px',null);
		    layer.style.setProperty('margin','0px',null);
		    layer.style.setProperty('border-top-color',layer.lightBorderColor,null);
		    layer.style.setProperty('border-left-color',layer.lightBorderColor,null);
		    layer.style.setProperty('border-bottom-color',layer.darkBorderColor,null);
		    layer.style.setProperty('border-right-color',layer.darkBorderColor,null);
		    }
		else if(!cx__capabilities.Dom0IE)
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
		else if(cx__capabilities.Dom0IE)
		    {
		    /*layer.style.borderStyle = 'solid';
		    layer.style.borderWidth = '1px';
		    layer.style.margin = '0px';		    
		    layer.style.padding = '0px';*/
		    if (layer.tristate) setClip(layer, layer.orig_ct, layer.orig_cr, layer.orig_cb, layer.orig_cl);
		    layer.style.borderTopColor = layer.lightBorderColor;
		    layer.style.borderLeftColor = layer.lightBorderColor;
		    layer.style.borderBottomColor = layer.darkBorderColor;
		    layer.style.borderRightColor = layer.darkBorderColor;
		    }
		break;

	    case 2: /* point and click */
		moveTo(layer,layer.orig_x+1,layer.orig_y+1);
		if(cx__capabilities.Dom2CSS)
		    {
		    layer.style.setProperty('border-width','1px',null);
		    layer.style.setProperty('margin','0px',null);
		    layer.style.setProperty('border-top-color',layer.darkBorderColor,null);
		    layer.style.setProperty('border-left-color',layer.darkBorderColor,null);
		    layer.style.setProperty('border-bottom-color',layer.lightBorderColor,null);
		    layer.style.setProperty('border-right-color',layer.lightBorderColor,null);
		    }
		else if(!cx__capabilities.Dom0IE)
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
		else if(cx__capabilities.Dom0IE)
		    {
		    if (layer.tristate) setClip(layer, layer.orig_ct, layer.orig_cr, layer.orig_cb, layer.orig_cl);
		    /*layer.style.borderStyle = 'solid';
		    layer.style.borderWidth = '1px';
		    layer.style.margin = '0px';*/
		    /*layer.style.borderTopColor = layer.darkBorderColor;
		    layer.style.borderLeftColor = layer.darkBorderColor;
		    layer.style.borderBottomColor = layer.lightBorderColor;
		    layer.style.borderRightColor = layer.lightBorderColor;*/
		    layer.style.borderColor = 'gray white white gray';
		    }
		break;
	    }
	}
    }

function tb_dblclick(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'tb' && cx__capabilities.Dom0IE)
	{
	tb_mousedown(e);
	tb_mouseup(e);
	}
    }

function tb_mousedown(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'tb' && ly.enabled)
        {
        tb_setmode(ly,2);
        cn_activate(ly, 'MouseDown');
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tb_mouseup(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'tb' && ly.enabled)
        {
        if (e.pageX >= getPageX(ly) &&
            e.pageX < getPageX(ly) + getClipWidth(ly) &&
            e.pageY >= getPageY(ly) &&
            e.pageY < getPageY(ly) + getClipHeight(ly))
            {
            tb_setmode(ly,1);
            cn_activate(ly, 'Click');
            cn_activate(ly, 'MouseUp');
            }
        else
            {
            tb_setmode(ly,0);
            }
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tb_mouseover(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'tb' && ly.enabled)
        {
	if (ly.mode != 2) tb_setmode(ly,1);
        cn_activate(ly, 'MouseOver');
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tb_mouseout(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'tb' && ly.enabled)
        {
	if (ly.mode != 2) tb_setmode(ly,0);
        cn_activate(ly, 'MouseOut');
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tb_mousemove(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'tb' && ly.enabled)
        {
        cn_activate(ly, 'MouseMove');
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

