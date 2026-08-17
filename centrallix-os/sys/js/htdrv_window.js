// Copyright (C) 1998-2026 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.


// A resize may move the widget a window is placed against, so every window is
// placed again from its descriptor.  We defer a frame to let the page settle,
// and collapse events to one pass per frame for performance.
let wn_place_pending = false;
window.addEventListener('resize', () => {
    if (wn_place_pending) return;
    wn_place_pending = true;
    requestAnimationFrame(() => {
	wn_place_pending = false;
	wn_place_all();
    });
});

var wn_popped = {};

function wn_deinit()
    {
    // If we moved it to top level, move it back so it can get cleaned up.
    if (this.orig_parent)
	this.orig_parent.appendChild(this);

    // Remove references
    for(var i in wn_list)
	if (wn_list[i] == this)
	    {
	    wn_list.splice(i,1);
	    break;
	    }
    if (wn_current == this)
	wn_current = null;
    if (wn_topwin == this)
	wn_topwin = null;
    }

function wn_init(param)
    {
    var l = param.mainlayer;
    var titlebar = param.titlebar;
    htr_init_layer(l,l,"wn");
    htr_init_layer(param.clayer,l,"wn");
    ifc_init_widget(l);
    l.destroy_widget = wn_deinit;

    // Determine titlebar.
    if (titlebar)
	{
	htr_init_layer(titlebar,l,"wn");
	titlebar.subkind = 'titlebar';
	}
    else
	titlebar = l;
    l.titlebar = titlebar;

    l.keep_kbd_focus = true;
    l.ContentLayer = param.clayer;
    l.ContentLayer.maxheight = l.ContentLayer.minheight = getClipHeight(l.ContentLayer);
    l.ContentLayer.maxwidth = l.ContentLayer.minwidth = getClipWidth(l.ContentLayer);

    l.orig_width = pg_get_style(l,'clip.width');
    l.orig_height = pg_get_style(l,'clip.height');
    l.orig_right = pg_get_style(l,'clip.right');
    l.orig_left = pg_get_style(l,'clip.left');
    l.orig_bottom = pg_get_style(l,'clip.bottom');
    l.orig_top = pg_get_style(l,'clip.top');

    l.gshade = param.gshade;
    l.closetype = param.closetype;
    l.is_modal = param.modal;
    l.working = false;
    l.shaded = false;
    l.loaded = false;

    /** make sure the images are set up **/
    l.has_titlebar = 0;
    for(var i=0;i<pg_images(titlebar).length;i++)
	{
	pg_images(titlebar)[i].layer = titlebar;
	pg_images(titlebar)[i].kind = 'wn';
	if (pg_images(titlebar)[i].name == 'close')
	    l.has_titlebar = 1;
	}

    l.open_params = {};
    l.orig_parent = null;
    l.is_toplevel = false;
    if (param.toplevel == 1)
	{
	var tl = pg_toplevel_layer(l);
	if (tl && tl != window && tl != document && tl != l)
	    {
	    l.orig_parent = l.parentNode;
	    l.is_toplevel = true;
	    var x = getPageX(l);
	    var y = getPageY(l);
	    moveAbove(l, tl);
	    moveToAbsolute(l, x, y);
	    }
	}
    
    /*** Save how the server placed this window (see wn_place() for modes).  Set
     *** before the window joins the wn_list, which is used for resize
     *** re-placements.  centeredx/centeredy say the server-side layout centered
     *** the window rather than placing it at a fixed spot (see apos.c for the
     *** detection), so it gets centered again on resize.  Any other window was
     *** given a fixed spot that CSS already holds, recorded as "wherever it is".
     ***/
    if (param.centeredx === 1 || param.centeredy === 1)
	l.placement = {
	    mode: 'center',
	    centered_x: (param.centeredx === 1),
	    centered_y: (param.centeredy === 1),
	    in_container: !l.is_toplevel,
	    };
    else
	l.placement = { mode:'absolute', x:null, y:null, attract:0 };

    wn_list.push(l);
    wn_bring_top(l);

    // Actions
    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("SetVisibility", wn_setvisibility);
    ia.Add("ToggleVisibility", wn_togglevisibility);
    ia.Add("Open", wn_openwin);
    ia.Add("Close", wn_closewin);
    ia.Add("Popup", wn_popup);
    ia.Add("Shade", wn_action_shade);
    ia.Add("Unshade", wn_action_unshade);
    ia.Add("Point", wn_action_point);

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");
    ie.Add("Load");
    ie.Add("Open");
    ie.Add("Close");

    // Register as a triggerer of reveal/obscure events
    l.SetVisibilityBH = wn_setvisibility_bh;
    l.SetVisibilityTH = wn_setvisibility_th;
    l.Reveal = wn_cb_reveal;
    l.wn_popup_bh = wn_popup_bh;
    pg_reveal_register_triggerer(l);
    pg_reveal_register_listener(l);

    l.is_visible = 0;

    if (htr_getvisibility(l) == 'inherit' || htr_getvisibility(l) == 'visible')
	{
	l.is_visible = 1;
	pg_addsched_fn(window, "pg_reveal_event", [l,l,'Reveal'], 0);
	}

    // force on page...
    if (getPageY(l) + l.orig_height > getInnerHeight())
	moveToAbsolute(l, getPageX(l), getInnerHeight() - l.orig_height - 2);

    // Show container API
    l.showcontainer = wn_showcontainer;

    if (l.is_modal && l.is_visible) pg_setmodal(l, true);

    return l;
    }

