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

function eb_getlist(content)
    {
    if (!content) return content;
    var vals = String(content).split(/,/);
    if(content==vals[0])
	return content;
    else
	{
	return vals;
	}
    }

function eb_getvalue()
    {
    if(this.form.mode == 'Query')
	return eb_getlist(this.content);
    return this.content;
    }

function eb_actionsetvalue(aparam)
    {
    var oldval = this.content;
    if ((typeof aparam.Value) != 'undefined')
	{
	this.internal_setvalue(aparam.Value);
	this.DoDataChange(0, 0);
	if (this.form) this.form.DataNotify(this, true);
	this.changed=true;
	cn_activate(this,"DataModify", {Value:aparam.Value, FromKeyboard:0, FromOSRC:0, OldValue:oldval});
	}
    }

function eb_actionsetvaldesc(aparam)
    {
    if ((typeof aparam.Description) != 'undefined')
	{
	this.descriptions[this.content] = aparam.Description;
	if (this.description != aparam.Description)
	    this.Update(this.content);
	}
    }

function eb_internal_setvalue(v)
    {
    this.was_null = (v == null);
    if (v != null)
	v = new String(v);
    var c = 0;
    if (eb_current == this)
	c = eb_length(v);
    this.Update(v);
    this.addHistory(v);
    }

function eb_setvalue(v,f)
    {
    this.internal_setvalue(v);
    this.DoDataChange(1, 0);
    }

function eb_clearvalue()
    {
    this.was_null = true;
    this.Update(null);
    this.DoDataChange(1, 0);
    }

function eb_content_changed(p,o,n)
    {
    this.Update('' + n);
    return '' + n;
    }

function eb_set_content(v)
    {
    if (typeof v == 'undefined') v = null;
    if (this.content != v)
	{
	htr_unwatch(this, "content", "content_changed");
	if (!v)
	    this.content = v;
	else
	    this.content = String(v).valueOf();
	htr_watch(this, "content", "content_changed");
	}
    }

function eb_enable()
    {
    if (this.bg) htr_setbgcolor(this, this.bg);
    if (this.bgi) htr_setbgimage(this, this.bgi);
    this.enabled='full';
    $(this.ContentLayer).prop('disabled', false);
    }

function eb_disable()
    {
    if (this.bg) htr_setbgcolor(this, '#e0e0e0');
    this.enabled='disabled';
    $(this.ContentLayer).prop('disabled', true);
    if (eb_current == this)
	{
	eb_deselect();
	if(this.form) this.form.TabNotify(this);
	}
    }

function eb_readonly()
    {
    if (this.bg) htr_setbgcolor(this, this.bg);
    if (this.bgi) htr_setbgimage(this, this.bgi);
    this.enabled='readonly';
    $(this.ContentLayer).prop('disabled', true);
    }

function eb_inputwidth()
    {
    if (!this.ContentLayer.value)
	return 0;
    var span = document.createElement('span');
    $(span).text(this.ContentLayer.value);
    $(span).css($(this.ContentLayer).css(["font-family","font-size","text-decoration","font-weight"]));
    $(span).css({'white-space':'pre'});
    this.appendChild(span);
    var w = span.getBoundingClientRect().width;
    this.removeChild(span);
    return w;
    }

function eb_setdesc(txt)
    {
    if (!this.DescLayer)
	this.DescLayer = htr_new_layer($(this).width(), this);
    $(this.DescLayer).text(txt?('(' + txt + ')'):'');
    $(this.DescLayer).css
	({
	"z-index":"-1",
	"color":this.desc_fgcolor?this.desc_fgcolor:"#808080",
	"top":($(this).height() - $(this.DescLayer).height())/2 + "px",
	"left":(this.input_width() + (this.content?4:0) + 5) + "px",
	"visibility":"inherit",
	"white-space":"nowrap",
	});
    }

function eb_settext(l,txt)
    {
    var vistxt = txt;
    if (vistxt == null) vistxt = '';
    l.set_content(txt);
    l.ContentLayer.value = htutil_obscure(vistxt);
    }


function eb_set_l_img(l, stat)
    {
    if (l.l_img_on && !stat)
	l.l_img.src = "/sys/images/eb_edg.gif";
    else if (!l.l_img_on && stat)
	l.l_img.src = "/sys/images/eb_lft.gif";
    l.l_img_on = stat;
    }


