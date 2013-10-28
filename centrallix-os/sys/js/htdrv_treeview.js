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
	    if (width) nl.style.width = width + 'px';
	    nl.className = l.divclass;
	    //setClip(0, width, 0, 0);
	    pg_set_style(nl, 'position','absolute');
	    pg_set_style(nl, 'overflow','visible');
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
    if (l.selected)
	{
	l.root.selectitem(null);
	}
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
    tv_init({layer:this.root, fname:aparam.NewRoot, loader:this.root.ld, width:getClipWidth(this.root), newroot:aparam.NewRootObj, branches:this.show_branches, use3d:this.use3d, showrb:this.show_root_branch, icon:this.icon, divclass:this.divclass, sbg:this.sel_bg, desc:this.ord_desc});
    //tv_init({layer:this.root, fname:aparam.NewRoot, loader:this.root.ld, pdoc:this.root.pdoc, width:getClipWidth(this.root), newroot:aparam.NewRootObj, branches:this.show_branches});
    if (aparam.Expand == 'yes') this.root.expand(null);
    }

function tv_doclick(e)
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
	    r.selectitem(e.target.layer);
	    r.ifcProbe(ifEvent).Activate('ClickItem', {Pathname:path, Name:e.target.layer.objn, Label:e.target.layer.link_txt, HRef:e.target.layer.link_href, Caller:r});
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
	    if (r.ifcProbe(ifEvent).Activate('RightClickItem', {Pathname:hr, Caller:r, X:e.pageX, Y:e.pageY, Label:e.target.layer.link_txt, Name:e.target.layer.objn}) != null)
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
	if (l.parent.imgs[i] == l.root.imgnames.mid_blank || l.parent.imgs[i] == l.root.imgnames.cnr_closed || l.parent.imgs[i] == l.root.imgnames.cnr_leaf || l.parent.imgs[i] == l.root.imgnames.cnr_open)
	    l.imgs[i] = l.root.imgnames.mid_blank;
	else
	    l.imgs[i] = l.root.imgnames.mid_branch;
	}
    if (l.tree_depth > start_img)
	{
	if (is_last)
	    {
	    if (has_subobj == null || has_subobj == true)
		l.imgs[l.tree_depth-1] = l.root.imgnames.cnr_closed;
	    else
		l.imgs[l.tree_depth-1] = l.root.imgnames.cnr_leaf;
	    }
	else
	    {
	    if (has_subobj == null || has_subobj == true)
		l.imgs[l.tree_depth-1] = l.root.imgnames.mid_closed;
	    else
		l.imgs[l.tree_depth-1] = l.root.imgnames.mid_leaf;
	    }
	}

    // build the layer
    l.expanded = false;
    if(cx__capabilities.Dom0NS)
	{
	var tvtext = "<nobr>";
	for(var i = start_img; i<=l.tree_depth; i++)
	    tvtext += "<IMG width='" + l.root.iconwidth + "' SRC='" + l.imgs[i] + "' align='left'>";
	tvtext += "&nbsp;<A HREF='" + link_href + "'>" +
	    (link_bold?"<b>":"") + htutil_encode(htutil_obscure(link_text)) + (link_bold?"<b>":"") + "</A></nobr>";
	if (l.tvtext != tvtext)
	    {
	    l.tvtext = tvtext;
	    l.document.tags.A.textDecoration = "none";
	    htr_write_content(l, l.tvtext);
	    //l.document.writeln(l.tvtext);
	    //l.document.close();
	    l.document.tags.A.textDecoration = "none";
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
	var nobr = document.createElement('nobr');
	l.appendChild(nobr);
	for(var i = start_img; i<=l.tree_depth; i++)
	    {
	    var img = document.createElement('img');
	    img.setAttribute('width', '' + l.root.iconwidth);
	    img.setAttribute('src',l.imgs[i]);
	    img.setAttribute('align','left');
	    nobr.appendChild(img);
	    }

	/** the space - a0 is the unicode value for nbsp. **/
	nobr.appendChild(document.createTextNode("\u00a0"));

	/** the link **/
	var a = document.createElement('a');
	a.setAttribute('href',link_href);
	a.layer = l;
	a.style.textDecoration = "none";
	if(link_bold)
	    {
	    var bold = document.createElement('b');
	    bold.appendChild(document.createTextNode(String(htutil_obscure(link_text)).replace(/ /g, "\u00a0")));
	    a.appendChild(bold);
	    }
	else
	    {
	    a.appendChild(document.createTextNode(String(htutil_obscure(link_text)).replace(/ /g, "\u00a0")));
	    }
	nobr.appendChild(a);
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
		l.expanded=false;
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
		    tv_build_layer(l,l.root.imgnames.ico_file,"",link_txt, false, false);
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
	return pg_links(l.ld).length-1;
	}
    }

