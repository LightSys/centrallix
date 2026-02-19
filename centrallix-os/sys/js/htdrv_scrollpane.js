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


/** ===== Config ===== **/

/** Whether to ignore zooming (ctrl + scroll) for scrolling events. **/
const sp_ignore_zoom = true;

/*** The overall scroll speed percentage modifier applied any time the scroll
 *** wheel is used, in decimal representation (so 1.00 is 100%).
 ***/
const sp_scroll_mod = 1.00;

/*** The scroll speed percentage modifier applied for fast scrolling (aka.
 *** when using the scroll wheel while holding the alt key), using decimal
 *** representation (so 1.00 is 100%).
 *** 
 *** Note: Chromium-based browsers often use alt to scroll horizontally, so
 ***       fast scrolling may not function there.
 ***/
const sp_fast_scroll_mod = 8.00;

/*** The scroll percentage modifier applied for inverted scrolling (aka. when
 *** using the scroll wheel while holding down the shift key), using decimal
 *** representation (so 1.00 is 100%).
 *** 
 *** Set this to a positive number to disable inverted scrolling.
 ***/
const sp_inverted_scroll_mod = -1.00;



/** ===== Globals ===== **/

/*** If a drag is in progress, stores the target image for the drag. Equals
 *** undefined if no drag is in progress.
 ***/
let sp_drag_img = undefined;

/*** If a button is pressed, stores the target image for the button so that it
 *** can be reset when it is no longer pressed. Equals undefined if no button
 *** is currently being pressed.
 ***/
let sp_button_img = undefined;

/*** Stores the mainlayer that the mouse is currently over. Equals undefined if
 *** the mouse is not over a scroll pane.
 ***/
let sp_target_mainlayer = undefined;

/*** True if a click might be in progress.
 *** 
 *** A click requires the user to generate a MouseDown event and a MouseUp
 *** event in succession that are both on the scroll pane widget.
 ***/
let sp_click_in_progress = false;


/** Event handler to updates the thumb when the scroll pane resizes. **/
const sp_handle_resize = ({ target: layer }) => layer.UpdateThumb();
const sp_resize_observer = new ResizeObserver((entries) => entries.forEach(sp_handle_resize));

function sp_init({ layer: pane, area_name, thumb_name })
    {
    /** Query the area and the thumb elements. **/
    let area, thumb;
    if (cx__capabilities.Dom1HTML)
	{
	area = document.getElementById(area_name);
	thumb = document.getElementById(thumb_name);
	}
    else if (cx__capabilities.Dom0NS)
	{
	/** Netscape fallback to getElementById manually. **/
	const layers = pg_layers(pane);
	for (let i = 0; i < layers.length; i++)
	    {
	    const main_layer = layers[i];
	    if (main_layer.name === area_name) area = main_layer;
	    if (main_layer.name === thumb_name) thumb = main_layer;
	    }
	}
    else alert('Browser not supported!!');
    
    /** Set values for the area and thumb elements. */
    area.nofocus = true;
    thumb.nofocus = true;
    
    /** Set images, i.e. the up & down button and the scroll bar. */
    const images = pg_images(pane);
    for (let image of images)
	{
	const { name } = image;
	
	/** Affect only the intended images. **/
	if (name === 'd' || name === 'u' || name === 'b')
	    {
	    image.kind = 'sp';
	    image.pane = pane;
	    image.mainlayer = pane;
	    image.layer = image;
	    image.thumb = thumb;
	    image.area = area;
	    }
	}
    
    /** Set the scroll thumb. **/
    const thumb_image = pg_images(thumb)[0];
    thumb_image.kind = 'sp';
    thumb_image.pane = pane;
    thumb_image.mainlayer = pane;
    thumb_image.layer = thumb_image;
    thumb_image.thumb = thumb;
    thumb_image.area = area;
    
    /** Init layers and widget. **/
    htr_init_layer(pane, pane, 'sp');
    htr_init_layer(thumb, pane, 'sp');
    htr_init_layer(area, pane, 'sp');
    ifc_init_widget(pane);
    pane.thumb = thumb;
    pane.area = area;
    pane.UpdateThumb = sp_update_thumb;
    
    /** Register actions with Centrallix. **/
    const ia = pane.ifcProbeAdd(ifAction);
    ia.Add("ScrollTo", sp_action_ScrollTo);
    
    /** Register events with the browser. **/
    const ie = pane.ifcProbeAdd(ifEvent);
    ie.Add("Scroll");
    ie.Add("Click");
    ie.Add("Wheel");
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");
    
    /** Scan child elements to find the height of the scroll content. */
    const { top } = area.getBoundingClientRect();
    let maxBottom = top;
    for (const child of area.children)
	{
	const { bottom } = child.getBoundingClientRect();
	if (bottom > maxBottom) maxBottom = bottom;
	}
    area.content_height = Math.max(0, Math.round(maxBottom - top));
    
    /** Watch for changes in content height. **/
    if (cx__capabilities.Dom0IE)
	{
	area.runtimeStyle.clip.pane = pane;
	// how to watch this in IE?
	area.runtimeStyle.clip.onpropertychange = sp_WatchHeight;
	}
    else
	{
	area.clip.pane = pane;
	area.clip.watch("height", sp_WatchHeight);
	}
    
    /** Watch for changes in the size of the scroll pane. **/
    sp_resize_observer.observe(pane);
    }

