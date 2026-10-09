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

/** Shared across every button on the page. **/
const gb_imgcache = {};
let gb_current = null;

/*** gb_cell - the content div inside the button's pane.  Everything the
 *** button styles at runtime lives on this element rather than the pane.
 ***/
function gb_cell(layer)
    {
    return layer.getElementsByClassName('cell')[0];
    }

/*** gb_span - the text span, absent on image-only buttons.
 ***/
function gb_span(layer)
    {
    return layer.getElementsByTagName('span')[0];
    }

/*** gb_setimage - show one of the button's four images.  Pointer states only
 *** apply while the button is enabled, so a disabled button keeps the image the
 *** server painted; gb_setenable passes force to drive the transition itself.
 ***/
function gb_setimage(layer, src, force)
    {
    if (src === '' || src === layer.current_image) return;
    if (!force && layer.enabled === false) return;
    layer.current_image = src;
    if (layer.type == 'textoverimage')
	gb_cell(layer).style.backgroundImage = 'URL(' + src + ')';
    else if (layer.img_element !== undefined)
	pg_set(layer.img_element, 'src', src);
    }

/*** gb_preload - keep a decoded copy of a state image so that swapping to it
 *** does not go to the network mid-interaction.
 ***/
function gb_preload(src)
    {
    if (src === '') return;
    if (gb_imgcache[src] === undefined)
	{
	gb_imgcache[src] = new Image();
	gb_imgcache[src].src = src;
	}
    }

function gb_init(param)
    {
    const l = param.layer;

    l.nofocus = true;
    htr_init_layer(l,l,'gb');
    ifc_init_widget(l);

    // Identity and type
    l.buttonName = param.name;
    l.buttonText = param.text;
    l.type = param.type;

    // Images
    l.image = param.image;
    l.point_image = param.point_image;
    l.click_image = param.click_image;
    l.disabled_image = param.disabled_image;

    // Colors and border
    l.text_color = param.text_color;
    l.shadow_color = param.shadow_color;
    l.disabled_text_color = param.disabled_text_color;
    l.border_style = param.border_style;
    l.border_color = param.border_color;

    // Behavior
    l.do_repeat = param.repeat;
    l.repeat_sched_id = null;
    l.tooltip = param.tooltip;
    l.tooltip_id = null;
    l.trigger = gb_trigger;

    // DOM wiring
    l.img_element = l.getElementsByTagName('img')[0];
    l.firstChild.mainlayer = l;
    htutil_tag_images(l, 'gb', l, l);

    /*** enabled and current_image must agree with what the server already put in the
     *** DOM, and must be set before gb_setmode() so it does not swap the image
     *** out from under a button that rendered disabled.
     ***/
    l.enabled = param.enabled;
    l.current_image = (l.enabled) ? param.image : param.disabled_image;

    /** Preload the state images so the first hover does not flash. **/
    gb_preload(param.image);
    gb_preload(param.point_image);
    gb_preload(param.click_image);
    gb_preload(param.disabled_image);

    l.tristate = param.tristate;
    l.mode = -1;
    gb_setmode(l, 0);

    l.gb_setenable = gb_setenable;
    htr_watch(l, 'enabled', 'gb_setenable');

    // Values
    const iv = l.ifcProbeAdd(ifValue);
    iv.Add("text", gb_cb_gettext, gb_cb_settext);
    iv.Add("enabled", "enabled");

    // Events
    const ie = l.ifcProbeAdd(ifEvent);
    ie.Add("Click");
    ie.Add("MouseUp");
    ie.Add("MouseDown");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");

    // Actions
    const ia = l.ifcProbeAdd(ifAction);
    ia.Add("SetText", gb_action_settext);
    ia.Add("Click", gb_action_click);
    ia.Add("Enable", gb_enable);
    ia.Add("Disable", gb_disable);

    // Mobile Safari workaround
    const span = gb_span(l);
    if (span !== undefined) span.addEventListener('click', function() {});
    }

function gb_enable()
    {
    this.enabled = true;
    }

function gb_disable()
    {
    this.enabled = false;
    }

/*** gb_trigger - fire MouseDown, and keep firing it while the button is held
 *** down if the widget asked to repeat.
 ***/
function gb_trigger()
    {
    if (this.do_repeat)
	{
	if (this.repeat_sched_id === null)
	    this.repeat_sched_id = pg_addsched_fn(this, 'trigger', [], 500);
	else
	    this.repeat_sched_id = pg_addsched_fn(this, 'trigger', [], 200);
	}
    cn_activate(this, 'MouseDown');
    }

function gb_action_settext(aparam)
    {
    const span = gb_span(this);
    if (span !== undefined) span.textContent = aparam.Text;
    this.buttonText = aparam.Text;
    }

function gb_action_click(aparam)
    {
    if (this.enabled)
	{
	aparam.from_action = 1;
	this.ifcProbe(ifEvent).Activate('Click', aparam);
	}
    }

// used by ifValue
function gb_cb_gettext(attr)
    {
    return '';
    }

function gb_cb_settext(attr, val)
    {
    this.ifcProbe(ifAction).Invoke('SetText', {Text:val});
    return;
    }

/*** gb_setenable - follow the widget's 'enabled' property.  mode is reset so a
 *** button toggled while the pointer is still over it does not keep the pointer
 *** state it had before, which gb_setmode would otherwise treat as current.
 ***/
