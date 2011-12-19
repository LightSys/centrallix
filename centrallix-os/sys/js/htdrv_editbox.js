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
    //htr_alert(aparam,1);
    var oldval = this.content;
    if ((typeof aparam.Value) != 'undefined')
	{
	this.internal_setvalue(aparam.Value);
	//cn_activate(this,"DataChange", {Value:aparam.Value, FromKeyboard:0, FromOSRC:0});
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
	var olddesc = this.saved_description;
	var oldvdv = this.description_value;
	this.saved_description = aparam.Description;
	this.description_value = this.content;
	if (olddesc != this.saved_description || oldvdv != this.description_value)
	    this.Update(this.content, this.cursorCol);
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
    this.Update(v, c);
    this.addHistory(v);
    }

function eb_setvalue(v,f)
    {
    this.internal_setvalue(v);
    this.DoDataChange(1, 0);
    //cn_activate(this,"DataChange", {Value:v, FromKeyboard:0, FromOSRC:1});
    }

function eb_clearvalue()
    {
    this.was_null = true;
    // fake it by just making the content invisible - much faster :)
    if (this != eb_current && !this.empty_desc)
	{
	this.set_content(null);
	this.viscontent = null;
	this.charOffset = 0;
	this.cursorCol = 0;
	htr_setvisibility(this.mainlayer.ContentLayer, 'hidden');
	eb_set_l_img(this, false);
	eb_set_r_img(this, false);
	}
    else
	{
	this.Update(null, 0);
	}
    //cn_activate(this,"DataChange", {Value:null, FromKeyboard:0, FromOSRC:1});
    this.DoDataChange(1, 0);
    }

function eb_content_changed(p,o,n)
    {
    this.Update('' + n, 0);
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
    //if (cx__capabilities.Dom0IE)
    //    pg_set_style(this,'bgColor',this.bg);
    //else
    //	eval('this.document.'+this.xbg);
    this.enabled='full';
    }

function eb_disable()
    {
    if (this.bg) htr_setbgcolor(this, '#e0e0e0');
    //this.document.background='';
    //pg_set_style(this.document, 'bgColor','#e0e0e0');
    this.enabled='disabled';
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
    //eval('this.document.'+this.bg);
    this.enabled='readonly';
    }

function eb_settext_cb()
    {
    if (this.mainlayer.dbl_buffer)
	{
	htr_setvisibility(this.mainlayer.HiddenLayer, 'inherit');
	htr_setvisibility(this.mainlayer.ContentLayer, 'hidden');
	//setRelativeX(this.mainlayer.ContentLayer, getRelativeX(this.mainlayer.HiddenLayer));
	//setClipLeft(this.mainlayer.ContentLayer, getClipLeft(this.mainlayer.HiddenLayer));
	//setClipWidth(this.mainlayer.ContentLayer, getClipWidth(this.mainlayer.HiddenLayer));
    
	var tmp = this.mainlayer.ContentLayer;
	this.mainlayer.ContentLayer = this.mainlayer.HiddenLayer;
	this.mainlayer.HiddenLayer = tmp;
	}
    else
	{
	htr_setvisibility(this.mainlayer.ContentLayer, 'inherit');
	}

    this.mainlayer.is_busy = false;
    if (this.mainlayer.content != this.mainlayer.viscontent)
	eb_settext(this.mainlayer, this.mainlayer.content);
    }

function eb_settext(l,txt)
    {
    var vistxt = txt;
    if (vistxt == null) vistxt = '';
    l.set_content(txt);
    var wl = l.dbl_buffer?l.HiddenLayer:l.ContentLayer;
    var enctxt = '<pre style="padding:0px; margin:0px;">' + htutil_encode(htutil_obscure(vistxt));
    var descr = '';
    if ((new String(l.content)).valueOf() == (new String(l.description_value)).valueOf() && l.saved_description)
	{
	if (l != eb_current || l.content)
	    descr = l.saved_description;
	}
    if (descr != l.description)
	l.description = descr;
    if (l != eb_current && (l.content == '' || l.content == null) && l.empty_desc)
	descr = l.empty_desc;
    if (descr)
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
	}
    }


