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

// Form manipulation

function dd_getvalue() 
    {
    if (this.form.mode == 'Query' && this.query_multiselect)
	{
	var vals = new Array();
	for(var j in this.selectedItems)
	    vals.push(this.Values[this.selectedItems[j]].value);
	return vals;
	}
    if (typeof this.VisLayer.index == 'undefined' || this.VisLayer.index == null)
	return null;

    // GRB - maybe this should just be "return this.value" unconditionally.
    if (this.Values[this.VisLayer.index])
	return this.Values[this.VisLayer.index].value;
    else    
	return this.value;
    }

function dd_action_set_value(aparam)
    {
    this.setvalue(aparam.Value);
    dd_datachange(this);
    //this.ifcProbe(ifEvent).Activate('DataModify', {Value:this.Values[this.VisLayer.index].value});
    }

function dd_setvalue(v) 
    {
    //pg_debug('dd_setvalue: ' + v);
    //if (!this.PaneLayer) this.PaneLayer = dd_create_pane(this);
    //pg_debug(' ... ');
    for (var i in this.Values)
	{
	if (this.Values[i].value == v)
	    {
	    //pg_debug(' (' + this.Values[i].label + ')\n');
	    dd_select_item(this, i, 'osrc');
	    return true;
	    }
	}
    //pg_debug(' (none)\n');
    //return false;

    // allow setting the value when dropdown doesn't contain it, cuz it might later.
    this.value = v;
    this.label = null;
    htr_setvisibility(this.VisLayer, 'hidden');
    return true;
    }

function dd_clearvalue()
    {
    dd_select_item(this, null, 'osrc');
    }

function dd_resetvalue()
    {
    this.clearvalue();
    }

function dd_enable()
    {
    pg_images(this)[0].src = '/sys/images/ico15b.gif';
    htr_setbgcolor(this, this.bg);
    this.keyhandler = dd_keyhandler;
    this.enabled = 'full';
    }

function dd_readonly()
    {
    pg_images(this)[0].src = '/sys/images/ico15b.gif';
    htr_setbgcolor(this, "#e0e0e0");
    this.keyhandler = null;
    this.enabled = 'readonly';
    }

function dd_disable()
    {
    if (dd_current)
	{
	htr_setvisibility(dd_current.PaneLayer, 'hidden');
	dd_current = null;
	}
    pg_images(this)[0].src = '/sys/images/ico15a.gif';
    htr_setbgcolor(this, "#e0e0e0");
    this.keyhandler = null;
    this.enabled = 'disabled';
    }

// Normal functions

