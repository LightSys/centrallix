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

function ht_addtext(aparam)
    {
    this.mainlayer.content += aparam.Text;
    }
function ht_addtext_bh()
    {
    pg_resize(this.mainlayer.parentLayer);
    }
function ht_showtext(aparam)
    {
    this.document.write("<pre>" + this.mainlayer.content + "</pre>\n");
    this.document.close();
    pg_resize(this.mainlayer.parentLayer);
    }

function ht_sourcechanged(prop,oldval,newval)
    {
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
    tmpl = l.curLayer;
    tmpl.visibility = 'hidden';
    l.curLayer = l.altLayer;
    l.altLayer = tmpl;
    l.curLayer.onload = ht_reloaded;
    l.curLayer.bgColor = null;
    l.curLayer.load(l.newsrc,l.clip.width);
    }

function ht_fadestep()
    {
    ht_fadeobj.faderLayer.background.src = '/sys/images/fade_' + ht_fadeobj.transition + '_0' + ht_fadeobj.count + '.gif';
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
    if (l.faderLayer.clip.height < l.curLayer.clip.height)
	l.faderLayer.clip.height=l.curLayer.clip.height;
    if (l.faderLayer.clip.width < l.curLayer.clip.width)
	l.faderLayer.clip.width=l.curLayer.clip.width;
    l.faderLayer.moveAbove(l.curLayer);
    l.faderLayer.visibility='inherit';
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
    e.target.mainlayer.watch('source',ht_sourcechanged);
    e.target.clip.height = e.target.document.height;
    e.target.mainlayer.faderLayer.moveAbove(e.target);
    e.target.visibility = 'inherit';
    if (htutil_url_cmp(e.target.mainlayer.source, document.location.href))
	{
	for(i=0;i<e.target.document.links.length;i++)
	    {
	    e.target.document.links[i].layer = e.target.mainlayer;
	    e.target.document.links[i].kind = 'ht';
	    }
	}
    pg_resize(e.target.mainlayer.parentLayer);
    if (e.target.mainlayer.transition && e.target.mainlayer.transition != 'normal')
	ht_startfade(e.target.mainlayer,e.target.mainlayer.transition,'in',null);
    }

function ht_click(e)
    {
    e.target.layer.source = e.target.href;
    return false;
    }

function ht_init(l,l2,fl,source,pdoc,w,h,p)
    {
    l.mainlayer = l;
    l2.mainlayer = l;
    fl.mainlayer = l;
    l.kind = 'ht';
    l2.kind = 'ht';
    fl.kind = 'ht';
    l.pdoc = pdoc;
    l2.pdoc = pdoc;
    l.document.layer = l;
    l2.document.layer = l2;
    fl.document.layer = fl;
    l.curLayer = l;
    l.altLayer = l2;
    l.faderLayer = fl;
    l.LSParent = p;
    l.content = '';
    if (h != -1)
	{
	l.clip.height = h;
	l2.clip.height = h;
	}
    if (w != -1)
	{
	l.clip.width = w;
	l2.clip.width = w;
	}
    if (source.substr(0,5) == 'http:')
	{
	l.onload = ht_reloaded;
	l.load(source,w);
	}
    else if (source.substr(0,6) == 'debug:')
	{
	pg_debug_register_log(l);
	}
    l.source = source;
    l.ActionLoadPage = ht_loadpage;
    l.ActionAddText = ht_addtext;
    l.ActionShowText = ht_showtext;
    l.watch('source', ht_sourcechanged);
    }