function eb_set_r_img(l, stat)
    {
    if (l.r_img_on && !stat)
	l.r_img.src = "/sys/images/eb_edg.gif";
    else if (!l.r_img_on && stat)
	l.r_img.src = "/sys/images/eb_rgt.gif";
    l.r_img_on = stat;
    }


function eb_length(c)
    {
    if (c == null)
	return 0;
    else
	return c.length;
    }


function eb_add_history(txt)
    {
    if (txt == null || txt == undefined) txt = this.content;
    this.hist_offset = -1;
    if (txt != null && txt != '')
	{
	for(var i=0;i<this.value_history.length;i++)
	    {
	    if (this.value_history[i] == (txt + ''))
		{
		this.value_history.splice(i,1);
		break;
		}
	    }
	this.value_history.splice(0,0,txt);
	if (this.value_history.length > 32)
	    this.value_history.pop();
	return true;
	}
    else
	{
	return false;
	}
    }


// Update the text, cursor, and left/right edge arrows
function eb_update(txt)
    {
    // New text value
    if (txt != this.content)
	eb_settext(this,txt);

    // Value description field
    var descr = '';
    if (this.descriptions[this.content] && (!this.has_focus || this.content))
	descr = this.descriptions[this.content];
    if (descr != this.description)
	this.description = descr;
    if (!this.has_focus && (this.content == '' || this.content == null) && this.empty_desc)
	descr = this.empty_desc;
    this.set_desc(descr);

    // Left and right 'more text here' arrows
    pg_addsched_fn(this, function()
	{
	eb_set_l_img(this, this.ContentLayer.scrollLeft > 0);
	eb_set_r_img(this, this.ContentLayer.scrollLeft < ((this.ContentLayer.scrollLeftMax === undefined)?(this.ContentLayer.scrollWidth - this.ContentLayer.clientWidth - 1):this.ContentLayer.scrollLeftMax));
	} , [], 10);
    }


function eb_paste(e)
    {
	return;
    }


function eb_receiving_input(e)
    {
    var eb=this.mainlayer;
    //var sel = document.getSelection();
    //var range = sel.getRangeAt(0);
    var rstart = eb.ContentLayer.selectionStart;
    var rend = eb.ContentLayer.selectionEnd;
    //var rstart = range.startOffset;
    //var rend = range.endOffset;
    //htr_alert(sel.focusNode, 1);
    var orig_curtxt = this.value;
    var changed = false;
    var curtxt = orig_curtxt.replace(/\n$/,"");
    var curlen = curtxt?curtxt.length:0;

    if (curtxt != orig_curtxt)
	{
	changed = true;
	}
    if (rend > curlen)
	{
	changed = true;
	rend = curlen;
	}
    if ((e >= 32 && e < 127) || e > 127)
	{
	newtxt = cx_hints_checkmodify(l,txt,vistxt.substr(0,l.cursorCol) + String.fromCharCode(k) + vistxt.substr(l.cursorCol,vistxt.length), l._form_type);
        if (newtxt != txt)
	    {
	    cursoradj = 1;
	    }
	}
    if (rstart > curlen)
	{
	changed = true;
	rstart = curlen;
	}
    for(var i=0; i<curlen; i++)
	{
	if (curtxt.charCodeAt(i) < 32 || curtxt.charCodeAt(i) == 127)
	    {
	    curtxt = curtxt.substr(0,i) + ' ' + curtxt.substr(i+1);
	    changed = true;
	    }
	}
    if (changed)
	{
	this.value = curtxt;
	//range.setStart(range.startContainer, rstart);
	//range.setEnd(range.startContainer, rend);
	eb.ContentLayer.setSelectionRange(rstart, rend);
	}

    var oldtxt = eb.content;
    var newcurtxt = cx_hints_checkmodify(eb, oldtxt, curtxt, eb._form_type);
    if (eb.was_null && newcurtxt == '')
	newcurtxt = null;
    if (newcurtxt != curtxt)
	{
	pg_addsched_fn(eb, function()
	    {
	    this.Update(curtxt = newcurtxt);
	    }, [], 10);
	}
    else
	eb.set_content(curtxt);
    if (eb.form) eb.form.DataNotify(eb);
    eb.changed=true;
    cn_activate(eb,"DataModify", {Value:curtxt, FromKeyboard:1, FromOSRC:0, OldValue:oldtxt});
    eb.Update(curtxt);

    return;
    }
    


