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

function cmp_cb_load_complete_4()
    {
    var dname = pg_links(this.cmp.loader)[0].target;
    var startupname = "startup_" + dname;
    window[startupname]();
    this.cmp.ifcProbe(ifEvent).Activate('LoadComplete', {});
    }

function cmp_cb_load_complete_3()
    {
    var larr = htr_get_layers(this.cmp.loader2);
    this._oarr = new Array();
    for(var i=0;i<larr.length;i++)
	{
	this.cmp.loader.parentNode.appendChild(larr[i]);
	this._oarr.push(larr[i]);
	}
    //this.cmp.larr = larr;
    var scripts = document.getElementsByTagName('script');
    var css = document.getElementsByTagName('style');
    var head = document.getElementsByTagName('head')[0];
    var scripts_to_move = new Array();
    var css_to_move = new Array();
    for(var i=0;i<scripts.length;i++)
	{
	if (scripts[i].parentNode != head) 
	    scripts_to_move.push(scripts[i]);
	}
    for(var i=0;i<scripts_to_move.length;i++)
	{
	var newscript = document.createElement('script');
	var oldscript = scripts_to_move[i];
	if (oldscript.src) newscript.src = oldscript.src;
	if (oldscript.type) newscript.type = oldscript.type;
	if (oldscript.language) newscript.language = oldscript.language;
	if (oldscript.text) newscript.text = oldscript.text;
	head.appendChild(newscript);
	this._oarr.push(newscript);
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
    pg_addsched_fn(this.cmp, 'cmp_cb_load_complete_4', [], 200);
    }

function cmp_cb_load_complete_2()
    {
    pg_addsched_fn(this.cmp, 'cmp_cb_load_complete_3', [], 30);
    }

function cmp_cb_load_complete_2ns()
    {
    //htr_alert(document.layers,1);
    var lnks = pg_links(this.loader);
    var dname = lnks[0].target;
    this.loader['startup_' + dname]();
    this.cmp.ifcProbe(ifEvent).Activate('LoadComplete', {});
    }

function cmp_cb_load_complete()
    {
    if (cx__capabilities.Dom0NS)
	{
	var larr = pg_layers(this.cmp.loader);
	var layers_to_move = new Array();
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
	pg_serialized_write(this.cmp.loader2, this.contentWindow.document.getElementsByTagName("html")[0].innerHTML, cmp_cb_load_complete_2);
	}
    }


function cmp_instantiate(aparam)
    {
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
    var p = wgtrGetContainer(wgtrGetParent(this));
    var geom;
    if (this.is_toplevel)
	geom = tohex16(pg_width) + tohex16(pg_height) + tohex16(pg_charw) + tohex16(pg_charh) + tohex16(pg_parah);
    else
	geom = tohex16(getWidth(p)) + tohex16(getHeight(p)) + tohex16(pg_charw) + tohex16(pg_charh) + tohex16(pg_parah);
    var graft = wgtrGetNamespace(this) + ':' + wgtrGetName(this);
    var url = this.path + "?cx__geom=" + escape(geom) + "&cx__graft=" + escape(graft) + "&cx__akey=" + escape(akey);

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
	url += (htutil_escape(pr) + '=' + htutil_escape(v));
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
    var cmps_to_destroy = new Array();
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
		if (cmp.objects[j].tagName == 'DIV')
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

function cmp_register_component(c)
    {
    var newcmp = {cmp:c, cmpnamespace:wgtrGetNamespace(c), cmpname:wgtrGetName(c), objects:this._oarr, graft:this._graft, geom:this._geom, url:this._url, name:this._name, events:[], actions:[]};
    this.components.push(newcmp);
    if (this.is_visible)
	pg_addsched_fn(newcmp.cmp, 'HandleReveal', ['Reveal', null], 0);
    this._oarr = new Array();
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
	    e.cmp_revealcmps = new Array();
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
		return ctx.cmp_revealcmps[0].cmp.HandleReveal(e, ctx);
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
    var re = /^[A-Za-z_][A-Za-z0-9_]*$/;
    if (!re.test(actionname)) return false;
    for(var i=0;i<this.components.length;i++)
	if (this.components[i].cmp == cmp)
	    this.components[i].actions.push(actionname);
    if (!this.ifcProbe(ifAction).Exists(actionname))
	this.ifcProbe(ifAction).Add(actionname, new Function("aparam", "this.cmp_action(\"" + actionname + "\", aparam);"));
    }


function cmp_action(aname, aparam)
    {
    for(var i=0;i<this.components.length;i++)
	{
	for(var j=0;j<this.components[i].actions.length;j++)
	    {
	    if (this.components[i].actions[j] == aname)
		{
		//htr_alert(aparam,1);
		this.components[i].cmp.ifcProbe(ifEvent).Activate(aname, aparam);
		break;
		}
	    }
	}
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


function cmp_init(param)
    {
    var node = param.node;
    node.destroy_widget = cmp_deinit;
    node.is_static = param.is_static;
    node.is_toplevel = param.is_top;
    node.allow_multi = param.allow_multi;
    node.auto_destroy = param.auto_destroy;
    node.cmp = node;
    node._oarr = new Array();
    node.components = new Array();
    node.templates = new Array();
    if (!param.is_static)
	{
	node.loader = param.loader;
	node.loader.cmp = node;
	node.path = param.path;
	htr_init_layer(node.loader,node,'cmp');
	node.stublayer = htr_new_layer(pg_width);
	htr_setvisibility(node.stublayer, 'hidden');
	node.stublayer = htr_new_layer(pg_width, node.stublayer);
	htr_setvisibility(node.stublayer, 'hidden');

	if (cx__capabilities.Dom1HTML)
	    {
	    node.loader2 = htr_new_layer(pg_width, node.loader.parentNode);
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
    ifc_init_widget(node);

    node.RegisterComponent = cmp_register_component;
    node.AddParam = cmp_add_param;
    node.Params = new Array();

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

    return node;
    }