function dd_keyhandler(l,e,k)
    {
    //if (!dd_current) return;
    var dd = this.mainlayer;
    if (dd.enabled != 'full') return 1;
    if ((k >= 65 && k <= 90) || (k >= 97 && k <= 122) || (k >= 48 && k <= 57))
	{
	if (!dd.PaneLayer || htr_getvisibility(dd.PaneLayer) != 'inherit')
	    dd_expand(dd);
	if (k < 97 && k > 57) 
	    {
	    var k_lower = k + 32;
	    var k_upper = k;
	    k = k + 32;
	    }
	else if (k > 57)
	    {
	    var k_lower = k;
	    var k_upper = k - 32;
	    }
	else
	    {
	    var k_lower = k;
	    var k_upper = k;
	    }

	this.time_stop = new Date();
	if (this.time_start && this.time_start < this.time_stop 
		&& (this.time_stop - this.time_start) >= 1000) 
	    {
	    this.keystring = null;
	    this.match = null;
	    this.lastmatch = null;
	    this.time_start = null;
	    this.time_stop = null;
	    }
		
	if (!this.keystring)
	    {
	    for (var i=0; i < this.Values.length; i++)
		{
		if (this.Values[i].label == '(none selected)' || this.Values[i].hide) continue;
		if (this.Values[i].label.substring(0, 1) == 
			String.fromCharCode(k_upper) ||
		    this.Values[i].label.substring(0, 1) == 
			String.fromCharCode(k_lower))
		    {
		    dd_hilight_item(this,i);
			this.lastmatch = i;
		    i=this.Values.length;
		    }
		}
	    if (!this.lastmatch)
		{
		for (var i=0; i < this.Values.length; i++)
		    {
		    if (this.Values[i].label == '(none selected)' || this.Values[i].hide) continue;
		    if (this.Values[i].label.toUpperCase().indexOf(String.fromCharCode(k_upper)) >= 0)
			{
			dd_hilight_item(this,i);
			    this.lastmatch = i;
			i=this.Values.length;
			break;
			}
		    }
		}

	    this.keystring = String.fromCharCode(k);
	    this.time_start = new Date();

	    if (!this.lastmatch) 
		{
		this.keystring = null;
		this.match = null;
		this.lastmatch = null;
		this.time_start = null;
		this.time_stop = null;
		dd_hilight_item(this, 0);
		}
	    }
	
	//find as you type code
	else
	    {
	    this.keystring += String.fromCharCode(k);
	    this.match = false;
	    this.time_start = new Date();
			
	    for (var i=0; i < this.Values.length; i++) 
		{
		if (this.Values[i].label == '(none selected)' || this.Values[i].hide) continue;
		if ((this.Values[i].label.substring(0, 
			this.keystring.length).toLowerCase())
			== this.keystring && !this.match)
		    {
		    dd_hilight_item(this,i);
		    //found a good match
		    this.match = true;
		    this.lastmatch = i;
		    }		
		}
	    if (!this.match)
		{
		for (var i=0; i < this.Values.length; i++)
		    {
		    if (this.Values[i].label == '(none selected)' || this.Values[i].hide) continue;
		    if (this.Values[i].label.toUpperCase().indexOf(this.keystring.toUpperCase()) >= 0)
			{
			dd_hilight_item(this,i);
			this.match = true;
			this.lastmatch = i;
			break;
			}
		    }
		}
	    if (!this.match) 
		{
		this.keystring = this.keystring.substring(0, 
			(this.keystring.length - 1));
		dd_hilight_item(this, this.lastmatch);
	    	this.match = true;
		}
	    }
	/* CODE TO enable cycling through letter by repeatedly pressing 
	the same button: such as pressing 't' and cycling through tommy, 
	tranquilizer, tiger, and tivo contained in a dropdown list
	
	disabled to enable typing more of a word in the list to narrow it 
	down to the correct  word */
	/*else
	    {
	    var first = -1;
	    var last = -1;
	    var next = -1;
	    for (var i=0; i < this.Values.length; i++)
		{
		if (this.Values[i].label.substring(0, 1) == 
			String.fromCharCode(k_upper) ||
		    this.Values[i].label.substring(0, 1) == 
			String.fromCharCode(k_lower))
		    {
		    if (first < 0) { first = i; last = i; }
		    for (var j=i; j < this.Values.length && 
			(this.Values[j].label.substring(0, 1) == 
				String.fromCharCode(k_upper) ||
			 this.Values[j].label.substring(0, 1) == 
				String.fromCharCode(k_lower)); j++)
			{
			if (this.Items[j] == this.Items[this.SelectedItem])
			    next = j + 1;
			last = j;
			}
		    if (next <= last)
			dd_hilight_item(this,next);
		    else
			dd_hilight_item(this,first);
		    i=this.Values.length;
		    }
		}
	    }*/
	}
    else if (k == 32)
	{
	dd_expand(this);
	}
    else if (k == 13)
	{
	if (!this.PaneLayer || htr_getvisibility(this.PaneLayer) != 'inherit')
	    {
	    if (this.form) this.form.RetNotify(this);
	    }
	else
	    {
	    dd_select_item(this,this.SelectedItem, 'keyboard');
	    dd_datachange(this);
	    dd_collapse(this);
	    dd_unhilight_item(this,this.SelectedItem);
	    }
	}
    else if (k == 9)
	{
	if (this.form)
	    {
	    if (this.PaneLayer && htr_getvisibility(this.PaneLayer) == 'inherit' && 
		    (this.SelectedItem !== null))
		{
		dd_select_item(this,this.SelectedItem, 'keyboard');
		dd_datachange(this);
		dd_collapse(this);
		dd_unhilight_item(this,this.SelectedItem);
		}
	    if (e.shiftKey)
		this.form.ShiftTabNotify(this);
	    else
		this.form.TabNotify(this);
	    }
	}
    else if (k == 27)
	{
	if (this.PaneLayer && htr_getvisibility(this.PaneLayer) == 'inherit') 
	    {
	    dd_collapse(this);
	    }
	else
	    if (dd.form) dd.form.EscNotify(dd);
	}
    else if (e.keyName == 'down')
	{
	if (!this.PaneLayer || htr_getvisibility(this.PaneLayer) == 'hidden')
	    dd_expand(this);
	for(var i=0;i<this.Values.length;i++)
	    if ((this.SelectedItem == null || i > this.SelectedItem) && !this.Values[i].hide)
		{
		dd_hilight_item(this, i);
		break;
		}
	}
    else if (e.keyName == 'up')
	{
	if (!this.PaneLayer || htr_getvisibility(this.PaneLayer) == 'hidden')
	    dd_expand(this);
	for(var i=this.Values.length-1;i>=0;i--)
	    if ((this.SelectedItem == null || i < this.SelectedItem) && !this.Values[i].hide)
		{
		dd_hilight_item(this, i);
		break;
		}
	}
    dd_lastkey = k;
    return true;
    }

function dd_notmember(val,list)
    {
    for(var i in list)
	{
	if(val==list[i]) return false;
	}
    return true;
    }

function dd_hilight_item(l,i)
    {
    if (i == null)
	{
	if (l.Values && l.Values[0] && l.Values[0].value == null)
	    i = 0;
	else
	    return;
	}
    if (l.SelectedItem != null && dd_notmember(l.SelectedItem, l.selectedItems))
	dd_unhilight_item(l,l.SelectedItem);
    l.SelectedItem = i;
    if (l.Items[i])
	{
	htr_setbgcolor(l.Items[i], l.hl);
	dd_scroll_to(l,i);
	}
    }

function dd_unhilight_item(l,i)
    {
    if (i == null)
	{
	if (l.Values[0].value == null)
	    i = 0;
	else
	    return;
	}
    l.SelectedItem = null;
    if (l.Items[i])
	htr_setbgcolor(l.Items[i], l.bg);
    }