function wn_action_point(aparam)
    {
    htr_action_point(this, aparam);
    }

// Popup - pops up a window in the way that a menu might pop up.
function wn_popup(aparam)
    {
    var pop_to = null;
    var pop_to_x = 0;
    var pop_to_y = 0;
    var pop_to_height = 0;
    var pop_to_width = 0;
    var geom = null;

    for (var w in wn_popped)
	{
	wn_popped[w].ifcProbe(ifAction).Invoke("Close", {});
	}

    if (aparam.PopTo)
	{
	if (wgtrIsNode(aparam.PopTo))
	    pop_to = aparam.PopTo;
	else 
	    pop_to = wgtrGetNode(this, aparam.PopTo);
	}
    if (pop_to)
	{
	geom = wgtrGetGeom(pop_to);
	if (geom)
	    {
	    pop_to_x = geom.x;
	    pop_to_y = geom.y;
	    pop_to_width = geom.width;
	    pop_to_height = geom.height;
	    }
	}
    if (aparam.X) pop_to_x = parseInt(aparam.X);
    if (aparam.Y) pop_to_y = parseInt(aparam.Y);

    /** A size given by the action wins over the widget's own, every time. **/
    var width_override = (aparam.Width) ? parseInt(aparam.Width) : null;
    var height_override = (aparam.Height) ? parseInt(aparam.Height) : null;
    if (width_override !== null) pop_to_width = width_override;
    if (height_override !== null) pop_to_height = height_override;

    if (aparam.OffsetX) pop_to_x += parseInt(aparam.OffsetX);
    if (aparam.OffsetY) pop_to_y += parseInt(aparam.OffsetY);

    /*** Record what we popped up against.  The window is not showing yet, so
     *** wn_place() positions it later.  The offsets keep the action's own
     *** adjustments on top of the widget's live geometry.
     ***/
    if (pop_to_x || pop_to_y)
	this.placement = {
	    mode: 'popup',
	    at: (geom) ? pop_to : null,
	    x: pop_to_x,
	    y: pop_to_y,
	    width: pop_to_width,
	    height: pop_to_height,
	    offset_x: (geom) ? pop_to_x - geom.x : 0,
	    offset_y: (geom) ? pop_to_y - geom.y : 0,
	    width_override: width_override,
	    height_override: height_override,

	    /** Tells the Open below that this placement is the one just asked for. **/
	    from_action: true,
	};

    if (aparam.ExtendTo)
	this.extended_region = wgtrGetGeom(aparam.ExtendTo);

    this.popped_above = wgtrGlobalFindContainer(wgtrGetParent(this), "widget/childwindow");
    if (this.popped_above)
	this.popped_above.has_popup = this;

    wn_popped[this.id] = this;
    return this.ifcProbe(ifAction).Invoke('Open',aparam);
    //pg_addsched_fn(this, 'wn_popup_bh', [aparam], 0);
    }

function wn_popup_bh(aparam)
    {
    return this.ifcProbe(ifAction).Invoke('Open',aparam);
    }

// Called when we need to make sure the container is visible, typically
// because a widget within it needs to have keyboard focus.  In the case
// of a window, we un-shade it, make it visible, and bring it to the
// top.
function wn_showcontainer()
    {
    if (htr_getvisibility(this) != 'inherit')
	this.SetVisibilityTH(true);
    if (this.shaded)
	wn_windowshade(this);
    wn_bring_top(this);
    if (this.is_toplevel) return false; /* prevent bubble up if made toplevel */
    return true;
    }

function wn_action_shade()
    {
    if (!this.shaded)
	wn_windowshade(this);
    }

function wn_action_unshade()
    {
    if (this.shaded)
	wn_windowshade(this);
    }

// Called when our reveal/obscure request has been acted upon.
// context 'c' == whether to be visible (true) or not (false).
function wn_cb_reveal(e)
    {
    if ((e.eventName == 'RevealOK' && e.c == true) || (e.eventName == 'ObscureOK' && e.c == false))
	this.SetVisibilityBH(e.c);
    if (e.eventName == 'Reveal' && htr_getvisibility(this) == 'inherit')
	{
	if (!this.loaded)
	    {
	    this.loaded = true;
	    this.ifcProbe(ifEvent).Activate("Load", {});
	    }
	this.ifcProbe(ifEvent).Activate("Open", this.open_params);
	}
    return true;
    }