function eb_keydown(e)
    {
    var eb = this.mainlayer;

    // check before keypress...
    if (isCancel(eb.ifcProbe(ifEvent).Activate('BeforeKeyPress', {Code:e.keyCode, Name:htr_code_to_keyname(e.keyCode)})))
	{
	e.preventDefault();
	return;
	}

    if (e.keyCode == (KeyboardEvent.DOM_VK_RETURN || 13) || e.keyCode == (KeyboardEvent.DOM_VK_ENTER || 14))
	{
	if (eb.form)
	    eb.form.RetNotify(eb);
	eb.addHistory();
	cn_activate(eb, 'ReturnPressed', {});
	eb.DoDataChange(0, 1);
	}
    else if (e.keyCode == (KeyboardEvent.DOM_VK_TAB || 9) && !e.shiftKey)
	{
	if (eb.form) eb.form.TabNotify(eb);
	cn_activate(eb, 'TabPressed', {Shift:0});
	eb.DoDataChange(0, 1);
	}
    else if (e.keyCode == (KeyboardEvent.DOM_VK_TAB || 9) && e.shiftKey)
	{
	if (eb.form) eb.form.ShiftTabNotify(eb);
	cn_activate(eb, 'TabPressed', {Shift:1});
	eb.DoDataChange(0, 1);
	}
    else if (e.keyCode == (KeyboardEvent.DOM_VK_ESCAPE || 27))
	{
	if (eb.form) eb.form.EscNotify(eb);
	cn_activate(eb, 'EscapePressed', {});
	}
    else if (e.keyCode == (KeyboardEvent.DOM_VK_DOWN || 40))
	{
	if (eb.hist_offset == -1)
	    {
	    eb.addHistory();
	    var newtxt = "";
	    }
	else if (l.hist_offset == 0)
	    {
	    eb.hist_offset--;
	    var newtxt = "";
	    }
	else
	    {
	    eb.hist_offset--;
	    var newtxt = eb.value_history[eb.hist_offset];
	    }
	if (eb.form) eb.form.DataNotify(eb);
	eb.changed=true;
	cn_activate(eb,"DataModify", {Value:newtxt, FromKeyboard:1, FromOSRC:0, OldValue:eb.content});
	eb.Update(newtxt);
	pg_addsched_fn(eb, function()
	    {
	    this.ContentLayer.setSelectionRange(eb_length(this.content), eb_length(this.content));
	    }, [], 10);
	}
    else if (e.keyCode == (KeyboardEvent.DOM_VK_UP || 38))
	{
	if (eb.hist_offset < eb.value_history.length - 1)
	    {
	    if (eb.hist_offset == -1)
		{
		if (eb.addHistory())
		    eb.hist_offset = 0;
		}
	    eb.hist_offset++;
	    var newtxt = eb.value_history[eb.hist_offset];
	    if (newtxt != undefined)
		{
		if (eb.form) eb.form.DataNotify(eb);
		eb.changed=true;
		cn_activate(eb,"DataModify", {Value:newtxt, FromKeyboard:1, FromOSRC:0, OldValue:eb.content});
		eb.Update(newtxt);
		}
	    else
		eb.hist_offset--;
	    pg_addsched_fn(eb, function()
		{
		this.ContentLayer.setSelectionRange(eb_length(this.content), eb_length(this.content));
		}, [], 10);
	    }
	}
    else
	{
	eb.Update(eb.content);
	return;
	}

    e.preventDefault();
    return;
    }


function eb_keyup(e)
    {
    return;
    }


function eb_keypress(e)
    {
    return;
    }


function eb_keyhandler(l,e,k)
    {
    if(l.enabled!='full') return 1;
    cn_activate(l, "KeyPress", {Code:k, Name:e.keyName, Modifiers:e.modifiers, Content:l.content});
    if (e.keyName == 'f3' && e.Dom2Event.type == 'keypress') return true;
    return false;
    }


function eb_do_data_change(from_osrc, from_kbd)
    {
    var nv = cx_hints_checkmodify(this, this.value, this.content, null, true);
    if (nv != this.content)
	{
	this.internal_setvalue(nv);
	}
    if (isCancel(this.ifcProbe(ifEvent).Activate('BeforeDataChange', {OldValue:this.value, Value:nv, FromOSRC:from_osrc, FromKeyboard:from_kbd})))
	{
	this.internal_setvalue(this.value);
	return false;
	}
    this.oldvalue = this.value;
    this.value = nv;
    cn_activate(this, "DataChange", {Value:this.value, OldValue:this.oldvalue, FromOSRC:from_osrc, FromKeyboard:from_kbd});
    }