function dd_collapse(l)
    {
	if(!l) return; //assume already collapsed
	l.keystring = null;
	l.match = null;
	l.lastmatch = null;
	l.time_start = null;
	l.time_stop = null;
    if (l && l.PaneLayer && htr_getvisibility(l.PaneLayer) == 'inherit')
	{
	//setClipHeight(l, getClipHeight(l) - getClipHeight(l.PaneLayer));
	//pg_resize_area(l.area,getClipWidth(l)+1,getClipHeight(l)+1, -1, -1);
	htr_setvisibility(l.PaneLayer, 'hidden');
	dd_current = null;
	}
    }

function dd_expand(l)
    {
    var offs;
    if (!l.osrc && !cx_hints_teststyle(l.mainlayer, cx_hints_style.notnull) && l.Values[0] && l.Values[0].value != null)
	{
	var nullitem = new Array();
	if (l.mainlayer.w < 108)
	    nullitem.label = '(none)';
	else
	    nullitem.label = '(none selected)';
	nullitem.value = null;
	l.Values.splice(0,0,nullitem);
	}
    if (l && !l.PaneLayer) 
	l.PaneLayer = dd_create_pane(l);
    if (l && htr_getvisibility(l.PaneLayer) != 'inherit')
	{
	pg_stackpopup(l.PaneLayer, l);
	pg_positionpopup(l.PaneLayer, $(l).offset().left, $(l).offset().top, l.h, 
		getClipWidth(l));
	htr_setvisibility(l.PaneLayer, 'inherit');
	dd_current = l;
	if (getPageY(l.PaneLayer) < getPageY(l))
	    offs = getPageY(l.PaneLayer) - getPageY(l) - 1;
	else
	    offs = getPageY(l.PaneLayer) - getPageY(l) - 1 - getClipHeight(l);
	//setClipHeight(l, getClipHeight(l) + getClipHeight(l.PaneLayer));
	//pg_resize_area(l.area, getClipWidth(l)+1, 
	//	getClipHeight(l)+1+getClipHeight(l.PaneLayer),
	//    getPageX(l.PaneLayer) - getPageX(l) - 1,
	//    offs);
	for(var i = 0; i<l.Values.length; i++)
	    if (l.value == l.Values[i].value)
		{
		l.VisLayer.index = i;
		break;
		}
	dd_hilight_item(l,l.VisLayer.index);
	}
    }

function dd_contextmenu(e){
    //if inside widget
    if(e.kind == 'dd_itm')
	{
	return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	}
    else
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
}

function dd_select_item(l,i,from)
    {
    /*if (l.Values && l.Values[i] && l.Values[i].label)
	pg_debug('dd_select_item: ' + i + ' (' + l.Values[i].label + ')\n');
    else
	pg_debug('dd_select_item: ' + i + ' (?)\n');*/
    var c = "<TABLE height=" + (pg_parah) + " cellpadding=1 cellspacing=0 border=0><TR><TD valign=middle nowrap>";
    if (i!=null)
	{
	if(l.form && l.form.mode == 'Query' && l.query_multiselect)
	    {
	    if(!l.selectedItems)
		{
		l.selectedItems = new Array();
		l.selectedItems[0]=i;
		}
	    else
		{
		for(var k=0;k<l.selectedItems.length;k++)
		    if(l.selectedItems[k]==i)
			break;
		if(k == l.selectedItems.length)
		    l.selectedItems.push(i);
		}
	    /*if (!(i==0 && l.Values[i].value==null)) 
		c += l.Values[i].label;
	    else
		c += '<i>' + l.Values[i].label + '</i>';*/
	    var firstone = true;
	    var items = '';
	    for(var j in l.selectedItems)
		{
		if(!firstone)
		    items += ', ';
		firstone = false;
		if( !(l.selectedItems[j]==0 && l.Values[l.selectedItems[j]].value==null))
		    {
		    items += l.Values[l.selectedItems[j]].label; //this should be the only value if they pick none
		    }
		else
		    {
		    items = '<i>' + l.Values[l.selectedItems[j]].label + '</i>';
		    for(var k in l.Items)
			{
			htr_setbgcolor(l.Items[k],l.bg);
			}
		    l.selectedItems = null;
		    break;
		    }
		}
	    c+= items;
	    }
	else
	    {
	    if( !(i==0 && l.Values[i].value==null))
		c += htutil_encode(htutil_obscure(l.Values[i].label));
	    else
		c += '<i>' + htutil_encode(htutil_obscure(l.Values[i].label)) + '</i>';
	    }
	}
    c += "</TD></TR></TABLE>";
    htr_write_content(l.HidLayer, c);
    //pg_serialized_write(l.HidLayer, c, null);
    l.HidLayer.index = i;
    moveTo(l.HidLayer, 2, ((l.h-2) - pg_parah)/2);
    resizeTo(l.HidLayer, l.w, l.h);
    
    htr_setvisibility(l.HidLayer, 'inherit');
    setClipWidth(l.HidLayer, l.w-21);
    htr_setvisibility(l.VisLayer, 'hidden');
    var t=l.VisLayer;
    l.VisLayer = l.HidLayer;
    l.HidLayer = t;
    //pg_debug('new id = ' + l.VisLayer.id + '\n');
    if (i != null)
	{
	l.value = l.Values[l.VisLayer.index].value;
	l.label = l.Values[l.VisLayer.index].label;
	}
    else
	{
	l.value = null;
	l.label = null;
	}
    if(l.Mode == 3)
	{
	//change record
	//alert(i);
	//l.osrc.MoveToRecord(i);
	}
    if (from != 'init')
	cn_activate(l, "DataChange", {Value:l.value, Label:l.label, FromOSRC:(from == 'osrc')});
    }