/** ========== Getter functions ========== **/
/** Functions to compute common values needed often in this code. **/

/** @returns The height of content inside the scroll pane (even if not all of it is visible). **/
function sp_get_content_height(area)
    {
    return area.content_height;
    }

/** @returns The height of visible area available to the scroll pane. **/
function sp_get_available_height(pane)
    {
    return parseInt(getComputedStyle(pane).height);
    }

/** @returns The height of the content outside the available visible area of the scroll pane. **/
function sp_get_nonvisible_height(pane)
    {
    return sp_get_content_height(pane.area) - sp_get_available_height(pane);
    }

/** @returns The height of visible area available to the scroll bar. **/
function sp_get_scrollbar_height(pane)
    {
    /** The up and down buttons and thumb are each 18px. **/
    return sp_get_available_height(pane) - (3*18);
    }

/** @returns The distance down that the scroll pane has been scrolled. **/
function sp_get_scroll_dist(area)
    {
    return -getRelativeY(area);	
    }


/*** Update the scroll pane to handle the height of its contained content
 *** (aka. its child widgets) changing.
 *** 
 *** @param property Unused
 *** @param old_value The old height of the child content (unused).
 *** @param new_value The new height of the child content.
 ***/
function sp_WatchHeight(property, old_value, new_value)
    {
    const { pane } = this;
    const { area } = pane;
    
    /** Handle legacy Internet Explorer behavior. **/
    if (cx__capabilities.Dom0IE)
	new_value = htr_get_watch_newval(window.event);

    /** Update the internal content height value. **/
    area.content_height = new_value;
    
    /** Get the available height of the visible area. **/
    const available_height = sp_get_available_height(pane);
    
    /** Scroll to the top if the content is now smaller than the visible area. **/
    if (getRelativeY(area) + new_value < available_height)
	setRelativeY(area, available_height - new_value);
    if (new_value < available_height) setRelativeY(area, 0);
    
    /** Update the scroll thumb. **/
    pane.UpdateThumb();
    
    return new_value;
    }

/** Called when the ScrollTo action is used. **/
function sp_action_ScrollTo({ Percent, Offset, RangeStart, RangeEnd })
    {
    const pane = this;
    const available = sp_get_available_height(pane);
    const nonvisible_height = sp_get_nonvisible_height(pane); // Height of non-visible content.
    
    /** Ignore the action if all content is visible. **/
    if (nonvisible_height <= 0) return;
    
    /** Calculate the new location to scroll to. **/
    const new_scroll_height =
	(Offset !== undefined) ? Offset :
	(Percent !== undefined) ? Math.clamp(0, Percent / 100, 1) * nonvisible_height :
	(RangeStart !== undefined && RangeEnd !== undefined) ?
	    Math.clamp(RangeEnd - available, sp_get_scroll_dist(pane.area), RangeStart) :
	0; /* Fallback default value. */
    
    /** Scroll to the new location. **/
    sp_scroll_to(pane, new_scroll_height);
    }