// Top Half of set visibility routine - before obscure/reveal checks.
function wn_setvisibility_th(v)
    {
    var cur_vis = htr_getvisibility(this);
    if (v && (cur_vis != 'inherit' && cur_vis != 'visible'))
	pg_reveal_event(this, v, 'RevealCheck');
    else if (!v && (cur_vis == 'inherit' || cur_vis == 'visible'))
	pg_reveal_event(this, v, 'ObscureCheck');
    else if (v)
	wn_bring_top(this);
    return;
    }

// Bottom Half of set visibility routine - after obscure/reveal checks.
// this is where we really close it or make it visible.
function wn_setvisibility_bh(v)
    {
    if (!v)
	{
	pg_reveal_event(this, v, 'Obscure');
	this.ifcProbe(ifEvent).Activate("Close", {});
	wn_close(this);
	}
    else
	{
	$(this).css({display:"block"});
	pg_reveal_event(this, v, 'Reveal');
	if (!this.loaded)
	    {
	    this.loaded = true;
	    this.ifcProbe(ifEvent).Activate("Load", {});
	    }
	/** Whichever window is on top until this one takes that spot. **/
	var prev_topwin = wn_topwin;

	wn_bring_top(this);
	htr_setvisibility(this,'inherit');
	this.is_visible = 1;
	if (this.is_modal) pg_setmodal(this, true);
	this.ifcProbe(ifEvent).Activate("Open", this.open_params);

	// Place the window now that it is visible: a pointed window has to be
	// measured, and it cannot be measured while it is hidden.
	wn_place(this);

	/*** Nudge a window clear if it landed exactly on the one below it.  Only
	 *** a window at a plain spot cascades: a centered, pointed, or popped-up
	 *** one sits where it does for a reason.
	 ***/
	if (this.do_cascade && this.placement.mode === 'absolute' && prev_topwin && prev_topwin !== this
	    && getPageX(this) === getPageX(prev_topwin)
	    && getPageY(this) === getPageY(prev_topwin))
	    {
	    moveBy(this, 16, 16);

	    /*** A remembered spot has to be nudged too, or a resize undoes it.  A
	     *** null spot is read live, so it already reads the nudged position.
	     ***/
	    if (this.placement.x != null) this.placement.x += 16;
	    if (this.placement.y != null) this.placement.y += 16;
	    }
	}
    }

function wn_unset_windowshade(l)
    {
    if (cx__capabilities.Dom0IE)
        {
        l = wn_layer;
        }
    l.clicked = 0;
    l.tid = null;
    }

function wn_windowshade_ie(l)
    {
    wn_windowshade(l);
    }

function wn_windowshade_ns_moz(l)
    {
    if (l.clicked == 1)
	{
	if (l.tid) clearTimeout(l.tid);
	l.tid = null;
	l.clicked = 0;
	wn_windowshade(l);
	}
    else
	{
	l.clicked = 1;
	if (l.tid) clearTimeout(l.tid);
	l.tid = setTimeout(wn_unset_windowshade, 500, l);
	}
    }

function wn_windowshade(l)
    {
    // for IE we use the dbl click event, for NS and Moz we use two mousedown's
    var duration = 200;
    var speed = 30;

    /** A shaded window shows nothing but its titlebar. **/
    var shade_height = (l.titlebar !== l) ? $(l.titlebar).outerHeight() : 24;

    if (!l.shaded && !l.working)
	{
	if (l.gshade)
	    {
	    var size = Math.ceil((getClipHeight(l)-shade_height)*speed/duration);
	    l.working = true;
	    wn_graphical_shade(l,shade_height,speed,size);
	    }
	else
	    {
	    /*** Shading changes the height only, and remembers the height it had
	     *** so unshading restores that height.
	     ***/
	    l.unshaded_height = $(l).outerHeight();
	    setClipHeight(l, shade_height);
	    pg_set_style(l, 'height', shade_height);
	    }
	l.shaded = true;
	}
    else if (!l.working)
	{
	if (l.gshade)
	    {
	    const size = Math.ceil((l.orig_height-shade_height)*speed/duration);
	    l.working = true;
	    wn_graphical_shade(l,l.orig_height,speed,size);
	    }
	else
	    {
	    setClipHeight(l, l.orig_height);
	    pg_set_style(l, 'height', l.unshaded_height);
	    }
	l.shaded = false;
	}
    }

function wn_manual_unshade(l)
    {
    moveBy(l.ContentLayer, 0, getClipTop(l.ContentLayer));
    setClipTop(l.ContentLayer, 0);
    }