function tv_MakeRoom(tv_tgt_layer, linkcnt)
    {
    if (window != tv_tgt_layer.pdoc.tv_layer_tgt)
	setClipHeight(tv_tgt_layer.pdoc.tv_layer_tgt,getClipHeight(tv_tgt_layer.pdoc.tv_layer_tgt)+ tv_tgt_layer.root.rowheight*(linkcnt));

    var tgtTop = getRelativeY(tv_tgt_layer);
    var layers = pg_layers(tv_tgt_layer.pdoc);
    for (var j=0;j<layers.length;j++)
	{
	var sl = layers[j];
	var slTop = getRelativeY(sl);
	if (slTop >= tgtTop + tv_tgt_layer.root.rowheight && sl != tv_tgt_layer && htr_getvisibility(sl) == 'inherit')
	    setRelativeY(sl, slTop+tv_tgt_layer.root.rowheight*linkcnt);
	}
    return tv_tgt_layer.root.rowheight*linkcnt;
    }

function tv_clear_objs(l) { for(var i = 2; i<l.pdoc.layers.length;i++) l.pdoc.layers[i].objptr = null; }

function tv_BuildNewLayers(l, linkcnt)
    {
    /** pre-load some variables **/
    //var tgtClipWidth = tv_tgt_layer.clip.width;
    var tgtClipWidth = l.mainlayer.setwidth - (getRelativeX(l) - getRelativeX(l.mainlayer)) - l.root.iconwidth;
    var tgtX = getRelativeX(l);
    var tgtY = getRelativeY(l);
    var jsProps = null;
    var can_expand;
    var links;
    
    //if (tv_tgt_layer) links = pg_links(tv_tgt_layer.ld);
    if (!l.isjs) links = pg_links(l.ld);

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
	//var one_layer = tv_new_layer(tgtClipWidth,l.pdoc,l.mainlayer);
	//var one_layer = tv_new_layer(null,l.pdoc,l.mainlayer);
	var one_layer = tv_new_layer(l.mainlayer.setwidth,l.pdoc,l.mainlayer);
	//setClipWidth(one_layer, tgtClipWidth);
	setClipWidth(one_layer, l.mainlayer.setwidth);
	setClipHeight(one_layer, l.root.rowheight);
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
		    im = l.root.imgnames.ico_file;
		    }
		else
		    {
		    im = l.root.imgnames.ico_folder;
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
		im = l.root.imgnames.ico_file;
		}
	    one_layer.isjs=true;
	    one_layer.objn=j;
	    can_expand = (im == l.root.imgnames.ico_folder);
	    }
	else
	    {
	    link_txt = cx_info_extract_str(links[i].text);
	    link_href = links[i].href;
	    one_link = link_href.substring(link_href.lastIndexOf('/')+1,link_href.length);
	    if (one_link[0] == ' ') one_link = one_link.substring(1,one_link.length);
	    one_layer.objn = one_link;
	    im = l.root.imgnames.ico_file;
	    if (link_txt == '' || link_txt == null) link_txt = one_link;
	    else link_txt = one_link + ' - ' + link_txt;
	    one_link = one_link + '/';
	    var flags = cx_info_extract_flags(links[i].text);
	    if (flags & cx_info_flags.can_have_subobj) im = l.root.imgnames.ico_folder;
	    if (flags & cx_info_flags.no_subobj) can_expand = false;
	    if (flags & cx_info_flags.has_subobj) can_expand = true;
	    }

	one_layer.tree_depth = one_layer.parent.tree_depth+1;
	one_layer.root = l.root;
	tv_build_layer(one_layer,im,link_href,link_txt, link_bold, i == linkcnt, can_expand);
	one_layer.link_href = link_href;
	one_layer.fname = l.fname + one_link;
	one_layer.layer = one_layer;
	one_layer.link_txt = link_txt;
	if (l.mainlayer.show_branches)
	    moveTo(one_layer, tgtX, tgtY + l.root.rowheight*i);
	else
	    moveTo(one_layer, tgtX + l.root.iconwidth, tgtY + l.root.rowheight*i);
	htr_setvisibility(one_layer, 'inherit');
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
	one_layer.expanded = false;
	one_layer.expanding = false;
	one_layer.autoexpanded = false;
	one_layer.zIndex = l.zIndex;
	one_layer.pdoc = l.pdoc;
	one_layer.ld = l.ld;
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
    //var l=tv_tgt_layer;
    var l = this.loadtarget;
    var linkcnt = tv_GetLinkCnt(l);
    var offset;

    if (linkcnt < 0 && l.mainlayer.show_branches && l.tree_depth > 0)
	{
	if (l.imgs[l.tree_depth-1] == l.root.imgnames.mid_open)
	    l.imgs[l.tree_depth-1] = l.root.imgnames.mid_leaf;
	else if (l.imgs[l.tree_depth-1] == l.root.imgnames.cnr_open)
	    l.imgs[l.tree_depth-1] = l.root.imgnames.cnr_leaf;
	var imgs = pg_images(l);
	pg_set(imgs[l.tree_depth-1], 'src', l.imgs[l.tree_depth-1]);
	}

    if (linkcnt < 0) linkcnt = 0;

    offset = tv_MakeRoom(l, linkcnt);

    tv_BuildNewLayers(l, linkcnt);

    if(cx__capabilities.Dom0NS)
	{
	pg_resize(l.parentLayer);
	}
    else if(cx__capabilities.Dom1HTML)
	{
	pg_resize(l.parentNode);
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
	    pl.ifcProbe(ifAction).Invoke('ScrollTo', {RangeStart:getRelativeY(l), RangeEnd:getRelativeY(l)+offset+l.root.rowheight});
	    }
	}

    pg_set(l.img,'src',l.img.realsrc);
    if (l.root.use3d)
	pg_set(l.img,'src',htutil_subst_last(l.img.src,'b.gif'));
    l.img.realsrc = null;
    tv_tgt_layer = null;

    l.expanding = false;

    if (l.mainlayer.expand_completion_cb)
	l.mainlayer.expand_completion_cb();
    return false;
    }


