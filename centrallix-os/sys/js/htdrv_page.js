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


/** returns an attribute of the element in pixels **/
function pg_get_style(element,attr)
    {
    if(cx__capabilities.Dom1HTML && cx__capabilities.Dom2CSS)
	{
	if(attr.substring(0,5) == 'clip.')
	    {
	    return eval('element.' + attr);
	    }
	var comp_style = window.getComputedStyle(element,null);
	var cssValue = comp_style.getPropertyCSSValue(attr);
	if(cssValue.cssValueType != CSSValue.CSS_PRIMITIVE_VALUE)
	    {
	    alert(attr + ': ' + cssValue.cssValueType);
	    return null;
	    }
	if(cssValue.primitiveType >= CSSPrimitiveValue.CSS_STRING)
	    return cssValue.getStringValue();
	return cssValue.getFloatValue(CSSPrimitiveValue.CSS_PX);
	}
    else if(cx__capabilities.Dom0NS)
	{
	return eval('element.' + attr);
	}
    else
	{
	alert('cannot calculate CSS values for this browser');
	}
    }

function pg_set_style(element,attr, value)
    {
    if(cx__capabilities.Dom1HTML && cx__capabilities.Dom2CSS)
	{
	if(attr.substr(0,5) == 'clip.')
	    {
	    eval('element.' + attr + ' = value;');
	    return;
	    }
	element.style.setProperty(attr,value + "px","");
	return;
	}
    else if(cx__capabilities.Dom0NS)
	{
	eval('element.' + attr + ' = value;');
	return;
	}
    else
	{
	alert('cannot set CSS values for this browser');
	return;
	}
    }

function pg_ping_init(l,i)
    {
    l.tid=setInterval(pg_ping_send,i,l);
    }

function pg_ping_recieve()
    {
    var link;
    //confirm("recieving");
    if(cx__capabilities.Dom1HTML)
	{
	link = this.contentDocument.getElementsByTagName("a")[0];
	}
    else if(cx__capabilities.Dom0NS)
	{
	link = this.document.links[0];
	}
    else
	{
	return false;
	}
    if(link.target!=='OK')
	{
	clearInterval(this.tid);
	confirm('you have been disconnected from the server');
	}
    }

function pg_ping_send(p)
    {
    //confirm('sending');
    p.onload=pg_ping_recieve;
    if(cx__capabilities.Dom1HTML)
	{
	p.setAttribute('src','/INTERNAL/ping');
	}
    else if(cx__capabilities.Dom0NS)
	{
	p.src='/INTERNAL/ping';
	}
    }

/** Function to get the images attacked to a layer **/
function pg_images(o)
    {
    if(cx__capabilities.Dom1HTML)
	{
	return o.getElementsByTagName("img");
	}
    else if(cx__capabilities.Dom0NS || cx__capabilities.Dom0IE)
	{
	return o.document.images;
	}
    else
	{
	return null;
	}
    }

/** function to set an attribute **/
function pg_set(o,a,v)
    {
    if(cx__capabilities.Dom1HTML)
	{
	return o.setAttribute(a,v);
	}
    else
	{
	o[a]=v;
	}
    }

function pg_get_computed_clip(o)
    {
    if(cx__capabilities.Dom2CSS)
	return getComputedStyle(o,null).getPropertyCSSValue('clip').getRectValue();
    else if(cx__capabilities.Dom0NS)
	return o.clip;
    else
	return null;
    }

function pg_get_clip(o)
    {
    if(cx__capabilities.Dom2CSS2)
	{
	var clip = o.style.getPropertyCSSValue('clip');
	if(clip)
	    return clip.getRectValue();
	else
	    {
	    var computed = getComputedStyle(o,null).getPropertyCSSValue('clip').cssText;
	    o.style.setProperty('clip',computed,"");
	    clip = o.style.getPropertyValue('clip');
	    alert(clip);
	    clip = o.style.getPropertyCSSValue('clip');
	    alert(clip);
	    return clip.getRectValue;
	    }
	}
    else if(cx__capabilities.Dom0NS)
	return o.clip;
    else
	return null;
    }


/** Function to emulate getElementByID **/
function pg_getelementbyid(nm)
    {
    if (this.layers)
	return this.layers[nm];
    else if (this.all)
	return this.all[nm];
    else
	return null;
    }

