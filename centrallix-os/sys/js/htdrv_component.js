// Copyright (C) 1998-2006 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

/*
  by Seth Bird:

  This file is where the "async hack" (see README file in js/ folder)
  is implemented in order to request for components. A js 'component'
  is the actual iframe ('layer' in NS) and is what should be the
  'this' for cmp_initializer. The async requests will be for a
  "componentdecl" widget (which is where the actual definition of the
  component is). When the componentdecl widget is finished loading, it
  will then be moved out of the layer/iframe into the main page as its
  own tree so that the componentdecl widget is actually not ever
  nested in the main widget tree. This is done so that there is no
  scope clobbering (ie, so that there are no overshadowing of widget
  names inside the main widget tree).
 */

// this passes control to the js startup function (see README in js/
// folder), then (after startup returns) triggers 'LoadComplete' event.
function cmp_cb_load_complete_4()
    {
    var lnk = pg_links(this.cmp.loader);
    if (lnk && lnk[0] && lnk[0].target)
	{
	var dname = pg_links(this.cmp.loader)[0].target;
	var startupname = "startup_" + dname;
	if (dname && window[startupname] && window['cmpd_init'])
	    {
	    // FF4 *NOT* obeying W3C async/defer setting to properly
	    // serialize script loading, and is parsing the last script
	    // before loading and parsing the other ones.  THUS we do
	    // this hack to make sure all scripts are parsed.
	    //
	    var scripts = document.getElementsByTagName('script');
	    var all_ready = true;
	    for(var i=0;i<scripts.length;i++)
		{
		var s = scripts[i];
		if (s && s.is_new)
		    {
		    if (!pg_scriptavailable(s))
			all_ready = false;
		    else
			s.is_new = false;
		    }
		}
	    if (all_ready)
		{
		window[startupname]();
		this.cmp.ifcProbe(ifEvent).Activate('LoadComplete', {});
		if (this.cmp.components.length)
		    this.cmp.components[this.cmp.components.length-1].cmp.HandleLoadComplete();
		return;
		}
	    }
	}

    // argh, the DOM isn't ready yet from the xfer of elements.
    this.try_cnt++;
    if (this.try_cnt > 100)
	{
	// after 5 seconds, lose all hope.
	alert("Failed to load component for " + wgtrGetName(this));
	}
    else
	{
	// try again in 0.05 seconds
	pg_addsched_fn(this, 'cmp_cb_load_complete_4', [], 50);
	}
    }

//this function simply moves all scripts and stylesheets into <head>   //SETH: ?? why are the scripts and stylesheets being moved to the head?
function cmp_cb_load_complete_3()
    {
    var larr = htr_get_layers(this.cmp.loader2);
    this._oarr = [];
    for(var i=0;i<larr.length;i++)
	{
	if (larr[i].id != '_firebugConsole')
	    {
	    this.cmp.loader.parentNode.appendChild(larr[i]);
	    this._oarr.push(larr[i]);
	    }
	}
    //this.cmp.larr = larr;
    var scripts = document.getElementsByTagName('script');
    var css = document.getElementsByTagName('style');
    var head = document.getElementsByTagName('head')[0];
    var curscripts = [];
    var scripts_to_move = [];
    var css_to_move = [];
    for(var i=0;i<scripts.length;i++)
	{
	if (scripts[i].parentNode != head) 
	    scripts_to_move.push(scripts[i]);
	else if (scripts[i].src)
	    curscripts[scripts[i].src] = true;
	}
    for(var i=0;i<scripts_to_move.length;i++)
	{
	var oldscript = scripts_to_move[i];
	var oldsrc = oldscript.src;
	//var oldsrc_regex = /([a-z]*:\/\/[^\/])(.*)/;
	//var matches = oldsrc_regex.exec(oldsrc);
	//if (matches && matches[1] && matches[2]) oldsrc = matches[2];
	//if (!curscripts[oldsrc])
	if (!oldscript.src || !pg_scriptavailable(oldscript))
	    {
	    var newscript = document.createElement('script');
	    newscript.async = (oldscript.text)?true:false; // for FF4
	    newscript.defer = (oldscript.text)?true:false; // for FF4
	    if (oldscript.src) newscript.src = oldscript.src;
	    if (oldscript.type) newscript.type = oldscript.type;
	    if (oldscript.language) newscript.language = oldscript.language;
	    if (oldscript.text) newscript.text = oldscript.text;
	    newscript.is_new = true;
	    head.appendChild(newscript);
	    this._oarr.push(newscript);
	    }
	}
    for(var i=0;i<css.length;i++)
	{
	if (css[i].parentNode != head) 
	    css_to_move.push(css[i]);
	}
    for(var i=0;i<css_to_move.length;i++)
	{
	head.appendChild(css_to_move[i]);
	this._oarr.push(css_to_move[i]);
	}
    this.cmp.try_cnt = 0;
    pg_addsched_fn(this.cmp, 'cmp_cb_load_complete_4', [], 30);
    }

