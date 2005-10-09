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

// define a value for a cell in the menu.  This does not physically add
// the item, just lets us know what its value is.
function mn_additem(param)
    {
    var item = new Object();
    var id;
    item.value = param.value;
    item.check = param.check;
    item.submenu = param.submenu;
    item.icon = param.icon;
    item.enabled = param.enabled;
    item.onright = param.onright;
    if (item.onright)
	{
	id = this.coords.length - this.n_last - 2;
	this.n_last++;
	}
    else
	{
	id = this.n_first;
	this.n_first++;
	}
    if (item.check != null) 
	{
	item.ckbox = this.ckboxs[id];
	}
    if (this.horiz)
	{
	item.width = Math.abs(this.coords[id].x - this.coords[id+1].x)+1;
	item.height = this.act_h - 6;
	item.x = Math.min(this.coords[id].x, this.coords[id+1].x);
	item.y = cx__capabilities.Dom0NS?3:2;
	}
    else
	{
	item.width = this.act_w - 6 + (cx__capabilities.Dom0NS?0:2);
	item.height = Math.abs(this.coords[id].y - this.coords[id+1].y)+1;
	item.x = cx__capabilities.Dom0NS?3:2;
	item.y = this.coords[id].y - 1;
	}
    this.items.push(item);
    return item;
    }


function mn_highlight(item, actv)
    {
    if (mn_current && mn_current != this)
	mn_current.UnHighlight();
    mn_current = this;
    if (actv)
	{
	if (this.active_bgimage) htr_setbgimage(this.hlayer, this.active_bgimage);
	if (this.active_bgcolor) htr_setbgcolor(this.hlayer, this.active_bgcolor);
	}
    else
	{
	if (this.highlight_bgimage) htr_setbgimage(this.hlayer, this.highlight_bgimage);
	if (this.highlight_bgcolor) htr_setbgcolor(this.hlayer, this.highlight_bgcolor);
	}
    resizeTo(this.hlayer, item.width, item.height);
    moveTo(this.hlayer, item.x, item.y);
    htr_setvisibility(this.hlayer,"inherit");
    }


function mn_unhighlight()
    {
    if (!this.cur_highlight) return;
    if (this.nextActive) return;
    htr_setvisibility(this.hlayer, "hidden");
    this.cur_highlight = null;
    }


function mn_check_unhighlight()
    {
    if (mn_current == this) return;
    this.UnHighlight();
    }


function mn_popup(aparam)
    {
    this.DeactivateAll();
    this.Activate(aparam.X, aparam.Y, document);
    return true;
    }


function mn_activate(x,y,p)
    {
    if (!this.popup) return;
    var found = false;
    for(var i=0;i<mn_active.length;i++)
	{
	if (mn_active[i] == this)
	    {
	    found = true;
	    break;
	    }
	}
    if (!found) mn_active.push(this);
    moveTo(this, x, y);
    pg_stackpopup(this, p);
    this.nextActive = null;
    if (p.kind == "mn") p.nextActive = this;
    htr_setvisibility(this, "inherit");
    if (!mn_current) mn_current = this;
    return;
    }


function mn_activate_item(item)
    {
    this.Highlight(item, true);
    if (item.submenu)
	{
	if (this.VChildren[item.submenu])
	    {
	    if (this.horiz)
		{
		var x = getPageX(this) + item.x;
		var y = getPageY(this) + item.y + item.height;
		}
	    else
		{
		var x = getPageX(this) + item.x + item.width;
		var y = getPageY(this) + item.y;
		}
	    this.VChildren[item.submenu].Activate(x, y, this);
	    }
	}
    else if (item.check != null)
	{
	item.check = !item.check;
	if (item.check)
	    item.ckbox.src = "/sys/images/checkbox_checked.gif";
	else
	    item.ckbox.src = "/sys/images/checkbox_unchecked.gif";
	if (this.popup) pg_addsched_fn(this, "Deactivate", [],0);
	}
    else
	{
	if (!mn_tmout) mn_tmout = pg_addsched_fn(this, "DeactivateAll", [], 300);
	}
    }


function mn_deactivate()
    {
    if (this.nextActive) 
	{
	this.nextActive.Deactivate();
	this.nextActive = null;
	}
    this.UnHighlight();
    if (this.popup) htr_setvisibility(this, "hidden");
    if (this.popup) for(var i=0;i<mn_active.length;i++)
	{
	if (mn_active[i] == this)
	    {
	    mn_active.splice(i, 1);
	    break;
	    }
	}
    return;
    }


function mn_deactivate_all()
    {
    for(var i=0;i<mn_active.length;i++)
	mn_active[i].Deactivate();
    }