/** Recalculate and update the location of the scroll thumb. **/
function sp_update_thumb()
    {
    const pane = this;
    const { area, thumb } = pane;

    /** Get the height of nonvisible content. **/
    const nonvisible_height = sp_get_nonvisible_height(pane);
    
    /** Handle the case where all content is visible. **/
    if (nonvisible_height <= 0)
	{
	/** All we need to do is to move the scroll thumb to the top. **/
	setRelativeY(thumb, 18);
	return;
	}
    
    /** Calculate where the scroll thumb should be based on the scroll progress. **/
    let scroll_dist = sp_get_scroll_dist(area);
    if (scroll_dist > nonvisible_height)
	{
	/** Scroll down to fill the new space at the bottom of the scroll pane. **/
	setRelativeY(area, -nonvisible_height);
	scroll_dist = nonvisible_height;
	}
    const progress = scroll_dist / nonvisible_height;
    const progress_scaled = 18 + sp_get_scrollbar_height(pane) * progress;
    
    /** Set the scroll thumb to the calculated location. **/
    setRelativeY(thumb, progress_scaled);
    }

/*** Scroll the scroll pane to the specified `scroll_height`.
 *** 
 *** @param pane The affected scroll pane DOM node.
 *** @param scroll_height The new height, in pixels, that the content should
 *** 	 be scrolled to as a result of this scroll.
 ***/
function sp_scroll_to(pane, scroll_height)
    {
    /** Ignore undefined target pane. **/
    if (!pane) return;
    
    /** Don't scroll if there's no content to scroll. **/
    const nonvisible_height = sp_get_nonvisible_height(pane);
    if (nonvisible_height < 0) return;
    
    /** Save the current scroll amount for later. **/
    const scroll_height_old = sp_get_scroll_dist(pane.area);
    
    /** Clamp the scroll height within the bounds of the scroll bar. **/
    const scroll_height_new = Math.clamp(0, scroll_height, nonvisible_height);
     
    /** Update the content. **/
    setRelativeY(pane.area, -scroll_height_new);
    pane.UpdateThumb();
    
    /** Construct the param for the centrallix 'Scroll' event. **/
    const percent_old = (scroll_height_old / nonvisible_height) * 100;
    const percent_new = (scroll_height_new / nonvisible_height) * 100;
    const param = { Percent: percent_new, Change: percent_new - percent_old };
    
    /** Schedule the scroll event to allow the page to repaint first. **/
    setTimeout(() => cn_activate(pane, 'Scroll', param), 0);
    }

/*** Scroll the scroll pane down by the specified amount.
 *** 
 *** @param pane The affected scroll pane DOM node.
 *** @param amount The amount, in pixels, that the content should move up as a
 *** 	result of this scroll.
 ***/
function sp_scroll(pane, amount)
    {
    /** Ignore undefined target pane. **/
    if (!pane) return;
    
    /** Scroll the required amount. **/
    sp_scroll_to(pane, sp_get_scroll_dist(pane.area) + amount);
    }

/*** Maps an event object into the params that should be used when propagating
 *** the event into centrallix.
 *** 
 *** @param event The event being converted.
 *** @returns An object to use as params for the centrallix event.
 ***/
function sp_get_event_params(event)
    {
    const { ctrlKey, shiftKey, altKey, metaKey, button } = event.Dom2Event;
    return {
	ctrlKey: (ctrlKey) ? 1 : 0,
	shiftKey: (shiftKey) ? 1 : 0,
	altKey: (altKey) ? 1 : 0,
	metaKey: (metaKey) ? 1 : 0,
	button,
    };
    }

