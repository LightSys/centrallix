// Copyright (C) 1998-2004 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.


// Sets the value of the current tab (but not the appearance), without
// triggering on-change events.
function tc_set_tab_unwatched()
    {
    htr_unwatch(this.tabctl,"selected","tc_selection_changed");
    htr_unwatch(this.tabctl,"selected_index","tc_selection_changed");
    this.tabctl.selected_index = this.tabindex;
    this.tabctl.selected = this.tabname;
    this.tabctl.current_tab = this;
    htr_watch(this.tabctl,"selected", "tc_selection_changed");
    htr_watch(this.tabctl,"selected_index", "tc_selection_changed");
    }


// Makes the given tab current.
function tc_makecurrent()
    {
    var t;
    if (this.tabctl.tloc != 4 && htr_getzindex(this.tab) > htr_getzindex(this.tabctl)) return 0;
    for(var i=0;i<this.tabctl.tabs.length;i++)
	{
	t = this.tabctl.tabs[i];
	if (t != this && (t.tabctl.tloc == 4 || htr_getzindex(t.tab) > htr_getzindex(this.tab)))
	    {
	    htr_setzindex(t, htr_getzindex(this.tabctl) - 1);
	    htr_setvisibility(t, 'hidden');
	    if (t.tabctl.tloc != 4)
		{
		htr_setzindex(t.tab, htr_getzindex(this.tabctl) - 1);
		t.tab.marker_image.src = '/sys/images/tab_lft3.gif';
		moveBy(t.tab, this.tabctl.xo, this.tabctl.yo);
		//setClipItem(t.tab, t.tabctl.cl, getClipItem(t.tab, t.tabctl.cl) + t.tabctl.ci);
		if (this.tabctl.inactive_bgColor) htr_setbgcolor(t.tab, this.tabctl.inactive_bgColor);
		if (this.tabctl.inactive_bgnd) htr_setbgimage(t.tab, this.tabctl.inactive_bgnd);
		}
	    }
	}
    htr_setzindex(this, htr_getzindex(this.tabctl) + 1);
    htr_setvisibility(this,'inherit');
    if (this.tabctl.tloc != 4)
	{
	if (this.tabctl.main_bgColor) htr_setbgcolor(this.tab, this.tabctl.main_bgColor);
	if (this.tabctl.main_bgnd) htr_setbgimage(this.tab, this.tabctl.main_bgnd);
	htr_setzindex(this.tab, htr_getzindex(this.tabctl) + 1);
	this.tab.marker_image.src = '/sys/images/tab_lft2.gif';
	moveBy(this.tab, -this.tabctl.xo, -this.tabctl.yo);
	//setClipItem(this.tab, this.tabctl.cl, getClipItem(this.tab, this.tabctl.cl) - this.tabctl.ci);
	}
    this.setTabUnwatched();
    this.tabctl.ifcProbe(ifEvent).Activate("TabChanged", {Selected:this.tabctl.selected, SelectedIndex:this.tabctl.selected_index});
    }

function tc_makenotcurrent(t)
    {
    htr_setzindex(t,htr_getzindex(t.tabctl) - 1);
    htr_setvisibility(t,'hidden');
    if (t.tabctl.tloc != 4)
	{
	htr_setzindex(t.tab,htr_getzindex(t.tabctl) - 1);
	t.tab.marker_image.src = '/sys/images/tab_lft3.gif';
	moveBy(t.tab, t.tabctl.xo, t.tabctl.yo);
	//setClipItem(t.tab, t.tabctl.cl, getClipItem(t.tab, t.tabctl.cl) + t.tabctl.ci);
	if (t.tabctl.inactive_bgColor) htr_setbgcolor(t.tab, t.tabctl.inactive_bgColor);
	if (t.tabctl.inactive_bgnd) htr_setbgimage(t.tab, t.tabctl.inactive_bgnd);
	}
    }

