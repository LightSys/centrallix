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

function eb_getvalue()
    {
    return this.content;
    }

function eb_actionsetvalue(aparam)
    {
    if (aparam.Value) this.setvalue(aparam.Value);
    }

function eb_setvalue(v,f)
    {
    this.Update(v, 0);
    }

function eb_clearvalue()
    {
    // fake it by just making the content invisible - much faster :)
    if (this != eb_current)
	{
	this.set_content('');
	this.tmpcontent = '';
	this.charOffset = 0;
	this.cursorCol = 0;
	htr_setvisibility(this.mainlayer.ContentLayer, 'hidden');
	eb_set_l_img(this, false);
	eb_set_r_img(this, false);
	}
    else
	{
	this.Update('', 0);
	}
    }

function eb_content_changed(p,o,n)
    {
    this.Update('' + n, 0);
    return '' + n;
    }

function eb_set_content(v)
    {
    htr_unwatch(this, "content", "content_changed");
    this.content = v;
    htr_watch(this, "content", "content_changed");
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
    htr_setvisibility(this.mainlayer.HiddenLayer, 'inherit');
    htr_setvisibility(this.mainlayer.ContentLayer, 'hidden');
    setRelativeX(this.mainlayer.ContentLayer, getRelativeX(this.mainlayer.HiddenLayer));
    setClipLeft(this.mainlayer.ContentLayer, getClipLeft(this.mainlayer.HiddenLayer));
    setClipWidth(this.mainlayer.ContentLayer, getClipWidth(this.mainlayer.HiddenLayer));
    
    var tmp = this.mainlayer.ContentLayer;
    this.mainlayer.ContentLayer = this.mainlayer.HiddenLayer;
    this.mainlayer.HiddenLayer = tmp;
    this.mainlayer.is_busy = false;
    if (this.mainlayer.content != this.mainlayer.tmpcontent)
	eb_settext(this.mainlayer, this.mainlayer.tmpcontent);
    }