/*** Extract a node from an event, then traverse up the parent tree of the node
 *** to search for a valid pane from a scroll pane widget, returning undefined
 *** if it cannot be found.
 *** 
 *** @param event The event in which to search for the pane.
 *** @returns The scroll pane, if it can be found, or undefined otherwise.
 ***/
function sp_get_pane(event)
    {
    let pane = event.target ?? event.mainlayer ?? event.layer;
    if (!pane) console.warn("sp_get_pane() failed to find pane in event:", event);
    
    /** Search for a scrollpane element in the parent tree. **/
    while (pane)
	{
	if (!pane) break;
	const { id } = pane;
	
	if (id && id.startsWith('sp') && id.endsWith('pane'))
	    {
	    /** We found it. **/
	    return pane;
	    }
	
	/** Traverse up the parent tree. **/
	pane = pane.parentElement;
	}
    
    /** Explicitly return undefined to avoid JIT deoptimization. */
    return undefined;
    }
    
/*** Handles scroll wheel events anywhere on the page.
 *** 
 *** @param e The event that has occurred.
 *** @returns An event result (see ht_render.js).
 ***/
function sp_wheel(e)
    {
    const { ctrlKey, shiftKey, altKey, deltaMode, deltaY } = e.Dom2Event;
    
    /** Ignore events on other DOM nodes if we can't find a scroll pane. **/
    if (!(pane = sp_get_pane(e)))
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    
    /** Trigger Centrallix events. **/
    cn_activate(pane, 'Wheel', sp_get_event_params(e));
    
    /** Ignore zoom events (ctrlKey held). **/
    if (ctrlKey && sp_ignore_zoom)
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    
    /** Handle deltaMode. **/
    let deltaYpx = 0;
    switch (deltaMode)
	{
	case 0: deltaYpx = deltaY;               break; /* Pixels. */
	case 1: deltaYpx = deltaY * 16;          break; /* Lines. */
	case 2: deltaYpx = deltaY * pane.height; break; /* Pages. */
	default:
	    {
	    /** Unknown units, so assume px and clamp to prevent chaos. **/
	    console.warn('Unknown deltaMode', deltaMode, '(value ' + deltaY + ')');
	    deltaYpx = Math.clamp(10, deltaY, 100);
	    }
	}
    
    /** Calculate the scroll distance (with modifiers). **/
    const inverted_mod = (shiftKey) ? sp_inverted_scroll_mod : 1.0;
    const fast_mod = (altKey) ? sp_fast_scroll_mod : 1.0;
    const dist = deltaYpx * sp_scroll_mod * inverted_mod * fast_mod;
    
    /** Scroll to the new location. **/
    sp_scroll(pane, dist);
    
    /** End of event. */
    return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
    }

/*** Handles mouse down events anywhere on the page.
 *** 
 *** @param e The event that has occurred.
 *** @returns An event result (see ht_render.js).
 ***/