function tv_osrc_data_available()
    {
    //this.Clear();
    }

function tv_osrc_replica_moved()
    {
    //this.Refresh();
    //if (this.param.show_select) this.SelectItemById(this.osrc.CurrentRecord);
    }

function tv_osrc_is_discard_ready()
    {
    return true;
    }

function tv_osrc_object_available(o)
    {
    //this.Refresh();
    //if (this.param.show_select) this.SelectItemById(this.osrc.CurrentRecord);
    }

function tv_osrc_object_created(o)
    {
    //this.Refresh();
    }

function tv_osrc_object_modified(o)
    {
    //this.Refresh();
    //if (this.param.show_select) this.SelectItemById(this.osrc.CurrentRecord);
    }

function tv_osrc_object_deleted(o)
    {
    //this.Refresh();
    }

function tv_osrc_operation_complete(o)
    {
    return true;
    }

function tv_add_rule(node, param)
    {
    node.treeview = this;
    node.ruletype = param.ruletype;
    if (node.ruletype == 'osrc-link')
	{
	node.osrc = wgtrGetNode(node, param.osrc);
	node.pattern = param.pattern;
	if (param.query_form)
	    node.query_form = wgtrGetNode(node, param.query_form);

	// OSRC client callbacks.
	if (node.osrc)
	    {
	    node.DataAvailable = tv_osrc_data_available;
	    node.ReplicaMoved = tv_osrc_replica_moved;
	    node.IsDiscardReady = tv_osrc_is_discard_ready;
	    node.ObjectAvailable = tv_osrc_object_available;
	    node.ObjectCreated = tv_osrc_object_created;
	    node.ObjectModified = tv_osrc_object_modified;
	    node.ObjectDeleted = tv_osrc_object_deleted;
	    node.OperationComplete = tv_osrc_operation_complete;
	    node.osrc.Register(node);
	    }
	}
    else if (node.ruletype == 'list')
	{
	node.pattern = param.pattern;
	node.list = param.list;
	}
    return node;
    }