function cmp_cb_load_complete_2()
    {
    pg_addsched_fn(this.cmp, 'cmp_cb_load_complete_3', [], 30);
    }

// this passes control to the js startup function (see README in js/
// folder), then (after startup returns) triggers 'LoadComplete' event.
function cmp_cb_load_complete_2ns()
    {
    //htr_alert(document.layers,1);
    var lnks = pg_links(this.loader);
    var dname = lnks[0].target;
    this.loader['startup_' + dname]();
    this.cmp.ifcProbe(ifEvent).Activate('LoadComplete', {});
    }

//this is the heart of this file. this function is what moves the
//loaded component (in the iframe) into the main tree.
function cmp_cb_load_complete()
    {
    if (cx__capabilities.Dom0NS)
	{
	var larr = pg_layers(this.cmp.loader);
	var layers_to_move = [];
	for(var i=0;i<larr.length;i++)
	    {
	    layers_to_move.push(larr[i]);
	    }
	for(var i=0;i<layers_to_move.length;i++)
	    {
	    moveAbove(layers_to_move[i], this.cmp.loader);
	    if (!this.cmp.loader.parentLayer.document.layers[layers_to_move[i].id])
		this.cmp.loader.parentLayer.document.layers[layers_to_move[i].id] = layers_to_move[i];
	    this.cmp._oarr.push(layers_to_move[i]);
	    }
	pg_addsched_fn(this.cmp, 'cmp_cb_load_complete_2ns', [], 300);
	}
    else
	{
	//pg_serialized_write(this.cmp.loader2, this.contentWindow.document.getElementsByTagName("html")[0].innerHTML, cmp_cb_load_complete_2);
	this.cmp.loader2.innerHTML = this.contentWindow.document.getElementsByTagName("html")[0].innerHTML;
	//while(this.cmp.loader2.firstChild) this.cmp.loader2.removeChild(this.cmp.loader2.firstChild);
	//this.cmp.loader2.appendChild(this.contentWindow.document.getElementsByTagName("html")[0]);
	this.cmp.cmp_cb_load_complete_2();
	}
    }