// Adds a new tab to the tab control
function tc_addtab(l_tab, l_page, l, nm, type,fieldname)
    {
    var newx;
    var newy;
    if (!l_tab) l_tab = new Object();
    l_page.tabname = nm;
    l_page.type = type;
    l_page.fieldname = fieldname;
    l_page.tabindex = this.tabs.length+1;
    htr_init_layer(l_page,l,'tc_pn');
    ifc_init_widget(l_page);
    if (l.tloc != 4) 
	{
	htr_init_layer(l_tab,l,'tc');
	if (l.tloc == 0 || l.tloc == 1) // top or bottom
	    {
	    if (this.tabs.length > 0)
		{
		//alert(htr_getphyswidth(this.tabs[this.tabs.length-1]));
		newx = getPageX(this.tabs[this.tabs.length-1].tab) + $(this.tabs[this.tabs.length-1].tab).outerWidth() + 1;
		if (htr_getvisibility(this.tabs[this.tabs.length-1]) == 'inherit') newx += l.xo;
		}
	    else
		newx = getPageX(this);
	    }
	else if (l.tloc == 2) // left
	    newx = getPageX(this)- htr_getviswidth(l_tab) + 0;
	else if (l.tloc == 3) // right
	    newx = getPageX(this) + htr_getviswidth(this) + 1;

	if (l.tloc == 2 || l.tloc == 3) // left or right
	    {
	    if (this.tabs.length > 0)
		{
		newy = getPageY(this.tabs[this.tabs.length-1].tab) + 26;
		if (htr_getvisibility(this.tabs[this.tabs.length-1]) == 'inherit') newy += l.yo;
		}
	    else
		newy = getPageY(this);
	    }
	else if (l.tloc == 1) // bottom
	    newy = getPageY(this)+ htr_getvisheight(this) + 1;
	else // top
	    newy = getPageY(this) - 24;

	// Clipping
	switch(l.tloc)
	    {
	    case 0: // top
		$(l_tab).css('clip', 'rect(-10px, ' + ($(l_tab).outerWidth()+10) + 'px, 25px, -10px)');
		break;
	    case 1: // bottom
		$(l_tab).css('clip', 'rect(0px, ' + ($(l_tab).outerWidth()+10) + 'px, 35px, -10px)');
		break;
	    case 2: // left
		$(l_tab).css('clip', 'rect(-10px, ' + ($(l_tab).outerWidth()) + 'px, 35px, -10px)');
		break;
	    case 3: // right
		$(l_tab).css('clip', 'rect(-10px, ' + ($(l_tab).outerWidth()+10) + 'px, 35px, 0px)');
		break;
	    }
	}
    else
	{
	newx = 0;
	newy = 0;
	}

    if (htr_getvisibility(l_page) != 'inherit')
	{
	if (l.tloc != 4)
	    {
	    newx += l.xo;
	    newy += l.yo;
	    //setClipItem(l_tab, l.cl, getClipItem(l_tab, l.cl) + l.ci);
	    if (l.inactive_bgColor) htr_setbgcolor(l_tab, l.inactive_bgColor);
	    else if (l.main_bgColor) htr_setbgcolor(l_tab, l.main_bgColor);
	    if (l.inactive_bgnd) htr_setbgimage(l_tab, l.inactive_bgnd);
	    else if (l.main_bgnd) htr_setbgimage(l_tab, l.main_bgnd);
	    }
	}
    else
	{
	htr_unwatch(l,"selected","tc_selection_changed");
	htr_unwatch(l,"selected_index","tc_selection_changed");
	l.selected = l_page.tabname;
	l.selected_index = l_page.tabindex;
	l.current_tab = l_page;
	l.init_tab = l_page;
	pg_addsched_fn(window,"pg_reveal_event",new Array(l_page,l_page,'Reveal'), 0);
	htr_watch(l,"selected", "tc_selection_changed");
	htr_watch(l,"selected_index", "tc_selection_changed");
	if (l.tloc != 4)
	    {
	    if (l.main_bgColor) htr_setbgcolor(l_tab, l.main_bgColor);
	    if (l.main_bgnd) htr_setbgimage(l_tab, l.main_bgnd);
	    }
	}
    if (l.tloc != 4)
	{
	var images = pg_images(l_tab);
	for(var i=0;i<images.length;i++)
	    {
	    images[i].layer = l_tab;
	    images[i].kind = 'tc';
	    if  (images[i].width == 5) l_tab.marker_image = images[i];
	    }
	}
    this.tabs[this.tabs.length++] = l_page;
    l_page.tabctl = this;
    l_page.tab = l_tab;
    l_page.makeCurrent = tc_makecurrent;
    l_page.setTabUnwatched = tc_set_tab_unwatched;
    if (l.tloc != 4)
	{
	moveToAbsolute(l_tab, newx + 0.001, newy + 0.001);
	}
    l_tab.tabpage = l_page;
    l_tab.tabctl = this;
    //setClipWidth(l_page, getClipWidth(this)-2);
    //setClipHeight(l_page, getClipHeight(this)-2);

    // Indicate that we generate reveal/obscure notifications
    l_page.Reveal = tc_cb_reveal;
    pg_reveal_register_triggerer(l_page);
    //if (htr_getvisibility(l_page) == 'inherit') pg_addsched("pg_reveal(" + l_tab.tabname + ")");
    //l_page.is_visible = (l.tloc != 4 && htr_getvisibility(l_tab) == 'inherit');
    l_page.is_visible = true;

    l_page.tc_visible_changed = tc_visible_changed;
    htr_watch(l_page,"is_visible", "tc_visible_changed"); //visible property
    var iv = l_page.ifcProbeAdd(ifValue);
    iv.Add("visible", "is_visible");

    // Show Container API
    l_page.showcontainer = tc_showcontainer;
    l_tab.showcontainer = tc_showcontainer;

    return l_page;
    }

