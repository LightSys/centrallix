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
    for (i=0; i < this.Values.length; i++)
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
    this.document.images[4].src = '/sys/images/ico15b.gif';
    this.keyhandler = dd_keyhandler;
    this.enabled = 'full';
    }

function dd_readonly()
    {
    this.document.images[4].src = '/sys/images/ico15b.gif';
    this.keyhandler = null;
    this.enabled = 'readonly';
    }

function dd_disable()
    {
    if (dd_current)
	{
	dd_current.PaneLayer.visibility = 'hide';
	dd_current = null;
	}
    this.document.images[4].src = '/sys/images/ico15a.gif';
    this.keyhandler = null;
    this.enabled = 'disabled';
    }

// Normal functions

function dd_keyhandler(l,e,k)
    {
    if (!dd_current) return;
    if (dd_current.enabled != 'full') return 1;
    if ((k >= 65 && k <= 90) || (k >= 97 && k <= 122))
	{
	if (k < 97) 
	    {
	    k_lower = k + 32;
	    k_upper = k;
	    k = k + 32;
	    }
	else
	    {
	    k_lower = k;
	    k_upper = k - 32;
	    }
	if (!dd_lastkey || dd_lastkey != k)
	    {
	    for (i=0; i < this.Values.length; i++)
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
	    for (i=0; i < this.Values.length; i++)
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
    else if (k == 13 && dd_lastkey != 13)
	{
	dd_select_item(this,this.SelectedItem);
	dd_unhilight_item(this,this.SelectedItem);
	}
    dd_lastkey = k;
    return false;
    }

function dd_hilight_item(l,i)
    {
    dd_scroll_to(l,i);
    if (l.SelectedItem != null)
	dd_unhilight_item(l,l.SelectedItem);
    l.SelectedItem = i;
    l.Items[i].bgColor=l.hl;
    }

function dd_unhilight_item(l,i)
    {
    l.SelectedItem = null;
    l.Items[i].bgColor=l.bg;
    }

function dd_select_item(l,i)
    {
    l.HidLayer.document.write("<TABLE height=18 cellpadding=1 cellspacing=0 border=0><TR><TD valign=middle>");
    if (i!=null) l.HidLayer.document.write(l.Values[i][0]);
    l.HidLayer.document.write("</TD></TR></TABLE>");
    l.HidLayer.document.close();
    l.HidLayer.visibility = 'inherit';
    l.HidLayer.index = i;
    l.VisLayer.visibility = 'hide';
    var t=l.VisLayer;
    l.VisLayer = l.HidLayer;
    l.HidLayer = t;
    if(l.form)
	{
	l.form.DataNotify(l);
	cn_activate(l, "DataChange");
	}
    l.PaneLayer.visibility = 'hide';
    dd_current = null;
    }

function dd_getfocus()
    {
    cn_activate(this, "GetFocus");
    return 0;
    }

function dd_losefocus()
    {
    cn_activate(this, "LoseFocus");
    return true;
    }

function dd_toggle(l) 
    {
    for (i=0; i<l.document.images.length;i++) {
	if (i == 4)
	    continue;
	else if (l.document.images[i].src.substr(-14, 6) == 'dkgrey')
	    l.document.images[i].src = '/sys/images/white_1x1.png';
	else
	    l.document.images[i].src = '/sys/images/dkgrey_1x1.png';
	}
    }

function dd_scroll_to(l, n)
    {
    var top=dd_current.PaneLayer.ScrLayer.clip.top;
    var btm=top+(dd_current.PaneLayer.clip.height-4);
    var il=l.Items[n];

    if (il.y>=top && il.y+16<=btm) //none
	return;
    else if (il.y<top) //up
	{
	dd_target_img=l.PaneLayer.BarLayer.document.images[0];
	dd_incr = (top-il.y);
	}
    else //down
	{
	dd_target_img=l.PaneLayer.BarLayer.document.images[2];
	dd_incr = (top-il.y+16);
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
    var ti=dd_target_img;
    var px=dd_incr;
    var ly=dd_current.PaneLayer.ScrLayer;
    var ht1=ly.y-2;
    var ht2=dd_current.PaneLayer.h-ly.clip.height+ht1;
    var h=dd_current.PaneLayer.h;
    var d=h-dd_current.PaneLayer.clip.height-4;
    var v=dd_current.PaneLayer.clip.height-(3*18)-4;
    if (ht1+px>0) px = -ht1;
    if (ht2+px<0) px = -ht2;
    if (px<0 && ht2>0)
	{
	ly.y += px;
	ly.clip.height -= px;
	ly.clip.top -= px;
	if (t==null)
	    {
	    if (d<=0) ti.thum.y=18;
	    else ti.thum.y=18+v*(-(ly.y+2)/d);
	    }
	}
    else if (px>0 && ht1<0)
	{
	ly.y += px;
	ly.clip.height -= px;
	ly.clip.top -= px;
	if (t==null)
	    {
	    if (d<=0) ti.thum.y=18;
	    else ti.thum.y=22+v*(-(ly.y+2)/d);
	    }
	}
    }

function dd_create_pane(l)
    {
    p = new Layer(1024);
    p.kind = 'dd_pn';
    p.visibility = 'hide';
    p.document.layer = p;
    p.mainlayer = l;
    p.document.write("<BODY bgcolor="+l.bg+">");
    p.document.write("<TABLE border=0 cellpadding=0 cellspacing=0 width="+l.w+" height="+l.h2+">");
    p.document.write("<TR><TD><IMG SRC=/sys/images/white_1x1.png height=1></TD>");
    p.document.write("  <TD><IMG SRC=/sys/images/white_1x1.png height=1 width="+(l.w-2)+"></TD>");
    p.document.write("  <TD><IMG SRC=/sys/images/white_1x1.png height=1></TD></TR>");
    p.document.write("<TR><TD><IMG SRC=/sys/images/white_1x1.png height="+(l.h2-2)+" width=1></TD>");
    p.document.write("  <TD valign=top>");
    p.document.write("  <TABLE border=0 cellpadding=0 cellspacing=0 width="+(l.w-2)+" height="+(l.h2-2)+">");
    p.document.write("  <TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD>");
    p.document.write("    <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width="+(l.w-4)+"></TD>");
    p.document.write("    <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD></TR>");
    p.document.write("  <TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png height="+(l.h2-4)+" width=1></TD>");
    p.document.write("    <TD valign=top>");
    p.document.write("    </TD>");
    p.document.write("    <TD><IMG SRC=/sys/images/white_1x1.png height="+(l.h2-4)+" width=1></TD></TR>");
    p.document.write("  <TR><TD><IMG SRC=/sys/images/white_1x1.png height=1></TD>");
    p.document.write("    <TD><IMG SRC=/sys/images/white_1x1.png height=1 width="+(l.w-4)+"></TD>");
    p.document.write("    <TD><IMG SRC=/sys/images/white_1x1.png height=1></TD></TR>");
    p.document.write("  </TABLE>");
    p.document.write("  </TD>");
    p.document.write("  <TD><IMG SRC=/sys/images/dkgrey_1x1.png height="+(l.h2-2)+" width=1></TD></TR>");
    p.document.write("<TR><TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD>");
    p.document.write("  <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1 width="+(l.w-2)+"></TD>");
    p.document.write("  <TD><IMG SRC=/sys/images/dkgrey_1x1.png height=1></TD></TR>");
    p.document.write("</TABLE>");
    p.document.write("</BODY>");
    p.document.close();
    htutil_tag_images(p.document,'dt_pn',p,l);

    /**  Create scroll background layer  **/
    p.ScrLayer = new Layer(1024, p);
    p.ScrLayer.document.layer = p;
    p.ScrLayer.mainlayer = l;
    p.ScrLayer.x = 2; p.ScrLayer.y = 2;
    p.ScrLayer.clip.height = l.h2;
    if (l.NumDisplay < l.NumElements)
	{
	/**  If we need a scrollbar, put one in  **/
	p.ScrLayer.clip.width = p.clip.width - 22;

	p.BarLayer = new Layer(1024, p)
	p.BarLayer.kind = 'dd_sc';
	p.BarLayer.x = l.w-20; p.BarLayer.y = 2;
	p.BarLayer.visibility = 'inherit';
	var pd = p.BarLayer.document;
	pd.layer = p.BarLayer;
	pd.mainlayer = l;
	pd.write('<TABLE border=0 cellpadding=0 cellspacing=0 width=18 height='+(l.h2-4)+'>');
	pd.write('<TR><TD><IMG name=u src=/sys/images/ico13b.gif></TD></TR>');
	pd.write('<TR><TD><IMG name=b src=/sys/images/trans_1.gif height='+(l.h2-40)+'></TD></TR>');
	pd.write('<TR><TD><IMG name=d src=/sys/images/ico12b.gif></TD></TR>');
	pd.write('</TABLE>');
	pd.close();
	pd.images[0].mainlayer = pd.images[1].mainlayer = pd.images[2].mainlayer = l;
	pd.images[0].kind = pd.images[1].kind = pd.images[2].kind = 'dd_sc';
	l.imgup = pd.images[0];
	l.imgdn = pd.images[2];

	p.TmbLayer = new Layer(1024, p);
	pd.images[0].thum = pd.images[2].thum = p.TmbLayer;
	p.TmbLayer.x = l.w-20; p.TmbLayer.y = 20;
	p.TmbLayer.visibility = 'inherit';
	var pd = p.TmbLayer.document;
	pd.write('<IMG src=/sys/images/ico14b.gif NAME=t>');
	pd.close();
	pd.mainlayer = l;
	pd.images[0].mainlayer = l;
	pd.images[0].thum = p.TmbLayer;
	pd.images[0].kind = 'dd_sc';
	l.imgtm = pd.images[0];
	}
    else
	{
	/**  If no scrollbar is needed, don't use one!  **/
	p.ScrLayer.clip.width = p.clip.width - 4;
	}
    p.ScrLayer.clip.height = p.clip.height - 4;
    p.ScrLayer.visibility = 'inherit';

    /**  Add items  **/
    for (var i=0; i < l.Values.length; i++)
	{
	if (!l.Items[i])
	    {
	    l.Items[i] = new Layer(1024, p.ScrLayer);
	    l.Items[i].kind = 'dd_itm';
	    }
	l.Items[i].mainlayer = l;
	l.Items[i].document.layer = l.Items[i];
	l.Items[i].x = 0;
	l.Items[i].y = (i*16);
	l.Items[i].clip.width = p.ScrLayer.clip.width;
	l.Items[i].clip.height = 16;
	l.Items[i].document.write(l.Values[i][0]);
	l.Items[i].document.close();
	l.Items[i].visibility = 'inherit';
	l.Items[i].index = i;
	}

    return p;
    }

function dd_add_items(l,ary)
    {
    l.Values = ary;
    l.NumElements = l.Values.length;
    l.h2 = ((l.NumDisplay<l.NumElements?l.NumDisplay:l.NumElements)*16)+4;
    l.PaneLayer = dd_create_pane(l);
    l.PaneLayer.h = l.NumElements*16;
    l.PaneLayer.mainlayer = l;
    }

function dd_init(l,c1,c2,bg,hl,fn,d,m,s,w,h)
    {
    l.NumDisplay = d;
    l.Mode = m;
    l.SQL = s;
    l.VisLayer = c1;
    l.HidLayer = c2;
    l.VisLayer.document.layer = l.HidLayer.document.layer = l;
    l.VisLayer.mainlayer = l.HidLayer.mainlayer = l;
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
    l.document.layer = l;
    l.mainlayer = l;
    l.kind = 'dd';
    htutil_tag_images(l.document,'dd',l,l);
    pg_addarea(l, -1, -1, l.clip.width+1, l.clip.height+1, 'dd', 'dd', 0);
    if (fm_current) fm_current.Register(l);
    return l;
    }
