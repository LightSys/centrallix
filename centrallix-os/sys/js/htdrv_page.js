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

function pg_ping_init(l,i)
    {
    l.tid=setInterval(pg_ping_send,i,l);
    }

function pg_ping_recieve()
    {
    if(this.document.links[0].target!=='OK')
	{
	clearInterval(this.tid);
	confirm('you have been disconnected from the server');
	}
    }

function pg_ping_send(p)
    {
    //confirm('sending');
    p.onload=pg_ping_recieve;
    p.src='/INTERNAL/ping';
    }

/** Function to set modal mode to a layer. **/
function pg_setmodal(l)
    {
    pg_modallayer = l;
    }

/** Function to find out whether image or layer is in a layer **/
function pg_isinlayer(outer,inner)
    {
    if (inner == outer) return true;
    if(!outer) return true;
    if(!inner) return false;
    var i = 0;
    for(i=0;i<outer.layers.length;i++)
	{
	if (outer.layers[i] == inner) return true;
	if (pg_isinlayer(outer.layers[i], inner)) return true;
	}
    for(i=0;i<outer.document.images.length;i++)
	{
	if (outer.document.images[i] == inner) return true;
	}
    return false;
    }

/** Function to make four layers into a box **/
function pg_mkbox(pl, x,y,w,h, s, tl,bl,rl,ll, c1,c2, z)
    {
    tl.visibility = 'hidden';
    bl.visibility = 'hidden';
    rl.visibility = 'hidden';
    ll.visibility = 'hidden';
    tl.bgColor = c1;
    ll.bgColor = c1;
    bl.bgColor = c2;
    rl.bgColor = c2;
    tl.resizeTo(w,1);
    tl.moveAbove(pl);
    tl.moveToAbsolute(x,y);
    tl.zIndex = z;
    bl.resizeTo(w+s-1,1);
    bl.moveAbove(pl);
    bl.moveToAbsolute(x,y+h-s+1);
    bl.zIndex = z;
    ll.resizeTo(1,h);
    ll.moveAbove(pl);
    ll.moveToAbsolute(x,y);
    ll.zIndex = z;
    rl.resizeTo(1,h+1);
    rl.moveAbove(pl);
    rl.moveToAbsolute(x+w-s+1,y);
    rl.zIndex = z;
    tl.visibility = 'inherit';
    bl.visibility = 'inherit';
    rl.visibility = 'inherit';
    ll.visibility = 'inherit';
    return;
    }

/** To hide a box **/
function pg_hidebox(tl,bl,rl,ll)
    {
    tl.visibility = 'hidden';
    bl.visibility = 'hidden';
    rl.visibility = 'hidden';
    ll.visibility = 'hidden';
    tl.moveAbove(document.layers.pgtvl);
    bl.moveAbove(document.layers.pgtvl);
    rl.moveAbove(document.layers.pgtvl);
    ll.moveAbove(document.layers.pgtvl);
    return;
    }

/** Function to make a new clickable "area" **INTERNAL** **/
function pg_area(pl,x,y,w,h,cls,nm,f)
    {
    this.layer = pl;
    this.x = x;
    this.y = y;
    this.width = w;
    this.height = h;
    this.name = nm;
    this.cls = cls;
    this.flags = f;
    return this;
    }

/** Function to resize a given page area **/
function pg_resize_area(a,w,h)
    {
    var x=a.layer.pageX+a.x;
    var y=a.layer.pageY+a.y;
    var tl=document.layers.pgtop;
    var bl=document.layers.pgbtm;
    var ll=document.layers.pglft;
    var rl=document.layers.pgrgt;
    a.width = w;
    a.height = h;
    tl.resizeTo(w,1);
    bl.resizeTo(w+1,1);
    rl.resizeTo(1,h+1);
    ll.resizeTo(1,h);
    tl.moveToAbsolute(x,y);
    bl.moveToAbsolute(x,y+h);
    ll.moveToAbsolute(x,y);
    rl.moveToAbsolute(x+w,y);
    }

/** Function to add a new area to the arealist **/
function pg_addarea(pl,x,y,w,h,cls,nm,f)
    {
    a = new pg_area(pl,x,y,w,h,cls,nm,f);
    pg_arealist.splice(0,0,a);
    return a;
    }

/** Function to remove an existing area... **/
function pg_removearea(a)
    {
    for(i=0;i<pg_arealist.length;i++)
	{
	if (pg_arealist[i] == a)
	    {
	    pg_arealist.splice(i,1);
	    return 1;
	    }
	}
    return 0;
    }