function dd_datachange(l)
    {
    if (l.form) l.form.DataNotify(l);
    l.ifcProbe(ifEvent).Activate('DataModify', {Value:(l.Values.length && l.VisLayer.index !== null && l.Values[l.VisLayer.index])?(l.Values[l.VisLayer.index].value):null});
    }

function dd_getfocus()
    {
    if (this.enabled != 'full') return 0;
    if(this.form)
	{
	if (!this.form.FocusNotify(this)) return 0;
	}
    //dd_expand(this);
    cn_activate(this, "GetFocus");
    return 1;
    }

function dd_losefocus()
    {
    cn_activate(this, "LoseFocus");
    //dd_collapse(this); // this now done in mousedown.
    return true;
    }

function dd_toggle(l, dn) 
    {
    var imgs = pg_images(l);
    for (var i=0; i<imgs.length;i++) 
	{
	//if (i == 4)
	//    continue;
	if ((imgs[i].downimg && dn) || (imgs[i].upimg && !dn))
	    imgs[i].src = '/sys/images/white_1x1.png';
	else if ((imgs[i].upimg && dn) || (imgs[i].downimg && !dn))
	    imgs[i].src = '/sys/images/dkgrey_1x1.png';
	}
    }

function dd_scroll_to(l, n)
    {
    var top = getClipTop(dd_current.PaneLayer.ScrLayer);
    var btm=top+(getClipHeight(dd_current.PaneLayer)-4);
    var il=l.Items[n];
    if (!il) return;

    if (getRelativeY(il)>=top && getRelativeY(il)+(pg_parah)<=btm) //none
	return;
    else if (getRelativeY(il)<top) //up
	{
	var imgs = pg_images(l.PaneLayer.BarLayer);
	dd_target_img = imgs[0];
	dd_incr = (top-getRelativeY(il));
	}
    else //down
	{
	var imgs = pg_images(l.PaneLayer.BarLayer);
	dd_target_img = imgs[2];
	dd_incr = (top-getRelativeY(il)+((pg_parah)*(dd_current.NumDisplay-1)));
	}
    dd_scroll();
    }

function dd_scroll_tm()
    {
    dd_scroll();
    dd_timeout=setTimeout(dd_scroll_tm,50);
    return false;
    }

function dd_scroll(t)
    {
    var ti = dd_target_img;
    var px = dd_incr;
    if (!dd_current) return;
    var ly = dd_current.PaneLayer.ScrLayer;
    var ht1 = getRelativeY(ly) - 2;
    var h = dd_current.PaneLayer.h;
    var ht2 = h - dd_current.h2 + ht1;
    var d = h - getClipHeight(dd_current.PaneLayer) + 4;
    var v = getClipHeight(dd_current.PaneLayer) - (3*18) - 4;
    if (px > -ht1) px = -ht1;
    if (px < -ht2) px = -ht2;
    if ((px<0 && ht2>0) || (px>0 && ht1<0)) // up or down
	{
	moveBy(ly, 0, px);
	setClipTop(ly, getClipTop(ly) - px);
	setClipHeight(ly, dd_current.h2);
	if (t==null)
	    {
	    if (d<=0) 
		setRelativeY(ti.thum,18);
	    else 
		setRelativeY(ti.thum,20+(-v*((getRelativeY(ly)-2)/d)));
	    }
	}
    }
 
