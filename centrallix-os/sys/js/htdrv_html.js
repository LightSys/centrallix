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

function ht_loadpage(aparam)
    {
    this.transition = aparam.Transition;
    this.mode = aparam.Mode;
    this.source = aparam.Source;
    }

function ht_setvalue(aparam)
    {
    this.mainlayer.content = aparam.Value;
    if (aparam.ContentType) this.mainlayer.content_type = aparam.ContentType;
    }
function ht_addtext(aparam)
    {
    this.mainlayer.content += aparam.Text;
    if (aparam.ContentType && aparam.ContentType != this.mainlayer.content_type)
	{
	// remove all current content if changing type
	this.mainlayer.content = aparam.Text;
	this.mainlayer.content_type = aparam.ContentType;
	}
    }
function ht_showtext(aparam)
    {
    if (cx__capabilities.Dom0NS)
	{
	if (this.mainlayer.content_type == 'text/plain')
	    {
	    // special case for text/plain due to buggy NS4
	    var newtxt = '<pre>';
	    var htmltxt = this.mainlayer.content.replace(/&/g,'&amp;');
	    htmltxt = htmltxt.replace(/>/g,'&gt;');
	    htmltxt = htmltxt.replace(/</g,'&lt;');
	    newtxt += htmltxt;
	    newtxt += '</pre>';
	    this.document.open('text/html', 'replace');
	    }
	else
	    {
	    this.document.open(this.mainlayer.content_type, 'replace');
	    var newtxt = this.mainlayer.content;
	    }
	}
    else
	{
	newtxt = this.mainlayer.content;
	}
    htr_write_content(this, newtxt);
    setClipHeight(this, getdocHeight(this));
    pg_resize(this.mainlayer.parentLayer);
    }

function ht_sourcechanged(prop,oldval,newval)
    {
    htr_unwatch(this, 'source', 'ht_sourcechanged');
    if (this.mode != 'dynamic' || (this.mode == 'dynamic' && newval.substr(0,5)=='http:'))
	{
	this.newsrc = newval;
	if (this.transition && this.transition != 'normal')
	    {
	    ht_startfade(this,this.transition,'out',ht_dosourcechange);
	    }
	else
	    ht_dosourcechange(this);
	}
    return newval;
    }

function ht_dosourcechange(l)
    {
    var tmpl = l.curLayer;
    htr_setvisibility(tmpl, 'hidden');
    l.curLayer = l.altLayer;
    l.altLayer = tmpl;
    htr_setbgcolor(l.curLayer, null);
    pg_serialized_load(l.curLayer, l.newsrc, ht_reloaded);
    //l.curLayer.onload = ht_reloaded;
    //l.curLayer.load(l.newsrc,l.clip.width);
    }

function ht_fadestep()
    {
    htr_setbgimage(ht_fadeobj.faderLayer, '/sys/images/fade_' + ht_fadeobj.transition + '_0' + ht_fadeobj.count + '.gif');
    ht_fadeobj.count++;
    if (ht_fadeobj.count == 5 || ht_fadeobj.count >= 9)
	{
	if (ht_fadeobj.completeFn) return ht_fadeobj.completeFn(ht_fadeobj);
	else return;
	}
    setTimeout(ht_fadestep,100);
    }

function ht_startfade(l,ftype,inout,fn)
    {
    ht_fadeobj = l;
    if (getClipHeight(l.faderLayer) < getClipHeight(l.curLayer))
	setClipHeight(l.faderLayer, getClipHeight(l.curLayer));
    if (getClipWidth(l.faderLayer) < getClipWidth(l.curLayer))
	setClipWidth(l.faderLayer, getClipWidth(l.curLayer));
    moveAbove(l.faderLayer, l.curLayer);
    htr_setvisibility(l.faderLayer, 'inherit');
    l.completeFn = fn;
    if (inout == 'in')
	{
	l.count=5;
	setTimeout(ht_fadestep,20);
	}
    else
	{
	l.count=1;
	setTimeout(ht_fadestep,20);
	}
    }

function ht_reloaded(e)
    {
    //this.mainlayer.watch('source',ht_sourcechanged);
    htr_watch(this.mainlayer, 'source', 'ht_sourcechanged');
    //this.clip.height = this.document.height;
    setClipHeight(this, getdocHeight(this));
    moveAbove(this.mainlayer.faderLayer, this);
    htr_setvisibility(this, 'inherit');
    if (htutil_url_cmp(this.mainlayer.source, document.location.href))
	{
	var lnks = pg_links(this);
	for(var i=0;i<lnks.length;i++)
	    {
	    lnks[i].layer = this.mainlayer;
	    lnks[i].kind = 'ht';
	    }
	}
    pg_resize(this.mainlayer.parentLayer);
    if (this.mainlayer.transition && this.mainlayer.transition != 'normal')
	ht_startfade(this.mainlayer,this.mainlayer.transition,'in',null);
    }

function ht_click(e)
    {
    if (e.target.href)
    	e.target.layer.mainlayer.source = e.target.href;
    return false;
    }

function ht_init(l,l2,fl,source,pdoc,w,h,p)
    {
    htr_init_layer(l,l,'ht');
    htr_init_layer(l2,l,'ht');
    htr_init_layer(fl,l,'ht');
    l.pdoc = pdoc;
    l2.pdoc = pdoc;
    l.curLayer = l;
    l.altLayer = l2;
    l.faderLayer = fl;
    l.LSParent = p;
    l.content = '';
    l.content_type = 'text/html';
    if (h != -1)
	{
	setClipHeight(l, h);
	setClipHeight(l2, h);
	}
    else
	{
	setClipHeight(l, getdocHeight(l));
	}
    pg_set_style(l, 'height', getdocHeight(l));
    if (w != -1)
	{
	setClipWidth(l, w);
	setClipWidth(l2, w);
	}
    else
	{
	setClipWidth(l, getdocWidth(l));
	}
    pg_set_style(l, 'width', getdocWidth(l));
    if (source.substr(0,5) == 'http:')
	{
	pg_serialized_load(l, source, ht_reloaded);
	//l.onload = ht_reloaded;
	//l.load(source,w);
	}
    else if (source.substr(0,6) == 'debug:')
	{
	pg_debug_register_log(l);
	}
    l.source = source;
    l.ActionLoadPage = ht_loadpage;
    l.ActionAddText = ht_addtext;
    l.ActionShowText = ht_showtext;
    l.ActionSetValue = ht_setvalue;
    //l.watch('source', ht_sourcechanged);
    l.ht_sourcechanged = ht_sourcechanged;
    htr_watch(l, 'source', 'ht_sourcechanged');
    pg_resize(l.parentLayer);
    return l;
    }