function cmp_instantiate(aparam)
    {
    //this function cycles through all contained components (?)
    if (this.components.length > 0 && !this.allow_multi)
	{
	if (this.auto_destroy)
	    {
	    this.ifcProbe(ifAction).Invoke('Destroy', {_inst_aparam:aparam});
	    return true;
	    }
	return false;
	}
    if (this.is_static)
	return false;
    var p = wgtrGetParentContainer(this);
    var w,h;
    if (this.orig_h > 0 && this.orig_w > 0)
	{
	w = this.orig_w;
	h = this.orig_h;
	}
    else if (this.is_toplevel)
	{
	w = pg_width;
	h = pg_height;
	}
    else
	{
	w = getWidth(p);
	h = getHeight(p);
	}
    var geom = tohex16(w) + tohex16(h) + tohex16(pg_charw) + tohex16(pg_charh) + tohex16(pg_parah);
    var graft = wgtrGetNamespace(this) + ':' + wgtrGetName(this);
    if (aparam.Path && aparam.Path[0] == '/' && aparam.Path[1] != '/')
	var path = aparam.Path;
    else
	var path = this.path;
    var url = path + "?cx__geom=" + encodeURIComponent(geom) + "&cx__graft=" + encodeURIComponent(graft) + "&cx__akey=" + encodeURIComponent(akey);
    if (this.orig_x != 0 || this.orig_y != 0)
	{
	url += "&cx__xoffset=" + encodeURIComponent(this.orig_x) + "&cx__yoffset=" + encodeURIComponent(this.orig_y);
	}

    if (this.templates.length > 0)
	{
	url += "&cx__templates=";
	var cnt = 0;
	for(var i=0;i<this.templates.length;i++)
	    {
	    if (this.templates[i].indexOf("|") == -1)
		{
		if (cnt != 0) url += "|";
		url += htutil_escape(this.templates[i]);
		cnt++
		}
	    }
	}

    // Get params supplied in the app first
    for(var pr in this.Params)
	{
	if (typeof aparam[pr] == 'undefined')
	    {
	    var v = this.Params[pr];
	    var r = wgtrCheckReference(v);
	    if (r) v = r;
	    if (url.lastIndexOf('?') > url.lastIndexOf('/'))
		url += '&';
	    else
		url += '?';
	    url += (htutil_escape(pr) + '=' + htutil_escape(v));
	    }
	}

    // Second, get params supplied in the connector (overrides app params)
    for(var pr in aparam)
	{
	var v = aparam[pr];
	var r = wgtrCheckReference(v);
	if (r) v = r;
	if (url.lastIndexOf('?') > url.lastIndexOf('/'))
	    url += '&';
	else
	    url += '?';
	if (v !== null)
	    url += (htutil_escape(pr) + '=' + htutil_escape(v));
	}

    // Let server know what scripts we have loaded already from /sys/js
    var scripts = document.getElementsByTagName("script");
    var regex = /([a-z]*:\/\/[^\/]*)(\/sys\/js\/[^\/]*\.js)/
    var script_list = "";
    for(var i=0; i<scripts.length; i++)
	{
	var matches = regex.exec(scripts[i].src);
	if (matches && matches[2])
	    script_list += matches[2] + ",";
	}
    if (script_list)
	{
	if (url.lastIndexOf('?') > url.lastIndexOf('/'))
	    url += '&';
	else
	    url += '?';
	url += "cx__scripts=" + htutil_escape(script_list);
	}

    this._graft = graft;
    this._url = url;
    this._geom = geom;
    this._name = aparam.Name;
    if (cx__capabilities.Dom0NS)
	{
	moveAbove(this.loader, this.stublayer);
	this.loader = htr_new_loader(p);
	this.loader.cmp = this;
	htr_setvisibility(this.loader, 'hidden');
	htr_init_layer(this.loader,this,'cmp');
	}
    pg_addsched_fn(this.cmp, 'cmp_instantiate_2', [url], 100);
    return true;
    }

function cmp_instantiate_2(url)
    {
    pg_serialized_load(this.loader, url, cmp_cb_load_complete);
    return true;
    }

function cmp_destroy(aparam)
    {
    if (this.is_static)
	return false;
    var cmps_to_destroy = [];
    for(var i in this.components)
	{
	var cmp = this.components[i];
	if (aparam.Namespace == cmp.cmpnamespace || aparam.Name == cmp.name || (!aparam.Namespace && !aparam.Name))
	    cmps_to_destroy.push(cmp);
	}
    var onecmp = cmps_to_destroy.shift();
    if (onecmp)
	onecmp.cmp.HandleReveal("ObscureCheck", {destroyctx:true, inst_aparam:aparam._inst_aparam, cmplist:cmps_to_destroy, donelist:[onecmp], curcmp:onecmp, cmpobj:this});
    }