/** Function to walk the DOM and set up getElementByID emulation **/
function pg_set_emulation(d)
    {
    var a = null;
    var i = 0;
    if (d.document) d = d.document;
    d.getElementByID = pg_getelementbyid;
    if (this.layers)
	a = this.layers;
    else if (this.all)
	a = this.all;
    else
	a = null;
    if (a)
	{
	for(i=0;i<a.length;i++)
	    {
	    pg_set_emulation(a[i]);
	    }
	}
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
    if(cx__capabilities.Dom1HTML)
	{
	pg_status = document.getElementById("pgstat");
	}
    else if(cx__capabilities.Dom0NS)
	{
	pg_status = document.layers.pgstat;
	}
    else if(cx__capabilities.Dom0IE)
	{
	pg_status = document.all.pgstat;
	}
    else
	{
	return false;
	}
    
    if (!pg_status)
	return false;
    pg_status.zIndex = 1000000;
    pg_status.visibility = 'visible';
    pg_set_emulation(document);
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


function pg_addsched(e)
    {
    pg_schedtimeoutlist.push(e);
    if(!pg_schedtimeout) pg_schedtimeout = setTimeout(pg_dosched, 0);
    }

function pg_expchange(p,o,n)
    {
    if (o==n) return n;
    for(var i=0;i<pg_explist.length;i++)
	{
	var exp = pg_explist[i];
	for(var j=0;j<exp.ParamList.length;j++)
	    {
	    var item = exp.ParamList[j];
	    if (this == item[2] && p == item[1])
		{
		//alert("eval " + exp.Objname + "." + exp.Propname + " = " + exp.Expression);
		pg_addsched(exp.Objname + "." + exp.Propname + " = " + exp.Expression);
		}
	    }
	}
    return n;
    }


function pg_dosched()
    {
    var p = null;
    if (pg_schedtimeoutlist.length > 0)
    	{
	p = pg_schedtimeoutlist.pop();
	eval(p);
	}

    if (pg_schedtimeoutlist.length > 0)
	{
	pg_schedtimeout = setTimeout(pg_dosched,0);
	}
    else
	{
	pg_schedtimeout = null;
	}
    }

function pg_expression(o,p,e,l)
    {
    var expobj = new Object();
    expobj.Objname = o;
    expobj.Propname = p;
    expobj.Expression = e;
    expobj.ParamList = l;
    eval(expobj.Objname + "." + expobj.Propname + " = " + expobj.Expression);
    pg_explist.push(expobj);
    for(var i=0; i<l.length; i++)
	{
	var item = l[i];
	item[2] = eval(item[0]); // get obj reference
	item[2].watch(item[1], pg_expchange);
	}
    }


function pg_removemousefocus()
    {
    if (pg_curarea.flags & 1) pg_hidebox(document.layers.pgtop,document.layers.pgbtm,document.layers.pgrgt,document.layers.pglft);
    pg_curarea = null;
    return true;
    }

function pg_findfocusarea(l, xo, yo)
    {
    for(var i=0;i<pg_arealist.length;i++) 
	{
	if (l == pg_arealist[i].layer && ((xo == null && yo == null) || (xo >= pg_arealist[i].x &&
	    yo >= pg_arealist[i].y && xo < pg_arealist[i].x+pg_arealist[i].width &&
	    yo < pg_arealist[i].y+pg_arealist[i].height && l != pg_arealist[i])))
	    {
	    if (l == pg_arealist[i]) break;
	    return pg_arealist[i];
	    }
	}
    return null;
    }

function pg_setmousefocus(l, xo, yo)
    {
    var a = pg_findfocusarea(l, xo, yo);
    if (a && a != pg_curarea)
	{
	pg_curarea = a;
	if (pg_curarea.flags & 1)
	    {
	    // wants mouse focus
	    var x = pg_curarea.layer.pageX+pg_curarea.x;
	    var y = pg_curarea.layer.pageY+pg_curarea.y;
	    var w = pg_curarea.width;
	    var h = pg_curarea.height;
	    pg_mkbox(l, x,y,w,h, 1, document.layers.pgtop,document.layers.pgbtm,document.layers.pgrgt,document.layers.pglft, page.mscolor1, page.mscolor2, document.layers.pgktop.zIndex-1);
	    }
	}
    }

function pg_removekbdfocus()
    {
    if (pg_curkbdlayer)
	{
	if (pg_curkbdlayer.losefocushandler && !pg_curkbdlayer.losefocushandler()) return false;
	pg_mkbox(null,0,0,0,0, 1, document.layers.pgktop,document.layers.pgkbtm,document.layers.pgkrgt,document.layers.pgklft, page.kbcolor1, page.kbcolor2, document.layers.pgtop.zIndex+100);
	}
    return true;
    }

function pg_setkbdfocus(l, a, xo, yo)
    {
    if (!a)
	{
	a = pg_findfocusarea(l, xo, yo);
	}
    var x = a.layer.pageX+a.x;
    var y = a.layer.pageY+a.y;
    var w = a.width;
    var h = a.height;
    var prevLayer = pg_curkbdlayer;
    var prevArea = pg_curkbdarea;
    pg_curkbdarea = a;
    pg_curkbdlayer = l;

    if (pg_curkbdlayer && pg_curkbdlayer.getfocushandler)
	{
	var v=pg_curkbdlayer.getfocushandler(xo,yo,a.layer,a.cls,a.name,a);
	if (v & 1)
	    {
	    // mk box for kbd focus
	    if (prevArea != a)
		{
		pg_mkbox(l ,x,y,w,h, 1, document.layers.pgktop,document.layers.pgkbtm,document.layers.pgkrgt,document.layers.pgklft, page.kbcolor1, page.kbcolor2, document.layers.pgtop.zIndex+100);
		}
	    }
	if (v & 2)
	    {
	    // mk box for data focus
	    if (l.pg_dttop != null)
		{
		// data focus moving within a control - remove old one
		pg_hidebox(l.pg_dttop,l.pg_dtbtm,l.pg_dtrgt,l.pg_dtlft);
		}
	    else
		{
		// mk new data focus box for this control.
		l.pg_dttop = new Layer(1152);
		l.pg_dtbtm = new Layer(1152);
		l.pg_dtrgt = new Layer(2);
		l.pg_dtlft = new Layer(2);
		}
	    pg_mkbox(l,x-1,y-1,w+2,h+2, 1, l.pg_dttop,l.pg_dtbtm,l.pg_dtrgt,l.pg_dtlft, page.dtcolor1, page.dtcolor2, document.layers.pgtop.zIndex+100);
	    }
	}
    return true;
    }

