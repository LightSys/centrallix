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

/*  i think this isn't used - lme
function tv_show_obj(obj)
    {
    var result='';
    for (var i in obj) result += 'obj.' + i + ' = ' + obj[i] + '\\n';
    return result;
    }
*/

function tv_new_layer(width,pdoc,l)
    {
    var nl;
    if (pdoc.tv_layer_cache != null)
	{
	nl = pdoc.tv_layer_cache;
	pdoc.tv_layer_cache = nl.next;
	nl.next = null;
	tv_cache_cnt--;
	}
    else
	{
	nl = new Layer(width,pdoc.tv_layer_tgt);
	tv_alloc_cnt++;
	}
    nl.kind = 'tv';
    nl.document.layer = l;
    nl.mainlayer = l;
    return nl;
    }

function tv_cache_layer(l,pdoc)
    {
    l.next = pdoc.tv_layer_cache;
    pdoc.tv_layer_cache = l;
    l.visibility = 'hidden';
    tv_cache_cnt++;
    }

function tv_click(e)
    {
    if (e.which == 3 || e.which == 2)
	{
	tv_rclick(e);
	return false;
	}
    var l=e.target.layer;
    if(l.isjs)
	{
	if (l.parent)
	    {
	    switch(typeof(l.parent.obj[l.objn]))
		{
		case "function":
		    if(confirm("Run Function?"))
			{
			r=prompt("Parameters (fill in array)?",new Array("p1"));
			if(r==undefined) confirm('Return value:'+l.parent.obj[l.objn].apply(l.parent.obj));
			else confirm('Return value:'+l.parent.obj[l.objn].apply(l.parent.obj,eval(r)));
			}
		    break;
		case "object":
		    if(confirm("Change root to here?"))
			{
			l.root.collapse();
			if(l.parent.obj[l.objn].name) tv_init(l.root,"javascript:"+l.parent.obj[l.objn].name,l.root.ld,l.root.pdoc,l.root.clip.width,l.root.LSParent,l.parent.obj[l.objn]);
			else tv_init(l.root,"javascript:"+l.objn,l.root.ld,l.root.pdoc,l.root.clip.width,l.root.LSParent,l.parent.obj[l.objn]);
			}
		    break;
		}
	    }
	return false;
	}
    else
	{
	if (e.target.href != null)
	    {
	    eparam = new Object();
	    eparam.Pathname = e.target.href;
	    eparam.Caller = e.target.layer.root;
	    if (e.target.layer.root.EventClickItem != null)
		cn_activate(e.target.layer.root,'ClickItem', eparam);
	    delete eparam;
	    }
	return false;
	}
    }

function tv_rclick(e)
    {
    if (e.target.layer.document.links != null && (e.which == 3 || e.which == 2))
	{
	if(e.target.layer.isjs)
	    {
	    var l=e.target.layer;
	    if (l.parent)
		{
		switch(typeof(l.parent.obj[l.objn]))
		    {
		    case "object":
			r=prompt('code to run in context of object:','');
			if(r!=undefined)
			    {
			    (new Function(r)).apply(l.parent.obj[l.objn]);
			    }
			break;
		    }
		}
	    return false;
	    }
	else
	    {
	    hr = e.target.layer.document.links[0].href;
	    eparam = new Object();
	    eparam.Pathname = hr;
	    eparam.Caller = e.target.layer.root;
	    eparam.X = e.pageX;
	    eparam.Y = e.pageY;
	    if (e.target.layer.root.EventRightClickItem != null)
		{
		cn_activate(e.target.layer.root, 'RightClickItem', eparam);
		delete eparam;
		return false;
		}
	    delete eparam;
	    }
	}
    return true;
    }


function tv_doalert()
    {
    alert(this);
    }

/***
*** 3/12/2002 -- Jonathan Rupp
***   I added the ability for this widget to view the javascript DOM
***   Just start it with a source of javascript:object and it will show
***     the DOM instead of a server filesystem/database tree
***   note: functions will pop up in a new window, objects
***     expand and show off their properties
***/

