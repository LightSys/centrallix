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
	//var olddesc = this.saved_description;
	//var oldvdv = this.description_value;
	this.descriptions[this.content] = aparam.Description;
	//if (olddesc != aparam.Description || oldvdv != this.content)
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
    l.ContentLayer.value = vistxt;

    /*var wl = l.dbl_buffer?l.HiddenLayer:l.ContentLayer;
    var enctxt = '<pre style="padding:0px; margin:0px;">' + htutil_encode(htutil_obscure(vistxt));*/
    /*if (descr)
	enctxt += '<span style="color:' + htutil_encode(l.desc_fgcolor) + '">' + ((l.content == '' || l.content == null)?'':' ') + '(' + htutil_encode(htutil_obscure(descr)) + ')</span>';
    enctxt += '</pre>';
    if (!l.is_busy)
	{
	l.is_busy = true;
	l.viscontent = txt;
	if (cx__capabilities.Dom0NS) // only serialize EB's for NS4
	    {
	    pg_serialized_write(wl, enctxt, eb_settext_cb);
	    }
	else
	    {
	    htr_write_content(wl, enctxt);
	    l.eb_settext_cb();
	    }
	}*/
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

    // Make sure ibeam cursor is within visual area.   ******|**[******          ]
    /*var wpos = $(this).offset();
    var cpos = $(this.ContentLayer).offset();
    var sel_rect = this.range.getClientRects()[0];
    if (!sel_rect)
	return;
    var cursor_x = sel_rect.left;
    var new_left = cpos.left;
    while (cursor_x + (new_left - cpos.left) < wpos.left)
	new_left += Math.ceil($(this).width()/3);
    while (cursor_x + (new_left - cpos.left) > $(this).width() + new_left)
	new_left -= Math.ceil($(this).width()/3);
    if (new_left > wpos.left)
	new_left = wpos.left
    $(this.ContentLayer).offset({left:new_left, top:cpos.top});*/

    // Set left/right arrows indicating more content available
    //eb_set_l_img(this, new_left < wpos.left);
    //eb_set_r_img(this, new_left + $(this.ContentLayer).width() > $(this).width() + wpos.left);

    /*var newx;
    var newclipl, newclipr;
    var wl = this.dbl_buffer?this.HiddenLayer:this.ContentLayer;
    var diff = cursor - this.cursorCol;
    if (txt != null)
	txt = new String(txt);
    var ldiff = eb_length(txt) - eb_length(this.content);
    this.cursorCol = cursor;
    if (this.cursorCol < 0) this.cursorCol = 0;
    if (this.cursorCol > this.charWidth + this.charOffset)
	{
	this.charOffset = this.cursorCol - this.charWidth;
	}
    if (this.cursorCol < this.charOffset)
	{
	this.charOffset = this.cursorCol;
	}
    if (this.charOffset > 0 && this.cursorCol - this.charOffset < this.charWidth*2/3 && diff < 0)
	{
	this.charOffset--;
	}
    if (this.charOffset < eb_length(txt) - this.charWidth && this.cursorCol - this.charOffset > this.charWidth*2/3 && ldiff < 0)
	{
	this.charOffset = this.cursorCol - parseInt(this.charWidth*2/3);
	}
    if (eb_current == this)
	pg_set_style(ibeam_current, 'visibility', 'hidden');
    newx = 5 - this.charOffset*text_metric.charWidth;
    newclipl = this.charOffset*text_metric.charWidth;
    newclipr = newclipl + this.charWidth*text_metric.charWidth;
    if (wl._eb_x != newx)
	{
	setRelativeX(wl, newx);
	wl._eb_x = newx;
	}
    if (wl._eb_clipl != newclipl)
	{
	setClipLeft(wl, newclipl);
	wl._eb_clipl = newclipl;
	}
    if (wl._eb_clipr != newclipr)
	{
	setClipRight(wl, newclipr);
	wl._eb_clipr = newclipr;
	}
    if (eb_current == this)
	moveToAbsolute(ibeam_current, getPageX(wl) + this.cursorCol*text_metric.charWidth, getPageY(wl));
    eb_settext(this, txt);
    eb_set_l_img(this, this.charOffset > 0);
    eb_set_r_img(this, this.charOffset + this.charWidth < eb_length(txt));
    if (eb_current == this)
	pg_set_style(ibeam_current,'visibility', 'inherit');*/
    }


