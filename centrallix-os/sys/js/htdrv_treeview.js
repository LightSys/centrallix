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
    if (pdoc.tv_layer_cache.length > 0)
	{
	/*nl = pdoc.tv_layer_cache;
	pdoc.tv_layer_cache = nl.next;
	nl.next = null;*/
	tv_cache_cnt--;
	nl = pdoc.tv_layer_cache.pop();
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
	    nl.style.width = width;
	    //setClip(0, width, 0, 0);
	    pg_set_style(nl, 'position','relative');
	    pdoc.appendChild(nl);
	    }
	else
	    {
	    alert('treeview: browser not supported');
	    }
	tv_alloc_cnt++;
	}
    htr_init_layer(nl, l, 'tv');
    return nl;
    }

function tv_cache_layer(l,pdoc)
    {
    if (cx__capabilities.Dom1HTML)
        {
		//pg_debug('tv_cache_layer: ' + pdoc.id + '\n');
        }
    else if (cx__capabilities.Dom0NS)
        {
		//pg_debug('tv_cache_layer: ' + pdoc.layer.name + '\n');
		}
    /*l.next = pdoc.tv_layer_cache;
    pdoc.tv_layer_cache = l;*/
    pdoc.tv_layer_cache.push(l);
    pg_set_style_string(l,'visibility','hidden');
    tv_cache_cnt++;
    }

function tv_action_setfocus(aparam)
    {
    }

function tv_action_setroot(aparam)
    {
    // Make sure we've collapsed the current tree
    this.root.collapse();

    // Set the root
    if (!aparam.NewRoot) aparam.NewRoot = 'javascript:window';
    if (!aparam.NewRootObj) aparam.NewRootObj = null;
    tv_init({layer:this.root, fname:aparam.NewRoot, loader:this.root.ld, width:getClipWidth(this.root), newroot:aparam.NewRootObj, branches:this.show_branches});
    //tv_init({layer:this.root, fname:aparam.NewRoot, loader:this.root.ld, pdoc:this.root.pdoc, width:getClipWidth(this.root), newroot:aparam.NewRootObj, branches:this.show_branches});
    if (aparam.Expand == 'yes') this.root.expand();
    }

function tv_click(e)
    {
    if (!cx__capabilities.Dom0IE)
        {
    	if (e.which == 3 || e.which == 2)
	    {
	    tv_rclick(e);
	    return false;
	    }
	}
    else
        {
    	if (e.button == 3 || e.button == 2)
	    {
	    tv_rclick(e);
	    return false;
	    }
        }

    var l=e.target.layer;

    if(l.isjs)
	{
	if (l.parent)
	    {
	    switch(typeof(l.parent.objptr[l.objn]))
		{
		case "function":
		    if(confirm("Run Function?"))
			{
			r=prompt("Parameters (fill in array)?",new Array("p1"));
			if(r==undefined) confirm('Return value:'+l.parent.objptr[l.objn].apply(l.parent.objptr));
			else confirm('Return value:'+l.parent.objptr[l.objn].apply(l.parent.objptr,eval(r)));
			}
		    break;
		case "object":
		    if(confirm("Change root to here?"))
			{
			var nr;
			if(l.parent.objptr[l.objn].name)
			    nr = "javascript:"+l.parent.objptr[l.objn].name;
			else
			    nr = "javascript:"+l.objn;
			l.mainlayer.ifcProbe(ifAction).Invoke("SetRoot", {NewRoot:nr, NewRootObj:l.parent.objptr[l.objn]});
			//else tv_init(l.root,"javascript:"+l.objn,l.root.ld,l.root.pdoc,l.root.clip.width,l.root.LSParent,l.parent.objptr[l.objn]);
			}
		    break;
		}
	    }
	return false;
	}
    else
	{
	if (e.target.layer && e.target.layer.link_href)
	    {
	    var path = e.target.layer.fname;
	    var r = e.target.layer.root;
	    if (path.lastIndexOf('/') == path.length-1)
		path = path.substring(0,path.length-1);
	    r.ifcProbe(ifEvent).Activate('ClickItem', {Pathname:path, HRef:e.target.layer.link_href, Caller:r});
	    }
	return false;
	}
    }