function tc_selection_changed(prop,o,n)
    {
    var tabindex = null;
    if (o == n) return n;
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
    if (this.selchange_schedid)
	pg_delsched(this.selchange_schedid);
    this.selchange_schedid = pg_addsched_fn(this,"ChangeSelection1", new Array(this.tabs[tabindex-1]), 0);
    return n;
    }

function tc_action_set_tab(aparam)
    {
    if (aparam.Tab) this.selected = aparam.Tab;
    else if (aparam.TabIndex) this.selected_index = parseInt(aparam.TabIndex);
    }

function tc_showcontainer()
    {
    var pg = this.tabpage?this.tabpage:this;
    if (htr_getvisibility(pg) != 'inherit')
	{
	this.tabctl.ChangeSelection1(pg);
	}
    return true;
    }

function tc_clear_tabs(tabs)
    {
    for(var i=0;i<tabs.length;i++)
	{
	if(tabs[i].type=='generated')
	    {
	    //setClipWidth(tabs[i],0);
	    if(!tabs[i].tabctl.tc_layer_cache) tabs[i].tabctl.tc_layer_cache = new Array();
	    tabs[i].tabctl.tc_layer_cache.push(tabs[i]);
	    tabs.splice(i,1);
	    i--; //because we just removed an element
	    }
	}
    }

function tc_direct_parent(t)
    {
    if(cx__capabilities.Dom0NS)
	return t.parentLayer;
    else
	return t.parentNode;
    }