function eb_action_set_focus(aparam)
    {
    var x = (typeof aparam.X == 'undefined')?null:aparam.X;
    var y = (typeof aparam.Y == 'undefined')?null:aparam.Y;
    pg_setkbdfocus(this, null, x, y);
    }

function eb_checkfocus(e)
    {
    var form = this.mainlayer.form;
    if (form && !form.is_focusable)
	{
	e.preventDefault();
	e.currentTarget.blur();
	}
    return;
    }

function eb_browserfocus(e)
    {
    this.mainlayer.has_focus = true;
    }

function eb_select(x,y,l,c,n,a,k)
    {
    if(this.enabled != 'full') return 0;
    this.ContentLayer.focus();
    var got_focus = $(this.ContentLayer).is(':focus');
    if (!got_focus)
	pg_addsched_fn(this.ContentLayer, function() { this.focus() }, {}, 200);
    if (k)
	pg_addsched_fn(this, function()
	    {
	    this.ContentLayer.setSelectionRange(eb_length(this.content), eb_length(this.content));
	    this.Update(this.content);
	    }, [], got_focus?10:201);
    this.has_focus = true;
    eb_current = this;
    if(this.form)
	if (!this.form.FocusNotify(this)) return 0;
    cn_activate(this,"GetFocus", {});
    return 1;
    }

function eb_deselect(p)
    {
    this.ContentLayer.blur();
    this.has_focus = false;
    eb_current = null;
    if (this.changed)
	{
	if (!p || !p.nodatachange)
	    {
	    this.DoDataChange(0, 1);
	    this.changed=false;
	    }
	}
    this.ContentLayer.setSelectionRange(0,0);
    this.Update(this.content);
    cn_activate(this,"LoseFocus", {});
    this.addHistory();
    return true;
    }


function eb_mselect(x,y,l,c,n,a)
    {
    var offs = $(this).offset();
    if (this.ContentLayer.scrollWidth > this.ContentLayer.clientWidth)
	this.tipid = pg_tooltip(this.tooltip?this.tooltip:this.content, offs.left + x, offs.top + y);
    else if (this.tooltip)
	this.tipid = pg_tooltip(this.tooltip, offs.left + x, offs.top + y);
    return 1;
    }


function eb_mdeselect(l,c,n,a)
    {
    if (this.tipid) pg_canceltip(this.tipid);
    this.tipid = null;
    return 1;
    }


