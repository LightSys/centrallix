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
	    t.pageY += 2;
	    t.pageX += 1;
	    t.clip.height = 23;
	    }
	}
    this.zIndex = this.tabctl.zIndex + 1;
    this.tabpage.visibility = 'inherit';
    this.document.images['tb'].src = '/sys/images/tab_lft2.gif';
    this.pageY -= 2;
    this.pageX -= 1;
    this.clip.height = 25;
    }

function tc_addtab(l_tab, l_page, l)
    {
    if (this.tabs.length > 0)
	{
	newx = this.tabs[this.tabs.length-1].pageX + this.tabs[this.tabs.length-1].document.width + 1;
	if (this.tabs[this.tabs.length-1].tabpage.visibility == 'inherit') newx += 1;
	}
    else
	newx = this.pageX;
    newy = this.pageY - 24;
    if (l_page.visibility != 'inherit')
	{
	newx += 1;
	newy += 2;
	l_tab.clip.height = 23;
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

function tc_init(l)
    {
    l.mainlayer = l;
    l.document.layer = l;
    l.kind = 'tc';
    l.currentTab = 1;
    l.nTabs = 0;
    l.isShorting = 0;
    l.tabs = new Array();
    l.addTab = tc_addtab;
    if (tc_tabs == null) tc_tabs = new Array();
    tc_tabs[tc_tabs.length++] = l;
    return l;
    }