function dd_create_pane(l)
    {
    // First, check to see if we need a NULL entry
    //if (!cx_hints_teststyle(l.mainlayer, cx_hints_style.notnull))
//	{
//	var nullitem = new Array();
//	if (l.mainlayer.w < 108)
//	    nullitem.label = '(none)';
//	else
//	    nullitem.label = '(none selected)';
//	nullitem.value = null;
//	l.Values.splice(0,0,nullitem);
//	}

    // Create the layer
    var cnt = 0;
    for (var i=0; i < l.Values.length; i++)
	{
	if (!l.Values[i].hide)
	    {
	    cnt++;
	    }
	}
    l.NumElements = cnt;
    l.h2 = ((l.NumDisplay<l.NumElements?l.NumDisplay:l.NumElements)*(pg_parah))+4;
    var p = htr_new_layer(null,pg_toplevel_layer(l));
    //pg_debug(' x ');
    htr_init_layer(p, l, 'dd_pn');
    htr_setvisibility(p, 'hidden');
    var c = "<BODY bgcolor="+l.bg+">";
    c += "<TABLE border=0 cellpadding=0 cellspacing=0 width="+l.popup_width+" height="+l.h2+">";
    c += "<TR><TD><IMG SRC=/sys/images/white_1x1.png height=1></TD>";
    c += "  <TD><IMG SRC=/sys/images/white_1x1.png height=1 width="+(l.popup_width-2)+"></TD>";
    c += "  <TD><IMG SRC=/sys/images/white_1x1.png height=1></TD></TR>";
    c += "<TR><TD><IMG SRC=/sys/images/white_1x1.png height="+(l.h2-2)+" width=1></TD>";
    c += "  <TD valign=top>";
    c += "  </TD>";
    c += "  <TD><IMG SRC=/sys/images/dkgrey_1x1.png height="+(l.h2-2)+" width=1></TD></TR>";
    c += "<TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD>";
    c += "  <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width="+(l.popup_width-2)+"></TD>";
    c += "  <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD></TR>";
    c += "</TABLE>";
    c += "</BODY>";
    htr_setbgcolor(p, l.bg);
    htr_write_content(p, c);
    //pg_serialized_write(p, c, null);
    htutil_tag_images(p,'dt_pn',p,l);
    pg_stackpopup(p,l);
    setClipHeight(p, l.h2);
    setClipWidth(p, l.popup_width);

    /**  Create scroll background layer  **/
    p.ScrLayer = htr_new_layer(null, p);
    htr_init_layer(p.ScrLayer, p, "dd_sc");
    moveTo(p.ScrLayer, 2, 2);
    setClipHeight(p.ScrLayer, l.h2);
    if (l.NumDisplay < l.NumElements)
	{
	/**  If we need a scrollbar, put one in  **/
	setClipWidth(p.ScrLayer, getClipWidth(p) - 22);

	p.BarLayer = htr_new_layer(null, p)
	htr_init_layer(p.BarLayer, l, 'dd_sc');
	moveTo(p.BarLayer, l.popup_width-20, 2);
	htr_setvisibility(p.BarLayer, 'inherit');
	c = '<TABLE border=0 cellpadding=0 cellspacing=0 width=18 height='+(l.h2-4)+'>';
	c += '<TR><TD><IMG name=u src=/sys/images/ico13b.gif></TD></TR>';
	c += '<TR><TD><IMG name=b src=/sys/images/trans_1.gif height='+(l.h2-40)+'></TD></TR>';
	c += '<TR><TD><IMG name=d src=/sys/images/ico12b.gif></TD></TR>';
	c += '</TABLE>';
	htr_write_content(p.BarLayer, c);
	//pg_serialized_write(p.BarLayer, c, null);
	var imgs = pg_images(p.BarLayer);
	imgs[0].mainlayer = imgs[1].mainlayer = imgs[2].mainlayer = l;
	imgs[0].kind = imgs[1].kind = imgs[2].kind = 'dd_sc';
	l.imgup = imgs[0];
	l.imgdn = imgs[2];

	p.TmbLayer = htr_new_layer(null, p);
	imgs[0].thum = imgs[1].thum = imgs[2].thum = p.TmbLayer;
	moveTo(p.TmbLayer, l.popup_width-20, 20);
	htr_setvisibility(p.TmbLayer, 'inherit');
	p.TmbLayer.mainlayer = l;
	htr_write_content(p.TmbLayer,'<IMG src=/sys/images/ico14b.gif NAME=t draggable="false">');
	//pg_serialized_write(p.TmbLayer,'<IMG src=/sys/images/ico14b.gif NAME=t>', null);
	imgs = pg_images(p.TmbLayer);
	imgs[0].mainlayer = l;
	imgs[0].thum = p.TmbLayer;
	imgs[0].kind = 'dd_sc';
	l.imgtm = imgs[0];
	}
    else
	{
	/**  If no scrollbar is needed, don't use one!  **/
	setClipWidth(p.ScrLayer, getClipWidth(p) - 4);
	}
    setClipHeight(p.ScrLayer, getClipHeight(p) - 4);
    htr_setvisibility(p.ScrLayer, 'inherit');

    /**  Add items  **/
    var w = getClipWidth(p.ScrLayer);
    l.Items = [];
    var cnt = 0;
    for (var i=0; i < l.Values.length; i++)
	{
	if (!l.Values[i].hide)
	    {
	    if (!l.Items[i])
		{
		l.Items[i] = htr_new_layer(w, p.ScrLayer);
		htr_init_layer(l.Items[i], l, 'dd_itm');
		}
	    moveTo(l.Items[i], 1, cnt*(pg_parah));
	    setClipWidth(l.Items[i], w);
	    setClipHeight(l.Items[i], (pg_parah));
	    resizeTo(l.Items[i], w, (pg_parah));
	    if (i==0 && l.Values[i].value == null)
		htr_write_content(l.Items[i], '<i>' + htutil_encode(l.Values[i].label) + '</i>');
		//pg_serialized_write(l.Items[i], '<i>' + l.Values[i].label + '</i>',null);
	    else
		htr_write_content(l.Items[i], htutil_encode(l.Values[i].label));
		//pg_serialized_write(l.Items[i], l.Values[i].label, null);
	    htr_setvisibility(l.Items[i], 'inherit');
	    l.Items[i].index = i;

	    cnt++;
	    }
	else if (l.Items[i])
	    {
	    htr_setvisibility(l.Items[i], 'hidden');
	    }
	}

    p.h = l.NumElements*(pg_parah);
    p.mainlayer = l;

    return p;
    }