function eb_mouseup(e)
    {
    if (e.kind == 'eb') cn_activate(e.mainlayer, 'Click');
    if (e.kind == 'eb') cn_activate(e.mainlayer, 'MouseUp');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function eb_mousedown(e)
    {
    if (e.kind == 'eb') cn_activate(e.mainlayer, 'MouseDown');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function eb_mouseover(e)
    {
    if (e.kind == 'eb') cn_activate(e.mainlayer, 'MouseOver');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function eb_mouseout(e)
    {
    if (e.kind == 'eb') cn_activate(e.mainlayer, 'MouseOut');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function eb_mousemove(e)
    {
    if (e.kind == 'eb') cn_activate(e.mainlayer, 'MouseMove');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function eb_cb_reveal(e)
    {
    switch (e.eventName) 
	{
	case 'Reveal':
	    this.form.Reveal(this,e);
	    break;
	case 'Obscure':
	    this.form.Reveal(this,e);
	    break;
	case 'RevealCheck':
	case 'ObscureCheck':
	    if (this.form) this.form.Reveal(this,e);
	    else pg_reveal_check_ok(e);
	    break;
	}

    return true;
    }

/**
* l - base layer
* c1 - content layer 1
* is_readonly - if the editbox is read only
* main_bg - background color
**/
function eb_init(param)
    {
    var l = param.layer;
    var c1 = param.c1;

    if (!param.mainBackground)
	{
	l.bg = '#c0c0c0';
	}
    else
	{
	l.bg = htr_extract_bgcolor(param.mainBackground);
	l.bgi = htr_extract_bgimage(param.mainBackground);
	}

    l.desc_fgcolor = param.desc_fgcolor;
    l.empty_desc = param.empty_desc;

    htr_init_layer(l,l,'eb');
    htr_init_layer(c1,l,'eb');
    ifc_init_widget(l);
    l.fieldname = param.fieldname;
    l.tooltip = param.tooltip;

    // Left/Right arrow images
    var imgs = pg_images(l);
    for(var i=0;i<imgs.length;i++)
	{
	if (imgs[i].name == 'l') l.l_img = imgs[i];
	else if (imgs[i].name == 'r') l.r_img = imgs[i];
	}
    l.l_img_on = false;
    l.r_img_on = false;
    l.has_focus = false;

    // Set up params for displaying the content.
    l.ContentLayer = c1;
    l.viscontent = '';
    l.content = '';
    l.value = '';
    l.description = '';
    l.descriptions = {};
    l.Update = eb_update;
    l.addHistory = eb_add_history;
    l.was_null = false;
    l.value_history = [];
    l.hist_offset = -1;

    // Callbacks
    l.keyhandler = eb_keyhandler;
    l.getfocushandler = eb_select;
    l.losefocushandler = eb_deselect;
    l.getmousefocushandler = eb_mselect;
    l.losemousefocushandler = eb_mdeselect;
    l.tipid = null;
    l.getvalue = eb_getvalue;
    l.setvalue = eb_setvalue;
    l.internal_setvalue = eb_internal_setvalue;
    l.clearvalue = eb_clearvalue;
    l.setoptions = null;
    l.enablenew = eb_enable;  // We have added enablenew and enablemodify.  See docs
    l.disable = eb_disable;
    l.readonly = eb_readonly;
    l.enable = eb_enable;
    if (param.isReadOnly)
	{
	l.enablemodify = eb_disable;
	l.enabled = 'disable';
	$(l.ContentLayer).prop('disabled', true);
	}
    else
	{
	l.enablemodify = eb_enable;
	l.enabled = 'full';
	$(l.ContentLayer).prop('disabled', false);
	}
    l.isFormStatusWidget = false;
    if (cx__capabilities.CSSBox)
	pg_addarea(l, -1,-1,$(l).width()+3,$(l).height()+3, 'ebox', 'ebox', param.isReadOnly?0:3);
    else
	pg_addarea(l, -1,-1,$(l).width()+1,$(l).height()+1, 'ebox', 'ebox', param.isReadOnly?0:3);
    if (param.form)
	l.form = wgtrGetNode(l, param.form);
    else
	l.form = wgtrFindContainer(l,"widget/form");
    if (l.form) l.form.Register(l);
    l.changed = false;
    l.oldvalue = null;
    l.value = null;
    l.DoDataChange = eb_do_data_change;

    if (l.form)
	{
	l.Reveal = eb_cb_reveal;
	if (pg_reveal_register_listener(l)) 
	    {
	    // already visible
	    l.form.Reveal(l,{ eventName:'Reveal' });
	    }
	}

    // Callback when user is trying to change the content.
    $(l.ContentLayer).on("input", eb_receiving_input);
    $(l.ContentLayer).on("keydown", eb_keydown);
    $(l.ContentLayer).on("keyup", eb_keyup);
    $(l.ContentLayer).on("keypress", eb_keypress);
    $(l.ContentLayer).on("focus", eb_checkfocus);
    $(l.ContentLayer).css({"outline":"none", "border":"1px transparent", "background-color":"transparent"});

    // Callbacks for internal management of 'content' value
    l.set_content = eb_set_content;
    l.set_desc = eb_setdesc;
    l.content_changed = eb_content_changed;
    l.input_width = eb_inputwidth;

    // Hot properties
    htr_watch(l,'content','content_changed');
    //htr_watch(l,'value','content_changed');

    //var iv = l.ifcProbeAdd(ifValue);
    //iv.Add("content", eb_v_get_content, eb_v_set_content);
    //iv.Add("value", eb_v_get_value, eb_v_set_value);

    // Events
    var ie = l.ifcProbeAdd(ifEvent);
    ie.Add("Click");
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseMove");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("GetFocus");
    ie.Add("LoseFocus");
    ie.Add("DataChange");
    ie.Add("BeforeDataChange");
    ie.Add("DataModify");
    ie.Add("BeforeKeyPress");
    ie.Add("KeyPress");
    ie.Add("TabPressed");
    ie.Add("ReturnPressed");
    ie.Add("EscapePressed");

    // Actions
    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("SetValue", eb_actionsetvalue);
    ia.Add("SetValueDescription", eb_actionsetvaldesc);
    ia.Add("Enable", eb_enable);
    ia.Add("Disable", eb_disable);
    ia.Add("SetFocus", eb_action_set_focus);

    if (l.empty_desc) l.Update('');

    return l;
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_editbox.js'] = true;