function wn_graphical_shade(l,to,speed,size)
    {
    var height = getClipHeight(l);
    if (to < height)
    	{
	if (height - size < to)
	    {
	    setClipHeight(l, to);
	    l.working = false;
	    return;
	    }
	else
	    setClipHeight(l, height - size);
	setClipTop(l.ContentLayer, getClipTop(l.ContentLayer) + size);
	moveBy(l.ContentLayer, 0, -size);
	}
    else
        {
	if (height + size > to)
	    {
	    setClipHeight(l, to);
	    l.working = false;
	    return;
	    }
	else
	    setClipHeight(l, height + size);
	setClipTop(l.ContentLayer, getClipTop(l.ContentLayer) - size);
	moveBy(l.ContentLayer, 0, size);
	}
    setTimeout(wn_graphical_shade,speed,l,to,speed,size);
    }

function wn_close(l)
    {
    if (l.is_modal) pg_setmodal(l, false);
    if (wn_popped[l.id]) delete wn_popped[l.id];
    l.no_close = false;
    l.extended_region = null;
    if (l.popped_above)
	{
	l.popped_above.has_popup = null;
	l.popped_above = null;
	}
    if (l.has_popup)
	{
	wn_close(l.has_popup);
	}

    htr_setvisibility(l,'hidden');
    $(l).css({display:"none"});
    wn_hide_point(l);
    l.is_visible = 0;
    }

function wn_togglevisibility(aparam)
    {
    var vis = htr_getvisibility(this);
    if (vis != 'inherit' && vis != 'visible')
	{
	//this.SetVisibilityTH(true);
	wn_openwin.call(this, aparam);
	}
    else
	{
	this.SetVisibilityTH(false);
	}
    }

function wn_closewin(aparam)
    {
    aparam.IsVisible = 0;
    return this.ifcProbe(ifAction).Invoke('SetVisibility',aparam);
    }

function wn_openwin(aparam)
    {
    this.open_params = aparam;
    aparam.IsVisible = 1;

    /** Record the intent, not just the location, so we can recalculate on resize. **/
    let point_at = aparam.PointAt;
    if (point_at && (typeof point_at !== 'object' || !wgtrIsNode(point_at)))
	point_at = wgtrGetNode(this, point_at);
    if (point_at)
	this.placement = {
	    mode: 'point',
	    at: point_at,
	    side: aparam.PointSide,
	    offset: aparam.PointOffset,
	};
    else if (aparam.X !== undefined && aparam.Y !== undefined)
	this.placement = {
	    mode: 'absolute',
	    x: parseInt(aparam.X),
	    y: parseInt(aparam.Y),
	    attract: 0,
	};
    else if (aparam.Center && aparam.Center != 'no')
	/** Asking for centered means centered in the viewport, on both axes. **/
	this.placement = { mode:'center', centered_x:true, centered_y:true, in_container:false };
    else if (this.placement.from_action)
	/** The Popup action placed this window and is opening it now. **/
	delete this.placement.from_action;
    else if (this.placement.mode === 'point' || this.placement.mode === 'popup')
	/*** Opened again without being told what to place it against: it stays
	 *** where it is, but it is no longer attached to anything.
	 ***/
	this.placement = { mode:'absolute', x:null, y:null, attract:0 };

    /** Only a window placed by pointing has a point. **/
    if (this.placement.mode !== 'point') delete this.point_local;

    /*** A hidden window cannot be measured, so wn_setvisibility_bh() places it
     *** once it is showing -- before the browser paints, so it is never seen at
     *** its old spot.  An already-open window gets no reveal and never reaches
     *** there (see wn_setvisibility_th), so Open means "move now".
     ***/
    if (this.is_visible) wn_place(this);

    return this.ifcProbe(ifAction).Invoke('SetVisibility',aparam);
    }

function wn_setvisibility(aparam)
    {
    if (aparam.IsVisible == null || aparam.IsVisible == 1 || aparam.IsVisible == '1' || aparam.IsVisible == true)
	{
	if (typeof aparam.IsModal != 'undefined') this.is_modal = aparam.IsModal;
	this.no_close = aparam.NoClose;
	this.do_cascade = aparam.Cascade;
	this.SetVisibilityTH(true);
	}
    else
	{
	this.SetVisibilityTH(false);
	}
    }

/** How much of a window must stay visible when it is dragged past an edge. **/
const WN_MIN_VISIBLE_LEFT = 24;
const WN_MIN_VISIBLE_RIGHT = 32;
const WN_MIN_VISIBLE_BOTTOM = 24;

/** The gap a point spans, between a window and the widget it points at. **/
const WN_POINT_GAP = 15;

/*** How each side is laid out: the axis the window slides along to line its
 *** point up with the widget, and whether it is before the widget on the other
 *** axis or after it.
 ***/
const WN_POINT_SIDES = {
    bottom: { along:'x', before:true  },
    top:    { along:'x', before:false },
    right:  { along:'y', before:true  },
    left:   { along:'y', before:false },
    };

/*** The part of the viewport a window can occupy: the viewport less any
 *** scrollbars.  documentElement's client size reports exactly that, although
 *** window.innerWidth and window.innerHeight would include the scrollbars.
 ***/
function wn_get_viewport()
    {
    const de = document.documentElement;
    return { width: de.clientWidth, height: de.clientHeight };
    }

