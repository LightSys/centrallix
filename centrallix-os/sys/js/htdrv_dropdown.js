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
    return this.Values[this.VisLayer.index][1];
    }

function dd_setvalue(v) 
    {
    if (!this.PaneLayer) this.PaneLayer = dd_create_pane(this);
    for (var i=0; i < this.Values.length; i++)
	{
	if (this.Values[i][1] == v)
	    {
	    dd_select_item(this, i);
	    return true;
	    }
	}
    return false;
}

function dd_clearvalue()
    {
    dd_select_item(this, null);
    }

function dd_resetvalue()
    {
    this.clearvalue();
    }

function dd_enable()
    {
    pg_images(this)[4].src = '/sys/images/ico15b.gif';
    this.keyhandler = dd_keyhandler;
    this.enabled = 'full';
    }

function dd_readonly()
    {
    pg_images(this)[4].src = '/sys/images/ico15b.gif';
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
    pg_images(this)[4].src = '/sys/images/ico15a.gif';
    this.keyhandler = null;
    this.enabled = 'disabled';
    }

// Normal functions

function dd_keyhandler(l,e,k)
    {
    //if (!dd_current) return;
    var dd = this.mainlayer;
    if (dd.enabled != 'full') return 1;
    if ((k >= 65 && k <= 90) || (k >= 97 && k <= 122))
	{
	if (htr_getvisibility(dd.PaneLayer) != 'inherit')
	    dd_expand(dd);
	if (k < 97) 
	    {
	    var k_lower = k + 32;
	    var k_upper = k;
	    k = k + 32;
	    }
	else
	    {
	    var k_lower = k;
	    var k_upper = k - 32;
	    }
	if (!dd_lastkey || dd_lastkey != k)
	    {
	    for (var i=0; i < this.Values.length; i++)
		{
		if (this.Values[i][0].substring(0, 1) == String.fromCharCode(k_upper) ||
		    this.Values[i][0].substring(0, 1) == String.fromCharCode(k_lower))
		    {
		    dd_hilight_item(this,i);
		    i=this.Values.length;
		    }
		}
	    }
	else
	    {
	    var first = -1;
	    var last = -1;
	    var next = -1;
	    for (var i=0; i < this.Values.length; i++)
		{
		if (this.Values[i][0].substring(0, 1) == String.fromCharCode(k_upper) ||
		    this.Values[i][0].substring(0, 1) == String.fromCharCode(k_lower))
		    {
		    if (first < 0) { first = i; last = i; }
		    for (var j=i; j < this.Values.length && 
			(this.Values[j][0].substring(0, 1) == String.fromCharCode(k_upper) ||
			 this.Values[j][0].substring(0, 1) == String.fromCharCode(k_lower)); j++)
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
	    }
	}
    else if (k == 32)
	{
	dd_expand(this);
	}
    else if (k == 13)
	{
	if (htr_getvisibility(this.PaneLayer) != 'inherit')
	    {
	    if (this.form) this.form.RetNotify(this);
	    }
	else
	    {
	    dd_select_item(this,this.SelectedItem);
	    dd_collapse(this);
	    dd_unhilight_item(this,this.SelectedItem);
	    }
	}
    else if (k == 9)
	{
	if (this.form) this.form.TabNotify(this);
	}
    else if (k == 27)
	{
	if (htr_getvisibility(this.PaneLayer) == 'inherit')
	    dd_collapse(this);
	else
	    if (dd.form) dd.form.EscNotify(dd);
	}
    dd_lastkey = k;
    return false;
    }

function dd_hilight_item(l,i)
    {
    if (i == null)
	{
	if (l.Values[0][1] == null)
	    i = 0;
	else
	    return;
	}
    if (l.SelectedItem != null)
	dd_unhilight_item(l,l.SelectedItem);
    l.SelectedItem = i;
    l.Items[i].bgColor=l.hl;
    dd_scroll_to(l,i);
    }

function dd_unhilight_item(l,i)
    {
    if (i == null)
	{
	if (l.Values[0][1] == null)
	    i = 0;
	else
	    return;
	}
    l.SelectedItem = null;
    l.Items[i].bgColor=l.bg;
    }

function dd_collapse(l)
    {
    if (l && l.PaneLayer && htr_getvisibility(l.PaneLayer) == 'inherit')
	{
	setClipHeight(l, getClipHeight(l) - getClipHeight(l.PaneLayer));
	pg_resize_area(l.area,getClipWidth(l)+1,getClipHeight(l)+1);
	htr_setvisibility(l.PaneLayer, 'hidden');
	dd_current = null;
	}
    }

function dd_expand(l)
    {
    if (l && !l.PaneLayer) 
	l.PaneLayer = dd_create_pane(l);
    if (l && htr_getvisibility(l.PaneLayer) != 'inherit')
	{
	moveToAbsolute(l.PaneLayer, getPageX(l), getPageY(l)+20);
	pg_stackpopup(l.PaneLayer, l);
	htr_setvisibility(l.PaneLayer, 'inherit');
	dd_current = l;
	setClipHeight(l, getClipHeight(l) + getClipHeight(l.PaneLayer));
	pg_resize_area(l.area, getClipWidth(l)+1, getClipHeight(l)+1);
	dd_hilight_item(l,l.VisLayer.index);
	}
    }

function dd_select_item(l,i)
    {
    var c = "<TABLE height=18 cellpadding=1 cellspacing=0 border=0><TR><TD valign=middle nowrap>";
    if (i!=null)
	{
	if (!(i==0 && l.Values[i][1]==null)) 
	    c += l.Values[i][0];
	else
	    c += '<i>' + l.Values[i][0] + '</i>';
	}
    c += "</TD></TR></TABLE>";
    htr_write_content(l.HidLayer, c);
    l.HidLayer.index = i;
    htr_setvisibility(l.HidLayer, 'inherit');
    setClipWidth(l.HidLayer, l.w-21);
    htr_setvisibility(l.VisLayer, 'hidden');
    var t=l.VisLayer;
    l.VisLayer = l.HidLayer;
    l.HidLayer = t;
    if(l.form)
	{
	l.form.DataNotify(l);
	cn_activate(l, "DataChange");
	}
    }

function dd_getfocus()
    {
    //dd_expand(this);
    cn_activate(this, "GetFocus");
    return 1;
    }

function dd_losefocus()
    {
    cn_activate(this, "LoseFocus");
    dd_collapse(this);
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

    if (getRelativeY(il)>=top && getRelativeY(il)+16<=btm) //none
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
	dd_incr = (top-getRelativeY(il)+(16*(dd_current.NumDisplay-1)));
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
    var ly = dd_current.PaneLayer.ScrLayer;
    var ht1 = getRelativeY(ly) - 2;
    var ht2 = dd_current.PaneLayer.h - getClipHeight(ly) + ht1;
    var h = dd_current.PaneLayer.h;
    var d = h - getClipHeight(dd_current.PaneLayer) + 4;
    var v = getClipHeight(dd_current.PaneLayer) - (3*18) - 4;
    if (ht1+px>0) px = -ht1;
    if (ht2+px<0) px = -ht2;
    if ((px<0 && ht2>0) || (px>0 && ht1<0)) // up or down
	{
	moveBy(ly, 0, px);
	setClipHeight(ly, getClipHeight(ly) - px);
	setClipTop(ly, getClipTop(ly) - px);
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
    if (!cx_hints_teststyle(l.mainlayer, cx_hints_style.notnull))
	{
	var nullitem = new Array();
	if (l.mainlayer.w < 108)
	    nullitem[0] = '(none)';
	else
	    nullitem[0] = '(none selected)';
	nullitem[1] = null;
	l.Values.splice(0,0,nullitem);
	}

    // Create the layer
    l.NumElements = l.Values.length;
    l.h2 = ((l.NumDisplay<l.NumElements?l.NumDisplay:l.NumElements)*16)+4;
    var p = htr_new_layer(1024,pg_toplevel_layer(l));
    htr_init_layer(p, l, 'dd_pn');
    htr_setvisibility(p, 'hidden');
    var c = "<BODY bgcolor="+l.bg+">";
    c += "<TABLE border=0 cellpadding=0 cellspacing=0 width="+l.w+" height="+l.h2+">";
    c += "<TR><TD><IMG SRC=/sys/images/white_1x1.png height=1></TD>";
    c += "  <TD><IMG SRC=/sys/images/white_1x1.png height=1 width="+(l.w-2)+"></TD>";
    c += "  <TD><IMG SRC=/sys/images/white_1x1.png height=1></TD></TR>";
    c += "<TR><TD><IMG SRC=/sys/images/white_1x1.png height="+(l.h2-2)+" width=1></TD>";
    c += "  <TD valign=top>";
    c += "  </TD>";
    c += "  <TD><IMG SRC=/sys/images/dkgrey_1x1.png height="+(l.h2-2)+" width=1></TD></TR>";
    c += "<TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD>";
    c += "  <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width="+(l.w-2)+"></TD>";
    c += "  <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD></TR>";
    c += "</TABLE>";
    c += "</BODY>";
    htr_write_content(p, c);
    htutil_tag_images(p.document,'dt_pn',p,l);
    pg_stackpopup(p,l);

    /**  Create scroll background layer  **/
    p.ScrLayer = htr_new_layer(1024, p);
    p.ScrLayer.document.layer = p;
    p.ScrLayer.mainlayer = l;
    moveTo(p.ScrLayer, 2, 2);
    setClipHeight(p.ScrLayer, l.h2);
    if (l.NumDisplay < l.NumElements)
	{
	/**  If we need a scrollbar, put one in  **/
	setClipWidth(p.ScrLayer, getClipWidth(p) - 22);

	p.BarLayer = htr_new_layer(1024, p)
	htr_init_layer(p.BarLayer, l, 'dd_sc');
	moveTo(p.BarLayer, l.w-20, 2);
	htr_setvisibility(p.BarLayer, 'inherit');
	c = '<TABLE border=0 cellpadding=0 cellspacing=0 width=18 height='+(l.h2-4)+'>';
	c += '<TR><TD><IMG name=u src=/sys/images/ico13b.gif></TD></TR>';
	c += '<TR><TD><IMG name=b src=/sys/images/trans_1.gif height='+(l.h2-40)+'></TD></TR>';
	c += '<TR><TD><IMG name=d src=/sys/images/ico12b.gif></TD></TR>';
	c += '</TABLE>';
	htr_write_content(p.BarLayer, c);
	var imgs = pg_images(p.BarLayer);
	imgs[0].mainlayer = imgs[1].mainlayer = imgs[2].mainlayer = l;
	imgs[0].kind = imgs[1].kind = imgs[2].kind = 'dd_sc';
	l.imgup = imgs[0];
	l.imgdn = imgs[2];

	p.TmbLayer = htr_new_layer(1024, p);
	imgs[0].thum = imgs[1].thum = imgs[2].thum = p.TmbLayer;
	moveTo(p.TmbLayer, l.w-20, 20);
	htr_setvisibility(p.TmbLayer, 'inherit');
	p.TmbLayer.mainlayer = l;
	htr_write_content(p.TmbLayer,'<IMG src=/sys/images/ico14b.gif NAME=t>');
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
    for (var i=0; i < l.Values.length; i++)
	{
	if (!l.Items[i])
	    {
	    l.Items[i] = htr_new_layer(1024, p.ScrLayer);
	    htr_init_layer(l.Items[i], l, 'dd_itm');
	    }
	moveTo(l.Items[i], 0, i*16);
	setClipWidth(l.Items[i], getClipWidth(p.ScrLayer));
	setClipHeight(l.Items[i], 16);
	if (i==0 && l.Values[i][1] == null)
	    htr_write_content(l.Items[i], '<i>' + l.Values[i][0] + '</i>');
	else
	    htr_write_content(l.Items[i], l.Values[i][0]);
	htr_setvisibility(l.Items[i], 'inherit');
	l.Items[i].index = i;
	}

    p.h = l.NumElements*16;
    p.mainlayer = l;

    return p;
    }


function dd_add_items(l,ary)
    {
    for(var i=0; i<ary.length;i++) 
	{
	ary[i][0] = htutil_rtrim(ary[i][0]);
	ary[i][1] = htutil_rtrim(ary[i][1]);
	}
    l.Values = ary;
    }


function dd_init(l,c1,c2,bg,hl,fn,d,m,s,w,h)
    {
    l.NumDisplay = d;
    l.Mode = m;
    l.SQL = s;
    l.VisLayer = c1;
    l.HidLayer = c2;
    htr_init_layer(l.VisLayer, l, 'ddtxt');
    htr_init_layer(l.HidLayer, l, 'ddtxt');
    l.VisLayer.index = null;
    l.HidLayer.index = null;
    l.Items = new Array();
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
    l.losefocushandler = dd_losefocus;
    l.getfocushandler = dd_getfocus;
    l.bg = bg;
    l.hl = hl;
    l.w = w; l.h = h;
    l.fieldname = fn;
    l.enabled = 'full';
    l.form = fm_current;
    htr_init_layer(l,l,'dd');
    htutil_tag_images(l.document,'dd',l,l);
    var imgs = pg_images(l);
    for(var i = 0; i<imgs.length; i++)
	{
	if (imgs[i].src.substr(-14, 6) == 'dkgrey')
	    imgs[i].downimg = true;
	else if (imgs[i].src.substr(-13,5) == 'white')
	    imgs[i].upimg = true;
	}
    l.area = pg_addarea(l, -1, -1, getClipWidth(l)+1, getClipHeight(l)+1, 'dd', 'dd', 3);
    if (fm_current) fm_current.Register(l);
    return l;
    }
