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
    if (this.zIndex > this.tabctl.zIndex) return 0;
    for(i=0;i<this.tabctl.tabs.length;i++)
	{
	t = this.tabctl.tabs[i];
	if (t != this && t.zIndex > this.tabctl.zIndex)
	    {
	    t.zIndex = this.tabctl.zIndex - 1;
	    t.tabpage.visibility = 'hidden';
	    t.document.images['tb'].src = '/sys/images/tab_lft3.gif';
	    t.pageY += this.tabctl.yo;
	    t.pageX += this.tabctl.xo;
	    t.clip[this.tabctl.cl] += this.tabctl.ci;
	    }
	}
    this.zIndex = this.tabctl.zIndex + 1;
    this.tabpage.visibility = 'inherit';
    this.document.images['tb'].src = '/sys/images/tab_lft2.gif';
    this.pageY -= this.tabctl.yo;
    this.pageX -= this.tabctl.xo;
    this.clip[this.tabctl.cl] -= this.tabctl.ci;
    }

function tc_addtab(l_tab, l_page, l)
    {
    if (l.tloc == 0 || l.tloc == 1) // top or bottom
	{
	if (this.tabs.length > 0)
	    {
	    newx = this.tabs[this.tabs.length-1].pageX + this.tabs[this.tabs.length-1].document.width + 1;
	    if (this.tabs[this.tabs.length-1].tabpage.visibility == 'inherit') newx += l.xo;
	    }
	else
	    newx = this.pageX;
	}
    else if (l.tloc == 2) // left
	newx = this.pageX - l_tab.clip.width + 1;
    else if (l.tloc == 3) // right
	newx = this.pageX + this.clip.width - 1;

    if (l.tloc == 2 || l.tloc == 3) // left or right
	{
	if (this.tabs.length > 0)
	    {
	    newy = this.tabs[this.tabs.length-1].pageY + 26;
	    if (this.tabs[this.tabs.length-1].tabpage.visibility == 'inherit') newy += l.yo;
	    }
	else
	    newy = this.pageY;
	}
    else if (l.tloc == 1) // bottom
	newy = this.pageY + this.clip.height - 1;
    else // top
	newy = this.pageY - 24;
	    
    if (l_page.visibility != 'inherit')
	{
	newx += l.xo;
	newy += l.yo;
	l_tab.clip[l.cl] -= l.ci;
	}
    for(i=0;i<l_tab.document.images.length;i++)
	{
	l_tab.document.images[i].layer = l_tab;
	l_tab.document.images[i].kind = 'tc';
	}
    this.tabs[this.tabs.length++] = l_tab;
    l_page.kind = 'tc_pn';
    l_page.document.layer = l_page;
    l_page.mainlayer = l;
    l_tab.mainlayer = l;
    l_tab.document.layer = l_tab;
    l_tab.tabpage = l_page;
    l_tab.tabctl = this;
    l_tab.makeCurrent = tc_makecurrent;
    l_tab.pageX = newx;
    l_tab.pageY = newy;
    l_tab.kind = 'tc';
    l_tab.document.layer = l_tab;
    l_tab.document.Layer = l_tab;
    l_page.clip.width = this.clip.width-2;
    l_page.clip.height = this.clip.height-2;
    return l_tab;
    }

function tc_init(l,tloc)
    {
    l.mainlayer = l;
    l.document.layer = l;
    l.kind = 'tc';
    l.currentTab = 1;
    l.nTabs = 0;
    l.isShorting = 0;
    l.tabs = new Array();
    l.addTab = tc_addtab;
    l.tloc = tloc;
    if (tc_tabs == null) tc_tabs = new Array();
    tc_tabs[tc_tabs.length++] = l;
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