function tc_updated(p1)
    {
    var osrc = this.osrc;
    var tabs = this.tabs;
    var vals = new Array();
    var targetval,targettab;

    if(this.oldreplica && this.osrc.replica == this.oldreplica){
	//replica is the same so just switch tabs!
	for(var i in tabs)
	    if(tabs[i].recordnumber == osrc.CurrentRecord)
		{
		tabs[i].makeCurrent();
		tabs[i].tc_visible_changed('visible','hidden','inherit');
		return; //done!
		}
    }
    else this.oldreplica = this.osrc.replica;
    tc_clear_tabs(this.tabs);
    for(var i in tabs)
	{
	if(tabs[i].type!='dynamic')
	    continue; //ignore non-dynamic tabs
	//htr_setvisibility(tabs[i],'inherit');
	htr_setvisibility(tabs[i],'hidden');
	for(var j in osrc.replica)
	    {
	    var rec = osrc.replica[j];
	    for(var k in rec)
		{
		if(rec[k].oid == tabs[i].fieldname)
		    {
		    vals[j] = rec[k].value;
		    if(j==osrc.CurrentRecord) targetval = j;
		    break; //this makes sure we don't do duplicates
		    }
		}
	    }
	for(var j in vals)
	    {
	    var newtab,tabparent,pageparent,newpage,content;
	    //make sure there is never a tbody tag in content for NS4, ever.
	    if(cx__capabilities.Dom0NS)
		content = "\n    <table cellspacing=0 cellpadding=0 border=0><tr><td colspan=3 background=\"/sys/images/white_1x1.png\"><img src=\"/sys/images/white_1x1.png\"></td></tr><tr><td width=6><img src=\"/sys/images/white_1x1.png\" heigth=24 width=1><img src=\"/sys/images/tab_lft3.gif\" name=\"tb\" heigth=24></td><td valign=\"middle\" align=\"center\"><font color=\"black\"><b>&nbsp;"+vals[j]+"&nbsp;</b></font></td><td align=\"right><img src=\"/sys/images/dkgrey_1x1.png\" width=1 height=24></td></tr></table>\n";
	    else
		content = "\n    <table style=\"border-style: solid; border-color: white gray gray white; border-width: 1px 1px 0px;\" border=\"0\" cellpadding=\"0\" cellspacing=\"0\"><tr><td><img src=\"/sys/images/tab_lft3.gif\" align=\"left\" height=\"24\" width=\"5\"></td><td align=\"center\"><b>&nbsp;"+vals[j]+"&nbsp;</b></td></tr></table>\n";
	    tabparent = tc_direct_parent(tabs[i]);
	    if(this.tc_layer_cache && this.tc_layer_cache.length >0) newtab = this.tc_layer_cache.pop();
	    else newtab = htr_new_layer(null,tabparent);
	    pageparent = tc_direct_parent(tabs[i].tabpage)
	    newpage = htr_new_layer(null,pageparent);
	    newtab.marker_image = tabs[i].marker_image;
	    newtab.marker_image.src = '/sys/images/tab_lft3.gif';
	    $(newtab).find('span').text('&nbsp;' + htutil_encode(vals[j]) + '&nbsp;');
	    //htr_write_content(newtab,content);
	    htr_setvisibility(newtab,'inherit');
	    htr_setvisibility(newpage,'inherit');
	    htr_setzindex(newtab,14);
	    this.addTab(newtab,newpage,this,vals[j],'generated','');
	    //setClipWidth(newtab,htr_getphyswidth(newtab));
	    //setClipHeight(newtab,26);
	    
	    newpage.osrcdata = vals[j];
	    newpage.recordnumber = j;

	    tc_makenotcurrent(newpage);
	    if(j==targetval) targettab = newpage.tabindex-1;

	    if(targettab)
		{
		this.tabs[targettab].makeCurrent();
		this.tabs[targettab].tc_visible_changed('visible','hidden','inherit');
		}
	    }
	}
    }