function tv_selectitem(item)
    {
    if (this.cur_selected && this.cur_selected.selected)
	{
	htr_setbackground(this.cur_selected, null);
	this.cur_selected.className = this.divclass;
	this.cur_selected.selected = false;
	this.cur_selected = null;
	this.selected_name = null;
	this.selected_label = null;
	}
    if (item)
	{
	this.cur_selected = item;
	htr_setbackground(item, this.sel_bg);
	this.cur_selected.selected = true;
	this.cur_selected.className = (this.divclass + 'h');
	this.selected_name = this.cur_selected.objn;
	this.selected_label = this.cur_selected.link_txt;
	this.ifcProbe(ifEvent).Activate('SelectItem', {Pathname:item.fname, Name:this.selected_name, Label:this.selected_label, HRef:item.link_href, Caller:item.root});
	}
    else
	{
	this.ifcProbe(ifEvent).Activate('SelectItem', {Pathname:null, Name:null, Label:null, HRef:null, Caller:null});
	}
    }


function tv_init(param)
    {
    var l = param.layer;
    //var pdoc = param.pdoc;
    //l.LSParent = param.parent;
    l.fname = param.fname;
    l.show_branches = param.branches;
    l.show_root_branch = param.showrb;
    l.use3d = param.use3d;
    l.icon = param.icon;
    l.divclass = param.divclass;
    l.sel_bg = param.sbg;
    l.ord_desc = param.desc;
    l.searchqueue = [];
    l.patharray = [];
    l.cur_selected = null;
    l.selected_name = null;
    l.selected_label = null;
    l.search_cnt = 0;
    l.cur_search = null;
    l.rows = [];
    l.imgnames = {};
    if (l.use3d)
	{
	l.rowheight = 20;
	l.iconwidth = 18;
	l.imgnames.cnr_closed = '/sys/images/tree_corner_closed.gif';
	l.imgnames.cnr_leaf = '/sys/images/tree_corner_leaf.gif';
	l.imgnames.cnr_open = '/sys/images/tree_corner_open.gif';
	l.imgnames.mid_blank = '/sys/images/tree_middle_blank.gif';
	l.imgnames.mid_branch = '/sys/images/tree_middle_branch.gif';
	l.imgnames.mid_closed = '/sys/images/tree_middle_closed.gif';
	l.imgnames.mid_leaf = '/sys/images/tree_middle_leaf.gif';
	l.imgnames.mid_open = '/sys/images/tree_middle_open.gif';
	l.imgnames.ico_loading = '/sys/images/ico11c.gif';
	l.imgnames.ico_file = '/sys/images/ico01b.gif';
	l.imgnames.ico_folder = '/sys/images/ico02b.gif';
	}
    else
	{
	l.rowheight = 16;
	l.iconwidth = 16;
	l.imgnames.cnr_closed = '/sys/images/tree_sm_corner_closed.gif';
	l.imgnames.cnr_leaf = '/sys/images/tree_sm_corner_leaf.gif';
	l.imgnames.cnr_open = '/sys/images/tree_sm_corner_open.gif';
	l.imgnames.mid_blank = '/sys/images/tree_sm_middle_blank.gif';
	l.imgnames.mid_branch = '/sys/images/tree_sm_middle_branch.gif';
	l.imgnames.mid_closed = '/sys/images/tree_sm_middle_closed.gif';
	l.imgnames.mid_leaf = '/sys/images/tree_sm_middle_leaf.gif';
	l.imgnames.mid_open = '/sys/images/tree_sm_middle_open.gif';
	l.imgnames.ico_loading = '/sys/images/ico11_16x16.gif';
	l.imgnames.ico_file = '/sys/images/ico01_16x16.gif';
	l.imgnames.ico_folder = '/sys/images/ico02_16x16.gif';
	}
    if (l.icon)
	{
	l.imgnames.ico_file = l.icon;
	l.imgnames.ico_folder = l.icon;
	}
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
    if (l.show_root_branch) l.tree_depth = 0;
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
	tv_build_layer(l,l.root.imgnames.ico_folder,"",l.fname,false,null);
	if (param.newroot)
	    l.objptr = param.newroot;
	else if (t=(/javascript:([a-zA-Z0-9_]*)/).exec(param.fname))
	    l.objptr = eval(t[1]);
	//else
	    //l.objptr = window;
	l.isjs=l.objptr?true:false;
	}
    l.expanded = false;
    l.expanding = false;
    l.img = pg_images(l)[0];
    if (l.img)
	{
	l.img.layer = l;
	l.img.kind = 'tv';
	}
    l.pdoc = wgtrGetParentContainer(l);
    //l.pdoc = pdoc;
    l.ld = param.loader;
    l.ld.mainlayer = l;
    htr_init_layer(l,l,'tv');
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
    l.searchfor = tv_searchfor;
    l.selectitem = tv_selectitem;
    l.findpath = tv_findpath;
    l.beginfindpath = tv_beginfindpath;
    l.addRule = tv_add_rule;

    l.initial_reveal = false;

    if (!l.is_initialized)
	{
	ifc_init_widget(l);

	// Actions
	var ia=l.ifcProbeAdd(ifAction);
	ia.Add("SetRoot", tv_action_setroot);
	ia.Add("SetFocus", tv_action_setfocus);
	ia.Add("Search", tv_action_search);
	ia.Add("SearchNext", tv_action_search_next);

	// Events
	var ie=l.ifcProbeAdd(ifEvent);
	ie.Add("Click");
	ie.Add("MouseDown");
	ie.Add("MouseUp");
	ie.Add("MouseOver");
	ie.Add("MouseOut");
	ie.Add("MouseMove");
	ie.Add("ClickItem");
	ie.Add("SelectItem");
	ie.Add("RightClickItem");

	// Request reveal/obscure notifications
	l.Reveal = tv_cb_reveal;
	if (pg_reveal_register_listener(l))
	    {
	    // already visible
	    l.Reveal({eventName:'Reveal'});
	    }
	}
    else
	{
	// auto expand if not showing root
	if (htr_getvisibility(l) != 'inherit') l.root.expand(null);    
	l.initial_reveal = true;
	}

    l.is_initialized = true;
    }