/// REPLACE ITEMS IN DROPDOWN
function dd_add_items(l,ary)
    {
    var sel = null;
    var vsel = null;
    l.Values = [];
    l.allValues = null;
    for(var i in ary) 
	{
	ary[i].label = htutil_rtrim(ary[i].label);
	ary[i].value = htutil_rtrim(ary[i].value);
	ary[i].font = htutil_rtrim(ary[i].font);
	ary[i].fontsize = htutil_rtrim(ary[i].fontsize);
	if (ary[i].wname)
	    {
	    ary[i].wname = htutil_rtrim(ary[i].wname);
	    }
	if (ary[i].sel) sel = i;
	if (l.value != null && ary[i].value == l.value) vsel = i;
	}
    l.Values = ary;
    if (sel != null)
	{
	if (!l.form)
	    dd_select_item(l, sel, l.init_items?'additems':'init');
	if (typeof ary[sel].value == 'number')
	    cx_set_hints(this, "d=" + ary[sel].value, "widget");
	else
	    cx_set_hints(this, "d=" + escape(cxjs_quote(ary[sel].value)), "widget");
	}
    else
	{
	cx_set_hints(this, "", "widget");
	}
    if (ary.length == 0 && l.value != null)
	{
	// temporarily hide it, no values matching the dd value.
	htr_setvisibility(l.VisLayer, 'hidden');
	//dd_select_item(l, null);
	}
    else
	{
	var found = false;
	for (var i in l.Values)
	    {
	    if (l.Values[i].value == l.value)
		{
		found = true;
		dd_select_item(l, i, l.init_items?'additems':'init');
		break;
		}
	    }
	//if (!found && this.invalid_select_default && this.value)
	if (!found && this.invalid_select_default && (!this.form || this.form.mode == 'Modify'))
	    cx_hints_setdefault(this);
	}
    l.init_items = true;
    }

// Event scripts
function dd_mouseout(e)
    {
    var ti=dd_target_img;
    if (ti && ti.name == 't' && dd_current)
        return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
    }