function sp_mousedown(e)
    {
    /** Ignore events with no target or events from other DOM nodes. **/
    if (!e.target || e.kind !== 'sp')
	return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    
    /** Trigger Centrallix events. **/
    cn_activate(e.mainlayer, 'MouseDown', sp_get_event_params(e));
    
    /** Ignore right clicks for some actions. **/
    if (e.Dom2Event.button !== 2)
	{
	/** Check what button is being targetted. **/
	const target_img = e.target;
	switch (target_img.name)
	    {
	    /** Up or down button was clicked. **/
	    case 'u':
	    case 'd':
		{
		const scroll_amount = (target_img.name === 'd') ? 16 : -16;
		sp_scroll(target_img.pane, scroll_amount);
		
		/** Update the button to appear pressed. **/
		pg_set(target_img, 'src', htutil_subst_last(target_img.src, "c.gif"));
		sp_button_img = target_img; /* Ensure it is unpressed later. */
		
		/** Event handled. **/
		return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
		}
	    
	    /** Scroll bar was clicked. **/
	    case 'b':
		{
		/** Move one page in the direction of the mouse pointer. **/
		const up = (e.pageY < getPageY(target_img.thumb) + 9);
		const amount = (target_img.height + 36) * ((up) ? -1 : 1);
		sp_scroll(target_img.pane, amount);
		
		/** Event handled. **/
		return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
		}
	    
	    /** Scroll thumb was clicked. */
	    case 't':
		{
		/** Start a drag. **/
		sp_drag_img = target_img;
		
		/** Event handled. **/
		return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
		}
	    }
	}
    
    /*** The user clicked on the widget (not one of its buttons), so track that
     *** a click might be in progress.
     ***/
    sp_click_in_progress = true;
    
    /** Ignore the event. **/
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

/*** Handles mouse move events anywhere on the page.
 *** 
 *** @param e The event that has occurred.
 *** @returns An event result (see ht_render.js).
 ***/
function sp_mousemove(e)
    {
    /** Trigger Centrallix events. **/
    const pane = sp_get_pane(e);
    if (pane) cn_activate(pane, 'MouseMove');
    
    /** Monitor events on other DOM nodes to detect MouseOut. **/
    if (!pane && sp_target_mainlayer)
	{
	/** Mouse out has occurred, ending the mouse over. **/
	cn_activate(sp_target_mainlayer, 'MouseOut');
	sp_target_mainlayer = undefined;
	}
    
    /** Check if a drag is in progress (aka. the drag target exists and is a scroll thumb). **/
    const target_img = sp_drag_img;
    if (target_img && target_img.kind === 'sp' && target_img.name === 't')
	{
	const { pane } = target_img;
	
	/** Get drag_dist: the distance that the scroll bar should move. **/
	const page_y = getPageY(target_img.thumb);
	const drag_dist = e.pageY - page_y;
	
	/** Scale drag_dist to the distance that the content should move. **/
	const scrollbar_height = sp_get_scrollbar_height(pane);
	const nonvisible_height = sp_get_nonvisible_height(pane);
	const content_drag_dist = (drag_dist / scrollbar_height) * nonvisible_height;
	
	/** Scroll the content by the required amount to reach the new mouse location. **/
	sp_scroll(target_img.pane, content_drag_dist);
	
	/** Event handled. **/
	return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	}
    
    /** Continue the event. **/
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

/*** Handles mouse up events anywhere on the page.
 *** 
 *** @param e The event that has occurred.
 *** @returns An event result (see ht_render.js).
 ***/
function sp_mouseup(e)
    {
    /** Trigger Centrallix events. **/
    if (e.kind === 'sp')
	{
	const params = sp_get_event_params(e);
	cn_activate(e.mainlayer, 'MouseUp', params);
	if (sp_click_in_progress)
	    cn_activate(e.mainlayer, 'Click', params);
	}
    
    /** A click is no longer in progress. **/
    sp_click_in_progress = false;
    
    /** Check for an active drag. **/
    if (sp_drag_img)
	{
	/** End the drag. **/
	sp_drag_img = undefined;
	}
    
    /** Check for a pressed button. **/
    if (sp_button_img)
	{
	/** Reset the pressed button. **/
	pg_set(sp_button_img, 'src', htutil_subst_last(sp_button_img.src, "b.gif"));
	sp_button_img = undefined;
	}
    
    /** Continue the event. **/
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

/*** Handles mouse over events anywhere on the page.
 *** 
 *** @param e The event that has occurred.
 *** @returns An event result (see ht_render.js).
 ***/
function sp_mouseover(e)
    {
    /** Check for mouse over on an sp element. **/
    if (sp_target_mainlayer && e.kind === 'sp')
	{
	/** Begin a mouse over. **/
	cn_activate(e.mainlayer, 'MouseOver');
	sp_target_mainlayer = e.mainlayer;
	}
    
    /** Continue the event. **/
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

/** Indicate loading is complete. **/
if (window.pg_scripts) pg_scripts['htdrv_scrollpane.js'] = true;