function tv_expand(cb)
    {
    var l = this;
    if (l==null) return false;
    if (!l.expanding || !l.mainlayer.expand_completion_cb)
	l.mainlayer.expand_completion_cb = cb;
    if (l.expanded)
	{
	if (!l.expanding)
	    {
	    if (l.mainlayer.expand_completion_cb)
		l.mainlayer.expand_completion_cb();
	    }
	return false;
	}
    if (!l.autoexpanded)
	{
	var sl = l;
	while(sl.parent)
	    {
	    // a manual expand at a deeper level cancels autoexpanded
	    // settings at higher levels
	    sl.parent.autoexpanded = false;
	    sl = sl.parent;
	    }
	}
    else
	{
	// autocollapse an autoexpanded node at same level?
	var arr = pg_layers(l.pdoc);
	var len = arr.length;
	for(var i = 0; i < len; i++)
	    {
	    var one = arr[i];
	    if (one.parent == l.parent && one.autoexpanded)
		one.collapse();
	    }
	}
    if (l.img.realsrc != null) return false;
    l.img.realsrc=l.img.src;
    pg_set(l.img,'src',l.root.imgnames.ico_loading);
    tv_tgt_layer = l;
    l.tvtext = '';

    if (l.mainlayer.show_branches && l.tree_depth > 0)
	{
	if (l.imgs[l.tree_depth-1] == l.root.imgnames.mid_closed)
	    l.imgs[l.tree_depth-1] = l.root.imgnames.mid_open;
	else if (l.imgs[l.tree_depth-1] == l.root.imgnames.cnr_closed)
	    l.imgs[l.tree_depth-1] = l.root.imgnames.cnr_open;
	var imgs = pg_images(l);
	pg_set(imgs[l.tree_depth-1], 'src', l.imgs[l.tree_depth-1]);
	}

    l.expanded = true;
    l.expanding = true;
    l.ld.loadtarget = l;
    if(l.isjs)
	{
	l.ld.onload=tv_loaded;
	l.ld.onload();
	}
    else
	{
	if (l.fname.substr(l.fname.length-1,1) == '/' && l.fname.length > 1)
	    use_fname = l.fname.substr(0,l.fname.length-1);
	else
	    use_fname = l.fname;
	if (use_fname.lastIndexOf('?') > use_fname.lastIndexOf('/', use_fname.length-2))
	    pg_serialized_load(l.ld, use_fname + '&cx__akey='+akey+'&ls__mode=list&ls__info=1&ls__orderdesc=' + l.ord_desc, tv_loaded);
	else
	    pg_serialized_load(l.ld, use_fname + '?cx__akey='+akey+'&ls__mode=list&ls__info=1&ls__orderdesc=' + l.ord_desc, tv_loaded);
	}
    }

