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
    const { tabctl: t, tab } = this;
    const { tabs, tloc } = t;

    if (tloc !== 'None' && htr_getzindex(tab) > htr_getzindex(t)) return 0;
    for(let i = 0; i < tabs.length; i++)
	{
	const cur = tabs[i];
	if (cur !== this && (cur.tabctl.tloc === 'None' || htr_getzindex(cur.tab) > htr_getzindex(tab)))
	    {
	    htr_setzindex(cur, htr_getzindex(t) - 1);
	    htr_setvisibility(cur, 'hidden');
	    if (cur.tabctl.tloc !== 'None')
		{
		htr_setzindex(cur.tab, htr_getzindex(t) - 1);
		cur.tab.marker_image.src = '/sys/images/tab_lft3.gif';
		cur.tab.classList.remove('tab_selected');
		// moveBy(cur.tab, t.xo, t.yo);
		if (t.inactive_bgColor) htr_setbgcolor(cur.tab, t.inactive_bgColor);
		if (t.inactive_bgnd) htr_setbgimage(cur.tab, t.inactive_bgnd);
		}
	    }
	}
    htr_setzindex(this, htr_getzindex(t) + 1);
    htr_setvisibility(this,'inherit');
    if (tloc !== 'None')
	{
	if (t.main_bgColor) htr_setbgcolor(tab, t.main_bgColor);
	if (t.main_bgnd) htr_setbgimage(tab, t.main_bgnd);
	htr_setzindex(tab, htr_getzindex(t) + 1);
	tab.marker_image.src = '/sys/images/tab_lft2.gif';
	tab.classList.add('tab_selected');
	// moveBy(tab, -t.xo, -t.yo);
	}
    this.setTabUnwatched();
    t.ifcProbe(ifEvent).Activate("TabChanged", { Selected:t.selected, SelectedIndex:t.selected_index });
    }

function tc_makenotcurrent(page)
    {
    const { tabctl, tab } = page;

    htr_setzindex(page, htr_getzindex(tabctl) - 1);
    htr_setvisibility(page, 'hidden');
    
    if (tabctl.tloc !== 'None')
	{
	htr_setzindex(page.tab,htr_getzindex(page.tabctl) - 1);
	tab.marker_image.src = '/sys/images/tab_lft3.gif';
	tab.classList.remove('tab_selected');
	// moveBy(tab, tabctl.xo, tabctl.yo);
	if (tabctl.inactive_bgColor) htr_setbgcolor(tab, tabctl.inactive_bgColor);
	if (tabctl.inactive_bgnd) htr_setbgimage(tab, tabctl.inactive_bgnd);
	}
    }
    
/*** Adds a new tab to the tab control. This function deals with whether or
 *** not that tab is selected as LITTLE AS POSSIBLE since that piece of state
 *** should be handled elsewhere with functions like tc_makenotcurrent() or
 *** tc_makecurrent().
 *** 
 *** @param l_tab The tab being added.
 *** @param l_page The page to which the tab is being added.
 *** @param l The layer where this change will occur.
 *** @param nm The name of the tab.
 ***/