function cmp_destroy_2(ctx)
    {
    var cmps_to_destroy = ctx.cmplist;
    onecmp = cmps_to_destroy.shift();
    if (onecmp)
	{
	ctx.curcmp = onecmp;
	ctx.donelist.push(onecmp);
	onecmp.cmp.HandleReveal("ObscureCheck", ctx);
	return;
	}

    // Ok, done with obscurecheck.  Now do obscure.
    for(var i in ctx.donelist)
	{
	ctx.donelist[i].cmp.HandleReveal("Obscure", ctx);
	}

    // Potentially relocate resources that we don't need to be destroying
    pg_reclaim_objects();

    // Shutdown the widgets
    for(var i in ctx.donelist)
	{
	wgtrDeinitTree(ctx.donelist[i].cmp);
	wgtrRemoveNamespace(ctx.donelist[i].cmpnamespace);
	}

    // Move everything out of the main workspace.
    // Hopefully GC is smart enough to clean it up.
    for(var i in ctx.donelist)
	{
	var cmp = ctx.donelist[i];

	//htr_alert(this,1);
	if (cx__capabilities.Dom1HTML)
	    {
	    for (var j = 0; j<cmp.objects.length;j++)
		{
		if (cmp.objects[j].tagName == 'DIV' && cmp.objects[j].id != '_firebugConsole')
		    {
		    moveAbove(cmp.objects[j], this.stublayer);
		    }
		else
		    {
		    var oldChild = cmp.objects[j].parentNode.removeChild(cmp.objects[j]);
		    delete oldChild;
		    delete cmp.objects[j];
		    }
		}
	    }
	else if (cx__capabilities.Dom0NS)
	    {
	    // For NS4, all objects are Layers.
	    for (var j = 0; j<cmp.objects.length;j++)
		{
		moveAbove(cmp.objects[j], this.stublayer);
		}
	    }

	// Remove it from the list
	for(var j in this.components)
	    {
	    if (cmp == this.components[j])
		{
		this.components.splice(j, 1);
		break;
		}
	    }
	}

    // Go go gadget garbage collector!
    if (cx__capabilities.Dom0NS)
	{
	this.stublayer.parentLayer.cmp = this;
	this.stublayer.parentLayer.cmp_destroy_3 = cmp_destroy_3;
	pg_serialized_write(this.stublayer.parentLayer, "<html><body></body></html>", cmp_destroy_3);
	}
    else if (cx__capabilities.Dom1HTML)
	{
	this.stublayer.parentNode.cmp = this;
	this.stublayer.parentNode.cmp_destroy_3 = cmp_destroy_3;
	pg_serialized_write(this.stublayer.parentNode, "<html><body></body></html>", cmp_destroy_3);
	}

    if (ctx.inst_aparam)
	this.ifcProbe(ifAction).Invoke('Instantiate', ctx.inst_aparam);

    return true;
    }

function cmp_destroy_3()
    {
    this.cmp.stublayer = htr_new_layer(pg_width, this);
    htr_setvisibility(this.cmp.stublayer, 'hidden');
    }

function cmp_register_proptarget(c, t)
    {
    for(var i in this.components)
	{
	if (this.components[i].cmp == c)
	    this.components[i].proptarget = t;
	}
    }

function cmp_register_component(c)
    {
    var newcmp = {cmp:c, cmpnamespace:wgtrGetNamespace(c), cmpname:wgtrGetName(c), objects:this._oarr, graft:this._graft, geom:this._geom, url:this._url, name:this._name, events:[], actions:[], proptarget:null, propwatched:{} };
    this.components.push(newcmp);
    if (this.is_visible)
	pg_addsched_fn(newcmp.cmp, 'HandleReveal', ['Reveal', null], 0);
    if (this.is_static && this.init_bh_finished)
	pg_addsched_fn(newcmp.cmp, 'HandleLoadComplete', [], 0);
    this._oarr = [];
    }

function cmp_cb_reveal(e)
    {
    var rval = true;
    switch(e.eventName)
	{
	case 'Reveal':
	    this.is_visible = true;
	    for(var c in this.components)
		{
		var cmp = this.components[c];
		if (cmp && cmp.cmp && cmp.cmp.HandleReveal)
		    rval = (rval && cmp.cmp.HandleReveal(e.eventName, e));
		}
	    break;
	case 'Obscure':
	    this.is_visible = false;
	    for(var c in this.components)
		{
		var cmp = this.components[c];
		if (cmp && cmp.cmp && cmp.cmp.HandleReveal)
		    rval = (rval && cmp.cmp.HandleReveal(e.eventName, e));
		}
	    break;
	case 'RevealCheck':
	case 'ObscureCheck':
	    e.cmp_revealcmps = [];
	    for(var c in this.components)
		{
		var cmp = this.components[c];
		if (cmp && cmp.cmp && cmp.cmp.HandleReveal)
		    e.cmp_revealcmps.push(cmp);
		}
	    if (e.cmp_revealcmps.length == 0)
		pg_reveal_check_ok(e);
	    else
		rval = e.cmp_revealcmps[0].cmp.HandleReveal(e.eventName, e);
	    break;
	}
    return rval;
    }


// Called from the embedded component itself
function cmp_cb_handle_reveal(e,ctx)
    {
    switch(e)
	{
	case 'RevealOK':
	case 'ObscureOK':
	    if (e == 'ObscureOK' && ctx && ctx.destroyctx)
		{
		return this.cmp_destroy_2(ctx);
		}
	    ctx.cmp_revealcmps.shift();
	    if (ctx.cmp_revealcmps.length == 0)
		return pg_reveal_check_ok(ctx);
	    else
		return ctx.cmp_revealcmps[0].cmp.HandleReveal(ctx.eventName, ctx);
	case 'RevealFailed':
	case 'ObscureFailed':
	    if (e.eventName == 'ObscureFailed' && ctx && ctx.destroyctx)
		return true;
	    return pg_reveal_check_veto(ctx);
	}
    return true;
    }