function tv_loaded(e)
    {
    one_layer=null;
    cnt=0;
    nullcnt=0;
    l=tv_tgt_layer;
    if(l.isjs)
	{
	if(typeof(l.obj)=="function")
	    {
	    var win=window.open();
	    win.document.write("<PRE>"+l.obj+"</PRE>");
	    win.document.close();
	    linkcnt=last=0;
	    }
	else
	    {
	    last=0;
	    for(var i in l.obj) last++;
	    linkcnt=last;
	    if(!l.obj)
		{
		l.expanded=0;
		linkcnt=last=0;
		var ret=prompt(l.objn,l.parent.obj[l.objn]);
		if(ret!=undefined)
		    {
		    switch(typeof(l.parent.obj[l.objn]))
			{
			case "boolean":
			    if(ret=="true" || ret==1 || ret==-1)
				{
				l.parent.obj[l.objn]=true;
				}
			    else
				{
				l.parent.obj[l.objn]=false;
				}
			    break;
			default:
			    l.parent.obj[l.objn]=ret;
			}
			o=l.parent.obj[l.objn];
		    link_txt=l.objn+" ("+typeof(o)+"): "+o;
		    tvtext = "<IMG SRC=/sys/images/ico01b.gif align=left>&nbsp;<A HREF=''>" + link_txt + "</A>";
		    if (l.tvtext != tvtext)
			{
			l.tvtext = tvtext;
			l.document.writeln(l.tvtext);
			l.document.close();
			}
		    }
		}
	    }
	}
    else
	{
	last = tv_tgt_layer.ld.document.links.length - 1;
	linkcnt = tv_tgt_layer.ld.document.links.length-1;
	}
    if (linkcnt < 0) linkcnt = 0;
    if (window != tv_tgt_layer.pdoc.tv_layer_tgt)
	    tv_tgt_layer.pdoc.tv_layer_tgt.clip.height += 20*(linkcnt);
    for (j=0;j<tv_tgt_layer.pdoc.layers.length;j++)
	{
	sl = tv_tgt_layer.pdoc.layers[j];
	if (sl.pageY >= tv_tgt_layer.pageY + 20 && sl != tv_tgt_layer && sl.visibility == 'inherit')
	    {
	    sl.pageY += 20*(linkcnt);
	    }
	}
    for (i=1;i<=linkcnt;i++)
	{
	one_layer = tv_new_layer(tv_tgt_layer.clip.width,tv_tgt_layer.pdoc,l);
	one_layer.parent=l;
	one_layer.collapse=one_layer.parent.collapse;
	one_layer.expand=one_layer.parent.expand;
	if(l.isjs)
	    {
	    k=0;
	    for(m in l.obj)
		{
		if(k!=i) { k++; j=m; }
		}
	    if(j=="applets" || j=="embeds")
		{
		o=null;
		t="object";
		}
	    else
		{
		var o=l.obj[j];
		t=typeof(o);
		}
	    link_href="";
	    one_link=j;
	    if(o && (t=="object" || t=="function"))
		{
		one_layer.obj=o;
		link_txt=j+" ("+t+"): ";
		if(t=="function")
		    {
		    im='01';
		    }
		else
		    {
		    im = '02';
		    link_txt="<b>"+link_txt+"</b>";
		    if(o.name) link_txt+=" "+o.name;
		    }
		}
	    else
		{
		link_txt=j+" ("+t+"): "+o;
		one_layer.obj=null;
		im = '01';
		}
	    one_layer.isjs=true;
	    one_layer.objn=j;
	    }
	else
	    {
	    link_txt = tv_tgt_layer.ld.document.links[i].text;
	    link_href = tv_tgt_layer.ld.document.links[i].href;
	    one_link = link_href.substring(link_href.lastIndexOf('/')+1,link_href.length);
	    if (one_link[0] == ' ') one_link = one_link.substring(1,one_link.length);
	    im = '01';
	    if (link_txt == '' || link_txt == null) link_txt = one_link;
	    else link_txt = one_link + '&nbsp;-&nbsp;' + link_txt;
	    if (one_link.lastIndexOf('/') > 0) im = '02';
	    else one_link = one_link + '/';
	    }
	imgs = '';

	one_layer.fname = tv_tgt_layer.fname + one_link;
	//alert(one_layer.fname);
	tvtext = imgs + "<IMG SRC=/sys/images/ico" + im + "b.gif align=left>&nbsp;<A HREF=" + link_href + ">" + link_txt + "</A>";
	if (one_layer.tvtext != tvtext)
	    {
	    one_layer.tvtext = tvtext;
	    one_layer.document.writeln(one_layer.tvtext);
	    one_layer.document.close();
	    }
	one_layer.type = im;
	one_layer.moveTo(tv_tgt_layer.x + 20, tv_tgt_layer.y + 20*i);
	one_layer.visibility = 'inherit';
	one_layer.img = one_layer.document.images[one_layer.document.images.length-1];
	one_layer.img.kind = 'tv';
	if (one_layer.document.links.length != 0)
	    {
	    one_layer.document.links[0].kind = 'tv';
	    one_layer.document.links[0].layer = one_layer;
	    }
	one_layer.expanded = 0;
	one_layer.img.layer = one_layer;
	one_layer.zIndex = tv_tgt_layer.zIndex;
	one_layer.pdoc = tv_tgt_layer.pdoc;
	one_layer.ld = tv_tgt_layer.ld;
	one_layer.root = tv_tgt_layer.root;
	if (one_layer.clip.width != tv_tgt_layer.clip.width - one_layer.pageX + tv_tgt_layer.pageX)
	    one_layer.clip.width = tv_tgt_layer.clip.width - one_layer.pageX + tv_tgt_layer.pageX;
	cnt++;
	}
    pg_resize(tv_tgt_layer.parentLayer);
    tv_tgt_layer.img.src = tv_tgt_layer.img.realsrc;
    tv_tgt_layer.img.src = htutil_subst_last(tv_tgt_layer.img.src,'b.gif');
    tv_tgt_layer.img.realsrc = null;
    tv_tgt_layer = null;
    return false;
    }