function tc_addtab(l_tab, l_page, l, nm, type, fieldname)
    {
    const tabctl = this, { tabs } = tabctl, { tloc, tab_h, tab_spacing } = l;
    
    let x, y;
    if (!l_tab) l_tab = {};
    l_page.tabname = nm;
    l_page.type = type;
    l_page.fieldname = fieldname;
    l_page.tabindex = tabs.length + 1;
    htr_init_layer(l_page,l,'tc_pn');
    ifc_init_widget(l_page);

    /** Calculate the location and flexibility to render the tab. **/
    if (tloc === 'None')
	{
	x = 0;
	y = 0;
	}
    else
	{
	htr_init_layer(l_tab, l, 'tc');

	/** Calculate x coordinate. **/
	if (tloc === 'Top' || tloc === 'Bottom')
	    {
	    if (tabs.length > 0)
		{
		const previous_tab = tabs[tabs.length - 1].tab;
		x = getRelativeX(previous_tab) + $(previous_tab).outerWidth() + tab_spacing;
		}
	    else if (l.tab_fl_x)
		{
		/** Copy tabctl.style.left to avoid small but noticeable inconsistencies. **/
		setRelativeX(l_tab, tabctl.style.left);
		}
	    else
		{
		/** Math for inflexible tabs do not suffer from the inconsistencies handled above. **/
		x = getRelativeX(tabctl);
		}
	    }
	else if (tloc === 'Left')
	    x = getRelativeX(tabctl) - htr_getviswidth(l_tab); 
	else // Right
	    x = getRelativeX(tabctl); // + htr_getviswidth(tabctl) // Included in xtoffset (see below)

	/** Calculate y coordinate. **/
	if (tloc === 'Left' || tloc === 'Right')
	    {
	    if (tabs.length > 0)
		{
		const previous_tab = tabs[tabs.length - 1].tab;
		y = getRelativeY(previous_tab) + tab_h + tab_spacing;
		}
	    else if (l.tab_fl_y)
		/** Copy tabctl.style.top to avoid small but noticeable inconsistencies. **/
		setRelativeY(l_tab, tabctl.style.top);
	    else
		/** Math for inflexible tabs do not suffer from inconsistencies. * */
		y = getRelativeY(tabctl);
	    }
	else if (tloc === 'Bottom')
	    y = getRelativeY(tabctl); // + htr_getvisheight(tabctl) // Included in ytoffset (see below)
	else // Top
	    y = getRelativeY(tabctl) - tab_h;

	/** Apply the same tab offsets used on the server. **/
	x += l.xtoffset;
	y += l.ytoffset;
	
	/** Space out tab away from previous tab to account for borders. **/
	if (tabs.length > 0)
	    {
	    switch (tloc)
		{
		case 'Top': case 'Bottom': x += 2; break;
		case 'Left': case 'Right': y += 2; break;
		}
	    }
	}

    if (htr_getvisibility(l_page) === 'inherit')
	{
	htr_unwatch(l,"selected","tc_selection_changed");
	htr_unwatch(l,"selected_index","tc_selection_changed");
	l.selected = l_page.tabname;
	l.selected_index = l_page.tabindex;
	l.current_tab = l_page;
	l.init_tab = l_page;
	pg_addsched_fn(window,"pg_reveal_event",[l_page,l_page,'Reveal'], 0);
	htr_watch(l,"selected", "tc_selection_changed");
	htr_watch(l,"selected_index", "tc_selection_changed");
	if (tloc !== 'None')
	    {
	    if (l.main_bgColor) htr_setbgcolor(l_tab, l.main_bgColor);
	    if (l.main_bgnd) htr_setbgimage(l_tab, l.main_bgnd);
	    }
	}
    
    if (tloc !== 'None')
	{
	const images = pg_images(l_tab);
	for (let i = 0; i < images.length; i++)
	    {
	    const image = images[i];
	    image.layer = l_tab;
	    image.kind = 'tc';
	    if (image.width === 5) l_tab.marker_image = image;
	    }
	}
    
    tabs[tabs.length++] = l_page;
    l_page.tabctl = tabctl;
    l_page.tab = l_tab;
    l_page.makeCurrent = tc_makecurrent;
    l_page.setTabUnwatched = tc_set_tab_unwatched;
    l_tab.tabpage = l_page;
    l_tab.tabctl = tabctl;

    /** Render the tab at the location calculated above. **/
    if (tloc !== 'None' && l.do_rendering)
	{
	/*** Get the parent width and height.  The top-level body element is
	 *** sized wrong, so we use the window size if it is the parent.
	 ***/
	const is_top_level = (l_tab.parentElement.tagName === 'BODY');
	const style    = (is_top_level) ? undefined          : getComputedStyle(l_tab.parentElement);
	const parent_w = (is_top_level) ? innerWidth  + 'px' : style.width;
	const parent_h = (is_top_level) ? innerHeight + 'px' : style.height;
	if (x) setRelativeX(l_tab, `calc(${x}px + (100% - ${parent_w}) * ${l.tab_fl_x})`);
	if (y) setRelativeY(l_tab, `calc(${y}px + (100% - ${parent_h}) * ${l.tab_fl_y})`);
	}

    // Indicate that we generate reveal/obscure notifications
    l_page.Reveal = tc_cb_reveal;
    pg_reveal_register_triggerer(l_page);
    //if (htr_getvisibility(l_page) == 'inherit') pg_addsched("pg_reveal(" + l_tab.tabname + ")");
    //l_page.is_visible = (tloc !== 'None' && htr_getvisibility(l_tab) == 'inherit');
    l_page.is_visible = true;

    l_page.tc_visible_changed = tc_visible_changed;
    htr_watch(l_page,"is_visible", "tc_visible_changed"); //visible property
    const iv = l_page.ifcProbeAdd(ifValue);
    iv.Add("visible", "is_visible");

    // Show Container API
    l_page.showcontainer = tc_showcontainer;
    l_tab.showcontainer = tc_showcontainer;

    return l_page;
    }