function cmp_add_param(name, value)
    {
    this.Params[name] = htutil_unpack(value);
    }


function cmp_add_action(cmp, actionname)
    {
    //var re = /^[A-Za-z_][A-Za-z0-9_]*$/;
    //if (!re.test(actionname)) return false;
    for(var i=0;i<this.components.length;i++)
	if (this.components[i].cmp == cmp)
	    this.components[i].actions.push(actionname);
    if (!this.ifcProbe(ifAction).Exists(actionname))
	//this.ifcProbe(ifAction).Add(actionname, new Function("aparam", "return this.cmp_action(\"" + actionname + "\", aparam);"));
	this.ifcProbe(ifAction).Add(actionname, function (aparam) { return this.cmp_action(actionname, aparam); } );
    }


function cmp_action(aname, aparam)
    {
    var rval = null;
    for(var i=0;i<this.components.length;i++)
	{
	for(var j=0;j<this.components[i].actions.length;j++)
	    {
	    if (this.components[i].actions[j] == aname)
		{
		//htr_alert(aparam,1);
		rval = this.components[i].cmp.ifcProbe(ifEvent).Activate(aname, aparam);
		break;
		}
	    }
	if (isCancel(rval)) break;
	}
    return rval;
    }


function cmp_add_event(cmp, eventname)
    {
    var re = /^[A-Za-z_][A-Za-z0-9_]*$/;
    if (!re.test(eventname)) return false;
    for(var i=0;i<this.components.length;i++)
	if (this.components[i].cmp == cmp)
	    this.components[i].events.push(eventname);
    if (!this.ifcProbe(ifEvent).Exists(eventname))
	this.ifcProbe(ifEvent).Add(eventname);
    }


function cmp_deinit()
    {
    if (this.is_static)
	{
	for(var i in this.components)
	    {
	    wgtrDeinitTree(this.components[i].cmp);
	    }
	}
    else
	{
	this.ifcProbe(ifAction).Invoke('Destroy', {_inst_aparam:null});
	}
    }


function cmp_get_geom()
    {
    return {x:this.orig_x?this.orig_x:0, y:this.orig_y?this.orig_y:0, width:this.orig_w?this.orig_w:0, height:this.orig_h?this.orig_h:0 };
    }


function cmp_value_changing(obj, prop, ov, nv)
    {
    for(var i in this.components)
	{
	var cmp = this.components[i];
	if ((cmp.proptarget && obj == cmp.proptarget) || wgtrGetRoot(obj) == cmp.cmp)
	    {
	    var realprop = prop;
	    if (wgtrGetType(obj) == 'widget/parameter')
		realprop = wgtrGetName(obj);
	    if (cmp.propwatched[realprop])
		{
		return this.ifcProbe(ifValue).Changing(realprop, nv, false, ov, true);
		}
	    }
	}
    return nv;
    }

function cmp_endinit(c)
    {
    return;
    }

function cmp_value_getter(n)
    {
    var v;

    for(var i in this.components)
	{
	var cmp = this.components[i];

	// try property target first, if one.
	if (cmp.proptarget)
	    {
	    v = wgtrProbeProperty(cmp.proptarget, n);
	    if (!wgtrIsUndefined(v))
		{
		if (!(cmp.propwatched[n]))
		    {
		    // if someone asked for the property, assume they might watch it.
		    cmp.propwatched[n] = true;
		    if (cmp.proptarget.ifcProbe(ifValue))
			cmp.proptarget.ifcProbe(ifValue).Watch(n, this, cmp_value_changing);
		    else	
			htr_cwatch(cmp.proptarget, n, this, 'cmp_value_changing');
		    }
		return v;
		}
	    }

	// try param on the cmp
	v = wgtrProbeProperty(cmp.cmp, n);
	if (!wgtrIsUndefined(v))
	    {
	    if (!(cmp.propwatched[n]))
		{
		cmp.propwatched[n] = true;
		wgtrWatchProperty(cmp.cmp, n, this, 'cmp_value_changing');
		}
	    return v;
	    }
	}

    alert("Property " + n + " is undefined for component " + wgtrGetName(this));
    return null;
    }

