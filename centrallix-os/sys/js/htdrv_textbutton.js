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
    //var l2 = param.layer2;
    //var l3 = param.layer3;
    //l.LSParent = param.parent;
    l.nofocus = true;
    //l2.nofocus = true;
    htr_init_layer(l,l,'tb');
    //htr_init_layer(l2,l,'tb');
    //htr_init_layer(l3,l,'tb');
    ifc_init_widget(l);
    if(!cx__capabilities.Dom2CSS && !cx__capabilities.Dom0IE)
	{
	param.top.nofocus = true;
	param.right.nofocus = true;
	param.bottom.nofocus = true;
	param.left.nofocus = true;
	}
    l.buttonName = param.name;
    l.buttonText = param.text;
    l.enab_c1 = param.c1;
    l.enab_c2 = param.c2;
    l.disab_c1 = param.dc1;
    //l.span = param.span;
    l.firstChild.mainlayer = l;
    htutil_tag_images(l, 'tb', l, l);

    //l.l2 = l2;
    //l.l3 = l3;
    l.tp = param.top;
    l.btm = param.bottom;
    l.lft = param.left;
    l.rgt = param.right;
    l.orig_x = getRelativeX(l);
    l.orig_y = getRelativeY(l);
    /*l.orig_ct = parseInt(getClipTop(l));
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
	}*/
    l.tristate = param.tristate;
    l.mode = -1;
    tb_setmode(l,0);
    /*if (htr_getvisibility(l3) == 'inherit' || htr_getvisibility(l3) == 'visible')
	l.enabled = false;
    else
	l.enabled = true;*/
    l.enabled = param.ena?true:false;

    l.tb_setenable = tb_setenable;
    if (!cx__capabilities.Dom0IE)
        htr_watch(l, 'enabled', 'tb_setenable');
    else
    	{
    	//alert("watch is not supported!");
    	l.onpropertychange = tb_propchange;
	}

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("Click");
    ie.Add("MouseUp");
    ie.Add("MouseDown");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");

    // Values
    var iv = l.ifcProbeAdd(ifValue);
    iv.Add("text", tb_cb_gettext, tb_cb_settext);
    iv.Add("enabled", "enabled");

    // Actions
    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("SetText", tb_action_settext);

    // Mobile Safari workaround
    $(l).find("span").on("click", function() {});
    }

function tb_action_settext(aparam)
    {
    $(this).find("span").text(aparam.Text);
    //this.firstChild.firstChild.firstChild.firstChild.firstChild.firstChild.firstChild.firstChild.data = aparam.Text;
    //this.l2.firstChild.firstChild.firstChild.firstChild.firstChild.firstChild.firstChild.firstChild.data = aparam.Text;
    //this.l3.firstChild.firstChild.firstChild.firstChild.firstChild.firstChild.firstChild.firstChild.data = aparam.Text;
    }

// used by ifValue
function tb_cb_gettext(attr)
    {
    return '';
    }