function tc_init(param)
    {
    // Basic stuff...
    var l = param.layer; 
    htr_init_layer(l,l,'tc');
    ifc_init_widget(l);
    l.tabs = new Array();
    l.addTab = tc_addtab;
    l.current_tab = null;
    l.init_tab = null;
    l.tloc = param.tloc;
    if (tc_tabs == null) tc_tabs = new Array();
    tc_tabs[tc_tabs.length++] = l;

    // Background color/image selection...
    l.main_bgColor = htr_extract_bgcolor(param.mainBackground);
    l.main_bgnd = htr_extract_bgimage(param.mainBackground);
    l.inactive_bgColor = htr_extract_bgcolor(param.inactiveBackground);
    l.inactive_bgnd = htr_extract_bgimage(param.inactiveBackground);

    // Properties available to other widgets, that can be used to
    // change current tab index as well.
    l.selected = null;
    l.selected_index = 1;
    htr_watch(l,"selected", "tc_selection_changed");
    htr_watch(l,"selected_index", "tc_selection_changed");
    l.tc_selection_changed = tc_selection_changed;
    l.selchange_schedid = null;

    l.osrc = wgtrFindContainer(l, "widget/osrc");
    if(l.osrc)
	{
	l.osrc.Register(l);
	l.IsDiscardReady=new Function('return true');
	l.DataAvailable=new Function('return true');
	l.ObjectAvailable=tc_updated;
	l.ReplicaMoved=new Function('return true');
	l.OperationComplete=new Function('return true');
	l.ObjectDeleted=tc_updated;
	l.ObjectCreated=tc_updated;
	l.ObjectModified=tc_updated;
	}

    // Actions
    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("SetTab", tc_action_set_tab);

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("TabChanged");
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");

    // Reveal/Obscure mechanism
    l.ChangeSelection1 = tc_changeselection_1;
    l.ChangeSelection2 = tc_changeselection_2;
    l.ChangeSelection3 = tc_changeselection_3;

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
	case 4: // none
	    l.xo = 0;
	    l.yo = 0;
	    l.cl = "bottom";
	    l.ci = 0;
	    break;
	}
    return l;
    }

function tc_visible_changed(prop,o,n)
    {
    var t = this.tabctl;
    var xo = t.xo;
    var yo = t.yo;
    if(n) htr_setvisibility(this.tab, 'inherit');
    else htr_setvisibility(this.tab, 'hidden');
    // which tab should be selected? 
    if(htr_getvisibility(t.tabs[t.selected_index-1].tab)!='inherit')
	{
	//try default tab
	if(htr_getvisibility(t.init_tab.tab)=='inherit')
	    {
	    // This is forced, so we skip the obscure/reveal checks
	    t.ChangeSelection3(t.tabs[t.init_tab.tabindex-1]);
	    //t.tabs[t.init_tab.tabindex-1].makeCurrent();
	    }
	else //otherwise find first tab not hidden
	    {
	    for(var i=0; i<t.tabs.length;i++)
		{
		if(htr_getvisibility(t.tabs[i].tab)=='inherit')
		    {
		    // This is forced, so we skip the obscure/reveal checks
		    t.ChangeSelection3(t.tabs[i]);
		    //t.tabs[i].makeCurrent();
		    break;
		    }
		}
	    }
	}
    
    if(this.tabctl.tloc == 2) //left
	{
	var currx = getRelativeX(t)-$(t.tabs[0].tab).width()+4, curry = getRelativeY(t); //initial values
	for(var i = 0; i< this.tabctl.tabs.length; i++)
	    {
	    if(htr_getvisibility(this.tabctl.tabs[i].tab)=='inherit')
		{
		if(this.tabctl.selected_index-1 == i) currx-=xo; //stick out
		moveTo(this.tabctl.tabs[i].tab,currx,curry);
		curry+=$(this.tabctl.tabs[i].tab).height()+1;
		if(this.tabctl.selected_index-1 == i)
		    {
		    curry+=yo; currx+=xo;
		    }
		}
	    }
	}
    else if(this.tabctl.tloc == 0) //top
	{
	var currx = getPageX(t), curry = getPageY(t)-22; //currently height is fixed at 26
	for(var i in this.tabctl.tabs)
	    {
	    if(htr_getvisibility(this.tabctl.tabs[i].tab)=='inherit')
		{
		if(this.tabctl.selected_index-1 == i) curry-=yo; //stick out
		moveToAbsolute(this.tabctl.tabs[i].tab,currx,curry);
		currx+=$(this.tabctl.tabs[i].tab).width()+2;
		if(this.tabctl.selected_index-1 == i)
		    {
		    currx+=xo; curry+=yo;
		    }
		}
	    }
	}
    else if(this.tabctl.tloc == 3) //right
	{
	var currx = getRelativeX(t)+$(t).width()-3, curry = getRelativeY(t); //currently height is fixed at 26
	for(var i = 0; i< this.tabctl.tabs.length; i++)
	    {
	    if(htr_getvisibility(this.tabctl.tabs[i].tab)=='inherit')
		{
		if(this.tabctl.selected_index-1 == i) currx-=xo; //stick out
		moveTo(this.tabctl.tabs[i].tab,currx,curry);
		curry+=$(this.tabctl.tabs[i].tab).height()+1;
		if(this.tabctl.selected_index-1 == i)
		    {
		    curry+=yo; currx+=xo;
		    }
		}
	    }
	}
    else //bottom
	{    
	var currx = getRelativeX(t), curry = getRelativeY(t)+$(t).height()-3; //currently height is fixed at 26
	for(var i = 0; i< this.tabctl.tabs.length; i++)
	    {
	    if(htr_getvisibility(this.tabctl.tabs[i].tab)=='inherit')
		{
		if(this.tabctl.selected_index-1 == i) curry-=yo; //stick out
		moveTo(this.tabctl.tabs[i].tab,currx,curry);
		currx+=$(this.tabctl.tabs[i].tab).width()+2;
		if(this.tabctl.selected_index-1 == i)
		    {
		    currx+=xo; curry+=yo;
		    }
		}
	    }
	}
    
    }