// Grab the ibeam and own it
function eb_grab_ibeam()
    {
    htr_setvisibility(ibeam_current, 'hidden');
    if (cx__capabilities.Dom1HTML)
	eb_current.appendChild(ibeam_current);
    moveAbove(ibeam_current,eb_current);
    htr_setzindex(ibeam_current, htr_getzindex(eb_current) + 2);
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
    if (txt == null) txt = this.content;
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
function eb_update(txt, cursor)
    {
    var newx;
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
    /*if (this.charOffset > 0 && txt.length < this.charWidth)
	{
	this.charOffset = 0;
	}*/
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
	pg_set_style(ibeam_current,'visibility', 'inherit');
    }


function eb_keyhandler(l,e,k)
    {
    if (e.keyName == 'escape') window.ebesccnt=window.ebesccnt?(window.ebesccnt+1):1;
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
    if (k >= 32 && k < 127)
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
	//if (k != 27 && pg_username == 'gbeeley') htr_alert(e, 1);
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
    cn_activate(l, "KeyPress", {Code:k, Name:e.keyName, Modifiers:e.modifiers, Content:l.content});
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

function eb_select(x,y,l,c,n,a,k)
    {
    if(l.enabled != 'full') return 0;
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
    cn_activate(l,"GetFocus", {});
    return 1;
    }

function eb_deselect(p)
    {
    htr_setvisibility(ibeam_current, 'hidden');
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
	}
    return true;
    }


function eb_mselect(x,y,l,c,n,a)
    {
    if (this.charOffset > 0 || this.charOffset + this.charWidth < eb_length(this.content))
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

    if (param.c2)
	{
	var c2 = param.c2; 
	l.dbl_buffer = true;
	}
    else
	{
	l.dbl_buffer = false;
	}

    if (!param.mainBackground)
	{
	l.bg = '#c0c0c0';
	/*if (cx__capabilities.Dom0NS)
	    {
	    l.bg = "bgcolor='#c0c0c0'";
	    }
	else if (cx__capabilities.Dom0IE)
	    {
	    l.bg = "backgroundColor='#c0c0c0'";
	    }*/
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
    if (l.dbl_buffer) 
	htr_init_layer(c2,l,'eb');
    ifc_init_widget(l);
    l.fieldname = param.fieldname;
    l.tooltip = param.tooltip;
    ibeam_init();

    // Left/Right arrow images
    var imgs = pg_images(l);
    for(var i=0;i<imgs.length;i++)
	{
	if (imgs[i].name == 'l') l.l_img = imgs[i];
	else if (imgs[i].name == 'r') l.r_img = imgs[i];
	}
    l.l_img_on = false;
    l.r_img_on = false;

    // Set up params for displaying the content.
    l.ContentLayer = c1;
    if (l.dbl_buffer)
	{
	l.HiddenLayer = c2;
	l.HiddenLayer._eb_x = -1;
	l.HiddenLayer._eb_clipr = -1;
	l.HiddenLayer._eb_clipl = -1;
	}
    l.ContentLayer._eb_x = -1;
    l.ContentLayer._eb_clipr = -1;
    l.ContentLayer._eb_clipl = -1;
    l.is_busy = false;
    l.charWidth = Math.floor((getClipWidth(l)-10)/text_metric.charWidth);
    l.cursorCol = 0;
    l.charOffset = 0;
    l.viscontent = '';
    l.content = '';
    l.value = '';
    l.description = '';
    l.saved_description = '';
    l.description_value = '';
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
    l.eb_settext_cb = eb_settext_cb;
    l.enable = eb_enable;
    if (param.isReadOnly)
	{
	l.enablemodify = eb_disable;
	l.enabled = 'disable';
	}
    else
	{
	l.enablemodify = eb_enable;
	l.enabled = 'full';
	}
    l.isFormStatusWidget = false;
    if (cx__capabilities.CSSBox)
	pg_addarea(l, -1,-1,getClipWidth(l)+3,getClipHeight(l)+3, 'ebox', 'ebox', param.isReadOnly?0:3);
    else
	pg_addarea(l, -1,-1,getClipWidth(l)+1,getClipHeight(l)+1, 'ebox', 'ebox', param.isReadOnly?0:3);
    setRelativeY(c1, (getClipHeight(l) - text_metric.charHeight)/2 + (cx__capabilities.CSSBox?1:0));
    if (l.dbl_buffer)
	setRelativeY(c2, (getClipHeight(l) - text_metric.charHeight)/2 + (cx__capabilities.CSSBox?1:0));
    if (param.form)
	l.form = wgtrGetNode(l, param.form);
    else
	l.form = wgtrFindContainer(l,"widget/form");
    if (l.form) l.form.Register(l);
    l.changed = false;
    l.oldvalue = null;
    l.value = null;
    l.DoDataChange = eb_do_data_change;

    // Callbacks for internal management of 'content' value
    l.set_content = eb_set_content;
    l.content_changed = eb_content_changed;

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

    if (l.empty_desc) l.Update('', 0);

    return l;
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_editbox.js'] = true;
