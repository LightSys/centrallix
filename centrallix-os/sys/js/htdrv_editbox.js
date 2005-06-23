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

function eb_mkkeyboard(p)
    {
    }

function eb_getvalue()
    {
    return this.content;
    }

function eb_update_cursor(eb,val)
    {
    if(eb.cursorCol>val.length);
	{
	eb.cursorCol=0;
	}
    if(eb.cursorlayer == ibeam_current)
	{
	moveToAbsolute(ibeam_current, getPageX(eb.ContentLayer) + eb.cursorCol*text_metric.charWidth, getPageY(eb.ContentLayer));
	}
    }

function eb_actionsetvalue(aparam)
    {
    if (aparam.Value) this.setvalue(aparam.Value);
    }

function eb_setvalue(v,f)
    {
    eb_settext(this,String(v));
    eb_update_cursor(this,String(v));
    }

function eb_clearvalue()
    {
    eb_settext(this,new String(''));
    eb_update_cursor(this,String(''));
    }

function eb_enable()
    {
    if (cx__capabilities.Dom0IE)
        pg_set_style(this,'bgColor',this.bg);
    else
	eval('this.document.'+this.xbg);
    this.enabled='full';
    }

function eb_disable()
    {
    this.document.background='';
    pg_set_style(this.document, 'bgColor','#e0e0e0');
    this.enabled='disabled';
    }

function eb_readonly()
    {
    eval('this.document.'+this.bg);
    this.enabled='readonly';
    }

function eb_settext_cb()
    {
    htr_setvisibility(this.mainlayer.HiddenLayer, 'inherit');
    htr_setvisibility(this.mainlayer.ContentLayer, 'hidden');
    
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
	l.content=txt;
	pg_serialized_write(l.HiddenLayer, '<PRE style="padding:0px; margin:0px;">' + htutil_encode(txt) + '</PRE> ', eb_settext_cb);
	}
    }

function eb_keyhandler(l,e,k)
    {
    if(!eb_current) return;
    if(eb_current.enabled!='full') return 1;
    if(k != 9 && k != 10 && k != 13 && k != 27 && eb_current.form) eb_current.form.DataNotify(eb_current);
    var adj = 0;
    var txt = l.content;
    var newtxt;
    var charClip = Math.ceil((getPageX(l) - getPageX(l.ContentLayer)) / text_metric.charWidth);
    var relPos = l.cursorCol - charClip;
    if (k == 9)
	{
	if(l.form) l.form.TabNotify(this);
	cn_activate(l,'TabPressed');
	}
    if (k == 10 || k == 13)
	{
	if(l.form) l.form.RetNotify(this);
	cn_activate(l,'ReturnPressed');
	}
    if (k == 27)
	{
	if (l.form) l.form.EscNotify(this);
	cn_activate(l,'EscapePressed');
	}
    if (k >= 32 && k < 127)
	{
	newtxt = cx_hints_checkmodify(l,txt,txt.substr(0,l.cursorCol) + String.fromCharCode(k) + txt.substr(l.cursorCol,txt.length), l._form_type);
	if (newtxt != txt)
	    {
	    l.cursorCol++;
	    if (relPos >= l.charWidth) adj = -text_metric.charWidth; 
	    }
	}
    else if (k == 8 && l.cursorCol > 0)
	{
	newtxt = cx_hints_checkmodify(l,txt,txt.substr(0,l.cursorCol-1) + txt.substr(l.cursorCol,txt.length));
	if (newtxt != txt)
	    {
	    l.cursorCol--;
	    if (relPos <= 1 && charClip > 0)
		{
		if (charClip < l.charWidth) adj = charClip * text_metric.charWidth;
		else adj = l.charWidth * text_metric.charWidth;
		}
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
    pg_set_style(ibeam_current, 'visibility', 'hidden');
    eb_settext(l,newtxt);
    setPageX(l.ContentLayer, getPageX(l.ContentLayer) + adj);
    setPageX(l.HiddenLayer, getPageX(l.HiddenLayer)+ adj);
    moveToAbsolute(ibeam_current, getPageX(l.ContentLayer) + l.cursorCol*text_metric.charWidth, getPageY(l.ContentLayer));
    pg_set_style(ibeam_current,'visibility', 'inherit');
    l.changed=true;
    cn_activate(l,"DataChange");
    return false;
    }

function eb_select(x,y,l,c,n)
    {
    if(l.enabled != 'full') return 0;
    if(l.form) l.form.FocusNotify(l);
    if (x == null && y == null)
	l.cursorCol = l.content.length;
    else
	l.cursorCol = Math.round((x + getPageX(l) - getPageX(l.ContentLayer))/text_metric.charWidth);
    if (l.cursorCol > l.content.length) l.cursorCol = l.content.length;
    if (eb_current) eb_current.cursorlayer = null;     
    eb_current = l;    
    eb_current.cursorlayer = ibeam_current;    
    setPageX(eb_current.ContentLayer, getPageX(eb_current)+1);
    setPageX(eb_current.HiddenLayer, getPageX(eb_current)+1);
    htr_setvisibility(ibeam_current, 'hidden');
    if (cx__capabilities.Dom1HTML)
	l.appendChild(ibeam_current);
    moveAbove(ibeam_current,eb_current);
    moveToAbsolute(ibeam_current, getPageX(eb_current.ContentLayer) + eb_current.cursorCol*text_metric.charWidth, getPageY(eb_current.ContentLayer));    
    htr_setzindex(ibeam_current, htr_getzindex(eb_current) + 2);
    htr_setvisibility(ibeam_current, 'inherit');
    cn_activate(l,"GetFocus");
    return 1;
    }

function eb_deselect()
    {
    htr_setvisibility(ibeam_current, 'hidden');
    cn_activate(eb_current,"LoseFocus");
    if (eb_current)
	{
	eb_current.cursorlayer = null;
	if (eb_current.changed) eb_current.changed=false;
	setPageX(eb_current.ContentLayer, getPageX(eb_current)+1);
	setPageX(eb_current.HiddenLayer, getPageX(eb_current)+1);
	eb_current = null;
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
    var c2 = param.c2; 
    if (!param.mainBackground)
	{
	if (cx__capabilities.Dom0NS)
	    {
	    l.bg = "bgcolor='#c0c0c0'";
	    }
	else if (cx__capabilities.Dom0IE)
	    {
	    l.bg = "backgroundColor='#c0c0c0'";
	    }
	}
    else
	{
	l.bg = param.mainBackground;
	}
    htr_init_layer(l,l,'eb');
    htr_init_layer(c1,l,'eb');
    htr_init_layer(c2,l,'eb');
    l.ActionSetValue = eb_actionsetvalue;
    l.ContentLayer = c1;
    l.HiddenLayer = c2;
    l.fieldname = param.fieldname;
    l.is_busy = false;
    ibeam_init();
    l.charWidth = Math.floor((getClipWidth(l)-3)/text_metric.charWidth);
    l.content = '';
    l.tmpcontent = '';
    l.keyhandler = eb_keyhandler;
    l.getfocushandler = eb_select;
    l.losefocushandler = eb_deselect;
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
    if (fm_current) fm_current.Register(l);
    l.form = fm_current;
    l.changed = false;
    return l;
    }