function eb_settext(l,txt)
    {
    l.tmpcontent = txt;
    if (!l.is_busy)
	{
	l.is_busy = true;
	l.set_content(txt);
	pg_serialized_write(l.HiddenLayer, '<pre style="padding:0px; margin:0px;">' + htutil_encode(txt) + '</pre> ', eb_settext_cb);
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


// Update the text, cursor, and left/right edge arrows
function eb_update(txt, cursor)
    {
    var newx;
    var newclipl, newclipw;
    var diff = cursor - this.cursorCol;
    var ldiff = txt.length - this.content.length;
    txt = String(txt);
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
    if (this.charOffset < txt.length - this.charWidth && this.cursorCol - this.charOffset > this.charWidth*2/3 && ldiff < 0)
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
    newclipw = this.charWidth*text_metric.charWidth;
    setRelativeX(this.HiddenLayer, newx);
    setClipLeft(this.HiddenLayer, newclipl);
    setClipWidth(this.HiddenLayer, newclipw);
    if (eb_current == this)
	moveToAbsolute(ibeam_current, getPageX(this.HiddenLayer) + this.cursorCol*text_metric.charWidth, getPageY(this.HiddenLayer));
    eb_settext(this, txt);
    eb_set_l_img(this, this.charOffset > 0);
    eb_set_r_img(this, this.charOffset + this.charWidth < txt.length);
    if (eb_current == this)
	pg_set_style(ibeam_current,'visibility', 'inherit');
    }


function eb_keyhandler(l,e,k)
    {
    if(!eb_current) return;
    if(eb_current.enabled!='full') return 1;
    var txt = l.content;
    var newtxt;
    var cursoradj = 0;
    if (k == 9)
	{
	if(l.form) l.form.TabNotify(this);
	cn_activate(l,'TabPressed', {});
	}
    if (k == 10 || k == 13)
	{
	if(l.form) l.form.RetNotify(this);
	cn_activate(l,'ReturnPressed', {});
	}
    if (k == 27)
	{
	if (l.form) l.form.EscNotify(this);
	cn_activate(l,'EscapePressed', {});
	}
    if (k >= 32 && k < 127)
	{
	newtxt = cx_hints_checkmodify(l,txt,txt.substr(0,l.cursorCol) + String.fromCharCode(k) + txt.substr(l.cursorCol,txt.length), l._form_type);
	if (newtxt != txt)
	    {
	    cursoradj = 1;
	    }
	}
    else if (k == 8 && l.cursorCol > 0)
	{
	newtxt = cx_hints_checkmodify(l,txt,txt.substr(0,l.cursorCol-1) + txt.substr(l.cursorCol,txt.length));
	if (newtxt != txt)
	    {
	    cursoradj = -1;
	    }
	}
    else if (k == 127 && l.cursorCol < txt.length)
	{
	newtxt = cx_hints_checkmodify(l,txt,txt.substr(0,l.cursorCol) + txt.substr(l.cursorCol+1,txt.length));
	}
    else
	{
	return true;
	}
    if (newtxt != txt || cursoradj != 0)
	{
	l.Update(newtxt, l.cursorCol + cursoradj);
	}
    if (newtxt != txt)
	{
	//if(k != 9 && k != 10 && k != 13 && k != 27 && eb_current.form) 
	if (l.form) l.form.DataNotify(l);
	l.changed=true;
	cn_activate(l,"DataChange", {});
	}
    return false;
    }

function eb_select(x,y,l,c,n,a,k)
    {
    if(l.enabled != 'full') return 0;
    if(l.form) l.form.FocusNotify(l);
    if (k)
	l.cursorCol = l.content.length;
    else
	l.cursorCol = Math.round((x + getPageX(l) - getPageX(l.ContentLayer))/text_metric.charWidth);
    if (l.cursorCol > l.content.length) l.cursorCol = l.content.length;
    if (eb_current) eb_current.cursorlayer = null;     
    eb_current = l;    
    eb_current.cursorlayer = ibeam_current;    
    eb_grab_ibeam();
    eb_current.Update(eb_current.content, eb_current.cursorCol);
    htr_setvisibility(ibeam_current, 'inherit');
    cn_activate(l,"GetFocus", {});
    return 1;
    }

function eb_deselect()
    {
    htr_setvisibility(ibeam_current, 'hidden');
    if (eb_current)
	{
	cn_activate(eb_current,"LoseFocus", {});
	eb_current.cursorlayer = null;
	if (eb_current.changed) eb_current.changed=false;
	eb_current.charOffset=0;
	eb_current.cursorCol=0;
	eb_current.Update(eb_current.content, eb_current.cursorCol);
	htr_setvisibility(ibeam_current, 'hidden');
	eb_current = null;
	}
    return true;
    }


function eb_mselect(x,y,l,c,n,a)
    {
    if (this.charOffset > 0 || this.charOffset + this.charWidth < this.content.length)
	this.tipid = pg_tooltip(this.content, getPageX(this) + x, getPageY(this) + y);
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
    var c2 = param.c2; 
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
    htr_init_layer(l,l,'eb');
    htr_init_layer(c1,l,'eb');
    htr_init_layer(c2,l,'eb');
    ifc_init_widget(l);
    l.fieldname = param.fieldname;
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
    l.HiddenLayer = c2;
    l.is_busy = false;
    l.charWidth = Math.floor((getClipWidth(l)-10)/text_metric.charWidth);
    l.cursorCol = 0;
    l.charOffset = 0;
    l.content = '';
    l.tmpcontent = '';
    l.Update = eb_update;

    // Callbacks
    l.keyhandler = eb_keyhandler;
    l.getfocushandler = eb_select;
    l.losefocushandler = eb_deselect;
    l.getmousefocushandler = eb_mselect;
    l.losemousefocushandler = eb_mdeselect;
    l.tipid = null;
    l.getvalue = eb_getvalue;
    l.setvalue = eb_setvalue;
    l.clearvalue = eb_clearvalue;
    l.setoptions = null;
    l.enablenew = eb_enable;  // We have added enablenew and enablemodify.  See docs
    l.disable = eb_disable;
    l.readonly = eb_readonly;
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
    setRelativeY(c2, (getClipHeight(l) - text_metric.charHeight)/2 + (cx__capabilities.CSSBox?1:0));
    l.form = null;
    if (param.form) l.form = wgtrGetNode(l, param.form);
    if (!l.form) l.form = wgtrFindContainer(l,"widget/form");
    if (l.form) l.form.Register(l);
    l.changed = false;

    // Callbacks for internal management of 'content' value
    l.set_content = eb_set_content;
    l.content_changed = eb_content_changed;

    // Hot properties
    htr_watch(l,'content','content_changed');

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
    ie.Add("TabPressed");
    ie.Add("ReturnPressed");
    ie.Add("EscapePressed");

    // Actions
    var ia = l.ifcProbeAdd(ifAction);
    ia.Add("SetValue", eb_actionsetvalue);

    return l;
    }