function eb_paste(e)
    {
/*    if (eb_current && e.pastedText)
	{
	var pasted = new String(e.pastedText);
	for(var i=0; i<pasted.length; i++)
	    {
	    var k = pasted.charCodeAt(i);

	    // Convert control codes into spaces.
	    if (k < 32  || k == 127)
		k = 32;

	    // turn it into a keypress.
	    eb_keyhandler(eb_current, {}, k);
	    }
	}*/
    }


function eb_receiving_input(e)
    {
    var eb=this.mainlayer;
    var sel = document.getSelection();
    var range = sel.getRangeAt(0);
    var rstart = range.startOffset;
    var rend = range.endOffset;
    //var curtxt = $(this).text();
    //var curtxt = htutil_encode($(this).html(), true).replace(/&lt;br&gt;/g,"\n").replace(/\n$/,"")
    //var orig_curtxt=$(this).html().replace(/<br[^>]*>/g,"\n").replace(/<[^>]*>/g,"");
    var orig_curtxt = this.value;
    var changed = false;
    var curtxt = orig_curtxt.replace(/\n$/,"");

    if (curtxt != orig_curtxt)
	changed = true;
    if (rend > curtxt.length)
	{
	changed = true;
	rend = curtxt.length;
	}
    if (rstart > curtxt.length)
	{
	changed = true;
	rstart = curtxt.length;
	}
    for(var i=0; i<curtxt.length; i++)
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
	range.setStart(range.startContainer, rstart);
	range.setEnd(range.startContainer, rend);
	}

    var oldtxt = eb.content;
    var newcurtxt = cx_hints_checkmodify(eb, oldtxt, curtxt, eb._form_type);
    if (newcurtxt != curtxt)
	pg_addsched_fn(eb, function()
	    {
	    eb.Update(curtxt = newcurtxt);
	    }, {}, 10);
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
/*    if (e.keyName == 'escape') window.ebesccnt=window.ebesccnt?(window.ebesccnt+1):1;
    if(!eb_current) return;
    if(eb_current.enabled!='full') return 1;
    var txt = l.content;
    var vistxt = (txt == null)?'':txt;
    var newtxt = txt;
    var cursoradj = 0;
    if (isCancel(l.ifcProbe(ifEvent).Activate('BeforeKeyPress', {Code:k, Name:e.keyName})))
	return false;
    if (k == 9 && !e.shiftKey)
	{
	if(l.form) l.form.TabNotify(this);
	cn_activate(l,'TabPressed', {Shift:0});
	}
    if (k == 9 && e.shiftKey)
	{
	if(l.form) l.form.ShiftTabNotify(this);
	cn_activate(l,'TabPressed', {Shift:1});
	}
    if (k == 10 || k == 13)
	{
	if (l.form)
	    l.form.RetNotify(this);
	l.addHistory();
	cn_activate(l,'ReturnPressed', {});
	}
    if (k == 27)
	{
	if (l.form) l.form.EscNotify(this);
	cn_activate(l,'EscapePressed', {});
	}
    if (k >= 32 && k < 127 && !e.ctrlKey)
	{
	newtxt = cx_hints_checkmodify(l,txt,vistxt.substr(0,l.cursorCol) + String.fromCharCode(k) + vistxt.substr(l.cursorCol,vistxt.length), l._form_type);
	if (newtxt != txt)
	    {
	    cursoradj = 1;
	    }
	}
    else if (k == 8 && l.cursorCol > 0)
	{
	newtxt = cx_hints_checkmodify(l,txt,vistxt.substr(0,l.cursorCol-1) + vistxt.substr(l.cursorCol,eb_length(txt)));
	if (newtxt != txt)
	    {
	    cursoradj = -1;
	    }
	}
    else if (k == 21 && eb_length(txt) > 0)
	{
	newtxt = "";
	cursoradj = -l.cursorCol;
	}
    else if (k == 127 && l.cursorCol < eb_length(txt))
	{
	newtxt = cx_hints_checkmodify(l,txt,vistxt.substr(0,l.cursorCol) + vistxt.substr(l.cursorCol+1,eb_length(txt)));
	}
    else if (k == 0 && e.keyName == 'home')
	{
	cursoradj = -l.cursorCol;
	}
    else if (k == 0 && e.keyName == 'end')
	{
	cursoradj = eb_length(txt) - l.cursorCol;
	}
    else if (k == 0 && e.keyName == 'left' && l.cursorCol > 0)
	{
	cursoradj = -1;
	}
    else if (k == 0 && e.keyName == 'right' && l.cursorCol < eb_length(txt))
	{
	cursoradj = 1;
	}
    else if (k == 0 && e.keyName == 'up' && l.hist_offset < l.value_history.length - 1)
	{
	if (l.hist_offset == -1)
	    {
	    if (l.addHistory())
		l.hist_offset = 0;
	    }
	l.hist_offset++;
	newtxt = l.value_history[l.hist_offset];
	if (l.cursorCol > eb_length(newtxt) || l.cursorCol == eb_length(txt))
	    cursoradj = eb_length(newtxt) - l.cursorCol;
	}
    else if (k == 0 && e.keyName == 'down')
	{
	if (l.hist_offset == -1)
	    {
	    l.addHistory();
	    newtxt = "";
	    cursoradj = -l.cursorCol;
	    }
	else if (l.hist_offset == 0)
	    {
	    l.hist_offset--;
	    newtxt = "";
	    cursoradj = -l.cursorCol;
	    }
	else
	    {
	    l.hist_offset--;
	    newtxt = l.value_history[l.hist_offset];
	    if (l.cursorCol > eb_length(newtxt) || l.cursorCol == eb_length(txt))
		cursoradj = eb_length(newtxt) - l.cursorCol;
	    }
	}
    else
	{
	return true;
	}
    if (newtxt != txt || cursoradj != 0)
	{
	if (l.was_null && newtxt == '') newtxt = null;
	l.Update(newtxt, l.cursorCol + cursoradj);
	}
    if (newtxt != txt)
	{
	//if(k != 9 && k != 10 && k != 13 && k != 27 && eb_current.form) 
	if (l.form) l.form.DataNotify(l);
	l.changed=true;
	cn_activate(l,"DataModify", {Value:newtxt, FromKeyboard:1, FromOSRC:0, OldValue:txt});
	}
    if (k == 13 || k == 9 || k == 10)
	l.DoDataChange(0, 1);
	//cn_activate(l, "DataChange", {Value:newtxt, FromOSRC:0, FromKeyboard:1});
    cn_activate(l, "KeyPress", {Code:k, Name:e.keyName, Modifiers:e.modifiers, Content:l.content});*/
    cn_activate(l, "KeyPress", {Code:k, Name:e.keyName, Modifiers:e.modifiers, Content:l.content});
    if (e.keyName == 'f3') return true;
    return false;
    }


