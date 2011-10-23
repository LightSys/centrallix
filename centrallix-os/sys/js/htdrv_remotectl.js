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


/** Function that gets run when we got a block of cmds from the channel **/
function rc_loaded()
    {
    }
    
/** Function to initialize the remote ctl server **/
function rc_init(l,c,s)
    {
    l.channel = c;
    l.server = s;
    l.address = 'http://' + s + '/?ls__mode=readchannel&ls__channel=' + c;
    l.onload = rc_loaded;
    l.source = l.address;
    return l;
    }

/** This function handles the 'LoadPage' action **/
function ht_loadpage(aparam)
    {
    this.source = aparam.Source;
    }

/** This function gets run when the user assigns to the 'source' property **/
function ht_sourcechanged(prop,oldval,newval)
    {
    if (newval.substr(0,5)=='http:')
        {
        tmpl = this.curLayer;
        tmpl.visibility = 'hidden';
        this.curLayer = this.altLayer;
        this.altLayer = tmpl;
        this.curLayer.onload = ht_reloaded;
        this.curLayer.bgColor = null;
        this.curLayer.load(newval,this.clip.width);
        }
    return newval;
    }

/** This function is called when the layer is reloaded. **/
function ht_reloaded(e)
    {
    e.target.mainLayer.watch('source',ht_sourcechanged);
    e.target.clip.height = e.target.document.height;
    e.target.visibility = 'inherit';
/*"    e.target.document.captureEvents(Event.CLICK);
    e.target.document.onclick = ht_click;*/
    for(i=0;i<e.target.document.links.length;i++)
        {
        e.target.document.links[i].layer = e.target.mainLayer;
        e.target.document.links[i].kind = 'ht';
        }
    pg_resize(e.target.mainLayer.parentLayer);
    }

/** This function is called when the user clicks on a link in the html pane **/
function ht_click(e)
    {
    e.target.layer.source = e.target.href;
    return false;
    }
    
/** Our initialization processor function. **/
function ht_init(l,l2,source,pdoc,w,h,p)
    {
    l.mainLayer = l;
    l2.mainLayer = l;
    l.curLayer = l;
    l.altLayer = l2;
    l.LSParent = p;
    l.kind = 'ht';
    l2.kind = 'ht';
    l.pdoc = pdoc;
    l2.pdoc = pdoc;
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
    l.source = source;
    l.watch('source', ht_sourcechanged);
    l.ActionLoadPage = ht_loadpage;
    l.document.Layer = l;
    l2.document.Layer = l2;
    }

// Load indication
if (window.pg_scripts) pg_scripts['htdrv_remotectl.js'] = true;
