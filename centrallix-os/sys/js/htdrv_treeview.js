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
	if(cx__capabilities.Dom0NS)
	    {
	    nl = new Layer(width,pdoc.tv_layer_tgt);
	    }
	else if(cx__capabilities.Dom1HTML)
	    {
	    nl = document.createElement('DIV');
	    nl.style.setProperty('width',width + 'px','');
	    nl.style.setProperty('height','20px','');
	    nl.style.setProperty('clip','rect(0px,' + width + 'px, 20px, 0px)','');
	    nl.style.setProperty('overflow','hidden','');
	    pdoc.tv_layer_tgt.appendChild(nl);
	    }
	else
	    {
	    alert('browser not supported');
	    }
	tv_alloc_cnt++;
	}
    nl.kind = 'tv';
    if(cx__capabilities.Dom0NS)
	{
	nl.document.layer = l;
	}
    else if(cx__capabilities.Dom1HTML)
	{
	nl.layer = l;
	}
    else
	{
	alert('browser not supported');
	}
    nl.mainlayer = l;
    return nl;
    }

function tv_cache_layer(l,pdoc)
    {
    l.next = pdoc.tv_layer_cache;
    pdoc.tv_layer_cache = l;
    pg_set_style_string(l,'visibility','hidden');
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
    var links = pg_links(e.target.layer);
    if (links != null && (e.which == 3 || e.which == 2))
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

function tv_build_layer(l,img_src,link_href,link_text, link_bold)
    {
    if(cx__capabilities.Dom0NS)
	{
	tvtext = "<IMG SRC='" + img_src + "' align='left'>&nbsp;<A HREF='" + link_href + "'>" + 
	    (link_bold?"<b>":"") + link_text + (link_bold?"<b>":"") + "</A>";
	if (l.tvtext != tvtext)
	    {
	    l.tvtext = tvtext;
	    l.document.writeln(l.tvtext);
	    l.document.close();
	    }
	}
    else if(cx__capabilities.Dom1HTML)
	{
	var c;
	/** remove all current children of this node **/
	while(c = l.firstChild)
	    {
	    l.removeChild(c);
	    }
	
	/** the image **/
	var img = document.createElement('img');
	img.setAttribute('src',img_src);
	img.setAttribute('align','left');
	l.appendChild(img);
	
	/** the space **/
	l.appendChild(document.createTextNode(" "));

	/** the link **/
	var a = document.createElement('a');
	a.setAttribute('href',link_href);
	if(link_bold)
	    {
	    var bold = document.createElement('b');
	    bold.appendChild(document.createTextNode(link_text));
	    a.appendChild(bold);
	    }
	else
	    {
	    a.appendChild(document.createTextNode(link_text));
	    }
	l.appendChild(a);
	}
    else
	{
	alert('browser not supported');
	}
    }


function tv_GetLinkCnt(l)
    {
    if(l.isjs)
	{
	if(typeof(l.obj)=="function")
	    {
	    var win=window.open();
	    win.document.write("<PRE>"+l.obj+"</PRE>");
	    win.document.close();
	    return 0;
	    }
	else
	    {
	    if(!l.obj)
		{
		l.expanded=0;
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
		    var link_txt=l.objn+" ("+typeof(o)+"): "+o;
		    tv_build_layer(l,"/sys/images/ico01b.gif","",link_txt);
		    }
		return 0;
		}
	    var linkcnt=0;
	    for(var i in l.obj) linkcnt++;
	    return linkcnt;
	    }
	}
    else
	{
	return pg_links(tv_tgt_layer.ld).length-1;
	}
    }

function tv_MakeRoom(tv_tgt_layer, linkcnt)
    {
    if (window != tv_tgt_layer.pdoc.tv_layer_tgt)
	tv_tgt_layer.pdoc.tv_layer_tgt.clip.height += 20*(linkcnt);

    var tgtTop = pg_get_style(tv_tgt_layer,"top");
    var layers = pg_layers(tv_tgt_layer.pdoc);
    for (j=0;j<layers.length;j++)
	{
	var sl = layers[j];
	if(cx__capabilities.Dom2CSS)
	    {
	    /** much faster code for DOM2CSS1 compliant browsers **/
	    var slTop = pg_get_style(sl,"top");
	    var visibility = pg_get_style(sl,'visibility');
	    if (slTop >= tgtTop + 20 && sl != tv_tgt_layer && (visibility == 'inherit' || visibility == 'visible') )
		{
		pg_set_style(sl,"top",slTop+20*linkcnt);
		}
	    }
	else
	    {
	    if (sl.pageY >= tv_tgt_layer.pageY + 20 && sl != tv_tgt_layer && sl.visibility == 'inherit')
		{
		sl.pageY += 20*(linkcnt);
		}
	    }
	}
    }