function tv_action_search(aparam)
    {
    if (aparam.Value) this.searchfor(aparam.Value);
    }

function tv_action_search_next(aparam)
    {
    if (!this.search_cnt) return;
    if (this.search_cnt >= this.rows.length) this.search_cnt = 0;
    this.beginfindpath();
    }

function tv_searchfor(str)
    {
    var ckstr = (new String(str)).match(/^[\w ,.-]*$/);
    if (!ckstr || !ckstr[0]) return;
    ckstr = ckstr[0].toUpperCase();
    this.searchqueue.push(ckstr);
    if (this.searchqueue.length == 1)
	{
	this.cur_search = ckstr;

	// stay on current item?
	if (this.cur_selected && this.cur_selected.link_txt && (new String(this.cur_selected.link_txt)).toUpperCase().indexOf(ckstr) >= 0)
	    {
	    this.searchqueue.shift();
	    return;
	    }
	var ckfname = (new String(this.root.fname)).match(/^\/[^\s,;]*$/);
	if (!ckfname || !ckfname[0]) return;
	ckfname = ckfname[0];
	var q = "select :name, :annotation, :__cx_path from subtree " + ckfname + " having charindex(\"" + ckstr + "\", upper(:name)) > 0 or charindex(\"" + ckstr + "\", upper(:annotation)) > 0";
	pg_serialized_load(this.ld, '/?cx__akey=' + akey + '&ls__rowcount=10&ls__mode=query&ls__sql=' + htutil_escape(q), tv_searchloaded, true);
	}
    }

function tv_searchloaded()
    {
    // remove the one we just did
    this.mainlayer.searchqueue.shift();

    // parse it
    this.mainlayer.search_cnt = 0;
    this.mainlayer.rows = htr_parselinks(pg_links(this));
    if (this.mainlayer.rows.length == 0)
	{
	this.mainlayer.selectitem(null);
	}

    this.mainlayer.beginfindpath();
    }


