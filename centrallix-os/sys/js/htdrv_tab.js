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

function tc_makecurrent()
    {
    if (htr_getzindex(this) > htr_getzindex(this.tabctl)) return 0;
    for(i=0;i<this.tabctl.tabs.length;i++)
	{
	t = this.tabctl.tabs[i];
	if (t != this && htr_getzindex(t) > htr_getzindex(this))
	    {
	    htr_setzindex(t,htr_getzindex(this.tabctl) - 1);
	    htr_setzindex(t.tabpage,htr_getzindex(this.tabctl) - 1);
	    htr_setvisibility(t.tabpage,'hidden');
	    t.marker_image.src = '/sys/images/tab_lft3.gif';
	    t.pageY += this.tabctl.yo;
	    t.pageX += this.tabctl.xo;
	    t.clip[this.tabctl.cl] += this.tabctl.ci;
	    if (this.tabctl.inactive_bgColor) htr_setbgcolor(t,this.tabctl.inactive_bgColor);
	    if (this.tabctl.inactive_bgnd) htr_setbgimage(t,this.tabctl.inactive_bgnd);
	    }
	}
    if (this.tabctl.main_bgColor) htr_setbgcolor(this,this.tabctl.main_bgColor);
    if (this.tabctl.main_bgnd) htr_setbgimage(this,this.tabctl.main_bgnd);
    htr_setzindex(this, htr_getzindex(this.tabctl) + 1);
    htr_setzindex(this.tabpage, htr_getzindex(this.tabctl) + 1);
    htr_setvisibility(this.tabpage,'inherit');
    this.marker_image.src = '/sys/images/tab_lft2.gif';
    this.pageY -= this.tabctl.yo;
    this.pageX -= this.tabctl.xo;
    this.clip[this.tabctl.cl] -= this.tabctl.ci;
    htr_unwatch(this.tabctl,"selected","tc_selection_changed");
    htr_unwatch(this.tabctl,"selected_index","tc_selection_changed");
    this.tabctl.selected_index = this.tabindex;
    this.tabctl.selected = this.tabname;
    htr_watch(this.tabctl,"selected", "tc_selection_changed");
    htr_watch(this.tabctl,"selected_index", "tc_selection_changed");
    }

function tc_addtab(l_tab, l_page, l, nm)
    {
    l_tab.tabname = nm;
    l_tab.tabindex = this.tabs.length+1;
    htr_init_layer(l_page,l,'tc_pn');
    htr_init_layer(l_tab,l,'tc');
    if (l.tloc == 0 || l.tloc == 1) // top or bottom
	{
	if (this.tabs.length > 0)
	    {
	    newx = this.tabs[this.tabs.length-1].pageX + htr_getphyswidth(this.tabs[this.tabs.length-1]) + 1;
	    if (htr_getvisibility(this.tabs[this.tabs.length-1].tabpage) == 'inherit') newx += l.xo;
	    }
	else
	    newx = this.pageX;
	}
    else if (l.tloc == 2) // left
	newx = this.pageX - htr_getviswidth(l_tab) + 1;
    else if (l.tloc == 3) // right
	newx = this.pageX + htr_getviswidth(this) - 1;

    if (l.tloc == 2 || l.tloc == 3) // left or right
	{
	if (this.tabs.length > 0)
	    {
	    newy = this.tabs[this.tabs.length-1].pageY + 26;
	    if (htr_getvisibility(this.tabs[this.tabs.length-1].tabpage) == 'inherit') newy += l.yo;
	    }
	else
	    newy = this.pageY;
	}
    else if (l.tloc == 1) // bottom
	newy = this.pageY + htr_getvisheight(this) - 1;
    else // top
	newy = this.pageY - 24;
	    
    if (htr_getvisibility(l_page) != 'inherit')
	{
	newx += l.xo;
	newy += l.yo;
	l_tab.clip[l.cl] -= l.ci;
	if (l.inactive_bgColor) htr_setbgcolor(l_tab, l.inactive_bgColor);
	if (l.inactive_bgnd) htr_setbgimage(l_tab, l.inactive_bgnd);
	}
    else
	{
	htr_unwatch(l,"selected","tc_selection_changed");
	htr_unwatch(l,"selected_index","tc_selection_changed");
	l.selected = l_tab.tabname;
	l.selected_index = l_tab.tabindex;
	htr_watch(l,"selected", "tc_selection_changed");
	htr_watch(l,"selected_index", "tc_selection_changed");
	if (l.main_bgColor) htr_setbgcolor(l_tab, l.main_bgColor);
	if (l.main_bgnd) htr_setbgimage(l_tab, l.main_bgnd);
	}
    var images = pg_images(l_tab);
    for(i=0;i<images.length;i++)
	{
	images[i].layer = l_tab;
	images[i].kind = 'tc';
	if  (images[i].width == 5) l_tab.marker_image = images[i];
	}
    this.tabs[this.tabs.length++] = l_tab;
    l_tab.tabpage = l_page;
    l_tab.tabctl = this;
    l_tab.makeCurrent = tc_makecurrent;
    l_tab.pageX = newx;
    l_tab.pageY = newy;
    l_page.clip.width = this.clip.width-2;
    l_page.clip.height = this.clip.height-2;
    return l_tab;
    }

function tc_selection_changed(prop,o,n)
    {
    var tabindex = null;
    // find index if name specified
    if (prop == 'selected')
	{
	for (var i=0; i<this.tabs.length; i++)
	    if (this.tabs[i].tabname == n)
		{
		tabindex = i+1;
		break;
		}
	}
    else
	tabindex = n;
    if (tabindex < 1 || tabindex > this.tabs.length) return o;

    // okay to change tab.
    //this.tabs[tabindex-1].makeCurrent();
    pg_addsched("makeCurrent()", this.tabs[tabindex-1]);
    return n;
    }

function tc_init(l,tloc,mb,ib)
    {
    // Basic stuff...
    htr_init_layer(l,l,'tc');
    l.tabs = new Array();
    l.addTab = tc_addtab;
    l.tloc = tloc;
    if (tc_tabs == null) tc_tabs = new Array();
    tc_tabs[tc_tabs.length++] = l;

    // Background color/image selection...
    l.main_bgColor = htr_extract_bgcolor(mb);
    l.main_bgnd = htr_extract_bgimage(mb);
    l.inactive_bgColor = htr_extract_bgcolor(ib);
    l.inactive_bgnd = htr_extract_bgimage(ib);

    // Properties available to other widgets, that can be used to
    // change current tab index as well.
    l.selected = null;
    l.selected_index = 1;
    htr_watch(l,"selected", "tc_selection_changed");
    htr_watch(l,"selected_index", "tc_selection_changed");
    l.tc_selection_changed = tc_selection_changed;

    // Movement geometries and clipping for tabs
    switch(l.tloc)
	{
	case 0: // top
	    l.xo = +1;
	    l.yo = +2;
	    l.cl = "bottom";
	    l.ci = -2;
	    break;
	case 1: // bottom
	    l.xo = +1;
	    l.yo = -2;
	    l.cl = "top";
	    l.ci = +2;
	    break;
	case 2: // left
	    l.xo = +2;
	    l.yo = +1;
	    l.cl = "right";
	    l.ci = -2;
	    break;
	case 3: // right
	    l.xo = -2;
	    l.yo = +1;
	    l.cl = "left";
	    l.ci = +2;
	    break;
	}
    return l;
    }