function tv_BuildNewLayers(l, linkcnt)
    {
    /** pre-load some variables **/
    var tgtClipWidth = tv_tgt_layer.clip.width;
    var tgtX = tv_tgt_layer.x;
    var tgtY = tv_tgt_layer.y;

    var jsProps = null
    if(l.isjs)
	{
	jsProps = new Array();
	for(var prop in l.obj)
	    {
	    jsProps.push(prop);
	    }
	}
    
    for(var i=1;i<=linkcnt;i++)
	{
	var link_txt;
	var link_href;
	var link_bold;
    
	var link_bold = 0;
	var one_layer = tv_new_layer(tgtClipWidth,tv_tgt_layer.pdoc,l);
	one_layer.parent=l;
	one_layer.collapse=one_layer.parent.collapse;
	one_layer.expand=one_layer.parent.expand;
	if(l.isjs)
	    {
	    var j = jsProps[i-1];
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
		    link_bold = 1;
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

	one_layer.fname = tv_tgt_layer.fname + one_link;
	tv_build_layer(one_layer,"/sys/images/ico" + im + "b.gif",link_href,link_txt, link_bold);
	one_layer.type = im;
	one_layer.moveTo(tgtX + 20, tgtY + 20*i);
	pg_set_style_string(one_layer,'visibility','inherit');
	var images = pg_images(one_layer);
	one_layer.img = images[images.length-1];
	one_layer.img.kind = 'tv';
	var links = pg_links(one_layer);
	if (links.length != 0)
	    {
	    links[0].kind = 'tv';
	    links[0].layer = one_layer;
	    }
	one_layer.expanded = 0;
	one_layer.img.layer = one_layer;
	one_layer.zIndex = tv_tgt_layer.zIndex;
	one_layer.pdoc = tv_tgt_layer.pdoc;
	one_layer.ld = tv_tgt_layer.ld;
	one_layer.root = tv_tgt_layer.root;
	//if (one_layer.clip.width != tgtClipWidth - one_layer.pageX + tv_tgt_layer.pageX)
	//    one_layer.clip.width = tgtClipWidth - one_layer.pageX + tv_tgt_layer.pageX;
	}
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
    var one_layer=null;
    var cnt=0;
    var nullcnt=0;
    var l=tv_tgt_layer;

    var linkcnt = tv_GetLinkCnt(l);

    if (linkcnt < 0) linkcnt = 0;

    tv_MakeRoom(l, linkcnt);

    tv_BuildNewLayers(l, linkcnt);

    if(cx__capabilities.Dom0NS)
	{
	pg_resize(tv_tgt_layer.parentLayer);
	}
    else if(cx__capabilities.Dom1HTML)
	{
	pg_resize(tv_tgt_layer.parentNode);
	}
    else
	{
	alert('browser not supported');
	}

    pg_set(tv_tgt_layer.img,'src',tv_tgt_layer.img.realsrc);
    pg_set(tv_tgt_layer.img,'src',htutil_subst_last(tv_tgt_layer.img.src,'b.gif'));
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
	tv_build_layer(l,"/sys/images/ico02b.gif","",l.fname);
	l.obj=newroot;
	l.isjs=true;
	}
    l.expanded = 0;
    l.type = '02';
    l.img = pg_images(l)[0];
    l.img.layer = l;
    l.img.kind = 'tv';
    l.kind = 'tv';
    l.pdoc = pdoc;
    l.ld = loader;
    if(cx__capabilities.Dom0NS)
	{
	l.document.layer = l;
	}
    else if(cx__capabilities.Dom1HTML)
	{
	l.layer = l;
	}
    else
	{
	alert('browser not supported');
	}
    l.mainlayer = l;
    //l.ld.parent = l;
    l.root = l;
    pdoc.tv_layer_cache = null;
    if(cx__capabilities.Dom0NS)
	{
	pdoc.tv_layer_tgt = l.parentLayer;
	}
    else if(cx__capabilities.Dom1HTML)
	{
	pdoc.tv_layer_tgt = l.parentNode;
	}
    else
	{
	alert('browser not supported');
	}
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
    pg_set(l.img,'src','/sys/images/ico11c.gif');
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
	    pg_set(l.ld,'src',use_fname + '&ls__mode=list');
	else
	    pg_set(l.ld,'src',use_fname + '?ls__mode=list');
	l.ld.onload = tv_loaded;
	}
    }

function tv_collapse()
    {
    var l = this;
    if (l==null) return false;
    if (l.img.realsrc != null) return false;
    l.img.realsrc=l.img.src;
    pg_set(l.img,'src','/sys/images/ico11c.gif');
    tv_tgt_layer = l;
    
    if (l.expanded==0) return false;
    
    l.expanded = 0;
    cnt = 0;
    var layers = pg_layers(l.pdoc);
    for(i=layers.length-1;i>=0;i--)
	{
	sl = layers[i];
	if (sl.fname!=null && sl!=l && l.fname==sl.fname.substring(0,l.fname.length))
	    {
	    tv_cache_layer(sl,l.pdoc);
	    delete layers[i];
	    sl.fname = null;
	    if(cx__capabilities.Dom0NS)
		{
		sl.document.onmouseup = 0;
		}
	    else if(cx__capabilities.Dom1HTML)
		{
		sl.onmouseup = 0;
		}
	    else
		{
		alert('browser not supported');
		}
	    cnt++;
	    }
	layers = pg_layers(l.pdoc);
	}
    for (j=0;j<layers.length;j++)
	{
	sl = layers[j];
	var visibility = pg_get_style(sl,'visibility');
	if (sl.pageY > l.pageY && (visibility == 'inherit' || visibility == 'visible') )
	    {
	    sl.pageY -= 20*cnt;
	    }
	}
    if(cx__capabilities.Dom0NS)
	{
	pg_resize(tv_tgt_layer.parentLayer);
	}
    else if(cx__capabilities.Dom1HTML)
	{
	pg_resize(tv_tgt_layer.parentNode);
	}
    else
	{
	alert('browser not supported');
	}

    pg_set(l.img,'src',l.img.realsrc);
    pg_set(l.img,'src',htutil_subst_last(l.img.src,'b.gif'));
    l.img.realsrc = null;
    }
