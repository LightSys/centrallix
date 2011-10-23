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

function gb_init(param)
    {
	var type = param.type;
	if(type=='image' || type=='textoverimage')
		{
	    var l = param.layer;
	    var w = param.width;
	    var h = param.height;
	    l.type = param.type;
	    //l.LSParent = param.parentobj;
	    l.nofocus = true;
	    if(type=='textoverimage')
		{
		var l2 = param.layer2;
		l2.nofocus = true;
		htr_init_layer(l2,l,'gb');
		}
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
	    htr_init_layer(l,l, 'gb');
	    //l.layer = l;
	    ifc_init_widget(l);
	    //l.img.layer = l;
	    //l.img.mainlayer = l;
	    //l.img.kind = 'gb';
	    l.cursrc = param.n;
	    setClipWidth(l, w);

	    l.buttonName = param.name;
	    if (h == -1) l.nImage = new Image();
	    else l.nImage = new Image(w,h);
	    pg_set(l.nImage,'src',param.n);
	    if (h == -1) l.pImage = new Image();
	    else l.pImage = new Image(w,h);
	    pg_set(l.pImage,'src',param.p);
	    if (h == -1) l.cImage = new Image();
	    else l.cImage = new Image(w,h);
	    pg_set(l.cImage,'src',param.c);
	    if (h == -1) l.dImage = new Image();
	    else l.dImage = new Image(w,h);
	    pg_set(l.dImage,'src',param.d);
	    l.enabled = param.enable;
	    if (cx__capabilities.Dom0IE)
	    	{
	    	l.attachEvent("onpropertychange",gb_setenable);
		}
	    else
	    	{
		l.watch("enabled",gb_setenable);
		}
	htutil_tag_images(l,'gb',l,l);
	}
	//end of image init
	else {
    var l = param.layer;
    var l2 = param.layer2;
    var l3 = param.layer3;
    var h = param.height;
    var w = param.width;
    //l.LSParent = param.parent;
    l.type = param.type;
    l.nofocus = true;
    l2.nofocus = true;
    
    /*iniitalize image array */
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

    
    htr_init_layer(l,l,'gb');
    htr_init_layer(l2,l,'gb');
    htr_init_layer(l3,l,'gb');
    ifc_init_widget(l);
    //l.img.layer = l
    //l.img.mainlayer = l
    //l.img.kind = 'gb';
    //l.cursrc = param.n;

    if(!cx__capabilities.Dom2CSS && !cx__capabilities.Dom0IE)
	{
	param.top.nofocus = true;
	param.right.nofocus = true;
	param.bottom.nofocus = true;
	param.left.nofocus = true;
	}
    l.buttonName = param.name;
    l.buttonText = param.text;

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
    /* set images */
    if (h == -1) l.nImage = new Image();
    else l.nImage = new Image(w,h);
    pg_set(l.nImage,'src',param.n);
    if (h == -1) l.pImage = new Image();
    else l.pImage = new Image(w,h);
    pg_set(l.pImage,'src',param.p);
    if (h == -1) l.cImage = new Image();
    else l.cImage = new Image(w,h);
    pg_set(l.cImage,'src',param.c);
    if (h == -1) l.dImage = new Image();
    else l.dImage = new Image(w,h);
    pg_set(l.dImage,'src',param.d);

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
    gb_setmode(l,0);
    if (htr_getvisibility(l3) == 'inherit' || htr_getvisibility(l3) == 'visible')
	l.enabled = false;
    else
	l.enabled = true;
	
    if (!cx__capabilities.Dom0IE)
        l.watch('enabled', gb_setenable);
    else
    	{
    	//alert("watch is not supported!");
    	l.onpropertychange = gb_propchange;
	}

    // Events
    htutil_tag_images(l,'gb',l,l);
    htutil_tag_images(l2,'gb',l,l);
    htutil_tag_images(l3,'gb',l,l);
	}
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("Click");
    ie.Add("MouseUp");
    ie.Add("MouseDown");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");

    }

function gb_propchange(prop, oldv, newv)
    {
    if (prop == 'enabled') gb_setenable(prop, oldv, newv);
    }