function gb_setenable(prop, oldv, newv)
    {
    const span = gb_span(this);
    const imgs = this.getElementsByTagName('img');

    if (newv)
	{
	// make enabled
	this.classList.remove('gb_disabled');
	if (span !== undefined)
	    {
	    span.style.color = this.text_color;
	    span.style.textShadow = '1px 1px ' + this.shadow_color;
	    }
	for (let i = 0; i < imgs.length; i++) imgs[i].style.opacity = '1.0';
	gb_setimage(this, this.image, true);
	this.mode = -1;
	}
    else
	{
	// make disabled
	this.classList.add('gb_disabled');
	if (span !== undefined)
	    {
	    span.style.color = this.disabled_text_color;
	    span.style.textShadow = '';
	    }
	for (let i = 0; i < imgs.length; i++) imgs[i].style.opacity = '0.3';
	gb_setimage(this, this.disabled_image, true);
	this.mode = -1;
	}
    return newv;
    }

/*** gb_setborder - paint the cell's border for the current state.  A button
 *** that declared no border keeps none: the server sized it on that basis, so
 *** drawing one here would both frame a bare icon and overflow its box.
 ***/
function gb_setborder(layer, style, color)
    {
    if (layer.border_style == 'none') return;
    gb_cell(layer).style.borderStyle = style;
    gb_cell(layer).style.borderColor = color;
    }

function gb_setmode(layer, mode)
    {
    if (layer.tristate == 0 && mode == 0) mode = 1;
    if (mode != layer.mode)
	{
	layer.mode = mode;
	switch(mode)
	    {
	    case 0: /* no point no click */
		gb_setborder(layer, 'solid', 'transparent');
		gb_setimage(layer, layer.image);
		break;

	    case 1: /* point, but no click */
		gb_setborder(layer, layer.border_style, layer.border_color);
		gb_setimage(layer, layer.point_image);
		break;

	    case 2: /* point and click */
		gb_setborder(layer,
		    (layer.border_style == 'outset') ? 'inset' : layer.border_style,
		    layer.border_color);
		gb_setimage(layer, layer.click_image);
		break;
	    }
	}
    }

function gb_mousedown(e)
    {
    let ly = e.layer;
    if (ly.mainlayer !== undefined) ly = ly.mainlayer;
    if (ly.kind == 'gb' && ly.enabled)
        {
        gb_setmode(ly,2);
	gb_current = ly;
	ly.trigger();
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function gb_mouseup(e)
    {
    let ly = e.layer;
    if (ly.mainlayer !== undefined) ly = ly.mainlayer;
    /** Stop the repeat on the button that started it, wherever the pointer is. **/
    if (gb_current !== null && gb_current.repeat_sched_id !== null)
	{
	pg_delsched(gb_current.repeat_sched_id);
	gb_current.repeat_sched_id = null;
	}
    if (ly.kind == 'gb' && ly.enabled)
        {
	const rect = ly.getBoundingClientRect();
	const left = rect.left + window.pageXOffset;
	const top = rect.top + window.pageYOffset;
        if (e.pageX >= left && e.pageX < left + rect.width &&
            e.pageY >= top && e.pageY < top + rect.height)
            {
	    if (ly.mode == 2)
		{
		gb_setmode(ly,1);
		ly.ifcProbe(ifEvent).Activate('Click', { from_action:0 });
		ly.ifcProbe(ifEvent).Activate('MouseUp', {});
		}
            }
        else
            {
            gb_setmode(ly,0);
            }
        }
    if (gb_current && gb_current.mode == 2 && gb_current != ly)
	{
	gb_setmode(gb_current,0);
	}
    gb_current = null;
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function gb_mouseover(e)
    {
    let ly = e.layer;
    if (ly.mainlayer !== undefined) ly = ly.mainlayer;
    if (ly.kind == 'gb' && ly.enabled)
        {
	if (ly.mode != 2) gb_setmode(ly,1);
	if (ly.tooltip !== '') ly.tooltip_id = pg_tooltip(ly.tooltip, e.pageX, e.pageY);
	ly.ifcProbe(ifEvent).Activate('MouseOver', {});
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function gb_mouseout(e)
    {
    let ly = e.layer;
    if (ly.mainlayer !== undefined) ly = ly.mainlayer;
    if (ly.kind == 'gb' && ly.enabled)
        {
	if (ly.tooltip_id !== null)
	    {
	    pg_canceltip(ly.tooltip_id);
	    ly.tooltip_id = null;
	    }
	if (ly.mode != 2) gb_setmode(ly,0);
	ly.ifcProbe(ifEvent).Activate('MouseOut', {});
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function gb_mousemove(e)
    {
    let ly = e.layer;
    if (ly.mainlayer !== undefined) ly = ly.mainlayer;
    if (ly.kind == 'gb' && ly.enabled)
        {
	if (ly.tooltip_id !== null)
	    {
	    pg_canceltip(ly.tooltip_id);
	    ly.tooltip_id = pg_tooltip(ly.tooltip, e.pageX, e.pageY);
	    }
	ly.ifcProbe(ifEvent).Activate('MouseMove', {});
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }


// Load indication
if (window.pg_scripts) pg_scripts['htdrv_button.js'] = true;