function tc_selection_changed(prop,o,n)
    {
    const { tabs } = this;

    var tabindex = null;
    if (o == n) return n;
    // find index if name specified
    if (prop == 'selected')
	{
	for (let i = 0; i < tabs.length; i++)
	    if (tabs[i].tabname === n)
		{
		tabindex = i + 1;
		break;
		}
	}
    else
	tabindex = n;
    if (tabindex < 1 || tabindex > tabs.length) return o;

    // okay to change tab.
    //tabs[tabindex-1].makeCurrent();
    if (this.selchange_schedid)
	pg_delsched(this.selchange_schedid);
    this.selchange_schedid = pg_addsched_fn(this, "ChangeSelection1", new Array(tabs[tabindex - 1]), 0);
    return n;
    }

function tc_action_set_tab({ Tab, TabIndex })
    {
    if (Tab) this.selected = Tab;
    else if (TabIndex) this.selected_index = parseInt(TabIndex);
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
    for (let i = 0; i < tabs.length; i++)
	{
	const cur = tabs[i];
	if (cur.type === 'generated')
	    {
	    if (!cur.tabctl.tc_layer_cache) cur.tabctl.tc_layer_cache = [];
	    cur.tabctl.tc_layer_cache.push(cur);
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
    const { osrc, tabs } = this;
    const vals = [];
    let targetval, targettab;

    if(this.oldreplica && osrc.replica === this.oldreplica){
	//replica is the same so just switch tabs!
	for(var i in tabs)
	    {
	    const cur_tab = tabs[i];
	    if(cur_tab.recordnumber === osrc.CurrentRecord)
		{
		cur_tab.makeCurrent();
		cur_tab.tc_visible_changed('visible','hidden','inherit');
		return; //done!
		}
	    }
    }
    else this.oldreplica = osrc.replica;

    tc_clear_tabs(tabs);
    for(var i in tabs)
	{
	const cur_tab = tabs[i];
	if(cur_tab.type !== 'dynamic') continue; //ignore non-dynamic tabs
	
	//htr_setvisibility(cur_tab,'inherit');
	htr_setvisibility(cur_tab,'hidden');
	for(var j in osrc.replica)
	    {
	    var rec = osrc.replica[j];
	    for(var k in rec)
		{
		if(rec[k].oid === cur_tab.fieldname)
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
		content =
		    '\n\t<table ' +
			'style="' +
			    'border-style: solid; ' + 
			    'border-color: white gray gray white; ' + 
			    'border-width: 1px 1px 0px; ' +
			'"' +
			'border="0" ' +
			'cellpadding="0" ' +
			'cellspacing="0" ' +
		    '><tr>' +
			'<td><img src="/sys/images/tab_lft3.gif" align="left" height="24" width="5"></td>' +
			'<td align="center"><b>&nbsp;' + vals[j] + '&nbsp;</b></td>' +
		    '</tr></table>\n';
	    
	    tabparent = tc_direct_parent(cur_tab);
	    if(this.tc_layer_cache && this.tc_layer_cache.length >0) newtab = this.tc_layer_cache.pop();
	    else newtab = htr_new_layer(null,tabparent);
	    pageparent = tc_direct_parent(cur_tab.tabpage)
	    newpage = htr_new_layer(null,pageparent);
	    newtab.marker_image = cur_tab.marker_image;
	    newtab.marker_image.src = '/sys/images/tab_lft3.gif';
	    $(newtab).find('span').text('&nbsp;' + htutil_encode(vals[j]) + '&nbsp;');
	    //htr_write_content(newtab,content);
	    htr_setvisibility(newtab,'inherit');
	    htr_setvisibility(newpage,'inherit');
	    htr_setzindex(newtab,14);
	    this.addTab(newtab,newpage,this,vals[j],'generated','');
	    
	    newpage.osrcdata = vals[j];
	    newpage.recordnumber = j;

	    tc_makenotcurrent(newpage);
	    if(j==targetval) targettab = newpage.tabindex-1;

	    if(targettab)
		{
		tabs[targettab].makeCurrent();
		tabs[targettab].tc_visible_changed('visible','hidden','inherit');
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
    l.tabs = [];
    l.addTab = tc_addtab;
    l.current_tab = null;
    l.init_tab = null;
    l.do_rendering = param.do_client_rendering;
    l.select_x_offset = param.select_x_offset;
    l.select_y_offset = param.select_y_offset;
    l.xtoffset = param.xtoffset;
    l.ytoffset = param.ytoffset;
    l.tab_spacing = param.tab_spacing;
    l.tab_h = param.tab_h;
    if (tc_tabs == null) tc_tabs = [];
    tc_tabs[tc_tabs.length++] = l;
    l.tloc = param.tloc;

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

    return l;
    }

/*** Idk what this does...
 ***
 *** @param prop Unused, for some reason.
 *** @param o Unused again...
 *** @param n If this is true, it makes this.tab inherit visibility. Otherwise, hidden.
 *** @returns nothing... idk what this is doing.
 */
function tc_visible_changed(prop, o, n)
    {
    const { tabctl: t } = this;
    const { tabs, tloc } = t;

    if (tloc === 'None') console.warn("tc_visible_changed() called on tab contol with tab_location = none.");

    if(n) htr_setvisibility(this.tab, 'inherit');
    else htr_setvisibility(this.tab, 'hidden');

    /** This nonsense is why we need goto in js. **/
    const pickSelectedTab = () =>
	{
	// If a visible tab is already selected, we're done.
	const selected = tabs[t.selected_index-1];
	if (htr_getvisibility(selected.tab) === 'inherit') return;
	
	// Try to select the initial tab.
	const initial = t.init_tab;
	if (htr_getvisibility(initial.tab) === 'inherit')
	    {
	    // This is forced, so we skip the obscure/reveal checks.
	    t.ChangeSelection3(tabs[initial.tabindex - 1]);
	    return;
	    }
	
	// Otherwise, pick the first visible tab.
	for (let i = 0; i < tabs.length; i++)
	    {
	    const cur_tab = tabs[i];
	    if (htr_getvisibility(cur_tab.tab) === 'inherit')
		{
		// This is forced, so we skip the obscure/reveal checks.
		t.ChangeSelection3(cur_tab);
		return;
		}
	    }
	}
    pickSelectedTab();

    // Determine the initial values for cur_x and cur_y.
    let cur_x, cur_y;
    switch (tloc)
	{
	case 'Top':
	    // Idk why top uses getPage instead of getRelative.
	    // They're inside containers too, aren't they?
	    cur_x = getPageX(t);
	    cur_y = getPageY(t) - 22; // currently height is fixed at 26
	    break;
	case 'Bottom':
	    cur_x = getRelativeX(t);
	    cur_y = getRelativeY(t) + $(t).height() - 3; // currently height is fixed at 26
	    break;
	case 'Left':
	    cur_x = getRelativeX(t) - $(tabs[0].tab).width() + 4;
	    cur_y = getRelativeY(t); // initial values
	    break;
	case 'Right':
	    cur_x = getRelativeX(t) + $(t).width() - 3;
	    cur_y = getRelativeY(t); // currently height is fixed at 26
	    break;
	}

    for (let i = 0; i < tabs.length; i++)
	{
	const cur_tab = tabs[i].tab;

	/** If the tab isn't visible, skip it. **/
	if(htr_getvisibility(cur_tab) !== 'inherit') continue;

	/** Update the class for CSS if the tab is selected. **/
	if(t.selected_index === i + 1) cur_tab.classList.add('tab_selected');
	
	/** Update tab location. **/
	if (tloc === 'Top') moveToAbsolute(cur_tab, cur_x, cur_y); // idk why Top needs special treatment..
	else moveTo(cur_tab, cur_x, cur_y);

	/** Update cur_x or cur_y. **/
	if(tloc === 'Top' || tloc === 'Bottom')
	     cur_x += $(cur_tab).width() + 2; // Top or Bottom
	else cur_y += $(cur_tab).height() + 1; // Left or Right
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