function mn_mousemove(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (mn_current)
	{
	var found = false;
	ly = mn_current;
	var x = e.pageX - getPageX(ly);
	var y = e.pageY - getPageY(ly);
	for(var i = 0; i < ly.items.length; i++)
	    {
	    if (x >= ly.items[i].x && x <= ly.items[i].x + ly.items[i].width && y >= ly.items[i].y && y <= ly.items[i].y + ly.items[i].height)
		{
		if (ly.cur_highlight != ly.items[i])
		    {
		    var activated = false;
		    if (ly.nextActive) 
			{
			ly.nextActive.Deactivate();
			ly.nextActive = null;
			activated = true;
			}
		    ly.UnHighlight();
		    ly.cur_highlight = ly.items[i];
		    ly.Highlight(ly.items[i], false);
		    if (mn_tmout) pg_delsched(mn_tmout);
		    mn_tmout = null;
		    if (activated && ly.items[i].submenu)
			ly.ActivateItem(ly.items[i]);
		    }
		found = true;
		break;
		}
	    }
	if (!found) 
	    {
	    ly.UnHighlight();
	    }
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function mn_mouseout(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.kind == "mn")
	{
	ly = ly.mainlayer;
	if (mn_current == ly)
	    {
	    mn_current = null;
	    pg_addsched_fn(ly, "CkUnHighlight", [],0);
	    //if (!mn_tmout) mn_tmout = pg_addsched_fn(ly, "DeactivateAll", [], 300);
	    }
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function mn_mouseover(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (ly.kind == "mn")
	{
	ly = ly.mainlayer;
	mn_current = ly;
	if (mn_tmout) pg_delsched(mn_tmout);
	mn_tmout = null;
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function mn_mousedown(e)
    {
    var ly = (typeof e.target.layer != "undefined" && e.target.layer != null)?e.target.layer:e.target;
    if (mn_current && mn_current.cur_highlight && (mn_current == ly.mainlayer || ly == document))
	{
	if (mn_current.nextActive)
	    {
	    mn_current.nextActive.Deactivate();
	    mn_current.nextActive = null;
	    mn_current.Highlight(mn_current.cur_highlight, false);
	    }
	else
	    mn_current.ActivateItem(mn_current.cur_highlight);
	}
    else
	{
	mn_deactivate_all();
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

// initialization
function mn_init(param)
    {
    // Setup the layer...
    var menu = param.layer;
    menu.clayer = param.clayer;
    menu.hlayer = param.hlayer;
    htr_init_layer(menu, menu, "mn");
    htr_init_layer(menu.clayer, menu, "mn");
    htr_init_layer(menu.hlayer, menu, "mn");
    menu.main_bgimage = htr_extract_bgimage(param.bgnd);
    menu.main_bgcolor = htr_extract_bgcolor(param.bgnd);
    menu.highlight_bgimage = htr_extract_bgimage(param.high);
    menu.highlight_bgcolor = htr_extract_bgcolor(param.high);
    menu.active_bgimage = htr_extract_bgimage(param.actv);
    menu.active_bgcolor = htr_extract_bgcolor(param.actv);
    menu.textcolor = param.txt;
    menu.spec_w = param.w;
    menu.spec_h = param.h;
    menu.horiz = param.horiz;
    menu.popup = param.pop;
    menu.objname = param.name;
    menu.cur_highlight = null;

    if (cx__capabilities.CSS2)
	{
	pg_set_style(menu,'height',menu.scrollHeight);
	pg_set_style(menu,'width',menu.scrollWidth);
	}
    menu.act_w = getClipWidth(menu.clayer);
    menu.act_h = getClipHeight(menu.clayer);
    if (cx__capabilities.CSSBox) menu.act_h += 2;
    htutil_tag_images(menu.clayer, "mn", menu.clayer, menu);

    // Store data to determine cell sizes
    var imgs = pg_images(menu.clayer);
    var nmstr = 'xy_' + param.name;
    menu.coords = new Array();
    menu.ckboxs = new Array();
    for(var i=0; i<imgs.length; i++)
	{
	if (imgs[i].name.substr(0,nmstr.length) == nmstr)
	    {
	    menu.coords.push(new Object());
	    menu.coords[parseInt(imgs[i].name.substr(nmstr.length,255))].x = 
		getRelativeX(imgs[i]);
	    menu.coords[parseInt(imgs[i].name.substr(nmstr.length,255))].y = 
		getRelativeY(imgs[i]);
	    }
	else if (imgs[i].name.substr(0,3) == "cb_")
	    {
	    menu.ckboxs[parseInt(imgs[i].name.substr(3,255))] = imgs[i];
	    }
	}
    menu.items = new Array();
    menu.n_first = 0;
    menu.n_last = 0;

    // Methods
    menu.AddItem = mn_additem;
    menu.Highlight = mn_highlight;
    menu.UnHighlight = mn_unhighlight;
    menu.Activate = mn_activate;
    menu.ActivateItem = mn_activate_item;
    menu.Deactivate = mn_deactivate;
    menu.DeactivateAll = mn_deactivate_all;
    menu.CkUnHighlight = mn_check_unhighlight;

    // Actions
    menu.ActionPopup = mn_popup;

    menu.nextActive = null;
    if (!menu.popup)
	mn_active.push(menu);

    return menu;
    }