/*** Computes where a window can be: snapping to the edges of the viewport,
 *** and keeping it from moving too far outside.  This only calculates, so it
 *** is safe to call while measuring a batch of windows.
 ***
 *** Coordinates are page coordinates, see moveToAbsolute().  They compare
 *** against viewport sizes, which holds because the Centrallix layout is
 *** generated to fit the viewport and so does not scroll.
 ***
 *** @param wn The window to place.
 *** @param attract The number of pixels from the edge of the viewport at
 *** which windows snap to the edge.
 *** @param x The desired x coordinate for the window.
 *** @param y The desired y coordinate for the window.
 *** @param viewport The usable viewport, from wn_get_viewport().
 *** @returns The allowed {x, y} nearest to the ones asked for.
 ***/
function wn_clamp_position(wn, attract, x, y, viewport)
    {
    /** Get useful values. **/
    const wn_width = getClipWidth(wn);
    const wn_height = getClipHeight(wn);
    const available_width = viewport.width;
    const available_height = viewport.height;
    let new_x, new_y;

    /** X: Handle snapping to edges. **/
    if (!attract) attract = 0;
    if (Math.isBetween(-attract, x, attract)) new_x = 0;
    else if (Math.isBetween(available_width - attract, x + wn_width, available_width + attract))
	new_x = available_width - wn_width;

    /** X: Prevent windows getting lost off the left side of the page. **/
    else if (x + wn_width < WN_MIN_VISIBLE_LEFT) new_x = WN_MIN_VISIBLE_LEFT - wn_width;
    else if (x > available_width - WN_MIN_VISIBLE_RIGHT) new_x = available_width - WN_MIN_VISIBLE_RIGHT;

    /** X: Default case, no movement needed. **/
    else new_x = x;

    /** Y: Handle snapping to edges. **/
    if (Math.isBetween(-attract, y, attract)) new_y = 0;
    else if (Math.isBetween(available_height - attract, y + wn_height, available_height + attract))
	new_y = available_height - wn_height;

    /** Y: Prevent windows from going too far off the screen. **/
    else new_y = Math.clamp(0, y, available_height - WN_MIN_VISIBLE_BOTTOM);

    return { x:new_x, y:new_y };
    }

/*** Moves a window, with no reference to global variables: it just takes params
 *** and moves.  Snapping and the on-screen guards still apply.
 ***
 *** @param wn The window to affect.
 *** @param attract The number of pixels from the edge of the viewport at
 *** which windows snap to the edge.
 *** @param x The new x coordinate for moving the window.
 *** @param y The new y coordinate for moving the window.
 ***/
function wn_do_move_internal(wn, attract, x, y)
    {
    const pos = wn_clamp_position(wn, attract, x, y, wn_get_viewport());

    /** Move the window to the new location. **/
    moveToAbsolute(wn, pos.x, pos.y);

    /** An attached point moves with the window it belongs to. **/
    wn_update_point(wn, {});

    /** Clicking and dragging a window is not a click event. **/
    wn.clicked = 0;
    }

function wn_do_move()
    {
    /** Dereference globals once for performance. **/
    const { wn_current, pg_attract, wn_new_x, wn_new_y } = window;

    /** No window is selected, so we don't have to move anything. **/
    if (wn_current === null) return true;

    /** A dragged window keeps where it was dropped, moving only to stay on screen. **/
    wn_current.placement = {
	mode: 'absolute',
	x: wn_new_x,
	y: wn_new_y,
	attract: pg_attract,
    };

    /** Call the non-responsive version. **/
    wn_do_move_internal(wn_current, pg_attract, wn_new_x, wn_new_y);

    return true;
    }

/*** Placement (object)
 ***
 *** A window remembers how it was placed, not just where it landed, so a resize
 *** can correctly recalculate the position.  This supports the following modes:
 ***
 ***   absolute  A spot it was put at, by a drag or by Open with X and Y, or
 ***             null for "wherever it is", which is how the server places one.
 ***   center    Centered, per axis, in the viewport or in its own container,
 ***             recentered as that space changes.
 ***   point     Pointing at a widget, with an arrow between the two.
 ***   popup     Popped up against a widget, the way a menu appears.
 ***
 *** Every mode is kept on screen, and none of them stores a coordinate the
 *** server generated -- a window the server placed sits where its CSS puts it,
 *** relative to its container, and the browser already keeps it there when the
 *** container moves.  Storing that spot would only fight the browser for it.
 ***/

/*** The space a centered window is centered in, in page coordinates: the
 *** viewport, or its own container for a window the server-side layout centered
 *** inside one.  apos.c centers against the nearest visual container up the tree
 *** (its VisualRef), which is what parentNode gives us here, because a
 *** non-visual widget emits no container of its own for the window to sit in.
 ***
 *** @param wn The window being centered.
 *** @param viewport The usable viewport, from wn_get_viewport().
 *** @returns The {x, y, width, height} to center within.
 ***/