function tv_beginfindpath()
    {
    var s_name = null;
    var s_path = null;
    var s_annot = null;

    if (this.rows.length > this.search_cnt)
	{
	for (var i in this.rows[this.search_cnt])
	    {
	    var col = this.rows[this.search_cnt][i];
	    switch(col.oid)
		{
		case 'name': s_name = col.value; break;
		case 'annotation': s_annot = col.value; break;
		case '__cx_path': s_path = col.value; break;
		}
	    }
	}

    // find it in the tree
    if (s_name && s_path)
	{
	this.search_cnt++;

	// doesn't match this one?
	if ((new String(s_name)).toUpperCase().indexOf(this.cur_search) < 0 && (new String(s_annot)).toUpperCase().indexOf(this.cur_search) < 0)
	    {
	    this.ifcProbe(ifAction).Invoke("SearchNext", {});
	    }
	else
	    {
	    this.patharray = (new String(s_path)).split('/');
	    this.pathitem = this.root;
	    if (!this.pathitem.expanded)
		this.pathitem.autoexpanded = true;
	    this.pathitem.expand(tv_findpath);
	    }
	}
    else
	{
	// get last request placed
	var newstr = this.searchqueue.pop();
	this.searchqueue = [];

	// run the request
	if (newstr) this.searchfor(newstr);
	}
    }


function tv_findpath()
    {
    // if no more items, we're done
    if (this.patharray.length == 0) return;

    var curitem = this.patharray.shift();

    // Find the 'current' item.
    var lyrs = pg_layers(this.pdoc);
    var len = lyrs.length;
    for(var i = 0; i < len; i++)
	{
	var l = lyrs[i];
	if (l.parent == this.pathitem && l.objn == curitem)
	    {
	    this.pathitem = l;
	    }
	}

    // Scroll to what we found so far
    var pl = wgtrGetParent(this);
    if (pl.ifcProbe && pl.ifcProbe(ifAction).Exists('ScrollTo'))
	{
	pl.ifcProbe(ifAction).Invoke('ScrollTo', {RangeStart:getRelativeY(this.pathitem), RangeEnd:getRelativeY(this.pathitem)+this.root.rowheight});
	}

    // Last item to find?
    if (this.patharray.length == 0)
	{
	// if this is the last item, highlight it.
	this.selectitem(this.pathitem);

	// get last request placed
	var newstr = this.searchqueue.pop();
	this.searchqueue = [];

	// run the request
	if (newstr) this.searchfor(newstr);
	}
    else
	{
	// otherwise, expand it and keep looking
	if (!this.pathitem.expanded)
	    this.pathitem.autoexpanded = true;
	this.pathitem.expand(tv_findpath);
	}
    }