function tv_init(l,fname,loader,pdoc,w,p,newroot)
    {
    l.LSParent = p;
    l.fname = fname;
    if(newroot==undefined)
	{ /* not re-init */
	if(t=(/javascript:(.*)/).exec(fname))
	   {
	   l.isjs=true;
	   if(t[1])
	       {
	       l.obj=eval(t[1]);
	       }
	   else
	       {
	       l.obj=window;
	       }
	   }
	}
    else
	{ /* re-init */
	l.document.write("<IMG SRC=/sys/images/ico02b.gif align=left>&nbsp;"+l.fname+"</DIV>");
	l.document.close();
	l.obj=newroot;
	l.isjs=true;
	}
    l.expanded = 0;
    l.type = '02';
    l.img = l.document.images[0];
    l.img.layer = l;
    l.img.kind = 'tv';
    l.kind = 'tv';
    l.pdoc = pdoc;
    l.ld = loader;
    l.document.layer = l;
    l.mainlayer = l;
    //l.ld.parent = l;
    l.root = l;
    pdoc.tv_layer_cache = null;
    pdoc.tv_layer_tgt = l.parentLayer;
    l.clip.width = w;
    l.childimgs = '';
    l.collapse=tv_collapse;
    l.expand=tv_expand;
    }

function tv_expand()
    {
    var l = this;
    if (l==null) return false;
    if (l.img.realsrc != null) return false;
    l.img.realsrc=l.img.src;
    l.img.src='/sys/images/ico11c.gif';
    tv_tgt_layer = l;
    
    if (l.expanded==1) return false;
    
    l.expanded = 1;
    if(l.isjs)
	{
	l.ld.onload=tv_loaded
	l.ld.onload();
	}
    else
	{
	if (l.fname.substr(l.fname.length-1,1) == '/' && l.fname.length > 1)
	    use_fname = l.fname.substr(0,l.fname.length-1);
	else
	    use_fname = l.fname;
	if (use_fname.lastIndexOf('?') > use_fname.lastIndexOf('/', use_fname.length-2))
	    l.ld.src = use_fname + '&ls__mode=list';
	else
	    l.ld.src = use_fname + '?ls__mode=list';
	l.ld.onload = tv_loaded;
	}
    }

function tv_collapse()
    {
    var l = this;
    if (l==null) return false;
    if (l.img.realsrc != null) return false;
    l.img.realsrc=l.img.src;
    l.img.src='/sys/images/ico11c.gif';
    tv_tgt_layer = l;
    
    if (l.expanded==0) return false;
    
    l.expanded = 0;
    cnt = 0;
    for(i=l.pdoc.layers.length-1;i>=0;i--)
	{
	sl = l.pdoc.layers[i];
	if (sl.fname!=null && sl!=l && l.fname==sl.fname.substring(0,l.fname.length))
	    {
	    tv_cache_layer(sl,l.pdoc);
	    delete l.pdoc.layers[i];
	    sl.fname = null;
	    sl.document.onmouseup = 0;
	    cnt++;
	    }
	}
    for (j=0;j<l.pdoc.layers.length;j++)
	{
	sl = l.pdoc.layers[j];
	if (sl.pageY > l.pageY && sl.visibility == 'inherit')
	    {
	    sl.pageY -= 20*cnt;
	    }
	}
    pg_resize(l.parentLayer);
    l.img.src=l.img.realsrc;
    l.img.src = htutil_subst_last(l.img.src,'b.gif');
    l.img.realsrc = null;
    }