function wn_center_space(wn, viewport)
    {
    if (!wn.placement.in_container)
	return { x:0, y:0, width:viewport.width, height:viewport.height };

    const rect = wn.parentNode.getBoundingClientRect();
    return {
	x: rect.left + window.scrollX,
	y: rect.top + window.scrollY,
	width: rect.width,
	height: rect.height,
	};
    }

/*** Works out where a window should sit.  This only measures: nothing is moved,
 *** so a batch of windows can be measured before any of them are moved, which
 *** keeps the reads from interleaving with writes and forcing a reflow apiece.
 ***
 *** @param wn The window to place.
 *** @param viewport The usable viewport, from wn_get_viewport().
 *** @returns {x, y} for the window, and pt_x/pt_y for its point if it has one,
 ***          or null if the window should be left where it is.
 ***/
function wn_compute_placement(wn, viewport)
    {
    switch (wn.placement.mode)
	{
	case 'absolute':
	    {
	    /*** A spot the window was put at, or null for "wherever it is now".
	     *** A null spot is read live every time and never remembered: the
	     *** server places these with CSS, relative to their container, so the
	     *** browser keeps them with a container that moves and there is
	     *** nothing stored here to go stale.  Either way it stays on screen.
	     ***/
	    const p = wn.placement;
	    return wn_clamp_position(wn, p.attract,
		(p.x != null) ? p.x : getPageX(wn),
		(p.y != null) ? p.y : getPageY(wn),
		viewport
	    );
	    }

	case 'popup':
	    {
	    const p = wn.placement;

	    /** Pop up based on where our attached widget is now. **/
	    const at_geom = (p.at) ? wgtrGetGeom(p.at) : null;
	    if (!at_geom)
		return pg_computepopup(wn, p.x, p.y, p.height, p.width);

	    return pg_computepopup(wn,
		at_geom.x + p.offset_x,
		at_geom.y + p.offset_y,
		(p.height_override) ?? at_geom.height,
		(p.width_override) ?? at_geom.width
	    );
	    }

	case 'center':
	    {
	    /*** Centered in the viewport, or in its own container for a window the
	     *** server-side layout centered inside one.  An axis that is not
	     *** centered keeps the spot it has.  Clamped like the rest: a window
	     *** too big to center still has to be reachable.
	     ***/
	    const p = wn.placement;
	    const rect = wn.getBoundingClientRect();
	    const space = wn_center_space(wn, viewport);
	    return wn_clamp_position(wn, 0,
		(p.centered_x) ? space.x + Math.max(0, (space.width - rect.width) / 2) : getPageX(wn),
		(p.centered_y) ? space.y + Math.max(0, (space.height - rect.height) / 2) : getPageY(wn),
		viewport
	    );
	    }

	case 'point':
	    return wn_compute_point(wn, viewport);

	default:
	    console.warn('wn_compute_placement() - FAIL: Unknown placement mode ' + wn.placement.mode + ' on', wn);
	    return null;
	}
    }

/*** Calculates where a pointed window should be, and where on its edge the
 *** point should be drawn.  Returns, does not apply, like wn_compute_placement().
 ***
 *** The point coordinates returned are in the window's own space, as required
 *** by htutil_point().  Values that are negative or past the far edge put the
 *** point on that side of the window.
 ***
 *** @param wn The window to place.
 *** @param viewport The usable viewport, from wn_get_viewport().
 *** @returns {x, y, pt_x, pt_y}, or null if there is no room to point.
 ***/