// Reveal() interface function - called when a triggerer event occurs.
// c == tab (not tabpage) to make current.
// this == tab (not tabpage) event (revealcheck/obscurecheck) was processed for.
function tc_cb_reveal(e)
    {
    switch(e.eventName)
	{
	case 'ObscureOK':
	    this.tabctl.ChangeSelection2(e.c);
	    break;
	case 'RevealOK':
	    this.tabctl.ChangeSelection3(e.c);
	    break;
	case 'ObscureFailed':
	case 'RevealFailed':
	    this.tabctl.current_tab.setTabUnwatched();
	    break;
	}
    return true;
    }

// change selection - first function, called initially.
function tc_changeselection_1(c)
    {
    this.selchange_schedid = null;
    pg_reveal_event(c.tabctl.current_tab, c, 'ObscureCheck');
    }
// change selection - second function, called when ObscureCheck succeeds.
function tc_changeselection_2(c)
    {
    pg_reveal_event(c, c, 'RevealCheck');
    }
// change selection - third function, called when RevealCheck succeeds.
function tc_changeselection_3(c)
    {
    pg_reveal_event(c, c, 'Reveal');
    pg_reveal_event(c.tabctl.current_tab, c, 'Obscure');
    if(c.recordnumber)
	{
	var osrc = this.osrc;
	osrc.MoveToRecord(c.recordnumber);
	//we don't need to make this current because it will get selected
	//automatically when the record moves
	}
    else
	c.makeCurrent();
    }

function tc_mousedown(e)
    {
    if (e.mainlayer && e.mainkind == 'tc')
	 cn_activate(e.mainlayer, 'MouseDown');
    if (e.kind == 'tc' && e.layer.tabctl)
	 e.layer.tabctl.ChangeSelection1(e.layer.tabpage);
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tc_mouseup(e)
    {
    if (e.mainlayer && e.mainkind == 'tc') 
	cn_activate(e.mainlayer, 'MouseUp');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tc_mousemove(e)
    {
    if (!e.mainlayer || e.mainkind != 'tc')
        {
        if (tc_cur_mainlayer) cn_activate(tc_cur_mainlayer, 'MouseOut');
            tc_cur_mainlayer = null;
        }
    else if (e.kind && e.kind.substr(0,2) == 'tc')
	{
        cn_activate(e.mainlayer, 'MouseMove');
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tc_mouseover(e)
    {
    if (e.kind && e.kind.substr(0,2) == 'tc')
        {
        cn_activate(e.mainlayer, 'MouseOver');
        tc_cur_mainlayer = e.mainlayer;
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_tab.js'] = true;