function cmp_value_setter(n, v)
    {
    var oldv;

    for(var i in this.components)
	{
	var cmp = this.components[i];
	if (cmp.proptarget)
	    {
	    oldv = wgtrProbeProperty(cmp.proptarget, n);
	    if (!wgtrIsUndefined(oldv))
		return wgtrSetProperty(cmp.proptarget, n, v);
	    }

	oldv = wgtrProbeProperty(cmp.cmp, n);
	if (!wgtrIsUndefined(oldv))
	    return wgtrSetProperty(cmp.cmp, n, v);
	}
    }


//'this' should be the actual iframe/layer DOM element.
function cmp_init(param)
    {
    var node = param.node;
    node.destroy_widget = cmp_deinit;
    node.is_static = param.is_static;
    node.is_toplevel = param.is_top;
    node.allow_multi = param.allow_multi;
    node.auto_destroy = param.auto_destroy;
    node.orig_h = param.height;
    node.orig_w = param.width;
    node.orig_x = param.xpos;
    node.orig_y = param.ypos;
    node.cmp = node;
    node._oarr = [];
    node.components = [];
    node.templates = [];
    if (!param.is_static)
	{
	node.loader = param.loader;
	node.loader.cmp = node;
	node.path = param.path;
	htr_init_layer(node.loader,node,'cmp');
	node.loader.style.display = 'none';
	node.stublayer = htr_new_layer(pg_width);
	node.stublayer.style.display = 'none';
	htr_setvisibility(node.stublayer, 'hidden');
	node.stublayer = htr_new_layer(pg_width, node.stublayer);
	node.stublayer.style.display = 'none';
	htr_setvisibility(node.stublayer, 'hidden');

	if (cx__capabilities.Dom1HTML)
	    {
	    node.loader2 = htr_new_layer(pg_width, node.loader.parentNode);
	    node.loader2.style.display = 'none';
	    node.loader2.cmp = node;
	    htr_setvisibility(node.loader2, 'hidden');
	    }
	}
    node.is_visible = false;
    node.cmp_cb_load_complete = cmp_cb_load_complete;
    node.cmp_cb_load_complete_2 = cmp_cb_load_complete_2;
    node.cmp_cb_load_complete_2ns = cmp_cb_load_complete_2ns;
    node.cmp_cb_load_complete_3 = cmp_cb_load_complete_3;
    node.cmp_cb_load_complete_4 = cmp_cb_load_complete_4;
    node.cmp_action = cmp_action;

    node.cmp_instantiate_2 = cmp_instantiate_2;
    node.cmp_destroy_2 = cmp_destroy_2;

    node.cmp_value_changing = cmp_value_changing;

    ifc_init_widget(node);

    node.RegisterComponent = cmp_register_component;
    node.AddParam = cmp_add_param;
    node.Params = [];
    node.getGeom = cmp_get_geom;
    node.registerPropTarget = cmp_register_proptarget;
    node.InitBH = cmp_init_bh;

    // Obscure/Reveal
    node.Reveal = cmp_cb_reveal; // from other widgets in same NS
    node.HandleReveal = cmp_cb_handle_reveal; // from component
    if (pg_reveal_register_listener(node))
	{
	node.is_visible = true;
	}

    // Event/Action handling for components instantiated inside this one
    node.AddAction = cmp_add_action;
    node.AddEvent = cmp_add_event;

    // Events
    var ie = node.ifcProbeAdd(ifEvent);
    ie.EnableLateConnectBinding();
    ie.Add("LoadComplete");

    // Actions
    var ia = node.ifcProbeAdd(ifAction);
    ia.Add("Instantiate", cmp_instantiate);
    ia.Add("Destroy", cmp_destroy);

    // values
    var iv = node.ifcProbeAdd(ifValue);
    iv.SetNonexistentCallback(cmp_value_getter, cmp_value_setter);

    // Finish initialization...
    pg_addsched_fn(node, "InitBH", [], 0);

    return node;
    }

function cmp_init_bh()
    {
    if (this.is_static && this.components[0])
	this.components[0].cmp.HandleLoadComplete();
    this.init_bh_finished = true;
    }


// Load indication
if (window.pg_scripts) pg_scripts['htdrv_component.js'] = true;