function wn_compute_point(wn, viewport)
    {
    const { at, offset } = wn.placement;
    if (!at) return null;

    /** Border radius of this window **/
    let brtxt = $(wn).css('border-radius');
    if (!brtxt) brtxt = $(wn).css('border-bottom-left-radius'); // grrr firefox
    const min_offset = parseInt(brtxt) + 20;

    /** Get the geometry of the pointed widget. **/
    const geom = (at.GetSelectedGeom) ? at.GetSelectedGeom() : wgtrGetGeom(at);
    if (!geom) return null;
    const using_offset = (offset != undefined && offset != null);

    /*** Which side of the window does the point go on?  The designer requested
     *** side is always honored.  Otherwise, we choose based on available room,
     *** so this must be recomputed on resize.
     ***/
    let point_side = wn.placement.side;
    if (!point_side)
	{
	/** Get top, bottom, left, and right space. **/
	const space_t = geom.y;
	const space_b = viewport.height - geom.height - space_t;
	const space_l = geom.x;
	const space_r = viewport.width - geom.width - space_l;

	/** Put the window on the side with the most space. **/
	if (space_t >= space_b && space_t >= space_r && space_t >= space_l)
	    point_side = 'bottom';
	else if (space_b >= space_r && space_b >= space_l)
	    point_side = 'top';
	else if (space_r >= space_l)
	    point_side = 'left';
	else
	    point_side = 'right';
	}

    /** Compute coordinates from the side. **/
    const side = WN_POINT_SIDES[point_side];
    if (!side)
	{
	console.warn('wn_compute_point() - FAIL: Unknown point side ' + point_side + ' on', wn);
	return null;
	}
    const across = (side.along === 'x') ? 'y' : 'x';

    /** Geometry keyed by axis, so one set of formulas serves every side. **/
    const wn_size = { x:$(wn).outerWidth(), y:$(wn).outerHeight() };
    const at_pos = { x:geom.x, y:geom.y };
    const at_size = { x:geom.width, y:geom.height };
    const view_size = { x:viewport.width, y:viewport.height };

    /** Compute the max distance along the edge that the point can be. **/
    const max_pt = wn_size[side.along] - min_offset;
    if (min_offset > max_pt) return null;

    /*** The span of the widget the point may aim at, and the spot on it the
     *** point aims for.  An offset names a single spot instead of the span.
     ***/
    const aim_from = at_pos[side.along] + ((using_offset) ? offset : 0);
    const aim_to   = at_pos[side.along] + ((using_offset) ? offset : at_size[side.along]);
    const aim_at   = at_pos[side.along] + ((using_offset) ? offset : at_size[side.along] / 2);

    /** Where the window may be: go with the midpoint of that range. **/
    const min_win = Math.max(aim_from - max_pt, 0);
    const max_win = Math.min(aim_to - min_offset, view_size[side.along] - wn_size[side.along]);
    if (min_win > max_win) return null;
    const win_along = (min_win + max_win) / 2;

    /** Aim the point from the window's location, as near its target as its own edge allows. **/
    const pt_along = Math.clamp(min_offset, aim_at - win_along, max_pt);

    /** Across the other axis the window clears the widget by the point's gap. **/
    const win_across = (side.before)
	? at_pos[across] - wn_size[across] - WN_POINT_GAP
	: at_pos[across] + at_size[across] + WN_POINT_GAP;
    const pt_across = (side.before) ? wn_size[across] + WN_POINT_GAP : -WN_POINT_GAP;

    return (side.along === 'x')
	? { x:win_along, y:win_across, pt_x:pt_along, pt_y:pt_across }
	: { x:win_across, y:win_along, pt_x:pt_across, pt_y:pt_along };
    }

/*** Draws or redraws the point belonging to a window, if it has one.  Must be
 *** called after the window has been moved, because htutil_point() reads the
 *** window's position to work out where to put the point.
 ***
 *** @param wn The window whose point should be updated.
 *** @param geom Placement geometry from wn_compute_placement().  If it carries
 ***        point coordinates they are used and remembered; otherwise the point
 ***        keeps the position on the window's edge that it already had, so it
 ***        travels with a window that has merely been moved.
 ***/
function wn_update_point(wn, geom)
    {
    if (geom.pt_x != undefined)
	wn.point_local = { x:geom.pt_x, y:geom.pt_y };

    if (wn.point_local)
	{
	const divs = htutil_point(wn, wn.point_local.x, wn.point_local.y, null, null, null, wn.point1, wn.point2);
	wn.point1 = divs.p1;
	wn.point2 = divs.p2;
	}
    else if (wn.resize && wn.resize.param)
	{
	/*** A point put there by the Point action.  Its coordinates are in the
	 *** window's own space, so it only needs to be drawn again where the
	 *** window is now.
	 ***/
	htr_update_point(wn);
	}
    }

/** Hides the point belonging to a window, if it has one. **/
function wn_hide_point(wn)
    {
    const { point1, point2 } = wn;
    if (point1) htr_setvisibility(point1, 'hidden');
    if (point2) htr_setvisibility(point2, 'hidden');
    }

/** Places a single window (and its point) from its placement descriptor. **/
function wn_place(wn)
    {
    const geom = wn_compute_placement(wn, wn_get_viewport());
    if (!geom)
	{
	/** Nowhere to point at any more, so stop pointing. **/
	if (wn.placement.mode === 'point') wn_hide_point(wn);
	return;
	}

    moveToAbsolute(wn, geom.x, geom.y);
    wn_update_point(wn, geom);
    }

/*** Places every open window, in three passes:  measure, move, then draw points.
 *** Measuring all of them first keeps that pass to one reflow.  The later passes
 *** still cost a reflow apiece, which is not worth avoiding for the handful of
 *** windows ever open at once.
 ***/
function wn_place_all()
    {
    const viewport = wn_get_viewport();
    const work = [];

    /** Measure. **/
    for (const wn of wn_list)
	{
	if (!wn.is_visible) continue;
	work.push({ wn: wn, geom: wn_compute_placement(wn, viewport) });
	}

    /** Move. **/
    for (const { wn, geom } of work)
	if (geom) moveToAbsolute(wn, geom.x, geom.y);

    /** Draw the points. **/
    for (const { wn, geom } of work)
	{
	if (geom) wn_update_point(wn, geom);
	else if (wn.placement.mode === 'point') wn_hide_point(wn);
	}
    }