function tb_cb_settext(attr, val)
    {
    this.ifcProbe(ifAction).Invoke('SetText', {Text:val});
    return;
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
    //if (typeof newv == "boolean")
    //{
    	//alert("here--" + newv);
    if (newv)
	{
	// make enabled
	$(this).find("span").css({'color': this.enab_c1, 'text-shadow': '1px 1px ' + this.enab_c2});
	$(this).find("img").css({'opacity': '1.0'});
	//pg_set_style_string(this.span, 'color', this.enab_c1);
	//pg_set_style_string(this.span, 'text-shadow', '1px 1px ' + this.enab_c2);
	//pg_set_style_string(this.l2,'visibility','inherit');
	//pg_set_style_string(this.l3,'visibility','hidden');
	}
    else
	{
	// make disabled
	$(this).find("span").css({'color': this.disab_c1, 'text-shadow': ''});
	$(this).find("img").css({'opacity': '0.3'});
	//pg_set_style_string(this.span, 'color', this.disab_c1);
	//pg_set_style_string(this.span, 'text-shadow', '');
	//pg_set_style_string(this.l2,'visibility','hidden');
	//pg_set_style_string(this.l3,'visibility','inherit');
	}
    //}
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
		$(layer).find(".cell").css({'border-style':'solid', 'border-color':'transparent'});
		/*if(cx__capabilities.Dom2CSS)
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
		    setClip(layer, layer.orig_ct+1, layer.orig_cr-1, layer.orig_cb-1, layer.orig_cl+1);
		    }		    */
		break;

	    case 1: /* point, but no click */
		moveTo(layer,layer.orig_x,layer.orig_y);
		$(layer).find(".cell").css({'border-style':wgtrGetServerProperty(layer, 'border_style', 'outset'), 'border-color':wgtrGetServerProperty(layer, 'border_color', '#c0c0c0')});
		/*if(cx__capabilities.Dom2CSS)
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
		    if (layer.tristate) setClip(layer, layer.orig_ct, layer.orig_cr, layer.orig_cb, layer.orig_cl);
		    layer.style.borderTopColor = layer.lightBorderColor;
		    layer.style.borderLeftColor = layer.lightBorderColor;
		    layer.style.borderBottomColor = layer.darkBorderColor;
		    layer.style.borderRightColor = layer.darkBorderColor;
		    }*/
		break;

	    case 2: /* point and click */
		moveTo(layer,layer.orig_x+1,layer.orig_y+1);
		var bstyle = wgtrGetServerProperty(layer, 'border_style', 'outset');
		if (bstyle == 'outset')
		    bstyle = 'inset';
		$(layer).find(".cell").css({'border-style':bstyle, 'border-color':wgtrGetServerProperty(layer, 'border_color', '#c0c0c0')});
		/*if(cx__capabilities.Dom2CSS)
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
		    layer.style.borderColor = 'gray white white gray';
		    }*/
		break;
	    }
	}
    }

function tb_dblclick(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'tb' && cx__capabilities.Dom0IE)
	{
	tb_mousedown(e);
	tb_mouseup(e);
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tb_mousedown(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'tb' && ly.enabled)
        {
        tb_setmode(ly,2);
	tb_current = ly;
	tb_current.ifcProbe(ifEvent).Activate('MouseDown', {});
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tb_mouseup(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'tb' && ly.enabled)
        {
        if (e.pageX >= $(ly).offset().left &&
            e.pageX < $(ly).offset().left + $(ly).width() &&
            e.pageY >= $(ly).offset().top &&
            e.pageY < $(ly).offset().top + $(ly).height())
            {
	    if (ly.mode == 2)
		{
		tb_setmode(ly,1);
		ly.ifcProbe(ifEvent).Activate('Click', {});
		ly.ifcProbe(ifEvent).Activate('MouseUp', {});
		}
            }
        else
            {
            tb_setmode(ly,0);
            }
        }
    if (tb_current && tb_current.mode == 2 && tb_current != ly)
	{
	tb_setmode(tb_current,0);
	}
    tb_current = null;
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tb_mouseover(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'tb' && ly.enabled)
        {
	if (ly.mode != 2) tb_setmode(ly,1);
	ly.ifcProbe(ifEvent).Activate('MouseOver', {});
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tb_mouseout(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'tb' && ly.enabled)
        {
	if (ly.mode != 2) tb_setmode(ly,0);
	ly.ifcProbe(ifEvent).Activate('MouseOut', {});
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tb_mousemove(e)
    {
    var ly = e.layer;
    if (ly.mainlayer) ly = ly.mainlayer;
    if (ly.kind == 'tb' && ly.enabled)
        {
	ly.ifcProbe(ifEvent).Activate('MouseMove', {});
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }


// Load indication
if (window.pg_scripts) pg_scripts['htdrv_textbutton.js'] = true;