function gb_setenable(prop, oldv, newv)
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
    //status = 'gb_setenable -- ' + this.id + ' -- ' + prop + ', ' + oldv + ', ' + newv;
    if (typeof newv == "boolean")
    {
    	//alert("here--" + newv);
    if (newv == true)
	{
	// make enabled
	pg_set_style_string(this.l2,'visibility','inherit');
	pg_set_style_string(this.l3,'visibility','hidden');
	var layer = this.mainlayer;
	var newsrc = layer.nImage.src;
	if(type != 'text' && newsrc != layer.cursrc)
	    {
	    layer.cursrc = newsrc;
	    pg_set(layer.img, 'src', newsrc);
	    }
	}
    else
	{
	// make disabled
	pg_set_style_string(this.l2,'visibility','hidden');
	pg_set_style_string(this.l3,'visibility','inherit');
	var layer = this.mainlayer;
	var newsrc = layer.dImage.src;
	if(type != 'text' && newsrc != layer.cursrc)
	    {
	    layer.cursrc = newsrc;
	    pg_set(layer.img, 'src', newsrc);
	    }
	}
    }
    return newv;
    }

function gb_setmode(layer,mode)
    {
    var type = layer.type;
    if (layer.tristate == 0 && mode == 0) mode = 1;
    if (mode != layer.mode)
	{
	//status = layer.id + " " + mode;
	layer.mode = mode;
	//status = "tristate " + layer.id + " " + layer.tristate;
	switch(mode)
	    {
	    case 0: /* no point no click */
		var newsrc = layer.nImage.src;
		if(type != 'text' && newsrc != layer.cursrc)
		    {
		    layer.cursrc = newsrc;
		    pg_set(layer.img, 'src', newsrc);
		    if(type=='image' || type=='textoverimage') return;
		    }
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
		var newsrc = layer.pImage.src;
		//alert(mode==1);
		if(type != 'text' && newsrc != layer.cursrc)
		    {
		    layer.cursrc = newsrc;
		    pg_set(layer.img, 'src', newsrc);
		    if(type=='image' || type=='textoverimage' ) return;
		    }
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
		var newsrc = layer.cImage.src;
		if(type != 'text' && newsrc != layer.cursrc)
		    {
		    layer.cursrc = newsrc;
		    pg_set(layer.img, 'src', newsrc);
		    if(type=='image' || type=='textoverimage') return;
		    }
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

function gb_dblclick(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'gb' && cx__capabilities.Dom0IE)
	{
	gb_mousedown(e);
	gb_mouseup(e);
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function gb_mousedown(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'gb' && ly.enabled)
        {
        gb_setmode(ly,2);
	gb_current = ly;
	gb_cur_img = ly.mainlayer.img;
	gb_current.ifcProbe(ifEvent).Activate('MouseDown', {});
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function gb_mouseup(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if(ly.type=='image' || ly.type=='textoverimage' )
	{
	if (gb_cur_img){
	        if (e.pageX >= getPageX(gb_cur_img.layer) && e.pageX < getPageX(gb_cur_img.layer) + getClipWidth(gb_cur_img.layer) && e.pageY >= getPageY(gb_cur_img.layer) && e.pageY < getPageY(gb_cur_img.layer) + getClipHeight(gb_cur_img.layer))
	            {
		    ly.ifcProbe(ifEvent).Activate('Click', {});
		    ly.ifcProbe(ifEvent).Activate('MouseUp', {});
		    gb_setmode(ly, 1);
	            }
	        else
	            {
		    gb_setmode(ly, 0);
	            }
	        gb_cur_img = null;
	        }
	}
    else if (ly.kind == 'gb' && ly.enabled)
        {
        if (e.pageX >= getPageX(ly) &&
            e.pageX < getPageX(ly) + getClipWidth(ly) &&
            e.pageY >= getPageY(ly) &&
            e.pageY < getPageY(ly) + getClipHeight(ly))
            {
	    if (ly.mode == 2)
		{
		gb_setmode(ly,1);
		ly.ifcProbe(ifEvent).Activate('Click', {});
		ly.ifcProbe(ifEvent).Activate('MouseUp', {});
		}
            }
        else
            {
            gb_setmode(ly,0);
            }
        }
    if (gb_current && gb_current.mode == 2 && gb_current != ly)
	{
	gb_setmode(gb_current,0);
	}
    gb_current = null;
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function gb_mouseover(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'gb' && ly.enabled)
        {
	if (ly.mode != 2) gb_setmode(ly,1);
	ly.ifcProbe(ifEvent).Activate('MouseOver', {});
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function gb_mouseout(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'gb' && ly.enabled)
        {
	if (ly.mode != 2) gb_setmode(ly,0);
	ly.ifcProbe(ifEvent).Activate('MouseOut', {});
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function gb_mousemove(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'gb' && ly.enabled)
        {
	ly.ifcProbe(ifEvent).Activate('MouseMove', {});
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }


// Load indication
if (window.pg_scripts) pg_scripts['htdrv_button.js'] = true;