function tv_collapse()
    {
    var l = this;
    var sl;
    if (l==null) return false;
    if (!l.expanded) return false;
    if (l.img.realsrc != null) return false;
    l.img.realsrc=l.img.src;
    pg_set(l.img,'src',l.root.imgnames.ico_loading);
    tv_tgt_layer = l;

    if (l.mainlayer.show_branches && l.tree_depth > 0)
	{
	if (l.imgs[l.tree_depth-1] == l.root.imgnames.mid_open)
	    l.imgs[l.tree_depth-1] = l.root.imgnames.mid_closed;
	else if (l.imgs[l.tree_depth-1] == l.root.imgnames.cnr_open)
	    l.imgs[l.tree_depth-1] = l.root.imgnames.cnr_closed;
	var imgs = pg_images(l);
	pg_set(imgs[l.tree_depth-1], 'src', l.imgs[l.tree_depth-1]);
	}


    l.expanded = false;
    l.autoexpanded = false;
    var cnt = 0;
    var lyrs = pg_layers(l.pdoc);
    var len = lyrs.length;
    pg_debug('tv_collapse: ' + l.fname + '\n');
    for(var i=len-1;i>=0;i--)
	{
	sl = lyrs[i];
	if (sl.fname!=null && sl!=l && sl.fname != l.fname && l.fname==sl.fname.substring(0,l.fname.length))
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
	    setRelativeY(sl, getRelativeY(sl)-l.root.rowheight*cnt);
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
    if (l.root.use3d)
	pg_set(l.img,'src',htutil_subst_last(l.img.src,'b.gif'));
    l.img.realsrc = null;
    }


// Called when the treeview is revealed/shown to the user
function tv_cb_reveal(event)
    {
    switch(event.eventName)
	{
	case 'Reveal':
	    if (!this.initial_reveal)
		{
		// Auto expand if not showing root, otherwise what's the use?
		if (htr_getvisibility(this) != 'inherit') this.root.expand(null);    
		this.initial_reveal = true;
		}
	    break;
	case 'Obscure':
	    break;
	case 'RevealCheck':
	    pg_reveal_check_ok(event);
	    break;
	case 'ObscureCheck':
	    pg_reveal_check_ok(event);
	    break;
	}
    }


// Event handlers
function tv_click(e)
    {
    if (cx__capabilities.Dom0NS)
	{
	if (e.target != null && e.target.kind == 'tv' && e.target.href != null)
	    {
	    cn_activate(e.mainlayer, 'Click');
	    if (tv_doclick(e))
		return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
	    else
		return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	    }
	}
    else
	{
	if (e.target != null && e.target.kind == 'tv' && (e.target.nodeName == 'A' || e.target.nodeName == 'DIV'))
	    {
	    //htr_alert(e,1);
	    //alert(e.target.objn);
	    //e.mainlayer.ifcProbe(ifEvent).Activate('ClickItem', {Pathname:e.target.fname, Name:e.target.objn, Caller:e.mainlayer, X:e.pageX, Y:e.pageY});
	    cn_activate(e.mainlayer, 'Click');
	    if (tv_doclick(e))
		return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
	    else
		return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	    }
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tv_mousedown(e)
    {
    if (e.kind == 'tv') cn_activate(e.mainlayer, 'MouseDown');
    if (e.target && e.target.kind == 'tv' && ((cx__capabilities.Dom0NS && e.target.href == null) || (!cx__capabilities.Dom0NS && e.target.nodeName != 'A' && e.target.nodeName != 'DIV')))
        {
        if (e.which == 3)
            {
            return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
	    //return tv_rclick(e);
            }
        else
            {
            tv_target_img = e.target;
	    if (tv_target_img.layer.root.use3d)
		pg_set(tv_target_img.layer.img,'src',htutil_subst_last(tv_target_img.layer.img.src,'c.gif'));
            }
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tv_mouseup(e)
    {
    if (e.kind == 'tv') cn_activate(e.mainlayer, 'MouseUp');
    if (e.target != null && e.target.kind == 'tv' && e.which == 3) 
	return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
    if (tv_target_img != null && tv_target_img.kind == 'tv')
        {
        var l = tv_target_img.layer;
        tv_target_img = null;
        if (!l.expanded)
            {
            if (l.expand(null))
		return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
	    else
		return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
            }
        else
            {
            if (l.collapse())
		return EVENT_HALT | EVENT_ALLOW_DEFAULT_ACTION;
	    else
		return EVENT_HALT | EVENT_PREVENT_DEFAULT_ACTION;
            }
        }
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tv_mouseover(e)
    {
    if (e.kind == 'tv')
	{
	cn_activate(e.mainlayer, 'MouseOver');
	if (getClipWidth(e.layer) <= getdocWidth(e.layer)+2 && e.layer.link_txt)
	    e.layer.tipid = pg_tooltip(e.layer.link_txt, e.pageX, e.pageY);
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tv_mousemove(e)
    {
    if (e.kind == 'tv') cn_activate(e.mainlayer, 'MouseMove');
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }

function tv_mouseout(e)
    {
    if (e.kind == 'tv')
	{
	cn_activate(e.mainlayer, 'MouseOut');
	if (e.layer.tipid)
	    {
	    pg_canceltip(e.layer.tipid);
	    e.layer.tipid = null;
	    }
	}
    return EVENT_CONTINUE | EVENT_ALLOW_DEFAULT_ACTION;
    }


// Load indication
if (window.pg_scripts) pg_scripts['htdrv_treeview.js'] = true;