function tv_rclick(e)
    {
    var links = pg_links(e.target.layer);
    var which;
    if (!cx__capabilities.Dom0IE)
        {
        which = e.which;
        }
    else
    	{
    	which = e.button;
    	}
    if (links != null && (which == 3 || which == 2))
	{
	if(e.target.layer.isjs)
	    {
	    var l=e.target.layer;
	    if (l.parent)
		{
		switch(typeof(l.parent.objptr[l.objn]))
		    {
		    case "object":
			r=prompt('code to run in context of object:','');
			if(r!=undefined)
			    {
			    (new Function(r)).apply(l.parent.objptr[l.objn]);
			    }
			break;
		    }
		}
	    return false;
	    }
	else
	    {
	    var hr = e.target.layer.document.links[0].href;
	    var r = e.target.layer.root;
	    if (r.ifcProbe(ifEvent).Activate('RightClickItem', {Pathname:hr, Caller:r, X:e.pageX, Y:e.pageY}) != null)
		return false;
	    }
	}
    return true;
    }


function tv_doalert()
    {
    alert(this);
    }

function tv_build_layer(l,img_src,link_href,link_text, link_bold, is_last, has_subobj)
    {
    // build the image list
    l.imgs = new Array();
    l.imgs[l.tree_depth] = img_src;
    var start_img = (l.mainlayer.show_branches)?0:l.tree_depth;
    for(var i = start_img; i<l.tree_depth-1; i++)
	{
	if (l.parent.imgs[i] == '/sys/images/tree_middle_blank.gif' || l.parent.imgs[i] == '/sys/images/tree_corner_closed.gif' || l.parent.imgs[i] == '/sys/images/tree_corner_leaf.gif' || l.parent.imgs[i] == '/sys/images/tree_corner_open.gif')
	    l.imgs[i] = '/sys/images/tree_middle_blank.gif';
	else
	    l.imgs[i] = '/sys/images/tree_middle_branch.gif';
	}
    if (l.tree_depth > start_img)
	{
	if (is_last)
	    {
	    if (has_subobj == null || has_subobj == true)
		l.imgs[l.tree_depth-1] = '/sys/images/tree_corner_closed.gif';
	    else
		l.imgs[l.tree_depth-1] = '/sys/images/tree_corner_leaf.gif';
	    }
	else
	    {
	    if (has_subobj == null || has_subobj == true)
		l.imgs[l.tree_depth-1] = '/sys/images/tree_middle_closed.gif';
	    else
		l.imgs[l.tree_depth-1] = '/sys/images/tree_middle_leaf.gif';
	    }
	}

    // build the layer
    if(cx__capabilities.Dom0NS)
	{
	var tvtext = "";
	for(var i = start_img; i<=l.tree_depth; i++)
	    tvtext += "<IMG width='18' SRC='" + l.imgs[i] + "' align='left'>";
	tvtext += "&nbsp;<A HREF='" + link_href + "'>" +
	    (link_bold?"<b>":"") + link_text + (link_bold?"<b>":"") + "</A>";
	if (l.tvtext != tvtext)
	    {
	    l.expanded = 0;
	    l.tvtext = tvtext;
	    l.document.tags.A.textDecoration = "none";
	    l.document.writeln(l.tvtext);
	    l.document.close();
	    //pg_serialized_write(l, l.tvtext, null);
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
	for(var i = start_img; i<=l.tree_depth; i++)
	    {
	    var img = document.createElement('img');
	    img.setAttribute('width', '18');
	    img.setAttribute('src',l.imgs[i]);
	    img.setAttribute('align','left');
	    l.appendChild(img);
	    }

	/** the space - a0 is the unicode value for nbsp. **/
	l.appendChild(document.createTextNode("\u00a0"));

	/** the link **/
	var a = document.createElement('a');
	a.setAttribute('href',link_href);
	a.layer = l;
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
	if(typeof(l.objptr)=="function")
	    {
	    var win=window.open();
	    win.document.write("<PRE>"+l.objptr+"</PRE>");
	    win.document.close();
	    return 0;
	    }
	else
	    {
	    if(!l.objptr)
		{
		l.expanded=0;
		var ret=prompt(l.objn,l.parent.objptr[l.objn]);
		if(ret!=undefined)
		    {
		    switch(typeof(l.parent.objptr[l.objn]))
			{
			case "boolean":
			    if(ret=="true" || ret==1 || ret==-1)
				{
				l.parent.objptr[l.objn]=true;
				}
			    else
				{
				l.parent.objptr[l.objn]=false;
				}
			    break;
			default:
			    l.parent.objptr[l.objn]=ret;
			}
			o=l.parent.objptr[l.objn];
		    var link_txt=l.objn+"&nbsp;("+typeof(o)+"):&nbsp;"+o;
		    tv_build_layer(l,"/sys/images/ico01b.gif","",link_txt, false, false);
		    }
		return 0;
		}
	    var linkcnt=0;
	    for(var i in l.objptr) linkcnt++;
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
	setClipHeight(tv_tgt_layer.pdoc.tv_layer_tgt,getClipHeight(tv_tgt_layer.pdoc.tv_layer_tgt)+ 20*(linkcnt));

    var tgtTop = getRelativeY(tv_tgt_layer);
    var layers = pg_layers(tv_tgt_layer.pdoc);
    for (var j=0;j<layers.length;j++)
	{
	var sl = layers[j];
	if(cx__capabilities.Dom2CSS)
	    {
	    /** much faster code for DOM2CSS1 compliant browsers **/
	    var slTop = getRelativeY(sl);
	    var visibility = pg_get_style(sl,'visibility');
	    if (slTop >= tgtTop + 20 && sl != tv_tgt_layer && (visibility == 'inherit' || visibility == 'visible') )
		{
		setRelativeY(sl, slTop+20*linkcnt);
		}
	    }
	else
	    {
	    if (getRelativeY(sl) >= getRelativeY(tv_tgt_layer) + 20 && sl != tv_tgt_layer && sl.visibility == 'inherit')
		{
		setRelativeY(sl, getRelativeY(sl) + 20*(linkcnt));
		}
	    }
	}
    return 20*linkcnt;
    }

function tv_clear_objs(l) { for(var i = 2; i<l.pdoc.layers.length;i++) l.pdoc.layers[i].objptr = null; }

function tv_BuildNewLayers(l, linkcnt)
    {
    /** pre-load some variables **/
    //var tgtClipWidth = tv_tgt_layer.clip.width;
    var tgtClipWidth = tv_tgt_layer.mainlayer.setwidth - (getRelativeX(tv_tgt_layer) - getRelativeX(tv_tgt_layer.mainlayer)) - 20;
    var tgtX = getRelativeX(tv_tgt_layer);
    var tgtY = getRelativeY(tv_tgt_layer);
    var jsProps = null;
    var can_expand;
    var links;
    
    if (tv_tgt_layer) links = pg_links(tv_tgt_layer.ld);

    if(l.isjs)
	{
	jsProps = new Array();
	for(var prop in l.objptr)
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
	var one_link;
	var one_layer = tv_new_layer(tgtClipWidth,tv_tgt_layer.pdoc,l.mainlayer);
	var im;
	can_expand = null;

	one_layer.parent=l;
	one_layer.collapse=one_layer.parent.collapse;
	one_layer.expand=one_layer.parent.expand;
	if(l.isjs)
	    {
	    var j = jsProps[i-1];
	    if(j=="applets" || j=="embeds")
		{
		var o=null;
		var t="object";
		}
	    else
		{
		var o=l.objptr[j];
		var t=typeof(o);
		}
	    link_href="";
	    one_link=j + '.';
	    if(o && (t=="object" || t=="function"))
		{
		one_layer.objptr=o;
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
		if (t=='string' && o.length > 64)
		    link_txt = j+" ("+t+"): "+o.substr(0,64) + " ...";
		else
		    link_txt=j+" ("+t+"): "+o;
		one_layer.objptr=null;
		im = '01';
		}
	    one_layer.isjs=true;
	    one_layer.objn=j;
	    can_expand = (im == '02');
	    }
	else
	    {
	    link_txt = cx_info_extract_str(links[i].text);
	    link_href = links[i].href;
	    one_link = link_href.substring(link_href.lastIndexOf('/')+1,link_href.length);
	    if (one_link[0] == ' ') one_link = one_link.substring(1,one_link.length);
	    im = '01';
	    if (link_txt == '' || link_txt == null) link_txt = one_link;
	    else link_txt = one_link + ' - ' + link_txt;
	    //if (one_link.lastIndexOf('/') > 0) im = '02';
	    //else 
	    one_link = one_link + '/';
	    var flags = cx_info_extract_flags(links[i].text);
	    if (flags & cx_info_flags.can_have_subobj) im = '02';
	    if (flags & cx_info_flags.no_subobj) can_expand = false;
	    if (flags & cx_info_flags.has_subobj) can_expand = true;
	    }

	one_layer.tree_depth = one_layer.parent.tree_depth+1;
	tv_build_layer(one_layer,"/sys/images/ico" + im + "b.gif",link_href,link_txt, link_bold, i == linkcnt, can_expand);
	one_layer.link_href = link_href;
	one_layer.fname = tv_tgt_layer.fname + one_link;
	one_layer.type = im;
	one_layer.layer = one_layer;
	/*if (cx__capabilities.Dom0NS)
	    {*/
	    if (l.mainlayer.show_branches)
		moveTo(one_layer, tgtX, tgtY + 20*i);
	    else
		moveTo(one_layer, tgtX + 20, tgtY + 20*i);
	    /*}
	else if (cx__capabilities.Dom1HTML)
	    {
	    moveTo(one_layer, tgtX + 20, tgtY + 20);
	    }*/
	htr_setvisibility(one_layer, 'inherit');
	//pg_set_style_string(one_layer,'visibility','inherit');
	var images = pg_images(one_layer);
	one_layer.img = images[images.length-1];
	one_layer.img.kind = 'tv';
	one_layer.img.layer = one_layer;
	if (one_layer.mainlayer.show_branches && one_layer.tree_depth > 0)
	    {
	    images[images.length-2].kind = 'tv';
	    images[images.length-2].layer = one_layer;
	    }
	var one_links = pg_links(one_layer);
	if (one_links.length != 0)
	    {
	    one_links[0].kind = 'tv';
	    one_links[0].layer = one_layer;
	    }
	one_layer.expanded = 0;
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
    var offset;

    if (linkcnt < 0 && l.mainlayer.show_branches && l.tree_depth > 0)
	{
	if (l.imgs[l.tree_depth-1] == '/sys/images/tree_middle_open.gif')
	    l.imgs[l.tree_depth-1] = '/sys/images/tree_middle_leaf.gif';
	else if (l.imgs[l.tree_depth-1] == '/sys/images/tree_corner_open.gif')
	    l.imgs[l.tree_depth-1] = '/sys/images/tree_corner_leaf.gif';
	var imgs = pg_images(l);
	pg_set(imgs[l.tree_depth-1], 'src', l.imgs[l.tree_depth-1]);
	}

    if (linkcnt < 0) linkcnt = 0;

    offset = tv_MakeRoom(l, linkcnt);

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

    // scroll container?
    if (l.mainlayer.parentLayer)
	{
	var pl = l.mainlayer.parentLayer.mainlayer;
	if (pl.ifcProbe && pl.ifcProbe(ifAction).Exists('ScrollTo'))
	    {
	    pl.ifcProbe(ifAction).Invoke('ScrollTo', {RangeStart:getRelativeY(l), RangeEnd:getRelativeY(l)+offset+20});
	    }
	}

    pg_set(tv_tgt_layer.img,'src',tv_tgt_layer.img.realsrc);
    pg_set(tv_tgt_layer.img,'src',htutil_subst_last(tv_tgt_layer.img.src,'b.gif'));
    tv_tgt_layer.img.realsrc = null;
    tv_tgt_layer = null;
    return false;
    }

function tv_init(param)
    {
    var l = param.layer;
    //var pdoc = param.pdoc;
    //l.LSParent = param.parent;
    l.fname = param.fname;
    l.show_branches = param.branches;
    if (htr_getvisibility(l) == 'inherit')
	{
	l.tree_depth = 0;
	l.show_root = true;
	}
    else
	{
	l.tree_depth = -1;
	l.show_root = false;
	}
    var t;
    if(!l.is_initialized)
	{ /* not re-init */
	if(t=(/javascript:([a-zA-Z0-9_]*)/).exec(param.fname))
	   {
	   l.isjs=true;
	   if(t[1])
	       {
	       l.objptr=eval(t[1]);
	       }
	   else
	       {
	       l.objptr=window;
	       }
	   }
	}
    else
	{ /* re-init */
	tv_build_layer(l,"/sys/images/ico02b.gif","",l.fname,false,null);
	if (param.newroot)
	    l.objptr = param.newroot;
	else if (t=(/javascript:([a-zA-Z0-9_]*)/).exec(param.fname))
	    l.objptr = eval(t[1]);
	else
	    l.objptr = window;
	l.isjs=true;
	}
    l.expanded = 0;
    l.type = '02';
    l.img = pg_images(l)[0];
    l.img.layer = l;
    l.img.kind = 'tv';
    l.pdoc = wgtrGetContainer(wgtrGetParent(l));
    //l.pdoc = pdoc;
    l.ld = param.loader;
    htr_init_layer(l,l,'tv');
    ifc_init_widget(l);
    //l.ld.parent = l;
    l.root = l;
    if (!l.pdoc.tv_layer_cache) l.pdoc.tv_layer_cache = new Array();
    if(cx__capabilities.Dom0NS)
	{
	l.pdoc.tv_layer_tgt = l.parentLayer;
	}
    else if(cx__capabilities.Dom1HTML)
	{
	l.pdoc.tv_layer_tgt = l.parentNode;
	}
    else
	{
	alert('browser not supported');
	}
    setClipWidth(l, param.width);
    l.setwidth = param.width;
    l.childimgs = '';
    l.collapse=tv_collapse;
    l.expand=tv_expand;

    // Actions
    var ia=l.ifcProbeAdd(ifAction);
    ia.Add("SetRoot", tv_action_setroot);
    ia.Add("SetFocus", tv_action_setfocus);

    // Events
    var ie=l.ifcProbeAdd(ifEvent);
    ie.Add("Click");
    ie.Add("MouseDown");
    ie.Add("MouseUp");
    ie.Add("MouseOver");
    ie.Add("MouseOut");
    ie.Add("MouseMove");
    ie.Add("ClickItem");
    ie.Add("RightClickItem");

    l.is_initialized = true;

    // Auto expand if not showing root, otherwise what's the use?
    if (htr_getvisibility(l) != 'inherit') l.root.expand();    
    }

function tv_expand()
    {
    var l = this;
    if (l==null) return false;
    if (l.expanded==1) return false;
    if (l.img.realsrc != null) return false;
    l.img.realsrc=l.img.src;
    pg_set(l.img,'src','/sys/images/ico11c.gif');
    tv_tgt_layer = l;
    l.tvtext = '';

    if (l.mainlayer.show_branches && l.tree_depth > 0)
	{
	if (l.imgs[l.tree_depth-1] == '/sys/images/tree_middle_closed.gif')
	    l.imgs[l.tree_depth-1] = '/sys/images/tree_middle_open.gif';
	else if (l.imgs[l.tree_depth-1] == '/sys/images/tree_corner_closed.gif')
	    l.imgs[l.tree_depth-1] = '/sys/images/tree_corner_open.gif';
	var imgs = pg_images(l);
	pg_set(imgs[l.tree_depth-1], 'src', l.imgs[l.tree_depth-1]);
	}

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
	    pg_serialized_load(l.ld, use_fname + '&ls__mode=list&ls__info=1', tv_loaded);
	else
	    pg_serialized_load(l.ld, use_fname + '?ls__mode=list&ls__info=1', tv_loaded);
	}
    }

function tv_collapse()
    {
    var l = this;
    var sl;
    if (l==null) return false;
    if (l.expanded==0) return false;
    if (l.img.realsrc != null) return false;
    l.img.realsrc=l.img.src;
    pg_set(l.img,'src','/sys/images/ico11c.gif');
    tv_tgt_layer = l;

    if (l.mainlayer.show_branches && l.tree_depth > 0)
	{
	if (l.imgs[l.tree_depth-1] == '/sys/images/tree_middle_open.gif')
	    l.imgs[l.tree_depth-1] = '/sys/images/tree_middle_closed.gif';
	else if (l.imgs[l.tree_depth-1] == '/sys/images/tree_corner_open.gif')
	    l.imgs[l.tree_depth-1] = '/sys/images/tree_corner_closed.gif';
	var imgs = pg_images(l);
	pg_set(imgs[l.tree_depth-1], 'src', l.imgs[l.tree_depth-1]);
	}


    l.expanded = 0;
    var cnt = 0;
    var lyrs = pg_layers(l.pdoc);
    var len = lyrs.length;
    pg_debug('tv_collapse: ' + l.fname + '\n');
    for(var i=len-1;i>=0;i--)
	{
	sl = lyrs[i];
	if (sl.fname!=null && sl!=l && l.fname==sl.fname.substring(0,l.fname.length))
	    {
	    //pg_debug('tv_collapse: caching ' + sl.fname + '\n');
	    //alert(sl.fname);
	    tv_cache_layer(sl,l.pdoc);
	    //delete lyrs[i];
	    sl.fname = null;
	    if(cx__capabilities.Dom0NS)
		{
		sl.document.onmouseup = null;
		}
	    else if(cx__capabilities.Dom1HTML)
		{
		sl.onmouseup = null;
		}
	    else
		{
		alert('browser not supported');
		}
	    cnt++;
	    }
	//layers = pg_layers(l.pdoc);
	}
    //sl = layers[0];
    var vis = '';
    for (var j=0;j<len;j++)
	{
	sl = lyrs[j];
	var visibility = pg_get_style(sl,'visibility');
	if (getRelativeY(sl) > getRelativeY(l) && (visibility == 'inherit' || visibility == 'visible') )
	    {
	    setRelativeY(sl, getRelativeY(sl)-20*cnt);
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