/** Add a universal resize manager function. **/
function pg_resize(l)
    {
    maxheight=0;
    maxwidth=0;
    for(i=0;i<l.document.layers.length;i++)
	{
	cl = l.document.layers[i];
	if ((cl.visibility == 'show' || cl.visibility == 'inherit') && cl.y + cl.clip.height > maxheight)
	    maxheight = cl.y + cl.clip.height;
	if ((cl.visibility == 'show' || cl.visibility == 'inherit') && cl.x + cl.clip.width > maxwidth)
	    maxwidth = cl.x + cl.clip.width;
	}
    if (l.maxheight && maxheight > l.maxheight) maxheight = l.maxheight;
    if (l.minheight && maxheight < l.minheight) maxheight = l.minheight;
    if (l!=window) l.clip.height = maxheight;
    else l.document.height = maxheight;
    if (l.maxwidth && maxwidth > l.maxwidth) maxwidth = l.maxwidth;
    if (l.minwidth && maxwidth < l.minwidth) maxwidth = l.minwidth;
    if (l!=window) l.clip.width = maxwidth;
    else l.document.width = maxwidth;
    }

/** Add a universal "is visible" function that handles inherited visibility. **/
function pg_isvisible(l)
    {
    if (l.visibility == 'show') return 1;
    else if (l.visibility == 'hidden') return 0;
    else if (l == window || l.parentLayer == null) return 1;
    else return pg_isvisible(l.parentLayer);
    }

/** Cursor flash **/
function pg_togglecursor()
    {
    if (pg_curkbdlayer != null && pg_curkbdlayer.cursorlayer != null)
	{
	if (pg_curkbdlayer.cursorlayer.visibility != 'inherit')
	    pg_curkbdlayer.cursorlayer.visibility = 'inherit';
	else
	    pg_curkbdlayer.cursorlayer.visibility = 'hidden';
	}
    setTimeout(pg_togglecursor,333);
    }

/** Keyboard input handling **/
function pg_addkey(s,e,mod,modmask,mlayer,klayer,tgt,action,aparam)
    {
    kd = new Object();
    kd.startcode = s;
    kd.endcode = e;
    kd.mod = mod;
    kd.modmask = modmask;
    kd.mouselayer = mlayer;
    kd.kbdlayer = klayer;
    kd.target_obj = tgt;
    kd.fnname = 'Action' + action;
    kd.aparam = aparam;
    pg_keylist.splice(0,0,kd);
    pg_keylist.sort(pg_cmpkey);
    return kd;
    }
    
function pg_cmpkey(k1,k2)
    {
    return (k1.endcode-k1.startcode) - (k2.endcode-k2.startcode);
    }
    
function pg_removekey(kd)
    {
    for(i=0;i<pg_keylist.length;i++)
	{
	if (pg_keylist[i] == kd)
	    {
	    pg_keylist.splice(i,1);
	    return 1;
	    }
	}
    return 0;
    }
    
function pg_keytimeout()
    {
    if (pg_lastkey != -1)
	{
	e = new Object();
	e.which = pg_lastkey;
	e.modifiers = pg_lastmodifiers;
	pg_keyhandler(pg_lastkey, pg_lastmodifiers, e);
	delete e;
	pg_keytimeoutid = setTimeout(pg_keytimeout, 50);
	}
    }

function pg_keyhandler(k,m,e)
    {
    pg_lastmodifiers = m;
    if (pg_curkbdlayer != null && pg_curkbdlayer.keyhandler != null && pg_curkbdlayer.keyhandler(pg_curkbdlayer,e,k) == true) return false;
    for(i=0;i<pg_keylist.length;i++)
	{
	if (k >= pg_keylist[i].startcode && k <= pg_keylist[i].endcode && (pg_keylist[i].kbdlayer == null || pg_keylist[i].kbdlayer == pg_curkbdlayer) && (pg_keylist[i].mouselayer == null || pg_keylist[i].mouselayer == pg_curlayer) && (m & pg_keylist[i].modmask) == pg_keylist[i].mod)
	    {
	    pg_keylist[i].aparam.KeyCode = k;
	    pg_keylist[i].target_obj[pg_keylist[i].fnname](pg_keylist[i].aparam);
	    return false;
	    }
	}
    return false;
    }
 
function pg_status_init()
    {
    pg_status = null;
    if(cn_browser.netscape47)
	{
	pg_status = document.layers.pgstat;
	}
    else if(cn_browser.mozilla)
	{
	pg_status = document.getElementById("pgstat");
	}
    
    if (!pg_status)
	return false;
    pg_status.zIndex = 1000000;
    pg_status.visibility = 'visible';
    }
 
function pg_status_close()
    {
    if (!pg_status)
	return false
    pg_status.visibility = 'hide';
    }

function pg_init(l,a,gs,ct)
    {
    l.ActionLoadPage = pg_load_page;
    pg_attract = a;
    return l;
    }

function pg_load_page(aparam)
    {
    window.location.href = aparam.Source;
    }

function pg_mvpginpt(ly)
    {
    var a=(document.height-window.innerHeight-2)>=0?16:1;
    var b=(document.width-window.innerWidth-2)>=0?22:5;
    ly.moveTo(window.innerWidth-a+window.pageXOffset, window.innerHeight-b+window.pageYOffset);
    if (a>1||b>5) setTimeout(pg_mvpginpt, 500, ly);
    }
