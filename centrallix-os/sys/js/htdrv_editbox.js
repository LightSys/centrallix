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
    if(eb.cursorlayer == eb_ibeam)
	{
	eb_ibeam.moveToAbsolute(eb.ContentLayer.pageX + eb.cursorCol*eb_metric.charWidth, eb.ContentLayer.pageY);
	}
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
    eval('this.document.'+this.bg);
    this.enabled='full';
    }

function eb_disable()
    {
    this.document.background='';
    this.document.bgColor='#e0e0e0';
    this.enabled='disabled';
    }

function eb_readonly()
    {
    eval('this.document.'+this.bg);
    this.enabled='readonly';
    }

function eb_settext(l,txt)
    {
    l.HiddenLayer.document.write('<PRE>' + htutil_encode(txt) + '</PRE> ');
    l.HiddenLayer.document.close();
    l.HiddenLayer.visibility = 'inherit';
    l.ContentLayer.visibility = 'hidden';
    tmp = l.ContentLayer;
    l.ContentLayer = l.HiddenLayer;
    l.HiddenLayer = tmp;
    l.content=txt;
    }

function eb_keyhandler(l,e,k)
    {
    if(!eb_current) return;
    if(eb_current.enabled!='full') return 1;
    if(eb_current.form) eb_current.form.DataNotify(eb_current);
    adj = 0;
    txt = l.content;
    var charClip = Math.ceil((l.pageX - l.ContentLayer.pageX) / eb_metric.charWidth);
    var relPos = l.cursorCol - charClip;
    if (k == 9)
	{
	if(l.form) l.form.TabNotify(this)
	}
    if (k >= 32 && k < 127)
	{
	newtxt = txt.substr(0,l.cursorCol) + String.fromCharCode(k) + txt.substr(l.cursorCol,txt.length);
	l.cursorCol++;
	if (relPos >= l.charWidth) adj = -eb_metric.charWidth; 
	}
    else if (k == 8 && l.cursorCol > 0)
	{
	newtxt = txt.substr(0,l.cursorCol-1) + txt.substr(l.cursorCol,txt.length);
	l.cursorCol--;
	if (relPos <= 1 && charClip > 0)
	    {
	    if (charClip < l.charWidth) adj = charClip * eb_metric.charWidth;
	    else adj = l.charWidth * eb_metric.charWidth;
	    }
	}
    else if (k == 127 && l.cursorCol < txt.length)
	{
	newtxt = txt.substr(0,l.cursorCol) + txt.substr(l.cursorCol+1,txt.length);
	}
    else
	{
	return true;
	}
    eb_ibeam.visibility = 'hidden';
    eb_settext(l,newtxt);
    l.ContentLayer.pageX += adj;
    l.HiddenLayer.pageX += adj;
    eb_ibeam.moveToAbsolute(l.ContentLayer.pageX + l.cursorCol*eb_metric.charWidth, l.ContentLayer.pageY);
    eb_ibeam.visibility = 'inherit';
    l.changed=true;
    return false;
    }

function eb_select(x,y,l,c,n)
    {
    if(l.form) l.form.FocusNotify(l);
    if(l.enabled != 'full') return 0;
    l.cursorCol = Math.round((x + l.pageX - l.ContentLayer.pageX)/eb_metric.charWidth);
    if (l.cursorCol > l.content.length) l.cursorCol = l.content.length;
    if (eb_current) eb_current.cursorlayer = null;
    eb_current = l;
    eb_current.cursorlayer = eb_ibeam;
    eb_ibeam.visibility = 'hidden';
    eb_ibeam.moveAbove(eb_current);
    eb_ibeam.moveToAbsolute(eb_current.ContentLayer.pageX + eb_current.cursorCol*eb_metric.charWidth, eb_current.ContentLayer.pageY);
    eb_ibeam.zIndex = eb_current.zIndex + 2;
    eb_ibeam.visibility = 'inherit';
    return 1;
    }

function eb_deselect()
    {
    eb_ibeam.visibility = 'hidden';
    if (eb_current) eb_current.cursorlayer = null;
    if(eb_current && eb_current.changed)
	{
	eb_current.changed=false;
	}
    eb_current.ContentLayer.pageX = eb_current.pageX;
    eb_current.HiddenLayer.pageX = eb_current.pageX;
    eb_current = null;
    return true;
    }

function eb_init(l,c1,c2,fieldname,is_readonly,main_bg)
    {
    if (!main_bg)
	{
	l.bg = "bgcolor='#c0c0c0'";
	}
    else
	{
	l.bg = main_bg;
	}
    l.kind = 'editbox';
    l.document.Layer = l;
    l.ContentLayer = c1;
    l.HiddenLayer = c2;
    l.fieldname = fieldname
    if (!eb_ibeam || !eb_metric)
	{
	eb_metric = new Layer(24);
	eb_metric.visibility = 'hidden';
	eb_metric.document.write('<pre>xx</pre>');
	eb_metric.document.close();
	w2 = eb_metric.clip.width;
	h1 = eb_metric.clip.height;
	eb_metric.document.write('<pre>x\nx</pre>');
	eb_metric.document.close();
	eb_metric.charHeight = eb_metric.clip.height - h1;
	eb_metric.charWidth = w2 - eb_metric.clip.width;
	eb_ibeam = new Layer(1);
	eb_ibeam.visibility = 'hidden';
	eb_ibeam.document.write('<body bgcolor=' + page.dtcolor1 + '></body>');
	eb_ibeam.document.close();
	eb_ibeam.resizeTo(1,eb_metric.charHeight);
	}
    l.charWidth = Math.floor((l.clip.width-2)/eb_metric.charWidth);
    c1.mainlayer = l;
    c2.mainlayer = l;
    c1.kind = 'eb';
    c2.kind = 'eb';
    l.content = '';
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
    if (is_readonly)
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
    pg_addarea(l, -1,-1,l.clip.width+1,l.clip.height+1, 'ebox', 'ebox', 1);
    c1.y = ((l.clip.height - eb_metric.charHeight)/2);
    c2.y = ((l.clip.height - eb_metric.charHeight)/2);
    if (fm_current) fm_current.Register(l);
    l.form = fm_current;
    l.changed = false;
    return l;
    }