function wn_adjust_z(l,zi)
    {
    var cur_z = htr_getzindex(l);
    if (zi && (typeof cur_z != undefined))
	{
	cur_z += zi;
	htr_setzindex(l,cur_z);
	if (l.point1)
	    htr_setzindex(l.point1,cur_z+1);
	if (l.point2)
	    htr_setzindex(l.point2,cur_z+2);
	}
    if (cur_z > wn_top_z) wn_top_z = cur_z;
    return true;
    }

function wn_bring_top(l)
    {
    if (wn_topwin == l) return true;
    wn_adjust_z(l, wn_top_z - htr_getzindex(l) + 4);
    wn_topwin = l;
    if (l.has_popup)
	wn_bring_top(l.has_popup);
    }

// FIXME: does this MOUSEDOWN work if for NS4 if there is no title?
function wn_mousedown(e)
    {
    for (var w in wn_popped)
	{
	var wn = wn_popped[w];
	var pgx = getPageX(wn);
	var pgy = getPageY(wn);
	if ((!(e.pageX >= pgx && e.pageX < pgx + getClipWidth(wn) && e.pageY >= pgy && e.pageY < pgy + getClipHeight(wn))) && (!wn.extended_region || (!(e.pageX >= wn.extended_region.x && e.pageX < wn.extended_region.x + wn.extended_region.width && e.pageY >= wn.extended_region.y && e.pageY < wn.extended_region.y + wn.extended_region.width))))
	    {
	    wn.ifcProbe(ifAction).Invoke('SetVisibility',{IsVisible:0});
	    }
	}
    if (e.kind == 'wn')
        {
        if (e.target.name == 'close')
            pg_set(e.target,'src','/sys/images/02bigclose.gif');
        else if ((e.mainlayer.has_titlebar && cx__capabilities.Dom0NS && e.pageY < e.mainlayer.pageY + 24) ||
                (cx__capabilities.Dom1HTML && e.layer.subkind == 'titlebar' ))
            {
	    /** Initiate a window drag. **/
            wn_current = e.mainlayer;
            wn_msx = e.pageX;
            wn_msy = e.pageY;
            wn_new_x = null;
            wn_new_y = null;
            wn_moved = 0;
	    e.layer.style.cursor = 'grabbing';
	    if (!cx__capabilities.Dom0IE) wn_windowshade_ns_moz(e.mainlayer);
	    return EVENT_CONTINUE | EVENT_PREVENT_DEFAULT_ACTION;
	    }
        cn_activate(e.mainlayer, 'MouseDown');
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function wn_dblclick(e)
    {
    if (e.kind == 'wn')
	{
        if ((e.mainlayer.has_titlebar && cx__capabilities.Dom0NS && e.pageY < e.mainlayer.pageY + 24) ||
                (cx__capabilities.Dom1HTML && e.layer.subkind == 'titlebar' ))
            {
            if (cx__capabilities.Dom0IE) wn_windowshade_ie(e.mainlayer);
            }
	}
    }

function wn_mouseup(e)
    {
    if (e.target != null && e.target.name == 'close' && e.target.kind == 'wn')
        {
        pg_set(e.target,'src','/sys/images/01bigclose.gif');
	if (e.mainlayer.no_close != true) e.mainlayer.SetVisibilityTH(false);
        }
    else if (e.layer.document != null && pg_images(e.layer).length > 6 && pg_images(e.layer)[6].name == 'close')
        {
        pg_set(pg_images(e.layer)[6],'src','/sys/images/01bigclose.gif');
        }
    if (wn_current != null)
        {
        if (wn_moved == 0) wn_bring_top(wn_current);
	wn_current.titlebar.style.cursor = 'grab';
        }
    if (e.kind == 'wn') cn_activate(e.mainlayer, 'MouseUp');
    
    /** End the active window drag (if one exists). **/
    wn_current = null;
    
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function wn_mousemove(e)
    {
    if (e.kind == 'wn') cn_activate(e.mainlayer, 'MouseMove');
    if (wn_current != null)
        {
        wn_current.clicked = 0;
	if (wn_current.tid) clearTimeout(wn_current.tid);
	wn_current.tid = null;
        if (wn_new_x == null)
            {
            wn_new_x = getPageX(wn_current) + e.pageX-wn_msx;
            wn_new_y = getPageY(wn_current) + e.pageY-wn_msy;
            }
        else
            {
            wn_new_x += (e.pageX - wn_msx);
            wn_new_y += (e.pageY - wn_msy);
            }
        wn_do_move();
        wn_moved = 1;
        wn_msx = e.pageX;
        wn_msy = e.pageY;
        return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function wn_mouseover(e)
    {
    if (e.kind == 'wn') cn_activate(e.mainlayer, 'MouseOver');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function wn_mouseout(e)
    {
    if (e.kind == 'wn') cn_activate(e.mainlayer, 'MouseOut');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }


// Load indication
if (window.pg_scripts) pg_scripts['htdrv_window.js'] = true;