function dd_mousemove(e)
    {
    var ti=dd_target_img;
    if (ti != null && ti.name == 't' && dd_current && dd_current.enabled!='disabled')
        {
        var pl=ti.mainlayer.PaneLayer;
        var v=getClipHeight(pl)-(3*18)-4;
        var new_y=dd_thum_y+(e.pageY-dd_click_y)
	var pl_y = getPageY(pl);
        if (new_y > pl_y+20+v) new_y=pl_y+20+v;
        if (new_y < pl_y+20) new_y=pl_y+20;
        setPageY(ti.thum,new_y);
        var h=dd_current.PaneLayer.h;
        var d=h-getClipHeight(pl)+4;
        if (d<0) d=0;
        dd_incr = (((getRelativeY(ti.thum)-22)/(v-4))*-d)-getRelativeY(dd_current.PaneLayer.ScrLayer);
        dd_scroll(0);
        return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    if (e.mainlayer && e.mainkind == 'dd')
        {
        cn_activate(e.mainlayer, 'MouseMove');
        dd_cur_mainlayer = null;
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function dd_mouseover(e)
    {
    if (e.mainlayer && e.mainkind == 'dd')
        {
        cn_activate(e.mainlayer, 'MouseOver');
        dd_cur_mainlayer = e.mainlayer;
        }
    if (e.kind == 'dd_itm' && dd_current && dd_current.enabled=='full')
        {
        dd_lastkey = null;
        dd_hilight_item(dd_current, e.layer.index);
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function dd_mouseup(e)
    {
    if (e.mainlayer && e.mainkind == 'dd')
        {
        cn_activate(e.mainlayer, 'MouseUp');
	}
    if (dd_timeout != null)
        {
        clearTimeout(dd_timeout);
        dd_timeout = null;
        dd_incr = 0;
        }
    if (dd_target_img != null)
        {
        if (dd_target_img.kind && dd_target_img.kind.substr(0,2) == 'dd' && (dd_target_img.name == 'u' || dd_target_img.name == 'd'))
            pg_set(dd_target_img,'src',htutil_subst_last(dd_target_img.src,"b.gif"));
        dd_target_img = null;
        }
    if ((e.kind == 'dd' || e.kind == 'ddtxt') && e.mainlayer.enabled != 'disabled')
        {
        dd_toggle(e.mainlayer,false);
        }
    if (e.which == 2 || e.which == 3) return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function dd_mousedown(e)
    {
    if (e.mainlayer && e.mainkind == 'dd') cn_activate(e.mainlayer, 'MouseDown');
    dd_target_img = e.target;
    if (e.kind == 'dd_itm' && dd_current && dd_current.enabled == 'full')
        {
	var cur = dd_current;
	if(e.which == 2 || e.which == 3)
	    {
	    /*	FIXME
		In NS4 after you have right clicked the widget will not let you select a new item until after
		you have left clicked (which collapses the menu)
		A related issue is that after you right click, the next click outside the widget will be trapped.
	    */
	    e.mainlayer.ifcProbe(ifEvent).Activate('RightClick',{Label:e.mainlayer.Values[e.layer.index].label,Value:e.mainlayer.Values[e.layer.index].value,X:e.pageX, Y:e.pageY});
            dd_select_item(dd_current, e.layer.index, 'mouse');
            dd_datachange(dd_current);
	    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	    }
	if(e.mainlayer.Mode == 3)
	    {
	    // OSRC-selector dropdown
	    if(e.mainlayer.Values[e.layer.index].osrcindex)
		e.mainlayer.osrc.MoveToRecord(e.mainlayer.Values[e.layer.index].osrcindex); 
	    dd_collapse(dd_current);
	    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
	    }
	else
	    {
	    dd_select_item(dd_current, e.layer.index, 'mouse');
	    dd_datachange(dd_current);
	    }
	if(!dd_current.form || dd_current.form.mode != 'Query' || !dd_current.query_multiselect)
	    dd_collapse(dd_current);
	else 
	    if (dd_current.Items[e.layer.index])
		htr_setbgcolor(dd_current.Items[e.layer.index], dd_current.hl);
	    //dd_hilight_item(dd_current, e.layer.index);
	// Re-select the dropdown
	pg_setkbdfocus(cur, null, null, null);
        }
    else if (e.kind == 'dd_sc')
        {
        switch(e.layer.name)
            {
            case 'u':
                pg_set(e.layer,'src','/sys/images/ico13c.gif');
                dd_incr = 8;
                dd_scroll();
                dd_timeout = setTimeout(dd_scroll_tm,300);
                break;
            case 'd':
                pg_set(e.layer, 'src', '/sys/images/ico12c.gif');
                dd_incr = -8;
                dd_scroll();
                dd_timeout = setTimeout(dd_scroll_tm,300);
                break;
            case 'b':
                dd_incr = dd_target_img.height+36;
                if (e.pageY > getPageY(dd_target_img.thum)+9) dd_incr = -dd_incr;
                dd_scroll();
                dd_timeout = setTimeout(dd_scroll_tm,300);
                break;
            case 't':
                dd_click_x = e.pageX;
                dd_click_y = e.pageY;
                dd_thum_y = getPageY(dd_target_img.thum);
                break;
            }
        }
    else if ((e.kind == 'dd' || e.kind == 'ddtxt') && e.mainlayer.enabled != 'disabled')
        {
        if (dd_current)
            {
            dd_toggle(dd_current,true);
            dd_collapse(dd_current);
            }
        else
            {
            dd_toggle(e.mainlayer,true);
            dd_expand(e.mainlayer);
            if(e.mainlayer.form)
                e.mainlayer.form.FocusNotify(e.layer);
            }
        }
    if (dd_current && dd_current != e.layer && dd_current.PaneLayer != e.layer && (!e.mainlayer || dd_current != e.mainlayer))
        {
        dd_collapse(dd_current);
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function dd_update(p1)
    {
    var osrc = this.osrc;
    var l = this.mainlayer;
    var targetval;
    //make array of labels and values wnames are irrelevant because there are no corresponding widgets
    var vals = new Array();
    for(var i in osrc.replica)
	{
	var rec = osrc.replica[i];
	for(var k in rec)
	    {
	    if(rec[k].oid == l.fieldname)
		{
		if(i==osrc.CurrentRecord) targetval = vals.length;
		vals[vals.length] = new Object({osrcindex: parseInt(i), label: rec[k].value, value: rec[k].value}); //append object
		break; //don't do duplicates
		}
	    }
	}
    l.additems(l,vals);
    dd_select_item(l,targetval, 'osrc'); //select the correct index
    dd_collapse(this);
    l.PaneLayer = null;
    }

function dd_action_set_group(aparam)
    {
    var oldval = this.value;
    if (!this.allValues) this.allValues = this.Values;
    this.currentGroup = aparam.Group;
    this.currentMin = aparam.Min;
    this.currentMax = aparam.Max;
    dd_collapse(this);
    this.PaneLayer = null;
    this.Values = [];

    // Only show entries matching the specified group
    var new_select = null;
    for(var i=0; i<this.allValues.length; i++)
	{
	if (!this.currentGroup || !this.allValues[i].grp || this.currentGroup == this.allValues[i].grp)
	    {
	    if (typeof aparam.Min == 'undefined' || aparam.Min === null || aparam.Min <= this.allValues[i].value)
		{
		if (typeof aparam.Max == 'undefined' || aparam.Max === null || aparam.Max >= this.allValues[i].value)
		    {
		    this.Values.push({label:this.allValues[i].label, value:this.allValues[i].value});
		    if (this.allValues[i].value == oldval)
			new_select = this.Values.length - 1;
		    }
		}
	    }
	}
    if (new_select !== null)
	dd_select_item(this, new_select, 'grp');
    }

function dd_clear_layers()
    {
    var vals = new Array();
    var l = this.mainlayer;
    l.additems(l,vals);
    }

function dd_changemode()
    {
    if(this.form.mode == "Query")
	{
	this.selectedItems = null;
	}
    else if (this.form.mode == "View" || this.form.mode == "NoData")
	{
	//unselect items
	this.selectedItems = null;
	for(var k in this.Items)
	    {
	    htr_setbgcolor(this.Items[k],this.bg);
	    }
	}
    }

function dd_sql_loaded()
    {
    var dd = this.dd;
    var rows = htr_parselinks(pg_links(this));
    var items = [];

    // For Positional, the params are: label, value, selected, group, hidden.
    for(var r in rows)
	{
	var row = rows[r];
	var item = {};
	for(var c in row)
	    {
	    var col = row[c];
	    switch(c)
		{
		case 'label': item.label = col.value; break;
		case 'value': item.value = col.value; break;
		case 'selected': item.sel = col.value; break;
		case 'grp': item.grp = col.value; break;
		case 'hidden': item.hide = col.value; break;
		}
	    }
	items.push(item);
	}
    dd.additems(dd,items);
    dd_collapse(dd);
    dd.PaneLayer = null;
    dd.ifcProbe(ifAction).Invoke("SetGroup", {Group:dd.currentGroup, Min:dd.currentMin, Max:dd.currentMax});
    }

function dd_action_set_items(aparam)
    {
    if (aparam.SQL)
	{
	if (!this.sql_loader)
	    {
	    this.sql_loader = htr_new_loader(this);
	    this.sql_loader.dd = this;
	    this.sql_loader.dd_sql_loaded = dd_sql_loaded;
	    htr_setvisibility(this.sql_loader, 'hidden');
	    }

	var rowlimit = parseInt(aparam.RowLimit)?parseInt(aparam.RowLimit):50;

	var url = "/?cx__akey=" + akey + "&ls__mode=query&ls__rowcount=" + rowlimit + "&ls__sql=" + htutil_escape(aparam.SQL);
	pg_serialized_load(this.sql_loader, url, dd_sql_loaded);
	}
    }

function dd_cb_reveal(e)
    {
    switch (e.eventName) 
	{
	case 'Reveal':
	    if (this.form)
		this.form.Reveal(this,e);
	    else if (this.osrc)
		this.osrc.Reveal(this);
	    break;
	case 'Obscure':
	    // yes the below is correct. API is different between form and osrc.
	    if (this.form)
		this.form.Reveal(this,e);
	    else if (this.osrc)
		this.osrc.Obscure(this);
	    break;
	case 'RevealCheck':
	case 'ObscureCheck':
	    if (this.form)
		this.form.Reveal(this,e);
	    else
		pg_reveal_check_ok(e);
	    break;
	}

    return true;
    }

function dd_deinit()
    {
    dd_collapse(this);
    if (this.form)
	this.form.DeRegister(this);
    if (dd_current == this)
	dd_current = null;
    }

function dd_init(param)
    {
    var l = param.layer;
    l.NumDisplay = param.numDisplay;
    l.Mode = param.mode;
    l.SQL = param.sql;
    l.popup_width = param.popup_width?param.popup_width:param.width;
    l.VisLayer = param.c1;
    l.HidLayer = param.c2;
    htr_init_layer(l.VisLayer, l, 'ddtxt');
    htr_init_layer(l.HidLayer, l, 'ddtxt');
    ifc_init_widget(l);
    l.VisLayer.index = null;
    l.HidLayer.index = null;
    l.Items = new Array();
    l.Values = [];
    if (l.NumDisplay < 5)
	{
	l.NumDisplay = 5;
	}
    l.setvalue   = dd_setvalue;
    l.getvalue   = dd_getvalue;
    l.enable     = dd_enable;
    l.readonly   = dd_readonly;
    l.disable    = dd_disable;
    l.clearvalue = dd_clearvalue;
    l.resetvalue = dd_resetvalue;
    l.keyhandler = dd_keyhandler;
    l.additems = dd_add_items;
    l.destroy_widget = dd_deinit;

    if(l.Mode == 3)
	{
	if (param.osrc)
	    l.osrc = wgtrGetNode(l, param.osrc);
	else
	    l.osrc = wgtrFindContainer(l, "widget/osrc");
        if(!l.osrc)
	    {
	    alert("Drop Down in objectsource mode needs to be inside an osrc widget!");
	    }
	l.osrc.Register(l);
	l.IsDiscardReady=new Function('return true;');
	l.IsUnsaved = false;
	l.DataAvailable=dd_clear_layers;
	l.ObjectAvailable=dd_update;
	l.ReplicaMoved=dd_update;
	l.OperationComplete=new Function();
	l.ObjectDeleted=dd_update;
	l.ObjectCreated=dd_update;
	//l.ObjectModified=dd_object_modified;
	}
    l.losefocushandler = dd_losefocus;
    l.getfocushandler = dd_getfocus;
    l.bg = param.background;
    l.hl = param.highlight;
    l.w = param.width; l.h = param.height;
    l.fieldname = param.fieldname;
    l.enabled = 'full';
    if (l.Mode != 3)
	{
	if (param.form)
	    l.form = wgtrGetNode(l, param.form);
	else
	    l.form = wgtrFindContainer(l,"widget/form");
	}
    l.query_multiselect = param.qms;
    l.invalid_select_default = param.ivs;
    l.value = null;
    l.label = null;
    htr_init_layer(l,l,'dd');
    htutil_tag_images(l,'dd',l,l);
    var imgs = pg_images(l);
    for(var i = 0; i<imgs.length; i++)
	{
	if (imgs[i].src.substr(-14, 6) == 'dkgrey')
	    imgs[i].downimg = true;
	else if (imgs[i].src.substr(-13,5) == 'white')
	    imgs[i].upimg = true;
	}
    l.area = pg_addarea(l, -1, -1, getClipWidth(l)+3, 
	    getClipHeight(l)+3, 'dd', 'dd', 3);
    if (l.form) l.form.Register(l);
    l.init_items = false;

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseMove");
    ie.Add("DataChange");
    ie.Add("DataModify");
    ie.Add("GetFocus");
    ie.Add("LoseFocus");
    ie.Add("RightClick");

    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("SetValue", dd_action_set_value);
    ia.Add("SetGroup", dd_action_set_group);
    ia.Add("SetItems", dd_action_set_items);
    ia.Add("ClearItems", dd_clear_layers);

    if (l.form)
	{
	l.form.ifcProbe(ifEvent).Hook('StatusChange',dd_changemode,l);
	}

    if (l.form || l.osrc)
	{
	l.Reveal = dd_cb_reveal;
	if (pg_reveal_register_listener(l)) 
	    {
	    // already visible
	    if (l.form)
		l.form.Reveal(l, { eventName:'Reveal' });
	    else
		l.osrc.Reveal(l, { eventName:'Reveal' });
	    }
	}

    return l;
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_dropdown.js'] = true;