function eb_do_data_change(from_osrc, from_kbd)
    {
    var nv = cx_hints_checkmodify(this, this.value, this.content, null, true);
    if (nv != this.content)
	{
	this.internal_setvalue(nv);
	//if (from_kbd && this.form) this.form.DataNotify(this);
	}
    if (isCancel(this.ifcProbe(ifEvent).Activate('BeforeDataChange', {OldValue:this.value, Value:nv, FromOSRC:from_osrc, FromKeyboard:from_kbd})))
	{
	this.internal_setvalue(this.value);
	//if (from_kbd && this.form) this.form.DataNotify(this);
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
	pg_addsched_fn(this.ContentLayer, function() { this.focus() }, {}, 200);;
    if (k)
	pg_addsched_fn(this, function()
	    {
	    this.ContentLayer.setSelectionRange(eb_length(this.content), eb_length(this.content));
	    this.Update(this.content);
	    }, [], got_focus?10:201);
    this.has_focus = true;
    if(this.form)
	if (!this.form.FocusNotify(this)) return 0;
    cn_activate(this,"GetFocus", {});
    /*if(l.enabled != 'full') return 0;
    if(l.form)
	{
	if (!l.form.FocusNotify(l)) return 0;
	}
    if (k)
	l.cursorCol = eb_length(l.content);
    else
	l.cursorCol = Math.round((x + getPageX(l) - getPageX(l.ContentLayer))/text_metric.charWidth);
    if (l.cursorCol > eb_length(l.content)) l.cursorCol = eb_length(l.content);
    if (eb_current) eb_current.cursorlayer = null;     
    eb_current = l;    
    eb_current.cursorlayer = ibeam_current;    
    eb_grab_ibeam();
    eb_current.Update(eb_current.content, eb_current.cursorCol);
    htr_setvisibility(ibeam_current, 'inherit');
    cn_activate(l,"GetFocus", {});*/
    return 1;
    }

function eb_deselect(p)
    {
    this.ContentLayer.blur();
    this.has_focus = false;
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
    /*htr_setvisibility(ibeam_current, 'hidden');
    if (eb_current)
	{
	cn_activate(eb_current,"LoseFocus", {});
	eb_current.cursorlayer = null;
	if (eb_current.changed)
	    {
	    if (!p || !p.nodatachange)
		{
		eb_current.DoDataChange(0, 1);
		//cn_activate(eb_current,"DataChange", {Value:eb_current.content, FromOSRC:0, FromKeyboard:1});
		eb_current.changed=false;
		}
	    }
	eb_current.charOffset=0;
	eb_current.cursorCol=0;
	var eb = eb_current;
	eb_current = null;
	eb.Update(eb.content, eb.cursorCol);
	htr_setvisibility(ibeam_current, 'hidden');
	eb.addHistory();
	}*/
    return true;
    }


function eb_mselect(x,y,l,c,n,a)
    {
    if (this.ContentLayer.scrollWidth > this.ContentLayer.clientWidth)
	this.tipid = pg_tooltip(this.tooltip?this.tooltip:this.content, getPageX(this) + x, getPageY(this) + y);
    else if (this.tooltip)
	this.tipid = pg_tooltip(this.tooltip, getPageX(this) + x, getPageY(this) + y);
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
* c2 - content layer 2 - hidden
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
    //ibeam_init();

    l.range = document.createRange();

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
    l.ContentLayer._eb_x = -1;
    l.ContentLayer._eb_clipr = -1;
    l.ContentLayer._eb_clipl = -1;
    l.is_busy = false;
    //l.charWidth = Math.floor((getClipWidth(l)-10)/text_metric.charWidth);
    l.cursorCol = 0;
    l.charOffset = 0;
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
    //l.eb_settext_cb = eb_settext_cb;
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
	pg_addarea(l, -1,-1,getClipWidth(l)+3,getClipHeight(l)+3, 'ebox', 'ebox', param.isReadOnly?0:3);
    else
	pg_addarea(l, -1,-1,getClipWidth(l)+1,getClipHeight(l)+1, 'ebox', 'ebox', param.isReadOnly?0:3);
    //setRelativeY(c1, (getClipHeight(l) - text_metric.charHeight)/2 + (cx__capabilities.CSSBox?1:0));
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
    //$(l.ContentLayer).attr("contentEditable", "true");
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
